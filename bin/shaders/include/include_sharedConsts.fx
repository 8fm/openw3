#ifndef SHARED_PIXEL_CONSTS_INCLUDED
#define SHARED_PIXEL_CONSTS_INCLUDED

#define CUBE_ARRAY_CAPACITY			7


struct ShaderCullingEnvProbeParams
{
	float4x3	viewToLocal;
	float3		normalScale;
	uint		probeIndex;				// ace_todo: this adds some redundancy
};

struct ShaderCommonEnvProbeParams
{
	float		weight;
	float3		probePos;
	float3		areaMarginScale;
	float4x4	areaWorldToLocal;
	float4		intensities;
	float4x4	parallaxWorldToLocal;	
	uint		slotIndex;
};

START_CB( SharedPixelConsts, 12 )
	// generic
	float4 cameraPosition;
	float4x4 worldToView;
	float4x4 screenToView_UNUSED;
	float4x4 viewToWorld;
	float4x4 projectionMatrix;
	float4x4 screenToWorld;
	float4 cameraNearFar;
	float4 cameraDepthRange;						//< ace_todo: unused -> remove
	float4 screenDimensions;	
	float4 numTiles;	
	float4x4 lastFrameViewReprojectionMatrix;
	float4x4 lastFrameProjectionMatrix;
	float4 localReflectionParam0;
	float4 localReflectionParam1;

	// Speed tree stuff
	float4 speedTreeRandomColorFallback;
	
	// lighting
	float4 translucencyParams0;
	float4 translucencyParams1;	
	
	// fog
	float4 fogSunDir;
	float4 fogColorFront;
	float4 fogColorMiddle;
	float4 fogColorBack;
	float4 fogBaseParams;
	float4 fogDensityParamsScene;
	float4 fogDensityParamsSky;			
	float4 aerialColorFront;
	float4 aerialColorMiddle;
	float4 aerialColorBack;
	float4 aerialParams;

	// speed tree
	float4 speedTreeBillboardsParams;
	float4 speedTreeParams;
	float4 speedTreeRandomColorLumWeightsTrees;
	float4 speedTreeRandomColorParamsTrees0;
	float4 speedTreeRandomColorParamsTrees1;
	float4 speedTreeRandomColorParamsTrees2;
	float4 speedTreeRandomColorLumWeightsBranches;
	float4 speedTreeRandomColorParamsBranches0;
	float4 speedTreeRandomColorParamsBranches1;
	float4 speedTreeRandomColorParamsBranches2;
	float4 speedTreeRandomColorLumWeightsGrass;
	float4 speedTreeRandomColorParamsGrass0;
	float4 speedTreeRandomColorParamsGrass1;
	float4 speedTreeRandomColorParamsGrass2;

	//
	float4 terrainPigmentParams;
	float4 speedTreeBillboardsGrainParams0;
	float4 speedTreeBillboardsGrainParams1;

	// weather env / blending params
	float4 weatherAndPrescaleParams;
	float4 windParams;
	float4 skyboxShadingParams;
	
	// ssao
	float4 ssaoParams;

	// msaaParams
	float4 msaaParams;

	// envmap
	float4 localLightsExtraParams;
	float4 cascadesSize;

	//
	float4 surfaceDimensions;

	// envprobes
	int								numCullingEnvProbeParams;						// ace_todo: remove culling params when I'm convinced we're dripping tiled culling for envProbes
	ShaderCullingEnvProbeParams		cullingEnvProbeParams[CUBE_ARRAY_CAPACITY - 1];	// doesn't include global probe
	ShaderCommonEnvProbeParams		commonEnvProbeParams[CUBE_ARRAY_CAPACITY];

	//
	float4 pbrSimpleParams0;
	float4 pbrSimpleParams1;

	//
	float4 cameraFOV;

	//
	float4 ssaoClampParams0;
	float4 ssaoClampParams1;
	
	//
	float4 fogCustomValuesEnv0;
	float4 fogCustomRangesEnv0;
	float4 fogCustomValuesEnv1;
	float4 fogCustomRangesEnv1;
	float4 mostImportantEnvsBlendParams;

	float4 fogDensityParamsClouds;

	// sky
	float4 skyColor;
	float4 skyColorHorizon;
	float4 sunColorHorizon;
	float4 sunBackHorizonColor;
	float4 sunColorSky;
	float4 moonColorHorizon;
	float4 moonBackHorizonColor;
	float4 moonColorSky;
	float4 skyParams1;
	float4 skyParamsSun;
	float4 skyParamsMoon;
	float4 skyParamsInfluence;

	//
	float4x4 screenToWorldRevProjAware;
	float4x4 pixelCoordToWorldRevProjAware;
	float4x4 pixelCoordToWorld;
	
	//
	float4 lightColorParams;

	//
	float4 halfScreenDimensions;
	float4 halfSurfaceDimensions;
	
	//
	float4 colorGroups[SHARED_CONSTS_COLOR_GROUPS_CAPACITY];
