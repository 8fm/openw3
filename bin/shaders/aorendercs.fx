// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// Developed by Team Hemi @ Xbox
//
// Author:  James Stanard (jstanard@microsoft.com)
//          Martin Fuller (mafuller@microsoft.com)

#include "common.fx"

#define __XBOX_REGALLOC_VGPR_LIMIT 32

#define DONT_PROCESS_FAR_Z
//#define SAMPLE_EXHAUSTIVELY
#define ALLOW_BACK_PROJECTION		// turn this off for a little bit of extra speed

#ifndef INTERLEAVE_RESULT
#define WIDE_SAMPLING 1
#endif

#ifdef INTERLEAVE_RESULT
TEXTURE2D_ARRAY<float>	DepthTex		: register(t0);
#else
TEXTURE2D<float>		DepthTex		: register(t0);
#endif
#ifdef USE_NORMAL_BUFFER
# ifdef INTERLEAVE_RESULT
TEXTURE2D_ARRAY<float3>	NormalTex		: register(t1);
# else
TEXTURE2D<float3>		NormalTex		: register(t1);
# endif
#endif
RW_TEXTURE2D<float>		Occlusion		: register(u0);
SamplerState			PointSampler	: register(s0);

START_CB( CustomConstantBuffer, 3 )
#ifndef USE_NORMAL_BUFFER
	float4 gInvThicknessTable[3];
#endif
	float4 gSampleWeightTable[3];
	float2 gInvSliceDimension;
	float  gRejectFadeoff;
#ifdef USE_NORMAL_BUFFER
	float  gNormalAOMultiply;
	float  gNormalBackprojectionToleranceAdd;
	float  gNormalBackprojectionToleranceMul;
#endif
END_CB

#ifdef USE_NORMAL_BUFFER
static const float4 gInvThicknessTable[3] = {{ 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }};

START_CB( FrustumDelta, 1 )
	float4 CameraPos;
	float4 FrustumPoint;
	float4 FrustumHDelta;
	float4 FrustumVDelta;
END_CB
#endif

#if WIDE_SAMPLING
// 32x32 cache size:  the 16x16 in the center forms the area of focus with the 8-pixel perimeter used for wide gathering.
# define TILE_DIM 32
# define THREAD_COUNT_X 16
# define THREAD_COUNT_Y 16
#else
// 16x16 cache size:  the 8x8 in the center forms the area of focus with the 4-pixel perimeter used for gathering.
# define TILE_DIM 16
# define THREAD_COUNT_X 8
# define THREAD_COUNT_Y 8
#endif

#ifdef USE_NORMAL_BUFFER
GROUPSHARED float3 Positions[TILE_DIM * TILE_DIM];
#else
GROUPSHARED float DepthSamples[TILE_DIM * TILE_DIM];
#endif

#if !_XBOX_ONE
#define __XB_Med3_F32 clamp
#endif

#ifdef USE_NORMAL_BUFFER
float3 ComputePosition(float2 uv, float depth)
{
	float3 pos = FrustumPoint.xyz;
	pos += uv.x * FrustumHDelta.xyz;
	pos += (1.0 - uv.y) * FrustumVDelta.xyz;
	return (depth * pos) + CameraPos.xyz;
}

float ComputeNormalAO( float3 normal, float3 pos, uint index )
{
	float3 dir	= Positions[index] - pos;
	float invD	= 1.0f / length(dir);
	float ao = dot(normal, dir * invD);

#ifdef ALLOW_BACK_PROJECTION
	ao			+= gNormalBackprojectionToleranceAdd;
	ao			*= gNormalBackprojectionToleranceMul;
#endif
	ao			 = max(0.0, ao);	

	return ao * invD;
}

float TestSamplePair( float3 normal, float3 pos, float frontDepth, float invRange, uint base, int offset )
{
    float normalAo1 = ComputeNormalAO(normal, pos, base + offset);
	float normalAo2 = ComputeNormalAO(normal, pos, base - offset);
	return 0.5f * (normalAo1 + normalAo2);
}

#else

float TestSamplePair( float3 normal, float3 pos, float frontDepth, float invRange, uint base, int offset )
{
	// "Disocclusion" measures the penetration depth of the depth sample within the sphere.
	// Disocclusion < 0 (full occlusion) -> the sample fell in front of the sphere
	// Disocclusion > 1 (no occlusion) -> the sample fell behind the sphere
	float disocclusion1 = DepthSamples[base + offset] * invRange - frontDepth;
	float disocclusion2 = DepthSamples[base - offset] * invRange - frontDepth;

	// A negative disocclusion is considered an invalid or rejected sample.  It's too far in front of the sphere
	// center and potentially hides other occluding surfaces behind it.  We take this into account to gradually
	// fade off occlusion of its twin sample the farther the sample gets in front of the sphere.
	float output;

	output = 
		saturate(max(disocclusion1, -gRejectFadeoff * disocclusion2)) +
		saturate(max(disocclusion2, -gRejectFadeoff * disocclusion1));
/*
	float pseudoDisocclusion1 = saturate(gRejectFadeoff * disocclusion1);
	float pseudoDisocclusion2 = saturate(gRejectFadeoff * disocclusion2);

	output = 
		__XB_Med3_F32(disocclusion1, pseudoDisocclusion2, 1.0) +
		__XB_Med3_F32(disocclusion2, pseudoDisocclusion1, 1.0) -
		pseudoDisocclusion1 * pseudoDisocclusion2;
*/
	return output;
}

