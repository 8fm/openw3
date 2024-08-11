/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderCamera.h"
#include "envParameters.h"
#include "environmentGameParams.h"
#include "showFlags.h"

class CHitProxyMap;
class IViewport;
class IRenderGameplayRenderTarget;
class CRenderTexture;

enum EMaterialDebugMode : Int32;

//dex++: Total (engine max) number of shadow cascades allowed
// I do not recomend to change this above 4 (some constants are packed into float4 variables)
// deeper refactor fo constant buffers is required if values larged than 4 are required.
#define MAX_CASCADES		4
//dex--

/// Rendering mode
enum ERenderingMode : Int32
{
	RM_Wireframe,
	RM_HitProxies,
	RM_Flat,
	RM_Shaded,
	RM_ShadowDepth,
};

/// DisplayMode feature filter
enum EDisplayModeFeatureFilter
{
	DMFF_LightingAmbient				= FLAG( 0 ),
	DMFF_LightingDiffuse				= FLAG( 1 ),
	DMFF_LightingSpecular				= FLAG( 2 ),
	DMFF_LightingReflection				= FLAG( 3 ),
	DMFF_LightingTranslucency			= FLAG( 4 ),
	DMFF_ToneMappingLuminanceUpdate		= FLAG( 5 ),
	DMFF_EnvProbeUpdate					= FLAG( 6 ),

	DMFF_MASK_ALL						= 0xffffffff,
	DMFF_MASK_LIGHTING					= DMFF_LightingAmbient | DMFF_LightingDiffuse | DMFF_LightingSpecular | DMFF_LightingReflection | DMFF_LightingTranslucency,
};

enum EVisualDebugCommonOptions : Int32
{
	VDCommon_MaxRenderingDistance	= 0,
	VDCommon_DebugLinesThickness	= 1,
	VDCommon__MAX_INDEX
};

/// SCameraLightModifiers
struct SCameraLightModifiers
{
	SCameraLightModifiers()
	{
		SetIdentity( false );
	}

	void SetIdentity( bool lightEnabled )
	{
		offsetFront = 0.f;
		offsetRight = 0.f;
		offsetUp = 0.f;

		colorOverrideAmount = 0.f;
		colorOverrideR = 0.f;
		colorOverrideG = 0.f;
		colorOverrideB = 0.f;

		brightnessScale = lightEnabled ? 1.f : 0.f;
		radiusScale = 1.f;

		attenuationOverrideAmount = 0.f;
		attenuationOverride = 0.f;

		absoluteValues = 0;
	}

	Vector GetColorOverride() const
	{
		return Vector ( Max( 0.f, colorOverrideR ), Max( 0.f, colorOverrideG ), Max( 0.f, colorOverrideB ) ) * brightnessScale;
	}

	Float	offsetFront;
	Float	offsetRight;
	Float	offsetUp;

	Float	colorOverrideAmount;
	Float	colorOverrideR;
	Float	colorOverrideG;
	Float	colorOverrideB;

	Float	brightnessScale;
	Float	radiusScale;

	Float	attenuationOverrideAmount;
	Float	attenuationOverride;

	Uint32	absoluteValues;
};

/// SCameraLightsModifiersSetup

enum ECameraLightModType
{
	ECLT_Scene,				//< value 0 to be a fallback for ECLT_Dialog, ECLT_Cutscene
	// ECLT_Dialog,			< this label was use back in the days and now it should fallback to ECLT_Scene (so make sure that by the time adding ECLT_Dialog, all cutscenes were resaved)
	// ECLT_Cutscene,		< this label was use back in the days and now it should fallback to ECLT_Scene (so make sure that by the time adding ECLT_Cutscene, all cutscenes were resaved)
	ECLT_Gameplay,
	ECLT_DialogScene,		// they want it back! But to avoid any case of old files being around i used this instead of ECLT_Dialog
	ECLT_Interior,			// extra special interior camera lights

	ECLT_MAX
};

BEGIN_ENUM_RTTI(ECameraLightModType)
	ENUM_OPTION( ECLT_Scene )
	ENUM_OPTION( ECLT_Gameplay )
	ENUM_OPTION( ECLT_DialogScene )
	ENUM_OPTION( ECLT_Interior )
END_ENUM_RTTI()

struct SCameraLightsTypeModifier
{
	void SetIdentity( bool lightEnabled )
	{
		m_lightModifier0.SetIdentity( lightEnabled );
		m_lightModifier1.SetIdentity( lightEnabled );
	}