END_CB

#define GBUFF_MATERIAL_MASK_DEFAULT				0
#define GBUFF_MATERIAL_MASK_ENCODED_DEFAULT		0
#define GBUFF_MATERIAL_FLAG_GRASS				1
#define GBUFF_MATERIAL_FLAG_TREES				2
#define GBUFF_MATERIAL_FLAG_BRANCHES			4
#define GBUFF_MATERIAL_FLAG_BILLBOARDS			8
#define GBUFF_SPEEDTREE_SHADOWS					16

float EncodeGBuffMaterialFlagsMask( uint mask )
{
	return (mask + 0.5) / 255.f;
}

uint DecodeGBuffMaterialFlagsMask( float encodedMask )
{
	return (uint)(encodedMask * 255 + 0.5);
}

float3 CalcSkyColor( in float3 worldPos, in float3 worldDirection )
{
	const float3 moonDir = skyParamsMoon.yzw;
	const float3 sunDir = skyParamsSun.yzw;
	const float moonInfluence = saturate( skyParamsInfluence.y );
	const float sunInfluence = saturate( skyParamsInfluence.x );
	
	const float3 worldVec = normalize(worldDirection);
	const float moonDot_xy = dot(normalize(moonDir.xy),normalize(worldVec.xy));
	const float moonDot_xyz = dot(moonDir.xyz,worldVec.xyz);
	const float sunDot_xy = dot(normalize(sunDir.xy),normalize(worldVec.xy));
	const float sunDot_xyz = dot(sunDir.xyz,worldVec.xyz);
	
	float3 overallColor = skyColor.xyz;
	
	const float3 moonHorizonEffect = lerp( moonBackHorizonColor.xyz, moonColorHorizon.xyz, moonDot_xy *0.5f + 0.5f );
	const float3 sunHorizonEffect = lerp( sunBackHorizonColor.xyz, sunColorHorizon.xyz, sunDot_xy *0.5f + 0.5f );
	
	float3 horizonColor = skyColorHorizon.xyz;
	horizonColor = lerp(horizonColor, moonHorizonEffect, (1 - worldVec.z * worldVec.z ) * moonInfluence );
	horizonColor = lerp(horizonColor, sunHorizonEffect, (1 - worldVec.z * worldVec.z ) * sunInfluence );

	worldPos = cameraPosition.xyz + 1000 * normalize(worldPos - cameraPosition.xyz);

	float hor_factor = saturate( 1.0 / max( 0.0001, 0.1 + ((worldPos.z + 710) / 1000.0) * skyParams1.z ) );
	hor_factor = pow( hor_factor, 2.8 );
	hor_factor = 1 - hor_factor;

	float3 colorWithHorizon = lerp(horizonColor,overallColor,hor_factor);

	const float moonEffect = pow( saturate( moonDot_xyz ), skyParamsMoon.x ) * moonInfluence;
	const float sunEffect = pow( saturate( sunDot_xyz ), skyParamsSun.x ) * sunInfluence;
	colorWithHorizon = lerp(colorWithHorizon,moonColorSky.xyz,moonEffect);
	colorWithHorizon = lerp(colorWithHorizon,sunColorSky.xyz,sunEffect);	

	float3 color = colorWithHorizon * skyParams1.x * skyParams1.y;

	return color;
}

