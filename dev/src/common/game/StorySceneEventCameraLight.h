#pragma once
#include "storySceneEvent.h"



enum ECameraLightBitfield
{
	ECLB_AbsoluteBrightness = FLAG( 0 ),
	ECLB_AbsoluteRadius		= FLAG( 1 ),
	ECLB_AbsoluteOffset		= FLAG( 2 ),
};

BEGIN_BITFIELD_RTTI( ECameraLightBitfield, 1 )
	BITFIELD_OPTION( ECLB_AbsoluteBrightness )
	BITFIELD_OPTION( ECLB_AbsoluteRadius )
	BITFIELD_OPTION( ECLB_AbsoluteOffset )
END_BITFIELD_RTTI()

struct SStorySceneCameraLightMod
{
	SStorySceneCameraLightMod();
	DECLARE_RTTI_STRUCT( SStorySceneCameraLightMod )
	Bool							m_deactivateLight;
	Bool							m_useCustomLight;
	SSimpleCurve					m_overrideColor;
	Vector							m_lightOffset;
	Float							m_brightnessScale;
	Float							m_radiusScale;
	Bool							m_useCustomAttenuation;
	Float							m_attenuation;
	Uint8							m_usageMask;
};

BEGIN_CLASS_RTTI( SStorySceneCameraLightMod )
	PROPERTY_EDIT( m_deactivateLight, TXT("") )	
	PROPERTY_EDIT( m_useCustomLight, TXT("") )
	PROPERTY_EDIT( m_overrideColor, TXT("") )
	PROPERTY_EDIT( m_lightOffset, TXT("") )
	PROPERTY_EDIT( m_brightnessScale, TXT("") )
	PROPERTY_EDIT( m_radiusScale, TXT("") )
	PROPERTY_EDIT( m_useCustomAttenuation, TXT("") )
	PROPERTY_EDIT( m_attenuation, TXT("") )
	PROPERTY_BITFIELD_EDIT( m_usageMask, ECameraLightBitfield, TXT("") )	
END_CLASS_RTTI()

class CStorySceneEventCameraLight : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventCameraLight, CStorySceneEvent )

public:
	CStorySceneEventCameraLight();
	CStorySceneEventCameraLight( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventCameraLight* Clone() const override;

	ECameraLightModType			m_cameralightType;
	Float						m_cameraOffsetFront;
	SStorySceneCameraLightMod	m_lightMod1;
	SStorySceneCameraLightMod	m_lightMod2;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	
	static void CopyLightData( SCameraLightModifiers& mod, const SStorySceneCameraLightMod& lightMod, Float dayTime );	
};

BEGIN_CLASS_RTTI( CStorySceneEventCameraLight )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_EDIT( m_cameralightType, TXT("") )
	PROPERTY_INLINED( m_lightMod1, TXT("") )
	PROPERTY_INLINED( m_lightMod2, TXT("") )
END_CLASS_RTTI()

