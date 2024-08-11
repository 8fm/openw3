/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "cityLightManager.h"

#include "../../common/engine/environmentManager.h"
#include "../../common/engine/weatherManager.h"
#include "../../common/engine/gameTimeManager.h"

#define MAX_CITY_LIGHT_TOGGLE_TIME			30.0f
#define MAX_CITY_LIGHTS_TOGGLED_PER_FRAME	50

IMPLEMENT_ENGINE_CLASS( CCityLightManager );

//#pragma optimize ( "", off )
CCityLightManager::CCityLightManager()
	: m_isEnabled ( true )
	, m_cityLightsOn  ( false )
	, m_weatherLightsOn ( false )
	, m_updateEnabled( true )
	, m_weatherManager( nullptr )
	, m_deltaTotal ( 0.f )
{
	m_lightsIterator.Init( &m_lights );
	m_nightStart = 20;
	m_nightEnd = 5;
}

void CCityLightManager::ToggleLight( CGameplayLightComponent* glComponent, Bool immediate/*= false*/ )
{
	if( glComponent->IsCityLight() )
	{
		if( glComponent->IsAffectedByWeather() )
		{
			//both: CITY and WEATHER
			if (immediate)
			{
				glComponent->SetInitialLight( m_cityLightsOn && m_weatherLightsOn );
			}
			else
			{
				glComponent->SetLight( m_cityLightsOn && m_weatherLightsOn );
			}
		}
		else
		{
			//CITY
			if (immediate)
			{
				glComponent->SetInitialLight( m_cityLightsOn );
			}
			else
			{
				glComponent->SetLight( m_cityLightsOn );
			}
		}
	}
	else
	{
		//WEATHER 
		//since only weather OR city lights will register themselves, we can safely assume that we'll never have a case of !city && !weather
		//if the light is affected by weather AND it's RAINING, then turn off the light.
		if ( immediate )
		{
			glComponent->SetInitialLight( m_weatherLightsOn );
		}
		else
		{
			glComponent->SetLight( m_weatherLightsOn );
		}
	}
}

void CCityLightManager::ToggleLights( Bool immediate )
{
	CTimeCounter timer;

	// Immediate toggle case - expensive!

	if ( immediate )
	{
		for ( CGameplayLightComponent* light : m_lights )
		{
			ToggleLight( light, immediate );
		}

		LOG_ENGINE( TXT("Toggled %d city lights in %f secs"), m_lights.Length(), timer.GetTimePeriod() );
		return;
	}

	// Move lights to temporary array

	TDynArray< CGameplayLightComponent* > sortedLights;
	sortedLights.Reserve( m_lights.Length() );
	m_lights.Fill( sortedLights );

	// Generate toggle time for each light

	CStandardRand& randomGenerator = GEngine->GetRandomNumberGenerator();
	for ( CGameplayLightComponent* light : sortedLights )
	{
		light->m_timeToToggle = randomGenerator.Get< Float >( 0.0f, MAX_CITY_LIGHT_TOGGLE_TIME );
	}

	// Sort by time to toggle

	struct CmpByTimeToToggle
	{
		RED_FORCE_INLINE Bool operator () ( const CGameplayLightComponent* a, const CGameplayLightComponent* b ) const
		{
			return a->m_timeToToggle < b->m_timeToToggle;
		}
	};

	Sort( sortedLights.Begin(), sortedLights.End(), CmpByTimeToToggle() );

	// Reinsert into the list

	m_lights.Clear();
	for ( CGameplayLightComponent* light : sortedLights )
	{
		m_lights.PushBack( light );
	}

	// Restart iteration

	m_lightsIterator.Begin();
	m_lightToggleTimer = 0.0f;

	LOG_ENGINE( TXT("Randomized order of %d city lights in %f secs"), m_lights.Length(), timer.GetTimePeriod() );
}

void CCityLightManager::OnWorldStart( const CGameInfo& gameInfo )
{
	//on world start, get the weatherManager and set the weather override (figure out if it's raining or not)
	CWorld* world = GGame->GetActiveWorld();

	if ( world )
	{
		CEnvironmentManager* envManager = world->GetEnvironmentManager();

		if ( envManager )
		{
			m_weatherManager = envManager->GetWeatherManager();
			m_weatherLightsOn = GetWeatherConditions();
		}
	}

	//also, set the city override (figure out if it's night or day)
	m_cityLightsOn = GetTODConditions();

	//and process the lights for the 1st time
	ToggleLights( true );
}

Bool CCityLightManager::GetWeatherConditions()
{
	if ( m_weatherManager )
	{
		return !( m_weatherManager->GetEffectStrength( EWeatherEffectType::WET_RAIN ) >= 0.5f ); //weather lights should be OFF if rain is greater than or equal to 0.5
	}
	else
		return false;
}

Bool CCityLightManager::GetTODConditions()
{
	Uint32 hour = GGame->GetTimeManager()->GetTime().Hours();

	if ( (hour >= m_nightStart || hour <= m_nightEnd) )
	{
		return true;
	}
	else if ( (hour > m_nightEnd && hour < m_nightStart) )
	{
		return false;
	}

	return false;
}

void CCityLightManager::Tick( Float timeDelta )
{
	PC_SCOPE( CCityLightManager::Tick );

	//do not toggle any lights if we're in a cutscene
	if ( GCommonGame->GetPlayer() && GCommonGame->GetPlayer()->IsInNonGameplayScene() )
	{
		return;
	}

	if( !m_updateEnabled )
	{
		return;
	}

	m_deltaTotal += timeDelta;

	//query changes in conditions once every 10 secs
	if ( m_deltaTotal >= 10.f )
	{
		UpdateToggleLights( false );
	}

	// Continue toggling lights

	TickToggleLights( timeDelta );
}

void CCityLightManager::UpdateToggleLights( Bool immediate /*= false*/ )
{
	Bool currentWeatherConditions = GetWeatherConditions();
	Bool currentTODConditions = GetTODConditions();

	//if there's been a change in conditions, process the list of lights and update conditions
	if ( currentTODConditions != m_cityLightsOn || currentWeatherConditions != m_weatherLightsOn )
	{
		m_cityLightsOn = currentTODConditions;
		m_weatherLightsOn = currentWeatherConditions;

		ToggleLights( immediate );
	}

	m_deltaTotal = 0.f;
}

void CCityLightManager::TickToggleLights( Float timeDelta )
{
	m_lightToggleTimer += timeDelta;

	for ( Uint32 numLightsToggled = 0;
		( !m_lightsIterator.IsEnd() && m_lightsIterator->m_timeToToggle < m_lightToggleTimer && numLightsToggled < MAX_CITY_LIGHTS_TOGGLED_PER_FRAME );
		++m_lightsIterator, ++numLightsToggled )
	{
		ToggleLight( *m_lightsIterator );
	}
}


///////////////////////////////////////////////SCRIPTS/////////////////////////////////////////////////////////////////////

void CCityLightManager::funcSetEnabled( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, value, false );
	FINISH_PARAMETERS;

	SetEnabled(value);
}


void CCityLightManager::funcIsEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_isEnabled ); 
}

void CCityLightManager::funcForceUpdate(CScriptStackFrame& stack, void* result)
{
	FINISH_PARAMETERS;

	UpdateToggleLights( true );
}

void CCityLightManager::funcSetUpdateEnabled(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( Bool, value, false );
	FINISH_PARAMETERS;

	m_updateEnabled = value;
}


/* DEBUG PURPOSES ONLY */
void CCityLightManager::funcDebugToggleAll( CScriptStackFrame& stack, void* result )
{
	ToggleLights( true );
}
//#pragma optimize ( "", on )