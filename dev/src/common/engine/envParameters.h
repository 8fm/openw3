/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "environmentAreaParams.h"
#include "windParameters.h"
#include "renderResource.h"
#include "cameraDirector.h"

class CParticleComponent;
class CEntity;

/// Area environment registration id
typedef Int32 TEnvManagerAreaEnvId;

/// Time-based blending state
enum EEnvManagerTimeBlendState
{
	ETBS_Disabled=0,		/* No time-based blending */
	ETBS_BlendIn,			/* Blending in */
	ETBS_BlendOut			/* Blending out */
};

/// Area environment data
struct SEnvManagerAreaEnvData
{
	SEnvManagerAreaEnvData ()
		: areaEnv ( EnvResetMode_CurvesEmpty )
		, id ( -1 )
		, priority( 1 )
		, timeBlendState( ETBS_Disabled )
		, timeBlendFactor( 1.0f )
	{}

	CAreaEnvironmentParams						areaEnv;
	THandle< class CAreaEnvironmentComponent >	areaComponent;
	TEnvManagerAreaEnvId						id;
	Int32										priority;
	Float										blendFactor;
	Float										distanceFromEdge;
	EEnvManagerTimeBlendState					timeBlendState;
	Float										timeBlendFactor;
	Float										timeBlendStart;
	Float										timeBlendTarget;
	Float										originalBlendFactor;
	String										pathToEnvDefinition;
#ifndef NO_DEBUG_WINDOWS
	Float										appliedBlendFactor;			/* shown in environment editor */
	Float										appliedMostImportantFactor;	/* shown in environment editor (-1 if not one of two most important) */
#endif
	THandle< CParticleComponent >				particleSystemRainDrops;
	THandle< CParticleComponent >				particleSystemRainSplashes;
	THandle< CEntity >							dynamicEntity;
};

// Character wind params
struct SCharacterWindParams
{
public:
	DECLARE_RTTI_STRUCT( SCharacterWindParams );

	SCharacterWindParams();

	Float				m_primaryDensity;
	Float				m_secondaryDensity;
	Float				m_primaryOscilationFrequency;
	Float				m_secondaryOscilationFrequency;
	Float				m_primaryDistance;
	Float				m_secondaryDistance;
	Float				m_gustFrequency;
	Float				m_gustDistance;
};

BEGIN_NODEFAULT_CLASS_RTTI( SCharacterWindParams );
PROPERTY_EDIT( m_primaryDensity, TXT("m_primaryDensity") );
PROPERTY_EDIT( m_secondaryDensity, TXT("m_secondaryDensity") );
PROPERTY_EDIT( m_primaryOscilationFrequency, TXT("m_primaryOscilationFrequency") );
PROPERTY_EDIT( m_secondaryOscilationFrequency, TXT("m_secondaryOscilationFrequency") );
PROPERTY_EDIT( m_primaryDistance, TXT("m_primaryDistance") );
PROPERTY_EDIT( m_secondaryDistance, TXT("m_secondaryDistance") );
PROPERTY_EDIT( m_gustFrequency, TXT("m_gustFrequency") );
PROPERTY_EDIT( m_gustDistance, TXT("m_gustDistance") );
END_CLASS_RTTI();


/// Global lighting
class CRenderBaseLightParams
{
public:
	Vector				m_lightDirection;						//!< Direction of sun light
	Vector				m_sunDirection;							//!< Direction of sun
	Vector				m_moonDirection;						//!< Direction of moon
	Vector				m_sunLightDiffuse;						//!< Sun light diffuse
	Vector				m_sunLightDiffuseLightSide;				//!< Sun light diffuse light side
	Vector				m_sunLightDiffuseLightOppositeSide;		//!< Sun light diffuse light opposite side
	Bool				m_hdrAdaptationDisabled;				//!< HDR disabled

public:
	CRenderBaseLightParams();
	~CRenderBaseLightParams();
};

/// SDayPointEnvironmentParams
struct SDayPointEnvironmentParams
{
public:
	
	Vector						m_globalLightDirection;
	Vector						m_sunDirection;
	Vector						m_moonDirection;
	
	CWindParameters				m_windParameters;	

