// TODO: if enabled, use bit accurate integer math for reconstructing depth from zPlanes
//#define BIT_ACCURATE

#ifndef SAMPLE_COUNT
	#define SAMPLE_COUNT				1
#endif

#if (SAMPLE_COUNT > 1)
	#define ENABLE_MSAA
	#define SAMPLE_ID					0
#endif

#if defined(ENABLE_MSAA)
	TEXTURE2D_MS<float> depthSrc	: register(t0);
	TEXTURE2D_MS<uint> stencilSrc	: register(t1);
#else
	TEXTURE2D<float> depthSrc		: register(t0);
	TEXTURE2D<uint> stencilSrc		: register(t1);
#endif
STRUCTBUFFER(uint) zDataLayout		: register(t2);
BYTEBUFFER hTileBuffer				: register(t3);

// P4_16x16
uint GetHTileAddress(uint2 tileCoord, uint2 numTiles)
{
	const uint pipeCount = 4;
	// cache line tiling?
	const bool linearMode = true;

	// in units of tiles
	const uint clWidth = 64;
	const uint clHeight = 32;
	const uint clSize = 512; // per pipe
	
	const uint clCountX = numTiles.x / clWidth; // clPitch
	//const uint clCountY = numTiles.y / clHeight;
	
	const uint slicePitch = numTiles.x * numTiles.y / pipeCount;
	const uint macroPitch = (linearMode ? numTiles.x : clWidth) / 4;
	
	// cache line
	uint clX = tileCoord.x / clWidth;
	uint clY = tileCoord.y / clHeight;
	
	// for 2D array and 3D textures (including cube maps)
	uint tileSlice = 0; // tileZ
	uint sliceOffset = slicePitch * tileSlice;
	
	// cache line tiling (unused for linearMode)
	uint clOffset = linearMode ? 0 : (clX + clCountX * clY) * clSize;
	
	uint tileX0 = tileCoord.x & 1;
	uint tileY0 = tileCoord.y & 1;
	uint tileX1 = (tileCoord.x >> 1) & 1;
	uint tileY1 = (tileCoord.y >> 1) & 1;
	// specific to pipe config
	uint tileOffset = (tileX1 ^ tileY0) | (tileX1 << 1);
	//uint tileOffset = (tileX1 ^ tileY0) | (tileCoord.x & 2);

	// specific to pipe config
	uint pipe = (tileX0 ^ tileY0 ^ tileX1) | ((tileX1 ^ tileY1) << 1);
	//uint pipe = (tileX0 | (tileCoord.y & 2)) ^ tileOffset;
	//uint pipe = (tileX0 | tileY1 << 1) ^ tileOffset;

	// macro (4x4 tile) tiling
	uint macroX = (linearMode ? tileCoord.x : (tileCoord.x % clWidth)) / 4;
	uint macroY = (linearMode ? tileCoord.y : (tileCoord.y % clHeight)) / 4;
	uint macroOffset = (macroX + macroY * macroPitch) << 2 | tileOffset;
	
	uint tileIndex = sliceOffset + clOffset + macroOffset;
	uint tileByteOffset = tileIndex << 2;

	
	// The 2-bit pipe value gets inserted into the tileByteOffset at bits 8 and 9
	return (tileByteOffset & ~0xff) << 2 | (pipe << 8) | (tileByteOffset & 0xff);
}

// the micro tile index for the given coord
uint DepthElementFromCoord(uint2 coord)
{
	return
		(((coord.x >> 0) & 1) << 0) | (((coord.y >> 0) & 1) << 1) | (((coord.x >> 1) & 1) << 2) |
		(((coord.y >> 1) & 1) << 3) | (((coord.x >> 2) & 1) << 4) | (((coord.y >> 2) & 1) << 5);
}


