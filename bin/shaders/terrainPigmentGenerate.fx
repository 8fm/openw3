#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"
#include "terrainUtilities.fx"


TEXTURE2D_ARRAY<uint>	tCMClipMap 		: register( t0 );
TEXTURE2D_ARRAY<float2>	tNormals		: register( t1 );
TEXTURE2D_ARRAY<float4>	tColorMap		: register( t2 );
TEXTURE2D_ARRAY<float4>	tDiffuseArray	: register( t3 );

SamplerState		sPointClampNoMip	: register( s0 );
SamplerState		sLinearClampNoMip	: register( s1 );
SamplerState		sPointWrapMip		: register( s2 );

#define vWindowRect			PSC_Custom_0
#define vColorWindowRect	PSC_Custom_1
#define	vClipIndices		PSC_Custom_2
#define	vViewport			PSC_Custom_3

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

float4 SampleMaterialTexture( int texIndex )
{
	const float2 uv = 0.5;
	const float mipIndex = 99; //< big value to force last mipmap
	return SAMPLE_LEVEL( tDiffuseArray, sPointWrapMip, float3( uv, texIndex ), mipIndex );
}

float2 RecalculateUVInLargerRect( float2 uv, float4 smallerRect, float4 largerRect )
{
	float2 worldSpace = smallerRect.xy + float2( smallerRect.zw - smallerRect.xy ) * uv.xy;
	return abs(( worldSpace.xy - largerRect.xy ) / ( largerRect.zw - largerRect.xy ));
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	float2 vpos = i.pos.xy;
	float2 coord = (vpos - vViewport.xy + 0.5) / vViewport.zw;

	const int cmLevelIndex = (int)vClipIndices.x;
	const int colorLevelIndex = (int)vClipIndices.y;
	const float2 regionUVMain = coord;
	const float2 regionUVColorMap = RecalculateUVInLargerRect( coord, vWindowRect, vColorWindowRect );

	// Sample colorMap
	const float4 colorMapSample = tColorMap.Sample( sLinearClampNoMip, float3( regionUVColorMap.xy, colorLevelIndex ), int2(0,0) );

	// Sample normalmap
	const float2 normalMapSample = tNormals.Sample( sLinearClampNoMip, float3( regionUVMain.xy, cmLevelIndex ), int2(0,0) );

	// Decode control map
	uint horizontalTexturesIndex;
	uint verticalTexturesIndex;
	uint slopeThresholdIndex;
	uint uvMultFlagsVertical;	
	{
		int2 cmTexelCoords = 0;
		{
			float3 clipmapDim;
			tCMClipMap.GetDimensions( clipmapDim.x, clipmapDim.y, clipmapDim.z );
			cmTexelCoords = regionUVMain * clipmapDim.xy - 0.5;
		}	
			
		uint controlMapValue = tCMClipMap.Load( int4( cmTexelCoords, cmLevelIndex, 0 ) ).x;
		DecodeControlMapValues( controlMapValue, horizontalTexturesIndex, verticalTexturesIndex, slopeThresholdIndex, uvMultFlagsVertical );
	}

	// Sample colors
	const float4 horizontalColor = SampleMaterialTexture( horizontalTexturesIndex );
	const float4 verticalColor = SampleMaterialTexture( verticalTexturesIndex );

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
	
	// Compute slope tangent
	float verticalSurfaceTangent = ComputeSlopeTangent( biasedFlattenedCombinedVerticalNormal, slopeThreshold, saturate( slopeThreshold + texParamsHorizontal[0].x ) );	

	// Build final colors
	float3 finalColor = lerp( horizontalColor.xyz, verticalColor.xyz, verticalSurfaceTangent );
	float3 resultColor = ApplyColorMap( finalColor.xyz, colorMapSample.xyz );

	// desaturate a bit for better visual effect
	{
		const float desaturation = 0.2;
		resultColor = lerp( resultColor, dot( float3( 0.3, 0.5, 0.2 ), resultColor ).xxx, desaturation );
	}
		
	//resultColor.xyz = colorMapSample.xyz;
	//resultColor.xyz = pow( vertexNormal.xyz * 0.5 + 0.5, 1.0 / 2.2 );
	//resultColor.xyz = colorMapSample.xyz;
	//resultColor.xyz = horizontalColor.xyz;
	//resultColor.xyz = verticalColor.xyz;
	
	return float4 ( resultColor, 1 );
}

#endif
