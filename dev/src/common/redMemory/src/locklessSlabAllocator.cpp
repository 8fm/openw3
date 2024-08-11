/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "locklessSlabAllocator.h"
#include "systemAllocator.h"
#include "threadIdProvider.h"
#include "slabChunk.h"
#include "slabHeader.h"
#include "threadMonitor.h"

#include <xutility>
#include "flags.h"

namespace red
{
namespace memory
{
	const u32 c_initialChunkCount = 300;

	LocklessSlabAllocator::SlabChunkAllocator::SlabChunkAllocator()
		:	m_systemAllocator( nullptr ),
			m_nexChunkAddress( 0 ),
			m_firstChunkAddress( 0 ),
			m_chunkCount( 0 ),
			m_freeChunkCount( 0 )
	{
		std::memset( m_freeChunks, 0, sizeof( m_freeChunks ) );
	}

	void LocklessSlabAllocator::SlabChunkAllocator::Initialize()
	{
		for( u32 count = 0; count != c_initialChunkCount; ++count )
		{
			SlabChunk * chunk = AllocateChunk();
			ForceFree( 0, chunk );
		}

		m_freeChunkCount = c_initialChunkCount;
	}

	SlabChunk * LocklessSlabAllocator::SlabChunkAllocator::Allocate( u32 allocatorId )
	{
		SlabChunk * chunk = nullptr;
		
		for( u32 index = allocatorId; index != c_maxThreadCount && !chunk; ++index )
		{
			chunk = TryTakingFreeChunk( index );
		}

		// We do not have a chunk yet. 
		// Might be either there is no chunk available in most relevant list, or massive contention problem.
		// Loop through the whole array of free chunk list to find one.
		for( u32 index = 0; index != c_maxThreadCount && !chunk; ++index )
		{
			chunk = TryTakingFreeChunk( index );
		}

		// Did we found a chunk in the end ? Yes, take it, else create a new one.
		if( chunk )
		{
			atomic::Decrement32( &m_freeChunkCount );
			return chunk;
		}

		return AllocateChunk();
	}

	SlabChunk * LocklessSlabAllocator::SlabChunkAllocator::TryTakingFreeChunk( u32 id )
	{
		SlabChunk * chunk = nullptr;
		FreeChunks & chunks = m_freeChunks[ id ];
		if( chunks.chunkList && chunks.lock.TryAcquire() )
		{
			// We got the lock! But list could have changed between that time. Check again.
			if( chunks.chunkList )
			{
				chunk = chunks.chunkList;
				chunks.chunkList = chunk->nextFree;
			}

			chunks.lock.Release(); 
		}

		return chunk;
	}

	SlabChunk * LocklessSlabAllocator::SlabChunkAllocator::AllocateChunk()
	{
		atomic::Increment32( &m_chunkCount );

		SystemBlock block = {};

		{
			// Gotta make sure that multiple thread won't get same chunk address. 
			// No worries, it will be very fast. 
			// If it turns out to be a problem, it could be implemented using atomics.
			CScopedLock< CSpinLock > scopedLock( m_monitor );
			block = ComputeRequiredSystemBlockForSlabChunk( m_nexChunkAddress );
			m_nexChunkAddress = block.address + block.size;
		}
		
		block = m_systemAllocator->Commit( block, Flags_CPU_Read_Write ); // this allocator is cpu only and need read/write access.
		
		return block.address ? CreateSlabChunk( block ) : nullptr;
	}

