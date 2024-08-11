/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "storySceneEventEnhancedCameraBlend.h"
#include "storyScenePlayer.h"
#include "storySceneEventsCollector_events.h"
#include "storySceneEventCustomCamera.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventEnhancedCameraBlend );

CStorySceneEventEnhancedCameraBlend::CStorySceneEventEnhancedCameraBlend()
	: CStorySceneEventCurveBlend()
{}

CStorySceneEventEnhancedCameraBlend::CStorySceneEventEnhancedCameraBlend( const String& eventName, CStorySceneElement* sceneElement, const String& trackName )
	: CStorySceneEventCurveBlend( eventName, sceneElement, trackName, 0.0f, 1.0f )
{}

CStorySceneEventEnhancedCameraBlend* CStorySceneEventEnhancedCameraBlend::Clone() const
{
	return new CStorySceneEventEnhancedCameraBlend( *this );
}

void CStorySceneEventEnhancedCameraBlend::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( !scenePlayer->IsGameplayNow() )
	{
		StorySceneEventsCollector::CameraBlend event( this );
		event.m_eventTimeAbs = timeInfo.m_timeAbs;
		event.m_eventTimeLocal = timeInfo.m_timeLocal;
		if ( GetCameraStateAt( 0.0f, &event.m_currentCameraState ) )
		{
			collector.AddEvent( event );
		}
	}
}

void CStorySceneEventEnhancedCameraBlend::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if ( !scenePlayer->IsGameplayNow() )
	{
		StorySceneEventsCollector::CameraBlend event( this );
		event.m_eventTimeAbs = timeInfo.m_timeAbs;
		event.m_eventTimeLocal = timeInfo.m_timeLocal;
		if ( GetCameraStateAt( data[ i_durationMS ], &event.m_currentCameraState ) )
		{
			collector.AddEvent( event );
		}
	}

	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );
}

void CStorySceneEventEnhancedCameraBlend::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	if ( !scenePlayer->IsGameplayNow() )
	{
		StorySceneEventsCollector::CameraBlend event( this );
		event.m_eventTimeAbs = timeInfo.m_timeAbs;
		event.m_eventTimeLocal = timeInfo.m_timeLocal;
		if ( GetCameraStateAt( timeInfo.m_timeLocal, &event.m_currentCameraState ) )
		{
			collector.AddEvent( event );
		}
	}
}

Uint32 CStorySceneEventEnhancedCameraBlend::GetNumExtraCurveDataStreams() const
{
	return EExtraCurveData_COUNT;
}

void CStorySceneEventEnhancedCameraBlend::GetKeyDataFromEvent( CStorySceneEvent* keyEvent, CurveKeyData& result ) const
{
	CStorySceneEventCamera* customCameraEvent = Cast< CStorySceneEventCamera >( keyEvent );
	ASSERT( customCameraEvent );
	if( const StorySceneCameraDefinition* cam = customCameraEvent->GetCameraDefinition() )
	{
		CopyCameraDefinitionToKeyData( result, cam );
	}	
}

void CStorySceneEventEnhancedCameraBlend::SetKeyDataToEvent( CStorySceneEvent* keyEvent, const CurveKeyData& data ) const
{
	CStorySceneEventCustomCamera* customCameraEvent = Cast< CStorySceneEventCustomCamera >( keyEvent );	
	ASSERT( customCameraEvent );
	if( StorySceneCameraDefinition* cam = customCameraEvent->GetCameraDefinition() )
	{
		CopyKeyDataToCameraDefinition( data, cam );
	}
	
}

Bool CStorySceneEventEnhancedCameraBlend::GetCameraStateAt( Float localTime, StorySceneCameraDefinition* result ) const
{
	// Copy values from base definition

	*result = m_baseCameraDefinition;

	// Get transform and extra data from the curve

	if ( m_curve.Size() < 2 )
	{
		return false;
	}

	CurveKeyData data;
	m_curve.GetRootTransform( localTime, data.m_transform );
	m_curve.GetExtraDataValues( localTime, data.m_extraData );

	// Copy into camera definition

	CopyKeyDataToCameraDefinition( data, result );
	return true;
}

Bool CStorySceneEventEnhancedCameraBlend::GetAbsoluteCameraStateAt( Float localTime, StorySceneCameraDefinition* result ) const
{
	// Copy values from base definition

	*result = m_baseCameraDefinition;

	// Get transform and extra data from the curve

	if ( m_curve.Size() < 2 )
	{
		return false;
	}

	CurveKeyData data;
	m_curve.GetAbsoluteTransform( localTime, data.m_transform );
	m_curve.GetExtraDataValues( localTime, data.m_extraData );

	// Copy into camera definition

	CopyKeyDataToCameraDefinition( data, result );
	return true;
}

void CStorySceneEventEnhancedCameraBlend::CopyCameraDefinitionToKeyData( CurveKeyData& dst, const StorySceneCameraDefinition* src )
{
	dst.m_transform = src->m_cameraTransform;
	dst.m_extraData[ EExtraCurveData_FOV ] = src->m_cameraFov;
	dst.m_extraData[ EExtraCurveData_Zoom ] = src->m_cameraZoom;
	dst.m_extraData[ EExtraCurveData_DOFBlurDistFar ] = src->m_dofBlurDistFar;
	dst.m_extraData[ EExtraCurveData_DOFBlurDistNear ] = src->m_dofBlurDistNear;
	dst.m_extraData[ EExtraCurveData_DOFFocusDistFar ] = src->m_dofFocusDistFar;
	dst.m_extraData[ EExtraCurveData_DOFFocusDistNear ] = src->m_dofFocusDistNear;
	dst.m_extraData[ EExtraCurveData_DOFIntensity ] = src->m_dofIntensity;
}

void CStorySceneEventEnhancedCameraBlend::CopyKeyDataToCameraDefinition( const CurveKeyData& src, StorySceneCameraDefinition* dst )
{
	dst->m_cameraTransform = src.m_transform;
	dst->m_cameraFov = src.m_extraData[ EExtraCurveData_FOV ];
	dst->m_cameraZoom = src.m_extraData[ EExtraCurveData_Zoom ];
	dst->m_dofBlurDistFar = src.m_extraData[ EExtraCurveData_DOFBlurDistFar ];
	dst->m_dofBlurDistNear = src.m_extraData[ EExtraCurveData_DOFBlurDistNear ];
	dst->m_dofFocusDistFar = src.m_extraData[ EExtraCurveData_DOFFocusDistFar ];
	dst->m_dofFocusDistNear = src.m_extraData[ EExtraCurveData_DOFFocusDistNear ];
	dst->m_dofIntensity = src.m_extraData[ EExtraCurveData_DOFIntensity ];
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
