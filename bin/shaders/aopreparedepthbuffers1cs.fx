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

RW_TEXTURE2D<float>				LinearZ			: register(u0);
RW_TEXTURE2D<float2>			DS2x			: register(u1);
RW_TEXTURE2D_ARRAY<float>		DS2xAtlas		: register(u2);
RW_TEXTURE2D<float2>			DS4x			: register(u3);
RW_TEXTURE2D_ARRAY<float>		DS4xAtlas		: register(u4);
#ifdef USE_NORMALS
RW_TEXTURE2D<float3>			Normals2x		: register(u5);
RW_TEXTURE2D_ARRAY<float3>		Normals2xAtlas	: register(u6);
#endif

START_CB( CustomConstantBuffer, 3 )
	float ZMagic;
	float ZClear;
	uint2 TiledDimensions;		// For 1080p, this is 240x136
END_CB

#define DEPTH_ALREADY_DECOMPRESSED	1

#if _XBOX_ONE && !DEPTH_ALREADY_DECOMPRESSED

#ifdef USE_NORMALS
TEXTURE2D<float3>			NormalsSrc		: register(t4);
#endif

#include "aodepthdecompressutility.fx"

float DecompressAndLinearize( uint2 tile, uint2 pixelCoord, uint linearIndex )
{
	float depth = DecompressDepth( tile, TiledDimensions, pixelCoord, linearIndex, ZClear );
	float dist = 1.0 / (ZMagic * depth + 1.0);
	LinearZ[tile << 3 | pixelCoord] = dist;
	return dist;
}

#else

TEXTURE2D<float>		Depth		: register(t0);
#ifdef USE_NORMALS
TEXTURE2D<float3>		NormalsSrc	: register(t1);
#endif

float Linearize( uint2 st )
{
	float depth = Depth[st];
	float dist = 1.0 / (ZMagic * depth + 1.0);
	LinearZ[st] = dist;
	return dist;
}

#endif

GROUPSHARED float g_CacheW[256];
#ifdef USE_NORMALS
GROUPSHARED float3 g_CacheNormals[256];
#endif

#ifdef USE_MAX_DEPTH

float MaxDepth( float depth1, float depth2, float depth3, float depth4 )
{
#if _XBOX_ONE
	return __XB_Max3_F32(depth1.x, depth2.x, max(depth3.x, depth4.x));
#else
	return max(max(depth1.x, depth2.x), max(depth3.x, depth4.x));
#endif
}

#endif


#ifdef USE_NORMALS
float3 DecodeNormal(float3 normal)
{
	normal  = normalize(normal - 0.5f);		// best fit normals
	normal += 1.0f;
	normal *= 0.5f;

	return normal;
}
#endif


