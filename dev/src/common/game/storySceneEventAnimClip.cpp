/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "storySceneEventAnimClip.h"
#include "storyScenePlayer.h"
#include "..\engine\behaviorGraphAnimationManualSlot.h"
#include "..\engine\animatedComponent.h"
#include "..\engine\behaviorGraphStack.h"
#include "storySceneEvent.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventAnimClip );

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

CStorySceneEventAnimClip::CStorySceneEventAnimClip()
	: CStorySceneEventDuration()
	, m_actor( CName::NONE )
	, m_blendIn( 0.5f )
	, m_blendOut( 0.5f )
	, m_clipFront( 0.0f )
	, m_clipEnd( -1.0f )
	, m_stretch( 1.0f )
	, m_weight( 1.f )
	, m_allowLookatsLevel( LL_Body )
	, m_forceAnimationTimeFlag( false )
	, m_forceAnimationTime( 0.f )
	, m_allowPoseCorrection( true )
{

}

CStorySceneEventAnimClip::CStorySceneEventAnimClip( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEventDuration( eventName, sceneElement, startTime, 1.0f, trackName )
	, m_actor( actor )
	, m_blendIn( 0.5f )
	, m_blendOut( 0.5f )
	, m_clipFront( 0.0f )
	, m_clipEnd( -1.0f )
	, m_stretch( 1.0f )
	, m_weight( 1.f )
	, m_allowLookatsLevel( LL_Body )
	, m_forceAnimationTimeFlag( false )
	, m_forceAnimationTime( 0.f )
	, m_allowPoseCorrection( true )
{

}

void CStorySceneEventAnimClip::OnDeinit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const
{
	// HACK
	CStorySceneEventsCollector c;
	SStorySceneEventTimeInfo timeInfo;
	CStorySceneEventAnimClip::OnEnd( data, scenePlayer, c, timeInfo );

	TBaseClass::OnDeinit( data, scenePlayer );
}

void CStorySceneEventAnimClip::AddEventToCollector( CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, SAnimationState& dialogAnimationState, const SStorySceneEventTimeInfo& timeInfo, Float blendWeight ) const
{
	CEntity* actor = scenePlayer->GetSceneActorEntity( m_actor );

	if ( !actor )
	{
		actor = scenePlayer->GetScenePropEntity( m_actor );
	}

	if ( actor && IsBodyOrMimicMode() )
	{
		StorySceneEventsCollector::BodyAnimation event( this, m_actor );
		event.m_eventTimeAbs = timeInfo.m_timeAbs;
		event.m_eventTimeLocal = timeInfo.m_timeLocal;
		event.m_weight = blendWeight;
		event.m_animationState = dialogAnimationState;
		event.m_ID = GetGUID();
		event.m_allowPoseCorrection = m_allowPoseCorrection;

		OnAddExtraDataToEvent( event );

		collector.AddEvent( event );
	}
	else if ( actor && !IsBodyOrMimicMode() )
	{
		StorySceneEventsCollector::MimicsAnimation event( this, m_actor );
		event.m_eventTimeAbs = timeInfo.m_timeAbs;
		event.m_eventTimeLocal = timeInfo.m_timeLocal;
		event.m_weight = blendWeight;
		event.m_animationState = dialogAnimationState;
		event.m_ID = GetGUID();
		event.m_allowPoseCorrection = m_allowPoseCorrection;

		OnAddExtraDataToEvent( event );

		collector.AddEvent( event );
	}
}

void CStorySceneEventAnimClip::RemoveEventFromCollector( CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if ( IsBodyOrMimicMode() )
	{
		StorySceneEventsCollector::BodyAnimation event( this, m_actor );

		OnAddExtraDataToEvent( event );

		collector.RemoveEvent( event );
	}
	else
	{
		StorySceneEventsCollector::MimicsAnimation event( this, m_actor );

		OnAddExtraDataToEvent( event );

		collector.RemoveEvent( event );
	}
}

void CStorySceneEventAnimClip::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	const Float timeDelta = timeInfo.m_timeDelta;
	
	// calculate event time without stretch and scaling
	const Float scalingFactor = GetAnimationStretch() * GetInstanceScalingFactor( data );
	const Float eventBaseTime = timeInfo.m_timeLocal / scalingFactor;
	const Float animTimeDelta = timeDelta / scalingFactor;
	
	const Float currAnimTime = m_clipFront + eventBaseTime;
	const Float prevAnimTime =  ::Max< Float >( currAnimTime - animTimeDelta, 0.0f );

	SAnimationState dialogAnimationState;
	dialogAnimationState.m_animation = GetAnimationName();
	dialogAnimationState.m_prevTime = prevAnimTime;
	dialogAnimationState.m_currTime = currAnimTime;

	if ( !dialogAnimationState.m_animation && !ShouldPlayEmptyAnimation() )
	{
		return;
	}

	Float blendWeight = 0.f;
	if ( !m_voiceWeightCurve.m_useCurve )
	{
		// calculate event duration without stretch and scaling
		const Float baseDuration = GetInstanceDuration( data ) / ( GetAnimationStretch() * GetInstanceScalingFactor( data ) );

		// We're passing base time and base duration so blend in/out will behave
		// as if they were stretched/scaled - see CalculateBlendWeight() function.
		blendWeight = m_weight * CalculateBlendWeight( eventBaseTime, baseDuration );
	}
	else
	{
		blendWeight = Clamp( m_voiceWeightCurve.m_curve.GetFloatValue( m_voiceWeightCurve.m_timeOffset + m_voiceWeightCurve.m_curve.GetMaxTime() * timeInfo.m_progress ), 0.f, 1.f );
		blendWeight *= m_voiceWeightCurve.m_valueMulPre;
		blendWeight += m_voiceWeightCurve.m_valueOffset;
		blendWeight *= m_voiceWeightCurve.m_valueMulPost;
	}

	ASSERT( blendWeight >= 0.f && blendWeight <= 1.f );

	if ( m_forceAnimationTimeFlag )
	{
		dialogAnimationState.m_prevTime = m_forceAnimationTime;
		dialogAnimationState.m_currTime = m_forceAnimationTime;
	}

	AddEventToCollector( scenePlayer, collector, dialogAnimationState, timeInfo, blendWeight );
}

