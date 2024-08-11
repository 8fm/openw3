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

// LAVA++
#include "../../include/common.fx"
#include "../../include/globalConstantsPS.fx"
#include "../../include/include_utilities.fx"
#include "../../include/include_sharedConsts.fx"
// LAVA--

///////////////////////////////////////////////////////////////////////
//	Pixel shader entry point
//
//	Main depth-only pixel shader for 3D geometry

$PIXEL_MAIN_FUNCTION_DECL$
{
	#if (ST_ONLY_BRANCHES_PRESENT)
	{
		// all possible variables that may come from the vertex shader; the "v2p" prefix indicates that these
		// variables are values that go from [v]ertex-[2]-[p]ixel shader
		float2 v2p_vDiffuseTexCoords = float2(0.0, 0.0);
		float  v2p_fFadeToBillboard = 1.0;

		// set initial values for those v2p parameters that are in use
		$PIXEL_GET_PARAMS_FROM_INPUT_ATTRIBS$

		// branch geometry almost always has opaque textures if which case we use an
		// empty/null pixel shader for shadow casting
		const int2 shSpaceCrd = (int2)v2p_vUserInterpolant0.xy; //< using userInterpolant instead of vpos because for some reason it doesn't introduce any visibile overhead (vpos had 0.4ms hit on ps4, didn't measure the hit on durango, but verified that the cost is the same this way)
		if ( v2p_fFadeToBillboard.x < CalcDissolvePattern( shSpaceCrd, 2 ) )
		{
			discard;
		}
	}	
	#else
		// all possible variables that may come from the vertex shader; the "v2p" prefix indicates that these
		// variables are values that go from [v]ertex-[2]-[p]ixel shader
		float2 v2p_vDiffuseTexCoords = float2(0.0, 0.0);
		float  v2p_fFadeToBillboard = 1.0;

		// set initial values for those v2p parameters that are in use
		$PIXEL_GET_PARAMS_FROM_INPUT_ATTRIBS$

		// diffuse texture lookup
		float fAlpha = SampleTexture(DiffuseMap, v2p_vDiffuseTexCoords).a;

		// scale alpha by material alpha scalar set in Modeler
		{
			// Original solution, doesn't look too good with some alphas..
			// But we use it in scenes/cutscene in order to prevent dissolve based checkerboard on character faces etc.
			const float erodedAlpha = fAlpha * v2p_fFadeToBillboard;
			
			// Calculate dissolveAlpha
			float dissolvedAlpha = 0;
			{
				const int2 shSpaceCrd = (int2)v2p_vUserInterpolant0.xy; //< using userInterpolant instead of vpos because for some reason it doesn't introduce any visibile overhead (vpos had 0.4ms hit on ps4, didn't measure the hit on durango, but verified that the cost is the same this way)
				float patternValue = CalcDissolvePattern( shSpaceCrd, 2 );
				dissolvedAlpha = fAlpha - (v2p_fFadeToBillboard < patternValue ? 2 : 0);
			}
			
			// Calculate final alpha
			fAlpha = lerp( erodedAlpha, dissolvedAlpha, cascadesSize.z );
		}

		// attempt to get earliest possible exit if pixel is transparent
		clip(fAlpha - ST_ALPHA_KILL_THRESHOLD);
	#endif
}

