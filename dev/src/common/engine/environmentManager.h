/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../core/countedBool.h"
#include "gameSession.h"
#include "environmentGameParams.h"
#include "envParameters.h"
#include "renderFrameInfo.h"

#define WORLDENV_DEFINITIONS_ROOT					TXT( "environment\\definitions\\" )
#define THUMBNAIL_GEN_ENVIRONMENT					TXT( "engine\\thumbnails\\thumbnail_env.env" )
#define INVALID_AREA_ENV_ID							-1

class IGameSaver;
class CDirectory;

/// Environment modifier
enum EEnvManagerModifier
{
	EMM_None = 0,
	EMM_GBuffAlbedo,
		EMM_GBUFF_FIRST = EMM_GBuffAlbedo,
	EMM_GBuffAlbedoNormalized,
	EMM_GBuffNormalsWorldSpace,
	EMM_GBuffNormalsViewSpace,
	EMM_GBuffSpecularity,
	EMM_GBuffRoughness,
	EMM_GBuffTranslucency,
		EMM_GBUFF_LAST = EMM_GBuffTranslucency,
	EMM_Depth,
	EMM_LinearAll,
	EMM_DecomposedAmbient,
	EMM_DecomposedDiffuse,
	EMM_DecomposedSpecular,
	EMM_DecomposedReflection,
	EMM_DecomposedLightingAmbient,
	EMM_DecomposedLightingReflection,
	EMM_DimmersSurface,
	EMM_DimmersVolume,
	EMM_ComplexityEnvProbes,
	EMM_MaskShadow,
	EMM_MaskSSAO,
	EMM_MaskInterior,
	EMM_MaskDimmers,
	EMM_InteriorsVolume,
	EMM_InteriorsFactor,
	EMM_Bloom,	
	EMM_StencilMix,
	EMM_Stencil0,
	EMM_Stencil1,
	EMM_Stencil2,
	EMM_Stencil3,
	EMM_Stencil4,
	EMM_Stencil5,
	EMM_Stencil6,
	EMM_Stencil7,
	EMM_LocalReflections,
	EMM_LightsOverlay,
	EMM_LightsOverlayDensity,

	EMM_MAX,
};

