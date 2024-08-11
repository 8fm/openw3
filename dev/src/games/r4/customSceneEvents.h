#pragma once
#include "..\..\common\game\storySceneEvent.h"

//////////////////////////////////////////////////////////////////////////
//copy of CGameplayEffects::ESurfacePostFXType in renderer
enum ESceneEventSurfacePostFXType
{
	ES_Frost,
	ES_Burn
};

BEGIN_ENUM_RTTI( ESceneEventSurfacePostFXType )
	ENUM_OPTION( ES_Frost )
	ENUM_OPTION( ES_Burn )
	END_ENUM_RTTI()

class CStorySceneEventSurfaceEffect : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventSurfaceEffect, CStorySceneEvent )

protected:
	Vector m_position;
	ESceneEventSurfacePostFXType m_type;
	Float m_fadeInTime;
	Float m_fadeOutTime;
	Float m_durationTime;
	Float m_radius;

public:
	CStorySceneEventSurfaceEffect()
	{}
	CStorySceneEventSurfaceEffect( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
		: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	{}

	// compiler generated cctor is ok

	virtual CStorySceneEventSurfaceEffect* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventSurfaceEffect )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_EDIT( m_type, TXT( "" ) )
	PROPERTY_EDIT( m_position, TXT( "" ) )
	PROPERTY_EDIT( m_fadeInTime, TXT( "" ) )
	PROPERTY_EDIT( m_fadeOutTime, TXT( "" ) )
	PROPERTY_EDIT( m_durationTime, TXT( "" ) )
	PROPERTY_EDIT( m_radius, TXT( "" ) )
END_CLASS_RTTI()
