#pragma once
#include "customSceneEvents.h"
#include "..\..\common\engine\extAnimCutsceneEvent.h"


class CExtAnimCutsceneSurfaceEffect : public CExtAnimCutsceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CExtAnimCutsceneSurfaceEffect, CExtAnimCutsceneEvent )

protected:
	Vector m_position;
	ESceneEventSurfacePostFXType m_type;
	Float m_fadeInTime;
	Float m_fadeOutTime;
	Float m_durationTime;
	Float m_radius;
	Bool  m_worldPos;

public:
	CExtAnimCutsceneSurfaceEffect()
	{}
	CExtAnimCutsceneSurfaceEffect( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
		: CExtAnimCutsceneEvent( eventName, animationName, startTime, trackName ), m_worldPos( false )

	{}
public:
	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component, CCutsceneInstance* cs ) const;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneSurfaceEffect )
	PARENT_CLASS( CExtAnimCutsceneEvent )
	PROPERTY_EDIT( m_type, TXT( "" ) )
	PROPERTY_EDIT( m_worldPos, TXT( "" ) )	
	PROPERTY_EDIT( m_position, TXT( "" ) )
	PROPERTY_EDIT( m_radius, TXT( "" ) )
	PROPERTY_EDIT( m_fadeInTime, TXT( "" ) )
	PROPERTY_EDIT( m_fadeOutTime, TXT( "" ) )
	PROPERTY_EDIT( m_durationTime, TXT( "" ) )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CExtAnimCutsceneHideEntityEvent : public CExtAnimCutsceneEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneHideEntityEvent )

	CName m_entTohideTag;

public:
	CExtAnimCutsceneHideEntityEvent() {}
	CExtAnimCutsceneHideEntityEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
		: CExtAnimCutsceneEvent( eventName, animationName, startTime, trackName ) {}

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component, CCutsceneInstance* cs ) const;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneHideEntityEvent )
	PARENT_CLASS( CExtAnimCutsceneEvent )
	PROPERTY_EDIT( m_entTohideTag, TXT("") )
END_CLASS_RTTI()