BEGIN_ENUM_RTTI( EEnvManagerModifier )
	ENUM_OPTION( EMM_None												)
	ENUM_OPTION( EMM_GBuffAlbedo										)
	//ENUM_OPTION( EMM_GBUFF_FIRST = EMM_GBuffAlbedo					)
	ENUM_OPTION( EMM_GBuffAlbedoNormalized								)
	ENUM_OPTION( EMM_GBuffNormalsWorldSpace								)
	ENUM_OPTION( EMM_GBuffNormalsViewSpace								)
	ENUM_OPTION( EMM_GBuffSpecularity									)
	ENUM_OPTION( EMM_GBuffRoughness										)
	ENUM_OPTION( EMM_GBuffTranslucency									)
	//ENUM_OPTION( EMM_GBUFF_LAST = EMM_GBuffTranslucency				)
	ENUM_OPTION( EMM_Depth												)
	ENUM_OPTION( EMM_LinearAll											)
	ENUM_OPTION( EMM_DecomposedAmbient									)
	ENUM_OPTION( EMM_DecomposedDiffuse									)
	ENUM_OPTION( EMM_DecomposedSpecular									)
	ENUM_OPTION( EMM_DecomposedReflection								)
	ENUM_OPTION( EMM_DecomposedLightingAmbient							)
	ENUM_OPTION( EMM_DecomposedLightingReflection						)
	ENUM_OPTION( EMM_DimmersSurface										)
	ENUM_OPTION( EMM_DimmersVolume										)
	ENUM_OPTION( EMM_ComplexityEnvProbes								)
	ENUM_OPTION( EMM_MaskShadow											)
	ENUM_OPTION( EMM_MaskSSAO											)
	ENUM_OPTION( EMM_MaskInterior										)
	ENUM_OPTION( EMM_MaskDimmers										)
	ENUM_OPTION( EMM_InteriorsVolume									)
	ENUM_OPTION( EMM_InteriorsFactor									)
	ENUM_OPTION( EMM_Bloom												)
	ENUM_OPTION( EMM_StencilMix											)
	ENUM_OPTION( EMM_Stencil0											)
	ENUM_OPTION( EMM_Stencil1											)
	ENUM_OPTION( EMM_Stencil2											)
	ENUM_OPTION( EMM_Stencil3											)
	ENUM_OPTION( EMM_Stencil4											)
	ENUM_OPTION( EMM_Stencil5											)
	ENUM_OPTION( EMM_Stencil6											)
	ENUM_OPTION( EMM_Stencil7											)
	ENUM_OPTION( EMM_LocalReflections									)
	ENUM_OPTION( EMM_LightsOverlay										)
	ENUM_OPTION( EMM_LightsOverlayDensity								)
			
	ENUM_OPTION( EMM_MAX												)
	END_ENUM_RTTI()

	enum EDebugPostProcess
{
	EDPP_Gamma,
	EDPP_EnableInstantAdaptation,
	EDPP_EnableGlobalLightingTrajectory,
	EDPP_EnableEnvProbeInstantUpdate,
	EDPP_AllowEnvProbeUpdate,
	EDPP_AllowBloom,
	EDPP_AllowColorMod,
	EDPP_AllowAntialiasing,
	EDPP_AllowGlobalFog,
	EDPP_AllowDOF,
	EDPP_AllowSSAO,
	EDPP_AllowCloudsShadow,
	EDPP_AllowVignette,
	EDPP_DisableTonemapping,
	EDPP_ForceCutsceneDofMode,
	EDPP_AllowWaterShader,
	EDPP_DisplayMode,
};

BEGIN_ENUM_RTTI( EDebugPostProcess )
	ENUM_OPTION( EDPP_Gamma								)
	ENUM_OPTION( EDPP_EnableInstantAdaptation								)
	ENUM_OPTION( EDPP_EnableGlobalLightingTrajectory								)
	ENUM_OPTION( EDPP_AllowEnvProbeUpdate								)
	ENUM_OPTION( EDPP_AllowBloom								)
	ENUM_OPTION( EDPP_AllowColorMod								)
	ENUM_OPTION( EDPP_AllowAntialiasing								)
	ENUM_OPTION( EDPP_AllowGlobalFog								)
	ENUM_OPTION( EDPP_AllowDOF								)
	ENUM_OPTION( EDPP_AllowSSAO								)
	ENUM_OPTION( EDPP_AllowCloudsShadow								)
	ENUM_OPTION( EDPP_AllowVignette								)
	ENUM_OPTION( EDPP_DisableTonemapping								)
	ENUM_OPTION( EDPP_ForceCutsceneDofMode								)
	ENUM_OPTION( EDPP_AllowWaterShader								)
	ENUM_OPTION( EDPP_DisplayMode								)
	END_ENUM_RTTI()

