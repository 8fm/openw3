#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"

SamplerState	s_TextureColor      : register( s0 );
Texture2D		t_TextureColor		: register( t0 );

SamplerState	s_TextureDelta      : register( s1 );
Texture2D		t_TextureDelta		: register( t1 );

#define vRefractionScale PSC_Custom_0
#define vClampValue PSC_Custom_1

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

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o = (VS_OUTPUT)0;

	o._pos  = i.pos;
	
	return o;
}

#ifdef PIXELSHADER

float4 ps_main( PS_INPUT i ) : SYS_TARGET_OUTPUT0
{
    float2 coord 	 = (i.vpos.xy + HALF_PIXEL_OFFSET ) * PSC_ViewportSize.zw;	
	float4 smp_delta = t_TextureDelta.Sample( s_TextureDelta, coord );
	float2 delta	 = (smp_delta.xy - smp_delta.zw) * vRefractionScale.xy * ACCUMULATIVE_REFRACTION_MAX_OFFSET;
	
	coord				= min( coord + delta, vClampValue.zw );
	float4 smp_color	= t_TextureColor.Sample( s_TextureColor, coord );
	float4 result_color = float4 ( smp_color.xyz, 1.0 );

#if MSAA_NUM_SAMPLES > 1
	result_color.a = saturate( length(delta *  PSC_ViewportSize.xy) / 1.0 );
#endif
	
	return result_color;
}

#endif
