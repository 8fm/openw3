#include "postfx_common.fx"

// We're using a multiplier here because on Durango temporary target is 6e4 instead of r11g11b10 (6e5 for RG and 5e5 for B), 
// in which case there is a noticeable conversion error for low values.
// For high values, there is no visible difference with the multiplier, so we should be good with this.
//
// On my test case there were some occasional differences with the value of 4, but there are no differences with value of 8.
#define DOF_COLOR_MULTIPLIER		8.0

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#if defined(PIXELSHADER)

	#define dofParamsVector		PSC_Custom_0
	#define	dofParamsVector2	PSC_Custom_1
	#define dofParamsVector3	PSC_Custom_2

	float DofAmountForProjDepthRevProjAware( int2 pixelCoord, float projDepth )
	{
		float distance = DeprojectDepthRevProjAware( projDepth );
		float amount = saturate( dofParamsVector.z * sqrt( saturate( (distance - dofParamsVector.x) * dofParamsVector.w ) ) );
		if ( IsSkyByLinearDepth( distance ) )
		{
			float3 worldVec = PositionFromDepthRevProjAware( projDepth, pixelCoord ) - cameraPosition.xyz;
			float3 v = normalize( worldVec );
			amount *= saturate( 1 - (v.z - dofParamsVector3.x) * dofParamsVector3.y );
		}

		return amount;
	} 

#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
	
#if IS_GAMEPLAY_PREPARE
	
Texture2D		t_TextureColor			: register( t0 );
Texture2D		t_TextureDepth			: register( t1 );

struct VS_INPUT
{
	float4 pos		   : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos	   		: SYS_POSITION;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   		= i.pos;
	
	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{ 
	const int2 pixelCoord = i.pos.xy;
	
	const float  depth = t_TextureDepth[ pixelCoord ].x;
	const float3 color = t_TextureColor[ pixelCoord ].xyz;	

	float  dofAmount = DofAmountForProjDepthRevProjAware( pixelCoord, depth );

	//dofAmount = dofAmount > 0 ? 1 : 0;
	dofAmount = ceil( dofAmount * 3.0) / 3.0;

	return float4( color.xyz * ((DOF_COLOR_MULTIPLIER * 3.0) * dofAmount), dofAmount );
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#if IS_GAMEPLAY_APPLY

Texture2D		t_TextureColor			: register( t0 );
Texture2D		t_TextureDepth			: register( t1 );

SamplerState	s_Linear				: register( s0 );

struct VS_INPUT
{
	float4 pos		   : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos	   		: SYS_POSITION;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   		= i.pos;
	
	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main ( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
    const int2 pixelCoord = i.pos.xy ;
    const float depth = t_TextureDepth[ pixelCoord ].x;
    const float dofAmount = DofAmountForProjDepthRevProjAware( pixelCoord , depth ) ;

	[branch]
	if ( dofAmount <= 0.01 )
	{
		return 0;
	}

// 	if ( (pixelCoord.x / 64) % 2 != (pixelCoord.y / 64) % 2 && pixelCoord.y < 360 )
// 	{
// 		return float4( 1, 1, 0, 1 );
// 	}
	
    const float2 uv = i.pos.xy * surfaceDimensions.zw ;
    const float2 uvStep = dofAmount * surfaceDimensions.zw ;
    
    float4 resultColor = 0;
    {
        resultColor += 1 * SAMPLE_LEVEL( t_TextureColor, s_Linear, uv, 0 ) ;
        resultColor += 4 * SAMPLE_LEVEL( t_TextureColor, s_Linear, uv + uvStep * float2 ( - 0.5 , - 1.5 ), 0 ) ;
        resultColor += 4 * SAMPLE_LEVEL( t_TextureColor, s_Linear, uv + uvStep * float2 ( 1.5 , - 0.5 ), 0 ) ;
        resultColor += 4 * SAMPLE_LEVEL( t_TextureColor, s_Linear, uv + uvStep * float2 ( 0.5 , 1.5 ), 0 ) ;
        resultColor += 4 * SAMPLE_LEVEL( t_TextureColor, s_Linear, uv + uvStep * float2 ( - 1.5 , 0.5 ), 0 ) ;
        resultColor += 1 * SAMPLE_LEVEL( t_TextureColor, s_Linear, uv + uvStep * float2 ( - 2 , - 1 ), 0 ) ;
        resultColor += 1 * SAMPLE_LEVEL( t_TextureColor, s_Linear, uv + uvStep * float2 ( 1 , - 2 ), 0 ) ;
        resultColor += 1 * SAMPLE_LEVEL( t_TextureColor, s_Linear, uv + uvStep * float2 ( 2 , 1 ), 0 ) ;
        resultColor += 1 * SAMPLE_LEVEL( t_TextureColor, s_Linear, uv + uvStep * float2 ( - 1 , 2 ), 0 ) ;
    }
    resultColor.xyz /= max ( 0.001, resultColor.w );
	resultColor.xyz /= (3.0 * DOF_COLOR_MULTIPLIER);
    return float4 ( resultColor.xyz , saturate ( 25 * resultColor.w ) ) ;
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
