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

TEXTURE2D<float>			DS4x		: register(t0);
RW_TEXTURE2D<float>			DS8x		: register(u0);
RW_TEXTURE2D_ARRAY<float>	DS8xAtlas	: register(u1);
RW_TEXTURE2D<float>			DS16x		: register(u2);
RW_TEXTURE2D_ARRAY<float>	DS16xAtlas	: register(u3);

SamplerState		LinearSampler	: register(s0);

START_CB( CustomConstantBuffer, 3 )
	float2 InvSourceDimension;
END_CB

#if _XBOX_ONE
// Instead of using shader ALU to min/max, you can use a sampler mode that returns the min/max of the four samples.
// This is exposed on Xbox One with D3D11X_SAMPLER_DESC.
SamplerState		MaxSampler		: register(s1);
#endif

#ifdef USE_MAX_DEPTH

float MaxDepth( float d1, float d2, float d3, float d4 )
{
#if _XBOX_ONE
	return __XB_Max3_F32(d1, d2, max(d3, d4));
#else
	return max(max(d1, d2), max(d3, d4));
#endif
}

float MaxDepth( float4 depths )
{
	return MaxDepth(depths.x, depths.y, depths.z, depths.w);
}

#endif	// USE_MAX_DEPTH

GROUPSHARED float g_cache[64];

[NUMTHREADS( 8, 8, 1 )]
void cs_main( uint3 Gid : SYS_GROUP_ID, uint GI : SYS_GROUP_INDEX, uint3 GTid : SYS_GROUP_THREAD_ID, uint3 DTid : SYS_DISPATCH_THREAD_ID )
{
#ifdef USE_MAX_DEPTH
	float2 uv = (DTid.xy << 1 | 1) * InvSourceDimension;
#if _XBOX_ONE
	float m1 = DS4x.SampleLevel(MaxSampler, uv, 0);
#else
	float m1 = MaxDepth( DS4x.Gather(LinearSampler, uv) );
#endif
#else	// !USE_MAX_DEPTH
	float depth = DS4x[DTid.xy << 1 | (GTid.xy & 1)];
	float m1 = depth;
#endif
	{
		uint2 st = DTid.xy;
		uint2 stAtlas = st >> 2;
		uint stSlice = (st.x & 3) | ((st.y & 3) << 2);
		DS8x[st] = m1;
		DS8xAtlas[uint3(stAtlas, stSlice)] = m1;
		g_cache[GI] = m1;
	}

	GROUP_BARRIER_GROUP_SYNC;

	if ((GI & 011) == 0)
	{
#ifdef USE_MAX_DEPTH
		float m2 = g_cache[GI+1];
		float m3 = g_cache[GI+8];
		float m4 = g_cache[GI+9];
		m1 = MaxDepth(m1, m2, m3, m4);
#else
		uint choiceOffset = (GTid.x & 2) << 2 | (GTid.y & 2) >> 1;
		m1 = g_cache[GI + choiceOffset];
#endif
		uint2 st = DTid.xy >> 1;
		uint2 stAtlas = st >> 2;
		uint stSlice = (st.x & 3) | ((st.y & 3) << 2);
		DS16x[st] = m1;
		DS16xAtlas[uint3(stAtlas, stSlice)] = m1;
	}
}
