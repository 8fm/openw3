#define MAX_LIGHTS_PER_TILE					256
#define TILED_DEFERRED_DIMMERS_CAPACITY		192
#define TILED_DEFERRED_LIGHTS_CAPACITY		256
#define MAX_DIMMERS_PER_TILE				192
#define MAX_GRADIENT_KEYS					16

#include "common.fx"

#define TILE_SIZE							16
#define TILE_SIZE_INTERIOR_FULLRES			(TILE_SIZE * WEATHER_VOLUMES_SIZE_DIV)

#include "include_sharedConsts.fx"
#include "include_globalFog.fx"
#include "include_msaa.fx"
#include "include_utilities.fx"
#include "include_envProbe.fx"

#define b_add(base, blend) 			min(base + blend, 1.0)
#define b_substract(base, blend) 	max(base + blend - 1.0, 0.0)
#define b_lighten(base, blend) 		max(blend, base)
#define b_darken(base, blend) 		min(blend, base)
#define b_linearlight(base, blend) 	(blend < 0.5 ? b_linearburn(base, (2.0 * blend)) : b_lineardodge(base, (2.0 * (blend - 0.5))))
#define b_screen(base, blend) 		(1.0 - ((1.0 - base) * (1.0 - blend)))
#define b_overlay(base, blend) 		(base < 0.5 ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend)))
#define b_softlight(base, blend) 	((blend < 0.5) ? (2.0 * base * blend + base * base * (1.0 - 2.0 * blend)) : (sqrt(base) * (2.0 * blend - 1.0) + 2.0 * base * (1.0 - blend)))
#define b_colordodge(base, blend) 	((blend == 1.0) ? blend : min(base / (1.0 - blend), 1.0))
#define b_colorburn(base, blend) 	((blend == 0.0) ? blend : max((1.0 - ((1.0 - base) / blend)), 0.0))
#define n_vividlight(base, blend) 	((blend < 0.5) ? b_colorburn(base, (2.0 * blend)) : b_colordodge(base, (2.0 * (blend - 0.5))))
#define b_pinlight(base, blend) 	((blend < 0.5) ? b_darken(base, (2.0 * blend)) : b_lighten(base, (2.0 *(blend - 0.5))))
#define b_hardmix(base, blend) 		((b_vividlight(base, blend) < 0.5) ? 0.0 : 1.0)
#define b_reflect(base, blend) 		((blend == 1.0) ? blend : min(base * base / (1.0 - blend), 1.0))

Texture2D					volumeDepthTexture 		: register(t24);
TEXTURE2D_ARRAY<float4> 	CloudsShadowTexture 	: register(t14);
SamplerState 				CloudsShadowSampler		: register(s14);
SamplerState				samplPoint				: register(s8);
SamplerComparisonState		samplShadowComparison	: register(s9);
SamplerState				samplLinear				: register(s10);

//dex++: terrain shadows
TEXTURE2D_ARRAY<float> TerrainShadowAtlas : register(t19);

//dex++: in defered we need to reproject pixels back to terrain to prevent precission issues
// in forward we do not need to do that as the world position is accurate, we gain a extra texture slot
#ifdef DEFERRED
	TEXTURE2D_ARRAY<float> TerrainClipMap : register(t15);
#else
#endif

#ifndef DEFERRED
	// wrap in another ifndef to avoid defining FORWARD more than once, which can break shader parsing/preprocessing
	#ifndef FORWARD
		#define FORWARD
	#endif
#endif

struct DimmerParams
{
	float4x3 viewToLocal;
	float3 normalScale;
	uint  fadeAlphaAndInsideMarkFactor;
	float outsideMarkFactor;
	float insideAmbientLevel;
	float outsideAmbientLevel;
	float marginFactor;
};

struct LightParams
{
	float4 positionAndRadius;
	float4 positionAndRadiusCull;
	float4 direction;
	float4 colorAndType;
	float4 params; // in case of spot, spot params
	float4 params2;
	float4 staticShadowmapRegion;
	float4  dynamicShadowmapRegions0;
	float4  dynamicShadowmapRegions1;
};

START_CB( CSConstants, 13 )
	// global light
	float4 lightDir;
	float4 lightColor;
	float4 lightSpecularColorAndShadowAmount;		// ace_fix: remove - seems it's not being used anymore
	float4 lightTweaks;
	
	// global shadows
	float4x4 mShadowTransform;
	float4 ShadowOffsetsX;
	float4 ShadowOffsetsY; 
	float4 ShadowHalfSizes;
	float4 ShadowParams[4]; // MAX CASCADES ( offsetX, offsetY, size, filterSize )
	float4 ShadowPoissonOffsetAndBias; // poison offset, bias, num cascades
	float4 ShadowTextureSize;
	float4 ShadowFadeScales;
	int	   ShadowQuality; //dex: general shadow quality params
	float4 SpeedTreeShadowParams[4]; // MAX CASCADES ( filterSize, shadowGradient, 0, 0 )
	
	// ambient
	float4 ambientColorTop;
	float4 ambientColorBottom;
	float4 ambientColorFront;
	float4 ambientColorBack;
	float4 ambientColorSide;
		
	// sky
	float4 _skyColor;				// ace_fix: remove - unused
	float4 _skyColorHorizon;		// ace_fix: remove - unused
	float4 _sunColorHorizon;		// ace_fix: remove - unused
	float4 _sunBackHorizonColor;	// ace_fix: remove - unused
	float4 _sunColorSky;			// ace_fix: remove - unused
	
	//
	float4 interiorParams;
	float4 interiorRangeParams;
	
	// terrain shadows
	int iNumTerrainShadowsWindows;
	int iNumTerrainTextureWindows;
	float4 vTerrainShadowsParams;
	float4 vTerrainShadowsParams2; // ( offset, scale, terrain_smooth, mesh_smooth )
	float4 vTerrainShadowsParams3; // terrain heightmap decompression
	float4 vTerrainShadowsWindows[5];
	float4 vTerrainTextureWindows[5];

	//
	float4 histogramParams_UNUSED;

	float4 ambientShadowAmount;		//< ace_fix: remove

	float4 lightColorLightSide;
	float4 lightColorLightOppositeSide;
	float4 vShadowDepthRanges;

	float4 characterLightParams;
	float4 characterEyeBlicksColor;	//< shadowed scale in alpha
	float4 fresnelGainParams;
	// KEEP AT THE END !

	// 
	int lightNum;
	LightParams lights[TILED_DEFERRED_LIGHTS_CAPACITY];

	//
	int dimmerNum;
	DimmerParams dimmers[TILED_DEFERRED_DIMMERS_CAPACITY];
END_CB

// see globalWater.h also
#define NUM_LAKES_MAX			44
#define NUM_DISPLACEMENTS_MAX	16

START_CB( WaterConstants, 4 )
	struct SGlobalWater
	{
		float4	uvScale;
		float4	amplitudeScale;
		float	tessFactor;
		float	dynamicWaterResolution;		
		float	dynamicWaterResolutionInv;
		//padding1
		float4	choppyFactors;		
		
	} GlobalWater;

	struct SSimulationCamera
	{
		float4	position;
		float	nearPlane;
		float	farPlane;
		float	tanFov;
		float	tanFov_x_ratio;
	} SimulationCamera;

	struct SLakes
	{
		uint 	numLakes;
		// padding3
		float4	offsets[NUM_LAKES_MAX];
	} Lakes;

	struct SDisplacements	//Boats'n'stuff
	{
		uint 	numDispl;
		// padding3
		float4	displacementData[ NUM_DISPLACEMENTS_MAX * 4 ];	// 1 boat = 1 matrix, but somebody decided it's a good idea to hide that fact and store it in rows. Hence the *4.
	} Displacements;
END_CB

float rand(float2 co)
{
    return frac(sin(dot(co.xy ,float2(12.9898f,78.233f))) * 43758.5453f);
}

bool GetFloatBitValueByMask( float value, uint mask )
{
	return asuint( value ) & mask;
}

bool GetFloatBitValue( float value, uint bitIndex )
{
	return GetFloatBitValueByMask( value, 1 << bitIndex );
}

float GetLocalLightsAttenuationInteriorScale( float flags, float interiorFactor )
{
	float att = 1;
	att *= GetFloatBitValueByMask( flags, SHADER_LIGHT_USAGE_MASK_INTERIOR ) ? 1 : (1 == interiorFactor ? 1 : 0);
	att *= GetFloatBitValueByMask( flags, SHADER_LIGHT_USAGE_MASK_EXTERIOR ) ? 1 : (1 == interiorFactor ? 0 : 1);
	return att;
}		

bool IsLightFlagsCharacterModifierEnabled( float flags )
{
	return GetFloatBitValueByMask( flags, SHADER_LIGHT_USAGE_MASK_CAMERA_LIGHT );
}

bool IsSpot( LightParams light )
{
	return GetFloatBitValue( light.colorAndType.w, 0 );
}

bool GetLightUsageFlag( LightParams light, uint flagIndex )
{
	//index corresponds to "LUM_Custom" number
	return GetFloatBitValue( light.colorAndType.w, flagIndex + 1);
}

bool GetLightUsageMask( LightParams light, uint mask )
{
	//return strue if at least one flag matches
	return GetFloatBitValueByMask( light.colorAndType.w, mask << 1 );
}

float4 GetGradientColor( float position, int numPoints, float positions[MAX_GRADIENT_KEYS], float4 colors[MAX_GRADIENT_KEYS], int interpolation, bool cyclic )
{
	float4 colorA = float4( 0.0f, 0.0f, 0.0f, 1.0f );
	float4 colorB = float4( 0.0f, 0.0f, 0.0f, 1.0f );
	float lerpFactor = 0.5f;
	if( position >= positions[ 0 ] && position <= positions[ numPoints - 1] )
	{
		[unroll]
		for ( int i = 0; i < numPoints - 1; i++ )
		{
			if ( position >= positions[ i ] && position <= positions[ i + 1 ] )
			{
				colorA = colors[ i ];
				colorB = colors[ i + 1 ];
				lerpFactor = ( position - positions[ i ] ) / ( positions[ i + 1 ] - positions[ i ] );
				// This is a fix for "fatal internal compiler error" in PS4 SDK 1.7.5 and below
				// TODO: switch back to "break;" after installing PS4 SDK 2.0
				switch( interpolation )
				{
				case 0 : //No Interpolation
					return colorA;
				case 1 : //Linear Interpolation
					return lerp( colorA, colorB, lerpFactor );
				default: //Default smooth
					return lerp( colorA, colorB, smoothstep( 0.0f, 1.0f, lerpFactor ) );
				}
			}
		}	
	}
	else
	{
		if( cyclic )
		{
			colorA = colors[ numPoints - 1];
			colorB = colors[ 0 ];

			if ( position <= positions[ 0 ] )
			{
				lerpFactor = ( position + 1 - positions[ numPoints - 1 ] ) / ( positions[ 0 ] + 1 - positions[ numPoints - 1 ] );
			}
			else
			{
				lerpFactor = ( position - positions[ numPoints - 1 ] ) / ( positions[ 0 ] + 1 - positions[ numPoints - 1 ] );
			}
		}
		else
		{
			if ( position <= positions[ 0 ] )
			{
				colorA = colors[ 0 ];
				colorB = colors[ 0 ];
			}
			else
			{
				colorA = colors[ numPoints - 1];
				colorB = colors[ numPoints - 1];
			}
		}
	}

	switch( interpolation )
	{
	case 0 : //No Interpolation
		return colorA;
	case 1 : //Linear Interpolation
		return lerp( colorA, colorB, lerpFactor );
	default: //Default smooth
		return lerp( colorA, colorB, smoothstep( 0.0f, 1.0f, lerpFactor ) );
	}
}

float3 Lambert( in float3 L, in float3 N, in float3 V, in float translucency )
{
	float  NL                  = dot(N,L);
	return saturate( NL ).xxx;
}

float BlinnPhong( in float3 L, in float3 N, in float3 V, in float glossiness )
{	
	float _spec = 0;
	if ( dot(L,N) > 0.0f )
	{
		float3 H = normalize( V+L );

		//	float R0 = 0.1f;
		//float fresnelTerm = R0 + (1-R0)*pow(saturate(dot(H,L)), 5.0f);
		float fresnelTerm = 0.1f + 0.9f * pow(saturate(dot(H,L)), 5.0f);

		_spec = fresnelTerm * pow( saturate( dot(H,N) ), glossiness ) / max(dot(V,N),dot(L,N));
	}
	
	return _spec;
}

#if defined(COMPILE_IN_SHADING_HAIR)
	struct SCompileInLightingParams
	{
		float3	vertexNormal;
		float3 tangent;
		float3 bitangent;
		float  anisotropy;
		float4	data0;
		float4	data1;
		float4	data2;
	};
#elif defined(COMPILE_IN_SHADING_SKIN)
	struct SCompileInLightingParams
	{
		float3	vertexNormal;
		float4	data0;
		float4	data1;
		float4	data2;
		float4	data3;
		float4	data4;
		float4	data5;
		float4	data6;
		float	ao;
	};
#elif defined(COMPILE_IN_SHADING_EYE)
	struct SCompileInLightingParams
	{
		float3	vertexNormal;
		float4	data0;
		float4	data1;
		float4	data2;
	};
#elif defined(COMPILE_IN_DYNAMIC_DECALS)
	struct SCompileInLightingParams
	{
		uint stencilValue;
	};
#else
	struct SCompileInLightingParams
	{
		// empty		
	};
#endif

float3 CalcFresnelPBRPipeline( in float3 V, in float3 H, in float3 specularity, in float roughness, in float translucency )
{
	const float pow_base  = saturate(1 - abs(dot(H,V))); // abs instead of saturate removes full reflection envmap on the backfacing foliage leaves
	const float pow_base2 = pow_base * pow_base;
	const float pow_base4 = pow_base2 * pow_base2;
	const float pow_base5 = pow_base4 * pow_base;

	const float dim_range = fresnelGainParams.z;
	const float dim_factor = 1 - roughness;
	return specularity + fresnelGainParams.x * max( 0, 1 - specularity ) * pow_base5 / ((dim_range+1) - dim_range * dim_factor);
}

float3 CalcFresnelPBRPipelineEnvProbeReflection( in float3 V, in float3 H, in float3 specularity, in float roughness, in float translucency, float3 H_flat, float extraGainScale )
{
	float dotHV = dot(H,V);
	dotHV = abs( dotHV );
	dotHV = max( dotHV, abs( dot(H_flat,V) ) );

	const float pow_base  = saturate(1 - abs(dotHV)); // abs instead of saturate removes full reflection envmap on the backfacing foliage leaves
	const float pow_base2 = pow_base * pow_base;
	const float pow_base4 = pow_base2 * pow_base2;
	const float pow_base5 = pow_base4 * pow_base;

	const float dim_range = fresnelGainParams.w;
	const float dim_factor = 1 - roughness;
	return specularity + min( extraGainScale, fresnelGainParams.y ) * max( 0, 1 - specularity ) * pow_base5 / ((dim_range+1) - dim_range * dim_factor);
}

float3 Checker( float2 uv, float3 colorA, float3 colorB )
{
	float2 stripes = floor(frac(uv) * 2.0f);
	return stripes.x == stripes.y ? colorA : colorB;
}

