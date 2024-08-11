#include "postfx_common.fx"

SamplerState	s_TextureColor  : register( s0 );
Texture2D		t_TextureColor	: register( t0 );

#define			vTextureFullRes		PSC_Custom_0.xy
#define			vTargetFullRes		PSC_Custom_1.xy
#define			vAreaRes			PSC_Custom_2.xy
#define			iSourceOffsetXY		((int2)PSC_Custom_3.xy)
#define			iTargetOffsetXY		((int2)PSC_Custom_3.zw)
#define			vSunPos				PSC_Custom_4.xy
#define			iPassIndex			((int)PSC_Custom_4.z)
#define			fShaftsRange		PSC_Custom_5.x
#define			fShaftsShapeExp			PSC_Custom_5.y
#define			fShaftsShapeInvSquare	PSC_Custom_5.z
#define			cColor				(PSC_Custom_6.xyz)
#define			fThreshold			(PSC_Custom_7.x)
#define			fThresholdRange		(PSC_Custom_7.y)


#if defined(QUALITY_LEVEL) && QUALITY_LEVEL == 0
#define NUM_SAMPLES									16
#elif QUALITY_LEVEL == 1
#define NUM_SAMPLES									20
#elif QUALITY_LEVEL == 2
#define NUM_SAMPLES									24
#else
#error Unsupported value for QUALITY_LEVEL (must be defined, and must be 0,1,2)
#endif

// debug stuff
#define ENABLE_DEBUG_FIRST_PASS_ONLY				0
#define ENABLE_CLAMP_ERROR_DISPLAY					0
#define ENABLE_CENTER_DISTANCE_THRESHOLD_DISPLAY	0


	
struct VS_INPUT
{
	float4 pos		: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos	   	: SYS_POSITION;
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
	const int2 pixelCoord = i.pos.xy - iTargetOffsetXY;
	const float2 coordMin = (0.5 + (float2)iSourceOffsetXY) / vTextureFullRes;
	const float2 coordMax = ((float2)iSourceOffsetXY + vAreaRes - 0.5) / vTextureFullRes;
	const float2 coord = (pixelCoord + 0.5 + (float2)iSourceOffsetXY) / vTextureFullRes;
	const float2 blurCenter = lerp( coordMin, coordMax, vSunPos );
		
	float4 sum = float4(0,0,0,0);

	const float2 blurVector = (blurCenter - coord);
	const float  blurLength = length( blurVector );

	const float2 fullVec = fShaftsRange * normalize( blurVector ) * ( coordMax.y - coordMin.y ) / length( normalize( blurVector ) * float2( vTextureFullRes.x / vTextureFullRes.y, 1 ) );
	const float2 stepStart = coord;

	float2 stepVec = fullVec / NUM_SAMPLES;		
	int numIters = (blurLength / length( stepVec ) + 1);
	if ( iPassIndex )
	{
		stepVec = stepVec / NUM_SAMPLES;
		numIters = (blurLength / length( stepVec ) + 1);
	}

	{		
		float2 t;
		t.x = stepVec.x > 0	?	 (coordMax.x - coord.x) 		:	(coord.x - coordMin.x);
		t.y = stepVec.y > 0	?	 (coordMax.y - coord.y) 		:   (coord.y - coordMin.y);

		numIters = min( numIters, (int)( t.x / abs( stepVec.x ) + 1 ) );
		numIters = min( numIters, (int)( t.y / abs( stepVec.y ) + 1 ) );
	}	
	
#if ENABLE_CLAMP_ERROR_DISPLAY
	float error = 0;
#endif
	
	float centerDistFactor = 0;
	{
		float d = length( (coord - blurCenter) * float2( vTextureFullRes.x / vTextureFullRes.y, 1 ) ) / (coordMax.y - coordMin.y);
		float t = saturate( d / fShaftsRange );
		centerDistFactor = t;
	}

	[branch]
	if ( centerDistFactor < 1 )
	{
		[branch]
		if ( 0 != iPassIndex )
		{
			[unroll]
			for ( int i=0; i<NUM_SAMPLES; ++i )
			{		
				const float weight = (i < numIters ? 1 : 0 );
				const float2 currCoord = stepStart + stepVec * i;		
				sum += weight * SAMPLE_LEVEL( t_TextureColor, s_TextureColor, currCoord, 0 );
		#if ENABLE_CLAMP_ERROR_DISPLAY
				error = weight > 0 ? max( error, length( currCoord - clamp(currCoord, coordMin, coordMax ) ) ) : error;
		#endif
			}

			sum.xyz *= cColor;
		}
		else
		{
			[unroll]
			for ( int i=0; i<NUM_SAMPLES; ++i )
			{		
				const float weight = (i < numIters ? 1 : 0 );
				const float2 currCoord = stepStart + stepVec * i;		
				float3 col = SAMPLE_LEVEL( t_TextureColor, s_TextureColor, currCoord, 0 );
									
				// ace_optimize: move this to prepass
				{
					float lumCurr = dot( RGB_LUMINANCE_WEIGHTS_LINEAR, col );
					float lumDiff = max( 0, lumCurr - fThreshold );
					float lumTarget = lumDiff * saturate( lumDiff * fThresholdRange );
					col *= lumTarget / max( 0.0001, lumCurr );
				}

				sum.xyz += weight * col;
		#if ENABLE_CLAMP_ERROR_DISPLAY
				error = weight > 0 ? max( error, length( currCoord - clamp(currCoord, coordMin, coordMax ) ) ) : error;
		#endif
			}
		}

		sum *= (1.f / NUM_SAMPLES);

		if ( iPassIndex )
		{
			float t = 1;
			t *= pow( 1 - centerDistFactor, fShaftsShapeExp );
			t /= (1.0 + fShaftsShapeInvSquare * centerDistFactor * centerDistFactor);
			sum.xyz *= t;
		} 
	}
#if ENABLE_CENTER_DISTANCE_THRESHOLD_DISPLAY
	else
	{
		sum = float4( 10, 10, 0, 1 );
	}
#endif
	
#if ENABLE_DEBUG_FIRST_PASS_ONLY
	sum = iPassIndex ? float4( cColor, 1 ) * t_TextureColor.Sample( s_TextureColor, coord ) : sum;
#endif

#if ENABLE_CLAMP_ERROR_DISPLAY
	sum.xyz += float3(1,0,0) * error * 10000;
#endif

	return float4( sum.xyz, 1 );
}
#endif
