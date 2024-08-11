/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "wetnessComponent.h"
#include "world.h"
#include "component.h"
#include "entity.h"
#include "layer.h"
#include "globalWaterUpdateParams.h"
#include "weatherManager.h"

IMPLEMENT_ENGINE_CLASS( CWetnessComponent );

CWetnessComponent::CWetnessComponent()
	: m_tickTimeMarker( -1 )
	, m_weatherManager( nullptr )
	, m_blendInFromOcean( 3.0f )
	, m_blendInFromRain( 6.0f )
	, m_blendOutFromOcean( 90.0f )
	, m_blendOutFromRain( 3.0f )
	, m_shouldUpdateFromOcean( true )
{
}

void CWetnessComponent::OnAttached(CWorld* world)
{
	TBaseClass::OnAttached(world);

	CEnvironmentManager* envMgr = world->GetEnvironmentManager();
	if ( envMgr )
	{
		m_weatherManager = envMgr->GetWeatherManager();
	}

	// default speed
	const Float defSpeed = 0.01f;

	// how fast we're adding up wetness from water
	m_wetnessParams.m_blendInFromOceanSpeed = m_blendInFromOcean > 0.f ? ( 1.f / Clamp<Float>( m_blendInFromOcean, 0.001f , NumericLimits<Float>::Infinity() ) ) : defSpeed;

	// how long we wait until complete dry when leaving water
	m_wetnessParams.m_blendOutFromOceanSpeed = m_blendOutFromOcean > 0.f ? ( 1.f / Clamp<Float>( m_blendOutFromOcean, 0.001f , NumericLimits<Float>::Infinity() ) ) : defSpeed;
	
	// how fast we're adding up wetness from rain
	m_wetnessParams.m_blendInFromRainSpeed = m_blendInFromRain > 0.f ? ( 1.f /  Clamp<Float>( m_blendInFromRain , 0.001f , NumericLimits<Float>::Infinity() ) ) : defSpeed;

	// how long we wait until complete dry when leaving rain
	m_wetnessParams.m_blendOutFromRainSpeed = m_blendOutFromRain > 0.f ? ( 1.f /  Clamp<Float>( m_blendOutFromRain , 0.001f , NumericLimits<Float>::Infinity() ) ) : defSpeed;

}


void CWetnessComponent::OnDetached( CWorld* world )
{
	m_weatherManager = nullptr;
	TBaseClass::OnDetached(world);
}

void CWetnessComponent::OnCinematicStorySceneStarted()
{
	m_shouldUpdateFromOcean = false;
}

void CWetnessComponent::OnCinematicStorySceneEnded() 
{
	m_shouldUpdateFromOcean = true;
}

void CWetnessComponent::UpdateValues( SWetnessParams& params )
{
	// we have to update params
	CWorld* world = GetEntity()->GetLayer()->GetWorld();
	if ( !m_weatherManager || !world )
	{
		return;
	}

	const Uint64 currTick = GEngine->GetCurrentEngineTick();

	if ( currTick != m_tickTimeMarker )
	{
		// check different dt
		const Float dt = GEngine->GetLastTimeDelta();
		// update l2w
		m_localToWorld = GetEntity()->GetLocalToWorld();

		const Vector& pos = m_localToWorld.GetTranslationRef();
		Uint32 lod = 0;
		Float dist_to_cam_sqr_2d = Vector(world->GetCameraPosition() - pos).SquareMag2();
		
		if( dist_to_cam_sqr_2d > 900.0f )
		{
			lod = 2;
		}
		else if( dist_to_cam_sqr_2d > 400.0f )
		{
			lod = 1;
		}
		
		m_wetnessParams.m_waterLevelZ = world->GetWaterLevel( pos, lod );
		
		Float weatherWetness = 0.f;
		
		// if player is in interior, decrease rain wetness
		if( m_weatherManager->GetPlayerIsInInterior() ) 
		{	
			m_wetnessParams.m_interior -= dt * m_wetnessParams.m_blendOutFromRainSpeed;
		}
		else
		{
			weatherWetness = m_weatherManager->GetDelayedWetSurfaceEffectStrength();
			// if player is outside and its raining, increase rain wetness
			if( m_weatherManager->GetEffectStrength( EWeatherEffectType::WET_RAIN ) > 0.0f )
			{
				m_wetnessParams.m_interior += dt * m_wetnessParams.m_blendInFromRainSpeed;
				m_wetnessParams.m_interior = Min( m_wetnessParams.m_interior, pow(weatherWetness , 4.3f) );
			}
			// if player is outside and its not raining, decrease rain wetness
			else
			{
				m_wetnessParams.m_interior -= dt * m_wetnessParams.m_blendOutFromRainSpeed;
				m_wetnessParams.m_interior = Max( m_wetnessParams.m_interior, weatherWetness );
			}			
		}		

		//clamp value
		m_wetnessParams.m_interior = Clamp<Float>( m_wetnessParams.m_interior, 0.f, 1.0f );

		// cache tick marker
		m_tickTimeMarker = currTick;
	}

	// update output params
	params = m_wetnessParams;
}

