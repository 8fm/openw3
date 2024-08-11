/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "environmentManager.h"
#include "game.h"
#include "world.h"
#include "cameraComponent.h"

IMPLEMENT_ENGINE_CLASS( CEnvRadialBlurParameters );
IMPLEMENT_ENGINE_CLASS( CEnvDayCycleOverrideParameters );
IMPLEMENT_ENGINE_CLASS( CEnvDisplaySettingsParams );
IMPLEMENT_ENGINE_CLASS( CEnvBrightnessTintParameters );
IMPLEMENT_ENGINE_CLASS( CGameEnvironmentParams );

// --------------------------------------------------------------------------

CEnvRadialBlurParameters::CEnvRadialBlurParameters ()
	: m_radialBlurSource( 0, 0, 0, 0 )
	, m_radialBlurAmount( 0.0f )
	, m_sineWaveAmount( 0.0f )
	, m_sineWaveSpeed( 8.0f )
	, m_sineWaveFreq( 5.0f )
	, m_centerMultiplier( 0.0f )
	, m_distance( 10000.0f )
{}

// --------------------------------------------------------------------------

SEnvLightShaftParameters::SEnvLightShaftParameters()
{
	m_bloomTint = Vector(1.0f,1.0f,1.0f,1.0f);
	m_radius = 20.0f;
	m_distanceFade = 0.0f;
	m_outerRadius = 60.0f;
	m_innerRadius = 50.0f;
	m_bloomScale = 2.0f;
	m_bloomTreshold = 0.0f;
	m_blurScale = 100.0f;
	m_screenFactor = 0.5f;
	m_autoHideDistance = 15.0f;
	m_spotLight = false;
	m_additiveBlend = true;
	m_ignoreWorldSpace = true;
}

// --------------------------------------------------------------------------

CEnvDayCycleOverrideParameters::CEnvDayCycleOverrideParameters ()
	: m_fakeDayCycleEnable( false )
	, m_fakeDayCycleHour( 0 )
	, m_enableCustomSunRotation( false )
	, m_customSunRotation( 0, 45.0f, 45.0f )
{}

Vector CEnvDayCycleOverrideParameters::GetCustomSunDirection() const
{
	return -m_customSunRotation.ToMatrix().TransformVector( -Vector::EY );
}

// --------------------------------------------------------------------------

CEnvDisplaySettingsParams::CEnvDisplaySettingsParams ()
	: m_enableInstantAdaptation( false )
	, m_enableGlobalLightingTrajectory( false )
	, m_enableEnvProbeInstantUpdate( false )
	, m_allowEnvProbeUpdate ( true )
	, m_allowBloom( true )
	, m_allowColorMod( true )
	, m_allowAntialiasing( true )
	, m_allowGlobalFog( true )
	, m_allowDOF( true )
	, m_allowSSAO( true )
	, m_allowCloudsShadow( true )
	, m_allowVignette( true )
	, m_forceCutsceneDofMode( false )
	, m_allowWaterShader( true )
	, m_gamma( 2.2f )
	, m_disableTonemapping( false )
	, m_displayMode( EMM_None )
{}

// --------------------------------------------------------------------------

CGameEnvironmentParams::CGameEnvironmentParams ()
: m_cutsceneDofMode( false )
, m_fullscreenBlurIntensity( 0.f )
, m_gameUnderwaterBrightness( 0.f )
, m_cutsceneOrDialog( false )
{}

// --------------------------------------------------------------------------

CEnvRadialBlurParameters* CRadialBlurManager::GenerateNewRadialBlurParams()
{
	CEnvRadialBlurParameters* params = new CEnvRadialBlurParameters();
	m_radialBlurParams.PushBack( params );

	return params;
}

void CRadialBlurManager::RemoveRadialBlurParams( CEnvRadialBlurParameters* params )
{
	m_radialBlurParams.Remove( params );
}


