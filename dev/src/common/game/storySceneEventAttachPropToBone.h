#pragma once

#include "storySceneEventDuration.h"

class CStorySceneEventAttachPropToSlot : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventAttachPropToSlot, CStorySceneEvent )

	CName	m_propId;
	CName	m_actorName;
	CName	m_slotName;
	
	Bool	m_activate;
	Bool	m_snapAtStart;
	Bool	m_showHide;

	EngineTransform	m_offset;

public:
	CStorySceneEventAttachPropToSlot();
	CStorySceneEventAttachPropToSlot( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor,const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventAttachPropToSlot* Clone() const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	EngineTransform& GetTransformRef(){ return m_offset; };
	CName GetAttachmentActor() const { return m_actorName; }
	CName GetSlotName() const { return m_slotName; }
	Bool SnapAtStart() const { return m_snapAtStart; }

#ifndef NO_EDITOR
	Bool IsActivation() const { return m_activate; }
#endif
};

BEGIN_CLASS_RTTI( CStorySceneEventAttachPropToSlot )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_propId, TXT("ID of property to be attached"), TXT( "DialogPropTag" ) );
	PROPERTY_EDIT( m_activate, TXT("Attach / Detach") );
	PROPERTY_CUSTOM_EDIT( m_actorName, TXT("Voicetag of actor ( Can be left empty when detaching )"), TXT( "DialogVoicePropTag" ) );
	PROPERTY_EDIT( m_slotName, TXT("Name of bone prop will be attached to ( Can be left empty when detaching )") );		
	PROPERTY_EDIT( m_snapAtStart, TXT("Teleport prop entity from current location to bone location") );	
	PROPERTY_EDIT( m_showHide, TXT("Show/Hide prop when processing this event") );		
	PROPERTY_EDIT( m_offset, TXT( "Prop offset relative to the bone" ) )
END_CLASS_RTTI();