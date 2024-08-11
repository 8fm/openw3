///////////////////////////////////////////////////////////////////////
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

#ifndef ST_INCLUDE_SHADOW_UTILITY
#define ST_INCLUDE_SHADOW_UTILITY


///////////////////////////////////////////////////////////////////////  
//  SampleShadowMap
//
//	Look up a single shadow map value. Expecting PCF to be enabled. Can
//	be a single lookup or average of four, depending on bSmooth.

#if (ST_DIRECTX11) || (ST_PS4)
	float SampleShadowMap(Texture2D tShadowMap, SamplerComparisonState samShadowMap, float4 vPosInLightSpace, bool bSmooth)
#elif (ST_OPENGL)
	// first parameter isn't used/needed by GLSL; there are only samplers, not textures & samplers
	float SampleShadowMap(int tShadowMap, sampler2DShadow samShadowMap, float4 vPosInLightSpace, bool bSmooth)
#else
	float SampleShadowMap(texture tShadowMap, sampler2D samShadowMap, float4 vPosInLightSpace, bool bSmooth)
#endif
{
	#if (!ST_XBOX_360)
		float fSample;

		if (bSmooth)
		{
			// average four PCF lookups
			float4 vSum0;
			vSum0.x = SampleTextureCompare(tShadowMap, samShadowMap, vPosInLightSpace);
			vSum0.y = SampleTextureCompare(tShadowMap, samShadowMap, vPosInLightSpace + m_sShadows.m_avShadowSmoothingTable[0]);
			vSum0.z = SampleTextureCompare(tShadowMap, samShadowMap, vPosInLightSpace + m_sShadows.m_avShadowSmoothingTable[1]);
			vSum0.w = SampleTextureCompare(tShadowMap, samShadowMap, vPosInLightSpace + m_sShadows.m_avShadowSmoothingTable[2]);

			fSample = dot(vSum0, float4(0.25, 0.25, 0.25, 0.25));
		}
		else
			// single PCF lookup
			fSample = SampleTextureCompare(tShadowMap, samShadowMap, vPosInLightSpace);

		return (ST_PS3 ? (1.0 - fSample) : fSample);

	#else // !ST_XBOX_360

		// use a 4-tap bilinear lookup

		// note: bSmoothingFilter is ignored on 360 version
		float4 vWeights;
		float fLOD;
		float4 vSampledDepth;
		asm 
		{
			tfetch2D vSampledDepth.x___, vPosInLightSpace.xy, samShadowMap, OffsetX = -0.5, OffsetY = -0.5
			tfetch2D vSampledDepth._x__, vPosInLightSpace.xy, samShadowMap, OffsetX =  0.5, OffsetY = -0.5
			tfetch2D vSampledDepth.__x_, vPosInLightSpace.xy, samShadowMap, OffsetX = -0.5, OffsetY =  0.5
			tfetch2D vSampledDepth.___x, vPosInLightSpace.xy, samShadowMap, OffsetX =  0.5, OffsetY =  0.5
		
			getWeights2D vWeights, vPosInLightSpace.xy, samShadowMap, MagFilter=linear, MinFilter=linear
		};

		vWeights = float4((1.0 - vWeights.x) * (1.0 - vWeights.y), vWeights.x * (1.0 - vWeights.y), (1.0 - vWeights.x) * vWeights.y, vWeights.x * vWeights.y);
		float4 vAttenuation = step(vPosInLightSpace.z, vSampledDepth);
		
		return dot(vAttenuation, vWeights);

	#endif // !ST_XBOX_360
}


///////////////////////////////////////////////////////////////////////  
//  ShadowMapLookup
//
//	Given fDepth (position_projection.z / far_clip), this function will
//	choose the correct cascaded shadow map and do the shadow lookup for
//	that map. ST_SHADOWS_NUM_MAPS may be from 1 to 4. The code is written so
//	that the shader compiler optimizer should  remove comparisons not
//	needed for unused shadow maps.
//
//	It currently does not blend between cascades; it's a hard transition
//	between cascades.

float ShadowMapLookup(float4 vPosInLightSpace0, 
					  float4 vPosInLightSpace1,
					  float4 vPosInLightSpace2,
					  float4 vPosInLightSpace3,
					  float fDepth, 
					  bool bSmooth)
{
	float fShadow = 1.0;

	#if (ST_OPENGL)

		bvec4 vGreater = greaterThan(float4(fDepth, fDepth, fDepth, fDepth), m_sShadows.m_vShadowMapRanges.xyzw);

		if (ST_SHADOWS_NUM_MAPS > 3 && vGreater.z)
			fShadow = SampleShadowMap(3, ShadowMap3, vPosInLightSpace3, bSmooth);
		else if (ST_SHADOWS_NUM_MAPS > 2 && vGreater.y)
			fShadow = SampleShadowMap(2, ShadowMap2, vPosInLightSpace2, bSmooth);
		else if (ST_SHADOWS_NUM_MAPS > 1 && vGreater.x)
			fShadow = SampleShadowMap(1, ShadowMap1, vPosInLightSpace1, bSmooth);
		else if (ST_SHADOWS_NUM_MAPS > 0)
			fShadow = SampleShadowMap(0, ShadowMap0, vPosInLightSpace0, bSmooth);

	#elif (ST_DIRECTX9) || (ST_PS3)

		float4 vGreater = (fDepth.xxxx > m_sShadows.m_vShadowMapRanges.xyzw);

		if (ST_SHADOWS_NUM_MAPS > 3 && vGreater.z > 0.0)
			fShadow = SampleShadowMap(ShadowMap3, samShadowMap3, vPosInLightSpace3, bSmooth);
		else if (ST_SHADOWS_NUM_MAPS > 2 && vGreater.y > 0.0)
			fShadow = SampleShadowMap(ShadowMap2, samShadowMap2, vPosInLightSpace2, bSmooth);
		else if (ST_SHADOWS_NUM_MAPS > 1 && vGreater.x > 0.0)
			fShadow = SampleShadowMap(ShadowMap1, samShadowMap1, vPosInLightSpace1, bSmooth);
		else if (ST_SHADOWS_NUM_MAPS > 0)
			fShadow = SampleShadowMap(ShadowMap0, samShadowMap0, vPosInLightSpace0, bSmooth);

	#else

		float4 vGreater = (fDepth.xxxx > m_sShadows.m_vShadowMapRanges.xyzw);

		if (ST_SHADOWS_NUM_MAPS > 3 && vGreater.z > 0.0)
			fShadow = SampleShadowMap(ShadowMap3, samShadowMapComparison, vPosInLightSpace3, bSmooth);
		else if (ST_SHADOWS_NUM_MAPS > 2 && vGreater.y > 0.0)
			fShadow = SampleShadowMap(ShadowMap2, samShadowMapComparison, vPosInLightSpace2, bSmooth);
		else if (ST_SHADOWS_NUM_MAPS > 1 && vGreater.x > 0.0)
			fShadow = SampleShadowMap(ShadowMap1, samShadowMapComparison, vPosInLightSpace1, bSmooth);
		else if (ST_SHADOWS_NUM_MAPS > 0)
			fShadow = SampleShadowMap(ShadowMap0, samShadowMapComparison, vPosInLightSpace0, bSmooth);

	#endif

	return FadeShadowByDepth(fShadow, fDepth, m_sShadows.m_fFadeStartPercent, m_sShadows.m_fFadeInverseDistance);
}

#endif // ST_INCLUDE_SHADOW_UTILITY
