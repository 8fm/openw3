#ifdef IS_SKY_FILTER
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

#include "postfx_common.fx"

Texture2D		t_TextureColor	: register( t0 );
Texture2D		t_TextureDepth	: register( t1 );

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
	const int2 pixelCoord = (int2)i.pos.xy;
	const float depth = t_TextureDepth[pixelCoord].x;
	const float3 color = t_TextureColor[pixelCoord].xyz;
	const float skyFactor = (IsSkyByProjectedDepthRevProjAware( depth ) ? 1.f : 0.f);
	return float4 ( color * skyFactor, 1.f );
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
#else
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

#include "postfx_common.fx"

Texture2D		t_TextureColor	: register( t0 );
SamplerState	s_TextureColor  : register( s0 );
#define			vTextureFullRes	PSC_Custom_0.xy
#define			vTextureAreaRes	PSC_Custom_1.xy
#define			vTargetFullRes	PSC_Custom_2.xy
#define			vTargetAreaRes	PSC_Custom_3.xy
#define			iSourceOffsetXY	((int2)PSC_Custom_4.xy)
#define			iTargetOffsetXY	((int2)PSC_Custom_4.zw)
#define			vClampRange		PSC_Custom_5
#define			fScale				PSC_Custom_6.x
#if IS_BRIGHTPASS
# define		fBrightnessMax		PSC_Custom_7.x
# define		fBrightThreshold	PSC_Custom_8.x
# define		fBrightInvRange		PSC_Custom_8.y
# define		v3Weights			PSC_Custom_9.xyz
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
	const int2 pixelCoord = (int2)i.pos.xy - iTargetOffsetXY;

	float2 coord = (((pixelCoord + 0.5) / vTargetAreaRes * vTextureAreaRes) + iSourceOffsetXY) / vTextureFullRes;
	float2 clampMin = (vClampRange.xy + 0.5) / vTextureFullRes;
	float2 clampMax = (vClampRange.zw + 0.5) / vTextureFullRes;
	
	float3 col = 0;
	col += SAMPLE_LEVEL( t_TextureColor, s_TextureColor, clamp( coord + float2(-1,-1) / vTextureFullRes, clampMin, clampMax ), 0 ).xyz;
	col += SAMPLE_LEVEL( t_TextureColor, s_TextureColor, clamp( coord + float2( 1,-1) / vTextureFullRes, clampMin, clampMax ), 0 ).xyz;
	col += SAMPLE_LEVEL( t_TextureColor, s_TextureColor, clamp( coord + float2(-1, 1) / vTextureFullRes, clampMin, clampMax ), 0 ).xyz;
	col += SAMPLE_LEVEL( t_TextureColor, s_TextureColor, clamp( coord + float2( 1, 1) / vTextureFullRes, clampMin, clampMax ), 0 ).xyz;
	col *= 0.25;

#if IS_BRIGHTPASS
	
	float lumCurr  = dot( v3Weights, col.xyz );
	float lumDiff = max( 0, lumCurr - fBrightThreshold );
	float lumTarget = lumDiff * saturate( lumDiff * fBrightInvRange );
	
	col *= min( fBrightnessMax, lumTarget ) / max( 0.0001, lumCurr );
	
#endif

	col *= fScale;

	return float4( col, 0 );
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
#endif
