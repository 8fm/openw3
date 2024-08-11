#include "build.h"
#include "storySceneEventLightProperties.h"
#include "storyScenePlayer.h"
#include "storySceneItems.h"
#include "../../common/core/gatheredResource.h"
#include "../engine/environmentManager.h"
#include "storySceneUtils.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( SStorySceneSpotLightProperties );
IMPLEMENT_ENGINE_CLASS( SStorySceneLightDimmerProperties );
IMPLEMENT_ENGINE_CLASS( CStorySceneEventLightProperties );
IMPLEMENT_RTTI_ENUM( ESceneEventLightColorSource )

CStorySceneEventLightProperties::CStorySceneEventLightProperties()
	: m_lightId( CName::NONE )
	, m_enabled( true )
	, m_color( 0, 0, 0 )
	, m_additiveChanges( true )
	, m_lightColorSource( ELCS_EnvLightColor1 )
	, m_useGlobalCoords( false )
{
	ResetCurves();	
}

CStorySceneEventLightProperties::CStorySceneEventLightProperties( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_lightId( actor )
	, m_enabled( true )
	, m_color( 0, 0, 0 )
	, m_additiveChanges( true )
	, m_lightColorSource( ELCS_EnvLightColor1 )
	, m_useGlobalCoords( false )
{
	ResetCurves();
}

CStorySceneEventLightProperties* CStorySceneEventLightProperties::Clone() const
{
	return new CStorySceneEventLightProperties( *this );
}

void CStorySceneEventLightProperties::ResetCurves()
{
	m_brightness.Reset( SCT_Float, 50.f, 0.f );
	m_brightness.AddPoint( 0.1f, SSimpleCurvePoint::BuildValue( Color::WHITE, 150.f ) );

	m_radius.Reset( SCT_Float, 10.f, 0.f );
	m_radius.AddPoint( 0.1f, SSimpleCurvePoint::BuildValue( Color::WHITE, 10.f ) );

	m_attenuation.Reset( SCT_Float, 2.f, 0.f );
	m_attenuation.AddPoint( 0.1f, SSimpleCurvePoint::BuildValue( Color::WHITE, 0.5f ) );
}


Float CStorySceneEventLightProperties::GetBrightness( CWorld* onWorld ) const 
{
	Float importTime = 0.1f;

	if ( onWorld )
	{
		if( CEnvironmentManager* envMgr = onWorld->GetEnvironmentManager())
		{
			importTime = envMgr->GetCurrentGameTime().ToFloat() / (24.f * 60.f * 60.f);
		}
	}

	return m_brightness.GetFloatValue( importTime );
}

Float CStorySceneEventLightProperties::GetAttenuation( CWorld* onWorld ) const 
{
	Float importTime = 0.1f;

	if ( onWorld )
	{
		if( CEnvironmentManager* envMgr = onWorld->GetEnvironmentManager())
		{
			importTime = envMgr->GetCurrentGameTime().ToFloat() / (24.f * 60.f * 60.f);
		}
	}

	return m_attenuation.GetFloatValue( importTime );
}


Float CStorySceneEventLightProperties::GetRadius( CWorld* onWorld ) const 
{
	Float importTime = 0.1f;

	if ( onWorld )
	{
		if( CEnvironmentManager* envMgr = onWorld->GetEnvironmentManager())
		{
			importTime = envMgr->GetCurrentGameTime().ToFloat() / (24.f * 60.f * 60.f);
		}
	}

	return m_radius.GetFloatValue( importTime );
}

Color CStorySceneEventLightProperties::GetColor( CStoryScenePlayer* scenePlayer ) const
{
	if ( m_lightColorSource != ELCS_CustomLightColor )
	{
		return StorySceneUtils::GetEnvLightVal( scenePlayer->GetLayer(), Int32( m_lightColorSource ) );
	}
	else
	{
		return m_color;
	}
}

void CStorySceneEventLightProperties::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	CWorld* onWorld = scenePlayer->GetLayer()->GetWorld();
	if ( m_lightId && onWorld )
	{	
		StorySceneEventsCollector::LightProperty evt( this, m_lightId );
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_enabled = m_enabled;
		evt.m_additiveChanges = m_additiveChanges;
		evt.m_color	= GetColor( scenePlayer );
		evt.m_radius = GetRadius( onWorld );
		evt.m_attenuation = GetAttenuation( onWorld );
		evt.m_brightness = GetBrightness( onWorld );	
		if ( m_useGlobalCoords )
		{
			evt.m_placementSS = StorySceneUtils::CalcSSFromWS( m_placement, scenePlayer->GetSceneDirector()->GetCurrentScenePlacement() );
		}
		else
		{
			evt.m_placementSS = m_placement;
		}
		evt.m_sceneTransformWS = scenePlayer->GetSceneDirector()->GetCurrentScenePlacement();
		evt.m_innerAngle = m_spotLightProperties.m_innerAngle;
		evt.m_outerAngle = m_spotLightProperties.m_outerAngle;
		evt.m_softness = m_spotLightProperties.m_softness;
		evt.m_flickering = m_flickering;
		evt.m_attachment = m_attachment;
		evt.m_lightTracker = m_lightTracker;
		evt.m_ambientLevel = m_dimmerProperties.GetAmbientLevel( onWorld );
		evt.m_marginFactor = m_dimmerProperties.GetMarginFactor( onWorld );

		collector.AddEvent( evt );
	}
}

#ifndef NO_EDITOR

void CStorySceneEventLightProperties::LoadDataFromOtherEvent( const CStorySceneEventLightProperties* other )
{
	m_brightness = other->m_brightness;
	m_lightTracker = other->m_lightTracker;
	m_radius = other->m_radius;
	m_attenuation = other->m_attenuation;
	m_color = other->m_color;
	m_lightColorSource = other->m_lightColorSource;
	m_flickering = other->m_flickering;
	m_spotLightProperties = other->m_spotLightProperties;
}

#endif

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif

Float SStorySceneLightDimmerProperties::GetMarginFactor( CWorld* onWorld ) const
{
	Float importTime = 0.1f;

	if ( onWorld )
	{
		if( CEnvironmentManager* envMgr = onWorld->GetEnvironmentManager())
		{
			importTime = envMgr->GetCurrentGameTime().ToFloat() / (24.f * 60.f * 60.f);
		}
	}

	return m_marginFactor.GetFloatValue( importTime );
}

Float SStorySceneLightDimmerProperties::GetAmbientLevel( CWorld* onWorld ) const
{
	Float importTime = 0.1f;

	if ( onWorld )
	{
		if( CEnvironmentManager* envMgr = onWorld->GetEnvironmentManager())
		{
			importTime = envMgr->GetCurrentGameTime().ToFloat() / (24.f * 60.f * 60.f);
		}
	}

	return m_ambientLevel.GetFloatValue( importTime );
}

SStorySceneLightDimmerProperties::SStorySceneLightDimmerProperties()
{
	m_ambientLevel.Reset( SCT_Float, 0.2f, 0.f );
	m_ambientLevel.AddPoint( 0.1f, SSimpleCurvePoint::BuildValue( Color::WHITE, 1.f ) );

	m_marginFactor.Reset( SCT_Float, 0.2f, 0.f );
	m_marginFactor.AddPoint( 0.1f, SSimpleCurvePoint::BuildValue( Color::WHITE, 1.f ) );
}
