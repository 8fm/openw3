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
//	Main lighting/wind vertex shader for billboard geometry

#if (!ST_OPENGL)

	void main(
			  float4 v2p_vInterpolant0 : ST_VS_OUTPUT, // projection
			  float3 v2p_vInterpolant1 : TEXCOORD0  // texcoords, alpha scalar
			 )

#else // !ST_OPENGL

	// v2p_vInterpolant0 / projection is implicit
	varying float3 v2p_vInterpolant1; // texcoords, alpha scalar

	void main(void)

#endif // !ST_OPENGL

{
	float4 vPixelShaderReturn = float4(0.0, 0.0, 0.0, 1.0);

	// input attributes via interpolants
	float2 v2p_vTexCoords = v2p_vInterpolant1.xy;
	float  v2p_fAlphaScalar = v2p_vInterpolant1.z;

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

	// attempt to get earliest possible exit if pixel is transparent
	CheckForEarlyExit(vPixelShaderReturn.a, ST_TRANSPARENCY_ACTIVE);
}
