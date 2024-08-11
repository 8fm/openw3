#include "postfx_common.fx"

#define vTexCoordTransformColor     	VSC_Custom_0

SamplerState	s_TextureColor  : register( s0 );
Texture2D		t_TextureColor	: register( t0 );

#define vBlurParams			    PSC_Custom_0	// blur size
#define vFlareParams			PSC_Custom_1	// screen position
#define vClampParams			PSC_Custom_2	// screen clamping paramters
#define vAspectRatio			PSC_Custom_3	

struct VS_INPUT
{
	float4 pos		   : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos	   	: SYS_POSITION;
	float2 coordColor  	: TEXCOORD0;
	float2 normalized	: TEXCOORD1;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   		= i.pos;
	o.normalized		= i.pos.xy * float2( 0.5f, -0.5f ) + float2( 0.5f, 0.5f );
	o.coordColor 		= ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformColor );

	return o;
}

#endif

static const int NumSamples = 64;

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0 
{
	// Accumualted sum
	float4 sum = float4(0,0,0,0);

	// We need to apply aspect ratio correction to the blur values
	float2 normalizedUV = i.normalized.xy;
	float2 blurVector = (vFlareParams.xy - normalizedUV);
	float blurLength = length( blurVector ) * 0.5f;

	// Shorten the length of the vector to limit undersampling
	blurVector = blurLength > 0 ? blurVector / blurLength * min(sqrt(blurLength) * .5f, blurLength) : 0;
	blurVector *= vBlurParams.x / (float)(NumSamples);
//return float4( frac(blurVector * 1000), 0, 0 );

	// Calculate weights
	float2 linearWeight = 2 * ( NumSamples.xx - float2(0, 1))  / (float)(NumSamples.xx);
	float2 linearWeightDelta = -float2(4, 4) / (float)(NumSamples.xx);
	
	// Calculate blur iterators
	float4 sampleUVs = i.coordColor.xyxy + blurVector.xyxy * float4(0, 0, 1, 1);
	float4 sampleUVsDelta = blurVector.xyxy * 2;
	sampleUVsDelta *= vAspectRatio.zwzw * 2;

	// Operate on two samples at a time to minimize ALU instructions
	for (int i = 0; i < NumSamples; i += 2)
	{
		// Use a weight that is linearly increasing away from the blur origin
		// This allows the tail of an occluder to blend out smoothly
		float2 weight = min(4.0f * linearWeight * linearWeight, linearWeight);

		// Clamp the sample position to make sure we only sample valid parts of the texture
		float4 clampedUVs = clamp( sampleUVs, vClampParams.xyxy, vClampParams.zwzw ); 

		// Accumulate
		sum += t_TextureColor.Sample( s_TextureColor, clampedUVs.xy ) * float4( weight.xxx, linearWeight.x );
		sum += t_TextureColor.Sample( s_TextureColor, clampedUVs.zw ) * float4( weight.yyy, linearWeight.y );

		// Move
		linearWeight += linearWeightDelta;
		sampleUVs += sampleUVsDelta;
	}
	
	// Average
	return float4( sum / NumSamples);
}
#endif
