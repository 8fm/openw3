#include "build.h"
#include "StorySceneEventCameraLight.h"
#include "storyScenePlayer.h"
#include "..\engine\entity.h"
#include "..\engine\layer.h"
#include "..\engine\world.h"
#include "..\engine\environmentManager.h"
#include "storySceneEventsCollector_events.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventCameraLight );
IMPLEMENT_ENGINE_CLASS( SStorySceneCameraLightMod );
IMPLEMENT_RTTI_BITFIELD( ECameraLightBitfield );

CStorySceneEventCameraLight::CStorySceneEventCameraLight( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName ) 
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_cameraOffsetFront( 0.f )
{
}

CStorySceneEventCameraLight::CStorySceneEventCameraLight() 
	: m_cameraOffsetFront( 0.f )
{
}

CStorySceneEventCameraLight* CStorySceneEventCameraLight::Clone() const 
{
	return new CStorySceneEventCameraLight( *this );
}

void CStorySceneEventCameraLight::CopyLightData( SCameraLightModifiers& mod, const SStorySceneCameraLightMod& lightMod, Float dayTime )
{
	mod.SetIdentity( false );

	if ( lightMod.m_deactivateLight )
	{
		return;
	}

	mod.brightnessScale	= lightMod.m_brightnessScale;
	mod.radiusScale		= lightMod.m_radiusScale;
	mod.offsetFront		= lightMod.m_lightOffset.Y;
	mod.offsetRight		= lightMod.m_lightOffset.X;
	mod.offsetUp		= lightMod.m_lightOffset.Z;
	mod.absoluteValues	= lightMod.m_usageMask;

	if ( lightMod.m_useCustomAttenuation )
	{
		mod.attenuationOverrideAmount = 1;
		mod.attenuationOverride = lightMod.m_attenuation;
	}

	if ( lightMod.m_useCustomLight )
	{
		Vector color = SSimpleCurvePoint::GetValueColorScaledGammaToLinear( lightMod.m_overrideColor.GetValue( dayTime ), true );
		mod.colorOverrideAmount	= 1;
		mod.colorOverrideR		= color.X;
		mod.colorOverrideG		= color.Y;
		mod.colorOverrideB		= color.Z;
	}
}


void CStorySceneEventCameraLight::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	StorySceneEventsCollector::CameraLightProp evt;
	evt.m_event = this;

	SCameraLightsModifiersSetup &setup = evt.cameraSetup;
	SCameraLightsTypeModifier &mods = setup.m_modifiersByType[ m_cameralightType ];

	// Reset all
	setup.SetModifiersIdentity( false );
	
	Float dayTime = 0.f;
	if ( CWorld* world = scenePlayer->GetLayer()->GetWorld() )
	{
		if( CEnvironmentManager* envMgr = world->GetEnvironmentManager() )
		{
			dayTime = envMgr->GetCurrentGameTime().ToFloat() / (24.f * 60.f * 60.f);
		}
	}

	// Apply to given type
	CopyLightData( mods.m_lightModifier0, m_lightMod1, dayTime );
	CopyLightData( mods.m_lightModifier1, m_lightMod2, dayTime );

	//
	collector.AddEvent( evt );
}

SStorySceneCameraLightMod::SStorySceneCameraLightMod() 
	: m_deactivateLight( false )
	, m_useCustomLight( false )
	, m_lightOffset( Vector::ZERO_3D_POINT )
	, m_brightnessScale( 1.f )
	, m_radiusScale( 1.f )
	, m_useCustomAttenuation( false )
	, m_attenuation( 1.f )
	, m_usageMask( 0 )
{
	m_overrideColor.Reset( SCT_ColorScaled, 50.f, 0.f );
	m_overrideColor.AddPoint( 0.1f, SSimpleCurvePoint::BuildValue( Color::WHITE, 90.f ) );
}