#if defined(COMPILE_IN_SHADING_SKIN)
float3 CalculateSkinTranslucencyGain( in float3 L, in float3 V, in float3 N, in float translucency, in SCompileInLightingParams extraLightingParams )
{
	float3 gain = 0;

	[branch]
	if ( translucency > 0 )
	{
		float4 _translucencyParams0 = 0;
		_translucencyParams0.x = 0.2; // params.m_translucencyViewDependency.GetScalar(), 
		_translucencyParams0.y = 0; // UNUSED params.m_translucencyBaseFlatness.GetScalar(), 
		_translucencyParams0.z = 0; // UNUSED params.m_translucencyFlatBrightness.GetScalar() / (4.f * M_PI),
		_translucencyParams0.w = 0.381; // params.m_translucencyGainBrightness.GetScalar() );

		float view_dependency_factor = _translucencyParams0.x;			

		float scatter_normal = (dot( L, N ) + 1) * 0.5;
		float back_contribution = (dot( L, -V ) + 1) * 0.5;
		float reduction_normal = (dot( N, -V ) + 1) * 0.5;

		back_contribution *= back_contribution;
		back_contribution *= back_contribution;

		const float transmission_gain = _translucencyParams0.w * reduction_normal * lerp( scatter_normal, back_contribution, view_dependency_factor );

		//const float3 uniformDiffuse = _translucencyParams0.z * (1 - specularity);
		//diffuse = lerp( diffuse, uniformDiffuse, _translucencyParams0.y * translucency );
		//diffuse += transmission_gain * translucency;

		//const float  sssDiffuseScale			= extraLightingParams.data6.z;		// 1

		gain += transmission_gain * translucency;// * sssDiffuseScale;
	}

	return gain;
}
#endif

float3 NormalizeDiffusePBRPipeline( float3 diffuse, in float3 L, in float3 V, in float3 N, in float translucency, float3 specularity, float roughness )
{
	float3 normFactor = (1 - CalcFresnelPBRPipeline( V, normalize( V+L ), specularity, roughness, translucency ));		//< 'new order' style
	
	diffuse *= normFactor;
	diffuse /= PI;
	
#if !defined(COMPILE_IN_SHADING_SKIN)
	[branch]
	if ( translucency > 0 )
	{
		float view_dependency_factor = translucencyParams0.x;			

		float scatter_normal = (dot( L, N ) + 1) * 0.5;
		float back_contribution = (dot( L, -V ) + 1) * 0.5;
		float reduction_normal = (dot( N, -V ) + 1) * 0.5;

		back_contribution *= back_contribution;
		back_contribution *= back_contribution;

		const float transmission_gain = translucencyParams0.w * reduction_normal * lerp( scatter_normal, back_contribution, view_dependency_factor );

		const float3 uniformDiffuse = translucencyParams0.z * (1 - specularity);
		diffuse = lerp( diffuse, uniformDiffuse, translucencyParams0.y * translucency );
		diffuse += transmission_gain * translucency;
	}
#endif
	
	return diffuse;
}

float3 NormalizeAmbientPBRPipeline( float3 ambient, in float translucency, in float3 V, in float3 N, float3 specularity, in float roughness, float3 N_flat, float extraGainScale )
{
	float3 normFactor = 1 - CalcFresnelPBRPipelineEnvProbeReflection( V, N, specularity, roughness, translucency, N_flat, extraGainScale );

	ambient *= normFactor;

	return ambient;
}

float3 CalcPoweredWrapLightNormFactor( float3 e, float w )
{
	/*
	valUp = Integrate[ (Cos[y] + e) / (1 + e) * Sin[y], { x, 0, 2 * Pi}, {y, 0, Pi/2 }  ]
	valDown = Integrate[ (1 - (1 - Power[1 - (-Cos[y]/e), w]))*(e/(1 + e)) * Sin[y], { x, 0, 2 * Pi}, {y, Pi/2, ArcCos[-e] }, Assumptions -> { e >= 0, w >= 1 } ]
	valSum = valUp + valDown
	Simplify[valSum]
	*/
	// btw: simplified version for w=2
	// return 0.1 * 3 * (1 + e) / (3 + 6 * e + 2 * e * e);
	return (1 + e) * (1 + w) / (1 + 2 * e * e + w + 2 * e * (1 + w));
}

float3 DiffuseLightingPBRPipeline( out float3 outDiffPreNorm, in float3 L, float3 N, in float3 V, in float translucency, float3 specularity, float roughness, in SCompileInLightingParams extraLightingParams, float2 bleedScaleBias = float2(1,0) )
{
    float3 _diff = 0;

	float postNormFactor = 1;

#if defined(COMPILE_IN_SHADING_SKIN)

	const float3 sssDetailBleed			= extraLightingParams.data0.xyz; // pow already there			float3( 0.65, 0.25, 0 );
	const float  sssCoarseWrap			= extraLightingParams.data2.z;									// 0.1
	const float3 sssCoarseColor			= extraLightingParams.data5.xyz; // pow already there			float3( 1.f, 0.85, 0.55 );
	const float  sssCoarseColorInvRange	= extraLightingParams.data6.x;		// 3.333
	const float  sssDiffuseDistribPow		= extraLightingParams.data6.y;		// 1.1
	const float  sssDiffuseScale			= extraLightingParams.data6.z;		// 1

	const float3 VN = normalize( extraLightingParams.vertexNormal.xyz );

	const float _bumpiness = saturate( lerp( extraLightingParams.data2.x, extraLightingParams.data2.y, pow( saturate( (dot(VN,L) + sssCoarseWrap) / (1.0 + sssCoarseWrap) ), extraLightingParams.data2.w ) ) );
	N = normalize( lerp( VN, N, _bumpiness ) );

	postNormFactor = sssDiffuseScale;
		
	float coarseWrapScale = 1.0 / (1.0 + sssCoarseWrap);
	float coarseWrapBias = sssCoarseWrap * coarseWrapScale;
	float coarseNL = dot( N, L ) * coarseWrapScale + coarseWrapBias;
	float3 N_r = normalize( lerp( VN, N, 1.0 - sssDetailBleed.x ) );
	float3 N_g = normalize( lerp( VN, N, 1.0 - sssDetailBleed.y ) );
	float3 N_b = normalize( lerp( VN, N, 1.0 - sssDetailBleed.z ) );
	float3 NL = float3 (
		saturate( max( dot( N_r, L ) * coarseWrapScale + coarseWrapBias, coarseNL ) ),
		saturate( max( dot( N_g, L ) * coarseWrapScale + coarseWrapBias, coarseNL ) ),
		saturate( max( dot( N_b, L ) * coarseWrapScale + coarseWrapBias, coarseNL ) ) );
	NL = pow( NL, sssDiffuseDistribPow );
	NL *= lerp( sssCoarseColor, 1, saturate( coarseNL * sssCoarseColorInvRange ) );

	float3 ___diff = saturate( NL );

#elif defined(COMPILE_IN_SHADING_HAIR)

	float3 VN = normalize( extraLightingParams.vertexNormal.xyz );

	float _extent = 1;
	_extent *= saturate( (dot( VN, L ) + extraLightingParams.data0.x) / (1 + extraLightingParams.data0.x)  );

	float3  NL = (dot(N,L) + _extent) / (1.0 + _extent);

	postNormFactor = 1.0 / (1.0 + _extent);

	float3 ___diff = saturate( NL ).xxx;

#elif defined(COMPILE_IN_SHADING_EYE)

	float3 VN = normalize( extraLightingParams.vertexNormal.xyz );

	float _extent = extraLightingParams.data0.x;
	
	float3  NL = (dot(N,L) + _extent) / (1.0 + _extent);
	
	postNormFactor = 1.0 / (1.0 + _extent);
	
	float3 ___diff = saturate( NL ).xxx;

#else
	float  NL = dot(N,L);
	#if defined( ENABLE_BILLBOARDS_LIGHT_BLEED )
		#if defined( BILLBOARDS_LIGHT_BLEED_WRAP )
			float3 ___diff = saturate( NL * bleedScaleBias.x + bleedScaleBias.y );
		# else
			float3 ___diff = (saturate( NL ) * bleedScaleBias.x + bleedScaleBias.y);
		#endif
	#else
		float3 ___diff = saturate( NL ).xxx;
	#endif
#endif

	outDiffPreNorm = ___diff;

	_diff   = ___diff;
	_diff = NormalizeDiffusePBRPipeline( _diff, L, V, N, translucency, specularity, roughness );
	_diff *= postNormFactor;

	return _diff.xyz;
}

float3 DiffuseLightingPBRPipeline( in float3 L, float3 N, in float3 V, in float translucency, float3 specularity, float roughness, in SCompileInLightingParams extraLightingParams, float2 bleedScaleBias = float2(1,0) )
{
	float3 preNormDecoy = 0;
	return DiffuseLightingPBRPipeline( preNormDecoy, L, N, V, translucency, specularity, roughness, extraLightingParams, bleedScaleBias );
}

float3 SpecularLightingPBRPipeline( in float3 L, in float3 N, in float3 V, in float3 specularity, in float roughness, in float translucency, in SCompileInLightingParams extraLightingParams )
{
    float3 _spec = 0;

	[branch]
	if ( dot(L,N) > 0.0f )
	{
		float3 H = normalize( V+L );
		float3 F = 0.0f;
		
		const float NH = dot( N, H );
		const float NV = dot( N, V );
		const float NL = dot( N, L );
		const float sNH = saturate( NH );
		//const float sNV = saturate( NV );
		const float sNL = saturate( NL );
		
		float D = 0.0f;
		
		// D term
				
		#if defined(COMPILE_IN_SHADING_HAIR)
		{	
			float alphaT = roughness * roughness;
			float alphaB = lerp( 0, alphaT, 1 - extraLightingParams.anisotropy );

			{	
				// this fixup is needed here to prevent division by zero.
				// btw: on most of the gpu's there were no visual problems, but on some 
				//      gpu'a alphaBlending get's messed up without this fix.
				alphaT = max( 0.0001, alphaT );
				alphaB = max( 0.0001, alphaB );
			}

			const float shiftMask = extraLightingParams.data0.z;
			
			float3 M = H;
			float3 T = normalize( extraLightingParams.tangent );
			T = normalize( T - N * dot( T, N ) );
			float3 B = normalize( cross( T, N ) + N * shiftMask );
			
			float a0 = pow( dot( T, M ) / alphaT, 2.0 );
			float a1 = pow( dot( B, M ) / alphaB, 2.0 );
			float a2 = pow( saturate(dot( N, M )), 2.0 );

			D = 1.0 / (3.14152 * alphaT * alphaB * pow( a0 + a1 + a2, 2.0 ));
		}
		#elif defined(COMPILE_IN_SHADING_SKIN)
		{
			float a = pow(roughness, 2.0f);
			float aSq = pow(a, 2.0f);
			D = aSq/( 3.14152f*( pow( ( pow( sNH, 2.0f )*( aSq-1.0f ) + 1.0f ), 2.0f ) ) );
		}
		#else
		{
			float a = pow(roughness, 2.0f);
			float aSq = pow(a, 2.0f);
			D = aSq/( 3.14152f*( pow( ( pow( sNH, 2.0f )*( aSq-1.0f ) + 1.0f ), 2.0f ) ) );
		}
		#endif

		// 'new order' trick to use NV instead of sNV to avoid some specular discontinuities.
		// actually I added the 'abs' because the result is more plausible IMO
		float manipulatedNV = abs( NV ); 

		//G term
		float k = pow( (roughness + 1.0f), 2.0f )/8;
		float Gl = sNL/(sNL*(1-k) + k);
		float Gv = manipulatedNV/(manipulatedNV*(1-k) + k);
		float G = Gl * Gv;
		
		//F term
		F = CalcFresnelPBRPipeline( V, H, specularity, roughness, translucency );
				
		_spec.xyz = D*G*F/(4.0f*manipulatedNV*sNL);	
		_spec *= sNL;
	}

	return _spec;
}

float3 PositionFromDepth(in float depth, in uint2 pixelCoord, in uint2 customScreenDimensions )
{
    float2 cpos = (pixelCoord + 0.5f) / customScreenDimensions.xy;
    cpos *= 2.0f;
    cpos -= 1.0f;
    cpos.y *= -1.0f;
    float4 positionWS = mul(screenToWorld, float4(cpos, depth, 1.0f) );
    return positionWS.xyz / positionWS.w;
}

float3 PositionFromDepth(in float depth, in uint2 pixelCoord)
{
	return PositionFromDepth( depth, pixelCoord, screenDimensions.xy );
}

#if (defined DEFERRED ) || (defined FORWARD)

//dex++
TEXTURE2D_ARRAY<float>  ShadowMapGlobal : register(t8);
TEXTURE2D_ARRAY<float>  DynamicShadowMapsArray : register(t9);
TEXTURECUBE_ARRAY<float> StaticShadowMapsArray : register(t10);
//dex--

#include "include_pcf.fx"

struct LightingData
{
	float3 preNormDiffuse;
	float3 diffuse;
	float3 specular;
};

//dex++: gauss weights
static const float GaussWeights[5][5] = {
	{ 1, 4, 7, 4, 1 },
	{ 4, 16, 26, 16, 4 }, 
	{ 7, 26, 41, 26, 7 }, 
	{ 4, 16, 26, 16, 4 }, 
	{ 1, 4, 7, 4, 1 }
};
//dex--

