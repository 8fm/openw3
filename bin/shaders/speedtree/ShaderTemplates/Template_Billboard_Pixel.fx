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

#include "Include_SetUp.fx"
#include "Include_Uniforms.fx"
#include "Include_SamplersAndTextureMacros.fx"
#include "Include_Utility.fx"
#include "Include_TreeTextures.fx"
#include "Include_TextureUtility.fx"
#include "Include_FogAndSky.fx"
#include "Include_UserInterpolants.fx"


///////////////////////////////////////////////////////////////////////
//	Pixel shader entry point
//
//	Main lighting/wind forward vertex shader for billboard geometry

#if (!ST_OPENGL)

	float4 main(float4 v2p_vInterpolant0			: ST_VS_OUTPUT, // projection
				float3 v2p_vInterpolant1			: TEXCOORD0,	// texcoords, alpha scalar
				float4 v2p_vInterpolant2			: TEXCOORD1,	// tangent-space normal, fog scalar
				float3 v2p_vBinormalInterpolant		: TEXCOORD2,	// world-space binormal
				float3 v2p_vHueVariationInterpolant	: TEXCOORD3		// hue variation

				// optional interpolants, based on user config
				#if (ST_FOG_COLOR == ST_FOG_COLOR_DYNAMIC)
					, float3 v2p_vFogInterpolant			: TEXCOORD4 // dynamic fog color
				#endif
				) : ST_PS_OUTPUT

#else // !ST_OPENGL

	// v2p_vInterpolant0 / projection is implicit
	varying float3 v2p_vInterpolant1; // texcoords, alpha scalar
	varying float4 v2p_vInterpolant2; // tangent-space normal, fog scalar

	// optional interpolants, based on user config
	#if (ST_FOG_COLOR == ST_FOG_COLOR_DYNAMIC)
		varying float3 v2p_vFogInterpolant; // dynamic fog color
	#endif
	#if (ST_EFFECT_AMBIENT_IMAGE_LIGHTING)
		varying float3 v2p_vBinormalInterpolant; // world-space binormal
	#endif
	#if (ST_EFFECT_HUE_VARIATION)
		varying float3 v2p_vHueVariationInterpolant; // hue variation
	#endif

	void main(void)

#endif // !ST_OPENGL

{
	float4 vPixelShaderReturn = float4(0.0, 0.0, 0.0, 1.0);

	// input attributes via interpolants
	float2 v2p_vTexCoords = v2p_vInterpolant1.xy;
	float  v2p_fAlphaScalar = v2p_vInterpolant1.z;
	float3 v2p_vTangentSpaceNormal = v2p_vInterpolant2.xyz;
	float  v2p_fFogScalar = v2p_vInterpolant2.w;
	float3 v2p_vWorldSpaceBinormal = v2p_vBinormalInterpolant.xyz;
	float3 v2p_vHueVariation = v2p_vHueVariationInterpolant.xyz;

	// optional input attributes
	float3 v2p_vFogColor = m_vFogColor;
	#if (ST_FOG_COLOR == ST_FOG_COLOR_DYNAMIC)
		v2p_vFogColor = v2p_vFogInterpolant.xyz;
	#endif

	// diffuse texture lookup
	//
	// .rgb = written by the Compiler app to be (diffuse_map * diffuse_light_scalar * ambient_occlusion)
	//   .a = alpha mask
	float4 texDiffuseTexture = SampleTexture(DiffuseMap, v2p_vTexCoords);

	// test for possible early exit condition
	vPixelShaderReturn.a = texDiffuseTexture.a * v2p_fAlphaScalar;
	if (ST_ALPHA_TEST_NOISE)
	{
		float fDelta = (m_fAlphaScalar - v2p_fAlphaScalar) / m_fAlphaScalar;
		vPixelShaderReturn.a -= AlphaTestNoise_Billboard(v2p_vTexCoords) * fDelta;
	}
	CheckForEarlyExit(vPixelShaderReturn.a, ST_TRANSPARENCY_ACTIVE);

	// hue variation
	texDiffuseTexture.rgb += v2p_vHueVariation;

	// normal texture lookup
	//
	// .rgb = normal map
	//   .a = ambient hint
	float4 texNormalTexture = SampleTexture(NormalMap, v2p_vTexCoords);

	// lighting computation
	v2p_vTangentSpaceNormal = normalize(v2p_vTangentSpaceNormal);
	float3 vNormal = DecodeVectorFromColor(texNormalTexture).rgb;
	float fDot = dot(vNormal, v2p_vTangentSpaceNormal);

	// ambient contribution
	float3 vAmbientComponent = texNormalTexture.a * m_vAmbientColor;

	// modify simple ambient lighting with ambient image lookup based on normal
	if (ST_EFFECT_AMBIENT_IMAGE_LIGHTING)
	{
		// orient normal map normal back to world space
		float3 vWorldSpaceNormal = -m_vCameraDirection;
		float3 vWorldSpaceTangent = normalize(cross(v2p_vWorldSpaceBinormal, vWorldSpaceNormal)); // todo: normalize necessary?
		float3 vDerivedWorldSpaceNormal = vNormal.x * vWorldSpaceTangent +
										  vNormal.y	* v2p_vWorldSpaceBinormal +
										  vNormal.z	* vWorldSpaceNormal;

		// compute uv lookup into image map
		float2 vImageMapUV = float2(vDerivedWorldSpaceNormal.x * 0.5 + vDerivedWorldSpaceNormal.y * 0.5,
									1.0 - (vDerivedWorldSpaceNormal.z + 1.0) * 0.5);

		float4 texImageBasedLighting = SampleTexture(ImageBasedAmbientLighting, vImageMapUV);

		vAmbientComponent *= texImageBasedLighting.rgb * m_fAmbientImageScalar;
	}

	// combine diffuse and ambient components
	vPixelShaderReturn.rgb = DotProductLighting(vAmbientComponent, m_vDiffuseColor, fDot) * texDiffuseTexture.rgb;

	// fog
	if (ST_FOG_CURVE != ST_FOG_CURVE_DISABLED)
		vPixelShaderReturn.rgb = lerp(v2p_vFogColor, vPixelShaderReturn.rgb, v2p_fFogScalar);

	ST_PIXEL_COLOR_RETURN(vPixelShaderReturn);
}
