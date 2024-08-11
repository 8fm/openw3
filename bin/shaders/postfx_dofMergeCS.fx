#include "postfx_common.fx"

#define ENABLE_CLAMP 1

#define vTexCoordTransformColor	VSC_Custom_0
#define vTexCoordTransformBlur	VSC_Custom_1
#define vTexelOffset			PSC_Custom_0
#define vDofParamsRange			PSC_Custom_1
#define vDofParamsIntensity		PSC_Custom_2
#define vClampParams			PSC_Custom_3

SamplerState	s_TextureColor      : register( s0 );
Texture2D		t_TextureColor		: register( t0 );

SamplerState	s_TextureBlurLow    : register( s1 );
Texture2D		t_TextureBlurLow	: register( t1 );

SamplerState	s_TextureBlurHigh   : register( s2 );
Texture2D		t_TextureBlurHigh	: register( t2 );

SamplerState	s_TextureDepth	   	: register( s3 );
Texture2D		t_TextureDepth		: register( t3 );

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
	float2 coordColor  : TEXCOORD0;
	float2 coordBlur   : TEXCOORD1;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   = i.pos;
	o.coordColor = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformColor );
	o.coordBlur  = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformBlur );

	return o;
}

#endif

#ifdef PIXELSHADER

float CalcBlurWeight( float depth )
{
#ifndef NO_FAR
	return	
		saturate( (vDofParamsRange.x - depth) * vDofParamsRange.y ) +
		saturate( (depth - vDofParamsRange.z) * vDofParamsRange.w );
#else
	return	
		saturate( (vDofParamsRange.x - depth) * vDofParamsRange.y );
#endif
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
#if ENABLE_CLAMP
	const float2 coord_blur_low		= min( vClampParams.xy, i.coordBlur );
	const float2 coord_blur_high	= min( vClampParams.zw, i.coordBlur );
#else
	const float2 coord_blur_low		= i.coordBlur;
	const float2 coord_blur_high	= i.coordBlur;
#endif

	const float4 orig_color 		= t_TextureColor.	Sample( s_TextureColor, 	i.coordColor );
	const float4 blur_color_low 	= t_TextureBlurLow.	Sample( s_TextureBlurLow,	coord_blur_low );
	const float4 blur_color_high	= t_TextureBlurHigh.Sample( s_TextureBlurHigh, 	coord_blur_high );
	
	// weight
	const float depth = DeprojectDepthRevProjAware( t_TextureDepth.Sample( s_TextureDepth, i.coordColor ).x );
	float f = CalcBlurWeight( depth ) * vDofParamsIntensity.y;

	f = pow(f, 0.5);
	
	const float w0 = saturate( 1 -      f  * 2 );
	const float w2 = saturate( 1 - (1 - f) * 2 );
	const float w1 = 1 - (w0 + w2);
	
	return ( w0 * orig_color + ( w1 * blur_color_low + ( w2 * blur_color_high ) ) );
}
#endif