uint DepthElementFromCoordTable(uint2 coord)
{
	const uint lookup[64] =
	{
		DepthElementFromCoord(uint2(0, 0)),
		DepthElementFromCoord(uint2(1, 0)),
		DepthElementFromCoord(uint2(2, 0)),
		DepthElementFromCoord(uint2(3, 0)),
		DepthElementFromCoord(uint2(4, 0)),
		DepthElementFromCoord(uint2(5, 0)),
		DepthElementFromCoord(uint2(6, 0)),
		DepthElementFromCoord(uint2(7, 0)),
		
		DepthElementFromCoord(uint2(0, 1)),
		DepthElementFromCoord(uint2(1, 1)),
		DepthElementFromCoord(uint2(2, 1)),
		DepthElementFromCoord(uint2(3, 1)),
		DepthElementFromCoord(uint2(4, 1)),
		DepthElementFromCoord(uint2(5, 1)),
		DepthElementFromCoord(uint2(6, 1)),
		DepthElementFromCoord(uint2(7, 1)),
		
		DepthElementFromCoord(uint2(0, 2)),
		DepthElementFromCoord(uint2(1, 2)),
		DepthElementFromCoord(uint2(2, 2)),
		DepthElementFromCoord(uint2(3, 2)),
		DepthElementFromCoord(uint2(4, 2)),
		DepthElementFromCoord(uint2(5, 2)),
		DepthElementFromCoord(uint2(6, 2)),
		DepthElementFromCoord(uint2(7, 2)),
		
		DepthElementFromCoord(uint2(0, 3)),
		DepthElementFromCoord(uint2(1, 3)),
		DepthElementFromCoord(uint2(2, 3)),
		DepthElementFromCoord(uint2(3, 3)),
		DepthElementFromCoord(uint2(4, 3)),
		DepthElementFromCoord(uint2(5, 3)),
		DepthElementFromCoord(uint2(6, 3)),
		DepthElementFromCoord(uint2(7, 3)),
		
		DepthElementFromCoord(uint2(0, 4)),
		DepthElementFromCoord(uint2(1, 4)),
		DepthElementFromCoord(uint2(2, 4)),
		DepthElementFromCoord(uint2(3, 4)),
		DepthElementFromCoord(uint2(4, 4)),
		DepthElementFromCoord(uint2(5, 4)),
		DepthElementFromCoord(uint2(6, 4)),
		DepthElementFromCoord(uint2(7, 4)),
		
		DepthElementFromCoord(uint2(0, 5)),
		DepthElementFromCoord(uint2(1, 5)),
		DepthElementFromCoord(uint2(2, 5)),
		DepthElementFromCoord(uint2(3, 5)),
		DepthElementFromCoord(uint2(4, 5)),
		DepthElementFromCoord(uint2(5, 5)),
		DepthElementFromCoord(uint2(6, 5)),
		DepthElementFromCoord(uint2(7, 5)),
		
		DepthElementFromCoord(uint2(0, 6)),
		DepthElementFromCoord(uint2(1, 6)),
		DepthElementFromCoord(uint2(2, 6)),
		DepthElementFromCoord(uint2(3, 6)),
		DepthElementFromCoord(uint2(4, 6)),
		DepthElementFromCoord(uint2(5, 6)),
		DepthElementFromCoord(uint2(6, 6)),
		DepthElementFromCoord(uint2(7, 6)),
		
		DepthElementFromCoord(uint2(0, 7)),
		DepthElementFromCoord(uint2(1, 7)),
		DepthElementFromCoord(uint2(2, 7)),
		DepthElementFromCoord(uint2(3, 7)),
		DepthElementFromCoord(uint2(4, 7)),
		DepthElementFromCoord(uint2(5, 7)),
		DepthElementFromCoord(uint2(6, 7)),
		DepthElementFromCoord(uint2(7, 7)),
	};

	return lookup[coord.x + coord.y * 8];
}

// the coord for the given micro tile index
uint2 DepthCoordFromElement(uint element)
{
	return uint2(
		(((element >> 0) & 1) << 0) | (((element >> 2) & 1) << 1) | (((element >> 4) & 1) << 2),
		(((element >> 1) & 1) << 0) | (((element >> 3) & 1) << 1) | (((element >> 5) & 1) << 2)
		);
}