float CalcLightAttenuation( in int lightIndex, in float3 worldPosition, in float3 lightVectorUnnormalized )
{
	const float dist = length(lightVectorUnnormalized);
	const float dist2 = dist * dist * lights[lightIndex].params2.x;
	const float norm_dist = dist / lights[lightIndex].positionAndRadius.w;
	const float norm_dist2 = norm_dist * norm_dist;
	const float norm_dist4 = norm_dist2 * norm_dist2;
	const float att_num = saturate( 1 - norm_dist4 );
	const float att_num2 = att_num * att_num;
	
	//
#if !(defined(IS_PURE_DEFERRED_LIGHT_POINT) || defined(IS_PURE_DEFERRED_LIGHT_SPOT))
	const bool isSpot = IsSpot( lights[lightIndex] );
	const bool isShadowedDynamic = lights[lightIndex].params2.z > 0.0f;
	const bool isShadowedStatic = lights[lightIndex].staticShadowmapRegion.x > 0.0f;
#else
	#if defined(IS_PURE_DEFERRED_LIGHT_SPOT)
	const bool isSpot = true;
	#else
	const bool isSpot = false;
	#endif

	#if defined(IS_PURE_DEFERRED_LIGHT_SHADOWED) && IS_PURE_DEFERRED_LIGHT_SHADOWED
	const bool isShadowedDynamic = lights[lightIndex].params2.z > 0.0f;
	const bool isShadowedStatic = lights[lightIndex].staticShadowmapRegion.x > 0.0f;
	#else
	const bool isShadowedDynamic = false;
	const bool isShadowedStatic = false;
	#endif
#endif
		
	// attenuation
	float att0 = att_num2 / (dist2 + 1);

	// shadow factor
	float shadowFactor = 1.0f;
		
	// spot	angle attenuation
	if ( isSpot )
	{
		float angleCosine = dot( -normalize(lightVectorUnnormalized), lights[lightIndex].direction.xyz );
		att0 *= pow( saturate( angleCosine * lights[lightIndex].params.y + lights[lightIndex].params.z ), lights[lightIndex].params.w );
	}
	
	// optional dynamic shadows
	[branch]
	if ( isShadowedDynamic && (att0 > 0.0f) )
	{
		float4 sideInfo;
		if ( isSpot )
		{
			float3 localPos = worldPosition.xyz - lights[lightIndex].positionAndRadius.xyz;

			float3 v2 = lights[lightIndex].direction.xyz;
			float3 v1 = lights[lightIndex].dynamicShadowmapRegions0.yzw;
			float3 v0 = cross( v2, v1 );

			float3x3 mat = float3x3( v0, v1, v2 );
			localPos = mul( mat, localPos );

			sideInfo = float4( localPos, lights[lightIndex].dynamicShadowmapRegions0.x );
		}
		else
		{
			float3 localPos = worldPosition.xyz - lights[lightIndex].positionAndRadius.xyz;

			float3 L = lightVectorUnnormalized / dist;
			float3 aL = abs( L );

// 			float4 sideInfoX = L.x > 0 ? float4( localPos.y, localPos.z, -localPos.x, lights[lightIndex].dynamicShadowmapRegions0.x )  : float4( -localPos.y, localPos.z, localPos.x, lights[lightIndex].dynamicShadowmapRegions0.y );
// 			float4 sideInfoY = L.y > 0 ? float4( -localPos.x, localPos.z, -localPos.y, lights[lightIndex].dynamicShadowmapRegions0.z ) : float4( localPos.x, localPos.z, localPos.y, lights[lightIndex].dynamicShadowmapRegions0.w );
// 			float4 sideInfoZ = L.z > 0 ? float4( localPos.x, localPos.y, -localPos.z, lights[lightIndex].dynamicShadowmapRegions1.x )  : float4( localPos.x, -localPos.y, localPos.z, lights[lightIndex].dynamicShadowmapRegions1.y );
// 
// 			sideInfo = ( aL.x > max(aL.y, aL.z) ) ? sideInfoX : (( aL.y > max(aL.x, aL.z) ) ? sideInfoY : sideInfoZ );

			if ( aL.x > max(aL.y, aL.z) )			sideInfo = L.x > 0 ? float4( localPos.y, localPos.z, -localPos.x, lights[lightIndex].dynamicShadowmapRegions0.x )  : float4( -localPos.y, localPos.z, localPos.x, lights[lightIndex].dynamicShadowmapRegions0.y );
			else if ( aL.y > max(aL.x, aL.z) )		sideInfo = L.y > 0 ? float4( -localPos.x, localPos.z, -localPos.y, lights[lightIndex].dynamicShadowmapRegions0.z ) : float4( localPos.x, localPos.z, localPos.y, lights[lightIndex].dynamicShadowmapRegions0.w );
			else									sideInfo = L.z > 0 ? float4( localPos.x, localPos.y, -localPos.z, lights[lightIndex].dynamicShadowmapRegions1.x )  : float4( localPos.x, -localPos.y, localPos.z, lights[lightIndex].dynamicShadowmapRegions1.y );
		}

		// calculate position projected to shadow space
		float2 posShadowSpace = 0;
		{				
			float projScale = lights[lightIndex].params2.y;
			float proj0 = lights[lightIndex].dynamicShadowmapRegions1.z;
			float proj1 = lights[lightIndex].dynamicShadowmapRegions1.w;
			float4x4 tform = float4x4( float4(projScale,0,0,0), float4(0,projScale,0,0), float4(0,0,proj0,proj1), float4(0,0,1,0) );

			float4 shpos = mul( tform, float4(sideInfo.xyz,1) );
			posShadowSpace = shpos.xy / shpos.w * float2( 0.5, -0.5 ) + 0.5;
		}

		uint regionData = asuint( sideInfo.w );
		float2 regionOffset;
		regionOffset.x = ((regionData >> 20) & ((1<<10)-1));
		regionOffset.y = ((regionData >> 10) & ((1<<10)-1));
		float regionSize = ((regionData >> 0) & ((1<<10)-1));
		float regionSlice = (regionData >> 30);
		
		[branch]
		if ( regionSize > 0.0f )
		{
			float3 shadowCoord = float3( (regionOffset + ( posShadowSpace * regionSize )) / 1024.f, regionSlice );
			float samplesESM = SAMPLE_LEVEL( DynamicShadowMapsArray, samplLinear, shadowCoord.xyz, 0 );
			shadowFactor *= saturate( exp( 100.0f * (samplesESM - norm_dist * 0.99) ));
		}
	}

	// optional static shadows
	[branch]
	if ( isShadowedStatic && (att0 > 0.0f) )
	{
		const float cubeIndex = lights[lightIndex].staticShadowmapRegion.y;
		const float4 cubeCoords = float4( lightVectorUnnormalized.xyz, cubeIndex );
		
		// ESM shadows, more narrow blend range to prevent bleeding from static geometry
		float samplesESM = SAMPLE_LEVEL( StaticShadowMapsArray, samplLinear, cubeCoords, 0 );
		shadowFactor *= saturate( exp( 100.0f * (samplesESM - norm_dist * 0.99) ));
	}
	
	// shadow fading
	shadowFactor = lerp( 1.0f, shadowFactor, lights[lightIndex].direction.w );
	
	// return merged attenuation and shadows
	return att0.x * shadowFactor;
}

float CalculateShadowSurfaceDepth( in float3 pos, float maxDepth, float blur )
{
	const float maxDepthFagootPimpInfinityMax_Final = 0.1;
	float shadowSurfaceDepth = 0.0f;

	//float3 worldPos = pos;
	float4 pos_sh = mul( mShadowTransform, float4(pos, 1) );

	//dex++: shadow pixel is on the far plane (or beyond) of the shadowmap
	// this is the case of pixels that are very far away and fall outside cascade depth bounds
	// do not calculate shadow factor for such pixels, assume they are visible
	[branch]
	if ( abs(pos_sh.z) >= 0.999f )
	{
		return maxDepthFagootPimpInfinityMax_Final;
	}
	
	// Sample cascades
	{
		float4 _dx 			= (pos_sh.xxxx - ShadowOffsetsX);
		float4 _dy 			= (pos_sh.yyyy - ShadowOffsetsY);
		float4 _max_d = max(abs(_dx), abs(_dy)); 

		// one for every cascade we are outside
		float4 _it 			= (_max_d < ShadowHalfSizes);

		// calculate the primary cascade we are inside
		// in cascade 0: 1 1 1 1  // 4
		// in cascade 1: 0 1 1 1  // 3
		// in cascade 2: 0 0 1 1  // 2
		// in cascade 3: 0 0 0 1  // 1
		// outside:      0 0 0 0  // 0
		const float maxCascades = ShadowPoissonOffsetAndBias.z;
		float4 mask = ( maxCascades.xxxx > float4(0.5f, 1.5f, 2.5f, 3.5f ) );
		float _i = dot( _it, mask );
				
		// outside cascade system
		[branch]
		if ( _i < 0.5f )
		{
			return maxDepthFagootPimpInfinityMax_Final;
		}		
					
		const int lastCascadeID = (int)( maxCascades ) - 1;

		// calculate cascade index
		int iCascadeIndex = (int)( maxCascades - _i );

		// calculate shadow factor for current cascade
		{
			// normalize coordinates to fit inside shadow cascade
			const float4 shadowParam	= ShadowParams[iCascadeIndex];
			const float2 _halfSegSize	= float2(shadowParam.z,-shadowParam.z);
			const float2 _center		= shadowParam.xy;
			const float3 sh_coord		= float3( pos_sh.xy * _halfSegSize + _center , pos_sh.z );
		
			// calculate shadowmap coord
			const float kernelSize = shadowParam.w * blur;
			const float shadowGradient = 0.0f;

			shadowSurfaceDepth = ShadowSurfaceDepth( ShadowMapGlobal, sh_coord, iCascadeIndex, kernelSize, maxDepth );
		}
		
		// Last cascade border blend
		// TODO add proper blend out for last cascade
		shadowSurfaceDepth += step(lastCascadeID,iCascadeIndex) * maxDepthFagootPimpInfinityMax_Final;
	}

	// Return shadow color
	return shadowSurfaceDepth;
}

float CalcShadowFromCascade( in float3 pos_sh, in int iCascadeIndex, in int qualityDegradation = 0, in int isSpeedTreeShadowReceiver = false )
{
	// normalize coordinates to fit inside shadow cascade
	const float4 shadowParam	= ShadowParams[iCascadeIndex];
	const float2 _halfSegSize	= float2(shadowParam.z,-shadowParam.z);
	const float2 _center		= shadowParam.xy;

	// Setup final sh_coord
	const float3 sh_coord		= float3( pos_sh.xy * _halfSegSize + _center, pos_sh.z );
	
	const float2 params = isSpeedTreeShadowReceiver ? SpeedTreeShadowParams[ iCascadeIndex ].xy : float2( ShadowParams[ iCascadeIndex ].w , 0.0f );

	return PCF_Shadow( ShadowMapGlobal, sh_coord, iCascadeIndex, params.x, params.y, qualityDegradation );
}

//dex++: refactored
float CalcShadowFactor( in float3 pos, in bool filter, in int qualityDegradation = 0, in int isSpeedTreeShadowReceiver = false, in uint2 pixelCoord = uint2(0,0) )
{
	float shadow_factor = 1.0f;	

	//float3 worldPos = pos;
	const float4 pos_sh = mul( mShadowTransform, float4(pos, 1) );

	//dex++: shadow pixel is on the far plane (or beyond) of the shadowmap
	// this is the case of pixels that are very far away and fall outside cascade depth bounds
	// do not calculate shadow factor for such pixels, assume they are visible
	[branch] 
	if ( abs(pos_sh.z) >= 0.999f )
	{
		return 1.0f;
	}
	
	// Sample cascades
	{
		const float4 _dx 			= (pos_sh.xxxx - ShadowOffsetsX);
		const float4 _dy 			= (pos_sh.yyyy - ShadowOffsetsY);
		const float4 _max_d			= max(abs(_dx), abs(_dy)); 

		// one for every cascade we are outside
		const float4 _it 			= (_max_d < ShadowHalfSizes);

		// calculate the primary cascade we are inside
		// in cascade 0: 1 1 1 1  // 4
		// in cascade 1: 0 1 1 1  // 3
		// in cascade 2: 0 0 1 1  // 2
		// in cascade 3: 0 0 0 1  // 1
		// outside:      0 0 0 0  // 0
		const float maxCascades = ShadowPoissonOffsetAndBias.z;
		const float4 mask = ( maxCascades.xxxx > float4(0.5f, 1.5f, 2.5f, 3.5f ) );
		const float _i = dot( _it, mask );

		// outside cascade system
		[branch]
		if ( _i < 0.5f )
		{
			//distance test
			/*const float distToCamera = length( pos - cameraPosition );
			if ( distToCamera < 70.0f )
				return 0.0f;*/

			return 1.0f;
		}

		// calculate cascade fade factors ( last 20% of each cascade )
		const float4 fadeFactors = saturate( ( ( _max_d / ShadowHalfSizes ) - ( 1.0f - ShadowFadeScales ) ) / ShadowFadeScales );

		// calculate cascade index
		int iCascadeIndex = (int)( maxCascades - _i );
		const int lastCascadeID = (int)( maxCascades ) - 1;

		// calculate shadow factor for current cascade
		{
#ifdef USE_GLOBAL_SHADOW_PASS
	#ifdef BLEND_CASCADES
			[branch]
			if( fadeFactors[iCascadeIndex] > 0.0f )
			{
				const float s0 = CalcShadowFromCascade( pos_sh , iCascadeIndex  , qualityDegradation, isSpeedTreeShadowReceiver );
				const float s1 = CalcShadowFromCascade( pos_sh , iCascadeIndex+1, qualityDegradation, isSpeedTreeShadowReceiver );
				shadow_factor = lerp( s0, s1, fadeFactors[iCascadeIndex] );
			}
			else
	#else
		#ifdef USE_SHADOW_BLEND_DISSOLVE
			iCascadeIndex += CalcDissolvePattern( pixelCoord , 4 ) < fadeFactors[iCascadeIndex] ? 1 : 0;
			iCascadeIndex = min( iCascadeIndex , lastCascadeID );
		#else
			[branch]
			if( fadeFactors[iCascadeIndex] > 0.0f )
			{
				const float s0 = CalcShadowFromCascade( pos_sh , iCascadeIndex  , qualityDegradation, isSpeedTreeShadowReceiver );
				const float s1 = CalcShadowFromCascade( pos_sh , iCascadeIndex+1, qualityDegradation, isSpeedTreeShadowReceiver );
				shadow_factor = lerp( s0, s1, fadeFactors[iCascadeIndex] );
			}
			else
		#endif // USE_SHADOW_BLEND_DISSOLVE
	#endif // BLEND_CASCADES
#endif // USE_GLOBAL_SHADOW_PASS
			{
				shadow_factor = CalcShadowFromCascade( pos_sh , iCascadeIndex, qualityDegradation, isSpeedTreeShadowReceiver );
			}
		}

		// Last cascade border blend
		shadow_factor = saturate( step(lastCascadeID,iCascadeIndex) * fadeFactors[lastCascadeID] + shadow_factor );

	}		

	// Return shadow color
	return shadow_factor;
}

