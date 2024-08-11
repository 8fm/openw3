/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "environmentManager.h"
#include "weatherManager.h"
#include "renderCommands.h"
#include "mesh.h"
#include "textureArray.h"
#include "game.h"
#include "gameTimeManager.h"
#include "soundSystem.h"
#include "environmentComponentArea.h"
#include "worldIterators.h"
#include "layer.h"
#include "world.h"
#include "clipMap.h"
#include "dynamicLayer.h"
#include "environmentDefinition.h"
#include "../core/2darray.h"
#include "../core/gameSave.h"
#include "../core/depot.h"
#include "../core/scriptStackFrame.h"
#include "../core/feedback.h"
#include "../core/dataError.h"
#include "entity.h"
#include "cameraComponent.h"
#include "materialInstance.h"
#include "utils.h"

#define LOG_DEBUG_INFO				0

EEnvManagerModifier	CEnvironmentManager::s_modifier = EMM_None;

RED_DEFINE_STATIC_NAME( OnRainStarted );
RED_DEFINE_STATIC_NAME( OnRainEnded );
RED_DEFINE_STATIC_NAME( QuestEnvDepotPath );
RED_DEFINE_STATIC_NAME( QuestEnvPriority );
RED_DEFINE_STATIC_NAME( QuestEnvBlendFactor );

IMPLEMENT_RTTI_ENUM( EEnvManagerModifier );
IMPLEMENT_RTTI_ENUM( EDebugPostProcess );

static CAreaEnvironmentComponent* FindAreaEnvironmentComponent( const String &name );

namespace Config
{
	TConfigVar< Float >		cvGameplayCameraLightFadeToInterior( "Rendering", "GameplayCameraLightFadeToInterior", 0.5f, eConsoleVarFlag_Developer );
	TConfigVar< Float >		cvGameplayCameraLightFadeToExterior( "Rendering", "GameplayCameraLightFadeToExterior", 0.5f, eConsoleVarFlag_Developer );
}

/////////////////////////////////////////////////////////////////////////////
// CEnvironmentManager

CEnvironmentManager::CEnvironmentManager ( CWorld *world )
 : m_defaultCurvesParams ( EnvResetMode_CurvesDefault )
 , m_currAreaEnvParams ( EnvResetMode_CurvesDefault )
 , m_globalId( -1 )
 , m_scenesId( -1 )
 , m_questId( -1 ) 
 , m_environmentBlends( 0 )
 , m_environmentSlots( 0 )
 , m_mostImportantBlendFactor( 0 ) 
 , m_lastKnownGameTime( 0 )
 // E3 DEMOHACK storm lightning
 , m_baseLightingParamsOverride( NULL )
 , m_ambientOverride( Vector::ZEROS )	// ZEROS means no override
 , m_balanceOverride( NULL )
 , m_hiResShadowMaxExtents( -1.0f )
 // -
 , m_lastGameplayCameraLightsFactor( 1.f )
 , m_lastGameplayCameraInterior( false )
 , m_lastGameplayCameraLightsCamPos( 0, 0, 0 )
 , m_lastGameplayCameraLightsTime( -1 )
 , m_distantLightOverride( -1.0f )
{
	COMPILE_ASSERT( -1 == INVALID_AREA_ENV_ID );
	m_areaEnvIdGenerator = 1; //< don't start at zero because it may be incorrectly used as 'invalid id' in some places

	m_world = world;
	m_currAreaEnvParamsForced = false;
	m_currAreaEnvUpdated = false;
	m_instantAdaptationTrigger = true;
	m_instantDissolveTrigger = true;	
	m_envChangesDisabled = false;
	m_radialBlurDefaultParams = m_radialBlurManager.GenerateNewRadialBlurParams();
#ifndef NO_EDITOR
	m_activeCamera = NULL;
#endif
	
	// Create weather system
	m_weatherManager = new CWeatherManager( this );
	m_weatherManager->AddWeatherConditionsFromTemplate( world->GetEnvironmentParameters().m_weatherTemplate.Get() );
	m_weatherManager->QueueRandomWeatherChange();

	// Create dynamic entity for particle systems
	EntitySpawnInfo info;
	//info.m_name = TXT("DynamicParticlesEntity");
	CLayer* dynamicLayer = world->GetDynamicLayer();


#ifndef NO_DEBUG_WINDOWS
	// gather all predefined environments from engine/environments
	m_defaultAreaEnvironments.Clear();
	m_defaultAreaEnvironmentsNames.Clear();

	// keep the default env as OFF option
	if( m_world->GetEnvironmentParameters().m_environmentDefinition.IsValid() )
	{		
		Red::TUniquePtr< CAreaEnvironmentParams > aep( new CAreaEnvironmentParams( m_world->GetEnvironmentParameters().m_environmentDefinition->GetAreaEnvironmentParams() ) );
		m_defaultAreaEnvironments.PushBack( std::move( aep ) );
	}	
	else
		m_defaultAreaEnvironments.PushBack( Red::TUniquePtr< CAreaEnvironmentParams >( new CAreaEnvironmentParams( EnvResetMode_Default ) ) );

	m_defaultAreaEnvironmentsNames.PushBack( TXT("default") );

	// TODO
	CDirectory* dir = GDepot->FindPath( TXT("engine\\environments\\") );

	if( dir ) 
	{
		const TFiles& files = dir->GetFiles();
		for ( CDiskFile* file : files )
		{
			if ( file->GetFileName().EndsWith( TXT("env") ) )
			{
				THandle< CEnvironmentDefinition > ed = Cast< CEnvironmentDefinition >( file->Load() );
				if ( ed  )
				{
					Red::TUniquePtr< CAreaEnvironmentParams > aep( new CAreaEnvironmentParams( ed->GetAreaEnvironmentParams() ) );
					m_defaultAreaEnvironments.PushBack( std::move( aep ) );
					m_defaultAreaEnvironmentsNames.PushBack( file->GetFileName().StringBefore( TXT(".env") ) );
				}
			}			
		}

		if( m_defaultAreaEnvironments.Empty() ) GFeedback->ShowWarn(TXT("Not even a single predefined environment found! - Get latest on: data//engine//environments") );
	}
	else
		GFeedback->ShowWarn(TXT("Predefined environments directory not found! - Get latest on: data//engine//environments") );
#endif
}

CEnvironmentManager::~CEnvironmentManager ()
{
	m_radialBlurManager.RemoveRadialBlurParams( m_radialBlurDefaultParams );

	// Delete weather manager
	ASSERT( m_weatherManager, TXT("Weather manager managed to be NULL") );
	delete m_weatherManager;
	m_weatherManager = nullptr;
}


void CEnvironmentManager::OnGameStart( const CGameInfo& gameInfo )
{
	// Register in save game crap
	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );

	// Load state 
	if ( gameInfo.m_gameLoadStream )
	{
		IGameLoader* loader = gameInfo.m_gameLoadStream;
		CGameSaverBlock block0( loader, CNAME(envManager) );

		// Sepia
		Uint32 sepiaActive = 0;
		loader->ReadValue( CNAME(sepiaActive), sepiaActive );
		if ( sepiaActive )
		{
			EnableSepiaEffect( 0.0f );
		}

		// Weather condition
		CName weatherConditionName;
		loader->ReadValue( CNAME( weatherConditionName ), weatherConditionName );

		// Quest environment
		{
			String questEnvDepotPath;
			Int32 questEnvPriority = -1;
			Float questEnvBlendFactor = 1;
			loader->ReadValue( CNAME( QuestEnvDepotPath ), questEnvDepotPath );
			loader->ReadValue( CNAME( QuestEnvPriority ), questEnvPriority );
			loader->ReadValue( CNAME( QuestEnvBlendFactor ), questEnvBlendFactor );

			THandle< CEnvironmentDefinition > envDef;
			if ( !questEnvDepotPath.Empty() )
			{
				envDef = LoadResource< CEnvironmentDefinition >( questEnvDepotPath );
			}

			if ( envDef )
			{
				ActivateQuestEnvironment( *envDef, questEnvPriority, questEnvBlendFactor, 0, 0 );
			}
		}
	}
	else if ( ! gameInfo.m_keepExistingLayers ) // "GREEN PAD"
	{
	}	
}

void CEnvironmentManager::OnAfterLoadingScreenGameStart( const CGameInfo& gameInfo ) 
{
	ASSERT( m_weatherManager, TXT("Weather manager managed to be NULL") );	

	// Load state 
	if ( gameInfo.m_gameLoadStream )
	{
		IGameLoader* loader = gameInfo.m_gameLoadStream;
		CGameSaverBlock block0( loader, CNAME(envManager) );

		Uint32 sepiaActive = 0;
		loader->ReadValue( CNAME(sepiaActive), sepiaActive );

		// Weather condition
		CName weatherConditionName;
		loader->ReadValue( CNAME( weatherConditionName ), weatherConditionName );
		if ( weatherConditionName != CName::NONE ) m_weatherManager->RequestWeatherChangeTo( weatherConditionName, true );		
	}

	// starting new game / fast travel to new world
	// this will make sure weather is randomized	
	m_weatherManager->StabilizeCurrentWeather();
	m_weatherManager->SetRunning( true );
}

void CEnvironmentManager::OnShutdownAtGameEnd()
{
	while ( m_sepiaEnabled )
	{
		DisableSepiaEffect(0.0f);
	}

	if ( -1 != m_questId )
	{
		DeactivateEnvironment( m_questId );
	}

	if( m_weatherManager )
	{
		m_weatherManager->SetRunning( false );
	}
	else
	{
		ASSERT( m_weatherManager, TXT("Weather manager managed to be NULL") );
	}	

	// Register in save game crap
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );
}

bool CEnvironmentManager::OnSaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	CGameSaverBlock block( saver, CNAME(envManager) );

	// Save sepia
	saver->WriteValue( CNAME(sepiaActive), (Uint32)(m_sepiaEnabled) );
		
	// Save weather condition
	const CName weatherConditionName = m_weatherManager->GetCurrentWeatherCondition( true ).m_name;
	saver->WriteValue( CNAME( weatherConditionName ), weatherConditionName );

	// Save quest environment
	{
		String questEnvDepotPath;
		Int32 questEnvPriority = -1;
		Float questEnvBlendFactor = 1;

		// Get quest env params
		RED_ASSERT( (-1 != m_questId) == (!m_questEnvPath.Empty()) );
		if ( -1 != m_questId )
		{
			Int32 dataIndex = FindAreaEnvById( m_questId );
			RED_ASSERT( -1 != dataIndex && !m_questEnvPath.Empty() );
			if ( -1 != dataIndex && !m_questEnvPath.Empty() )
			{
				questEnvDepotPath = m_questEnvPath;
				questEnvPriority = m_areaEnvironments[dataIndex].priority;
				questEnvBlendFactor = m_areaEnvironments[dataIndex].blendFactor;
			}
		}

		// Write values
		saver->WriteValue( CNAME( QuestEnvDepotPath ), questEnvDepotPath );
		saver->WriteValue( CNAME( QuestEnvPriority ), questEnvPriority );
		saver->WriteValue( CNAME( QuestEnvBlendFactor ), questEnvBlendFactor );
	}

	END_TIMER_BLOCK( time )

	return true;
}


#ifndef NO_DEBUG_WINDOWS
const TDynArray<String>& CEnvironmentManager::GetDefaultEnvironmentsNames() const
{
	return m_defaultAreaEnvironmentsNames;
}
#endif

