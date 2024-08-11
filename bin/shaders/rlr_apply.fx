#include "postfx_common.fx"
#include "include_localReflection.fx"

Texture2D		t_TextureReflection		: register( t0 );
Texture2D		t_TextureMaskFull		: register( t1 );

#if !IS_FULL_APPLY
	Texture2D			t_TextureDepth			: register( t2 );
#endif

Texture2D		t_RLRSky			: register( t3 );

SamplerState	s_SamplerPoint		: register( s0 );
SamplerState	s_SamplerLinear		: register( s1 );
SamplerState	s_SamplerReflection	: register( s2 );

#define			vFeedbackDimensions		PSC_Custom_0
#define			vRLRSkyParams			PSC_Custom_1
#define			vMaskDimensions			PSC_Custom_2

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
	const uint2 pixelCoord = (uint2)i.pos.xy;

	float4 result	= 0;
	float4 mask		= t_TextureMaskFull[pixelCoord];
	
	float2 coordOffset = 0;
	{
		// The final perceptive offset is independent of the rendering resolution and
		// the size of reflection buffers. Which is quite cool :)
		// Also the onscreen offset ratio is constant.
		//
		// Decoded offset value is in range of {-max_offset_value .. max_offset_value}.
		// Vertical offset value of 1 means shifting coordinate by one vertical screen size.
		// Horizontal offset value is being adjusted to keep the onscreen aspect ratio.
		//
		// Positive horizontal offsets sample from the right screen side (so they shift the image to the left)
		// Positive vertical offsets sample from the bottom screen side (so they shift the image to the top)

		coordOffset = (mask.xy - 0.5) * (2.0 * REALTIME_REFLECTIONS_MAX_OFFSET_VALUE);
		coordOffset.x *= vFeedbackDimensions.y / vFeedbackDimensions.x;
		coordOffset *= vFeedbackDimensions.xy / vFeedbackDimensions.zw;
	}

	const float2 coordOrig = (pixelCoord + 0.5) / screenDimensions.xy * (vFeedbackDimensions.xy / vFeedbackDimensions.zw);
	float2 coord = coordOrig;
	{
		coord += coordOffset;
		coord = clamp( coord, 0.5 / vFeedbackDimensions.zw, (vFeedbackDimensions.xy - 1.5) / vFeedbackDimensions.zw );	
	}	

	const float2 coord_ddx = ddx( coord );
	const float2 coord_ddy = ddy( coord );

 	[branch]
 	if ( mask.w > 0 )
	{
		mask.w = pow( mask.w, REALTIME_REFLECTIONS_MASK_GAMMA );

		const float amount = mask.w;			

		float4 reflectionValue = 0;
		#if IS_FULL_APPLY
		{
			// Read the reflected color
			reflectionValue = SAMPLE_GRADIENT( t_TextureReflection, s_SamplerReflection, coord, coord_ddx, coord_ddy );
			//reflectionValue = SAMPLE_LEVEL( t_TextureReflection, s_SamplerReflection, coord, 0 );
			
			/*
			// In case we fetched a texel with no reflection value, then we need some information there anyway.
			// So the cheesy solution here is to just use the reflection value without the offset (we can assume it's valid).
			// Not the best solution out there, but works as a complete fallback.
			const float threshold = 0.0001;
			[branch]
			if ( reflectionValue.w <= threshold )
			{
				const int num_fallback_iters = 5;
				for ( int i=0; i<num_fallback_iters && reflectionValue.w <= threshold; ++i )
				{
					float t = (i + 1.0) / num_fallback_iters;
					reflectionValue = SAMPLE_LEVEL( t_TextureReflection, s_SamplerReflection, lerp( coord, coordOrig, t ), 0 );
				}
			}
			*/
		}
		#else
		{
			float depthValue = t_TextureDepth[pixelCoord];
			float3 worldPos = PositionFromDepthRevProjAware( depthValue, pixelCoord + coordOffset * screenDimensions.xy * (vFeedbackDimensions.zw / vFeedbackDimensions.xy) ); // let's approximate the world pos. I'm aware it's not perfect, but seems to be enough.
			float3 worldNormal = float3( 0, 0, 1 );
			// using the same manipulated reflection vector with no traced reflections is here to preserve as much of the original look as possible (just without RLR)
			float3 reflectedDir = CalcDirReflectedWorld( worldPos, worldNormal );
			reflectionValue = float4( SampleRLREnvProbe( mask.z > 0.5, worldPos, reflectedDir, t_RLRSky, s_SamplerLinear, vRLRSkyParams ).xyz, 1 );
		}
		#endif

		// reflection color is premultiplied 
		// (see function which calculates the reflection for more details)
		if ( reflectionValue.w > 0.0 )
		{
			reflectionValue.xyz /= reflectionValue.w;
		}

		result = float4 ( reflectionValue.xyz * amount, 0 );
	}

	return result;
}
#endif