void CStorySceneEventAnimClip::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	CActor* actor = Cast< CActor >( scenePlayer->GetSceneActorEntity( m_actor ) );
	if ( actor )
	{
		actor->SetLookatFilterData( m_allowLookatsLevel, GetAnimationName() );
	}
}

void CStorySceneEventAnimClip::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	CActor* actor = Cast< CActor >( scenePlayer->GetSceneActorEntity( m_actor ) );
	if ( actor )
	{
		actor->RemoveLookatFilterData( GetAnimationName() );
	}

	RemoveEventFromCollector( scenePlayer, collector, timeInfo );

	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );
}

/*
Calculates blend weight for given event time.

CAUTION Calculating blend weight for stretched/scaled events
1. If eventTime and eventDuration include stretch/scale then blend in/out will behave
    as if they were not stretched/scaled.
2. If eventTime and eventDuration are stripped of stretch/scale then blend in/out will
   behave as if they were scaled.

CStorySceneEventAnimClip::OnProcess() uses CalculateBlendWeight() as it is described in 2.
However, some derived classes, i.e. CStorySceneEventChangePose and CStorySceneEventMimics,
calculate blend weight (without using CalculateBlendWeight() function) in such a way that
blend in/out behave as if they were not scaled. This probably means we have a problem as
CStorySceneEventChangePose will put into collector two events:
- StorySceneEventsCollector::BodyAnimation or StorySceneEventsCollector::MimicsAnimation
  coming from CStorySceneEventAnimClip and
- StorySceneEventsCollector::ActorChangeState coming from CStorySceneEventChangePose itself.
The problem is that the first event uses stretched/scaled blend in/out while the second one
uses non-stretched/non-scaled blend in/out. There is a similar problem with CStorySceneEventMimics.
*/
Float CStorySceneEventAnimClip::CalculateBlendWeight( Float eventTime, Float eventDuration ) const
{
		Float blendWeight = 1.0f;
		if ( eventTime <= m_blendIn )
		{
			blendWeight = eventTime / m_blendIn;
		}
		else if ( eventTime >= eventDuration - m_blendOut )
		{
			blendWeight = ( eventDuration - eventTime ) / m_blendOut;
		}
		blendWeight = ::Clamp< Float >( blendWeight, 0.0f, 1.0f );

		return BehaviorUtils::BezierInterpolation( blendWeight );
}

