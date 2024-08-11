#include "build.h"
#include "r6CameraDirector.h"
#include "r6CameraComponent.h"
#include "../../common/engine/evaluatorVector.h"
IMPLEMENT_ENGINE_CLASS( CR6CameraDirector );






CR6CameraDirector::CR6CameraDirector()
	: m_cameraShake( 0.0f )
	, m_cameraShakeTime( 0.0f )
	, m_cameraShakeAmplitude( 0.0f )
	, m_autoCameraManagementOn( true )
{
}





void CR6CameraDirector::Update( Float timeDelta )
{
	if( m_autoCameraManagementOn )
	{
		UpdateAutoCameraManager( timeDelta );
	}

	TBaseClass::Update( timeDelta );

	// update camera shake
	m_cameraShake = Max<Float>( m_cameraShake - timeDelta, 0.0f );
}





void CR6CameraDirector::CacheCameraData()
{
	TBaseClass::CacheCameraData();

	// Camera shake (which is independent of which camera is active, blending etc).
	if( m_cameraShake > 0.001f )
	{
		Float amount = m_cameraShake / m_cameraShakeTime;
		//Vector tmpTranslation = m_cachedTransform.GetTranslation();
		//tmpTranslation += GRandomVector3().Normalized() * GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ) * amount * m_cameraShakeAmplitude;
		//m_cachedTransform.SetTranslation(tmpTranslation);
		m_cachedData.m_position += GRandomVector3().Normalized() * GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ) * amount * m_cameraShakeAmplitude;
	}
}






void CR6CameraDirector::UpdateAutoCameraManager( Float timeDelta )
{
	RED_UNUSED( timeDelta );

	CR6Player* player = Cast< CR6Player >( GGame->GetPlayerEntity() );

	if( !player )
	{
		return;
	}

	CR6CameraComponent* curr = player->GetCurrentAutoCameraComponent();
	RED_ASSERT( curr );

	if( m_cameras.Empty() || ( m_cameras.Back().GetCamera() != curr ) )
	{
		Float blendInTime = IsAnyCameraActive() ? curr->GetBlendInTime() : 0.0f;
		ActivateCamera( curr, player, blendInTime );
	}
}







void CR6CameraDirector::SetAllCamerasShake( Float time, Float amplitude )
{
	m_cameraShake = time;
	m_cameraShakeTime = time;
	m_cameraShakeAmplitude = amplitude;
}









// ------------------------------------------- SCRIPT EXPORTS -------------------------------------------

void CR6CameraDirector::funcSetAllCamerasShake( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, time, 0.0f );
	GET_PARAMETER( Float, amplitude, 0.0f );
	FINISH_PARAMETERS;

	SetAllCamerasShake( time, amplitude );
}





void CR6CameraDirector::funcGetPointInCachedLocal( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, worldPoint, Vector::ZEROS );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetPointInCachedLocal( worldPoint ) );
}




void CR6CameraDirector::funcTurnAutoCameraManagementOn( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	TurnAutoCameraManagementOn();
}




void CR6CameraDirector::funcTurnAutoCameraManagementOff( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	TurnAutoCameraManagementOff();
}


