#define DEFERRED

#include "common.fx"
#include "commonCS.fx"
#include "include_constants.fx"
#include "include_dimmers.fx"

RW_BYTEBUFFER LightListTransparentOutput	: register(u0);
Texture2D<float> DepthTexture				: register(t0);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GROUPSHARED uint TileMaxZ[TILE_SIZE];
GROUPSHARED uint TileLightListTransparent[MAX_LIGHTS_PER_TILE];
GROUPSHARED uint NumTileLightsTransparent;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CullLight( uint lightIndex, float4 frustumPlanes[6] )
{
	float3 lightPosition = mul( worldToView, float4 ( lights[lightIndex].positionAndRadius.xyz, 1 ) ).xyz;
	float cutoffRadius = lights[lightIndex].positionAndRadius.w;

	// 5 frustum planes are common, 1 is a znear for solids, 1 is znear for transparent
	float d0 = dot(frustumPlanes[0], float4(lightPosition, 1.0f));
	float d1 = dot(frustumPlanes[1], float4(lightPosition, 1.0f));
	float d2 = dot(frustumPlanes[2], float4(lightPosition, 1.0f));
	float d3 = dot(frustumPlanes[3], float4(lightPosition, 1.0f));
	float d4 = dot(frustumPlanes[4], float4(lightPosition, 1.0f));
	float d5 = dot(frustumPlanes[5], float4(lightPosition, 1.0f));
	
	// test if the grid does contain this light (more conservative, for transparent pixels)
    bool inFrustumTransparent = (d0 >= -cutoffRadius) && (d1 >= -cutoffRadius) && (d2 >= -cutoffRadius) && (d3 >= -cutoffRadius) && (d4 >= -cutoffRadius) && (d5 >= -cutoffRadius);
		
	// spot light culling - no branching, all of the ALU is nicelly eaten
	if ( lights[lightIndex].colorAndType.w >= 0.8f )
	{
		// spot lights
		float3 lightPositionCull = mul( worldToView, float4 ( lights[lightIndex].positionAndRadiusCull.xyz, 1 ) ).xyz;
		float cutoffRadiusCull = lights[lightIndex].positionAndRadiusCull.w;

		// plane tests
		float d0 = dot(frustumPlanes[0], float4(lightPositionCull, 1.0f));
		float d1 = dot(frustumPlanes[1], float4(lightPositionCull, 1.0f));
		float d2 = dot(frustumPlanes[2], float4(lightPositionCull, 1.0f));
		float d3 = dot(frustumPlanes[3], float4(lightPositionCull, 1.0f));
		float d4 = dot(frustumPlanes[4], float4(lightPositionCull, 1.0f));
		float d5 = dot(frustumPlanes[5], float4(lightPositionCull, 1.0f));

		// test if the grid does contain this light (more conservative, for transparent pixels)
		if ( inFrustumTransparent )
		{
			inFrustumTransparent = (d0 >= -cutoffRadiusCull) && (d1 >= -cutoffRadiusCull) && (d2 >= -cutoffRadiusCull) && (d3 >= -cutoffRadiusCull) && (d4 >= -cutoffRadiusCull) && (d5 >= -cutoffRadiusCull);
		}
	}

	// if the light is visible emit it to the light list
	if ( inFrustumTransparent )
	{
		uint listIndexTransparent;
		INTERLOCKED_ADD( NumTileLightsTransparent, 1, listIndexTransparent );
		if ( listIndexTransparent < MAX_LIGHTS_PER_TILE )
		{
			TileLightListTransparent[listIndexTransparent] = lightIndex;
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
		TileLightListTransparent[0] = 0;
		NumTileLightsTransparent = 0;
    }
	if(groupThreadIdx < TILE_SIZE)
	{
        TileMaxZ[groupThreadIdx] = 0;
	}

    GROUP_BARRIER_GROUP_SYNC;

	INTERLOCKED_MAX(TileMaxZ[groupThreadIdx%TILE_SIZE], asuint(zw));

    GROUP_BARRIER_GROUP_SYNC;

	uint maxValue = TileMaxZ[0];
	[unroll]
	for ( uint i=1; i<TILE_SIZE; ++i )
		maxValue = max( maxValue, TileMaxZ[i] );
    float maxTileZ = DeprojectDepth( asfloat(maxValue) );

	// Work out scale/bias from [0, 1]
    float2 tileScale = float2(screenDimensions.xy) * rcp(2.0f * float2(TILE_SIZE, TILE_SIZE));
    float2 tileBias = tileScale - float2(groupId.xy);

    // Now work out composite projection matrix
    // Relevant matrix columns for this tile frusta
    float4 c1 = float4(projectionMatrix._11 * tileScale.x, 0.0f, tileBias.x, 0.0f);
    float4 c2 = float4(0.0f, -projectionMatrix._22 * tileScale.y, tileBias.y, 0.0f);
    float4 c4 = float4(0.0f, 0.0f, 1.0f, 0.0f);

    // Derive frustum planes
    float4 frustumPlanes[6];

    // Sides
    frustumPlanes[0] = c4 - c1;
    frustumPlanes[1] = c4 + c1;
    frustumPlanes[2] = c4 - c2;
    frustumPlanes[3] = c4 + c2;

    // Near/far
    frustumPlanes[4] = float4(0.0f, 0.0f, -1.0f,  maxTileZ);
	
	// For transparent
	frustumPlanes[5] = float4(0.0f, 0.0f,  1.0f, 0.0f );    

    // Normalize frustum planes (near/far already normalized)
	frustumPlanes[0] /= length(frustumPlanes[0].xyz);
	frustumPlanes[1] /= length(frustumPlanes[1].xyz);
	frustumPlanes[2] /= length(frustumPlanes[2].xyz);
	frustumPlanes[3] /= length(frustumPlanes[3].xyz);

	// Cull lights for this tile
	[loop]
	for (int lightIndex = groupThreadIdx; lightIndex < lightNum; lightIndex += TILE_SIZE*TILE_SIZE)
	{
		CullLight( lightIndex, frustumPlanes );
	}

	GROUP_BARRIER_GROUP_SYNC;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OutputTileLists( uint2 groupId, uint groupThreadIdx )
{
	// Outputing lights lists
    if(groupThreadIdx < MAX_LIGHTS_PER_TILE)
    {
        // Write out the indices
        uint tileIdx = groupId.y * numTiles.x + groupId.x;
        uint bufferIdx = tileIdx * MAX_LIGHTS_PER_TILE + groupThreadIdx;
		uint lightIdx;
		if ( groupThreadIdx < NumTileLightsTransparent )
		{
			lightIdx = TileLightListTransparent[groupThreadIdx];
		}
		else
		{	
			lightIdx = MAX_LIGHTS_PER_TILE;
		}

        LightListTransparentOutput.Store(bufferIdx*4, lightIdx);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

[NUMTHREADS(TILE_SIZE, TILE_SIZE, 1)]
void cs_main(uint3 GroupID : SYS_GROUP_ID, uint3 GroupThreadID : SYS_GROUP_THREAD_ID)
{
	const uint2 pixelCoord		= GroupID.xy * uint2(TILE_SIZE, TILE_SIZE) + GroupThreadID.xy;
	const uint groupThreadIdx	= GroupThreadID.y * TILE_SIZE + GroupThreadID.x;
	const float zw				= DepthTexture[pixelCoord].x;
	const bool isSky			= IsSkyByProjectedDepthRevProjAware( zw );

	PerformCulling( GroupID.xy, groupThreadIdx, zw );
	OutputTileLists( GroupID.xy, groupThreadIdx );
}