	CRenderResourceSmartPtr		m_cloudsShadowTexture;
	CRenderResourceSmartPtr		m_fakeCloudsShadowTexture;
	Float						m_fakeCloudsShadowSize;
	Float						m_fakeCloudsShadowSpeed;
	Float						m_fakeCloudsShadowCurrentTextureIndex;
	Float						m_fakeCloudsShadowTargetTextureIndex;

	CRenderResourceSmartPtr		m_vignetteTexture;
	CRenderResourceSmartPtr		m_bestfitNormalsTexture;
	CRenderResourceSmartPtr		m_interiorFallbackAmbientTexture;
	CRenderResourceSmartPtr		m_interiorFallbackReflectionTexture;
	CRenderResourceSmartPtr		m_bokehTexture;
	CRenderResourceSmartPtr		m_cameraDirtTexture;
	Float						m_cameraDirtNumVerticalTiles;
	CRenderResourceSmartPtr		m_wetSurfaceTexture;		

	Vector						m_skyBoxWeatherBlend;
	Vector						m_waterShadingParams;	
	Vector						m_waterShadingParamsExtra;
	Vector						m_waterShadingParamsExtra2;
	Float						m_waterFoamIntensity;
	Float						m_underWaterBrightness;
	Float						m_underWaterFogIntensity;
	Float						m_delayedWetSurfaceEffectStrength;
	Float						m_immediateWetSurfaceEffectStrength;
	Float						m_weatherEffectStrength;

	Float						m_gamma;
	Float						m_toneMappingAdaptationSpeedUp;
	Float						m_toneMappingAdaptationSpeedDown;
	Float						m_skyDayAmount;	
	Float						m_convergence;
	
	Float						m_nearPlaneWeightForced;
	
	Bool						m_useMoonForShafts;

	SBokehDofParams				m_bokehDofParams;

public:
	SDayPointEnvironmentParams ();

	/// Reset defaults
	void Reset();

	CRenderResourceSmartPtr		GetBestFitNormalsTexture() const;
};


/// Motion blur settings
struct SWorldMotionBlurSettings
{
	DECLARE_RTTI_STRUCT( SWorldMotionBlurSettings )

	Bool								m_isPostTonemapping;
	Float								m_distanceNear;
	Float								m_distanceRange;
	Float								m_strengthNear;
	Float								m_strengthFar;
	Float								m_fullBlendOverPixels;
	Float								m_standoutDistanceNear;
	Float								m_standoutDistanceRange;
	Float								m_standoutAmountNear;
	Float								m_standoutAmountFar;
	Float								m_sharpenAmount;

	SWorldMotionBlurSettings();
};

BEGIN_CLASS_RTTI( SWorldMotionBlurSettings );  
PROPERTY_EDIT(        m_isPostTonemapping,		TXT("m_isPostTonemapping") );
PROPERTY_EDIT(        m_distanceNear,			TXT("m_distanceNear") );
PROPERTY_EDIT(        m_distanceRange,			TXT("m_distanceRange") );
PROPERTY_EDIT(        m_strengthNear,			TXT("m_strengthNear") );
PROPERTY_EDIT(        m_strengthFar,			TXT("m_strengthFar") );
PROPERTY_EDIT(        m_fullBlendOverPixels,	TXT("m_fullBlendOverPixels") );
PROPERTY_EDIT(        m_standoutDistanceNear,	TXT("m_standoutDistanceNear") );
PROPERTY_EDIT(        m_standoutDistanceRange,	TXT("m_standoutDistanceRange") );
PROPERTY_EDIT(        m_standoutAmountNear,		TXT("m_standoutAmountNear") );
PROPERTY_EDIT(        m_standoutAmountFar,		TXT("m_standoutAmountFar") );
PROPERTY_EDIT(        m_sharpenAmount,			TXT("m_sharpenAmount") );	
END_CLASS_RTTI();

/// World render settings
struct SWorldRenderSettings
{
	DECLARE_RTTI_STRUCT( SWorldRenderSettings )

