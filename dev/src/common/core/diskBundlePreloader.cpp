/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "depot.h"
#include "depotBundles.h"
#include "diskBundle.h"
#include "diskBundlePreloader.h"
#include "bundleFileReaderDecompression.h"
#include "memoryFileReader.h"
#include "configVar.h"
#include "fileSystemProfilerWrapper.h"
#include "fileDecompression.h"
#include "ioTagResolver.h"
#include "../redIO/redIO.h"
#include "..\redMemoryFramework\redMemorySystemMemoryStats.h"
#include "..\redMemoryFramework\redMemoryAllocatorInfo.h"
#include "hashset.h"
#include "resourceLoading.h"

namespace Config
{
	TConfigVar< Int32, Validation::IntRange< 4, 1024 > >		cvBundleBurstReadBlock( "FileSystem/BundlePreload", "BundleBurstReadBlockKB", 512 );
	TConfigVar< Int32, Validation::IntRange< 1, 64 > >			cvBundleBurstReadMaxChunk( "FileSystem/BundlePreload", "BundleBurstReadMaxChunkMB", 15 );
}

CDiskBundlePreloader::CDiskBundlePreloader()
{
}

void CDiskBundlePreloader::BuildChunkMap( const Uint32 maxChunkSize, const TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement >& placement, TDynArray< IOChunk >& outChunks )
{
	Uint32 filePos = placement[0].m_offsetInBundle;
	Uint32 fileIndex = 0;
	while ( fileIndex < placement.Size() )
	{
		// start new chunk
		IOChunk newChunk;
		newChunk.m_diskOffset = filePos;
		newChunk.m_firstFile = fileIndex;

		// keep adding stuff until we run out of space in the loading buffer
		while ( fileIndex < placement.Size() )
		{
			// will it fit ?
			const Uint32 endOfTheFile = placement[fileIndex].m_offsetInBundle + placement[fileIndex].m_sizeInBundle;
			if ( (endOfTheFile-newChunk.m_diskOffset) > maxChunkSize )
				break;

			// keep going
			fileIndex += 1;
			filePos = endOfTheFile;
		}

		// push to list
		newChunk.m_diskSize = (filePos - newChunk.m_diskOffset);
		newChunk.m_numFiles = fileIndex - newChunk.m_firstFile;
		outChunks.PushBack( newChunk );
	}

	// stats
	LOG_CORE( TXT("Created %d IO chunks"), outChunks.Size() );
	for ( Uint32 i=0; i<outChunks.Size(); ++i )
	{
		LOG_CORE( TXT("  Chunkd[%d]: %d files, %1.2fKB"), i, outChunks[i].m_numFiles, outChunks[i].m_diskSize / 1024.0f );
	}
}

void CDiskBundlePreloader::LoadMissingDependencies( const TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement >& filesPlacement )
{
	CDependencyCollector* collector = GDepot->GetDependencyCache().AllocateCollector();

	if ( !collector )
	{
		return;
	}

	THashSet< Red::Core::Bundle::FileID > usedFileIDs;

	for ( Uint32 i=0; i<filesPlacement.Size(); ++i )
	{
		const Red::Core::Bundle::FileID fileID = filesPlacement[i].m_fileID;
		CBundleDiskFile* file = GDepot->GetBundles()->GetBundleDiskFile( fileID );
		if ( file )
		{
			usedFileIDs.Insert( fileID );

			Uint32 depCacheIndex = file->GetDepCacheIndex();
			if ( depCacheIndex != 0 )
			{
				GDepot->GetDependencyCache().CollectDependencies( *collector, depCacheIndex );
			}
		}
	}

	TDynArray< CDiskFile* > files;

	for ( Uint32 i = 0; i < collector->Size(); ++i )
	{
		const Red::Core::Bundle::FileID depIndex = collector->GetFileIndex( i );
		CDiskFile* file = GDepot->GetMappedDiskFile( depIndex );

		if ( file && !( file->IsLoaded() || file->IsFailed() || file->IsLooseFile() ) )
		{
			CBundleDiskFile* bundleFile = static_cast< CBundleDiskFile* >( file );
			const Red::Core::Bundle::FileID fileID = bundleFile->GetFileID();

			if ( !usedFileIDs.Exist( fileID ) )
			{
				files.PushBack( file );
			}
		}
	}

	SResourceLoader::GetInstance().Load( nullptr, (CDiskFile**)files.Data(), files.Size(), eResourceLoadingPriority_Normal, nullptr);

	collector->Release();
}

