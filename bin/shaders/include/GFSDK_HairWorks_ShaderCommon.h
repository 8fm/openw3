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

#ifndef __HAIRWORKS_SHADER_COMMON_H_
#define __HAIRWORKS_SHADER_COMMON_H_

#ifndef TWO_PI
#define TWO_PI 3.141592 * 2.0f
#endif

#ifndef FLT_EPSILON
#define FLT_EPSILON 0.00000001f
#endif

#ifndef SAMPLE_LEVEL
#define SAMPLE_LEVEL( _texture, _sampler, _coord, _level )	_texture.SampleLevel( _sampler, _coord, _level )
#endif

#ifndef SYS_POSITION
#define SYS_POSITION					SV_Position
#endif

#ifndef NOINTERPOLATION
#define	NOINTERPOLATION					nointerpolation
#endif

#ifndef USE_PIXEL_SHADER_INTERPOLATION
#define USE_PIXEL_SHADER_INTERPOLATION 1
#endif

#if USE_PIXEL_SHADER_INTERPOLATION
#define EMIT_WORLD_POS 0
#define EMIT_HAIRTEX 1
#define EMIT_TEXCOORD 1
#define EMIT_TANGENT 0
#define EMIT_NORMAL 0
#else
#define EMIT_WORLD_POS 0
#define EMIT_HAIRTEX 1
#define EMIT_TEXCOORD 1
#define EMIT_TANGENT 1
#define EMIT_NORMAL 1
#endif

#ifdef _CPP // to include this header in CPP file, define this preprocess variable _CPP

#ifndef float4
#define float4			gfsdk_float4
#endif

#ifndef float3
#define float3			gfsdk_float3
#endif

#ifndef float2
#define float2			gfsdk_float2
#endif

#ifndef float4x4
#define float4x4		gfsdk_float4x4
#endif

#ifndef row_major
#define row_major		
#endif

#ifndef float4x4
#define float4x4		gfsdk_float4x4
#endif

#endif

//////////////////////////////////////////////////////////////////////////////
// basic hair material from constant buffer
// modify this to fit hair into your shading pipeline
//////////////////////////////////////////////////////////////////////////////
struct GFSDK_Hair_Material // = 48floats, 12 float4
{
	// 12 floats (= 3 float4)
	float4			rootColor; 
	float4			tipColor; 
	float4			specularColor; 

	// 4 floats (= 1 float4)
	float			diffuseBlend;
	float			diffuseScale;
	float			diffuseHairNormalWeight;
	float			textureBrightness;

	// 4 floats (= 1 float4)
	float			diffuseNoiseFreqU;
	float			diffuseNoiseFreqV;
	float			diffuseNoiseScale;
	float			diffuseNoiseGain;

	// 4 floats (= 1 float4)
	float			specularPrimaryScale;
	float			specularPrimaryPower;
	float			specularPrimaryBreakup;
	float			specularNoiseScale;

	// 4 floats (= 1 float4)
	float			specularSecondaryScale;
	float			specularSecondaryPower;
	float			specularSecondaryOffset;
	float			_specularReserved_;

	// // 4 floats (= 1 float4)
	float			ambientEnvScale;
	float			specularEnvScale;
	float			_envReserved1_;
	float			_envReserved2_;

	// 4 floats (= 1 float4)
	float			rootTipColorWeight;
	float			rootTipColorFalloff;
	float			shadowSigma;
	float			strandBlendScale;

	// 4 floats (= 1 float4)
	float			glintStrength;
	float			glintCount;
	float			glintExponent;
	float			rootAlphaFalloff;

};

//////////////////////////////////////////////////////////////////////////////
// Use this data structure to manage all hair related cbuffer data within your own cbuffer
struct GFSDK_Hair_ConstantBuffer
{
	// camera information = 56 floats = 14 float4
	row_major	float4x4	inverseViewProjection; // inverse of view projection matrix
	row_major	float4x4	inverseProjection; // inverse of projection matrix
	row_major	float4x4	inverseViewport; // inverse of viewport transform
	row_major	float4x4	inverseViewProjectionViewport; // inverse of world to screen matrix

	float4					camPosition;		  // position of camera center
	float4					modelCenter; // center of the growth mesh model

	row_major	float4x4	shadowViewMatrix; // view matrix for high resolution shadow
	row_major	float4x4	shadowWorldToScreenMatrix; // world to screen for high resolution shadow

	// 4 floats (= 1 float4)
	int						useRootColorTexture;
	int						useTipColorTexture; 
	int						useStrandTexture;
	int						useSpecularTexture;

	int						useWeightTexture;
	int						receiveShadows;		
	int						shadowUseLeftHanded;
	float					materialWeight;

	int						strandBlendMode;
	int						colorizeMode;	
	int						strandPointCount;
	int						__reservedMode2__;

	float					lodDistanceFactor;		
	float					lodDetailFactor;		
	float					lodAlphaFactor;
	float					__reservedLOD___;
	

