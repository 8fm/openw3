#include "common.fx"
#include "include_msaa.fx"

#if IS_BUFFER_RESOLVE
Texture2D<float4>		ColorTexture		: register(t0);
#else
TEXTURE2D_MS<float4>		ColorTexture		: register(t0);
#endif

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

	float4 result = 0;
	
#if IS_BUFFER_RESOLVE
	{
		const uint2 subPixelCoord = GetMSAABufferSampleCoord( pixelCoord, 0 );
		result = ColorTexture[ subPixelCoord ];
	}
#else
	{
		result = ColorTexture.Load( pixelCoord, 0 );
	}
#endif
	
	return result;
}



#endif