#endif

float TestSamples( float3 normal, float3 pos, uint centerIdx, uint x, uint y, float invDepth, float invThickness )
{
#if WIDE_SAMPLING
	x <<= 1;
	y <<= 1;
#endif

	float invRange = invThickness * invDepth;
	float frontDepth = invThickness - 0.5;

	if (y == 0)
	{
		// Axial
		float o1 = TestSamplePair(normal, pos, frontDepth, invRange, centerIdx, x);
		float o2 = TestSamplePair(normal, pos, frontDepth, invRange, centerIdx, x * TILE_DIM);

		return 0.5f * (o1 + o2);
	}
	else if (x == y)
	{
		// Diagonal
		float o1 = TestSamplePair(normal, pos, frontDepth, invRange, centerIdx, x * TILE_DIM - x);
		float o2 = TestSamplePair(normal, pos, frontDepth, invRange, centerIdx, x * TILE_DIM + x);

		return 0.5f * (o1 + o2);
	}
	else
	{
		// L-Shaped
		float o1 = TestSamplePair(normal, pos, frontDepth, invRange, centerIdx, y * TILE_DIM + x);
		float o2 = TestSamplePair(normal, pos, frontDepth, invRange, centerIdx, y * TILE_DIM - x);
		float o3 = TestSamplePair(normal, pos, frontDepth, invRange, centerIdx, x * TILE_DIM + y);
		float o4 = TestSamplePair(normal, pos, frontDepth, invRange, centerIdx, x * TILE_DIM - y);

		return 0.25f * (o1 + o2 + o3 + o4);
	}
}