void CEnvironmentManager::SearchForAllAreaEnvs( CDirectory* rootDir, TDynArray< CEnvironmentDefinition* > &outParamas )
{
	if ( rootDir )
	{
		// Scan subdirectories
		for ( CDirectory* sub : rootDir->GetDirectories() )
		{			
			SearchForAllAreaEnvs( sub, outParamas );
		}

		// Scan all files
		for ( CDiskFile* file : rootDir->GetFiles() )
		{
			// Add the file to the array if it is an environment
			if ( file->GetFileName().EndsWith( TXT(".env") ) )
			{		
				THandle< CEnvironmentDefinition > envDef = Cast< CEnvironmentDefinition >( file->Load() );
				if ( envDef )
				{
					outParamas.PushBack( envDef );
				}
			}
		}
	}
}

void CEnvironmentManager::UpdateWeatherParametersFromWorld()
{
	m_weatherManager->AddWeatherConditionsFromTemplate( GetWorld() ? GetWorld()->GetEnvironmentParameters().m_weatherTemplate.Get() : NULL );
}

bool CEnvironmentManager::IsAreaEnvironmentActive( TEnvManagerAreaEnvId id ) const
{
	if ( INVALID_AREA_ENV_ID != id )
	{
		for ( Uint32 i=0; i<m_areaEnvironments.Size(); ++i )
		{
			Int32 aeId = m_areaEnvironments[i].id;
			if ( aeId == id )
			{
				return true;
			}
		}
	}
	return false;
}

TEnvManagerAreaEnvId CEnvironmentManager::GenerateAreaEnvId()
{
	TEnvManagerAreaEnvId newId;
	do 
	{
		newId = m_areaEnvIdGenerator++;
	}
	while (newId == INVALID_AREA_ENV_ID);
	return newId;
}

TEnvManagerAreaEnvId CEnvironmentManager::ActivateQuestEnvironment( const CEnvironmentDefinition &def, Int32 priority, Float blendFactor, Float blendInTime, Float prevQuestIdBlendOutTime )
{
	// Deactivate current quest environment
	if ( -1 != m_questId )
	{
		DeactivateEnvironment( m_questId, prevQuestIdBlendOutTime, true );
		RED_ASSERT( -1 == m_questId );
	}

	// Activate environment
	TEnvManagerAreaEnvId envId = ActivateAreaEnvironment( &def, nullptr, priority, blendFactor, blendInTime );
	if ( -1 == envId )
	{
		return -1;
	}

	// Init quest id related stuff
	m_questId = envId;
	m_questEnvPath = def.GetDepotPath();
	
	//
	return envId;
}

TEnvManagerAreaEnvId CEnvironmentManager::ActivateAreaEnvironment( const CEnvironmentDefinition* const def, class CAreaEnvironmentComponent* areaComponent, Int32 priority, Float blendFactor, Float blendInTime /*= 0.0f*/, TEnvManagerAreaEnvId prevIdHint /*= -1*/ )
{
	// get environment params
	const CAreaEnvironmentParams &params = def->GetAreaEnvironmentParams();

	TEnvManagerAreaEnvId areaId;
	Int32 dataIndex = FindAreaEnvById( prevIdHint );

	// Make sure the texture resources are loaded and available so they're properly
	// copied around (this avoids GC collecting them prematurely)
	params.m_finalColorBalance.m_balanceMap0.Get();
	params.m_finalColorBalance.m_balanceMap1.Get();

	// Environment does exist...
	if ( -1 != dataIndex )
	{
		SEnvManagerAreaEnvData &data = m_areaEnvironments[dataIndex];

		// Change environment data
		data.pathToEnvDefinition = def->GetDepotPath();
		data.areaEnv = params;
		data.areaComponent = areaComponent;
		data.priority = priority;
		data.blendFactor = blendFactor;
		if ( blendInTime > 0.0001f )
		{
			data.timeBlendStart = GGame->GetEngineTime() - data.timeBlendFactor * blendInTime;
			data.timeBlendState = ETBS_BlendIn;
			data.timeBlendTarget = data.timeBlendStart + blendInTime;
		}
		else
		{
			data.timeBlendStart = 0.f;
			data.timeBlendState = ETBS_Disabled;
			data.timeBlendTarget = 0.f;
		}

		// Set env update trigger so that we would know to update current environment
		m_currAreaEnvUpdated = false;

		// We're done
#if LOG_DEBUG_INFO
		LOG_ENGINE(TXT("EnvManager: ACTIVATION update"));
		LogState();
#endif
		areaId = data.id;
	}
	else
	{
		// Make sure we haven't reached the maximum number of area environments
		if ( m_areaEnvironments.Size() >= MAX_ENVIRONMENT_BLENDS )
		{
			DATA_HALT( DES_Major, ( areaComponent != NULL ) ? CResourceObtainer::GetResource( areaComponent ) : m_world, TXT("Environments"), TXT("Too many active environments!") );
			return -1;
		}

		// Generate new environment id
		TEnvManagerAreaEnvId newEnvId = GenerateAreaEnvId();
		if ( INVALID_AREA_ENV_ID == newEnvId )
		{
			WARN_ENGINE(TXT("Unable to activate environment - failed to generate id."));
			return INVALID_AREA_ENV_ID;
		}

		// Add new environment
		m_areaEnvironments.PushBack( SEnvManagerAreaEnvData () );
		const Uint32 newEnvIndex		= m_areaEnvironments.Size() - 1;
		SEnvManagerAreaEnvData &newData	= m_areaEnvironments.Back();
		newData.id						= newEnvId;
		newData.areaEnv					= params;
		newData.areaComponent			= areaComponent;
		newData.priority				= priority;
		newData.blendFactor				= blendFactor;
		newData.pathToEnvDefinition		= def->GetDepotPath();
#ifndef NO_DEBUG_WINDOWS
		newData.appliedBlendFactor = 0;
		newData.appliedMostImportantFactor = -1;
#endif
		if ( blendInTime > 0.0001f )
		{
			newData.timeBlendStart = GGame->GetEngineTime();
			newData.timeBlendState = ETBS_BlendIn;
			newData.timeBlendTarget = newData.timeBlendStart + blendInTime;
		}


		// Set env update trigger so that we'll know to update current environment
		m_currAreaEnvUpdated = false;

#if LOG_DEBUG_INFO
		LOG_ENGINE(TXT("EnvManager: ACTIVATION new"));
		LogState();
#endif

		areaId = newEnvId;
	}

	// Rebuild slots
	RebuildEnvironmentSlots();

	// Dispatch event
	EDITOR_DISPATCH_EVENT( CNAME(GlobalAreaEnvironmentsListUpdate), NULL );

	return areaId;
}

void CEnvironmentManager::ChangeAreaEnvironmentParameters( TEnvManagerAreaEnvId id, const CAreaEnvironmentParams &params )
{
	Int32 dataIndex = FindAreaEnvById( id );
	if ( -1 != dataIndex )
	{
		m_areaEnvironments[dataIndex].areaEnv = params;
		m_currAreaEnvUpdated = false;
	}
}

void CEnvironmentManager::ChangeAreaEnvironmentBlendFactor( TEnvManagerAreaEnvId id, Float blendFactor )
{
	Int32 dataIndex = FindAreaEnvById( id );
	if ( -1 != dataIndex )
	{
		blendFactor = Clamp( blendFactor, 0.f, 1.f );
		Float &refBlendFactor = m_areaEnvironments[dataIndex].blendFactor;
		if ( refBlendFactor != blendFactor )
		{
			refBlendFactor = blendFactor;
			m_currAreaEnvUpdated = false;
		}
	}
}

void CEnvironmentManager::DeactivateEnvironment( TEnvManagerAreaEnvId id, Float blendOutTime, Bool forceDropCachedId )
{
	// Find the index of the environment
	Int32 dataIndex = FindAreaEnvById( id );
	if ( -1 == dataIndex )
	{
		return;
	}

	// If a blendout time was given, check if the final time guarantees a wait for later
	if ( blendOutTime > 0.0001f )
	{
		m_areaEnvironments[dataIndex].timeBlendState = ETBS_BlendOut;
		m_areaEnvironments[dataIndex].timeBlendStart = GGame->GetEngineTime() - (1.0f - m_areaEnvironments[dataIndex].timeBlendFactor) * blendOutTime;
		m_areaEnvironments[dataIndex].timeBlendTarget = m_areaEnvironments[dataIndex].timeBlendStart + blendOutTime;

		if ( forceDropCachedId )
		{
			DropCachedEnvironmentId( id );
		}

		return;
	}

	// No blendout time given, remove it now
	m_areaEnvironments.RemoveAt( dataIndex );

	// Remove locally cached id's if any of those got removed
	DropCachedEnvironmentId( id );

	// Rebuild slots
	RebuildEnvironmentSlots();

	// Dispatch editor event
	EDITOR_DISPATCH_EVENT( CNAME(GlobalAreaEnvironmentsListUpdate), NULL );

#if LOG_DEBUG_INFO
	LOG_ENGINE(TXT("EnvManager: DEACTIVATION"));
	LogState();
#endif
}

Int32 CEnvironmentManager::FindAreaEnvById( TEnvManagerAreaEnvId id ) const
{
	for ( Int32 i=0; i<(Int32)m_areaEnvironments.Size(); ++i )
		if ( id == m_areaEnvironments[i].id )
			return i;
	return -1;
}

GameTime CEnvironmentManager::GetCurrentGameTime( bool continous ) const
{
	const CGameEnvironmentParams &gameParams = GetGameEnvironmentParams();

	GameTime gameTime = 0;
	if ( gameParams.m_dayCycleOverride.m_fakeDayCycleEnable )
	{
		Float hour = gameParams.m_dayCycleOverride.m_fakeDayCycleHour;
		gameTime = GameTime ( (Int32)( hour * 60 * 60 ) );
	}
	else
	{
		gameTime = (GGame && GGame->GetTimeManager() ? GGame->GetTimeManager()->GetTime() : 0);
	}

	if ( !continous )
	{
		const Int32 range = 24 * 60 * 60;
		gameTime = gameTime.m_seconds >= range ? GameTime( gameTime.m_seconds % range ) : gameTime;
	}

	return gameTime;
}

EngineTime CEnvironmentManager::GetCurrentEnvAnimationTime() const
{
	EngineTime animTime = 0.f;
	if ( NULL!=GGame && !GetGameEnvironmentParams().m_dayCycleOverride.m_fakeDayCycleEnable )
	{
		animTime = GGame->GetEngineTime();
	}
	else
	{
		animTime = GetCurrentGameTime( true ).ToFloat();
	}
	return animTime;
}

bool CEnvironmentManager::IsAreaEnvironmentForced() const
{
	return m_currAreaEnvParamsForced;
}

void CEnvironmentManager::EnableForcedAreaEnvironment( const CAreaEnvironmentParams &areaEnv )
{
	m_currAreaEnvParams = areaEnv;
	m_currAreaEnvParamsForced = true;
	m_currAreaEnvUpdated = true;
}

void CEnvironmentManager::DisableForceAreaEnvironment()
{
	if ( m_currAreaEnvParamsForced )
	{
		m_currAreaEnvParamsForced = false;
		m_currAreaEnvUpdated = false;
	}
}