uint2 DepthCoordFromElementTable(uint element)
{
	const uint2 lookup[64] =
	{
		DepthCoordFromElement(8 * 0 + 0),
		DepthCoordFromElement(8 * 0 + 1),
		DepthCoordFromElement(8 * 0 + 2),
		DepthCoordFromElement(8 * 0 + 3),
		DepthCoordFromElement(8 * 0 + 4),
		DepthCoordFromElement(8 * 0 + 5),
		DepthCoordFromElement(8 * 0 + 6),
		DepthCoordFromElement(8 * 0 + 7),
		
		DepthCoordFromElement(8 * 1 + 0),
		DepthCoordFromElement(8 * 1 + 1),
		DepthCoordFromElement(8 * 1 + 2),
		DepthCoordFromElement(8 * 1 + 3),
		DepthCoordFromElement(8 * 1 + 4),
		DepthCoordFromElement(8 * 1 + 5),
		DepthCoordFromElement(8 * 1 + 6),
		DepthCoordFromElement(8 * 1 + 7),
		
		DepthCoordFromElement(8 * 2 + 0),
		DepthCoordFromElement(8 * 2 + 1),
		DepthCoordFromElement(8 * 2 + 2),
		DepthCoordFromElement(8 * 2 + 3),
		DepthCoordFromElement(8 * 2 + 4),
		DepthCoordFromElement(8 * 2 + 5),
		DepthCoordFromElement(8 * 2 + 6),
		DepthCoordFromElement(8 * 2 + 7),
		
		DepthCoordFromElement(8 * 3 + 0),
		DepthCoordFromElement(8 * 3 + 1),
		DepthCoordFromElement(8 * 3 + 2),
		DepthCoordFromElement(8 * 3 + 3),
		DepthCoordFromElement(8 * 3 + 4),
		DepthCoordFromElement(8 * 3 + 5),
		DepthCoordFromElement(8 * 3 + 6),
		DepthCoordFromElement(8 * 3 + 7),
		
		DepthCoordFromElement(8 * 4 + 0),
		DepthCoordFromElement(8 * 4 + 1),
		DepthCoordFromElement(8 * 4 + 2),
		DepthCoordFromElement(8 * 4 + 3),
		DepthCoordFromElement(8 * 4 + 4),
		DepthCoordFromElement(8 * 4 + 5),
		DepthCoordFromElement(8 * 4 + 6),
		DepthCoordFromElement(8 * 4 + 7),
		
		DepthCoordFromElement(8 * 5 + 0),
		DepthCoordFromElement(8 * 5 + 1),
		DepthCoordFromElement(8 * 5 + 2),
		DepthCoordFromElement(8 * 5 + 3),
		DepthCoordFromElement(8 * 5 + 4),
		DepthCoordFromElement(8 * 5 + 5),
		DepthCoordFromElement(8 * 5 + 6),
		DepthCoordFromElement(8 * 5 + 7),
		
		DepthCoordFromElement(8 * 6 + 0),
		DepthCoordFromElement(8 * 6 + 1),
		DepthCoordFromElement(8 * 6 + 2),
		DepthCoordFromElement(8 * 6 + 3),
		DepthCoordFromElement(8 * 6 + 4),
		DepthCoordFromElement(8 * 6 + 5),
		DepthCoordFromElement(8 * 6 + 6),
		DepthCoordFromElement(8 * 6 + 7),
		
		DepthCoordFromElement(8 * 7 + 0),
		DepthCoordFromElement(8 * 7 + 1),
		DepthCoordFromElement(8 * 7 + 2),
		DepthCoordFromElement(8 * 7 + 3),
		DepthCoordFromElement(8 * 7 + 4),
		DepthCoordFromElement(8 * 7 + 5),
		DepthCoordFromElement(8 * 7 + 6),
		DepthCoordFromElement(8 * 7 + 7),
	};

	return lookup[element];
}

