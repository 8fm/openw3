
#define NEW_CAT

#ifdef NEW_CAT

#include "postfx_common.fx"

SamplerState	s_TextureColor      : register( s0 );
Texture2D		t_TextureColor		: register( t0 );

SamplerState	sTextureDepth		: register( s1 );
Texture2D		tTextureDepth		: register( t1 );

SamplerState	sObjectColor		: register( s2 );
Texture2D		tObjectColor		: register( t2 );

#define vTexCoordTransform VSC_Custom_0

#define EffectScale			PSC_Custom_0.x
#define DesaturationScale	PSC_Custom_0.y
#define BlurStrength		PSC_Custom_0.z
#define NearBrightening		PSC_Custom_0.w

#define viewRange			PSC_Custom_1.x
#define viewPulse			PSC_Custom_1.y
#define tintColorNear		PSC_Custom_2
#define tintColorFar		PSC_Custom_3
#define playerPosition		PSC_Custom_4
#define objectHightLightColor		PSC_Custom_5.xyz

#define vCoordMax			PSC_Custom_6.xy
#define vTexelSize			PSC_Custom_6.zw


struct VS_INPUT
{
    float4 pos      : POSITION0;
};

struct VS_OUTPUT
{
    float2 coord   : TEXCOORD0;
    float4 pos    : SYS_POSITION;
};


#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos  = i.pos;
	o.coord = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransform );
	
	return o;
}

#endif

#ifdef PIXELSHADER

#define offsetSqr 0.707

static const float2 samples[4] = { 
	float2(1.0,0.0),
	float2(offsetSqr,offsetSqr),
	float2(0.0,1.0),
	float2(-offsetSqr,offsetSqr),
};

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	// - - - - - - - - - - - -
	// Read input targets
	float2 ixy = i.pos.xy;
    const float2 coord 	 = ixy * PSC_ViewportSize.zw;
	const float2 halfCoord = ixy * vCoordMax;

	/*
	float2 fromCenter = 2.0f* i.vpos.xy/PSC_ViewportSubSize.xy - 1.0f;		
	fromCenter *= dot( fromCenter, fromCenter ) * 0.05 * EffectScale;
	fromCenter = clamp( fromCenter, float2(-0.4f,-0.4f), float2(0.4f,0.4f) );

	coord -= ( fromCenter * viewPulse ) * PSC_ViewportSubSize.zw;
	*/

	const float4 smp_color = t_TextureColor.Sample( s_TextureColor, coord );
	
	const float depth = tTextureDepth.Sample( sTextureDepth, coord ).x;	
	const float3 wPos = PositionFromDepthRevProjAware( depth, i.pos.xy );

	const bool isSky = IsSkyByProjectedDepthRevProjAware( depth );
	// - - - - - - - - - - - -
	// Near range to the player

	const float3 wRay = ( wPos - playerPosition.xyz );
	const float wRayDistance = length( wRay );
		
	const float frontWave = 1.0 - clamp( 1.5 * EffectScale/viewRange - wRayDistance , 0.0f , 1.0f );
	
	float nearCutoff = saturate( wRayDistance / 10.0 );
	nearCutoff *= nearCutoff;
	float nearScale = min( wRayDistance * viewRange , 1.0f );
	const float blurMinMaxScale = min( wRayDistance * viewRange * 0.5 , 1.0f );
	const float brightScale = lerp( NearBrightening * 0.25 , NearBrightening * ( 1.0 - nearScale ) + NearBrightening , nearCutoff );
	nearScale = 1.0 - nearScale*nearScale;
	
	// - - - - - - - - - - - -
	// Blur bullshit 
	float3 blurColorMin = smp_color.xyz;
	float3 blurColorMax = smp_color.xyz;
	const float2 blurScale = PSC_ViewportSize.zw * BlurStrength;
	/*
	[unroll]
	for( int i = 0; i < 4; ++i )
	{
		float2 __uv = samples[i] *  blurScale + coord;
		// if( IsSkyByProjectedDepthRevProjAware( tTextureDepth.Sample( sTextureDepth, __uv ).x ) == isSky )
		{
			float3 tmp = t_TextureColor.Sample( s_TextureColor, __uv ).xyz;
			blurColorMin = min( tmp , blurColorMin );
			blurColorMax = max( tmp , blurColorMax );
		}
		__uv = samples[i] *  -blurScale + coord;
		// if( IsSkyByProjectedDepthRevProjAware( tTextureDepth.Sample( sTextureDepth, __uv ).x ) == isSky )
		{
			float3 tmp = t_TextureColor.Sample( s_TextureColor, __uv ).xyz;
			blurColorMin = min( tmp , blurColorMin );
			blurColorMax = max( tmp , blurColorMax );
		}
	}
	*/
	const float3 color = lerp( lerp( blurColorMin , blurColorMax , blurMinMaxScale*blurMinMaxScale ) , smp_color.xyz , nearScale );

	// - - - - - - - - - - - -
	// Desaturation shit

	const float val = dot( color , RGB_LUMINANCE_WEIGHTS_LINEAR_Sepia );

	const float brightnessNear = brightScale;
	const float brightnessFar = brightScale * 0.2;
	
	const float3 tintFinal = lerp( tintColorFar * brightnessFar , tintColorNear * brightnessNear , nearScale );

	float3 finalColor = lerp( color , tintFinal * val , DesaturationScale ) ; 

	// - - - - - - - - - - - -
	// Adding objects hightlight
	
	float2 objBlurH = vTexelSize * float2( offsetSqr, 0.0f );
	float2 objBlurV = vTexelSize * float2( 0.0f , offsetSqr );
	
	float objectHighlight=0.2*(	tObjectColor.Sample( sObjectColor, halfCoord ).x +
								tObjectColor.Sample( sObjectColor, halfCoord + objBlurH ).x +
								tObjectColor.Sample( sObjectColor, halfCoord - objBlurH ).x +
								tObjectColor.Sample( sObjectColor, halfCoord + objBlurV ).x +
								tObjectColor.Sample( sObjectColor, halfCoord - objBlurV ).x );

	finalColor += objectHightLightColor  * ( objectHighlight * EffectScale );
	
	// - - - - - - - - - - - -

	finalColor = lerp( smp_color.xyz , finalColor , EffectScale );

	return float4( min( finalColor , 1.1f ) , 1.0f );
	// return float4( nearCutoff , 0.0 , 0.0 , 1.0 );
}
#endif


