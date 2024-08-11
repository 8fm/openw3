/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "poseProviderFactory.h"
#include "poseProvider.h"
#include "posePool.h"

CPoseProviderFactory::~CPoseProviderFactory()
{}

PoseProviderHandle CPoseProviderFactory::CreatePoseProvider( const CSkeleton * skeleton ) const
{
	PoseProviderHandle provider = ::CreatePoseProvider( skeleton, m_pool );
	return provider;
}

void CPoseProviderFactory::SetInternalPosePool( PosePoolHandle pool )
{
	m_pool = pool;
}

Red::TUniquePtr< CPoseProviderFactory > CreatePoseProviderFactory()
{
	Red::TUniquePtr< CPoseProviderFactory > factory( new CPoseProviderFactory );
	PosePoolHandle pool = CreatePosePool();
	factory->SetInternalPosePool( pool );
	return factory;
}
