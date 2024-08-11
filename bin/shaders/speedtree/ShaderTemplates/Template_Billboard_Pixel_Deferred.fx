///////////////////////////////////////////////////////////////////////  
$HEADER$
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com


///////////////////////////////////////////////////////////////////////
//	Lots of #defines controlled by the user settings during SRT compilation

$DEFINES$


///////////////////////////////////////////////////////////////////////
//	Include files

#define PIXEL_SHADER
#include "Include_SetUp.fx"
#include "Include_Uniforms.fx"
#include "Include_SamplersAndTextureMacros.fx"
#include "Include_Utility.fx"
#include "Include_TreeTextures.fx"
#include "Include_TextureUtility.fx"
#include "Include_FogAndSky.fx"
#include "Include_UserInterpolants.fx"
#include "Include_Gbuffer.fx"

// LAVA+
#include "../../include/common.fx"
#include "../../include/globalConstantsPS.fx"
#include "../../include/include_utilities.fx"
#include "../../include/include_sharedConsts.fx"
#include "../../include/include_envProbe.fx"

float3 SpeedTreeApplyRandomColorAlbedoRuntimeNonGrass( float4 randColorData, float3 col, float controlValue )
{
	if ( controlValue > 0.501 )
	{		
		if ( controlValue > 0.521 )
		{
			col = SpeedTreeApplyRandomColorAlbedoTrees( randColorData, col );				
		}
		else if ( controlValue > 0.511 )
		{
			col = SpeedTreeApplyRandomColorAlbedoBranches( randColorData, col );		
		}
		else
		{
			col = SpeedTreeApplyRandomColorAlbedoGrass( randColorData, col );
		}
	}
	/* this case is not really needed here
	else
	{
		col = SpeedTreeApplyRandomColorFallback( randColorData, col );
	}
	*/

	return col;	
}

float3 SpeedTreeApplyRandomColorAlbedoRuntime( float4 randColorData, float3 col )
{
	return SpeedTreeApplyRandomColorAlbedoRuntimeNonGrass( randColorData, col, m_vLavaCustomBaseTreeParams.x );
}
// LAVA-

///////////////////////////////////////////////////////////////////////
//	Pixel shader entry point
//
//	Main lighting/wind deferred vertex shader for billboard geometry

#ifdef __PSSL__
	[RE_Z]
#elif defined(__XBOX_ONE)
	#define __XBOX_FORCE_PS_ZORDER_RE_Z 1
#endif
#if (!ST_OPENGL)

	ST_OUTPUT_TYPE main(float4 v2p_vInterpolant0			: ST_VS_OUTPUT,	// projection
						float3 v2p_vInterpolant1			: TEXCOORD0,	// texcoords, alpha scalar
						float3 v2p_vInterpolant2			: TEXCOORD1,	// instance worldspace up vector
						float3 v2p_vHueVariationInterpolant	: TEXCOORD2	// hue variation
                    //LAVA++
						,	float4 v2p_vInterpolant3 			: TEXCOORD3
						,	float3 v2p_vInterpolant4 			: TEXCOORD4
					//LAVA--
						) : ST_PS_OUTPUT

#else // !ST_OPENGL

	// v2p_vInterpolant0 / projection is implicit
	varying float3 v2p_vInterpolant1; // texcoords, alpha scalar
	varying float3 v2p_vInterpolant2; // instance worldspace up vector
	//LAVA++
	varying float4 v2p_vInterpolant3;
	varying float3 v2p_vInterpolant4;
	//LAVA--
	varying float3 v2p_vHueVariationInterpolant; // hue variation

	void main(void)

