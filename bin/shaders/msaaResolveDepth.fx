#include "common.fx"
#include "include_msaa.fx"

TEXTURE2D_MS<float>		DepthTexture		: register(t0);


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

float ps_main( VS_OUTPUT i ) : SYS_DEPTH_OUTPUT
{
	const uint2 pixelCoord = i.pos.xy;
	return DepthTexture.Load( pixelCoord, 0 ).x;
}

#endif
