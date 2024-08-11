#define BOKEH_SUPER_DUPER_SAMPLES_LVL 0

#include "bokehSamples.fx"
#include "postfx_common.fx"

#define vTexCoordTransformFull	VSC_Custom_0
#define vTexCoordTransformHalf	VSC_Custom_1

#define vTexCoordClamp		PSC_Custom_0
#define vTexCoordDim		PSC_Custom_1
#define vTexCameraParams	PSC_Custom_2
#define bokehRatio			PSC_Custom_3.xy

#define planeInFocus		vTexCameraParams.x
#define focalLength			vTexCameraParams.y
#define apertureDiameter	vTexCameraParams.z
#define maxCoCsize			vTexCameraParams.w 
#define cicreHexBlend		vTexCoordDim.w

#define BRIGHT_DOTS

#ifdef BRIGHT_DOTS
	#define MakeColor(a)	((a)*(a))
	#define GetColor(a)		sqrt(a)
#else
	#define MakeColor(a)	(a)
	#define GetColor(a)		(a)
#endif

SamplerState	s_ColorNear				: register( s0 );
Texture2D		t_ColorNear				: register( t0 );

SamplerState	s_ColorFar				: register( s1 );
Texture2D		t_ColorFar				: register( t1 );

SamplerState	s_CacheColor			: register( s2 );
Texture2D		t_CacheColor			: register( t2 );

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
	float2 coord       : TEXCOORD0;
};

struct PS_OUTPUT
{
	float4 colorNear	: SYS_TARGET_OUTPUT0;
	float4 colorFar		: SYS_TARGET_OUTPUT1;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   = i.pos;

	o.coord = ApplyTexCoordTransform( i.pos.xy , vTexCoordTransformHalf );

	return o;
}

#endif

#ifdef PIXELSHADER


float2 CoCToArea( float2 r )
{
	// r = 0.25f * ( r / ( vTexCoordDim.y * 18.0f ) );
	// return max( r*r , 0.00001f );
	return r;
}


void ps_main( VS_OUTPUT i, out PS_OUTPUT o ) 
{		
	float2 uv = i.coord.xy;
	
	float3 cacheValues = t_CacheColor.Sample( s_CacheColor, uv ).xyz; // [ int2( i.pos.xy * 2.0f ) ].xyz; // it need to be sampled, doue to interpolation of subpixels

	/////////////////////////////////////////////////
	// NEAR

	float samplingRadiusNear = cacheValues.x;
	float weightNear = cacheValues.y + 0.001f;

	float accumulatedSamplesNear = weightNear;
	float3 resultNear = MakeColor( t_ColorNear.Sample( s_ColorNear, uv ).xyz ) * weightNear;
	
	/////////////////////////////////////////////////
	// FAR

	float samplingRadiusFar = cacheValues.x;
	float weightFar = cacheValues.z + 0.001f;

	float accumulatedSamplesFar = weightFar;
	float3 resultFar = MakeColor( t_ColorFar.Sample( s_ColorFar, uv ).xyz ) * weightFar;

	/////////////////////////////////////////////////
	[loop]
	for ( int i = 0; i < BOKEH_SAMPLE_COUNT; i++ )
	{
		
		// This works until the radius for both near and far is exacly the same
		float2 sampleVector = ( samples[i].zw * cicreHexBlend + samples[i].xy ) * samplingRadiusNear;
		float2 sampleUV = sampleVector * bokehRatio + uv;
		float2 commonWeight = CoCToArea( t_CacheColor.Sample( s_CacheColor, sampleUV ).yz );	

		/////////////////////////////////////////////////
		// NEAR
		{
			// float2 sampleVector = ( samples[i].zw * cicreHexBlend + samples[i].xy ) * samplingRadiusNear;
			// float2 sampleUV = sampleVector * bokehRatio + uv;
			// float sampleWeight = t_CacheColor.Sample( s_CacheColor, sampleUV ).y;

			float3 sampledColor =  t_ColorNear.Sample( s_ColorNear, sampleUV ).xyz;
			float sampleWeight = commonWeight.x;

			resultNear += MakeColor( sampledColor ) * sampleWeight;
			accumulatedSamplesNear += sampleWeight;
		}
		/////////////////////////////////////////////////
		// FAR
		{
			// float2 sampleVector = ( samples[i].zw * cicreHexBlend + samples[i].xy ) * samplingRadiusFar;
			// float2 sampleUV = sampleVector * bokehRatio + uv;
			// float sampleWeight = t_CacheColor.Sample( s_CacheColor, sampleUV ).y;

			float3 sampledColor =  t_ColorFar.Sample( s_ColorFar, sampleUV ).xyz;
			float sampleWeight = commonWeight.y;

			resultFar += MakeColor( sampledColor ) * sampleWeight;
			accumulatedSamplesFar += sampleWeight;
		}
	}

	/////////////////////////////////////////////////
	// Shit this out
	o.colorNear = float4( GetColor( resultNear / accumulatedSamplesNear ) , 1.0 );
	o.colorFar  = float4( GetColor( resultFar / accumulatedSamplesFar ) , 1.0 );
}
#endif
