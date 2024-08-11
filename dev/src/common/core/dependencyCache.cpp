/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scopedPtr.h"
#include "fileSys.h"
#include "profiler.h"
#include "staticarray.h"
#include "dependencyCache.h"

#include "diskFile.h"
#include "depot.h"
#include "configVar.h"

namespace Config
{
	TConfigVar< Int32 >		cvMaxDependencyCacheFiles( "Depot", "MaxDependencyCacheFiles", 320000, eConsoleVarFlag_ReadOnly );
}

//---

CDependencyCollector::CDependencyCollector( CDependencyCache& cache )
	: m_nextFree( nullptr )
	, m_cache( &cache )
{
	// maximum file count
	const Uint32 maxFiles = Config::cvMaxDependencyCacheFiles.Get(); // EP1 + EP2 + all DCS + gold is estimated ~300k, add some margin
	RED_FATAL_ASSERT( cache.GetNumResources() < maxFiles, "To many files - increase the magic number" );

	// allocate the bit buffer for each file
	m_excludedFiles.Resize( maxFiles );
	m_visitedFiles.Resize( maxFiles );

	// allocate file list
	m_fileList.Reserve( maxFiles / 4 );
}

CDependencyCollector::~CDependencyCollector()
{
}

void CDependencyCollector::Release()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_cache->m_freeListLock );

	m_nextFree = m_cache->m_freeList;
	m_cache->m_freeList = this;
}

//---

CDependencyCache::CDependencyCache()
{
}

CDependencyCache::~CDependencyCache()
{
}

Bool GDumpDepCacheMerge = false;

Bool CDependencyCache::Load( const String& absoluteFilePath )
{
	// load
	{
		CTimeCounter timer;

		// open the source file in non buffered mode
		Red::TScopedPtr< IFile > reader( GFileManager->CreateFileReader( absoluteFilePath, FOF_AbsolutePath ) );
		if ( !reader )
		{
			ERR_CORE( TXT("Unable to open dependency cache from '%ls' - loading will be much slower"), absoluteFilePath.AsChar() );
			return false;
		}

		// magic number
		Uint32 magic, version;
		*reader << magic;
		*reader << version;

		// valid ?
		if ( magic != FILE_MAGIC || version != FILE_VERSION )
		{
			ERR_CORE( TXT("Deprecacted dependency cache '%ls' - loading will be much slower"), absoluteFilePath.AsChar() );
			return false;
		}

		// load resource map
		m_resources.SerializeBulk( *reader );

		// load entry data
		m_dependencyIndices.SerializeBulk( *reader );
		m_dependencyExcluded.SerializeBulk( *reader );
		m_dependencyBuffer.SerializeBulk( *reader );

		if ( absoluteFilePath.ContainsSubstring(TXT("bob") ) )
		{
			GDumpDepCacheMerge = true;
		}

		// stats
		static const Float oneMB = 1024.0f * 1024.0f;
		LOG_CORE( TXT("Dependency cache loaded in %1.2fs, %d entries, %d deps"), 
			timer.GetTimePeriod(), m_dependencyIndices.Size(), m_dependencyBuffer.Size() );
		LOG_CORE( TXT("Dependency cache: %1.2fMB, mapping: %1.2fMB"), 
			(m_dependencyIndices.DataSize() + m_dependencyBuffer.DataSize() + m_dependencyExcluded.DataSize()) / oneMB,
			(m_resources.DataSize()) / oneMB );			
	}

	// build local mapping (optional - will not be needed in cooked build)
	{
		CTimeCounter timer;
		
		if ( (m_resources.Size() + 1) > m_resourceMap.Size() )
			m_resourceMap.Reserve( m_resources.Size() + 1 );

		for ( Uint32 i=0; i<m_resources.Size(); ++i )
		{
			m_resourceMap.Set( m_resources[i], i );
		}

		// stats
		static const Float oneMB = 1024.0f * 1024.0f;
		LOG_CORE( TXT("Dependency mapping built in %1.2fs (%1.2fMB)"), 
			timer.GetTimePeriod(),
			(m_resourceMap.DataSize()) / oneMB );
	}

	// loaded
	return true;
}

const Uint32 CDependencyCache::GetNumResources() const
{
	return m_resources.Size();
}

const Uint32 CDependencyCache::MapResourceToIndex( const ResourceID& id ) const
{
	Uint32 index = 0;
	m_resourceMap.Find( id, index );
	return index;
}

const CDependencyCache::ResourceID CDependencyCache::MapIndexToResource( const Uint32 index ) const
{
	// invalid index
	if ( index == 0 || index >= m_resources.Size() )
		return ResourceID("");

	// get resource ID
	return m_resources[ index ];
}

