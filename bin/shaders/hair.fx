// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
// 

#include "commonPS.fx"
#ifdef VERTEXSHADER
#include "commonVS.fx"
#endif
#include "include_constants.fx"

// define this variable so that HairWorks shader functions are optmized
#define USE_PIXEL_SHADER_INTERPOLATION 1

// common hairworks shader header must be included
#include "GFSDK_HairWorks_ShaderCommon.h" 

//////////////////////////////////////////////////////////////////////////////
// buffers and textures
//////////////////////////////////////////////////////////////////////////////
Texture2D	g_rootHairColorTexture: register(t0); // texture map for hair root colors
Texture2D	g_tipHairColorTexture : register(t1); // texture map for hair tip colors
Texture2D	g_strandTexture : register(t2);	// texture map for per strand color

// additional resources for optimization
#if USE_PIXEL_SHADER_INTERPOLATION
GFSDK_HAIR_DECLARE_RESOURCES(t3, t4, t5);
#endif

//////////////////////////////////////////////////////////////////////////////
// constant buffer for render parameters
//////////////////////////////////////////////////////////////////////////////
START_CB(cbPerFrame, 10)
	// .... Add other cbuffer variables to get your own data

	GFSDK_Hair_ConstantBuffer	g_hairConstantBuffer; // hairworks portion of constant buffer data

	// .... Add other cbuffer variables to get your own data
END_CB

//////////////////////////////////////////////////////////////////////////////
// sampler states
//////////////////////////////////////////////////////////////////////////////
SamplerState samLinear; // expecting a linear texture sampler

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
};

struct PS_OUTPUT
{
	float4 color : SYS_TARGET_OUTPUT0;
};

#ifdef VERTEXSHADER
//////////////////////////////////////////////////////////////////////////////////////////////
// Dummy vertex shader
//////////////////////////////////////////////////////////////////////////////////////////////
VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;
	o.pos   = i.pos;
	return o;
}
#endif

#ifdef PIXELSHADER

////////////////////////////////////////////////////////////////////////////////////////////////
float3 applyFog(float3 worldSpacePosition, float3 color)
{
	float3 resultColor = ApplyFog( color, false, false, worldSpacePosition ).xyz;

	return resultColor;
}

