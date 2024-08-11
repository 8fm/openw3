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

// LAVA++
#include "../../include/common.fx"
#include "../../include/globalConstantsPS.fx"
#include "../../include/include_utilities.fx"
#include "../../include/include_sharedConsts.fx"
#include "../../include/include_envProbe.fx"
#if (ST_USED_AS_GRASS)
	#include "../../include/include_pigment.fx"
#endif

#define LAVA_TREE_PART_MARK_VALUE			(1 - m_fOneMinusAmbientContrastFactor)

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
	else
	{
		col = SpeedTreeApplyRandomColorFallback( randColorData, col );
	}

	return col;	
}

float3 SpeedTreeApplyRandomColorAlbedoRuntime( float4 randColorData, float3 col )
{
#if (ST_USED_AS_GRASS)
	if ( m_vLavaCustomBaseTreeParams.z > 0.501 )
	{
		col = SpeedTreeApplyRandomColorAlbedoGrass( randColorData, col );
	}
	else
	{
		col = SpeedTreeApplyRandomColorFallback( randColorData, col );
	}
#else
	col = SpeedTreeApplyRandomColorAlbedoRuntimeNonGrass( randColorData, col, LAVA_TREE_PART_MARK_VALUE );
#endif

	return col;
}

float SpeedTreeBuildGBuffMaterialMask()
{
	uint mask = GBUFF_MATERIAL_MASK_DEFAULT;

#if (ST_USED_AS_GRASS)
	//if ( g_vGrassParams1.x > 0.501 )
	{
		mask = GBUFF_MATERIAL_FLAG_GRASS;
	}
#else
	const float controlValue = LAVA_TREE_PART_MARK_VALUE;
	if ( controlValue > 0.501 )
	{		
		if ( controlValue > 0.521 )
		{
			mask = GBUFF_MATERIAL_FLAG_TREES;
		}
		else if ( controlValue > 0.511 )
		{
			mask = GBUFF_MATERIAL_FLAG_BRANCHES;
		}
		else
		{
			mask = GBUFF_MATERIAL_FLAG_GRASS;
		}
	}
#endif

	return EncodeGBuffMaterialFlagsMask( mask );
}

#if (ST_USED_AS_GRASS)

float3 ApplyTerrainPigmentRuntime( float4 pigment, float3 albedo, float localVerticalPos, float dist )
{
	const float amountScale = m_vLavaCustomBaseTreeParams.x;
	const float distInfluence = m_vLavaCustomBaseTreeParams.y;
	const float pigmentStartPos = m_vLavaCustomMaterialParams.x;
	const float pigmentGradient = m_vLavaCustomMaterialParams.y;
	
	return ApplyTerrainPigment( pigment, albedo, dist, distInfluence, localVerticalPos, pigmentStartPos, pigmentGradient, amountScale );
}

#endif

// LAVA--

///////////////////////////////////////////////////////////////////////
//	Pixel shader entry point
//
//	Main lighting/wind forward pixel shader for 3D geometry

#ifdef __PSSL__
	[RE_Z]
#elif defined(__XBOX_ONE)
	#define __XBOX_FORCE_PS_ZORDER_RE_Z 1