CDependencyCollector* CDependencyCache::AllocateCollector() const
{
	CDependencyCollector* ret = nullptr;

	// get one from free list
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_freeListLock );

		if ( m_freeList )
		{
			ret = m_freeList;
			m_freeList = ret->m_nextFree;
			ret->m_nextFree = nullptr;
		}
	}

	// allocate new one
	if ( !ret )
	{
		ret = new CDependencyCollector( const_cast< CDependencyCache& >( *this ) );
	}

	// prepare the file list
	ret->m_fileList.ClearFast();

	// cleanup bits
	ret->m_excludedFiles.ClearAll();
	ret->m_visitedFiles.ClearAll();

	// return allocated collector
	return ret;
}

static Bool GDebugDependencies = false;

void CDependencyCache::CollectDependencies( class CDependencyCollector& outCollector, const Uint32 rootFileIndex, const Bool isRoot /*= true*/ ) const
{
	// make sure we are NOT accessing the dependency cache during resize
	m_resizeLock.AcquireReadShared();

	CollectDependencies_NoLock( outCollector, rootFileIndex, isRoot );

	m_resizeLock.ReleaseReadShared();
}

void CDependencyCache::CollectDependencies_NoLock( class CDependencyCollector& outCollector, const Uint32 rootFileIndex, const Bool isRoot /*= true*/ ) const
{
	// skip if already visited
	if ( outCollector.IsMarked( rootFileIndex ) )
		return;

	// NOTE: we DO NOT mark the file here as this may result in not optimal dependency list
	// instead we mark it at the deepest dependency level possible

	// debug stuff
	if ( GDebugDependencies )
	{
		const CDiskFile* rootFile = GDepot->GetMappedDiskFile(rootFileIndex);
		LOG_CORE( TXT("Rootfile %d: '%ls'"), rootFileIndex, rootFile ? rootFile->GetDepotPath().AsChar() : TXT("null") );
		RED_UNUSED( rootFile );
	}

	// exclude files from dependency check (inplace files)
	{
		const Uint32 excludeIndex = m_dependencyExcluded[ rootFileIndex ];
		if ( excludeIndex )
		{
			// process the buffer - a list of child dependencies
			const Uint32* depList = &m_dependencyBuffer[ excludeIndex ];

			Uint32 depLocalIndex = 0;
			while ( *depList )
			{
				const Uint32 fileIndex = *depList++;
				outCollector.Exclude( fileIndex );
			}
		}
	}

	// process the file dependencies first
	// TODO: consider unrolling, usually not worth it because the dependency depth is small (4-5 levels)
	const Uint32 depIndex = m_dependencyIndices[ rootFileIndex ];
	if ( depIndex )
	{
		// process the buffer - a list of child dependencies
		const Uint32* depList = &m_dependencyBuffer[ depIndex ];

		Uint32 depLocalIndex = 0;
		while ( *depList )
		{
			// debug stuff
			if ( GDebugDependencies )
			{
				const CDiskFile* depFile = GDepot->GetMappedDiskFile(*depList);
				LOG_CORE( TXT(" Dep[%d] of %d: (%d) '%ls'"), 
					depLocalIndex++, rootFileIndex, *depList, depFile ? depFile->GetDepotPath().AsChar() : TXT("null") );
				RED_UNUSED( depFile );
				RED_UNUSED( depLocalIndex );
			}

			// recurse to the lower level
			const Uint32 childDep = *depList;
			CollectDependencies_NoLock( outCollector, childDep, false );
			++depList;
		}
	}
	
	// add to the dependency list
	if ( !outCollector.IsMarked( rootFileIndex ) ) // may have been marked in the child recursion
	{
		outCollector.Mark( rootFileIndex );

		if ( !outCollector.IsExcluded( rootFileIndex ) )
		{
			outCollector.Add( rootFileIndex );
		}
	}
}

//---

