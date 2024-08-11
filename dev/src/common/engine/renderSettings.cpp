/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderSettings.h"
#include "../core/configFileManager.h"
#include "../core/gatheredResource.h"
#include "../core/xmlReader.h"
#include "../core/depot.h"
#include "renderFrameInfo.h"
#include "inGameConfig.h"

namespace Config
{
	/***** Hack, until some day *****/
	template<typename TType>
	RED_INLINE TType HackyPlatformValue( TType pc, TType durango, TType orbis )
	{
#if defined(RED_PLATFORM_WINPC)
		return pc;
#elif defined(RED_PLATFORM_DURANGO)
		return durango;
#elif defined(RED_PLATFORM_ORBIS)
		return orbis;
#else
		return pc;
#endif
	};

	String PackResolutionAsString( Int32 width, Int32 height )
	{
		return ToString(width) + TXT("x") + ToString(height);
	}

	namespace Consts
	{
		const Int32 minimumResolutionWidth = 1024;
		const Int32 minimumResolutionHeight = 720;
	}

	/***** Render Settings *****/
	TConfigVar<Int32> cvMaxResidentMipMap			( "TextureStreaming", "MaxResidentMipMap",		6,									eConsoleVarFlag_Save );		//!< Maximum resident mip map
	TConfigVar<Int32> cvMaxTextureSize				( "Rendering", "MaxTextureSize",				2048,								eConsoleVarFlag_Save );		//!< Maximum allowed texture size
	TConfigVar<Int32> cvMaxAtlasTextureSize			( "Rendering", "MaxAtlasTextureSize",			2048,								eConsoleVarFlag_Save );		//!< Maximum allowed atlas texture size
	TConfigVar<Int32> cvMaxCubeShadowSize			( "Rendering", "MaxCubeShadowSize",				512,								eConsoleVarFlag_Save );		//!< Size of cubemap for shadows
	TConfigVar<Int32> cvMaxSpotShadowSize			( "Rendering", "MaxSpotShadowSize",				512,								eConsoleVarFlag_Save );		//!< Size of shadowmap for shadows
	TConfigVar<Int32> cvMaxTextureAnisotropy		( "Rendering", "MaxTextureAnizotropy",			HackyPlatformValue(8,2,2),			eConsoleVarFlag_Save );		//!< Max anizotropy level applied for aniso samplers

	TConfigVar<Int32> cvTextureDownscale			( "Rendering", "TextureDownscale",				HackyPlatformValue(1,0,0),			eConsoleVarFlag_Save );		//!< Texture downscale factor			IMPORTANT: for PC it's 1
	TConfigVar<Int32> cvDetailTextureDownscale		( "Rendering", "DetailTextureDownscale",		HackyPlatformValue(0,0,0),			eConsoleVarFlag_Save );		//!< Detail texture downscale factor
	TConfigVar<Int32> cvAtlasTextureDownscale		( "Rendering", "AtlasTextureDownscale",			HackyPlatformValue(1,0,0),			eConsoleVarFlag_Save );		//!< Atlas texture downscale factor		IMPORTANT: for PC it's 1
	TConfigVar<Int32> cvTextureMemoryBudget			( "Rendering", "TextureMemoryBudget",			HackyPlatformValue(300,300,300),	eConsoleVarFlag_Save );		//!< Budget for all textures, in MB		IMPORTANT: for XB1 and PS4 it's 500
	TConfigVar<Int32> cvTextureMemoryGUIBudget		( "Rendering", "TextureMemoryGUIBudget",		80,									eConsoleVarFlag_Save );		//!< Budget for GUI textures, in MB
	TConfigVar<Int32> cvTextureTimeBudget			( "Rendering", "TextureTimeBudget",				HackyPlatformValue(5,10,10),		eConsoleVarFlag_Save );		//!< Budget for streaming, in ms		IMPORTANT: for PC it's 5
	TConfigVar<Int32> cvTextureInFlightBudget		( "Rendering", "TextureInFlightBudget",			64,									eConsoleVarFlag_Save );		//!< Budget for streaming buffers not consumed by the renderer
	TConfigVar<Int32> cvTextureInFlightCountBudget	( "Rendering", "TextureInFlightCountBudget",	12,									eConsoleVarFlag_Save );		//!< Number of active inflight (streaming) textures there can be
	
	TConfigVar<Float> cvScaleformTextureUnstreamDelay( "Rendering", "ScaleformTextureUnstreamDelay",2.5f,								eConsoleVarFlag_Save );		//!< Time after non visible textures will be unstreamed

	TConfigVar<Float> cvDecalsChance				( "Rendering",	 "DecalsChance",				1.0f,								eConsoleVarFlag_Save );		//!< Chance of dynamic decals spawning
	TConfigVar<Float> cvDecalsSpawnDistanceCutoff	( "Rendering",	 "DecalsSpawnDistanceCutoff",	10.0f,								eConsoleVarFlag_Save );		//!< Distance beyond which particles will not spawn decals