////////////////////////////////////////////////////////////////////////////////////////////////
void calculateTiledLights(inout float3 diffuse, inout float3 specular, float3 worldSpacePosition, uint2 pixelCoord, GFSDK_Hair_ShaderAttributes attr, GFSDK_Hair_Material mat, GFSDK_Hair_ConstantBuffer hairConstantBuffer, float interiorFactor)
{
	float3 resultColor = 0;
	const uint2  tileIdx		= pixelCoord.xy / TILE_SIZE.xx;
    const uint   bufferIdx		= ((tileIdx.y * (int)numTiles.x) + tileIdx.x) * MAX_LIGHTS_PER_TILE;

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
		if ( attenuation > 0 )
		{
			float3 lightDir = normalize( L );
			float3 lightColor = attenuation * lights[lIdx].colorAndType.xyz;

			diffuse += GFSDK_Hair_ComputeHairDiffuseShading(lightColor, lightDir, attr, mat, hairConstantBuffer);
			specular +=  GFSDK_Hair_ComputeHairSpecularShading(lightColor, lightDir, attr, mat, hairConstantBuffer);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
void calculateEnvLights(float shadowFactor, inout float3 ambient, inout float3 diffuse, inout float3 specular, inout float3 reflection, float3 worldSpacePosition, uint2 pixelCoord, GFSDK_Hair_ShaderAttributes attr, GFSDK_Hair_Material mat, float interiorFactor)
{
	float3 envProbeAmbient  = float3 ( 0, 0, 0 );
	float3 envProbeSpecular = float3 ( 0, 0, 0 );

	const float roughness = 1.0f; //////////
	const float translucency = 0.0f; ////////////////

	float shadowFactorFinal = shadowFactor;
	{
		const float3 globalLightDir = normalize(lightDir.xyz);

		const float _dot = dot( globalLightDir.xyz, attr.N.xyz );
		const float subsurfFactor = 0.25; //< just assume some constant value since we can't really
		const float _extent = saturate( (_dot + subsurfFactor) / (1 + subsurfFactor) ); // we don't have vertex normal here so just use the pixelNormal
		const float nl = (saturate( _dot ) + _extent) / (1.0 + _extent);

		shadowFactorFinal = min( nl, shadowFactor );
	}
		
	float3 N = attr.N;
	float3 V = attr.V;
	{ 
		const float ambientMipIndex		= 0;
		const float envProbeRoughness	= roughness;
		const float3 envProbeNormal	= N;

		CalcEnvProbes_NormalBasedMipLod( envProbeAmbient, envProbeSpecular, worldSpacePosition, V, envProbeNormal, envProbeNormal, ambientMipIndex, envProbeRoughness, pixelCoord, true, interiorFactor );
		
		ApplyCharactersLightingBoostConstBased( shadowFactorFinal, envProbeAmbient, envProbeSpecular );
		envProbeAmbient *= mat.ambientEnvScale;

		const float envprobe_distance_scale		= CalcEnvProbesDistantBrightness( worldSpacePosition );

		const float  envprobe_diffuse_scale		= pbrSimpleParams0.z;
		const float3 envprobe_ambient_scale		= envprobe_distance_scale * lerp( pbrSimpleParams0.yyy, pbrSimpleParams0.xxx, shadowFactorFinal );
		const float  envprobe_specular_scale	= pbrSimpleParams0.w;
		const float3 envprobe_reflection_scale	= envprobe_distance_scale * lerp( pbrSimpleParams1.yyy, pbrSimpleParams1.xxx, shadowFactorFinal );			

		diffuse		*= envprobe_diffuse_scale;	
		specular	*= envprobe_specular_scale;

		const float  extraEnvProbeFresnelScale = mat.specularEnvScale;//1; // do not use mat.specularEnvScale;
		const float3 flatNormal = N;
		  
		//extraEnvProbeFresnelScale *= extraLightingParams.data0.w; //< apply gain mask

		const float3 specularity = mat.specularEnvScale * mat.specularColor.rgb;

		ambient += NormalizeAmbientPBRPipeline( envprobe_ambient_scale * envProbeAmbient, translucency, V, N, specularity, roughness, flatNormal, extraEnvProbeFresnelScale );
		reflection += envprobe_reflection_scale * envProbeSpecular * CalcFresnelPBRPipelineEnvProbeReflection( V, N, specularity, roughness, translucency, flatNormal, extraEnvProbeFresnelScale );
	}
} 

//////////////////////////////////////////////////////////////////////////////////////////////
// HairWorks version of PBR shading adapted from CalculateLightingPBRPipeline() in include_constants.fx
//////////////////////////////////////////////////////////////////////////////////////////////
float3 calculateLightingPBRPipeline( 
	GFSDK_Hair_ShaderAttributes attr, 
	GFSDK_Hair_Material mat, 
	GFSDK_Hair_ConstantBuffer hairConstantBuffer, 
	float3 albedo, float roughness, uint2 pixelCoord)
{
	float3 worldSpacePosition = attr.P;

	// get shadow factor 
	const float shadowFactor = (hairConstantBuffer.receiveShadows) ? CalcFullShadowFactor( worldSpacePosition, pixelCoord) : 1.0f;

	float litFactor = shadowFactor;

	// compute the glint factor
	float glint = GFSDK_Hair_ComputeHairGlint(attr, mat, hairConstantBuffer);

	// accumulated hair shading terms
	float3 ambient = 0;
	float3 diffuse = 0;
	float3 specular = 0;  
	float3 reflection = 0;
	 
	float dimmerFactor = 1;
	float interiorFactor = 1;	 
	{
		int qualityDegradation = 1;

		const float2 dimmerAndInteriorFactor = CalcDimmersFactorAndInteriorFactorTransparency( worldSpacePosition, pixelCoord );

		//shadowFactor = CalcGlobalShadow( N, worldSpacePosition, pixelCoord, qualityDegradation );
		dimmerFactor = dimmerAndInteriorFactor.x;
		interiorFactor = dimmerAndInteriorFactor.y;
	}

	// global lighting
	{
		float3 globalLightColor = litFactor * CalcGlobalLightColor( worldSpacePosition );
		float3 globalLightDir = normalize(lightDir.xyz);

		diffuse += GFSDK_Hair_ComputeHairDiffuseShading(globalLightColor, globalLightDir, attr, mat, hairConstantBuffer, glint);
		specular +=  GFSDK_Hair_ComputeHairSpecularShading(globalLightColor, globalLightDir, attr, mat, hairConstantBuffer, glint);
	}

	// tiled point lights
	{
		calculateTiledLights(diffuse, specular, worldSpacePosition, pixelCoord, attr, mat, hairConstantBuffer, interiorFactor);
	}

	// env lights
	{
		calculateEnvLights(shadowFactor, ambient, diffuse, specular, reflection, worldSpacePosition, pixelCoord, attr, mat, interiorFactor);
	}

	// AO
	{
		const float3 ssaoMod				= 1; // we don't want SSAO here (dimmers only)
		const float3 AO_probe				= GetProbesSSAOScaleByDimmerFactor( dimmerFactor ) * ssaoMod;
		const float3 AO_nonProbe			= ssaoParams.x * ssaoMod + ssaoParams.y;		

		ambient		*= AO_probe;
		reflection	*= AO_probe;
		diffuse		*= AO_nonProbe;
		specular	*= AO_nonProbe;		
	}

	// combine all colors
	float3 resultColor = albedo * (ambient + diffuse) + specular + reflection;

	// add fog
	resultColor = applyFog(worldSpacePosition, resultColor);

	// sometimes we get negative color?
	resultColor = max(0, resultColor);

	return resultColor;

}

//////////////////////////////////////////////////////////////////////////////////////////////
// Pixel shader for hair rendering
//////////////////////////////////////////////////////////////////////////////////////////////
[EARLY_DEPTH_STENCIL] void ps_main(GFSDK_Hair_PixelShaderInput input, out PS_OUTPUT o )
{   
	// use full alpha
	o.color.a = 1.0f;	 

	// collect hair resources.
	_GFSDK_HAIR_USE_RESOURCES_; 

	// get all the attibutes needed for hair shading with this function
	GFSDK_Hair_ShaderAttributes attr = GFSDK_Hair_GetShaderAttributes(input, g_hairConstantBuffer);  
  
	// set up hair material.
	GFSDK_Hair_Material mat = g_hairConstantBuffer.hairMaterial;

	// sample hair color from textures
	float3 hairColor = GFSDK_Hair_SampleHairColorStrandTex(
		g_hairConstantBuffer, mat, samLinear, g_rootHairColorTexture, g_tipHairColorTexture, g_strandTexture, attr.texcoords);

	// add noise to hair color
	float noise = GFSDK_Hair_ComputeColorNoise(attr, mat, g_hairConstantBuffer);	
	hairColor.xyz *= lerp(1.0f, noise, saturate(mat.diffuseNoiseScale));

	// apply gamma to sampled hair color to get linear albedo
	float3 albedo = pow(hairColor.xyz, GAMMA_TO_LINEAR_EXPONENT);
	
	// add noise to specular
	mat.specularColor.xyz *= lerp(1.0f, noise, saturate(mat.specularNoiseScale)); // add noise to specular color

	// apply gamma to specular color as well
	mat.specularColor.xyz = pow(mat.specularColor.xyz, GAMMA_TO_LINEAR_EXPONENT);

	//	compute fragment color using PBR hair shader 
	o.color.rgb = calculateLightingPBRPipeline( attr, mat, g_hairConstantBuffer, albedo, noise, input.position.xy );
	o.color.a = GFSDK_Hair_ComputeAlpha(g_hairConstantBuffer, mat, attr);
}
#endif // PIXELSHADER
 