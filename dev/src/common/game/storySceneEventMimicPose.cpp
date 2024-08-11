/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "storySceneEventMimicPose.h"
#include "storyScenePlayer.h"
#include "sceneLog.h"
#include "../engine/mimicComponent.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

RED_DEFINE_NAMED_NAME( MIMIC_POSE_ENABLE_VARIABLE, "enablePose" );
RED_DEFINE_NAMED_NAME( MIMIC_POSE_INDEX_VARIABLE, "poseIndex" );
RED_DEFINE_NAMED_NAME( MIMIC_POSE_BLEND_VARIABLE, "poseBlend" );
RED_DEFINE_NAMED_NAME( MIMIC_POSE_WEIGHT_VARIABLE, "poseWeight" );

IMPLEMENT_ENGINE_CLASS( CStorySceneEventMimicsPose );

CStorySceneEventMimicsPose::CStorySceneEventMimicsPose()
	: m_actor( CName::NONE )
	, m_poseName( CName::NONE )
	, m_weight( 1.f )
	, m_useWeightCurve( false )
{

}

CStorySceneEventMimicsPose::CStorySceneEventMimicsPose( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEventDuration( eventName, sceneElement, startTime, 0.8f, trackName )
	, m_actor( actor )
	, m_poseName( CName::NONE )
	, m_weight( 1.f )
	, m_useWeightCurve( false )
{

}

CStorySceneEventMimicsPose* CStorySceneEventMimicsPose::Clone() const
{
	return new CStorySceneEventMimicsPose( *this );
}

void CStorySceneEventMimicsPose::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	Float weight = 0.f;
	if ( m_useWeightCurve )
	{
		weight = m_weight * Clamp( m_weightCurve.GetFloatValue( 0.f ), 0.f, 1.f );
	}

	Progress( scenePlayer, collector, weight );
}

void CStorySceneEventMimicsPose::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	Float weight = m_weight * timeInfo.m_progress;
	if ( m_useWeightCurve )
	{
		weight = m_weight * Clamp( m_weightCurve.GetFloatValue( timeInfo.m_progress ), 0.f, 1.f );
	}

	Progress( scenePlayer, collector, weight );
}

void CStorySceneEventMimicsPose::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );

	Float weight = m_weight;
	if ( m_useWeightCurve )
	{
		weight = m_weight * Clamp( m_weightCurve.GetFloatValue( 1.f ), 0.f, 1.f );
	}

	Progress( scenePlayer, collector, weight );
}

Int32 CStorySceneEventMimicsPose::GetMimicPoseIndex( const CActor* actor ) const
{
	const CMimicComponent* mimic = actor->GetMimicComponent();
	if ( mimic && mimic->GetMimicFace())
	{
		return mimic->GetExtendedMimics().FindTrackPose( m_poseName );
	}
	return -1;
}

void CStorySceneEventMimicsPose::Progress( CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, Float weight ) const
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

	const Int32 poseIndex = GetMimicPoseIndex( actor );

	// TODO blend time jest 0
	actor->SetMimicVariable( CNAME( MIMIC_POSE_BLEND_VARIABLE ), 0.f );
	//

	if ( weight > 0.f )
	{
		actor->SetMimicVariable( CNAME( MIMIC_POSE_ENABLE_VARIABLE ), 1.0f );
	}
	else
	{
		actor->SetMimicVariable( CNAME( MIMIC_POSE_ENABLE_VARIABLE ), 0.0f );
	}

	actor->SetMimicVariable( CNAME( MIMIC_POSE_INDEX_VARIABLE ), (Float) poseIndex );
	actor->SetMimicVariable( CNAME( MIMIC_POSE_WEIGHT_VARIABLE ), weight );
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
