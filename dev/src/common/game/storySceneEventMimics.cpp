/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storySceneEventMimics.h"
#include "storySceneUtils.h"
#include "storySceneSystem.h"
#include "../engine/mimicComponent.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventMimics )

CStorySceneEventMimics::CStorySceneEventMimics( )
   : CStorySceneEventAnimClip()
   , m_useWeightCurve( false )
   , m_mimicsPoseWeight( 1.f )
{
}

CStorySceneEventMimics::CStorySceneEventMimics( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEventAnimClip( eventName, sceneElement, startTime, actor, trackName )
	, m_useWeightCurve( false )
	, m_mimicsPoseWeight( 1.f )
{
}

CStorySceneEventMimics* CStorySceneEventMimics::Clone() const
{
	return new CStorySceneEventMimics( *this );
}

void CStorySceneEventMimics::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
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
	}

	Float idleBlendWeight = w;
	if ( m_useWeightCurve )
	{
		idleBlendWeight = Clamp( m_weightCurve.GetFloatValue( idleBlendWeight ), 0.f, 1.f );
	}
	else
	{
		idleBlendWeight = BehaviorUtils::BezierInterpolation( idleBlendWeight );
	}

	StorySceneEventsCollector::ActorChangeState evt( this, m_actor );
	evt.m_eventTimeLocal = timeInfo.m_timeLocal;
	evt.m_eventTimeAbs = timeInfo.m_timeAbs;
	evt.m_mimicsBlendWeight = idleBlendWeight;
	evt.m_mimicsBlendSet = true;
	evt.m_mimicsPoseWeight = m_mimicsPoseWeight;
	
	SetupEventAnimationData( evt );

	if ( evt.m_state.IsSomethingSet() || DoesForceAnyIdle() )
	{
		collector.AddEvent( evt );
	}
}

void CStorySceneEventMimics::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );

	StorySceneEventsCollector::ActorChangeState evt( this, m_actor );
	evt.m_eventTimeLocal = timeInfo.m_timeLocal;
	evt.m_eventTimeAbs = timeInfo.m_timeAbs;
	evt.m_mimicsBlendWeight = 1.f;
	evt.m_mimicsBlendSet = true;
	evt.m_mimicsPoseWeight = m_mimicsPoseWeight;
	
	SetupEventAnimationData( evt );

	if ( evt.m_state.IsSomethingSet() || DoesForceAnyIdle() )
	{
		collector.AddEvent( evt );
	}
}

Bool CStorySceneEventMimics::DoesForceAnyIdle() const
{
	return m_forceMimicsIdleAnimation_Eyes || m_forceMimicsIdleAnimation_Pose || m_forceMimicsIdleAnimation_Animation;
}

void CStorySceneEventMimics::SetupEventAnimationData( StorySceneEventsCollector::ActorChangeState& evt ) const
{
	evt.m_forceMimicsIdleEyesAnimation = m_forceMimicsIdleAnimation_Eyes;
	evt.m_forceMimicsIdlePoseAnimation = m_forceMimicsIdleAnimation_Pose;
	evt.m_forceMimicsIdleAnimAnimation = m_forceMimicsIdleAnimation_Animation;

	evt.m_state.m_mimicsEmotionalState = m_mimicsEmotionalState;
	evt.m_state.m_mimicsLayerEyes = m_mimicsLayer_Eyes;
	evt.m_state.m_mimicsLayerPose = m_mimicsLayer_Pose;
	evt.m_state.m_mimicsLayerAnimation = m_mimicsLayer_Animation;
}

void CStorySceneEventMimics::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( m_stateName )
	{
		CName status;
		CName poseName;

		StorySceneUtils::ConvertStateToAnimationFilters( m_stateName, status, m_mimicsEmotionalState, poseName );
		m_stateName = CName::NONE;
	}

	CacheMimicsLayerFromEmoState( false );
}

const CAnimatedComponent* CStorySceneEventMimics::GetAnimatedComponentForActor( const CStoryScenePlayer* scenePlayer ) const
{
	const CEntity* actor = scenePlayer->GetSceneActorEntity( m_actor );
	if ( actor && actor->QueryActorInterface() )
	{
		return actor->QueryActorInterface()->GetMimicComponent();
	}

	return nullptr;
}

