#include "commonVS.fx"
#include "adaptiveTessellation.fx"

TEXTURE2D_ARRAY<float>	tClipMap 	: register( t0 );
SamplerState			sLinearClampNoMip : register( s0 );

TEXTURE2D_ARRAY<uint>	tCMClipMap 	: register( t1 );
SamplerState			sControlMap : register( s1 );

TEXTURE2D_ARRAY<float4>	tErrors 	: register( t2 );

TEXTURE2D<float4>		tClipWindows : register( t3 );

#ifdef RS_TERRAIN_TOOL_STAMP_VISIBLE
Texture2D<float>		tStamp		: register( t4 );
SamplerState			sStamp		: register( s4 );
#endif

#include "terrainConstants.fx"
DEFINE_TERRAIN_PARAMS_CB( 5 );

#define MIN_EDGE_TESS_FACTOR 2

/*
	With RS_TERRAIN_TOOL_STAMP_VISIBLE defined:
	VSC_Custom_4:  xy - world space center of stamp, zw - world space direction of stamp's local X axis
	VSC_Custom_5:	x - stamp height scale; y - stamp height offset; z - preview mode 0=replace, 1=add; w - UNUSED
*/

// Utilis

float GetScreenSpaceError( float edgeError, float3 edgeCenterWorldSpacePos )
{
	const float3 normal = float3( 0.0f, 0.0f, 1.0f );

    float3 edgeCenterShiftedPoint1_WS = edgeCenterWorldSpacePos - normal * edgeError/2.f;
    float3 edgeCenterShiftedPoint2_WS = edgeCenterWorldSpacePos + normal * edgeError/2.f;

    float4 edgeCenterShiftedPoint1_PS = mul( float4( edgeCenterShiftedPoint1_WS, 1 ), VSC_WorldToScreen );
    float4 edgeCenterShiftedPoint2_PS = mul( float4( edgeCenterShiftedPoint2_WS, 1 ), VSC_WorldToScreen );

    edgeCenterShiftedPoint1_PS /= edgeCenterShiftedPoint1_PS.w;
    edgeCenterShiftedPoint2_PS /= edgeCenterShiftedPoint2_PS.w;
    
    edgeCenterShiftedPoint1_PS.xy *= VSC_ViewportParams/2;
    edgeCenterShiftedPoint2_PS.xy *= VSC_ViewportParams/2;

    float scrSpaceError = length(edgeCenterShiftedPoint1_PS.xy - edgeCenterShiftedPoint2_PS.xy);
    
    return scrSpaceError;
}

float CalculateEdgeTessFactor( float4 errors, float3 edgeCenterWorldSpacePos, int level )
{
    float edgeTessFactor = TESS_BLOCK_RES;

#define MAX_ERR         3.4e+38f
    float4 scrSpaceErrors = float4( MAX_ERR, MAX_ERR, MAX_ERR, MAX_ERR );

#if (TESS_BLOCK_RES >= 4)
    // We can simplify an edge by a factor of 2
    scrSpaceErrors.x = GetScreenSpaceError( errors.x, edgeCenterWorldSpacePos.xyz );
#endif
#if (TESS_BLOCK_RES >= 8)
    // We can simplify an edge by a factor of 4
    scrSpaceErrors.y = GetScreenSpaceError( errors.y, edgeCenterWorldSpacePos.xyz );
#endif
#if (TESS_BLOCK_RES >= 16)
    // We can simplify an edge by a factor of 8
    scrSpaceErrors.z = GetScreenSpaceError( errors.z, edgeCenterWorldSpacePos.xyz );
#endif
#if (TESS_BLOCK_RES >= 32)
    // We can simplify an edge by a factor of 16
    scrSpaceErrors.w = GetScreenSpaceError( errors.w, edgeCenterWorldSpacePos.xyz );
#endif

    // Compare screen space errors with the threshold
	float4 cmp;
	if ( level == 0 )
	{
		// For level 0, we want integer simplification, so the player won't see the terrain bubbling under their feet.
		const float4 errThresh = TerrainParams.screenSpaceErrorThresholdNear;
		cmp = ( scrSpaceErrors.xyzw < errThresh );
	}
	else
	{
		// For distant levels, use a continuous tess factor, so that there's less popping resulting from the larger error threshold.
		const float4 errThresh = TerrainParams.screenSpaceErrorThresholdFar;
		cmp = saturate( ( errThresh - scrSpaceErrors.xyzw ) * 0.5 + 0.5 );
	}

    // Calculate number of errors less than the threshold
    float simplPower = dot( cmp, float4(1,1,1,1) );
    // Compute simplification factor
    float simplFactor = exp2( simplPower );
    // Calculate edge tessellation factor
    edgeTessFactor /= simplFactor;

    return max( edgeTessFactor, MIN_EDGE_TESS_FACTOR );
}

