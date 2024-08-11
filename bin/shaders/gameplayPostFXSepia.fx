#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"


SamplerState	s_TextureColor      : register( s0 );
Texture2D		t_TextureColor		: register( t0 );

#define desaturation PSC_Custom_0

struct VS_INPUT
{
    float4 pos      : POSITION0;
};

struct VS_OUTPUT
{
    float4 vec_v   : TEXCOORD0;
    float4 _pos    : SYS_POSITION;
};

struct PS_INPUT
{
    float4 vec_v   : TEXCOORD0;
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
    float2 coord 	 = (i.vpos.xy + HALF_PIXEL_OFFSET ) * PSC_ViewportSize.zw;
	float4 smp_color = t_TextureColor.Sample( s_TextureColor, coord );

	float val = dot( smp_color.xyz, RGB_LUMINANCE_WEIGHTS_LINEAR_Sepia );
	float4 desaturated = float4( val * 1.2, val * 1.0, val * 0.8, smp_color.w );
	
	return lerp(smp_color, desaturated, desaturation.x );
}
#endif