#if WIDE_SAMPLING
[NUMTHREADS( 16, 16, 1 )]
#else
[NUMTHREADS( 8, 8, 1 )]
#endif
void cs_main( uint3 Gid : SYS_GROUP_ID, uint GI : SYS_GROUP_INDEX, uint3 GTid : SYS_GROUP_THREAD_ID, uint3 DTid : SYS_DISPATCH_THREAD_ID )
{
	// Fetch four depths
#ifdef INTERLEAVE_RESULT
	float2 QuadCenterUV						= (DTid.xy + GTid.xy - 3) * gInvSliceDimension;
	float4 depths							= DepthTex.Gather(PointSampler, float3(QuadCenterUV, DTid.z));
#else
	float2 QuadCenterUV						= (DTid.xy + GTid.xy - 7) * gInvSliceDimension;
	float4 depths							= DepthTex.Gather(PointSampler, QuadCenterUV);
#endif
	int destIdx								= GTid.x * 2 + GTid.y * 2 * TILE_DIM;

	// populate LDS
#ifdef USE_NORMAL_BUFFER
	float3 dx1								= FrustumPoint.xyz + QuadCenterUV.x * FrustumHDelta.xyz;
	float3 dx2								= FrustumPoint.xyz + (QuadCenterUV.x + gInvSliceDimension.x) * FrustumHDelta.xyz;
	float3 dy1								= (1.0f - QuadCenterUV.y) * FrustumVDelta.xyz;
	float3 dy2								= (1.0f - (QuadCenterUV.y + gInvSliceDimension.y)) * FrustumVDelta.xyz;

	Positions[destIdx]						= (dx1 + dy1) * depths.w;
	Positions[destIdx + 1]					= (dx2 + dy1) * depths.z;
	Positions[destIdx + TILE_DIM]			= (dx1 + dy2) * depths.x;
	Positions[destIdx + TILE_DIM + 1]		= (dx2 + dy2) * depths.y;
#ifdef INTERLEAVE_RESULT
	float3 normal							= NormalTex[DTid.xyz];
	float depth								= DepthTex[DTid.xyz];
#else
	float3 normal							= NormalTex[DTid.xy];
	float depth								= DepthTex[DTid.xy];
#endif
	normal									= (2.0f * normal) - 1.0f;
#else
	DepthSamples[destIdx]					= depths.w;
	DepthSamples[destIdx + 1]				= depths.z;
	DepthSamples[destIdx + TILE_DIM]		= depths.x;
	DepthSamples[destIdx + TILE_DIM + 1]	= depths.y;
#endif

	GROUP_BARRIER_GROUP_SYNC;

#if WIDE_SAMPLING
	uint thisIdx = GTid.x + GTid.y * TILE_DIM + 8 * TILE_DIM + 8;
#else
	uint thisIdx = GTid.x + GTid.y * TILE_DIM + 4 * TILE_DIM + 4;
#endif
	float ao								= 1.0f;

#ifdef DONT_PROCESS_FAR_Z
#ifndef USE_NORMAL_BUFFER
	float depth								= DepthSamples[thisIdx];
#endif
	[branch] if(depth < 1.0)
	{
#endif
		ao									= 0.0;

#ifdef USE_NORMAL_BUFFER
		float3 pos							= Positions[thisIdx];
		const float invThisDepth			= 0.0f;
#else
		float3 pos							= float3(0.0f, 0.0f, 0.0f);
		float3 normal						= float3(0.0f, 0.0f, 0.0f);	
		float invThisDepth					= 1.0 / DepthSamples[thisIdx];
#endif

#ifdef SAMPLE_EXHAUSTIVELY
		// 68 sample all cells in *within* a circular radius of 5
		ao += gSampleWeightTable[0].x * TestSamples(normal, pos, thisIdx, 1, 0, invThisDepth, gInvThicknessTable[0].x);
		ao += gSampleWeightTable[0].y * TestSamples(normal, pos, thisIdx, 2, 0, invThisDepth, gInvThicknessTable[0].y);
		ao += gSampleWeightTable[0].z * TestSamples(normal, pos, thisIdx, 3, 0, invThisDepth, gInvThicknessTable[0].z);
		ao += gSampleWeightTable[0].w * TestSamples(normal, pos, thisIdx, 4, 0, invThisDepth, gInvThicknessTable[0].w);
		ao += gSampleWeightTable[1].x * TestSamples(normal, pos, thisIdx, 1, 1, invThisDepth, gInvThicknessTable[1].x);
		ao += gSampleWeightTable[2].x * TestSamples(normal, pos, thisIdx, 2, 2, invThisDepth, gInvThicknessTable[2].x);
		ao += gSampleWeightTable[2].w * TestSamples(normal, pos, thisIdx, 3, 3, invThisDepth, gInvThicknessTable[2].w);
		ao += gSampleWeightTable[1].y * TestSamples(normal, pos, thisIdx, 1, 2, invThisDepth, gInvThicknessTable[1].y);
		ao += gSampleWeightTable[1].z * TestSamples(normal, pos, thisIdx, 1, 3, invThisDepth, gInvThicknessTable[1].z);
		ao += gSampleWeightTable[1].w * TestSamples(normal, pos, thisIdx, 1, 4, invThisDepth, gInvThicknessTable[1].w);
		ao += gSampleWeightTable[2].y * TestSamples(normal, pos, thisIdx, 2, 3, invThisDepth, gInvThicknessTable[2].y);
		ao += gSampleWeightTable[2].z * TestSamples(normal, pos, thisIdx, 2, 4, invThisDepth, gInvThicknessTable[2].z);
#else
		// 36 sample every-other cell in a checker board pattern
		ao += gSampleWeightTable[0].y * TestSamples(normal, pos, thisIdx, 2, 0, invThisDepth, gInvThicknessTable[0].y);
		ao += gSampleWeightTable[0].w * TestSamples(normal, pos, thisIdx, 4, 0, invThisDepth, gInvThicknessTable[0].w);
		ao += gSampleWeightTable[1].x * TestSamples(normal, pos, thisIdx, 1, 1, invThisDepth, gInvThicknessTable[1].x);
		ao += gSampleWeightTable[2].x * TestSamples(normal, pos, thisIdx, 2, 2, invThisDepth, gInvThicknessTable[2].x);
		ao += gSampleWeightTable[2].w * TestSamples(normal, pos, thisIdx, 3, 3, invThisDepth, gInvThicknessTable[2].w);
		ao += gSampleWeightTable[1].z * TestSamples(normal, pos, thisIdx, 1, 3, invThisDepth, gInvThicknessTable[1].z);
		ao += gSampleWeightTable[2].z * TestSamples(normal, pos, thisIdx, 2, 4, invThisDepth, gInvThicknessTable[2].z);
#endif
#ifdef USE_NORMAL_BUFFER
		ao  = 1.0f - (gNormalAOMultiply * ao * depth);
		ao *= ao;
#endif
#ifdef DONT_PROCESS_FAR_Z
	}
#endif

#ifdef INTERLEAVE_RESULT
	uint2 OutPixel = DTid.xy << 2 | uint2(DTid.z & 3, DTid.z >> 2);
#else
	uint2 OutPixel = DTid.xy;
#endif
	Occlusion[OutPixel] = ao;
}