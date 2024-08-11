#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"

#include "terrainConstants.fx"
DEFINE_TERRAIN_PARAMS_CB( 5 );

TEXTURE2D_ARRAY<float>	tClipMap 	: register( t0 );
SamplerState			sPointClamp : register( s0 );
TEXTURE2D_ARRAY<uint>	tCMClipMap 	: register( t1 );

//#define ALL_AT_ONCE

// This whole technique is heavily inspired by the GPU Pro 3 book article and the associated samples

///////////////////////////////////////////////////////////////////
// Vertex shader is basically generating a conventional fullscreen quad
///////////////////////////////////////////////////////////////////

struct VS_OUTPUT
{
	float4	ScreenPos		: SYS_POSITION;
    float2	ElevationMapUV	: TEXCOORD0;
#ifdef ALL_AT_ONCE
    uint	InstID			: INST_ID;
#endif
};

#ifdef ALL_AT_ONCE
struct GS_OUTPUT
{
    VS_OUTPUT VSOutput;
    uint RenderTargetIndex	: SYS_RENDER_TARGET_ARRAY_INDEX;
    float TexArrInd			: TEX_ARRAY_IND_FLOAT;
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

#endif

///////////////////////////////////////////////////////////////////
// Geometry shader's job is to pick a proper render target (one per clipmap level)
///////////////////////////////////////////////////////////////////

#ifdef ALL_AT_ONCE	// Don't need geometry shader at all in this case

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

////////////////////////////////////////////////////////////////////
// Pixel shader renders to the slice in tesselation blocks errors array.
////////////////////////////////////////////////////////////////////

#ifdef PIXELSHADER

#pragma PSSL_target_output_format(default FMT_32_ABGR)

#ifdef ALL_AT_ONCE
float4 ps_main( GS_OUTPUT In ) : SYS_TARGET_OUTPUT0
#else
float4 ps_main( VS_OUTPUT In ) : SYS_TARGET_OUTPUT0
#endif
{
#ifdef ALL_AT_ONCE
	float2 elevationMapUV = In.VSOutput.ElevationMapUV;
	float elevDataTexArrayIndex = In.TexArrInd;
#else
	float2 elevationMapUV = In.ElevationMapUV;
	float elevDataTexArrayIndex = PSC_Custom_3.x;
#endif


	#define MAX_TESS_BLOCK_ERROR 1.7e+34f

    float tessBlockErrors[4] = {0, 0, 0, 0};

    float2 srcElevDataTextureSize;
    float numSlices;

    tClipMap.GetDimensions( srcElevDataTextureSize.x, srcElevDataTextureSize.y, numSlices );

    float2 elevDataTexelSize = 1.f / srcElevDataTextureSize;

	float2 elevDataMaxTexCoord = srcElevDataTextureSize - 1;

#ifdef GET_ELEV
#   undef GET_ELEV
#endif

#define GET_ELEV( Col, Row ) tClipMap.Sample( sPointClamp, float3(elevationMapUV.xy + float2(Col, Row) * elevDataTexelSize, elevDataTexArrayIndex ) )

	bool hasHole = false;

    for( int coarseLevel = 1; coarseLevel <= 4; coarseLevel++ )
    {
        float tessBlockCurrLevelError = 0.f;
        int stepSize = 1 << coarseLevel;
        // Minimum tessellation factor for the tessellation block is 2, which 
        // corresponds to the step g_iTessBlockSize/2. There is no point in 
        // considering larger steps
        //
        //g_iTessBlockSize
        //  |<----->|
        //   _______ 
        //  |\  |  /|
        //  |__\|/__|
        //  |  /|\  |
        //  |/__|__\|
        if( stepSize > TESS_BLOCK_RES/2 )
            break;

        // Tessellation block covers height map samples in the range [-g_iTessBlockSize/2, g_iTessBlockSize/2]
        for(int _row = -(TESS_BLOCK_RES>>1); _row <= (TESS_BLOCK_RES>>1); _row++)
            for(int _col = -(TESS_BLOCK_RES>>1); _col <= (TESS_BLOCK_RES>>1); _col++)
            {
                int col0 = _col & (-stepSize);
                int row0 = _row & (-stepSize);
                int col1 = col0 + stepSize;
                int row1 = row0 + stepSize;

                float horzWeight = ((float)_col - (float)col0) / (float)stepSize;
                float vertWeight = ((float)_row - (float)row0) / (float)stepSize;

				float4 elev;
                elev.x = GET_ELEV(col0, row0);
                elev.y = GET_ELEV(col1, row0);
                elev.z = GET_ELEV(col0, row1);
                elev.w = GET_ELEV(col1, row1);
				elev = lerp( TerrainParams.lowestElevation.xxxx, TerrainParams.highestElevation.xxxx, elev );
                float interpolatedElev = lerp( lerp( elev.x, elev.y, horzWeight ), lerp( elev.z, elev.w, horzWeight ), vertWeight );

                float currElev = GET_ELEV( _col, _row );
				currElev = lerp( TerrainParams.lowestElevation, TerrainParams.highestElevation, currElev );
                float currElevError = abs( currElev - interpolatedElev);

				if ( elevDataTexArrayIndex == 0.0f && coarseLevel == 1 )
				{
					// If a terrain hole is here, apply a huge error value.
					uint controlMapValue = tCMClipMap.Load( int4( ( elevationMapUV.xy + float2( _col, _row ) * elevDataTexelSize ) * elevDataMaxTexCoord.xy, (int)elevDataTexArrayIndex, 0 ) );
					if ( controlMapValue == 0 )
					{
						hasHole = true;
					}
				}

                tessBlockCurrLevelError = max( tessBlockCurrLevelError, currElevError);
            }

        tessBlockErrors[coarseLevel-1] = tessBlockCurrLevelError;
    }

	if ( hasHole ) tessBlockErrors[3] = 1.0f;

    // Load tessellation block error with respect to finer levels
    // TessBlockErrorBounds texture[array] has the same dimension as the tess block error texture
    // being rendered. Thus we can use ElevationMapUV to load the data. The only thing is that we 
    // need to remove offset by 0.5*ElevDataTexelSize

    //float TessBlockErrorBound = g_tex2DTessBlockErrorBoundsArr.Sample(samPointClamp, float3(ElevationMapUV.xy - 0.5*ElevDataTexelSize, InstID)) * HEIGHT_MAP_SAMPLING_SCALE;

    return float4( tessBlockErrors[0], tessBlockErrors[1], tessBlockErrors[2], tessBlockErrors[3] );// + TessBlockErrorBound;
}

#endif
