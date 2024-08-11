#pragma once

#include "storySceneEvent.h"

class CStorySceneEventWorldPropPlacement : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventWorldPropPlacement, CStorySceneEvent );

protected:
	CName				m_propId;
	EngineTransform		m_placement;
	Bool				m_showHide;

public:
	CStorySceneEventWorldPropPlacement();
	CStorySceneEventWorldPropPlacement( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName id,const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventWorldPropPlacement* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	RED_INLINE EngineTransform& GetTransformRef()					{ return m_placement; }
	RED_INLINE const EngineTransform& GetTransformRef() const		{ return m_placement; }

#ifndef NO_EDITOR
	void SetPlacement( const EngineTransform& transform ) { m_placement = transform; }
#endif
};

BEGIN_CLASS_RTTI( CStorySceneEventWorldPropPlacement );
	PARENT_CLASS( CStorySceneEvent );
	PROPERTY_CUSTOM_EDIT( m_propId, TXT( "PropID" ), TXT( "DialogPropTag" )  )
	PROPERTY_EDIT( m_placement, TXT( "Placement" ) );
	PROPERTY_EDIT( m_showHide, TXT("") )
END_CLASS_RTTI();
