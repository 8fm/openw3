/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "textureGroup.h"
#include "inGameConfig.h"
#include "inGameConfigInterface.h"
#include "inGameConfigSpecialization.h"
#include "drawableComponent.h"
#include "../core/configVar.h"

namespace Config
{
	/***** Textures configs *****/
	extern TConfigVar<Int32> cvMaxResidentMipMap;
	extern TConfigVar<Int32> cvMaxTextureSize;
	extern TConfigVar<Int32> cvMaxAtlasTextureSize;
	extern TConfigVar<Int32> cvMaxCubeShadowSize;
	extern TConfigVar<Int32> cvMaxSpotShadowSize;
	extern TConfigVar<Int32> cvMaxTextureAnisotropy;

	extern TConfigVar<Int32> cvTextureDownscale;
	extern TConfigVar<Int32> cvDetailTextureDownscale;
	extern TConfigVar<Int32> cvAtlasTextureDownscale;
	extern TConfigVar<Int32> cvTextureMemoryBudget;
	extern TConfigVar<Int32> cvTextureMemoryGUIBudget;
	extern TConfigVar<Int32> cvTextureTimeBudget;
	extern TConfigVar<Int32> cvTextureInFlightBudget;
	extern TConfigVar<Int32> cvTextureInFlightCountBudget;
	extern TConfigVar<Float> cvScaleformTextureUnstreamDelay;

	extern TConfigVar<Float> cvDecalsChance;
	extern TConfigVar<Float> cvDecalsSpawnDistanceCutoff;

	// texture streaming globals
	extern TConfigVar<Float> cvTextureStreamingDistanceLimitSq;
	extern TConfigVar<Float> cvTextureStreamingCharacterDistanceLimitSq;
	extern TConfigVar<Float> cvTextureStreamingHeadsDistanceLimitSq;
	extern TConfigVar<Bool> cvTextureStreamingReduceGameplayLOD;

	// new
	extern TConfigVar<Int32> cvUberSampling;
	extern TConfigVar<Float> cvTextureMipBias;
	extern TConfigVar<Int32> cvMsaaLevel;
	extern TConfigVar<Int32> cvDynamicDecalsLimit;
	extern TConfigVar<Int32> cvDynamicDecalsChunkLimit;
	extern TConfigVar<Float> cvMeshRenderingDistanceScale;
	extern TConfigVar<Float> cvMeshLODDistanceScale;
	extern TConfigVar<Float> cvTerrainScreenSpaceErrorThresholdNear;
	extern TConfigVar<Float> cvTerrainScreenSpaceErrorThresholdFar;
	extern TConfigVar<Float> cvTerrainErrorMetricMultiplier;
	extern TConfigVar<Bool> cvForceInstantAdaptation;
	extern TConfigVar<Bool> cvForceInstantEnvProbeUpdate;
	extern TConfigVar<Bool> cvEnableTemporalAA;
	extern TConfigVar<Bool> cvHighPrecisionRendering;
	extern TConfigVar<Float, Validation::FloatRange<1, 100, 10>> cvCascadeShadowFadeTreshold;		// Range in 0.1f..10.0f
	extern TConfigVar<Float> cvCascadeShadowDistanceScale0;		
	extern TConfigVar<Float> cvCascadeShadowDistanceScale1;		
	extern TConfigVar<Float> cvCascadeShadowDistanceScale2;		
	extern TConfigVar<Float> cvCascadeShadowDistanceScale3;		
	extern TConfigVar<Int32, Validation::IntRange<256, 8192>> cvCascadeShadowmapSize;
	extern TConfigVar<Int32> cvCascadeShadowQuality;
	extern TConfigVar<Int32, Validation::IntRange<1, MAX_CASCADES>> cvMaxCascadeCount;
	extern TConfigVar<Int32, Validation::IntRange<1, 5>> cvMaxTerrainShadowAtlasCount;
	extern TConfigVar<Int32> cvGlobalOceanTesselationFactor;
	extern TConfigVar<Bool> cvUseDynamicWaterSimulation;
	extern TConfigVar<Int32> cvMaxEmittersCount;
	extern TConfigVar<Int32> cvMaxParticlesCount;
	extern TConfigVar<Int32> cvMaxParticleMeshChunksCount;
	extern TConfigVar<Int32> cvMaxParticleTriangleCount;
	extern TConfigVar<Int32> cvMaxMeshTrianglesCount;
	extern TConfigVar<Int32> cvMaxMeshChunksCount;
	extern TConfigVar<Int32> cvVisibleObjectsBudget;
	extern TConfigVar<Int32> cvMaxMeshesSizeBudget;
	extern TConfigVar<Int32> cvOcclusionQueryAdditionalMemory;
	extern TConfigVar<Int32> cvHairWorksLevel;
	extern TConfigVar<Int32> cvHairWorksAALevel;
	extern TConfigVar<Int32> cvHairWorksGlobalStrandSmoothness;
	extern TConfigVar<Float> cvHairWorksGlobalDensityLimit;
	extern TConfigVar<Float> cvHairWorksGlobalDensityQuality;
	extern TConfigVar<Float> cvHairWorksGlobalDetailLODFactor;
	extern TConfigVar<Float> cvHairWorksGlobalWidthLimit;
	extern TConfigVar<Float> cvTerrainReadAheadDistance;
	extern TConfigVar<Int32> cvTerrainTileMipDataBudget;
	extern TConfigVar<Float> cvTerrainTileTimeout;
	extern TConfigVar<Float> cvTerrainTileLoadingTimeout;
	extern TConfigVar<Float> cvTerrainTileMinTimeout;
	extern TConfigVar<Int32> cvForcedDebugPreviewIndex;
	extern TConfigVar<Bool>  cvForcedRendererResolution;
	extern TConfigVar<Bool>  cvForcedRendererOverlayResolution;
	extern TConfigVar<Bool>  cvForcedRendererBackBufferResolution;
	extern TConfigVar<Int32> cvForcedRendererResolutionWidth;
	extern TConfigVar<Int32> cvForcedRendererResolutionHeight;
	extern TConfigVar< Int32, Validation::IntRange<0, 24> > cvFoliageMaxVisibilityDepth;
	extern TConfigVar< Float, Validation::FloatRange< 0, 500, 100 > > cvFoliageDistanceScale;
	extern TConfigVar< Float, Validation::FloatRange< 0, 500, 100 > > cvGrassDistanceScale;

