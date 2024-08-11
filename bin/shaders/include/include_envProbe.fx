#ifndef ENVPROBE_FX_H_INCLUDED
#define ENVPROBE_FX_H_INCLUDED


#define REFLECTION_CUBE_RESOLUTION		128
#define AMBIENT_CUBE_NUM_MIPS			2
#define REFLECTION_CUBE_NUM_MIPS		6

#define AMBIENT_VERTEX_BASED_MIP_INDEX		(AMBIENT_CUBE_NUM_MIPS - 1)
#define REFLECTION_VERTEX_BASED_MIP_INDEX	(REFLECTION_CUBE_NUM_MIPS - 1)

#define ENVPROBE_MIPLEVEL_SCALE		 0.625
float CalcEnvProbeMipLevel( float3 N, int2 pixelCoord )
{
	return ENVPROBE_MIPLEVEL_SCALE * CalcCubeMipLevel( REFLECTION_CUBE_RESOLUTION, N, pixelCoord );
}

float EncodeEnvProbeMipLevel( float3 N, int2 pixelCoord )
{
	float lvl = CalcEnvProbeMipLevel( N, pixelCoord );
	return lvl / REFLECTION_CUBE_NUM_MIPS;
}

float DecodeEnvProbeMipLevel( float encodedValue )
{
	return encodedValue * REFLECTION_CUBE_NUM_MIPS;
}

/// 'dir' MUST BE NORMALIZED !!!
float2 CalcEnvProbeAtlasCoord( float3 dir, int sliceIndex )
{
	const int sphereFaceIndex = GetCubeDirParaboloidIndex( dir );
	float2 crd = CubeToParaboloid( dir, sphereFaceIndex );

	float extent = 1 << (REFLECTION_CUBE_NUM_MIPS - 1);
	float sphereRes = REFLECTION_CUBE_RESOLUTION;
	float sumRes = extent * 2 + sphereRes;
	crd = lerp( extent / sumRes, 1 - extent / sumRes, crd );

	crd = ( float2( 0, sliceIndex ) + (crd + float2( sphereFaceIndex, 0 )) ) / float2( 2, CUBE_ARRAY_CAPACITY );	
	return crd;
}

float2 ChangeEnvProbeAtlasCoordSlice_ByOffset( float2 coord, int sliceDiff )
{
	coord.y += sliceDiff / (float) CUBE_ARRAY_CAPACITY;
	return coord;
}

float3 SampleEnvProbeAmbientExplicitCoord( TEXTURE2D<float4> tex, SamplerState smp, float2 explicitCoord, float mipIndex = 0 )
{	
	return SAMPLE_LEVEL( tex, smp, explicitCoord, mipIndex ).xyz;
}

float3 SampleEnvProbeAmbient( TEXTURE2D<float4> tex, int sliceIndex, SamplerState smp, float3 N, float mipIndex = 0 )
{
	float2 coord = CalcEnvProbeAtlasCoord( N, sliceIndex );
	return SampleEnvProbeAmbientExplicitCoord( tex, smp, coord, mipIndex );
}

float3 SampleEnvProbeReflectionExplicitCoord( TEXTURE2D<float4> tex, SamplerState smp, float2 explicitCoord, float mipIndex )
{
	return SAMPLE_LEVEL( tex, smp, explicitCoord, mipIndex ).xyz;
}

float3 SampleEnvProbeReflection( TEXTURE2D<float4> tex, int sliceIndex, SamplerState smp, float3 N, float mipIndex )
{
	float2 coord = CalcEnvProbeAtlasCoord( N, sliceIndex );
	return SampleEnvProbeReflectionExplicitCoord( tex, smp, coord, mipIndex ).xyz;
}

#ifdef DEFERRED

	TEXTURE2D<float4>			EnvProbeAtlasAmbient	: register(t6);
	TEXTURE2D<float4>			EnvProbeAtlasReflection	: register(t7);
	SamplerState 				EnvProbeSampler			: register(s6);

#else

	TEXTURE2D<float4>			EnvProbeAtlasAmbient	: register(t22);
	TEXTURE2D<float4>			EnvProbeAtlasReflection	: register(t23);
	SamplerState 				EnvProbeSampler			: register(s11);

#endif

