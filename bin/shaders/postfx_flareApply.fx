#include "postfx_common.fx"

#define vTexCoordTransformColor  VSC_Custom_0

SamplerState	s_TextureColor      : register( s0 );
Texture2D		t_TextureColor		: register( t0 );

#define vFlareBloomColor		PSC_Custom_2

#if IS_SCREEN
	SamplerState	s_TextureTarget     : register( s1 );
	Texture2D		t_TextureTarget		: register( t1 );
	#define vTexCoordTransformTarget	VSC_Custom_1
	#define fBlendThresholdAmount		PSC_Custom_3.x
#endif


struct VS_INPUT
{
	float4 pos		   : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos	   	: SYS_POSITION;
	float2 coordColor  	: TEXCOORD0;

#if IS_SCREEN	
	float2 coordTarget  	: TEXCOORD1;
#endif

	float2 normalized	: TEXCOORD2;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   			= i.pos;
	o.normalized		= i.pos.xy * float2( 0.5f, -0.5f ) + float2( 0.5f, 0.5f );
	o.coordColor 		= ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformColor );

#if IS_SCREEN	
	o.coordTarget 		= ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformTarget );
#endif

	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0 
{
	float4 LightShaftColor = t_TextureColor.Sample( s_TextureColor, i.coordColor );
	
#if IS_SCREEN		
	float3 SceneColor = t_TextureTarget.Sample( s_TextureTarget, i.coordTarget ).xyz;
#endif

	float BloomScreenBlendThreshold = vFlareBloomColor.w;	
	
#if IS_SCREEN
	float Luminance = dot(SceneColor, RGB_LUMINANCE_WEIGHTS_LINEAR_FlareApply);

	// Use an exponential function that converges on 0 slowly
	// This minimizes the halo created by the screen blend when the source image is a bright gradient
	float BloomScreenBlendFactor = lerp( 1, saturate(BloomScreenBlendThreshold * exp2(-3 * Luminance)), fBlendThresholdAmount );

#else
	float BloomScreenBlendFactor = saturate(BloomScreenBlendThreshold);
#endif
	
	// Use a screen blend to apply bloom to scene color, darken scene color by the occlusion factor
	float3 finalColor = saturate(float3(LightShaftColor.rgb * vFlareBloomColor.rgb * BloomScreenBlendFactor));

#if IS_SCREEN
	return float4( 1.0f - ((1.0f - SceneColor.xyz) ) *(1.0f - finalColor.xyz),1);	
#else
	return float4( finalColor.xyz, 1.0f );
#endif
}
#endif