	/***** Post process configs *****/
	extern TConfigVar<Bool> cvAllowBloom;		
	extern TConfigVar<Bool> cvAllowShafts;		
	extern TConfigVar<Int32> cvShaftsHeightLimitQ0;
	extern TConfigVar<Int32> cvShaftsHeightLimitQ1;
	extern TConfigVar<Bool> cvAllowAntialias;
	extern TConfigVar<Bool> cvAllowMSAA;		
	extern TConfigVar<Bool> cvAllowBlur;		
	extern TConfigVar<Bool> cvAllowDOF;			
	extern TConfigVar<Bool> cvAllowCutsceneDOF;
	extern TConfigVar<Bool> cvAllowVignette;	
	extern TConfigVar<Int32> cvAllowSharpen;
	extern TConfigVar<Int32> cvSSAOVersion;
	extern TConfigVar<Bool> cvAllowMotionBlur;
	extern TConfigVar<Float> cvMotionBlurPositionTreshold;
	extern TConfigVar<Float> cvMotionBlurRotationTreshold;
	extern TConfigVar<Bool> cvAllowFog;			
	extern TConfigVar<Bool> cvAllowUnderwater;
	extern TConfigVar<Bool> cvAllowChromaticAberration;			

	/***** Viewport *****/
	extern TConfigVar<String>	cvResolution;
	extern TConfigVar<Int32>	cvFullScreenMode;
	extern TConfigVar<Bool>		cvVSync;
	extern TConfigVar<Int32>	cvVSyncThreshold;

	/***** Level of Detail *****/
	extern TConfigVar<Int32> cvMeshLodGameplayDownscale;
	extern TConfigVar<Int32> cvCharacterLodGameplayDownscale;

	extern TConfigVar<Float> cvDecalsHideDistance;
	extern TConfigVar<Float> cvDynDecalsHideDistance;
	extern TConfigVar<Float> cvStripeHideDistance;
	extern TConfigVar<Float> cvSwarmHideDistance;
	extern TConfigVar<Float> cvDimmerHideDistance;


	/***** Other *****/
	extern TConfigVar<Float> cvGamma;
	extern TConfigVar<Float> cvInventoryFixedLuminance;
	extern TConfigVar<Float> cvInventoryBgColorScaleR;
	extern TConfigVar<Float> cvInventoryBgColorScaleG;
	extern TConfigVar<Float> cvInventoryBgColorScaleB;
	extern TConfigVar<Bool> cvAllowClothSimulationOnGpu;