static const Char* GetDebugPreviewName( const EEnvManagerModifier tag )
{
#define TEST(x) case x: return TXT(#x);
	switch ( tag )
	{
		TEST(EMM_None);
		TEST(EMM_GBuffAlbedo);
		TEST(EMM_GBuffAlbedoNormalized);
		TEST(EMM_GBuffNormalsWorldSpace);
		TEST(EMM_GBuffNormalsViewSpace);
		TEST(EMM_GBuffSpecularity);
		TEST(EMM_GBuffRoughness);
		TEST(EMM_GBuffTranslucency);
		TEST(EMM_Depth);
		TEST(EMM_LinearAll);
		TEST(EMM_DecomposedAmbient);
		TEST(EMM_DecomposedDiffuse);
		TEST(EMM_DecomposedSpecular);
		TEST(EMM_DecomposedReflection);
		TEST(EMM_DecomposedLightingAmbient);
		TEST(EMM_DecomposedLightingReflection);
		TEST(EMM_DimmersSurface);
		TEST(EMM_DimmersVolume);
		TEST(EMM_ComplexityEnvProbes);
		TEST(EMM_MaskShadow);
		TEST(EMM_MaskSSAO);
		TEST(EMM_MaskInterior);
		TEST(EMM_MaskDimmers);
		TEST(EMM_InteriorsVolume);
		TEST(EMM_InteriorsFactor);
		TEST(EMM_Bloom);
		TEST(EMM_StencilMix);
		TEST(EMM_Stencil0);
		TEST(EMM_Stencil1);
		TEST(EMM_Stencil2);
		TEST(EMM_Stencil3);
		TEST(EMM_Stencil4);
		TEST(EMM_Stencil5);
		TEST(EMM_Stencil6);
		TEST(EMM_Stencil7);
		TEST(EMM_LocalReflections);
		TEST(EMM_LightsOverlay);
		TEST(EMM_LightsOverlayDensity);
	}
#undef TEST

	return TXT("Unknown");
}

#define MAX_ENVIRONMENT_BLENDS 8

struct SEnvironmentBlendInfo
{
	CAreaEnvironmentParams params;
	Float blendFactor;
};

/// CEnvironmentManager
class CEnvironmentManager : public IGameSaveSection
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );
private:
	static EEnvManagerModifier s_modifier;

public:
	/// Set modifier
	static void					SetModifier( EEnvManagerModifier modifier ) { s_modifier = modifier;	}
	static EEnvManagerModifier	GetModifier()								{ return s_modifier;		}

protected:	

	struct SEnvManagerPrioritySlot
	{
		Int32	priority;
		CGUID	guid;
		Uint32	environmentIndex;
	};

public:
	// E3 DEMOHACK storm lightning
	struct SBalanceOverride
	{
		Vector							m_parametricBalanceLowOverride;
		Vector							m_parametricBalanceMidOverride;
		Vector							m_parametricBalanceHighOverride;
	};
	// -
	enum
	{
		ScenesEnvironmentPriority	= 1024
	};

public:
	typedef TDynArray< SEnvManagerAreaEnvData, MC_RenderData >	TAreaEnvironmentsArray;
	
protected:
	CWorld*							m_world;	
	CAreaEnvironmentParams			m_defaultCurvesParams;
	CAreaEnvironmentParams			m_currAreaEnvParams;
	TAreaEnvironmentsArray			m_areaEnvironments;
	SEnvManagerPrioritySlot			m_environmentSlot[MAX_ENVIRONMENT_BLENDS];
	Uint32							m_environmentSlots;
	TEnvManagerAreaEnvId			m_globalId;
	TEnvManagerAreaEnvId			m_scenesId;
	TEnvManagerAreaEnvId			m_questId;
	String							m_questEnvPath;

#ifndef NO_DEBUG_WINDOWS
	/// engine default environments
	TDynArray< Red::TUniquePtr< CAreaEnvironmentParams > >	m_defaultAreaEnvironments;
	/// names for widget choice list fill up
	TDynArray< String >							m_defaultAreaEnvironmentsNames;
