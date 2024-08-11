
#include "postfx_common.fx"


// Parameters
#define value4RT0 PSC_Custom_0
#define value4RT1 PSC_Custom_1
#define value4RT2 PSC_Custom_2
#define value4RT3 PSC_Custom_3

#if FOCUS_MODE_CLEAR
#define PlayerPosition		PSC_Custom_7.xyz
#define EffectDistanceShift	PSC_Custom_6.y
#define EffectStrength		PSC_Custom_6.x
#endif

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

struct PS_OUTPUT
{
  	float4 outCOLOR0 : SYS_TARGET_OUTPUT0;
  	float4 outCOLOR1 : SYS_TARGET_OUTPUT1;
  	float4 outCOLOR2 : SYS_TARGET_OUTPUT2;
  	float4 outCOLOR3 : SYS_TARGET_OUTPUT3;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o._pos  = i.pos;

	return o;
}

#endif

#ifdef PIXELSHADER

#if FOCUS_MODE_CLEAR
// Fade out by distance
float DistanceFade( float3 wPos )
{	
	float distance = length( wPos - PlayerPosition );
	// extending view the distance over the time	
	
	float distanceMax = 0.0f; 
	
	if( EffectDistanceShift > 0.03f )
	{
		distance *= 0.12f;
		distanceMax = 1.0f + pow(abs(EffectDistanceShift), 2.8f) * 120.0f;
	}
	else
	{
		distanceMax = 1.0f + pow(abs(EffectDistanceShift), 0.8f) * 120.0f;		
	}	

	distance = 1.0f - pow( distance/distanceMax, 1.6f );
	
	return saturate( distance );
}
#endif

PS_OUTPUT ps_main( PS_INPUT i )
{
	PS_OUTPUT result;
	result.outCOLOR0 = value4RT0;
	result.outCOLOR1 = value4RT1;
	result.outCOLOR2 = value4RT2;
	result.outCOLOR3 = value4RT3;

#if FOCUS_MODE_CLEAR
	// calculate depth			
	float2 coord 	 = (i.vpos.xy + HALF_PIXEL_OFFSET) * PSC_ViewportSize.zw;
	float depth = SYS_SAMPLE( PSSMP_SceneDepth, coord );
	float3 wPos = PositionFromDepthRevProjAware( depth, i.vpos.xy );
	float distanceFade = DistanceFade( wPos );	
	
	result.outCOLOR0.xyz *= distanceFade;	
#endif

    return result;
}

#endif
