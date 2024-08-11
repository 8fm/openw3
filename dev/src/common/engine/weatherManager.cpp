/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "weatherManager.h"
#include "particleSystem.h"
#include "renderCommands.h"

#include "../../common/core/gatheredResource.h"
#include "../core/depot.h"
#include "../core/scriptStackFrame.h"
#include "textureArray.h"
#include "game.h"
#include "environmentManager.h"
#include "particleComponent.h"
#include "layer.h"
#include "dynamicLayer.h"
#include "world.h"
#include "environmentDefinition.h"
#include "../core/2darray.h"
#include "baseEngine.h"
#include "entity.h"
#include "soundSystem.h"

#define WEATHER_ENVIRONMENT_PRIORITY_FIRST 250
#define WEATHER_ENVIRONMENT_PRIORITY_LAST  (WEATHER_ENVIRONMENT_PRIORITY_FIRST + 25)
#define WEATHER_INCOMING_EFFECT (m_effectRingBuffInd + 1)%2
#define WEATHER_ONGOING_EFFECT (m_effectRingBuffInd)%2

inline Uint32 BuildAvailableWeatherConditionPriority( Int32 conditionIndex )
{
	RED_ASSERT( conditionIndex >= 0 );
	Uint32 priority = WEATHER_ENVIRONMENT_PRIORITY_FIRST + conditionIndex;
	RED_ASSERT( priority <= WEATHER_ENVIRONMENT_PRIORITY_LAST );
	priority = Min<Uint32>( priority, WEATHER_ENVIRONMENT_PRIORITY_LAST );
	return priority;
}

RED_DEFINE_STATIC_NAME( WeatherConditionsArrayChanged );

CGatheredResource wetSurfaceControlTexture( TXT("environment\\water\\global_ocean\\surface_flow.texarray"), RGF_Startup );

SWeatherCondition SWeatherCondition::CLEAR_WEATHER;

const AnsiChar* CWeatherManager::THUNDERS_START = "amb_g_thunder_far_loop_start";
const AnsiChar* CWeatherManager::THUNDERS_STOP = "amb_g_thunder_far_loop_stop";

static String ToString( EWeatherEffectType type )
{
	switch ( type )
	{
	case WET_CLOUDS:
		return TXT("CLOUDS");

	case WET_RAIN:
		return TXT("RAIN");

	case WET_HAIL:
		return TXT("HAIL");

	case WET_SNOW:
		return TXT("SNOW");

	case WET_THUNDERBOLT:
		return TXT("THUNDERBOLT");

	default:
		return TXT("CLOUDS");
	}
}

static void FromString( String fromString, EWeatherEffectType &toType )
{
	String result;

	for(Uint32 i=0; i<WET_EFFECTS_TYPES_MAX; i++)
	{
		result = ToString( (EWeatherEffectType)i );
		if( Red::System::StringCompare( fromString.AsChar(), result.AsChar() ) == 0 )
		{
			toType = (EWeatherEffectType)i;
			break;
		}
	}
}


////////////////////
// CWeatherManager 
CWeatherManager::CWeatherManager( CEnvironmentManager* environmentManager )
	: m_environmentManager( environmentManager ) // note: this reference comes from the CEM constructor
	, m_currentWeatherCondition( SWeatherCondition::CLEAR_WEATHER )
    , m_sourceWeatherCondition( SWeatherCondition::CLEAR_WEATHER )
	, m_targetWeatherCondition( SWeatherCondition::CLEAR_WEATHER )
	, m_blendTime( WEATHER_CONDITION_BLEND_TIME )
	, m_currentBlendTime( -1.0f )
	, m_envId( INVALID_AREA_ENV_ID )
	, m_running( false )
	, m_questPause( false )
	, m_currentEvolutionTime( 0.0f )
	, m_averageEvolutionTime( 2.0f*WEATHER_CONDITION_BLEND_TIME )	
	, m_effectRingBuffInd( 0 )
	, m_weatherBlendType( WBT_CLEAR_TO_CLEAR )
	, m_processQueue( false )
	, m_queueWeatherCondition( SWeatherCondition::CLEAR_WEATHER )
	, m_queueWeatherConditionPriority( WEATHER_ENVIRONMENT_PRIORITY_FIRST )
	, m_isBlendTimeForced( false )
	, m_playerIsInInteriorRefCount( 0 )
	, m_windFactorEventOverride( 1.0f )
{
	// load default water resources here	
	m_wetSurfaceTexture = wetSurfaceControlTexture.LoadAndGet< CTextureArray >();	
	ASSERT( m_wetSurfaceTexture, TXT("No default wet surface control array found! Weather system - surface rain will be turned off") );

	// temoprary
	if( GetWorld()->GetEnvironmentParameters().m_localWindDampers )
	{
		m_SimplexTree = GetWorld()->GetEnvironmentParameters().m_localWindDampers->Get();
	}

	for( Uint32 i=0; i<WEATHER_EFFECTS_MAX; i++ )
	{
		m_weatherEffectsBuff[i][0] = nullptr;
		m_weatherEffectsBuff[i][1] = nullptr;

		//m_currentWeatherEffects[i] = SWeatherEffect();		
	}	

	// used for quick feed of actual weather effect state to other systems
	for( Uint32 i=0; i<WET_EFFECTS_TYPES_MAX; i++ ) 
	{
		m_cachedWeatherEffects[ i ] = SWeatherEffect();
		m_cachedWeatherEffects[ i ].m_currentStrength = 0.0f;
		m_cachedWeatherEffects[ i ].m_type = (EWeatherEffectType)i;
	}
		
	m_postFX_DelayedWetnessStrength = 0.0f;
	m_postFX_DelayedWetnessEnabled = false;

	m_postFX_ImmediateWetnessStrength = 0.0f;
	m_postFX_ImmediateWetnessEnabled = false;		

	const SWeatherCondition& initialWeatherCondition = SWeatherCondition::CLEAR_WEATHER;
	RequestWeatherChangeTo( initialWeatherCondition, WEATHER_ENVIRONMENT_PRIORITY_FIRST, true );
}

CWeatherManager::~CWeatherManager()
{		
	if( m_currentWeatherCondition.m_backgroundThunder || m_sourceWeatherCondition.m_backgroundThunder )
	{
		GSoundSystem->SoundEvent( THUNDERS_STOP );
	}

	RemoveWeatherConditions();
}

void CWeatherManager::WeatherConditionsChanged()
{
	EDITOR_DISPATCH_EVENT( CNAME( WeatherConditionsArrayChanged ), NULL );
}

void CWeatherManager::DisableActiveWeatherConditionBlending( Float blendTime )
{
	if ( m_envId != INVALID_AREA_ENV_ID )
	{
		GetEnvironmentManager()->DeactivateEnvironment( m_envId, blendTime );
		m_envId = INVALID_AREA_ENV_ID;
	}
}

void CWeatherManager::RemoveWeatherConditions()
{
	// Disable all active weather condition blending
	DisableActiveWeatherConditionBlending( 0.0f );

	// Clear all available conditions
	m_availableWeatherConditions.Clear();
}

