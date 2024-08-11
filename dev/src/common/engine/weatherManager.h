/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/simplexTreeResource.h"
#include "environmentManager.h"
#include "particleComponent.h"
#include "baseEngine.h"

#define WEATHER_EFFECTS_MAX 5
#define WEATHER_CONDITION_BLEND_TIME 45.0f

class CParticleSystem;

enum EWeatherBlendType
{
	WBT_CLEAR_TO_OVERCAST,
	WBT_OVERCAST_TO_CLEAR,
	WBT_OVERCAST_TO_OVERCAST,
	WBT_CLEAR_TO_CLEAR
};

enum EWeatherEffectType				// See bottom ToString( weatherEffectType type )!!!!
{
	WET_EFFECT_FIRST,
	WET_CLOUDS = WET_EFFECT_FIRST,
	WET_RAIN,
	WET_SNOW,
	WET_HAIL,	
	WET_THUNDERBOLT,
	WET_EFFECTS_TYPES_MAX				// MAX !!!
};

struct SWeatherEvent
{
	Bool							m_active;
	Float							m_lifeTime;
	Float							m_lifeRandom;
	Float							m_currentlifeTime;

	SWeatherEvent()
	{
		m_active = false;
		m_lifeTime = 1.0f;
		m_lifeRandom = 0.0f;
		m_currentlifeTime = 0.0f;
	}

	Bool Tick( Float deltaTime )
	{
		if( m_active )
		{
			m_currentlifeTime -= deltaTime;
			if( m_currentlifeTime <= 0.0f )
			{
				// trigger the sound event here
				m_currentlifeTime = m_lifeTime + ( GEngine->GetRandomNumberGenerator().Get< Float >( 0.0f, m_lifeRandom ) );
				return true;
			}
		}

		return false;
	}
};

struct SWeatherEffect
{
	Bool							m_active;
	Float							m_probability;
	Float							m_currentStrength;
	Float							m_targetStrength;
	EWeatherEffectType				m_type;
	THandle< CParticleSystem >		m_particleSystem;
	Uint8							m_particlePriority;

	SWeatherEffect()
		:	m_active( false ),
			m_probability( 0.0f ),
			m_currentStrength( 0.0f ),
			m_targetStrength( 0.0f ),
			m_particlePriority( 100 ),
			m_type( WET_CLOUDS ),
			m_particleSystem( nullptr )
	{}
};

/// SWeatherCondition
struct SWeatherCondition
{
	CName							m_name;							//!< Name of the weather condition
	Float							m_occurrenceProbability;		//!< Probability that the condition will occur
	Float							m_windScale;
	Float							m_environmentWindStrengthOverride;

	Float							m_skyboxTextureIndex;			//!< Skybox texture index from the current array
	Float							m_fakeShadowTextureIndex;		//!< Fake shadow texture index from the current array
	Bool							m_backgroundThunder;			//!< Should use distant thunders sounds

	THandle< CEnvironmentDefinition >	m_environmentDefinition;	//!< Optional environment definition
	Float							m_envDefBlend;					//!< Maximum blend for m_environmentDefinition
	Float							m_occurenceTime;				//!< Typical time for this condition to stay visible (seconds)
	Float							m_blendTime;

	SWeatherEffect					m_weatherEffect[ WEATHER_EFFECTS_MAX ];	//!< Weather particle effects with additional info
	SWeatherEvent					m_event;

	// Default constructor
	RED_INLINE SWeatherCondition()
		: m_occurrenceProbability( 0.0f )
		, m_skyboxTextureIndex( -1.0f )	
		, m_fakeShadowTextureIndex( -1.0f )
		, m_environmentDefinition( NULL )
		, m_envDefBlend( 0.0f )	
		, m_occurenceTime( 600.0f )		
		, m_windScale( 0.1f )
		, m_blendTime( WEATHER_CONDITION_BLEND_TIME )
		, m_environmentWindStrengthOverride( 1.0f )
		, m_backgroundThunder( false )
		, m_name( CName::NONE )
	{
		for(Uint32 i=0; i<WEATHER_EFFECTS_MAX; i++) m_weatherEffect[i] = SWeatherEffect();
	}

	// Set wind parameters for this condition
	RED_INLINE void SetWindParams( Float envCloudsVelocityOverride, Float envWindStrengthOverride )
	{
		//m_windParams.m_accumTimeSinceGameStart = GGame->GetTimeManager()->GetTimeAccum();		
		m_environmentWindStrengthOverride = envWindStrengthOverride;
	}

	// Clear weather constant
	static SWeatherCondition CLEAR_WEATHER;
};

/// CWeatherManager
class CWeatherManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );
public:
	static const AnsiChar* THUNDERS_START;
	static const AnsiChar* THUNDERS_STOP;

