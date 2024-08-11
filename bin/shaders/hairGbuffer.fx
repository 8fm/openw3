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
#include "inc_bestfitnormals.fx"

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

struct PS_OUTPUT
{
	float4 color 					: SYS_TARGET_OUTPUT0;
	float4 normal					: SYS_TARGET_OUTPUT1;
	float4 specular					: SYS_TARGET_OUTPUT2;
};

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
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
//////////////////////////////////////////////////////////////////////////////////////////////
// Pixel shader for hair rendering
//////////////////////////////////////////////////////////////////////////////////////////////
[EARLY_DEPTH_STENCIL] PS_OUTPUT ps_main(GFSDK_Hair_PixelShaderInput input)
{   
	// collect hair resources.
	_GFSDK_HAIR_USE_RESOURCES_; 

	// get all the attibutes needed for hair shading with this function
	GFSDK_Hair_ShaderAttributes attr = GFSDK_Hair_GetShaderAttributes(input, g_hairConstantBuffer);
  
	// set up hair material.
	GFSDK_Hair_Material mat = g_hairConstantBuffer.hairMaterial;

	// sample hair color from textures
	float3 hairColor = GFSDK_Hair_SampleHairColorStrandTex(
		g_hairConstantBuffer, mat, samLinear, g_rootHairColorTexture, g_tipHairColorTexture, g_strandTexture, attr.texcoords);

	PS_OUTPUT o = (PS_OUTPUT)0;

	float translucencyFactor = 0.0;
	float3 specularity = mat.specularPrimaryScale;
	float3 normal = attr.N;
	float glossinessFactor = .5;

	float3 packed_normal = CompressNormalsToUnsignedGBuffer( attr.N );
	
	o.color = float4( hairColor.xyz, translucencyFactor );
	o.normal = float4( packed_normal.xyz, glossinessFactor );
	o.specular = float4( specularity.xyz, 0.f );

	return o;
}
#endif // PIXELSHADER