Bool CEnvironmentManager::SwitchToPredefinedEnv( Uint32 selection )
{
	RED_UNUSED( selection );
#ifndef NO_DEBUG_WINDOWS
	if( !m_defaultAreaEnvironments.Empty() && selection < m_defaultAreaEnvironments.Size() )
	{
		CAreaEnvironmentParams aep = (*m_defaultAreaEnvironments[ selection ]);	
		SWorldEnvironmentParameters wep = m_world->GetEnvironmentParameters();
		
		if ( !wep.m_environmentDefinition.IsValid() )
		{
			wep.m_environmentDefinition = new CEnvironmentDefinition();		
		}
		wep.m_environmentDefinition->SetAreaEnvironmentParams( aep );		
		m_world->SetEnvironmentParameters( wep );

		// this stuff doesn't seem to work anymore
		//EnableForcedAreaEnvironment( (*m_defaultAreaEnvironments[ selection ]) );					
		return true;
	}
	else
	{
		DisableForceAreaEnvironment();
		return false;
	}
#else
	return false;
#endif	// NO_EDITOR
}

/*
// clouds system testing
TDynArray< SWeatherCloudDefinition* > CEnvironmentManager::GetDynamicClouds()
{
	return m_weatherManager->GetDynamicClouds();
}
*/

#if 0
void CEnvironmentManager::RequestWeatherChangeTo( EWeatherType state, Float blendOverTime )
{
	// clamp weather blending times to reasonable values
	m_weatherLogic.SetBlendTime( Clamp(blendOverTime, 0.01f, 3600.0f ) );
	m_weatherLogic.RequestWeatherChangeTo( EWeatherType (state) );
}
#endif

// E3 DEMOHACK stormlightning
void CEnvironmentManager::SetBaseLightingOverride( CRenderBaseLightParams* params )
{
	m_baseLightingParamsOverride = params;
}

void CEnvironmentManager::SetAmbientOverride( const Vector& ambient )
{
	m_ambientOverride = ambient;
}

void CEnvironmentManager::SetBalanceOverride( SBalanceOverride* balanceOverride )
{
	m_balanceOverride = balanceOverride;
}
// -

#if 0
Bool CEnvironmentManager::SwitchToPredefinedWeatherCondition( Uint32 selection )
{
	//if( selection > -1 && selection < WT_COUNT )
	{
		m_weatherLogic.RequestWeatherChangeTo( EWeatherType (selection) );
		return true;
	}

	return false;
}
#endif

Vector CEnvironmentManager::GetCurrentAreaEnvUpdateReferencePos() const
{
	// Get active camera
	const CCameraComponent *activeCamera = NULL;

#ifndef NO_EDITOR
	// Editor only stuff
	const CCameraComponent* localCamera = m_activeCamera.Get();
	if( localCamera != NULL )
	{
		activeCamera = localCamera;
	}
#endif

	return activeCamera ? activeCamera->GetWorldPosition() : m_world ? m_world->GetCameraPosition() : Vector::ZERO_3D_POINT;
}

void CEnvironmentManager::UpdateCurrentAreaEnvironment()
{
	// Override some params by modifier
	if ( Config::cvForcedDebugPreviewIndex.Get() >= 0 && Config::cvForcedDebugPreviewIndex.Get() < (Int32)EMM_MAX )
	{
		m_gameEnvParams.m_displaySettings.m_displayMode = (Uint8)Config::cvForcedDebugPreviewIndex.Get();
	}		
	else
	{
		m_gameEnvParams.m_displaySettings.m_displayMode = (Uint8)s_modifier;
	}

	if ( m_currAreaEnvParamsForced )
	{
		m_currAreaEnvUpdated = true;
	}	

	// Update current area environment parameters

	if ( !m_currAreaEnvUpdated )
	{
		const Float importTime = GetCurrentGameTime().ToFloat() / (24.f * 60.f * 60.f);
		const Vector position = GetCurrentAreaEnvUpdateReferencePos();

		// Update current environment
		CalculateEnvironmentForGivenPositionAndTime( position, importTime, m_currAreaEnvParams );

		// Apply weather modifications
		ApplyWeatherModifications( m_currAreaEnvParams, importTime );

		//
		m_gameEnvParams.m_displaySettings.m_disableTonemapping = false;


		// Mark as updated
		m_currAreaEnvUpdated = true;
	}
}

void CEnvironmentManager::UpdateCameraParams()
{

	// Get active camera
	const CCameraComponent *activeCamera = NULL;

#ifndef NO_EDITOR
	// Editor only stuff

	const CCameraComponent* localCamera = m_activeCamera.Get();
	if( localCamera != NULL )
	{
		activeCamera = localCamera;
	}

#endif

	// Override some environment params by camera
	if( m_world || activeCamera )
	{
		const SDofParams& cameraDofParams = activeCamera ? activeCamera->GetDofParams() : m_world->GetCameraDirector()->GetDofParams();

		if ( cameraDofParams.overrideFactor > 0.001f )
		{
			const Float importTime = GetCurrentGameTime().ToFloat() / (24.f * 60.f * 60.f);

			CEnvDepthOfFieldParameters dofParams;
			dofParams.m_activated = true;
			dofParams.m_nearBlurDist.Clear().AddPoint( 0.0f, SSimpleCurvePoint::BuildValueScalar( cameraDofParams.dofBlurDistNear ) );
			dofParams.m_nearFocusDist.Clear().AddPoint( 0.0f, SSimpleCurvePoint::BuildValueScalar( cameraDofParams.dofFocusDistNear ) );
			dofParams.m_farFocusDist.Clear().AddPoint( 0.0f, SSimpleCurvePoint::BuildValueScalar( cameraDofParams.dofFocusDistFar ) );
			dofParams.m_farBlurDist.Clear().AddPoint( 0.0f, SSimpleCurvePoint::BuildValueScalar( cameraDofParams.dofBlurDistFar ) );
			dofParams.m_intensity.Clear().AddPoint( 0.0f, SSimpleCurvePoint::BuildValueScalar( cameraDofParams.dofIntensity ) );
			m_currAreaEnvParams.m_depthOfField.ImportDayPointValue( m_currAreaEnvParams.m_depthOfField, dofParams, Clamp(cameraDofParams.overrideFactor, 0.f, 1.f), importTime );
		}
	}

	// Update game environment parameters

	m_gameEnvParams.m_cutsceneDofMode = m_gameEnvParams.m_displaySettings.m_forceCutsceneDofMode;
	if ( !m_gameEnvParams.m_cutsceneDofMode && ( m_world || activeCamera ) )
	{
		const SDofParams* cameraDofParams = nullptr;
		if( activeCamera )
		{
			cameraDofParams = &activeCamera->GetDofParams();
		}
		else if( m_world->GetCameraDirector()->GetNumCameras() > 0 )
		{
			cameraDofParams = &m_world->GetCameraDirector()->GetDofParams();
		}

		m_gameEnvParams.m_cutsceneDofMode = cameraDofParams && cameraDofParams->overrideFactor > 0.001f;
	}

	if ( GGame && ( GGame->IsPlayingCachetDialog() || GGame->IsPlayingCameraCutscene() ) )
	{
		m_gameEnvParams.m_cutsceneOrDialog = true;
	}
	else
	{
		m_gameEnvParams.m_cutsceneOrDialog = false;
	}
	
}

void CEnvironmentManager::UpdateBaseLightingParams()
{
	if ( m_world )
	{
		const SDayPointEnvironmentParams dayPointParams = GetDayPointEnvironmentParams();
		const CAreaEnvironmentParams &areaEnvParams = GetCurrentAreaEnvironmentParams();
		const CEnvGlobalLightParameters &globalLightParams = areaEnvParams.m_globalLight;

		// E3 DEMOHACK storm lightning
		if ( m_baseLightingParamsOverride )
		{
			m_baseLightingParams = *m_baseLightingParamsOverride;
			if ( m_baseLightingParams.m_lightDirection == Vector::ZEROS )
			{
				m_baseLightingParams.m_lightDirection = dayPointParams.m_globalLightDirection;
				m_baseLightingParams.m_sunDirection = dayPointParams.m_globalLightDirection;
				m_baseLightingParams.m_moonDirection = dayPointParams.m_globalLightDirection;
			}
			m_baseLightingParams.m_hdrAdaptationDisabled = m_HDRAdaptationDisabled;
		}
		else
		{
			m_baseLightingParams = CRenderBaseLightParams();

			// Setup sky/sun lighting
			m_baseLightingParams.m_lightDirection = dayPointParams.m_globalLightDirection;
			m_baseLightingParams.m_sunDirection = dayPointParams.m_sunDirection;
			m_baseLightingParams.m_moonDirection = dayPointParams.m_moonDirection;
			m_baseLightingParams.m_sunLightDiffuse = globalLightParams.m_sunColor.GetCachedPoint().GetColorScaledGammaToLinear( true );
			m_baseLightingParams.m_sunLightDiffuseLightSide = globalLightParams.m_sunColorLightSide.GetCachedPoint().GetColorScaledGammaToLinear( true );
			m_baseLightingParams.m_sunLightDiffuseLightOppositeSide = globalLightParams.m_sunColorLightOppositeSide.GetCachedPoint().GetColorScaledGammaToLinear( true );
			m_baseLightingParams.m_hdrAdaptationDisabled = m_HDRAdaptationDisabled;
		}
	}
}

void CEnvironmentManager::SetupAreaEnvironmentsForPosition( const Vector &pos, bool allowLocalEnvChanges, bool allowGlobalEnvChanges )
{
	if ( !allowLocalEnvChanges && !allowGlobalEnvChanges )
	{
		return;
	}

	if ( !GetWorld() )
	{
		return;
	}

	for ( WorldAttachedComponentsIterator it( GetWorld() ); it; ++it )
	{
		CAreaEnvironmentComponent *component = Cast< CAreaEnvironmentComponent > ( *it );
		if ( !component )
		{
			continue;
		}

		// Reject global area environments if requested
		if ( !allowLocalEnvChanges )
		{
			continue;
		}

		// Activate/deactivate
		if ( component->TestPointOverlap( pos ) )
		{
			component->Activate();
		}
		else
		{
			component->Deactivate();
		}
	}
}

void CEnvironmentManager::GetActiveAreaEnvironmentDefinitions( TDynArray< String >& defs )
{
	for ( TAreaEnvironmentsArray::iterator it = m_areaEnvironments.Begin(), itEnd = m_areaEnvironments.End(); it != itEnd; ++it )
	{
		defs.PushBack( CFilePath( it->pathToEnvDefinition ).GetFileName() );
	}
}

void CEnvironmentManager::LogState() const
{
	LOG_ENGINE(TXT("-------------------------------------- EnvMgr (totalEnv %i)"), (int)m_areaEnvironments.Size());
	for ( Uint32 i=0; i<m_areaEnvironments.Size(); ++i )
	{
		const SEnvManagerAreaEnvData& areaEnvironment = m_areaEnvironments[i];
		
		if ( areaEnvironment.areaComponent.IsValid() )
		{
			LOG_ENGINE( TXT("%i) Area Component in '%ls', id = %i, blend distance = %f, blend scale = %f"), (int)i, areaEnvironment.areaComponent.Get()->GetEntity()->GetName().AsChar(), areaEnvironment.id, areaEnvironment.areaComponent.Get()->GetBlendingDistance(), areaEnvironment.areaComponent.Get()->GetBlendingScale() );
		}
		else
		{
			LOG_ENGINE( TXT("%i) Global Environment, id = %i"), (int)i, areaEnvironment.id );
		}
	}
	LOG_ENGINE(TXT("-------------------------------------- /EnvMgr"));
}

