#include "postfx_common.fx"

// Samplers

Texture2D		t_TextureColor		: register( t0 );
SamplerState	s_Point				: register( s0 );
SamplerState	s_Linear			: register( s1 );


#define vTexCoordTransformColorDepth  VSC_Custom_0

#ifdef ENABLE_VIGNETTE
#define vTexCoordTransformVignette	VSC_Custom_2
#define vVignetteWeights		  	(PSC_Custom_6.xyz)
#define fVignetteOpacity			(PSC_Custom_6.w)
#define vVignetteColor		  		PSC_Custom_7

#if ENABLE_VIGNETTE == 1
SamplerState	s_TextureVignette   : register( s2 );
Texture2D		t_TextureVignette	: register( t2 );
#endif

#endif
	
#define vGammaExponent 				PSC_Custom_8
#define vColorTint					PSC_Custom_9
#define vStandardTransform			PSC_Custom_15

#if ENABLE_DOUBLE_BALANCE && !ENABLE_BALANCE
# error wtf
#endif

#if ENABLE_BALANCE
#define vBalanceRanges				PSC_Custom_10
#define vBalanceLow					PSC_Custom_11
#define vBalanceMid					PSC_Custom_12
#define vBalanceHigh				PSC_Custom_13
#define vLevels						PSC_Custom_14
#endif

#if ENABLE_ABERRATION
#define vAberrationParams			PSC_Custom_16
#define vAberrationParams2			PSC_Custom_17
#endif
	
struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   		: SYS_POSITION;
	float2 coordColorDepth	: TEXCOORD0;
	
#ifdef ENABLE_VIGNETTE
	float2 coordVignette : TEXCOORD2;
#endif	
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos          		= i.pos;
	o.coordColorDepth   = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformColorDepth );

#ifdef ENABLE_VIGNETTE
	o.coordVignette= ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformVignette );
#endif	

	return o;
}

#endif

#ifdef PIXELSHADER

// http://chilliant.blogspot.com/2010/11/rgbhsv-in-hlsl.html
float3 Hue(float H)
{
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);
    return saturate(float3(R,G,B));
}

// http://chilliant.blogspot.com/2010/11/rgbhsv-in-hlsl.html
float3 HSVtoRGB(in float3 HSV)
{
	HSV.x -= floor( HSV.x );
    return ((Hue(HSV.x) - 1) * HSV.y + 1) * HSV.z;
}

// http://chilliant.blogspot.com/2010/11/rgbhsv-in-hlsl.html
float3 RGBtoHSV(in float3 RGB)
{
    float3 HSV = 0;
    HSV.z = max(RGB.r, max(RGB.g, RGB.b));
    float M = min(RGB.r, min(RGB.g, RGB.b));
    float C = HSV.z - M;
    if (C != 0)
    {
        HSV.y = C / HSV.z;
        float3 Delta = (HSV.z - RGB) / C;
        Delta.rgb -= Delta.brg;
        Delta.rg += float2(2,4);
        if (RGB.r >= HSV.z)
            HSV.x = Delta.b;
        else if (RGB.g >= HSV.z)
            HSV.x = Delta.r;
        else
            HSV.x = Delta.g;
        HSV.x = frac(HSV.x / 6);
    }
    return HSV;
}

#if ENABLE_BALANCE

void ApplyHueMod( inout float3 hsvValue, inout float hueBrightnessValue, float4 hueShift, float hueBrightness )
{
	float v = hsvValue.x + hueShift.x;
	float amount = saturate( abs(v > 0.5 ? v - 1 : v) * hueShift.y + hueShift.z );
	hsvValue.x += amount * hueShift.w;
	hueBrightnessValue *= lerp( 1, hueBrightness.x, amount );
}

// When enabled, gives more plausible blending (you don't see the hue shifting as much). 
// The cost is 5 instructions.
#define USE_COLORBASED_BLEND 1

