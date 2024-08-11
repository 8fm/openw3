#include "postfx_common.fx"

#define fAdaptationAlphaUp		PSC_Custom_0.x
#define fAdaptationAlphaDown	PSC_Custom_0.y

SamplerState	s_Luminance		: register( s0 );
Texture2D		t_Luminance		: register( t0 );

SamplerState	s_LuminanceOld	: register( s1 );
Texture2D		t_LuminanceOld	: register( t1 );


struct VS_INPUT
{
	float4 pos	: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos  	: SYS_POSITION;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;	
	
	o.pos   = i.pos;	

	return o;
}

#endif

#ifdef PIXELSHADER

#ifdef __PSSL__
#pragma PSSL_target_output_format(default FMT_32_AR)
#endif

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const float3 new_lum = SAMPLE_LEVEL( t_Luminance, s_Luminance, float2( 0.0f,0.0f ), 0.0f ).xyz;
	const float3 old_lum = SAMPLE_LEVEL( t_LuminanceOld, s_LuminanceOld, float2( 0.0f,0.0f ), 0.0f ).xyz;

	const float adapt_alpha = new_lum.x >= old_lum.x ? fAdaptationAlphaUp : fAdaptationAlphaDown;
	const float lum = lerp( new_lum.x, old_lum.x, adapt_alpha );
	
	// The luminance we are looking for
	return lum.xxxx;
}
#endif