#else

#include "postfx_common.fx"

SamplerState	s_TextureColor      : register( s0 );
Texture2D		t_TextureColor		: register( t0 );

SamplerState	sTextureDepth		: register( s1 );
Texture2D		tTextureDepth		: register( t1 );

SamplerState	sObjectColor		: register( s2 );
Texture2D		tObjectColor		: register( t2 );

#define vTexCoordTransform VSC_Custom_0

#define EffectScale			PSC_Custom_0.x
#define DesaturationScale	PSC_Custom_0.y
#define BlurStrength		PSC_Custom_0.z
#define NearBrightening		PSC_Custom_0.w

#define viewRange			PSC_Custom_1.x
#define viewPulse			PSC_Custom_1.y
#define tintColorNear		PSC_Custom_2
#define tintColorFar		PSC_Custom_3
#define playerPosition		PSC_Custom_4
#define objectHightLightColor		PSC_Custom_5.xyz

#define vCoordMax			PSC_Custom_6.xy
#define vTexelSize			PSC_Custom_6.zw


struct VS_INPUT
{
    float4 pos      : POSITION0;
};

struct VS_OUTPUT
{
    float2 coord   : TEXCOORD0;
    float4 pos    : SYS_POSITION;
};


#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos  = i.pos;
	o.coord = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransform );
	
	return o;
}

#endif

#ifdef PIXELSHADER

#define offsetSqr 0.707

