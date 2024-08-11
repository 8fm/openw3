#include "postfx_common.fx"

Texture2D				t_Texture			: register( t0 );
SamplerState			s_Texture			: register( s0 );
#define					iTargetWidth		((int)PSC_Custom_0.x)
#define					iTargetHeight		((int)PSC_Custom_0.y)
#define					iSphereRes			((int)PSC_Custom_0.z)
#define					iMarginRes			((int)PSC_Custom_0.w)
#define					v2BlurDir			(PSC_Custom_1.xy)
#define					iMipIndex			((int)PSC_Custom_1.z)

#if !IS_SMALL_BLUR
# define					vWeights0			(PSC_Custom_2)
# define					vWeights1			(PSC_Custom_3)
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

	float CalcGauss( float normFactor )
	{
		float a = 1;
		float b = 0;
		float c = 2.1f;
		float d = 6.4f;
		float e = 1.5f;

		return pow( a * exp( - pow((normFactor - b) * d, 2) / (2 * c * c) ), e );
	}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const int2 pixelCoord = (int2)i.pos.xy;

	int segmentSize = iSphereRes + 2 * iMarginRes;
	int2 segmentIdx = pixelCoord / segmentSize;
	int2 clampMin = segmentIdx * segmentSize;
	int2 clampMax = clampMin + segmentSize - 1;

	const float2 coordBase = (pixelCoord + 0.5) / float2( iTargetWidth, iTargetHeight );
	const float2 coordClampMin = (clampMin + 0.5) / float2( iTargetWidth, iTargetHeight );
	const float2 coordClampMax = (clampMax + 0.5) / float2( iTargetWidth, iTargetHeight );

	float2 coordStep = v2BlurDir / float2( iTargetWidth, iTargetHeight );

	// scale step size to compensate non uniform paraboloid distribution
	// ace_todo: find a more accurate (this ones was judged 'by the eye' and faster method)
	coordStep *= lerp( 1.0, 0.5, abs( ParaboloidToCube( ((pixelCoord - clampMin) - iMarginRes + 0.5) / iSphereRes, 0 ).z ) );	// sphereFaceIndex doesn't matter here
	
	// TODO: move this to parameters
	if ( 4 == iMipIndex )
	{
		coordStep *= 0.85;
	}
	if ( 5 == iMipIndex )
	{
		coordStep *= 0.5;
	}
	
	float3 sum = 0;

#if 1
	#if IS_SMALL_BLUR
	{
		float2 crd0 = coordBase - 1 * coordStep;
		float2 crd1 = coordBase;
		float2 crd2 = coordBase + 1 * coordStep;

		float w0 = 1;
		float w1 = 0.5;

		sum += w1 * (all( crd0 == clamp( crd0, coordClampMin, coordClampMax ) )) * SAMPLE_LEVEL( t_Texture, s_Texture, crd0, 0 ).xyz;
		sum += w0 * (all( crd1 == clamp( crd1, coordClampMin, coordClampMax ) )) * SAMPLE_LEVEL( t_Texture, s_Texture, crd1, 0 ).xyz;
		sum += w1 * (all( crd2 == clamp( crd2, coordClampMin, coordClampMax ) )) * SAMPLE_LEVEL( t_Texture, s_Texture, crd2, 0 ).xyz;
		sum /= w0 + 2 * w1;
	}
	#else
	{
		const int range = 5;
		const float weights[range + 1] = { vWeights0.x, vWeights0.y, vWeights0.z, vWeights0.w, vWeights1.x, vWeights1.y };		
		[unroll]
		for ( int k=-range; k<=range; ++k )
		{
			float weight = weights[ abs(k) ];

			float2 crd = coordBase + k * coordStep;
			float2 clampedCrd = clamp( crd, coordClampMin, coordClampMax );		

			float3 val = SAMPLE_LEVEL( t_Texture, s_Texture, crd, 0 ).xyz;
			
			sum += weight * (all(crd == clampedCrd) ? val : 0);
		}
	}
	#endif
#else
	{
		sum += vWeights1.y * SAMPLE_LEVEL( t_Texture, s_Texture, coordBase - 5 * coordStep, 0 ).xyz;
		sum += vWeights1.x * SAMPLE_LEVEL( t_Texture, s_Texture, coordBase - 4 * coordStep, 0 ).xyz;
		sum += vWeights0.w * SAMPLE_LEVEL( t_Texture, s_Texture, coordBase - 3 * coordStep, 0 ).xyz;
		sum += vWeights0.z * SAMPLE_LEVEL( t_Texture, s_Texture, coordBase - 2 * coordStep, 0 ).xyz;
		sum += vWeights0.y * SAMPLE_LEVEL( t_Texture, s_Texture, coordBase - 1 * coordStep, 0 ).xyz;
		sum += vWeights0.x * SAMPLE_LEVEL( t_Texture, s_Texture, coordBase                , 0 ).xyz;
		sum += vWeights0.y * SAMPLE_LEVEL( t_Texture, s_Texture, coordBase + 1 * coordStep, 0 ).xyz;
		sum += vWeights0.z * SAMPLE_LEVEL( t_Texture, s_Texture, coordBase + 2 * coordStep, 0 ).xyz;
		sum += vWeights0.w * SAMPLE_LEVEL( t_Texture, s_Texture, coordBase + 3 * coordStep, 0 ).xyz;
		sum += vWeights1.x * SAMPLE_LEVEL( t_Texture, s_Texture, coordBase + 4 * coordStep, 0 ).xyz;
		sum += vWeights1.y * SAMPLE_LEVEL( t_Texture, s_Texture, coordBase + 5 * coordStep, 0 ).xyz;
	}
#endif

	return float4 ( sum, 1 );
}
#endif
