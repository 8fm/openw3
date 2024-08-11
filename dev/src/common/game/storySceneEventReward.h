#pragma once
#include "storySceneEvent.h"

class CStorySceneEventReward : public CStorySceneEvent
{
public:
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventReward, CStorySceneEvent )

	CName	m_npcTag;
	CName	m_itemName;
	CName   m_rewardName;

	Int32	m_quantity;
	Bool	m_dontInformGui;

public:

	CStorySceneEventReward();

	CStorySceneEventReward( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventReward* Clone() const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	DECLARE_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventReward, TXT("Add misc event"), TXT("Reward to player"), TXT("IMG_DIALOG_EQUIP_ITEM") )	
};

BEGIN_CLASS_RTTI( CStorySceneEventReward )
	PARENT_CLASS( CStorySceneEvent )	
	PROPERTY_EDIT( m_npcTag, TXT("") )
	PROPERTY_CUSTOM_EDIT( m_itemName, TXT( "Item name" ), TXT( "ItemSelection" ) )
	PROPERTY_CUSTOM_EDIT( m_rewardName, TXT("Name of the reward"), TXT("RewardSelection") )
	PROPERTY_EDIT( m_quantity, TXT("") )
	PROPERTY( m_dontInformGui )
END_CLASS_RTTI()




class CStorySceneEventSetupItemForSync : public CStorySceneEvent
{
public:
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventSetupItemForSync, CStorySceneEvent )

	CName	m_actorToSyncTo;
	CName	m_itemName;
	Bool	m_activate;
public:

	CStorySceneEventSetupItemForSync() : m_activate( true )
	{}

	CStorySceneEventSetupItemForSync( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName )
		: CStorySceneEvent( eventName, sceneElement, startTime, trackName ), m_activate( true )
	{}

	virtual CStorySceneEventSetupItemForSync* Clone() const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	DECLARE_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventSetupItemForSync, TXT("Add misc event"), TXT("Setup item for sync"), TXT("IMG_DIALOG_EQUIP_ITEM") )	
};

BEGIN_CLASS_RTTI( CStorySceneEventSetupItemForSync )
	PARENT_CLASS( CStorySceneEvent )	
	PROPERTY_EDIT( m_itemName, TXT("") )
	PROPERTY_EDIT( m_activate, TXT("") )
	PROPERTY_CUSTOM_EDIT( m_actorToSyncTo, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
END_CLASS_RTTI()