bool IsReversedProjectionCamera()
{
	return 1.f == cameraDepthRange.y;
}

# define PSGetProjectionCameraSkyDepth			(1 - cameraDepthRange.y)
# define PSGetProjectionCameraNearestDepth		(cameraDepthRange.y)

float DecodeGlobalShadowBufferSSAO( float4 bufferValue )
{
	return bufferValue[GLOBAL_SHADOW_BUFFER_CHANNEL_SSAO];
}

float DecodeGlobalShadowBufferShadow( float4 bufferValue )
{
	return bufferValue[GLOBAL_SHADOW_BUFFER_CHANNEL_SHADOW];
}

float DecodeGlobalShadowBufferInteriorFactor( float4 bufferValue )
{
	return bufferValue[GLOBAL_SHADOW_BUFFER_CHANNEL_INTERIOR_FACTOR];
}

float DecodeGlobalShadowBufferDimmers( float4 bufferValue )
{
	return bufferValue[GLOBAL_SHADOW_BUFFER_CHANNEL_DIMMERS];
}

float TransformDepthRevProjAware( float depth )
{
	return depth * cameraDepthRange.x + cameraDepthRange.y;
}

float3 PositionFromDepthFullCustom(in float depth, in float2 pixelCoord, float2 customScreenDimensions, float4x4 customScreenToWorld )
{
    float2 cpos = (pixelCoord + 0.5f) / customScreenDimensions;
    cpos *= 2.0f;
    cpos -= 1.0f;
    cpos.y *= -1.0f;
    float4 positionWS = mul( customScreenToWorld, float4(cpos, depth, 1.0f) );
    return positionWS.xyz / positionWS.w;
}

float3 PositionFromDepth(in float depth, in float2 pixelCoord, float2 customScreenDimensions )
{
	return PositionFromDepthFullCustom( depth, pixelCoord, customScreenDimensions, screenToWorld );
}

float3 PositionFromDepth(in float depth, in float2 pixelCoord )
{
	float4 positionWS = mul(pixelCoordToWorld, float4(pixelCoord, depth, 1.0f) );
    return positionWS.xyz / positionWS.w;
}

float3 PositionFromDepthRevProjAware(in float depth, in float2 pixelCoord, float2 customScreenDimensions )
{
    float2 cpos = (pixelCoord + 0.5f) / customScreenDimensions;
    cpos *= 2.0f;
    cpos -= 1.0f;
    cpos.y *= -1.0f;
    float4 positionWS = mul(screenToWorldRevProjAware, float4(cpos, depth, 1.0f) );
    return positionWS.xyz / positionWS.w;
}

float3 PositionFromDepthRevProjAware(in float depth, in float2 pixelCoord )
{
	float4 positionWS = mul(pixelCoordToWorldRevProjAware, float4(pixelCoord, depth, 1.0f) );
    return positionWS.xyz / positionWS.w;
}

float ProjectDepth( in float depth )
{
	return 1.0 / (cameraNearFar.x * depth) - cameraNearFar.y / cameraNearFar.x;
}

#define DEPROJECT_DEPTH_DIV_THRESHOLD 0.0001
float DeprojectDepth( in float depth )
{
	// make linear and deproject (now it should be in view space)
	float div = depth * cameraNearFar.x + cameraNearFar.y;
	return 1.0 / max( DEPROJECT_DEPTH_DIV_THRESHOLD, div );
}

