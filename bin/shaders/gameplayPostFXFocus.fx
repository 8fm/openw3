#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"
#include "include_constants.fx"

SamplerState	sTextureColor		: register( s0 );
Texture2D		tTextureColor		: register( t0 );

SamplerState	sTextureDepth		: register( s1 );
Texture2D		tTextureDepth		: register( t1 );

SamplerState	sTextureDynamic		: register( s2 );
Texture2D		tTextureDynamic		: register( t2 );

Texture2D		tTextureMask		: register( t3 );

#define EffectStrength		PSC_Custom_6.x
#define EffectDistanceShift	PSC_Custom_6.y
#define HighlightBoost		PSC_Custom_6.z
#define DimmingFactor		PSC_Custom_6.w

#define EffectZoom			PSC_Custom_1.x
#define FadeDistanceRange	PSC_Custom_1.y
#define CenteringExponents	PSC_Custom_1.zw

#define CenteringFadeStart	PSC_Custom_2.xy
#define CenteringFadeRange	PSC_Custom_2.zw

struct VS_INPUT
{
    float4 pos      : POSITION0;
};

struct VS_OUTPUT
{    
    float4 _pos    : SYS_POSITION;
};

struct PS_INPUT
{
 	float4 vpos    : SYS_POSITION;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o = (VS_OUTPUT)0;

	o._pos  = i.pos;
	
	return o;
}

#endif

#ifdef PIXELSHADER

// Highlight only objects that are in camera center
float CenterFade( float2 screenpos )
{
	float2 fromCenter = abs( ( screenpos / PSC_ViewportSubSize.xy ) * 2.0f - 1.0f );

	// Adjust to account for aspect ratio. Basically we assume height is static, and widescreen adds to the left/right edges.
	fromCenter.x *= PSC_ViewportSubSize.x / PSC_ViewportSubSize.y;

	//fromCenter = saturate( ( fromCenter - CenteringFadeStart ) / CenteringFadeRange );	
	fromCenter = saturate( ( fromCenter - CenteringFadeStart ) / 1.8f );

	return 1.0f - saturate( length( pow( fromCenter, 2.5f ) ) );
	//return 1.0f - saturate( length( pow( fromCenter, CenteringExponents ) ) );
}

float4 ps_main( PS_INPUT i ) : SYS_TARGET_OUTPUT0
{
	float2 coord 	 = (i.vpos.xy + HALF_PIXEL_OFFSET) * PSC_ViewportSize.zw;		
	float cFade = CenterFade( i.vpos.xy );

	const float focusGlobalIntensity = saturate( EffectStrength );
			
	float highlightBlur = 0.03f;
			
	// add simple blur	
	float radAngle = 0.0f;	
	float3 blurred_color = float3(0.0f, 0.0f, 0.0f);	 	

	float dampRef = 0.03f;
	float screenX = i.vpos.x/PSC_ViewportSubSize.x;
	float screenY = i.vpos.y/PSC_ViewportSubSize.y;
	
	float dampBlur = saturate( 20.0f*( 
	saturate( dampRef - screenX )
	+ saturate( dampRef - screenY )
	+ saturate( screenX - (1.0f - dampRef) )
	+ saturate( screenY - (1.0f - dampRef) )
	) );
	    		
	float2 fromCenter = 2.0f* i.vpos.xy/PSC_ViewportSubSize.xy - 1.0f;		
	fromCenter = fromCenter*dot( fromCenter, fromCenter );
	fromCenter *= 0.1f*focusGlobalIntensity;	
	fromCenter = EffectZoom * clamp( fromCenter, float2(-0.4f,-0.4f), float2(0.4f,0.4f) );
	
	coord = coord - fromCenter*PSC_ViewportSubSize.zw;

	float4 smp_color = tTextureColor.Sample( sTextureColor, coord );
		
	float scale = 0.5f;
	float d1 = tTextureDynamic.Sample( sTextureDynamic, (coord*scale) ).x/8.0f;
	float d2 = tTextureDynamic.Sample( sTextureDynamic, (coord*scale)+float2(0.5f,0.0f) ).x/8.0f;
	//float3 d3 = tTextureDynamic.Sample( sTextureDynamic, (coord*skala)+float2(0.0f,0.5f) ).xyz;
	//float3 d4 = tTextureDynamic.Sample( sTextureDynamic, (coord*skala)+float2(0.5f,0.5f) ).xyz;
	
	//return 1.0f*float4(d1+d2,0,0,1);
	
	for(int _i=0; _i<8; _i++)
	{	
		radAngle = 3.1415f*45.0f*_i/180.0f;	
		float2 offset = float2(0,0);
		offset.x = cos( radAngle - PSC_TimeVector.x*0.1f );
		offset.y = sin( radAngle - PSC_TimeVector.x*0.1f );
		offset *= highlightBlur*(1.0f - dampBlur);
		
		d1 += tTextureDynamic.Sample( sTextureDynamic, ((coord + offset/8.0f)*scale) ).x/8.0f;
		d2 += tTextureDynamic.Sample( sTextureDynamic, ((coord + offset/8.0f)*scale)+float2(0.5f,0.0f) ).x/8.0f;

		offset *= fromCenter;				
		blurred_color.xyz += tTextureColor.Sample( sTextureColor, coord + offset ).xyz/8.0f;		
	}		
	
	//const float diffiusionSpeed = 1.0f;
	float2 diffusion = float2(d1.x,d2.x);	
			
	float4 maskValues = tTextureMask.Sample( sTextureColor, coord );	
	diffusion -= float2(0.8f, 0.75f)*maskValues.xy;
	diffusion = saturate(diffusion);

	blurred_color.xyz = lerp( dot( blurred_color.xyz, 0.3f ).xxx, blurred_color.xyz, cFade ) * 0.6f;	// Darkening whole screen and desaturating borders

	float4 result = float4(0.0f, 0.0f, 0.0f, 1.0f);
	result.xyz = lerp( smp_color.xyz, blurred_color.xyz, focusGlobalIntensity );
		
	// TODO: expose this
	// custom0
	float3 focusModeTint = 1.2f * PSC_Custom_4.xyz*diffusion.y;
	
	// interactive
	focusModeTint.xyz += PSC_Custom_5.xyz*diffusion.x;
		
	result.xyz = lerp( result.xyz, saturate(focusModeTint.xyz), saturate( dot(focusModeTint.xyz,1.0f) ) );	
		
	return result;
}
#endif
