/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "storySceneEvent.h"
#include "storySceneEventCustomCamera.h"
#include "storyScenePlayer.h"
#include "storySceneEventsCollector_events.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventCustomCamera );

CStorySceneEventCustomCamera::CStorySceneEventCustomCamera()
	: CStorySceneEventCamera()
	, m_cameraTranslation()
	, m_cameraRotation()
	, m_cameraZoom( 0.0f )
	, m_dofFocusDistFar( 0.0f )
	, m_dofBlurDistFar( 0.0f )
	, m_dofIntensity( 0.0f )
	, m_dofFocusDistNear( 0.0f )
	, m_dofBlurDistNear( 0.0f )
	, m_enableCameraNoise( true )
{

}

CStorySceneEventCustomCamera::CStorySceneEventCustomCamera( const String& eventName, CStorySceneElement* sceneElement,  Float startTime, 
															const StorySceneCameraDefinition& camDef, const String& trackName )
	: CStorySceneEventCamera( eventName, sceneElement, startTime, trackName )
	, m_cameraDefinition( camDef )
	, m_cameraTranslation( camDef.m_cameraTransform.GetPosition() )
	, m_cameraRotation( camDef.m_cameraTransform.GetRotation() )
	, m_cameraZoom( camDef.m_cameraZoom )
	, m_cameraFov( camDef.m_cameraFov )
	, m_dofFocusDistFar( camDef.m_dofFocusDistFar )
	, m_dofBlurDistFar( camDef.m_dofBlurDistFar )
	, m_dofIntensity( camDef.m_dofIntensity )
	, m_dofFocusDistNear( camDef.m_dofFocusDistNear )
	, m_dofBlurDistNear( camDef.m_dofBlurDistNear )
	, m_enableCameraNoise( true )

{
}

CStorySceneEventCustomCamera::CStorySceneEventCustomCamera( const String& eventName, CStorySceneElement* sceneElement,
		   Float startTime, const Vector& cameraTranslation, const EulerAngles& cameraRotation,
		   Float cameraZoom, Float cameraFov, Float dofFocusDistFar, Float dofBlurDistFar,
		   Float dofIntensity, Float dofFocusDistNear, Float dofBlurDistNear, const ApertureDofParams& apertureDofParams,
		   const String& trackName )
	: CStorySceneEventCamera( eventName, sceneElement, startTime, trackName )
	, m_cameraTranslation( cameraTranslation )
	, m_cameraRotation( cameraRotation )
	, m_cameraZoom( cameraZoom )
	, m_cameraFov( cameraFov )
	, m_dofFocusDistFar( dofFocusDistFar )
	, m_dofBlurDistFar( dofBlurDistFar )
	, m_dofIntensity( dofIntensity )
	, m_dofFocusDistNear( dofFocusDistNear )
	, m_dofBlurDistNear( dofBlurDistNear )
	, m_enableCameraNoise( true )
{
	m_cameraDefinition.m_cameraName = CName( eventName );
	m_cameraDefinition.m_cameraTransform.SetPosition( cameraTranslation );
	m_cameraDefinition.m_cameraTransform.SetRotation( cameraRotation );
	m_cameraDefinition.m_cameraZoom = cameraZoom;
	m_cameraDefinition.m_cameraFov = cameraFov;
	m_cameraDefinition.m_dofFocusDistFar = dofFocusDistFar;
	m_cameraDefinition.m_dofBlurDistFar = dofBlurDistFar;
	m_cameraDefinition.m_dofIntensity = dofIntensity;
	m_cameraDefinition.m_dofFocusDistNear = dofFocusDistNear;
	m_cameraDefinition.m_dofBlurDistNear = dofBlurDistNear;
	m_cameraDefinition.m_enableCameraNoise = true;
	m_cameraDefinition.m_dof = apertureDofParams;
}

CStorySceneEventCustomCamera* CStorySceneEventCustomCamera::Clone() const
{
	return new CStorySceneEventCustomCamera( *this );
}

void CStorySceneEventCustomCamera::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( !scenePlayer->IsInGameplay() )
	{
		StorySceneEventsCollector::CameraShot event( this );
		event.m_eventTimeAbs = timeInfo.m_timeAbs;
		event.m_eventTimeLocal = timeInfo.m_timeLocal;
		event.m_enableCameraNoise = m_enableCameraNoise;
		event.m_definition = m_cameraDefinition;

		collector.AddEvent( event );
	}
}

void CStorySceneEventCustomCamera::Serialize( IFile& file )
{
	TBaseClass::Serialize( file );

	if ( file.IsReader() && m_cameraDefinition.m_dof.IsDefault() )
	{
		m_cameraDefinition.m_dof.FromEngineDofParams( m_cameraDefinition.m_dofFocusDistNear, m_cameraDefinition.m_dofFocusDistFar );
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