#endif

	bool							m_currAreaEnvParamsForced;
	bool							m_currAreaEnvUpdated;
	TEnvManagerAreaEnvId			m_areaEnvIdGenerator;
	Bool							m_instantAdaptationTrigger;
	Bool							m_instantDissolveTrigger;
	CGameEnvironmentParams			m_gameEnvParams;
	CRenderBaseLightParams			m_baseLightingParams;
	Float							m_cachedRainStrength;	
	Float							m_lastKnownGameTime;
	CRadialBlurManager				m_radialBlurManager;
	CLightShaftManager				m_lightShaftManager;
	CBrightnessTintManager			m_brightnessTintManager;
	CEnvRadialBlurParameters*		m_radialBlurDefaultParams;
	CountedBool						m_HDRAdaptationDisabled;
	CountedBool						m_sepiaEnabled;
	Bool							m_envChangesDisabled;
	mutable Vector					m_sunDirection;
	mutable Vector					m_moonDirection;
	mutable Vector					m_globalLightDirection;
	SEnvironmentBlendInfo			m_environmentBlend[MAX_ENVIRONMENT_BLENDS];
	Uint32							m_environmentBlends;
	Uint32							m_mostImportantEnvironment;
	Uint32							m_secondMostImportantEnvironment;
	Float							m_mostImportantBlendFactor;
	class CWeatherManager*			m_weatherManager;
	SCameraLightsModifiersSetup		m_cameraLightsModifiers;
	Float							m_hiResShadowMaxExtents;
	Float							m_lastGameplayCameraLightsFactor;
	Bool							m_lastGameplayCameraInterior;
	Float							m_lastGameplayCameraLightsTime;
	Vector							m_lastGameplayCameraLightsCamPos;
	Float							m_distantLightOverride;

	// E3 DEMOHACK storm lightning
	CRenderBaseLightParams*			m_baseLightingParamsOverride;
	Vector							m_ambientOverride; //< TODO: remove/unused
	SBalanceOverride*				m_balanceOverride;
	// -
	
#ifndef NO_EDITOR
	THandle< CCameraComponent >		m_activeCamera;
#endif

public:
	CEnvironmentManager ( CWorld *world );
	virtual ~CEnvironmentManager ();

public:
	/// Returns world this environment belongs to
	RED_INLINE CWorld* GetWorld() const { return m_world; }

	/// Returns the weather manager
	RED_INLINE class CWeatherManager* GetWeatherManager() const { return m_weatherManager; }

	/// Returns quest environment ID
	RED_INLINE TEnvManagerAreaEnvId GetQuestEnvironmentID( ) { return m_questId; }

	Vector GetCurrentWindVector( const Vector& point );
	Vector GetCurrentWindVector();	
	Float GetWindScale();
	
public:
	/// Returns game time
	GameTime GetCurrentGameTime( bool continous = false ) const;
	
	void UpdateWindParameters( const Float envCloudsVelocityOverride, const Float envWindStrengthOverride );
	void SearchForAllAreaEnvs( CDirectory* rootDir, TDynArray< CEnvironmentDefinition* > &outParamas );
	void UpdateWeatherParametersFromWorld();

	/// Return time that may be used for rendering (high precision and continous)
	EngineTime GetCurrentEnvAnimationTime() const;

#ifndef NO_DEBUG_WINDOWS
	/// Returns the default environment resources loaded from \\engine\\environments\\*.env
	const TDynArray<String>& GetDefaultEnvironmentsNames() const;
	RED_INLINE const TAreaEnvironmentsArray& GetActiveEnvironments() const { return m_areaEnvironments; }
#endif
	
public:
	//! Initialize at game start, called directly in StartGame, but should always be implemented
	virtual void OnGameStart( const CGameInfo& gameInfo );

	//! Initialize anything left, called AFTER game world has been loaded and loading screen video ended
	virtual void OnAfterLoadingScreenGameStart( const CGameInfo& gameInfo );

	//! Shutdown at game end, called directly in EndGame, but should always be implemented
	virtual void OnShutdownAtGameEnd();

	//! Save game
	virtual bool OnSaveGame( IGameSaver* saver );