static const float2 samples[4] = { 
	float2(1.0,0.0),
	float2(offsetSqr,offsetSqr),
	float2(0.0,1.0),
	float2(-offsetSqr,offsetSqr),
};

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	// - - - - - - - - - - - -
	// Read input targets
	float2 ixy = i.pos.xy;
    const float2 coord 	 = ixy * PSC_ViewportSize.zw;
	const float2 halfCoord = ixy * vCoordMax;

	/*
	float2 fromCenter = 2.0f* i.vpos.xy/PSC_ViewportSubSize.xy - 1.0f;		
	fromCenter *= dot( fromCenter, fromCenter ) * 0.05 * EffectScale;
	fromCenter = clamp( fromCenter, float2(-0.4f,-0.4f), float2(0.4f,0.4f) );

	coord -= ( fromCenter * viewPulse ) * PSC_ViewportSubSize.zw;
	*/

	const float4 smp_color = t_TextureColor.Sample( s_TextureColor, coord );
	
	const float depth = tTextureDepth.Sample( sTextureDepth, coord ).x;	
	const float3 wPos = PositionFromDepthRevProjAware( depth, i.pos.xy );

	// const bool isSky = IsSkyByProjectedDepthRevProjAware( depth );
	// - - - - - - - - - - - -
	// Near range to the player

	const float3 wRay = ( wPos - playerPosition.xyz );
	const float wRayDistance = length( wRay );

	const float frontWave = 1.0 - clamp( 1.5 * EffectScale/viewRange - wRayDistance , 0.0f , 1.0f );

	float nearScale = min( wRayDistance * viewRange , 1.0f );
	const float blurMinMaxScale = min( wRayDistance * viewRange * 0.5 , 1.0f );
	const float brightScale = NearBrightening * ( 1.0 - nearScale + 8.0*frontWave ) + NearBrightening;
	nearScale = 1.0 - nearScale*nearScale;
	
	// - - - - - - - - - - - -
	// Blur bullshit 
	float3 blurColorMin = smp_color.xyz;
	float3 blurColorMax = smp_color.xyz;
	const float2 blurScale = PSC_ViewportSize.zw * BlurStrength;
	/*
	[unroll]
	for( int i = 0; i < 4; ++i )
	{
		float2 __uv = samples[i] *  blurScale + coord;
		// if( IsSkyByProjectedDepthRevProjAware( tTextureDepth.Sample( sTextureDepth, __uv ).x ) == isSky )
		{
			float3 tmp = t_TextureColor.Sample( s_TextureColor, __uv ).xyz;
			blurColorMin = min( tmp , blurColorMin );
			blurColorMax = max( tmp , blurColorMax );
		}
		__uv = samples[i] *  -blurScale + coord;
		// if( IsSkyByProjectedDepthRevProjAware( tTextureDepth.Sample( sTextureDepth, __uv ).x ) == isSky )
		{
			float3 tmp = t_TextureColor.Sample( s_TextureColor, __uv ).xyz;
			blurColorMin = min( tmp , blurColorMin );
			blurColorMax = max( tmp , blurColorMax );
		}
	}
	*/
	const float3 color = lerp( lerp( blurColorMin , blurColorMax , blurMinMaxScale*blurMinMaxScale ) , smp_color.xyz , nearScale );

	// - - - - - - - - - - - -
	// Desaturation shit

	const float val = dot( color , RGB_LUMINANCE_WEIGHTS_LINEAR_Sepia );

	const float3 tintFinal = lerp( tintColorFar , tintColorNear * brightScale , nearScale );

	float3 finalColor = lerp( color , tintFinal * val , DesaturationScale ) ; 

	// - - - - - - - - - - - -
	// Adding objects hightlight
	
	float2 objBlurH = vTexelSize * float2( offsetSqr, 0.0f );
	float2 objBlurV = vTexelSize * float2( 0.0f , offsetSqr );
	
	float objectHighlight=0.2*(	tObjectColor.Sample( sObjectColor, halfCoord ).x +
								tObjectColor.Sample( sObjectColor, halfCoord + objBlurH ).x +
								tObjectColor.Sample( sObjectColor, halfCoord - objBlurH ).x +
								tObjectColor.Sample( sObjectColor, halfCoord + objBlurV ).x +
								tObjectColor.Sample( sObjectColor, halfCoord - objBlurV ).x );

	finalColor += objectHightLightColor  * ( objectHighlight * EffectScale );
	
	// - - - - - - - - - - - -

	finalColor = lerp( smp_color.xyz , finalColor , EffectScale );

	return float4( min( finalColor , 1.1f ) , 1.0f );
}
#endif


#endif