float DeprojectDepthRevProjAware( in float depth )
{
	depth = TransformDepthRevProjAware( depth );							// ace_fix precision might not be the best here !!!
	// make linear and deproject (now it should be in view space)
	float div = depth * cameraNearFar.x + cameraNearFar.y;
	return 1.0 / max( DEPROJECT_DEPTH_DIV_THRESHOLD, div );
}

bool IsSkyByProjectedDepth( float depth )
{
	return depth >= 1.0;
}

bool IsSkyByProjectedDepthRevProjAware( float depth )
{
	return TransformDepthRevProjAware( depth ) >= 1.0;
}

bool IsSkyByLinearDepth( float depth )
{
	return depth >= 0.999 * DeprojectDepth( 1.0 ); //< ace_todo: this is kinda shitty
}

float CalcEnvProbesDistantBrightnessByHorizDist( float horizDist )
{
	return lerp( 1.0, pbrSimpleParams1.z, saturate( horizDist * lightColorParams.x + lightColorParams.y ) );
}

float CalcEnvProbesDistantBrightness( float3 worldSpacePosition )
{
	float displaceHoriz = length( worldSpacePosition.xy - cameraPosition.xy );
	return CalcEnvProbesDistantBrightnessByHorizDist( displaceHoriz );
}

float3 ModulateSSAOByTranslucency( float3 ssaoValue, float translucency )
{
	return lerp( float3(1.0, 1.0, 1.0), ssaoValue, lerp( 1.0, ssaoParams.z, translucency ) ); // ace_optimize (test solution for too dark ssao on grass)
}

float3 ProcessSampledSSAO( float ssaoValue )
{
	return saturate( ssaoValue * ssaoClampParams0.xyz + ssaoClampParams1.xyz );
}

float3 SpeedTreeApplyRandomColorAlbedo_CustomColors( float4 randColorData, float3 lumWeights, float4 randomColorParams0, float4 randomColorParams1, float4 randomColorParams2, float3 col )
{
	const float  lum = dot( lumWeights.xyz, col.xyz );
	const float3 colorWeights = randColorData.xyz;
	const float4 mergedData = colorWeights.x * randomColorParams0 + colorWeights.y * randomColorParams1 + colorWeights.z * randomColorParams2;
	const float3 randColor = max( 0.0, 1 - mergedData.xyz );
	const float  randSaturation = mergedData.w;
	
	col = pow( max( 0, col ), 2.2 );
	col = max( 0.0, lum + (col - lum) * randSaturation );
	col = col * randColor.xyz;
	col = pow( col, 1.0 / 2.2 );

	return col;
}

float3 SpeedTreeApplyRandomColorAlbedoTrees( float4 randColorData, float3 col )
{
	return SpeedTreeApplyRandomColorAlbedo_CustomColors( randColorData, speedTreeRandomColorLumWeightsTrees.xyz, speedTreeRandomColorParamsTrees0, speedTreeRandomColorParamsTrees1, speedTreeRandomColorParamsTrees2, col );
}

float3 SpeedTreeApplyRandomColorAlbedoBranches( float4 randColorData, float3 col )
{
	return SpeedTreeApplyRandomColorAlbedo_CustomColors( randColorData, speedTreeRandomColorLumWeightsBranches.xyz, speedTreeRandomColorParamsBranches0, speedTreeRandomColorParamsBranches1, speedTreeRandomColorParamsBranches2, col );
}

float3 SpeedTreeApplyRandomColorAlbedoGrass( float4 randColorData, float3 col )
{
	return SpeedTreeApplyRandomColorAlbedo_CustomColors( randColorData, speedTreeRandomColorLumWeightsGrass.xyz, speedTreeRandomColorParamsGrass0, speedTreeRandomColorParamsGrass1, speedTreeRandomColorParamsGrass2, col );
}