private:
	class CEnvironmentManager*					m_environmentManager;			//!< The environment manager this weather manager belongs to

	TDynArray< SWeatherCondition >				m_availableWeatherConditions;	//!< All available weather conditions
	SWeatherCondition							m_currentWeatherCondition;		//!< Current weather condition
	SWeatherCondition							m_sourceWeatherCondition;		//!< Weather blending: source weather condition
	SWeatherCondition							m_targetWeatherCondition;		//!< Weather blending: target weather condition
	
	Float										m_blendTime;						//!< Blend time between different weather conditions
	Float										m_currentBlendTime;				//!< How far we are in the blending process now
	Bool											m_isBlendTimeForced;				//!< Is blend time forced from quest / script?

	TEnvManagerAreaEnvId							m_envId;							//!< Environment ID for environment blending
	Bool											m_running;						//!< The weather system is running
	Bool											m_questPause;					//!< Weather system can be paused (not randomizing conditions) based on quest status

	Float										m_currentEvolutionTime;
	Float										m_averageEvolutionTime;

	EWeatherBlendType							m_weatherBlendType;

	Float										m_currentSkyboxBlendRatio;
	
	THandle< CEntity >							m_effectEntity;		
	Uint32										m_effectRingBuffInd;

	THandle< CParticleComponent	>				m_weatherEffectsBuff[ WEATHER_EFFECTS_MAX ][ 2 ];	
	SWeatherEffect								m_cachedWeatherEffects[ WET_EFFECTS_TYPES_MAX ];	

	// TODO
	Bool											m_postFX_DelayedWetnessEnabled;
	Float										m_postFX_DelayedWetnessStrength;

	Bool											m_postFX_ImmediateWetnessEnabled;
	Float										m_postFX_ImmediateWetnessStrength;

	Float										m_windFactorEventOverride;

	SWeatherCondition							m_queueWeatherCondition;
	Uint32										m_queueWeatherConditionPriority;
	Bool											m_processQueue;

	CTextureArray*								m_wetSurfaceTexture;

	CWindParameters								m_windParams;					//!< Wind parameters

	CSimplexTreeNode								m_SimplexTree;

	TDynArray< CWindAreaComponent* >				m_windAreaComponents;
	Red::Threads::CAtomic< Int32 >				m_playerIsInInteriorRefCount;

												// Called by other functions when the available weather conditions array contents change
	void										WeatherConditionsChanged();

												// Disable all weather condition blending using the blendTime
	void										DisableActiveWeatherConditionBlending( Float blendTime );
	Int32										FindEffect( CParticleSystem* targetPs, Uint32 ringType );
	RED_INLINE Float							GetBlendingRatioFromTimeline( Float ratio, Float startingPoint, Float finishingPoint ) {return ( Clamp<Float>( ratio, startingPoint, finishingPoint ) - startingPoint) / (finishingPoint - startingPoint);}

	void										SetWeatherBlendType();
	void										FinalizeCurrentBlend();
	void										CreateEffectsComponents();
	void										ActivateEffect( Uint32 slot, Uint32 buffRing, Float effectAlpha );
	void										SyncCachedEffects();
	void										SyncEffectInfo( Float currentEffectStrength, Float targetEffectStrength, Uint32 i );

