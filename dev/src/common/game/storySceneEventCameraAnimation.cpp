/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storySceneEvent.h"
#include "storySceneEventCameraAnimation.h"
#include "storyScenePlayer.h"
#include "sceneLog.h"
#include "../engine/camera.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventCameraAnim )

CStorySceneEventCameraAnim::CStorySceneEventCameraAnim()
	: CStorySceneEventAnimClip()
	, m_isIdle( false )
{

}

CStorySceneEventCameraAnim::CStorySceneEventCameraAnim( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEventAnimClip( eventName, sceneElement, startTime, actor, trackName )
	, m_isIdle( false )
{
}

CStorySceneEventCameraAnim* CStorySceneEventCameraAnim::Clone() const
{
	return new CStorySceneEventCameraAnim( *this );
}

const CAnimatedComponent* CStorySceneEventCameraAnim::GetAnimatedComponentForActor( const CStoryScenePlayer* scenePlayer ) const
{
	const CCamera* c = scenePlayer->GetSceneCamera();
	return c ? c->GetRootAnimatedComponent() : nullptr;
}

void CStorySceneEventCameraAnim::AddEventToCollector( CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, SAnimationState& dialogAnimationState, const SStorySceneEventTimeInfo& timeInfo, Float blendWeight ) const
{
	if ( !scenePlayer->IsGameplayNow() )
	{
		StorySceneEventsCollector::CameraAnimation event( this );
		event.m_eventTimeAbs = timeInfo.m_timeAbs;
		event.m_eventTimeLocal = timeInfo.m_timeLocal;
		event.m_animationWeight = blendWeight;
		event.m_blendWeight = timeInfo.m_progress;
		event.m_animationState = dialogAnimationState;
		event.m_isIdle = m_isIdle;

		collector.AddEvent( event );
	}
}

void CStorySceneEventCameraAnim::RemoveEventFromCollector( CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if ( !scenePlayer->IsGameplayNow() )
	{
		if ( m_isIdle )
		{
			SAnimationState dialogAnimationState;
			dialogAnimationState.m_animation = GetAnimationName();

			StorySceneEventsCollector::CameraAnimation event( this );
			event.m_eventTimeAbs = timeInfo.m_timeAbs;
			event.m_eventTimeLocal = timeInfo.m_timeLocal;
			event.m_animationWeight = m_weight;
			event.m_blendWeight = 1.f;
			event.m_animationState = dialogAnimationState;
			event.m_isIdle = m_isIdle;

			collector.AddEvent( event );
		}
		else
		{
			StorySceneEventsCollector::CameraAnimation event( this );

			collector.RemoveEvent( event );
		}
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
