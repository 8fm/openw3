/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "storySceneEventBlend.h"
#include "storyScenePlayer.h"
#include "storySceneDirector.h"
#include "storySceneEventsCollector_events.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventBlend );

CStorySceneEventBlend::CStorySceneEventBlend()
	: CStorySceneEventDuration()
{
}

CStorySceneEventBlend::CStorySceneEventBlend( const String& eventName, CStorySceneElement* sceneElement, const String& trackName, Float startTime, Float duration )
	: CStorySceneEventDuration( eventName, sceneElement, startTime, duration, trackName )
{
}

void CStorySceneEventBlend::OnGuidChanged( CGUID oldGuid, CGUID newGuid )
{
	CStorySceneEventDuration::OnGuidChanged( oldGuid, newGuid );

	for( Uint32 i = 0, numKeys = m_keys.Size(); i < numKeys; ++i )
	{
		if( m_keys[ i ] == oldGuid )
		{
			m_keys[ i ] = newGuid;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventCurveBlend );

CStorySceneEventCurveBlend::CStorySceneEventCurveBlend()
	: CStorySceneEventBlend()
{
}

CStorySceneEventCurveBlend::CStorySceneEventCurveBlend( const String& eventName, CStorySceneElement* sceneElement, const String& trackName, Float startTime, Float duration )
	: CStorySceneEventBlend( eventName, sceneElement, trackName, startTime, duration )
{
}

void CStorySceneEventCurveBlend::InitCurve()
{
	const Uint32 numExtraCurveDataStreams = GetNumExtraCurveDataStreams();
	ASSERT( numExtraCurveDataStreams <= ARRAY_COUNT_U32( CurveKeyData().m_extraData ) );

	m_curve.Reset();
	m_curve.SetCurveType( ECurveType_EngineTransform, NULL, false, numExtraCurveDataStreams );
	m_curve.SetPositionInterpolationMode( ECurveInterpolationMode_Automatic );
	m_curve.SetTransformationRelativeMode( ECurveRelativeMode_InitialTransform );
	m_curve.SetShowFlags( SHOW_CurveAnimations );
	m_curve.SetLooping( false );
	m_curve.SetColor( Color::YELLOW );
	m_curve.EnableAutomaticTimeByDistanceRecalculation( false );
	m_curve.EnableEaseParams( true );
}

void CStorySceneEventCurveBlend::OnInit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const
{
	TBaseClass::OnInit( data, scenePlayer );

	SMultiCurve* curve = const_cast<SMultiCurve*>( &m_curve );
	ASSERT( !curve->GetParent(), TXT("Using single curve for multiple animations at the same time not supported.") );
	curve->SetParent( scenePlayer );

	// Set curve total time - we need to refresh it to make it correct in current locale. Without this, curve
	// total time would be correct only if current locale is the same as locale in which this event was created.
	curve->SetTotalTime( data[ i_durationMS ] );
}

void CStorySceneEventCurveBlend::OnDeinit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const
{
	TBaseClass::OnDeinit( data, scenePlayer );

	SMultiCurve* curve = const_cast<SMultiCurve*>( &m_curve );
	curve->SetParent( NULL );
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
