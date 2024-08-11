/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_ENGINE_POSE_PROVIDER_H_
#define _RED_ENGINE_POSE_PROVIDER_H_

#include "poseHandle.h"
#include "poseTypes.h"
#include "../redThreads/readWriteSpinLock.h"

class CSkeleton;
class CPoseBlock;
struct SPoseProviderStats;

class CPoseProvider
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_PoseManagement );

public:
	CPoseProvider();
	RED_MOCKABLE ~CPoseProvider();

	// AcquirePose will always return a valid CPoseHandle. 
	// Worst case is that it will allocate a pose directly from default pool.
	CPoseHandle AcquirePose(); 
	
	RED_MOCKABLE void Sanitize();

	const CSkeleton * GetSkeleton() const;
	void GetStats( SPoseProviderStats & stats ) const;

	void SetInternalSkeleton( const CSkeleton * skeleton );
	void SetInternalPosePool( PosePoolHandle pool );
	void PushInternalPoseBlock( CPoseBlock * block );
	void ReleaseInternalPoseBlocks();

private:

	typedef TDynArray< CPoseBlock*, MC_PoseManagement > PoseBlockContainer;

	CPoseHandle FindAvailablePose();
	CPoseHandle CreateFallbackPose();
	RED_MOCKABLE CPoseHandle AllocateFallbackPose() const;
	void AcquirePoseBlock();

	const CSkeleton * m_skeleton;
	PosePoolHandle m_pool;
	PoseBlockContainer m_blockContainer;
	Red::Threads::CAtomic< Int32 > m_blockCount;

	mutable Red::Threads::CRWSpinLock m_lock;

#ifndef RED_FINAL_BUILD
	String m_skeletonDepotPath;
#endif
};

PoseProviderHandle CreatePoseProvider( const CSkeleton * skeleton, PosePoolHandle pool );

#endif
