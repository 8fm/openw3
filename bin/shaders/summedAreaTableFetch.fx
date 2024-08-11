#include "postfx_common.fx"

SamplerState	Sampler				: register( s0 );
Texture2D		TextureColor0		: register( t0 );
Texture2D		TextureColor1		: register( t1 );


#define TARGET_RESOLUTION		PSC_Custom_0.x
#define FACE_INDEX				PSC_Custom_0.y
#define MIP_INDEX				PSC_Custom_0.z
#define FILTER_SIZE				PSC_Custom_0.w
#define DIR_TRANSFORM_X			PSC_Custom_1.xyz
#define DIR_TRANSFORM_Y			PSC_Custom_2.xyz
#define DIR_TRANSFORM_Z			PSC_Custom_3.xyz


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

float4 SampleSAT( Texture2D tex, float2 uv, float2 size )
{
	float4 result = SAMPLE_LEVEL( tex, Sampler, uv + 0.5 * size, 0 );
	result -= SAMPLE_LEVEL( tex, Sampler, uv + float2( 0.5, -0.5 ) * size, 0 );
	result -= SAMPLE_LEVEL( tex, Sampler, uv + float2( -0.5, 0.5 ) * size, 0 );
	result += SAMPLE_LEVEL( tex, Sampler, uv - 0.5 * size, 0 );
	result /= size.x * size.y;

// 	if ( uv.x <= 0 || uv.y <= 0 || uv.x >= 1 || uv.y >= 1 )
// 	{
// 		result = 0;
// 	}

	return result;
}

float3 FilterSAT( float3 dir, float2 filterSize )
{
	float2 uv0 = float2( 0.5, -0.5 ) * dir.xy / max(0.001, 1.0 - dir.z) + 0.5;
	float2 uv1 = float2( -0.5, 0.5 ) * dir.xy / max(0.001, 1.0 + dir.z) + 0.5;

	float4 val0 = SampleSAT( TextureColor0, uv0, filterSize );
	float4 val1 = SampleSAT( TextureColor1, uv1, filterSize );
	float4 valSum = val0 + val1;
	//float4 valSum = val0/max(0.01, val0.w) + val1/max(0.01, val1.w);// + val1;

	return valSum.xyz / max( 0.1, valSum.w );
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	uint2 vpos = (uint2)i.pos.xy;
	
	const float3 dir = mul( normalize( float3 ( (vpos + 0.5) / (float2)(TARGET_RESOLUTION) * 2.f - 1.f, -1.f ) ), float3x3( DIR_TRANSFORM_X, DIR_TRANSFORM_Y, DIR_TRANSFORM_Z ) );

	float4 result = 0;	
	result += float4 ( FilterSAT( dir, FILTER_SIZE ), 1 );

//	result += float4 ( FilterSAT( dir, FILTER_SIZE ), 1 ) * 0.25;
// 	result += float4 ( FilterSAT( dir, FILTER_SIZE * 0.5 ), 1 ) * 0.5;
//	result += float4 ( FilterSAT( dir, FILTER_SIZE * 0.25 ), 1 ) * 0.75;
//	result += float4 ( FilterSAT( dir, FILTER_SIZE * 0.15 ), 1 ) * 1.00;

	/*
	float2 shape0 = float2 ( 1, 0.15);
	float2 shape1 = float2 ( 0.8, 0.5 );
	result += float4 ( FilterSAT( dir, shape0.xy * FILTER_SIZE ), 1 ) * 1;
	result += float4 ( FilterSAT( dir, shape0.yx * FILTER_SIZE ), 1 ) * 1;
	result += float4 ( FilterSAT( dir, shape1.xy * FILTER_SIZE ), 1 ) * 1;
	result += float4 ( FilterSAT( dir, shape1.yx * FILTER_SIZE ), 1 ) * 1;
	//*/

	/*
	FILTER_SIZE = 1.0 / 128;

	float2 shape0 = float2 ( 1, 1);
	float2 shape1 = float2 ( 0.8, 0.4 );
	result += float4 ( FilterSAT( dir, 0.25 * FILTER_SIZE ), 1 ) * 1;
	result += float4 ( FilterSAT( dir, 0.45 * FILTER_SIZE ), 1 ) * 1;
	result += float4 ( FilterSAT( dir, 0.55 * FILTER_SIZE ), 1 ) * 1;
	result += float4 ( FilterSAT( dir, 0.60 * FILTER_SIZE ), 1 ) * 1;
	*/

	return result / result.w;
}
#endif