[NUMTHREADS( 8, 8, 1 )]
void cs_main( uint3 Gid : SYS_GROUP_ID, uint GI : SYS_GROUP_INDEX, uint3 GTid : SYS_GROUP_THREAD_ID, uint3 DTid : SYS_DISPATCH_THREAD_ID )
{
#if _XBOX_ONE && !DEPTH_ALREADY_DECOMPRESSED
	uint linearIndex = GI & 63;
	uint2 pixelCoord = DepthCoordFromElementTable(linearIndex);
	uint destIdx = pixelCoord.x + pixelCoord.y * 16;
	uint2 tile1 = Gid.xy << 1;

	g_CacheW[ destIdx +  0  ] = DecompressAndLinearize(tile1 | uint2(0, 0), pixelCoord, linearIndex);
	g_CacheW[ destIdx +  8  ] = DecompressAndLinearize(tile1 | uint2(1, 0), pixelCoord, linearIndex);
	g_CacheW[ destIdx + 128 ] = DecompressAndLinearize(tile1 | uint2(0, 1), pixelCoord, linearIndex);
	g_CacheW[ destIdx + 136 ] = DecompressAndLinearize(tile1 | uint2(1, 1), pixelCoord, linearIndex);

# ifdef USE_NORMALS
	g_CacheNormals[ destIdx +  0  ] = DecodeNormal(NormalsSrc.Load(int3(((tile1 | uint2(0, 0)) << 3) | pixelCoord, 0)));
	g_CacheNormals[ destIdx +  8  ] = DecodeNormal(NormalsSrc.Load(int3(((tile1 | uint2(1, 0)) << 3) | pixelCoord, 0)));
	g_CacheNormals[ destIdx + 128 ] = DecodeNormal(NormalsSrc.Load(int3(((tile1 | uint2(0, 1)) << 3) | pixelCoord, 0)));
	g_CacheNormals[ destIdx + 136 ] = DecodeNormal(NormalsSrc.Load(int3(((tile1 | uint2(1, 1)) << 3) | pixelCoord, 0)));
# endif
#else
	uint2 startST = Gid.xy << 4 | GTid.xy;
	uint destIdx = GTid.y << 4 | GTid.x;
	g_CacheW[ destIdx +  0  ] = Linearize(startST | uint2(0, 0));
	g_CacheW[ destIdx +  8  ] = Linearize(startST | uint2(8, 0));
	g_CacheW[ destIdx + 128 ] = Linearize(startST | uint2(0, 8));
	g_CacheW[ destIdx + 136 ] = Linearize(startST | uint2(8, 8));
# ifdef USE_NORMALS
	g_CacheNormals[ destIdx +  0  ] = DecodeNormal(NormalsSrc[startST | uint2(0, 0)]);
	g_CacheNormals[ destIdx +  8  ] = DecodeNormal(NormalsSrc[startST | uint2(8, 0)]);
	g_CacheNormals[ destIdx + 128 ] = DecodeNormal(NormalsSrc[startST | uint2(0, 8)]);
	g_CacheNormals[ destIdx + 136 ] = DecodeNormal(NormalsSrc[startST | uint2(8, 8)]);
# endif
#endif

	GROUP_BARRIER_GROUP_SYNC;

	uint ldsIndex = (GTid.x << 1) | (GTid.y << 5);

#ifdef USE_MAX_DEPTH
	float w1 = g_CacheW[ldsIndex];
	float w2 = g_CacheW[ldsIndex+1];
	float w3 = g_CacheW[ldsIndex+16];
	float w4 = g_CacheW[ldsIndex+17];
	w1 = MaxDepth( w1, w2, w3, w4 );
# ifdef USE_NORMALS
	#error To implement
# endif
#else
	uint choiceOffset = (GTid.x & 1) << 4 | (GTid.y & 1);
	float w1 = g_CacheW[ldsIndex + choiceOffset];
# ifdef USE_NORMALS
	float3 n1 = g_CacheNormals[ldsIndex + choiceOffset];
# endif
#endif
	g_CacheW[ldsIndex] = w1;
#ifdef USE_NORMALS
	g_CacheNormals[ldsIndex] = n1;
#endif

	GROUP_BARRIER_GROUP_SYNC;

	uint2 st = DTid.xy;
	uint slice = (st.x & 3) | ((st.y & 3) << 2);

	DS2x[st] = w1;
	DS2xAtlas[uint3(st >> 2, slice)] = w1;
#ifdef USE_NORMALS
	Normals2x[st] = n1;
	Normals2xAtlas[uint3(st >> 2, slice)] = n1;
#endif

	if ((GI & 011) == 0)
	{
#ifdef USE_MAX_DEPTH
		w2 = g_CacheW[ldsIndex+2];
		w3 = g_CacheW[ldsIndex+32];
		w4 = g_CacheW[ldsIndex+34];
		w1 = MaxDepth(w1, w2, w3, w4);
#else
		choiceOffset = (GTid.x & 2) << 4 | (GTid.y & 2);
		w1 = g_CacheW[ldsIndex + choiceOffset];
#endif
		st = DTid.xy >> 1;
		slice = (st.x & 3) | ((st.y & 3) << 2);
		DS4x[st] = w1;
		DS4xAtlas[uint3(st >> 2, slice)] = w1;
	}
}
