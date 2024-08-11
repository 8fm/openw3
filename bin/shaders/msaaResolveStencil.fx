#include "common.fx"
#include "include_msaa.fx"

TEXTURE2D_MS<float4>		MaskTexture		: register(t0);


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

	o.pos   = i.pos;

	return o;
}
#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const uint2 pixelCoord = i.pos.xy;

	if ( MaskTexture.Load( pixelCoord, 0 ).x <= 0.5 )
	{
		discard;
	}

	return 1;
}

#endif
