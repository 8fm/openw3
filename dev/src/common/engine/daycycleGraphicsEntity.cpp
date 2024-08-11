#include "build.h"
#include "renderCommands.h"
#include "environmentManager.h"
#include "gameTime.h"
#include "daycycleGraphicsEntity.h"
#include "lightComponent.h"
#include "renderProxyIterator.h"
#include "componentIterator.h"
#include "world.h"
#include "layer.h"
#include "tickManager.h"
#include "baseEngine.h"

IMPLEMENT_ENGINE_CLASS( CDaycycleGraphicsEntity );

CDaycycleGraphicsEntity::CDaycycleGraphicsEntity()
	: m_startEmissiveHour ( 22.0f )
	, m_startEmissiveFadeTime ( 0.1f )
	, m_endEmissiveHour ( 5.0f )
	, m_endEmissiveFadeTime ( 0.5f )
	, m_randomRange( 0.5f )
	, m_lightBrightnessWhenOnMin( 1.0f )
	, m_lightBrightnessWhenOnMax( 1.0f )
	, m_engineValueXWhenOff( 0.0f )
	, m_engineValueXWhenOnMin( 1.0f )
	, m_engineValueXWhenOnMax( 1.0f )
	, m_engineValueYWhenOff( 0.0f )
	, m_engineValueYWhenOn( 1.0f )
	, m_engineValueZWhenOff( 0.0f )
	, m_engineValueZWhenOn( 1.0f )
	, m_engineValueWWhenOff( 0.0f )
	, m_engineValueWWhenOn( 1.0f )
	, m_engineValueColorWhenOff( Color::WHITE )
	, m_engineValueColorWhenOn( Color::WHITE )
	, m_startStopEffects( false )
	, m_startStopLightsAndEngineValues( true )
	, m_particleAlphaWhenOff( 0.0f )
	, m_particleAlphaWhenOnMin( 1.0f )
	, m_particleAlphaWhenOnMax( 1.0f )
	, m_flickeringPeriod( 0.1f )
	, m_lightRadius( 1.0f )
	, m_overrideRadius( false )
	, m_lightAutoHideDistance( 5.0f )
	, m_lightAutoHideRange( 2.0f )
	, m_lightRandomOffsetX( 0.02f )
	, m_lightRandomOffsetY( 0.02f )
	, m_lightRandomOffsetZ( 0.02f )
	, m_wasChangingParams( false )
{
}

void CDaycycleGraphicsEntity::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetTickManager()->AddEntity( this );	

	m_randomizedStartHour = m_startEmissiveHour + m_randomRange * GEngine->GetRandomNumberGenerator().Get< Float >( -0.5f , 0.5f );
	m_randomizedStartHourFadeEnd = m_startEmissiveHour + m_startEmissiveFadeTime;
	m_randomizedEndHour = m_endEmissiveHour + m_randomRange * GEngine->GetRandomNumberGenerator().Get< Float >( -0.5f , 0.5f );
	m_randomizedEndHourFadeEnd = m_endEmissiveHour + m_endEmissiveFadeTime;

	m_randomizedStartHour = m_randomizedStartHour * 60.0f * 60.0f;
	m_randomizedStartHourFadeEnd = m_randomizedStartHourFadeEnd * 60.0f * 60.0f;
	m_randomizedEndHour = m_randomizedEndHour * 60.0f * 60.0f;
	m_randomizedEndHourFadeEnd = m_randomizedEndHourFadeEnd * 60.0f * 60.0f;
	m_tickCounter = 0;
	m_lastVal = 0.0f;

	m_lastFlickerRandomizedTime = 0.0f;
	m_lastFlickerRandomizedValue = 1.0f;
	m_nextFlickerRandomizedValue = 1.0f;

	m_lastRandomOffset = Vector::ZEROS;
	m_nextRandomOffset = Vector::ZEROS;
	m_animatePosition = m_lightRandomOffsetX != 0.0f || m_lightRandomOffsetY != 0.0f || m_lightRandomOffsetZ != 0.0f;

	m_localTimer = 0.0f;

	m_flicker = m_startStopLightsAndEngineValues && (m_lightBrightnessWhenOnMax != m_lightBrightnessWhenOnMin 
		|| m_particleAlphaWhenOnMin != m_particleAlphaWhenOnMax 
		|| m_engineValueXWhenOnMin != m_engineValueXWhenOnMax );

	m_lightInfo.ClearFast();

	for ( ComponentIterator< CLightComponent > it( this ); it; ++it )
	{
		SLightInfo info;

		CLightComponent* light = *it;
		info.brightness = light->GetBrightness();
		info.pos = light->GetPosition();
		info.wasEnabled = light->IsEnabled();
		m_lightInfo.Insert( light->GetName(), info );
	}

}

void CDaycycleGraphicsEntity::OnDetached( CWorld* world )
{
	world->GetTickManager()->RemoveEntity( this );	

	// Restore SHIT
	RestoreAll();

	m_lightInfo.ClearFast();

	TBaseClass::OnDetached( world );
}