public:

	RED_INLINE	void							SetWindFactorEventOverride( Float v ){ m_windFactorEventOverride = v; }
	RED_INLINE Bool 							GetPlayerIsInInterior() const { return m_playerIsInInteriorRefCount.GetValue() > 0; }
	void										SetPlayerIsInInterior( Bool isInInterior );
	void										ResetPlayerIsInInterior();

												// Returns the world this weather manager belongs to
	RED_INLINE class CWorld*					GetWorld() const { return m_environmentManager->GetWorld(); }

												// Returns the environment manager this weather manager belongs to
	RED_INLINE class CEnvironmentManager*		GetEnvironmentManager() const { return m_environmentManager; }

												// Returns true if the weather system is running, otherwise false if it is paused
	RED_INLINE Bool								IsRunning() const { return m_running; }

												// Returns the current weather condition
	RED_INLINE const SWeatherCondition&			GetCurrentWeatherCondition( Bool saveGameRequest = false ) const { return saveGameRequest ? ( m_currentBlendTime > 0.0f ? m_targetWeatherCondition : m_currentWeatherCondition ) : m_currentWeatherCondition; }

												// Returns the source weather condition
	RED_INLINE const SWeatherCondition&			GetTargetWeatherCondition() const { return m_targetWeatherCondition; }

												// Returns weather blend ratio
	RED_INLINE Float							GetBlendingRatio() const { return Clamp<Float>( 1.0f - m_currentBlendTime/m_blendTime, 0.0f, 1.0f ); }

	RED_INLINE Vector							GetSkyboxBlendRatio() const { return Vector( m_currentWeatherCondition.m_skyboxTextureIndex, m_targetWeatherCondition.m_skyboxTextureIndex, m_currentSkyboxBlendRatio ); }

	RED_INLINE Vector							GetFakeShadowsParameters() const { return Vector( m_currentWeatherCondition.m_fakeShadowTextureIndex, m_targetWeatherCondition.m_fakeShadowTextureIndex, 0.0f, 0.0f ); }
		
												// Wind
	RED_INLINE Float							GetWindScale() const { return m_windParams.GetWindScale() * m_windFactorEventOverride; };
	RED_INLINE Float							GetWindRotationZ() const { return m_windParams.GetWindRotationZ(); };		
	RED_INLINE Vector							GetCurrentWindVector(){ return m_windParams.GetWindDirection() * GetWindScale(); };
	Vector										GetCurrentWindVector( const Vector & gp );
	Bool										GetCurrentWindVector( Uint32 elementsCount, void* inputPos, void* outputPos, size_t stride );
	
	CWindParameters								GetCurrentWindParameters();

	RED_INLINE TEnvManagerAreaEnvId				GetCurrentEnvironmentId() const { return m_envId; };

												// Returns the weather conditions array
	RED_INLINE const TDynArray< SWeatherCondition >& GetWeatherConditions() const { return m_availableWeatherConditions; }

	RED_INLINE const CTextureArray*				GetWetSurfaceTexture() const { return m_wetSurfaceTexture; }

	RED_INLINE const Float						GetEffectStrength( EWeatherEffectType effectTypeToGet ) const { return m_cachedWeatherEffects[ effectTypeToGet ].m_currentStrength; }
	
	RED_INLINE const Float						GetDelayedWetSurfaceEffectStrength() const { return m_postFX_DelayedWetnessStrength; }	
	RED_INLINE const Float						GetImmediateWetSurfaceEffectStrength() const { return m_postFX_ImmediateWetnessStrength; }			
	void											GetCurrentWeatherConditionInfo( String& currentConditionName, Float& currentConditionStatus );	
	Bool										GetRandomWeatherCondition( Uint32& mostProbableIndex );

	#ifndef NO_EDITOR
	String										GetDebugStatus();
	#endif
public:
	CWeatherManager( class CEnvironmentManager* environmentManager );

	~CWeatherManager();

	// Remove all weather conditions
	void RemoveWeatherConditions();

	// Add the passed weather condition
	void AddWeatherCondition( const SWeatherCondition& condition );

	// Add weather conditions from the passed template 
	void AddWeatherConditionsFromTemplate( C2dArray* weatherTemplate );

	// Returns the weather condition with that name or NULL on failure (note: do not keep the weather condition around since
	// it is a pointer to an array and it will be invalidated soon - the only reason this returns a pointer is to indicate
	// failure with the NULL value)
	const SWeatherCondition* FindWeatherCondition( const CName& weatherConditionName, Int32 *outAvailableConditionIndex ) const;

	// Request a weather change from the current weather state to the given weather state
	void RequestWeatherChangeTo( const SWeatherCondition& targetCondition, Uint32 priority, Bool skipQueue = false );
	
	// Request a weather change from the current weather state to weather state with the given name - returns false on failure
	Bool RequestWeatherChangeTo( const CName& weatherConditionName, Bool skipQueue = false );
	
	// Request a weather to randomize next condition;
	Bool QueueRandomWeatherChange();

	// Set the blend time for weather change requests
	void SetBlendTime( Float t );

	// Set the running state of the wather system - true (default) makes it evolve, false pauses it
	void SetRunning( Bool enable );

	// Tick the weather manager
	void Tick( float timeDelta, Float gameHoursPerMinute );

	// Set current wind parameters
	void SetWindParams( const Float envCloudsVelocityOverride, const Float envWindStrengthOverride );
	
	// Serialize objects for the garbage collector
	void SerializeForGC( IFile& file );

	// Stabilize current weather condition (remove blending)
	void StabilizeCurrentWeather();	

	// Register / Unregister WindComponent
	void RegisterCWindAreaComponent( CWindAreaComponent* comp );
	void UnRegisterCWindAreaComponent( CWindAreaComponent* comp );

	// Sets particle system on slot and ring
	void SetParticleSystem( Uint32 slot, Uint32 buffRing , CParticleSystem * ps , const CParticleComponent::SEffectInfo * info = nullptr );

	void AddEvolutionTime( Float time );

	// quest/design team request, pause the weather system
	void RequestWeatherPause( Bool setPause ) { m_questPause = setPause; };

};
