#include "postfx_common.fx"

TextureCube				t_Texture				: register( t0 );
SamplerState			s_Texture				: register( s0 );
#define					iTargetFullWidth		((int)PSC_Custom_0.x)
#define					iTargetFullHeight		((int)PSC_Custom_0.y)
#define					iTargetSphereRes		((int)PSC_Custom_0.z)
#define					iMarginRes				((int)PSC_Custom_0.w)
#define					iTargetSphereOffsetXY	((int2)PSC_Custom_1.xy)
#define					iExtentRes				((int)PSC_Custom_1.z)

#if IS_INTERIOR_FALLBACK
#define					cInteriorFallbackColorLow		(PSC_Custom_2.xyz)
#define					cInteriorFallbackColorMiddle	(PSC_Custom_3.xyz)
#define					cInteriorFallbackColorHigh		(PSC_Custom_4.xyz)
#define					cInteriorFallbackParams			(PSC_Custom_5.xyzw)
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
	const int2 pixelCoord = (int2)i.pos.xy - iTargetSphereOffsetXY;
	
	float2 uv = 0;
	int sphereFaceIndex = 0;
	if ( pixelCoord.x >= iTargetSphereRes + 2 * iMarginRes + 2 * iExtentRes )
	{
		sphereFaceIndex = 1;
		uv = (pixelCoord - int2(iTargetSphereRes + 3 * iMarginRes + 3 * iExtentRes, iMarginRes + iExtentRes) + 0.5) / iTargetSphereRes;
	}
	else
	{
		sphereFaceIndex = 0;
		uv = (pixelCoord - int2(iMarginRes + iExtentRes, iMarginRes + iExtentRes) + 0.5) / iTargetSphereRes;
	}

	float4 coord = ParaboloidToCube( uv, sphereFaceIndex );
	float3 result = SAMPLE_LEVEL( t_Texture, s_Texture, coord.xyz, 0 ).xyz;

#if IS_INTERIOR_FALLBACK
	{
		result *= result; // gamma -> linear approx
		const float lum = dot( float3( 0.3, 0.5, 0.2 ), result );
		const float2 factors = saturate( (lum - cInteriorFallbackParams.xy) * cInteriorFallbackParams.zw );
		float3 weights = float3( 1 - factors.x, factors.x * (1 - factors.y), factors.y );
		weights /= dot( 1, weights );
		const float3 colMul = weights.x * cInteriorFallbackColorLow + weights.y * cInteriorFallbackColorMiddle + weights.z * cInteriorFallbackColorHigh;
		result *= colMul;
	}
#endif

	return float4 ( result, 1 );
}
#endif