void CDaycycleGraphicsEntity::RestoreAll()
{
	if ( m_lightInfo.Empty() )
	{
		return;
	}

	for ( ComponentIterator< CLightComponent > it( this ); it; ++it )
	{
		CLightComponent* light = *it;
		
		SLightInfo lightInfo;
		if ( m_lightInfo.Find( light->GetName(), lightInfo ) )
		{
			light->SetEnabled( lightInfo.wasEnabled );
			light->SetPosition( lightInfo.pos );
			light->SetBrightness( lightInfo.brightness );
		}
		else
		{
			ASSERT(!"Light was here and is gone, WTF?");
		}
	}
}


void CDaycycleGraphicsEntity::OnTick( Float timeDelta )
{	
	PC_SCOPE_PIX( DaycycleGraphicsEntityTick );

	if ( IsAttached() )
	{
		m_localTimer += timeDelta;

		// Do not slow down too much when game is active
		const Uint32 counterTest = (GGame && GGame->IsActive() ) ? ( (m_flicker || m_animatePosition) ? 1 : 100 ) : 1;

		if ( ++m_tickCounter == counterTest )
		{
			m_tickCounter = 0;
		}
		else
		{
			return;
		}

		// We need to be attached
		CWorld* attachedWorld = GetLayer()->GetWorld();
		ASSERT( attachedWorld );

		// Get shit
		CEnvironmentManager *envManager	= attachedWorld->GetEnvironmentManager();
		ASSERT( envManager );

		Float time = envManager->GetCurrentGameTime( false ).ToFloat();

		RecalcAndSendEffectParameter( time );

	}
}


void CDaycycleGraphicsEntity::OnPreDependencyMap( IFile& mapper )
{
	RestoreAll();
	TBaseClass::OnPreDependencyMap( mapper );
}



void CDaycycleGraphicsEntity::RecalcAndSendEffectParameter( Float time )
{
	Float val = 0.0f;

	if ( m_randomizedStartHour > m_randomizedEndHour )
	{
		if ( time >= 0.0f && time <= m_randomizedEndHour )
		{
			val = 1.0f;
		} 
		else if ( time <= (24.0f*60.0f*60.0f) && time >= m_randomizedStartHourFadeEnd )
		{
			val = 1.0f;
		}
		else if ( time >= m_randomizedEndHourFadeEnd && time <= m_randomizedStartHour )
		{
			val = 0.0f;
		}
		else if ( time > m_randomizedStartHour && time < m_randomizedStartHourFadeEnd )
		{
			val = ( time - m_randomizedStartHour ) / ( m_randomizedStartHourFadeEnd - m_randomizedStartHour );
		}
		else if ( time > m_randomizedEndHour && time < m_randomizedEndHourFadeEnd )
		{
			val = 1.0f - ( time - m_randomizedEndHour ) / ( m_randomizedEndHourFadeEnd - m_randomizedEndHour );
		}
	}
	else
	{
		if ( time >= 0.0f && time <= m_randomizedStartHour )
		{
			val = 0.0f;
		} 
		else if ( time <= (24.0f*60.0f*60.0f) && time >= m_randomizedEndHourFadeEnd )
		{
			val = 0.0f;
		}
		else if ( time >= m_randomizedStartHourFadeEnd && time <= m_randomizedEndHour )
		{
			val = 1.0f;
		}
		else if ( time > m_randomizedStartHour && time < m_randomizedStartHourFadeEnd )
		{
			val = ( time - m_randomizedStartHour ) / ( m_randomizedStartHourFadeEnd - m_randomizedStartHour );
		}
		else if ( time > m_randomizedEndHour && time < m_randomizedEndHourFadeEnd )
		{
			val = 1.0f - ( time - m_randomizedEndHour ) / ( m_randomizedEndHourFadeEnd - m_randomizedEndHour );
		}

	}

	if ( m_wasChangingParams && (val == 0.0f || !m_startStopLightsAndEngineValues ) )
	{
		m_wasChangingParams = false;
		RestoreAll();
	}

	if ( val != m_lastVal || ( (val == 1.0f) && (m_flicker || m_animatePosition) ) )
	{
		if ( m_startStopLightsAndEngineValues )
		{
			SendParameters( val );
		}

		if ( m_startStopEffects && ( val != m_lastVal ) )
		{
			if ( val == 0.0f )
			{
				StopAllEffects();
			}
			else if ( (val > 0.0f) && (m_lastVal == 0.0f) )
			{
				if ( m_template )
				{
					const TDynArray< CFXDefinition* >& effects = m_template->GetEffects();
					for ( Uint32 i = 0; i < effects.Size(); ++i )
					{
						PlayEffect( effects[i] );
					}
				}
			}

		}

		m_lastVal = val;
	}

}

