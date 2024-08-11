
#pragma once

#include "storySceneEvent.h"

enum ESceneItemEventMode
{
	SIEM_Default,
	SIEM_Mount,
	SIEM_Unmount
};

BEGIN_ENUM_RTTI( ESceneItemEventMode )
	ENUM_OPTION( SIEM_Default )
	ENUM_OPTION( SIEM_Mount )
	ENUM_OPTION( SIEM_Unmount )
END_ENUM_RTTI()

class CStorySceneEventEquipItem : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventEquipItem, CStorySceneEvent )

protected:
	CName	m_actor;

	CName	m_leftItem;
	CName	m_rightItem;

	ESceneItemEventMode	m_internalMode;;

	CName   m_ignoreItemsWithTag;

	Bool	m_instant;

public:
	CStorySceneEventEquipItem();
	CStorySceneEventEquipItem( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventEquipItem* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	RED_INLINE CName GetActorName() const { return m_actor; }
	RED_INLINE CName GetLeftItem() const { return m_leftItem; }
	RED_INLINE CName GetRightItem() const { return m_rightItem; }

	void PreprocessItems( CStoryScenePlayer* player ) const;
};

BEGIN_CLASS_RTTI( CStorySceneEventEquipItem )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT(  m_leftItem, TXT( "Item to be held in left hand" ), TXT( "ChooseItem" ) );
	PROPERTY_CUSTOM_EDIT(  m_rightItem, TXT( "Item to be held in right hand" ), TXT( "ChooseItem" ) );
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_ignoreItemsWithTag, TXT("When finding item by category") );
	PROPERTY_EDIT( m_internalMode, TXT("Experimental - use this to mount non hand held items") )
	PROPERTY_EDIT( m_instant, TXT("Equip instantly") );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneMorphEvent : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneMorphEvent, CStorySceneEvent )

protected:
	CName	m_actor;
	Float	m_weight;
	CName	m_morphComponentId;

public:
	CStorySceneMorphEvent();
	CStorySceneMorphEvent( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneMorphEvent* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual CName GetSubject() const override { return m_actor; }
	Float GetWeight() const { return m_weight; }
	CName GetMorphComponentId() const { return m_morphComponentId; }
};

BEGIN_CLASS_RTTI( CStorySceneMorphEvent )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_weight, TXT("Weight") );
PROPERTY_EDIT( m_morphComponentId, TXT("Morph ID") );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
