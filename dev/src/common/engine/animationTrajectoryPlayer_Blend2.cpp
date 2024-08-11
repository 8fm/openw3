
#include "build.h"
#include "animationTrajectoryPlayer_Blend2.h"
#include "animationTrajectoryPlayer.h"
#include "animationTrajectoryVisualizer.h"
#include "../core/mathUtils.h"
#include "game.h"
#include "renderFrame.h"
#include "actorInterface.h"
#include "entity.h"

AnimationTrajectoryPlayer_Blend2::AnimationTrajectoryPlayer_Blend2( const SAnimationTrajectoryPlayerToken& token, const CAnimatedComponent* component )
	: m_pointWasChanged( false )
	, m_weight( 1.f )
	, hack_lastIndex( -1 )
{
	ASSERT( token.m_isValid );
	ASSERT( token.m_animationA );
	ASSERT( token.m_animationB );
	ASSERT( token.m_trajectoryParamA );
	ASSERT( token.m_trajectoryParamB );
	ASSERT( token.m_weightA >= 0.f && token.m_weightA <= 1.f );

	m_animationStateA.m_animation = token.m_animationA->GetName();
	m_animationStateA.m_currTime = 0.f;
	m_animationStateA.m_prevTime = 0.f;

	m_animationStateB.m_animation = token.m_animationB->GetName();
	m_animationStateB.m_currTime = 0.f;
	m_animationStateB.m_prevTime = 0.f;

	m_animationWeight = token.m_weightA;

	component->GetLocalToWorld( m_localToWorld );

	m_trajectorySrcA = &(token.m_trajectoryParamA->GetDataR());
	m_trajectorySrcB = &(token.m_trajectoryParamB->GetDataR());

	token.m_trajectoryParamA->GetSyncPointRightMS( m_trajectorySyncPointA );
	token.m_trajectoryParamB->GetSyncPointRightMS( m_trajectorySyncPointB );

	const Uint32 trajectorySize = m_trajectorySrcA->m_pointsMSO.Size();
	m_trajectoryLS.Resize( trajectorySize );
	m_trajectoryMS.Resize( trajectorySize );
	m_trajectoryWS.Resize( trajectorySize );

	RefreshTrajectoryDatas();

	m_timeController.Set( m_trajectorySrcA->GetSyncPointTime(), token.m_timeFactor, token.m_syncPointDuration, token.m_duration, token.m_trajectoryParamA );

	m_duration = token.m_animationA->GetDuration();
	m_blendIn = token.m_blendIn;
	m_blendOut = token.m_blendOut;
	m_proxy = token.m_proxy;
	m_proxySyncType = token.m_proxySyncType;

	ASSERT( m_weight == 1.f );
}

AnimationTrajectoryPlayer_Blend2::~AnimationTrajectoryPlayer_Blend2()
{

}

void AnimationTrajectoryPlayer_Blend2::RefreshTrajectoryDatas()
{
	const Uint32 size = m_trajectoryLS.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		m_trajectoryLS[ i ] = Vector::Interpolate( m_trajectorySrcA->m_pointsLSO[ i ], m_trajectorySrcB->m_pointsLSO[ i ], m_animationWeight );
		m_trajectoryMS[ i ] = Vector::Interpolate( m_trajectorySrcA->m_pointsMSO[ i ], m_trajectorySrcB->m_pointsMSO[ i ], m_animationWeight );
		m_trajectoryWS[ i ] = m_localToWorld.TransformPoint( m_trajectoryMS[ i ] );
	}
}

Bool AnimationTrajectoryPlayer_Blend2::UpdatePoint( const Matrix& l2w, const Vector& pointWS )
{
	const Vector pointA = l2w.TransformPoint( m_trajectorySyncPointA );
	const Vector pointB = l2w.TransformPoint( m_trajectorySyncPointB );

	const Float p = MathUtils::GeometryUtils::ProjectVecOnEdgeUnclamped( pointWS, pointA, pointB );
	if ( p >= 0.f && p <= 1.f )
	{
		m_localToWorld = l2w;
		m_animationWeight = p;

		RefreshTrajectoryDatas();

		return true;
	}
	else
	{
		// TEMP for now

		m_localToWorld = l2w;
		m_animationWeight = Clamp( p, 0.f, 1.f );

		RefreshTrajectoryDatas();

		return false;
	}
}

