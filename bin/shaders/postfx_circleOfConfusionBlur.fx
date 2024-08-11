#include "bokehSamples.fx"
#include "postfx_common.fx"

#define vTexCoordTransformFull	VSC_Custom_0
#define vTexCoordTransformHalf	VSC_Custom_1

#define vTexCoordClamp		PSC_Custom_0
#define vTexCoordDim		PSC_Custom_1
#define vTexCameraParams	PSC_Custom_2
#define bokehRatio			PSC_Custom_3.xy
#define blurDir				PSC_Custom_6.xy

SamplerState	s_CacheColor				: register( s0 );
Texture2D		t_CacheColor				: register( t0 );

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

	o.coord = ApplyTexCoordTransform( i.pos.xy , vTexCoordTransformHalf );
	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{		
	float2 uv = i.coord.xy;
	
	float oryginalCoC = t_CacheColor.Sample( s_CacheColor, uv ).x;

	float2 radius = 0.002 * blurDir;
	
	float value = 0.0;

	[unroll]
	for (int i = -6; i <= 6; i++ )
	{
		float2 coord = radius * float(i) + uv;
		float sampled = t_CacheColor.Sample( s_CacheColor, coord ).y;
		value = max( value , sampled * ( 1.0 - abs( i / 7.0f ) ) );
	}
	
	return float4( oryginalCoC , value , 0 , 1 );
}
#endif