float getDepthFromZPlane(uint3 zPlane, int2 subPixelPos)
{
#if defined(ENABLE_MSAA)
	subPixelPos -= 64; // [0, 127] to [-64, 63]

	// 28-bit signed ints for slopeX and slopeY.  The remaining upper 8 bits are an exponent.
	// We must sign extend by shifting up and down.
	int centerZ = int(zPlane.z << 1); // sign extend
	int slopeX = int(zPlane.x << 4) >> 4; // sign extend
	int slopeY = int(zPlane.y << 8) >> 4 | int(zPlane.x >> 28); // sign extend
	float scale = asfloat(((zPlane.y >> 24) - 26) << 23);
	uint multisampled = (zPlane.z >> 31);

	if (!multisampled)
	{
		// multisampling was disabled for this plane, use the pixel center
		subPixelPos = (subPixelPos & ~7) | 8;
	}

	// When we have 24-bit integer multiply in the compiler...
	//float depth = float(centerZ + slopeX * subPixelPos.x + slopeY * subPixelPos.y);
	float depth = float(centerZ) * 8.0f + dot(float2(slopeX, slopeY), float2(subPixelPos));
#else
	subPixelPos -= 8; // [0, 15] to [-8, 7]  (actually {-7, -5, -3, -1, 1, 3, 5, 7};  0 is the location of centerZ)

	// In this implementation, we discard 4 bits of each zPlane element (leaving 24 inc. sign bit)
	// A float32 can only hold 24 bits of precision, so this is sufficient.  Also, we have cheap 24-bit integer
	// multiplies on Xbox One.
	int centerZ = int(zPlane.z << 1) >> 4;	// sign extend		(28-bit)
	int slopeX = int(zPlane.x << 4) >> 8;	// sign extend		(24-bit)
	int slopeY = int(zPlane.y << 8) >> 8;	// sign extend		(24-bit)

	// The shared exponent is stored in the upper 8 bits of zPlane.y.  We want to bias it by -26 + 7
	float scale = asfloat(((zPlane.y >> 24) - 19) << 23);

#if _XBOX_ONE
	// Do we really ever do this anywhere but on Xbox One?  How about comparing XDK revisions for when these HLSL
	// intrinsics for 24-bit integer multiplies became available.  (P.S. The Sea Islands documentation says that
	// the 3rd argument to MadI24 can be 32-bit.)
	int temp = __XB_MadI24(slopeY, subPixelPos.y, centerZ);
	float depth = float(__XB_MadI24(slopeX, subPixelPos.x, temp));
#else
	float depth = float(centerZ) + dot(float2(slopeX, slopeY), float2(subPixelPos));
#endif
#endif

	return saturate(depth * scale);
}

#define DEPTH_UINT_COUNT (64 * SAMPLE_COUNT)
GROUPSHARED uint sharedDepthData[DEPTH_UINT_COUNT];