void CWeatherManager::AddWeatherCondition( const SWeatherCondition& condition )
{
	// Check if there is already a condition with that name
	// NOTE: use a hashset if this search proves to be slow
	Int32 conditionIndex = -1;
	for ( Int32 i=0; i < m_availableWeatherConditions.SizeInt(); ++i )
	{
		if ( m_availableWeatherConditions[i].m_name == condition.m_name )
		{
			conditionIndex = i;
			break;
		}
	}

	// Add or update the weather condition to the list
	if ( conditionIndex == -1 )
	{
		conditionIndex = static_cast<Int32>( m_availableWeatherConditions.Size() );
		m_availableWeatherConditions.PushBack( condition );
	}
	else
	{
		m_availableWeatherConditions[conditionIndex] = condition;
	}

	// Notify changes
	RED_ASSERT( BuildAvailableWeatherConditionPriority( conditionIndex ) >= WEATHER_ENVIRONMENT_PRIORITY_FIRST );
	RED_ASSERT( BuildAvailableWeatherConditionPriority( conditionIndex ) <= WEATHER_ENVIRONMENT_PRIORITY_LAST );
	WeatherConditionsChanged();
}

void CWeatherManager::AddWeatherConditionsFromTemplate( C2dArray* weatherTemplate )
{
	// Make sure the weather template isn't NULL
	if ( !weatherTemplate )
	{
		return;
	}

	// Add the conditions
	for ( Uint32 i=0; i < weatherTemplate->GetNumberOfRows(); ++i )
	{
		SWeatherCondition newCondition;
		Bool validCondition = true;
		Float fVal = 0.0f;
		Bool bVal = false;

		// Name
		newCondition.m_name = CName( weatherTemplate->GetValue( TXT("name"), i ) );

		// Occurrence probability
		validCondition = validCondition && FromString( weatherTemplate->GetValue( TXT("probability"), i ), fVal );
		newCondition.m_occurrenceProbability = fVal;

		// Wind parameters: scale
		validCondition = validCondition && FromString( weatherTemplate->GetValue( TXT("windScale"), i ), fVal );		
		newCondition.m_windScale = fVal;

		// Blend in time
		validCondition = validCondition && FromString( weatherTemplate->GetValue( TXT("blendTime"), i ), fVal );
		newCondition.m_blendTime = fVal;

		// Skybox overcast
		validCondition = validCondition && FromString( weatherTemplate->GetValue( TXT("skybox"), i ), fVal );
		newCondition.m_skyboxTextureIndex = fVal;

		// Fake shadow texture index
		validCondition = validCondition && FromString( weatherTemplate->GetValue( TXT("fakeShadow"), i ), fVal );
		newCondition.m_fakeShadowTextureIndex = fVal;

		// Should we use thunder sound effect( HACK - we need a proper solution )
		validCondition = validCondition && FromString( weatherTemplate->GetValue( TXT("backgroundThunder"), i ), bVal );
		newCondition.m_backgroundThunder = bVal;

		// Optional environment definition
		String sVal = weatherTemplate->GetValue( TXT("envPath"), i );
		if ( !sVal.Empty() )
		{
			// Environment definition resource
			CEnvironmentDefinition* envDef = Cast< CEnvironmentDefinition >( GDepot->LoadResource( sVal ) );
			ASSERT( envDef, TXT("Invalid environment definition in weather condition template") );
			validCondition = validCondition && envDef != NULL;
			newCondition.m_environmentDefinition = envDef;

			// Environment definition maximum blending
			validCondition = validCondition && FromString( weatherTemplate->GetValue( TXT("envBlend"), i ), fVal );
			newCondition.m_envDefBlend = Clamp( fVal, 0.0f, 1.0f );

			// Special case: if envblend is zero then disable it
			if ( newCondition.m_envDefBlend < 0.0001f )
			{
				newCondition.m_environmentDefinition = NULL;
				newCondition.m_envDefBlend = 0.0f;
			}
		}

		// how long this condition will stay visible
		validCondition = validCondition && FromString( weatherTemplate->GetValue( TXT("occurenceTime"), i ), fVal );
		newCondition.m_occurenceTime = fVal;
		
		// Load and manage all the effects
		for(Uint32 j=0; j<WEATHER_EFFECTS_MAX; j++)
		{
			// 1_effect; 2_effect; 3_effect; etc....
			String ePath = weatherTemplate->GetValue( String::Printf( TXT("%d_effect"), j+1 ).AsChar(), i );
			String sType = weatherTemplate->GetValue( String::Printf( TXT("%d_type"), j+1 ).AsChar(), i );	

			Float fProbability = 0.0f;
			Float fStrength = 0.0f;
			Uint8 fPriority = 0;
			EWeatherEffectType wType = WET_CLOUDS;
			
			// overloaded !!!
			FromString( sType, wType );

			FromString( weatherTemplate->GetValue( String::Printf( TXT("%d_prob"), j+1 ), i ).AsChar(), fProbability );
			FromString( weatherTemplate->GetValue( String::Printf( TXT("%d_strength"), j+1 ), i ).AsChar(), fStrength );
			FromString( weatherTemplate->GetValue( String::Printf( TXT("%d_priority"), j+1 ), i ).AsChar(), fPriority );

			if( !ePath.Empty() )
			{
				THandle< CParticleSystem > psSys = LoadResource< CParticleSystem >( ePath );
				ASSERT( psSys, TXT("Invalid particle system: '%ls' in weather condition template"), ePath.AsChar() );
				
				if( psSys.Get() != NULL && fProbability > 0.0f && fStrength > 0.0f ) 
				{					 
					newCondition.m_weatherEffect[j].m_type = wType;
					newCondition.m_weatherEffect[j].m_targetStrength = fStrength;
					newCondition.m_weatherEffect[j].m_probability = fProbability;
					newCondition.m_weatherEffect[j].m_particleSystem = psSys;
					newCondition.m_weatherEffect[j].m_particlePriority = fPriority;
				}
			}
		}		

		// Add the new weather condition if it is valid
		//ASSERT( validCondition, TXT("Invalid condition in weather condition template") );
		if ( validCondition )
		{
			AddWeatherCondition( newCondition );
		}
	}
}

const SWeatherCondition* CWeatherManager::FindWeatherCondition( const CName& weatherConditionName, Int32 *outAvailableConditionIndex ) const
{
	Int32 found_idx = -1;
	for ( Uint32 i=0; i<m_availableWeatherConditions.Size(); ++i )
	{
		if ( m_availableWeatherConditions[i].m_name == weatherConditionName )
		{
			found_idx = static_cast<Int32>( i );
			break;
		}
	}

	if ( nullptr != outAvailableConditionIndex )
	{
		*outAvailableConditionIndex = found_idx;
	}

	return -1 != found_idx ? &m_availableWeatherConditions[ found_idx ] : nullptr;
}

void CWeatherManager::AddEvolutionTime( Float time )
{
	m_averageEvolutionTime += time;
}

