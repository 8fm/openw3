#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"


SamplerState	s_TextureColor : register( s0 );
Texture2D		t_TextureColor : register( t0 );


struct VS_INPUT
{
    float4 pos      : POSITION0;
};

struct VS_OUTPUT
{
    float4 _pos    : SYS_POSITION;
};

struct PS_INPUT
{
	float4 vpos    : SYS_POSITION;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o = (VS_OUTPUT)0;

	o._pos  = i.pos;

	return o;
}

#endif

#ifdef PIXELSHADER


float4 ps_main( PS_INPUT i ) : SYS_TARGET_OUTPUT0
{
	const float edgeKernel1[9] =
	{
		-1.0f, 0.0f, 1.0f,
		-2.0f, 0.0f, 2.0f,
		-1.0f, 0.0f, 1.0f,
	};

	const float edgeKernel2[9] =
	{
		-1.0f, -2.0f, -1.0f,
		 0.0f,  0.0f,  0.0f,
		 1.0f,  2.0f,  1.0f,
	};

	const float2 offsets[9] =
	{
		{-1,-1}, {0,-1}, {1,-1},
		{-1, 0}, {0, 0}, {1, 0},
		{-1, 1}, {0, 1}, {1, 1},
	};


	const float4 fillColor = PSC_Custom_0;
	const float4 borderColor = PSC_Custom_1;
	const float borderWidth = PSC_Custom_2.x;
	const int group = int( PSC_Custom_2.y );

	// Get this pixel's contribution to the current outline group.
	const float thisPixel = t_TextureColor.Sample( s_TextureColor, (i.vpos.xy + HALF_PIXEL_OFFSET ) * PSC_ViewportSize.zw )[ group ];
	// If no contribution, early-out.
	if ( thisPixel == 0 ) discard;

	float2 filterResult = 0.0f;
	for ( int j = 0; j < 9; j++ )
	{
		float2 coord = (i.vpos.xy + borderWidth * offsets[j] + HALF_PIXEL_OFFSET ) * PSC_ViewportSize.zw;
		float groupWeight = t_TextureColor.Sample( s_TextureColor, coord )[ group ];

		filterResult += abs( thisPixel - groupWeight ) * float2( edgeKernel1[j], edgeKernel2[j] );
	}
	
	// sqrt(18) == 4.24264 is maximal result:
	//	001
	//	001 (or many others that give the same)
	//	111
	float borderStrength = length( filterResult ) / 4.24264f;
	return lerp( fillColor, borderColor, borderStrength ) * thisPixel;
}
#endif