float DecompressDepth( uint2 tileCoord, uint2 numTiles, uint2 pixelCoord, uint threadIndex, float depthClear = 0.0 )
{
	// load depth data
// TODO: for higher MSAA levels (4x for sure, 2x?) only load the worst-case
// number of bytes for a compressed tile here. In 4x MSAA case, that'd be
// 192 bytes for zPlane and 128 bytes (256-byte aligned) for pMask, better than
// loading a full 1024-byte tile. Note that this would complicate the case where
// the tile is already decompressed a bit, since we'd have to do another
// fetch to grab the decompressed value rather than counting on it already
// being in LDS.
	uint2 globalPixel = tileCoord << 3 | pixelCoord;
#if defined(ENABLE_MSAA)
	[unroll]
	for (uint s = 0; s < SAMPLE_COUNT; s++)
		sharedDepthData[threadIndex * SAMPLE_COUNT + s] = asuint(depthSrc.Load(globalPixel, s));
#else
	sharedDepthData[threadIndex] = asuint(depthSrc[globalPixel]);
#endif
	
	// load hTile
	uint tileAddress = GetHTileAddress(tileCoord, numTiles);
	uint hTileValue = hTileBuffer.Load(tileAddress);
	uint zMask = hTileValue & 15;
	
	GROUP_BARRIER_GROUP_SYNC;
	
	if (zMask == 0)
	{
		// tile is clear
		// write the clear value, and early-out
		return depthClear;
	}
	else if (zMask == 15)
	{
		// tile is already decompressed
#if defined(ENABLE_MSAA)
		return asfloat(sharedDepthData[threadIndex * SAMPLE_COUNT + SAMPLE_ID]);
#else
		return asfloat(sharedDepthData[threadIndex]);
#endif
	}

	uint packedValue = zDataLayout[zMask];
	uint zPlaneCount = packedValue & 31;				// 5 bits
	uint pMaskElementSizeBits = (packedValue >> 5) & 7;	// 3 bits (but has values of 1 - 4)
	uint is32ByteChunk = (packedValue >> 8) & 1;		// 1 bit
	uint pMaskOffsetElement = (packedValue >> 9) & 127;	// 7 bits
	uint maxLoadIndex_WithoutZDataOffset = (packedValue >> 16) & 255;	// 8 bits
	
	// not sure where the HW docs refer to this...
	// also, if TILE_COUNT_X is not even, this may need to be (tileCoord.x + tileCoord.y * TILE_COUNT_X) & 1
	uint zDataOffset = (is32ByteChunk & tileCoord.x) << 3;

	uint zPlaneIndex = 0;
	if (zPlaneCount > 1)
	{
#if defined(ENABLE_MSAA)
		uint element = threadIndex * SAMPLE_COUNT + SAMPLE_ID;
		pMaskElementSizeBits = max(pMaskElementSizeBits, 2); // not sure what's going on here
#else
		uint element = threadIndex;
#endif
		uint bitOffset = element * pMaskElementSizeBits;
		uint PlaneIndexOffset = zDataOffset + pMaskOffsetElement + (bitOffset >> 5);

		bitOffset &= 31;

		zPlaneIndex = sharedDepthData[PlaneIndexOffset] >> bitOffset;
		uint bitsRead = 32 - bitOffset;
		if (pMaskElementSizeBits > bitsRead)
			zPlaneIndex |= sharedDepthData[PlaneIndexOffset + 1] << bitsRead;

		// These are equivalent, but I hope the compiler will generate a S_BFE_U32 instruction either way.
		//zPlaneIndex &= ~(0xffffffff << pMaskElementSizeBits);
		zPlaneIndex &= ((1 << pMaskElementSizeBits) - 1);
	}

	uint PlaneOffset = zDataOffset + zPlaneIndex * 3;
	uint3 zPlane = uint3(sharedDepthData[PlaneOffset], sharedDepthData[PlaneOffset + 1], sharedDepthData[PlaneOffset + 2]);
	
#if defined(ENABLE_MSAA)

#if __PSSL__
	int2 subPixelPos = pixelCoord * 16 + 8 + int2(depthSrc.GetSamplePoint(SAMPLE_ID) * 16.0f);
#else
	int2 subPixelPos = pixelCoord * 16 + 8 + int2(depthSrc.GetSamplePosition(SAMPLE_ID) * 16.0f);
#endif

#else
	int2 subPixelPos = pixelCoord << 1 | 1;
#endif

	return getDepthFromZPlane(zPlane, subPixelPos);
}