void CWeatherManager::RequestWeatherChangeTo( const SWeatherCondition& targetCondition, Uint32 priority, Bool skipQueue )
{	
	RED_ASSERT( priority >= WEATHER_ENVIRONMENT_PRIORITY_FIRST );
	RED_ASSERT( priority <= WEATHER_ENVIRONMENT_PRIORITY_LAST );

	// cancel whole queue in case we want to skip it (loading state from game save)
	if( m_processQueue && skipQueue ) m_processQueue = false;

	// We are in-between blending, this should not happen in the game!
	if( !skipQueue && m_currentBlendTime > 0.0f ) 
	{
		m_queueWeatherCondition = targetCondition;
		m_queueWeatherConditionPriority = priority;
		m_processQueue = true;

		// If the manager is paused, switch to the new weather immediately
		if( !m_running ) 
		{
			// target conditions becomes current now
			FinalizeCurrentBlend();
			StabilizeCurrentWeather();
		}
	}
	else
	{			
		// hashes
		if ( (m_currentWeatherCondition.m_name != targetCondition.m_name) || !m_effectEntity.Get() )
		{
			// notify wind parameters to change wind direction			
			m_windParams.NotifyConditionsChanged();

			m_targetWeatherCondition = targetCondition;

			m_sourceWeatherCondition = m_currentWeatherCondition;
			m_averageEvolutionTime = targetCondition.m_occurenceTime + 1.1f*m_blendTime;

			Float newBlendTime = m_isBlendTimeForced ? m_blendTime : targetCondition.m_blendTime;

			// If we are in pause mode - make blend instant
			if( !m_running ) newBlendTime = 0.0f;

			// If the current weather condition has an environment id, blend it out
			DisableActiveWeatherConditionBlending( newBlendTime );				
			
			// Update source/target values
			m_currentBlendTime = newBlendTime;
			m_blendTime = m_currentBlendTime;

			if( m_targetWeatherCondition.m_backgroundThunder && !m_currentWeatherCondition.m_backgroundThunder )
			{
				GSoundSystem->SoundEvent( THUNDERS_START );
			}

			// If the new target weather environment has an environment definition, blend it in
			if ( targetCondition.m_environmentDefinition )
			{
				m_envId = GetEnvironmentManager()->ActivateAreaEnvironment( targetCondition.m_environmentDefinition.Get(), NULL, priority, targetCondition.m_envDefBlend, newBlendTime );
			}

			// Create the effect entity if necessary
			Bool resetEffectComponents = false;
			if ( !m_effectEntity.Get() )
			{
				EntitySpawnInfo info;
				info.m_entityClass = ClassID< CEntity >();
				info.m_spawnPosition = GetWorld()->GetCameraPosition();
				info.m_entityNotSavable = true;
				CEntity* entity = GetWorld()->GetDynamicLayer()->CreateEntitySync( info );

				m_effectEntity = THandle< CEntity >( entity );
				resetEffectComponents = true;
			}

			// We have the effect entity, lets trigger right effect	
			//if( m_targetWeatherCondition.m_effectParticles ) m_targetWeatherCondition.m_effectParticles->ForceFullyLoad();
			for(Uint32 i=0; i<WEATHER_EFFECTS_MAX; i++)
			{
				if( m_targetWeatherCondition.m_weatherEffect[i].m_particleSystem ) m_targetWeatherCondition.m_weatherEffect[i].m_particleSystem->ForceFullyLoad();
			}			
			
			if ( resetEffectComponents ) CreateEffectsComponents();	
						
			Float effectAlpha = 0.0f;
			//CParticleComponent::SEffectInfo info;
			//info.m_alpha = 0.0f;

			// assume that we want the effect to be fully visible when at pause (editor only)
			if( !m_running ) effectAlpha = 1.0f;
				//info.m_alpha = 1.0f;

			CStandardRand& r = GEngine->GetRandomNumberGenerator();
			Uint32 effectsRandomized = 0;
			Uint32 effectsInTemplates = 0;
			Uint32 mostProbableEffect = 0;
			Float mostProbableVal = 0.0f;

			// try to activate appropriate effects that are available in this weather condition			
			for( Uint32 j=0; j<WEATHER_EFFECTS_MAX; j++ )
			{
				if( m_targetWeatherCondition.m_weatherEffect[j].m_targetStrength > 0.0f )
				{
					Float targetConditionEffectProbability = r.Get<Float>();					

					if( m_targetWeatherCondition.m_weatherEffect[j].m_particleSystem != nullptr )
					{
						if( m_targetWeatherCondition.m_weatherEffect[j].m_probability > 0.0f )
						{
							effectsInTemplates++;
							if( mostProbableVal < m_targetWeatherCondition.m_weatherEffect[j].m_probability )
							{
								mostProbableVal = m_targetWeatherCondition.m_weatherEffect[j].m_probability;
								mostProbableEffect = j;
							}

							if( m_targetWeatherCondition.m_weatherEffect[j].m_probability >= targetConditionEffectProbability )
							{
								effectsRandomized++;
								ActivateEffect( j, WEATHER_INCOMING_EFFECT, effectAlpha );
							}					
						}
					}					
				}
			}

			// unfortunately didn't randomize to attach any effect in this template, so try to pick at least one (this is fx guys request)
			if( effectsRandomized == 0 && effectsInTemplates > 0 )
			{					
				ActivateEffect( mostProbableEffect, WEATHER_INCOMING_EFFECT, effectAlpha );
			}

			SetWeatherBlendType();

			if( !m_running ) 
			{
				// target conditions becomes current now
				FinalizeCurrentBlend();
				StabilizeCurrentWeather();
			}
		}		
	}	
}

void CWeatherManager::SetParticleSystem( Uint32 slot, Uint32 buffRing , CParticleSystem * ps , const CParticleComponent::SEffectInfo * info )
{
	CParticleComponent * component = m_weatherEffectsBuff[ slot ][ buffRing ];

	// Set temporal render priority during spawning emitters
	component->SetParticleSystem( ps );

	if( component->GetRenderProxy() )
	{
		( new CRenderCommand_SetParticlePriority( component->GetRenderProxy() , m_cachedWeatherEffects[slot].m_particlePriority ) )->Commit();
	}

	if( info )
	{
		m_weatherEffectsBuff[ slot][ buffRing ]->SetEffectInfo( *info );

		// detach render proxy for effect that is about to be disabled (we set null into PS)
		if( info->m_alpha < 0.01f ) component->Reset();
		//RED_LOG ( WeatherEffect, TXT("SetParticleSystem effect: %d, alpha: %.5f"), slot, info->m_alpha );
	}
	//else
	//	RED_LOG ( WeatherEffect, TXT("WTF %d"), slot );
}

void CWeatherManager::ActivateEffect( Uint32 slot, Uint32 buffRing, Float effectAlpha )
{
	CParticleComponent::SEffectInfo info;
	info.m_alpha = effectAlpha;

	if( !m_weatherEffectsBuff[ slot ][ buffRing ]->IsAttached() ) 
	{
		m_weatherEffectsBuff[ slot][ buffRing ]->AttachToWorld( GetWorld() );
	}

	CParticleSystem * ps = m_targetWeatherCondition.m_weatherEffect[ slot ].m_particleSystem.Get();
	SetParticleSystem( slot , buffRing , ps , &info );

	m_currentWeatherCondition.m_weatherEffect[ slot ].m_active = true;						

	m_targetWeatherCondition.m_weatherEffect[ slot ].m_active = true;
	m_targetWeatherCondition.m_weatherEffect[ slot ].m_currentStrength = 0.0f;	

	if( !m_running ) m_weatherEffectsBuff[ slot ][ buffRing ]->Reset();
}

Bool CWeatherManager::RequestWeatherChangeTo( const CName& weatherConditionName, Bool skipQueue )
{
	// Search for the condition
	Int32 conditionIndex = -1;
	const SWeatherCondition* condition = FindWeatherCondition( weatherConditionName, &conditionIndex );

	// Found...
	if ( condition )
	{		
		RequestWeatherChangeTo( *condition, BuildAvailableWeatherConditionPriority( conditionIndex ), skipQueue );
		return true;
	}

	// Not found...
	return false;
}

