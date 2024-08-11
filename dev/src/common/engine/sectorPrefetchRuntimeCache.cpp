/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorPrefetchGrid.h"
#include "sectorPrefetchQuery.h"
#include "sectorPrefetchRuntimeCache.h"
#include "sectorPrefetchMemoryBuffer.h"
#include "sectorPrefetchDataLoader.h"

#include "../../common/core/directory.h"
#include "../../common/core/2darray.h"

// hack
#include "../../common/core/fileSystemProfilerWrapper.h"
#include "../../common/core/bundleFileReaderDecompression.h"

//#define PREFETCH_LOG( x, ... ) fwprintf( stderr, x, __VA_ARGS__ );
//#define PREFETCH_TIME_COUNTER( _name ) CTimeCounter _name;

#define PREFETCH_LOG(x, ... )
#define PREFETCH_TIME_COUNTER( _name )

CSectorPrefetchRuntimeCache::CSectorPrefetchRuntimeCache( const CSectorPrefetchMemoryBuffer& memoryBuffer )
	: m_memoryBuffer( &memoryBuffer )
	, m_loader( nullptr )
	, m_grid( nullptr )

	, m_tickCounter( 0 )
	, m_memoryUsed( 0 )
	, m_memoryAllocatorPos( 0 )
{
	m_cachedEntries.Reserve( MAX_ENTRIES );
}

CSectorPrefetchRuntimeCache::~CSectorPrefetchRuntimeCache()
{
	// close loader (finish all loading shit)
	if ( m_loader )
	{
		delete m_loader;
		m_loader = nullptr;
	}

	// delete static grid
	if ( m_grid )
	{
		delete m_grid;
		m_grid = nullptr;
	}
}

const Bool CSectorPrefetchRuntimeCache::Initialize( const String& absoluteFilePath )
{
	// load static grid data
	m_grid = new CSectorPrefetchGrid();
	if ( !m_grid->Load( absoluteFilePath ) )
	{
		delete m_grid;
		m_grid = nullptr;

		ERR_ENGINE( TXT("Sector prefetch grid not loaded, runtime cache will NOT be created") );
		return false;
	}

	// open async file (create loader)
	m_loader = new CSectorPrefetchDataLoader( absoluteFilePath );

	// create cell wrappers
	const Uint32 numCells = m_grid->m_cells.Size();
	m_cachedCells.Resize( numCells );
	for ( Uint32 i=0; i<numCells; ++i )
	{
		auto& cachedCell = m_cachedCells[i];
		const auto& srcCell = m_grid->m_cells[i];

		// copy data
		cachedCell.m_lastTickUsed = 0;
		cachedCell.m_memoryDataOffset = 0; // not allocated yet
		cachedCell.m_state = eCell_NotLoaded;
		cachedCell.m_dataSize = srcCell.m_dataSize;
		cachedCell.m_diskDataOffset = srcCell.m_dataOffset;
	}

	// create a list of all known resources
	m_allKnownEntries.Reserve( m_grid->m_entries.Size() );
	for ( const auto& entry : m_grid->m_entries )
	{
		m_allKnownEntries.Insert( entry.m_resourceHash );
	}
	m_allKnownEntries.Shrink();

	// done
	LOG_ENGINE( TXT("Sector data prefetch ready, %d cells in cache, %d unqiue resources"), 
		m_cachedCells.Size(), m_allKnownEntries.Size() );

	m_ignoredBuffers.Clear();

	//!========================================================================================================================
	//! Load list of ignored buffers
	//! ignored list is in C2dArray format and is in the same director as streaming.cache
	//! ex. if streaming cache have path: D:\APP_HOME\app\content\content4\streaming.cache
	//!       ignore list will have path: D:\APP_HOME\app\content\content4\streaming.cache.ignore.csv
	//!========================================================================================================================
	size_t lastSeparatorIndex = 0;
	absoluteFilePath.FindCharacter( '\\', lastSeparatorIndex, true );

	String streamingCacheDirPath = absoluteFilePath.LeftString(lastSeparatorIndex+1);
	String streamingCacheFileName = absoluteFilePath.RightString(absoluteFilePath.GetLength() - (lastSeparatorIndex+1));
	streamingCacheFileName += TXT(".ignore.csv");
	
	CDirectory streamingCacheDir(streamingCacheDirPath.AsChar(), streamingCacheDirPath.Size()-1, NULL);
	CDiskFile* streamingCacheIgnoreListFile = streamingCacheDir.FindLocalFile(streamingCacheFileName);
	if( streamingCacheIgnoreListFile )
	{
		THandle< C2dArray > ignoreListFile = Cast< C2dArray >( streamingCacheIgnoreListFile->Load() ) ;
		if(ignoreListFile)
		{
			const Uint32 count = ignoreListFile->GetNumberOfRows();

			for( Uint32 i = 0; i < count; ++i )
			{
				const String fileDepotPath = ignoreListFile->GetValue(0, i);
				const Uint64 resourceHash = Red::CalculatePathHash64(fileDepotPath.AsChar());
				m_ignoredBuffers.Insert(resourceHash);
			}
		}
		ignoreListFile->Discard();
	}
	//!========================================================================================================================
	return true;
}

