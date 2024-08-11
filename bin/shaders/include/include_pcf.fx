#define NUM_SAMPLES_HQ				27
#define NUM_SAMPLES_LQ				13
#define NUM_SAMPLES_DEPTH			6

static const float2 shadowDepthSamples[NUM_SAMPLES_DEPTH] =
{
	float2( 0.0f, 0.0f ),
	float2( 0.0f, 1.0f ),
	float2( 0.95106f, 0.30902f ),
	float2( 0.58778f, -0.80902f ),
	float2( -0.95106f, 0.30902f ),
	float2( -0.58778f, -0.80902f ),
};

static const float2 poissonDiskHQ[NUM_SAMPLES_HQ] =
{ 
	float2( 0, 0 ),
	float2( -0.2633539f, 0.03965382f ),
	float2( -0.7172024f, 0.3683033f ),
	float2( -0.01995306f, -0.3988392f ),
	float2( -0.4409938f, -0.22101f ),
	float2( 0.2200392f, 0.2943448f ),
	float2( -0.7675657f, -0.1482753f ),
	float2( -0.2381019f, 0.5471062f ),
	float2( 0.2173031f, -0.1345879f ),
	float2( -0.6387774f, 0.7561339f ),
	float2( -0.1614144f, 0.9776521f ),
	float2( 0.2156319f, 0.6123413f ),
	float2( 0.2328875f, 0.9452872f ),
	float2( -0.9440644f, 0.1236077f ),
	float2( -0.7408237f, -0.507683f ),
	float2( -0.4113019f, -0.8905967f ),
	float2( -0.2486429f, -0.6213993f ),
	float2( 0.696785f, 0.2644937f ),
	float2( 0.5394363f, 0.8173215f ),
	float2( 0.6151208f, -0.149864f ),
	float2( 0.09365336f, -0.7817475f ),
	float2( 0.2768067f, -0.4895968f ),
	float2( 0.6639181f, -0.6007172f ),
	float2( 0.3880369f, -0.8950894f ),
	float2( 0.9916144f, -0.07939152f ),
	float2( 0.7831848f, 0.6029348f ),
	float2( 0.8998603f, -0.3983543f ),
}; 

static const float2 poissonDiskLQ[NUM_SAMPLES_LQ] =
{ 
	float2( 0, 0 ),
	float2(-0.5077208f, -0.6170899f),
	float2(-0.05154902f, -0.7508098f),
	float2(-0.7641042f, 0.2009152f),
	float2(0.007284774f, -0.205947f),
	float2(-0.366204f, 0.7323797f),
	float2(-0.9239582f, -0.2559643f),
	float2(-0.286924f, 0.278053f),
	float2(0.7723012f, 0.1573329f),
	float2(0.378513f, 0.4052199f),
	float2(0.4630217f, -0.6914619f),
	float2(0.423523f, 0.863028f),
	float2(0.8558643f, -0.3382554f),
}; 

float PenumbraSize(float zReceiver, float zBlocker) //Parallel plane estimation
{ 
    return (zReceiver - zBlocker) / zBlocker; 
}

void FindBlocker( TEXTURE2D_ARRAY<float> sShadowMapArray,
				in float searchWidth,
				out float avgBlockerDepth,  
                out float numBlockers, 
                float3 uvc, float zReceiver ) 
{ 
	float blockerSum = 0;
    numBlockers = 0;      
    for( int i = 0; i < NUM_SAMPLES_LQ; ++i ) 
    { 
		float shadowMapDepth = SAMPLE_LEVEL( sShadowMapArray, samplPoint,
			float3( uvc.xy + poissonDiskLQ[i].xy * searchWidth, uvc.z ), 0); 

        if ( shadowMapDepth < zReceiver )
		{ 
                blockerSum += shadowMapDepth; 
                numBlockers++; 
        } 
	}

    avgBlockerDepth = blockerSum / numBlockers; 
}