	// materials = 48 floats = 12 float4
	GFSDK_Hair_Material		hairMaterial; // per hair instance material

	// noise table
	float4					_noiseTable[256]; // 1024 floats
};

// Codes below are for use with hlsl shaders only
#ifndef _CPP 

//////////////////////////////////////////////////////////////////////////////
// Input to this pixel shader (output from eariler geometry/vertex stages)
// Do NOT use these values directly from your shader. Use GFSDK_HairGetShaderAttributes() .
//////////////////////////////////////////////////////////////////////////////
struct GFSDK_Hair_PixelShaderInput
{
					float4		position			: SYS_POSITION;

#if EMIT_TEXCOORD
NOINTERPOLATION		float		compTexcoord		: COMP_TEX_COORD;
#endif

#if EMIT_HAIRTEX
					float		hairtex				: HAIR_TEX;
#endif

#if USE_PIXEL_SHADER_INTERPOLATION

NOINTERPOLATION		uint		primitiveID			: PRIMITIVE_ID;
NOINTERPOLATION		float		coords				: COORDS;

#else

#if EMIT_TANGENT
					float3		tangent				: TANGENT;
#endif

#if EMIT_NORMAL
					float3		normal				: NORMAL;
#endif

#endif

#if	EMIT_WORLD_POS
					float3			worldpos			: JKL;
#endif
};

/////////////////////////////////////////////////////////////////////////////////////////////
// To get attributes for shaders, use this function
//
// GFSDK_Hair_GetShaderAttributes(
//	GFSDK_Hair_PixelShaderInput input, 
//	GFSDK_Hair_ConstantBuffer	hairConstantBuffer
// )
// 
// , where input is the pixel shader input and hairConstantBuffer is constant buffer defined for HairWorks
//
// This function filles all the attributes needed for hair shading such as,
//		world position (P),
//		tangent (T),
//		world space normal (N), 
//		texture coordinates (texcoords),
//		view vector (V), 
//		unique hair identifier (hairID)
//
//////////////////////////////////////////////////////////////////////////////
struct GFSDK_Hair_ShaderAttributes
{
	float3	P; // world coord position
	float3	T; // world space tangent vector
	float3	N; // world space normal vector at the root
	float4	texcoords; // texture coordinates on hair root 
					  // .xy: texcoord on the hair root
					  // .z: texcoord along the hair
					  // .w: texcoord along the hair quad
	float3	V; // world space view vector
	float	hairID; // unique hair identifier
};

//////////////////////////////////////////////////////////////////////////////
// return normalized noise (0..1) from a unique hash id
inline float GFSDK_Hair_GetNormalizedNoise(unsigned int hash, GFSDK_Hair_ConstantBuffer hairConstantBuffer)
{
	const unsigned int mask = 1023; 
	unsigned int id = hash & mask;

	unsigned int noiseIdx1 = id / 4;
	unsigned int noiseIdx2 = id % 4;

	return hairConstantBuffer._noiseTable[noiseIdx1][noiseIdx2];
}

//////////////////////////////////////////////////////////////////////////////
// return signed noise (-1 .. 1) from a unique hash id
inline float GFSDK_Hair_GetSignedNoise(unsigned int hash, GFSDK_Hair_ConstantBuffer hairConstantBuffer)
{
	float v = GFSDK_Hair_GetNormalizedNoise(hash, hairConstantBuffer);
	return 2.0f * (v - 0.5f);
}

//////////////////////////////////////////////////////////////////////////////
// return vector noise [-1..1, -1..1, -1..1] from a unique hash id
inline float3 GFSDK_Hair_GetVectorNoise(unsigned int seed, GFSDK_Hair_ConstantBuffer hairConstantBuffer)
{
	float x = GFSDK_Hair_GetSignedNoise(seed, hairConstantBuffer);
	float y = GFSDK_Hair_GetSignedNoise(seed + 1229, hairConstantBuffer);
	float z = GFSDK_Hair_GetSignedNoise(seed + 2131, hairConstantBuffer);

	return float3(x,y,z);
}

//////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_Hair_ComputeHairShading(
	float3 Lcolor, // light color and illumination
	float3 Ldir, // light direction

	float3 V, // view vector
	float3 N, // surface normal
	float3 T, // hair tangent

	float3 diffuseColor, // diffuse albedo
	float3 specularColor, // specularity

	float diffuseBlend,
	float primaryScale,
	float primaryShininess,
	float secondaryScale,
	float secondaryShininess,
	float secondaryOffset
	)
{
	// diffuse hair shading
	float TdotL = clamp(dot( T , Ldir), -1.0f, 1.0f);
	float diffuseSkin = max(0, dot( N, Ldir));
	float diffuseHair = sqrt( 1.0f - TdotL*TdotL );
	
	float diffuseSum = lerp(diffuseHair, diffuseSkin, diffuseBlend);
	
	// primary specular
	float3 H = normalize(V + Ldir);
	float TdotH = clamp(dot(T, H), -1.0f, 1.0f);
	float specPrimary = sqrt(1.0f - TdotH*TdotH);
	specPrimary = pow(max(0, specPrimary), primaryShininess);

	// secondary
	TdotH = clamp(TdotH + secondaryOffset, -1.0, 1.0);
	float specSecondary = sqrt(1 - TdotH*TdotH);
	specSecondary = pow(max(0, specSecondary), secondaryShininess);

	// specular sum
	float specularSum = primaryScale * specPrimary + secondaryScale * specSecondary;

	float3 output = diffuseSum * (Lcolor * diffuseColor) + specularSum * (Lcolor * specularColor);

	return output;
}

