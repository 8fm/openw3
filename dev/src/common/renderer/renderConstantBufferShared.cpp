/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderCollector.h"
#include "renderRenderSurfaces.h"
#include "renderEnvProbe.h"
#include "renderEnvProbeManager.h"

#define TILE_SIZE						16
#define TILE_SIZE_INTERIOR_FULLRES		(TILE_SIZE * WEATHER_VOLUMES_SIZE_DIV)

namespace Config
{
	TConfigVar<Float> cvSpeedTreeTerrainNormalmapFlatness			( "Rendering/SpeedTree", "SpeedTreeTerrainNormalmapFlatness",				1.5f );
}

struct ShaderCullingEnvProbeParams
{
	Vector viewToLocal0;
	Vector viewToLocal1;
	Vector viewToLocal2;
	Float normalScaleX;	
	Float normalScaleY;	
	Float normalScaleZ;
	Uint32 probeIndex;
};

struct SShaderCommonEnvProbeParams
{
	Float	weight;
	Float	probePosX;
	Float	probePosY;
	Float	probePosZ;
	Float	areaMarginScaleX;
	Float	areaMarginScaleY;
	Float	areaMarginScaleZ;
	Matrix	areaWorldToLocal;
	Vector  intensities;
	Matrix	parallaxWorldToLocal;
	Uint32	slotIndex;
};

struct SharedConstantbuffer
{
	// generic
	Vector cameraPosition;
	Matrix worldToView;
	Matrix screenToView_UNUSED;						//< ace_todo: remove (unused)
	Matrix viewToWorld;
	Matrix projectionMatrix;
	Matrix screenToWorld;
	Vector cameraNearFar;
	Vector cameraDepthRange;
	Vector screenDimensions;
	Vector numTiles;
	Matrix lastFrameViewReprojectionMatrix;
	Matrix lastFrameProjectionMatrix;
	Vector localReflectionParam0;			//< ace_todo: unused -> remove
	Vector localReflectionParam1;			//< ace_todo: unused -> remove

	// Speed tree stuff
	Vector speedTreeRandomColorFallback;
	
	// lighting
	Vector translucencyParams0;
	Vector translucencyParams1;

	// fog
	Vector fogSunDir;
	Vector fogColorFront;
	Vector fogColorMiddle;
	Vector fogColorBack;
	Vector fogBaseParams;
	Vector fogDensityParamsScene;
	Vector fogDensityParamsSky;		
	Vector aerialColorFront;
	Vector aerialColorMiddle;
	Vector aerialColorBack;
	Vector aerialParams;

	// speed tree
	Vector speedTreeBillboardsParams;
	Vector speedTreeParams;
	Vector speedTreeRandomColorLumWeightsTrees;
	Vector speedTreeRandomColorParamsTrees0;	// reversed color ( 1 - color ). saturation in alpha channel.
	Vector speedTreeRandomColorParamsTrees1;
	Vector speedTreeRandomColorParamsTrees2;
	Vector speedTreeRandomColorLumWeightsBranches;
	Vector speedTreeRandomColorParamsBranches0;	// reversed color ( 1 - color ). saturation in alpha channel.
	Vector speedTreeRandomColorParamsBranches1;
	Vector speedTreeRandomColorParamsBranches2;
	Vector speedTreeRandomColorLumWeightsGrass;
	Vector speedTreeRandomColorParamsGrass0;	// reversed color ( 1 - color ). saturation in alpha channel.
	Vector speedTreeRandomColorParamsGrass1;
	Vector speedTreeRandomColorParamsGrass2;

	//
	Vector terrainPigmentParams;
	Vector speedTreeBillboardsGrainParams0;
	Vector speedTreeBillboardsGrainParams1;

	// weather env / blending params
	Vector weatherAndPrescaleParams;
	Vector windParams;
	Vector skyboxShadingParams;

	// ssao
	Vector ssaoParams;

	// msaaParams
	Vector msaaParams;

	// envmap
	Vector localLightsExtraParams;
	Vector cascadesSize;

	//
	Vector surfaceDimensions;

	// envprobes
	int									numCullingEnvProbeParams;
	ShaderCullingEnvProbeParams			cullingEnvProbeParams[CRenderEnvProbeManager::CUBE_ARRAY_CAPACITY - 1];		// doesn't include global probes
	SShaderCommonEnvProbeParams			commonEnvProbeParams[CRenderEnvProbeManager::CUBE_ARRAY_CAPACITY];

	// pbr
	Vector pbrSimpleParams0;
	Vector pbrSimpleParams1;

	//
	Vector cameraFOV;

	//
	Vector ssaoClampParams0;
	Vector ssaoClampParams1;