float CalculateCloudsShadow( float3 worldPos )
{
	float farScaleMultiplier = 20.0f;

	float3 cloudsNearUV = float3( 0.0f, 0.0f, 0.0f );
	float3 cloudsFarUV = float3( 0.0f, 0.0f, 0.0f );

	float3 scaledWorldPosNear = 1.0f;
	float currentShadowNear = 1.0f;
	float targetShadowNear = 1.0f;
	float cloudShadowNear = 0.0f;

	float3 scaledWorldPosFar = 1.0f;
	float currentShadowFar = 1.0f;
	float targetShadowFar = 1.0f;
	float cloudShadowFar = 0.0f;

	float cloudShadowScaleNear = weatherAndPrescaleParams.x;
	float cloudShadowScaleFar = cloudShadowScaleNear * farScaleMultiplier;

	float2 cloudShadowOffset = windParams.xy;
	int currentShadowTextureIndex = trunc( skyboxShadingParams.x );
	int targetShadowTextureIndex = trunc( skyboxShadingParams.y );
	float distanceToCamera = length( worldPos.xyz - cameraPosition.xyz );


	// E3 DEMO Request
	// disabling usage of the skybox textures shadow	
	/*

	float3 globalLightPos = lightDir.xyz*(1000000.0f);
	float3 sunDirToCamera = normalize(worldPos - globalLightPos);
	float skydomeRadius = 3600.0f;	
	float3 v = globalLightPos;
	float vdir = dot( v, sunDirToCamera );
	
	float sq = sqrt( abs( dot( vdir, vdir ) - ( dot( v, v ) - skydomeRadius * skydomeRadius ) ) );	
	float tnear = -vdir + sq;
	float tfar = -vdir - sq;
	
	float minT = min(tnear, tfar);
	float3 p = normalize( globalLightPos + sunDirToCamera*minT );
	
	float3 pr = normalize( float3(p.x, p.y, 0.0f) );
	
	cloudsUV.y = acos( dot(p, pr) );
	cloudsUV.x = atan2( pr.y, pr.x );
	
	if( cloudsUV.x < 0.0f ) cloudsUV.x = 2.0f*3.141592 - abs( cloudsUV.x );
	
	cloudsUV.y = cloudsUV.y/(0.5f*3.141592);
	cloudsUV.x = cloudsUV.x/(3.141592) + weatherAndPrescaleParams.z;
	
	float curentCloudTextureIndex = skyboxShadingParams.z;
	float targetCloudTextureIndex = skyboxShadingParams.w;	

	float weatherBlend0 = SAMPLE_LEVEL( CloudsShadowTexture, CloudsShadowSampler, float3( cloudsUV.xy, curentCloudTextureIndex ), 0 ).w;
	float weatherBlend1 = SAMPLE_LEVEL( CloudsShadowTexture, CloudsShadowSampler, float3( cloudsUV.xy, targetCloudTextureIndex ), 0 ).w;
	
	cloudShadow = lerp( weatherBlend0, weatherBlend1, saturate(weatherAndPrescaleParams.w) );	
	
	return 1 - saturate( skyboxShadingParams.x*pow( cloudShadow, clamp( skyboxShadingParams.y, 0.1f, 100.0f ) ) );
	*/

	float weatherBlendFactor = weatherAndPrescaleParams.w;

	scaledWorldPosNear = ( worldPos.xyz + float3( cloudShadowOffset, 0 ) )* 0.01f / cloudShadowScaleNear;
	scaledWorldPosFar = ( worldPos.xyz + float3( cloudShadowOffset * farScaleMultiplier, 0 ) )* 0.01f / cloudShadowScaleFar;

	cloudsNearUV.xyz = scaledWorldPosNear.xyz - float3(  lightDir.xy, 0 ) * scaledWorldPosNear.z / lightDir.z ;
	cloudsFarUV.xyz = scaledWorldPosFar.xyz - float3(  lightDir.xy, 0 ) * scaledWorldPosFar.z / lightDir.z ;

	currentShadowNear = SAMPLE_LEVEL( CloudsShadowTexture, CloudsShadowSampler, float3( cloudsNearUV.xy, currentShadowTextureIndex ), 1 ).x;
	targetShadowNear = SAMPLE_LEVEL( CloudsShadowTexture, CloudsShadowSampler, float3( cloudsNearUV.xy, targetShadowTextureIndex ), 1 ).x;

	cloudShadowNear = lerp( currentShadowNear, targetShadowNear, weatherBlendFactor );

	currentShadowFar = SAMPLE_LEVEL( CloudsShadowTexture, CloudsShadowSampler, float3( cloudsFarUV.xy, currentShadowTextureIndex ), 1 ).x;
	targetShadowFar = SAMPLE_LEVEL( CloudsShadowTexture, CloudsShadowSampler, float3( cloudsFarUV.xy, targetShadowTextureIndex ), 1 ).x;

	cloudShadowFar = lerp( currentShadowFar, targetShadowFar, weatherBlendFactor );

	float angularFade = saturate( ( lightDir.z - 0.25f ) / ( 0.1f ) );
	float distanceFade = saturate( ( distanceToCamera - 100.0f ) / ( 250.0f - 100.0f ) );

	return 1.0f - angularFade * lerp( cloudShadowNear, cloudShadowFar,  distanceFade );
}

//dex++: calculate terrain height at given world position
#ifdef DEFERRED
float2 CalcTerrainHeight( in float3 worldSpacePosition )
{
	[branch]
	if ( iNumTerrainTextureWindows > 0 )
	{
		// select terrain window
		for ( int i=0; i<iNumTerrainTextureWindows; ++i )
		{
			float2 windowCoords = (worldSpacePosition.xy - vTerrainShadowsWindows[i].xy) * vTerrainShadowsWindows[i].zw;
			if ( windowCoords.x >= 0 && windowCoords.y >= 0 && windowCoords.x <= 1 && windowCoords.y <= 1 )
			{
				// invert ( WHY? somehere out there in the terrain code something is inverted... )
				//windowCoords.y = 1.0f - windowCoords.y;
				windowCoords = ( windowCoords * ( vTerrainTextureWindows[i].zw - vTerrainTextureWindows[i].xy ) + vTerrainTextureWindows[i].xy );

				// adjust pixel coords ( empirical )
				windowCoords.x += 0.5f / 1024.0f;
				windowCoords.y -= 0.5f / 1024.0f;

				// sample heightmap
				float height0 = vTerrainShadowsParams3.y + vTerrainShadowsParams3.x * SAMPLE_LEVEL( TerrainClipMap, samplLinear, float3( windowCoords.xy, i ), 0 ).x;

				// sample neightbour pixels (to calculate slope)
				float height01 = vTerrainShadowsParams3.y + vTerrainShadowsParams3.x * SAMPLE_LEVEL( TerrainClipMap, samplLinear, float3( windowCoords.xy + float2(1.0f/512.0f,0), i ), 0 ).x;
				float height10 = vTerrainShadowsParams3.y + vTerrainShadowsParams3.x * SAMPLE_LEVEL( TerrainClipMap, samplLinear, float3( windowCoords.xy + float2(0,1.0f/512.0f), i ), 0 ).x;
				float height02 = vTerrainShadowsParams3.y + vTerrainShadowsParams3.x * SAMPLE_LEVEL( TerrainClipMap, samplLinear, float3( windowCoords.xy + float2(-1.0f/512.0f,0), i ), 0 ).x;
				float height20 = vTerrainShadowsParams3.y + vTerrainShadowsParams3.x * SAMPLE_LEVEL( TerrainClipMap, samplLinear, float3( windowCoords.xy + float2(0,-1.0f/512.0f), i ), 0 ).x;

				// calculate derivative ( simplified )
				const float devX = max( abs( height01 - height0 ), abs( height02 - height0 ) );
				const float devY = max( abs( height10 - height0 ), abs( height20 - height0 ) );
				const float dev = max( devX, devY );

				// decompress
				return float2( height0, dev );
			}
		}
	}

	// no terrain height
	return float2( -10000.0f, 0.0f );
}
#endif
//dex--

//dex++: terrain shadows integration
float CalcTerrainShadows( in float3 worldSpacePosition )
{	 
	[branch]
	if ( iNumTerrainShadowsWindows > 0 )
	{
		// calcualte terrain height
		#ifdef DEFERRED
			const float2 terrainOrgHeight = CalcTerrainHeight( worldSpacePosition );
		#endif

		// calculate distance to camera
		const float terrainShadowDistance = vTerrainShadowsParams.y;
		const float distToCamera = length( worldSpacePosition.xy - cameraPosition.xy );
		const float distToCamera3D = length( worldSpacePosition.xyz - cameraPosition.xyz );
		// TODO: this may be execute 100% of the time so the "branch" may be omitted
		[branch]
		if ( distToCamera < terrainShadowDistance )
		{
			// select terrain window
			for ( int i=0; i<iNumTerrainShadowsWindows; ++i )
			{
				float2 windowCoords = (worldSpacePosition.xy - vTerrainShadowsWindows[i].xy) * vTerrainShadowsWindows[i].zw;
				if ( windowCoords.x >= 0 && windowCoords.y >= 0 && windowCoords.x <= 1 && windowCoords.y <= 1 )
				{
					// invert ( WHY? somehere out there in the terrain code something is inverted... )
					windowCoords.y = 1.0f - windowCoords.y;

					// calculate terrain shadow factor
					// adjust terrain shadow blending based on the terrain slope
					#ifdef DEFERRED
						const float terrainShadowBlendSlope = 1.0f / max( 0.1f, terrainOrgHeight.y );
					#else
						const float terrainShadowBlendSlope = 1.0f;
					#endif
					const float terrainShadowBlend = min( terrainShadowBlendSlope, vTerrainShadowsParams.x / max( 1, distToCamera * vTerrainShadowsParams2.z ) );
					const float terrainHeight = vTerrainShadowsParams3.y + vTerrainShadowsParams3.x * SAMPLE_LEVEL( TerrainShadowAtlas, samplLinear, float3( windowCoords.xy, i ), 0 );

					// if the point is close enough to the terrain snap it ( prevents depth issues )
					#ifdef DEFERRED					
						const float terrainShadowSnapDistance = 0.05f + distToCamera3D * 0.02f;
						if ( abs( terrainOrgHeight.x - worldSpacePosition.z ) < terrainShadowSnapDistance )
						{
							worldSpacePosition.z = terrainOrgHeight.x;
						}
					#endif

					// Blend between the moved shadow
					const float terrainShadowBias = 0.1f;

					// X - master (terrain) shadows
					const float terrainShadows = 1.0f - saturate( ((terrainHeight-terrainShadowBias) - worldSpacePosition.z) * terrainShadowBlend );

					// fade the terrain shadows away
					const float terrainShadowFadeOffset = vTerrainShadowsParams.z;
					const float terrainShadowFadeMul = vTerrainShadowsParams.w;
					float shadowFade = saturate( (distToCamera + terrainShadowFadeOffset) * terrainShadowFadeMul );
					return lerp( terrainShadows, 1.0f, shadowFade );
				}
			}
		}
	}

	return 1.0f;
}
//dex--

float3 CalcParametricAmbientColor( float3 N_world, float3 orientationVector, float3 colorFront, float3 colorBack, float3 colorTop, float3 colorBottom, float3 colorSide, float shadowFactor )
{
	return float3 ( 1, 0, 1 );
}

float3 CalcParametricAmbientColor( float3 N_world, float shadowFactor )
{
	float3 orientationVector = float3 ( lightDir.xy, 0 );
	return CalcParametricAmbientColor( N_world, orientationVector, 0, 0, 0, 0, 0, shadowFactor );
}

float CalcGlobalShadow( in float3 _UNUSED_N, in float3 worldSpacePosition, in float2 pixelCoord, int qualityDegradation = 0, in int isSpeedTreeShadowReceiver = false, in bool calcCascadesShadow = true )
{
	float shadow = 0;

	//const float3 L = lightDir.xyz;
	//[branch]
	//if ( dot( L, N ) > 0 )
	{
		shadow = CalcTerrainShadows(worldSpacePosition);

		[branch]
		if ( shadow > 0.0 )
		{
			if ( calcCascadesShadow )
			{
				shadow *= CalcShadowFactor( worldSpacePosition, false, qualityDegradation, isSpeedTreeShadowReceiver, uint2(pixelCoord) );
			}

			shadow *= CalculateCloudsShadow( worldSpacePosition );
		}
	}

	return shadow;
}

float3 CalcGlobalLightColor( float3 worldSpacePosition )
{
	float displaceHoriz = length( worldSpacePosition.xy - cameraPosition.xy );
	float displaceVert  = worldSpacePosition.z;

	float3 farCol = lerp( lightColorLightOppositeSide.xyz, lightColorLightSide.xyz, saturate( displaceVert * lightColorParams.z + lightColorParams.w ) );
	float3 col = lerp( lightColor.xyz, farCol, saturate( displaceHoriz * lightColorParams.x + lightColorParams.y ) );
	
	return col;
}

//dex++: added face slope for better shadow map biasing
LightingData CalcGlobalLight( in float3 V, in float3 N, in float translucency, in float glossiness, in float3 worldSpacePosition, in float2 pixelCoord, float slopeBias, float forcedShadow )
//dex--
{
	const float3 L = lightDir.xyz;	

	LightingData ld;
	ld.preNormDiffuse = 0;
	ld.diffuse = pixelCoord.x-pixelCoord.x;
	ld.specular = 0;

	//dex++: calculate shadows from cascades only if there is no terrain shadow
	[branch]
	if ( forcedShadow > 0.0 )
	{	
		float3 diffuseFactor = Lambert( L, N, V, translucency ).xyz;
		[branch]
		if ( dot(diffuseFactor.xyz,1) > 0 )
		{
			float shadow = forcedShadow;

			const float shadowDiffuse = shadow;
			const float shadowSpecular = shadow;
			const float3 globalLightColor = CalcGlobalLightColor( worldSpacePosition );

			ld.diffuse = shadowDiffuse * diffuseFactor * globalLightColor;
			ld.specular = shadowSpecular * BlinnPhong( L, N, V, glossiness ) * globalLightColor;
		}
	}
	//dex--
	
	return ld;
}

LightingData CalcGlobalLightPBRPipeline( in float3 V, in float3 N_diff, in float3 N_spec, in float translucency, in float3 specularity, in float roughness, in float3 worldSpacePosition, in float2 pixelCoord, float slopeBias, float forcedShadow, SCompileInLightingParams extraLightingParams, float encodedMaterialFlags = GBUFF_MATERIAL_MASK_ENCODED_DEFAULT )
{
	const float3 L = lightDir.xyz;	

	LightingData ld;
	ld.preNormDiffuse = 0;
	ld.diffuse = 0;
	ld.specular = 0;

	float zero = 0;

	//dex++: calculate shadows from cascades only if there is no terrain shadow
	[branch]
	if ( forcedShadow > zero )
	{	
		float2 bleedScaleBias = float2( 1, 0 );
#if defined(ENABLE_BILLBOARDS_LIGHT_BLEED)
		if ( 0 != (GBUFF_MATERIAL_FLAG_BILLBOARDS & DecodeGBuffMaterialFlagsMask( encodedMaterialFlags )) )
		{
			bleedScaleBias = lightTweaks.xy;
		}
#endif

#if !defined(COMPILE_IN_SHADING_SKIN) && !defined(COMPILE_IN_SHADING_HAIR) && !defined(COMPILE_IN_SHADING_EYE)
		[branch]
		if ( saturate( dot( L, N_diff ) ) + bleedScaleBias.y + translucency > 0 ) // branch not based on diffuseFactor anymore, since it can be balanced down to zero by specular, and this would branch out the specular
#endif
		{
			float3 preNormDiffuse = 0;
			float3 diffuseFactor = DiffuseLightingPBRPipeline( preNormDiffuse, L, N_diff, V, translucency, specularity, roughness, extraLightingParams, bleedScaleBias ).xyz;		
			float shadow = forcedShadow;

			ld.preNormDiffuse = preNormDiffuse;

			const float shadowDiffuse = shadow;
			const float shadowSpecular = shadow;
			const float3 globalLightColor = CalcGlobalLightColor( worldSpacePosition );

			#if defined(COMPILE_IN_SHADING_SKIN)			
			ld.diffuse = shadowDiffuse * (diffuseFactor + CalculateSkinTranslucencyGain( L, V, N_diff, translucency, extraLightingParams )) * globalLightColor;
			#else
			ld.diffuse = shadowDiffuse * diffuseFactor * globalLightColor;
			#endif

			#if !defined(COMPILE_IN_SHADING_HAIR_TRANSPARENT)
			ld.specular = shadowSpecular * SpecularLightingPBRPipeline( L, N_spec, V, specularity, roughness, translucency, extraLightingParams ) * globalLightColor;
			#endif
		}
	}
	//dex--
	
	return ld;
}

LightingData CalcGlobalLightPBRPipeline( in float3 V, in float3 N, in float translucency, in float3 specularity, in float roughness, in float3 worldSpacePosition, in float2 pixelCoord, float slopeBias, float forcedShadow, SCompileInLightingParams extraLightingParams, float encodedMaterialFlags = GBUFF_MATERIAL_MASK_ENCODED_DEFAULT )
{
	return CalcGlobalLightPBRPipeline( V, N, N, translucency, specularity, roughness, worldSpacePosition, pixelCoord, slopeBias, forcedShadow, extraLightingParams, encodedMaterialFlags );
}

