#include "postfx_common.fx"

Texture2D		TextureColor		: register( t0 );

#define PASS_OFFSET			PSC_Custom_0.x
#define OFFSET_MASK			PSC_Custom_0.yz

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
};

#ifdef VERTEXSHADER
VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;
	o.pos = i.pos;	
	return o;
}
#endif

#ifdef PIXELSHADER

#pragma PSSL_target_output_format(default FMT_32_ABGR)

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	uint2 vpos = (uint2)i.pos.xyz;

	float4 val0 = TextureColor[vpos];
	float4 val1 = 0;

	if ( dot( OFFSET_MASK, vpos ) >= PASS_OFFSET )
	{
		val1 = TextureColor[vpos - PASS_OFFSET * OFFSET_MASK];
	}

	return val0 + val1;
}
#endif
