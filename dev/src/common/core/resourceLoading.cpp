/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "diskFile.h"
#include "resource.h"
#include "resourceLoading.h"
#include "depot.h"
#include "dependencyCache.h"
#include "fileSystemProfilerWrapper.h"
#include "asyncFileAccess.h"
#include "ioTags.h"
#include "configVar.h"

namespace Config
{
	TConfigVar< Int32, Validation::IntRange<0, 4096> >		cvFileQueueSizeGame( "ResourceLoading", "FileQueueSizeGame", 200 );
	TConfigVar< Int32, Validation::IntRange<0, 4096> >		cvFileQueueSizeLoading( "ResourceLoading", "FileQueueSizeLoading", 200 );
	TConfigVar< Int32, Validation::IntRange<0, 4096> >		cvAsyncFileCountTreshold( "ResourceLoading", "AsyncLoadingFileCountTreshold", 1 );
	TConfigVar< Bool >										cvUseAsyncLoading( "ResourceLoading", "UseAsyncLoading", true );
	TConfigVar< Bool >										cvUseDependencyCache( "ResourceLoading", "UseDependencyCache", true );
}
//----

ResourceLoadingContext::ResourceLoadingContext()
	: m_isAsyncLoader( false )
	, m_isImportDependency( false )
	, m_isPrefetchFile( false )
	, m_stats( nullptr )
	, m_customFileStream( nullptr )
	, m_priority( eResourceLoadingPriority_Normal )
	, m_importLoader( nullptr )
{}

//---

ResourceLoadingTask::ResourceLoadingTask( CDiskFile* diskFile )
	: m_diskFile( diskFile )
{
}

ResourceLoadingTask::~ResourceLoadingTask()
{
}

//---

CResourceLoader::CResourceLoader()
	: m_isLoadingMode( false )
{
}

CResourceLoader::~CResourceLoader()
{
}

void CResourceLoader::ToggleLoadingMode( const Bool isInLoadingMode )
{
	if ( m_isLoadingMode != isInLoadingMode )
	{
		m_isLoadingMode = isInLoadingMode;
		if ( isInLoadingMode )
		{
			LOG_CORE( TXT("Loading mode: ON - expect increased memory usage for loading stuff") );
		}
		else
		{
			LOG_CORE( TXT("Loading mode: OFF") );
		}
	}
}

Uint8 CResourceLoader::ResolveIOTag( const EResourceLoadingPriority priority )
{
	switch ( priority )
	{
		case eResourceLoadingPriority_Low: 
			return eIOTag_ResourceLow;

		case eResourceLoadingPriority_High:
			return eIOTag_ResourceImmediate;
	}

	return eIOTag_ResourceNormal;
}

namespace Helper
{
	static String GetBatchName( const Uint32 numfiles )
	{
		static Uint32 batchIndex = 0;
		return String::Printf( TXT("Batch %d (%d files)"), batchIndex++, numfiles );
	}
}

void CResourceLoader::Load( CDiskFile* rootFile, CDiskFile** files, Uint32 numFiles, EResourceLoadingPriority priority, class IDependencyImportLoader* dependencyLoader )
{
#ifdef RED_PROFILE_FILE_SYSTEM
	const String profileName = rootFile ? rootFile->GetDepotPath() : Helper::GetBatchName( numFiles );
	RedIOProfiler::ProfileDiskFileLoadResourceStart( profileName.AsChar() );
#endif

	// Prepare a merged dependency list of the files to load
	TDynArray< CDiskFile* > loadingList;
	PrepareDependencies( files, numFiles, loadingList );

	// Paranoid check
	RED_FATAL_ASSERT( !rootFile || (loadingList.Back() == rootFile), "Invalid dependency list for '%ls' - root file is not reported", rootFile->GetDepotPath().AsChar() );

	// TODO: analyze and optimize file loading queue placement
	// Mostly: try to load all tiny file first (all textures), finish with the root resources
	// Try to glue files from the same bundles together
	LoadManyAsync( rootFile, priority, loadingList.TypedData(), loadingList.Size(), dependencyLoader );

#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileDiskFileLoadResourceEnd( profileName.AsChar() );
#endif
}