void* CSectorPrefetchRuntimeCache::GetCachedData( const Uint64 resourceHash, const Uint32 expectedDataSize, DataAllocFunction allocator, DataFreeFunction deallocator ) const
{
	TScopeLock lock( m_lock );

	// do we have any entry for this hash ?
	Uint32 globalEntryIndex = 0;
	if ( !m_cachedEntries.Find( resourceHash, globalEntryIndex ) )
	{
		// we know this resource, but for some reason it's not yet cached (maybe a problem in the grid generation)
#ifndef RED_FINAL_BUILD
		if ( m_allKnownEntries.Exist( resourceHash ) )
		{
			WARN_ENGINE( TXT("StreamingPrefetch: Resource %016llX is known but not cached, size %d"), 
				resourceHash, expectedDataSize );

			// find the NOT LOADED cells this resource is referenced
			for ( Uint32 cellIndex=0; cellIndex<m_grid->m_cells.Size(); ++cellIndex )
			{
				const auto& sourceCell = m_grid->m_cells[cellIndex];

				bool found = false;
				for ( Uint32 i=0; i<sourceCell.m_numEntries; ++i )
				{
					const auto& sourceEntry = m_grid->m_entries[ sourceCell.m_firstEntry + i ];
					if ( sourceEntry.m_resourceHash == resourceHash )
					{
						found = true;
						break;
					}
				}

				if ( found )
				{
					WARN_ENGINE( TXT("StreamingPrefetch: Found in cell #%d, [%d,%d]"), 
						cellIndex, sourceCell.m_cellX, sourceCell.m_cellY );
				}
			}

			// list loaded cells
			Uint32 numLoadedCells = 0;
			for ( Uint32 cellIndex=0; cellIndex<m_grid->m_cells.Size(); ++cellIndex )
			{
				if ( m_cachedCells[cellIndex].m_state != eCell_NotLoaded )
				{
					const auto& sourceCell = m_grid->m_cells[cellIndex];
					WARN_ENGINE( TXT("StreamingPrefetch: Loaded cell #%d, [%d,%d]"), 
						cellIndex, sourceCell.m_cellX, sourceCell.m_cellY );

					numLoadedCells += 1;
				}
			}
			WARN_ENGINE( TXT("StreamingPrefetch: We have %d loaded cells"), numLoadedCells );
		}
#endif

		return nullptr;
	}

	//! skip loading ignored buffers 
	//! buffer can be ignored if is patched 
	//! stream cache dose not have patching system so we have to skip such content
	if(m_ignoredBuffers.Exist(resourceHash))
	{
		LOG_ENGINE( TXT("StreamingPrefetch: Skip loading %016llX it is patched."), resourceHash );
		return nullptr;
	}

	// get the entry information
	const auto& sourceEntry = m_grid->m_entries[ globalEntryIndex ];

	// is the cell data loaded ?
	const CachedCell& cell = m_cachedCells[ sourceEntry.m_cellIndex ];
	if ( cell.m_state != eCell_Ready )
	{
		if ( cell.m_state == eCell_Loading )
		{
			const auto& sourceCell = m_grid->m_cells[ sourceEntry.m_cellIndex ];
			PREFETCH_LOG( L"StreamingPrefetch: Lost opportunity, cell [%d,%d] still loading\n", sourceCell.m_cellX, sourceCell.m_cellY );
		}
		return nullptr;
	}

	// decompression size is different (incompatible data)
	if ( sourceEntry.m_uncompressedSize != expectedDataSize )
	{
		ERR_ENGINE( TXT("StreamingPrefetch: Data mismatch for resource %016llX, %d != %d"),
			resourceHash, sourceEntry.m_uncompressedSize, expectedDataSize );
		return nullptr;
	}

	// allocate output memory
	void* outMemory = allocator( expectedDataSize, 16 ); // todo: what is the exact alignment we need here ?
	if ( !outMemory )
		return nullptr;

	// stats
	const auto& sourceCell = m_grid->m_cells[ sourceEntry.m_cellIndex ];
	LOG_ENGINE( TXT("StreamingPrefetch: Loading data from streaming cache, cell [%d,%d], entry %d, size %1.2f KB"), 
		sourceCell.m_cellX, sourceCell.m_cellY, globalEntryIndex, sourceEntry.m_uncompressedSize / 1024.0 );

	// get the data buffer
	const void* compressedData = OffsetPtr( (const void*)m_memoryBuffer->GetMemory(), cell.m_memoryDataOffset + sourceEntry.m_dataOffset );
	if ( sourceEntry.m_compressionType == Red::Core::Bundle::CT_Uncompressed )
	{
		// direct copy
		Red::MemoryCopy( outMemory, compressedData, expectedDataSize );
	}
	else
	{
		// update profiling stats
#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileDiskFileSyncDecompressStart( sourceEntry.m_uncompressedSize, (Uint8) sourceEntry.m_compressionType );
#endif	

		// allocate output decompression buffer
		void* decompressionBuffer = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_IO, MC_IOTempshit, sourceEntry.m_uncompressedSize, 16 );
		if ( !decompressionBuffer )
		{
			PREFETCH_LOG( L"StreamingPrefetch: Not enough IO memory to prefetch, cell [%d,%d], entry %d, size %1.2f KB\n",
				sourceCell.m_cellX, sourceCell.m_cellY, globalEntryIndex, sourceEntry.m_uncompressedSize / 1024.0 );
			return nullptr;
		}

		// allocate decompressed memory
		if ( !BundleFileReaderDecompression::DecompressFileBufferSynch( 
			(Red::Core::Bundle::ECompressionType) sourceEntry.m_compressionType, 
			compressedData, sourceEntry.m_dataSize,
			decompressionBuffer, sourceEntry.m_uncompressedSize ) )
		{
			// free memory
			RED_MEMORY_FREE( MemoryPool_IO, MC_IOTempshit, decompressionBuffer );

			// compression error
			RED_FATAL( "Decompression failed!" );
			deallocator( outMemory );

			// update profiling stats
#ifdef RED_PROFILE_FILE_SYSTEM
			RedIOProfiler::ProfileDiskFileSyncDecompressEnd();
#endif	
			return nullptr;
		}

		// copy to target in bulk (needed for GPU memory)
		Red::MemoryCopy( outMemory, decompressionBuffer, sourceEntry.m_uncompressedSize );

		// free memory
		RED_MEMORY_FREE( MemoryPool_IO, MC_IOTempshit, decompressionBuffer );

		// update profiling stats
#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileDiskFileSyncDecompressEnd();
#endif	
	}

	// data used, update internal LRU tracking stuff
	m_tickCounter += 1;
	cell.m_lastTickUsed = m_tickCounter;
	return outMemory;
}