	void LocklessSlabAllocator::SlabChunkAllocator::Free( u32 allocatorId, SlabChunk * chunk )
	{
		atomic::Increment32( &m_freeChunkCount );

		for( u32 index = allocatorId; index != c_maxThreadCount; ++index )
		{
			FreeChunks & availableChunk = m_freeChunks[ index ];
			if( availableChunk.lock.TryAcquire() )
			{
				chunk->nextFree = availableChunk.chunkList;
				availableChunk.chunkList = chunk;
				availableChunk.lock.Release();
				return;
			}
		}

		// Restart check from first local allocator. Give chunk to the first one that give lock.
		for( u32 index = 0; index != c_maxThreadCount; ++index )
		{
			FreeChunks & availableChunk = m_freeChunks[ index ];
			if( availableChunk.lock.TryAcquire() )
			{
				chunk->nextFree = availableChunk.chunkList;
				availableChunk.chunkList = chunk;
				availableChunk.lock.Release();
				return;
			}
		}

		// If we reach this point, we have a lot of contention.
		// Multiple thread are freeing full chunk or/and trying to recycle memory
		// This should be extremely rare. 
		// We got to give memory back, so lock FreeChunks list bound to this thread.
		ForceFree( allocatorId, chunk );
	}

	void LocklessSlabAllocator::SlabChunkAllocator::ForceFree( u32 allocatorId, SlabChunk * chunk )
	{
		FreeChunks & availableChunk = m_freeChunks[ allocatorId ];
		CScopedLock< CSpinLock > lock( availableChunk.lock );
		chunk->nextFree = availableChunk.chunkList;
		availableChunk.chunkList = chunk;
	}

	void LocklessSlabAllocator::SlabChunkAllocator::SetSystemAllocator( SystemAllocator * allocator )
	{
		m_systemAllocator = allocator;
	}
	
	void LocklessSlabAllocator::SlabChunkAllocator::SetNextMemoryAddress( u64 address )
	{
		m_firstChunkAddress = address;
		m_nexChunkAddress = address;
	}

	void LocklessSlabAllocator::SlabChunkAllocator::BuildMetrics( LocklessSlabAllocatorMetrics & metrics )
	{
		metrics.metrics.consumedSystemMemoryBytes = m_nexChunkAddress - m_firstChunkAddress;  
		metrics.freeChunkCount = m_freeChunkCount;
		metrics.usedChunkCount = m_chunkCount - m_freeChunkCount;
	}

	LocklessSlabAllocator::LocklessSlabAllocator()
		:	m_systemAllocator( nullptr ),
			m_threadIdProvider( nullptr ),
			m_threadMonitor( nullptr )
	{
		m_threadIdProvider = &m_threadIdProviderStorage;
		m_threadIdDictionary.fill( 0 );
	}

	LocklessSlabAllocator::~LocklessSlabAllocator()
	{}

	void LocklessSlabAllocator::Initialize( const LocklessSlabAllocatorParameter & parameter )
	{
		m_systemAllocator = parameter.systemAllocator;
		m_threadMonitor = parameter.threadMonitor;

		RED_MEMORY_ASSERT( m_systemAllocator, "LocklessSlabAllocator need access to SystemAllocator." );
		RED_MEMORY_ASSERT( m_threadMonitor, "LocklessSlabAllocator need access to ThreadMonitor." );
		
		m_reservedMemoryRange = m_systemAllocator->ReserveVirtualRange( parameter.virtualRangeSize, Flags_CPU_Read_Write );
		const u64 nextMemoryAddress = m_reservedMemoryRange.start;
		m_slabChunkAllocator.SetSystemAllocator( m_systemAllocator );
		m_slabChunkAllocator.SetNextMemoryAddress( nextMemoryAddress );
		m_slabChunkAllocator.Initialize();

		const SlabAllocatorParameter slabAllocatorParameter = 
		{
			m_reservedMemoryRange,
			&m_slabChunkAllocator
		};

		for( u32 id = 0; id != c_maxThreadCount; ++id )
		{
			SlabAllocator & allocator = m_allocators[ id ];
			allocator.SetId( id );
			allocator.Initialize( slabAllocatorParameter );
		}

		m_threadMonitor->RegisterOnThreadDiedSignal( LocklessSlabAllocator::NotifyOnThreadDied, this );
	}

