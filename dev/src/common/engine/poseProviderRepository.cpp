/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "poseProviderRepository.h"
#include "poseProvider.h"
#include "poseProviderFactory.h"
#include <map>
#include "skeleton.h"

const Uint32 c_maxPoseProvider = 512;

typedef Red::Threads::CScopedLock< Red::Threads::CRWSpinLock > ScopedWriteLock;
typedef Red::Threads::CScopedSharedLock< Red::Threads::CRWSpinLock > ScopedReadLock;


CPoseProviderRepository::CPoseProviderRepository()
	: m_factory( CreatePoseProviderFactory() )
{
	m_poseProviderContainer.Resize( c_maxPoseProvider );
}

CPoseProviderRepository::~CPoseProviderRepository()
{}

PoseProviderHandle CPoseProviderRepository::AcquireProvider( const CSkeleton* skeleton )
{
	RED_FATAL_ASSERT( skeleton, "CPoseProvider is bound to CSkeleton. It can't be nullptr" );
	
	Uint32 firstAvailableSpot = 0;
	PoseProviderHandle handle = FindProvider( skeleton, firstAvailableSpot );
	if( !handle )
	{
		return CreateProvider( skeleton, firstAvailableSpot );
	}	

	return handle;
}

PoseProviderHandle CPoseProviderRepository::FindProvider( const CSkeleton* skeleton, Uint32 & firstAvailableSpot ) const
{
	firstAvailableSpot = c_maxPoseProvider;

	ScopedReadLock scopedLock( m_lock ); // multiple thread can safely use this function

	for( Uint32 index = 0, end = m_poseProviderContainer.Size(); index != end; ++index )
	{
		InternalPoseProviderHandle internalHandle = m_poseProviderContainer[ index ]; 
		PoseProviderHandle handle = internalHandle.Lock();

		if( !handle )
		{
			firstAvailableSpot = Min( index, firstAvailableSpot );
		}
		else if( handle->GetSkeleton() == skeleton )
		{
			return handle;
		}
	}

	return PoseProviderHandle();
}

PoseProviderHandle CPoseProviderRepository::CreateProvider( const CSkeleton* skeleton, Uint32 availableSpot )
{
	PoseProviderHandle handle = m_factory->CreatePoseProvider( skeleton );

	// I'm going to write the storage. Make sure no read are ongoing. Don't worry, I'll make it fast. 
	ScopedWriteLock scopedLock( m_lock ); 
	
	// There is a slim chance that the spot I wanted got taken before me. Loop to find a new one.
	for( ; availableSpot != m_poseProviderContainer.Size(); ++availableSpot )
	{
		InternalPoseProviderHandle & internalHandle = m_poseProviderContainer[ availableSpot ];
		if( internalHandle.Expired() )
		{
			// Spot is still valid
			internalHandle = handle;
			return handle;
		}
	}

	// Worst case scenario, no spot found. Let's not crash, give a valid Provider anyway.
	// If this happen it is possible two provider for same  skeleton exist for brief short time.
	return handle; 
}

void CPoseProviderRepository::Sanitize()
{
	// I'm going to sanitize all provider. Do not create one while I'm working.
	ScopedWriteLock scopedLock( m_lock ); 
	for( Uint32 index = 0, end = m_poseProviderContainer.Size(); index != end; ++index )
	{
		InternalPoseProviderHandle internalHandle = m_poseProviderContainer[ index ]; 
		PoseProviderHandle handle = internalHandle.Lock();
		if( handle )
		{
			handle->Sanitize();
		}
	}
}

void CPoseProviderRepository::SetInternalProviderFactory( Red::TUniquePtr< CPoseProviderFactory > factory )
{
	m_factory = std::move( factory );
}