const Bool CSectorPrefetchRuntimeCache::FreeUnusedCells( const Uint32 amountOfMemoryToFree )
{
	PC_SCOPE_PIX( FreeUnusedCells );

	// sort cell by the time it was last used
	struct CellInfo
	{
		CachedCell*	m_cell;
		Uint32		m_lastTick;
		Uint32		m_dataSize;
	};
	TDynArray< CellInfo > cells;
	cells.Reserve( m_cachedCells.Size() );

	// get info about cells that could unload
	Uint32 totalMemoryToFree = 0;
	Uint32 totalMemoryKeptBecauseItWasFresh = 0;
	for ( auto& cell : m_cachedCells )
	{
		if ( cell.m_state == eCell_Ready )
		{
			if ( cell.m_lastTickUsed == m_tickCounter )
			{
				totalMemoryKeptBecauseItWasFresh += cell.m_dataSize;
				continue;
			}

			CellInfo info;
			info.m_cell = &cell;
			info.m_dataSize = cell.m_dataSize;
			info.m_lastTick = cell.m_lastTickUsed;
			cells.PushBack( info );

			totalMemoryToFree += cell.m_dataSize;
		}
	}

	// sort cells in order of the last tick whey were used, this simplifies unloading
	// number of objets: ~700, this is called very rarely (~once per minute, worst case: novigrad, ~once per 20s)
	LOG_ENGINE( TXT("StreamingPrefetch: Found %d cells that could be freed, %1.2f MB total, %1.2f MB fresh"), 
		cells.Size(), totalMemoryToFree / (1024.0*1024.0), totalMemoryKeptBecauseItWasFresh / (1024.0*1024.0) );
	::Sort( cells.Begin(), cells.End(), []( const CellInfo& a, const CellInfo& b ) { return a.m_lastTick > b.m_lastTick; } );

	// free memory by releasing it from the cells that were not used for a long time
	Uint32 numFreedMemory = 0;
	Uint32 numFreedCells = 0;
	while ( numFreedMemory < amountOfMemoryToFree && !cells.Empty() )
	{
		const CellInfo& lastCell = cells.Back();

		// reset cell
		RED_FATAL_ASSERT( lastCell.m_cell->m_state == eCell_Ready, "Cell is not ready to be freed" );
		lastCell.m_cell->m_state = eCell_NotLoaded;
		lastCell.m_cell->m_memoryDataOffset = 0;

		// free memory but DO NOT TOUCH the allocator position
		RED_FATAL_ASSERT( lastCell.m_dataSize <= m_memoryUsed, "Freed cell is larged then the allocated memory" );
		m_memoryUsed -= lastCell.m_dataSize;

		// stats
		numFreedMemory += lastCell.m_dataSize;
		numFreedCells += 1;

		// pop
		cells.PopBack();
	}

	// have we freed enough ?
	if ( numFreedMemory >= amountOfMemoryToFree )
	{
		LOG_ENGINE( TXT("StreamingPrefetch: Freed %1.2f MB from %d cells. Enough."),
			numFreedMemory / (1024.0*1024.0), numFreedCells );
		return true;
	}
	else
	{
		WARN_ENGINE( TXT("StreamingPrefetch: All possible memory freed but it's still not enough. Waiting for pending loading to finish.") );	
		return false;
	}
}

