/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_FRAMEWORK_GPU_ALLOCATOR_H
#define _RED_MEMORY_FRAMEWORK_GPU_ALLOCATOR_H
#pragma once

#include "../redSystem/types.h"
#include "redMemoryRegionAllocator.h"
#include "redMemoryListHelpers.h"

namespace Red { namespace MemoryFramework {

	namespace GpuAllocatorImpl
	{
		class RegionNode;
	}

	template< typename TSyncLock >
	class GpuAllocator : public RegionAllocator 
	{
	public:
		GpuAllocator();
		virtual ~GpuAllocator();

		class CreationParameters;
		virtual EAllocatorInitResults Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags );
		virtual void Release( );

		virtual Red::System::Bool IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired );
		virtual Red::System::MemSize ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback );

		virtual void RequestAllocatorInfo( AllocatorInfo& info );
		virtual void WalkAllocator( AllocatorWalker* theWalker );
		virtual void WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker );
		virtual void DumpDebugOutput();
		virtual void OnOutOfMemory();
		virtual MemoryRegionHandle RuntimeAllocateRegion( Red::System::MemSize size, Red::System::MemSize alignment, RegionLifetimeHint lifetimeHint ) override;
		virtual EAllocatorFreeResults RuntimeFreeRegion( MemoryRegionHandle handle ) override;
		virtual MemoryRegionHandle RuntimeSplitRegion( MemoryRegionHandle baseRegion, Red::System::MemSize splitPosition, Red::System::MemSize splitBlockAlignment ) override;

		MemoryRegionHandle AllocateRegion( Red::System::MemSize size, Red::System::MemSize alignment, RegionLifetimeHint lifetimeHint );
		EAllocatorFreeResults FreeRegion( MemoryRegionHandle handle );
		MemoryRegionHandle SplitRegion( MemoryRegionHandle baseRegion, Red::System::MemSize splitPosition, Red::System::MemSize splitBlockAlignment );
		void UnlockRegion( MemoryRegionHandle handle );		// Regions can only be defragged or freed once they are unlocked. Synchronisation based on allocator lock
		void LockRegion( MemoryRegionHandle handle );		// Regions can only be defragged or freed once they are unlocked. Synchronisation based on allocator lock

		// Defragmentation
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
			// May also make sense to have a "freeSpaceLimit", and not shift data to fill empty spaces bigger than that.
			// Can then spend more time filling small holes rather than moving large holes to the end.
		};

		// Request defrag. This locks the allocator, patches the regions, and fires callbacks of 'move' commands the external user MUST commit before FinaliseDefragmentation is called
		Red::System::MemUint RequestDefragmentation( DefragMode defragMode, const DefragSettings& settings );

		// Finalise defrag. Checks internal state, and unlocks the allocator for use again
		void FinaliseDefragmentation();

		// Call after all memory operations have finished for a defrag. Should be called as early as possible, and definitely
		// before the next time the pool is defragmented.
		void AfterDefragmentationMemoryFinished( DefragMode defragMode );

	private:
		void InitialiseRegionPool();
		MemoryRegionHandle AllocateRegionBottom( Red::System::MemSize size, Red::System::MemSize alignment );
		MemoryRegionHandle AllocateRegionTop( Red::System::MemSize size, Red::System::MemSize alignment );
		EAllocatorFreeResults FreeRegionBottom( GpuAllocatorImpl::RegionNode* theNode );
		EAllocatorFreeResults FreeRegionTop( GpuAllocatorImpl::RegionNode* theNode );

		GpuAllocatorImpl::RegionNode* FindEmptyNodeInList( Red::System::MemSize size, Red::System::MemSize alignment, GpuAllocatorImpl::RegionNode* head, GpuAllocatorImpl::RegionNode* tail );
		Bool NodeCanFitRequest( GpuAllocatorImpl::RegionNode* node, Red::System::MemSize size, Red::System::MemSize alignment );
		Red::System::MemSize GetUnusedAvailable() const;

		GpuAllocatorImpl::RegionNode* MakeFreeRegion( Red::System::MemUint address, Red::System::MemSize size, Bool initiallyLocked );
		void ReleaseFreeRegion( GpuAllocatorImpl::RegionNode* node );

		GpuAllocatorImpl::RegionNode* SplitNode( GpuAllocatorImpl::RegionNode*& listHead, GpuAllocatorImpl::RegionNode*& listTail, GpuAllocatorImpl::RegionNode* sourceNode, Red::System::MemSize offset );
		GpuAllocatorImpl::RegionNode* PrepareAllocatedNode( GpuAllocatorImpl::RegionNode*& listHead, GpuAllocatorImpl::RegionNode*& listTail, GpuAllocatorImpl::RegionNode* theNode,  Red::System::MemSize size, Red::System::MemSize alignment );
		GpuAllocatorImpl::RegionNode* CoalesceFreeNode( GpuAllocatorImpl::RegionNode*& head, GpuAllocatorImpl::RegionNode*& tail, GpuAllocatorImpl::RegionNode* theNode );

		Uint32 DefragmentBottomRegions( const DefragSettings& settings, Red::System::MemUint& recommendedStartingPoint );
		Uint32 DefragmentTopRegions( const DefragSettings& settings, Red::System::MemUint& recommendedStartingPoint );

		void* m_gpuMemoryBlock;
		void* m_cpuMemoryBlock;

		// Pool of free region headers. !NOTE! These are NOT free areas of memory, this is simply a pool of structures used for bookmarking
		GpuAllocatorImpl::RegionNode* m_freeRegionHead;
		GpuAllocatorImpl::RegionNode* m_freeRegionTail;

		// Bottom List (Low addresses, grow forwards). Represents physical layout, nodes are linked in address order
		GpuAllocatorImpl::RegionNode* m_bottomPhysicalHead;
		GpuAllocatorImpl::RegionNode* m_bottomPhysicalTail;

		// Top List (High addresses, grow backwards). Represents physical layout, nodes are linked in address order
		GpuAllocatorImpl::RegionNode* m_topPhysicalHead;
		GpuAllocatorImpl::RegionNode* m_topPhysicalTail;

		// Fragmentation counter, simply to stop simple scoping problems
		Int32 m_fragmentationScope;

		CreationParameters m_creationParameters;
		mutable TSyncLock m_syncLock;
	};

	template< typename TSyncLock >
	class GpuAllocator< TSyncLock >::CreationParameters : public IAllocatorCreationParameters
	{
	public:
		CreationParameters()
			: m_gpuBudget( 0 ), m_cpuBudget( 0 ), m_gpuFlags( 0 ), m_cpuFlags( 0 )
		{
		}
		CreationParameters( Red::System::MemSize gpuBudget, Red::System::MemSize cpuBudget, Red::System::Uint32 gpuFlags, Red::System::Uint32 cpuFlags )
			: m_gpuBudget( gpuBudget )
			, m_cpuBudget( cpuBudget )
			, m_gpuFlags( gpuFlags )
			, m_cpuFlags( cpuFlags )
		{
		}
		virtual ~CreationParameters()	{ }

		Red::System::MemSize m_gpuBudget;
		Red::System::MemSize m_cpuBudget;
		Red::System::Uint32 m_gpuFlags;
		Red::System::Uint32 m_cpuFlags;
	};
} }

#endif