	//
	Vector fogCustomValuesEnv0;
	Vector fogCustomRangesEnv0;
	Vector fogCustomValuesEnv1;
	Vector fogCustomRangesEnv1;
	Vector mostImportantEnvsBlendParams;

	Vector fogDensityParamsClouds;

	// sky
	Vector skyColor;
	Vector skyColorHorizon;
	Vector sunColorHorizon;
	Vector sunBackHorizonColor;
	Vector sunColorSky;
	Vector moonColorHorizon;
	Vector moonBackHorizonColor;
	Vector moonColorSky;
	Vector skyParams1;
	Vector skyParamsSun;
	Vector skyParamsMoon;
	Vector skyParamsInfluence;

	//
	Matrix screenToWorldRevProjAware;
	Matrix pixelCoordToWorldRevProjAware;
	Matrix pixelCoordToWorld;

	//
	Vector lightColorParams;

	//
	Vector halfScreenDimensions;
	Vector halfSurfaceDimensions;

	// color groups
	Vector colorGroups[SHARED_CONSTS_COLOR_GROUPS_CAPACITY];
};

void CRenderInterface::CalculateSharedConstants( const CRenderFrameInfo &info, Uint32 fullRenderTargetWidth, Uint32 fullRenderTargetHeight, Int32 forcedEnvProbeIndex, Float forcedEnvProbeWeight )
{
	// Ensure buffer created
	if ( !m_sharedConstantBuffer )
	{
		m_sharedConstantBuffer = GpuApi::CreateBuffer( sizeof(SharedConstantbuffer), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		ASSERT ( m_sharedConstantBuffer );
		GpuApi::SetBufferDebugPath( m_sharedConstantBuffer, "shared cbuffer" );
	}

	// Lock the buffer
	void* pConstantData = GpuApi::LockBuffer( m_sharedConstantBuffer, GpuApi::BLF_Discard, 0, sizeof(SharedConstantbuffer) );
	if ( !pConstantData )
	{
		ASSERT( !"Failed to lock shared constant buffer" );
		return;
	}
	SharedConstantbuffer *cbuff = static_cast<SharedConstantbuffer*>( pConstantData );

	const Float oneOverNear = 1.f/info.m_camera.GetNearPlane();
	const Float oneOverFar = 1.f/info.m_camera.GetFarPlane();
	const Float oneOverW = 1.f/info.m_camera.GetViewToScreen().V[0].A[0];
	const Float oneOverH = 1.f/info.m_camera.GetViewToScreen().V[1].A[1];
	
	// generic
	cbuff->cameraPosition = info.m_camera.GetPosition();
	cbuff->worldToView = info.m_camera.GetWorldToView();
	cbuff->screenToView_UNUSED = Matrix::IDENTITY; // info.m_camera.GetScreenToView();
	cbuff->screenToWorldRevProjAware = info.m_camera.GetScreenToWorldRevProjAware();
	cbuff->viewToWorld = info.m_camera.GetViewToWorld();
	cbuff->projectionMatrix = info.m_camera.GetViewToScreen();
	cbuff->screenToWorld = info.m_camera.GetScreenToWorld();
	cbuff->cameraNearFar = Vector ( oneOverFar - oneOverNear, oneOverNear, oneOverW, oneOverH );
	cbuff->cameraDepthRange = info.m_camera.IsReversedProjection() ? Vector ( -1, 1, info.m_camera.GetNearPlane(), info.m_camera.GetFarPlane() ) : Vector ( 1, 0, info.m_camera.GetNearPlane(), info.m_camera.GetFarPlane() );
	cbuff->surfaceDimensions = Vector( (Float)fullRenderTargetWidth, (Float)fullRenderTargetHeight, (Float)1.0f/fullRenderTargetWidth, (Float)1.0f/fullRenderTargetHeight );
	cbuff->halfSurfaceDimensions = Vector( (Float)(fullRenderTargetWidth>>1), (Float)(fullRenderTargetHeight>>1), (Float)1.0f/(fullRenderTargetWidth>>1), (Float)1.0f/(fullRenderTargetHeight>>1) );
	cbuff->screenDimensions = Vector( (Float)info.m_width, (Float)info.m_height, (Float)1.0f/info.m_width, (Float)1.0f/info.m_height );
	cbuff->halfScreenDimensions = Vector( (Float)(info.m_width>>1), (Float)(info.m_height>>1), (Float)1.0f/(info.m_width>>1), (Float)1.0f/(info.m_height>>1) );
	{
		const Float numTilesHoriz			= ceilf((Float)info.m_width / (Float)TILE_SIZE);
		const Float numTilesVert			= ceilf((Float)info.m_height / (Float)TILE_SIZE);
		const Float numTilesHorizInterior	= ceilf((Float)info.m_width / (Float)TILE_SIZE_INTERIOR_FULLRES);
		const Float numTilesVertInterior	= ceilf((Float)info.m_height / (Float)TILE_SIZE_INTERIOR_FULLRES);
		cbuff->numTiles = Vector( numTilesHoriz, numTilesVert, numTilesHorizInterior, numTilesVertInterior );
	}
	cbuff->cameraFOV = Vector ( info.m_camera.GetFOV(), DEG2RAD( info.m_camera.GetFOV() ), tanf( 0.5f * DEG2RAD( info.m_camera.GetFOV() ) ), 0.f );

	//
	{
		Matrix pixelToScreenMatrix;
		{
			const Float invWidth = 1.f / info.m_width;
			const Float invHeight = 1.f / info.m_height;
			pixelToScreenMatrix.SetIdentity().SetScale33( Vector ( 2.f * invWidth, -2.f * invHeight, 1.f ) ).SetTranslation( invWidth - 1, -(invHeight - 1), 0.f );
		}

		cbuff->pixelCoordToWorldRevProjAware = pixelToScreenMatrix * info.m_camera.GetScreenToWorldRevProjAware();
		cbuff->pixelCoordToWorld = pixelToScreenMatrix * info.m_camera.GetScreenToWorld();
	}

	// color groups
	COMPILE_ASSERT( SHARED_CONSTS_COLOR_GROUPS_CAPACITY == ECG_MAX );
	for ( Uint32 group_i=0; group_i<ECG_MAX; ++group_i )
	{
		const EEnvColorGroup group = (EEnvColorGroup)group_i;
		cbuff->colorGroups[group_i] = info.m_envParametersArea.m_colorGroups.GetCurveForColorGroup( group ).GetColorScaledGammaToLinear( true );
		cbuff->colorGroups[group_i].W = info.m_envParametersArea.m_colorGroups.GetAlphaForColorGroup( group );
	}
	
	// Setup reprojection matrices
	const SRenderCameraLastFrameData &lastFrameCamera = info.m_camera.GetLastFrameData();
	if ( lastFrameCamera.m_isValid )
	{
		cbuff->lastFrameViewReprojectionMatrix = info.m_camera.GetViewToWorld() * lastFrameCamera.m_worldToView;
		cbuff->lastFrameProjectionMatrix = lastFrameCamera.m_viewToScreen;
	}
	else
	{
		cbuff->lastFrameViewReprojectionMatrix = Matrix::IDENTITY;
		cbuff->lastFrameProjectionMatrix = info.m_camera.GetViewToScreen();
	}
	
	// Setup localReflection related parameters
	{
		cbuff->localReflectionParam0 = Vector::ZEROS;
		cbuff->localReflectionParam1 = Vector::ZEROS;
	}

	// Translucency
	{
		const CEnvGlobalLightParametersAtPoint &params = info.m_envParametersArea.m_globalLight;

		const Float valueScale		  = 0 != (info.m_renderFeaturesFlags & DMFF_LightingTranslucency) ? 1.f : 0.f;
		
		cbuff->translucencyParams0 = Vector (
				params.m_translucencyViewDependency.GetScalar(), 
				params.m_translucencyBaseFlatness.GetScalar(), 
				params.m_translucencyFlatBrightness.GetScalar() / (4.f * M_PI),
				params.m_translucencyGainBrightness.GetScalar() );

		cbuff->translucencyParams1 = Vector (
			params.m_translucencyFresnelScaleLight.GetScalar(), 
			params.m_translucencyFresnelScaleReflection.GetScalar(), 
			0,
			0 );
	}
	
	// Fog
	{
		const CEnvGlobalFogParametersAtPoint &fogParams = info.m_envParametersArea.m_globalFog;

		const Vector sunDir = -info.m_envParametersDayPoint.m_globalLightDirection;
		const Float densityScale = info.m_envParametersGame.m_displaySettings.m_allowGlobalFog ? CEnvGlobalFogParameters::m_fogDensityMultiplier : 0.f;

		cbuff->fogSunDir = Vector (-sunDir.X, -sunDir.Y, -sunDir.Z, 1.f);
		cbuff->fogColorFront = fogParams.m_fogColorFront.GetColorScaledGammaToLinear( true ) * Vector (1,1,1,0);
		cbuff->fogColorMiddle = fogParams.m_fogColorMiddle.GetColorScaledGammaToLinear( true ) * Vector (1,1,1,0);
		cbuff->fogColorBack = fogParams.m_fogColorBack.GetColorScaledGammaToLinear( true ) * Vector (1,1,1,0);
		cbuff->fogBaseParams = Vector ( 
			fogParams.m_fogVertDensityRimRange.GetScalarClampMin( -0.995f ),
			-1.f * fogParams.m_fogVertOffset.GetScalar(),
			fogParams.m_fogDistClamp.GetScalarClampMin( 0.005f ),
			fogParams.m_fogFinalExp.GetScalarClampMin( 0.f ) );
		cbuff->fogDensityParamsScene = Vector ( 
			densityScale * fogParams.m_fogDensity.GetScalarClampMin( 0.f ),
			-1.f * fogParams.m_fogVertDensity.GetScalar() * (1.f / fogParams.m_fogVertDensityLightFront.GetScalarClampMin( 0.0001f ) ),
			-1.f * fogParams.m_fogVertDensity.GetScalar() * (1.f / fogParams.m_fogVertDensityLightBack.GetScalarClampMin( 0.0001f ) ),
			0.f );
		cbuff->fogDensityParamsSky = Vector (
			densityScale * fogParams.m_fogDensity.GetScalarClampMin( 0.f ) * fogParams.m_fogSkyDensityScale.GetScalarClampMin( 0.f ),
			-1.f * fogParams.m_fogVertDensity.GetScalar() * (1.f / (fogParams.m_fogVertDensityLightFront.GetScalarClampMin( 0.0001f ) * fogParams.m_fogSkyVertDensityLightFrontScale.GetScalarClampMin( 0.0001f ) ) ),
			-1.f * fogParams.m_fogVertDensity.GetScalar() * (1.f / (fogParams.m_fogVertDensityLightBack.GetScalarClampMin( 0.0001f ) * fogParams.m_fogSkyVertDensityLightBackScale.GetScalarClampMin( 0.0001f ) ) ),
			0.f );

		cbuff->fogDensityParamsClouds = Vector ( 
			densityScale * fogParams.m_fogDensity.GetScalarClampMin( 0.f ) * fogParams.m_fogCloudsDensityScale.GetScalarClampMin( 0.f ),
			-1.f * fogParams.m_fogVertDensity.GetScalar() * (1.f / fogParams.m_fogVertDensityLightFront.GetScalarClampMin( 0.0001f ) ),
			-1.f * fogParams.m_fogVertDensity.GetScalar() * (1.f / fogParams.m_fogVertDensityLightBack.GetScalarClampMin( 0.0001f ) ),
			0.f );

		cbuff->aerialColorFront = fogParams.m_aerialColorFront.GetColorScaledGammaToLinear( true ) * Vector (1,1,1,0);
		cbuff->aerialColorMiddle = fogParams.m_aerialColorMiddle.GetColorScaledGammaToLinear( true ) * Vector (1,1,1,0);
		cbuff->aerialColorBack = fogParams.m_aerialColorBack.GetColorScaledGammaToLinear( true ) * Vector (1,1,1,0);
		cbuff->aerialParams = Vector ( 
			fogParams.m_aerialFinalExp.GetScalarClampMin( 0.f ),
			fogParams.m_fogAppearDistance.GetScalar(),
			1.f / fogParams.m_fogAppearRange.GetScalarClampMin( 0.001f ),
			0.f );
	}

	// Fog custom values
	{
		for ( Uint32 pass_i=0; pass_i<2; ++pass_i )
		{
			Vector &refCustomValues = pass_i ? cbuff->fogCustomValuesEnv1 : cbuff->fogCustomValuesEnv0;
			Vector &refCustomRanges = pass_i ? cbuff->fogCustomRangesEnv1 : cbuff->fogCustomRangesEnv0;
			const CEnvGlobalFogParametersAtPoint &fogParams = (info.m_envBlendingFactor > 0 ? (pass_i ? info.m_envParametersAreaBlend2 : info.m_envParametersAreaBlend1) : info.m_envParametersArea).m_globalFog;

			const Float customColorStart = fogParams.m_fogCustomColorStart.GetScalar();
			const Float customColorScale = 1.f / Max( 0.0001f, Abs(fogParams.m_fogCustomColorRange.GetScalar()) ) * (fogParams.m_fogCustomColorRange.GetScalar() > 0.f ? 1.f : -1.f);
			const Float customAmountStart = fogParams.m_fogCustomAmountScaleStart.GetScalar();
			const Float customAmountScale = 1.f / Max( 0.0001f, Abs(fogParams.m_fogCustomAmountScaleRange.GetScalar()) ) * (fogParams.m_fogCustomAmountScaleRange.GetScalar() >= 0.f ? 1.f : -1.f);

			refCustomValues = Vector (1,1,1,0) * fogParams.m_fogCustomColor.GetColorScaledGammaToLinear( true ) + Vector (0,0,0,1) * fogParams.m_fogCustomAmountScale.GetScalarClampMin( 0.f );
			refCustomRanges = Vector ( customColorScale, -customColorStart * customColorScale, customAmountScale, -customAmountStart * customAmountScale );
		}
	}

	// Most important envs blend info
	{
		const Float blendValue = info.m_envBlendingFactor;
		cbuff->mostImportantEnvsBlendParams = Vector ( blendValue, 1 - blendValue, 0, 0 );
	}

	// Sky
	{
		const CEnvGlobalSkyParametersAtPoint& skyParams = info.m_envParametersArea.m_sky;

		Vector sunColorHorizon = skyParams.m_sunColorHorizon.GetColorScaledGammaToLinear( true );
		Vector moonColorHorizon = skyParams.m_moonColorHorizon.GetColorScaledGammaToLinear( true );

		cbuff->skyColor = skyParams.m_skyColor.GetColorScaledGammaToLinear( true );
		cbuff->skyColorHorizon= skyParams.m_skyColorHorizon.GetColorScaledGammaToLinear( true );
		cbuff->sunColorHorizon.Set4( sunColorHorizon.X, sunColorHorizon.Y, sunColorHorizon.Z, 1.f );
		cbuff->sunBackHorizonColor = skyParams.m_sunBackHorizonColor.GetColorScaledGammaToLinear( true );
		cbuff->sunColorSky = skyParams.m_sunColorSky.GetColorScaledGammaToLinear( true ) * skyParams.m_sunColorSkyBrightness.GetScalarClampMin( 0.f );
		cbuff->moonColorHorizon.Set4( moonColorHorizon.X, moonColorHorizon.Y, moonColorHorizon.Z, 1.f );
		cbuff->moonBackHorizonColor = skyParams.m_moonBackHorizonColor.GetColorScaledGammaToLinear( true );
		cbuff->moonColorSky = skyParams.m_moonColorSky.GetColorScaledGammaToLinear( true ) * skyParams.m_moonColorSkyBrightness.GetScalarClampMin( 0.f );
		cbuff->skyParams1 = Vector( 
			Max( skyParams.m_activateFactor, 0.0f ), 
			skyParams.m_globalSkyBrightness.GetScalar(), 
			skyParams.m_horizonVerticalAttenuation.GetScalar(),
			0.f );
		cbuff->skyParamsSun = Vector( 
			skyParams.m_sunAreaSkySize.GetScalar(), 
			info.m_baseLightingParameters.m_sunDirection.X,
			info.m_baseLightingParameters.m_sunDirection.Y,
			info.m_baseLightingParameters.m_sunDirection.Z );
		cbuff->skyParamsMoon = Vector( 
			skyParams.m_moonAreaSkySize.GetScalar(), 
			info.m_baseLightingParameters.m_moonDirection.X,
			info.m_baseLightingParameters.m_moonDirection.Y,
			info.m_baseLightingParameters.m_moonDirection.Z );
		cbuff->skyParamsInfluence = Vector(
			skyParams.m_sunInfluence.GetScalarClamp( 0.f, 1.f ),
			skyParams.m_moonInfluence.GetScalarClamp( 0.f, 1.f ),
			0,
			0 );
	}

	// SpeedTree
	{
		const CEnvSpeedTreeParametersAtPoint &params = info.m_envParametersArea.m_speedTree;

		const Vector billboardsColor = params.m_billboardsColor.GetColorScaled( true ); //< in gamma space (a little tradeoff, speedtree operates in gamma space)
		const Float billboardsTranslucency = params.m_billboardsTranslucency.GetScalar();
		const Float ambientOcclusionScale = params.m_ambientOcclusionScale.GetScalarClampMin( 0.f );

		cbuff->speedTreeBillboardsParams.Set4( billboardsColor.X, billboardsColor.Y, billboardsColor.Z, billboardsTranslucency );
		cbuff->speedTreeParams.Set4( ambientOcclusionScale, Config::cvSpeedTreeTerrainNormalmapFlatness.Get(), 0, 0 );

		params.m_randomColorsTrees.BuildShaderParams( cbuff->speedTreeRandomColorLumWeightsTrees,	cbuff->speedTreeRandomColorParamsTrees0, cbuff->speedTreeRandomColorParamsTrees1, cbuff->speedTreeRandomColorParamsTrees2 );
		params.m_randomColorsBranches.BuildShaderParams( cbuff->speedTreeRandomColorLumWeightsBranches, cbuff->speedTreeRandomColorParamsBranches0, cbuff->speedTreeRandomColorParamsBranches1, cbuff->speedTreeRandomColorParamsBranches2 );
		params.m_randomColorsGrass.BuildShaderParams( cbuff->speedTreeRandomColorLumWeightsGrass,	cbuff->speedTreeRandomColorParamsGrass0, cbuff->speedTreeRandomColorParamsGrass1, cbuff->speedTreeRandomColorParamsGrass2 );

		cbuff->speedTreeRandomColorFallback = params.m_randomColorsFallback.GetColorScaledGammaToLinear( true );

		cbuff->terrainPigmentParams = Vector ( params.m_pigmentBrightness.GetScalar(), params.m_pigmentFloodStartDist.GetScalar(), params.m_pigmentFloodRange.GetScalar(), 0.f );
		
		cbuff->speedTreeBillboardsGrainParams0 = Vector ( info.m_speedTreeParameters.m_billboardsGrainBias, info.m_speedTreeParameters.m_billboardsGrainClipScale, info.m_speedTreeParameters.m_billboardsGrainClipBias, info.m_speedTreeParameters.m_billboardsGrainAlbedoScale );
		cbuff->speedTreeBillboardsGrainParams1 = Vector ( info.m_speedTreeParameters.m_billboardsGrainNormalScale, 1.f / Max( 0.0001f, info.m_speedTreeParameters.m_billboardsGrainClipDamping ), 0, 0 );
	}

	// weather env / blending params
	{													
		cbuff->weatherAndPrescaleParams = Vector( info.m_envParametersDayPoint.m_fakeCloudsShadowSize, info.m_envParametersDayPoint.m_weatherEffectStrength, info.m_envParametersDayPoint.m_windParameters.GetCloudsShadowsOffset().Z * info.m_envParametersDayPoint.m_fakeCloudsShadowSpeed, info.m_envParametersDayPoint.m_skyBoxWeatherBlend.Z );
		cbuff->windParams = Vector( 
			info.m_envParametersDayPoint.m_windParameters.GetCloudsShadowsOffset().X * info.m_envParametersDayPoint.m_fakeCloudsShadowSpeed,
			info.m_envParametersDayPoint.m_windParameters.GetCloudsShadowsOffset().Y * info.m_envParametersDayPoint.m_fakeCloudsShadowSpeed,
			info.m_envParametersDayPoint.m_windParameters.GetCloudsShadowsOffset().Z * info.m_envParametersDayPoint.m_fakeCloudsShadowSpeed,
			info.m_camera.GetPosition().Z - info.m_globalWaterLevelAtCameraPos
			);
		cbuff->skyboxShadingParams = Vector( info.m_envParametersDayPoint.m_fakeCloudsShadowCurrentTextureIndex, 
			info.m_envParametersDayPoint.m_fakeCloudsShadowTargetTextureIndex, info.m_envParametersDayPoint.m_skyBoxWeatherBlend.X, info.m_envParametersDayPoint.m_skyBoxWeatherBlend.Y );
	}

	// SSAO
	{
		const CEnvNVSSAOParametersAtPoint &params = info.m_envParametersArea.m_nvSsao;

		const Float nonAmbientAmount = params.m_nonAmbientInfluence.GetScalarClamp( 0.f, 1.f );
		const Float translucencyAmount = params.m_translucencyInfluence.GetScalarClamp( 0.f, 1.f );

		cbuff->ssaoParams = Vector( nonAmbientAmount, 1.f - nonAmbientAmount, translucencyAmount, 0.f );

		const Float p = 1 - params.m_valueClamp.GetScalar();
		const Vector col = params.m_ssaoColor.GetColorScaled( true );
		cbuff->ssaoClampParams0 = (Vector::ONES - col) * p;
		cbuff->ssaoClampParams1 = col * p + Vector::ONES * (1 - p);
	}

	// MSAA
	{	
		const Uint32 msaaLevel = GetEnabledMSAALevel( info );
		switch ( msaaLevel )
		{
		case 4:		cbuff->msaaParams.Set4( (Float)msaaLevel, 0.25f, 0.5f, 0.75f ); break;
		case 2:		cbuff->msaaParams.Set4( (Float)msaaLevel, 0.5f, 0.f, 0.f ); break;
		case 1:		cbuff->msaaParams.Set4( (Float)msaaLevel, 0.f, 0.f, 0.f ); break;
		default:	ASSERT ( !"Invalid/unsupported msaa level" ); cbuff->msaaParams.Set4( 1.f, 0.f, 0.f, 0.f );
		}
	}

	// cascades shadows
	{
		Uint32 csmSize = GetGlobalCascadesShadowResources().GetResolution();
		const Float dissolveAmountFactor = 1.f - Clamp( info.m_cameraLightsModifiersSetup.m_scenesSystemActiveFactor, 0.f, 1.f );
		cbuff->cascadesSize	= Vector ( (Float)csmSize, 0.5f * csmSize, dissolveAmountFactor, 0.f );
	}

	// local lights extra params
	{
		const CEnvCameraLightsSetupParametersAtPoint &params = info.m_envParametersArea.m_cameraLightsSetup;

		cbuff->localLightsExtraParams.Set4( params.m_cameraLightsNonCharacterScale.GetScalarClampMin( 0.f ), 0.f, 0.f, 0.f );
	}

	// pbr
	{
		const CEnvGlobalLightParametersAtPoint &params = info.m_envParametersArea.m_globalLight;

		const Float ambientScale	= (info.m_renderFeaturesFlags & DMFF_LightingAmbient)		? 1.f : 0.f;
		const Float diffuseScale	= (info.m_renderFeaturesFlags & DMFF_LightingDiffuse)		? 1.f : 0.f;
		const Float specularScale	= (info.m_renderFeaturesFlags & DMFF_LightingSpecular)		? 1.f : 0.f;
		const Float reflectionScale	= (info.m_renderFeaturesFlags & DMFF_LightingReflection)	? 1.f : 0.f;

		cbuff->pbrSimpleParams0.Set4( 
			params.m_envProbeAmbientScaleLight.GetScalarClampMin( 0.f ) * ambientScale,
			params.m_envProbeAmbientScaleShadow.GetScalarClampMin( 0.f ) * ambientScale,
			diffuseScale,
			specularScale );

		cbuff->pbrSimpleParams1.Set4( 
			params.m_envProbeReflectionScaleLight.GetScalarClampMin( 0.f ) * reflectionScale,
			params.m_envProbeReflectionScaleShadow.GetScalarClampMin( 0.f ) * reflectionScale,
			params.m_envProbeDistantScaleFactor.GetScalarClampMin( 0.f ),
			0.f );
	}

	// global light
	{
		const CEnvGlobalLightParametersAtPoint &globalLightParams = info.m_envParametersArea.m_globalLight;

		cbuff->lightColorParams.Set4(
			1.f / Max( 0.001f, globalLightParams.m_sunColorSidesMargin.GetScalarClampMin( 0.f ) ),
			-globalLightParams.m_sunColorCenterArea.GetScalarClampMin( 0.f ) / Max( 0.00005f, globalLightParams.m_sunColorSidesMargin.GetScalarClampMin( 0.f ) ),
			1.f / Max( 0.001f, globalLightParams.m_sunColorTopHeight.GetScalar() - globalLightParams.m_sunColorBottomHeight.GetScalar() ),
			-globalLightParams.m_sunColorBottomHeight.GetScalar() / Max( 0.00005f, globalLightParams.m_sunColorTopHeight.GetScalar() - globalLightParams.m_sunColorBottomHeight.GetScalar() ) );
	}

	// probe slots
	{
		cbuff->numCullingEnvProbeParams = 0;
		{
			const Uint32 * const slotsOrder = GetRenderer()->GetEnvProbeManager()->GetCubeSlotsNestingOrder();
			for ( Uint32 slot_idx_i=0; slot_idx_i<CRenderEnvProbeManager::CUBE_ARRAY_CAPACITY; ++slot_idx_i )
			{
				const Uint32 slot_i = slotsOrder[slot_idx_i];
				const CRenderEnvProbeManager::SCubeSlotData &src = GetRenderer()->GetEnvProbeManager()->GetCubeSlotData( slot_i );
				
				// culling params
				if ( CRenderEnvProbeManager::GLOBAL_SLOT_INDEX != slot_i )
				{
					if ( src.m_weight <= 0 )
					{
						// commented out continue because currenty we're using
						// parameters arrays directly for forward shading.
						// also it seems more optimal to just unrollLoop
						// through the params table even for deferred shading.

						//	continue;
					}

					ASSERT( cbuff->numCullingEnvProbeParams < ARRAY_COUNT( cbuff->cullingEnvProbeParams ) );
					ShaderCullingEnvProbeParams &cullParams = cbuff->cullingEnvProbeParams[ cbuff->numCullingEnvProbeParams ];
					++cbuff->numCullingEnvProbeParams;

					cullParams.probeIndex = slot_idx_i;

					const Matrix worldToLocal = src.m_probeParams.m_areaLocalToWorld.FullInverted(); // ace_optimize: full inverted to handle scaling 
					const Matrix localToWorld = src.m_probeParams.m_areaLocalToWorld;
					const Matrix viewToLocal = info.m_camera.GetViewToWorld() * worldToLocal;
					const Vector normalScale = localToWorld.GetScale33() / worldToLocal.GetPreScale33();

					cullParams.viewToLocal0 = viewToLocal.GetColumn( 0 );
					cullParams.viewToLocal1 = viewToLocal.GetColumn( 1 );
					cullParams.viewToLocal2 = viewToLocal.GetColumn( 2 );

					cullParams.normalScaleX = normalScale.X;
					cullParams.normalScaleY = normalScale.Y;
					cullParams.normalScaleZ = normalScale.Z;
				}

				SShaderCommonEnvProbeParams &dst = cbuff->commonEnvProbeParams[slot_idx_i];

				dst.slotIndex = slot_i;
				dst.parallaxWorldToLocal = src.m_probeParams.m_parallaxLocalToWorld.FullInverted();	//< ace_optimize: use fastInverse (don't see it ATM). we're ortho here! btw Inverted assumes doesn't invert the scale!

				Vector probePos = src.m_probeParams.m_probeOrigin;

				Float envProbeFlagsEncoded;
				{
					Uint32 envProbeFlags = 0;
					if ( src.m_probeParams.m_genParams.m_useInInterior || src.m_probeParams.m_genParams.m_useInExterior )
					{
						envProbeFlags |= src.m_probeParams.m_genParams.m_useInInterior ? ENVPROBE_FLAG_INTERIOR : 0;
						envProbeFlags |= src.m_probeParams.m_genParams.m_useInExterior ? ENVPROBE_FLAG_EXTERIOR : 0;
					}
					else
					{
						envProbeFlags |= ENVPROBE_FLAG_INTERIOR | ENVPROBE_FLAG_EXTERIOR;
					}

					COMPILE_ASSERT( sizeof(envProbeFlags) == sizeof(envProbeFlagsEncoded) );
					envProbeFlagsEncoded = reinterpret_cast< const Float& >( envProbeFlags );
				}
				
				dst.intensities.Set4( src.m_probeParams.GetEffectIntensity( info.m_gameTime ), envProbeFlagsEncoded, 0.f, 0.f );

				dst.probePosX = probePos.X;
				dst.probePosY = probePos.Y;
				dst.probePosZ = probePos.Z;
		
				GPUAPI_ASSERT( -1 == forcedEnvProbeWeight || forcedEnvProbeWeight >= 0 && forcedEnvProbeWeight <= 1 );
				if ( -1 != forcedEnvProbeIndex )
				{
					dst.areaWorldToLocal = Matrix().SetIdentity().SetTranslation( src.m_probeParams.m_areaLocalToWorld.GetTranslation() ).SetScale33( 99999.f ).FullInverted(); //< ace_optimize: use fastInverse (don't see it ATM). we're ortho here! btw Inverted assumes doesn't invert the scale!
					dst.areaMarginScaleX = 1.f / 0.000001f;
					dst.areaMarginScaleY = 1.f / 0.000001f;
					dst.areaMarginScaleZ = 1.f / 0.000001f;

					dst.weight = forcedEnvProbeIndex == (Int32)slot_i ? forcedEnvProbeWeight : 0;
				}
				else
				{
					dst.areaWorldToLocal = src.m_probeParams.m_areaLocalToWorld.FullInverted(); //< ace_optimize: use fastInverse (don't see it ATM). we're ortho here! btw Inverted assumes doesn't invert the scale!
					dst.areaMarginScaleX = 1.f / Max( 0.000001f, src.m_probeParams.m_areaMarginFactor.X * src.m_probeParams.m_areaMarginFactor.W );
					dst.areaMarginScaleY = 1.f / Max( 0.000001f, src.m_probeParams.m_areaMarginFactor.Y * src.m_probeParams.m_areaMarginFactor.W );
					dst.areaMarginScaleZ = 1.f / Max( 0.000001f, src.m_probeParams.m_areaMarginFactor.Z * src.m_probeParams.m_areaMarginFactor.W );

					dst.weight = forcedEnvProbeWeight;
				}

				if ( -1 == dst.weight )
				{
					dst.weight = src.m_weight * (CRenderEnvProbeManager::GLOBAL_SLOT_INDEX == slot_i ? 1.f : src.m_probeParams.m_contribution);
				}

				// Force 'empty' area for cubes with excluding weight in case we're
				// not branching out weights in the shaders but only the areas,
				// which is the case right now.
				if ( dst.weight <= 0 )
				{
					dst.areaWorldToLocal.SetIdentity();
					dst.areaWorldToLocal.SetScale33( 99999.f ); //< note that this is worldToLocal so inversie size here.
					dst.areaWorldToLocal.SetTranslation( Vector ( 0, 0, -999.f ) );
				}
			}
		}
	}

	// Unlock the buffer
	GpuApi::UnlockBuffer( m_sharedConstantBuffer );
}

void CRenderInterface::BindSharedConstants( GpuApi::eShaderType shaderStage )
{
	GpuApi::BindConstantBuffer( 12, m_sharedConstantBuffer, shaderStage );
}

void CRenderInterface::UnbindSharedConstants( GpuApi::eShaderType shaderStage )
{
	GpuApi::BindConstantBuffer( 12, GpuApi::BufferRef::Null(), shaderStage );
}