void CWeatherManager::CreateEffectsComponents()
{
	SComponentSpawnInfo cinfo;	

	for( Uint32 i=0; i<WEATHER_EFFECTS_MAX; i++ )
	{		
		m_weatherEffectsBuff[i][0] = Cast< CParticleComponent >( m_effectEntity.Get()->CreateComponent( ClassID< CParticleComponent >(), cinfo ) );
		m_weatherEffectsBuff[i][1] = Cast< CParticleComponent >( m_effectEntity.Get()->CreateComponent( ClassID< CParticleComponent >(), cinfo ) );	
	}
}

void CWeatherManager::SetBlendTime( Float t )
{
	m_blendTime = Clamp<Float>( t, 0.001f, NumericLimits< Float >::Infinity() );
	m_isBlendTimeForced = true;
}

void CWeatherManager::SetRunning( Bool enable )
{
	m_running = enable;
}

void CWeatherManager::SetWeatherBlendType()
{
	Bool targetTypeOvercast = false;
	Bool sourceTypeOvercast = false;
		
		
	for( Uint32 i=0; i<WEATHER_EFFECTS_MAX; i++ )
	{
		if( m_targetWeatherCondition.m_weatherEffect[i].m_targetStrength > 0.0f && (
			( m_targetWeatherCondition.m_weatherEffect[i].m_type == WET_RAIN )
			|| ( m_targetWeatherCondition.m_weatherEffect[i].m_type == WET_SNOW )
			|| ( m_targetWeatherCondition.m_weatherEffect[i].m_type == WET_HAIL ) ) )
		{
			targetTypeOvercast = true;
			break;
		}
	}

	for( Uint32 i=0; i<WEATHER_EFFECTS_MAX; i++ )
	{
		if( m_sourceWeatherCondition.m_weatherEffect[i].m_targetStrength > 0.0f && (
			( m_sourceWeatherCondition.m_weatherEffect[i].m_type == WET_RAIN )
			|| ( m_sourceWeatherCondition.m_weatherEffect[i].m_type == WET_SNOW )
			|| ( m_sourceWeatherCondition.m_weatherEffect[i].m_type == WET_HAIL ) ) )
		{
			sourceTypeOvercast = true;
			break;
		}
	}

	if( targetTypeOvercast && sourceTypeOvercast ) m_weatherBlendType = WBT_OVERCAST_TO_OVERCAST;
	else	
		if( !targetTypeOvercast && sourceTypeOvercast ) m_weatherBlendType = WBT_OVERCAST_TO_CLEAR;
		else
			if( !targetTypeOvercast && !sourceTypeOvercast ) m_weatherBlendType = WBT_CLEAR_TO_CLEAR;
			else
				if( targetTypeOvercast && !sourceTypeOvercast ) m_weatherBlendType = WBT_CLEAR_TO_OVERCAST;
}

void CWeatherManager::StabilizeCurrentWeather()
{			
	// Some condition waits to be blended next
	if( m_processQueue )
	{
		m_processQueue = false;
		RequestWeatherChangeTo( m_queueWeatherCondition, m_queueWeatherConditionPriority );
	}		

	// this part is stabilize button / editor only
	m_windParams.ForceWindParameters( m_currentWeatherCondition.m_windScale, true, m_currentWeatherCondition.m_environmentWindStrengthOverride );	
		
	RED_ASSERT( m_effectEntity.Get(), TXT("Weather system is missing its effect entity, that should never happen!") );

	if( m_effectEntity.Get() )
	{
		// Update effect particles alpha
		CParticleComponent::SEffectInfo info;

		for(Uint32 i=0; i<WEATHER_EFFECTS_MAX; i++)
		{
			if( m_currentWeatherCondition.m_weatherEffect[i].m_active )
			{
				m_currentWeatherCondition.m_weatherEffect[i].m_currentStrength = m_currentWeatherCondition.m_weatherEffect[i].m_targetStrength;
				info.m_alpha = m_currentWeatherCondition.m_weatherEffect[i].m_targetStrength;

				CParticleSystem* targetPs = m_currentWeatherCondition.m_weatherEffect[i].m_particleSystem.Get();
				CParticleSystem* currentPs = nullptr;

				for(Uint32 j=0; j<WEATHER_EFFECTS_MAX; j++)
				{
					currentPs = m_weatherEffectsBuff[j][ WEATHER_ONGOING_EFFECT ]->GetParticleSystem();
					if( currentPs != nullptr && currentPs == targetPs )
					{
						SetParticleSystem( j , WEATHER_ONGOING_EFFECT , targetPs , &info );
						break;
					}
				}				
			}		
		}
	}	

	SyncCachedEffects();
		
	// update delayed wetness/puddles
	if( m_postFX_DelayedWetnessEnabled ) m_postFX_DelayedWetnessStrength = 1.0f;
	else
		m_postFX_DelayedWetnessStrength = 0.0f;

	if( m_postFX_ImmediateWetnessEnabled ) m_postFX_ImmediateWetnessStrength = 1.0f;
	else
		m_postFX_ImmediateWetnessStrength = 0.0f;

	Float ratio = GetBlendingRatio();
	Float backgroundThunderBlend = 0.0f;
	if( !m_sourceWeatherCondition.m_backgroundThunder && m_targetWeatherCondition.m_backgroundThunder )
	{
		backgroundThunderBlend		= GetBlendingRatioFromTimeline( ratio, 0, 0.3f );
	}
	if( m_sourceWeatherCondition.m_backgroundThunder && !m_targetWeatherCondition.m_backgroundThunder )
	{
		backgroundThunderBlend		= 1.0f - GetBlendingRatioFromTimeline( ratio, 0.0f, 1.0f );
	}
	if ( m_sourceWeatherCondition.m_backgroundThunder && m_targetWeatherCondition.m_backgroundThunder )
	{
		backgroundThunderBlend = 1.0f;
	}
	GSoundSystem->SoundGlobalParameter( "thunder_intensity", backgroundThunderBlend );
}

void CWeatherManager::SyncCachedEffects()
{	
	m_postFX_DelayedWetnessEnabled = false;
	m_postFX_ImmediateWetnessEnabled = false;

	for(Uint32 i=0; i<WET_EFFECTS_TYPES_MAX; i++) m_cachedWeatherEffects[i].m_currentStrength = 0.0f;

	for(Uint32 i=0; i<WEATHER_EFFECTS_MAX; i++) 
	{
		if( m_currentWeatherCondition.m_weatherEffect[i].m_currentStrength > m_cachedWeatherEffects[ (Uint32) m_currentWeatherCondition.m_weatherEffect[i].m_type ].m_currentStrength )
		{
			m_cachedWeatherEffects[ (Uint32) m_currentWeatherCondition.m_weatherEffect[i].m_type ].m_currentStrength = m_currentWeatherCondition.m_weatherEffect[i].m_currentStrength;
			if( m_currentWeatherCondition.m_weatherEffect[i].m_type == WET_RAIN ) 
			{
				m_postFX_DelayedWetnessEnabled = true;
				m_postFX_ImmediateWetnessEnabled = true;
			}
		}

		if( m_targetWeatherCondition.m_weatherEffect[i].m_currentStrength > m_cachedWeatherEffects[ (Uint32) m_targetWeatherCondition.m_weatherEffect[i].m_type ].m_currentStrength )
		{
			m_cachedWeatherEffects[ (Uint32) m_targetWeatherCondition.m_weatherEffect[i].m_type ].m_currentStrength = m_targetWeatherCondition.m_weatherEffect[i].m_currentStrength;
			if( m_targetWeatherCondition.m_weatherEffect[i].m_type == WET_RAIN ) 
			{
				m_postFX_DelayedWetnessEnabled = true;
				m_postFX_ImmediateWetnessEnabled = true;
			}
		}		
	}
}