float3 ApplyColorParametricBalance( uint2 pixelCoord, float3 col )
{
	// levels
	{
		col.xyz = pow( max( 0, col.xyz * vLevels.x + vLevels.y ), vLevels.z );
	}

	// multiplication layer + saturation
	{
		const float lum_ranges = dot( RGB_LUMINANCE_WEIGHTS_GAMMA, col );
		const float tone_weight_low  = saturate( (lum_ranges - vBalanceRanges.x) * vBalanceRanges.y );
		const float tone_weight_high = saturate( (lum_ranges - vBalanceRanges.z) * vBalanceRanges.w );

		col = saturate( col );
		col = pow( col, 2.2 );	// ace_optimize: remove this (levels could be done in gamme space, so we can get rid of two pow(xyz)'s)
		
		const float lum = dot( RGB_LUMINANCE_WEIGHTS_LINEAR, col );
		
		float4 balanceParams = vBalanceMid;
		balanceParams = lerp( balanceParams, vBalanceLow,  tone_weight_low );
		balanceParams = lerp( balanceParams, vBalanceHigh, tone_weight_high );
		col = (lum + (col - lum) * balanceParams.w) * balanceParams.xyz;		
		col = saturate( col );
		col = pow( col, 1.0/2.2 );
	}

	return col;
}
#endif

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	float4 orig_color = 1;
	{
	#if ENABLE_ABERRATION
		{
			const float2 uv			= i.pos.xy * vAberrationParams2.zw;
			const float2 uvFullStep	= vAberrationParams2.zw;
			const float2 uvCenter	= vAberrationParams2.xy;

			const float fAberrationAmount			= vAberrationParams.x;
			const float fAberrationCenterStart		= vAberrationParams.y;
			const float fAberrationCenterInvRange	= vAberrationParams.z;

			float2 uvDir = (uv - uvCenter) / uvCenter;
			float len = length( uvDir );
			float t = saturate( (len - fAberrationCenterStart) * fAberrationCenterInvRange );

			orig_color.xyz = SAMPLE_LEVEL( t_TextureColor, s_Linear, uv, 0 ).xyz;

			[branch]
			if ( t > 0 )
			{
				t *= t; //< introduced blur looks more linear this way

				float targetLen = fAberrationAmount * t;
				uvDir *= targetLen / max( 0.0001, len );

				float2 uvStep = uvDir * uvFullStep;

				orig_color.x = SAMPLE_LEVEL( t_TextureColor, s_Linear, uv - 2 * uvStep, 0 ).x;
				orig_color.y = SAMPLE_LEVEL( t_TextureColor, s_Linear, uv - 1 * uvStep, 0 ).y;
				//orig_color.z = SAMPLE_LEVEL( t_TextureColor, s_Linear, uv - 0 * uvStep, 0 ).z;
			}			
		}
	#else
		{
			orig_color.xyz = SAMPLE_LEVEL( t_TextureColor, s_Point, i.coordColorDepth, 0 ).xyz;
		}
	#endif
	}
	
	float3 color_linear = orig_color.xyz;
	float3 color = pow( abs(color_linear), vGammaExponent.x );

#if ENABLE_BALANCE
	color = ApplyColorParametricBalance( i.pos.xy, color );
#endif

	color *= vColorTint.xyz;	
	
#ifdef ENABLE_VIGNETTE
	// yep - in gamma space (dark vignette color gets preserved better visualwise)
	
#if ENABLE_VIGNETTE == 1

	float vignette_factor = t_TextureVignette.Sample( s_TextureVignette, i.coordVignette ).x;

#elif ENABLE_VIGNETTE == 2

	// Carefully hand crafted values to match vignette texture
	float vignette_factor = saturate( ( length( i.coordVignette - float2(0.5,0.5) ) * 2.0 - 0.55 ) / 0.82 );
	const float v_f2 = vignette_factor*vignette_factor;
	const float v_f3 = v_f2*vignette_factor;
	const float v_f4 = v_f2*v_f2;
	vignette_factor = min( dot( float4(-0.1,-0.105,1.12,0.09) , float4(v_f4,v_f3,v_f2,vignette_factor) ) , 0.94 );

#endif
	
	vignette_factor *= fVignetteOpacity * saturate( 1.0f - dot( pow(color, 2.2), vVignetteWeights.xyz ) );
	color = lerp( color, vVignetteColor.xyz, saturate(vignette_factor) );

#endif

	color = saturate( lerp( vStandardTransform.x, vStandardTransform.y, color ) );

	return float4 ( color, 1.0f );
}

#endif
