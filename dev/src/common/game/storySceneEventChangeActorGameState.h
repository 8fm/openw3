#pragma once

#include "storySceneEvent.h"
#include "storySceneIncludes.h"

class CStorySceneEventChangeActorGameState : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventChangeActorGameState, CStorySceneEvent )

protected:
	CName		m_actor;

	Bool		m_fullAutoMode;
	Bool		m_snapToTerrain;
	Float		m_snapToTerrainDuration;
	Float		m_blendPoseDuration;
	Bool		m_forceResetClothAndDangles;

	Bool		m_switchToGameplayPose;
	CName		m_gameplayPoseTypeName;
	CName		m_raiseGlobalBehaviorEvent;
	Int32		m_activateBehaviorGraph;
	Int32		m_startGameplayAction;

public:
	CStorySceneEventChangeActorGameState();
	CStorySceneEventChangeActorGameState( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventChangeActorGameState* Clone() const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventChangeActorGameState )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	//PROPERTY_EDIT( m_fullAutoMode, TXT( "Calculate all parameters for best results" ))
	PROPERTY_EDIT( m_snapToTerrain, TXT( "Snaps an actor to terrain" ) )
	PROPERTY_EDIT( m_snapToTerrainDuration, TXT( "Snaps an actor to terrain transition time, 0 means instant" ) )
	PROPERTY_EDIT( m_blendPoseDuration, TXT("Blend pose duration time, 0 means instant, -1 same as snap to terrain duration ") )
	PROPERTY_EDIT( m_forceResetClothAndDangles, TXT("Force reset all cloth and dangle components") )
	PROPERTY_EDIT( m_switchToGameplayPose, TXT( "Switches an actor into gameplay pose" ) )
	PROPERTY_EDIT( m_gameplayPoseTypeName, TXT( "Name of the behavior graph flow node - Walk, Run etc." ) )
	PROPERTY_EDIT( m_raiseGlobalBehaviorEvent, TXT( "" ) )
	PROPERTY_CUSTOM_EDIT( m_activateBehaviorGraph, TXT(""), TXT("ScriptedEnum_EBehaviorGraph") );
	PROPERTY_CUSTOM_EDIT( m_startGameplayAction, TXT(""), TXT("ScriptedEnum_EStorySceneGameplayAction") );
END_CLASS_RTTI();