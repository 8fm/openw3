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
//

#include "common.fx"

TEXTURE2D<float>		LoResDB		: register(t0);
TEXTURE2D<float>		HiResDB		: register(t1);
TEXTURE2D<float>		LoResAO1	: register(t2);
#ifdef COMBINE_LOWER_RESOLUTIONS
TEXTURE2D<float>		LoResAO2	: register(t3);
#endif
#ifdef BLEND_WITH_HIGHER_RESOLUTION
TEXTURE2D<float>		HiResAO		: register(t4);
#endif

RW_TEXTURE2D<float>		AoResult	: register(u0);

SamplerState		LinearSampler	: register(s0);

START_CB( CustomConstantBuffer, 3 )
	float2	InvLowResolution;
	float2	InvHighResolution;
	float	NoiseFilterStrength;
	float	StepSize;
	float	kBlurTolerance;
	float	kUpsampleTolerance;
END_CB

// 16x16 pixels with an 8x8 center that we will be blurring writing out.  Each uint is two half floats.
GROUPSHARED float DepthCache[256];
GROUPSHARED float AOCache1[256];
#if defined(COMBINE_LOWER_RESOLUTIONS) && !defined(COMBINE_BEFORE_BLUR)
GROUPSHARED float AOCache2[256];
#endif

void PrefetchData( uint index, float2 uv )
{
	float4 AO1 = LoResAO1.Gather( LinearSampler, uv );

#ifdef COMBINE_LOWER_RESOLUTIONS
	float4 AO2 = LoResAO2.Gather( LinearSampler, uv );

	#ifdef COMBINE_BEFORE_BLUR
		#ifdef COMBINE_WITH_MUL
			AO1 *= AO2;
		#else
			AO1 = min(AO1, AO2);
		#endif
	#else
		AOCache2[index   ] = AO2.w;
		AOCache2[index+1 ] = AO2.z;
		AOCache2[index+16] = AO2.x;
		AOCache2[index+17] = AO2.y;
	#endif
#endif

	AOCache1[index   ] = AO1.w;
	AOCache1[index+ 1] = AO1.z;
	AOCache1[index+16] = AO1.x;
	AOCache1[index+17] = AO1.y;

	float4 ID = 1.0 / LoResDB.Gather( LinearSampler, uv );
	DepthCache[index   ] = ID.w;
	DepthCache[index+ 1] = ID.z;
	DepthCache[index+16] = ID.x;
	DepthCache[index+17] = ID.y;
}

float SmartBlur( float a, float b, float c, float d, float e, bool Left, int Middle, int Right )
{
	b = Left | Middle ? b : c;
	a = Left ? a : b;
	d = Right | Middle ? d : c;
	e = Right ? e : d;
	return ((a + e) / 2.0 + b + c + d) / 4.0;
}

bool CompareDeltas( float d1, float d2, float l1, float l2 )
{
	float temp = d1 * d2 + StepSize;
	return temp * temp > l1 * l2 * kBlurTolerance;
}

void BlurHorizontally( uint leftMostIndex )
{
	float a0 = AOCache1[leftMostIndex  ];
	float a1 = AOCache1[leftMostIndex+1];
	float a2 = AOCache1[leftMostIndex+2];
	float a3 = AOCache1[leftMostIndex+3];
	float a4 = AOCache1[leftMostIndex+4];
	float a5 = AOCache1[leftMostIndex+5];

	float d0 = DepthCache[leftMostIndex  ];
	float d1 = DepthCache[leftMostIndex+1];
	float d2 = DepthCache[leftMostIndex+2];
	float d3 = DepthCache[leftMostIndex+3];
	float d4 = DepthCache[leftMostIndex+4];
	float d5 = DepthCache[leftMostIndex+5];

	float d01 = d1 - d0;
	float d12 = d2 - d1;
	float d23 = d3 - d2;
	float d34 = d4 - d3;
	float d45 = d5 - d4;

	float l01 = d01 * d01 + StepSize;
	float l12 = d12 * d12 + StepSize;
	float l23 = d23 * d23 + StepSize;
	float l34 = d34 * d34 + StepSize;
	float l45 = d45 * d45 + StepSize;

	bool c02 = CompareDeltas( d01, d12, l01, l12 );
	bool c13 = CompareDeltas( d12, d23, l12, l23 );
	bool c24 = CompareDeltas( d23, d34, l23, l34 );
	bool c35 = CompareDeltas( d34, d45, l34, l45 );

	// needed in case thread size is not 64
#if !(_XBOX_ONE && __PSSL__)
	GROUP_BARRIER_GROUP_SYNC;
#endif
	AOCache1[leftMostIndex  ] = SmartBlur( a0, a1, a2, a3, a4, c02, c13, c24 );
	AOCache1[leftMostIndex+1] = SmartBlur( a1, a2, a3, a4, a5, c13, c24, c35 );
}