void CDiskBundlePreloader::Preload( Red::IO::CAsyncFileHandleCache::TFileHandle fileHandle, Red::Core::Bundle::BundleID bundleID, TDynArray< THandle< CResource > >& outLoadedResources )
{
	PC_SCOPE( PreloadBundle );	

	CTimeCounter timer;

	// we will be reading the whole bundle data - from start till the first buffer or end
	Uint32 readRangeStart=0, readRangeEnd=0;
	GDepot->GetBundles()->GetBundleBurstReadRange( bundleID, readRangeStart, readRangeEnd );

	// extract bundle file layout
	// get mapped bundled files that will be loaded
	TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement > files;
	TDynArray< CBundleDiskFile* > diskFiles;
	GDepot->GetBundles()->GetBundleContent( bundleID, readRangeStart, readRangeEnd, files, diskFiles );

	// no files reported, nothing to preload
	if ( files.Empty() )
	{
		LOG_CORE( TXT("Nothing to preload") );
		return;
	}

	// load dependencies that are not present in the bundle ( before we allocate memory for preloading buffers )
	LoadMissingDependencies( files );
	
	// find the biggest block size required for decompression
	Uint32 maxCompressedBlock = 0;
	Uint32 maxDecompressedBlock = 0;
	for ( const auto& info : files )
	{
		maxCompressedBlock = Max< Uint32 >( maxCompressedBlock, info.m_sizeInBundle );
		maxDecompressedBlock = Max< Uint32 >( maxDecompressedBlock, info.m_sizeInMemory );
	}

	// stats
	LOG_CORE( TXT("Bundle preloading, data size=%1.2fKB, max compressed=%1.2fKB, max uncompressed=%1.2fKB, max pool=%1.2fMB"), 
		(readRangeEnd-readRangeStart) / 1024.0f, maxCompressedBlock / 1024.0f, maxDecompressedBlock / 1024.0f,
		Memory::GetPoolBudget< MemoryPool_IO >() / (1024.0*1024.0) );

	// get the maximum read block size it's the leftover from preload memory minus the size of the largest decompression block
	const Uint32 maxMemoryBudget = (Uint32) Min<Uint64>( Memory::GetPoolBudget< MemoryPool_IO >(), 50 * 1024 * 1024 ); // this prevents PC from using all of the memory for IO which is needed for MODs
	const Uint32 minimumExtraBlock = 5 * 1024 * 1024;
	const Uint32 maxUsuableMemory = (Uint32)( maxMemoryBudget - 65536 ) - minimumExtraBlock;
	RED_FATAL_ASSERT( maxUsuableMemory > maxDecompressedBlock + maxCompressedBlock, "Not enough memory to preload bundle" );
	const Uint32 maxBlockSize = maxUsuableMemory - maxDecompressedBlock;

	// fatal assert if the required block will be not enough for the reading of the bundle
	if ( maxCompressedBlock > maxBlockSize )
	{
		ERR_CORE( TXT("Not enough memory to preload the bundle, there are compressed files that are bigger than the memory we can spare") );
		ERR_CORE( TXT("Biggest compresed file = %1.2fKB"), maxCompressedBlock / 1024.0f );
		ERR_CORE( TXT("Maximum block we can allocate = %1.2fKB"), maxBlockSize / 1024.0f );

		for ( const auto& info : files )
		{
			if ( info.m_sizeInBundle > maxBlockSize )
			{
				ERR_CORE( TXT("  FileID %d, SizeOnDisk=%d, SizeInMemory=%d"), info.m_fileID, info.m_sizeInBundle, info.m_sizeInMemory );
			}
		}

		// force crash
		RED_MEMORY_HALT( "Out of Memory for loading!" );

#ifdef RED_FINAL_BUILD
		// We force a crash in final builds, just so we can see this is a OOM situation.
		// On shipping, we should most likely disable this, unless we will be getting crash dumps from live machines
		int* forceCrashNow = nullptr;
		*forceCrashNow = 0xE0000000;
#endif
	}

	// flush ALL of the decompression tasks to free as much IO memory as possible
	GFileManager->GetDecompressionEngine()->FlushAndLock();

	// prepare IO chunks
	TDynArray< IOChunk > ioChunks;
	BuildChunkMap( maxBlockSize, files, ioChunks );

	// allocate memory for the file data
	// TODO: current decompression DOES NOT support memory that is not a contiguous block so we HAVE TO load the file in one go
	void* fileData = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_IO, MC_IOBurstReadBuffer, maxBlockSize, 16 );
	RED_FATAL_ASSERT( fileData != nullptr, "Out of memory in bundle preloading - IO pool is fragmented ?" );