void CEnvironmentManager::RebuildEnvironmentSlots()
{
	// Rebuild slots
	m_environmentSlots = 0;
	for ( Uint32 i=0; i<m_areaEnvironments.Size(); ++i )
	{
		if ( m_environmentSlots >= MAX_ENVIRONMENT_BLENDS )
		{
			ASSERT( !"Reached max number of simultaneous envs" );
			break;
		}

		const SEnvManagerAreaEnvData &currEnv = m_areaEnvironments[i];
		
		SEnvManagerPrioritySlot &slot = m_environmentSlot[ m_environmentSlots ];
		++m_environmentSlots;

		slot.environmentIndex = i;
		slot.priority = currEnv.priority;
		if ( currEnv.areaComponent.IsValid() )
		{
			slot.guid = currEnv.areaComponent->GetGUID();
		}
		else
		{
			slot.guid = CGUID::ZERO;
		}
	}

	// Sort slots with decreasing priority
	struct Local
	{
		static Bool Pred( const SEnvManagerPrioritySlot& e0, const SEnvManagerPrioritySlot& e1 )
		{
			// SORT_UTILS_VALIDATE_COMPARE_PREDICATE passes the same item twice which causes our
			// assert below that checks for GUIDs not being the same to fail
			if ( &e0 == &e1 )
			{
				return false;
			}

			if ( e0.priority > e1.priority )
			{
				return true;
			}

			if ( e0.priority == e1.priority )
			{
				ASSERT( e0.guid != e1.guid, TXT("Same GUIDs in environment manager slots, there might be unstable environment sorting!") );

				// to ensure sorting stability in case of the same priorities
				return e0.guid < e1.guid; 
			}

			return false;
		}		
	};
	Sort( m_environmentSlot, m_environmentSlot + m_environmentSlots, Local::Pred );
}

void CEnvironmentManager::UpdateTimeBasedEnvironmentBlending()
{
	Float time = GGame->GetEngineTime();

	for ( Int32 i=m_areaEnvironments.SizeInt() - 1; i>=0; --i )
	{
		SEnvManagerAreaEnvData& data = m_areaEnvironments[i];
		
		switch ( data.timeBlendState )
		{
		case ETBS_Disabled:
			data.timeBlendFactor = 1.0f;
			break;
		case ETBS_BlendIn:
		case ETBS_BlendOut:
			if ( time >= data.timeBlendTarget )
			{
				if ( data.timeBlendState == ETBS_BlendOut )
				{
					DeactivateEnvironment( data.id );
					continue;
				}
				data.timeBlendState = ETBS_Disabled;
				break;
			}

			data.timeBlendFactor = Clamp( ( time - data.timeBlendStart )/( data.timeBlendTarget - data.timeBlendStart ), 0.0f, 1.0f );

			// Invert blend factor for blendout
			if ( data.timeBlendState == ETBS_BlendOut )
			{
				data.timeBlendFactor = 1.0f - data.timeBlendFactor;
			}

			break;
		}
	}
}

// Calculates the contribution and blend factor for a single area for the passed position
static void CalculateEnvironmentAreaContribution( const Vector& position, const Vector& direction, CAreaEnvironmentComponent* areaComponent, Float distanceFromEdge, Float time, CAreaEnvironmentParams &outResult, Float& blendLerp )
{
	// Get blending distance parameter
	const Float blendingDistance = areaComponent->GetBlendingDistance();

	// Multiply blending with the specified blending distance
	if ( blendingDistance > 0.0001f )
	{
		// Blend from the area's edges
		blendLerp = Clamp( distanceFromEdge/blendingDistance, 0.0f, 1.0f );

		// Blend from above and below
		if ( areaComponent->GetBlendAboveAndBelow() )
		{
			// NOTE: this will not work with areas which do not have their upper/lower parths aligned to XY plane
			const Box& box = areaComponent->GetBoundingBox();
			Float distanceFromCaps = Min(box.Max.Z - position.Z, position.Z - box.Min.Z);
			blendLerp *= Clamp( distanceFromCaps/blendingDistance, 0.0f, 1.0f );
		}

		// For areas only below the terrain, also blend distance from terrain
		if ( areaComponent->GetTerrainSide() == ATS_OnlyBelowTerrain )
		{
			const Float terrainBlendingDistance = areaComponent->GetTerrainBlendingDistance();
			if ( terrainBlendingDistance > 0.0001f )
			{
				// Find height at position
				CWorld* world = areaComponent->GetLayer() ? areaComponent->GetLayer()->GetWorld() : nullptr;
				CClipMap* clipmap = world ? world->GetTerrain() : nullptr;
				Float height;
				clipmap->GetHeightForWorldPosition( position, height );

				blendLerp *= Clamp( (height - position.Z)/terrainBlendingDistance, 0.0f, 1.0f );
			}
		}
	}
	else // No blending distance specified, always use 1.0f here (might be affected below)
	{
		blendLerp = 1.0f;
	}

	// Pass area's environment parameters
	outResult = *areaComponent->GetParameters();

	// Apply points
	const TDynArray< SAreaEnvironmentPoint >& points = areaComponent->GetPoints();
	for ( Uint32 i=0; i < points.Size(); ++i )
	{
		// Calculate point contribution
		const SAreaEnvironmentPoint& point = points[i];
		Vector pointPosition = areaComponent->GetWorldPositionRef() + point.m_position;
		Vector positionInPointSpace = ( position - pointPosition )/Vector( point.m_scaleX, point.m_scaleY, point.m_scaleZ ) + pointPosition;
		Float distanceFromPoint = positionInPointSpace.DistanceTo( pointPosition );
		Float pointContribution = Clamp( Clamp( 1.0f - ( distanceFromPoint < point.m_innerRadius ? 0.0f : (distanceFromPoint - point.m_innerRadius)/( point.m_outerRadius - point.m_innerRadius ) ), 0.0f, 1.0f ) * point.m_blendScale, 0.0f, 1.0f );

		// Apply curve
		if ( point.m_useCurve )
		{
			pointContribution = point.m_curve.GetValue( pointContribution ).W;
		}

		// Apply blend
		switch ( point.m_blend )
		{
		case AEPB_DistanceOnly:
			break;
		case AEPB_CameraFocusAndDistance:
			{
				Vector pointToCameraNormal = ( positionInPointSpace - pointPosition ).Normalized3();
				Float angleBlend = Max( 0.0f, -direction.Dot3( pointToCameraNormal ) );
				pointContribution *= angleBlend;
			}
			break;
		case AEPB_CameraAngleAndDistance:
			{
				Vector pointToCameraNormal = ( positionInPointSpace - pointPosition ).Normalized3();
				Float angleBlend = Max( 0.0f, -direction.Dot3( pointToCameraNormal ) );
				angleBlend *= Max( 0.0f, -direction.Dot3( point.m_direction.TransformVector( Vector( 0, 1, 0 ) ) ) );
				pointContribution *= angleBlend;
			}
			break;
		}

		// Apply point
		switch ( point.m_type )
		{
		case AEPT_FadeOut:
			blendLerp *= 1.0f - pointContribution;
			break;
		case AEPT_FadeIn:
			blendLerp *= pointContribution;
			break;
		case AEPT_Additive:
			blendLerp += pointContribution;
			break;
		case AEPT_Subtractive:
			blendLerp -= pointContribution;
			break;
		case AEPT_SubEnvironment:
			// Apply sub-environment definition
			if ( point.m_environmentDefinition && pointContribution > 0.01f )
			{
				outResult.ImportDayPointValue( outResult, point.m_environmentDefinition->GetAreaEnvironmentParams(), pointContribution, time );
			}
			break;
		}
	}
	
	// Apply blending curve (if enabled)
	if ( areaComponent->GetBlendingCurveEnabled() )
	{
		blendLerp = areaComponent->GetBlendingCurve().GetValue( blendLerp ).W;
	}

	blendLerp = Clamp( blendLerp, 0.0f, 1.0f );
}

