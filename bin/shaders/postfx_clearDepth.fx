
#include "postfx_common.fx"


// Parameters
#define value4RT0 PSC_Custom_0


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

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o._pos  = i.pos;

	return o;
}

#ifdef PIXELSHADER

float4 ps_main( PS_INPUT i ) : SYS_TARGET_OUTPUT0
{
    return value4RT0;
}

#endif