float2 RecalculateUVInLargerRect( float2 uv, float4 smallerRect, float4 largerRect )
{
	float2 worldSpace = smallerRect.xy + float2( smallerRect.zw - smallerRect.xy ) * uv.xy;
	return abs(( worldSpace.xy - largerRect.xy ) / ( largerRect.zw - largerRect.xy ));
}

//--------------------------------------------------------------------------------------
// Vertex shader
//--------------------------------------------------------------------------------------

struct VS_INPUT
{
	float2 	pos				: POSITION0;			// norm
	float	patchSize		: TESS_PATCH_SIZE;		// world space
	float3  patchBias 		: TESS_PATCH_BIAS;		// xy - world space, z - clipmapLevel
};

struct VS_OUTPUT
{
	float2 pos 				: TESS_BLOCK_CORNER;
	float  blockSize 		: TESS_BLOCK_SIZE;
	float  clipmapLevel		: TESS_BLOCK_CM_LEVEL;
};

VS_OUTPUT vs_main( VS_INPUT input )
{
	VS_OUTPUT o = (VS_OUTPUT)0;
	
	// Generate control point position (tesselation block corner)
	o.pos = input.patchBias.xy + input.pos * input.patchSize;
	o.clipmapLevel = input.patchBias.z;
	
	// Setup tesselation block size
	o.blockSize = input.patchSize / NUM_TESS_BLOCKS_PER_PATCH;

	return o;
}

//--------------------------------------------------------------------------------------
// Hull shader
//--------------------------------------------------------------------------------------

struct HS_CONSTANT_DATA_OUTPUT
{
    float    Edges[4]			: SYS_EDGE_TESS_FACTOR;
    float    Inside[2]			: SYS_INSIDE_TESS_FACTOR;
	float	 attributes			: TESS_BLOCK_ATTRIBS;
};

struct HS_CONTROL_POINT_OUTPUT
{
	float2 pos 				: TESS_BLOCK_CORNER;
	float  blockSize 		: TESS_BLOCK_SIZE;
};

#define LOAD_ELEV( col, row, level ) lerp( TerrainParams.lowestElevation, TerrainParams.highestElevation, tClipMap.Load( int4( col, row, level, 0 ) ) );

#define SAMPLE_ELEV( uv, level ) lerp( TerrainParams.lowestElevation, TerrainParams.highestElevation, SAMPLE_LEVEL( tClipMap, sLinearClampNoMip, float3( uv, level ), 0, int2(0,0) ) );

