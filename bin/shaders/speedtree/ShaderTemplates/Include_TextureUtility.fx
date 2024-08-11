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

#ifndef ST_INCLUDE_TEXTURE_UTILITY
#define ST_INCLUDE_TEXTURE_UTILITY


// todo: why is this only being used by the deferred pixel shader?

///////////////////////////////////////////////////////////////////////
//  LookupColorWithDetail
//
//	This function encapsulates the diffuse and detail layer texture lookups.
//
//	It will always look up the diffuse texture. If a detail layer is present,
//	it will look it up and lerp between the diffuse and detail layer by the
//	detail map's alpha channel value. If the detail layer should fade away,
//	it will handle that was well.

float3 LookupColorWithDetail(float2 vDiffuseTexCoords, float2 vDetailTexCoords, float fRenderEffectsFade)
{
	float3 vFinal = SampleTexture(DiffuseMap, vDiffuseTexCoords).rgb;

	if (ST_EFFECT_DETAIL_LAYER)
	{
		float4 texDetailColor = SampleTexture(DetailDiffuseMap, vDetailTexCoords);
		if (ST_EFFECT_DETAIL_LAYER_FADE)
			texDetailColor.a *= fRenderEffectsFade;

		vFinal = lerp(vFinal, texDetailColor.rgb, texDetailColor.a);
	}

	return vFinal;
}


///////////////////////////////////////////////////////////////////////
//  LookupNormalWithDetail
//
//	This function encapsulates the diffuse and detail layer normal map
//	lookups.
//
//	It will always look up the diffuse normal texture. If a detail normal
//	layer is present, it will look it up and lerp between the diffuse and
//	detail layer normals by the detail normal's alpha channel value
//	by the detail's *color* alpha channel value (the same one used in
//	LookupColorWithDetail). If the detail layer should fade away, it will
//	handle that was well.
//
//	The normal map color values [0,1] are converted to a vector [-1,1] before
//	returning.

float3 LookupNormalWithDetail(float2 vDiffuseTexCoords, float2 vDetailTexCoords, float fRenderEffectsFade)
{
	float3 vFinal = SampleTexture(NormalMap, vDiffuseTexCoords).rgb;

	if (ST_EFFECT_DETAIL_NORMAL_LAYER)
	{
		float4 texDetailColor = SampleTexture(DetailDiffuseMap, vDetailTexCoords);
		if (ST_EFFECT_DETAIL_LAYER_FADE)
			texDetailColor.a *= fRenderEffectsFade;

		float3 vDetailNormal = SampleTexture(DetailNormalMap, vDetailTexCoords).rgb;
		vFinal = lerp(vFinal, vDetailNormal, texDetailColor.a);
	}

	return DecodeVectorFromColor(vFinal);
}


///////////////////////////////////////////////////////////////////////  
//	AlphaTestNoise_Billboard
//
//	The SpeedTree SDK generates a small noise texture to facilitate a
//	"fizzle" effect when alpha testing is used over alpha-to-coverage.
//	Alpha fizzle is the term applied to the "fizzling" from one LOD to
//	another based upon changing alpha testing values in unison with
//	returning alpha noise from the pixel shader.
//
//	This function uses the billboard's texcoords to lookup the noise
//	texture to get a smooth fizzle effect.

float AlphaTestNoise_Billboard(float2 vTexCoords)
{
	#define ST_BILLBOARD_ALPHA_NOISE_SCALAR 30.0

	return (ST_ALPHA_TEST_NOISE ? SampleTexture(NoiseMap, vTexCoords * ST_BILLBOARD_ALPHA_NOISE_SCALAR).a : 1.0);
}


///////////////////////////////////////////////////////////////////////  
//	AlphaTestNoise_3dTree
//
//	The SpeedTree SDK generates a small noise texture to facilitate a
//	"fizzle" effect when alpha testing is used over alpha-to-coverage.
//	Alpha fizzle is the term applied to the "fizzling" from one LOD to
//	another based upon changing alpha testing values in unison with
//	returning alpha noise from the pixel shader.
//
//	This function uses the 3d tree's texcoords to lookup the noise
//	texture to get a smooth fizzle effect.

float AlphaTestNoise_3dTree(float2 vTexCoords)
{
	#define ST_TREE_ALPHA_NOISE_SCALAR	0.5

	return (ST_ALPHA_TEST_NOISE ? SampleTexture(NoiseMap, vTexCoords * ST_TREE_ALPHA_NOISE_SCALAR).a : 1.0);
}

#endif // ST_INCLUDE_TEXTURE_UTILITY