void CWeatherManager::SyncEffectInfo( Float currentEffectStrength, Float targetEffectStrength, Uint32 i )
{
	CParticleComponent::SEffectInfo info;
	CParticleSystem* targetPs = m_targetWeatherCondition.m_weatherEffect[i].m_particleSystem.Get();
	CParticleSystem* currentPs = m_currentWeatherCondition.m_weatherEffect[i].m_particleSystem.Get();
	Int32 ind = -1;

	if( targetPs != nullptr )
	{
		ind = FindEffect( targetPs, WEATHER_INCOMING_EFFECT );
		if( ind > -1 )
		{
			info.m_alpha = targetEffectStrength;
			m_weatherEffectsBuff[ ind ][ WEATHER_INCOMING_EFFECT ]->SetEffectInfo( info );
		}
	}

	if( currentPs != nullptr )
	{
		ind = FindEffect( currentPs, WEATHER_ONGOING_EFFECT );
		if( ind > -1 )
		{
			info.m_alpha = currentEffectStrength;
			m_weatherEffectsBuff[ ind ][ WEATHER_ONGOING_EFFECT ]->SetEffectInfo( info );
		}
	}
}

void CWeatherManager::SetPlayerIsInInterior( Bool isInInterior )
{
	if( isInInterior )
	{
		m_playerIsInInteriorRefCount.Increment();
	}
	else
	{
		const Int32 newRefCount = m_playerIsInInteriorRefCount.Decrement();
		RED_UNUSED( newRefCount );
		RED_ASSERT( newRefCount >= 0 );
	}
}

void CWeatherManager::ResetPlayerIsInInterior()
{
	m_playerIsInInteriorRefCount.SetValue( 0 );
}

void CWeatherManager::FinalizeCurrentBlend()
{
	RED_ASSERT( m_effectEntity.Get(), TXT("Weather system is missing its effect entity, that should never happen!") );

	// Turn off not needed effects
	if( m_effectEntity.Get() )
	{
		for(Uint32 i=0; i<WEATHER_EFFECTS_MAX; i++)
		{
			if( m_currentWeatherCondition.m_weatherEffect[i].m_active )
			{
				Float currentEffectStrength = 0.0f;

				// make sure effect is fully blended in				
				Float targetEffectStrength = Clamp<Float>( m_targetWeatherCondition.m_weatherEffect[i].m_targetStrength, 0.0f, 1.0f);				
				m_currentWeatherCondition.m_weatherEffect[i].m_currentStrength = targetEffectStrength;				
								
				SyncEffectInfo( currentEffectStrength, targetEffectStrength, i );

				Int32 ind = -1;
				CParticleSystem* ps = m_currentWeatherCondition.m_weatherEffect[i].m_particleSystem.Get();
				if( ps != nullptr ) ind = FindEffect( ps, WEATHER_ONGOING_EFFECT );

				if( ind > -1 )
				{
					if( m_weatherEffectsBuff[ ind ][ WEATHER_ONGOING_EFFECT ]->IsAttached() ) 
					{
						m_weatherEffectsBuff[ ind ][ WEATHER_ONGOING_EFFECT ]->DetachFromWorld( GetWorld() );
						m_weatherEffectsBuff[ ind ][ WEATHER_ONGOING_EFFECT ]->SetParticleSystem( nullptr );
					}
				}								
			}						
		}	

		if( GetWorld() != nullptr ) GetWorld()->CallEvent( CNAME( OnWeatherChange ) );
	}

	m_effectRingBuffInd++;
	m_effectRingBuffInd %= 2;	

	m_sourceWeatherCondition = m_currentWeatherCondition;
	m_currentWeatherCondition = m_targetWeatherCondition;		

	m_currentBlendTime = -1.0f;
	m_currentEvolutionTime = 0.0f;
	
	m_currentSkyboxBlendRatio = 0.0f;

	if( m_sourceWeatherCondition.m_backgroundThunder && !m_targetWeatherCondition.m_backgroundThunder )
	{
		GSoundSystem->SoundEvent( THUNDERS_STOP );
	}

	if( m_isBlendTimeForced ) 
	{
		m_blendTime = WEATHER_CONDITION_BLEND_TIME;
		m_isBlendTimeForced = false;
	}

	for(Uint32 i=0; i<WEATHER_EFFECTS_MAX; i++)
	{
		if(	m_currentWeatherCondition.m_weatherEffect[i].m_type == WET_THUNDERBOLT && m_targetWeatherCondition.m_weatherEffect[i].m_targetStrength > 0.0f )
		{
			SWeatherEvent thunderSound;
			thunderSound.m_lifeTime = 10.0f;
			thunderSound.m_lifeRandom = 30.0f;
			thunderSound.m_active = true;

			m_currentWeatherCondition.m_event = thunderSound;
		}
	}

	SetWeatherBlendType();		 
}

void CWeatherManager::GetCurrentWeatherConditionInfo( String& currentConditionName, Float& currentConditionStatus )
{ 
	if( m_currentBlendTime > 0.0f )
	{
		currentConditionName = m_currentWeatherCondition.m_name.AsString();
		currentConditionName += TXT("  ...  ");
		currentConditionName += m_targetWeatherCondition.m_name.AsString();

		currentConditionStatus = 1.0f - (m_currentBlendTime / m_blendTime); 
	}
	else
	{
		currentConditionName = m_currentWeatherCondition.m_name.AsString(); 
		currentConditionStatus = m_currentEvolutionTime / m_averageEvolutionTime; 
	}	
}

Bool CWeatherManager::QueueRandomWeatherChange()
{
	Uint32 mostProbableIndex = 0;

	if( GetRandomWeatherCondition( mostProbableIndex ) )
	{
		m_queueWeatherCondition = m_availableWeatherConditions[ mostProbableIndex ];		
		m_queueWeatherConditionPriority = BuildAvailableWeatherConditionPriority( mostProbableIndex );
		m_processQueue = true;
		return true;
	}

	return false;
}
#ifndef NO_EDITOR
String CWeatherManager::GetDebugStatus()
{ 
	String ret,currentConditionName,queuedConditionName;
	Float currentEvolutionStatus = 0.0f;
	GetCurrentWeatherConditionInfo( currentConditionName, currentEvolutionStatus );	

	Uint32 environmentWeatherConditions = 0;
	Uint32 questWeatherConditions = 0;
	
	for(Uint32 i=0; i<m_availableWeatherConditions.Size(); ++i)
	{
		if( m_availableWeatherConditions[i].m_occurrenceProbability < 0.001f ) ++questWeatherConditions;
		else
			++environmentWeatherConditions;
	}

	if( currentConditionName.Empty() ) currentConditionName = TXT("NONE");
	if( m_processQueue )
	{
		if( m_queueWeatherCondition.m_name.Empty() ) queuedConditionName = TXT("NONE");
		else
			queuedConditionName = m_queueWeatherCondition.m_name.AsString();
	}

	ret = String::Printf( TXT("Weather system status: %ls"), m_running ? TXT("running") : TXT("paused") );
	ret += String::Printf( TXT("\nAvailabile QUEST conditions: %d"), questWeatherConditions );
	ret += String::Printf( TXT("\nAvailabile ENVIRONMENT conditions: %d"), environmentWeatherConditions );
	ret += m_questPause ? TXT("\n Quest pause: activated, WEATHER WILL NOT RANDOMIZE!!!") : TXT("\n");
	ret += m_processQueue ? String::Printf( TXT("\nWeather condition waiting in queue: %ls"), queuedConditionName.AsChar() ) : TXT("\n");
	ret += String::Printf( TXT("\n%ls (%0.2f) : %0.2f"), currentConditionName.AsChar(), m_currentWeatherCondition.m_occurrenceProbability, currentEvolutionStatus );

	/*
	for(Uint32 i=0; i<WEATHER_EFFECTS_MAX; i++)
	{
		if( m_currentWeatherCondition.m_weatherEffect[i].m_type == WET_THUNDERBOLT )
		{
			ret += String::Printf( TXT("\ncurrent THUNDERBOLT %d (%0.2f)"), i, m_currentWeatherCondition.m_weatherEffect[i].m_currentStrength );
		}

		if( m_targetWeatherCondition.m_weatherEffect[i].m_type == WET_THUNDERBOLT )
		{
			ret += String::Printf( TXT("\ntarget THUNDERBOLT %d (%0.2f)"), i, m_targetWeatherCondition.m_weatherEffect[i].m_currentStrength );
		}

		String eventStr = m_currentWeatherCondition.m_event.m_active ? TXT("active") : TXT("not-active");
		ret += String::Printf( TXT("\nevent: %ls"), eventStr.AsChar() );	
	}
	*/
	return ret;
}
#endif

