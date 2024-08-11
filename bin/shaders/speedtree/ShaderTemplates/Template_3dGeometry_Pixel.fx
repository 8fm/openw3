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
#include "Include_ShadowTextures.fx"
#include "Include_ShadowUtility.fx"


// LAVA+
#include "../../include/common.fx"
#include "../../include/globalConstantsPS.fx"
#include "../../include/include_utilities.fx"
#include "../../include/include_sharedConsts.fx"
#include "../../include/include_envProbe.fx"
#if (ST_USED_AS_GRASS)
	#include "../../include/include_pigment.fx"
#endif
// LAVA-

///////////////////////////////////////////////////////////////////////
//	Pixel shader entry point
//
//	Main lighting/wind forward pixel shader for 3D geometry

$PIXEL_MAIN_FUNCTION_DECL$
{
	float4 vPixelShaderReturn = float4(0.0, 0.0, 0.0, 1.0);

	// all possible variables that may come from the vertex shader; the "v2p" prefix indicates that these
	// variables are values that go from [v]ertex-[2]-[p]ixel shader
	float  v2p_fFogScalar = 0.0;
	float3 v2p_vFogColor = float3(0.0, 0.0, 0.0);
	float2 v2p_vDiffuseTexCoords = float2(0.0, 0.0);
	float3 v2p_vPerVertexLightingColor = float3(0.0, 0.0, 0.0);
	float3 v2p_vNormalInTangentSpace = float3(0.0, 0.0, 0.0);
	float  v2p_fPerVertexAmbientContrast = 0.0;
	float  v2p_fRenderEffectsFade = 0.0;
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

	float3 vCombinedAmbientColor = m_vAmbientColor * m_vDirLight.m_vAmbient;

	// diffuse texture lookup
	float4 texDiffuse = SampleTexture(DiffuseMap, v2p_vDiffuseTexCoords);

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

	// adjust diffuse texture with hue variation
	if (ST_EFFECT_HUE_VARIATION)
		texDiffuse.rgb += v2p_vHueVariation;

	// image-based ambient lighting
	if (ST_EFFECT_AMBIENT_IMAGE_LIGHTING)
	{
		// compute uv lookup into image map
		const float c_fImageMapU = v2p_vNormal.x * 0.5 + v2p_vNormal.y * 0.5;
		const float c_fImageMapV = (v2p_vNormal.z + 1.0) * 0.5;

		float4 texImageBasedLighting = SampleTexture(ImageBasedAmbientLighting, float2(c_fImageMapU, 1.0 - c_fImageMapV));

		// modify ambient color to reflect more complex image-based ambient value
		vCombinedAmbientColor.rgb *= texImageBasedLighting.rgb * m_fAmbientImageScalar;
	}

	// cascaded shadow map contribution (up to four, configured during compile-time)
	float fShadow = 1.0;
	if (ST_SHADOWS_ENABLED)
		fShadow = ShadowMapLookup(v2p_vShadowMapProjection0, v2p_vShadowMapProjection1, v2p_vShadowMapProjection2, v2p_vShadowMapProjection3, v2p_fShadowDepth, ST_SHADOWS_SMOOTH);

	// detail layer
	float4 texDetailDiffuse = float4(0.0, 0.0, 0.0, 1.0);
	if (ST_EFFECT_DETAIL_LAYER)
	{
		texDetailDiffuse = SampleTexture(DetailDiffuseMap, v2p_vDetailTexCoords);

		if (ST_EFFECT_DETAIL_LAYER_FADE)
			texDetailDiffuse.a *= v2p_fRenderEffectsFade;

		texDiffuse.rgb = lerp(texDiffuse.rgb, texDetailDiffuse.rgb, texDetailDiffuse.a);
	}

	// normal map
	float3 vNormalMapNormal = float3(0.5, 0.5, 0.5);
	if (ST_EFFECT_FORWARD_LIGHTING_PER_PIXEL)
		vNormalMapNormal = normalize(DecodeVectorFromColor(SampleTexture(NormalMap, v2p_vDiffuseTexCoords)).rgb);

	// per-vertex lighting (if enabled -- this shader will support per-vertex or per-pixel lighting (both forward) depending on config)
	float3 vFinalPerVertexColor = v2p_vPerVertexLightingColor;
	if (ST_EFFECT_FORWARD_LIGHTING_PER_VERTEX)
	{
		if (ST_SHADOWS_ENABLED)
		{
			// float3 v2p_vPerVertexLightingColor holds three important values
			const float c_fPerVertexAmbient = v2p_vPerVertexLightingColor.x;
			const float c_fPerVertexDot = v2p_vPerVertexLightingColor.y;
			const float c_fPerVertexAmbientOcclusion = v2p_vPerVertexLightingColor.z;

			vFinalPerVertexColor = DotProductLighting(vCombinedAmbientColor * c_fPerVertexAmbient, m_vDiffuseColor, c_fPerVertexDot * fShadow);
			vFinalPerVertexColor *= c_fPerVertexAmbientOcclusion;
		}

		// just pass through vertex shader value
		vPixelShaderReturn.rgb = vFinalPerVertexColor;
	}

	// detail normal map
	if (ST_EFFECT_DETAIL_NORMAL_LAYER)
	{
		// decode normal from detail normal map
		float3 vDetailMapNormal = DecodeVectorFromColor(SampleTexture(DetailNormalMap, v2p_vDetailTexCoords)).rgb;

		// the alpha component determines how much the detail overrides base diffuse; also can fade off per LOD
		float fDetailNormalMapScalar = texDetailDiffuse.a * (ST_EFFECT_DETAIL_LAYER_FADE ? v2p_fRenderEffectsFade : 1.0);

		// final normal is combination of base normal and detail normal
		vNormalMapNormal = lerp(vNormalMapNormal, vDetailMapNormal, fDetailNormalMapScalar);
	}

	// branch seam smoothing (may ultimately involve four texture lookups, depending on diffuse, normal, detail, and detail_normal)
	if (ST_EFFECT_BRANCH_SEAM_SMOOTHING)
	{
		// blend factor
		const float2 c_fSeamDiffuseTexcoords = v2p_vBranchSeamDiffuse.xy;
		const float2 c_fSeamDetailTexcoords = v2p_vBranchSeamDetail.xy;
		const float c_fSeamBlendWeight = v2p_vBranchSeamDiffuse.z;

		float fSeamBlend = min(pow(c_fSeamBlendWeight, m_fBranchSeamWeight), 1.0);

		if (ST_EFFECT_BRANCH_SEAM_SMOOTHING_FADE)
			fSeamBlend = lerp(1.0, fSeamBlend, v2p_fRenderEffectsFade);

		// diffuse (always done for branch smoothing)
		float4 texDiffuse2 = SampleTexture(DiffuseMap, c_fSeamDiffuseTexcoords);

		// detail layer may modify second diffuse lookup
		float4 texDetailDiffuse2 = float4(0.0, 0.0, 0.0, 0.0);
		if (ST_EFFECT_DETAIL_LAYER)
		{
			texDetailDiffuse2 = SampleTexture(DetailDiffuseMap, c_fSeamDetailTexcoords);

			if (ST_EFFECT_DETAIL_LAYER_FADE)
				texDetailDiffuse2.a *= v2p_fRenderEffectsFade;

			texDiffuse2.rgb = lerp(texDiffuse2.rgb, texDetailDiffuse2.rgb, texDetailDiffuse2.a);
		}
		texDiffuse = lerp(texDiffuse2, texDiffuse, fSeamBlend);

		// normal
		if (ST_EFFECT_FORWARD_LIGHTING_PER_PIXEL)
		{
			float3 vNormalMapNormal2 = DecodeVectorFromColor(SampleTexture(NormalMap, c_fSeamDiffuseTexcoords)).rgb;

			// detail
			if (ST_EFFECT_DETAIL_LAYER)
			{
				float3 vDetailMapNormal2 = DecodeVectorFromColor(SampleTexture(DetailNormalMap, c_fSeamDetailTexcoords)).rgb;

				#define c_fSeamDetailNormalScalar texDetailDiffuse2.a
				vNormalMapNormal2 = lerp(vNormalMapNormal2, vDetailMapNormal2, c_fSeamDetailNormalScalar);
			}

			vNormalMapNormal = lerp(vNormalMapNormal2, vNormalMapNormal, fSeamBlend);
		}
	}

	// per-pixel lighting
	if (ST_EFFECT_FORWARD_LIGHTING_PER_PIXEL)
	{
		v2p_vNormalInTangentSpace = normalize(v2p_vNormalInTangentSpace);
		float fDot = dot(vNormalMapNormal, v2p_vNormalInTangentSpace);

		// ambient component
		float3 vAmbientComponent = vCombinedAmbientColor;
		if (ST_EFFECT_AMBIENT_CONTRAST)
		{
			float fPerPixelAmbientContrast = AmbientContrast(m_fOneMinusAmbientContrastFactor, fDot);

			if (ST_EFFECT_FORWARD_LIGHTING_TRANSITION)
				// skew per-pixel ambient to the per-vertex value if transitioning between the two
				fPerPixelAmbientContrast = lerp(v2p_fPerVertexAmbientContrast, fPerPixelAmbientContrast, v2p_fRenderEffectsFade);

			vAmbientComponent *= fPerPixelAmbientContrast;
		}

		// combine diffuse and ambient components
		vPixelShaderReturn.rgb = DotProductLighting(vAmbientComponent, m_vDiffuseColor, fShadow * fDot);

		if (ST_EFFECT_AMBIENT_OCCLUSION)
			vPixelShaderReturn.rgb *= v2p_fAmbientOcclusion;

		if (ST_EFFECT_FORWARD_LIGHTING_TRANSITION)
			// todo: which?
			//vPixelShaderReturn.rgb = lerp(v2p_vPerVertexLightingColor, vPixelShaderReturn.rgb, v2p_fRenderEffectsFade);
			vPixelShaderReturn.rgb = lerp(vFinalPerVertexColor, vPixelShaderReturn.rgb, v2p_fRenderEffectsFade);
	}

	// transmission
	if (ST_EFFECT_TRANSMISSION)
	{
		// transmission mask
		float3 vTransmissionMask = SampleTexture(SpecularMaskMap, v2p_vDiffuseTexCoords).aaa;

		// compute unattenuated transmission component
		float3 vTransmissionComponent = m_vTransmissionColor * vTransmissionMask;

		// when fade is active, the transmission component is simply attenuated
		if (ST_EFFECT_TRANSMISSION_FADE)
			v2p_fTransmissionFactor *= v2p_fRenderEffectsFade;

		// add transmission mask & component to output color
		vPixelShaderReturn.rgb += vTransmissionComponent * v2p_fTransmissionFactor * saturate(fShadow + m_fTransmissionShadowBrightness);
	}

	// todo: should this go before or after transmission? check Modeler
	// diffuse texture map contribution
	vPixelShaderReturn.rgb *= texDiffuse.rgb;

	// specular contribution
	if (ST_EFFECT_SPECULAR)
	{
		// set default mask and specular component values
		float3 vSpecularMask = m_vSpecularColor;

		float3 vSpecularComponent = float3(0.0, 0.0, 0.0); // no specular effect is default

		// specular mask
		vSpecularMask *= SampleTexture(SpecularMaskMap, v2p_vDiffuseTexCoords).rgb;

		// specular for per-pixel
		if (ST_EFFECT_FORWARD_LIGHTING_PER_PIXEL)
		{
			// *per-pixel mode*
			//
			// if per-pixel is active, we compute the half vector in the vertex shader so it won't have to be done here.
			v2p_vSpecularHalfVector = normalize(v2p_vSpecularHalfVector);
			float fSpecularDot = saturate(dot(v2p_vSpecularHalfVector, vNormalMapNormal));

			float fPow = pow(fSpecularDot, m_fShininess);
			vSpecularComponent = float3(fPow, fPow, fPow);

			// *per-pixel transitioning to per-vertex*
			//
			// in this mode, lerp between specular intensity provided by vs and per-pixel color computed above
			if (ST_EFFECT_FORWARD_LIGHTING_TRANSITION)
				vSpecularComponent = lerp(float3(v2p_fPerVertexSpecularDot, v2p_fPerVertexSpecularDot, v2p_fPerVertexSpecularDot), vSpecularComponent, v2p_fRenderEffectsFade);

			// when fade is active, the specular component is simply attenuated
			if (ST_EFFECT_SPECULAR_FADE)
				vSpecularComponent *= v2p_fRenderEffectsFade;
		}
		// specular for per-vertex
		else
		{
			// *per-vertex mode*
			//
			// if per-vertex is active, the specular intensity is computed in the vs and passed into
			// the ps where it's used directly
			vSpecularComponent = float3(v2p_fPerVertexSpecularDot, v2p_fPerVertexSpecularDot, v2p_fPerVertexSpecularDot);
		}

		// add specular mask & component to output color
		vPixelShaderReturn.rgb += fShadow * vSpecularComponent * vSpecularMask;
	}

	// fogging
	if (ST_FOG_COLOR == ST_FOG_COLOR_CONSTANT)
		vPixelShaderReturn.rgb = lerp(m_vFogColor, vPixelShaderReturn.rgb, v2p_fFogScalar);
	else if (ST_FOG_COLOR == ST_FOG_COLOR_DYNAMIC)
		vPixelShaderReturn.rgb = lerp(v2p_vFogColor, vPixelShaderReturn.rgb, v2p_fFogScalar);

	ST_PIXEL_COLOR_RETURN(vPixelShaderReturn);
}

