#include "postfx_common.fx"

#define ENABLE_CLAMP 1

#define vTexCoordTransformColor VSC_Custom_0
#define vTexCoordTransformDepth VSC_Custom_1
#define vTexelDir				PSC_Custom_0
#define vDofParamsRange			PSC_Custom_1
#define vDofParamsIntensity		PSC_Custom_2
#define vClampParams			PSC_Custom_3

SamplerState	s_TextureColor      : register( s0 );
Texture2D		t_TextureColor		: register( t0 );

SamplerState	s_TextureDepth      : register( s1 );
Texture2D		t_TextureDepth		: register( t1 );

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
	float2 coordColor  : TEXCOORD0;
	float2 coordDepth  : TEXCOORD1;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos			= i.pos;
	o.coordColor	= ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformColor );
	o.coordDepth	= ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformDepth );

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

float CalcBlurIntensity( float depth )
{
	float focusNear = vDofParamsRange.x;
	return depth > focusNear ? vDofParamsIntensity.x : 1;
}

float2 ClampCoordColor( float2 coord )
{
#if ENABLE_CLAMP
	return min( coord, vClampParams.xy );
#else
	return coord;
#endif
}

float2 ClampCoordDepth( float2 coord )
{
#if ENABLE_CLAMP
	return min( coord, vClampParams.zw );
#else
	return coord;
#endif
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	#define RANGE 4
	
	const float weights[RANGE] = 
	{
		0.95,		0.9,		0.825,		0.7
	};
	
	float4 result = t_TextureColor.Sample( s_TextureColor, ClampCoordColor( i.coordColor ) );
	float orig_depth  = DeprojectDepthRevProjAware( t_TextureDepth.Sample( s_TextureDepth, ClampCoordDepth( i.coordDepth ) ).x );

	float orig_weight = CalcBlurWeight( orig_depth ).x;

	if ( orig_weight * vDofParamsIntensity.x > 0.05f )
	{
		float weights_sum = 1;
	
		float2 offset_multiplier = orig_weight.xx * vTexelDir.xy * vDofParamsIntensity.xx;

		[unroll]
		for ( int side=-1; 		side<=1; 			side+=2 		)
		[unroll]
		for ( int side_iter=0; 	side_iter<RANGE; 	side_iter+=1 	)
		{
			int    iter = side * (1 + side_iter);
			float2 off  = iter * offset_multiplier;
		
			float4 color  = t_TextureColor.Sample( s_TextureColor, ClampCoordColor( i.coordColor + off ) );

			float  depth = DeprojectDepthRevProjAware( t_TextureDepth.Sample( s_TextureDepth, ClampCoordDepth( i.coordDepth + off ) ).x );
			float  weight = CalcBlurWeight( weights[side_iter] * depth ).x;
		
			result += weight * color;
			weights_sum += weight;
		}
		result /= weights_sum;
	}
	
    return result;
}
#endif
