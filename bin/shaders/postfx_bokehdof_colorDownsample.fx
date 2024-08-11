#include "blurSamples.fx"
#include "postfx_common.fx"
#include "include_constants.fx"

#define vTexCoordTransformFull	VSC_Custom_0
#define vTexCoordTransformHalf	VSC_Custom_1

#define vTexCoordClamp		PSC_Custom_0
#define vTexCoordDim		PSC_Custom_1
#define samplingRatio		PSC_Custom_3.xy
#define maxCoCsize			PSC_Custom_2.w

static const float invSampleCount = 1.0f/(SAMPLE_COUNT_0*2);

SamplerState	s_TextureOrgiginal			: register( s0 );
Texture2D		t_TextureOrgiginal			: register( t0 );

SamplerState	s_CacheColor				: register( s1 );
Texture2D		t_CacheColor				: register( t1 );

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos			: SYS_POSITION;
	float2 coord		: TEXCOORD0;
};

struct PS_OUTPUT
{
	float4 colorNear	: SYS_TARGET_OUTPUT0;
	float4 colorFar		: SYS_TARGET_OUTPUT1;
	float4 coc			: SYS_TARGET_OUTPUT2;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   = i.pos;
	o.coord = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformFull );

	return o;
}

#endif

#ifdef PIXELSHADER

void ps_main( VS_OUTPUT i, out PS_OUTPUT o )
{	
	float2 uv = i.coord.xy;
	/*
	int2 coord = int2( i.pos.xy ) * 4;

	float4 colorNear =  t_TextureOrgiginal[ coord ];
	float coc	     =  t_CacheColor[ coord ].z;

	float4 colorFar = colorNear;

	float n = max( coc * -2.0 + 1.0 , 0.0 );
	float f = max( coc *  2.0 - 1.0 , 0.0 );

	float4 avgCoC = float4( n * 16.0 , 0.001 , f * 16.0f , 0.001 );
	*/
	
	float4 colorNear = float4( 0,0,0,0.001f );
	float4 colorFar = float4( 0,0,0,0.001f );

	float2 avgCoC = float2( 0.0 , 0.0 );

	[unroll]
	for( int x = 0; x < 4; ++x )
	{
		[unroll]
		for( int y = 0; y < 4; ++y )
		{
			int2 coord = int2( i.pos.xy ) * 4 + int2( x , y );

			float4 color =  t_TextureOrgiginal[ coord ];
			float  coc	 =  t_CacheColor[ coord ].z;

			float n = max( coc * -2.0 + 1.0 , 0.0 );
			float f = max( coc *  2.0 - 1.0 , 0.0 );

			colorNear += color * max( n , 0.001f );
			colorFar  += color * max( f , 0.001f );

			avgCoC.x += n * n;
			avgCoC.y += f * f;
		}
	}
	
	float totalAvgCoc = sqrt( ( avgCoC.x + avgCoC.y ) / 32.0 );

	float biggestCircleOfConfusion = 0.0;
	float samplingRadius = totalAvgCoc * maxCoCsize * 0.05;
	
	[unroll]
	for (int i = 0; i < SAMPLE_COUNT_0; i++ )
	{
		float sampledCoC = t_CacheColor.Sample( s_CacheColor , samples_0[i] * samplingRadius + uv ).x;
		biggestCircleOfConfusion = max( biggestCircleOfConfusion, sampledCoC );
	}

	o.colorNear	= colorNear / colorNear.a;
	o.colorFar	= colorFar / colorFar.a;

	o.coc.x		= biggestCircleOfConfusion;
	o.coc.y		= sqrt( avgCoC.x / 16.0f ); // for near bokeh and bluring
	o.coc.z		= sqrt( avgCoC.y / 16.0f ); // for far bokeh weight

}
#endif