	void LocklessSlabAllocator::Uninitialize()
	{
		RED_MEMORY_ASSERT( m_threadMonitor, "LocklessSlabAllocator need access to ThreadMonitor." );
		RED_MEMORY_ASSERT( m_systemAllocator, "LocklessSlabAllocator need access to SystemAllocator." );
		
		m_threadMonitor->UnregisterFromThreadDiedSignal( LocklessSlabAllocator::NotifyOnThreadDied, this );
		m_systemAllocator->ReleaseVirtualRange( m_reservedMemoryRange ); 
	}

	Block LocklessSlabAllocator::Allocate( u32 size )
	{
		SlabAllocator * allocator = AcquireLocalSlabAllocator();
		if( allocator )
		{
			return allocator->Allocate( size );
		}

		return NullBlock();
	}

	Block LocklessSlabAllocator::AllocateAligned( u32 size, u32 alignment )
	{
		SlabAllocator * allocator = AcquireLocalSlabAllocator();
		if( allocator )
		{
			return allocator->AllocateAligned( size, alignment );
		}

		return NullBlock();
	}

	Block LocklessSlabAllocator::Reallocate( Block & block, u32 size )
	{
		SlabAllocator * allocator = AcquireLocalSlabAllocator();

		if( allocator )
		{
			return allocator->Reallocate( block, size );
		}

		return NullBlock();
	}

	void LocklessSlabAllocator::Free( Block & block )
	{
		const u32 id = m_threadIdProvider->GetCurrentId();
		SlabAllocator * allocator = GetSlabAllocator( id );
		if( allocator )
		{
			allocator->Free( block );
		}
		else
		{
			// If there are no allocator local to this thread, than we are 100% sure the memory block was not allocated by this thread.
			// Simply push to local freelist. Owner will process it when it needs to.
			PushBlockToFreeList( block.address );
			block.size = GetSlabBlockSize( block.address );
		}
	}

	void LocklessSlabAllocator::RegisterCurrentThread()
	{
		const u32 id = m_threadIdProvider->GetCurrentId();

		if( std::find( m_threadIdDictionary.begin(), m_threadIdDictionary.end(), id ) == m_threadIdDictionary.end() )
		{
			auto iter = RegisterThread( id );
			RED_MEMORY_ASSERT( iter != m_threadIdDictionary.end(), "No more space for Thread in LocklessSlabAllocator." );
			if( iter != m_threadIdDictionary.end() )
			{
				m_threadMonitor->MonitorCurrentThread();	
			}
		}	
	}

	SlabAllocator * LocklessSlabAllocator::AcquireLocalSlabAllocator()
	{
		const u32 id = m_threadIdProvider->GetCurrentId();
		return GetSlabAllocator( id );
	}

	SlabAllocator * LocklessSlabAllocator::AcquireSlabAllocator( ThreadId id )
	{
		SlabAllocator * allocator = GetSlabAllocator( id );
		if( !allocator )
		{
			ThreadIdDictionary::iterator iter = RegisterThread( id );
			if( iter != m_threadIdDictionary.end() )
			{
				return &m_allocators[ std::distance( m_threadIdDictionary.begin(), iter ) ];
			}
		}

		return allocator;
	}

	SlabAllocator * LocklessSlabAllocator::GetSlabAllocator( ThreadId id )
	{
		// TODO pretty fast if c_slabMaxThreadCount is less than 16 as the whole dictionary fit in one cache line.
		// However, might be better to go with a simple hash/modulo look up. Like this we can have more than 16.
		
		for( u32 index = 0; index != c_maxThreadCount; ++index )
		{
			if( m_threadIdDictionary[ index ] == id )
				return &m_allocators[ index ];
		}

		return nullptr;
	}