float3 UnnormalizedParallaxCorrection( ShaderCommonEnvProbeParams params, float3 pos, float3 N )
{
	// performance bump - no parallax for hair shader
#if defined(COMPILE_IN_SHADING_HAIR)
	return N;
#else
	const float3 CubemapPositionWS = params.probePos;
	const float4 PositionWS = float4 ( pos, 1.0 );	
	const float3 RayWS = N;
	const float3 ReflDirectionWS = N;

	float3 RayLS = mul( (float3x3)params.parallaxWorldToLocal, ReflDirectionWS );
	float3 PositionLS = mul( params.parallaxWorldToLocal, PositionWS ).xyz;

	float3 ISectVal0 = 1.f / RayLS;
	float3 ISectVal1 = PositionLS * ISectVal0;	
	float3 FirstPlaneIntersect = ISectVal0 - ISectVal1; // (Unitary - PositionLS) / RayLS;
	float3 SecondPlaneIntersect = -ISectVal0 - ISectVal1; // (-Unitary - PositionLS) / RayLS;
	float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
	float Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));

	// Use Distance in WS directly to recover intersection
	float3 IntersectPositionWS = PositionWS.xyz + RayWS * Distance;
	return IntersectPositionWS - CubemapPositionWS;
#endif
}

float3 NormalizedParallaxCorrection( ShaderCommonEnvProbeParams params, float3 pos, float3 N )
{
	return normalize( UnnormalizedParallaxCorrection( params, pos, N ) );
}

#define CALCENVPROBES_WEIGHTS_START		0.00001
#define CALCENVPROBES_WEIGHTS_LIMIT		0.999

// Pre-cert hack in order to make materials with vertex light to compile on ps4 :)
// With value 0.999 the shader compiler went into infinite loop (with O3 and O4 optimization option)
#if defined(VERTEX_SHADER) || defined(VERTEXSHADER)
#define CALCENVPROBES_WEIGHTS_GLOBAL_LIMIT		1.0
#else
#define CALCENVPROBES_WEIGHTS_GLOBAL_LIMIT		0.999
#endif

// 'allowAmbient' and 'allowReflection' are here just in case to help compiler to compile out unneeded code
// if this function gets used for calculating either ambient or reflection only.
void CalcEnvProbes_MipLod( out float3 outAmbient, out float3 outReflection, float3 worldSpacePosition, float3 N, float3 R, float ambientMipIndex, float reflectionMipIndex, float roughness, bool allowLocalProbes = true, bool allowAmbient = true, bool allowReflection = true, float interiorFactor = -1 )
{
	const float roughnessScaleMagic = (REFLECTION_CUBE_NUM_MIPS * 1.75) * roughness; // manipulated in order for better specular matching
	const float reflectionGlossLod	= roughnessScaleMagic * roughness;
	const float reflectionCustomLod	= max( reflectionGlossLod, reflectionMipIndex );

	float  weightsSum = CALCENVPROBES_WEIGHTS_START;
	float3 envProbeAmbient = float3 ( 0, 0, 0 );
	float3 envProbeReflection = float3 ( 0, 0, 0 );
					
	const uint GLOBAL_SLOT_INDEX = 0;

	const float2 precomputedCoords = float2 ( CalcEnvProbeAtlasCoord( N, 0 ) ); //, CalcEnvProbeAtlasCoord( R, 0 ) );

 	if ( allowLocalProbes )
	{
		[unroll]
		for ( uint slot_i=1; slot_i<CUBE_ARRAY_CAPACITY && weightsSum < CALCENVPROBES_WEIGHTS_LIMIT ; ++slot_i )
		{
			const ShaderCommonEnvProbeParams params = commonEnvProbeParams[slot_i];
			
			float3 local_pos = mul( params.areaWorldToLocal, float4 ( worldSpacePosition, 1.0 ) ).xyz;
			float3 neg_abs_local_pos = 1 - abs(local_pos);
			
			float weight_scale = 1;
			uint flags = asuint( params.intensities.y );
				
			[branch]
			if ( -1 != interiorFactor )
			{
				weight_scale *= !(flags & ENVPROBE_FLAG_INTERIOR) ? interiorFactor     : 1;
				weight_scale *= !(flags & ENVPROBE_FLAG_EXTERIOR) ? 1 - interiorFactor : 1;
			}

			[branch]
			if ( all( neg_abs_local_pos > 0 ) && weight_scale > 0 )
			{
				float3 off = saturate( (neg_abs_local_pos) * params.areaMarginScale );
				const float probeWeight = (1 - weightsSum) * weight_scale * params.weight * ( off.x * off.y * off.z );
		
				weightsSum += probeWeight;
				const float valueScale = probeWeight * params.intensities.x;
				if ( allowAmbient )
				{
					envProbeAmbient += valueScale * SampleEnvProbeAmbientExplicitCoord( EnvProbeAtlasAmbient, EnvProbeSampler, ChangeEnvProbeAtlasCoordSlice_ByOffset( precomputedCoords.xy, params.slotIndex ), ambientMipIndex );
				}
				if ( allowReflection )
				{
					envProbeReflection += valueScale * SampleEnvProbeReflection( EnvProbeAtlasReflection, params.slotIndex, EnvProbeSampler, NormalizedParallaxCorrection( params, worldSpacePosition, R ), reflectionCustomLod );
				}
			}
		}
	}

	[branch]
	if ( weightsSum < CALCENVPROBES_WEIGHTS_GLOBAL_LIMIT )
	{
		const ShaderCommonEnvProbeParams params = commonEnvProbeParams[GLOBAL_SLOT_INDEX];
		const float probeWeight = (1 - weightsSum) * params.weight;
		
		weightsSum += probeWeight;				
		const float valueScale = probeWeight * params.intensities.x;	
		if ( allowAmbient )
		{
			envProbeAmbient += valueScale * SampleEnvProbeAmbientExplicitCoord( EnvProbeAtlasAmbient, EnvProbeSampler, ChangeEnvProbeAtlasCoordSlice_ByOffset( precomputedCoords.xy, params.slotIndex ), ambientMipIndex );
		}
		if ( allowReflection )
		{
			envProbeReflection += valueScale * SampleEnvProbeReflection( EnvProbeAtlasReflection, params.slotIndex, EnvProbeSampler, R, reflectionCustomLod );
		}
	}

	const float invWeightsSum = 1.f / weightsSum;
	outAmbient = envProbeAmbient * invWeightsSum;
	outReflection = envProbeReflection * invWeightsSum;
}