void DecompressDepthAndStencil( out float depth, out uint stencil,
	uint2 globalID, uint2 tileCoord, uint2 numTiles, uint2 pixelCoord, uint threadIndex,
	float depthClear = 0.0, uint stencilClear = 0 )
{
	// load depth data
// TODO: for higher MSAA levels (4x for sure, 2x?) only load the worst-case
// number of bytes for a compressed tile here. In 4x MSAA case, that'd be
// 192 bytes for zPlane and 128 bytes (256-byte aligned) for pMask, better than
// loading a full 1024-byte tile. Note that this would complicate the case where
// the tile is already decompressed a bit, since we'd have to do another
// fetch to grab the decompressed value rather than counting on it already
// being in LDS.
	uint2 globalPixel = tileCoord << 3 | pixelCoord;
#if defined(ENABLE_MSAA)
	[unroll]
	for (uint s = 0; s < SAMPLE_COUNT; s++)
		sharedDepthData[threadIndex * SAMPLE_COUNT + s] = asuint(depthSrc.Load(globalPixel, s));
#else
	sharedDepthData[threadIndex] = asuint(depthSrc[globalPixel]);
#endif
	
	// load hTile
	uint tileAddress = GetHTileAddress(tileCoord, numTiles);
	uint hTileValue = hTileBuffer.Load(tileAddress);
	uint zMask = hTileValue & 15;
	uint stencilState = (hTileValue >> 8) & 3;
	
	GROUP_BARRIER_GROUP_SYNC;
	
	// stencil
	if (stencilState == 0)
	{
		// clear
		stencil = stencilClear;
	}
	else if (stencilState == 1)
	{
		// compressed
		// grab the first stencil value and replicate it
		uint offset = 0;
		if ((tileCoord.x & 1) != 0)
			offset = 32 / SAMPLE_COUNT; // need to alternate with a 32-byte offset, similar to Z below.
		
		uint2 stencilTilePixel0 = DepthCoordFromElementTable(offset);
		uint2 stencilGlobalPixel0 = tileCoord * 8 + stencilTilePixel0;

# if defined(ENABLE_MSAA)
		stencil = stencilSrc.Load(stencilGlobalPixel0.xy, 0);
# else
		stencil = stencilSrc[stencilGlobalPixel0.xy];
# endif
	}
	else
	{
		// expanded
# if defined(ENABLE_MSAA)
		stencil = stencilSrc.Load(globalID, SAMPLE_ID);
# else
		stencil = stencilSrc[globalID];
# endif
	}
	
	if (zMask == 0)
	{
		// tile is clear
		// write the clear value, and early-out
		depth = depthClear;
		return;
	}
	else if (zMask == 15)
	{
		// tile is already decompressed
#if defined(ENABLE_MSAA)
		depth = asfloat(sharedDepthData[threadIndex * SAMPLE_COUNT + SAMPLE_ID]);
#else
		depth = asfloat(sharedDepthData[threadIndex]);
#endif
		
		return;
	}

	uint packedValue = zDataLayout[zMask];
	uint zPlaneCount = packedValue & 31;				// 5 bits
	uint pMaskElementSizeBits = (packedValue >> 5) & 7;	// 3 bits (but has values of 1 - 4)
	uint is32ByteChunk = (packedValue >> 8) & 1;		// 1 bit
	uint pMaskOffsetElement = (packedValue >> 9) & 127;	// 7 bits
	uint maxLoadIndex_WithoutZDataOffset = (packedValue >> 16) & 255;	// 8 bits
	
	// not sure where the HW docs refer to this...
	// also, if TILE_COUNT_X is not even, this may need to be (tileCoord.x + tileCoord.y * TILE_COUNT_X) & 1
	uint zDataOffset = (is32ByteChunk & tileCoord.x) << 3;
	
	uint zPlaneIndex = 0;
	if (zPlaneCount > 1)
	{
#if defined(ENABLE_MSAA)
		uint element = threadIndex * SAMPLE_COUNT + SAMPLE_ID;
		pMaskElementSizeBits = max(pMaskElementSizeBits, 2); // not sure what's going on here
#else
		uint element = threadIndex;
#endif
		uint bitOffset = element * pMaskElementSizeBits;
		uint PlaneIndexOffset = zDataOffset + pMaskOffsetElement + (bitOffset >> 5);

		bitOffset &= 31;

		zPlaneIndex = sharedDepthData[PlaneIndexOffset] >> bitOffset;
		uint bitsRead = 32 - bitOffset;
		if (pMaskElementSizeBits > bitsRead)
			zPlaneIndex |= sharedDepthData[PlaneIndexOffset + 1] << bitsRead;

		// These are equivalent, but I hope the compiler will generate a S_BFE_U32 instruction either way.
		//zPlaneIndex &= ~(0xffffffff << pMaskElementSizeBits);
		zPlaneIndex &= ((1 << pMaskElementSizeBits) - 1);
	}

	uint PlaneOffset = zDataOffset + zPlaneIndex * 3;
	uint3 zPlane = uint3(sharedDepthData[PlaneOffset], sharedDepthData[PlaneOffset + 1], sharedDepthData[PlaneOffset + 2]);
	
#if defined(ENABLE_MSAA)
	int2 subPixelPos = pixelCoord * 16 + 8 + int2(depthSrc.GetSamplePosition(SAMPLE_ID) * 16.0f);
#else
	int2 subPixelPos = pixelCoord << 1 | 1;
#endif

	depth = getDepthFromZPlane(zPlane, subPixelPos);
}