Bool CWeatherManager::GetRandomWeatherCondition( Uint32& mostProbableIndex )
{
	Bool ret = false;

	if( !m_availableWeatherConditions.Empty() )
	{		
		Float mostProbableVal = 1.0f;
		Int32 numOfTries = 50;
		Int32 currentIndex = -1;

		while( currentIndex < 0 )
		{
			for(Uint32 i=0; i<m_availableWeatherConditions.Size(); ++i)
			{
				// skip "quest" weather presets
				if( m_availableWeatherConditions[i].m_occurrenceProbability < 0.001f ) continue;

				Float rnd = GEngine->GetRandomNumberGenerator().Get< Float >();
				if( rnd < m_availableWeatherConditions[i].m_occurrenceProbability )
				{
					if( mostProbableVal > Abs(rnd - m_availableWeatherConditions[i].m_occurrenceProbability) )
					{
						mostProbableVal = Abs(rnd - m_availableWeatherConditions[i].m_occurrenceProbability);
						currentIndex = i;
						mostProbableIndex = currentIndex;
						ret = true;
					}
				}
			}

			--numOfTries;
			if( numOfTries < 0 ) currentIndex = 0;
		}				
	}

	return ret;
}

void CWeatherManager::Tick( float timeDelta, Float gameHoursPerMinute )
{
	PC_SCOPE_PIX( CWeatherManager_Tick );	

	// Update weather effect entity position
	Vector cPos = GetWorld()->GetCameraPosition();
	Vector cFrw = GetWorld()->GetCameraForward();
	
	// pre-sort the effects based on the order in the template
	if ( m_effectEntity.Get() && m_effectEntity.Get()->IsAttached() )
	{	
		m_effectEntity.Get()->SetPosition( cPos + cFrw*0.1f );	
	}

	// tick simple eventing system - for thunder sounds
	if( m_currentWeatherCondition.m_event.Tick( timeDelta ) )
	{		
		for(Uint32 i=0; i<WET_EFFECTS_TYPES_MAX; i++ )
		{		
			if(  m_currentWeatherCondition.m_weatherEffect[i].m_active && m_currentWeatherCondition.m_weatherEffect[i].m_type == WET_THUNDERBOLT )
			{
				if( m_currentWeatherCondition.m_weatherEffect[i].m_particleSystem != nullptr )
				{
					CParticleSystem* ps = m_currentWeatherCondition.m_weatherEffect[i].m_particleSystem.Get();
					if( ps != nullptr )
					{
						for(Uint32 j=0; j<WET_EFFECTS_TYPES_MAX; j++ )
						{
							if( m_weatherEffectsBuff[j][ WEATHER_ONGOING_EFFECT ] != nullptr && ps == m_weatherEffectsBuff[j][ WEATHER_ONGOING_EFFECT ]->GetParticleSystem() )
							{
								Vector thunderboltMinMax = Vector(20.0f, 20.f, 0.0f, 0.f);
								thunderboltMinMax.X += 100.0f*GEngine->GetRandomNumberGenerator().Get< Float >();
								thunderboltMinMax.Y += 100.0f*GEngine->GetRandomNumberGenerator().Get< Float >();

								m_weatherEffectsBuff[ j ][ WEATHER_ONGOING_EFFECT ]->SetPosition( Vector( thunderboltMinMax.X * cFrw.X, thunderboltMinMax.X * cFrw.Y, GetWorld()->GetCameraPosition().Z * -1.0f, 1.0f ) );
								m_weatherEffectsBuff[ j ][ WEATHER_ONGOING_EFFECT ]->Reset();
																
								GSoundSystem->SoundGlobalParameter( "thunderbolt_distance", MSqrt( thunderboltMinMax.X*thunderboltMinMax.X + thunderboltMinMax.Y*thunderboltMinMax.Y ) );
								GSoundSystem->SoundEvent( "fx_amb_thunder_close" );
								break;
							}
						}
					}
				}
			}
		}			
	}

	// Only do stuff when the system is running
	if ( !m_running )
	{
		return;
	}	

	Float gameTimeDelta = timeDelta*( gameHoursPerMinute/0.25f );

	// Unless quest holds the weather paused
	if( !m_questPause ) 
	{
		// Perform current state evolution
		m_currentEvolutionTime += gameTimeDelta;
	}	
	
	if( m_currentEvolutionTime > m_averageEvolutionTime )
	{
		m_currentEvolutionTime = 0.0f;
		
		Uint32 mostProbableIndex = 0;
		if ( GetRandomWeatherCondition( mostProbableIndex ) )		
		{
			RequestWeatherChangeTo( m_availableWeatherConditions[ mostProbableIndex ], BuildAvailableWeatherConditionPriority( mostProbableIndex ) );
		}		
	}

	// Perform weather states blending
	if ( m_currentBlendTime > 0.0f )
	{
		m_currentBlendTime -= gameTimeDelta;

		if ( m_currentBlendTime <= 0.0f )
		{
			FinalizeCurrentBlend();			
			StabilizeCurrentWeather();

			// Some condition waits to be blended next
			if( m_processQueue )
			{
				m_processQueue = false;
				RequestWeatherChangeTo( m_queueWeatherCondition, m_queueWeatherConditionPriority );
			}			
		}
		else
		{
			Float ratio = GetBlendingRatio();

			// timeline for the blend:		
			//													WBT_CLEAR_TO_OVERCAST
			//								non-effect condition -----------------------------------------------> effect condition
			//								0.0f - 0.3f					0.2f - 0.4f				0.3f - 0.5f					0.5 - 1.0;
			//								sky clouds + wind			effect strength			wet surface effect			puddles appearance
			//
			//													WBT_OVERCAST_TO_CLEAR
			//								effect condition ---------------------> non-effect condition
			//								0.0f - 0.2f					0.1f - 0.6f				0.5f - 1.0f 
			//								effect strength + wind		sky clouds				wet surface effect (puddles disappearance)
			
			Float skyboxBlend				= 0.0f;
			Float effectBlend				= 0.0f;			
			Float windBlend					= 0.0f;
			Float backgroundThunderBlend	= 0.0f;
			Float immediateWetness			= 0.0f;
			Float delayedThunderBolts		= 0.0f;

			Float skyboxTimelineStartingPoint = 0.0f;
			Float skyboxTimelineEndingPoint = 1.0f;

			Float effectTimelineStartingPoint = 0.2f;
			Float effectTimelineEndingPoint = 0.8f;
						
			Float windTimelineStartingPoint = 0.0f;
			Float windTimelineEndingPoint = 1.0f;
					
			
			switch ( m_weatherBlendType )
			{				
				case WBT_CLEAR_TO_OVERCAST:
					skyboxTimelineStartingPoint = 0.0f;
					skyboxTimelineEndingPoint = 0.8f;

					effectTimelineStartingPoint = 0.5f;
					effectTimelineEndingPoint = 1.0f;

					windTimelineStartingPoint = 0.0f;
					windTimelineEndingPoint = 0.6f;

					immediateWetness = GetBlendingRatioFromTimeline( ratio, effectTimelineStartingPoint, effectTimelineEndingPoint );
					delayedThunderBolts	= GetBlendingRatioFromTimeline( ratio, 0.75f, 1.0f );
					break;

				case WBT_OVERCAST_TO_CLEAR:
					skyboxTimelineStartingPoint = 0.0f;
					skyboxTimelineEndingPoint = 1.0f;	

					effectTimelineStartingPoint = 0.0f;
					effectTimelineEndingPoint = 0.5f;
					
					windTimelineStartingPoint = 0.0f;
					windTimelineEndingPoint = 0.8f;

					immediateWetness = GetBlendingRatioFromTimeline( 1.0f - ratio, 0.9f, 1.0f );
					delayedThunderBolts	= 1.0f - GetBlendingRatioFromTimeline( ratio, 0.0f, 0.2f );
					break;

				case WBT_OVERCAST_TO_OVERCAST:
					immediateWetness = 1.0f;

				default:
					// Already set
					break;				
			}		
			
			skyboxBlend					= GetBlendingRatioFromTimeline( ratio, skyboxTimelineStartingPoint, skyboxTimelineEndingPoint );
			effectBlend					= GetBlendingRatioFromTimeline( ratio, effectTimelineStartingPoint, effectTimelineEndingPoint );			
			windBlend					= GetBlendingRatioFromTimeline( ratio, windTimelineStartingPoint, windTimelineEndingPoint );
			
			// Update immediate surface wetness based on [RAIN] effect
			if( m_postFX_ImmediateWetnessEnabled ) m_postFX_ImmediateWetnessStrength = immediateWetness;

			if( !m_sourceWeatherCondition.m_backgroundThunder && m_targetWeatherCondition.m_backgroundThunder )
			{
				backgroundThunderBlend		= GetBlendingRatioFromTimeline( ratio, 0, 0.3f );
			}
			if( m_sourceWeatherCondition.m_backgroundThunder && !m_targetWeatherCondition.m_backgroundThunder )
			{
				backgroundThunderBlend		= 1.0f - GetBlendingRatioFromTimeline( ratio, 0.0f, 1.0f );
			}
			if ( m_sourceWeatherCondition.m_backgroundThunder && m_targetWeatherCondition.m_backgroundThunder )
			{
				backgroundThunderBlend = 1.0f;
			}
			GSoundSystem->SoundGlobalParameter( "thunder_intensity", backgroundThunderBlend );

			m_windParams.SetWindScale(	Lerp<Float>( windBlend, m_sourceWeatherCondition.m_windScale, m_targetWeatherCondition.m_windScale ) );

			// Blend skybox textures
			// currently not used
			//m_currentWeatherCondition.m_skyboxTextureIndex = Lerp<Float>( skyboxBlend, m_sourceWeatherCondition.m_skyboxTextureIndex, m_targetWeatherCondition.m_skyboxTextureIndex );
			m_currentSkyboxBlendRatio = skyboxBlend;

			// Update effect particles alpha
			CParticleComponent::SEffectInfo info;
			Float effectRatio = 1.0f;			
			Bool disableEvents = false;

			for(Uint32 i=0; i<WEATHER_EFFECTS_MAX; i++)
			{
				if( m_currentWeatherCondition.m_weatherEffect[i].m_active )
				{
					switch( m_currentWeatherCondition.m_weatherEffect[i].m_type )
					{
					case WET_CLOUDS:
								
									// blending in thunderbolts onto clouds
									if( m_targetWeatherCondition.m_weatherEffect[i].m_type == WET_THUNDERBOLT ) effectRatio = 1.0f - delayedThunderBolts;
									else
										effectRatio = 1.0f - skyboxBlend;
									break;

					case ( WET_RAIN || WET_SNOW || WET_HAIL ):

									effectRatio = 1.0f - effectBlend;									
									break;

					case WET_THUNDERBOLT:

									effectRatio = delayedThunderBolts;		
									if( effectRatio < 0.01f ) disableEvents = true;

									break;

					default:
						break;
					}

					// update particles
					Float currentEffectStrength = Clamp<Float>( effectRatio*m_currentWeatherCondition.m_weatherEffect[i].m_targetStrength, 0.0f, 1.0f);
					Float targetEffectStrength = Clamp<Float>( (1.0f-effectRatio)*m_targetWeatherCondition.m_weatherEffect[i].m_targetStrength, 0.0f, 1.0f);

					m_targetWeatherCondition.m_weatherEffect[i].m_currentStrength = targetEffectStrength;
					m_currentWeatherCondition.m_weatherEffect[i].m_currentStrength = currentEffectStrength;

					if( m_currentWeatherCondition.m_event.m_active && disableEvents ) m_currentWeatherCondition.m_event.m_active = false;

					if( m_effectEntity.Get() )
					{
						SyncEffectInfo( currentEffectStrength, targetEffectStrength, i );				
					}
				}			
			}

			SyncCachedEffects();
		}
	}

	// Update delayed wetness effect (puddles)
	Float dWetnessDir = 1.0f;
	if( !m_postFX_DelayedWetnessEnabled ) dWetnessDir = -1.0f;
	if( !m_postFX_ImmediateWetnessEnabled ) m_postFX_ImmediateWetnessStrength = 0.0f;

	m_postFX_DelayedWetnessStrength += gameTimeDelta == 0.0f ? 0.0f : (dWetnessDir/m_blendTime)*gameTimeDelta;
	m_postFX_DelayedWetnessStrength = Clamp<Float>(m_postFX_DelayedWetnessStrength, 0.0f, 1.0f);	

	// Ticks the wind params
	m_windParams.TickWindParameters( gameTimeDelta );	
}