	TConfigVar<Float> cvTextureStreamingDistanceLimitSq			( "Rendering", "TextureStreamingDistanceLimitSq",			40000.f,	eConsoleVarFlag_Save );		//!< No textures are streamed that are reported to be used only above this distance
	TConfigVar<Float> cvTextureStreamingCharacterDistanceLimitSq( "Rendering", "TextureStreamingCharacterDistanceLimitSq",	225.f,		eConsoleVarFlag_Save );		//!< No charcter textures are streamed that are reported to be used only above this distance
	TConfigVar<Float> cvTextureStreamingHeadsDistanceLimitSq	( "Rendering", "TextureStreamingHeadsDistanceLimitSq",		100.f,		eConsoleVarFlag_Save );		//!< No head textures are streamed that are reported to be used only above this distance
	TConfigVar<Bool> cvTextureStreamingReduceGameplayLOD		( "Rendering", "TextureStreamingReduceGameplayLOD",			true,		eConsoleVarFlag_Save );		//!< While in gameplay the top level MIP for some textures is dropped from MIP0 to MIP1 to reduce texture usage

	TConfigVar<Int32> cvUberSampling						( "Rendering", "UberSampling",						0,									eConsoleVarFlag_Save );		//!< Uber sampling samples
	TConfigVar<Float> cvTextureMipBias						( "Rendering", "TextureMipBias",					0,									eConsoleVarFlag_Save );		//!< Textures mip bias
	TConfigVar<Int32> cvMsaaLevel							( "Rendering", "MsaaLevel",							0,									eConsoleVarFlag_Save );		//!< MSAA Level
	TConfigVar<Int32> cvDynamicDecalsLimit					( "Rendering", "DynamicDecalsLimit",				HackyPlatformValue(60,60,60),		eConsoleVarFlag_Save );		//!< Dynamic decals limit per-scene
	TConfigVar<Int32> cvDynamicDecalsChunkLimit				( "Rendering", "DynamicDecalsChunkLimit",			HackyPlatformValue(30,30,30),		eConsoleVarFlag_Save );		//!< Dynamic decals (with chunks) limit per-scene
	TConfigVar<Float> cvMeshRenderingDistanceScale			( "Rendering", "MeshRenderingDistanceScale",		1.0f,								eConsoleVarFlag_Save );		//!< Mesh rendering distance scale
	TConfigVar<Float> cvMeshLODDistanceScale				( "Rendering", "MeshLODDistanceScale",				HackyPlatformValue(1.0f,1.0f,1.0f),	eConsoleVarFlag_Save );		//!< Mesh LOD distance scale
	TConfigVar<Float> cvTerrainScreenSpaceErrorThresholdNear( "Rendering", "TerrainScreenSpaceErrorThreshold",	2.0f,								eConsoleVarFlag_Save );		//!< A kind of tesselation factor, the higher the error the less ammount of triangles will the tesselation produce (clip lvl 0)
	TConfigVar<Float> cvTerrainScreenSpaceErrorThresholdFar	( "Rendering", "TerrainScreenSpaceErrorThresholdFar",	5.0f,							eConsoleVarFlag_Save );		//!< A kind of tesselation factor, the higher the error the less ammount of triangles will the tesselation produce (clip lvl >0)
	TConfigVar<Float> cvTerrainErrorMetricMultiplier		( "Rendering", "TerrainErrorMetricMultiplier",		10.0f,								eConsoleVarFlag_Save );
	TConfigVar<Bool>  cvForceInstantAdaptation				( "Rendering", "ForceInstantAdaptation",			false,								eConsoleVarFlag_Save );		//!< Force instant adaptation
	TConfigVar<Bool>  cvForceInstantEnvProbeUpdate			( "Rendering", "ForceInstantEnvProbeUpdate",		false,								eConsoleVarFlag_Save );		//!< Force instant envprobe update
	TConfigVar<Bool>  cvEnableTemporalAA					( "Rendering", "EnableTemporalAA",					false,								eConsoleVarFlag_Save );		//!< Enable temporal AA
	TConfigVar<Bool>  cvHighPrecisionRendering				( "Rendering", "EnableHighPrecision",				false 								                     );		//!< Enable high precision rendering
	TConfigVar<Float, Validation::FloatRange<1, 100, 10>> cvCascadeShadowFadeTreshold	( "Rendering", "CascadeShadowFadeTreshold",	1.0f,			eConsoleVarFlag_Save );		//!< Fade treshold for cascade shadows (1.0f is default)
	TConfigVar<Float> cvCascadeShadowDistanceScale0			( "Rendering", "CascadeShadowDistanceScale0",		1.0f,								eConsoleVarFlag_Save );		//!< 1st cascade extra distance
	TConfigVar<Float> cvCascadeShadowDistanceScale1			( "Rendering", "CascadeShadowDistanceScale1",		1.0f,								eConsoleVarFlag_Save );		//!< 2nd cascade extra distance
	TConfigVar<Float> cvCascadeShadowDistanceScale2			( "Rendering", "CascadeShadowDistanceScale2",		1.0f,								eConsoleVarFlag_Save );		//!< 3rd cascade extra distance
	TConfigVar<Float> cvCascadeShadowDistanceScale3			( "Rendering", "CascadeShadowDistanceScale3",		1.0f,								eConsoleVarFlag_Save );		//!< 4rd cascade extra distance
	TConfigVar<Int32, Validation::IntRange<256, 8192>> cvCascadeShadowmapSize	( "Rendering", "CascadeShadowmapSize",		1024,					eConsoleVarFlag_Save );		//!< Size of shadowmap used for cascades
	TConfigVar<Int32> cvCascadeShadowQuality				( "Rendering", "CascadeShadowQuality",				0,									eConsoleVarFlag_Save );		//!< Quality of the cascade shadows
	TConfigVar<Int32, Validation::IntRange<1, MAX_CASCADES>> cvMaxCascadeCount	( "Rendering", "MaxCascadeCount",			4,						eConsoleVarFlag_Save );		//!< Maximum number of cascades supported for profile
	TConfigVar<Int32, Validation::IntRange<1, 5>> cvMaxTerrainShadowAtlasCount	( "Rendering", "MaxTerrainShadowAtlasCount",3,						eConsoleVarFlag_Save );		//!< Maximum number of textures in the terrain atlas
	TConfigVar<Int32> cvGlobalOceanTesselationFactor		( "Rendering", "GlobalOceanTesselationFactor",		10,									eConsoleVarFlag_Save );		//!< Tesselation factor for ocean shaders
	TConfigVar<Bool>  cvUseDynamicWaterSimulation			( "Rendering", "UseDynamicWaterSimulation",			true,								eConsoleVarFlag_Save );		//!< Use dynamic water ....simulation
	