void CEnvironmentManager::CalculateEnvironmentForGivenPositionAndTime( const Vector& position, Float time, CAreaEnvironmentParams& outResult )
{
	const Vector direction = GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetCameraForward() : Vector::ZERO_3D_POINT;

	m_environmentBlends = 0;

#ifndef NO_DEBUG_WINDOWS
	// Reset applied blend factor
	for ( Uint32 i=0; i<m_areaEnvironments.Size(); ++i )
	{
		m_areaEnvironments[i].appliedBlendFactor = 0;
		m_areaEnvironments[i].appliedMostImportantFactor = -1;
	}
#endif

	// Blend all environment slots
	Int32 envBlendsSourceEnvIndices[ MAX_ENVIRONMENT_BLENDS ];
	for ( Uint32 i=0; i<m_environmentSlots; ++i )
	{
		const SEnvManagerPrioritySlot& slot = m_environmentSlot[i];

		// Figure out contributions from all environments with given priority
		Float totalDistanceFromAllAreas = 0.0f;
		Float areaEnvironmentCount = 0.0f;
		{
			Uint32 i0 = i;
			while ( i0 > 0 && m_environmentSlot[i0-1].priority == m_environmentSlot[i0].priority )
			{
				--i0;
			}

			Uint32 i1 = i+1;
			while ( i1 < m_environmentSlots && m_environmentSlot[i1-1].priority == m_environmentSlot[i1].priority )
			{
				++i1;
			}

			for ( Uint32 i=i0; i<i1; ++i )
			{
				SEnvManagerAreaEnvData &currData = m_areaEnvironments[ m_environmentSlot[i].environmentIndex ];
				CAreaEnvironmentComponent* areaComponent = currData.areaComponent.IsValid() ? currData.areaComponent.Get() : nullptr;
				if ( areaComponent )
				{
					currData.distanceFromEdge = areaComponent->CalcDistToClosestEdge2D( position );
					totalDistanceFromAllAreas += currData.distanceFromEdge;
					areaEnvironmentCount++;
				}
			}
		}

		// Blend environments from area components
		{
			const Uint32 dataAreaEnvIndex = m_environmentSlot[i].environmentIndex;
			SEnvManagerAreaEnvData& data = m_areaEnvironments[ dataAreaEnvIndex ];

			// If the environment area is blending out, use the stored original blend factor
			if ( data.timeBlendState == ETBS_BlendOut )
			{
				// Reuse the original blend factor
				envBlendsSourceEnvIndices[ m_environmentBlends ] = dataAreaEnvIndex;
				m_environmentBlend[m_environmentBlends].blendFactor = data.originalBlendFactor * data.timeBlendFactor;
				m_environmentBlend[m_environmentBlends].blendFactor = Clamp( m_environmentBlend[m_environmentBlends].blendFactor, 0.0f, 1.f );

				// Copy the environment params (it was changed by hierarchical blending calculations)
				m_environmentBlend[m_environmentBlends].params = data.areaEnv;

				//
				++m_environmentBlends;
			}
			else // otherwise calculate a new blend factor
			{
				CAreaEnvironmentComponent* areaComponent = data.areaComponent.IsValid() ? data.areaComponent.Get() : nullptr;
				// Blend area
				if ( areaComponent )
				{
					Float distanceContribution;
					
					m_environmentBlend[m_environmentBlends].params.Reset( EnvResetMode_CurvesEmpty );		// Avoids realloc by doing clearFast on data internally, otherwise this does too many allocs per frame

					// Get area's contribution
					CalculateEnvironmentAreaContribution( position, direction, areaComponent, data.distanceFromEdge, time, m_environmentBlend[m_environmentBlends].params, m_environmentBlend[m_environmentBlends].blendFactor );

					// Import values
					envBlendsSourceEnvIndices[ m_environmentBlends ] = dataAreaEnvIndex;

					// Calculate blend factor for this environment blend
					distanceContribution = data.distanceFromEdge < areaComponent->GetBlendingDistance() ? data.distanceFromEdge / totalDistanceFromAllAreas : 1.0f;
					m_environmentBlend[m_environmentBlends].blendFactor *= distanceContribution;
					m_environmentBlend[m_environmentBlends].blendFactor *= areaComponent->GetBlendingScale();
					data.originalBlendFactor = m_environmentBlend[m_environmentBlends].blendFactor; // store the un-timed blend factor for blendout use
					m_environmentBlend[m_environmentBlends].blendFactor *= data.timeBlendFactor;
					m_environmentBlend[m_environmentBlends].blendFactor = Clamp( m_environmentBlend[m_environmentBlends].blendFactor, 0.0f, 1.f );

					//
					++m_environmentBlends;
				}
				else // Found a global environment
				{
					// Reset blend params to empty so that curves will always get a single point (needed for CAreaEnvironmentParamsAtPoint conversion)
					envBlendsSourceEnvIndices[ m_environmentBlends ] = dataAreaEnvIndex;
					m_environmentBlend[m_environmentBlends].params = data.areaEnv;

					// Set blend factor from passed parameters
					data.originalBlendFactor = data.blendFactor;
					m_environmentBlend[m_environmentBlends].blendFactor = data.blendFactor * data.timeBlendFactor;

					//
					++m_environmentBlends;
				}
			}
		}
	}

	// Perform the actual blending
	Float effectiveBlendFactors[ MAX_ENVIRONMENT_BLENDS ];
	for ( Uint32 i=0; i<m_environmentBlends; ++i )
	{
		effectiveBlendFactors[i] = 1.f;
	}

	if ( m_environmentBlends > 0 )
	{	
		// Build environment with least priority
		m_environmentBlend[ m_environmentBlends - 1 ].params.ImportDayPointValue( m_defaultCurvesParams, m_environmentBlend[ m_environmentBlends - 1 ].params, 1.f, time );

		// Build the rest of environments
		for ( Int32 i=(Int32)m_environmentBlends-2; i>=0; --i )
		{
			m_environmentBlend[i].params.ImportDayPointValue( m_environmentBlend[i+1].params, m_environmentBlend[i].params, m_environmentBlend[i].blendFactor, time );
			for ( Uint32 j=i+1; j<m_environmentBlends; ++j )
			{
				effectiveBlendFactors[j] *= 1.f - m_environmentBlend[i].blendFactor;
			}
			effectiveBlendFactors[i] *= m_environmentBlend[i].blendFactor;
		}
	}

	// Find the two most important contributing environment blending slots
	m_mostImportantEnvironment = MAX_ENVIRONMENT_BLENDS;
	m_secondMostImportantEnvironment = MAX_ENVIRONMENT_BLENDS;

	if ( m_environmentBlends > 1 )
	{
		// Setup indices
		struct IndexInfo
		{
			int index;
			Float blendFactor;
		};
		IndexInfo indices[ MAX_ENVIRONMENT_BLENDS ];
		for ( Uint32 i=0; i<m_environmentBlends; ++i )
		{
			indices[i].index = i;
			indices[i].blendFactor = effectiveBlendFactors[i];
		}

		// Sort indices
		struct Local
		{
			static Bool Pred( const IndexInfo& e0, const IndexInfo& e1 )
			{			
				if ( e0.blendFactor > e1.blendFactor )
				{
					return true;
				}

				if ( e0.blendFactor == e1.blendFactor )
				{
					return e0.index > e1.index;
				}

				return false;
			}		
		};
		Sort( indices, indices + m_environmentBlends, Local::Pred );

		if ( 2 == m_environmentBlends || 0 == indices[0].blendFactor || 0 == indices[1].blendFactor )
		{
			Float f0 = indices[0].blendFactor;
			Float f1 = indices[1].blendFactor;
			if ( 0 == f0 || 0 == f1 )
			{
				m_mostImportantEnvironment = m_secondMostImportantEnvironment = 0;
				m_mostImportantBlendFactor = 0;
			}
			else
			{
				m_mostImportantEnvironment = indices[0].index;
				m_secondMostImportantEnvironment = indices[1].index;
				m_mostImportantBlendFactor = Clamp( f1 / Max(0.0001f, f0 + f1), 0.f, 1.f );
			}
		}
		else
		{			
			Float f0 = indices[0].blendFactor;
			Float f1 = indices[1].blendFactor;
			Float f2 = indices[2].blendFactor;
			RED_ASSERT( f0 >= f1 && f1 >= f2 && f1 > 0 && f2 >= 0 );
			{
				Float invMul = 1.f / Max( 0.0001f, f0 + f1 + f2 );
				f0 *= invMul;
				f1 *= invMul;
				f2 *= invMul;
			}

			f0 -= f2;
			f1 -= f2;
			{
				Float invMul = 1.f / Max( 0.0001f, f0 + f1 );
				f0 *= invMul;
				f1 *= invMul;
				f2  = invMul;
			}

			m_mostImportantEnvironment = indices[0].index;
			m_secondMostImportantEnvironment = indices[1].index;
			m_mostImportantBlendFactor = Clamp( f1 / Max(0.0001f, f0 + f1), 0.f, 1.f );
		}
	}
	else if ( 1 == m_environmentBlends )
	{
		m_mostImportantEnvironment = m_secondMostImportantEnvironment = m_environmentBlends - 1;
		m_mostImportantBlendFactor = 0;
	}
	else
	{
		m_mostImportantEnvironment = 0;
		m_secondMostImportantEnvironment = 0;
		m_mostImportantBlendFactor = 0;
	}

#ifndef NO_DEBUG_WINDOWS
	if ( m_mostImportantBlendFactor > 0 )
	{
		if ( m_mostImportantEnvironment < MAX_ENVIRONMENT_BLENDS )
		{
			Int32 idx = envBlendsSourceEnvIndices[ m_mostImportantEnvironment ];
			if ( idx >= 0 && idx < (Int32)m_areaEnvironments.Size() )
			{
				m_areaEnvironments[ idx ].appliedMostImportantFactor = 1.f - m_mostImportantBlendFactor;
			}
		}

		if ( m_secondMostImportantEnvironment < MAX_ENVIRONMENT_BLENDS )
		{
			Int32 idx = envBlendsSourceEnvIndices[ m_secondMostImportantEnvironment ];
			if ( idx >= 0 && idx < (Int32)m_areaEnvironments.Size() )
			{
				m_areaEnvironments[ idx ].appliedMostImportantFactor = m_mostImportantBlendFactor;
			}
		}
	}
	for ( Uint32 i=0; i<m_environmentBlends; ++i )
	{
		Int32 idx = envBlendsSourceEnvIndices[ i ];
		if ( idx >= 0 && idx < (Int32)m_areaEnvironments.Size() )
		{
			m_areaEnvironments[idx].appliedBlendFactor = effectiveBlendFactors[i];
		}
	}
#endif

	// Output the result
	outResult = m_environmentBlends > 0 ? m_environmentBlend[0].params : m_defaultCurvesParams;
}

void CEnvironmentManager::ApplyWeatherModifications( CAreaEnvironmentParams& params, Float time )
{
	// Apply weather modifications	
	Float currTime = GGame->GetEngineTime();

	if ( m_balanceOverride )
	{
		params.m_finalColorBalance.m_parametricBalanceLow.m_color.SetValue( (Uint32)(0), m_balanceOverride->m_parametricBalanceLowOverride );
		params.m_finalColorBalance.m_parametricBalanceMid.m_color.SetValue( (Uint32)(0), m_balanceOverride->m_parametricBalanceMidOverride );
		params.m_finalColorBalance.m_parametricBalanceHigh.m_color.SetValue( (Uint32)(0), m_balanceOverride->m_parametricBalanceHighOverride );
	}
	// -

	// sound setup

	GSoundSystem->SoundGlobalParameter( "wind_intensity",  m_weatherManager->GetWindScale() );
	GSoundSystem->SoundGlobalParameter( "rain_intensity",  m_weatherManager->GetEffectStrength( EWeatherEffectType::WET_RAIN ) );
	GSoundSystem->SoundGlobalParameter( "time",  time );
}

Vector CEnvironmentManager::GetCurrentWindVector( const Vector& point )
{
	if( m_weatherManager )
	{
		return m_weatherManager->GetCurrentWindVector( point );
	}
	else
		return Vector::ZEROS;
}
Vector CEnvironmentManager::GetCurrentWindVector()
{
	if( m_weatherManager )
	{
		return m_weatherManager->GetCurrentWindVector();
	}
	else
		return Vector::ZEROS;
}

Float CEnvironmentManager::GetWindScale()
{
	if( m_weatherManager )
	{
		return m_weatherManager->GetWindScale();
	}
	else
		return 0.0f;
}

void CEnvironmentManager::Tick_Single( float timeDelta )
{
	// Tick the current weather state
	m_weatherManager->Tick( timeDelta, GGame->GetTimeManager()->GetHoursPerMinute() );
}

void CEnvironmentManager::Tick_Parallel( float timeDelta )
{
	// Activate global env
	FindAndActivateGlobalEnv();

	// Update scenes environment
	UpdateScenesEnvironment();
		
	// Override wind parameters	
	if( m_currAreaEnvParams.m_windParams.m_activated ) 
		UpdateWindParameters( m_currAreaEnvParams.m_windParams.m_cloudsVelocityOverride.GetCachedPoint().GetScalar(), m_currAreaEnvParams.m_windParams.m_windStrengthOverride.GetCachedPoint().GetScalar() );
	// Update with default wind params
	else
		UpdateWindParameters( 0.01f, 1.0f );

	// What it says on the tin: Update time-based environment blending
	UpdateTimeBasedEnvironmentBlending();

	// Update cached area env if needed
	UpdateCurrentAreaEnvironment();

	// Update the base lighting params
	UpdateBaseLightingParams();
	
	// Update selected postprocess effects
#ifndef NO_EDITOR
	m_radialBlurManager.GenerateFinalParams( m_gameEnvParams.m_radialBlur, m_activeCamera.Get() );
	m_lightShaftManager.GenerateFinalParams( m_gameEnvParams.m_lightShaft, m_activeCamera.Get() );
#else
	m_radialBlurManager.GenerateFinalParams( m_gameEnvParams.m_radialBlur );
	m_lightShaftManager.GenerateFinalParams( m_gameEnvParams.m_lightShaft );
#endif
	m_brightnessTintManager.GenerateFinalParams( m_gameEnvParams.m_brightnessTint );
}

void CEnvironmentManager::Tick_TeleportFixup()
{
	m_currAreaEnvUpdated = false;

	// Update cached area env if needed
	UpdateCurrentAreaEnvironment();

	// Update the base lighting params
	UpdateBaseLightingParams();

	// Update selected postprocess effects
#ifndef NO_EDITOR
	m_radialBlurManager.GenerateFinalParams( m_gameEnvParams.m_radialBlur, m_activeCamera.Get() );
	m_lightShaftManager.GenerateFinalParams( m_gameEnvParams.m_lightShaft, m_activeCamera.Get() );
#else
	m_radialBlurManager.GenerateFinalParams( m_gameEnvParams.m_radialBlur );
	m_lightShaftManager.GenerateFinalParams( m_gameEnvParams.m_lightShaft );
#endif
	m_brightnessTintManager.GenerateFinalParams( m_gameEnvParams.m_brightnessTint );
}