//////////////////////////////////////////////////////////////////////////////
// Compute shaded color for hair (diffuse + specular)
//////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_Hair_ComputeHairShading(
	float3						Lcolor,
	float3						Ldir,
	GFSDK_Hair_ShaderAttributes	attr,
	GFSDK_Hair_Material			mat,
	float3						hairColor
	)
{
	return GFSDK_Hair_ComputeHairShading(
		Lcolor, Ldir,
		attr.V, attr.N, attr.T,
		hairColor,
		mat.specularColor.rgb,
		mat.diffuseBlend,
		mat.specularPrimaryScale,
		mat.specularPrimaryPower,
		mat.specularSecondaryScale,
		mat.specularSecondaryPower,
		mat.specularSecondaryOffset);
}

//////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_SampleBilinear(
	float val00, float val10, float val01, float val11, float u, float v)
{
	float val0 = lerp(val00, val10, u);
	float val1 = lerp(val01, val11, u);
	float val  = lerp(val0, val1, v);
	return val;
}

//////////////////////////////////////////////////////////////////////////////
// Compute structured noise in 1D
//////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_ComputeStructuredNoise(
	float						noiseCount,
	float						seed,
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer
)
{
	// seed along hair length
	float hash = noiseCount * seed;
	float noiseSeed = floor(hash);
	float noiseFrac = hash - noiseSeed - 0.5f;
	
	// seed for neighboring sample
	float seedNeighbor = (noiseFrac < 0) ? noiseSeed - 1.0f : noiseSeed + 1.0f;
	seedNeighbor = max(0, seedNeighbor);

	// sample 4 noise values for bilinear interpolation
	float seedSample0 = noiseSeed;
	float seedSample1 = seedNeighbor;

	float noise0 = GFSDK_Hair_GetNormalizedNoise(seedSample0, hairConstantBuffer);
	float noise1 = GFSDK_Hair_GetNormalizedNoise(seedSample1, hairConstantBuffer);

	// interpolated noise sample
	float noise = lerp(noise0, noise1, abs(noiseFrac));

	// scale noise by user param
	return noise;
}

//////////////////////////////////////////////////////////////////////////////
// Compute structured noise in 2D
//////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_ComputeStructuredNoise2D(
	float2						noiseSize,
	float2						seed,
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer
)
{

	float noiseX = GFSDK_Hair_ComputeStructuredNoise(noiseSize.x, seed.x, hairConstantBuffer);
	float noiseY = GFSDK_Hair_ComputeStructuredNoise(noiseSize.y, seed.y, hairConstantBuffer);
	float noise = pow(noiseX * noiseY, 0.5f);

	return noise;
}

//////////////////////////////////////////////////////////////////////////////
// Compute color noise
//////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_ComputeColorNoise(
	GFSDK_Hair_ShaderAttributes	attr,
	GFSDK_Hair_Material			mat,
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer
	)
{
	if (mat.diffuseNoiseScale == 0.0f)
		return 1.0f;

	float noise = GFSDK_Hair_ComputeStructuredNoise2D(
		float2(mat.diffuseNoiseFreqU, mat.diffuseNoiseFreqV),
		attr.texcoords.xy,
		hairConstantBuffer);

	noise = noise * (1.0f + mat.diffuseNoiseGain);

	noise = lerp(1.0f, noise, mat.diffuseNoiseScale);

	return noise;
}


//////////////////////////////////////////////////////////////////////////////
// Compute glint (dirty sparkels) term
//////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_ComputeHairGlint(
	GFSDK_Hair_ShaderAttributes	attr,
	GFSDK_Hair_Material			mat,
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer
)
{
	// read material parameters
	float glintSize			= mat.glintCount;
	float glintPower		= mat.glintExponent;

	// seed along hair length
	float lengthHash = glintSize * attr.texcoords.z;
	float lengthSeed = floor(lengthHash);
	float lengthFrac = lengthHash - lengthSeed - 0.5f;
	
	// seed for neighboring sample
	float lengthSeedNeighbor = (lengthFrac < 0) ? lengthSeed - 1.0f : lengthSeed + 1.0f;
	lengthSeedNeighbor = max(0, lengthSeedNeighbor);

	// sample 4 noise values for bilinear interpolation
	float seedSample0 = attr.hairID + lengthSeed;
	float seedSample1 = attr.hairID + lengthSeedNeighbor;

	float noise0 = GFSDK_Hair_GetNormalizedNoise(seedSample0, hairConstantBuffer);
	float noise1 = GFSDK_Hair_GetNormalizedNoise(seedSample1, hairConstantBuffer);

	// interpolated noise sample
	float noise = lerp(noise0, noise1, abs(lengthFrac));

	// apply gamma like power function
	noise = pow(noise, glintPower);

	// scale noise by user param
	return noise;
}

