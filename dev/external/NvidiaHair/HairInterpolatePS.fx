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
// Copyright © 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

//////////////////////////////////////////////////////////////////////////////
// buffers and textures
//////////////////////////////////////////////////////////////////////////////
Texture2D	g_rootHairColorTexture: register(t0); // texture map for hair root colors
Texture2D	g_tipColorTexture : register(t1);	// texture map for hair tip colors

//////////////////////////////////////////////////////////////////////////////
// constant buffer for render parameters
//////////////////////////////////////////////////////////////////////////////
cbuffer cbPerFrame : register(b0)
{
	float4		g_LightDir;			// lighting direction

	float3		g_EyePosition;		// camera position
	float		g_lodFactor;		// lod factor (0: full density, 1: no density)
	
	float4		g_baseHairColor;	// base hair color (not used when texture is applied)
	float4		g_diffuseColor;		// diffuse light color
	float4		g_specColor;		// specular light color
	float4		g_hdrScale;			// modulation factor for final color for HDR related rescaling of color
	
	float		g_alpha;			// output alpha value
	int			g_useTextures;		// whether to use textures or not
	int			g_useShadows;		// whether to use hair shadows or not (shadow values are computed and passed from eariler vertex stages)
	int			g_useShading;		// whether to use shading or not

	float		g_rootDarken;		// apply root darkening (disabled for now)
	float		g_shadowSigma;		// shadow attenuation factor
	float		g_ambientScale;		// scaling for ambient shading terms
	float		g_diffuseScale;		// scaling for diffuse shading term

	float		g_diffuseBlend;		// blending factor between hair diffuse lighting vs skin diffuse lighting (0 == all tangent, 1 == all normal)
	float		g_specularPrimaryScale; // scaling for primary specular highlight
	float		g_specularSecondaryScale; // scaling for secondary specular highlight
	float		g_specPowerPrimary;	// specular power for primary specular highlight

	float		g_specPowerSecondary; // specular power for secondary specular highlight
	float		g_secondarySpecularOffset; // offset value for secondary highlight shift
	int			g_useRootColorTexture; // use root color texture?
	int			g_useTipColorTexture;  // use tip color texture?
	
	int			g_colorizeLOD;	// colorized lod factor for debugging?
	float		pad[3];	// padding
}

//////////////////////////////////////////////////////////////////////////////
// sampler states
//////////////////////////////////////////////////////////////////////////////
SamplerState samLinear; // expecting one linear texture sampler
    
// input to this pixel shader (output from eariler geometry/vertex stages)
struct HairPixelShaderInput
{
    float4	position : SV_Position;  // position after view projection has been applied
    float4  tangentAndNormal : TANGENT_NORMAL; // tangent and normal compressed in xy format (prenormalized)
    float	shadow : SHADOW;          // shadows computed at 'per vertex' level in earlier shader stages
    nointerpolation float2 texcoords : TEXCOORDS;   // texture coords to sample color textures, etc.
    float	tex: TEXTUREACROSSSTRAND; // per hair texture, .x = 0,1 (left or right of the quad strip) .y = 0-1 along hair length
	float3	wPos : WorldPosition;   // world space position used for lighting, etc.
};

// convert prenormalized 2D vector back to 3d unit vector
inline float3 getUnitVector(float2 v)
{
	return float3(v.x, v.y, sqrt(max(0.0,1.0 - v.x * v.x - v.y * v.y)));
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Pixel shader for interpolated hair rendering
//////////////////////////////////////////////////////////////////////////////////////////////
float4 ps_main(HairPixelShaderInput input) : SV_Target
{        
	float4 baseColor = g_baseHairColor;
	float4 diffuseLightColor = g_diffuseColor;
    float4 specularLightColor = g_specColor;

	if (g_useTextures)
	{
		float4 rootColor = baseColor;
		float4 tipColor	= baseColor;

		if (g_useRootColorTexture)
			rootColor = (g_rootHairColorTexture.SampleLevel(samLinear,input.texcoords,0));  
		if (g_useTipColorTexture)
			tipColor = (g_tipColorTexture.SampleLevel(samLinear,input.texcoords,0));  

		float ratio = input.tex;
		baseColor = (1.0 - ratio) * rootColor + ratio * tipColor;			
	}
  	          
    // Local geometry   	
	float3 P = input.wPos;
	float3 L = normalize(g_LightDir.xyz); 
	float3 V = normalize(g_EyePosition - P);

	// tangent and normal are prenormalized
	float3 T = getUnitVector(input.tangentAndNormal.xy); 
	float3 N = getUnitVector(input.tangentAndNormal.zw);

	float absorption = 0.0f;
	if (g_useShadows)
		absorption = input.shadow;

    //kajiya and kay lighting
    
    //diffuse
    float TdotL = dot( T , L);
    float diffuseHair = sqrt( 1 - TdotL*TdotL );

	// example to use surface normal to blend in diffuse lighting on the surface
	float diffuseSkin = max(0, dot( -N, L));

	float diffuse = g_diffuseBlend * diffuseSkin + (1.0f - g_diffuseBlend) * diffuseHair;
	
    // half-angle specular
	float3 H = normalize(-V + L);

    float TdotH = dot(T, H);
	float specPrimary = sqrt(1 - TdotH*TdotH);
	specPrimary = pow(max(0, specPrimary), g_specPowerPrimary);

    // secondary
	TdotH = clamp(TdotH + g_secondarySpecularOffset, -1.0, 1.0);
	float specSecondary = sqrt(1 - TdotH*TdotH);
	specSecondary = pow(max(0, specSecondary), g_specPowerSecondary);

	// shadow
    float lit = exp(-absorption*g_shadowSigma) ;

	diffuse *= lit;
	specPrimary *= lit;
	specSecondary *= lit;

    float4 outputColor;

    outputColor.xyz = 0.0
		+ g_ambientScale * baseColor.xyz
		+ g_diffuseScale * diffuse * diffuseLightColor.xyz * baseColor.xyz
		+ g_specularPrimaryScale * specPrimary * specularLightColor.xyz				
		+ g_specularSecondaryScale * specSecondary * specularLightColor.xyz
		;

	// color adjustment due to hdr
	outputColor.xyz *= g_hdrScale.xyz;

	if (!g_useShading)
		outputColor.xyz = baseColor.xyz;

	outputColor.a = g_alpha;

	if (g_colorizeLOD)
	{
		if (g_lodFactor == 0.0f)
			outputColor.xyz = float3(0.0f, 0.0f, 1.0f);
		else if (g_lodFactor == 1.0f)
			outputColor.xyz = float3(1.0f, 1.0f, 1.0f);
		else
			outputColor.xyz = float3(g_lodFactor, 1.0f - g_lodFactor, 0.0f);
	}

	// for debugging and experimenting with the shader
	// uncomment the following lines to test
	//outputColor.rgb = float3(1,0,0); // all red color
	//outputColor.rgb = input.tex * float3(1,1,1); // darker at root, brighter at tip
	//outputColor.rgb = abs(T.xyz); // colorize hair with its tangnet vector
	//outputColor.rgb = abs(N.xyz); // colorize hair with its normal vector
	//outputColor.rgb = abs(diffuse.xxx);
                                       
    return outputColor;
}

