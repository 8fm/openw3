#include "commonPS.fx"
#include "globalConstantsVS.fx"

//#define PARALLAX

/// First stripe texture
Texture2D<float4>   tStripeDiff     : register( t0 );
SamplerState        sStripeDiff     : register( s0 );

/// First stripe normals
#ifdef HAS_FIRST_STRIPE_NORMALS
Texture2D<float4>   tStripeNorm     : register( t1 );
SamplerState        sStripeNorm     : register( s1 );
  #define HAS_STRIPE_NORMALS
#endif

/// Optional second stripe texture and normals
#ifdef IS_STRIPE_DOUBLE
Texture2D<float4>   tStripeDiff2    : register( t2 );
SamplerState        sStripeDiff2    : register( s2 );
  // Optional second normals
  #ifdef HAS_SECOND_STRIPE_NORMALS
Texture2D<float4>   tStripeNorm2    : register( t3 );
SamplerState        sStripeNorm2    : register( s3 );
    #ifndef HAS_STRIPE_NORMALS
      #define HAS_STRIPE_NORMALS
    #endif
  #endif

/// Optional (required if second texture/normal is given) blending texture
Texture2D<float>    tStripeAlpha    : register( t4 );
SamplerState        sStripeAlpha    : register( s4 );
#endif

#ifdef PARALLAX
Texture2D<float>    tStripeDepth    : register( t5 );
SamplerState        sStripeDepth    : register( s5 );
#endif

/// Depth buffer (only needed for projected stripes to find world pos)
#define SceneDepth t_PSSMP_SceneDepth

struct VS_INPUT
{
    float4  pos     : SYS_POSITION;
    float4  col     : COLOR0;
    float2  uv      : TEXCOORD0;
    float3  buvofs  : TEXCOORD1;
    float4  centwid : TEXCOORD2;
    float3  tang    : TANGENT;
};

struct VS_OUTPUT
{
    float4  pos     : SYS_POSITION;
    float4  col     : COLOR0;
    float2  uv      : TEXCOORD0;
#ifdef IS_STRIPE_DOUBLE
    float3  buvofs  : TEXCOORD1;
#endif
#ifdef IS_STRIPE_PROJECTED
    float3  wpos    : TEXCOORD2;
    float4  centwid : TEXCOORD3;
#endif
#ifdef HAS_STRIPE_NORMALS
    float4x4 wmat   : EXTRA_DATA;
    float4x4 omat   : MORE_DATA;
#endif
};

struct PS_OUTPUT
{
	float4 color : SYS_TARGET_OUTPUT0;
#ifdef HAS_STRIPE_NORMALS
	float4 normal : SYS_TARGET_OUTPUT1;
#endif
};

///////////////////////////////////////////////////////////////////////////////

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;
    
#ifndef IS_STRIPE_PROJECTED
	float3 unoffsettedWorldPos = mul( i.pos, VSC_LocalToWorld ).xyz;
    float offset = 0.02f + min( 0.05f, distance( unoffsettedWorldPos, VSC_CameraPosition.xyz )/1000.0f );
	float4 worldPos = mul( i.pos + float4( 0.0f, 0.0f, offset, 0.0f ), VSC_LocalToWorld );
#else
	float4 worldPos = mul( i.pos, VSC_LocalToWorld );
#endif
	
	o.pos = mul( worldPos, VSC_WorldToScreen );
	o.col = i.col;
	o.uv = i.uv;
#ifdef IS_STRIPE_DOUBLE
    o.buvofs = i.buvofs;
#endif
#ifdef IS_STRIPE_PROJECTED
    o.wpos = worldPos.xyz;
    o.centwid = float4( mul( float4( i.centwid.xyz, 1.0f ), VSC_LocalToWorld ).xyz, i.centwid.w );
#endif

