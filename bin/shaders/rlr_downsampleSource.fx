#include "postfx_common.fx"
#include "include_localReflection.fx"

Texture2D		t_TextureColor	: register( t0 );
Texture2D		t_TextureDepth	: register( t1 );
#if IS_COLOR
SamplerState	s_Linear		: register( s0 );
#endif

#define vRenderTargetSize		(PSC_Custom_0.xy)
#define vDownsampleRatio		(PSC_Custom_1)


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

#if IS_DEPTH
#pragma PSSL_target_output_format(default FMT_32_R)
#endif

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const uint2 hiResPixelCoord = (uint2)(vDownsampleRatio.zw * floor( i.pos.xy ));
	
#if IS_COLOR

	float4 value = t_TextureColor[ (int2)hiResPixelCoord ];

	// ace_hack: megahack for dealing with high values, which result in perceptive instant blendin into reflections accumulation buffer.
	{
		float lum = dot( 0.33333, value.xyz );
		float max_lum = 10.f;
		value.xyz *= min( lum, max_lum ) / max( 0.0001, lum );
	}

	return value;

#elif IS_DEPTH

	float value = t_TextureColor[ hiResPixelCoord ].x;

	return value;

#else

# error INVALID VERSION

#endif	
}
#endif
