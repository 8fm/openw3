#include "postfx_common.fx"

/*
 * Possible definitions:
 *
 *	ENABLE_SECOND_BALANCEMAP	- The environment defines a second balancemap blended with vMixParams
 *	ENABLE_BALANCEMAP_BLENDING	- The engine needs to blend between two environments with vMixParamsB.W
 *  ENABLE_SECOND_BALANCEMAPB	- The destination (second) environment for blending also defines a second balancemap
 *
 */

#define vTexCoordTransform		VSC_Custom_0
#define vTexCoordClamp 			PSC_Custom_0
#define vMixParams 				PSC_Custom_1
#define vMixParamsB				PSC_Custom_2

SamplerState	s_TextureColor  : register( s0 );
Texture2D		t_TextureColor	: register( t0 );

SamplerState	s_TextureBalanceMap    	: register( s1 );
Texture2D		t_TextureBalanceMap		: register( t1 );

#if ENABLE_SECOND_BALANCEMAP
	SamplerState	s_TextureBalanceMap2	: register( s2 );
	Texture2D		t_TextureBalanceMap2	: register( t2 );
#endif
#if ENABLE_BALANCEMAP_BLENDING
	SamplerState	s_TextureBalanceMapB	: register( s3 );
	Texture2D		t_TextureBalanceMapB	: register( t3 );
#if ENABLE_SECOND_BALANCEMAPB
	SamplerState	s_TextureBalanceMap2B	: register( s4 );
	Texture2D		t_TextureBalanceMap2B	: register( t4 );
#endif
#endif

//* 512x512
#define BalanceMapResolution     512
#define BalanceMapGridRes        8
#define BalanceMapGridRes2       64
#define BalanceMapDownscale      1
//*/

/* 64x64
#define BalanceMapResolution     64
#define BalanceMapGridRes        4
#define BalanceMapGridRes2       16
#define BalanceMapDownscale      4
//*/

float3 SampleBalanceMapWorker( Texture2D balanceMapTexture, SamplerState balanceMapSampler, float3 origColor )
{
	// not the most accurate implementation, 
	// but more than enough for work in progress version  

	origColor   = saturate( origColor );
	origColor.z = min( 0.99999, origColor.z );

	float2 coord = origColor.xy;

	{
		float eps = 1.0 / (BalanceMapResolution / BalanceMapGridRes);
		//coord.xy = clamp( (origColor.xy + 0.5 / 64) * (64.0 / 63.0), eps, 1 - eps );            
		coord.xy = (origColor.xy + 0.5 / (64.0 / BalanceMapDownscale)) * 255.0 / 256.0;
		coord.xy = clamp( coord.xy, eps, 1 - eps );      
	}

	float2 z_off = 0;
	z_off.x = origColor.z * BalanceMapGridRes2 - BalanceMapGridRes * floor( origColor.z * BalanceMapGridRes );
	z_off.y = origColor.z * BalanceMapGridRes;   
	z_off = floor( z_off ) / BalanceMapGridRes;   
	coord = coord.xy / BalanceMapGridRes;
	coord += z_off;

	/*
	coord.y += floor( origColor.z * BalanceMapGridRes ) / BalanceMapGridRes;
	//coord.x += fmod( origColor.z * 8.0, 1.0 ); // fmod's precision is not sufficient
	//coord.x += floor( (origColor.z - floor( origColor.z * 8.0 ) / 8.0) * 64.0 ) / 8.0;   
	coord.x += floor( origColor.z * BalanceMapGridRes2 - BalanceMapGridRes * floor( origColor.z * BalanceMapGridRes ) ) / BalanceMapGridRes;
	*/

	return SAMPLE_LEVEL( balanceMapTexture, balanceMapSampler, coord, 0 ).xyz;
}

float3 SampleBalanceMap( Texture2D balanceMapTexture, SamplerState balanceMapSampler, float3 origColor )
{
	float3 preOrigCol = origColor;
	float gamma = 1 / 2.2;

	origColor = pow( abs(origColor), gamma );

	float3 orig_col0 = origColor;
	float3 orig_col1 = origColor;

	float ctrl_z = orig_col0.z * 255.0 / 256.0;
	float m = ctrl_z * BalanceMapGridRes2;
	orig_col0.z = floor( m ) / BalanceMapGridRes2;
	orig_col1.z = ctrl_z + 1.0 / BalanceMapGridRes2;   
	float t = m - floor(m);

	float3 result = lerp( SampleBalanceMapWorker( balanceMapTexture, balanceMapSampler, orig_col0 ), SampleBalanceMapWorker( balanceMapTexture, balanceMapSampler, orig_col1 ), t );

	result = pow( abs(result), 1.0 / gamma );

	return result;
}


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
    float2 coord		= ApplyTexCoordClamp( i.coord, vTexCoordClamp );
    float4 origColor 	= t_TextureColor.Sample( s_TextureColor, coord );
	float4 color		= origColor;
	
	color.rgb = SampleBalanceMap( t_TextureBalanceMap, s_TextureBalanceMap, origColor.rgb ).rgb;
		
#if ENABLE_SECOND_BALANCEMAP
	{
		float lerp_factor   = vMixParams.x;
		float3 color2 =  SampleBalanceMap( t_TextureBalanceMap2, s_TextureBalanceMap2, origColor.rgb ).rgb;
		color.rgb = lerp( color.rgb, color2.rgb, lerp_factor );
	}
#endif

	float amount_factor = vMixParams.y;
	float brightness = vMixParams.z;	
	color.rgb = lerp( origColor.rgb, brightness * color.rgb, amount_factor );
	
#if ENABLE_BALANCEMAP_BLENDING
	{
		float blend_factor	= vMixParamsB.w;
		float3 colorb = SampleBalanceMap( t_TextureBalanceMapB, s_TextureBalanceMapB, origColor.rgb).rgb;
#if ENABLE_SECOND_BALANCEMAPB
		float lerp_factor = vMixParamsB.x;
		float3 color2b = SampleBalanceMap( t_TextureBalanceMap2B, s_TextureBalanceMap2B, origColor.rgb ).rgb;
		colorb = lerp( colorb, color2b, lerp_factor );
#endif

		float amount_factorb = vMixParamsB.y;
		float brightnessb = vMixParamsB.z;
		colorb = lerp( origColor.rgb, brightnessb * colorb, amount_factorb );

		color.rgb = lerp( color.rgb, colorb, blend_factor );
	}
#endif

    return color;
}
#endif
