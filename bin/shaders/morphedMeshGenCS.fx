#include "common.fx"
#include "commonCS.fx"

CS_CUSTOM_CONSTANT_BUFFER
	float4 Source_QS;
	float4 Source_QB;
	float4 Target_QS;
	float4 Target_QB;
	float4 Morph_QS;
	float4 Morph_QB;
	float Weight;
	uint NumVertices;
	uint Source_PosByteOffset;
	uint Source_TbnByteOffset;
	uint Source_TexByteOffset;
	uint Target_PosByteOffset;
	uint Target_TbnByteOffset;
	uint OutputByteOffset;
END_CS_CUSTOM_CONSTANT_BUFFER

// Input
BYTEBUFFER SourceVB : register(t0);
BYTEBUFFER TargetVB : register(t1);

// Output
RW_BYTEBUFFER StreamOut : register(u0);

#ifdef USE_CONTROL_TEX
SYS_SAMPLER( ControlTexture, 2 );
#endif

struct VB_INPUT
{
	float4		Position;
#ifdef USE_SKINNING
	// Skinning info. We aren't doing anything with it, so just take it as 32-bit uint and pass through, rather than converting
	// to something like float4 and then back.
	uint		BlendIndices;
	uint		BlendWeights;
#endif
	float3		Normal;
	float4		Tangent;
#ifdef USE_CONTROL_TEX
	float2		TexCoord;
#endif
};

#ifdef USE_SKINNING
// ushort4n position and ubyte4/ubyte4n skinning interleaved
static const uint POS_STRIDE = 16;
#else
// ushort4n position
static const uint POS_STRIDE = 8;
#endif

// 2x"10-10-10-2"
static const uint TBN_STRIDE = 8;

static const uint TEX_STRIDE = 4;

struct VB_OUTPUT
{
	// UShort4N packed. Quantized position.
#ifdef USE_SKINNING
	// Pack skinning weights/indices in here as well.
	uint4		Position;
#else
	uint2		Position;
#endif
	// 10-10-10-2 unorm packed. Normal and Tangent packed into one element, again because we're writing to multiple buffers.
	uint2		NormalTangent;
};

uint packDec4( float4 v )
{
	float4 limit = float4( 1023, 1023, 1023, 3 );
	uint4 vi = (uint4)clamp( saturate( v ) * limit, 0, limit );
	return ( vi.x ) | ( vi.y << 10 ) | ( vi.z << 20 ) | ( vi.w << 30 );
}

float4 unpackDec4( uint packedValue )
{
	return float4(
		( packedValue ) & 0x3ff,
		( packedValue >> 10 ) & 0x3ff,
		( packedValue >> 20 ) & 0x3ff,
		( packedValue >> 30 ) & 0x3
	) / float4(1023.0f, 1023.0f, 1023.0f, 3.0f );
}

uint2 packUShort4N( float4 v )
{
	uint4 vi = (uint4)clamp( saturate( v ) * 65535, 0, 65535 );
	return uint2(
		( vi.x ) | ( vi.y << 16 ),
		( vi.z ) | ( vi.w << 16 )
	);
}

float4 unpackUShort4N( uint2 packedValue )
{
	return float4(
		packedValue.x & 0xffff,
		( packedValue.x >> 16 ) & 0xffff,
		packedValue.y & 0xffff,
		( packedValue.y >> 16 ) & 0xffff
	) / 65535.0f;
}

void ExtractInputVertices( uint vidx, out VB_INPUT src, out VB_INPUT tar )
{
	src = (VB_INPUT)0;
	tar = (VB_INPUT)0;

	// Position
#ifdef USE_SKINNING
	uint4 origPosSkinSrc = SourceVB.Load4( Source_PosByteOffset + vidx*POS_STRIDE );
#else
	uint2 origPosSkinSrc = SourceVB.Load2( Source_PosByteOffset + vidx*POS_STRIDE );
#endif
	uint2 origPosSkinTar = TargetVB.Load2( Target_PosByteOffset + vidx*POS_STRIDE );
	src.Position = unpackUShort4N( origPosSkinSrc.xy );
	tar.Position = unpackUShort4N( origPosSkinTar );

#ifdef USE_SKINNING
	// Blend/Skin indices
	src.BlendIndices = origPosSkinSrc.z;
	src.BlendWeights = origPosSkinSrc.w;
#endif

	// Normal / Tangent
	uint2 origTbnSrc = SourceVB.Load2( Source_TbnByteOffset + vidx*TBN_STRIDE );
	uint2 origTbnTar = TargetVB.Load2( Target_TbnByteOffset + vidx*TBN_STRIDE );
	src.Normal = unpackDec4( origTbnSrc.x ).xyz;
	tar.Normal = unpackDec4( origTbnTar.x ).xyz;
	src.Tangent = unpackDec4( origTbnSrc.y );
	tar.Tangent = unpackDec4( origTbnTar.y );

#ifdef USE_CONTROL_TEX
	// Texcoord if needed
	uint origTex = SourceVB.Load( Source_TexByteOffset + vidx*TEX_STRIDE );
	uint2 unpackedOrigTex = uint2( origTex & 0xffff, ( origTex >> 16 ) & 0xffff );
	src.TexCoord = f16tof32( unpackedOrigTex );
#endif
}

#ifdef USE_CONTROL_TEX

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

#endif

#define THREADS_COUNT 256

[NUMTHREADS(THREADS_COUNT, 1, 1)]
void cs_main( uint3 DTid : SYS_DISPATCH_THREAD_ID )
{
	uint idx = DTid.x;

	if( idx < NumVertices )
	{
		VB_OUTPUT output;
		VB_INPUT inputSrc, inputTar;
		ExtractInputVertices( idx, inputSrc, inputTar );

		float blend = Weight;

	#ifdef USE_CONTROL_TEX
		float control = SYS_SAMPLE_LEVEL( ControlTexture, inputSrc.TexCoord, 0 ).x;
		blend = saturate( CalcBlending( blend, control ) );
	#endif

		// Uncompress potentially quantized positions
		float4 localPosition0 = float4( inputSrc.Position.xyz * Source_QS.xyz + Source_QB.xyz, 1 );
		float4 localPosition1 = float4( inputTar.Position.xyz * Target_QS.xyz + Target_QB.xyz, 1 );

		float4 position	= lerp( localPosition0,			localPosition1,			blend );
		// Re-compress position
		position		= ( position - Morph_QB ) / Morph_QS;

		float3 norm		= lerp( inputSrc.Normal,			inputTar.Normal,			blend );
		float4 tangent	= lerp( inputSrc.Tangent,			inputTar.Tangent,			blend );

		output.Position.xy	= packUShort4N( position );
	#ifdef USE_SKINNING
		output.Position.z	= inputSrc.BlendIndices;
		output.Position.w	= inputSrc.BlendWeights;
	#endif

		output.NormalTangent.x	= packDec4( float4( norm, 0 ) );
		output.NormalTangent.y	= packDec4( tangent );

		uint storeOffset = OutputByteOffset + idx * (POS_STRIDE+TBN_STRIDE);
	#ifdef USE_SKINNING
		StreamOut.Store4( storeOffset, output.Position );
	#else
		StreamOut.Store2( storeOffset, output.Position );
	#endif
		StreamOut.Store2( storeOffset + POS_STRIDE, output.NormalTangent );
	}
}