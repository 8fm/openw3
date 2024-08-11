/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "storySceneEventChangePose.h"
#include "storyScenePlayer.h"
#include "storySceneDirector.h"
#include "storySceneEventsCollector_events.h"
#include "storySceneUtils.h"
#include "storySceneSystem.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/skeletalAnimationContainer.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventChangePose );

CStorySceneEventChangePose::CStorySceneEventChangePose()
	: CStorySceneEventAnimClip()
	, m_useWeightCurve( false )
	, m_resetCloth( DRCDT_None )
	, m_useMotionExtraction( false )
{

}

CStorySceneEventChangePose::CStorySceneEventChangePose( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEventAnimClip( eventName, sceneElement, startTime, actor, trackName )
	, m_useWeightCurve( false )
	, m_resetCloth( DRCDT_None )
	, m_useMotionExtraction( false )
{

}

/*
Cctor.

Compiler generated cctor would also copy instance vars - we don't want that.
*/
CStorySceneEventChangePose::CStorySceneEventChangePose( const CStorySceneEventChangePose& other )
: CStorySceneEventAnimClip( other )
, m_stateName( other.m_stateName )
, m_status( other.m_status )
, m_emotionalState( other.m_emotionalState )
, m_poseName( other.m_poseName )
, m_transitionAnimation( other.m_transitionAnimation )
, m_useMotionExtraction( other.m_useMotionExtraction )
, m_forceBodyIdleAnimation( other.m_forceBodyIdleAnimation )
, m_useWeightCurve( other.m_useWeightCurve )
, m_weightCurve( other.m_weightCurve )
, m_resetCloth( other.m_resetCloth )
{}

CStorySceneEventChangePose* CStorySceneEventChangePose::Clone() const
{
	return new CStorySceneEventChangePose( *this );
}

void CStorySceneEventChangePose::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_animation;
	compiler << i_hasMotion;
}

void CStorySceneEventChangePose::OnInit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const
{
	TBaseClass::OnInit( data, scenePlayer );

	CSkeletalAnimationSetEntry*& anim = data[ i_animation ];
	anim = FindAnimation( scenePlayer );

	Bool& hasMotion = data[ i_hasMotion ];
	hasMotion = false;
	if ( anim )
	{
		hasMotion = HasAnimationMotion( anim );
	}
}

void CStorySceneEventChangePose::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	Float w = timeInfo.m_progress;
	if ( HasAnimation() )
	{
		// Variables t and d contain stretch so blend in/out behave as if they were
		// not-scaled/not-stretched - see CStorySceneEventAnimClip::CalculateBlendWeight()
		// for more info.

		const Float t = timeInfo.m_timeLocal;
		const Float d = GetInstanceDuration( data );

		if ( t <= m_blendIn )
		{
			w = 0.f;
		}
		else if ( t >= d - m_blendOut )
		{
			w = 1.f;
		}
		else
		{
			const Float clampTime = t - m_blendIn;
			const Float clampDuration = d - m_blendOut - m_blendIn;

			w = clampDuration > 0.f ? clampTime / clampDuration : 1.f;

			SCENE_ASSERT( w >= 0.f && w <= 1.f );
		}

		ApplyAnimationMotion( data, collector, timeInfo );
	}

	Float idleBlendWeight = w;
	if ( m_useWeightCurve )
	{
		idleBlendWeight = Clamp( m_weightCurve.GetFloatValue( idleBlendWeight ), 0.f, 1.f );
	}

	StorySceneEventsCollector::ActorChangeState evt( this, m_actor );
	evt.m_eventTimeLocal = timeInfo.m_timeLocal;
	evt.m_eventTimeAbs = timeInfo.m_timeAbs;
	evt.m_bodyBlendWeight = idleBlendWeight;
	evt.m_bodyBlendSet = true;
	evt.m_forceBodyIdleAnimation = m_forceBodyIdleAnimation;
	evt.m_ID = GetGUID();

	FillState( evt.m_state );

	if ( evt.m_state.IsSomethingSet() || m_forceBodyIdleAnimation )
	{
		collector.AddEvent( evt );
	}

}

void CStorySceneEventChangePose::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );

	StorySceneEventsCollector::ActorChangeState evt( this, m_actor );
	evt.m_eventTimeLocal = timeInfo.m_timeLocal;
	evt.m_eventTimeAbs = timeInfo.m_timeAbs;
	evt.m_bodyBlendWeight = 1.f;
	evt.m_bodyBlendSet = true;
	evt.m_forceBodyIdleAnimation = m_forceBodyIdleAnimation;
	evt.m_ID = GetGUID();

	FillState( evt.m_state );

	if ( evt.m_state.IsSomethingSet() || m_forceBodyIdleAnimation )
	{
		collector.AddEvent( evt );
	}

	if ( m_actor && m_resetCloth != DRCDT_None )
	{
		StorySceneEventsCollector::ActorResetClothAndDangles cEvt( this, m_actor );
		cEvt.m_eventTimeAbs = timeInfo.m_timeAbs;
		cEvt.m_eventTimeLocal = timeInfo.m_timeLocal;
		cEvt.m_forceRelaxedState = m_resetCloth == DRCDT_ResetAndRelax;

		collector.AddEvent( cEvt );
	}

	if ( HasAnimation() )
	{
		ApplyAnimationMotion( data, collector, timeInfo );
	}
}