#ifdef HAS_STRIPE_NORMALS
    o.wmat[0] = normalize( float4( i.tang, 0.0f ) );
    o.wmat[2] = float4( 0.0f, 0.0f, 1.0f, 0.0f );
    o.wmat[1] = normalize( float4( cross( o.wmat[0].xyz, o.wmat[2].xyz ), 0.0f ) );
    o.wmat = mul( o.wmat, VSC_LocalToWorld );
	o.wmat[3] = float4( 0.0f, 0.0f, 0.0f, 1.0f );
#endif

    return o;
}
#endif // VERTEXSHADER

///////////////////////////////////////////////////////////////////////////////

#ifdef PIXELSHADER
 
void ps_main( in VS_OUTPUT i, out PS_OUTPUT o )
{
	const uint2 pixelCoord = (uint2)i.pos.xy;

#ifdef IS_STRIPE_PROJECTED
	// calculate world position of the pixel we're going to modify
	float depth = SceneDepth[ i.pos.xy ].x;
	float2 screenSpaceUV = i.pos.xy / PSC_ViewportSubSize.xy;
	float3 wpos = PositionFromDepthRevProjAware( depth, pixelCoord );

    // Discard pixel if we're away from the stripe
	if ( distance( wpos.xy, i.centwid.xy ) > i.centwid.w ) discard;
#endif

#ifdef IS_STRIPE_DOUBLE
  #ifdef PARALLAX
    float3 eyeDir = normalize( wpos - PSC_CameraPosition.xyz );//PSC_CameraPosition.xyz - wpos );
    //float3 eyeDir = normalize( PSC_CameraVectorForward.xyz );
    //float4 enormal = tStripeNorm.Sample( sStripeNorm, i.uv, int2( 0, 0 ) );
    //float4 ewnormal = mul( float4( enormal.xyz * 2.0f - float3( 1.0f, 1.0f, 1.0f ), 1.0f ), i.wmat );
    eyeDir = normalize( mul( float4( eyeDir, 0.0f ), transpose( i.wmat ) ) );

    float sf = 0.12f;
    float bias = sf*-0.5f;
    //float height = bias + sf*tStripeDiff.Sample( sStripeDiff, i.uv, int2( 0, 0 ) ).a;
    //float height = bias + sf*tStripeAlpha.Sample( sStripeAlpha, i.uv, int2( 0, 0 ) );
    float height = bias + sf*tStripeDepth.Sample( sStripeDepth, i.uv, int2( 0, 0 ) );
    float2 nuv = frac(i.uv) + clamp(float2(height, height)*( eyeDir.xy ), float2(-1.0f, -1.0f), float2(1.0f, 1.0f));
    float4 col = tStripeDiff.Sample( sStripeDiff, nuv, int2( 0, 0 ) );
    height *= col.a;

#if 0
    int steps = 8;
    float initialheight = height;
    for ( int scan=0; scan < steps; ++scan )
    {
        float stept = scan*(1.0f/steps);
        float2 stepuv = frac(i.uv) + height*eyeDir.xy*stept;
        height = bias + sf*tStripeDepth.Sample( sStripeDepth, i.uv, int2( 0, 0 ) );
        if ( height > initialheight )
        {
            initialheight = height;
            nuv = stepuv;
        }
    }
#endif

#if 1
    for ( int correction=0; correction < 8; ++correction )
    {
        height = bias + sf*tStripeDepth.Sample( sStripeDepth, nuv, int2( 0, 0 ) );
        nuv = frac(i.uv) + clamp(float2(height, height)*( eyeDir.xy ), float2(-1.0f, -1.0f), float2(1.0f, 1.0f));
        //nuv = frac(i.uv) + float2(height, height)*( eyeDir.xy );
    }
#endif

#if 0
    //float3 xxx = float3( ( frac( eyeDir.xy ) + 1.0f ) * 0.5f, 0.0f );
    //float3 xxx = ( eyeDir.xxx + 1 )*0.5;
    //float3 xxx = float3( frac( nuv ), 0.0f );
    //float3 xxx = tStripeAlpha.Sample( sStripeAlpha, nuv, int2( 0, 0 ) ).xxx;
    //float3 xxx = tStripeDiff.Sample( sStripeDiff, nuv, int2( 0, 0 ) ).aaa;
    //float3 xxx = tStripeDepth.Sample( sStripeDiff, nuv, int2( 0, 0 ) ).xxx;

    float3 xxx = tStripeDiff.Sample( sStripeDiff, nuv, int2( 0, 0 ) ).rgb;
    //float3 yyy = tStripeNorm.Sample( sStripeNorm, nuv, int2( 0, 0 ) ).rgb;

    //float3 xxx = ( i.wmat[0].xyz + 1.0f ) * 0.5f;
    //float xxx = frac( wpos );

    o.color = float4( xxx, 1.0f );
    //o.normal = float4( yyy, 1.0f );

    return;
#endif
  
  #else // PARALLAX
    const float2 nuv = i.uv;
  #endif

#else // DOUBLE STRIPE
    const float2 nuv = i.uv;
#endif

    // First stripe textures
    float4 color = tStripeDiff.Sample( sStripeDiff, nuv, int2( 0, 0 ) );
    float alpha = color.a * i.col.a;

#ifdef HAS_FIRST_STRIPE_NORMALS
    float4 normal = tStripeNorm.Sample( sStripeNorm, nuv, int2( 0, 0 ) );
    float4 wnormal = mul( float4( normal.xyz * 2.0f - float3( 1.0f, 1.0f, 1.0f ), 1.0f ), i.wmat );
#endif

    // Second stripe textures
#ifdef IS_STRIPE_DOUBLE
    float4 color2 = tStripeDiff2.Sample( sStripeDiff2, nuv, int2( 0, 0 ) );
    float alpha2 = color2.a * i.col.a;
  #ifdef HAS_SECOND_STRIPE_NORMALS
    float4 normal2 = tStripeNorm2.Sample( sStripeNorm2, nuv, int2( 0, 0 ) );
    float4 wnormal2 = mul( float4( normal2.xyz * 2.0f - float3( 1.0f, 1.0f, 1.0f ), 1.0f ), i.wmat );
  #endif
    float blend = clamp( tStripeAlpha.Sample( sStripeAlpha, i.buvofs.xy, int2( 0, 0 ) ) + i.buvofs.z, 0.0f, 1.0f );
    float falpha = lerp( alpha, alpha2, blend );

  #ifdef VIS_STRIPE_BLEND
    o.color = float4( float3( blend, 1.0f-blend, 0.0f ), 1.0f );
  #else
    o.color = float4( lerp( color, color2, blend ).xyz*i.col.rgb, falpha );
  #endif
  #ifdef HAS_SECOND_STRIPE_NORMALS
    #ifdef HAS_FIRST_STRIPE_NORMALS
    o.normal = float4( ( ( lerp( wnormal.xyz, wnormal2.xyz, blend ) ) + float3( 1.0f, 1.0f, 1.0f ) )*0.5f, falpha );
    #else
    o.normal = float4( ( wnormal2.xyz + float3( 1.0f, 1.0f, 1.0f ) )*0.5f, falpha * blend );
    #endif
  #else
    #ifdef HAS_FIRST_STRIPE_NORMALS
    o.normal = float4( ( wnormal.xyz + float3( 1.0f, 1.0f, 1.0f ) )*0.5f, falpha * ( 1.0f - blend ) );
    #endif
  #endif
#else // IS_STRIPE_DOUBLE
    o.color = float4( color.rgb*i.col.rgb, alpha );
  #ifdef HAS_FIRST_STRIPE_NORMALS
    o.normal = float4( ( wnormal.xyz + float3( 1.0f, 1.0f, 1.0f ) )*0.5f, alpha );
  #endif
#endif // IS_STRIPE_DOUBLE
}

#endif // PIXELSHADER