	TConfigVar<Int32> cvHairWorksLevel						( "Rendering", "HairWorksLevel",					0,									eConsoleVarFlag_Save );		//!< Use nvidia hair works
	TConfigVar<Int32> cvHairWorksAALevel					( "Rendering", "HairWorksAALevel",					4,									eConsoleVarFlag_Save );		//!< HairWorks AA level. If 0, AA is disabled. 4 is default
	TConfigVar<Int32> cvHairWorksGlobalStrandSmoothness		( "Rendering", "HairWorksGlobalStrandSmoothness",	3,									eConsoleVarFlag_Save );		//!< HairWorksGlobalStrandSmoothness default
	TConfigVar<Float> cvHairWorksGlobalDensityLimit			( "Rendering", "HairWorkstGlobalDensityLimit",		2.0f,								eConsoleVarFlag_Save );		//!< HairWorkstGlobalDensityLimit default
	TConfigVar<Float> cvHairWorksGlobalDensityQuality		( "Rendering", "HairWorksGlobalDensityQuality",		1.0f,								eConsoleVarFlag_Save );		//!< HairWorksGlobalDensityQuality default
	TConfigVar<Float> cvHairWorksGlobalDetailLODFactor		( "Rendering", "HairWorksGlobalDetailLODFactor",	1.0f,								eConsoleVarFlag_Save );		//!< HairWorksGlobalDetailLODFactor default
	TConfigVar<Float> cvHairWorksGlobalWidthLimit			( "Rendering", "HairWorksGlobalWidthLimit",			5.0f,								eConsoleVarFlag_Save );		//!< HairWorksGlobalWidthLimit default

	TConfigVar<Float> cvTerrainReadAheadDistance			( "Rendering", "TerrainReadAheadDistance",			20.0f,								eConsoleVarFlag_Save );		//!< Distance to look ahead for async terrain loading
	TConfigVar<Int32> cvForcedDebugPreviewIndex				( "Rendering", "ForcedDebugPreviewIndex",			-1,									eConsoleVarFlag_Save );		//!< Forced debug preview index
	TConfigVar<Bool>  cvForcedRendererResolution			( "Rendering", "ForcedRendererResolution",			HackyPlatformValue( false, true, false), eConsoleVarFlag_ReadOnly );	
	TConfigVar<Bool>  cvForcedRendererOverlayResolution		( "Rendering", "ForcedRendererOverlayResolution",	HackyPlatformValue( false, false,false), eConsoleVarFlag_ReadOnly );	
	TConfigVar<Bool>  cvForcedRendererBackBufferResolution	( "Rendering", "ForcedRendererBackBufferResolution",HackyPlatformValue( false, true, false), eConsoleVarFlag_ReadOnly );	
	TConfigVar<Int32> cvForcedRendererResolutionWidth		( "Rendering", "ForcedRendererResolutionWidth",		HackyPlatformValue( 1920, 1600, 1920 ),	eConsoleVarFlag_ReadOnly );		
	TConfigVar<Int32> cvForcedRendererResolutionHeight		( "Rendering", "ForcedRendererResolutionHeight",	HackyPlatformValue( 1080, 900, 1080 ),	eConsoleVarFlag_ReadOnly );		

