#include "build.h"
#include "envParameters.h"
#include "../../common/core/gatheredResource.h"
#include "textureArray.h"
#include "../../common/core/depot.h"

CGatheredResource resNormalsBestFitTexture( TXT("engine\\textures\\normals_best_fit.xbm"), RGF_Startup );
CGatheredResource resBokehTexture( TXT("engine\\textures\\bokeh.xbm"), RGF_Startup );


IMPLEMENT_ENGINE_CLASS( SCharacterWindParams );
IMPLEMENT_ENGINE_CLASS( SWorldRenderSettings );
IMPLEMENT_ENGINE_CLASS( SWorldMotionBlurSettings );
IMPLEMENT_ENGINE_CLASS( SGlobalSpeedTreeParameters );

namespace Config
{
	const Float SPEEDTREE_COMMON_VALUE_DISABLED = -100;

	TConfigVar<Float> cvSpeedTreeCommonAlphaScalar3d				( "Rendering/SpeedTree", "SpeedTreeCommonAlphaScalar3d",				1.6f );
	TConfigVar<Float> cvSpeedTreeCommonAlphaScalarGrassNear			( "Rendering/SpeedTree", "SpeedTreeCommonAlphaScalarGrassNear",			1.f );
	TConfigVar<Float> cvSpeedTreeCommonAlphaScalarGrass				( "Rendering/SpeedTree", "SpeedTreeCommonAlphaScalarGrass",				4.f );
	TConfigVar<Float> cvSpeedTreeCommonAlphaScalarGrassDistNear		( "Rendering/SpeedTree", "SpeedTreeCommonAlphaScalarGrassDistNear",		3.f );
	TConfigVar<Float> cvSpeedTreeCommonAlphaScalarGrassDistFar		( "Rendering/SpeedTree", "SpeedTreeCommonAlphaScalarGrassDistFar",		15.f );
	TConfigVar<Float> cvSpeedTreeCommonAlphaScalarBillboards		( "Rendering/SpeedTree", "SpeedTreeCommonAlphaScalarBillboards",		1.1f );
	TConfigVar<Float> cvSpeedTreeCommonBillboardsGrainBias			( "Rendering/SpeedTree", "SpeedTreeCommonBillboardsGrainBias",			0.f );
	TConfigVar<Float> cvSpeedTreeCommonBillboardsGrainAlbedoScale	( "Rendering/SpeedTree", "SpeedTreeCommonBillboardsGrainAlbedoScale",	0.4f );
	TConfigVar<Float> cvSpeedTreeCommonBillboardsGrainNormalScale	( "Rendering/SpeedTree", "SpeedTreeCommonBillboardsGrainNormalScale",	0.f );
	TConfigVar<Float> cvSpeedTreeCommonBillboardsGrainClipScale		( "Rendering/SpeedTree", "SpeedTreeCommonBillboardsGrainClipScale",		1.15f );
	TConfigVar<Float> cvSpeedTreeCommonBillboardsGrainClipBias		( "Rendering/SpeedTree", "SpeedTreeCommonBillboardsGrainClipBias",		0.f );
	TConfigVar<Float> cvSpeedTreeCommonBillboardsGrainClipDamping	( "Rendering/SpeedTree", "SpeedTreeCommonBillboardsGrainClipDamping",	0.65f );
	TConfigVar<Float> cvSpeedTreeCommonGrassNormalsVariation		( "Rendering/SpeedTree", "SpeedTreeCommonGrassNormalsVariation",		0.15f );
}

/////////////////////////////////////////////////////////////////////////////
// Base scene lighting

CRenderBaseLightParams::CRenderBaseLightParams()
	: m_sunLightDiffuse( 2,2,2,2 )
	, m_sunLightDiffuseLightSide( 2,2,2,2 )
	, m_sunLightDiffuseLightOppositeSide( 2,2,2,2 )
	, m_hdrAdaptationDisabled( false )
{
	m_lightDirection = Vector( 1, 2, 3 ).Normalized3();
	m_sunDirection = m_lightDirection;
	m_moonDirection = m_lightDirection;
}

CRenderBaseLightParams::~CRenderBaseLightParams()
{
}


SCharacterWindParams::SCharacterWindParams()
{
	m_primaryDensity = 7.0f;
	m_secondaryDensity = 13.0f;
	m_primaryOscilationFrequency = 2.0f;
	m_secondaryOscilationFrequency = 2.0f;
	m_primaryDistance = 1.0f;
	m_secondaryDistance = 0.5f;
	m_gustFrequency = 1.8f;
	m_gustDistance = 1.0f;
}

/////////////////////////////////////////////////////////////////////////////