void CStorySceneEventChangePose::ApplyAnimationMotion( CStorySceneInstanceBuffer& data, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if ( m_useMotionExtraction && data[ i_hasMotion ] && data[ i_animation ] )
	{
		StorySceneEventsCollector::ActorMotion evt( this, m_actor );

		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;

		evt.m_eventTimeAbsStart = GetInstanceStartTime( data );
		evt.m_eventTimeAbsEnd = GetInstanceStartTime( data ) + GetInstanceDuration( data );
		evt.m_animation = data[ i_animation ];
		evt.m_weight = m_weight;
		evt.m_blendIn = m_blendIn;
		evt.m_blendOut = m_blendOut;
		evt.m_stretch = m_stretch;
		evt.m_clipFront = m_clipFront;

		collector.AddEvent( evt );
	}
}

CSkeletalAnimationSetEntry* CStorySceneEventChangePose::FindAnimation( const CStoryScenePlayer* scenePlayer ) const
{
	const CAnimatedComponent* ac = GetAnimatedComponentForActor( scenePlayer );
	if ( ac && ac->GetAnimationContainer() )
	{
		CSkeletalAnimationSetEntry* anim = ac->GetAnimationContainer()->FindAnimation( GetAnimationName() );
		if ( anim && anim->GetAnimation() )
		{
			return anim;
		}
	}

	return nullptr;
}

Bool CStorySceneEventChangePose::HasAnimationMotion( const CSkeletalAnimationSetEntry* animation ) const
{
	if ( animation->GetAnimation()->HasExtractedMotion() )
	{
		Matrix motion;
		animation->GetAnimation()->GetMovementAtTime( animation->GetDuration(), motion );

		const EulerAngles rot = motion.ToEulerAngles();
		const Vector& pos = motion.GetTranslationRef();

		if ( rot.AlmostEquals( EulerAngles::ZEROS ) && Vector::Near3( pos, Vector::ZERO_3D_POINT ) )
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	return false;
}

void CStorySceneEventChangePose::FillState( SStorySceneActorAnimationState& state ) const
{
	state.m_status = m_status;
	state.m_emotionalState = m_emotionalState;
	state.m_poseType = m_poseName;
}

#ifndef NO_EDITOR

void CStorySceneEventChangePose::OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName )
{
	if ( propertyName == TXT("status") || propertyName == TXT("emotionalState") || propertyName == TXT("poseName") )
	{
		SStorySceneActorAnimationState currState;
		if ( previewPlayer->GetPreviousActorAnimationState( m_actor, currState ) )
		{
			SStorySceneActorAnimationState destState( currState );

			if ( m_status != CName::NONE )
			{
				destState.m_status = m_status;
			}

			if ( m_emotionalState != CName::NONE )
			{
				destState.m_emotionalState = m_emotionalState;
			}

			if ( m_poseName != CName::NONE )
			{
				destState.m_poseType = m_poseName;
			}

			TDynArray< CName > transitions;
			if ( GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().FindBodyTransitions( currState, destState, transitions ) )
			{
				SCENE_ASSERT( transitions.Size() > 0 );
				m_transitionAnimation = transitions[ 0 ];

				m_clipFront = 0.f;
				m_clipEnd = GetAnimationDuration( previewPlayer );
				m_stretch = 1.f;

				TBaseClass::OnPreviewPropertyChanged( previewPlayer, propertyName );
			}
		}
		else
		{
			m_transitionAnimation = CName::NONE;
		}
	}
	else if ( propertyName == TXT("duration") )
	{
		if ( previewPlayer->GetEventDuration( *this ) == 0.f && m_resetCloth == DRCDT_None )
		{
			m_resetCloth = DRCDT_Reset;
		}
	}
}

#endif

void CStorySceneEventChangePose::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( !m_stateName.Empty() )
	{
		StorySceneUtils::ConvertStateToAnimationFilters( m_stateName, m_status, m_emotionalState, m_poseName );
		m_stateName = CName::NONE;
	}
}

void CStorySceneEventChangePose::SetAnimationState( const TDynArray< CName >& state )
{
	ASSERT( state.Size() == 3 );

	m_status = state[ 0 ];
	m_emotionalState = state[ 1 ];
	m_poseName = state[ 2 ];
}

void CStorySceneEventChangePose::CollectUsedAnimations( CStorySceneAnimationContainer& container ) const
{
	TBaseClass::CollectUsedAnimations( container );

	const CName actorId = GetSubject();

	container.AddBodyIdle( actorId, m_status, m_emotionalState, m_poseName );
	if ( m_forceBodyIdleAnimation )
	{
		container.AddBodyAnimation( actorId, m_forceBodyIdleAnimation );
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