	/***** Budgets *****/
	extern 	TConfigVar<Int32> cvMaxAllowedApexTicked;
	extern 	TConfigVar<Int32> cvMaxAllowedApexDestroTicked;
	extern 	TConfigVar<Int32> cvMaxAllowedLightsShadow;
	extern 	TConfigVar<Int32> cvMaxAllowedLightsNonShadows;
	extern 	TConfigVar<Int32> cvMaxAllowedDecalsSS;
	extern 	TConfigVar<Int32> cvMaxAllowedDecalsDynamic;
	extern 	TConfigVar<Int32> cvMaxAllowedParticlesCount;
	extern 	TConfigVar<Int32> cvMaxAllowedActiveEnvProbes;
	extern 	TConfigVar<Int32> cvMaxAllowedHiresChunks;
	extern 	TConfigVar<Int32> cvMaxAllowedSpeedTree;
	extern 	TConfigVar<Int32> cvMaxAllowedGrass;
	extern 	TConfigVar<Int32> cvMaxAllowedChunksStatic;
	extern 	TConfigVar<Int32> cvMaxAllowedTrianglesStatic;
	extern 	TConfigVar<Int32> cvMaxAllowedChunksSkinned;
	extern 	TConfigVar<Int32> cvMaxAllowedTrianglesSkinned;
	extern 	TConfigVar<Int32> cvMaxAllowedDynMeshes;
	extern 	TConfigVar<Int32> cvMaxAllowedStatMeshes;
	extern 	TConfigVar<Int32> cvMaxAllowedCharTextures;
	extern 	TConfigVar<Int32> cvMaxAllowedStatTextures;
	extern 	TConfigVar<Float> cvMaxAllowedApexTickedTime;
	extern 	TConfigVar<Float> cvMaxAllowedApexDestroTickedTime;
	extern 	TConfigVar<Float> cvMaxAllowedLightsShadowTime;
	extern 	TConfigVar<Float> cvMaxAllowedLightsNonShadowsTime;
	extern 	TConfigVar<Float> cvMaxAllowedDecalsSSTime;
	extern 	TConfigVar<Float> cvMaxAllowedDecalsDynamicTime;
	extern 	TConfigVar<Float> cvMaxAllowedParticlesCountTime;
	extern 	TConfigVar<Float> cvMaxAllowedActiveEnvProbesTime;
	extern 	TConfigVar<Float> cvMaxAllowedHiresChunksTime;
	extern 	TConfigVar<Float> cvMaxAllowedSpeedTreeTime;
	extern 	TConfigVar<Float> cvMaxAllowedGrassTime;
	extern 	TConfigVar<Float> cvMaxAllowedChunksStaticTime;
	extern 	TConfigVar<Float> cvMaxAllowedTrianglesStaticTime;
	extern 	TConfigVar<Float> cvMaxAllowedChunksSkinnedTime;
	extern 	TConfigVar<Float> cvMaxAllowedTrianglesSkinnedTime;

	/***** Screenshots and movie grabbing *****/
	extern TConfigVar<Float> cvMovieFramerate;
	extern TConfigVar<Bool>  cvMovieUbersampling;

	namespace Helper
	{
		Uint32 GetCurrentOutputMonitorConfig();
		//! Convert "1600x900" to 1600 and 900. This call always returns proper value.
		void ExtractDisplayModeValues( const String& mode, Int32& width, Int32& height );

		//! Convert 1600 and 900 to "1600x900"
		RED_INLINE String PackDisplayModeValues( Int32 width, Int32 height )
		{
			return String::Printf( TXT("%ix%i"), width, height );
		}

		//! List all currently available screen modes
		void ListAvailableDisplayModes( TDynArray<String>& outScreenModes );
		Bool GetBestMatchingResolution( const Int32 width, const Int32 height, Int32& outWidth, Int32& outHeight );
	}
}

/// Map of texture groups
typedef THashMap< CName, TextureGroup >	TTextureGroupsMap;

/// Manager of renderer settings
class CRenderSettingsManager
{
protected:
	TTextureGroupsMap	m_textureGroups;	//!< Texture groups

public:
	//! Get the texture groups settings
	RED_INLINE const TTextureGroupsMap& GetTextureGroups() const { return m_textureGroups; }

	//! Find texture group settings
	RED_INLINE const TextureGroup* GetTextureGroup( const CName& name ) const { return m_textureGroups.FindPtr( name ); }

public:
	CRenderSettingsManager();
	void LoadTextureGroups();
};

/// Singleton
typedef TSingleton< CRenderSettingsManager > SRenderSettingsManager;