void CResourceLoader::PrepareDependencies( CDiskFile** rootFiles, const Uint32 numRootFiles, TDynArray< CDiskFile* >& outLoadingFiles ) const
{
	// collect file dependencies
	CDependencyCollector* collector = GDepot->GetDependencyCache().AllocateCollector();

	// collect dependencies from ALL files, in order - this will create a merged list
	for ( Uint32 i=0; i<numRootFiles; ++i )
	{
		CDiskFile* file = rootFiles[i];

		// if we have valid dependency cache ID try to get the dependencies
		const Uint32 depRootIndex = file->GetDepCacheIndex();
		if ( depRootIndex == 0 || !collector )
		{
			// no dependency information for this file - load it directly
			outLoadingFiles.PushBack( file );
			continue;
		}

		// collect ALL dependencies of given resource
		GDepot->GetDependencyCache().CollectDependencies( *collector, depRootIndex, true );
	}

	// process the files for which we collected the dependencies
	if ( collector )
	{
		// allocate the new task list for the dependencies
		const Uint32 numDependencies = collector->Size();
		outLoadingFiles.Reserve( outLoadingFiles.Size() + numDependencies );

		// prepare the loading tasks
		for ( Uint32 i=0; i<numDependencies; ++i )
		{
			const Uint32 depFileIndex = collector->GetFileIndex( i );
			if ( !collector->IsExcluded( depFileIndex ) )
			{
				// lookup the matching disk file
				CDiskFile* diskFile = GDepot->GetMappedDiskFile( depFileIndex );
				if ( diskFile && !diskFile->IsLoaded() && !diskFile->IsFailed() )
				{
					outLoadingFiles.PushBack( diskFile );
				}
			}
		}

		// release the collector back to the global pool
		collector->Release();
	}
}

