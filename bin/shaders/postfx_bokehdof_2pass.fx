#include "blurSamples.fx"
#include "postfx_common.fx"
#include "include_constants.fx"

#define vTexCoordTransformFull	VSC_Custom_0
#define vTexCoordTransformHalf	VSC_Custom_1

#define vTexCoordClamp		PSC_Custom_0
#define vTexCoordDim		PSC_Custom_1
#define samplingRatio		PSC_Custom_3.zw

static const float invSampleCount = 1.0f/(SAMPLE_COUNT_0);

SamplerState	s_TextureOrgiginal			: register( s0 );
Texture2D		t_TextureOrgiginal			: register( t0 );

SamplerState	s_CacheColor				: register( s1 );
Texture2D		t_CacheColor				: register( t1 );


SamplerState	s_TextureNear				: register( s2 );
Texture2D		t_TextureNear				: register( t2 );

SamplerState	s_TextureFar				: register( s3 );
Texture2D		t_TextureFar				: register( t3 );

SamplerState	s_TextureCoCNearBlured  	: register( s4 );
Texture2D		t_TextureCoCNearBlured		: register( t4 );


struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
	float2 coordFull       : TEXCOORD0;
	float2 coordHalf       : TEXCOORD1;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   = i.pos;
	o.coordFull = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformFull );
	o.coordHalf = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformHalf );

	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{	
	float2 uvFull = i.coordFull.xy;
	float2 uvHalf = i.coordHalf.xy;

	float3 sampleData = t_CacheColor[ int2( i.pos.xy ) ].xyz;
	float sampleCoC =  sampleData.x;
	float avgCoC =  sampleData.y;
	
	float3 colorOrig = t_TextureOrgiginal[ int2( i.pos.xy ) ].xyz;

	float3 resultN = float3(0,0,0);
	float3 resultF = float3(0,0,0);

	float2 samplingRadius = sampleCoC * 0.5f * samplingRatio;

	[unroll]
	for ( int i = 0; i < SAMPLE_COUNT_0; ++i )
	{
		resultN += t_TextureNear.Sample( s_TextureNear, samples_0[i] * samplingRadius + uvHalf ).xyz;
		resultF += t_TextureFar.Sample( s_TextureFar, samples_0[i] * samplingRadius + uvHalf ).xyz;
	}

	resultN *= invSampleCount;
	resultF *= invSampleCount;
	
	const float nonLinearBlendScale = 0.15f;

	float depthScaleFar = pow( max( sampleData.z * 2.0 - 1.0 , 0.0 ) , nonLinearBlendScale );
	float3 result = lerp( colorOrig , resultF , depthScaleFar );
	
	float depthScaleNearBlured = t_TextureCoCNearBlured.Sample( s_TextureCoCNearBlured, uvHalf ).y;
	float depthScaleNear = pow( max( sampleData.z * -2.0 + 1.0 , 0.0 ) , nonLinearBlendScale );
	float depthScaleNearFinal = max( depthScaleNear , depthScaleNearBlured );

	result = lerp( result , resultN , depthScaleNearFinal );

	// return float4( depthScaleNearBlured , 0 , 0 , 1 );
	// float error = length( colorOrig - resultF );
	// return float4( depthScaleFar , 0 , 0 , 1 );
	/*
	if( uvFull.x < uvFull.y )
		return float4( result , 1.0f );
	if( uvFull.x < uvFull.y + 0.001 )
		return float4(1,0,0,0);
	return float4( resultF , 1.0f );
	*/
	return float4( result , 1.0f );

}
#endif