HS_CONSTANT_DATA_OUTPUT ConstantsHS( InputPatch<VS_OUTPUT, 1> p, uint PatchID : SYS_PRIMITIVE_ID )
{
    HS_CONSTANT_DATA_OUTPUT output  = (HS_CONSTANT_DATA_OUTPUT)0;

	const float2 cpPos = p[0].pos;
	const float edgeLen = p[0].blockSize;
	const int clipmapLevel = p[0].clipmapLevel;

	output.attributes.x = (float)clipmapLevel;

	// Level above 4 is never actually close enough to be visible in detail by a player
	// it saves about 0.8ms of gpu time though
	if( clipmapLevel < 5 )
	{
		float screenSpaceError = 0.0f;

		// Choose clipmap level and compute UV for it
		float2 patchOppositeCorner = cpPos + edgeLen.xx;
		float2 patchCenter = cpPos + edgeLen.xx / 2.0f;

		const float4 levelRect = tClipWindows.Load( int3( clipmapLevel, 0, 0 ) );
		const float2 levelRectSize = levelRect.zw - levelRect.xy;

		float3 leftEdgeCenter = float3( cpPos + float2( 0.0f, edgeLen/2.0f ), 0.0f );
		float3 rightEdgeCenter = float3( cpPos + float2( edgeLen, edgeLen/2.0f ), 0.0f );
		float3 bottomEdgeCenter = float3( cpPos + float2( edgeLen/2.0f, 0.0f ), 0.0f );
		float3 topEdgeCenter = float3( cpPos + float2( edgeLen/2.0f, edgeLen ), 0.0f );

		float3 errorMapDimensions;
		tErrors.GetDimensions( errorMapDimensions.x, errorMapDimensions.y, errorMapDimensions.z );

		float3 heightMapDimensions;
		tClipMap.GetDimensions( heightMapDimensions.x, heightMapDimensions.y, heightMapDimensions.z );

		float2 patchCenterRegionUV = abs( ( patchCenter - levelRect.xy ) / levelRectSize );

		// Get height texel
		int2 heightmapPos = patchCenterRegionUV * heightMapDimensions.xy;

		// Get error texel
		int2 errorMapPos = patchCenterRegionUV * errorMapDimensions.xy;
		float4 patchCenterErrors = tErrors.Load( int4( errorMapPos, clipmapLevel, 0 ) );
		
		// Get neighbour error texels
		float4 leftNeighbErrors = tErrors.Load( int4( errorMapPos.x - 1, errorMapPos.y, clipmapLevel, 0 ) );
		float4 rightNeighbErrors = tErrors.Load( int4( errorMapPos.x + 1, errorMapPos.y, clipmapLevel, 0 ) );
		float4 bottomNeighbErrors = tErrors.Load( int4( errorMapPos.x, errorMapPos.y - 1, clipmapLevel, 0 ) );
		float4 topNeighbErrors = tErrors.Load( int4( errorMapPos.x, errorMapPos.y + 1, clipmapLevel, 0 ) );

		// Get neighbour height texels
		const int hmStepSize = TESS_BLOCK_RES / 2;
		leftEdgeCenter.z = LOAD_ELEV( heightmapPos.x - hmStepSize, heightmapPos.y, clipmapLevel );
		rightEdgeCenter.z = LOAD_ELEV( heightmapPos.x + hmStepSize, heightmapPos.y, clipmapLevel );
		bottomEdgeCenter.z = LOAD_ELEV( heightmapPos.x, heightmapPos.y - hmStepSize, clipmapLevel );
		topEdgeCenter.z = LOAD_ELEV( heightmapPos.x, heightmapPos.y + hmStepSize, clipmapLevel );

		float leftTessFactor = CalculateEdgeTessFactor( max( patchCenterErrors, leftNeighbErrors ), leftEdgeCenter, clipmapLevel );
		float rightTessFactor = CalculateEdgeTessFactor( max( patchCenterErrors, rightNeighbErrors ), rightEdgeCenter, clipmapLevel );
		float bottomTessFactor = CalculateEdgeTessFactor( max( patchCenterErrors, bottomNeighbErrors ), bottomEdgeCenter, clipmapLevel );
		float topTessFactor = CalculateEdgeTessFactor( max( patchCenterErrors, topNeighbErrors ), topEdgeCenter, clipmapLevel );


		bool isInStamp = false;
	#ifdef RS_TERRAIN_TOOL_STAMP_VISIBLE
		// If this patch intersects the stamp's bounding box, bump up tessellation so that even when it's over a flat area it'll show
		// lots of detail.
		{
			const float MinStampTessFactor = 8;

			float2 stampCenter = VSC_Custom_4.xy;
			float2 stampAxisU = VSC_Custom_4.zw;
			float2 stampAxisV = float2( -stampAxisU.y, stampAxisU.x );
			float2 stampMin = stampCenter + min( stampAxisU, -stampAxisU ) + min( stampAxisV, -stampAxisV );
			float2 stampMax = stampCenter + max( stampAxisU, -stampAxisU ) + max( stampAxisV, -stampAxisV );

			float2 patchMin = cpPos;
			float2 patchMax = patchOppositeCorner;

			if ( all( patchMin <= stampMax && patchMax >= stampMin ) )
			{
				leftTessFactor = max( leftTessFactor, MinStampTessFactor );
				rightTessFactor = max( rightTessFactor, MinStampTessFactor );
				bottomTessFactor = max( bottomTessFactor, MinStampTessFactor );
				topTessFactor = max( topTessFactor, MinStampTessFactor );
				isInStamp = true;
			}
		}
	#endif


		// Assume that the stamp is going to need discards. MS_DISCARD_PASS is only used for level 0 patches, so we only check level 0
	#ifdef MS_DISCARD_PASS
		if ( patchCenterErrors.w == 0.0f && !isInStamp )
	#else
		if ( patchCenterErrors.w > 0.0f || ( isInStamp && clipmapLevel == 0 ) )
	#endif
		{
			leftTessFactor = rightTessFactor = bottomTessFactor = topTessFactor = 0;
		}

		//leftTessFactor = 2;
		//rightTessFactor = 2;
		//bottomTessFactor = 4;
		//topTessFactor = 4;
	
		// Assign tessellation levels
		output.Edges[0] = leftTessFactor;
		output.Edges[1] = bottomTessFactor;
		output.Edges[2] = rightTessFactor;
		output.Edges[3] = topTessFactor;
		output.Inside[0] = output.Inside[1] = min(output.Edges[0], min(output.Edges[2], min(output.Edges[1], output.Edges[3])));
	}
	else
	{
		output.Edges[0] = 1;
		output.Edges[1] = 1;
		output.Edges[2] = 1;
		output.Edges[3] = 1;
		output.Inside[0] = output.Inside[1] = 1;
	}

    return output;
}

