#include "postfx_common.fx"

Texture2D		t_TextureColor	: register( t0 );
Texture2D		t_TextureDepth	: register( t1 );
SamplerState	s_Linear		: register( s0 );

#define f2AreaSize				(PSC_Custom_0.xy)
#define f2TextureSize			(PSC_Custom_0.zw)
#define vSharpenParams			PSC_Custom_1
#define f2LumFilterScaleBias	(PSC_Custom_2.xy)

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
	// xxxxxxxxxxxxxxx remove fake sharpen from forward passes if this will get accepted

	const int2  pixelCoord = (int2)i.pos.xy;
	const float zw = t_TextureDepth[pixelCoord];
	const float distFactor = saturate( DeprojectDepthRevProjAware( zw ) * vSharpenParams.z + vSharpenParams.w );
	const float distStrength = (1 + lerp( vSharpenParams.x, vSharpenParams.y, distFactor )) * (IsSkyByProjectedDepthRevProjAware( zw ) ? 0 : 1); //< disable on the sky because it highlights potential banding	
	const float2 coord = (pixelCoord + 0.5) / f2TextureSize;		
	const float3 curColor = SAMPLE_LEVEL( t_TextureColor, s_Linear, coord, 0 ).xyz;

	float3 result = curColor;
	[branch]
	if ( distStrength > 0 )
	{
		const float2 pixelStep = 1.0 / f2TextureSize;
		const float2 halfPixelStep = 0.5 / f2TextureSize;

		float3 refColor = 0;
		{
			refColor += SAMPLE_LEVEL( t_TextureColor, s_Linear, coord + float2( -halfPixelStep.x, -halfPixelStep.y ), 0 ).xyz;
			refColor += SAMPLE_LEVEL( t_TextureColor, s_Linear, coord + float2( -halfPixelStep.x,  halfPixelStep.y ), 0 ).xyz;
			refColor += SAMPLE_LEVEL( t_TextureColor, s_Linear, coord + float2(  halfPixelStep.x, -halfPixelStep.y ), 0 ).xyz;
			refColor += SAMPLE_LEVEL( t_TextureColor, s_Linear, coord + float2(  halfPixelStep.x,  halfPixelStep.y ), 0 ).xyz;
			refColor *= 0.25;
		}
	
		float sharpenAmount = 1;
		{
			float3 _d = abs( curColor - refColor );
			float lumDiff = max( _d.x, max( _d.y, _d.z ) );
			sharpenAmount = saturate( lumDiff * f2LumFilterScaleBias.x + f2LumFilterScaleBias.y );
		}

		//return sharpenAmount;

		const float finalStrength = lerp( 1, distStrength, sharpenAmount );

		const float lumCurr = dot( RGB_LUMINANCE_WEIGHTS_LINEAR, curColor );
		const float lumRef  = dot( RGB_LUMINANCE_WEIGHTS_LINEAR, refColor );
		result = curColor / max(0.0001, lumCurr) * max( 0, lerp( lumRef, lumCurr, finalStrength ) );
	}

	return float4 ( result, 1 );
}
#endif
