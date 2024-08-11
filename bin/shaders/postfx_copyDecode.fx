#include "postfx_common.fx"
#include "include_constants.fx"
#include "include_utilities.fx"
#include "include_sharedConsts.fx"

Texture2D tColorTexture		: register( t0 );

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
	int2 vpos  = (int2)i.pos.xy;
	return float4( tColorTexture[vpos].xyz, 1.0f );
}
#endif
