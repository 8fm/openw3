#include "postfx_common.fx"


#define vTexCoordTransform	VSC_Custom_0
#define vTexCoordClamp		PSC_Custom_0

SamplerState	s_TextureDepth  : register( s0 );
Texture2D		t_TextureDepth	: register( t0 );


struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
	float2 coord       : TEXCOORD0;
};

#ifdef VERTEXSHADER
VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   = i.pos;
	o.coord = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransform );

	return o;
}
#endif

#ifdef PIXELSHADER

float ps_main( VS_OUTPUT i ) : SYS_DEPTH_OUTPUT
{
	float2 coord = ApplyTexCoordClamp( i.coord, vTexCoordClamp );
    return t_TextureDepth.Sample( s_TextureDepth, coord ).x;
}
#endif