void BlurVertically( uint topMostIndex )
{
	float a0 = AOCache1[topMostIndex   ];
	float a1 = AOCache1[topMostIndex+16];
	float a2 = AOCache1[topMostIndex+32];
	float a3 = AOCache1[topMostIndex+48];
	float a4 = AOCache1[topMostIndex+64];
	float a5 = AOCache1[topMostIndex+80];

	float d0 = DepthCache[topMostIndex+ 2];
	float d1 = DepthCache[topMostIndex+18];
	float d2 = DepthCache[topMostIndex+34];
	float d3 = DepthCache[topMostIndex+50];
	float d4 = DepthCache[topMostIndex+66];
	float d5 = DepthCache[topMostIndex+82];

	float d01 = d1 - d0;
	float d12 = d2 - d1;
	float d23 = d3 - d2;
	float d34 = d4 - d3;
	float d45 = d5 - d4;

	float l01 = d01 * d01 + StepSize;
	float l12 = d12 * d12 + StepSize;
	float l23 = d23 * d23 + StepSize;
	float l34 = d34 * d34 + StepSize;
	float l45 = d45 * d45 + StepSize;

	bool c02 = CompareDeltas( d01, d12, l01, l12 );
	bool c13 = CompareDeltas( d12, d23, l12, l23 );
	bool c24 = CompareDeltas( d23, d34, l23, l34 );
	bool c35 = CompareDeltas( d34, d45, l34, l45 );

	float aoResult1 = SmartBlur( a0, a1, a2, a3, a4, c02, c13, c24 );
	float aoResult2 = SmartBlur( a1, a2, a3, a4, a5, c13, c24, c35 );

#if defined(COMBINE_LOWER_RESOLUTIONS) && !defined(COMBINE_BEFORE_BLUR)
#ifdef COMBINE_WITH_MUL
	aoResult1 *= AOCache2[topMostIndex + 32 + 2];
	aoResult2 *= AOCache2[topMostIndex + 48 + 2];
#else
	aoResult1 = min(aoResult1, AOCache2[topMostIndex + 32 + 2]);
	aoResult2 = min(aoResult2, AOCache2[topMostIndex + 48 + 2]);
#endif
#endif

	// needed in case thread size is not 64
#if !(_XBOX_ONE && __PSSL__)
	GROUP_BARRIER_GROUP_SYNC;
#endif
	AOCache1[topMostIndex   ] = aoResult1;
	AOCache1[topMostIndex+16] = aoResult2;
}

// We essentially want 5 weights:  4 for each low-res pixel and 1 to blend in when none of the 4 really
// match.  The filter strength is 1 / DeltaZTolerance.  So a tolerance of 0.01 would yield a strength of 100.
// Note that a perfect match of low to high depths would yield a weight of 10^6, completely superceding any
// noise filtering.  The noise filter is intended to soften the effects of shimmering when the high-res depth
// buffer has a lot of small holes in it causing the low-res depth buffer to inaccurately represent it.
float BilateralUpsample( float HiDepth, float HiAO, float4 LowDepths, float4 LowAO )
{
	float4 weights = float4(9, 3, 1, 3) / ( abs(HiDepth - LowDepths) + kUpsampleTolerance );
	float TotalWeight = dot(weights, 1) + NoiseFilterStrength;
	float WeightedSum = dot(LowAO, weights) + NoiseFilterStrength;// * HiAO;
	return HiAO * WeightedSum / TotalWeight;
}

[NUMTHREADS( 8, 8, 1 )]
void cs_main( uint3 Gid : SYS_GROUP_ID, uint GI : SYS_GROUP_INDEX, uint3 GTid : SYS_GROUP_THREAD_ID, uint3 DTid : SYS_DISPATCH_THREAD_ID )
{
	//
	// Load 4 pixels per thread into LDS to fill the 16x16 LDS cache with depth and AO
	//
	PrefetchData( GTid.x << 1 | GTid.y << 5, int2(DTid.xy + GTid.xy - 2) * InvLowResolution );
	GROUP_BARRIER_GROUP_SYNC;

	//
	// Horizontally blur the pixels.
	//
	BlurHorizontally((GI / 5) * 16 + (GI % 5) * 2);
	GROUP_BARRIER_GROUP_SYNC;

	//
	// Vertically blur the pixels.
	//
	BlurVertically((GI / 10) * 32 + GI % 10);
	GROUP_BARRIER_GROUP_SYNC;

	//
	// Bilateral upsample
	//
	uint Idx0 = GTid.x + GTid.y * 16;
	float4 LoSSAOs = float4( AOCache1[Idx0+16], AOCache1[Idx0+17], AOCache1[Idx0+1], AOCache1[Idx0] );

	// We work on a quad of pixels at once because then we can gather 4 each of high and low-res depth values
	float2 UV0 = DTid.xy * InvLowResolution;
	float2 UV1 = DTid.xy * 2 * InvHighResolution;

#ifdef BLEND_WITH_HIGHER_RESOLUTION
	float4 HiSSAOs  = HiResAO.Gather(LinearSampler, UV1);
#else
	float4 HiSSAOs	= 1.0;
#endif
	float4 LoDepths = LoResDB.Gather(LinearSampler, UV0);
	float4 HiDepths = HiResDB.Gather(LinearSampler, UV1);

	int2 OutST = asint(DTid.xy << 1);
	AoResult[OutST + int2(-1,  0)] = BilateralUpsample( HiDepths.x, HiSSAOs.x, LoDepths.xyzw, LoSSAOs.xyzw );
	AoResult[OutST + int2( 0,  0)] = BilateralUpsample( HiDepths.y, HiSSAOs.y, LoDepths.yzwx, LoSSAOs.yzwx );
	AoResult[OutST + int2( 0, -1)] = BilateralUpsample( HiDepths.z, HiSSAOs.z, LoDepths.zwxy, LoSSAOs.zwxy );
	AoResult[OutST + int2(-1, -1)] = BilateralUpsample( HiDepths.w, HiSSAOs.w, LoDepths.wxyz, LoSSAOs.wxyz );
}