#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileVarAllocMemoryBlock( maxBlockSize );
#endif

	// allocate the decompression buffer
	void* decompressionData = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_IO, MC_IOTempshit, maxDecompressedBlock, 16 );
	RED_FATAL_ASSERT( decompressionData != nullptr, "Out of memory in bundle preloading (failed to alloc %u bytes)- IO pool is fragmented ?", maxDecompressedBlock );

	// both of our working buffers are allocated now, we can now unlock the decompression manager
	GFileManager->GetDecompressionEngine()->Unlock();

#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileVarAllocMemoryBlock( maxDecompressedBlock );
#endif

	// containers for the loaded resources
	outLoadedResources.Reserve( files.Size() );

	// process the chunks
	for ( const IOChunk& chunk : ioChunks )
	{
		// sync variable
		volatile Uint32 readDataPos = chunk.m_diskOffset;

		// setup reading state
		ReadingState readingState;
		readingState.m_bufferBase = fileData;
		readingState.m_fileMarkerPos = &readDataPos;
		readingState.m_readStartPos = chunk.m_diskOffset;
		readingState.m_readEndPos = chunk.m_diskOffset + chunk.m_diskSize;
		readingState.m_readBlockSize = Config::cvBundleBurstReadBlock.Get() * 1024;

		// prepare reading
		Red::IO::SAsyncReadToken readToken;
		readToken.m_buffer = fileData;
		readToken.m_userData = &readingState;
		readToken.m_offset = chunk.m_diskOffset;
		readToken.m_numberOfBytesToRead = Min< Uint32 >( chunk.m_diskSize, (Uint32) AlignOffset( chunk.m_diskOffset, readingState.m_readBlockSize ) - chunk.m_diskOffset );
		readToken.m_callback = &CDiskBundlePreloader::OnDataLoaded;

		// schedule first read - the rest will be handled automatically
		const auto resolvedPriority = GFileSysPriorityResovler.Resolve( eIOTag_BundlePreload );
		Red::IO::GAsyncIO.BeginRead( fileHandle, readToken, resolvedPriority, eIOTag_BundlePreload );

		// deserialize files as we get the data for them
		// to see if we have data for a given file we check if the reading pos is AFTER the end of the file 
		Uint32 fileIndex = chunk.m_firstFile;
		const Uint32 lastFileThisChunk = chunk.m_firstFile+chunk.m_numFiles;
		while ( fileIndex < lastFileThisChunk )
		{
			while ( fileIndex < lastFileThisChunk && (readDataPos >= ( files[ fileIndex ].m_offsetInBundle + files[ fileIndex ].m_sizeInBundle )) )
			{
				// get the mapped file 
				const auto& fileInfo = files[ fileIndex ];
				CBundleDiskFile* file = diskFiles[ fileIndex++ ];
				if ( !file )
					continue;

				// skip files that are failed or already loaded
				if ( file->IsLoaded() || file->IsFailed() )
				{					
					WARN_CORE( TXT("Preload problem: File '%ls' is already loaded"), file->GetDepotPath().AsChar() );

					// assume the file was preloaded anyway
					if ( file->GetResource() )
						outLoadedResources.PushBack( file->GetResource() );

					continue;
				}

				// acquire loading lock, this will fail if file is already loaded
				if ( !file->BeingLoading() )
				{
					WARN_CORE( TXT("Preload problem: File '%ls' is already loaded"), file->GetDepotPath().AsChar() );
					continue;
				}

				// decompress
				void* fileMemory = OffsetPtr( fileData, fileInfo.m_offsetInBundle - chunk.m_diskOffset );
				if ( fileInfo.m_compression != Red::Core::Bundle::CT_Uncompressed )
				{
#ifdef RED_PROFILE_FILE_SYSTEM
					RedIOProfiler::ProfileDiskFileSyncDecompressStart( fileInfo.m_sizeInMemory, fileInfo.m_compression );
#endif
					BundleFileReaderDecompression::DecompressFileBufferSynch( 
						fileInfo.m_compression,
						fileMemory, fileInfo.m_sizeInBundle,
						decompressionData, fileInfo.m_sizeInMemory );
#ifdef RED_PROFILE_FILE_SYSTEM
					RedIOProfiler::ProfileDiskFileSyncDecompressEnd();
#endif

					fileMemory = decompressionData;
				}

				// create access wrapper
				CMemoryFileReaderExternalBuffer reader( fileMemory, fileInfo.m_sizeInMemory );
				reader.m_flags &= ~FF_MemoryBased;
				reader.m_flags |= FF_FileBased;

				// deserialize
				SDiskFilePostLoadList poadLoad;
				file->InternalDeserialize( &reader, nullptr /*use default*/, poadLoad );

				// post load only if serialization succeeded
				if ( !file->IsFailed() )
				{
					file->InternalPostLoad( poadLoad );
				}

				// add loaded resource to the list
				CResource* loadedResource = file->GetResource();
				if ( loadedResource )
				{
					outLoadedResources.PushBack( loadedResource );
				}
			}

			// we are IO bound - wait for the IO
			if ( lastFileThisChunk < files.Size() )
			{
				Red::Threads::YieldCurrentThread();
			}
		}
	}

	// cleanup
	RED_MEMORY_FREE( MemoryPool_IO, MC_IOBurstReadBuffer, fileData );
	RED_MEMORY_FREE( MemoryPool_IO, MC_IOTempshit, decompressionData );

