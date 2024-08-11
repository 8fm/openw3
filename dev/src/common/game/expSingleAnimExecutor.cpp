
#include "build.h"
#include "expEvents.h"
#include "expSingleAnimExecutor.h"

ExpSingleAnimExecutor::ExpSingleAnimExecutor( const ExecutorSetup& setup, const CName& animName, Float blendIn /*= 0.f*/, Float blendOut /*= 0.f */, Float earlyEndOffset /*= 0.f */ )
	: ExpBaseExecutor( setup, blendIn, blendOut, earlyEndOffset )
	, m_animationEntry( NULL )
	, m_endWhenBlendingOut( false )
{
	m_animationEntry = FindAnimation( setup.m_entity, animName );
	if ( m_animationEntry )
	{
		if ( m_animationEntry->GetAnimation() )
		{
			m_animationEntry->GetAnimation()->AddUsage();
		}
		m_duration = m_animationEntry->GetDuration();
		m_animationState.m_animation = m_animationEntry->GetName();
	}

	m_animationState.m_currTime = 0.f;
	m_animationState.m_prevTime = 0.f;
}

ExpSingleAnimExecutor::~ExpSingleAnimExecutor()
{
	if ( m_animationEntry )
	{
		if ( m_animationEntry->GetAnimation() )
		{
			m_animationEntry->GetAnimation()->ReleaseUsage();
		}
	}
}

void ExpSingleAnimExecutor::SyncAnim( Float time )
{
	Float marker;
	WrapTime( time, marker );

	m_animationState.m_currTime = time;
	m_animationState.m_prevTime = time;
	m_slot.PlayAnimation( m_animationState, CalcWeight( time ) );
}

Bool ExpSingleAnimExecutor::UpdateAnimation( Float dt, Float& timeRest, ExpExecutorUpdateResult& result )
{
	m_animationState.m_prevTime = m_animationState.m_currTime;
	m_animationState.m_currTime += m_timeMul * dt;

	if ( m_firstUpdate && m_animationState.m_currTime != 0.f )
	{
		m_firstUpdate = false;

		result.AddNotification( ENT_AnimationStarted, m_animationState.m_animation );
	}

	Float marker1, marker2;
	WrapTime( m_animationState.m_prevTime, marker1 );
	WrapTime( m_animationState.m_currTime, marker2 );

	AlignPreTimeToCurr( m_animationState.m_prevTime, m_animationState.m_currTime );

	if ( m_slot.IsValid() )
	{
		ASSERT( m_animationState.m_prevTime >= 0.f );
		ASSERT( m_animationState.m_currTime >= 0.f );

		m_slot.PlayAnimation( m_animationState, CalcWeight( m_animationState.m_currTime ) );

		if ( IsBlendingOut( m_animationState.m_currTime ) )
		{
			RaiseBehaviorEventAtEnd();
		}

		if ( m_animationEntry )
		{
			CollectEvents( m_animationState.m_prevTime, m_animationState.m_currTime, result );
		}
	}

	UnwrapTime( m_animationState.m_prevTime, marker1 );
	UnwrapTime( m_animationState.m_currTime, marker2 );

	const Float absTime = MAbs( m_animationState.m_currTime );
	const Float endsAt = m_duration - m_earlyEndOffset;
	if ( absTime >= endsAt ||
		 ( m_endWhenBlendingOut && IsBlendingOut( m_animationState.m_currTime ) ) )
	{
		timeRest = absTime - endsAt;

		result.AddNotification( ENT_AnimationFinished, m_animationState.m_animation );

		OnEnd( result );

		return true;
	}

	return false;
}

void ExpSingleAnimExecutor::OnEnd( ExpExecutorUpdateResult& result )
{
}

void ExpSingleAnimExecutor::CollectEvents( Float prev, Float curr, ExpExecutorUpdateResult& result ) const
{
	ASSERT( m_animationEntry );

	TDynArray< CAnimationEventFired > events;
	m_animationEntry->GetEventsByTime( prev, curr, 0, 1.f, &events, NULL );

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