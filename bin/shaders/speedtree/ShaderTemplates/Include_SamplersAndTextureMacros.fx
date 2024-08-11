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

#ifndef ST_INCLUDE_SAMPLERS_AND_TEXTURE_MACROS
#define ST_INCLUDE_SAMPLERS_AND_TEXTURE_MACROS


///////////////////////////////////////////////////////////////////////
//  Common texture and sampler setup defines

#if (ST_DIRECTX11)

	shared SamplerState samStandard : register(s0);
	shared SamplerComparisonState samShadowMapComparison : register(s1);
	shared SamplerState samPoint : register(s2);
	shared SamplerState samLinearClamp : register(s3);

	#define DeclareTexture(name, reg) 								Texture2D name : register(t##reg);
	#define DeclareTextureMS(name, reg, num_samples)				Texture2DMS<float4, num_samples> name : register(t##reg);
	#define DeclareShadowMapTexture(name, reg) 						Texture2D name : register(t##reg);

	#define SampleTexture(name, texcoord) 							name.Sample(samStandard, texcoord)
	// todo
	#define SampleTextureSpecial(name, texcoord)					name.SampleLevel(samStandard, texcoord, 0)
	#define SampleTextureLinearClamp(name, texcoord)				name.SampleLevel(samLinearClamp, texcoord, 0)
	#define SampleTextureLod(name, texcoord, lod) 			 		name.SampleLevel(samPoint, texcoord, lod)
	#define SampleTextureMS(name, texcoord, sample) 				name.Load(texcoord, sample)
	#define SampleTextureCompare(name, sampler, texcoord)			((name).SampleCmpLevelZero(samShadowMapComparison, (texcoord).xy, (texcoord).z))
	
#elif (ST_PS4)

	SamplerState samStandard : register(s0);
	SamplerComparisonState samShadowMapComparison : register(s1);
	SamplerState samPoint : register(s2);
	SamplerState samLinearClamp : register(s3);

	#define DeclareTexture(name, reg) 								Texture2D name : register(t##reg);
	#define DeclareTextureMS(name, reg, num_samples)				Texture2D name : register(t##reg);
	#define DeclareShadowMapTexture(name, reg) 						Texture2D name : register(t##reg);

	#define SampleTexture(name, texcoord) 							name.Sample(samStandard, texcoord)
	
	#define SampleTextureSpecial(name, texcoord)					name.SampleLOD(samStandard, texcoord, 0) // todo
	#define SampleTextureLinearClamp(name, texcoord)				name.SampleLOD(samLinearClamp, texcoord, 0) // todo
	#define SampleTextureLod(name, texcoord, lod) 			 		name.SampleLOD(samPoint, texcoord, lod)
	#define SampleTextureMS(name, texcoord, sample)					name.Sample(samStandard, texcoord)
	#define SampleTextureCompare(name, sampler, texcoord) 			((name).SampleCmpLOD0(samShadowMapComparison, (texcoord).xy, (texcoord).z))

#elif (ST_OPENGL)

	#define DeclareTexture(name, sam_reg) 							uniform sampler2D name
	#define DeclareShadowMapTexture(name, sam_reg)					uniform sampler2DShadow name

	#if (__VERSION__ >= 150)
		#define SampleTexture(name, texcoord) 						texture(name, texcoord)
		#define SampleTextureLod(name, texcoord, lod) 			 	textureLod(name, texcoord, lod)
		#define SampleTextureLinearClamp(name, texcoord)			texture(name, texcoord)
		#define SampleTextureCompare(texture, sampler, texcoord) 	textureProj(sampler, texcoord)
	#else
		#define SampleTexture(name, texcoord) 						texture2D(name, texcoord)
		#define SampleTextureLinearClamp(name, texcoord)			texture2D(name, texcoord)
		#define SampleTextureLod(name, texcoord, lod) 			 	texture2D(name, texcoord)
		#define SampleTextureCompare(texture, sampler, texcoord) 	shadow2DProj(sampler, texcoord).r
	#endif
	
#else

	#if (ST_DIRECTX9) || (ST_PS3)
		#define DeclareTexture(name, reg) 							texture name; sampler2D sam##name : register(s##reg) = sampler_state { Texture = <name>; }
		#define DeclareTextureMS(name, reg, num_samples)			texture name; sampler2D sam##name : register(s##reg) = sampler_state { Texture = <name>; }
		#define DeclareShadowMapTexture(name, reg) 					texture name; sampler2D sam##name : register(s##reg) = sampler_state { Texture = <name>; }
	#endif
	
	#define SampleTexture(name, texcoord) 							tex2D(sam##name, texcoord)
	#define SampleTextureLod(name, texcoord, lod) 					tex2Dlod(sam##name, float4((texcoord).xy, 0.0, lod))
	#define SampleTextureCompare(name, sampler, texcoord) 			tex2Dproj(sampler, texcoord).r
	#define SampleTextureLinearClamp(name, texcoord)				tex2Dlod(sam##name, float4((texcoord).xy, 0.0, 0))
	// todo
	#define SampleTextureSpecial(name, texcoord)					tex2Dlod(sam##name, float4((texcoord).xy, 0.0, 0))
	#define SampleTextureMS(name, texcoord, sample) 				tex2D(sam##name, texcoord)

#endif

#endif // ST_INCLUDE_SAMPLERS_AND_TEXTURE_MACROS

