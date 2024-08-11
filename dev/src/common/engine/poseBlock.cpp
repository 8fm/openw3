/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "poseBlock.h"
#include "behaviorGraphOutput.h"
#include "animationEvent.h"
#include "skeleton.h"

CPoseBlock::CPoseBlock()
	:	m_poses( nullptr ),
		m_events( nullptr ),
		m_poseCount( 0 ),
		m_poseAvailable( 0 )
{}

CPoseBlock::~CPoseBlock()
{}

CPoseHandle CPoseBlock::TakePose()
{
	for( Uint32 index = 0; index != m_poseCount; ++index )
	{
		SBehaviorGraphOutput * pose = m_poses + index;
		if( pose->m_refCount.CompareExchange( 1, 0 ) == 0 )
		{
			// Pose as been reserved to you!
			m_poseAvailable.Decrement();

			// Mini Hack. Pose can have reallocated it's internal event buffer in very rare occasion.
			// To make sure, we don't keep that memory for a very long time, release it, and give back original buffer.
			if( pose->m_ownEventFiredMemory )
			{
				pose->SetEventFiredMemory( m_events + ( index * c_eventFiredDefaultCount ) );
			}

			return CPoseHandle( pose, this );
		}
	}

	return CPoseHandle();
}

void CPoseBlock::SetPoses( Uint32 poseCount, SBehaviorGraphOutput * poses, CAnimationEventFired * events )
{
	m_poseAvailable.SetValue( poseCount );
	m_poseCount = poseCount;
	m_poses = poses;
	m_events = events;
}

void CPoseBlock::Initialize( const CSkeleton * skeleton, Red::UniqueBuffer & boneBuffer, Red::UniqueBuffer & trackBuffer )
{
	m_boneBuffer = std::move( boneBuffer );
	m_floatTrackBuffer = std::move( trackBuffer );

	const Uint32 boneCount = skeleton->GetBonesNum();
	const Uint32 trackCount = skeleton->GetTracksNum();
	AnimQsTransform * bones = static_cast< AnimQsTransform* >( m_boneBuffer.Get() );
	AnimFloat * floatTracks = static_cast< AnimFloat* >( m_floatTrackBuffer.Get() );

	for( Uint32 index = 0; index != m_poseCount; ++index )
	{
		SBehaviorGraphOutput * pose = m_poses + index;
		CAnimationEventFired * events = m_events + ( index * c_eventFiredDefaultCount ); 

		RED_WARNING( pose->m_refCount.GetValue() == 0, "A CPoseHandle was leaked. A Skeleton is gone, but there still pose out there bound to that Skeleton!" );	
		
		struct SBehaviorGraphOutputParameter param = 
		{
			boneCount,
			trackCount,
			bones ? bones + ( boneCount * index ) : nullptr,
			floatTracks ? floatTracks + ( trackCount * index ) : nullptr,
			events,
			false
		};
		
		pose->Init( param );
		pose->Reset( skeleton );
	}
}

Uint32 CPoseBlock::GetMemoryUsage() const
{
	return m_boneBuffer.GetSize() + m_floatTrackBuffer.GetSize() + ( m_poseCount * sizeof( SBehaviorGraphOutput ) ) + ( m_poseCount * sizeof( SBehaviorGraphOutput ) ); 
}

Uint32 CPoseBlock::GetMemoryConsumed() const
{
	if( m_poseCount )
	{
		return ( GetMemoryUsage() / m_poseCount ) * ( m_poseCount - m_poseAvailable.GetValue() );
	}

	return 0;
}

Red::UniqueBuffer CPoseBlock::ReleaseBoneBuffer()
{
	return std::move( m_boneBuffer );
}	

Red::UniqueBuffer CPoseBlock::ReleaseFloatTrackBuffer()
{
	return std::move( m_floatTrackBuffer );
}
