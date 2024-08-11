#include "commonVS.fx"


struct VS_INPUT
{
	float4	Position		: POSITION0;
#ifdef USE_SKINNING
	uint4	BlendIndices	: BLENDINDICES0;
#ifndef SINGLE_BONE
	float4	BlendWeights	: BLENDWEIGHT0;
#endif
#endif
	float3	Normal			: NORMAL0;
};

struct VS_OUTPUT
{
	uint2	PackedData		: TEXCOORD0;
};

#ifdef VERTEXSHADER


#define WorldToDecal		VSC_Custom_Matrix
#define NormalFadeBias		(VSC_Custom_0.x)
#define NormalFadeScale		(VSC_Custom_0.y)
#define DepthFadePower		(VSC_Custom_0.z)
#define DepthFadeOrigin		(VSC_Custom_0.w)
#define TwoSided			(VSC_Custom_1.x != 0.0f)
#define Ellipsoidal			(VSC_Custom_1.y != 0.0f)
#define DecalDirection		(VSC_Custom_2.xyz)


// Limit for fade values. When we drop to float16's, large fades can become INF. We don't saturate though, because we
// do want to allow values outside the range, to get proper interpolation. Limiting to something that's still quite a
// bit above 1 will reduce the artifacts that may occur, so it shouldn't be noticeable.
#define MAX_FADE 100.0f



uint2 Float4_to_Half4( float4 src )
{
	uint4 asHalf = f32tof16( src );
	return uint2( asHalf.x | ( asHalf.y << 16 ), asHalf.z | ( asHalf.w << 16 ) );
}


VS_OUTPUT vs_main( in VS_INPUT input )
{
	float4 localPosition = float4( input.Position.xyz * VSC_QS.xyz + VSC_QB.xyz, 1 );

#ifdef USE_SKINNING
	float4x4 SkinningMatrix = (float4x4) 0;
#ifdef SINGLE_BONE
    float wet = 0.0f;
	float4 skinnedPosition = ApplySkinning( localPosition, float4(1,0,0,0), input.BlendIndices, VSC_SkinningData, SkinningMatrix, wet );
#else
    float wet = 0.0f;
	float4 skinnedPosition = ApplySkinning( localPosition, input.BlendWeights, input.BlendIndices, VSC_SkinningData, SkinningMatrix, wet );
#endif
	float4x4 skinnedLocalToWorld = mul( SkinningMatrix, VSC_LocalToWorld );
#else
	float4 skinnedPosition = localPosition;
	float4x4 skinnedLocalToWorld = VSC_LocalToWorld;
#endif

	float3 positionInDecal = mul( skinnedPosition, mul( VSC_LocalToWorld, WorldToDecal ) ).xyz;
	positionInDecal.y = positionInDecal.y * 0.5 + 0.5;


	float2 texCoord = positionInDecal.xz;
	if ( Ellipsoidal )
	{
		// Scale down towards ends. When scale reaches 0 or negative, then we just leave the texcoord at the middle of the
		// decal.
		const float scale = sin( positionInDecal.y * 3.14159265 );
		texCoord *= scale > 0 ? ( 1.0f / scale ) : 0.0f;
	}
	texCoord = texCoord * 0.5 + 0.5;

	// Fade out according to distance
	float distOnZ;
	if ( positionInDecal.y < DepthFadeOrigin )
	{
		// If the origin is on the edge or outside the decal, then we don't really have a well-defined way to fade out beyond
		// it. So, just say we're always at the max
		distOnZ = DepthFadeOrigin > 0 ? ( 1 - positionInDecal.y / DepthFadeOrigin ) : 1;
	}
	else
	{
		// Same here, if origin is on the other edge or outside.
		distOnZ = DepthFadeOrigin < 1 ? ( ( positionInDecal.y - DepthFadeOrigin ) / ( 1 - DepthFadeOrigin ) ) : 1;
	}
	// If distance is outside the decal, we'd end up with pow(0, DepthFadePower), which is especially bad if power is negative.
	float fade = distOnZ >= 1 ? 0 : pow( saturate( 1 - distOnZ ), DepthFadePower );

	// Fade out according to normal
	float3 worldNormal = normalize( mul( DecompressNormal( input.Normal ), (float3x3)skinnedLocalToWorld ) );
	float normalZ = dot( -worldNormal, DecalDirection );

	if ( TwoSided )
	{
		normalZ = abs( normalZ );
	}
	fade *= saturate( ( normalZ + NormalFadeBias ) * NormalFadeScale );

	// Clamp output values to a sane range. If they get too big, they may end up INF or QNAN when dropping to float16
	const float4 maxValues = { MAX_FADE, MAX_FADE, MAX_FADE, MAX_FADE };

	VS_OUTPUT output;
	output.PackedData = Float4_to_Half4( clamp( float4( texCoord.xy, positionInDecal.y, fade ), -maxValues, maxValues ) );

	return output;
}


[MAX_VERTEX_COUNT(1)]
void gs_main( GS_INPUT_POINT VS_OUTPUT In[1], inout GS_BUFFER_POINT<VS_OUTPUT> triStream )
{
	// simple pass-thru
	triStream.Append( In[0] );
}

#endif