protected:
	/// Returns index of given area env id
	Int32	FindAreaEnvById( TEnvManagerAreaEnvId id ) const;

	/// Generates new area environment id
	TEnvManagerAreaEnvId GenerateAreaEnvId();

	/// Removes area environment with given index
	void RemoveEnvironmentByIndex( Int32 envDataIndex );

	/// Rebuilds the environment slots
	void RebuildEnvironmentSlots();

	/// Updates the time-based environment blending states (may cause environments to be dropped)
	void UpdateTimeBasedEnvironmentBlending();

	/// Calculates area environment params for given position and time
	void CalculateEnvironmentForGivenPositionAndTime( const Vector& position, Float time, CAreaEnvironmentParams& outResult );

	/// Applies weather modifications to the passed area environment parameters (assumes single-point curves in parameters)
	void ApplyWeatherModifications( CAreaEnvironmentParams& params, Float time );

	/// Debug logging
	void LogState() const;

	/// Find global env to activate
	void FindAndActivateGlobalEnv();

public:

	/// Update camera parameters
	void UpdateCameraParams();

	/// Is area environment with given id active
	bool IsAreaEnvironmentActive( TEnvManagerAreaEnvId id ) const;

	/// Activates area environment with given params
	TEnvManagerAreaEnvId ActivateAreaEnvironment( const CEnvironmentDefinition* const def, class CAreaEnvironmentComponent* areaComponent, Int32 priority, Float blendFactor, Float blendInTime = 0.0f, TEnvManagerAreaEnvId prevIdHint = -1 );

	/// Activate quest environment
	TEnvManagerAreaEnvId ActivateQuestEnvironment( const CEnvironmentDefinition &def, Int32 priority, Float blendFactor, Float blendInTime, Float prevQuestIdBlendOutTime );
	
	/// Changes given area environment parameters
	void ChangeAreaEnvironmentParameters( TEnvManagerAreaEnvId id, const CAreaEnvironmentParams &params );

	/// Changes given environment blendFactor
	void ChangeAreaEnvironmentBlendFactor( TEnvManagerAreaEnvId id, Float blendFactor );
	
	/// Deactivates given environment if it's active
	void DeactivateEnvironment( TEnvManagerAreaEnvId id, Float blendOutTime = 0.0f, Bool forceDropCachedId = false );

	/// Is forced area environment settings enabled
	bool IsAreaEnvironmentForced() const;

	// E3 DEMOHACK storm lightning
	void SetBaseLightingOverride( CRenderBaseLightParams* params );
	void SetAmbientOverride( const Vector& ambient );
	void SetBalanceOverride( SBalanceOverride* balanceOverride );
	// -
	
public:
	// Enable/disable shit

	// Disable/reenable hdr
	void DisableHDR() { m_HDRAdaptationDisabled.Set(); }
	void EnableHDR() { m_HDRAdaptationDisabled.Unset(); }

	// Enable/disable sepia
	void EnableSepiaEffect( Float time );
	void DisableSepiaEffect( Float time );

public:
	/// Forces given area environment parameters
	void EnableForcedAreaEnvironment( const CAreaEnvironmentParams &areaEnv );

	/// Disables forcing area environment params
	void DisableForceAreaEnvironment();

	/// Sets game environment parameters
	void SetGameEnvironmentParams( const CGameEnvironmentParams &params );

	/// Disabling env changes for cutscenes
	void EnableEnvChanges(){ m_envChangesDisabled = false; }
	void DisableEnvChanges(){ m_envChangesDisabled = true; }
	Bool AreEnvChangesDisabled(){ return m_envChangesDisabled; }
	
	/// Switch to one of the predefined environments
	Bool SwitchToPredefinedEnv( Uint32 selection );

