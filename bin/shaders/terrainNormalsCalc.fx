#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"

#include "terrainConstants.fx"
DEFINE_TERRAIN_PARAMS_CB( 5 );

TEXTURE2D_ARRAY<float>	tClipMap 	: register( t0 );
SamplerState			sPointClamp : register( s0 );
TEXTURE2D_ARRAY<uint>	tCMClipMap 	: register( t1 );

//#define ALL_AT_ONCE

///////////////////////////////////////////////////////////////////
// Vertex shader is basically generating a conventional fullscreen quad
///////////////////////////////////////////////////////////////////

struct VS_OUTPUT
{
	float4	ScreenPos		: SYS_POSITION;
    float2	ClipmapUV		: TEXCOORD0;
#ifdef ALL_AT_ONCE
    uint	InstID			: INST_ID;
#endif
};

#ifdef ALL_AT_ONCE
struct GS_OUTPUT
{
    VS_OUTPUT	VSOutput;
    uint		RenderTargetIndex	: SYS_RENDER_TARGET_ARRAY_INDEX;
    float		TexArrInd			: TEX_ARRAY_IND_FLOAT;
};
#endif

#ifdef VERTEXSHADER

#ifdef ALL_AT_ONCE
VS_OUTPUT vs_main( in uint VertexId : SYS_VERTEX_ID,  in uint InstID : SYS_INSTANCE_ID )
#else
VS_OUTPUT vs_main( in uint VertexId : SYS_VERTEX_ID )
#endif
{
	float2 srcElevDataTextureSize;
    float numSlices;
    tClipMap.GetDimensions( srcElevDataTextureSize.x, srcElevDataTextureSize.y, numSlices );


    float4 dstTextureMinMaxUV = float4( -1, 1, 1, -1 );
    float4 srcElevAreaMinMaxUV = float4( 0, 0, 1, 1 );
    
    // Tessellation blocks do not cover boundary extensions, thus
    // it is neccessary to narrow the source height map UV range:
    //srcElevAreaMinMaxUV.xy += 2 / srcElevDataTextureSize;
    //srcElevAreaMinMaxUV.zw -= 2 / srcElevDataTextureSize;
    // During rasterization, height map UV will be interpolated to the center of each
    // tessellation block (x). However, we need the center of the height map texel (_):
    //    ___ ___ ___ ___     
    //   |   |   |   |   |
    //   |___|___|___|___|
    //   |   |   | _ |   |
    //   |___|___x___|___|
    //   |   |   |   |   |
    //   |___|___|___|___|
    //   |   |   |   |   |
    //   |___|___|___|___|
    // Thus it is necessary to add the following offset:
    //srcElevAreaMinMaxUV.xyzw += 0.5 / srcElevDataTextureSize.xyxy;

    VS_OUTPUT verts[4] = 
    {
#ifdef ALL_AT_ONCE
        { float4( dstTextureMinMaxUV.xy, 0.5, 1.0), srcElevAreaMinMaxUV.xy, InstID }, 
        { float4( dstTextureMinMaxUV.xw, 0.5, 1.0), srcElevAreaMinMaxUV.xw, InstID },
        { float4( dstTextureMinMaxUV.zy, 0.5, 1.0), srcElevAreaMinMaxUV.zy, InstID },
        { float4( dstTextureMinMaxUV.zw, 0.5, 1.0), srcElevAreaMinMaxUV.zw, InstID }
#else
		{ float4( dstTextureMinMaxUV.xy, 0.5, 1.0), srcElevAreaMinMaxUV.xy }, 
        { float4( dstTextureMinMaxUV.xw, 0.5, 1.0), srcElevAreaMinMaxUV.xw },
        { float4( dstTextureMinMaxUV.zy, 0.5, 1.0), srcElevAreaMinMaxUV.zy },
        { float4( dstTextureMinMaxUV.zw, 0.5, 1.0), srcElevAreaMinMaxUV.zw }
#endif
    };

    return verts[VertexId];
}

