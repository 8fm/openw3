/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "storySceneEventChangeActorGameState.h"
#include "storyScenePlayer.h"
#include "storySceneSystem.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventChangeActorGameState );

CStorySceneEventChangeActorGameState::CStorySceneEventChangeActorGameState()
	: CStorySceneEvent()
	, m_actor( CName::NONE )
	, m_snapToTerrain( true )
	, m_switchToGameplayPose( true )
	, m_snapToTerrainDuration( 0.f )
	, m_activateBehaviorGraph( 0 )
	, m_startGameplayAction( 0 )
	, m_blendPoseDuration( -1 )
	, m_fullAutoMode( false )
	, m_forceResetClothAndDangles( true )
{}

CStorySceneEventChangeActorGameState::CStorySceneEventChangeActorGameState( const String& eventName,
	   CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor( actor )
	, m_snapToTerrain( true )
	, m_switchToGameplayPose( true )
	, m_snapToTerrainDuration( 0.f )
	, m_activateBehaviorGraph( 0 )
	, m_startGameplayAction( 0 )
	, m_forceResetClothAndDangles( true )
{}

CStorySceneEventChangeActorGameState* CStorySceneEventChangeActorGameState::Clone() const
{
	return new CStorySceneEventChangeActorGameState( *this );
}

void CStorySceneEventChangeActorGameState::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );
	
	if ( m_actor )
	{
		StorySceneEventsCollector::ActorChangeGameState evt( this, m_actor );
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_snapToTerrain = m_snapToTerrain;
		evt.m_snapToTerrainDuration = m_snapToTerrainDuration;
		evt.m_switchToGameplayPose = m_switchToGameplayPose;
		evt.m_gameplayPoseTypeName = m_gameplayPoseTypeName;
		evt.m_raiseGlobalBehaviorEvent = m_raiseGlobalBehaviorEvent;
		evt.m_activateBehaviorGraph = m_activateBehaviorGraph;
		evt.m_startGameplayAction = m_startGameplayAction;
		evt.m_blendPoseDuration = m_blendPoseDuration >= 0.f ? m_blendPoseDuration : m_snapToTerrainDuration;
		evt.m_forceResetClothAndDangles = m_forceResetClothAndDangles;
		collector.AddEvent( evt );
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