	Float								m_cameraNearPlane;
	Float								m_cameraFarPlane;
	Bool								m_ssaoBlurEnable;
	Bool								m_ssaoNormalsEnable;
	Float								m_envProbeSecondAmbientFilterSize;
	Float								m_fakeCloudsShadowSize;
	Float								m_fakeCloudsShadowSpeed;
	THandle< CTextureArray >			m_fakeCloudsShadowTexture;
	Uint32								m_bloomLevelsRange;
	Uint32								m_bloomLevelsOffset;
	Float								m_bloomScaleConst;
	Float								m_bloomDownscaleDivBase;
	Float								m_bloomDownscaleDivExp;
	Float								m_bloomLevelScale0;
	Float								m_bloomLevelScale1;
	Float								m_bloomLevelScale2;
	Float								m_bloomLevelScale3;
	Float								m_bloomLevelScale4;
	Float								m_bloomLevelScale5;
	Float								m_bloomLevelScale6;
	Float								m_bloomLevelScale7;
	Float								m_bloomPrecision;
	Uint32								m_shaftsLevelIndex;
	Float								m_shaftsIntensity;
	Float								m_shaftsThresholdsScale;
	Float								m_fresnelScaleLights;
	Float								m_fresnelScaleEnvProbes;
	Float								m_fresnelRoughnessShape;
	Float								m_interiorDimmerAmbientLevel;
	Float								m_interiorVolumeSmoothExtent;
	Float								m_interiorVolumeSmoothRemovalRange;
	Float								m_interiorVolumesFadeStartDist;
	Float								m_interiorVolumesFadeRange;
	Float								m_interiorVolumesFadeEncodeRange;
	Float								m_distantLightStartDistance;
	Float								m_distantLightFadeDistance;
	Float								m_globalFlaresTransparencyThreshold;
	Float								m_globalFlaresTransparencyRange;
	Float								m_chromaticAberrationStart;
	Float								m_chromaticAberrationRange;
	SWorldMotionBlurSettings			m_motionBlurSettings;
	Float								m_interiorFallbackReflectionThresholdLow;
	Float								m_interiorFallbackReflectionThresholdHigh;
	Float								m_interiorFallbackReflectionBlendLow;
	Float								m_interiorFallbackReflectionBlendHigh;
	Bool								m_enableEnvProbeLights;
	
	SWorldRenderSettings()
		: m_cameraNearPlane( 0.4f )
		, m_cameraFarPlane( 8000.f )
		, m_ssaoBlurEnable( true )
		, m_ssaoNormalsEnable( true )
		, m_envProbeSecondAmbientFilterSize( 0.75 )
		, m_fakeCloudsShadowSize( 0.01f )
		, m_fakeCloudsShadowSpeed( 0.01f )
		, m_fakeCloudsShadowTexture( nullptr )
		, m_bloomLevelsRange( 6 )
		, m_bloomLevelsOffset( 0 )
		, m_bloomScaleConst( 1 )
		, m_bloomDownscaleDivBase( 2 )
		, m_bloomDownscaleDivExp( 1 )
		, m_bloomLevelScale0 ( 1 )
		, m_bloomLevelScale1 ( 1 )
		, m_bloomLevelScale2 ( 1 )
		, m_bloomLevelScale3 ( 1 )
		, m_bloomLevelScale4 ( 1 )
		, m_bloomLevelScale5 ( 1 )
		, m_bloomLevelScale6 ( 1 )
		, m_bloomLevelScale7 ( 1 )
		, m_bloomPrecision( 100 )
		, m_shaftsLevelIndex( 1 )
		, m_shaftsIntensity( 1 )
		, m_shaftsThresholdsScale( 1 )
		, m_fresnelScaleLights( 1 )
		, m_fresnelScaleEnvProbes( 1 )
		, m_fresnelRoughnessShape( 30 )
		, m_interiorDimmerAmbientLevel( 1.f )
		, m_interiorVolumeSmoothExtent( 0.25f )
		, m_interiorVolumeSmoothRemovalRange( 1.f )
		, m_interiorVolumesFadeStartDist( 20.f )
		, m_interiorVolumesFadeRange( 15.f )
		, m_interiorVolumesFadeEncodeRange( 120.f )
		, m_distantLightStartDistance( 50.0f )
		, m_distantLightFadeDistance( 15.0f )
		, m_globalFlaresTransparencyThreshold( 0.1f )
		, m_globalFlaresTransparencyRange( 0.2f )
		, m_chromaticAberrationStart( 0.2f )
		, m_chromaticAberrationRange( 0.8f )
		, m_interiorFallbackReflectionThresholdLow( 0.25f )
		, m_interiorFallbackReflectionThresholdHigh( 0.75f )
		, m_interiorFallbackReflectionBlendLow( 0.2f )
		, m_interiorFallbackReflectionBlendHigh( 0.2f )
		, m_enableEnvProbeLights( true )
	{}
};