///////////////////////////////////////////////////////////////////
// Geometry shader's job is to pick a proper render target (one per clipmap level)
///////////////////////////////////////////////////////////////////

#ifdef ALL_AT_ONCE // Don't need geometry shader if not rendering all slices at once
[MAX_VERTEX_COUNT(3)]
void gs_main( GS_INPUT_TRIANGLE VS_OUTPUT In[3], inout GS_BUFFER_TRIANGLE<GS_OUTPUT> triStream )
{
    const uint InstID = In[0].InstID;
    for( int i=0; i<3; i++ )
    {
        GS_OUTPUT Out;
        Out.VSOutput = In[i];
        Out.RenderTargetIndex = InstID;
        Out.TexArrInd = (float)InstID;
        triStream.Append( Out );
    }
}
#endif

#endif

////////////////////////////////////////////////////////////////////
// Pixel shader renders to the slice in normal map array.
////////////////////////////////////////////////////////////////////

#ifdef PIXELSHADER

#ifdef ALL_AT_ONCE
float2 ps_main( GS_OUTPUT In ) : SYS_TARGET_OUTPUT0
#else
float2 ps_main( VS_OUTPUT In ) : SYS_TARGET_OUTPUT0
#endif
{
#ifdef ALL_AT_ONCE
	const float2 clipmapUV = In.VSOutput.ClipmapUV;
	const float clipmapLevel = In.TexArrInd;
#else
	const float2 clipmapUV = In.ClipmapUV;
	const float clipmapLevel = PSC_Custom_3.x;
#endif

    float3 clipmapDims;
    tClipMap.GetDimensions( clipmapDims.x, clipmapDims.y, clipmapDims.z );

    float2 clipmapTexelSize = 1.f / clipmapDims.xy;

	const float interVertexSpace = TerrainParams.interVertexSpace * ( 1 << (int)clipmapLevel );

#define GET_ELEV( ColOffset, RowOffset ) tClipMap.Sample( sPointClamp, float3( clipmapUV.xy + float2(ColOffset, RowOffset) * clipmapTexelSize, clipmapLevel ) )

#if 0	// 3x3 kernel for normal calculation
	float heightLeftTop = GET_ELEV( -1, -1 );
    float heightMidTop = GET_ELEV( +0, -1 );
    float heightRightTop = GET_ELEV( +1, -1 );

    float heightLeftMid = GET_ELEV( -1, +0 );
    //float heightMidMid = GET_ELEV( +0, +0 );
    float heightRightMid = GET_ELEV( +1, +0 );

    float heightLeftBottom = GET_ELEV( -1, +1 );
    float heightMidBottom = GET_ELEV( +0, +1 );
    float heightRightBottom = GET_ELEV( +1, +1 );

	float3 normal;
    normal.x = ( heightLeftTop + heightLeftMid + heightLeftBottom ) - ( heightRightTop + heightRightMid + heightRightBottom );
    normal.y = ( heightLeftTop + heightMidTop + heightRightTop ) - ( heightLeftBottom + heightMidBottom + heightRightBottom );
    normal.z = interVertexSpace * 6.f;
#else	// 2x2 kernel
    float heightLeft	= GET_ELEV( -1, 0 );
    float heightRight	= GET_ELEV( 1, 0 );
	float heightTop		= GET_ELEV( 0, -1 );
    float heightBottom	= GET_ELEV( 0, 1 );

	float3 normal;
    normal.x = heightLeft - heightRight;
    normal.y = heightTop - heightBottom;
    normal.z = interVertexSpace * 2.f;
#endif
	normal.xy = ( TerrainParams.highestElevation.xx - TerrainParams.lowestElevation.xx ) * normal.xy;

	normal = normalize( normal );

	//return ( normal.xy + 1.0f ) * 0.5f;
	return normal.xy;
}

#endif
