
#include "build.h"

#include "../../common/engine/behaviorGraphStack.h"

#include "expBlendExecutor.h"
#include "expEvents.h"
#include "expIntarface.h"

ExpBlendExecutor::ExpBlendExecutor( const ExecutorSetup& setup, const CName& anim1, const CName& anim2, Float weight, Float blendIn, Float blendOut, Float earlyEndOffset )
	: ExpBaseExecutor( setup, blendIn, blendOut, earlyEndOffset )
{
	m_animationEntry[0] = FindAnimation( setup.m_entity, anim1 );
	if ( m_animationEntry[0] )
	{
		m_duration = m_animationEntry[0]->GetDuration();
		m_animationStates[0].m_animation = m_animationEntry[0]->GetName();
		m_animationStates[0].m_currTime = 0.f;
		m_animationStates[0].m_prevTime = 0.f;
	}

	m_animationEntry[1] = FindAnimation( setup.m_entity, anim2 );
	if ( m_animationEntry[1] )
	{
		// For now take the longest animation duration, in future we might scale the animation time
		const Float dur = m_animationEntry[1]->GetDuration();
		if( dur > m_duration )
		{
			m_duration = dur;
		}
		m_animationStates[1].m_animation = m_animationEntry[1]->GetName();
		m_animationStates[1].m_currTime = 0.f;
		m_animationStates[1].m_prevTime = 0.f;
	}
}

ExpBlendExecutor::~ExpBlendExecutor()
{

}

void ExpBlendExecutor::SyncAnim( Float time )
{
	Float marker;
	WrapTime( time, marker );

	m_animationStates[0].m_currTime = m_animationStates[1].m_currTime = time;
	m_animationStates[0].m_prevTime = m_animationStates[1].m_currTime = time;
	m_slot.PlayAnimations( m_animationStates[0], m_animationStates[1], m_blendWeight, CalcWeight( time ) );
}

Bool ExpBlendExecutor::UpdateAnimation( Float dt, Float& timeRest, ExpExecutorUpdateResult& result )
{
	m_animationStates[0].m_prevTime = m_animationStates[0].m_currTime;
	m_animationStates[0].m_currTime += m_timeMul * dt;

	if ( m_firstUpdate && m_animationStates[0].m_currTime != 0.f )
	{
		m_firstUpdate = false;

		result.AddNotification( ENT_AnimationStarted, m_animationStates[0].m_animation );
		result.AddNotification( ENT_AnimationStarted, m_animationStates[1].m_animation );
	}

	Float marker1, marker2;
	WrapTime( m_animationStates[0].m_prevTime, marker1 );
	WrapTime( m_animationStates[0].m_currTime, marker2 );


	AlignPreTimeToCurr( m_animationStates[0].m_prevTime, m_animationStates[0].m_currTime );

	if ( m_slot.IsValid() )
	{
		ASSERT( m_animationStates[0].m_prevTime >= 0.f );
		ASSERT( m_animationStates[0].m_currTime >= 0.f );

		m_animationStates[1].m_prevTime = m_animationStates[0].m_prevTime;
		m_animationStates[1].m_currTime = m_animationStates[0].m_currTime;

		m_slot.PlayAnimations( m_animationStates[0], m_animationStates[1], m_blendWeight, CalcWeight( m_animationStates[0].m_currTime ) );
		
		if ( m_animationEntry[0] )
		{
			CollectEvents( m_animationStates[0].m_prevTime, m_animationStates[0].m_currTime, result );
		}
	}

	UnwrapTime( m_animationStates[0].m_prevTime, marker1 );
	UnwrapTime( m_animationStates[0].m_currTime, marker2 );

	const Float absTime = MAbs( m_animationStates[0].m_currTime );
	if ( absTime >= m_duration )
	{
		timeRest = absTime - m_duration;

		return true;
	}

	return false;
}

void ExpBlendExecutor::CollectEvents( Float prev, Float curr, ExpExecutorUpdateResult& result ) const
{
	ASSERT( m_animationEntry[0] );
	ASSERT( m_animationEntry[1] );
	
	TDynArray< CAnimationEventFired > events;

	for( int i = 0; i < 2; ++i )
	{
		events.ClearFast();
		m_animationEntry[i]->GetEventsByTime( prev, curr, 0, 1.f, &events, NULL );

		const Uint32 size = events.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const CAnimationEventFired& evt = events[ i ];

			if ( IsType< CExpSyncEvent >( evt.m_extEvent ) )
			{
				const CExpSyncEvent* syncEvt = static_cast< const CExpSyncEvent* >( evt.m_extEvent );

				//...

				result.AddNotification( ENT_Event, syncEvt->GetEventName() );
			}
		}
	}
}