	SCameraLightModifiers m_lightModifier0;
	SCameraLightModifiers m_lightModifier1;
};

struct SCameraLightsModifiersSetup
{	
	SCameraLightsModifiersSetup()
	{
		SetDisableDof( false );
		SetScenesSystemActiveFactor( 0.f );
		SetModifiersAllIdentityOneEnabled( ECLT_Gameplay );
	}

	void SetDisabled()
	{
		SetDisableDof( false );
		SetScenesSystemActiveFactor( 0.f );
		SetModifiersIdentity( false );
	}

	void SetModifiersIdentity( bool lightEnabled )
	{
		for ( Uint32 type_i=0; type_i<ECLT_MAX; ++type_i )
		{
			m_modifiersByType[type_i].SetIdentity( lightEnabled );
		}
	}

	void SetModifiersAllIdentityOneEnabled( ECameraLightModType enabledType )
	{
		for ( Uint32 type_i=0; type_i<ECLT_MAX; ++type_i )
		{
			m_modifiersByType[type_i].SetIdentity( type_i == enabledType );
		}
	}

	void SetScenesSystemActiveFactor( Float newFactor )
	{
		m_scenesSystemActiveFactor = Clamp( newFactor, 0.f, 1.f );
	}

	void SetDisableDof( const Bool disable )
	{
		m_disableDof = disable;
	}

	Float						m_scenesSystemActiveFactor;
	Bool						m_disableDof;
	SCameraLightsTypeModifier	m_modifiersByType[ ECLT_MAX ];
};

/// Information about 2D overlay frame to render
struct CRenderFrameOverlayInfo
{
	Uint32							m_width;						// 2D overlay scene rendering width (viewport width from before RenderCommand)
	Uint32							m_height;						// 2D overlay scene rendering height (viewport height from before RenderCommand)
};

/// Information about frame to render
class CRenderFrameInfo
{
	friend class CRenderInterface;

private:
	IViewport*						m_viewport;						// Target viewport ( not needed if rendering surfaces given )

public:	
	IRenderGameplayRenderTarget*	m_renderTarget;					// Custom render target for offscreen rendering
	CRenderCamera					m_camera;						// Camera settings
	CRenderCamera					m_occlusionCamera;				// Camera used for occlusion culling
	ERenderingMode					m_renderingMode;				// Rendering mode
	Bool							m_allowPostSceneRender;			// Is post scene rendering allowed (postfx, final render etc)
	Bool							m_allowSkinningUpdate;			// If true, allows to animation being updated
	Bool							m_forceGBufferClear;			// Force clear gbuffers before rendering
	Bool							m_present;						// Should we present the frame
	Uint32							m_width;						// Scene rendering width (viewport width from before RenderCommand)
	Uint32							m_height;						// Scene rendering height (viewport height from before RenderCommand)
	Bool							m_customRenderResolution;		// Don't allow to treat frame as main renderer with hacky platform resolutions
	Matrix							m_canvasToWorld;				// Matrix that transforms from 2D onscreen canvas to world space
	Bool							m_instantAdaptation;			// Instant HDR exposure adaptation
	Color							m_clearColor;					// Clear color ( for editor panels )
	Float							m_worldTime;					// Current world time at the moment this frame was generated
	Float							m_engineTime;					// True engine time
	Float							m_cleanEngineTime;				// Clean engine time (zeroed with each game session start)
	Float							m_gameTime;						// Game time
	Uint32							m_gameDays;						// Game days
	Float							m_globalWaterLevelAtCameraPos;	// Global water level at camera pos
	Bool							m_isWorldScene;					// If the info was generated for main world scene, this is true
	Bool							m_isGamePaused;					// Is game is paused, this will be true
	Float							m_gameplayCameraLightsFactor;	// How much we should blend between interior and exterior scale for gameplay camera lights

	SDayPointEnvironmentParams		m_envParametersDayPoint;		// World environment parameters including wind / weather conditions
	
	CAreaEnvironmentParamsAtPoint	m_envParametersArea;			// Area environment parameters
	CAreaEnvironmentParamsAtPoint	m_envParametersAreaBlend1;		// First area environment parameters for blending (only some parts used)
	CAreaEnvironmentParamsAtPoint	m_envParametersAreaBlend2;		// Second area environment parameters for blending (only some parts used)
	Float							m_envBlendingFactor;			// Blending factor between m_envParametersArea and m_envParametersAreaBlend
	CGameEnvironmentParams			m_envParametersGame;			// Game environment parameters	
	Float							m_tonemapFixedLumiance;			// Fixed luminance if using tonemapping
	Vector							m_backgroundTextureColorScale;	// Scaling of the color for texture background