void CStorySceneEventMimics::CacheMimicsLayerFromEmoState( Bool force )
{
	if ( m_mimicsEmotionalState && ( force || ( !m_mimicsLayer_Eyes || !m_mimicsLayer_Pose || !m_mimicsLayer_Animation ) ) )
	{
		if ( force || !m_mimicsLayer_Eyes )
		{
			m_mimicsLayer_Eyes = m_mimicsEmotionalState;
		}

		if ( force || !m_mimicsLayer_Pose )
		{
			m_mimicsLayer_Pose = m_mimicsEmotionalState;
		}

		if ( force || !m_mimicsLayer_Animation )
		{
			m_mimicsLayer_Animation = m_mimicsEmotionalState;
		}
	}
}

/*
void CStorySceneEventMimics::CacheMimicsLayerFromEmoState( Bool force )
{
	if ( m_mimicsEmotionalState && ( force || ( !m_mimicsLayerEyes || !m_mimicsLayerPose || !m_mimicsLayerAnimation ) ) )
	{
		const CStorySceneAnimationList::MimicsEmoStatePreset* preset = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().FindMimicsAnimationByEmoState( m_mimicsEmotionalState );
		if ( preset )
		{
			if ( force || !m_mimicsLayerEyes )
			{
				m_mimicsLayerName_Eyes = preset->m_layerEyes;
				m_mimicsLayerEyes = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().FindMimicsAnimationByFriendlyName( m_mimicsLayerName_Eyes );
			}

			if ( force || !m_mimicsLayerPose )
			{
				m_mimicsLayerName_Pose = preset->m_layerPose;
				m_mimicsLayerPose = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().FindMimicsAnimationByFriendlyName( m_mimicsLayerName_Pose );
			}

			if ( force || !m_mimicsLayerAnimation )
			{
				m_mimicsLayerName_Animation = preset->m_layerAnimation;
				m_mimicsLayerAnimation = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().FindMimicsAnimationByFriendlyName( m_mimicsLayerName_Animation );
			}
		}
	}
}
*/

#ifndef NO_EDITOR

void CStorySceneEventMimics::OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName )
{
	if ( propertyName == TXT("mimicsEmotionalState") )
	{
		SStorySceneActorAnimationState currState;
		if ( previewPlayer->GetPreviousActorAnimationState( m_actor, currState ) )
		{
			SStorySceneActorAnimationState destState( currState );

			if ( m_mimicsEmotionalState != CName::NONE )
			{
				destState.m_mimicsEmotionalState = m_mimicsEmotionalState;
			}

			TDynArray< CName > transitions;
			if ( GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().FindMimicsTransitions( currState, destState, transitions ) )
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

		CacheMimicsLayerFromEmoState( true );
	}
	else if ( propertyName == TXT("mimicsLayer_Eyes") || propertyName == TXT("mimicsLayer_Pose") || propertyName == TXT("mimicsLayer_Animation") )
	{
		SetCustomEmotionalState();
	}
}

void CStorySceneEventMimics::SetCustomEmotionalState()
{
	m_mimicsEmotionalState = CName::NONE;
	m_transitionAnimation = CName::NONE;
}

void CStorySceneEventMimics::SetMimicData( const TDynArray<CName>& data )
{
	if ( data.Size() == 4 )
	{
		m_mimicsEmotionalState = data[0];
		m_mimicsLayer_Eyes = data[1];
		m_mimicsLayer_Pose = data[2];
		m_mimicsLayer_Animation = data[3];
	}				
}

#endif

void CStorySceneEventMimics::CollectUsedAnimations( CStorySceneAnimationContainer& container ) const
{
	TBaseClass::CollectUsedAnimations( container );

	const CName actorId = GetSubject();

	container.AddMimicIdle( actorId, m_mimicsLayer_Eyes, m_mimicsLayer_Pose, m_mimicsLayer_Animation );
	if ( m_forceMimicsIdleAnimation_Eyes )
	{
		container.AddMimicAnimation( actorId, m_forceMimicsIdleAnimation_Eyes );
	}
	if ( m_forceMimicsIdleAnimation_Pose )
	{
		container.AddMimicAnimation( actorId, m_forceMimicsIdleAnimation_Pose );
	}
	if ( m_forceMimicsIdleAnimation_Animation )
	{
		container.AddMimicAnimation( actorId, m_forceMimicsIdleAnimation_Animation );
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