public:
	CRadialBlurManager&	GetRadialBlurManager(){ return m_radialBlurManager; }

	CLightShaftManager&	GetLightShaftManager(){ return m_lightShaftManager; }

	CBrightnessTintManager&	GetBrightnessTintManager(){ return m_brightnessTintManager; }

	CEnvRadialBlurParameters& GetRadialBlurParams(){ return *m_radialBlurDefaultParams; }

	/// Get current area environment params
	const CAreaEnvironmentParams& GetCurrentAreaEnvironmentParams() const;

	/// Get two most important environment blends
	void GetMostImportantEnvironments( CAreaEnvironmentParams** first, CAreaEnvironmentParams** second, Float& blendFactor );

	/// Get current day point environment params
	const SDayPointEnvironmentParams GetDayPointEnvironmentParams() const;

	/// Get world render settings
	const SWorldRenderSettings& GetWorldRenderSettings() const;

	/// Get game environment params
	const CGameEnvironmentParams& GetGameEnvironmentParams() const;

	/// Get the base lighting params
	const CRenderBaseLightParams& GetBaseLightingParams() const;

	/// Get game environment params
	CGameEnvironmentParams& GetGameEnvironmentParams();

	/// Drop cached environment id
	void DropCachedEnvironmentId( TEnvManagerAreaEnvId id );

	const Vector& GetSunDirection() const { return m_sunDirection; }
	const Vector& GetMoonDirection() const { return m_moonDirection; }
	const Vector& GetGlobalLightDirection() const { return m_globalLightDirection; }

	void UpdateGameplayCameraLightsFactor( Bool isInterior, Float time, const Vector &cameraPosition, Bool forceInstant );
	Float GetGameplayCameraLightsFactor( Float time ) const;

public:
	// Generate environment editor fragments
	void GenerateEditorFragments( CRenderFrame* frame );

	// Set instant adaptation trigger (will be cleared when realized)
	void SetInstantAdaptationTrigger( Bool enable = true );

	// Set instant dissolve trigger (will be cleared when realized)
	void SetInstantDissolveTrigger( Bool enable = true );

	// Get near plane
	Float GetNearPlane() const;

	RED_INLINE void SetDistantLightOverride( const Float distantLightOverride ) { m_distantLightOverride = distantLightOverride; }

	RED_INLINE Float GetDistantLightOverride( ) const { return m_distantLightOverride; }

	// Get instant adaptation trigger
	RED_INLINE Bool GetInstantAdaptationTrigger() const { return m_instantAdaptationTrigger; }

	// Get instant dissolve trigger
	RED_INLINE Bool GetInstantDissolveTrigger() const { return m_instantDissolveTrigger; }

	// Set camera modifiers
	void SetCameraLightsModifiers( const SCameraLightsModifiersSetup &newValue );

	// Get camera modifiers
	RED_INLINE const SCameraLightsModifiersSetup& GetCameraLightsModifers() const { return m_cameraLightsModifiers; }

	// Get Hi Res Shadow Map Extents
	RED_INLINE const Float GetHiResShadowMapExtents() const { return m_hiResShadowMaxExtents; }

	// Set Hi Res Shadow Map Extents
	RED_INLINE void SetHiResShadowMapExtents( Float hiResShadowMapExtents ) { m_hiResShadowMaxExtents = hiResShadowMapExtents; }

	// Update scenes environment
	void UpdateScenesEnvironment();

public:
	// Generate global lighting trajectory fragments
	void GenerateGlobalLightingTrajectoryFragments( CRenderFrame* frame );

public:
	/// Get reference position for updating area environments
	Vector GetCurrentAreaEnvUpdateReferencePos() const;

	/// Updates locally cached current area environment
	void UpdateCurrentAreaEnvironment();

	/// Update base lighting settings
	void UpdateBaseLightingParams();

	/// Performs activation/deactivation of area environments, so that their activation status matches given position
	void SetupAreaEnvironmentsForPosition( const Vector &pos, bool allowLocalEnvChanges, bool allowGlobalEnvChanges );

	/// Get filenames (without extensions) of definitions of all area environments active at the moment
	void GetActiveAreaEnvironmentDefinitions( TDynArray< String >& defs );

	// Tick environment animation
	void Tick_Single( Float timeDelta );
	void Tick_Parallel( Float timeDelta );
	void Tick_TeleportFixup();

	// Serialize objects for the garbage collector
	void SerializeForGC( IFile& file );

#ifndef NO_EDITOR

	// Sets camera for DOF calculations
	void SetActiveCamera( CCameraComponent* camera );

#endif
};
