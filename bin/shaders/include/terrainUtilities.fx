#include "common.fx"

#pragma pack_matrix(row_major)
START_CB( TerrainPixelConsts, 6 )
	float2x4 TexParams[ 31 ];
	 // x1 -blend sharpness; y1 - lower threshold on horizontal surface; z1 - weaken normal on horizontal surface, w1 - fallof
	 // x2 -specularity; y2 - glossiness; z2 - fallof (same as w1), w2 - specular contrast
END_CB

float3 DecompressPregenNormal( float2 pregenNormal )
{
	return float3 ( pregenNormal.xy, sqrt( 1 - dot( pregenNormal.xy, pregenNormal.xy ) ) );
}

float LinearStep( float min, float max, float val )
{
	return saturate( ( val - min ) / ( max - min ) );
}

float ComputeSlopeTangent( float3 normal, float lowThreshold, float highThreshold )
{
	float NdotUp = saturate( dot( normal, float3( 0.0f, 0.0f, 1.0f ) ) );
	float x = acos( NdotUp );

	// Taylor expansion of tangent function (PSSL did some sick-weird artefacts when using tan(x))
	float x3 = x*x*x;
	float x5 = x3 * x * x;
	float t = x + x3/3.0f + 2.0f*x5/15.0f;
	
	return LinearStep( lowThreshold, highThreshold, saturate( t ) );
}

void DecodeControlMapValues( uint values, out uint horizontalTextureIndex, out uint verticalTextureIndex, out uint slopeThresholds, out uint uvMultVertical )
{
	 horizontalTextureIndex = ( values & 31 ) - 1;
	 verticalTextureIndex = ( ( values >> 5 ) & 31 ) - 1;
	 slopeThresholds = ( ( values >> 10 ) & 7 );
	 uvMultVertical = ( ( values >> 13 ) & 7 );
}

void DecodeControlMapValues( uint4 values, out uint4 horizontalTextureIndex, out uint4 verticalTextureIndex, out uint4 slopeThresholds, out uint4 uvMultVertical )
{
	 horizontalTextureIndex = ( values & 31 ) - 1;
	 verticalTextureIndex = ( ( values >> 5 ) & 31 ) - 1;
	 slopeThresholds = ( ( values >> 10 ) & 7 );
	 uvMultVertical = ( ( values >> 13 ) & 7 );
}

// Overlay blending for color map. Values below 0.5 will darken the result, values above 0.5 with lighten.	
float3 ApplyColorMap( float3 finalColor, float3 colorMap )
{
	return ( colorMap.xyz < 0.5f ) ? ( 2.0f * finalColor.xyz * colorMap.xyz ) : ( 1.0f - 2.0f * ( 1.0f - finalColor.xyz ) * ( 1.0f - colorMap.xyz ) );
}