float PCF_FilterHigh( TEXTURE2D_ARRAY<float> sShadowMapArray, float3 uvc, float zReceiver, float filterRadiusUV, float shadowGradient, int qualityDegradation ) 
{ 
    float sum = 0.0f; 

#if defined(COMPILE_IN_SHADING_HAIR)
	const int numSamples = 6;
#else
	const int numSamples = NUM_SAMPLES_HQ / min( 1 + qualityDegradation, NUM_SAMPLES_HQ );
#endif	

	[unroll]
	for ( int i = 0; i < numSamples; ++i ) 
    { 
		float2 offset = poissonDiskHQ[i] * filterRadiusUV; 
        float currZReceiver = zReceiver - shadowGradient * (i / (float)numSamples);        
        sum += SAMPLE_CMP_LEVEL0( sShadowMapArray, samplShadowComparison, float3(uvc.xy + offset.xy, uvc.z), currZReceiver );
    }

	float result = sum / numSamples;
	result *= result;
    return result; 
} 

float PCF_FilterLow( TEXTURE2D_ARRAY<float> sShadowMapArray, float3 uvc, float zReceiver, float filterRadiusUV, float shadowGradient, int qualityDegradation ) 
{ 
    float sum = 0.0f; 

#if defined(COMPILE_IN_SHADING_HAIR)
	const int numSamples = 1;
#else
	const int numSamples = NUM_SAMPLES_LQ / min( 1 + qualityDegradation, NUM_SAMPLES_LQ );
#endif		

	[unroll]
	for ( int i = 0; i < numSamples; ++i ) 
    { 
        float2 offset = poissonDiskLQ[i] * filterRadiusUV; 
		float currZReceiver = zReceiver - shadowGradient * (i / (float)numSamples);
        sum += SAMPLE_CMP_LEVEL0( sShadowMapArray, samplShadowComparison, float3(uvc.xy + offset.xy, uvc.z), currZReceiver ); 
    }

    float result = sum / numSamples;
    result *= result;
	return result; 
} 

float ShadowSurfaceDepth( TEXTURE2D_ARRAY<float> sShadowMapArray, float3 tc, int iCascadeIndex, float kernelSize, float maxDepth )
{
	float3 uvc = float3( tc.xy, iCascadeIndex );
	float zReceiver = tc.z; // Assumed to be eye-space z in this code

	float sum = 0.0f;
	float sh_depth = vShadowDepthRanges[iCascadeIndex];
	const int numSamples = NUM_SAMPLES_DEPTH;
	
	[unroll]
	for ( int i = 0; i < numSamples; ++i ) 
	{
		float2 offset = shadowDepthSamples[i] * kernelSize; 
		sum += clamp( (zReceiver - SAMPLE_LEVEL( sShadowMapArray, samplPoint, float3(uvc.xy + offset, uvc.z), 0).x ) * sh_depth, 0, maxDepth ); 
	}

	float result = sum / numSamples;

	return result / maxDepth; 

}

float PCF_Shadow( TEXTURE2D_ARRAY<float> sShadowMapArray, float3 tc, int iCascadeIndex, float kernelSize, float shadowGradient, int qualityDegradation )
{
    float3 uvc = float3( tc.xy, iCascadeIndex );
	float zReceiver = tc.z; // Assumed to be eye-space z in this code

	float retValue = 0;		

#if defined(COMPILE_IN_SHADING_HAIR)
	// Expand kernel size for the hair
	const float filterRadiusUV = 8.0f*kernelSize;
#else
	// Simple filter, constant kernel size
	const float filterRadiusUV = kernelSize;
#endif

	[branch]
	if ( ShadowQuality >= 1 )
	{		
		retValue = PCF_FilterHigh( sShadowMapArray, uvc, zReceiver, filterRadiusUV, shadowGradient, qualityDegradation ); 
	}
	else
	{	
		retValue = PCF_FilterLow( sShadowMapArray, uvc, zReceiver, filterRadiusUV, shadowGradient, qualityDegradation ); 
	}
	
	return retValue;
}
/* make sure leave an empty line at the end of ifdef'd files because of SpeedTree compiler error when including two ifdef'ed files one by one : it produces something like "#endif#ifdef XXXX" which results with a bug */
