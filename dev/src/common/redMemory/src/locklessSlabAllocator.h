/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_LOCKLESS_SLAB_ALLOCATOR_H_
#define _RED_MEMORY_LOCKLESS_SLAB_ALLOCATOR_H_

#include "../include/utils.h"

#include "allocator.h"
#include "slabAllocator.h"
#include "slabChunkAllocatorInterface.h"
#include "threadIdProvider.h"
#include "threadConstant.h"
#include "allocatorMetrics.h"

#include <array>

namespace red
{
namespace memory
{
	class SystemAllocator;
	class ThreadMonitor;

	struct LocklessSlabAllocatorMetrics
	{
		AllocatorMetrics metrics;

		u32 usedChunkCount;
		u32 freeChunkCount;
		u64 waste;
		double wastePercent;

		struct ThreadCacheMetric
		{
			u32 threadId;
			SlabAllocatorMetrics slabAllocatorInfo;
		};

		ThreadCacheMetric threadCacheInfo[ c_maxThreadCount ];
	};

	struct LocklessSlabAllocatorParameter
	{
		u64 virtualRangeSize;
		SystemAllocator * systemAllocator;
		ThreadMonitor * threadMonitor;
	};

	class RED_MEMORY_API LocklessSlabAllocator
	{
	public:

		RED_MEMORY_DECLARE_ALLOCATOR( LocklessSlabAllocator, 0x2EA16025, 8 );

		typedef u32 ThreadId;

		LocklessSlabAllocator();
		~LocklessSlabAllocator();

		void Initialize( const LocklessSlabAllocatorParameter & parameter );
		void Uninitialize();

		Block Allocate( u32 size );
		Block AllocateAligned( u32 size, u32 alignement );
		Block Reallocate( Block & block, u32 size );
		void Free( Block & block );

		bool OwnBlock( u64 block ) const;

		void RegisterCurrentThread();

		void BuildMetrics( LocklessSlabAllocatorMetrics & metrics );

		// UNIT TEST ONLY
		void InternalSetThreadIdProvider( const ThreadIdProvider * provider );
		SlabAllocator * InternalAcquireLocalAllocator( ThreadId id );
		void InternalRegisterThread( ThreadId id );
		SlabChunk * InternalAllocateChunk();
		void InternalSetFreeChunk( ThreadId id, SlabChunk * chunk );
		void InternalMarkAllLocalSlabAllocatorAsTaken();
		bool InternalIsThreadBoundToALocalSlabAllocator( ThreadId id ) const;

	private:

		class RED_MEMORY_API SlabChunkAllocator : public SlabChunkAllocatorInterface
		{
		public:
			SlabChunkAllocator();
			
			void Initialize();

			virtual SlabChunk * Allocate( u32 allocatorId ) override final;
			virtual void Free( u32 allocatorId, SlabChunk * chunk ) override final;

			void SetSystemAllocator( SystemAllocator * allocator );
			void SetNextMemoryAddress( u64 address );

			SlabChunk * AllocateChunk();
			void ForceFree( u32 allocatorId, SlabChunk * chunk );

			void BuildMetrics( LocklessSlabAllocatorMetrics & metrics );

		private:

			struct FreeChunks
			{
				SlabChunk * chunkList;
				CSpinLock lock;
			};

			SlabChunk * TryTakingFreeChunk( u32 id );

			FreeChunks m_freeChunks[ c_maxThreadCount ];
			SystemAllocator * m_systemAllocator;

			u64 m_nexChunkAddress;
			u64 m_firstChunkAddress;
			atomic::TAtomic32 m_chunkCount;
			atomic::TAtomic32 m_freeChunkCount;
			CSpinLock m_monitor; // m_nextChunkAddress need to be protected. Multiple thread might want a Chunk at same time.
		};

		typedef std::array< ThreadId, c_maxThreadCount > ThreadIdDictionary;

		LocklessSlabAllocator( const LocklessSlabAllocator& );
		LocklessSlabAllocator & operator=( const LocklessSlabAllocator& );

		SlabAllocator * AcquireLocalSlabAllocator();
		SlabAllocator * AcquireSlabAllocator( ThreadId id );
		SlabAllocator * GetSlabAllocator( ThreadId id );
		
		ThreadIdDictionary::iterator RegisterThread( ThreadId id );

		void OnThreadDied( ThreadId id );  
		static void NotifyOnThreadDied( ThreadId id, void * userData );
		
		RED_ALIGN( 64 ) ThreadIdDictionary m_threadIdDictionary;		
		RED_ALIGN( 64 ) SlabAllocator m_allocators[ c_maxThreadCount ];
	
		VirtualRange m_reservedMemoryRange;
		
		SystemAllocator * m_systemAllocator;
		SlabChunkAllocator m_slabChunkAllocator;

		const ThreadIdProvider * m_threadIdProvider; // For Unit Test only. Could be removed in final build.
		ThreadIdProvider m_threadIdProviderStorage;

		ThreadMonitor * m_threadMonitor;
	};
}
}

#include "locklessSlabAllocator.hpp"

#endif
