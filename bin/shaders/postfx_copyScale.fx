#include "postfx_common.fx"


#define vTexCoordTransform VSC_Custom_0
#define vTexCoordClamp PSC_Custom_0
#define vScale PSC_Custom_1

SamplerState	s_TextureColor      : register( s0 );
Texture2D		t_TextureColor		: register( t0 );


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

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	float2 coord      = ApplyTexCoordClamp( i.coord, vTexCoordClamp );
    float4 orig_color = t_TextureColor.Sample( s_TextureColor, coord );
    return orig_color * vScale.xxxx;
}
#endif