float3 SpeedTreeApplyRandomColorFallback( float4 randColorData, float3 col )
{
	col = pow( max( 0, col ), 2.2 );
	col = col * speedTreeRandomColorFallback.xyz;
	col = pow( col, 1.0 / 2.2 );

	return col;
}

float ApplyAlphaTransform( float alpha, float2 alphaTransform )
{
	return alpha * alphaTransform.x + alphaTransform.y;
}

float2 AddAlphaTransformOffset( float2 alphaTransform, float offset )
{
	return float2 ( alphaTransform.x, alphaTransform.y + offset );
}

uint BuildMSAACoverageMask_AlphaToCoverage( float alphaValue )
{
	uint result = 0xffffffff;
	
	[branch]
	if ( msaaParams.y > 0 )
	{
		/*
		// test code for manual dithering (requires vpos passed in)
		float r = 0;
		r += vpos.x * 6.2123125431414;
		r += vpos.y * 4.3125431414511;
		alphaValue -= fmod( r, msaaParams.y );
		*/

		result = ((alphaValue > 0) ? 0xfffffff1 : 0xfffffff0) + ((alphaValue > msaaParams.y) ? 2 : 0) + ((alphaValue > msaaParams.z) ? 4 : 0) + ((alphaValue > msaaParams.w) ? 8 : 0);
	}

	return result;
}

uint BuildMSAACoverageMask_AlphaToCoverage( float origAlpha, float2 alphaTransform, float alphaThreshold )
{
	return BuildMSAACoverageMask_AlphaToCoverage( (ApplyAlphaTransform( origAlpha, alphaTransform ) - alphaThreshold) / max( 0.0001, (ApplyAlphaTransform( 1.0, alphaTransform ) - alphaThreshold) ) );	
}

uint BuildMSAACoverageMask_AlphaTestSS( /*float origAlpha,*/ Texture2D tex, SamplerState smp, float2 coord, float2 alphaTransform, float alphaThreshold )
{	
#define BUILD_COVERAGE_MASK_SUPERSAMPLE(_NumSamples)				\
	{																\
		float2 alphaTransformFinal = AddAlphaTransformOffset( alphaTransform, -alphaThreshold );	\
																	\
		coverage = 0;												\
		float2 texCoord_ddx = ddx(coord);							\
		float2 texCoord_ddy = ddy(coord);							\
																	\
		[unroll]													\
		for (int i = 0; i < _NumSamples; ++i)						\
		{															\
			float2 texelOffset = msaaOffsets[i].x * texCoord_ddx;	\
			texelOffset += msaaOffsets[i].y * texCoord_ddy;													\
			float currAlpha = tex.Sample( smp, coord + texelOffset ).a;										\
			coverage |= ApplyAlphaTransform( currAlpha, alphaTransformFinal ) >= 0 ? 1 << i : 0;			\
		}																									\
	}

	uint coverage = 0xffffffff;
		
	[branch]
	if ( 1 != msaaParams.x )
	{
		[branch]
		if ( 2 == msaaParams.x )
		{
			SYS_STATIC const float2 msaaOffsets[2] = 
			{ 
				float2(0.25, 0.25), 
				float2(-0.25,-0.25) 
			};

			BUILD_COVERAGE_MASK_SUPERSAMPLE( 2 );
		}
		else if ( 4 == msaaParams.x )
		{
			SYS_STATIC const float2 msaaOffsets[4] =
			{
				float2(-0.125, -0.375), 
				float2(0.375, -0.125),
				float2(-0.375, 0.125), 
				float2(0.125, 0.375)
			};
			
			BUILD_COVERAGE_MASK_SUPERSAMPLE( 4 );
		}
	}

	return coverage;
	
#undef BUILD_COVERAGE_MASK_SUPERSAMPLE
}

#endif
/* make sure leave an empty line at the end of ifdef'd files because of SpeedTree compiler error when including two ifdef'ed files one by one : it produces something like "#endif#ifdef XXXX" which results with a bug */