//dex++: added face slope for better shadow map biasing
LightingData CalcGlobalLight( in float3 V, in float3 N, in float translucency, in float glossiness, in float3 worldSpacePosition, in float2 pixelCoord, float slopeBias=0.0f )
//dex--
{
	const float3 L = lightDir.xyz;	

	LightingData ld;
	ld.preNormDiffuse = 0;
	ld.diffuse = 0;
	ld.specular = 0;

	float globalShadowFactor = 0;

	//dex++: calculate shadows from cascades only if there is no terrain shadow
	const float terrainShadows = CalcTerrainShadows(worldSpacePosition);
	[branch]
	if ( terrainShadows > 0.0 )
	{	
		float3 diffuseFactor = Lambert( L, N, V, translucency ).xyz;
		[branch]
		if ( dot(diffuseFactor.xyz,1) > 0 )
		{
			float shadow = 0.0f;

	#if 1 
			// no filter
			shadow = CalcShadowFactor( worldSpacePosition, false );
	#else
			// filter
			shadow = CalcShadowFactor( worldSpacePosition, true );
	#endif

			//dex++: terrain shadow
			shadow = min( shadow, terrainShadows );
			//dex--

			shadow *= CalculateCloudsShadow( worldSpacePosition );

			globalShadowFactor = shadow;

			const float shadowDiffuse = shadow;
			const float shadowSpecular = shadow;
			const float3 globalLightColor = CalcGlobalLightColor( worldSpacePosition );

			ld.diffuse = shadowDiffuse * diffuseFactor * globalLightColor;
			ld.specular = shadowSpecular * BlinnPhong( L, N, V, glossiness ) * globalLightColor;
		}
	}
	//dex--

	return ld;
}

#endif

void ApplyCharactersLightingBoost( float4 boostParams, float3 shadowFactorFinal, inout float3 envProbeAmbient, inout float3 envProbeReflection )
{
	const float3 lighting_scale_ambient = lerp( boostParams.yyy, boostParams.xxx, shadowFactorFinal );
	const float3 lighting_scale_reflection = lerp( boostParams.www, boostParams.zzz, shadowFactorFinal );
	envProbeAmbient *= lighting_scale_ambient;
	envProbeReflection *= lighting_scale_reflection;
}

void ApplyCharactersLightingBoostConstBased( float3 shadowFactorFinal, inout float3 envProbeAmbient, inout float3 envProbeReflection )
{
#ifdef PIXELSHADER
	ApplyCharactersLightingBoost( float4( abs(PSC_CharactersLightingBoost.x), PSC_CharactersLightingBoost.yzw ), shadowFactorFinal, envProbeAmbient, envProbeReflection );
#endif
}

float3 GetLocalLightCharactersModification( float3 lightColor )
{
	return lightColor * localLightsExtraParams.x;
}

void ApplyCharactersLightingBoostStencilBased( uint stencilValue, float3 shadowFactorFinal, inout float3 envProbeAmbient, inout float3 envProbeReflection )
{
	if ( 0 != (stencilValue & LC_Characters) )
	{
		ApplyCharactersLightingBoost( characterLightParams, shadowFactorFinal, envProbeAmbient, envProbeReflection );
	}
}

float CalculateVolumeCutCustomVolumeTexture( float2 volumeTextureValue, float3 worldPosition, bool allowSmoothBorder = true )
{
	const float fadeStartDist = interiorRangeParams.x;
	const float fadeRangeInv = interiorRangeParams.y;
	const float encodeRange = interiorRangeParams.z;
	
	const float3 worldVec = worldPosition.xyz - cameraPosition.xyz;
	float distanceToCamera = length( worldVec );	
	
	float smoothBorderDist = 0.001;
	if ( allowSmoothBorder )
	{
		const float smoothBorderReductionRangeInv = interiorParams.w;
	
		float3 camDir = worldToView[2].xyz;
		float distAlongForward = dot( camDir, worldVec );
		float nearPlane = 1.0 / cameraNearFar.y;
		float minDist = distanceToCamera * nearPlane / distAlongForward;

		smoothBorderDist = interiorParams.z;
		smoothBorderDist *= saturate( (distanceToCamera - minDist) * smoothBorderReductionRangeInv );
		smoothBorderDist = max( 0.001, smoothBorderDist );
	}

	distanceToCamera /= encodeRange;

	float2 volumeTex = volumeTextureValue;		

	float volumeDistanceToExterior = volumeTex.x;
	float volumeDistanceToInterior = volumeTex.y;

	float interiorFactor = 0.0f;
	float fade = 1.0f;
	
	// exterior
	if( volumeDistanceToExterior < volumeDistanceToInterior )
	{
		if( distanceToCamera < volumeDistanceToExterior )
		{
			interiorFactor = 0;
		}
		else if ( distanceToCamera > volumeDistanceToInterior )
		{
			interiorFactor = 0;
		}
		else
		{
			interiorFactor = 1;
			// fade when watching another volume from the outside
			fade = saturate( 1.0f - (encodeRange*volumeDistanceToExterior - fadeStartDist)*fadeRangeInv );

			const float smooth_factor = saturate( (distanceToCamera - volumeDistanceToExterior) * encodeRange / smoothBorderDist );
			fade *= smooth_factor;
		}	
	}
	// interior
	else if( volumeDistanceToExterior > volumeDistanceToInterior )
	{
		if( distanceToCamera < volumeDistanceToInterior )
		{
			interiorFactor = 1;
		}
		else if ( distanceToCamera > volumeDistanceToExterior )
		{
			interiorFactor = 1;
			
			// fade when watching another volume from the inside
			fade = saturate( 1.0f - (encodeRange*volumeDistanceToExterior - fadeStartDist)*fadeRangeInv );			
		}
		else
		{
			// interiorFactor = 0;
			interiorFactor = 1;
			const float smooth_factor = saturate( (distanceToCamera - volumeDistanceToInterior) * encodeRange / smoothBorderDist );
			fade *= 1 - smooth_factor;
		}
	}	
	return saturate(1.0f - fade*interiorFactor);	
}

float CalculateVolumeCutByPixelCoord( int2 pixelCoord, float3 worldPosition, bool allowSmoothBorder = true )
{
	// TODO: embedded bilateral upscale would be nice here
	float2 volumeTex = volumeDepthTexture[ pixelCoord / WEATHER_VOLUMES_SIZE_DIV ].xy;		
	return CalculateVolumeCutCustomVolumeTexture( volumeTex, worldPosition, allowSmoothBorder );
}

#ifdef FORWARD

#ifdef __PSSL__
ByteBuffer TileLights : register(t20);
#else
ByteAddressBuffer TileLights : register(t20);
#endif

#if defined(PIXEL_SHADER) || defined(PIXELSHADER)
float CalculateVolumeCut( float2 screenUV, float3 worldPosition, bool allowSmoothBorder = true )
{
	// since this function is used in mbVolumeBlend material block
	// for onscreen effects lets
	// assume interior if we are underwater here
	if( windParams.w < 0.0f ) return 0.0f;

	float2 volumeTex = volumeDepthTexture.Sample( samplPoint, screenUV ).xy;		
	return CalculateVolumeCutCustomVolumeTexture( volumeTex, worldPosition, allowSmoothBorder );
}
#endif

#define ENABLE_DIMMERS_TRANSPARENCY
#include "include_dimmers.fx"

float CalcFullShadowFactor( in float3 worldSpacePosition, in float2 pixelCoord )
{
	//dex++: calculate shadows from cascades only if there is no terrain shadow
	
	float shadow = 0.0f;
	
	const float terrainShadows = CalcTerrainShadows(worldSpacePosition);
	[branch]
	if ( terrainShadows > 0.0 )
	{	
		shadow = CalcShadowFactor( worldSpacePosition, false ); // no filter

		//dex++: terrain shadow
		shadow = min( shadow, terrainShadows );
		//dex--

		shadow *= CalculateCloudsShadow( worldSpacePosition );
	}
	
	return shadow;
	//dex--
}

float3 CalculateLighting( float3 FaceN, float3 N, float3 albedo, float specularity, float translucency, float glossiness, float3 worldSpacePosition, float depth, uint2 pixelCoord, bool hasSpecular, float AO=1.0f )
{
	float dummy = FaceN.x-FaceN.x + albedo.x-albedo.x+specularity-specularity;
	if ( hasSpecular )
	{
		dummy += (translucency-translucency + glossiness-glossiness + worldSpacePosition.x-worldSpacePosition.x + depth-depth +AO-AO +pixelCoord.x-pixelCoord.x);
	}
	return saturate( N.z + dummy );
}

float3 CalculateDeferredLighting( float3 worldSpacePosition, float3 albedo, float3 N, float3 vertexNormal, float3 specularity, float roughness, float translucency, uint2 pixelCoord, SCompileInLightingParams extraLightingParams )
{
	N = normalize( N ) ;
	vertexNormal = normalize( vertexNormal );

	const uint2  tileIdx		= pixelCoord.xy / TILE_SIZE.xx;
    const uint   bufferIdx		= ((tileIdx.y * (int)numTiles.x) + tileIdx.x) * MAX_LIGHTS_PER_TILE;
	const float3 viewVec		= cameraPosition.xyz - worldSpacePosition;
	const float3 V				= normalize( viewVec );
	
	float3 diffuse = 0.0f;
	float3 specular = 0.0f;

	// for lights
	{
		[loop]
		for(int tileLightIdx = 0; tileLightIdx < MAX_LIGHTS_PER_TILE; ++tileLightIdx)
		{
			uint lIdx = TileLights.Load((bufferIdx + tileLightIdx)*4);

			[branch]
			if( lIdx >= MAX_LIGHTS_PER_TILE )
			break;
			[branch]
				if (lights[lIdx].colorAndType.w == 1.0f)
				{
				float3 L = lights[lIdx].positionAndRadius.xyz - worldSpacePosition;
				float attenuation = CalcLightAttenuation( lIdx, worldSpacePosition, L );
		
				[branch]
				if ( attenuation > 0 )
				{
					L = normalize( L );
					diffuse += attenuation * (DiffuseLightingPBRPipeline( L, N, V, translucency, specularity, roughness, extraLightingParams ).xyz * lights[lIdx].colorAndType.xyz);
					specular += attenuation * (SpecularLightingPBRPipeline( L, N, V, specularity, roughness, translucency, extraLightingParams ).xyz * lights[lIdx].colorAndType.xyz);
				}
			}
		}
	}

	float3 resultColor = albedo * diffuse + specular;

	return resultColor;
}



float3 CalculateMaskedLighting( float3 worldSpacePosition, float3 N, uint2 pixelCoord, uint flagIndex, float3 desaturationWeights)
{
	N = normalize( N ) ;

	const uint2  tileIdx		= pixelCoord.xy / TILE_SIZE.xx;
    const uint   bufferIdx		= ((tileIdx.y * (int)numTiles.x) + tileIdx.x) * MAX_LIGHTS_PER_TILE;
	const float3 viewVec		= cameraPosition.xyz - worldSpacePosition;
	const float3 V				= normalize( viewVec );
	
	float3 mask = 0.0f;

	// for lights
	{
		[loop]
		for(int tileLightIdx = 0; tileLightIdx < MAX_LIGHTS_PER_TILE; ++tileLightIdx)
		{
			uint lIdx = TileLights.Load( ( bufferIdx + tileLightIdx ) * 4 );

			[branch]
			if( lIdx >= MAX_LIGHTS_PER_TILE )
			{
				break;
			}

			[branch]
			if ( GetLightUsageFlag( lights[lIdx], flagIndex ) )
			{
				float3 L = lights[lIdx].positionAndRadius.xyz - worldSpacePosition;
				float attenuation = CalcLightAttenuation( lIdx, worldSpacePosition, L );
		
				[branch]
				if ( attenuation > 0 )
				{
					L = normalize( L );
					mask += attenuation * saturate( dot( N, L ) ) *  dot( lights[lIdx].colorAndType.xyz, desaturationWeights) ;
				}
			}
		}
	}

	return mask;
}

float CalcAntiLightBleedAttenuationHelper( float controlValue, float dotValue )
{
	float extent = 0.2;
	float dotMin = controlValue - extent;
	float frontDotValue  = dotValue * 0.5 + 0.5;
	return saturate( (frontDotValue - dotMin) / extent );	
	//return saturate( attValue / max(0.001, saturate( dot( N, L ) )) );
}

float CalcAntiLightBleedAttenuation( bool useRounded, float controlValue, float offsetUpFront, float offsetUpRound, float3 lightPos, float3 worldSpacePos )
{
	float attValue = 1;

#if defined(PIXEL_SHADER) || defined(PIXELSHADER)
	[branch]
	if ( 0 != PSC_Custom_HeadCenterPosition.w )
	{
		const float3 headCenterPos = PSC_Custom_HeadCenterPosition.xyz + PSC_Custom_HeadUpDirection.xyz * (useRounded ? offsetUpRound : offsetUpFront);
		const float3 refDir = useRounded ? normalize( worldSpacePos - headCenterPos ) : PSC_Custom_HeadFrontDirection.xyz;
		attValue = CalcAntiLightBleedAttenuationHelper( controlValue, dot( refDir, normalize( lightPos - headCenterPos ) ) );
	}
#endif

	return attValue;
}

float3 CalcAntiLightbleedDebugVisualisationColor( float3 worldSpacePosition, float upOffset )
{
	float3 color_mod = float3( 1,0.1,0.1 );
#if (defined(PIXEL_SHADER) || defined(PIXELSHADER))
	if ( 0 != PSC_Custom_HeadCenterPosition.w )
	{
		color_mod = float3( 0.1, 1, 0.1 );
		float t = saturate( (dot( normalize(worldSpacePosition - (PSC_Custom_HeadCenterPosition.xyz + PSC_Custom_HeadUpDirection.xyz * upOffset)), PSC_Custom_HeadFrontDirection.xyz ) - 0.65) / 0.1 );
		color_mod = lerp( color_mod, float3( 0.1, 1, 1 ), t );
	}
#endif
	return 2 * color_mod;
}

#ifdef CALC_LIGHTING_PBR_PIPELINE_EXPLICIT_ENVPROBE_LOD
float PrecomputeCalculateLightingEnvProbeMipIndex( float3 worldSpacePosition, float3 reflectionN, int2 pixelCoord )
{
	const float3 negV = normalize( worldSpacePosition.xyz - cameraPosition.xyz );
	const float3 R = reflect( negV, reflectionN );		
	float reflectionMipLod = CalcEnvProbeMipLevel( R, pixelCoord );
	return reflectionMipLod;
}
#endif

