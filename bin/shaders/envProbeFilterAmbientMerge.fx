#include "postfx_common.fx"


Texture2D		t_TextureColor		: register( t0 );
Texture2D		t_TextureSkyFactor	: register( t1 );

#define	vAmbient					PSC_Custom_0
#define vSceneAdd					PSC_Custom_1
#define vSkyColorTop				PSC_Custom_2
#define vSkyColorHorizon			PSC_Custom_3
#define fSkyShape					PSC_Custom_4.x
#define iSourceMipIndex				((int)PSC_Custom_4.y)
#define iSegmentSize				((int)PSC_Custom_4.z)
#define iExtent						((int)PSC_Custom_4.w)
#define DEFINE_FINAL_COLOR_TABLE	float4 ambientFinalColorTable[CUBE_ARRAY_CAPACITY] = { PSC_Custom_5, PSC_Custom_6, PSC_Custom_7, PSC_Custom_8, PSC_Custom_9, PSC_Custom_10, PSC_Custom_11 }
#define tFinalColor					ambientFinalColorTable

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos			: SYS_POSITION;
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
float4 ps_main( float4 vpos : SYS_POSITION ) : SYS_TARGET_OUTPUT0
{
	uint2 pixelCoord = vpos.xy;
	const float3 sceneColor = SAMPLE_MIPMAPS( t_TextureColor, iSourceMipIndex, pixelCoord ).xyz;
	const float skyFactor = SAMPLE_MIPMAPS( t_TextureSkyFactor, iSourceMipIndex, pixelCoord ).x;
	const int2 segmentIndex = pixelCoord.xy / iSegmentSize;
	
	float3 skyColor = 0;
	{
		const float extent = iExtent + 0.5;

		float shapeFactor = 0;
		const int range = 3;
		[unroll]
		for ( int i=0; i<range; ++i )
		{
			[unroll]
			for ( int j=0; j<range; ++j )
			{
				const float2 segmentCoord = (pixelCoord % iSegmentSize) + ((float2( i, j ) + 0.5) / range - 0.5) * 1.5 * (iSourceMipIndex + 1);
				float horizonFactor = length( (((segmentCoord - extent) + 0.5) / (iSegmentSize - 2 * extent) * 2 - 1) );
				//horizonFactor = 1 - abs(1 - horizonFactor);
				horizonFactor = saturate( horizonFactor );
				float currShapeFactor = pow( horizonFactor, fSkyShape );
				shapeFactor += currShapeFactor;
			}
		}
		shapeFactor /= range * range;

		skyColor = lerp( vSkyColorTop.xyz, vSkyColorHorizon.xyz, shapeFactor );
	}

	DEFINE_FINAL_COLOR_TABLE;
	float4 finalColorValue = tFinalColor[segmentIndex.y];
	
	float3 result = sceneColor * lerp( vAmbient.xyz + (1 - skyFactor) * vSceneAdd.xyz, 1, finalColorValue.w );
	result += skyColor * skyFactor;
	result *= finalColorValue.xyz;	
	
	return float4 ( result.xyz, 1 );
}
#endif