void CDaycycleGraphicsEntity::SendParameters( Float lerpFactor )
{
	Vector val = Vector( Lerp<Float>( lerpFactor, m_engineValueXWhenOff, m_engineValueXWhenOnMin ), 
		Lerp<Float>( lerpFactor, m_engineValueYWhenOff, m_engineValueYWhenOn ),
		Lerp<Float>( lerpFactor, m_engineValueZWhenOff, m_engineValueZWhenOn),
		Lerp<Float>( lerpFactor, m_engineValueWWhenOff, m_engineValueWWhenOn ) );
	const Vector color = Vector( Lerp<Float>( lerpFactor, m_engineValueColorWhenOff.R, m_engineValueColorWhenOn.R ), 
		Lerp<Float>( lerpFactor, m_engineValueColorWhenOff.G, m_engineValueColorWhenOn.G ),
		Lerp<Float>( lerpFactor, m_engineValueColorWhenOff.B, m_engineValueColorWhenOn.B ),
		Lerp<Float>( lerpFactor, m_engineValueColorWhenOff.A, m_engineValueColorWhenOn.A ) );

	Vector posOffset = Vector::ZEROS;

	Float particleAlpha = Lerp<Float>( lerpFactor, m_particleAlphaWhenOff, m_particleAlphaWhenOnMin );

	Float brightness = m_lightBrightnessWhenOnMin * lerpFactor;

	if ( lerpFactor == 1.0f )
	{
		const Float time = m_localTimer;

		Float flickerLerpFactor = Clamp<Float>( 
			Lerp<Float>( (time - m_lastFlickerRandomizedTime) / m_flickeringPeriod, m_lastFlickerRandomizedValue, m_nextFlickerRandomizedValue ),
			0.0f,
			1.0f );

		particleAlpha = Lerp<Float>( 
			flickerLerpFactor,
			m_particleAlphaWhenOnMin,
			m_particleAlphaWhenOnMax );

		brightness = Lerp<Float>( 
			flickerLerpFactor,
			m_lightBrightnessWhenOnMin,
			m_lightBrightnessWhenOnMax );

		val.X = Lerp<Float>( 
			flickerLerpFactor,
			m_engineValueXWhenOnMin,
			m_engineValueXWhenOnMax );
		
		posOffset = Lerp<Vector>( (time - m_lastFlickerRandomizedTime) / m_flickeringPeriod, m_lastRandomOffset, m_nextRandomOffset );


		if ( (m_lastFlickerRandomizedTime + m_flickeringPeriod) < time )
		{
			m_lastFlickerRandomizedValue = m_nextFlickerRandomizedValue;
			m_nextFlickerRandomizedValue = GEngine->GetRandomNumberGenerator().Get< Float >();
			m_lastFlickerRandomizedTime = time;
			m_lastRandomOffset = m_nextRandomOffset;

			Vector randomVector
			(
				GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ),
				GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ),
				GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ),
				0.0f
			);
			m_nextRandomOffset = Vector( m_lightRandomOffsetX, m_lightRandomOffsetY, m_lightRandomOffsetZ, 0.0f ) * randomVector;
		}
	}

	for ( RenderProxyEntityIterator iter( this, RPT_Mesh ); iter ; ++iter )
	{
		(new CRenderCommand_UpdateEffectParameters( *iter, val, 5 ))->Commit();
		(new CRenderCommand_UpdateEffectParameters( *iter, color, 4 ))->Commit();
	}

	{
		for ( ComponentIterator< CLightComponent > it( this ); it; ++it )
		{
			CLightComponent* light = *it;

			if ( !light->IsEnabled() && (lerpFactor > 0.0f) )
			{
				if ( m_overrideRadius )
				{
					//dex++
					light->SetAutoHideDistance( m_lightAutoHideDistance, m_lightAutoHideRange );
					//dex--
					light->SetRadius( m_lightRadius );
				}

				light->SetEnabled( true );
			}
			else if ( light->IsEnabled() && (lerpFactor == 0.0f) )
			{
				light->SetEnabled( false );
				continue;
			}

			SLightInfo lightInfo;
			if ( m_lightInfo.Find( light->GetName(), lightInfo ) )
			{
				light->SetPosition( lightInfo.pos + posOffset );
				light->SetBrightness( lightInfo.brightness * brightness );
			}
			else
			{
				light->SetPosition( posOffset );
				light->SetBrightness( brightness );
			}

			m_wasChangingParams = true;
		}

	}
	/*
	{
		for ( ComponentIterator< CFlareComponent > it( this ); it; ++it )
		{
			IRenderProxy* renderProxy = (*it)->GetRenderProxy();
			if ( renderProxy )
			{
				(new CRenderCommand_UpdateEffectParameters( renderProxy, val, 5 ))->Commit();
				(new CRenderCommand_UpdateEffectParameters( renderProxy, color, 4 ))->Commit();
			}
		}
	}
	*/
	{
		for ( ComponentIterator< CParticleComponent > it( this ); it; ++it )
		{
			CParticleComponent::SEffectInfo effectInfo;
			effectInfo.m_alpha = particleAlpha;
			(*it)->SetEffectInfo( effectInfo );
		}
	}

}