	TConfigVar< Int32, Validation::IntRange<0, 24> >		cvFoliageMaxVisibilityDepth( "Foliage",	"MaxVisibilityDepth", 12, eConsoleVarFlag_Save );

	TConfigVar< Float, Validation::FloatRange< 0, 500, 100 > >		cvFoliageDistanceScale(	"Rendering/SpeedTree", "FoliageDistanceScale",					1.0f, eConsoleVarFlag_Save );
	TConfigVar< Float, Validation::FloatRange< 0, 500, 100 > >		cvGrassDistanceScale(	"Rendering/SpeedTree", "GrassDistanceScale",					1.0f, eConsoleVarFlag_Save );

	/***** Post Process *****/
	TConfigVar<Bool> cvAllowBloom					( "PostProcess", "AllowBloom",					true,	eConsoleVarFlag_Save );		//!< Allow bloom
	TConfigVar<Bool> cvAllowShafts					( "PostProcess", "AllowShafts",					true,	eConsoleVarFlag_Save );		//!< Allow shafts
	TConfigVar<Int32> cvShaftsHeightLimitQ0			( "PostProcess", "ShaftsHeightLimitQ0",			1080	                     );		//!< Screen height <= this will use Q0 shaft shader
	TConfigVar<Int32> cvShaftsHeightLimitQ1			( "PostProcess", "ShaftsHeightLimitQ1",			1440	                     );		//!< Screen height <= this will use Q1 shaft shader. Anything bigger will use Q2
	TConfigVar<Bool> cvAllowAntialias				( "PostProcess", "AllowAntialias",				true,	eConsoleVarFlag_Save );		//!< Allow antialias
	TConfigVar<Bool> cvAllowMSAA					( "PostProcess", "AllowMSAA",					true,	eConsoleVarFlag_Save );		//!< Allow MSAA
	TConfigVar<Bool> cvAllowBlur					( "PostProcess", "AllowBlur",					true,	eConsoleVarFlag_Save );		//!< Allow blur
	TConfigVar<Bool> cvAllowDOF						( "PostProcess", "AllowDOF",					true,	eConsoleVarFlag_Save );		//!< Allow DOF
	TConfigVar<Bool> cvAllowCutsceneDOF				( "PostProcess", "AllowCutsceneDOF",			true,	eConsoleVarFlag_Save );		//!< Allow cutscene DOF
	TConfigVar<Bool> cvAllowVignette				( "PostProcess", "AllowVignette",				true,	eConsoleVarFlag_Save );		//!< Allow vignette
	TConfigVar<Int32> cvAllowSharpen				( "PostProcess", "SharpenAmount",				1,	eConsoleVarFlag_Save );		//!< Allow sharpen
	TConfigVar<Bool> cvAllowChromaticAberration		( "PostProcess", "AllowChromaticAberration",	true,	eConsoleVarFlag_Save );		//!< Allow chromatic aberration
	TConfigVar<Int32> cvSSAOVersion					( "PostProcess", "SSAOSolution",				HackyPlatformValue(1,1,1),		eConsoleVarFlag_Save );		//!< SSAO solution
	TConfigVar<Bool> cvAllowMotionBlur				( "PostProcess", "AllowMotionBlur",				true,	eConsoleVarFlag_Save );		//!< Allow motion blur
	TConfigVar<Float> cvMotionBlurPositionTreshold	( "PostProcess", "MotionBlurPositionTreshold",	0.05f,	eConsoleVarFlag_Save );		//!< 
	TConfigVar<Float> cvMotionBlurRotationTreshold	( "PostProcess", "MotionBlurRotationTreshold",	1.0f,	eConsoleVarFlag_Save );		//!< 
	TConfigVar<Bool> cvAllowFog						( "PostProcess", "AllowFog",					true,	eConsoleVarFlag_Save );		//!< Allow fog
	TConfigVar<Bool> cvAllowUnderwater				( "PostProcess", "AllowUnderwater",				true,	eConsoleVarFlag_Save );		//!< Allow underwater post-process

	/***** Viewport *****/
	TConfigVar<Int32> cvFullScreenMode			( "Viewport",	 "FullScreenMode",		1,										eConsoleVarFlag_Save );		// Maps to EViewportWindowMode
	TConfigVar<Bool>  cvVSync					( "Viewport",	 "VSync",				true,									eConsoleVarFlag_Save );		// Versital synchronization
	TConfigVar<Int32> cvVSyncThreshold			( "Viewport",	 "VSyncThreshold",		0,										eConsoleVarFlag_Save );		// percent of the frame where we allow tearing, max value is 100
	TConfigVar<Int32> cvOutputMonitor			( "Viewport",	 "OutputMonitor",		0,										eConsoleVarFlag_Save );		// Index of output monitor enumerated by GpuApi
	TConfigVar<String> cvResolution( "Viewport", "Resolution", PackResolutionAsString(-1, -1), eConsoleVarFlag_Save );		// Screen width and height

