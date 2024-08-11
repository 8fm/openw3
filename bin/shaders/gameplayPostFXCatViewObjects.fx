#define BOKEH_SUPER_DUPER_SAMPLES_LVL 0

#include "bokehSamples.fx"
#include "postfx_common.fx"

#define vTexCoordTransform	VSC_Custom_0

#define blurSize			PSC_Custom_0.x
#define hightlightInterior	PSC_Custom_0.y

#define vCoordMax			PSC_Custom_1.xy
#define vTexelSize			PSC_Custom_1.zw

Texture2D<uint2> StencilTexture		: register(t0);

#define LC_VisibleThroughWalls	(1 << 5)

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

float IsEnemy( float2 uv )
{
	return ( GetStencilValue( StencilTexture[ int2(uv*2) ] ) & LC_VisibleThroughWalls ) ? 1.0 : 0.0;
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	float2 uv = i.pos.xy * vCoordMax;

	float origMask = IsEnemy( uv );

	float avgMask		= 0.0f;

	[loop]
	for ( int j = 0; j < BOKEH_SAMPLE_COUNT; j++ )
	{
		float	mask = 1.0 - IsEnemy( samples[j].xy * 8.0 + uv );	
		avgMask		+= mask;
	}
	
	float result = ( avgMask/BOKEH_SAMPLE_COUNT ) * origMask + origMask * hightlightInterior;
	
	return float4( result , 0.0, 0.0, 1.0 );
}
#endif