BEGIN_CLASS_RTTI( SWorldRenderSettings );        
PROPERTY_EDIT(        m_cameraNearPlane,							TXT("Camera near plane") );
PROPERTY_EDIT(        m_cameraFarPlane,								TXT("Camera far plane") );
PROPERTY_EDIT(        m_ssaoBlurEnable,								TXT("SSAO blur enable") );
PROPERTY_EDIT(        m_ssaoNormalsEnable,							TXT("SSAO normals enable") );
PROPERTY_EDIT(        m_envProbeSecondAmbientFilterSize,			TXT("EnvProbe second ambient filter size - used for smooth surfaces, e.g. skin") );
PROPERTY_EDIT(        m_fakeCloudsShadowSize,						TXT("UV tile size for fake clouds shadow") );
PROPERTY_EDIT(        m_fakeCloudsShadowSpeed,						TXT("Fake clouds shadow size") );
PROPERTY_EDIT(        m_fakeCloudsShadowTexture,					TXT("Fake clouds shadow Texture") );
PROPERTY_EDIT(        m_bloomLevelsRange,							TXT("Bloom levels range") );
PROPERTY_EDIT(        m_bloomLevelsOffset,							TXT("Bloom levels offset") );
PROPERTY_EDIT(        m_bloomScaleConst,							TXT("Bloom scale const") );
PROPERTY_EDIT(        m_bloomDownscaleDivBase,						TXT("Bloom downscale div base") );
PROPERTY_EDIT(        m_bloomDownscaleDivExp,						TXT("Bloom downscale div exp") );
PROPERTY_EDIT(        m_bloomLevelScale0,							TXT("Bloom level scale 0") );
PROPERTY_EDIT(        m_bloomLevelScale1,							TXT("Bloom level scale 1") );
PROPERTY_EDIT(        m_bloomLevelScale2,							TXT("Bloom level scale 2") );
PROPERTY_EDIT(        m_bloomLevelScale3,							TXT("Bloom level scale 3") );
PROPERTY_EDIT(        m_bloomLevelScale4,							TXT("Bloom level scale 4") );
PROPERTY_EDIT(        m_bloomLevelScale5,							TXT("Bloom level scale 5") );
PROPERTY_EDIT(        m_bloomLevelScale6,							TXT("Bloom level scale 6") );
PROPERTY_EDIT(        m_bloomLevelScale7,							TXT("Bloom level scale 7") );
PROPERTY_EDIT(        m_bloomPrecision,								TXT("Bloom precision") );
PROPERTY_EDIT(        m_shaftsLevelIndex,							TXT("Shafts level index") );
PROPERTY_EDIT(        m_shaftsIntensity,							TXT("Shafts intensity") );
PROPERTY_EDIT(        m_shaftsThresholdsScale,						TXT("Shafts thresholds scale") );
PROPERTY_EDIT(        m_fresnelScaleLights,							TXT("Fresnel scale lights") );
PROPERTY_EDIT(        m_fresnelScaleEnvProbes,						TXT("Fresnel scale envProbes") );
PROPERTY_EDIT(        m_fresnelRoughnessShape,						TXT("Fresnel rougness shape") );
PROPERTY_EDIT(        m_interiorDimmerAmbientLevel,					TXT("Interior dimmer ambient level") );
PROPERTY_EDIT(        m_interiorVolumeSmoothExtent,					TXT("Interior volume smooth extent") );
PROPERTY_EDIT(        m_interiorVolumeSmoothRemovalRange,			TXT("Interior volume smooth removal range") );
PROPERTY_EDIT(        m_interiorVolumesFadeStartDist,				TXT("Interior volume fade start dist") );
PROPERTY_EDIT(        m_interiorVolumesFadeRange,					TXT("Interior volume fade range") );
PROPERTY_EDIT(        m_interiorVolumesFadeEncodeRange,				TXT("Interior volume encode range") );
PROPERTY_EDIT(        m_distantLightStartDistance,					TXT("Distance on wich all light becone distant lights") );
PROPERTY_EDIT(        m_distantLightFadeDistance,					TXT("Distance that affect blending regular lights into distant ones") );
PROPERTY_EDIT(        m_globalFlaresTransparencyThreshold,			TXT("Global flares transparency threshold") );
PROPERTY_EDIT(        m_globalFlaresTransparencyRange,				TXT("Global flares transparency range") );
PROPERTY_EDIT(        m_motionBlurSettings,							TXT("Motion blur settings") );
PROPERTY_EDIT(        m_chromaticAberrationStart,					TXT("Chromatic aberration start") );
PROPERTY_EDIT(        m_chromaticAberrationRange,					TXT("Chromatic aberration range") );
PROPERTY_EDIT(        m_interiorFallbackReflectionThresholdLow,		TXT("Interior fallback reflection threshold low") );
PROPERTY_EDIT(        m_interiorFallbackReflectionThresholdHigh,	TXT("Interior fallback reflection threshold high") );
PROPERTY_EDIT(        m_interiorFallbackReflectionBlendLow,			TXT("Interior fallback reflection blend low") );
PROPERTY_EDIT(        m_interiorFallbackReflectionBlendHigh,		TXT("Interior fallback reflection blend high") );
PROPERTY_EDIT(        m_enableEnvProbeLights,						TXT("Enable envprobe lights") );
END_CLASS_RTTI();


