/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_ENGINE_POSE_PROVIDER_FACTORY_H_
#define _RED_ENGINE_POSE_PROVIDER_FACTORY_H_

#include "poseTypes.h"

class CPoseProviderFactory
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_PoseManagement );

public:

	RED_MOCKABLE ~CPoseProviderFactory();

	RED_MOCKABLE PoseProviderHandle CreatePoseProvider( const CSkeleton * skeleton ) const;

	void SetInternalPosePool( PosePoolHandle pool );
	
private:

	PosePoolHandle m_pool;
};

Red::TUniquePtr< CPoseProviderFactory > CreatePoseProviderFactory();

#endif
