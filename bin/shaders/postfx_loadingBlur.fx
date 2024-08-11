#define BOKEH_SUPER_DUPER_SAMPLES_LVL 0

#include "bokehSamples.fx"
#include "postfx_common.fx"

#define vTexCoordTransform VSC_Custom_0

#define blurForce			PSC_Custom_0.x
#define sharpToBlurScale	PSC_Custom_0.y
#define fadeForce			PSC_Custom_0.z

#define vCoordMax			PSC_Custom_1.xy
#define vTexelSize			PSC_Custom_1.zw

SamplerState	s_TextureColor	: register( s0 );
Texture2D		t_TextureColor	: register( t0 );

SamplerState	s_BlurColor	: register( s1 );
Texture2D		t_BlurColor	: register( t1 );

SamplerState	s_BlurColor2	: register( s2 );
Texture2D		t_BlurColor2	: register( t2 );

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
#ifdef FINAL_BLEND

	int2 iuv = int2( i.pos.xy );

	float4 originalColor	= t_TextureColor[ iuv ];

	float4 prevColor		= t_BlurColor.Sample( s_BlurColor , i.coord );
	
	return lerp( originalColor , prevColor , fadeForce );
#else
	
	float force = blurForce;

	int2 uvO = int2( i.pos.xy * 8 );
	float4 originalColor	= t_TextureColor[ uvO ];
	[unroll]
	for( int k = 1; k < 8; k += k ) // 1 , 2 , 4
	{
		originalColor	+= t_TextureColor[ uvO + int2( k , k ) ];
		originalColor	+= t_TextureColor[ uvO + int2( -k , -k ) ];
		originalColor	+= t_TextureColor[ uvO + int2( -k , k ) ];
		originalColor	+= t_TextureColor[ uvO + int2( k , -k ) ];
	}

	originalColor *= 0.0769231; // 1.0 / ( 1.0f + 3.0 * 4.0f );

	float4 prevColor		= float4(0,0,0,0);

	[loop]
	for ( int j = 0; j < BOKEH_SAMPLE_COUNT; j++ )
	{
		float2 off		= samples[j].xy * vTexelSize.xy;
		float2 crd		= clamp( i.coord + off , float2(0,0), vCoordMax );

		prevColor		+= t_BlurColor.Sample( s_BlurColor , crd );
	}

	return lerp( originalColor , prevColor/BOKEH_SAMPLE_COUNT , sharpToBlurScale );
#endif
}
#endif