const Bool CSectorPrefetchRuntimeCache::DefragMemory()
{
	// TODO: we cannot allocate if we are loading and there's no enough free memory, that's because that once the IO request is submitted we cannot move the buffer
	if ( m_loader->IsLoading() )
		return false;

	// extract used cells
	struct CellInfo
	{
		CachedCell*		m_cell;
		Uint32			m_dataOffset;
		Uint32			m_dataSize;
	};
	TDynArray< CellInfo > cellsToDefrag;

	for ( auto& cell : m_cachedCells )
	{
		RED_FATAL_ASSERT( cell.m_state != eCell_Loading, "Cannot defrag while loading" );
		if ( cell.m_state == eCell_Ready )
		{
			CellInfo cellInfo;
			cellInfo.m_cell = &cell;
			cellInfo.m_dataSize = cell.m_dataSize;
			cellInfo.m_dataOffset = cell.m_memoryDataOffset;
			cellsToDefrag.PushBack( cellInfo );
		}
	}

	// sort the defrag cells by their position in the memory buffers
	::Sort( cellsToDefrag.Begin(), cellsToDefrag.End(), []( const CellInfo& a, const CellInfo& b ) { return a.m_dataOffset < b.m_dataOffset; } );

	// move stuff
	const Uint32 prevMemEnd = m_memoryAllocatorPos;
	m_memoryUsed = 0;
	m_memoryAllocatorPos = 0;
	for ( auto& cell : cellsToDefrag )
	{
		// copy data to new place
		void* newMemory = OffsetPtr( m_memoryBuffer->GetMemory(), m_memoryUsed );
		const void* oldMemory = OffsetPtr( m_memoryBuffer->GetMemory(), cell.m_dataOffset );
		Red::MemoryMove( newMemory, oldMemory, cell.m_dataSize ); // Note: DO NOT USE MemoryCopy here!
		// TODO: we may use DMA here, that's pretty simple to integrate

		// set new offset
		cell.m_cell->m_memoryDataOffset = m_memoryUsed;

		// advance to new place (this is the packing step)
		m_memoryUsed += cell.m_dataSize;
		m_memoryAllocatorPos += cell.m_dataSize;
	}

	// defrag log
	PREFETCH_LOG( L"StreamingPrefetch: Memory defrag %1.3f -> %1.3f MB\n",
		prevMemEnd / ( 1024.0*1024.0 ), m_memoryAllocatorPos / ( 1024.0*1024.0 ) );
	return true;
}