#endif
{
	// GLSL uses a different return mechanism
	SMultiRenderTargets sOutput;

	// setup outputs to be passed/packed into g-buffer out
	float3 out_vAlbedo = float3(0.0, 0.0, 0.0);
	float  out_fAlpha = 1.0;
	float3 out_vNormal = float3(0.0, 0.0, 0.0);
	float  out_fSpecularMask = 0.0;
	float  out_fSpecularPower = 128.0; //LAVA
	float  out_fTransmissionMask = 0.0;

	// LAVA++
	uint   out_Coverage = 0xffffffff;
	float3 out_vWorldPosition = v2p_vInterpolant4.xyz;
	// LAVA--
	// input attributes via interpolants
	float2 v2p_vTexCoords = v2p_vInterpolant1.xy;
	float  v2p_fAlphaScalar = v2p_vInterpolant1.z;
	float3 v2p_vInstaceUpVector = v2p_vInterpolant2.xyz;

	// optional input attributes
	float3 v2p_vHueVariation = v2p_vHueVariationInterpolant.xyz;

	// diffuse texture lookup
	//
	// .rgb = written by the Compiler app to be (diffuse_map * diffuse_light_scalar * ambient_occlusion)
	//   .a = alpha mask
	float4 texDiffuse = SampleTexture(DiffuseMap, v2p_vTexCoords);

	//LAVA++
	float3 grain = 0;
	{
		/*
		int2 vpos = (int2)v2p_vInterpolant0.xy;
		float3 tc = float3 ( abs(in_vTexCoords.xy) * terrainPigmentParams1.x, 0 );
		tc /= terrainPigmentParams1.y;
		tc = floor( tc );
		tc *= terrainPigmentParams1.y;
		grain = SpeedTreeCalcRandomColorData( tc ).xyz;
		*/
		
		uint w, h, w2, h2;										
		DiffuseMap.GetDimensions( w, h );						
		MipNoiseTexture.GetDimensions( w2, h2 );
		grain = MipNoiseTexture.SampleBias( samStandard, v2p_vTexCoords.xy * float2( w, h ) / float2( w2, h2 ), speedTreeBillboardsGrainParams0.x ).xyz;
	}
	// LAVA--


	// test for possible early exit condition
	out_fAlpha = texDiffuse.a * v2p_fAlphaScalar;
	if (ST_ALPHA_TEST_NOISE)
	{
		float fDelta = (m_fAlphaScalar - v2p_fAlphaScalar) / m_fAlphaScalar;
		out_fAlpha -= AlphaTestNoise_Billboard(v2p_vTexCoords) * fDelta;
	}

    // LAVA++
	out_fAlpha -= (grain.x * speedTreeBillboardsGrainParams0.y + speedTreeBillboardsGrainParams0.z) * saturate( texDiffuse.a * speedTreeBillboardsGrainParams1.y );		
	// LAVA--

	CheckForEarlyExit(out_fAlpha, ST_TRANSPARENCY_ACTIVE);

	// hue variation
	texDiffuse.rgb += v2p_vHueVariation;

	// extract and save diffuse color & intensity
	// LAVA++ applying random colorization
	out_vAlbedo = texDiffuse.rgb;
	out_vAlbedo.rgb = SpeedTreeApplyRandomColorAlbedoRuntime( v2p_vInterpolant3.rgba, out_vAlbedo.rgb );
	out_vAlbedo.rgb *= m_vDiffuseColor.rgb;
	out_vAlbedo.rgb *= speedTreeBillboardsParams.xyz;
	out_vAlbedo.rgb *= (grain.x - 0.5) * speedTreeBillboardsGrainParams0.w + 1;
	// LAVA--

	// extract and encode normal
	{
		// normal map lookup
		float4 texNormalMap = SampleTexture(NormalMap, v2p_vTexCoords);

		// convert to [-1,1] range
		float3 vRawNormal = DecodeVectorFromColor(texNormalMap).rgb;

                // LAVA+
		// Apply normal noise
		vRawNormal.xy *= (grain.y - 0.5) * speedTreeBillboardsGrainParams1.x + 1;
		vRawNormal.xyz = normalize( vRawNormal.xyz );
		// LAVA-
		// lighting vectors
		float3 vNormal = -m_vCameraDirection;
		float3 vBinormal = v2p_vInstaceUpVector;
		float3 vTangent = normalize(cross(vBinormal, vNormal));

		if (ST_COORDSYS_RIGHT_HANDED && ST_COORDSYS_Y_UP)
			vTangent = -vTangent;

		if (ST_COORDSYS_LEFT_HANDED)
		{
			vNormal = -vNormal;
			if (ST_COORDSYS_Y_UP)
				vTangent = -vTangent;
		}

		// orient normal to align with triangle
		float3x3 mTriangleOrientation = BuildOrientationMatrix(vTangent, vBinormal, vNormal);
		float3 vOrientedNormal = normalize(mul_float3x3_float3(mTriangleOrientation, vRawNormal));

		// encode vector back into color
		out_vNormal = vOrientedNormal /*LAVA++*/* 0.5f + 0.5f;
	}

	{
		// LAVA++
		out_fTransmissionMask = m_vTransmissionColor.g * speedTreeBillboardsParams.w;
		// LAVA--
	}
	
	const Float out_encodedGBuffMaterialMask = EncodeGBuffMaterialFlagsMask( GBUFF_MATERIAL_FLAG_BILLBOARDS );

	// output syntax is platform-specific, encapsulated in an include
	#include "InlinePixel_DeferredOutput.fx"
}