const CAnimatedComponent* CStorySceneEventAnimClip::GetAnimatedComponentForActor( const CStoryScenePlayer* scenePlayer ) const
{
	ASSERT( scenePlayer != NULL );

	const CEntity* actor = scenePlayer->GetSceneActorEntity( m_actor );
	
	if( actor != NULL )
	{
		return actor->GetRootAnimatedComponent();
	}
	
	return NULL;
}

void CStorySceneEventAnimClip::CollectUsedAnimations( CStorySceneAnimationContainer& container ) const
{
	if ( IsBodyOrMimicMode() )
	{
		container.AddBodyAnimation( GetSubject(), GetAnimationName() );
	}
	else
	{
		container.AddMimicAnimation( GetSubject(), GetAnimationName() );
	}
}

#ifndef  NO_EDITOR
void CStorySceneEventAnimClip::OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName ) 
{
	RefreshDuration( previewPlayer );

	if ( propertyName == CNAME(weight) )
	{
		m_weight = Clamp( m_weight, 0.f, 1.f );
	}
}

Float CStorySceneEventAnimClip::GetAnimationDuration( const CStoryScenePlayer* previewPlayer ) const
{
	const CAnimatedComponent* ac = GetAnimatedComponentForActor( previewPlayer );
	if ( !ac )
	{
		return 1.0f;
	}

	return ac->GetAnimationDuration( GetAnimationName() );
}

Float CStorySceneEventAnimClip::RefreshDuration( const CStoryScenePlayer* previewPlayer )
{
	const Float animationDuration = GetAnimationDuration( previewPlayer );

	RefreshDuration( animationDuration );

	return m_duration;
}

Float CStorySceneEventAnimClip::RefreshDuration( Float animationDuration )
{
	if ( animationDuration != m_cachedAnimationDuration && ( m_clipEnd < 0.0f || m_clipEnd > animationDuration || Abs<Float>(m_clipEnd - m_duration) < NumericLimits< Float >::Epsilon()) )
	{
		m_clipEnd = animationDuration;
	}
	m_cachedAnimationDuration = animationDuration;

	m_duration = ( m_clipEnd - m_clipFront ) * m_stretch;
	return m_duration;
}

#endif

void CStorySceneEventAnimClip::DoBakeScaleImpl( Float scalingFactor )
{
	CStorySceneEventDuration::DoBakeScaleImpl( scalingFactor );
	m_stretch *= scalingFactor;
}

#ifndef NO_EDITOR

/*
Adjusts animation front clip.

\param clipFront New value of front clip. Must satisfy: 0.0f <= clipFront && clipFront < m_clipEnd.
*/
void CStorySceneEventAnimClip::SetAnimationClipStart( Float clipFront )
{
	SCENE_ASSERT( 0.0f <= clipFront && clipFront < m_clipEnd );

	m_clipFront = clipFront;
	m_duration = ( m_clipEnd - m_clipFront ) * m_stretch;
}

/*
Adjusts animation end clip.

\param clipEnd New value of end clip. Must satisfy: m_clipFront < clipEnd && clipEnd <= m_cachedAnimationDuration.
*/
void CStorySceneEventAnimClip::SetAnimationClipEnd( Float clipEnd )
{
	SCENE_ASSERT( m_clipFront < clipEnd && clipEnd <= m_cachedAnimationDuration ); // TODO: do we always have m_cachedAnimationDuration? assert this

	m_clipEnd = clipEnd;
	m_duration = ( m_clipEnd - m_clipFront ) * m_stretch;
}

/*
Sets animation stretch.
*/
void CStorySceneEventAnimClip::SetAnimationStretch( Float stretch )
{
	SCENE_ASSERT( stretch > 0.0f );

	m_stretch = stretch;
	m_duration = ( m_clipEnd - m_clipFront ) * m_stretch;
}

#endif // !NO_EDITOR

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
