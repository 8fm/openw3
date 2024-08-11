#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"

Texture2D<float>sShadowMap	: register( t0 );
SamplerState	samp		: register( s0 );

struct VS_INPUT
{
	float4 pos		: POSITION0;
	float4 color	: COLOR0;
	float2 uv		: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos		: SYS_POSITION;
	float4 color	: TEXCOORD0;
	float2 uv		: TEXCOORD1;
	float3 dir		: TEXCOORD2;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos = float4( float2(-1,1) + i.pos.xy * float2(2,-2), 0, 1 );

	// Reproject magic
	const float nf = i.uv.y;
	const float zn = 0.05f;
	const float zf = nf / zn;
	const float fn = zf - zn;
	const float2 param = float2( zf / nf , -fn / nf );

	// Read encoded tangent value and use it to reconstruct distance from near plane
	float2 coord = i.color.xy * 2.0f - float2(1.0,1.0);
	float  offlineTangent = dot( i.color.zw , float2( 64.0 , 0.25 ) );

	// scale dir by inverse radius to output distance in range [0..1]
	o.dir = float3( coord * offlineTangent , 1.0 ) / zf;
	
	o.color = float4( i.pos.xy, param );
	o.uv = i.uv;

	return o;
}

#endif

#ifdef PIXELSHADER

//#define UBER

#ifdef UBER

static const float weights[7][7] = {
	{ 1,12,55,90,55,12,1},
	{12,148,665,1097,665,148,12},
	{55,665,2981,4915,2981,665,55},
	{90,1097,4915,8103,4915,1097,90},
	{55,665,2981,4915,2981,665,55},
	{12,148,665,1097,665,148,12},
	{ 1,12,55,90,55,12,1},
};

#else

static const float weights[5][5] = {
	{ 1, 4, 7, 4, 1 },
	{ 4, 16, 26, 16, 4 }, 
	{ 7, 26, 41, 26, 7 }, 
	{ 4, 16, 26, 16, 4 }, 
	{ 1, 4, 7, 4, 1 }
};

#endif

float DecomposeDepth( int2 src, int2 offset , float2 f )
{
	const float z = sShadowMap.Load( int3( src, 0 ), offset );
	return rcp( z * f.y + f.x );
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	// get the source pixel coordinates
	const float pixelOffset = i.uv.x;
	const float2 param = i.color.zw;
	const int2 SrcPixel = (int2)( i.pos.xy + pixelOffset.xx );

	float lengthScale = length(i.dir);

#ifdef UBER

	// gauss 7x7
	// TODO: make this separable ?
	float depth = 0;
	[unroll]
	for ( int dy=0; dy<7; ++dy )
	{
		[unroll]
		for ( int dx=0; dx<7; ++dx )
		{
			depth += DecomposeDepth( SrcPixel, int2( dx-1, dy-1 ), param ) * weights[dx][dy];
		}
	}

	return lengthScale * ( depth.xxxx / 50887.0f );

#else

	// gauss 5x5
	// TODO: make this separable ?
	float depth = 0;
	[unroll]
	for ( int dy=0; dy<5; ++dy )
	{
		[unroll]
		for ( int dx=0; dx<5; ++dx )
		{
			depth += DecomposeDepth( SrcPixel, int2( dx, dy ), param ) * weights[dx][dy];
		}
	}
	return lengthScale * ( depth.xxxx / 273.0f );

#endif

}

#endif
