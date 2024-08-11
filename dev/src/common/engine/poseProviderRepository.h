/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_POSE_PROVIDER_REPOSITORY_H_
#define _RED_POSE_PROVIDER_REPOSITORY_H_

#include "../redThreads/readWriteSpinLock.h"
#include "../core/uniquePtr.h"
#include "../core/atomicWeakPtr.h"

#include "poseTypes.h"

class CPoseProviderFactory;

class CPoseProviderRepository
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_PoseManagement );

public:

	CPoseProviderRepository();
	~CPoseProviderRepository();

	PoseProviderHandle AcquireProvider( const CSkeleton* skeleton );

	void Sanitize();

	void SetInternalProviderFactory( Red::TUniquePtr< CPoseProviderFactory > factory ); // For unit test only

private:

	typedef Red::TAtomicWeakPtr< CPoseProvider > InternalPoseProviderHandle;
	typedef TDynArray< InternalPoseProviderHandle, MC_PoseManagement > PoseProviderContainer;
	
	PoseProviderHandle FindProvider( const CSkeleton* skeleton, Uint32 & firstAvailableSpot ) const;
	PoseProviderHandle CreateProvider( const CSkeleton* skeleton, Uint32 firstAvailableSpot );

	PoseProviderContainer m_poseProviderContainer;
	Red::TUniquePtr< CPoseProviderFactory > m_factory;

	mutable Red::Threads::CRWSpinLock m_lock;
};

#endif