void CEnvironmentManager::SerializeForGC( IFile& file )
{
	// Serialize environment area params
	for ( Uint32 i=0; i < m_environmentBlends; ++i )
	{
		m_environmentBlend[i].params.GetClass()->SerializeGC( file, &m_environmentBlend[i].params );
	}
	for ( auto it=m_areaEnvironments.Begin(); it != m_areaEnvironments.End(); ++it )
	{
		(*it).areaEnv.GetClass()->SerializeGC( file, &(*it).areaEnv );
		if ( (*it).areaComponent.IsValid() )
		{
			CComponent* cmp = (*it).areaComponent.Get();
			file << cmp;
		}
	}
	m_currAreaEnvParams.GetClass()->SerializeGC( file, &m_currAreaEnvParams );

	// Ask the weather manager for its own objects
	m_weatherManager->SerializeForGC( file );
}

void CEnvironmentManager::FindAndActivateGlobalEnv()
{
	if ( m_world->GetEnvironmentParameters().m_environmentDefinition )
	{
		m_globalId = ActivateAreaEnvironment( m_world->GetEnvironmentParameters().m_environmentDefinition.Get(), NULL, 0, 1.0f, 0.0f, m_globalId );
	}
	else if ( -1 != m_globalId )
	{
		DeactivateEnvironment( m_globalId );
		m_globalId = -1;
	}

	if ( m_world->GetEnvironmentParameters().m_scenesEnvironmentDefinition )
	{
		Float scenesBlendFactor = m_cameraLightsModifiers.m_scenesSystemActiveFactor;
		if ( -1 != m_scenesId )
		{
			Int32 scenesDataIndex = FindAreaEnvById( m_scenesId );
			if ( -1 != scenesDataIndex )
			{
				scenesBlendFactor = m_areaEnvironments[scenesDataIndex].blendFactor;
			}
		}

		m_scenesId = ActivateAreaEnvironment( m_world->GetEnvironmentParameters().m_scenesEnvironmentDefinition.Get(), NULL, ScenesEnvironmentPriority, scenesBlendFactor, 0.0f, m_scenesId );		
	}
	else if ( -1 != m_scenesId )
	{
		DeactivateEnvironment( m_scenesId );
		m_scenesId = -1;
	}
}

void CEnvironmentManager::SetGameEnvironmentParams( const CGameEnvironmentParams &params )
{
	m_gameEnvParams = params;
}

void CEnvironmentManager::SetInstantAdaptationTrigger( Bool enable )
{
	if ( enable && !m_instantAdaptationTrigger )
	{
		UpdateCurrentAreaEnvironment();
	}

	m_instantAdaptationTrigger = enable;
}

void CEnvironmentManager::SetInstantDissolveTrigger( Bool enable )
{
	m_instantDissolveTrigger = enable;
}

Float CEnvironmentManager::GetNearPlane() const
{
	return GetWorldRenderSettings().m_cameraNearPlane;
}

void CEnvironmentManager::UpdateScenesEnvironment()
{
	if ( -1 == m_scenesId )
	{
		return;
	}

	const Float scenesBlendFactor = Clamp( m_cameraLightsModifiers.m_scenesSystemActiveFactor, 0.f, 1.f );
	ChangeAreaEnvironmentBlendFactor( m_scenesId, scenesBlendFactor );
}

void CEnvironmentManager::SetCameraLightsModifiers( const SCameraLightsModifiersSetup &newValue ) 
{ 
	m_cameraLightsModifiers = newValue;

	UpdateScenesEnvironment();
}

void CEnvironmentManager::GenerateGlobalLightingTrajectoryFragments( CRenderFrame* frame )
{
	// --------------------------------
	struct PassInfo
	{
		Vector (CGlobalLightingTrajectory::*func)( const GameTime & ) const;
		Color color;
	};
	// --------------------------------

	const CGlobalLightingTrajectory& trajectory = m_world->GetEnvironmentParameters().m_globalLightingTrajectory;

	PassInfo passes[] =
	{
		{ &CGlobalLightingTrajectory::GetMoonDirection,		Color::BLUE		},
		{ &CGlobalLightingTrajectory::GetSunDirection,		Color::YELLOW	},
		{ &CGlobalLightingTrajectory::GetLightDirection,	Color::WHITE	},
		{ nullptr }
	};

	const Vector displayOrigin	= frame->GetFrameInfo().m_camera.GetPosition();
	const Float  displayRadius	= 50.f;
	const Float  sphereRadius	= 0.025f * displayRadius;

	TDynArray< DebugVertex > lines;

	for ( Int32 pass_i=0; nullptr!=passes[pass_i].func; ++pass_i )
	{
		Vector prevDir;
		for ( Int32 second=0; second<=24*60*60; second+=30 )
		{
			Vector currDir = (trajectory.*passes[pass_i].func)( second );
			if ( second > 0 )
			{
				lines.PushBack( DebugVertex (displayOrigin + prevDir * displayRadius, passes[pass_i].color ) );
				lines.PushBack( DebugVertex (displayOrigin + currDir * displayRadius, passes[pass_i].color ) );
			}
			prevDir = currDir;
		}

		Vector currDir		= (trajectory.*passes[pass_i].func)(GetCurrentGameTime());
		Vector sphereCenter	= displayOrigin + currDir * displayRadius;
		frame->AddDebugSphere( sphereCenter, sphereRadius, Matrix::IDENTITY, passes[pass_i].color, true );
	}

	if ( !lines.Empty() )
	{
		frame->AddDebugLines( &lines[0], lines.Size(), true );
	}
}

void CEnvironmentManager::GenerateEditorFragments( CRenderFrame* frame )
{
	if ( m_gameEnvParams.m_displaySettings.m_enableGlobalLightingTrajectory )
	{
		GenerateGlobalLightingTrajectoryFragments( frame );
	}
	
	// Draw wind direction if we have wind
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_Wind ) && m_world )
	{	
		const Uint32 fWidth = frame->GetFrameInfo().m_width;
		const Uint32 fHeight = frame->GetFrameInfo().m_height;
		Vector windDir = m_world->GetWindAtPointForVisuals( frame->GetFrameInfo().m_camera.GetPosition(), false );

		// The indicator should cover approximately 20% of each viewport dimension. We use Min to keep it uniform, and avoid clipping with a very tall or wide viewport.
		Uint32 pxSize = (Uint32)( (Float) Min( fWidth, fHeight ) * 0.2f );

		// Position it in the top-right corner of the viewport.
		Uint32 pxX = fWidth - pxSize / 2;
		Uint32 pxY = fHeight - pxSize / 2;

		Float x = (Float)pxX / (Float)fWidth * 2.0f - 1.0f;
		Float y = (Float)pxY / (Float)fHeight * 2.0f - 1.0f;

		Vector center_c( x, y, 0.5f, 1.0f );
		Vector edge_c( 1.0f, y, 0.5f, 1.0f );
		Vector center_w = frame->GetFrameInfo().m_camera.GetScreenToWorld().TransformVectorWithW( center_c );
		Vector edge_w = frame->GetFrameInfo().m_camera.GetScreenToWorld().TransformVectorWithW( edge_c );
		center_w /= center_w.W;
		edge_w /= edge_w.W;

		Float scale = edge_w.DistanceTo( center_w ) * 0.5f;			

		// And draw an arrow in the direction of the wind.
		frame->AddDebug3DArrow( center_w, windDir, scale, 0.05f * scale, 0.1f * scale, 0.25f * scale, Color::WHITE, Color::MAGENTA );

		// Finally... a label to pull it all together...
		frame->AddDebugScreenFormatedText( pxX - pxSize / 2, 50, Color::WHITE, TXT("Wind Direction:") );		
	}
}

const CAreaEnvironmentParams& CEnvironmentManager::GetCurrentAreaEnvironmentParams() const
{
	return m_currAreaEnvParams;
}

void CEnvironmentManager::GetMostImportantEnvironments( CAreaEnvironmentParams** first, CAreaEnvironmentParams** second, Float& blendFactor )
{
	if ( m_mostImportantBlendFactor > 0.f && m_mostImportantEnvironment < MAX_ENVIRONMENT_BLENDS && m_secondMostImportantEnvironment < MAX_ENVIRONMENT_BLENDS )
	{
		*first = &m_environmentBlend[m_mostImportantEnvironment].params;
		*second = &m_environmentBlend[m_secondMostImportantEnvironment].params;
		blendFactor = m_mostImportantBlendFactor;
	}
	else
	{
		first = second = NULL;
		blendFactor = 0.0f;
	}
}

void CEnvironmentManager::UpdateGameplayCameraLightsFactor( Bool isInterior, Float time, const Vector &cameraPosition, Bool forceInstant )
{
	const Float cameraDistanceThreshold = 4.f;
	const Bool reset = forceInstant || -1 == m_lastGameplayCameraLightsTime || m_lastGameplayCameraLightsCamPos.DistanceTo( cameraPosition ) >= cameraDistanceThreshold;

	if ( reset )
	{
		m_lastGameplayCameraLightsFactor = isInterior ? 0.f : 1.f;
	}
	else
	{
		m_lastGameplayCameraLightsFactor = GetGameplayCameraLightsFactor( time );
	}

	m_lastGameplayCameraInterior = isInterior;
	m_lastGameplayCameraLightsCamPos = cameraPosition;
	m_lastGameplayCameraLightsTime = time;
}

Float CEnvironmentManager::GetGameplayCameraLightsFactor( Float time ) const
{
	if ( -1 == m_lastGameplayCameraLightsTime )
	{
		return m_lastGameplayCameraLightsFactor;
	}

	RED_ASSERT( time >= m_lastGameplayCameraLightsTime );
	const Float elapsed = Max( 0.f, time - m_lastGameplayCameraLightsTime );
	const Float fadeTime = m_lastGameplayCameraInterior ? Config::cvGameplayCameraLightFadeToInterior.Get() : Config::cvGameplayCameraLightFadeToExterior.Get();

	if ( fadeTime <= 0.001f )
	{
		return m_lastGameplayCameraInterior ? 0.f : 1.f;
	}
	
	const Float fadeSpeed = (m_lastGameplayCameraInterior ? -1.f : 1.f) / fadeTime;
	return Clamp( m_lastGameplayCameraLightsFactor + elapsed * fadeSpeed, 0.f, 1.f );
}

const SWorldRenderSettings& CEnvironmentManager::GetWorldRenderSettings() const
{
	const SWorldEnvironmentParameters& envParams = m_world->GetEnvironmentParameters();
	return envParams.m_renderSettings;
}

