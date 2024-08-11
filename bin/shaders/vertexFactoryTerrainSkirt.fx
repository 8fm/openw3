#include "commonVS.fx"
#include "terrainConstants.fx"
DEFINE_TERRAIN_PARAMS_CB( 5 );

TEXTURE2D_ARRAY<float>	tClipMap 	: register( t0 );
SamplerState			sLinearClampNoMip : register( s0 );

TEXTURE2D<float4>		tClipWindows : register( t1 );

struct VS_INPUT
{
	float2 	pos				: POSITION0;			// x - 0/1 (upper/lower), y - (0,1,2,3,4,5) vertex
	float4  patchBias 		: TESS_PATCH_BIAS;		// xy - world space, z - clipmapLevel, w - clipmap border side
};

$GENERATE_OUTPUTS_STRUCTURE

OUTPUTS vs_main( VS_INPUT i )
{
	VS_FAT_VERTEX FatVertex = (VS_FAT_VERTEX)0;

	float clipmapLevel = i.patchBias.z;
	float4 pos = float4(0.0f, 0.0f, 0.0f, 1.0f);

	float colorMapHeightmapIndex = clipmapLevel;
	const float2 sideOffsets[4] = { float2( 0.0f, 1.0f), float2( 0.0f, -1.0f), float2( 1.0f, 0.0f),	float2( -1.0f, 0.0f) }; 

	// scale level 0
	float scale = TerrainParams.quadTreeNodeSize * (2 << (int)clipmapLevel);
	pos.xy = i.patchBias.xy + sideOffsets[(int)i.patchBias.w] * (i.pos.y * (scale / 8.0f) );

	float4 windowRect = tClipWindows.Load( int3( clipmapLevel, 0, 0 ) );
	float4 validTex = tClipWindows.Load( int3( clipmapLevel, 1, 0 ) );

	float3 clipmapDim;
	tClipMap.GetDimensions( clipmapDim.x, clipmapDim.y, clipmapDim.z );

	float2 regionUV = abs( ( pos.xy - windowRect.xy ) / ( windowRect.zw - windowRect.xy ) );

	// Offset UV's by half-texel. This puts the texels at the vertex positions, instead of between vertices.
	regionUV.xy += 0.5f / clipmapDim.xy;

	regionUV = ( regionUV * ( validTex.zw - validTex.xy ) + validTex.xy );
	float2 regionUVColorMap;
	regionUVColorMap = regionUV;

	if ( colorMapHeightmapIndex < TerrainParams.colormapStartingIndex )
	{
		colorMapHeightmapIndex = TerrainParams.colormapStartingIndex;

		float4 cwindowRect = tClipWindows.Load( int3( colorMapHeightmapIndex, 0, 0 ) );
		float4 cvalidTex = tClipWindows.Load( int3( colorMapHeightmapIndex, 1, 0 ) );

		regionUVColorMap = abs( ( pos.xy - cwindowRect.xy ) / ( cwindowRect.zw - cwindowRect.xy ) );
		regionUVColorMap = ( regionUVColorMap * ( cvalidTex.zw - cvalidTex.xy ) + cvalidTex.xy );
	}

	// Sample heightmap
	float4 height = SAMPLE_LEVEL( tClipMap, sLinearClampNoMip, float3(regionUV, clipmapLevel), 0 );

	// Convert height from unorm to world space
	pos.z = lerp( TerrainParams.lowestElevation, TerrainParams.highestElevation, height.x ) + clipmapLevel/2.0f - i.pos.x * exp(clipmapLevel + 1);

	// Output world position (local and world space are the same in the case of terrain)
	FatVertex.WorldPosition = FatVertex.VertexPosition = pos.xyz;

	pos = mul( pos, VSC_WorldToScreen );

	// Output screen position
	FatVertex.ScreenPosition = pos;

	// Output clipmap window space UV
	FatVertex.UV = regionUV;
	FatVertex.UV2 = regionUVColorMap;

	// Output heightmap index
	FatVertex.TexIndex.x = clipmapLevel;
	FatVertex.TexIndex.y = colorMapHeightmapIndex;

	$GENERATE_OUTPUTS_FROM_FAT_VERTEX
	$END_OF_DOMAIN
}