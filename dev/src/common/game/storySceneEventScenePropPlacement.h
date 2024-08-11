
#pragma once

#include "storySceneEvent.h"

class CStorySceneEventScenePropPlacement : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventScenePropPlacement, CStorySceneEvent );

protected:
	CName				m_propId;
	EngineTransform		m_placement;
	Bool				m_showHide;
	Uint32				m_rotationCyclesPitch;
	Uint32				m_rotationCyclesRoll;
	Uint32				m_rotationCyclesYaw;

public:
	CStorySceneEventScenePropPlacement();
	CStorySceneEventScenePropPlacement( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName, const CName& id, const EngineTransform& placement );

	// compiler generated cctor is ok

	virtual CStorySceneEventScenePropPlacement* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	virtual CName GetSubject() const override { return m_propId; }

	RED_INLINE EngineTransform& GetTransformRef() { return m_placement; }
	RED_INLINE const EngineTransform& GetTransformRef() const { return m_placement; }

	RED_INLINE void GetRotationCycles( Uint32& pitch, Uint32& roll, Uint32& yaw ) const { pitch = m_rotationCyclesPitch; roll = m_rotationCyclesRoll; yaw = m_rotationCyclesYaw; }

#ifndef NO_EDITOR
	void SetPlacement( const EngineTransform& transform ) { m_placement = transform; }

	virtual void OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName ) override;
#endif
};

BEGIN_CLASS_RTTI( CStorySceneEventScenePropPlacement );
	PARENT_CLASS( CStorySceneEvent );
	PROPERTY_CUSTOM_EDIT( m_propId, TXT( "PropID" ), TXT( "DialogPropTag" )  )
	PROPERTY_EDIT( m_placement, TXT( "Placement" ) );
	PROPERTY_EDIT( m_showHide, TXT("") )
	PROPERTY_EDIT( m_rotationCyclesPitch, TXT( "Final rotation = m_placement + m_rotationCycles * 360" ) );
	PROPERTY_EDIT( m_rotationCyclesRoll, TXT( "Final rotation = m_placement + m_rotationCycles * 360" ) );
	PROPERTY_EDIT( m_rotationCyclesYaw, TXT( "Final rotation = m_placement + m_rotationCycles * 360" ) );
END_CLASS_RTTI();