Bool AnimationTrajectoryPlayer_Blend2::Update( Float& dt )
{
	const Float time = m_timeController.Update( dt );

	m_animationStateA.m_prevTime = m_animationStateA.m_currTime;
	m_animationStateA.m_currTime = time;

	m_animationStateB.m_prevTime = m_animationStateB.m_currTime;
	m_animationStateB.m_currTime = time;

	m_weight = 1.f;

	if ( m_blendIn > 0.f )
	{
		if ( m_proxySyncType == AMAST_CrossBlendIn )
		{
			CActionMoveAnimationProxy* proxy = m_proxy.Get();
			if ( proxy )
			{
				const Float currTime = proxy->m_currTime;
				const Float duration = proxy->m_duration;

				const Float startTime = Max( 0.f, duration - m_blendIn );
				if ( currTime > startTime )
				{
					const Float w = ( currTime - startTime ) / m_blendIn;
					ASSERT( w >= 0.f && w <= 1.f );

					m_weight = w;
				}
			}
		}
		else if ( m_animationStateA.m_currTime < m_blendIn )
		{
			const Float w = m_animationStateA.m_currTime / m_blendIn;
			ASSERT( w >= 0.f && w <= 1.f );

			m_weight = w;
		}
	}
	else if ( m_blendOut > 0.f )
	{
		/*if ( m_proxySyncType == AMAST_CrossBlendOut )
		{
			CActionMoveAnimationProxy* proxy = m_proxy.Get();
			if ( proxy )
			{
				const Float currTime = proxy->m_currTime;
				const Float duration = proxy->m_duration;
			}
		}*/

		const Float blendOutStart = Max( 0.f, m_duration - m_blendOut );

		if ( m_animationStateA.m_currTime > blendOutStart )
		{
			const Float w = 1.f - ( m_animationStateA.m_currTime - blendOutStart ) / m_blendOut;
			ASSERT( w >= 0.f && w <= 1.f );

			m_weight = w;
		}
	}

	HACK_SendTrajectoryToPlayer();

	return m_animationStateA.m_currTime >= m_duration;
}

Bool AnimationTrajectoryPlayer_Blend2::PlayAnimationOnSlot( CBehaviorManualSlotInterface& slot )
{
	return slot.IsValid() ? slot.PlayAnimations( m_animationStateA, m_animationStateB, m_animationWeight, m_weight ) : true;
}

Float AnimationTrajectoryPlayer_Blend2::GetTime() const
{
	return m_timeController.GetTime();
}

Bool AnimationTrajectoryPlayer_Blend2::IsBeforeSyncTime() const
{
	return m_timeController.IsBeforeSyncTime();
}

void AnimationTrajectoryPlayer_Blend2::GenerateFragments( CRenderFrame* frame ) const
{
	static Bool RENDER = false;
	if ( RENDER )
	{
		if ( m_trajectorySrcA )
		{
			AnimationTrajectoryVisualizer::DrawTrajectoryMSinWSWithPtrO( frame, *m_trajectorySrcA, m_localToWorld, m_animationStateA.m_currTime, m_duration );

			//AnimationTrajectoryVisualizer::InternalDrawTrajectoryWSWithPtr( frame, m_trajectoryLS, m_trajectoryMS, m_trajectorySrcA->m_syncFrame, m_localToWorld, m_animationStateA.m_currTime, m_duration, Color( 255, 128, 128 ) );
		}

		if ( m_trajectorySrcB )
		{
			AnimationTrajectoryVisualizer::DrawTrajectoryMSinWSWithPtrO( frame, *m_trajectorySrcB, m_localToWorld, m_animationStateB.m_currTime, m_duration );
		}
	}

	const Uint32 trajectorySize = m_trajectoryWS.Size();
	if ( trajectorySize > 0 )
	{
		Color color( 255, 128, 128 );

		Vector prev = m_trajectoryWS[ 0 ];
		for ( Uint32 i=1; i<trajectorySize; ++i )
		{
			Vector point = m_trajectoryWS[ i ];

			frame->AddDebugLine( prev, point, color, false );

			prev = point;
		}

		if ( m_trajectorySrcA )
		{
			ASSERT( m_trajectoryWS.Size() > m_trajectorySrcA->m_syncFrame );
			const Vector syncPos = m_trajectoryWS[ m_trajectorySrcA->m_syncFrame ];
			frame->AddDebugSphere( syncPos, 0.08f, Matrix::IDENTITY, Color( 255, 0, 0 ) );
		}
	}

	frame->AddDebugSphere( m_pointWS, 0.1f, Matrix::IDENTITY, Color( 255, 0, 0 ) );
}

void AnimationTrajectoryPlayer_Blend2::HACK_SendTrajectoryToPlayer()
{
	IActorInterface* a = GGame->GetPlayerEntity()->QueryActorInterface();
	if ( a && m_trajectorySrcA && !IsBeforeSyncTime() )
	{
		if ( hack_lastIndex == -1 )
		{
			const Float time = m_trajectorySrcA->GetSyncPointTime();

			Int32 frameA, frameB;
			Float w;
			if ( m_trajectorySrcA->FindKeys( time, frameA, frameB, w ) )
			{
				hack_lastIndex = Max( frameA-2, 0 );
				hack_lastPoint = m_trajectoryWS[ hack_lastIndex ];
			}
		}

		if ( hack_lastIndex != -1 )
		{
			const Float time = GetTime();

			Int32 frameA, frameB;
			Float w;
			if ( m_trajectorySrcA->FindKeys( time, frameA, frameB, w ) )
			{
				if ( frameB > hack_lastIndex )
				{
					const Int32 minIdx = hack_lastIndex;
					const Int32 maxIdx = frameB;
					const Int32 size = maxIdx - minIdx + 1;

					ASSERT( size > 0 );

					TDynArray< Vector > dataWS;
					dataWS.Resize( size );

					dataWS[ 0 ] = hack_lastPoint;

					for ( Int32 i=1; i<size; ++i )
					{
						dataWS[ i ] = m_trajectoryWS[ minIdx + i ];
					}

					a->Hack_SetSwordTrajectory( dataWS, w );

					hack_lastIndex = frameB;
					hack_lastPoint = m_trajectoryWS[ maxIdx ];
				}
			}
		}
	}
}
