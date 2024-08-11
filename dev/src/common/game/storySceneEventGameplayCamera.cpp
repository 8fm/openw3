#include "build.h"
#include "storySceneEventGameplayCamera.h"
#include "storyScenePlayer.h"
#include "storySceneVoicetagMapping.h"
#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventGameplayCamera );

CStorySceneEventGameplayCamera::CStorySceneEventGameplayCamera()
{

}

CStorySceneEventGameplayCamera::CStorySceneEventGameplayCamera( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName )
	: CStorySceneEventCamera( eventName, sceneElement, startTime, trackName )
{

}

/*
Cctor.

Compiler generated cctor would also copy instance vars - we don't want that.
*/
CStorySceneEventGameplayCamera::CStorySceneEventGameplayCamera( const CStorySceneEventGameplayCamera& other )
: CStorySceneEventCamera( other )
{}

void CStorySceneEventGameplayCamera::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );
	if ( !scenePlayer->IsGameplayNow() )
	{
		scenePlayer->SetResetGameplayCameraOnOutput( false ); // Don't reset (just switch) to avoid camera jump
	}
}

CStorySceneEventGameplayCamera* CStorySceneEventGameplayCamera::Clone() const
{
	return new CStorySceneEventGameplayCamera( *this );
}

void CStorySceneEventGameplayCamera::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_cameraResetScheduled;
	compiler << i_cameraDefinition;
}

void CStorySceneEventGameplayCamera::OnInitInstance( CStorySceneInstanceBuffer& instance ) const
{
	instance[ i_cameraResetScheduled ] = true;
}

Bool CStorySceneEventGameplayCamera::IsVolatileEvent() const 
{ 
	return true; 
}

const StorySceneCameraDefinition* CStorySceneEventGameplayCamera::GetCameraDefinition( CSceneEventFunctionSimpleArgs* args ) const
{
	if ( !args )
	{
		static StorySceneCameraDefinition s_dummyCameraDefinition;
		return &s_dummyCameraDefinition;
	}
	UpdateCameraDefinition( args );
	return &args->m_data[ i_cameraDefinition ];
}

StorySceneCameraDefinition* CStorySceneEventGameplayCamera::GetCameraDefinition( CSceneEventFunctionSimpleArgs* args )
{
	const StorySceneCameraDefinition* cameraDefinition = const_cast< const CStorySceneEventGameplayCamera* >( this )->GetCameraDefinition( args );
	return const_cast< StorySceneCameraDefinition* >( cameraDefinition );
}

void CStorySceneEventGameplayCamera::UpdateCameraDefinition( CSceneEventFunctionSimpleArgs* args ) const
{
	// Reset the camera once

	if ( Bool& cameraResetScheduled = args->m_data[ i_cameraResetScheduled ] )
	{
		GGame->ResetGameplayCamera();
		cameraResetScheduled = false;
	}

	// Get camera data

	ICamera::Data data;
	if ( !GGame->GetGameplayCameraData( data ) )
	{
		// Set up fake data such as to emulate gameplay camera
		
		if ( CActor* player = args->m_scenePlayer->GetSceneController()->FindPlayerActor() )
		{
			const Vector pivotPos = player->GetWorldPositionRef() + Vector( 0.0f, 0.0f, 1.8f );
			EulerAngles pivotAngles = player->GetWorldRotation();
			Vector pivotDir = pivotAngles.TransformPoint( Vector( 0.0f, 1.0f, 0.0f ) );

			data.m_position = pivotPos - pivotDir * 2.35f;
			data.m_rotation = pivotAngles;
			data.m_rotation.Pitch *= -1.0f;
			data.m_fov = 60.0f;
		}
	}

	// Transform camera transform into "scene player" space

	const EngineTransform scenePlayerTransformW = args->m_scenePlayer->GetSceneDirector()->GetCurrentScenePlacement();

	Matrix scenePlayerInvW;
	scenePlayerTransformW.CalcWorldToLocal( scenePlayerInvW );

	EngineTransform cameraTransformW;
	cameraTransformW.SetPosition( data.m_position );
	cameraTransformW.SetRotation( data.m_rotation );

	Matrix cameraW;
	cameraTransformW.CalcLocalToWorld( cameraW );

	Matrix cameraL;
	cameraL = cameraW * scenePlayerInvW;

	// Update camera definition

	StorySceneCameraDefinition& cameraDefinition = args->m_data[ i_cameraDefinition ];

	cameraDefinition.m_cameraTransform.SetPosition( cameraL.GetTranslationRef() );
	cameraDefinition.m_cameraTransform.SetRotation( cameraL.ToEulerAngles() );

	cameraDefinition.m_cameraFov = data.m_fov;

	cameraDefinition.m_dofIntensity = data.m_dofParams.dofIntensity;
	cameraDefinition.m_dofBlurDistFar = data.m_dofParams.dofBlurDistFar;
	cameraDefinition.m_dofBlurDistNear = data.m_dofParams.dofFocusDistNear;
	cameraDefinition.m_dofFocusDistFar = data.m_dofParams.dofFocusDistFar;
	cameraDefinition.m_dofFocusDistNear = data.m_dofParams.dofFocusDistNear;

	cameraDefinition.m_bokehDofParams = data.m_bokehDofParams;
}