const SDayPointEnvironmentParams CEnvironmentManager::GetDayPointEnvironmentParams() const
{
	SDayPointEnvironmentParams dayPointParams;
	dayPointParams.m_gamma						= GetGameEnvironmentParams().m_displaySettings.m_gamma;

	// Import world environment parameters

	const SWorldEnvironmentParameters& envParams = m_world->GetEnvironmentParameters();
	dayPointParams.m_toneMappingAdaptationSpeedUp	= envParams.m_toneMappingAdaptationSpeedUp;
	dayPointParams.m_toneMappingAdaptationSpeedDown	= envParams.m_toneMappingAdaptationSpeedDown;
	
	// use clouds shadows from the skybox material
	{
		THandle< CTextureArray > cloudsTextureArray;	
		CMaterialInstance *material = envParams.m_skybox.m_cloudsMaterial.Get();
		if( material && material->ReadParameter( CNAME(DiffuseMap), cloudsTextureArray ) ) 
		{
			dayPointParams.m_cloudsShadowTexture = cloudsTextureArray->GetRenderResource();
		}
		dayPointParams.m_fakeCloudsShadowSize = envParams.m_renderSettings.m_fakeCloudsShadowSize;
		dayPointParams.m_fakeCloudsShadowSpeed = envParams.m_renderSettings.m_fakeCloudsShadowSpeed;
	}
	
	dayPointParams.m_cameraDirtTexture				= envParams.m_cameraDirtTexture.Get();
	dayPointParams.m_cameraDirtNumVerticalTiles		= envParams.m_cameraDirtNumVerticalTiles;

	dayPointParams.m_vignetteTexture				= envParams.m_vignetteTexture.Get();

	dayPointParams.m_interiorFallbackAmbientTexture = envParams.m_interiorFallbackAmbientTexture.Get();
	dayPointParams.m_interiorFallbackReflectionTexture = envParams.m_interiorFallbackReflectionTexture.Get();
		
	dayPointParams.m_skyDayAmount					= 1.f;
		
	const Float importTime = GetCurrentGameTime().ToFloat() / (24.f * 60.f * 60.f);
	dayPointParams.m_skyDayAmount = envParams.m_globalLightingTrajectory.GetSkyDayAmountCurve().GetFloatValue( importTime );

	// Calculate light direction for current time
	const CGlobalLightingTrajectory& lightTrajectory = envParams.m_globalLightingTrajectory;
	if ( !GetGameEnvironmentParams().m_dayCycleOverride.m_enableCustomSunRotation )
	{
		dayPointParams.m_globalLightDirection = lightTrajectory.GetLightDirection( GetCurrentGameTime() );				
		dayPointParams.m_sunDirection = lightTrajectory.GetSunDirection( GetCurrentGameTime() );				
		dayPointParams.m_moonDirection = lightTrajectory.GetMoonDirection( GetCurrentGameTime() );	
	}
	else
	{
		dayPointParams.m_globalLightDirection = GetGameEnvironmentParams().m_dayCycleOverride.GetCustomSunDirection();
		dayPointParams.m_sunDirection = GetGameEnvironmentParams().m_dayCycleOverride.GetCustomSunDirection();		
		dayPointParams.m_moonDirection = GetGameEnvironmentParams().m_dayCycleOverride.GetCustomSunDirection();
	}

	// Shafts override
	dayPointParams.m_useMoonForShafts = lightTrajectory.AreMoonShaftsEnabled( GetCurrentGameTime() );

	// Optionally override light direction by area environment

	if ( m_currAreaEnvParams.m_globalLight.m_activatedFactorLightDir > 0.f )
	{	
		Vector curr_dir		= dayPointParams.m_globalLightDirection;
		Vector curr_sunDir	= dayPointParams.m_sunDirection;
		Vector curr_moonDir	= dayPointParams.m_moonDirection;
		
		const EulerAngles forcedLightDir = EulerAngles( 
			m_currAreaEnvParams.m_globalLight.m_forcedLightDirAnglesRoll.GetCachedPoint().GetScalar(), 
			m_currAreaEnvParams.m_globalLight.m_forcedLightDirAnglesPitch.GetCachedPoint().GetScalar(), 
			m_currAreaEnvParams.m_globalLight.m_forcedLightDirAnglesYaw.GetCachedPoint().GetScalar() );

		const EulerAngles forcedSunDir = EulerAngles( 
			m_currAreaEnvParams.m_globalLight.m_forcedSunDirAnglesRoll.GetCachedPoint().GetScalar(), 
			m_currAreaEnvParams.m_globalLight.m_forcedSunDirAnglesPitch.GetCachedPoint().GetScalar(), 
			m_currAreaEnvParams.m_globalLight.m_forcedSunDirAnglesYaw.GetCachedPoint().GetScalar() );

		const EulerAngles forcedMoonDir = EulerAngles( 
			m_currAreaEnvParams.m_globalLight.m_forcedMoonDirAnglesRoll.GetCachedPoint().GetScalar(), 
			m_currAreaEnvParams.m_globalLight.m_forcedMoonDirAnglesPitch.GetCachedPoint().GetScalar(), 
			m_currAreaEnvParams.m_globalLight.m_forcedMoonDirAnglesYaw.GetCachedPoint().GetScalar() );

		Vector forced_dir	= forcedLightDir.ToMatrix().GetAxisY();
		Vector forced_sunDir= forcedSunDir.ToMatrix().GetAxisY();
		Vector forced_moonDir=forcedMoonDir.ToMatrix().GetAxisY();

		// ace_todo: slerp
		Vector new_dir		= Lerp( Min( 1.f, m_currAreaEnvParams.m_globalLight.m_activatedFactorLightDir ), curr_dir, forced_dir );
		Float  len			= new_dir.Mag3();
		new_dir				= len>0.005f ? new_dir/len : Vector (0,0,-1);

		Vector new_SunDir	= Lerp( Min( 1.f, m_currAreaEnvParams.m_globalLight.m_activatedFactorLightDir ), curr_sunDir, forced_sunDir );
		Float  len_sun		= new_SunDir.Mag3();
		new_SunDir			= len_sun>0.005f ? new_SunDir/len_sun : Vector (0,0,-1);

		Vector new_moonDir	= Lerp( Min( 1.f, m_currAreaEnvParams.m_globalLight.m_activatedFactorLightDir ), curr_moonDir, forced_moonDir );
		Float  len_moon		= new_moonDir.Mag3();
		new_moonDir			= len_moon>0.005f ? new_moonDir/len_moon : Vector (0,0,-1);
		
		dayPointParams.m_globalLightDirection = new_dir;		
		dayPointParams.m_moonDirection = new_moonDir;
		dayPointParams.m_sunDirection = new_SunDir;
	}
		
	dayPointParams.m_windParameters = m_weatherManager->GetCurrentWindParameters();
	
	// Calculate clouds shadow offset
	//dayPointParams. = m_cloudsShadowOffset;
	
	m_globalLightDirection = dayPointParams.m_globalLightDirection;		
	m_moonDirection = dayPointParams.m_moonDirection;
	m_sunDirection = dayPointParams.m_sunDirection;
		
	// update skybox weather blend
	dayPointParams.m_skyBoxWeatherBlend = m_weatherManager->GetSkyboxBlendRatio();
	dayPointParams.m_fakeCloudsShadowCurrentTextureIndex = m_weatherManager->GetFakeShadowsParameters().X;
	dayPointParams.m_fakeCloudsShadowTargetTextureIndex = m_weatherManager->GetFakeShadowsParameters().Y;
	
	if( m_currAreaEnvParams.m_water.m_activated ) 
	{
		Vector col = Vector::ZEROS;
		col = m_currAreaEnvParams.m_water.m_waterColor.GetCachedPoint().GetColor().ToVector();

		const Vector underwater_col = m_currAreaEnvParams.m_water.m_underWaterColor.GetCachedPoint().GetColor().ToVector();

		dayPointParams.m_waterShadingParamsExtra2.X = underwater_col.X;
		dayPointParams.m_waterShadingParamsExtra2.Y = underwater_col.Y;
		dayPointParams.m_waterShadingParamsExtra2.Z = underwater_col.Z;

		dayPointParams.m_waterShadingParams.X = col.X;
		dayPointParams.m_waterShadingParams.Y = col.Y;
		dayPointParams.m_waterShadingParams.Z = col.Z;
		dayPointParams.m_waterShadingParams.W = m_currAreaEnvParams.m_water.m_waterFlowIntensity.GetCachedPoint().GetScalar();

		dayPointParams.m_waterShadingParamsExtra.X = m_currAreaEnvParams.m_water.m_waterFresnel.GetCachedPoint().GetScalar();
		dayPointParams.m_waterShadingParamsExtra.Y = m_currAreaEnvParams.m_water.m_waterCaustics.GetCachedPoint().GetScalar();
		dayPointParams.m_waterShadingParamsExtra.Z = Max( 0.001f, m_currAreaEnvParams.m_water.m_waterAmbientScale.GetCachedPoint().GetScalar() );
		dayPointParams.m_waterShadingParamsExtra.W = Max( 0.001f, m_currAreaEnvParams.m_water.m_waterDiffuseScale.GetCachedPoint().GetScalar() );

		dayPointParams.m_waterFoamIntensity = m_currAreaEnvParams.m_water.m_waterFoamIntensity.GetCachedPoint().GetScalar();

		dayPointParams.m_underWaterBrightness = m_currAreaEnvParams.m_water.m_underwaterBrightness.GetCachedPoint().GetScalar();
		dayPointParams.m_underWaterBrightness += m_gameEnvParams.m_gameUnderwaterBrightness;
		dayPointParams.m_underWaterFogIntensity = m_currAreaEnvParams.m_water.m_underWaterFogIntensity.GetCachedPoint().GetScalar();
	}

	// ...rain strength
	dayPointParams.m_delayedWetSurfaceEffectStrength	= m_weatherManager->GetDelayedWetSurfaceEffectStrength();	
	dayPointParams.m_immediateWetSurfaceEffectStrength  = m_weatherManager->GetImmediateWetSurfaceEffectStrength();	
	dayPointParams.m_wetSurfaceTexture					= m_weatherManager->GetWetSurfaceTexture();
	dayPointParams.m_weatherEffectStrength				= Clamp<Float>( m_weatherManager->GetEffectStrength( EWeatherEffectType::WET_RAIN ) + m_weatherManager->GetEffectStrength( EWeatherEffectType::WET_SNOW ) + m_weatherManager->GetEffectStrength( EWeatherEffectType::WET_HAIL ), 0.0f, 1.0f );	
	const SWorldRenderSettings& renderSetts 			= GetWorldRenderSettings();
	dayPointParams.m_fakeCloudsShadowTexture			= renderSetts.m_fakeCloudsShadowTexture ? renderSetts.m_fakeCloudsShadowTexture->GetRenderResource() : nullptr;


	const SBokehDofParams& cameraBokehDofParams =  m_world->GetCameraDirector()->GetBokehDofParams();
	dayPointParams.m_bokehDofParams = cameraBokehDofParams;

	return dayPointParams;
}

const CGameEnvironmentParams& CEnvironmentManager::GetGameEnvironmentParams() const
{
	return m_gameEnvParams;
}

CGameEnvironmentParams& CEnvironmentManager::GetGameEnvironmentParams()
{
	return m_gameEnvParams;
}

const CRenderBaseLightParams& CEnvironmentManager::GetBaseLightingParams() const
{
	return m_baseLightingParams;
}

void CEnvironmentManager::UpdateWindParameters( const Float envCloudsVelocityOverride, const Float envWindStrengthOverride )
{
	m_weatherManager->SetWindParams( envCloudsVelocityOverride, envWindStrengthOverride );
}

void CEnvironmentManager::EnableSepiaEffect( Float fadeTime )
{
	if ( !m_sepiaEnabled )
	{
		( new CRenderCommand_AddSepiaPostFx( fadeTime ) )->Commit();
	}
	m_sepiaEnabled.Set();
}

void CEnvironmentManager::DisableSepiaEffect( Float fadeTime )
{
	if ( !m_sepiaEnabled.Update( false ) )
	{
		( new CRenderCommand_RemoveSepiaPostFx( fadeTime ) )->Commit();
	}
}

void CEnvironmentManager::DropCachedEnvironmentId( TEnvManagerAreaEnvId id )
{
	if ( -1 == id )
	{
		return;
	}

	if ( m_globalId == id )		m_globalId = -1;
	if ( m_scenesId == id )		m_scenesId = -1;

	if ( m_questId == id )		
	{ 
		m_questId = -1; 
		m_questEnvPath.ClearFast(); 
	}
}

#ifndef NO_EDITOR