struct LockHelper
{
public:
	LockHelper( Red::Threads::CMutex& lock )
		: m_lock( &lock )
		, m_isLocked( false )
	{
	}

	~LockHelper()
	{
		if ( m_isLocked)
		{
			m_lock->Release();
		}
	}

	bool TryLock()
	{
		if ( m_lock->TryAcquire() )
		{
			m_isLocked = true;
			return true;
		}

		return false;
	}

private:
	Red::Threads::CMutex*	m_lock;
	Bool					m_isLocked;
};

const Bool CSectorPrefetchRuntimeCache::PrecacheCells( const Vector& worldPosition )
{
	PREFETCH_TIME_COUNTER( timer );
	PC_SCOPE_PIX( PrecacheCells );

	// no grid
	if ( !m_grid )
		return false;

	// get query
	const CSectorPrefetchQueryResult query = m_grid->QueryCellsAtPosition( worldPosition );

	// advance tick counter (time passing, used for LRU)
	m_tickCounter += 1;

	// if all cells are there do nothing
	Uint32 additionalMemoryNeeded = 0;
	Uint32 additionalCellsToLoad = 0;
	Uint32 totalCellMemoryToLoad = 0;
	TStaticArray< Int16, 64 > cellsToLoad;
	for ( Uint32 i=0; i<query.m_numCells; ++i )
	{
		const Int16 cellIndex = query.m_cells[i];
		if ( cellIndex < m_cachedCells.SizeInt() )
		{
			// the cell we require is not yet loaded
			auto& cell = m_cachedCells[cellIndex];
			if ( cell.m_state == eCell_NotLoaded )
			{
				cellsToLoad.PushBack( cellIndex );

				additionalMemoryNeeded += cell.m_dataSize;
				additionalCellsToLoad += 1;
			}
			else
			{
				// the cell is in the query but is getting loaded, bump the usage time
				cell.m_lastTickUsed = m_tickCounter;
			}

			// collect total memory this grid hierarchy requires
			totalCellMemoryToLoad += cell.m_dataSize;
		}
	}

	// everything is there (or loading)
	if ( additionalMemoryNeeded == 0 )
		return true;

	// lock the shit - this may fail if we are actively loading, this is needed to prevent very rare but painfull ~100ms stalls
	// there's no rush in the prefetch and we can wait another frame
	LockHelper scopeLock( m_lock );
	if ( !scopeLock.TryLock() )
	{
		PREFETCH_LOG( L"Denied!\n", 0 );
		return true;
	}

	// IO profiling
	RedIOProfilerBlock ioBlock( TXT("Prefetch") );

	// stats
	LOG_ENGINE( TXT("StreamingPrefetch: Requested %d cells to load (%1.3fMB), total chain (%1.3fMB)"), 
		additionalCellsToLoad, additionalMemoryNeeded / (1024.0*1024.0), totalCellMemoryToLoad / (1024.0*1024.0) );

	// if we don't have enough memory free the cells that are not used
	const Uint32 maxMemory = m_memoryBuffer->GetSize();
	if ( m_memoryUsed + additionalMemoryNeeded > maxMemory )
	{
		PREFETCH_TIME_COUNTER( timer );

		// free enough memory so we can allocate new blocks
		const Uint32 memoryToFree = (m_memoryUsed + additionalMemoryNeeded) - maxMemory;
		if ( !FreeUnusedCells( memoryToFree ) )
			return false; // this can happen if we have locked to many buffers in IO requests and there's no additional memory to free

		PREFETCH_LOG( L"Streaming prefetch cleanup: %1.2fms\n", timer.GetTimePeriodMS() );
	}

	// make sure we DO HAVE memory now
	RED_FATAL_ASSERT( m_memoryUsed + additionalMemoryNeeded <= maxMemory, "Still no memory to load new data" );

	// if we won't fit at the end of the allocation block than we need to defrag current memory
	if ( m_memoryAllocatorPos + additionalMemoryNeeded > maxMemory )
	{
		PREFETCH_TIME_COUNTER( timer );

		LOG_ENGINE( TXT("StreamingPrefetch: Requested memory won't fit at the end of the block. Defragmenting. ") );

		// optimize memory buffer, can fail
		if ( !DefragMemory() )
			return false;

		PREFETCH_LOG( L"Streaming prefetch defrag: %1.2fms\n", timer.GetTimePeriodMS() );
	}

	// make sure we DO HAVE block now
	RED_FATAL_ASSERT( m_memoryAllocatorPos + additionalMemoryNeeded <= maxMemory, "Memory is still fragmented" );

	// start loading jobs for the new cells
	for ( Uint32 i=0; i<cellsToLoad.Size(); ++i )
	{
		const Uint32 cellIndex = cellsToLoad[i];
		auto& cell = m_cachedCells[ cellIndex ];

		/// allocate memory range
		RED_FATAL_ASSERT( m_memoryUsed <= maxMemory, "Unexpected out of memory" );
		cell.m_memoryDataOffset = m_memoryAllocatorPos;
		m_memoryAllocatorPos += cell.m_dataSize;
		m_memoryUsed += cell.m_dataSize;

		/// configure cell in the loading state (data will not yet be avaiable but cell will NOT be moved in the memory until the loading is finished)
		cell.m_state = eCell_Loading;

		/// start loading shit
		void* cellMemory = OffsetPtr( m_memoryBuffer->GetMemory(), cell.m_memoryDataOffset );
		m_loader->RequestLoad( cell.m_diskDataOffset, cell.m_dataSize, cellMemory, [cellIndex, this]()
			{
				auto& loadedCell = m_cachedCells[ cellIndex ];
				RED_FATAL_ASSERT( loadedCell.m_state == eCell_Loading, "Expecting cell to be in loading state" );
				loadedCell.m_state = eCell_Ready;

				const auto& sourceCell = m_grid->m_cells[ cellIndex ];
				LOG_ENGINE( TXT("StreamingPrefetch: cell [%d,%d] loaded (%1.2fMB)"), 
					sourceCell.m_cellX, sourceCell.m_cellY, sourceCell.m_dataSize / (1024.0*1024.0) );
			} );
	}

	// we always rebuild the entry hashtable (for now), at least make sure we don't reallocate memory
	m_cachedEntries.ClearFast();

	// collect entries from all cells that are loading/loaded
	Uint32 numDataAvaiable = 0;
	for ( Uint32 i=0; i<m_cachedCells.Size(); ++i )
	{
		const auto& cachedCell = m_cachedCells[i];
		if ( cachedCell.m_state == eCell_NotLoaded )
			continue;

		// get source cell
		const auto& sourceCell = m_grid->m_cells[i];

		// process the entries
		for ( Uint32 j=0; j<sourceCell.m_numEntries; ++j )
		{
			const Uint32 globalEntryIndex = sourceCell.m_firstEntry + j;
			const auto& sourceEntry = m_grid->m_entries[ globalEntryIndex ];

			m_cachedEntries.Set( sourceEntry.m_resourceHash, globalEntryIndex );
			numDataAvaiable += sourceEntry.m_dataSize;
		}
	}

	// stats
	LOG_ENGINE( TXT("StreamingPrefetch: %d entries in cache (%1.2f MB)"),
		m_cachedEntries.Size(), numDataAvaiable / (1024.0*1024.0) );

	PREFETCH_LOG( L"Streaming prefetch update: %1.2fms\n", timer.GetTimePeriodMS() );

	// done
	return true;
}

const Bool CSectorPrefetchRuntimeCache::Exist(const Uint64 resourceHash) const
{
	return m_allKnownEntries.Exist(resourceHash);
}