void CDependencyCache::AppendDynamicCache( const CDependencyCache& cache )
{
	CTimeCounter timer;

	// lock cache for resize
	m_resizeLock.AcquireWriteExclusive();

	static const Uint32 MAX_DYNAMIC_FILES = 100000;
	typedef TStaticArray< Int32, MAX_DYNAMIC_FILES >  TFileMapping;

	// mapping incoming index -> current index
	TFileMapping globalFileMapping;
	TFileMapping incomingToNewMapping;

	const Uint32 numIncomingFiles = cache.m_resources.Size();
	globalFileMapping.Resize( numIncomingFiles );
	incomingToNewMapping.Resize( numIncomingFiles );

	// determine the mapping between existing dep cache and the dynamic one
	Uint32 numNewFiles = 0;
	for ( Uint32 i=0; i<numIncomingFiles; ++i )
	{
		const auto id = cache.m_resources[i]; // the ID of incoming resource

		// find it in current cache
		Uint32 existingIndex = 0;
		if ( m_resourceMap.Find( id, existingIndex ) )
		{
			globalFileMapping[i] = existingIndex;
			incomingToNewMapping[i] = -1; // NOT new
		}
		else
		{
			globalFileMapping[i] = m_resources.Size() + numNewFiles; // preallocate
			incomingToNewMapping[i] = numNewFiles; // preallocate
			numNewFiles += 1;
		}
	}

	// extend existing buffers
	const Uint32 baseExcluded = m_dependencyExcluded.Size();
	const Uint32 baseIndices = m_dependencyIndices.Size();
	const Uint32 baseDepIndex = m_dependencyBuffer.Size();
	m_dependencyExcluded.Resize( baseExcluded + numNewFiles );
	m_dependencyIndices.Resize( baseIndices + numNewFiles );
	m_dependencyBuffer.Resize( baseDepIndex + cache.m_dependencyBuffer.Size() );

	// new files
	LOG_CORE( TXT("Incoming dependency cache: %d files (%d new), %d indices, %d excluded, %d buffer entries"), 
		cache.m_resources.Size(), numNewFiles, 
		cache.m_dependencyIndices.Size(), cache.m_dependencyExcluded.Size(), cache.m_dependencyBuffer.Size() );

	// no new files - ignore the dep.cache (redundant data most likely)
	if ( numNewFiles != 0 )
	{
		// copy the NEW file ids
		const Uint32 baseResourceIndex = m_resources.Size();
		m_resourceMap.Reserve( m_resourceMap.Size() + numNewFiles );
		m_resources.Resize( baseResourceIndex + numNewFiles );
		for ( Uint32 i=0; i<numIncomingFiles; ++i )
		{
			const Uint32 newFileIndex = globalFileMapping[i];
			if ( newFileIndex >= baseResourceIndex )
			{
				const auto incomingFileID = cache.m_resources[ i ];
				m_resources[ newFileIndex ] = incomingFileID;
				m_resourceMap.Insert( incomingFileID, newFileIndex );
			}
		}

		// setup the remapped dependency buffer entries
		for ( Uint32 i=0; i<cache.m_dependencyBuffer.Size(); ++i )
		{
			m_dependencyBuffer[baseDepIndex + i] = globalFileMapping[ cache.m_dependencyBuffer[i] ];
		}

		// setup the new inclusion buffer indices
		for ( Uint32 i=0; i<cache.m_dependencyIndices.Size(); ++i )
		{
			const auto newEntryIndex = incomingToNewMapping[i];
			if ( newEntryIndex != -1 )
			{
				const auto index = cache.m_dependencyIndices[i];
				m_dependencyIndices[ baseIndices + newEntryIndex ] = index ? (baseDepIndex + index) : 0;
			}
			else
			{
				const auto index = cache.m_dependencyIndices[i];
				if ( index != 0 )
				{
					const auto existingFileIndex = globalFileMapping[ i ];
					RED_FATAL_ASSERT( existingFileIndex < (Int32)baseResourceIndex, "File not new" );
					m_dependencyIndices[ existingFileIndex ] = index + baseDepIndex;
				}
			}
		}

		// setup the new exclusion buffer indices
		Uint32 numReplacedExcludeChains = 0;
		Uint32 numAddedExcludeCachins = 0;
		for ( Uint32 i=0; i<cache.m_dependencyExcluded.Size(); ++i )
		{
			if ( GDumpDepCacheMerge )
			{
				auto index = cache.m_dependencyExcluded[i];
				const auto newEntryIndex = incomingToNewMapping[i];
				if ( index != 0 )
				{
					LOG_CORE( TXT("Exclude[%d] = %d (entry = %d)"), i, index, newEntryIndex );
					while ( index < (Int32) cache.m_dependencyBuffer.Size() )
					{
						const auto resIndex = cache.m_dependencyBuffer[index];
						LOG_CORE( TXT("   ID[%d] = %d"), index, resIndex );
						if ( resIndex == 0 )
							break;
						++index;
					}
				}
			}

			const auto newEntryIndex = incomingToNewMapping[i];
			if ( newEntryIndex != -1 )
			{
				const auto index = cache.m_dependencyExcluded[i];
				if ( index != 0 )
					numAddedExcludeCachins += 1;
				m_dependencyExcluded[ baseExcluded + newEntryIndex ] = index ? (baseDepIndex + index) : 0;
			}
			else
			{
				const auto index = cache.m_dependencyExcluded[i];
				if ( index != 0 )
				{
					numReplacedExcludeChains += 1;
					const auto existingFileIndex = globalFileMapping[ i ];
					RED_FATAL_ASSERT( existingFileIndex < (Int32)baseResourceIndex, "File not new" );
					m_dependencyExcluded[ existingFileIndex ] = index + baseDepIndex;
				}
			}

		}

		LOG_CORE( TXT("Replaced %d exclude chains, added %d exclude chains"), numReplacedExcludeChains, numAddedExcludeCachins );
		LOG_CORE( TXT("Dependency cache appended in in %1.2fms"), timer.GetTimePeriod() );
	}

	// release the lock
	m_resizeLock.ReleaseWriteExclusive();
}