#endif
$PIXEL_MAIN_FUNCTION_DECL$
{
	// output
	#if (!ST_OPENGL)
		// GLSL uses a different return mechanism
		SMultiRenderTargets sOutput;
	#endif
	
	//LAVA++
	// For trees, in non-depth-only passes (not shadows), dissolve the tree based on fade parameter.
	#if !(ST_USED_AS_GRASS) && (ST_USER_INTERPOLANT2)
	{
		float2 texSize;
		float2 tex = v2p_vInterpolant0.xy / 16;	// HACK : dissolve texture is 16x16
		tex = tex - floor( tex );
		float dissolveTex = SampleTexture(DissolveMap, tex).x;
		//float dissolveTex = CalcDissolvePattern( (int2)v2p_vInterpolant0.xy, 2 );
		if ( v2p_vUserInterpolant2.x > dissolveTex )
		{
			discard;
		}
	}
	#endif
	//LAVA--

	// setup outputs to be passed/packed into g-buffer out
	float3 out_vAlbedo = float3(0.0, 0.0, 0.0);
	float  out_fAlpha = 1.0;
	float3 out_vNormal = float3(0.0, 0.0, 0.0);
	float  out_fSpecularMask = 0.0;
	float  out_fSpecularPower = 30.0;
	float  out_fTransmissionMask = 0.0;

	// LAVA++
	uint   out_Coverage = 0xffffffff;
	float3 out_vWorldPosition = v2p_vUserInterpolant1.xyz;
	const float localVerticalPosition = v2p_vUserInterpolant1.w;
	const float cameraDist = length( out_vWorldPosition - m_vCameraPosition.xyz );
	// LAVA--
	// all possible variables that may come from the vertex shader; the "v2p" prefix indicates that these
	// variables are values that go from [v]ertex-[2]-[p]ixel shader
	float  v2p_fFogScalar = 0.0;
	float3 v2p_vFogColor = float3(0.0, 0.0, 0.0);
	float2 v2p_vDiffuseTexCoords = float2(0.0, 0.0);
	float3 v2p_vPerVertexLightingColor = float3(0.0, 0.0, 0.0);
	float3 v2p_vNormalInTangentSpace = float3(0.0, 0.0, 0.0);
	float  v2p_fPerVertexAmbientContrast = 0.0;
	float  v2p_fRenderEffectsFade = 1.0;
	float  v2p_fAmbientOcclusion = 0.0;
	float3 v2p_vHueVariation = float3(0.0, 0.0, 0.0);
	float2 v2p_vDetailTexCoords = float2(0.0, 0.0);
	float3 v2p_vNormal = float3(0.0, 0.0, 0.0);
	float3 v2p_vBinormal = float3(0.0, 0.0, 0.0);
	float3 v2p_vTangent = float3(0.0, 0.0, 0.0);
	float3 v2p_vSpecularHalfVector = float3(0.0, 0.0, 0.0);
	float  v2p_fPerVertexSpecularDot = 0.0;
	float  v2p_fFadeToBillboard = 1.0;
	float  v2p_fTransmissionFactor = 0.0;
	float3 v2p_vBranchSeamDiffuse = float3(0.0, 0.0, 0.0);
	float2 v2p_vBranchSeamDetail = float2(0.0, 0.0);
	float  v2p_fShadowDepth = 0.0;
	float4 v2p_vShadowMapProjection0 = float4(0.0, 0.0, 0.0, 0.0);
	float4 v2p_vShadowMapProjection1 = float4(0.0, 0.0, 0.0, 0.0);
	float4 v2p_vShadowMapProjection2 = float4(0.0, 0.0, 0.0, 0.0);
	float4 v2p_vShadowMapProjection3 = float4(0.0, 0.0, 0.0, 0.0);

	// set initial values for those v2p parameters that are in use
	$PIXEL_GET_PARAMS_FROM_INPUT_ATTRIBS$

	// diffuse texture lookup
	float4 texDiffuse = SampleTexture(DiffuseMap, v2p_vDiffuseTexCoords);

	// adjust alpha value if grass or fading to billboard (alpha "fizzle" with noise)
	if ( ST_USED_AS_GRASS )
	{
		out_fAlpha = texDiffuse.a * v2p_fFadeToBillboard;

		if (ST_ALPHA_TEST_NOISE)
		{
			float fDelta = (m_fAlphaScalar - v2p_fFadeToBillboard) / m_fAlphaScalar;
			out_fAlpha -= AlphaTestNoise_3dTree(v2p_vDiffuseTexCoords) * fDelta;
		}
	}
	else if ( ST_ONLY_BRANCHES_PRESENT )
	{
		// empty
	}
	else
	{
		out_fAlpha = texDiffuse.a * v2p_fFadeToBillboard;
	}

	// attempt to get earliest possible exit if pixel is transparent
	CheckForEarlyExit(out_fAlpha, ST_TRANSPARENCY_ACTIVE);

	// branch seam blending (active only for branch geometry)
	const float2 c_fSeamDiffuseTexcoords = v2p_vBranchSeamDiffuse.xy;
	const float2 c_fSeamDetailTexcoords = v2p_vBranchSeamDetail.xy;
	const float c_fSeamBlendWeight = v2p_vBranchSeamDiffuse.z;
	float fSeamBlend = min(pow(c_fSeamBlendWeight, m_fBranchSeamWeight), 1.0);

	// extract and save diffuse color & intensity
	{
		// diffuse color
		float3 vDiffuseColorOutput = LookupColorWithDetail(v2p_vDiffuseTexCoords, v2p_vDetailTexCoords, v2p_fRenderEffectsFade);

		if (ST_EFFECT_BRANCH_SEAM_SMOOTHING)
		{
			float3 vColorAtSeam = LookupColorWithDetail(v2p_vBranchSeamDiffuse.xy, v2p_vDetailTexCoords, v2p_fRenderEffectsFade);
			vDiffuseColorOutput = lerp(vColorAtSeam, vDiffuseColorOutput, fSeamBlend);
		}

		if (ST_EFFECT_HUE_VARIATION)
			vDiffuseColorOutput.rgb += v2p_vHueVariation;

		// LAVA++
		// out_vAlbedo.rgb = vDiffuseColorOutput * m_vDiffuseColor;
		out_vAlbedo.rgb = SpeedTreeApplyRandomColorAlbedoRuntime( v2p_vUserInterpolant0, vDiffuseColorOutput ) * m_vDiffuseColor.rgb /* LAVA++--*/* m_vDirLight.m_vDiffuse;
		// LAVA--
		
		if (ST_EFFECT_AMBIENT_OCCLUSION)
        {
			// LAVA++
			float ambientOcclusion = v2p_fAmbientOcclusion;
			const float ambientOcclusionAmount = speedTreeParams.x;
			ambientOcclusion = saturate( lerp( 1.0, ambientOcclusion, ambientOcclusionAmount ) );

			out_vAlbedo.rgb *= ambientOcclusion;
			// LAVA--
        }
		
		// LAVA++
		// Apply pigment
		#if (ST_USED_AS_GRASS)
		{
			const float4 pigment = v2p_vUserInterpolant2;
			out_vAlbedo.rgb = ApplyTerrainPigmentRuntime( pigment, out_vAlbedo.rgb, localVerticalPosition, cameraDist );
		}
		#endif
		// LAVA--
	}

	// lookup texture shared by both specular and transmission
	float4 texSpecularTransmissionMask = SampleTexture(SpecularMaskMap, v2p_vDiffuseTexCoords);

	// extract and encode normal
	{
		// transmission
		#if (ST_USED_AS_GRASS)
		float normalFlatness = 0;
		if ( !(m_vLavaCustomBaseTreeParams.w > 0) )
		{	
		#endif
			if (ST_EFFECT_TRANSMISSION)
			{
				out_fTransmissionMask = texSpecularTransmissionMask.a /* LAVA++--*/* m_vDirLight.m_vTransmission;

				// apply transmission color, but just the one color component
				out_fTransmissionMask *= RgbToLuminance(m_vTransmissionColor);

				// fade effect out if enabled
				if (ST_EFFECT_TRANSMISSION_FADE)
					out_fTransmissionMask *= v2p_fRenderEffectsFade;
			}
		#if (ST_USED_AS_GRASS)		
		}
		else
		{
			normalFlatness = speedTreeParams.y;
		}
		#endif		
		
		// Calculate normal
		{
			float3 vOutputNormal = LookupNormalWithDetail(v2p_vDiffuseTexCoords, v2p_vDetailTexCoords, v2p_fRenderEffectsFade);
			#if (ST_USED_AS_GRASS)
			vOutputNormal = normalize( vOutputNormal + float3(0,0,normalFlatness) );
			#endif

			if (ST_EFFECT_BRANCH_SEAM_SMOOTHING)
			{
				float3 vNormalAtSeam = LookupNormalWithDetail(v2p_vBranchSeamDiffuse.xy, v2p_vBranchSeamDetail, v2p_fRenderEffectsFade);
				vOutputNormal = lerp(vNormalAtSeam, vOutputNormal, fSeamBlend);
			}

			// orient normal to align with triangle
			float3 vBinormal = cross(v2p_vNormal, v2p_vTangent);
			float3x3 mTriangleOrientation = BuildOrientationMatrix(v2p_vTangent, vBinormal, v2p_vNormal);
			float3 vOrientedNormal = normalize(mul_float3x3_float3(mTriangleOrientation, vOutputNormal));

			// encode vector back into color
			out_vNormal = vOrientedNormal * 0.5f + 0.5f;
		}
	}

	// specular
	if (ST_EFFECT_SPECULAR)
	{
		// mask is rgb, but we can store only one color component
		out_fSpecularMask = RgbToLuminance(texSpecularTransmissionMask.rgb) /* LAVA++--*/* m_vDirLight.m_vSpecular;

		// apply specular color, but again, just the one color component
		out_fSpecularMask *= RgbToLuminance(m_vSpecularColor);

		if (ST_EFFECT_SPECULAR_FADE)
			out_fSpecularMask *= v2p_fRenderEffectsFade;

		// specular curve
		out_fSpecularPower = m_fShininess;
	}
	
	// build material mask
	const float out_encodedGBuffMaterialMask = SpeedTreeBuildGBuffMaterialMask();

	// output syntax is platform-specific, encapsulated in an include
	#include "InlinePixel_DeferredOutput.fx"
}
