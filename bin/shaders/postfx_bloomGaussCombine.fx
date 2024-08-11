#include "postfx_common.fx"

#if ENABLE_COMBINE_DIRT_OFF || ENABLE_COMBINE_DIRT_ON
Texture2D		combine_t_TextureColor				: register( t0 );
SamplerState	combine_s_TextureColor				: register( s0 );
	#if ENABLE_COMBINE_DIRT_ON
	Texture2D		combine_t_TextureDirt				: register( t1 );
	SamplerState	combine_s_TextureDirt				: register( s1 );
	#endif
#define			combine_vTextureFullRes				PSC_Custom_0.xy
#define			combine_vTextureAreaRes				PSC_Custom_1.xy
#define			combine_vTargetFullRes				PSC_Custom_2.xy
#define			combine_vTargetAreaRes				PSC_Custom_3.xy
#define			combine_iSourceOffsetXY				((int2)PSC_Custom_4.xy)
#define			combine_iTargetOffsetXY				((int2)PSC_Custom_4.zw)
#define			combine_vClampRange					PSC_Custom_5
#define			combine_cBloomColor					PSC_Custom_6.xyz
#define			combine_iLevelIndex					((int)PSC_Custom_6.w)
	#if ENABLE_COMBINE_DIRT_ON
	#define			combine_cDirtColor				PSC_Custom_7.xyz
	#define			combine_f2DirtWrap				PSC_Custom_8.xy
	#endif
#endif

#if ENABLE_GAUSS
Texture2D		gauss_t_TextureColor		: register( t1 );
#define			gauss_iSourceOffsetXY		((int2)PSC_Custom_10.xy)
#define			gauss_iTargetOffsetXY		((int2)PSC_Custom_10.zw)
#define			gauss_vBlurDir				PSC_Custom_11.xy
#define			gauss_vClampRange			PSC_Custom_12
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
	float4 result = 0;

#if ENABLE_COMBINE_DIRT_OFF || ENABLE_COMBINE_DIRT_ON
	{
		const int2 pixelCoord = (int2)i.pos.xy - combine_iTargetOffsetXY;

		float2 coord = (((pixelCoord + 0.5) / combine_vTargetAreaRes * combine_vTextureAreaRes) + combine_iSourceOffsetXY) / combine_vTextureFullRes;
		float2 clampMin = (combine_vClampRange.xy + 0.5) / combine_vTextureFullRes;
		float2 clampMax = (combine_vClampRange.zw + 0.5) / combine_vTextureFullRes;
		
		float3 col = SAMPLE_LEVEL( combine_t_TextureColor, combine_s_TextureColor, clamp( coord, clampMin, clampMax ), 0 ).xyz;

		float3 scale = 0;
		scale += combine_cBloomColor;
		#if ENABLE_COMBINE_DIRT_ON
		{
			float2 dirtUV = i.pos.xy * combine_f2DirtWrap;
			scale += combine_cDirtColor * SAMPLE( combine_t_TextureDirt, combine_s_TextureDirt, dirtUV ).xyz;
		}
		#endif
		result.xyz += scale * col;
	}
#endif

#if ENABLE_GAUSS
	{
		const int2 pixelCoord = (int2)i.pos.xy - gauss_iTargetOffsetXY;
		const int2 clampMin = (int2)gauss_vClampRange.xy;
		const int2 clampMax = (int2)gauss_vClampRange.zw;

		const int r = 4;
		//float weights[r + 1] = { 1 };
		//float weights[r + 1] = { 0.383103, 0.241843, 0.060626, 0.00598, };
		float weights[r + 1] = { 0.20236	,0.179044	,0.124009	,0.067234	,0.028532 };
		//float weights[r + 1] = { 0.382925	,0.24173	,0.060598	,0.005977	,0.000229	,0.000003 };

	#ifdef __PSSL__
		// This is a stupid hack for bloom blinking on the border of the screen on PS4.
		// Not sure why it happens ATM (looks like it's not any broken clamping), but the funny thing is that you can't capture the blinking
		// by the razor gpu... unless you'll do replay connect on any capture - then you'll the the blinking (animated blinking!)
		// on the preview ps4 screen.
		// Spooky shit.
		[loop]		
	#else
		[unroll]
	#endif
		for ( int i=-r; i<=r; ++i )
		{
			//float weight = 1.0 / (2.0 * r + 1.0);
			float weight = weights[abs(i)];
			int2 currCoord = pixelCoord + i * (int2)gauss_vBlurDir + gauss_iSourceOffsetXY;
			currCoord = clamp( currCoord, clampMin, clampMax );
			result.xyz += weight * gauss_t_TextureColor[currCoord].xyz;
		}
	}
#endif

	return result;
}
#endif
