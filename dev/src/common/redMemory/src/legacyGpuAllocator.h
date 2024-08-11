/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_LEGACY_GPU_ALLOCATOR_H_
#define _RED_MEMORY_LEGACY_GPU_ALLOCATOR_H_

#include "virtualRange.h"
#include "block.h"
#include "legacyGpuAllocatorRegion.h"
#include "allocator.h"

namespace red 
{ 
namespace memory 
{
	class SystemAllocator;

	enum RegionLifetimeHint
	{
		Region_Shortlived,
		Region_Longlived
	};

	struct LegacyGpuAllocatorParameter
	{
		SystemAllocator * systemAllocator;
		u64 gpuSize;
		u32 gpuFlags;
		u64 cpuSize;
	};

	class LegacyGpuAllocator
	{
	public:

		RED_MEMORY_DECLARE_ALLOCATOR( LegacyGpuAllocator, 0xEBF19F0A, 16 );

		enum DefragMode
		{
			DefragShortLivedRegions,
			DefragLongLivedRegions
		};

		// Move request - source address, source size, target address, user data. Note these may overlap!
		typedef void (*DefragMoveRequestCallback)( void*, void*, size_t, void* );

		struct DefragSettings
		{
			DefragMoveRequestCallback	moveRequestCb;
			void*						userData;
			Red::System::MemUint		startingPoint;		// 0 or returned value from previous RequestDefragmentation
			Uint32						maxMemoryToMove;
			Uint32						blockSizeLimit;
		};


		LegacyGpuAllocator();
		~LegacyGpuAllocator();

		void Initialize( const LegacyGpuAllocatorParameter & param );
		void Uninitialize();

		Block Allocate( u32 size );
		Block AllocateAligned( u32 size, u32 alignment );
		Block Reallocate( Block & block, u32 size );
		void Free( Block & block );
		u64 GetBlockSize( u64 address ) const;

		RegionHandle AllocateRegion( u64 size, u64 alignment, RegionLifetimeHint lifetimeHint );
		void FreeRegion( RegionHandle handle );
		RegionHandle SplitRegion( RegionHandle baseRegion, u64 splitPosition, u64 splitBlockAlignment );
		void UnlockRegion( RegionHandle handle );		// Regions can only be defragged or freed once they are unlocked. Synchronisation based on allocator lock
		void LockRegion( RegionHandle handle );		// Regions can only be defragged or freed once they are unlocked. Synchronisation based on allocator lock

		
		// Request defrag. This locks the allocator, patches the regions, and fires callbacks of 'move' commands the external user MUST commit before FinaliseDefragmentation is called
		Red::System::MemUint RequestDefragmentation( DefragMode defragMode, const DefragSettings& settings );

		// Finalise defrag. Checks internal state, and unlocks the allocator for use again
		void FinaliseDefragmentation();

	private:

		class RegionNode;

		LegacyGpuAllocator( const LegacyGpuAllocator& );
		LegacyGpuAllocator & operator=( const LegacyGpuAllocator& );

		void InitialiseRegionPool();
		RegionHandle AllocateRegionBottom( u64 size, u64 alignment );
		RegionHandle AllocateRegionTop( u64 size, u64 alignment );
		void FreeRegionBottom( RegionNode* theNode );
		void FreeRegionTop( RegionNode* theNode );

		RegionNode* FindEmptyNodeInList( u64 size, u64 alignment, RegionNode* head, RegionNode* tail );
		Bool NodeCanFitRequest( RegionNode* node, u64 size, u64 alignment );
		u64 GetUnusedAvailable() const;

		RegionNode* MakeFreeRegion( red::MemUint address, u64 size );
		void ReleaseFreeRegion( RegionNode* node );

		RegionNode* SplitNode( RegionNode*& listHead, RegionNode*& listTail, RegionNode* sourceNode, u64 offset );
		RegionNode* PrepareAllocatedNode( RegionNode*& listHead, RegionNode*& listTail, RegionNode* theNode,  u64 size, u64 alignment );
		RegionNode* CoalesceFreeNode( RegionNode*& head, RegionNode*& tail, RegionNode* theNode );

		u32 DefragmentBottomRegions( u32 maxMemoryToMove, DefragMoveRequestCallback userCallback, void* userData, u32 smallestGapToFill );
		u32 DefragmentTopRegions( u32 maxMemoryToMove, DefragMoveRequestCallback userCallback, void* userData, u32 smallestGapToFill );

		void* m_gpuMemoryBlock;
		void* m_cpuMemoryBlock;

		// Pool of free region headers. !NOTE! These are NOT free areas of memory, this is simply a pool of structures used for bookmarking
		RegionNode* m_freeRegionHead;
		RegionNode* m_freeRegionTail;

		// Bottom List (Low addresses, grow forwards). Represents physical layout, nodes are linked in address order
		RegionNode* m_bottomPhysicalHead;
		RegionNode* m_bottomPhysicalTail;

		// Top List (High addresses, grow backwards). Represents physical layout, nodes are linked in address order
		RegionNode* m_topPhysicalHead;
		RegionNode* m_topPhysicalTail;

		// Fragmentation counter, simply to stop simple scoping problems
		i32 m_fragmentationScope;

		mutable CMutex m_syncLock;
	
		VirtualRange m_gpuRange;
		VirtualRange m_cpuRange;

		SystemAllocator * m_systemAllocator;
	};
} 
}

#endif
