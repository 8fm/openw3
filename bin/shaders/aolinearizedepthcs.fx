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

RW_TEXTURE2D<float>		LinearZ		: register(u0);
RW_TEXTURE2D<float>		_Unused1	: register(u1);
RW_TEXTURE2D<uint>		_Unused2	: register(u2);

#if _XBOX_ONE

#include "aodepthdecompressutility.fx"

START_CB( CustomConstantBuffer, 3 )
	uint2 TiledDimensions;		// For 1080p, this is 240x136
	float ZMagic;				// (zFar - zNear) / zNear
	float ZClear;
END_CB

float DecompressAndLinearize( uint2 tile, uint2 pixelCoord, uint linearIndex )
{
	float depth = DecompressDepth( tile, TiledDimensions, pixelCoord, linearIndex, ZClear );
	float dist = 1.0 / (ZMagic * depth + 1.0);
	LinearZ[tile << 3 | pixelCoord] = dist;
	return dist;
}

[NUMTHREADS( 8, 8, 1 )]
void cs_main( uint3 Gid : SYS_GROUP_ID, uint GI : SYS_GROUP_INDEX, uint3 GTid : SYS_GROUP_THREAD_ID, uint3 DTid : SYS_DISPATCH_THREAD_ID )
{
	uint linearIndex = GI & 63;
	uint2 pixelCoord = DepthCoordFromElementTable(linearIndex);
	uint destIdx = pixelCoord.x + pixelCoord.y * 16;
	uint2 tile1 = Gid.xy << 1;

	DecompressAndLinearize(tile1 | uint2(0, 0), pixelCoord, linearIndex);
	DecompressAndLinearize(tile1 | uint2(1, 0), pixelCoord, linearIndex);
	DecompressAndLinearize(tile1 | uint2(0, 1), pixelCoord, linearIndex);
	DecompressAndLinearize(tile1 | uint2(1, 1), pixelCoord, linearIndex);
}

#else

TEXTURE2D<float>		Depth		: register(t0);

START_CB( CustomConstantBuffer, 3 )
	float ZMagic;				// (zFar - zNear) / zNear
END_CB

[NUMTHREADS( 16, 16, 1 )]
void cs_main( uint3 Gid : SYS_GROUP_ID, uint GI : SYS_GROUP_INDEX, uint3 GTid : SYS_GROUP_THREAD_ID, uint3 DTid : SYS_DISPATCH_THREAD_ID )
{
	LinearZ[DTid.xy] = 1.0 / (ZMagic * Depth[DTid.xy] + 1.0);
}

#endif