void SGlobalSpeedTreeParameters::ImportGlobalOverrides()
{
	struct Local
	{
		static void ImportParam( Float &refValue, const Config::TConfigVar<Float> &valueOverride )
		{
			if ( valueOverride.Get() > Config::SPEEDTREE_COMMON_VALUE_DISABLED )
			{
				refValue = valueOverride.Get();
			}
		}
	};

	Local::ImportParam( m_alphaScalar3d,				Config::cvSpeedTreeCommonAlphaScalar3d );
	Local::ImportParam( m_alphaScalarGrassNear,			Config::cvSpeedTreeCommonAlphaScalarGrassNear );
	Local::ImportParam( m_alphaScalarGrass,				Config::cvSpeedTreeCommonAlphaScalarGrass );
	Local::ImportParam( m_alphaScalarGrassDistNear,		Config::cvSpeedTreeCommonAlphaScalarGrassDistNear );
	Local::ImportParam( m_alphaScalarGrassDistFar,		Config::cvSpeedTreeCommonAlphaScalarGrassDistFar );
	Local::ImportParam( m_alphaScalarBillboards,		Config::cvSpeedTreeCommonAlphaScalarBillboards );
	Local::ImportParam( m_billboardsGrainBias,			Config::cvSpeedTreeCommonBillboardsGrainBias );
	Local::ImportParam( m_billboardsGrainAlbedoScale,	Config::cvSpeedTreeCommonBillboardsGrainAlbedoScale );
	Local::ImportParam( m_billboardsGrainNormalScale,	Config::cvSpeedTreeCommonBillboardsGrainNormalScale );
	Local::ImportParam( m_billboardsGrainClipScale,		Config::cvSpeedTreeCommonBillboardsGrainClipScale );
	Local::ImportParam( m_billboardsGrainClipBias,		Config::cvSpeedTreeCommonBillboardsGrainClipBias );
	Local::ImportParam( m_billboardsGrainClipDamping,	Config::cvSpeedTreeCommonBillboardsGrainClipDamping );
	Local::ImportParam( m_grassNormalsVariation,		Config::cvSpeedTreeCommonGrassNormalsVariation );
}

/////////////////////////////////////////////////////////////////////////////
// SWorldMotionBlurSettings

SWorldMotionBlurSettings::SWorldMotionBlurSettings()
	: m_isPostTonemapping( true )
	, m_distanceNear( 40.f )
	, m_distanceRange( 100.f )
	, m_strengthNear( 0.3f )
	, m_strengthFar( 0.8f )
	, m_fullBlendOverPixels( 15.f )
	, m_standoutDistanceNear( 5.f )
	, m_standoutDistanceRange( 5.f )
	, m_standoutAmountNear( 1.f )
	, m_standoutAmountFar( 0.f )
	, m_sharpenAmount( 0.f )
{}

/////////////////////////////////////////////////////////////////////////////
// SDayPointEnvironmentParams

SDayPointEnvironmentParams::SDayPointEnvironmentParams ()
{
	Reset();
}

void SDayPointEnvironmentParams::Reset()
{
	m_globalLightDirection = Vector (0, 0, -1);
	m_sunDirection = Vector (0, 0, 1);
	m_moonDirection = Vector (0, 0, 1);
	m_skyBoxWeatherBlend = Vector(0,0,0);
	m_waterShadingParams = Vector(0.0f, 0.0f, 0.0f, 0.6f);
	m_waterShadingParamsExtra = Vector(1.0f, 1.0f, 0.1f, 0.4f);
	m_waterShadingParamsExtra2 = Vector(0.6f, 0.8f, 0.7f, 1.0f);
	m_waterFoamIntensity = 0.0f;
	m_underWaterBrightness = 1.0f;
	m_underWaterFogIntensity = 0.95f;

	m_immediateWetSurfaceEffectStrength = 0.0f;
	m_delayedWetSurfaceEffectStrength = 0.0f;
	
	m_weatherEffectStrength = 0.0f;
	
	m_gamma = 2.2f;	
	m_toneMappingAdaptationSpeedUp = 15.0f;
	m_toneMappingAdaptationSpeedDown = 15.0f;
	m_cloudsShadowTexture.Reset();
		
	m_fakeCloudsShadowTexture.Reset();
	m_fakeCloudsShadowSize = 0.05f;
	m_fakeCloudsShadowSpeed = 0.01f;

	m_cameraDirtTexture.Reset();
	m_cameraDirtNumVerticalTiles = 1;
	m_vignetteTexture.Reset();
	m_interiorFallbackAmbientTexture.Reset();
	m_interiorFallbackReflectionTexture.Reset();
	m_skyDayAmount = 1.f;
	m_convergence = 0.32f;
	m_nearPlaneWeightForced = 1.f;
	m_useMoonForShafts = false;
	m_bokehDofParams.Reset();
}

CRenderResourceSmartPtr SDayPointEnvironmentParams::GetBestFitNormalsTexture() const
{
	if (GDeferredInit)
		return CRenderResourceSmartPtr();
	else
		return resNormalsBestFitTexture.LoadAndGet<CBitmapTexture>();
}