	/***** Level of Detail *****/
	TConfigVar<Int32> cvMeshLodGameplayDownscale		( "LevelOfDetail",	 "MeshLodGameplayDownscale",		0,			eConsoleVarFlag_Save );
	TConfigVar<Int32> cvCharacterLodGameplayDownscale	( "LevelOfDetail",	 "CharacterLodGameplayDownscale",	0,			eConsoleVarFlag_Save );

	TConfigVar<Float> cvDecalsHideDistance				( "LevelOfDetail",	 "DecalsHideDistance",				40.0f,		eConsoleVarFlag_Save );
	TConfigVar<Float> cvDynDecalsHideDistance			( "LevelOfDetail",	 "DynamicDecalsHideDistance",		20.0f,		eConsoleVarFlag_Save );
	TConfigVar<Float> cvStripeHideDistance				( "LevelOfDetail",	 "StripeHideDistance",				60.0f,		eConsoleVarFlag_Save );
	TConfigVar<Float> cvSwarmHideDistance				( "LevelOfDetail",	 "SwarmHideDistance",				60.0f,		eConsoleVarFlag_Save );
	TConfigVar<Float> cvDimmerHideDistance				( "LevelOfDetail",	 "DimmerHideDistance",				60.0f,		eConsoleVarFlag_Save );

	/***** Budgets *****/
	TConfigVar<Int32> cvTerrainTileMipDataBudget		( "Budget", "TerrainTileMipDataBudget",					100,		eConsoleVarFlag_Save );		//!< Maximum amount of terrain tile mip data before forcing eviction (MB)
	TConfigVar<Float> cvTerrainTileTimeout				( "Budget", "TerrainTileTimeout",						10.0f,		eConsoleVarFlag_Save );		//!< Amount of time before an unused terrain tile is auto-evicted
	TConfigVar<Float> cvTerrainTileLoadingTimeout		( "Budget", "TerrainTileLoadingTimeout",				5.0f,		eConsoleVarFlag_Save );		//!< Amount of time before an unused (and currently loading) tile is auto-evicted
	TConfigVar<Float> cvTerrainTileMinTimeout			( "Budget", "TerrainTileMinTimeout",					2.0f,		eConsoleVarFlag_Save );		//!< Minimum amount of time before a terrain tile can be force evicted
	TConfigVar<Int32> cvOcclusionQueryAdditionalMemory	( "Budget", "OcclusionQueryAdditionalMemory",			5,			eConsoleVarFlag_Save );		//!< Size of additional memory for Umbra Queries (in MB, so "5" means 5 MB)