	SGlobalSpeedTreeParameters		m_speedTreeParameters;			// Speed tree parameters
	CRenderBaseLightParams			m_baseLightingParameters;		// Parameters of base scene lighting
	SWorldRenderSettings			m_worldRenderSettings;			// World render settings
	SCameraLightsModifiersSetup		m_cameraLightsModifiersSetup;	// Camera lightsmodifiers setup
	Float							m_frameTime;					// Frame time for shader constants
	Bool							m_isNonInteractiveRendering;	// Was frame created by non interactive rendering process
	Bool							m_isLastFrameForced;			// Last frame force
	Bool							m_forceFade;					// Force the postprocess fade
	Bool							m_drawHUD;						// Hud during last frame force
	EMaterialDebugMode				m_materialDebugMode;			// Material debug mode
	Uint32							m_renderFeaturesFlags;			// Render features flags	

	Uint32							m_requestedNumCascades;							// Number of shadow cascades for global shadows ( at least one )
	Float							m_cascadeEndDistances[ MAX_CASCADES ];			// Cascade distances ( usually taken from WorldShadowInfo )
	Float							m_cascadeFilterSizes[ MAX_CASCADES ];			// Generalized kernel size for filter
	Float							m_speedTreeCascadeFilterSizes[ MAX_CASCADES ];	// Speedtree kernel size for filter
	Float							m_shadowBiasOffsetConstPerCascade[ MAX_CASCADES ];	// Shadow const bias per cascade
	Float							m_speedTreeShadowGradient;						// Speedtree shadow gradient
	Float							m_shadowBiasOffsetSlopeMul;				// Shadow slope offset multiplier
	Float							m_shadowBiasOffsetConst;				// Shadow const bias
	Float							m_shadowBiasCascadeMultiplier;			// Shadow bias multiplier per cascade
	Float							m_shadowEdgeFade[ MAX_CASCADES ];		// Shadow fade [m] on edge of the cascade
	Float							m_hiResShadowBiasOffsetSlopeMul;		// HiRes Shadow slope offset multiplier
	Float							m_hiResShadowBiasOffsetConst;			// HiRes Shadow const bias
	Float							m_hiResShadowTexelRadius;				// HiRes Shadow texel radius
	Float							m_hiResShadowMaxExtents;				// HiRes Shadow max extents

	//dex++: terrain shadows data
	Float							m_terrainShadowsDistance;				// Maximum rendering distance for terrain shadows ( 0.0f if disabled = the default )
	Float							m_terrainShadowsFadeRange;				// Range over which the terrain shadows fade away
	Float							m_terrainMeshShadowDistance;			// Distance at which the terrain shadows casted from meshes are drawn ( 0.0f if disabled = the default )
	Float							m_terrainMeshShadowFadeRange;			// Fade range for the mesh shadows
	Float							m_terrainShadowsBaseSmoothing;			//!< Terrain shadows base smoothing
	Float							m_terrainShadowsTerrainDistanceSoftness;//!< Controlls how much the terrain shadows got smoothed over distance
	Float							m_terrainShadowsMeshDistanceSoftness;	//!< Controlls how much the mesh shadows got smoothed over distance
	//dex--
	
	Bool							m_multiplanePresent;
	Bool							m_enableFPSDisplay;

//private: // this is made public to be able to prefetch
	Bool							m_renderingMask[ SHOW_MAX_INDEX ];	// Rendering mask

	Bool							m_allowSequentialCapture;				//!< Allow sequential capturing

public:
	//! Get the 2D matrix
	RED_INLINE const Matrix& GetCanvasToWorld() const { return m_canvasToWorld; }

	//! Get shadow distance
	RED_INLINE Float GetCascadeDistance( Uint32 cascadeIndex ) const { ASSERT( cascadeIndex < m_requestedNumCascades ); ASSERT( !(cascadeIndex > 0 && m_cascadeEndDistances[cascadeIndex-1] >= m_cascadeEndDistances[cascadeIndex]) ); return m_cascadeEndDistances[cascadeIndex]; }

	//! Is SHOW flag turned on
	RED_INLINE Bool IsShowFlagOn( EShowFlags flagIndex ) const { return m_renderingMask[ flagIndex ]; }