////////////////////////////////////////////
//
//		CWetnessSupplier
//
////////////////////////////////////////////

CWetnessSupplier::CWetnessSupplier( const CWetnessComponent* wc, Uint32 numBones )
	: m_wetnessComponent( wc )
	, m_boneCount( numBones )
{
	m_wetnessDataFromOcean.Resize( numBones );
	m_wetnessData.Resize( numBones );
}

CWetnessSupplier::~CWetnessSupplier()
{
}

void CWetnessSupplier::CalcWetness( const TDynArray< Matrix >& bonesWS )
{
	RED_FATAL_ASSERT( bonesWS.Size() == m_boneCount, "It's not that easy to calc wetness if bones cout is different, right?" );
	if ( CWetnessComponent* wc = m_wetnessComponent.Get() )
	{
		// check different dt 
		// TODO: play with delta time to support time scaling but without including game project here.
		const Float dt = GEngine->GetLastTimeDelta();

		// water level epsilon than will trigger wetness from ocean
		const Float waterOffset = 0.25f;

		CWetnessComponent::SWetnessParams params;
		wc->UpdateValues( params );

		const Uint32 boneCount = bonesWS.Size();

		//try dry out from top down
		const Float height = 1.5f;
		const Float l2wZPos = wc->GetLocalToWorld().GetTranslationRef().Z;
		
		// calculate wetness
		for( Uint32 i=0; i<boneCount; ++i )
		{
			const Vector& bonePosWS = bonesWS[i].GetTranslationRef();
				
			const Float boneHegiht = Clamp< Float >( bonePosWS.Z-l2wZPos, 0.01f, height );

			// top down dry out factor
			const Float dryOutWeight = 1.f + 0.5f * ( boneHegiht / height );

			// increase or decrease general wetness based on water level
			if( ( ( bonePosWS.Z - waterOffset ) < params.m_waterLevelZ ) && wc->ShouldUpdateWetnessFromOcean() )
			{
				m_wetnessDataFromOcean[i] = m_wetnessDataFromOcean[i] + dt * params.m_blendInFromOceanSpeed;
				m_wetnessDataFromOcean[i] = Clamp<Float>( ( m_wetnessDataFromOcean[i] ), 0.f, 1.f );
			}
			else
			{
				m_wetnessDataFromOcean[i] -= dt * params.m_blendOutFromOceanSpeed * dryOutWeight;
				m_wetnessDataFromOcean[i] = Clamp<Float>( ( m_wetnessDataFromOcean[i] ), 0.f, 1.f );
			}
		
			// add rain wetness based on accumulated interior factor
			m_wetnessData[ i ] = Clamp<Float>( ( m_wetnessDataFromOcean[i] + params.m_interior ), 0.f, 1.f );
		}
	}
}