#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileVarFreeMemoryBlock( maxBlockSize );
	RedIOProfiler::ProfileVarFreeMemoryBlock( maxDecompressedBlock );
#endif

	// finalize
	LOG_CORE( TXT("Bundle preloaded in %1.2fs (%1.2f KB/s), %d resource loaded (ouf of %d)"), 
		timer.GetTimePeriod(), 
		((readRangeEnd-readRangeStart) / (Float)timer.GetTimePeriod()) / 1024.0f,
		outLoadedResources.Size(), files.Size() );
}

Red::IO::ECallbackRequest CDiskBundlePreloader::OnDataLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	ReadingState* state = (ReadingState*) asyncReadToken.m_userData;	
	
	// failed
	RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Incomplete read" );

	// advance internal loading position
	Red::Threads::AtomicOps::ExchangeAdd32( (Red::Threads::AtomicOps::TAtomic32*) state->m_fileMarkerPos, (Uint32)numberOfBytesTransferred );

	// calculate remaining size
	const Uint32 endOffset = (Uint32)( asyncReadToken.m_offset + numberOfBytesTransferred );
	const Uint32 sizeRemained = state->m_readEndPos - endOffset;
	if ( sizeRemained > 0 )
	{
		asyncReadToken.m_offset += numberOfBytesTransferred;
		asyncReadToken.m_numberOfBytesToRead = Min< Uint32 >( sizeRemained, state->m_readBlockSize );
		asyncReadToken.m_buffer = OffsetPtr( state->m_bufferBase, asyncReadToken.m_offset - state->m_readStartPos );

		// request more stuff to be read
		return Red::IO::eCallbackRequest_More;
	}
	else
	{
		// we are done reading
		return Red::IO::eCallbackRequest_Finish;
	}
}