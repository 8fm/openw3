#include "commonVS.fx"


/// The input stream for the two meshes.
struct VS_INPUT
{
	float4		Position0		: POSITION0;
#ifdef USE_SKINNING
	// Skinning info. We aren't doing anything with it, so just take it as 32-bit uint and pass through, rather than converting
	// to something like float4 and then back.
	uint		BlendIndices0	: BLENDINDICES0;
	uint		BlendWeights0	: BLENDWEIGHT0;
#endif
	float3		Normal0			: NORMAL0;
	float4		Tangent0		: TANGENT0;

	float4		Position1		: POSITION1;
	float3		Normal1			: NORMAL1;
	float4		Tangent1		: TANGENT1;

#ifdef USE_CONTROL_TEX
	float2		TexCoord		: TEXCOORD0;
#endif
};


struct VS_OUTPUT
{
	// GS always outputs 32-bit components, so we have to pack these things ourselves.

	// UShort4N packed. Quantized position.
#ifdef USE_SKINNING
	// Pack skinning weights/indices in here as well.
	uint4		Position		: TEXCOORD0;
#else
	uint2		Position		: TEXCOORD0;
#endif

	// 10-10-10-2 unorm packed. Normal and Tangent packed into one element, again because we're writing to multiple buffers.
	uint2		NormalTangent	: NORMAL0;
};


#ifdef VERTEXSHADER


#ifdef USE_CONTROL_TEX
SYS_SAMPLER( ControlTexture, 0 );
#endif


uint packDec4( float4 v )
{
	float4 limit = float4( 1023, 1023, 1023, 3 );
	uint4 vi = (uint4)clamp( saturate( v ) * limit, 0, limit );
	return ( vi.x ) | ( vi.y << 10 ) | ( vi.z << 20 ) | ( vi.w << 30 );
}

uint2 packUShort4N( float4 v )
{
	uint4 vi = (uint4)clamp( saturate( v ) * 65535, 0, 65535 );
	return uint2(
		( vi.x ) | ( vi.y << 16 ),
		( vi.z ) | ( vi.w << 16 )
	);
}


#define Source_QS VSC_Custom_1
#define Source_QB VSC_Custom_2
#define Target_QS VSC_Custom_3
#define Target_QB VSC_Custom_4

#define Morph_QS VSC_Custom_5
#define Morph_QB VSC_Custom_6


// This is the same as is used in the morphed materials.
float CalcBlending( float morphRatio, float controlValue )
{
	// controlValue of 0 or 1 is not good, so rescale to slightly inside that range
	controlValue = (1.0f/255.0) + saturate( controlValue ) * (253.0/255.0);

	// when controlValue is small, we compress the 0-1 interpolation towards morphRatio=0
	if ( controlValue < 0.5 )
	{
		return ( 0.5 / controlValue ) * morphRatio;
	}
	// when controlValue is large, we compress the 0-1 interpolation towards morphRatio=1
	else
	{
		float ratio = ( 0.5 - controlValue ) / ( 1 - controlValue );
		return lerp( morphRatio, 1, ratio );
	}
}


VS_OUTPUT vs_main( in VS_INPUT input )
{
	VS_OUTPUT output;

	float blend = VSC_Custom_0.x;

#ifdef USE_CONTROL_TEX
	float control = SYS_SAMPLE_LEVEL( ControlTexture, input.TexCoord, 0 ).x;
	blend = saturate( CalcBlending( blend, control ) );
#endif

	// Uncompress potentially quantized positions
	float4 localPosition0 = float4( input.Position0.xyz * Source_QS.xyz + Source_QB.xyz, 1 );
	float4 localPosition1 = float4( input.Position1.xyz * Target_QS.xyz + Target_QB.xyz, 1 );

	float4 position	= lerp( localPosition0,			localPosition1,			blend );
	// Re-compress position
	position		= ( position - Morph_QB ) / Morph_QS;

	float3 norm		= lerp( input.Normal0,			input.Normal1,			blend );
	float4 tangent	= lerp( input.Tangent0,			input.Tangent1,			blend );

	output.Position.xy	= packUShort4N( position );
#ifdef USE_SKINNING
	output.Position.z	= input.BlendIndices0;
	output.Position.w	= input.BlendWeights0;
#endif

	output.NormalTangent.x	= packDec4( float4( norm, 0 ) );
	output.NormalTangent.y	= packDec4( tangent );

	return output;
}

[MAX_VERTEX_COUNT(1)]
void gs_main( GS_INPUT_POINT VS_OUTPUT In[1], inout GS_BUFFER_POINT<VS_OUTPUT> Out )
{
	Out.Append( In[0] );
}

#endif