Int32 CWeatherManager::FindEffect( CParticleSystem* targetPs, Uint32 ringType )
{
	CParticleSystem* currentPs = nullptr;

	for(Int32 j=0; j<WEATHER_EFFECTS_MAX; j++) 
	{
		currentPs = m_weatherEffectsBuff[j][ ringType ]->GetParticleSystem();
		if( currentPs == targetPs )
		{
			return j;
		}								
	}
	return -1;
}

void CWeatherManager::SetWindParams( const Float envCloudsVelocityOverride, const Float envWindStrengthOverride )
{
	m_currentWeatherCondition.SetWindParams( envCloudsVelocityOverride, envWindStrengthOverride );
	m_windParams.m_environmentWindStrengthOverride = envWindStrengthOverride;
}

void CWeatherManager::SerializeForGC( IFile& file )
{
	// Serialize weather condition environments
	for ( auto it=m_availableWeatherConditions.Begin(); it != m_availableWeatherConditions.End(); ++it )
	{
		if ( (*it).m_environmentDefinition )
		{
			file << (*it).m_environmentDefinition;
		}

		for( Uint32 i=0; i<WEATHER_EFFECTS_MAX; i++ )
		{
			if ( (*it).m_weatherEffect[i].m_particleSystem )
			{
				file << (*it).m_weatherEffect[i].m_particleSystem;
			}
		}
	}
	if ( m_currentWeatherCondition.m_environmentDefinition )
	{
		file << m_currentWeatherCondition.m_environmentDefinition;
	}
	if ( m_targetWeatherCondition.m_environmentDefinition )
	{
		file << m_targetWeatherCondition.m_environmentDefinition;
	}
	if ( m_sourceWeatherCondition.m_environmentDefinition )
	{
		file << m_sourceWeatherCondition.m_environmentDefinition;
	}
}