/// All params in linear space
#if defined(VERTEX_SHADER) || defined(VERTEXSHADER)
# define CALC_LIGHTING_PBR_PIPELINE_ALLOW_OPAQUE_BUFFERS		0
#else
# define CALC_LIGHTING_PBR_PIPELINE_ALLOW_OPAQUE_BUFFERS		1
#endif
float3 CalculateLightingPBRPipeline( float3 worldSpacePosition, float3 albedo, float3 N, float3 vertexNormal, float3 specularity, float roughness, float translucency, uint2 pixelCoord, SCompileInLightingParams extraLightingParams
#if CALC_LIGHTING_PBR_PIPELINE_ALLOW_OPAQUE_BUFFERS
	, bool useSSAO = true, bool useShadowBuffer = false 
#endif
#ifdef CALC_LIGHTING_PBR_PIPELINE_EXPLICIT_ENVPROBE_LOD
	, float explicitEnvProbeMipIndex = 999
#endif
	, bool applyGlobalFog = true
#ifdef CALC_LIGHTING_PBR_PIPELINE_EXPLICIT_SHADOW_FACTOR
	, float4 customShadowing = 0
#endif
#ifdef CALC_LIGHTING_PBR_PIPELINE_EXPLICIT_ENVPROBES
	, float3 explicitEnvProbeAmbient = 0
	, float3 explicitEnvProbeReflection = 0
#endif
	)
{
	N = normalize( N ) ;
	vertexNormal = normalize( vertexNormal );

	/*
	// debug stuff for comparing the difference between deferred stuff and forward stuff
	{
		if ( (pixelCoord.x / 256) % 2 != (pixelCoord.y / 256) % 2 )
		{
			discard;
		}

		if ( 0 == pixelCoord.x % 256 && 0 == pixelCoord.y % 16 || 0 == pixelCoord.y % 256 && 0 == pixelCoord.x % 16 )
		{
			return 1;
		}

		if ( 0 == pixelCoord.x % 256 && pixelCoord.y % 16 < 3 || 0 == pixelCoord.y % 256 && pixelCoord.x % 16 < 3 )
		{
			return 0;
		}
	}
	//*/

	const uint2  tileIdx		= pixelCoord.xy / TILE_SIZE.xx;
    const uint   bufferIdx		= ((tileIdx.y * (int)numTiles.x) + tileIdx.x) * MAX_LIGHTS_PER_TILE;
	const float3 viewVec		= cameraPosition.xyz - worldSpacePosition;
	const float3 V				= normalize( viewVec );
	
	float3 ambient = 0;
	float3 diffuse = 0;
	float3 specular = 0;
	float3 reflection = 0;

	// shadow factor
	float shadowFactor = 1;
	float dimmerFactor = 1;
	float interiorFactor = 1;
	
#if CALC_LIGHTING_PBR_PIPELINE_ALLOW_OPAQUE_BUFFERS
	[branch]
	if ( useShadowBuffer )
	{
		float4 maskValue = SYS_SAMPLE_TEXEL( PSSMP_GlobalShadowAndSSAO, pixelCoord );
		shadowFactor = DecodeGlobalShadowBufferShadow( maskValue );
		dimmerFactor = DecodeGlobalShadowBufferDimmers( maskValue );
		interiorFactor = DecodeGlobalShadowBufferInteriorFactor( maskValue );
	}
	else
#endif
	{
	#if defined(COMPILE_IN_SHADING_HAIR)
		int qualityDegradation = 1;
	#else
		int qualityDegradation = 0;
	#endif

	#ifdef CALC_LIGHTING_PBR_PIPELINE_EXPLICIT_SHADOW_FACTOR
		if ( customShadowing.w > 0.001 )
		{
			float3 normalizedCustomShadowing = customShadowing.xyz / customShadowing.w;
			shadowFactor = saturate( normalizedCustomShadowing.x );
			dimmerFactor = saturate( normalizedCustomShadowing.y );

			// We're using scale for interior factor because of local lights can be treated as interior and exterior only.
			// This is done based on the 'interiorFactor' value being compared to 1. By introducing scale we're handling numeric precision problems.
			// There are two values being interpolated in the custom shadowing vector - PREMUL_VALUE (which is premultiplied by W), and the actual W.
			// Final value is obtained by dividing PREMUL_VALUE by W, which doesn't produce numerically perfect result (i.e. (0.5 * 0.1) / (0.1) != 0.5), so 
			// we may getting interiorFactor=0.9999 for some pixels, which results in black pixels in case local light is marked as interior/exterior only.
			// To be more precise this bug happens (only on the borders of the screen) when the hair is in outside area, lit by an exterior only light.
			interiorFactor = saturate( GetInteriorFactorFullHalfLimitInv() * normalizedCustomShadowing.z );
		}
		else
		{
		#ifdef CALC_LIGHTING_PBR_PIPELINE_EXPLICIT_SF_ALLOW_DISCARD
			discard;
		#else
			shadowFactor = 0;
			dimmerFactor = 0;
			interiorFactor = 0.5;
		#endif
		}
	#else
		const float2 dimmerAndInteriorFactor = CalcDimmersFactorAndInteriorFactorTransparency( worldSpacePosition, pixelCoord );
		shadowFactor = CalcGlobalShadow( N, worldSpacePosition, pixelCoord, qualityDegradation );
		dimmerFactor = dimmerAndInteriorFactor.x;
		interiorFactor = dimmerAndInteriorFactor.y;
	#endif
	}

#if !defined(COMPILE_IN_SHADING_SKIN)
	// global light
	float3 shadowFactorFinal = shadowFactor;
	{
		const float slopeBias = 0; //saturate( 1 - dot( FaceN, lightDir.xyz ) );

		LightingData ld = CalcGlobalLightPBRPipeline( V, N, translucency, specularity, roughness, worldSpacePosition, pixelCoord / screenDimensions.xy, slopeBias, shadowFactor, extraLightingParams );
		shadowFactorFinal = min( ld.preNormDiffuse, shadowFactorFinal );
		
		diffuse += ld.diffuse;
		specular += ld.specular;
	}
#endif

	/*
	#if (defined(PIXEL_SHADER) || defined(PIXELSHADER)) && defined(COMPILE_IN_SHADING_SKIN)
		{
			const float4 antibleedControlData = extraLightingParams.data4;
			const bool useRounded = antibleedControlData.x > 0;
			const float upOffset = useRounded ? antibleedControlData.w : antibleedControlData.z;
			return albedo * CalcAntiLightbleedDebugVisualisationColor( worldSpacePosition.xyz, upOffset );
		}
	#endif
	//*/

	// for lights
	{
		[loop]
		for(int tileLightIdx = 0; tileLightIdx < MAX_LIGHTS_PER_TILE; ++tileLightIdx)
		{
			uint lIdx = TileLights.Load((bufferIdx + tileLightIdx)*4);

			[branch]
			if( lIdx >= MAX_LIGHTS_PER_TILE )
				break;
		
			float3 L = lights[lIdx].positionAndRadius.xyz - worldSpacePosition;
			float attenuation = CalcLightAttenuation( lIdx, worldSpacePosition, L );
	
		#if defined(COMPILE_IN_SHADING_SKIN)
			float translucency_skin_gain = translucency;
		#endif

		#if defined(COMPILE_IN_SHADING_SKIN) && (defined(PIXEL_SHADER) || defined(PIXELSHADER))
			{
				const float4 antibleedControlData = extraLightingParams.data4;
				const bool useRounded = antibleedControlData.x > 0;
				const float frontalControlValue = 200.f / 255.f;
				const float antiBleedAtt = CalcAntiLightBleedAttenuation( useRounded, (useRounded ? antibleedControlData.y : frontalControlValue), antibleedControlData.z, antibleedControlData.w, lights[lIdx].positionAndRadius.xyz, worldSpacePosition );
				
				attenuation *= antiBleedAtt;
				translucency_skin_gain *= antiBleedAtt;
			}
		#endif

			attenuation *= GetLocalLightsAttenuationInteriorScale( lights[lIdx].colorAndType.w, interiorFactor );
		
			[branch]
			if ( attenuation > 0 )
			{
			#ifdef PIXELSHADER
				const float lightsCharacterModifier = PSC_CharactersLightingBoost.x < 0 ? 1 : localLightsExtraParams.x;
				attenuation *= IsLightFlagsCharacterModifierEnabled( lights[lIdx].colorAndType.w ) ? lightsCharacterModifier : 1;
			#endif

				L = normalize( L );
				#if defined(COMPILE_IN_SHADING_SKIN)
				diffuse += attenuation * ((DiffuseLightingPBRPipeline( L, N, V, translucency, specularity, roughness, extraLightingParams ).xyz + CalculateSkinTranslucencyGain( L, V, N, translucency_skin_gain, extraLightingParams )) * lights[lIdx].colorAndType.xyz);
				#else
				diffuse += attenuation * (DiffuseLightingPBRPipeline( L, N, V, translucency, specularity, roughness, extraLightingParams ).xyz * lights[lIdx].colorAndType.xyz);
				#endif

				#if !defined(COMPILE_IN_SHADING_HAIR_TRANSPARENT)
				specular += attenuation * (SpecularLightingPBRPipeline( L, N, V, specularity, roughness, translucency, extraLightingParams ).xyz * lights[lIdx].colorAndType.xyz);
				#endif
			}
		}
	}

#if defined(COMPILE_IN_SHADING_SKIN)
	// global light
	float3 shadowFactorFinal = shadowFactor;
	{
		const float slopeBias = 0; //saturate( 1 - dot( FaceN, lightDir.xyz ) );
		
		LightingData ld = CalcGlobalLightPBRPipeline( V, N, translucency, specularity, roughness, worldSpacePosition, pixelCoord / screenDimensions.xy, slopeBias, shadowFactor, extraLightingParams );
		shadowFactorFinal = min( ld.preNormDiffuse, shadowFactorFinal );
		
		diffuse += ld.diffuse;
		specular += ld.specular;
	}
#endif

	// apply EnvProbes
	{
		float3 envProbeAmbient  = float3 ( 0, 0, 0 );
		float3 envProbeSpecular = float3 ( 0, 0, 0 );
		{
		#if defined(COMPILE_IN_SHADING_SKIN)
			const float ambientMipIndex		= 1;
			const float envProbeRoughness	= saturate( extraLightingParams.data1.w );
			const float3 envProbeNormal	= normalize( lerp( extraLightingParams.vertexNormal, N, extraLightingParams.data3.y ) );
		#else
			const float ambientMipIndex		= 0;
			const float envProbeRoughness	= roughness;
			const float3 envProbeNormal	= N;
		#endif

		#if defined(CALC_LIGHTING_PBR_PIPELINE_EXPLICIT_ENVPROBES)
			envProbeAmbient = explicitEnvProbeAmbient;
			envProbeSpecular = explicitEnvProbeReflection;
		#elif defined(CALC_LIGHTING_PBR_PIPELINE_EXPLICIT_ENVPROBE_LOD)
			{
				const float3 R = reflect( -V, envProbeNormal );			
				CalcEnvProbes_MipLod( envProbeAmbient, envProbeSpecular, worldSpacePosition, envProbeNormal, R, ambientMipIndex, explicitEnvProbeMipIndex, envProbeRoughness, true, true, true, interiorFactor );
			}
		#else
			CalcEnvProbes_NormalBasedMipLod( envProbeAmbient, envProbeSpecular, worldSpacePosition, V, envProbeNormal, envProbeNormal, ambientMipIndex, envProbeRoughness, pixelCoord, true, interiorFactor );
		#endif

		#if defined(COMPILE_IN_DYNAMIC_DECALS)
			ApplyCharactersLightingBoostStencilBased( extraLightingParams.stencilValue, shadowFactorFinal, envProbeAmbient, envProbeSpecular );
		#else
			ApplyCharactersLightingBoostConstBased( shadowFactorFinal, envProbeAmbient, envProbeSpecular );
		#endif
		}

		const float envprobe_distance_scale		= CalcEnvProbesDistantBrightness( worldSpacePosition );

		const float  envprobe_diffuse_scale		= pbrSimpleParams0.z;
		const float3 envprobe_ambient_scale		= envprobe_distance_scale * lerp( pbrSimpleParams0.yyy, pbrSimpleParams0.xxx, shadowFactorFinal );
		const float  envprobe_specular_scale	= pbrSimpleParams0.w;
		const float3 envprobe_reflection_scale	= envprobe_distance_scale * lerp( pbrSimpleParams1.yyy, pbrSimpleParams1.xxx, shadowFactorFinal );			

		diffuse		*= envprobe_diffuse_scale;	
		specular	*= envprobe_specular_scale;

		#if defined(COMPILE_IN_SHADING_SKIN)
		
			float extraEnvProbeFresnelScale = 1; //< 1. this shit is not needed because we don't need to hack any discontinuities since we have to original vertexNormal
			extraEnvProbeFresnelScale *= extraLightingParams.data0.w; //< apply gain mask
						
			const float3 envProbeSpecularity = saturate( extraLightingParams.data1.xyz );
			const float envProbeRoughness = saturate( extraLightingParams.data1.w );
			const float3 envProbeNormal	= normalize( lerp( extraLightingParams.vertexNormal, N, extraLightingParams.data3.y ) );

			envProbeAmbient = lerp(envProbeAmbient, dot(envProbeAmbient, float3( 0.3f, 0.5f, 0.2f )).xxx, 0.5f);

			ambient += NormalizeAmbientPBRPipeline( envprobe_ambient_scale * envProbeAmbient, translucency, V, envProbeNormal, envProbeSpecularity, envProbeRoughness, vertexNormal, extraEnvProbeFresnelScale );
			reflection += envprobe_reflection_scale * envProbeSpecular * CalcFresnelPBRPipelineEnvProbeReflection( V, envProbeNormal, envProbeSpecularity, envProbeRoughness, translucency, vertexNormal, extraEnvProbeFresnelScale );
		
		#else

			float  extraEnvProbeFresnelScale = 1; //< 1. this shit is not needed because we don't need to hack any discontinuities since we have to original vertexNormal
			float3 flatNormal = N;

			#if defined( COMPILE_IN_SHADING_HAIR )
				extraEnvProbeFresnelScale *= extraLightingParams.data0.w; //< apply gain mask
				flatNormal = vertexNormal;
			#endif

			ambient += NormalizeAmbientPBRPipeline( envprobe_ambient_scale * envProbeAmbient, translucency, V, N, specularity, roughness, flatNormal, extraEnvProbeFresnelScale );
			reflection += envprobe_reflection_scale * envProbeSpecular * CalcFresnelPBRPipelineEnvProbeReflection( V, N, specularity, roughness, translucency, flatNormal, extraEnvProbeFresnelScale );

		#endif
	}

	// apply AO
	{
		float3 nvidiaHBAO = 1;
#if CALC_LIGHTING_PBR_PIPELINE_ALLOW_OPAQUE_BUFFERS
		[branch]
		if ( useSSAO )
		{ 
			nvidiaHBAO = ProcessSampledSSAO( DecodeGlobalShadowBufferSSAO( SYS_SAMPLE_TEXEL( PSSMP_GlobalShadowAndSSAO, pixelCoord ) ) );
		}
#endif

		#if defined(COMPILE_IN_SHADING_SKIN)
					
			nvidiaHBAO = lerp( (1.0f).xxx, nvidiaHBAO, saturate(extraLightingParams.data3.x) );
			nvidiaHBAO = min( nvidiaHBAO, extraLightingParams.ao );			

		#elif defined(COMPILE_IN_SHADING_HAIR)
			
			nvidiaHBAO = lerp( (1.0f).xxx, nvidiaHBAO, saturate(extraLightingParams.data0.y) );

		#endif

		const float3 ssaoMod			= ModulateSSAOByTranslucency( nvidiaHBAO, translucency );
		const float3 AO_probe			= GetProbesSSAOScaleByDimmerFactor( dimmerFactor ) * ssaoMod;
		const float3 AO_nonProbe		= ssaoParams.x * ssaoMod + ssaoParams.y;

		ambient    *= AO_probe;
		reflection *= AO_probe;
		diffuse    *= AO_nonProbe;
		specular   *= AO_nonProbe;
	}
	
	// merge result
	float3 resultColor = albedo * (ambient + diffuse) + specular + reflection;

#if defined(COMPILE_IN_SHADING_SKIN)
	//resultColor = albedo * diffuse;
#endif
	
	// apply fog
	if ( applyGlobalFog )
	{
		resultColor = ApplyFog( resultColor, false, false, worldSpacePosition ).xyz;
	}

	//
	return resultColor;
}

