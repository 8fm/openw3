#define DEFERRED

#include "common.fx"
#include "commonCS.fx"
#include "include_constants.fx"

RW_TEXTURE2D<float4> OutputTexture			: register(u0);
RW_BYTEBUFFER DimmerListTransparentOutput	: register(u1);
Texture2D<float> DepthTexture				: register(t0);
Texture2D<float2> VolumeTexture				: register(t1);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Shared memory
GROUPSHARED uint TileMinZ;
GROUPSHARED uint TileMaxZ;

// Dimmers list for the tile
GROUPSHARED uint TileDimmerListOpaque[MAX_DIMMERS_PER_TILE];
GROUPSHARED uint NumTileDimmersOpaque;

GROUPSHARED uint TileDimmerListTransparent[MAX_DIMMERS_PER_TILE];
GROUPSHARED uint NumTileDimmersTransparent;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ENABLE_DIMMERS_TILED_DEFERRED
#include "include_dimmers.fx"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CullDimmer( uint dimmerIndex, float4 frustumPlanes[6] )
{
	const DimmerParams dimmer = dimmers[dimmerIndex];
		
	uint planesResult = 0;

	[unroll]
	for ( int plane_i=0; plane_i<6; ++plane_i )
	{
		float4 currPlane = frustumPlanes[plane_i];
		float3 planePoint = -currPlane.xyz * currPlane.w;
		float3 localPlanePoint = mul( float4 ( planePoint.xyz, 1.0 ), dimmer.viewToLocal );
		float3 localPlaneDir = mul( currPlane.xyz, (float3x3)dimmer.viewToLocal ) * dimmer.normalScale.xyz;
		float4 localPlane = float4 ( localPlaneDir, -dot( localPlanePoint, localPlaneDir ) );

		//const float fadeAlpha = (dimmer.fadeAlphaAndInsideMarkFactor >> 16) / (float)0xffff;
		const float insideMarkFactor = (dimmer.fadeAlphaAndInsideMarkFactor & 0xffff) / (float)0xffff;		

		bool _currInFrustum = true;
		if ( insideMarkFactor > 0 || -1 == dimmer.marginFactor )
		{
			float dd = dot( 1, float4 ( abs( localPlane.xyz ), localPlane.w ) );
			_currInFrustum = dd >= 0;
		}
		else
		{
			float _l = length( localPlane.xyz );
			_currInFrustum = _l > -localPlane.w;
		}	

		if ( _currInFrustum )
		{
			planesResult += (1 << plane_i);
		}
	}

	const uint transpMask = ((1<<5)-1);

#ifdef SKIP_CULLING
	const bool inFrustum = true;
#else
	const bool inFrustum = ((1<<6)-1) == planesResult;
#endif // SKIP_CULLING
	const bool inFrustumTransparent = transpMask == (planesResult & transpMask);

	if ( inFrustumTransparent )
	{
		uint listIndexTransparent;
		INTERLOCKED_ADD( NumTileDimmersTransparent, 1, listIndexTransparent );
		if ( listIndexTransparent < MAX_DIMMERS_PER_TILE )
		{
			TileDimmerListTransparent[listIndexTransparent] = dimmerIndex;
		}
	}

	if ( inFrustum )
	{
		uint listIndex;
		INTERLOCKED_ADD( NumTileDimmersOpaque, 1, listIndex );
		if ( listIndex < MAX_DIMMERS_PER_TILE )
		{
			TileDimmerListOpaque[listIndex] = dimmerIndex;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float4 TransformPlane( float4x4 mat, float4 plane )
{
	// ace_optimize
	float3 p = mul( mat, float4 ( -plane.xyz * plane.w, 1 ) ).xyz;
	float3 n = mul( (float3x3)mat, plane.xyz );
	return float4 ( n, -dot(n,p) );
}

void PerformCulling( uint2 groupId, uint groupThreadIdx, float zw )
{
	zw = TransformDepthRevProjAware( zw ); // ace_todo: we loose some precision here, but it might be more efficient when revProjAware is not compiled in (not 100% sure though)

	// Initialize shared memory
    if(groupThreadIdx == 0)
	{
		NumTileDimmersOpaque = 0;
		TileDimmerListOpaque[0] = 0;
		TileDimmerListTransparent[0] = 0;
		NumTileDimmersTransparent = 0;
        TileMinZ = 0x7F7FFFFF;      // Max float
        TileMaxZ = 0;
    }

    GROUP_BARRIER_GROUP_SYNC;

	INTERLOCKED_MIN(TileMinZ, asuint(zw));
	INTERLOCKED_MAX(TileMaxZ, asuint(zw));

    GROUP_BARRIER_GROUP_SYNC;

    float minTileZ = DeprojectDepth( asfloat(TileMinZ) );
    float maxTileZ = DeprojectDepth( asfloat(TileMaxZ) );

	// Work out scale/bias from [0, 1]
    float2 tileScale = float2(screenDimensions.xy) * rcp(2.0f * float2(TILE_SIZE_INTERIOR_FULLRES, TILE_SIZE_INTERIOR_FULLRES));
    float2 tileBias = tileScale - float2(groupId.xy);

    // Now work out composite projection matrix
    // Relevant matrix columns for this tile frusta
#ifdef SKIP_CULLING
	float4 c1 = float4( projectionMatrix._11 * tileScale.x,	 projectionMatrix._21,					tileBias.x,	projectionMatrix._41 );
	float4 c2 = float4( projectionMatrix._12,				-projectionMatrix._22 * tileScale.y,	tileBias.y, projectionMatrix._42 );
	float4 c4 = float4( projectionMatrix._14,				 projectionMatrix._24, 					1.0f,		projectionMatrix._44 );
#else
	float4 c1 = float4(projectionMatrix._11 * tileScale.x, 0.0f, tileBias.x, 0.0f);
    float4 c2 = float4(0.0f, -projectionMatrix._22 * tileScale.y, tileBias.y, 0.0f);
    float4 c4 = float4(0.0f, 0.0f, 1.0f, 0.0f);
#endif // SKIP_CULLING

    // Derive frustum planes
    float4 frustumPlanes[6];

    // Sides
    frustumPlanes[0] = c4 - c1;
    frustumPlanes[1] = c4 + c1;
    frustumPlanes[2] = c4 - c2;
    frustumPlanes[3] = c4 + c2;

    // Near/far
    frustumPlanes[4] = float4(0.0f, 0.0f, -1.0f,  maxTileZ);
	frustumPlanes[5] = float4(0.0f, 0.0f,  1.0f, -minTileZ);
	
	// For transparent
	//frustumPlanes[6] = float4(0.0f, 0.0f,  1.0f, 0.0f );    

    // Normalize frustum planes (near/far already normalized)
	frustumPlanes[0] /= length(frustumPlanes[0].xyz);
	frustumPlanes[1] /= length(frustumPlanes[1].xyz);
	frustumPlanes[2] /= length(frustumPlanes[2].xyz);
	frustumPlanes[3] /= length(frustumPlanes[3].xyz);

	[loop]
	for (int dimmerIndex = groupThreadIdx; dimmerIndex < dimmerNum; dimmerIndex += TILE_SIZE*TILE_SIZE)
	{
		CullDimmer( dimmerIndex, frustumPlanes );
	}

	GROUP_BARRIER_GROUP_SYNC;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OutputTileLists( uint2 groupId, uint groupThreadIdx )
{
	if ( groupThreadIdx < MAX_DIMMERS_PER_TILE)
	{
		// Write out the indices
		uint tileIdx = groupId.y * numTiles.z + groupId.x;
		uint bufferIdx = tileIdx * MAX_DIMMERS_PER_TILE + groupThreadIdx;
		uint dimmerIdx;
		if ( groupThreadIdx < NumTileDimmersTransparent )
		{
			dimmerIdx = TileDimmerListTransparent[groupThreadIdx];
		}
		else
		{	
			dimmerIdx = TILED_DEFERRED_DIMMERS_CAPACITY;
		}

		DimmerListTransparentOutput.Store(bufferIdx*4, dimmerIdx);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float FetchDepthTexture( uint2 pixelCoord )
{
	return DepthTexture[ pixelCoord ].x;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

[NUMTHREADS(TILE_SIZE, TILE_SIZE, 1)]
void cs_main(uint3 GroupID : SYS_GROUP_ID, uint3 GroupThreadID : SYS_GROUP_THREAD_ID)
{
	const uint2 pixelCoord		= GroupID.xy * uint2(TILE_SIZE, TILE_SIZE) + GroupThreadID.xy;
	const uint groupThreadIdx	= GroupThreadID.y * TILE_SIZE + GroupThreadID.x;
	const float zw				= FetchDepthTexture( pixelCoord * WEATHER_VOLUMES_SIZE_DIV );
	const bool isSky			= IsSkyByProjectedDepthRevProjAware( zw );

	PerformCulling( GroupID.xy, groupThreadIdx, zw );	
	
	float interiorFactor = 1;
	float dimmerFactor = 1;
	[branch]
	if ( !isSky )
	{
		const float3 worldSpacePosition = PositionFromDepthRevProjAware( zw, pixelCoord, screenDimensions / WEATHER_VOLUMES_SIZE_DIV );
		
		const float2 volumeTextureValue = VolumeTexture[ pixelCoord ];

		interiorFactor = CalculateVolumeCutCustomVolumeTexture( volumeTextureValue, worldSpacePosition );
		float2 dimmerAndInteriorResult = CalcDimmersFactorAndInteriorFactorTiledDeferred( worldSpacePosition, pixelCoord, interiorFactor );
		dimmerFactor = dimmerAndInteriorResult.x;
		interiorFactor = dimmerAndInteriorResult.y;
	}

	// Output color
	OutputTexture[pixelCoord] = float4 ( 0, 0, interiorFactor, dimmerFactor );

	// Output light buffers	
	OutputTileLists( GroupID.xy, groupThreadIdx );
}