	//! Change SHOW flag value
	RED_INLINE void SetShowFlag( EShowFlags flagIndex, Bool enable ) { m_renderingMask[ flagIndex ] = enable; }

	//! Is clouds shadow visible
	bool IsCloudsShadowVisible() const;

	//! Viewport properties
	RED_INLINE Bool IsViewportPresent() const { return m_viewport != NULL; }
	RED_INLINE void SetViewport( IViewport* viewport ) { m_viewport = viewport; }	
	Float GetRenderingDebugOption( EVisualDebugCommonOptions option  ) const;
	Bool IsClassRenderingDisabled( CClass * type ) const;
	Bool IsTemplateRenderingDisabled( const CEntityTemplate * entTemplate, const CClass* componentClass ) const;
	Bool IsTerrainToolStampVisible() const;
	Bool IsGrassMaskPaintMode() const;	

public:
	//! Init base frameInfo for rendering envprobes
	static CRenderFrameInfo BuildEnvProbeFaceRenderInfoBase( const CRenderFrameInfo &originalFrameInfo );

	//! Adapt frameInfo for rendering envprobes
	//* Can be called multiple times with different parameters. Initial frameInfo should be created with BuildEnvProbeFaceRenderInfoBase. */
	void AdaptEnvProbeFaceRenderInfo( const CRenderCamera &camera, Uint32 width, Uint32 height );

public:
	CRenderFrameInfo();
	explicit CRenderFrameInfo( IViewport* viewport, Bool instantAdaptation = true, Bool instantDissolve = true );
	explicit CRenderFrameInfo( Uint32 width, Uint32 height, ERenderingMode mode, const CRenderCamera& camera, EShowFlags singleShowFlag );
	explicit CRenderFrameInfo( Uint32 width, Uint32 height, ERenderingMode mode, const EShowFlags* negativeEndedMask, const CRenderCamera& camera );

	// Bind gameplay render target
	void SetGameplayRenderTarget( class IRenderGameplayRenderTarget* renderTarget );

	//dex++: Initialize frame shadow info from world shadow info
	void SetShadowConfig( const class CWorldShadowConfig& params );
	//dex--
	
	// Set day point environment parameters
	void SetDayPointEnvParams( const SDayPointEnvironmentParams& params );

	// Set area environment parameters
	void SetAreaEnvParams( const CAreaEnvironmentParamsAtPoint& params );

	// Set game environment parameters
	void SetGameEnvParams( const CGameEnvironmentParams& params );

	// Set base lighting
	void SetBaseLightingParams( const CRenderBaseLightParams& params );
		
	// Set speedtree parameters
	void SetSpeedTreeParams( const SGlobalSpeedTreeParameters& params );

	// Set world render settings
	void SetWorldRenderSettings( const SWorldRenderSettings& params );

	// Set world render settings
	void SetCameraLightModifiers( const SCameraLightsModifiersSetup& params );

	// Set world time at which this frame was generated
	void SetFrameTime( Float frameWorldTime );

	// Set true engine time at which this frame was generated
	void SetEngineTime( Float engineTime ){ m_engineTime = engineTime; }

	// Set clean engine time at which this frame was generated
	void SetCleanEngineTime( Float cleanEngineTime ){ m_cleanEngineTime = cleanEngineTime; }

	// Set game time
	void SetGameTime( Float gameTime, Uint32 gameDays );
	
	// Calculate rendering scale for object that should maintain fixed screen space size
	Float CalcScreenSpaceScale( const Vector& centerPoint ) const;

	// Enable one frame instant adaptation
	void SetInstantAdaptation( bool enable ) { m_instantAdaptation = enable; }

	// Project points to screen space ( returns XY position in pixel coordinates, Z normalize, W=0 if point is outside the screen )
	void ProjectPoints( const Vector* points, Vector* screenSpacePoints, Uint32 count ) const;

	// Update camera parameters stored in this frame
	void RefreshCameraParams();

	// Update camera parameters based on env near/far planes
	void UpdateEnvCameraParams();

	// Set material debug mode
	void SetMaterialDebugMode( EMaterialDebugMode mdm );

	// Set subpixel offset for both camera and occlusionCamera
	void SetSubpixelOffset( Float x, Float y, Uint32 screenW, Uint32 screenH );
	
	// Used for screenshot generation - do not use in normal rendering or anything, totally unsafe
	void SetSize( Uint32 width, Uint32 height );

	// Update internal matrices. Should be called after setting camera parameters or changing viewport size
	void UpdateMatrices();

private:

	void ResetCommonSettings();

};