	TConfigVar<Int32> cvMaxAllowedApexTicked			( "Budget", "cvMaxAllowedApexTicked",					60								 );		//!< Budget for apex clothes
	TConfigVar<Int32> cvMaxAllowedApexDestroTicked		( "Budget", "cvMaxAllowedApexDestroTicked",				20								 );		//!< Budget for apex destructibles
	TConfigVar<Int32> cvMaxAllowedLightsShadow			( "Budget", "cvMaxAllowedLightsShadow",					3								 );		//!< Budget for shadow casting lights
	TConfigVar<Int32> cvMaxAllowedLightsNonShadows		( "Budget", "cvMaxAllowedLightsNonShadows",				40								 );		//!< Budget for non shadow casting lights
	TConfigVar<Int32> cvMaxAllowedDecalsSS				( "Budget", "cvMaxAllowedDecalsSS",						160 							 );		//!< Budget for screen space decals
	TConfigVar<Int32> cvMaxAllowedDecalsDynamic			( "Budget", "cvMaxAllowedDecalsDynamic",				10								 );		//!< Budget for dynamic decals
	TConfigVar<Int32> cvMaxAllowedParticlesCount		( "Budget", "cvMaxAllowedParticlesCount",				1000							 );		//!< Budget for particles
	TConfigVar<Int32> cvMaxAllowedHiresChunks			( "Budget", "cvMaxAllowedHiresChunks",					25								 );		//!< Budget for chunks with high resolution shadows
	TConfigVar<Int32> cvMaxAllowedSpeedTree				( "Budget", "cvMaxAllowedSpeedTree",					25000							 );		//!< Budget for speed tree meshes
	TConfigVar<Int32> cvMaxAllowedGrass					( "Budget", "cvMaxAllowedGrass",						30000							 );		//!< Budget for grass instances
	TConfigVar<Int32> cvMaxAllowedChunksStatic			( "Budget", "cvMaxAllowedChunksStatic",					1500							 );		//!< Budget for static chunks rendered
	TConfigVar<Int32> cvMaxAllowedTrianglesStatic		( "Budget", "cvMaxAllowedTrianglesStatic",				500000							 );		//!< Budget for static triangles rendered
	TConfigVar<Int32> cvMaxAllowedChunksSkinned			( "Budget", "cvMaxAllowedChunksSkinned",				400								 );		//!< Budget for skinned chunks rendered
	TConfigVar<Int32> cvMaxAllowedTrianglesSkinned		( "Budget", "cvMaxAllowedTrianglesSkinned",				100000							 );		//!< Budget for skinned triangles rendered
	TConfigVar<Int32> cvMaxAllowedDynMeshes				( "Budget", "cvMaxAllowedDynMeshes",					104857600						 );		//!< Memory Budget for non static meshes
	TConfigVar<Int32> cvMaxAllowedStatMeshes			( "Budget", "cvMaxAllowedStatMeshes",					209715200						 );		//!< Memory Budget for static meshes
	TConfigVar<Int32> cvMaxAllowedCharTextures			( "Budget", "cvMaxAllowedCharTextures",					209715200						 );		//!< Memory Budget for textures of static meshes
	TConfigVar<Int32> cvMaxAllowedStatTextures			( "Budget", "cvMaxAllowedStatTextures",					314572800						 );		//!< Memory Budget for textures of non static meshes
	TConfigVar<Float> cvMaxAllowedApexTickedTime		( "Budget", "cvMaxAllowedApexTickedTime",				0.5f							 );		//!< Time Budget for apex clothes
	TConfigVar<Float> cvMaxAllowedApexDestroTickedTime	( "Budget", "cvMaxAllowedApexDestroTickedTime",			0.2f							 );		//!< Time Budget for apex destructibles
	TConfigVar<Float> cvMaxAllowedLightsShadowTime		( "Budget", "cvMaxAllowedLightsShadowTime",				1.0f							 );		//!< Time Budget for shadow casting lights
	TConfigVar<Float> cvMaxAllowedLightsNonShadowsTime	( "Budget", "cvMaxAllowedLightsNonShadowsTime",			0.2f							 );		//!< Time Budget for non shadow casting lights
	TConfigVar<Float> cvMaxAllowedDecalsSSTime			( "Budget", "cvMaxAllowedDecalsSSTime",					0.1f							 );		//!< Time Budget for screen space decals
	TConfigVar<Float> cvMaxAllowedDecalsDynamicTime		( "Budget", "cvMaxAllowedDecalsDynamicTime",			0.5f							 );		//!< Time Budget for dynamic decals
	TConfigVar<Float> cvMaxAllowedParticlesCountTime	( "Budget", "cvMaxAllowedParticlesCountTime",			0.5f							 );		//!< Time Budget for particles
	TConfigVar<Float> cvMaxAllowedActiveEnvProbesTime	( "Budget", "cvMaxAllowedActiveEnvProbesTime",			0.1f							 );		//!< Time Budget for active env probes
	TConfigVar<Float> cvMaxAllowedHiresChunksTime		( "Budget", "cvMaxAllowedHiresChunksTime",				0.2f							 );		//!< Time Budget for chunks with high resolution shadows
	TConfigVar<Float> cvMaxAllowedSpeedTreeTime			( "Budget", "cvMaxAllowedSpeedTreeTime",				2.2f							 );		//!< Time Budget for SpeedTree
	TConfigVar<Float> cvMaxAllowedGrassTime				( "Budget", "cvMaxAllowedGrassTime",					2.0f							 );		//!< Time Budget for Grass
	TConfigVar<Float> cvMaxAllowedChunksStaticTime		( "Budget", "cvMaxAllowedChunksStaticTime",				1.5f							 );		//!< Time Budget for static chunks rendered
	TConfigVar<Float> cvMaxAllowedChunksSkinnedTime		( "Budget", "cvMaxAllowedChunksSkinnedTime",			2.5f							 );		//!< Time Budget for skinned chunks rendered
	
	/***** Visuals *****/
	TConfigVar<Float> cvGamma							( "Visuals", "GammaValue",								1.f,		eConsoleVarFlag_Save );		//!< Gamma
	TConfigVar<Float> cvInventoryFixedLuminance			( "Visuals", "InventoryFixedLuminance",					.25f,		eConsoleVarFlag_Save );		//!< Fixed luminance used in inventory
	TConfigVar<Float> cvInventoryBgColorScaleR			( "Visuals", "InventoryBgColorScaleR",					0.02f,		eConsoleVarFlag_Save );		//!< Color scale for inventory background to prevent precision loss and banding
	TConfigVar<Float> cvInventoryBgColorScaleG			( "Visuals", "InventoryBgColorScaleG",					0.02f*1.7f,	eConsoleVarFlag_Save );		//!< Color scale for inventory background to prevent precision loss and banding
	TConfigVar<Float> cvInventoryBgColorScaleB			( "Visuals", "InventoryBgColorScaleB",					0.02f*1.8f,	eConsoleVarFlag_Save );		//!< Color scale for inventory background to prevent precision loss and banding
	TConfigVar<Bool> cvAllowClothSimulationOnGpu		( "Visuals", "AllowClothSimulationOnGpu",				false,		eConsoleVarFlag_Save );
	
