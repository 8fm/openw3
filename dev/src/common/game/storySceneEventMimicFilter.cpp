/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "storySceneEventMimicFilter.h"
#include "storyScenePlayer.h"
#include "sceneLog.h"
#include "../engine/mimicComponent.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

RED_DEFINE_NAMED_NAME( MIMIC_FILTER_ENABLE_VARIABLE, "enableFilter" );
RED_DEFINE_NAMED_NAME( MIMIC_FILTER_INDEX_VARIABLE, "filterIndex" );
RED_DEFINE_NAMED_NAME( MIMIC_FILTER_BLEND_VARIABLE, "filterBlend" );
RED_DEFINE_NAMED_NAME( MIMIC_FILTER_WEIGHT_VARIABLE, "filterWeight" );


IMPLEMENT_ENGINE_CLASS( CStorySceneEventMimicsFilter );

CStorySceneEventMimicsFilter::CStorySceneEventMimicsFilter()
	: m_actor( CName::NONE )
	, m_filterName( CName::NONE )
	, m_weight( 1.f )
	, m_useWeightCurve( false )
{

}

CStorySceneEventMimicsFilter::CStorySceneEventMimicsFilter( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEventDuration( eventName, sceneElement, startTime, 0.8f, trackName )
	, m_actor( actor )
	, m_filterName( CName::NONE )
	, m_weight( 1.f )
	, m_useWeightCurve( false )
{

}

CStorySceneEventMimicsFilter* CStorySceneEventMimicsFilter::Clone() const
{
	return new CStorySceneEventMimicsFilter( *this );
}

void CStorySceneEventMimicsFilter::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	Float weight = 0.f;
	if ( m_useWeightCurve )
	{
		weight = m_weight * Clamp( m_weightCurve.GetFloatValue( 0.f ), 0.f, 1.f );
	}

	Progress( scenePlayer, collector, weight );
}

void CStorySceneEventMimicsFilter::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	Float weight = m_weight * timeInfo.m_progress;
	if ( m_useWeightCurve )
	{
		weight = m_weight * Clamp( m_weightCurve.GetFloatValue( timeInfo.m_progress ), 0.f, 1.f );
	}

	Progress( scenePlayer, collector, weight );
}

void CStorySceneEventMimicsFilter::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );

	Float weight = m_weight;
	if ( m_useWeightCurve )
	{
		weight = m_weight * Clamp( m_weightCurve.GetFloatValue( 1.f ), 0.f, 1.f );
	}

	Progress( scenePlayer, collector, weight );
}

void CStorySceneEventMimicsFilter::Progress( CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, Float weight ) const
{
	// TODO - zmienic na collectora

	CActor* actor = scenePlayer->GetMappedActor( m_actor );
	if ( !actor )
	{
		return;
	}

	if ( !actor->HasMimic() )
	{
		return;
	}

	const Int32 filterIndex = GetMimicFilterIndex( actor );

	// TODO blend time jest 0
	actor->SetMimicVariable( CNAME( MIMIC_FILTER_BLEND_VARIABLE ), 0.f );
	//

	if ( weight > 0.f )
	{
		actor->SetMimicVariable( CNAME( MIMIC_FILTER_ENABLE_VARIABLE ), 1.0f );
	}
	else
	{
		actor->SetMimicVariable( CNAME( MIMIC_FILTER_ENABLE_VARIABLE ), 0.0f );
	}

	actor->SetMimicVariable( CNAME( MIMIC_FILTER_INDEX_VARIABLE ), (Float) filterIndex );
	actor->SetMimicVariable( CNAME( MIMIC_FILTER_WEIGHT_VARIABLE ), weight );
}

Int32 CStorySceneEventMimicsFilter::GetMimicFilterIndex( const CActor* actor ) const
{
	const CMimicComponent* mimic = actor->GetMimicComponent();
	if ( mimic && mimic->GetMimicFace() )
	{
		return mimic->GetExtendedMimics().FindFilterPose( m_filterName );
	}
	return -1;
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
