/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "storySceneEventCurveAnimation.h"
#include "storyScenePlayer.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventCurveAnimation );

CStorySceneEventCurveAnimation::CStorySceneEventCurveAnimation()
	: CStorySceneEventDuration()
{
	InitCurve();
}

CStorySceneEventCurveAnimation::CStorySceneEventCurveAnimation( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName, const EngineTransform& defaultPlacement )
	: CStorySceneEventDuration( eventName, sceneElement, startTime, 1.0f, trackName )
{
	InitCurve();
}

CStorySceneEventCurveAnimation* CStorySceneEventCurveAnimation::Clone() const
{
	return new CStorySceneEventCurveAnimation( *this );
}

void CStorySceneEventCurveAnimation::InitCurve()
{
	m_curve.SetCurveType( ECurveType_EngineTransform, NULL, false );
	m_curve.AddControlPoint( 0.0f, Vector::ZEROS );
	m_curve.AddControlPoint( 1.0f, Vector( 0.0f, 3.0f, 0.0f ) );
	m_curve.SetPositionInterpolationMode( ECurveInterpolationMode_Automatic );
	m_curve.SetTransformationRelativeMode( ECurveRelativeMode_InitialTransform );
	m_curve.SetShowFlags( SHOW_CurveAnimations );
	m_curve.SetLooping( false );
	m_curve.SetColor( Color::YELLOW );
	m_curve.EnableAutomaticTimeByDistanceRecalculation( true );
	m_curve.EnableEaseParams( true );
}

void CStorySceneEventCurveAnimation::OnInit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const
{
	TBaseClass::OnInit( data, scenePlayer );

	SMultiCurve* curve = const_cast<SMultiCurve*>( &m_curve );
	ASSERT( !curve->GetParent(), TXT("Using single curve for multiple animations at the same time not supported.") );
	curve->SetParent( scenePlayer );
}

void CStorySceneEventCurveAnimation::OnDeinit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const
{
	TBaseClass::OnDeinit( data, scenePlayer );

	SMultiCurve* curve = const_cast<SMultiCurve*>( &m_curve );
	curve->SetParent( NULL );
}

void CStorySceneEventCurveAnimation::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );
}

void CStorySceneEventCurveAnimation::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	const Float normalizedTime = timeInfo.m_timeLocal / data[ i_durationMS ];

	EngineTransform transform;
	m_curve.GetRootTransform( normalizedTime, transform );

	// TODO: Do something with the transform
}

void CStorySceneEventCurveAnimation::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