	/***** Screenshots and movie grabbing *****/
	TConfigVar<Float> cvMovieFramerate					( "Visuals", "MovieFramerate",							30.f,		eConsoleVarFlag_Save );		//!< Framerate to grab movies (fps)
	TConfigVar<Bool>  cvMovieUbersampling				( "Visuals", "MovieUbersampling",						false,		eConsoleVarFlag_Save );		//!< Ubersampling when grabbing movies

	/***** Helpers *****/
	namespace Helper
	{
		Uint32 GetCurrentOutputMonitorConfig()
		{
			Int32 result = 0;
			Int32 monitorCount = GpuApi::GetMonitorCount();
			if( monitorCount > 0 )
			{
				result= Clamp( Config::cvOutputMonitor.Get(), 0, GpuApi::GetMonitorCount() - 1 );
			}
			return (Uint32)result;
		}

		void GetNativeResolution( Int32& width, Int32& height )
		{
			GpuApi::GetNativeResolution( GetCurrentOutputMonitorConfig(), width, height );
		}

		void ExtractDisplayModeValues( const String& mode, Int32& width, Int32& height )
		{
			// Default values
			GetNativeResolution(width, height);

			Int32 tempWidth = width;
			Int32 tempHeight = height;

			TDynArray<String> splitted = mode.Split(TXT("x"));
			Bool isCountValid = splitted.Size() == 2;
			Bool isWidthValid = false;
			Bool isHeightValid = false;
			Bool isResolutionValid = false;
			if( isCountValid == true )
			{
				isWidthValid = FromString( splitted[0], tempWidth );
				isHeightValid = FromString( splitted[1], tempHeight );

				isResolutionValid = (tempWidth > 0) && (tempHeight > 0);
			}

			if( isCountValid && isWidthValid && isHeightValid && isResolutionValid )
			{
				width = tempWidth;
				height = tempHeight;
			}
			else
			{
				WARN_ENGINE( TXT("Resolution config is corrupted. Using native resolution") );
			}
		}

		Bool GetDisplayModes( Uint32& outModesNum, GpuApi::DisplayModeDesc** outModes )
		{
			Uint32 modesNum = 0;
			if( GpuApi::EnumerateDisplayModes( GetCurrentOutputMonitorConfig(), &modesNum ) == true )
			{
				if( modesNum != 0 )
				{
					(*outModes) = new GpuApi::DisplayModeDesc[modesNum];
					outModesNum = modesNum;

					auto sortPredicate = []( const GpuApi::DisplayModeDesc& left, const GpuApi::DisplayModeDesc& right )
					{ 
						if( left.width == right.width )
						{
							return left.height < right.height;
						}
							  
						return left.width < right.width; 
					};

					if( GpuApi::EnumerateDisplayModes( GetCurrentOutputMonitorConfig(), &modesNum, outModes ) == true )
					{
						Sort( &(*outModes)[0], &(*outModes)[modesNum], sortPredicate );
						return true;
					}
					else
					{
						delete [] (*outModes);
						return false;
					}
				}
			}

			return false;
		}

		Bool IsResolutionSupported( const Int32 width, const Int32 height )
		{
			return (width >= Consts::minimumResolutionWidth && height >= Consts::minimumResolutionHeight);
		}

		void ListAvailableResolutions( TDynArray<String>& outScreenModes )
		{
			Uint32 modesNum = 0;
			GpuApi::DisplayModeDesc* modes = nullptr;

			if( GetDisplayModes( modesNum, &modes ) == false )
			{
				return;
			}

			outScreenModes.Reserve( modesNum );

			for( Uint32 i=0; i<modesNum; ++i )
			{
				Int32 width = modes[i].width;
				Int32 height = modes[i].height;
				if( IsResolutionSupported( width, height ) == true )
				{
					outScreenModes.PushBack( PackDisplayModeValues( width, height ) );
				}
			}

			outScreenModes.Erase( Unique( outScreenModes.Begin(), outScreenModes.End() ), outScreenModes.End() );

			delete [] modes;
		}

		Bool GetBestMatchingResolution(const Int32 width, const Int32 height, Int32& outWidth, Int32& outHeight )
		{
			Uint32 modesNum = 0;
			GpuApi::DisplayModeDesc* modes = nullptr;

			if( GetDisplayModes( modesNum, &modes ) == false )
			{
				return false;
			}

			if( modesNum == 0 )
			{
				return false;
			}

			// Find first supported resolution
			Bool supportedResolutionFound = false;		// If we won't find any supported resolution, then this means GetBestMatchingResolution failed
			for( Uint32 i=0; i<modesNum; ++i )
			{
				if( IsResolutionSupported( modes[i].width, modes[i].height ) == true )
				{
					outWidth = modes[i].width;
					outHeight = modes[i].height;
					supportedResolutionFound = true;
					break;
				}
			}

			// Find best match
			if( supportedResolutionFound == true )
			{
				Int32 matchDelta = Abs( modes[0].width - width ) + Abs( modes[0].height - height );

				for( Uint32 i=0; i<modesNum; ++i )
				{
					if( IsResolutionSupported( modes[i].width, modes[i].height ) == true )
					{
						Int32 currentMatchDelta = Abs( modes[i].width - width ) + Abs( modes[i].height - height );
						if( currentMatchDelta < matchDelta )
						{
							outWidth = modes[i].width;
							outHeight = modes[i].height;
							matchDelta = currentMatchDelta;
						}
					}
				}
			}

			delete [] modes;

			return supportedResolutionFound;
		}

	}
}