[DOMAIN_PATCH_TYPE("quad")]
[HS_PARTITIONING("fractional_even")]
[HS_OUTPUT_TOPOLOGY("triangle_ccw")]
[HS_OUTPUT_CONTROL_POINTS(1)]
[HS_PATCH_CONSTANT_FUNC("ConstantsHS")]
[HS_MAX_TESS_FACTOR(TESS_BLOCK_RES )]
HS_CONTROL_POINT_OUTPUT hs_main( InputPatch<VS_OUTPUT, 1> inputPatch, uint uCPID : SYS_OUTPUT_CONTROL_POINT_ID )
{
    HS_CONTROL_POINT_OUTPUT    output = (HS_CONTROL_POINT_OUTPUT)0;

    // Copy inputs to outputs
    output.pos = inputPatch[uCPID].pos;
	output.blockSize = inputPatch[uCPID].blockSize;

    return output;
}

//--------------------------------------------------------------------------------------
// Domain Shader
//--------------------------------------------------------------------------------------

$GENERATE_OUTPUTS_STRUCTURE


[DOMAIN_PATCH_TYPE("quad")]
OUTPUTS ds_main( HS_CONSTANT_DATA_OUTPUT input, float2 quadUV : SYS_DOMAIN_LOCATION, 
             const OutputPatch<HS_CONTROL_POINT_OUTPUT, 1> quadPatch )
{
	VS_FAT_VERTEX FatVertex = (VS_FAT_VERTEX)0;

    // Interpolate world space position with quad uv coordinates
	float4 vWorldPos = float4( quadPatch[0].pos + quadUV * quadPatch[0].blockSize, 0.0f, 1.0f );

	// Choose clipmap level and compute UV for it
	float heightmapIndex = input.attributes.x;
	float colorMapHeightmapIndex = heightmapIndex;
	float2 regionUV, quadRegionUV;
	float2 regionUVColorMap;

	float4 windowRect = tClipWindows.Load( int3( heightmapIndex, 0, 0 ) );
	float4 validTex = tClipWindows.Load( int3( heightmapIndex, 1, 0 ) );

	float3 clipmapDim;
	tClipMap.GetDimensions( clipmapDim.x, clipmapDim.y, clipmapDim.z );

	regionUV = abs( ( vWorldPos.xy - windowRect.xy ) / ( windowRect.zw - windowRect.xy ) );

	// Offset UV's by half-texel. This puts the texels at the vertex positions, instead of between vertices.
	regionUV.xy += 0.5f / clipmapDim.xy;

	regionUV = ( regionUV * ( validTex.zw - validTex.xy ) + validTex.xy );
	regionUVColorMap = regionUV;

	if ( colorMapHeightmapIndex < TerrainParams.colormapStartingIndex )
	{
		colorMapHeightmapIndex = TerrainParams.colormapStartingIndex;

		float4 cwindowRect = tClipWindows.Load( int3( colorMapHeightmapIndex, 0, 0 ) );
		float4 cvalidTex = tClipWindows.Load( int3( colorMapHeightmapIndex, 1, 0 ) );

		regionUVColorMap = abs( ( vWorldPos.xy - cwindowRect.xy ) / ( cwindowRect.zw - cwindowRect.xy ) );
		regionUVColorMap = ( regionUVColorMap * ( cvalidTex.zw - cvalidTex.xy ) + cvalidTex.xy );
	}

	// Sample heightmap
	float4 height = SAMPLE_LEVEL( tClipMap, sLinearClampNoMip, float3(regionUV, heightmapIndex), 0 );

#ifndef MS_PREGENERATED_MAPS
	// Collect neighbour heights for in place normal computation
	float4 neighbourHeights;
	neighbourHeights.x /*left*/		= SAMPLE_LEVEL( tClipMap, sLinearClampNoMip, float3(regionUV - float2( 1.0f/clipmapDim.x, 0.0f ), (float)heightmapIndex ), 0 ).x;
	neighbourHeights.y /*right*/	= SAMPLE_LEVEL( tClipMap, sLinearClampNoMip, float3(regionUV + float2( 1.0f/clipmapDim.x, 0.0f ), (float)heightmapIndex ), 0 ).x;
	neighbourHeights.z /*top*/		= SAMPLE_LEVEL( tClipMap, sLinearClampNoMip, float3(regionUV - float2( 0.0f, 1.0f/clipmapDim.x ), (float)heightmapIndex ), 0 ).x;
	neighbourHeights.w /*bottom*/	= SAMPLE_LEVEL( tClipMap, sLinearClampNoMip, float3(regionUV + float2( 0.0f, 1.0f/clipmapDim.x ), (float)heightmapIndex ), 0 ).x;
	// Convert to actual world space heights
	neighbourHeights = lerp( TerrainParams.lowestElevation.xxxx, TerrainParams.highestElevation.xxxx, neighbourHeights );
#endif
		
	// Compute the doubled distance between adjacent height samples, it will be needed for normal computation
	float interVertexSpaceScaled = TerrainParams.interVertexSpace * pow( 2, heightmapIndex );
	const float doubleInterVertexSpaceScaled = 2.0f * interVertexSpaceScaled;

#ifdef RS_TERRAIN_TOOL_STAMP_VISIBLE
	float2 stampCenter = VSC_Custom_4.xy;
	float2 stampAxisU = VSC_Custom_4.zw;
	// Assume the stamp is square
	float2 stampAxisV = float2( -stampAxisU.y, stampAxisU.x );

	float2 stampUV = float2(
		dot( ( vWorldPos.xy - stampCenter ), stampAxisU ) / dot( stampAxisU, stampAxisU ),
		dot( ( vWorldPos.xy - stampCenter ), stampAxisV ) / dot( stampAxisV, stampAxisV )
		) * 0.5f + 0.5f;

	float4 stampSettings = VSC_Custom_5;
	float stampHeightScale = stampSettings.x;
	float stampHeightOffset = stampSettings.y;
	float stampAdditive = stampSettings.z;

#ifdef MS_PREGENERATED_MAPS
	float3 stampNormal = (float3)0;
#endif

	bool isInStamp = all( stampUV >= 0.0f ) && all( stampUV < 1.0f );
	if ( isInStamp )
	{
		// Since the stamp has different texel density and orientation from the terrain, we need to do some work to sample
		// proper locations for normal calculations.

		// rotate/scale world-space offset into the stamp's space.
		float2 bau = normalize( stampAxisU );
		float2 bav = normalize( stampAxisV );
		float4 uvOffset = float4( bau.x, bav.x, bau.y, bav.y ) / clipmapDim.x;

		// scale the offsets, so that they have the same world-space length as an offset in the main heightmap.
		float stampInterVertexSpaceScaled = ( 2.0f * length( stampAxisU ) ) / clipmapDim.x;
		uvOffset *= interVertexSpaceScaled / stampInterVertexSpaceScaled;

		// Sample stamp heightmap for elevation
		float stamp			= stampHeightOffset + stampHeightScale * SAMPLE_LEVEL( tStamp, sStamp, stampUV, 0 ).x;
		
		// Collect stamp neighbour heights for in place normal computation
		float4 stampNeighbourHeights;
		stampNeighbourHeights.x = SAMPLE_LEVEL( tStamp, sStamp, stampUV - uvOffset.xy, 0 ).x;
		stampNeighbourHeights.y = SAMPLE_LEVEL( tStamp, sStamp, stampUV + uvOffset.xy, 0 ).x;
		stampNeighbourHeights.z = SAMPLE_LEVEL( tStamp, sStamp, stampUV - uvOffset.zw, 0 ).x;
		stampNeighbourHeights.w = SAMPLE_LEVEL( tStamp, sStamp, stampUV + uvOffset.zw, 0 ).x;
		// Apply scale and offset
		stampNeighbourHeights *= stampHeightScale;
		stampNeighbourHeights += stampHeightOffset;
		// Convert to actual world space heights
		stampNeighbourHeights = lerp( TerrainParams.lowestElevation.xxxx, TerrainParams.highestElevation.xxxx, stampNeighbourHeights );

		// Add or replace, based on stamp settings.
		height			= clamp( stampAdditive*height 		+ stamp, 0, 1 );

#ifndef MS_PREGENERATED_MAPS
		// For in-place normals, consider stamp data in the computation
		neighbourHeights	= clamp( stampAdditive*neighbourHeights	+ stampNeighbourHeights, TerrainParams.lowestElevation.xxxx, TerrainParams.highestElevation.xxxx );
#else
		// For precomputed normals, just calculate the stamp normal, and pass it to the PS, so it can be blended with the pregenerated one.
		stampNormal = normalize( float3( stampNeighbourHeights.x - stampNeighbourHeights.y, stampNeighbourHeights.z - stampNeighbourHeights.w, doubleInterVertexSpaceScaled ) );
#endif
	}
#endif

	// Convert height from unorm to world space
    vWorldPos.z = lerp( TerrainParams.lowestElevation, TerrainParams.highestElevation, height.x );

	// Output world position (local and world space are the same in the case of terrain)
	FatVertex.WorldPosition = FatVertex.VertexPosition = vWorldPos.xyz;
	
	// Output screen position
	FatVertex.ScreenPosition = mul( vWorldPos, VSC_WorldToScreen );
	
	/////////////////////////////////////////////////
	// NORMAL VECTOR OUTPUT (two cases to consider: for in-place and pregenerated normals)
#ifndef MS_PREGENERATED_MAPS
	// We have the neighbour heights computed and including stamp heights if stamp is ON, so we just do the usual normal computation here
	#if 1
		FatVertex.WorldNormal = FatVertex.VertexNormal = normalize( float3( neighbourHeights.x - neighbourHeights.y, neighbourHeights.z - neighbourHeights.w, doubleInterVertexSpaceScaled ) );
	#else
		// Output tangent space basis
		FatVertex.WorldTangent = normalize( float3( doubleInterVertexSpaceScaled, 0, neighbourHeights.y - neighbourHeights.x ) );
		FatVertex.WorldBinormal = normalize( float3( 0, doubleInterVertexSpaceScaled, neighbourHeights.w - neighbourHeights.z ) );
		FatVertex.WorldNormal = FatVertex.VertexNormal = normalize( cross( FatVertex.WorldTangent, FatVertex.WorldBinormal ) );
	#endif
#endif

#if defined( MS_PREGENERATED_MAPS ) && defined( RS_TERRAIN_TOOL_STAMP_VISIBLE )
	// This case is special, as we have to pass stamp normal, for blending with the base, pregenerated one
	FatVertex.WorldNormal = FatVertex.VertexNormal = stampNormal;
#endif
	//////////////////////////////////////////////////
	
#ifdef RS_TERRAIN_TOOL_STAMP_VISIBLE
	// Vertex color is otherwise unused, so we'll pack some stamp information into it
	FatVertex.VertexColor = float4( saturate(stampUV), isInStamp ? 1.0f : 0.0f, stampAdditive );
#endif

	// Output clipmap window space UV
	FatVertex.UV = regionUV;
	FatVertex.UV2 = regionUVColorMap;
	
	// Output heightmap index
	FatVertex.TexIndex.x = heightmapIndex;
	FatVertex.TexIndex.y = colorMapHeightmapIndex;
	
$GENERATE_OUTPUTS_FROM_FAT_VERTEX
$END_OF_DOMAIN
}