float3 CalculateLightingComponents( float3 worldSpacePosition, float3 albedo, float3 N, float3 vertexNormal, float3 specularity, float roughness, float translucency, uint2 pixelCoord, bool excludeFlags, uint mask, bool globalDiffuse, bool globalSpecular, bool deferredDiffuse, bool deferredSpecular, bool envProbes, bool ao, bool fog, SCompileInLightingParams extraLightingParams
#if CALC_LIGHTING_PBR_PIPELINE_ALLOW_OPAQUE_BUFFERS
	, bool useSSAO = true, bool useShadowBuffer = false 
#endif
	)
{
	N = normalize( N ) ;
	vertexNormal = normalize( vertexNormal );

	const uint2  tileIdx		= pixelCoord.xy / TILE_SIZE.xx;
    const uint   bufferIdx		= ((tileIdx.y * (int)numTiles.x) + tileIdx.x) * MAX_LIGHTS_PER_TILE;
	const float3 viewVec		= cameraPosition.xyz - worldSpacePosition;
	const float3 V				= normalize( viewVec );
	
	float3 ambient = 0;
	float3 diffuse = 0;
	float3 specular = 0;
	float3 reflection = 0;

	// shadow factor
	float shadowFactor = 1;
	float dimmerFactor = 1;
	float interiorFactor = 1;
	
#if CALC_LIGHTING_PBR_PIPELINE_ALLOW_OPAQUE_BUFFERS
	[branch]
	if ( useShadowBuffer )
	{
		float4 maskValue = SYS_SAMPLE_TEXEL( PSSMP_GlobalShadowAndSSAO, pixelCoord );
		shadowFactor = DecodeGlobalShadowBufferShadow( maskValue );
		dimmerFactor = DecodeGlobalShadowBufferDimmers( maskValue );
		interiorFactor = DecodeGlobalShadowBufferInteriorFactor( maskValue );
	}
	else
#endif
	{
	#if defined(COMPILE_IN_SHADING_HAIR)
		int qualityDegradation = 1;
	#else
		int qualityDegradation = 0;
	#endif

		const float2 dimmerAndInteriorFactor = CalcDimmersFactorAndInteriorFactorTransparency( worldSpacePosition, pixelCoord );

		shadowFactor = CalcGlobalShadow( N, worldSpacePosition, pixelCoord, qualityDegradation );
		dimmerFactor = dimmerAndInteriorFactor.x;
		interiorFactor = dimmerAndInteriorFactor.y;
	}
	
	// global light
	float3 shadowFactorFinal = shadowFactor;
	
	if ( globalDiffuse || globalSpecular )
	{
		const float slopeBias = 0; //saturate( 1 - dot( FaceN, lightDir.xyz ) );

		LightingData ld = CalcGlobalLightPBRPipeline( V, N, translucency, specularity, roughness, worldSpacePosition, pixelCoord / screenDimensions.xy, slopeBias, shadowFactor, extraLightingParams );
		shadowFactorFinal = min( ld.preNormDiffuse, shadowFactorFinal );

		if( globalDiffuse )
		{
			diffuse += ld.diffuse;
		}
		if( globalSpecular )
		{
			specular += ld.specular;
		}
	}
	
	// for lights
	if ( deferredDiffuse || deferredSpecular )
	{
		[loop]
		for(int tileLightIdx = 0; tileLightIdx < MAX_LIGHTS_PER_TILE; ++tileLightIdx)
		{
			uint lIdx = TileLights.Load((bufferIdx + tileLightIdx)*4);

			[branch]
			if( lIdx >= MAX_LIGHTS_PER_TILE )
				break;
		
			float3 L = lights[lIdx].positionAndRadius.xyz - worldSpacePosition;
			float attenuation = CalcLightAttenuation( lIdx, worldSpacePosition, L );
		
			attenuation *= GetLocalLightsAttenuationInteriorScale( lights[lIdx].colorAndType.w, interiorFactor );
		
			[branch]
			if ( attenuation > 0 && ( GetLightUsageMask( lights[lIdx], mask ) && !excludeFlags ) )
			{
				L = normalize( L );
				if ( deferredDiffuse )
				{
					diffuse += attenuation * (DiffuseLightingPBRPipeline( L, N, V, translucency, specularity, roughness, extraLightingParams ).xyz * lights[lIdx].colorAndType.xyz);
				}
				if ( deferredSpecular )
				{
					specular += attenuation * (SpecularLightingPBRPipeline( L, N, V, specularity, roughness, translucency, extraLightingParams ).xyz * lights[lIdx].colorAndType.xyz);
				}
			}
		}
	}
	
	// apply EnvProbes
	if( envProbes )
	{
		float3 envProbeAmbient  = float3 ( 0, 0, 0 );
		float3 envProbeSpecular = float3 ( 0, 0, 0 );
		{
		#if defined(COMPILE_IN_SHADING_SKIN)
			const float ambientMipIndex		= 1;
			const float envProbeRoughness	= saturate( extraLightingParams.data1.w );
			const float3 envProbeNormal	= normalize( lerp( extraLightingParams.vertexNormal, N, extraLightingParams.data3.y ) );
		#else
			const float ambientMipIndex		= 0;
			const float envProbeRoughness	= roughness;
			const float3 envProbeNormal	= N;
		#endif

			CalcEnvProbes_NormalBasedMipLod( envProbeAmbient, envProbeSpecular, worldSpacePosition, V, envProbeNormal, envProbeNormal, ambientMipIndex, envProbeRoughness, pixelCoord, true, interiorFactor );

		#if defined(COMPILE_IN_DYNAMIC_DECALS)
			ApplyCharactersLightingBoostStencilBased( extraLightingParams.stencilValue, shadowFactorFinal, envProbeAmbient, envProbeSpecular );
		#else
			ApplyCharactersLightingBoostConstBased( shadowFactorFinal, envProbeAmbient, envProbeSpecular );
		#endif
		}

		const float envprobe_distance_scale		= CalcEnvProbesDistantBrightness( worldSpacePosition );

		const float  envprobe_diffuse_scale		= pbrSimpleParams0.z;
		const float3 envprobe_ambient_scale		= envprobe_distance_scale * lerp( pbrSimpleParams0.yyy, pbrSimpleParams0.xxx, shadowFactorFinal );
		const float  envprobe_specular_scale	= pbrSimpleParams0.w;
		const float3 envprobe_reflection_scale	= envprobe_distance_scale * lerp( pbrSimpleParams1.yyy, pbrSimpleParams1.xxx, shadowFactorFinal );			

		diffuse		*= envprobe_diffuse_scale;	
		specular	*= envprobe_specular_scale;

		#if defined(COMPILE_IN_SHADING_SKIN)
		
			float extraEnvProbeFresnelScale = 1; //< 1. this shit is not needed because we don't need to hack any discontinuities since we have to original vertexNormal
			extraEnvProbeFresnelScale *= extraLightingParams.data0.w; //< apply gain mask
						
			const float3 envProbeSpecularity = saturate( extraLightingParams.data1.xyz );
			const float envProbeRoughness = saturate( extraLightingParams.data1.w );
			const float3 envProbeNormal	= normalize( lerp( extraLightingParams.vertexNormal, N, extraLightingParams.data3.y ) );

			envProbeAmbient = lerp(envProbeAmbient, dot(envProbeAmbient, float3( 0.3f, 0.5f, 0.2f )).xxx, 0.5f);

			ambient += NormalizeAmbientPBRPipeline( envprobe_ambient_scale * envProbeAmbient, translucency, V, envProbeNormal, envProbeSpecularity, envProbeRoughness, vertexNormal, extraEnvProbeFresnelScale );
			reflection += envprobe_reflection_scale * envProbeSpecular * CalcFresnelPBRPipelineEnvProbeReflection( V, envProbeNormal, envProbeSpecularity, envProbeRoughness, translucency, vertexNormal, extraEnvProbeFresnelScale );
		
		#else

			float  extraEnvProbeFresnelScale = 1; //< 1. this shit is not needed because we don't need to hack any discontinuities since we have to original vertexNormal
			float3 flatNormal = N;

			#if defined( COMPILE_IN_SHADING_HAIR )
				extraEnvProbeFresnelScale *= extraLightingParams.data0.w; //< apply gain mask
				flatNormal = vertexNormal;
			#endif

			ambient += NormalizeAmbientPBRPipeline( envprobe_ambient_scale * envProbeAmbient, translucency, V, N, specularity, roughness, flatNormal, extraEnvProbeFresnelScale );
			reflection += envprobe_reflection_scale * envProbeSpecular * CalcFresnelPBRPipelineEnvProbeReflection( V, N, specularity, roughness, translucency, flatNormal, extraEnvProbeFresnelScale );

		#endif
	}

	// apply AO
	if( ao )
	{
		float3 nvidiaHBAO = 1;
#if CALC_LIGHTING_PBR_PIPELINE_ALLOW_OPAQUE_BUFFERS
		[branch]
		if ( useSSAO )
		{ 
			nvidiaHBAO = ProcessSampledSSAO( DecodeGlobalShadowBufferSSAO( SYS_SAMPLE_TEXEL( PSSMP_GlobalShadowAndSSAO, pixelCoord ) ) );
		}
#endif

		#if defined(COMPILE_IN_SHADING_SKIN)
					
			nvidiaHBAO = lerp( (1.0f).xxx, nvidiaHBAO, saturate(extraLightingParams.data3.x) );
			nvidiaHBAO = min( nvidiaHBAO, extraLightingParams.ao );			

		#elif defined(COMPILE_IN_SHADING_HAIR)
			
			nvidiaHBAO = lerp( (1.0f).xxx, nvidiaHBAO, saturate(extraLightingParams.data0.y) );

		#endif

		const float3 ssaoMod			= ModulateSSAOByTranslucency( nvidiaHBAO, translucency );
		const float3 AO_probe			= GetProbesSSAOScaleByDimmerFactor( dimmerFactor ) * ssaoMod;
		const float3 AO_nonProbe		= ssaoParams.x * ssaoMod + ssaoParams.y;

		ambient    *= AO_probe;
		reflection *= AO_probe;
		diffuse    *= AO_nonProbe;
		specular   *= AO_nonProbe;
	}
	
	// merge result
	float3 resultColor = albedo * ( ambient +  diffuse ) + specular + reflection;

#if defined(COMPILE_IN_SHADING_SKIN)
	//resultColor = albedo * diffuse;
#endif
	
	if( fog )
	{
		resultColor = ApplyFog( resultColor, false, false, worldSpacePosition ).xyz;
	}

	//
	return resultColor;
}

#if defined(VERTEX_SHADER) || defined(VERTEXSHADER)
#if CALC_LIGHTING_PBR_PIPELINE_ALLOW_OPAQUE_BUFFERS
# error Not compatible
#endif
float3 CalculateVertexLitParticlePipeline( float3 worldSpacePosition, float2 pixelCoord, float shadowSamples, float samplingRadius, SCompileInLightingParams extraLightingParams)
{	
	float4 screenPos = mul( projectionMatrix, mul( worldToView, float4( worldSpacePosition, 1 ) ) );
	float3 hpos = screenPos.xyz/max(0.0001f, screenPos.w);

	pixelCoord = /*saturate*/( hpos.xy * 0.5 * float2(1,-1) + 0.5 );	
	pixelCoord = clamp( pixelCoord, 0.5 * screenDimensions.zw, 1 - 0.5 * screenDimensions.zw );

	float3 N = lightDir.xyz; //using lightDir.xyz instead of N so we get dot(lightDir.xyz, lightDir.xyz) - we don't need sunlight direction in case of global light, just its intensity
	float3 specularity = 0.0f; // set to 0 - not used in particles anyway
	float roughness = 1.0f;
	float translucency = 0.0f; // set to 0 - not used in particles anyway

	float3 diffuse = float3( 0.0f, 0.0f, 0.0f);
	float3 ambient = float3( 0.0f, 0.0f, 0.0f);

	const float3 viewVec		= cameraPosition.xyz - worldSpacePosition;
	const float3 V				= normalize( viewVec );

	//Shadow
	//Hacky shimmering reduction
	//Done by custom shadow map sampling & adding more samples
	const float offset = 0.85f;
	const int qualityDegradation = 0;

	float shadowFactor = 1.0f;	

	shadowFactor = CalcTerrainShadows( worldSpacePosition );

	[branch]
	if ( shadowFactor > 0.0 && shadowSamples>0)
	{
		float4 pos_sh = mul( mShadowTransform, float4(worldSpacePosition, 1) );
		
		// we're inside the cascades
		[branch]
		if ( abs(pos_sh.z) < 0.999f )		
		{				
			const float maxCascades = ShadowPoissonOffsetAndBias.z;
			float4 mask = ( maxCascades.xxxx > float4(0.5f, 1.5f, 2.5f, 3.5f ) );

			float3 parLightDirUp = normalize( cross( lightDir.xyz, float3(0.0f, 0.0f, 1.0f) ) );
			float3 parLightDirRight = cross( parLightDirUp.xyz, lightDir.xyz );
				
			// TODO move to perf platform
			// or find better solution

			float covergence = 0.0f;
			float shadowFactorAdded = 0.0f;
			int currSampleCount = 0;
			int shadowSamplesHalved = floor(shadowSamples * 0.5);
			const float offset = samplingRadius / shadowSamples;  //range

			for(int i=-shadowSamplesHalved; i<=shadowSamplesHalved; i++)
			{		
				for(int j=-shadowSamplesHalved; j<=shadowSamplesHalved; j++)
				{		
					const float3 pos = worldSpacePosition + offset*((i*parLightDirUp + j*parLightDirRight) );				
				
					////////////////////
					pos_sh = mul( mShadowTransform, float4(pos, 1) );
				
					// Sample cascades
					{
						const float4 _dx 		= (pos_sh.xxxx - ShadowOffsetsX);
						const float4 _dy 		= (pos_sh.yyyy - ShadowOffsetsY);
						const float4 _max_d		= max(abs(_dx), abs(_dy)); 				
						const float4 _it 		= (_max_d < ShadowHalfSizes);
										
						const float _i			= dot( _it, mask );
				
						// outside cascade system
						[branch]
						if ( _i < 0.5f )
						{
							shadowFactor = 1.0f;
							break;
						}

						// calculate cascade index
						int iCascadeIndex = (int)( maxCascades - _i );

						// calculate shadow factor for current cascade
						{
							shadowFactorAdded += CalcShadowFromCascade( pos_sh , iCascadeIndex , qualityDegradation );
							currSampleCount += 1;					
						}
					}
				}
			}
			shadowFactor = min( shadowFactor, shadowFactorAdded / currSampleCount );
		}
	}


	float3 shadowFactorFinal = saturate(shadowFactor);

	//Lighting data  
	float slopeBias = 0.0f;
	LightingData ld = CalcGlobalLightPBRPipeline( V, N, translucency, specularity, roughness, worldSpacePosition, pixelCoord, slopeBias, shadowFactor, extraLightingParams );
	shadowFactorFinal = min( ld.preNormDiffuse, shadowFactorFinal );
	
	diffuse += ld.diffuse * shadowFactorFinal;
	
	//Dimmers	
	const float2 dimmerAndInteriorFactor = CalcDimmersFactorAndInteriorFactorTransparency( worldSpacePosition.xyz, pixelCoord * screenDimensions.xy );	
	const float dimmerFactor = saturate(dimmerAndInteriorFactor.x);
	const float interiorFactor = saturate( dimmerAndInteriorFactor.y );

	//Deffered lights

	[loop]
	for ( int lightIndex = 0; lightIndex < lightNum; lightIndex++)
	{
		float3 L = lights[lightIndex].positionAndRadius.xyz - worldSpacePosition;
		float attenuation = CalcLightAttenuation( lightIndex, worldSpacePosition, L );
		
		[branch]
		if ( attenuation > 0 )
		{
			L = normalize( L );
			N = L;
			diffuse += attenuation * (DiffuseLightingPBRPipeline( L, N, V, translucency, specularity, roughness, extraLightingParams ).xyz * lights[lightIndex].colorAndType.xyz);
		}
	}
		
	// EnvProbes
	{
		float3 envProbeAmbient  = float3 ( 0, 0, 0 );
		float3 envProbeSpecular = float3 ( 0, 0, 0 );
		{
			const float ambientMipIndex		= 0;
			const float envProbeRoughness	= roughness;
			const float3 envProbeNormal	=  lightDir.xyz;

			CalcEnvProbes_NormalBasedMipLod( envProbeAmbient, envProbeSpecular, worldSpacePosition, V, envProbeNormal, envProbeNormal, ambientMipIndex, envProbeRoughness, pixelCoord, true, interiorFactor );
		}

		const float envprobe_distance_scale		= CalcEnvProbesDistantBrightness( worldSpacePosition );

		const float  envprobe_diffuse_scale		= pbrSimpleParams0.z;
		const float3 envprobe_ambient_scale		= envprobe_distance_scale * lerp( pbrSimpleParams0.yyy, pbrSimpleParams0.xxx, shadowFactorFinal );

		diffuse		*= envprobe_diffuse_scale;	

		const float  extraEnvProbeFresnelScale = 1.0f;
		ambient  += NormalizeAmbientPBRPipeline( envprobe_ambient_scale * envProbeAmbient, translucency, V, N, specularity, roughness, N, extraEnvProbeFresnelScale );
	}	

	float3 resultColor = ( ambient + diffuse ) * dimmerFactor;
	return resultColor;
	
}
#endif