CGatheredResource resTextureGroups( TXT("engine\\textures\\texturegroups.xml"), RGF_Startup );

CRenderSettingsManager::CRenderSettingsManager()
{
	InGameConfig::Listing::CListingFunctionRegister& listFuncReg = InGameConfig::Listing::GListingFunctionRegister::GetInstance();

	// Register resolution listing function
	listFuncReg.RegisterListingFunction( CNAME(ListResolutions), 
		[=] ( TDynArray<InGameConfig::Listing::SEngineConfigPresetDesc>& outDescList )
	{
		TDynArray<String> screenModes;
		Config::Helper::ListAvailableResolutions( screenModes );

		for( String& mode : screenModes )
		{
			InGameConfig::Listing::SEngineConfigPresetDesc desc;
			desc.optionDisplayName = mode;
			desc.entriesDesc.PushBack( InGameConfig::Listing::SEngineConfigPresetEntryDesc( CNAME(Viewport), CNAME(Resolution), mode ) );
			outDescList.PushBack( desc );
		}
	}
		);

	// Register output monitor count listing function
	listFuncReg.RegisterListingFunction( CNAME( ListAvailableMonitorCount ), 
			[]( TDynArray<InGameConfig::Listing::SEngineConfigPresetDesc>& optionsDesc )
		{
			Int32 monitorCount = GpuApi::GetMonitorCount();
			for( Int32 i=0; i<monitorCount; ++i )
			{
				InGameConfig::Listing::SEngineConfigPresetDesc optDesc;
				optDesc.optionDisplayName = ToString( i+1 );
				optDesc.entriesDesc.PushBack( InGameConfig::Listing::SEngineConfigPresetEntryDesc( CName(TXT("Viewport")), CName(TXT("OutputMonitor")), ToString( i ) ));
				optionsDesc.PushBack( optDesc );
			}
		} );
}

namespace Helper
{
	template< typename T >
	T ReadXMLProperty( CXMLReader& xml, const Char* name, const T defaultValue )
	{
		T ret = defaultValue;

		IRTTIType* type = GetTypeObject<T>();
		RED_FATAL_ASSERT( type != nullptr, "Trying to load unknown property from XML" );

		if ( xml.BeginNode( name ) )
		{
			String value;
			if ( xml.Attribute( TXT("value"), value) )
			{
				// parse
				type->FromString( &ret, value );
			}

			xml.EndNode();
		}

		return ret;
	}
}

void CRenderSettingsManager::LoadTextureGroups()
{
	// Clear current list
	m_textureGroups.Clear();

	// Load XML
	{
		Red::TScopedPtr< CXMLReader > xml( GDepot->LoadXML( resTextureGroups.GetPath().ToString() ) );

		if ( xml && xml->BeginNode( TXT("texturegroups") ) )
		{
			while ( xml->BeginNode( TXT("group") ) )
			{
				String name;
				if ( xml->AttributeT( TXT("name"), name ) )
				{
					// load settings
					TextureGroup group;
					group.m_compression = Helper::ReadXMLProperty( *xml, TXT("compression"), group.m_compression );
					group.m_category = Helper::ReadXMLProperty( *xml, TXT("category"), group.m_category );
					group.m_isUser = Helper::ReadXMLProperty( *xml, TXT("user"), group.m_isUser );
					group.m_isStreamable = Helper::ReadXMLProperty( *xml, TXT("streamable"), group.m_isStreamable );
					group.m_isResizable = Helper::ReadXMLProperty( *xml, TXT("resizable"), group.m_isResizable );
					group.m_isDetailMap = Helper::ReadXMLProperty( *xml, TXT("detailmap"), group.m_isDetailMap );
					group.m_isAtlas = Helper::ReadXMLProperty( *xml, TXT("atlas"), group.m_isAtlas );
					group.m_maxSize = Helper::ReadXMLProperty( *xml, TXT("maxsize"), group.m_maxSize );
					group.m_hasMipchain = Helper::ReadXMLProperty( *xml, TXT("hasmips"), group.m_hasMipchain );
					group.m_highPriority = Helper::ReadXMLProperty( *xml, TXT("highPriority"), group.m_highPriority );

					// register
					m_textureGroups.Insert( CName( name.AsChar() ), group );
				}

				xml->EndNode();
			}
		}
	}
}
