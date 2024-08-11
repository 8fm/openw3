#include "postfx_common.fx"

#define vTexCoordTransform VSC_Custom_0
#define vTexCoordClamp     PSC_Custom_0
#define vDirection         PSC_Custom_1

SamplerState	s_TextureColor  : register( s0 );
Texture2D		t_TextureColor	: register( t0 );

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
    float4 result = 0;

	float total_weight = 0.0f;
	
	[unroll]
#if LESS	
    for ( int tap_i=0; tap_i<4; tap_i+=1 )
#else
	for ( int tap_i=0; tap_i<14; tap_i+=1 )
#endif
    {
#if LESS	
		float weight = 1.0f/pow(1.0f+abs(tap_i-1.5f),0.7f);
        float2 coord  = i.coord + vDirection.xy * (tap_i-1.5f) * 2 - vDirection.xy * 0.5f;
#else
		float weight = 1.0f/pow(1.0f+abs(tap_i-6),0.7f);
        float2 coord  = i.coord + vDirection.xy * (tap_i-6) * 2 - vDirection.xy * 0.5f;
#endif		
        result       += weight * t_TextureColor.Sample( s_TextureColor, ApplyTexCoordClamp( coord, vTexCoordClamp ) );
		total_weight += weight;
    }

    return result/total_weight;
}

#endif
