/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "game.h"
#include "environmentManager.h"
#include "gameTime.h"
#include "windowComponent.h"
#include "world.h"
#include "tickManager.h"
#include "entity.h"
#include "layer.h"
#include "baseEngine.h"

IMPLEMENT_ENGINE_CLASS( CWindowComponent );


CWindowComponent::CWindowComponent()
	: m_startEmissiveHour ( 22.0f )
	, m_startEmissiveFadeTime ( 0.1f )
	, m_endEmissiveHour ( 5.0f )
	, m_endEmissiveFadeTime ( 0.5f )
	, m_randomRange( 0.5f )
{

}

void CWindowComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

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

	world->GetTickManager()->AddToGroup( this, TICK_Main );	
}

void CWindowComponent::OnDetached( CWorld* world )
{
	world->GetTickManager()->RemoveFromGroup( this, TICK_Main );	
	TBaseClass::OnDetached( world );
}

void CWindowComponent::OnTick( Float timeDelta )
{
	PC_SCOPE_PIX( WindowComponentTick );

	// Do not slow down too much when game is active
	const Uint32 counterTest = (GGame && GGame->IsActive() ) ? 100 : 1;
	if ( ++m_tickCounter == counterTest )
	{
		m_tickCounter = 0;
	}
	else
	{
		return;
	}

	// We need to be attached
	CWorld* attachedWorld = GetEntity()->GetLayer()->GetWorld();
	ASSERT( attachedWorld );

	// Get shit
	CEnvironmentManager *envManager	= attachedWorld->GetEnvironmentManager();
	ASSERT( envManager );

	Float time = envManager->GetCurrentGameTime( false ).ToFloat();

	RecalcAndSendEffectParameter( time );
}

void CWindowComponent::RecalcAndSendEffectParameter( Float time )
{
	IRenderProxy* renderProxy = GetRenderProxy();
	
	if ( renderProxy )
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

		if ( val != m_lastVal )
		{
			// Create effect value
			EffectParameterValue effectValue;
			effectValue.SetFloat( val );

			// Apply
			SetEffectParameterValue( CNAME( MeshEffectScalar0 ), effectValue );
			m_lastVal = val;
		}
	}
}