void CWeatherManager::RegisterCWindAreaComponent( CWindAreaComponent* comp )
{
	m_windAreaComponents.PushBackUnique( comp );
}
void CWeatherManager::UnRegisterCWindAreaComponent( CWindAreaComponent* comp )
{
	m_windAreaComponents.Remove( comp );
}

// Script natives
#define GET_WEATHER_MANAGER \
	CWeatherManager* weatherManager = GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetEnvironmentManager() ? GGame->GetActiveWorld()->GetEnvironmentManager()->GetWeatherManager() : NULL;

static void funcGetWeatherConditionName( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	GET_WEATHER_MANAGER;

	RETURN_NAME( weatherManager ? weatherManager->GetCurrentWeatherCondition().m_name : CName::NONE );
}

static void funcRequestWeatherChangeTo( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, weatherName, CName::NONE );
	GET_PARAMETER( Float, blendTime, WEATHER_CONDITION_BLEND_TIME );
	GET_PARAMETER( Bool, questPause, false );
	FINISH_PARAMETERS;
	GET_WEATHER_MANAGER;
	
	if ( weatherManager)
	{
		const SWeatherCondition* weatherCondition = weatherManager->FindWeatherCondition( weatherName, nullptr );
		if ( !weatherCondition )
		{
			RETURN_BOOL( false );
			return;
		}

		weatherManager->SetBlendTime( blendTime );
		weatherManager->RequestWeatherChangeTo( weatherName, true );
		weatherManager->AddEvolutionTime( 900.0f );
		// quest/design team request, un-pause the weather system
		weatherManager->RequestWeatherPause( questPause );

		RETURN_BOOL( true );
	}
	else
	{
		RETURN_BOOL( false );
	}
}


static void funcRequestRandomWeatherChange( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, blendTime, WEATHER_CONDITION_BLEND_TIME );
	GET_PARAMETER( Bool, questPause, false );
	
	FINISH_PARAMETERS;
	GET_WEATHER_MANAGER;

	if( weatherManager != nullptr )
	{
		Bool presetFound = false;
		Uint32 mostProbableIndex = 0;
		const TDynArray<SWeatherCondition> arr = weatherManager->GetWeatherConditions();		

		if( weatherManager->GetRandomWeatherCondition( mostProbableIndex ) )
		{
			if( mostProbableIndex > -1 && mostProbableIndex < arr.Size() )
			{
				const SWeatherCondition* weatherCondition = &arr[ mostProbableIndex ];

				weatherManager->SetBlendTime( blendTime );
				weatherManager->RequestWeatherChangeTo( *weatherCondition, true );				
				presetFound = true;
			}
		}

		// un-pause pause the weather system
		weatherManager->RequestWeatherPause( questPause );

		if( !presetFound )
		{
			RETURN_BOOL( false );
			return;
		}		
		RETURN_BOOL( true );
	}
	else
	{
		RETURN_BOOL( false );
	}
}

static void funcGetRainStrength( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	GET_WEATHER_MANAGER;

	RETURN_FLOAT( weatherManager ? weatherManager->GetEffectStrength( EWeatherEffectType::WET_RAIN ) : 0.0f );
}

static void funcGetSnowStrength( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	GET_WEATHER_MANAGER;

	RETURN_FLOAT( weatherManager ? weatherManager->GetEffectStrength( EWeatherEffectType::WET_SNOW ) : 0.0f );
}

static void funcIsSkyClear( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	GET_WEATHER_MANAGER;
	Bool	isSkyClear	= true;
	Uint32	counter		= 0;
	for (Uint32 i = 0; i < WEATHER_EFFECTS_MAX ; i++)
	{
		if (weatherManager->GetCurrentWeatherCondition().m_weatherEffect[i].m_active && weatherManager->GetCurrentWeatherCondition().m_weatherEffect[i].m_type == WET_CLOUDS )
		{
			counter++;
		}
	}
	
	if(counter>=2)	isSkyClear = false;		// Clear sky preset has one CLOUD effect, Cloudy presets have more

	RETURN_BOOL( isSkyClear );
}

void ExportWeatherManagerNatives()
{
	NATIVE_GLOBAL_FUNCTION( "GetRainStrength",						funcGetRainStrength );
	NATIVE_GLOBAL_FUNCTION( "GetSnowStrength",						funcGetSnowStrength );
	NATIVE_GLOBAL_FUNCTION( "IsSkyClear",							funcIsSkyClear );
	NATIVE_GLOBAL_FUNCTION( "GetWeatherConditionName",				funcGetWeatherConditionName );
	NATIVE_GLOBAL_FUNCTION( "RequestWeatherChangeTo",				funcRequestWeatherChangeTo );
	NATIVE_GLOBAL_FUNCTION( "RequestRandomWeatherChange",			funcRequestRandomWeatherChange );
}

CWindParameters CWeatherManager::GetCurrentWindParameters()
{ 
	CWindParameters retParams = m_windParams;
	// scale with post overrides applied
	Float currScale = GetWindScale();
	retParams.SetWindScale( currScale );
	return retParams;
};

Vector CWeatherManager::GetCurrentWindVector( const Vector & gp )
{
	Vector wind = m_windParams.GetWindDirection() * GetWindScale();		
	{
		PC_SCOPE(SimplexTree_FindIDAtPoint)
		Int32 id = m_SimplexTree.FindIDAtPoint( gp.X, gp.Y );

		if( id==0 )
		{
			wind *= 0.0f;
		}
	}	
	return wind;
}

Bool CWeatherManager::GetCurrentWindVector( Uint32 elementsCount, void* inputPos, void* outputPos, size_t stride )
{	
	
	Vector wind =  m_windParams.GetWindDirection() * GetWindScale();
	
	for( Uint32 j = 0; j != elementsCount; ++j )
	{
		float* X = ( float* ) ( ( char* ) inputPos + stride * j );
		float* output = ( float* ) ( ( char* )outputPos + stride * j );

		Int32 id = m_SimplexTree.FindIDAtPoint( *X, *( X + 1 ) );

		if( id==0 )
		{
			*output = 0.0f;
			*( output + 1 ) = 0.0f;
		}
		else
		{
			*output = wind.X;
			*( output + 1 ) = wind.Y;
		}
	}
	return true;
}
