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
#include "Include_UserInterpolants.fx"


///////////////////////////////////////////////////////////////////////
//	Pixel shader entry point
//
//	Main depth-only pixel shader for 3D geometry

$PIXEL_MAIN_FUNCTION_DECL$
{
	float4 vPixelShaderReturn = float4(0.0, 0.0, 0.0, 1);

	// all possible variables that may come from the vertex shader; the "v2p" prefix indicates that these
	// variables are values that go from [v]ertex-[2]-[p]ixel shader
	float2 v2p_vDiffuseTexCoords = float2(0.0, 0.0);
	float  v2p_fFadeToBillboard = 1.0;

	// set initial values for those v2p parameters that are in use
	$PIXEL_GET_PARAMS_FROM_INPUT_ATTRIBS$

	// diffuse texture lookup
	float4 texDiffuse = ST_EFFECT_DIFFUSE_MAP_OPAQUE ? float4(1.0, 1.0, 1.0, 1.0) : SampleTexture(DiffuseMap, v2p_vDiffuseTexCoords);

	// adjust alpha value if grass or fading to billboard (alpha "fizzle" with noise)
	if (ST_EFFECT_FADE_TO_BILLBOARD || ST_USED_AS_GRASS)
	{
		vPixelShaderReturn.a = texDiffuse.a * v2p_fFadeToBillboard;

		if (ST_ALPHA_TEST_NOISE)
		{
			float fDelta = (m_fAlphaScalar - v2p_fFadeToBillboard) / m_fAlphaScalar;
			vPixelShaderReturn.a -= AlphaTestNoise_3dTree(v2p_vDiffuseTexCoords) * fDelta;
		}
	}
	else
		vPixelShaderReturn.a = texDiffuse.a * m_fAlphaScalar;

	// attempt to get earliest possible exit if pixel is transparent
	CheckForEarlyExit(vPixelShaderReturn.a, ST_TRANSPARENCY_ACTIVE);
}