	LocklessSlabAllocator::ThreadIdDictionary::iterator LocklessSlabAllocator::RegisterThread( ThreadId id )
	{
		for( ThreadIdDictionary::iterator iter = m_threadIdDictionary.begin(), end = m_threadIdDictionary.end(); iter != end; ++iter )
		{
			if( atomic::CompareExchange32( reinterpret_cast< atomic::TAtomic32* >( &( *iter ) ), id, 0 ) == 0 )
			{
				return iter;
			}
		}

		return m_threadIdDictionary.end();
	}

	void LocklessSlabAllocator::NotifyOnThreadDied( ThreadId id, void * userData )
	{
		LocklessSlabAllocator * allocator = static_cast< LocklessSlabAllocator * >( userData );
		allocator->OnThreadDied( id ); 
	}

	void LocklessSlabAllocator::OnThreadDied( ThreadId id )
	{
		SlabAllocator * localAllocator = GetSlabAllocator( id );
		if( localAllocator )
		{
			localAllocator->ForceProcessFreeList();
			auto index = std::distance( m_allocators, localAllocator );
			m_threadIdDictionary[ index ] = 0;
		}
	}

	void LocklessSlabAllocator::BuildMetrics( LocklessSlabAllocatorMetrics & metrics )
	{
		m_slabChunkAllocator.BuildMetrics( metrics );
		metrics.metrics.smallestBlockSize = c_slabMinAllocSize;
		metrics.metrics.largestBlockSize = c_slabMaxAllocSize;

		u64 waste = 0;
		u64 systemMemory = 0;

		for( u32 index = 0; index != c_maxThreadCount; ++index )
		{
			auto & threadCacheInfo = metrics.threadCacheInfo[ index ];
			threadCacheInfo.threadId = m_threadIdDictionary[ index ];
			m_allocators[ index ].BuildMetrics( threadCacheInfo.slabAllocatorInfo );
			metrics.metrics.consumedMemoryBytes += threadCacheInfo.slabAllocatorInfo.metrics.consumedMemoryBytes;
			waste += ( threadCacheInfo.slabAllocatorInfo.metrics.consumedSystemMemoryBytes - threadCacheInfo.slabAllocatorInfo.metrics.consumedMemoryBytes );
			systemMemory += threadCacheInfo.slabAllocatorInfo.metrics.consumedSystemMemoryBytes;
		}

		metrics.waste = waste;
		metrics.wastePercent = ( waste / static_cast< double >( systemMemory ) ) * 100.0;
	}

	SlabAllocator * LocklessSlabAllocator::InternalAcquireLocalAllocator( ThreadId id )
	{
		return AcquireSlabAllocator( id );
	}

	void LocklessSlabAllocator::InternalRegisterThread( ThreadId id )
	{
		RegisterThread( id );
	}

	void LocklessSlabAllocator::InternalSetThreadIdProvider( const ThreadIdProvider * provider )
	{
		m_threadIdProvider = provider;
	}

	SlabChunk * LocklessSlabAllocator::InternalAllocateChunk()
	{
		return m_slabChunkAllocator.AllocateChunk();
	}

	void LocklessSlabAllocator::InternalSetFreeChunk( ThreadId id, SlabChunk * chunk )
	{
		SlabAllocator * localAllocator = AcquireSlabAllocator( id );
		const u64 index = std::distance( m_allocators, localAllocator ); 
		m_slabChunkAllocator.ForceFree( static_cast< u32 >( index ), chunk );
	}

	void LocklessSlabAllocator::InternalMarkAllLocalSlabAllocatorAsTaken()
	{
		for( u32 index = 0; index != c_maxThreadCount; ++index )
		{
			if( m_threadIdDictionary[ index ] == 0 )
			{
				m_threadIdDictionary[ index ] = 0xcfcf;
			}
		}
	}

	bool LocklessSlabAllocator::InternalIsThreadBoundToALocalSlabAllocator( ThreadId id ) const
	{
		return std::find( m_threadIdDictionary.begin(), m_threadIdDictionary.end(), id ) != m_threadIdDictionary.end();
	}
}
}