//////////////////////////////////////////////////////////////////////////////
// Compute diffuse shading term only (no albedo is used)
//////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_Hair_ComputeHairDiffuseShading(
	float3 Lcolor, // light color and illumination
	float3 Ldir, // light direction

	GFSDK_Hair_ShaderAttributes	attr,
	GFSDK_Hair_Material			mat,
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer,

	float glint = 0.0f
)
{
	float3 N = attr.N;
	float3 T = attr.T; // hair tangent

	float diffuseBlend = mat.diffuseBlend; // blending factor

	// diffuse hair shading
	float TdotL = clamp(dot( T , Ldir), -1.0f, 1.0f);
	float diffuseSkin = max(0, dot( N, Ldir));
	float diffuseHair = sqrt( 1.0f - TdotL*TdotL );
	
	float diffuse = lerp(diffuseHair, diffuseSkin, diffuseBlend);
	float3 result = mat.diffuseScale * saturate(diffuse) * Lcolor;

	// apply glint
	float glintScale = mat.glintStrength;
	if (glintScale > 0)
	{
		float luminance = dot(Lcolor, float3(0.3, 0.5, 0.2));
		result += glintScale * glint * float3(luminance, luminance, luminance); 
	}

	return max(0, result);
}

//////////////////////////////////////////////////////////////////////////////
// Compute specular shading term only
//////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_Hair_ComputeHairSpecularShading(
	float3 Lcolor, // light color and illumination
	float3 Ldir, // light direction

	GFSDK_Hair_ShaderAttributes	attr,
	GFSDK_Hair_Material			mat,
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer,
	
	float glint = 0.0f)
{
	float3 T = attr.T; // hair tangent
	float3 V = attr.V; // view vector

	// read from hair material
	float primaryScale		= mat.specularPrimaryScale;
	float primaryShininess	= mat.specularPrimaryPower;
	float secondaryScale	= mat.specularSecondaryScale;
	float secondaryShininess = mat.specularSecondaryPower;
	float secondaryOffset	= mat.specularSecondaryOffset;
	float3 specularity		= mat.specularColor.rgb;
	float diffuseBlend		= mat.diffuseBlend; // blending factor

	// specular noise
	float hairID = attr.hairID;
	float hairSignedNoise = GFSDK_Hair_GetSignedNoise((unsigned int)hairID, hairConstantBuffer);
	float specPrimaryOffset = 0.5f * mat.specularPrimaryBreakup * hairSignedNoise;

	// primary specular
	float3 H = normalize(V + Ldir);
	float TdotH = clamp(dot(T, H), -1.0f, 1.0f);
	float TdotHshifted = clamp(TdotH + specPrimaryOffset, -1.0f, 1.0f);
	float specPrimary = sqrt(1.0f - TdotHshifted*TdotHshifted);
	specPrimary = pow(max(0, specPrimary), primaryShininess);

	// secondary
	TdotH = clamp(TdotH + secondaryOffset, -1.0, 1.0);
	float specSecondary = sqrt(1 - TdotH*TdotH);
	specSecondary = pow(max(0, specSecondary), secondaryShininess);

	// specular sum
	float specularSum = primaryScale * specPrimary + secondaryScale * specSecondary;

	float3 result = specularSum * (Lcolor * specularity);

	// apply glint
	float glintScale = mat.glintStrength;
	if (glintScale > 0)
	{		
		// multiplicative specular
		result *= lerp(1.0, glint, glintScale);
	}

	// visibility due to diffuse normal
	float3 N = attr.N;
	float visibilityScale = lerp(1.0f, saturate(dot(N, Ldir)), diffuseBlend);
	result *= visibilityScale;

	return max(0,result);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Computes blending factor between root and tip
//////////////////////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_GetRootTipRatio(const float s,  GFSDK_Hair_Material mat)
{
	float ratio = s;

	// add bias for root/tip color variation
	if (mat.rootTipColorWeight < 0.5f)
	{
		float slope = 2.0f * mat.rootTipColorWeight;
		ratio = slope * ratio;
	}
	else
	{
		float slope = 2.0f * (1.0f - mat.rootTipColorWeight) ;
		ratio = slope * (ratio - 1.0f) + 1.0f;
	}

	// modify ratio for falloff
	float slope = 1.0f / (mat.rootTipColorFalloff + 0.001f);
	ratio = saturate(0.5f + slope * (ratio - 0.5f));

	return ratio;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Returns hair color from textures for this hair fragment.
//////////////////////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_Hair_SampleHairColorTex(
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer,
	GFSDK_Hair_Material			mat, 
	SamplerState				texSampler, 
	Texture2D					rootColorTex, 
	Texture2D					tipColorTex, 
	float3						texcoords)
{
	float3 rootColor = mat.rootColor.rgb;
	float3 tipColor = mat.tipColor.rgb;

	if (hairConstantBuffer.useRootColorTexture)
		rootColor = (SAMPLE_LEVEL( rootColorTex, texSampler, texcoords.xy, 0 )).rgb;  
	if (hairConstantBuffer.useTipColorTexture)
		tipColor = (SAMPLE_LEVEL( tipColorTex, texSampler, texcoords.xy, 0 )).rgb;  

	float ratio = GFSDK_Hair_GetRootTipRatio(texcoords.z, mat);

	float3 hairColor = lerp(rootColor, tipColor, ratio);

	hairColor *= mat.textureBrightness;

	return hairColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Returns hair color from textures for this hair fragment.
//////////////////////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_Hair_SampleHairColorStrandTex(
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer,
	GFSDK_Hair_Material			mat, 
	SamplerState				texSampler, 
	Texture2D					rootColorTex, 
	Texture2D					tipColorTex, 
	Texture2D					strandColorTex, 
	float4						texcoords)
{
	float3 rootColor = mat.rootColor.rgb;
	float3 tipColor = mat.tipColor.rgb;

	if (hairConstantBuffer.useRootColorTexture)
		rootColor = (SAMPLE_LEVEL( rootColorTex, texSampler, texcoords.xy, 0 )).rgb;  
	if (hairConstantBuffer.useTipColorTexture)
		tipColor = (SAMPLE_LEVEL( tipColorTex, texSampler, texcoords.xy, 0 )).rgb;  

	float ratio = GFSDK_Hair_GetRootTipRatio(texcoords.z, mat);

	float3 hairColor = lerp(rootColor, tipColor, ratio);

	if (hairConstantBuffer.useStrandTexture)
	{
		float3 strandColor = (SAMPLE_LEVEL( strandColorTex, texSampler, texcoords.zw, 0 )).rgb;  

		switch(hairConstantBuffer.strandBlendMode)
		{
			case 0:
				hairColor = mat.strandBlendScale * strandColor;
				break;
			case 1:
				hairColor = lerp(hairColor, hairColor * strandColor, mat.strandBlendScale);
				break;
			case 2:
				hairColor += mat.strandBlendScale * strandColor;
				break;
			case 3:
				hairColor += mat.strandBlendScale * (strandColor - 0.5f);
				break;
		}
	}

	hairColor *= mat.textureBrightness;

	return hairColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Returns blended hair color between root and tip color
//////////////////////////////////////////////////////////////////////////////////////////////
float3 GFSDK_Hair_SampleHairColor(GFSDK_Hair_Material mat, float4 texcoords)
{
	float3 rootColor = mat.rootColor.rgb;
	float3 tipColor = mat.tipColor.rgb;

	float ratio = GFSDK_Hair_GetRootTipRatio(texcoords.z, mat);

	float3 hairColor = lerp(rootColor, tipColor, ratio);

	return hairColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Computes target alpha based on hair length alpha control
//////////////////////////////////////////////////////////////////////////////////////////////
float GFSDK_Hair_ComputeAlpha(
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer,
	GFSDK_Hair_Material			mat,
	GFSDK_Hair_ShaderAttributes attr
	)
{
	float lengthScale = attr.texcoords.z;

	float rootWeight = saturate( (lengthScale + FLT_EPSILON) / (mat.rootAlphaFalloff + FLT_EPSILON));
	float rootAlpha = lerp(0.0f, 1.0f, rootWeight);

	float lodAlpha = 1.0f - hairConstantBuffer.lodAlphaFactor;

	//float tipWeight = saturate((1.0f - attr.texcoords.z) / 0.1f);
	//float tipAlpha = lerp(baseAlpha, 1.0f, tipWeight);

	float alpha = rootAlpha * lodAlpha;

	return alpha;
}

/////////////////////////////////////////////////////////////////////////////////////////////
inline float GFSDK_HAIR_SoftDepthCmpGreater(float sampledDepth, float calcDepth)
{
	return max(0.0, sampledDepth - calcDepth);
}

inline float GFSDK_HAIR_SoftDepthCmpLess(float sampledDepth, float calcDepth)
{
	return max(0.0, calcDepth - sampledDepth);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Compute hair to hair shadow
//////////////////////////////////////////////////////////////////////////////////////////////
float GFSDK_Hair_ShadowPCF(
	float2 texcoord, 
	float calcDepth, 
	SamplerState texSampler, 
	Texture2D shadowTexture, 
	int shadowUseLeftHanded)
{
	float shadow = 0;
	float wsum = 0;

	float w, h;
	float numMipLevels;
	shadowTexture.GetDimensions(0, w, h, numMipLevels);

	float invResolution = 1.0f / float(w);

	[unroll]
	for (int dx = - 1; dx <= 1; dx ++) {
		for (int dy = -1; dy <= 1; dy ++) {
			
			float w = 1.0f / (1.0f + dx * dx + dy * dy);
			float2 coords = texcoord + float2(float(dx) * invResolution, float(dy) * invResolution);

			float sampleDepth = SAMPLE_LEVEL(shadowTexture, texSampler, coords, 0).r;  
			float shadowDepth = GFSDK_HAIR_SoftDepthCmpLess(sampleDepth, calcDepth);

			if (shadowUseLeftHanded == 0)
				shadowDepth = GFSDK_HAIR_SoftDepthCmpGreater(sampleDepth, calcDepth);

			shadow += w * shadowDepth;
			wsum += w;
		}
	}
	 
	float s = shadow / wsum;
	return s;
}

//////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_GetGlobalShadowFactor(
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer, 
	GFSDK_Hair_Material			mat,
	float3						wPos,
	SamplerState				texSampler, 
	Texture2D					stex)
{
	const bool useShadows = hairConstantBuffer.receiveShadows;

	if (!useShadows)
		return 1.0f;

	float lit = 1.0f;
	
	const float shadowAtten = mat.shadowSigma;

	float2 texcoords = mul(float4(wPos, 1), hairConstantBuffer.shadowWorldToScreenMatrix).xy;
	float z = mul(float4(wPos, 1), hairConstantBuffer.shadowViewMatrix).z;

	float shadow = GFSDK_Hair_ShadowPCF(texcoords, z, texSampler, stex, hairConstantBuffer.shadowUseLeftHanded);
	lit = exp(- shadow * shadowAtten);

	return lit;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Compute some visualization color option
//////////////////////////////////////////////////////////////////////////////////////////////
bool GFSDK_Hair_VisualizeColor(
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer,
	GFSDK_Hair_Material			hairMaterial,
	GFSDK_Hair_ShaderAttributes attr, 
	inout float3				outColor
	)
{
	switch (hairConstantBuffer.colorizeMode)
	{
	case 1: // LOD
		{
			float3 zeroColor = float3(0.0, 0.0f, 1.0f);
			float3 distanceColor	= float3(1.0f, 0.0f, 0.0f);
			float3 detailColor	= float3(0.0f, 1.0f, 0.0f);
			float3 alphaColor	= float3(1.0f, 1.0f, 0.0f);

			float distanceFactor = hairConstantBuffer.lodDistanceFactor;
			float detailFactor = hairConstantBuffer.lodDetailFactor;
			float alphaFactor = hairConstantBuffer.lodAlphaFactor;

			outColor.rgb = zeroColor;

			if (distanceFactor > 0.0f)
				outColor.rgb = lerp(zeroColor, distanceColor, distanceFactor);

			if (alphaFactor > 0.0f)
				outColor.rgb = lerp(zeroColor, alphaColor, alphaFactor);

			if (detailFactor > 0.0f)
				outColor.rgb = lerp(zeroColor, detailColor, detailFactor);

			break;
		}
	case 2: // tangent
		{
			outColor.rgb = 0.5f + 0.5f * attr.T.xyz; // colorize hair with its tangnet vector
			break;
		}
	case 3: // mesh normal
		{
			outColor.rgb = 0.5f + 0.5f * attr.N.xyz; // colorize hair with its normal vector
			break;
		}
	case 4: // mesh normal
		{
			outColor.rgb = 0.5f + 0.5f * attr.N.xyz; // colorize hair with its normal vector
			break;
		}
	default:
		return false;
	}

	return true; // color computed 
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Convert screen space position (SV_Position) to clip space
//////////////////////////////////////////////////////////////////////////////////////////////
inline float4 GFSDK_Hair_ScreenToClip(float4 input, GFSDK_Hair_ConstantBuffer hairConstantBuffer)
{
	float4 sp;

	// convert to ndc
	sp.xy = mul( float4(input.x, input.y, 0.0f, 1.0f), hairConstantBuffer.inverseViewport).xy;
	sp.zw = input.zw;

	// undo perspective division to get clip
	sp.xyz *= input.w; 

	return sp;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Convert screen space position (SV_Position) to view space position
//////////////////////////////////////////////////////////////////////////////////////////////
inline float4 GFSDK_Hair_ScreenToView(float4 pixelPosition, GFSDK_Hair_ConstantBuffer hairConstantBuffer)
{
	float4 ndc = GFSDK_Hair_ScreenToClip(pixelPosition, hairConstantBuffer);
	return mul( ndc, hairConstantBuffer.inverseProjection);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Convert screen space position (SV_Position) to world space position
//////////////////////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_Hair_ScreenToWorld(float4 pixelPosition, GFSDK_Hair_ConstantBuffer	hairConstantBuffer)
{
	float4 wp = mul(float4(pixelPosition.xyz, 1.0f), hairConstantBuffer.inverseViewProjectionViewport);
	wp.xyz /= wp.w;
	return wp.xyz;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// THESE ARE RESERVED FOR HAIRWORKS INTERNAL USE AND SUBJECT TO CHANGE FREQUENTLY WITHOUT NOTICE
/////////////////////////////////////////////////////////////////////////////////////////////
inline float GFSDK_Hair_PackFloat2(float2 v)
{
	const float base = 2048;

	float basey = floor(base * v.y);
	float packed = basey + v.x;

	return packed;
}

inline float2 GFSDK_Hair_UnpackFloat2(float packedVal)
{
	const float inv_base = 1.0f / 2048.0f;

	float ubase = floor(packedVal);
	float unpackedy = ubase * inv_base;
	float unpackedx = packedVal - ubase;

	return float2(unpackedx, unpackedy);
}

inline float GFSDK_Hair_UnpackSignedFloat(float x)
{
	return clamp(2.0f * (x - 0.5f), -1.0f, 1.0f);
}

inline float2 GFSDK_Hair_UnpackSignedFloat2(float x)
{
	float2 unpacked = GFSDK_Hair_UnpackFloat2(x);
	float sx = GFSDK_Hair_UnpackSignedFloat(unpacked.x);
	float sy = GFSDK_Hair_UnpackSignedFloat(unpacked.y);

	return float2(sx, sy);
}

//////////////////////////////////////////////////////////////////////////////
// To get attributes for shaders, use this function
//
// GFSDK_Hair_GetShaderAttributes(
//	GFSDK_Hair_PixelShaderInput input, 
//	GFSDK_Hair_ConstantBuffer	hairConstantBuffer
// )
// 
// , where input is the pixel shader input and hairConstantBuffer is constant buffer defined for HairWorks
//
// This function filles all the attributes needed for hair shading such as,
// world position (P), 
// tangent (T), 
// surface normal (N), 
// view vector (V), 
// and texcoord.
//
// DO NOT use macros below directly.
//////////////////////////////////////////////////////////////////////////////

inline GFSDK_Hair_ShaderAttributes 
GFSDK_HAIR_GET_SHADER_ATTRIBUTES(
	GFSDK_Hair_PixelShaderInput input, 
	GFSDK_Hair_ConstantBuffer	hairConstantBuffer
	)
{
	GFSDK_Hair_ShaderAttributes attr = (GFSDK_Hair_ShaderAttributes)0;

#if EMIT_WORLD_POS
	attr.P = input.worldpos.xyz; 
#else
	attr.P = GFSDK_Hair_ScreenToWorld(input.position, hairConstantBuffer);
#endif

#if EMIT_TEXCOORD
	attr.texcoords.xy = GFSDK_Hair_UnpackFloat2(input.compTexcoord);
#endif

#if EMIT_HAIRTEX
	attr.texcoords.z = input.hairtex;
#endif

	attr.texcoords.w = 0;

#if USE_PIXEL_SHADER_INTERPOLATION
	attr.T = 0;
	attr.N = 0;
#else

#if EMIT_TANGENT
	attr.T	= normalize(input.tangent.xyz);
#endif

#if EMIT_NORMAL
	attr.N	= normalize(input.normal.xyz);
#endif

#endif // USE_PIXEL_SHADER_INTERPOLATION

	attr.V = normalize(hairConstantBuffer.camPosition.xyz - attr.P);

	attr.hairID = floor(attr.texcoords.x * 2048 * 2048 + 2048 * attr.texcoords.y);

	return attr;
}

#if USE_PIXEL_SHADER_INTERPOLATION

/////////////////////////////////////////////////////////////////////////////////////////////
// For pixel shader attribute interpolation
/////////////////////////////////////////////////////////////////////////////////////////////
#define GFSDK_HAIR_INDICES_BUFFER_TYPE	Buffer<float3>
#define GFSDK_HAIR_TANGENT_BUFFER_TYPE	Buffer<float4>
#define GFSDK_HAIR_NORMAL_BUFFER_TYPE	Buffer<float4>

#define GFSDK_HAIR_DECLARE_RESOURCES(SLOT0, SLOT1, SLOT2) \
	GFSDK_HAIR_INDICES_BUFFER_TYPE	GFSDK_HAIR_RESOURCE_FACE_HAIR_INDICES	: register(SLOT0); \
	GFSDK_HAIR_TANGENT_BUFFER_TYPE	GFSDK_HAIR_RESOURCE_TANGENTS : register(SLOT1); \
	GFSDK_HAIR_NORMAL_BUFFER_TYPE	GFSDK_HAIR_RESOURCE_NORMALS: register(SLOT2); 

struct GFSDK_HAIR_RESOURCES
{
	GFSDK_HAIR_INDICES_BUFFER_TYPE	FACE_HAIR_INDICES;
	GFSDK_HAIR_TANGENT_BUFFER_TYPE	TANGENTS;
	GFSDK_HAIR_NORMAL_BUFFER_TYPE	NORMALS;
};

#define _GFSDK_HAIR_USE_RESOURCES_ \
	GFSDK_HAIR_RESOURCES GFSDK_HAIR_RESOUCES_VAR; \
	GFSDK_HAIR_RESOUCES_VAR.FACE_HAIR_INDICES	= GFSDK_HAIR_RESOURCE_FACE_HAIR_INDICES; \
	GFSDK_HAIR_RESOUCES_VAR.TANGENTS			= GFSDK_HAIR_RESOURCE_TANGENTS; \
	GFSDK_HAIR_RESOUCES_VAR.NORMALS				= GFSDK_HAIR_RESOURCE_NORMALS; 

//////////////////////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_HAIR_GET_INTERPOLATED_TANGENT(GFSDK_HAIR_RESOURCES resources, int3 vertexIndices, float2 coords)
{
	float3 t0 = resources.TANGENTS.Load( vertexIndices[0] ).xyz;
	float3 t1 = resources.TANGENTS.Load( vertexIndices[1] ).xyz;
	float3 t2 = resources.TANGENTS.Load( vertexIndices[2] ).xyz;

	return  coords.x * t0 + coords.y * t1 + (1.0f - coords.x - coords.y) * t2;
}

//////////////////////////////////////////////////////////////////////////////////////////////
inline float3 GFSDK_HAIR_GET_INTERPOLATED_NORMAL(GFSDK_HAIR_RESOURCES resources, int3 vertexIndices, float2 coords)
{
	float3 t0 = resources.NORMALS.Load( vertexIndices[0] ).xyz;
	float3 t1 = resources.NORMALS.Load( vertexIndices[1] ).xyz;
	float3 t2 = resources.NORMALS.Load( vertexIndices[2] ).xyz;

	return  coords.x * t0 + coords.y * t1 + (1.0f - coords.x - coords.y) * t2;
}

//////////////////////////////////////////////////////////////////////////////////////////////
inline int3 GFSDK_HAIR_GET_ROOT_INDICES(GFSDK_HAIR_RESOURCES resources, uint primitiveID)
{
	return floor(resources.FACE_HAIR_INDICES.Load(primitiveID));
}

////////////////////////////////////////////////////////////////////////////////////////////
inline GFSDK_Hair_ShaderAttributes 
GFSDK_HAIR_GET_SHADER_ATTRIBUTES_WITH_RESOURCES(
	const GFSDK_Hair_PixelShaderInput	input,
	const GFSDK_Hair_ConstantBuffer		hairConstantBuffer,
	const GFSDK_HAIR_RESOURCES			hairResources)
{
	// copy other attributes
	GFSDK_Hair_ShaderAttributes attr = GFSDK_HAIR_GET_SHADER_ATTRIBUTES(input, hairConstantBuffer);

	float		hairtex		= attr.texcoords.z;
	const int	numPoints	= hairConstantBuffer.strandPointCount;
	float		hairCoord	= hairtex * float(numPoints);
	int			vertexID0	= floor(hairCoord);
	int			vertexID1	= min(vertexID0 + 1, numPoints-1);
	float		hairFrac	= hairCoord - float(vertexID0);

	// compute interpolated tangents and normals
	int3 hairIndices = GFSDK_HAIR_GET_ROOT_INDICES(hairResources, input.primitiveID);
	int3 rootIndices =  hairIndices * numPoints;

	int3	vertexIndices0 = rootIndices + int3(vertexID0, vertexID0, vertexID0);
	int3	vertexIndices1 = rootIndices + int3(vertexID1, vertexID1, vertexID1);

	float2	coords = GFSDK_Hair_UnpackFloat2(input.coords);
	float3	T0 = GFSDK_HAIR_GET_INTERPOLATED_TANGENT(hairResources, vertexIndices0, coords);
	float3	N0 = GFSDK_HAIR_GET_INTERPOLATED_NORMAL(hairResources, vertexIndices0, coords);

	float3	T1 = GFSDK_HAIR_GET_INTERPOLATED_TANGENT(hairResources, vertexIndices1, coords);
	float3	N1 = GFSDK_HAIR_GET_INTERPOLATED_NORMAL(hairResources, vertexIndices1, coords);

	attr.T = normalize(lerp(T0, T1, hairFrac));
	attr.N = normalize(lerp(N0, N1, hairFrac));

	return attr;
}

#define GFSDK_Hair_GetShaderAttributes(INPUT, CBUFFER) GFSDK_HAIR_GET_SHADER_ATTRIBUTES_WITH_RESOURCES(INPUT, CBUFFER, GFSDK_HAIR_RESOUCES_VAR)
#else

#define _GFSDK_HAIR_USE_RESOURCES_  // no resource defined
#define GFSDK_Hair_GetShaderAttributes(INPUT, CBUFFER) GFSDK_HAIR_GET_SHADER_ATTRIBUTES(INPUT, CBUFFER)

#endif // #if USE_PIXEL_SHADER_INTERPOLATION

#endif // ifndef _CPP

#endif