/// All params in linear space
#if defined(COMPILE_IN_SHADING_EYE)

float3 CalculateLightingPBRPipelineEye( float3 worldSpacePosition, float3 blick, float3 albedo, float3 N_base, float3 N_round, float3 N_bubble, float3 vertexNormal, float3 specularity, float roughness, float translucency, uint2 pixelCoord, SCompileInLightingParams extraLightingParams, bool applyGlobalFog )
{	
	N_base = normalize( N_base );
	N_round = normalize( N_round );
	N_bubble = normalize( N_bubble );
	vertexNormal = normalize( vertexNormal );

	const uint2  tileIdx		= pixelCoord.xy / TILE_SIZE.xx;
    const uint   bufferIdx		= ((tileIdx.y * (int)numTiles.x) + tileIdx.x) * MAX_LIGHTS_PER_TILE;
	const float3 viewVec		= cameraPosition.xyz - worldSpacePosition;
	const float3 V				= normalize( viewVec );

	float3 ambient = 0;
	float3 diffuse = 0;
	float3 specular = 0;
	float3 reflection = 0;

	// get dimmer and shadow factor
	float dimmerFactor = 1;
	float shadowFactor = 1;
	float interiorFactor = 1;
	{
		float4 maskValue = SYS_SAMPLE_TEXEL( PSSMP_GlobalShadowAndSSAO, pixelCoord );
		dimmerFactor = DecodeGlobalShadowBufferDimmers( maskValue );
		shadowFactor = DecodeGlobalShadowBufferShadow( maskValue );
		interiorFactor = DecodeGlobalShadowBufferInteriorFactor( maskValue );
	}

	float3 shadowFactorFinal = shadowFactor;	
	{
		const float slopeBias = 0; //saturate( 1 - dot( FaceN, lightDir.xyz ) );

		LightingData ld = CalcGlobalLightPBRPipeline( V, N_base, N_bubble, translucency, specularity, roughness, worldSpacePosition, pixelCoord / screenDimensions.xy, slopeBias, shadowFactor, extraLightingParams );
		shadowFactorFinal = min( ld.preNormDiffuse, shadowFactorFinal );

		diffuse += ld.diffuse;
		specular += ld.specular;
	}

	/*
	#if (defined(PIXEL_SHADER) || defined(PIXELSHADER))
		{
			const float4 antibleedControlData = extraLightingParams.data1;
			const float upOffset = antibleedControlData.y;
			return albedo * CalcAntiLightbleedDebugVisualisationColor( worldSpacePosition, upOffset );
		}
	#endif
	//*/

	// for lights
	{
		[loop]
		for(int tileLightIdx = 0; tileLightIdx < MAX_LIGHTS_PER_TILE; ++tileLightIdx)
		{
			uint lIdx = TileLights.Load((bufferIdx + tileLightIdx)*4);

			[branch]
			if( lIdx >= MAX_LIGHTS_PER_TILE )
				break;
		
			float3 L = lights[lIdx].positionAndRadius.xyz - worldSpacePosition;
			float attenuation = CalcLightAttenuation( lIdx, worldSpacePosition, L );
		
			attenuation *= GetLocalLightsAttenuationInteriorScale( lights[lIdx].colorAndType.w, interiorFactor );
		
			#if (defined(PIXEL_SHADER) || defined(PIXELSHADER))
			{
				const float4 antibleedControlData = extraLightingParams.data1;
				const bool useRounded = true;
				const float antiBleedAtt = CalcAntiLightBleedAttenuation( useRounded, antibleedControlData.x, antibleedControlData.y, antibleedControlData.y, lights[lIdx].positionAndRadius.xyz, worldSpacePosition );
				
				attenuation *= antiBleedAtt;
			}
			#endif

			[branch]
			if ( attenuation > 0 )
			{
			#ifdef PIXELSHADER
				const float lightsCharacterModifier = PSC_CharactersLightingBoost.x < 0 ? 1 : localLightsExtraParams.x;
				attenuation *= IsLightFlagsCharacterModifierEnabled( lights[lIdx].colorAndType.w ) ? lightsCharacterModifier : 1;
			#endif

				L = normalize( L );
				diffuse += attenuation * (DiffuseLightingPBRPipeline( L, N_base, V, translucency, specularity, roughness, extraLightingParams ).xyz * lights[lIdx].colorAndType.xyz);
				specular += attenuation * (SpecularLightingPBRPipeline( L, N_bubble, V, specularity, roughness, translucency, extraLightingParams ).xyz * lights[lIdx].colorAndType.xyz);
			}
		}
	}

	// apply EnvProbes
	{
		float3 envProbeAmbient  = float3 ( 0, 0, 0 );
		float3 envProbeSpecular = float3 ( 0, 0, 0 );
		{
			const float ambientMipIndex		= 1;
			const float envProbeRoughness	= roughness;

			CalcEnvProbes_NormalBasedMipLod( envProbeAmbient, envProbeSpecular, worldSpacePosition, V, N_round, N_bubble, ambientMipIndex, envProbeRoughness, pixelCoord, true, interiorFactor );
			ApplyCharactersLightingBoostConstBased( shadowFactorFinal, envProbeAmbient, envProbeSpecular );
		}

		const float envprobe_distance_scale		= CalcEnvProbesDistantBrightness( worldSpacePosition );

		const float  envprobe_diffuse_scale		= pbrSimpleParams0.z;
		const float3 envprobe_ambient_scale		= envprobe_distance_scale * lerp( pbrSimpleParams0.yyy, pbrSimpleParams0.xxx, shadowFactorFinal );
		const float  envprobe_specular_scale	= pbrSimpleParams0.w;
		const float3 envprobe_reflection_scale	= envprobe_distance_scale * lerp( pbrSimpleParams1.yyy, pbrSimpleParams1.xxx, shadowFactorFinal );			

		diffuse		*= envprobe_diffuse_scale;	
		specular	*= envprobe_specular_scale;

		const float  extraEnvProbeFresnelScale = 1; //< 1. this shit is not needed because we don't need to hack any discontinuities since we have to original vertexNormal
		ambient  += NormalizeAmbientPBRPipeline( envprobe_ambient_scale * envProbeAmbient, translucency, V, N_round, specularity, roughness, N_round, extraEnvProbeFresnelScale );
		reflection += envprobe_reflection_scale * envProbeSpecular * CalcFresnelPBRPipelineEnvProbeReflection( V, N_round, specularity, roughness, translucency, N_round, extraEnvProbeFresnelScale );
	}

	// apply blick
	{
		float3 blickResult = blick;
		blickResult *= lerp( characterEyeBlicksColor.www, 1, shadowFactorFinal );
		blickResult *= characterEyeBlicksColor.xyz;
		
		specular += blickResult;
	}

	// apply AO
	{
		float3 nvidiaHBAO = 1;

		const float3 ssaoMod			= ModulateSSAOByTranslucency( nvidiaHBAO, translucency );
		const float3 AO_probe			= GetProbesSSAOScaleByDimmerFactor( dimmerFactor ) * ssaoMod;
		const float3 AO_nonProbe		= ssaoParams.x * ssaoMod + ssaoParams.y;

		ambient  	*= AO_probe;
		reflection 	*= AO_probe;
		diffuse  	*= AO_nonProbe;
		specular 	*= AO_nonProbe;
	}

	// merge result
	float3 resultColor = albedo * (ambient + diffuse) + specular + reflection;

	// apply fog
	if ( applyGlobalFog )
	{	
		resultColor = ApplyFog( resultColor, false, false, worldSpacePosition ).xyz;
	}

	//
	return resultColor;
}
#endif

// Simplified version, with global light omitted.
float3 CalculateLightingSpeedTree( float3 N, float3 worldSpacePosition, int2 pixelCoord )
{
	float dummy = worldSpacePosition.x-worldSpacePosition.x + pixelCoord.x-pixelCoord.x;
	return saturate( N.z + dummy );
}

#endif

#ifdef FORWARD

float3 CalcFresnelEnhanced( in float3 L, in float3 H, in float3 specularity )
{
	float3 R0 = specularity;
	float3 fresnelTerm = R0 + max( 0, 1 - R0 )*pow(saturate(1 - dot(H,L)), 5.0f);
	return fresnelTerm;
}

float3 CalcFresnelEnhancedDamped( in float3 L, in float3 H, in float3 specularity, float intensity, float strength )
{
	float3 R0 = specularity;
	float3 fresnelTerm = R0 + max( 0, 1 - R0 ) * (intensity * strength * pow(saturate(1 - dot(H,L)), lerp( 1.0, 5.0f, strength )));
	return fresnelTerm;
}

float3 BlinnPhongEnhanced( in float3 L, in float3 N, in float3 V, in float3 specularity, float hardness, in float glossiness )
{
    float3 _spec = 0;

	float3 H = normalize( V+L );
	
	float3 fresnelTerm = CalcFresnelEnhancedDamped( L, H, specularity, 1.0, hardness );

	//if ( dot(L,N) > 0.0f )
	{
		_spec = fresnelTerm * pow( saturate( dot(H,N) ), glossiness ) / max(dot(V,N),dot(L,N));
		_spec *= pow( saturate( dot(L,N) ), 0.25 );
	}

	return _spec;
}

LightingData CalcGlobalLightEnhanced( in float3 V, in float3 N, in float translucency, in float3 specularity, in float hardness, in float glossiness, in float3 worldSpacePosition, in float2 pixelCoord, float slopeBias, float forcedShadow )
{
	const float3 L = lightDir.xyz;	

	LightingData ld;
	ld.preNormDiffuse = 0;
	ld.diffuse = 0;
	ld.specular = 0;

	//dex++: calculate shadows from cascades only if there is no terrain shadow
	[branch]
	if ( forcedShadow > 0.0 )
	{	
		float3 diffuseFactor = Lambert( L, N, V, translucency ).xyz;
//		[branch]
//		if ( dot(diffuseFactor.xyz,1) > 0 )
		{
			float shadow = forcedShadow + pixelCoord.x-pixelCoord.x;

			const float shadowDiffuse = shadow;
			const float shadowSpecular = shadow;
			const float3 globalLightColor = CalcGlobalLightColor( worldSpacePosition );

			ld.diffuse = shadowDiffuse * diffuseFactor * globalLightColor;
			ld.specular = shadowSpecular * BlinnPhongEnhanced( L, N, V, specularity, hardness, glossiness ) * globalLightColor;
		}
	}
	//dex--
	
	return ld;
}

float CalculateNoise1D( int seed )
{	
	// voodoo and black magic (google libnoise for more info)

 	int A = (seed >> 13) ^ seed;	
	int AA	=( A * (A * A * 60493 + 19990303 ) + 1376312589 ) & 0x7fffffff;
	return AA / 2147483648.0f;
}

float CalculateNoise2D( int seed_x, int seed_y )
{	
	// even more voodoo and black magic (google libnoise for more info)

	int A = seed_x + REVERSE_BITS(seed_y);
 	int AA = (A >> 13) ^ A;	
	int AAA	=( AA * (AA * AA * 60493 + 19990303 ) + 1376312589 ) & 0x7fffffff;
	return AAA / 2147483648.0f;
}

// Manipulate diff / spec / rough based on wetness params
float4 CalculateWetnessSpecular( float3 Wetness, float Roughness, float3 Specularity )
{	
	float4 outp = float4( 0.12f , 0.12f , 0.12f , 1.0f );
	if (max(Specularity.x , (max(Specularity.y , Specularity.z ))) > 0.2f) outp = float4( Specularity.xyz , 1.0f );
	return lerp( float4(Specularity.xyz , 1.0f) , outp , Wetness.z );
}

float4 CalculateWetnessDiffuse( float3 Wetness, float4 Diffuse, float Roughness )
{
	float diffFactor = 0.85f;
	if (max(Diffuse.x , (max(Diffuse.y , Diffuse.z ))) > 0.22f) diffFactor = 0.7f;

	return float4( Diffuse.xyz * lerp( 1.0f, diffFactor, Wetness.z ), 1.0f );
}

float4 CalculateWetnessRoughness( float3 Wetness, float Roughness )
{
	float outp = 0.33f;
	if (Roughness < 0.33f ) outp = Roughness * 0.95f;
	outp = lerp( Roughness , outp , Wetness.z );
	return float4(outp,outp,outp,outp);
}

#endif
