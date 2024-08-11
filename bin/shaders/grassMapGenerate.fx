#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"
#include "terrainUtilities.fx"

TEXTURE2D_ARRAY<uint>	tCMClipMap 		: register( t0 );
TEXTURE2D_ARRAY<float2>	tNormals		: register( t1 );

struct VS_OUTPUT
{
	float4 ScreenPos		   : SYS_POSITION;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( in uint VertexId : SYS_VERTEX_ID )
{
    float4 dstTextureMinMaxUV = float4( -1, 1, 1, -1 );

    VS_OUTPUT verts[4] = 
    {
        { float4( -1,  1, 0.5, 1.0) }, 
        { float4( -1, -1, 0.5, 1.0) },
        { float4(  1,  1, 0.5, 1.0) },
        { float4(  1, -1, 0.5, 1.0) }
    };

    return verts[VertexId];
}

#endif

#ifdef PIXELSHADER

float ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const int2 screenPos = (int2)i.ScreenPos.xy;

	// Sample first slice of the normalmap
	const float2 normalMapSample = tNormals.Load( int4( screenPos, 0, 0 ) );
	
	// Sample and decode first slice of the control map
	uint horizontalTexturesIndex;
	uint verticalTexturesIndex;
	uint slopeThresholdIndex;
	uint uvMultFlagsVertical;	
	{
		const uint cMapSample = tCMClipMap.Load( int4( screenPos, 0, 0 ) );
		DecodeControlMapValues( cMapSample, horizontalTexturesIndex, verticalTexturesIndex, slopeThresholdIndex, uvMultFlagsVertical );
	}

	// Get texture params
	const float2x4 texParamsHorizontal = TexParams[ horizontalTexturesIndex ];
	const float2x4 texParamsVertical = TexParams[ verticalTexturesIndex ];

	// Combine vertex normal
	const float slopeThresholds[8] = { 0.0f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 1.0f };
	float slopeThreshold = slopeThresholds[ slopeThresholdIndex ];
	float3 vertexNormal = DecompressPregenNormal( normalMapSample );
	float3 combinedVerticalNormal = vertexNormal;
	float slopiness = dot( vertexNormal, float3( 0, 0, 1 ) );
	float3 flattenedCombinedVerticalNormal = lerp( combinedVerticalNormal, float3( 0, 0, 1 ), slopiness );
	float3 biasedFlattenedCombinedVerticalNormal = normalize( lerp( combinedVerticalNormal, flattenedCombinedVerticalNormal, texParamsVertical[0].y ) );
	
	float flattenedNdotUp = clamp( dot( biasedFlattenedCombinedVerticalNormal, float3( 0.0f, 0.0f, 1.0f ) ), 0.0f, 1.0f );
	float slopeAngle = acos( flattenedNdotUp );
	float slopeTangent = tan( slopeAngle );

	const float lowSlopeThreshold = slopeThreshold;
	slopeThreshold = slopeThreshold + 0.2f;

	float fade = ( slopeTangent - lowSlopeThreshold ) / ( slopeThreshold - lowSlopeThreshold + 0.01f );
	fade = clamp( fade, 0.0f, 1.0f );

	// Compute slope tangent
	//const float verticalSurfaceTangent = ComputeSlopeTangent( biasedFlattenedCombinedVerticalNormal, slopeThreshold, saturate( slopeThreshold + texParamsHorizontal[0].x ) );	
	
	return fade;
}

#endif