void CRadialBlurManager::GenerateFinalParams( CEnvRadialBlurParameters& outRadialBlurParams, CCameraComponent* cameraComponent /* = NULL */ )
{
	CEnvRadialBlurParameters parameters;
	parameters.m_sineWaveFreq = 0.0f;
	parameters.m_sineWaveSpeed = 0.0f;

	TStaticArray< Float, 128 > weights;
	TStaticArray< Float, 128 > weightsNormalized;

	Float totalWeigth = 0.0f;

	Vector cameraPosition;
	Vector cameraVector;

	if ( cameraComponent )
	{
		cameraPosition = cameraComponent->GetWorldPosition();
		cameraVector = cameraComponent->GetWorldForward();
	}
	else
	{
		if ( GGame && GGame->GetActiveWorld() )
		{
			cameraPosition = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
			cameraVector = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraForward();
		}
		else
		{
			cameraPosition = Vector( 0.0f, 0.0f, 0.0f, 0.0f );
			cameraVector = Vector( 1.0f, 0.0f, 0.0f, 0.0f );
		}
	}

	for ( Uint32 i = 0; i < Min<Uint32>( m_radialBlurParams.Size(), 128 ); ++i )
	{
		const CEnvRadialBlurParameters& blurParams = *m_radialBlurParams[i];
		Float weight = Clamp( (blurParams.m_radialBlurSource - cameraPosition ).Normalized3().Dot3( cameraVector.Normalized3() ), 0.0f , 1.0f )
			* (Clamp( 1.0f - ((blurParams.m_radialBlurSource - cameraPosition).Mag3() - blurParams.m_distance * 0.5f)/blurParams.m_distance, 0.0f, 1.0f ) / 0.5f);
		weight = weight * blurParams.m_radialBlurAmount;
		weights.PushBack( weight );
		weightsNormalized.PushBack( weight );
		totalWeigth += weight;
	}

	if ( totalWeigth <= 0.0f )
	{
		outRadialBlurParams = parameters;
		return;
	}

	for ( Uint32 i = 0; i < weights.Size(); ++i )
	{
		weightsNormalized[i] = weights[i] / totalWeigth;
	}

	for ( Uint32 i = 0; i < Min<Uint32>( m_radialBlurParams.Size(), 128 ); ++i )
	{
		const CEnvRadialBlurParameters& blurParams = *m_radialBlurParams[i];
		parameters.m_radialBlurAmount += blurParams.m_radialBlurAmount * weights[i];
		parameters.m_radialBlurSource += Vector( blurParams.m_radialBlurSource ).Mul3( weightsNormalized[i] );
		parameters.m_sineWaveAmount += blurParams.m_sineWaveAmount * weights[i];
		parameters.m_sineWaveFreq += blurParams.m_sineWaveFreq * weights[i];
		parameters.m_sineWaveSpeed += blurParams.m_sineWaveSpeed * weights[i];
		parameters.m_centerMultiplier += blurParams.m_centerMultiplier * weightsNormalized[i];
	}

	outRadialBlurParams = parameters;
}

// --------------------------------------------------------------------------

CEnvBrightnessTintParameters* CBrightnessTintManager::GenerateNewParams()
{
	CEnvBrightnessTintParameters* params = new CEnvBrightnessTintParameters();
	m_params.PushBack( params );

	return params;
}

void CBrightnessTintManager::RemoveParams( CEnvBrightnessTintParameters* params )
{
	m_params.Remove( params );
}

void CBrightnessTintManager::GenerateFinalParams( CEnvBrightnessTintParameters& outParams )
{
	CEnvBrightnessTintParameters parameters;

	TStaticArray< Float, 128 > weights;

	Float totalWeigth = 0.0f;

	for ( Uint32 i = 0; i < Min<Uint32>( m_params.Size(), 128 ); ++i )
	{
		const CEnvBrightnessTintParameters& params = *m_params[i];
		const Float	weight = params.m_brightness;
		weights.PushBack( weight );
		totalWeigth += weight;
	}

	if ( totalWeigth <= 0.0f )
	{
		outParams = parameters;
		return;
	}

	parameters.m_brightness = 0.0f;
	parameters.m_tint = Vector::ZEROS;

	for ( Uint32 i = 0; i < weights.Size(); ++i )
	{
		weights[i] = weights[i] / totalWeigth;
	}

	const Float tintWeight = 1.0f / (Float)(Min<Uint32>( m_params.Size(), 128 ));

	for ( Uint32 i = 0; i < Min<Uint32>( m_params.Size(), 128 ); ++i )
	{
		const CEnvBrightnessTintParameters& params = *m_params[i];
		parameters.m_brightness += params.m_brightness * weights[i];
		parameters.m_tint += params.m_tint * tintWeight;
	}

	outParams = parameters;
}

// --------------------------------------------------------------------------

SEnvLightShaftParameters* CLightShaftManager::GenerateNewParams()
{
	SEnvLightShaftParameters* params = new SEnvLightShaftParameters();
	m_params.PushBack( params );

	return params;
}

void CLightShaftManager::RemoveParams( SEnvLightShaftParameters* params )
{
	m_params.Remove( params );
}

void CLightShaftManager::GenerateFinalParams( TStaticArray<SEnvLightShaftParameters,4>& outParams, CCameraComponent* cameraComponent /* = NULL*/ )
{
	outParams.ClearFast();

	Vector cameraPosition;
	Vector cameraVector;

	if ( cameraComponent )
	{
		cameraPosition = cameraComponent->GetWorldPosition();
		cameraVector = cameraComponent->GetWorldForward();
	}
	else
	{
		if ( GGame && GGame->GetActiveWorld() )
		{
			cameraPosition = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
			cameraVector = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraForward();
		}
		else
		{
			cameraPosition = Vector( 0.0f, 0.0f, 0.0f, 0.0f );
			cameraVector = Vector( 1.0f, 0.0f, 0.0f, 0.0f );
		}
	}

	Uint32 counter = 0;

	for( Uint32 counter = 0; counter < m_params.Size() && outParams.Size() < 4; ++counter )
	{
		SEnvLightShaftParameters* param = m_params[counter];

		Vector dirVector = param->m_source - cameraPosition;

		param->m_distanceFade = Clamp( (dirVector.Mag3() - param->m_autoHideDistance * 0.5f)/param->m_autoHideDistance, 0.0f, 1.0f ) / 0.5f;
		dirVector.Normalize3();
		
		if ( param->m_distanceFade < 1.0f && dirVector.Dot3( cameraVector ) > 0.0f && param->m_screenFactor > 0.0f && param->m_blurScale > 0.0f && param->m_radius > 0.0f )
		{
			outParams.PushBack( *param );
		}
	}
}
