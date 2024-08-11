#include "postfx_common.fx"

#define TAPS_COUNT 10

#define vTexCoordTransform VSC_Custom_0
#define vTexCoordClamp     PSC_Custom_0
//float4  vTaps[TAPS_COUNT]  custom_register(59); //PSC_Custom_1

SamplerState	s_TextureColor	: register( s0 );
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
	
	float4 vTaps[TAPS_COUNT];
	vTaps[0] = PSC_Custom_1;
	vTaps[1] = PSC_Custom_2;
	vTaps[2] = PSC_Custom_3;
	vTaps[3] = PSC_Custom_4;
	vTaps[4] = PSC_Custom_5;
	vTaps[5] = PSC_Custom_6;
	vTaps[6] = PSC_Custom_7;
	vTaps[7] = PSC_Custom_8;
	vTaps[8] = PSC_Custom_9;
	vTaps[9] = PSC_Custom_10;

	[unroll]
    for ( int tap_i=0; tap_i<TAPS_COUNT; tap_i+=1 )
    {
        float4 tap    = vTaps[tap_i];
	    float2 coord  = ApplyTexCoordClamp( i.coord + tap.xy, vTexCoordClamp );
        result       += tap.z * t_TextureColor.Sample( s_TextureColor, coord );
	}

	return result;
}
#endif