uint CalcEnvProbes_MipLodComplexity( float3 worldSpacePosition )
{
	float weightsSum = CALCENVPROBES_WEIGHTS_START;
	uint complexity = 0;

	[unroll]
	for ( uint slot_i=1; slot_i<CUBE_ARRAY_CAPACITY && weightsSum < CALCENVPROBES_WEIGHTS_LIMIT; ++slot_i )
	{
		const ShaderCommonEnvProbeParams params = commonEnvProbeParams[slot_i];
		float3 local_pos = mul( params.areaWorldToLocal, float4 ( worldSpacePosition, 1.0 ) ).xyz;
		float3 neg_abs_local_pos = 1 - abs(local_pos);
			
		[branch]
		if ( all( neg_abs_local_pos > 0 ) )
		{
			float3 off = saturate( (neg_abs_local_pos) * params.areaMarginScale );
			const float probeWeight = (1 - weightsSum) * params.weight * ( off.x * off.y * off.z );
		
			weightsSum += probeWeight;

			complexity += 1;
		}
	}

	[branch]
	if ( weightsSum < CALCENVPROBES_WEIGHTS_LIMIT )
	{
		complexity += 1;
	}

	return complexity;
}

float3 CalcEnvProbeAmbient( float3 worldSpacePosition, float3 N, bool allowLocalProbes = true, float mipIndex = 0, float interiorFactor = -1 )
{
	float3 resultAmbient, resultReflection;
	CalcEnvProbes_MipLod( resultAmbient, resultReflection, worldSpacePosition, N, N, mipIndex, 0, 1, allowLocalProbes, true, false, interiorFactor );
	return resultAmbient;
}

float3 CalcGlobalEnvProbeAmbient( float3 worldSpacePosition, float3 N )
{
	return CalcEnvProbeAmbient( worldSpacePosition, N, false );
}

float3 CalcEnvProbeReflection_MipLod( float3 worldSpacePosition, float3 R, float mipLod, float roughness, bool allowLocalProbes = true, float interiorFactor = -1 )
{
	float3 resultAmbient, resultReflection;
	CalcEnvProbes_MipLod( resultAmbient, resultReflection, worldSpacePosition, R, R, 0, mipLod, roughness, allowLocalProbes, false, true, interiorFactor );
	return resultReflection;
}

float3 CalcEnvProbeReflection_NormalBasedMipLod( float3 worldSpacePosition, float3 V, float3 N, float roughness, int2 pixelCoord, bool allowLocalProbes, float interiorFactor = -1 )
{
	const float3 R = reflect( -V, N );		
	float mipLod = CalcEnvProbeMipLevel( R, pixelCoord );
	return CalcEnvProbeReflection_MipLod( worldSpacePosition, R, mipLod, roughness, allowLocalProbes, interiorFactor );
}

void CalcEnvProbes_NormalBasedMipLod( out float3 outAmbient, out float3 outReflection, float3 worldSpacePosition, float3 V, float3 ambientN, float3 reflectionN, float ambientMipIndex, float roughness, int2 pixelCoord, bool allowLocalProbes, float interiorFactor )
{
	const float3 R = reflect( -V, reflectionN );		
	float reflectionMipLod = CalcEnvProbeMipLevel( R, pixelCoord );
	CalcEnvProbes_MipLod( outAmbient, outReflection, worldSpacePosition, ambientN, R, ambientMipIndex, reflectionMipLod, roughness, allowLocalProbes, true, true, interiorFactor );
}

#endif