/// SGlobalSpeedTreeParameters
struct SGlobalSpeedTreeParameters
{
	DECLARE_RTTI_STRUCT( SGlobalSpeedTreeParameters )

	Float m_alphaScalar3d;
	Float m_alphaScalarGrassNear;
	Float m_alphaScalarGrass;
	Float m_alphaScalarGrassDistNear;
	Float m_alphaScalarGrassDistFar;
	Float m_alphaScalarBillboards;
	Float m_billboardsGrainBias;
	Float m_billboardsGrainAlbedoScale;
	Float m_billboardsGrainNormalScale;
	Float m_billboardsGrainClipScale;
	Float m_billboardsGrainClipBias;
	Float m_billboardsGrainClipDamping;
	Float m_grassNormalsVariation;

	SGlobalSpeedTreeParameters ()
		: m_alphaScalar3d ( 1.f )
		, m_alphaScalarGrassNear ( 1.f )
		, m_alphaScalarGrass ( 1.f )
		, m_alphaScalarGrassDistNear ( 0.f )
		, m_alphaScalarGrassDistFar ( 0.f )
		, m_alphaScalarBillboards ( 0.33f )
		, m_billboardsGrainBias ( -1.f )
		, m_billboardsGrainAlbedoScale ( 0.15f )
		, m_billboardsGrainNormalScale ( 0.f )
		, m_billboardsGrainClipScale ( 0.f )
		, m_billboardsGrainClipBias ( 0.f )
		, m_billboardsGrainClipDamping ( 0.f )
		, m_grassNormalsVariation ( 0.f )
	{}

	void ImportGlobalOverrides();
};

BEGIN_CLASS_RTTI( SGlobalSpeedTreeParameters );
PROPERTY_EDIT(        m_alphaScalar3d,								TXT("Alpha Scalar 3d") );
PROPERTY_EDIT(        m_alphaScalarGrassNear,						TXT("Alpha Scalar Grass Near Value") );
PROPERTY_EDIT(        m_alphaScalarGrass,							TXT("Alpha Scalar Grass Far Value") );
PROPERTY_EDIT(        m_alphaScalarGrassDistNear,					TXT("Alpha Scalar Grass Near Distance") );
PROPERTY_EDIT(        m_alphaScalarGrassDistFar,					TXT("Alpha Scalar Grass Far Distance") );
PROPERTY_EDIT(        m_alphaScalarBillboards,						TXT("Alpha Scalar Billboards") );
PROPERTY_EDIT(        m_billboardsGrainBias,						TXT("Billboards Grain Bias") );
PROPERTY_EDIT(        m_billboardsGrainAlbedoScale,					TXT("Billboards Grain Albedo Scale") );
PROPERTY_EDIT(        m_billboardsGrainNormalScale,					TXT("Billboards Grain Normal Scale") );
PROPERTY_EDIT(        m_billboardsGrainClipScale,					TXT("Billboards Grain Clip Scale") );
PROPERTY_EDIT(        m_billboardsGrainClipBias,					TXT("Billboards Grain Clip Bias") );
PROPERTY_EDIT(        m_billboardsGrainClipDamping,					TXT("Billboards Grain Clip Damping") );
PROPERTY_EDIT(        m_grassNormalsVariation,						TXT("Grass normals variation") );
END_CLASS_RTTI();