void CEnvironmentManager::SetActiveCamera( CCameraComponent* camera )
{
	m_activeCamera = camera;
}

#endif

/////////////////////////////////////////////////////////////////////////////
// Scripts connection

static CAreaEnvironmentComponent* FindAreaEnvironmentComponent( const String &name )
{
	THandle< CWorld > world = GGame ? GGame->GetActiveWorld() : nullptr;
	if ( !name.Empty() && nullptr != world.Get() )
	{
		for ( WorldAttachedComponentsIterator it( world ); it; ++it )
		{
			CAreaEnvironmentComponent *component = Cast< CAreaEnvironmentComponent > ( *it );
			if ( component && component->GetEntity() && component->GetEntity()->GetName() == name )
			{
				return component;
			}
		}
	}

	return NULL;
}

static void funcActivateEnvironmentDefinition( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEnvironmentDefinition >, environmentDefinition, NULL );
	GET_PARAMETER( Int32, priority, 1 );
	GET_PARAMETER( Float, blendFactor, 1.0f );
	GET_PARAMETER( Float, blendInTime, 0.0f );
	FINISH_PARAMETERS;

	CEnvironmentDefinition* envDef = environmentDefinition.Get();
	TEnvManagerAreaEnvId id = -1;

	if ( envDef )
	{
		CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
		if ( envManager )
		{
			id = envManager->ActivateAreaEnvironment( envDef, NULL, priority, blendFactor, blendInTime );
		}
	}

	RETURN_INT( id );
}

static void funcDeactivateEnvironment( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, envId, 1 );
	GET_PARAMETER( Float, blendOutTime, 0.0f );
	FINISH_PARAMETERS;

	CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
	if ( envManager )
	{
		envManager->DeactivateEnvironment( envId, blendOutTime );
	}
}

static void funcActivateQuestEnvironmentDefinition( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEnvironmentDefinition >, environmentDefinition, NULL );
	GET_PARAMETER( Int32, priority, 1 );
	GET_PARAMETER( Float, blendFactor, 1.0f );
	GET_PARAMETER( Float, blendTime, 0.0f );
	FINISH_PARAMETERS;

	CEnvironmentDefinition* envDef = environmentDefinition.Get();

	CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
	if ( envManager )
	{
		if ( envDef )
		{
			envManager->ActivateQuestEnvironment( *envDef, priority, blendFactor, blendTime, blendTime );
		}
		else if ( -1 != envManager->GetQuestEnvironmentID() )
		{
			envManager->DeactivateEnvironment( envManager->GetQuestEnvironmentID(), blendTime, true );
			RED_ASSERT( -1 == envManager->GetQuestEnvironmentID() );
		}
	}
}

static void funcRadialBlurSetup( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, blurSourcePos,	 Vector::ZEROS );		
	GET_PARAMETER( Float,  blurAmount,		 0.f );
	GET_PARAMETER( Float,  sineWaveAmount,	 0.f );
	GET_PARAMETER( Float,  sineWaveSpeed,	 8.f );
	GET_PARAMETER( Float,  sineWaveFreq,	 5.f );
	FINISH_PARAMETERS;

	CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
	if ( envManager )
	{
		// Init new blur params
		CEnvRadialBlurParameters newBlurParams;
		newBlurParams.m_radialBlurSource = blurSourcePos;
		newBlurParams.m_radialBlurAmount = blurAmount;
		newBlurParams.m_sineWaveAmount = sineWaveAmount;
		newBlurParams.m_sineWaveSpeed = sineWaveSpeed;
		newBlurParams.m_sineWaveFreq = sineWaveFreq;

		// Set new blur params
		envManager->GetRadialBlurParams() = newBlurParams;
	}
}

static void funcRadialBlurDisable( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
	if ( envManager )
	{
		// Init new blur params
		CEnvRadialBlurParameters newBlurParams;
		newBlurParams.m_radialBlurAmount = 0;
		newBlurParams.m_sineWaveAmount = 0;

		// Set new blur params
		envManager->GetRadialBlurParams() = newBlurParams;
	}
}

static void funcFullscreenBlurSetup( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float,  intensity,		 0.f );
	FINISH_PARAMETERS;

	CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
	if ( envManager )
	{
		// Set new blur params
		envManager->GetGameEnvironmentParams().m_fullscreenBlurIntensity = Clamp( intensity, 0.f, 1.f );
	}
}

static void funcForceFakeTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float,  time,		 0.f );
	FINISH_PARAMETERS;

	CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
	if ( envManager )
	{
		envManager->GetGameEnvironmentParams().m_dayCycleOverride.m_fakeDayCycleEnable = true;
		envManager->GetGameEnvironmentParams().m_dayCycleOverride.m_fakeDayCycleHour = time;
	}
}

static void funcDisableFakeTime( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
	if ( envManager )
	{
		envManager->GetGameEnvironmentParams().m_dayCycleOverride.m_fakeDayCycleEnable = false;
	}
}

static void funcSetUnderWaterBrightness( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float,  value,		 0.f );
	FINISH_PARAMETERS;

	CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
	if ( envManager )
	{
		envManager->GetGameEnvironmentParams().m_gameUnderwaterBrightness = value;
	}
}

static void funcGetActiveAreaEnvironmentDefinitions( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< String >, defs, TDynArray< String >() );
	FINISH_PARAMETERS;

	CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
	if ( envManager )
	{
		envManager->GetActiveAreaEnvironmentDefinitions( defs );
	}
}

#ifndef RED_FINAL_BUILD

	static void funcEnableDebugOverlayFilter( IScriptable* context, CScriptStackFrame& stack, void* result )
	{
		GET_PARAMETER(Uint32, filter	,1);
		FINISH_PARAMETERS;

		CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
		if ( envManager )
		{
			envManager->SetModifier((EEnvManagerModifier)filter);
		}
	}

	static void funcEnableDebugPostProcess( IScriptable* context, CScriptStackFrame& stack, void* result )
	{
		GET_PARAMETER(Uint32, postProcessName	,1);
		//GET_PARAMETER(CName, postProcessName, CName::NONE);
		GET_PARAMETER(bool, activate, false);
		FINISH_PARAMETERS;
		
		CEnvironmentManager *envManager = GGame && GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
		CGameEnvironmentParams params = envManager->GetGameEnvironmentParams();
		
		switch (postProcessName)
		{
		case 9://Gameplay Dof
			{
				if(activate)
				{
					params.m_displaySettings.m_allowDOF = true;
					envManager->SetGameEnvironmentParams( params );
				}
				if(!activate)
				{
					params.m_displaySettings.m_allowDOF = false;
					envManager->SetGameEnvironmentParams( params );
				}
			}
			break;
		case 5: //Bloom
			{
				if(activate)
				{
					params.m_displaySettings.m_allowBloom = true;
					envManager->SetGameEnvironmentParams( params );
				}
				if(!activate)
				{
					params.m_displaySettings.m_allowBloom = false;
					envManager->SetGameEnvironmentParams( params );
				}
			}
			break;
		case 6: //Color Balance
			{
				if(activate)
				{
					params.m_displaySettings.m_allowColorMod = true;
					envManager->SetGameEnvironmentParams( params );
				}
				if(!activate)
				{
					params.m_displaySettings.m_allowColorMod = false;
					envManager->SetGameEnvironmentParams( params );
				}
			}
			break;
		case 13: //Temporal Antialiasing disclaimer: this if for both temporal and fxaa
			{
				if(activate)
				{
					params.m_displaySettings.m_allowAntialiasing = true;
					envManager->SetGameEnvironmentParams( params );
				}
				if(!activate)
				{
					params.m_displaySettings.m_allowAntialiasing = false;
					envManager->SetGameEnvironmentParams( params );
				}
			}
			break;
		case 7: //FXAA - same situation as with temporal - it disables all antialiasing
			{
				if(activate)
				{
					params.m_displaySettings.m_allowAntialiasing = true;
					envManager->SetGameEnvironmentParams( params );
				}
				if(!activate)
				{
					params.m_displaySettings.m_allowAntialiasing = false;
					envManager->SetGameEnvironmentParams( params );
				}
			}
			break;
		case 8: //Global Fog
			{
				if(activate)
				{
					params.m_displaySettings.m_allowGlobalFog = true;
					envManager->SetGameEnvironmentParams( params );
				}
				if(!activate)
				{
					params.m_displaySettings.m_allowGlobalFog = false;
					envManager->SetGameEnvironmentParams( params );
				}
			}
			break;
		case 10: //SSAO
			{
				if(activate)
				{
					params.m_displaySettings.m_allowSSAO = true;
					envManager->SetGameEnvironmentParams( params );
				}
				if(!activate)
				{
					params.m_displaySettings.m_allowSSAO = false;
					envManager->SetGameEnvironmentParams( params );
				}
			}
			break;
		case 11: //Clouds Shadow
			{
				if(activate)
				{
					params.m_displaySettings.m_allowCloudsShadow = true;
					envManager->SetGameEnvironmentParams( params );
				}
				if(!activate)
				{
					params.m_displaySettings.m_allowCloudsShadow = false;
					envManager->SetGameEnvironmentParams( params );
				}
			}
			break;
		case 12: //Vignette
			{
				if(activate)
				{
					params.m_displaySettings.m_allowVignette = true;
					envManager->SetGameEnvironmentParams( params );
				}
				if(!activate)
				{
					params.m_displaySettings.m_allowVignette = false;
					envManager->SetGameEnvironmentParams( params );
				}
			}
			break;
		case 4: //Env Probe Update
			{
				if(activate)
				{
					params.m_displaySettings.m_allowEnvProbeUpdate = true;
					envManager->SetGameEnvironmentParams( params );
				}
				if(!activate)
				{
					params.m_displaySettings.m_allowEnvProbeUpdate = false;
					envManager->SetGameEnvironmentParams( params );
				}
			}
			break;
		}		
	}

#endif
	

void ExportEnvironmentManagerNatives()
{
	NATIVE_GLOBAL_FUNCTION( "ActivateEnvironmentDefinition",		funcActivateEnvironmentDefinition );
	NATIVE_GLOBAL_FUNCTION( "DeactivateEnvironment",				funcDeactivateEnvironment );
	NATIVE_GLOBAL_FUNCTION( "ActivateQuestEnvironmentDefinition",	funcActivateQuestEnvironmentDefinition );
	NATIVE_GLOBAL_FUNCTION( "RadialBlurSetup",						funcRadialBlurSetup );
	NATIVE_GLOBAL_FUNCTION( "RadialBlurDisable",					funcRadialBlurDisable );
	NATIVE_GLOBAL_FUNCTION( "FullscreenBlurSetup",					funcFullscreenBlurSetup );
	NATIVE_GLOBAL_FUNCTION( "ForceFakeEnvTime",						funcForceFakeTime );
	NATIVE_GLOBAL_FUNCTION( "DisableFakeEnvTime",					funcDisableFakeTime );
	NATIVE_GLOBAL_FUNCTION( "SetUnderWaterBrightness",				funcSetUnderWaterBrightness );
	NATIVE_GLOBAL_FUNCTION( "GetActiveAreaEnvironmentDefinitions",	funcGetActiveAreaEnvironmentDefinitions );

#ifndef RED_FINAL_BUILD
	NATIVE_GLOBAL_FUNCTION( "EnableDebugOverlayFilter",				funcEnableDebugOverlayFilter );
	NATIVE_GLOBAL_FUNCTION( "EnableDebugPostProcess",				funcEnableDebugPostProcess );
#endif

	extern void ExportWeatherManagerNatives();
	ExportWeatherManagerNatives();
}