void CResourceLoader::LoadManyAsync( CDiskFile* rootFile, const EResourceLoadingPriority priority, CDiskFile** loadingList, const Uint32 numLoadingListFiles, class IDependencyImportLoader* dependencyLoader )
{
	PC_SCOPE( LoadManyAsync );

	// determine the limits to use
	const Uint32 queueFileLimit = Max<Uint32>( 1, m_isLoadingMode ? Config::cvFileQueueSizeLoading.Get() : Config::cvFileQueueSizeGame.Get() );

	// process the file queue
	Uint32 numLeftToLoad = numLoadingListFiles;
	Uint32 currentPreload = 0;

	// preload queue
	TDynArray< AsyncFileInfo > preloadQueue;
	preloadQueue.Reserve( queueFileLimit );

	// resolve IO tag
	// TODO: we may use more fancy scheme here
	const auto ioTag = ResolveIOTag( priority );

	// NOTE: right now the queue is LINEAR, we assume that the dependency cache is working and the order of files is optimal
	// TODO: we could achieve better loading performance if we allowed loading resources out of order to some extent

	// stats
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileVarAddPendingFiles( numLoadingListFiles );
#endif

	// process tasks
	while ( numLeftToLoad > 0 )
	{
		/////////////////
		// PRELOADING 
		////////////////

		// add new stuff to the preloading queue if possible
		while ( preloadQueue.Size() < queueFileLimit && currentPreload < numLoadingListFiles )
		{
			// estimate the memory required for loading the next file in queue
			CDiskFile* preloadHead = loadingList[ currentPreload ];
			const Bool isRoot = preloadHead == rootFile;

			// is this file already loaded ?
			if ( preloadHead->IsLoaded() || preloadHead->IsFailed() )
			{
				numLeftToLoad -= 1;
				currentPreload += 1;
				continue;
			}

			// request an async reader to be created
			// NOTE this can fail for some reasons
			IAsyncFile* asyncFile = nullptr;
			const auto ret = preloadHead->CreateAsyncReader( ioTag, asyncFile );
			if ( ret == eAsyncReaderResult_Failed )
			{
				// fail the file
				ERR_CORE( TXT("Failed to create asynchronous preloading job for file '%ls'"), preloadHead->GetDepotPath().AsChar() );

				// report failure
				if ( isRoot || preloadHead->BeingLoading() )
				{
					preloadHead->InternalFailedLoading();
				}

				// advance loading
				currentPreload += 1;
				numLeftToLoad -= 1;
			}
			else if ( ret == eAsyncReaderResult_NotReady )
			{
				// we don't have resources yet
				// release some CPU time to allow other threads to continue
				Red::Threads::SleepOnCurrentThread(1);
				break;
			}
			else if ( ret == eAsyncReaderResult_OK )
			{
				// add to the preloading queue
				AsyncFileInfo info;
				info.m_asyncFile = asyncFile;
				info.m_depotFile = preloadHead;
				preloadQueue.PushBack(info);

				// try to preload next file
				currentPreload += 1;
			}
			else
			{
				RED_FATAL( "Invalid state" );
			}
		}

		/////////////////
		// PROCESSING
		////////////////

		// get the top resource in the queue
		if ( !preloadQueue.Empty() )
		{
			const AsyncFileInfo info = preloadQueue.Front();
			const Bool isRoot = info.m_depotFile == rootFile;

			// Check if we have the right reader
			IFile* reader = nullptr;
			const auto ret = info.m_asyncFile->GetReader( reader );
			if ( ret == IAsyncFile::eResult_Failed )
			{
				ERR_CORE( TXT("Failed to create file reader for file '%ls'"), info.m_depotFile->GetDepotPath().AsChar() );

				// report failure
				if ( isRoot || info.m_depotFile->BeingLoading() )
				{
					info.m_depotFile->InternalFailedLoading();
				}

				// file failed, but still, one less to go
				numLeftToLoad -= 1;

				// remove the entry from the queue
				preloadQueue.RemoveAt(0);

				// stats
#ifdef RED_PROFILE_FILE_SYSTEM
				RedIOProfiler::ProfileVarRemovePendingFiles( 1 );
#endif
			}
			else if ( ret == IAsyncFile::eResult_NotReady )
			{
				// data is not ready
				// release some CPU time to allow other threads to continue
				Red::Threads::SleepOnCurrentThread(1);
			}
			else if ( ret == IAsyncFile::eResult_OK )
			{
				// remove the entry from the queue
				preloadQueue.RemoveAt(0);

				// load data - atomic and done only once
				if ( isRoot || info.m_depotFile->BeingLoading() )
				{
					// deserialize data
					if ( reader )
					{
						// deserialize file from the loaded data
						SDiskFilePostLoadList postLoadList;
						info.m_depotFile->InternalDeserialize( reader, dependencyLoader, postLoadList );

						// post load only if serialization succeeded
						if ( !info.m_depotFile->IsFailed() )
						{
							info.m_depotFile->InternalPostLoad( postLoadList );
						}
					}
					else
					{
						// for some reason we cannot open the file and read the data (CRC error, missing file, etc)
						ERR_CORE( TXT("Failed to create file reader for file '%ls'"), info.m_depotFile->GetDepotPath().AsChar() );
						info.m_depotFile->InternalFailedLoading();
					}
				}

				// delete the reader
				delete reader;

				// release the async file
				info.m_asyncFile->Release();

				// file got loaded
				numLeftToLoad -= 1;

				// stats
#ifdef RED_PROFILE_FILE_SYSTEM
				RedIOProfiler::ProfileVarRemovePendingFiles( 1 );
#endif
			}
			else
			{
				RED_FATAL( "Invalid state" );
			}
		}
	}
}
