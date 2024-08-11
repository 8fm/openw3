#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"

Texture2D<float>		sShadowMap	: register( t0 );
SamplerState samp					: register( s0 );
TEXTURE2D_ARRAY<float>	sHeightMap	: register( t1 );
SamplerState samp2					: register( s1 );

struct VS_INPUT
{
	float4 pos : POSITION0;
	float4 color : COLOR0;
	float2 uv : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos : SYS_POSITION;
	float4 color : COLOR0;
	float2 uv : TEXCOORD0;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos = float4( float2(-1,-1) + i.pos.xy * float2(2,2), 0, 1 );
	o.uv = i.uv;
	o.color = float4( i.pos.xy, 0, 0 );

	return o;
}

#endif

#ifdef PIXELSHADER

bool IsOccluded( float3 WorldPos, float z )
{
	WorldPos.z = z;

	// calculate shadowmap coordinates
	float4 ShadowPos = mul( float4( WorldPos, 1), PSC_Custom_Matrix );
	const float2 ShadowUV = float2( 0.5f, 0.5f ) + ShadowPos.xy * float2( 0.5f, -0.5f );

	// sample shadowmap
	const float ShadowBias = 0.0001f;
	const float ShadowZ = SAMPLE_LEVEL( sShadowMap, samp, float3( ShadowUV, 0 ), 0 ).x + ShadowBias;

	// is occluded
	return (ShadowZ < ShadowPos.z);
}


#pragma PSSL_target_output_format(default FMT_32_ABGR)

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	// sample the height
	const float ClipMapLevel = PSC_Custom_2.x;	 
	float2 uv = i.uv;
	//uv = (floor( uv * 1024.0f ) + 0.5f) / 1024.0f;
	float normZ = SAMPLE_LEVEL( sHeightMap, samp2, float3( uv, ClipMapLevel ), 0 );

	// calculate world space position
	const float3 WorldOffset = PSC_Custom_0.xyz;
	const float3 WorldScale = PSC_Custom_1.xyz;
	const float3 WorldPos = WorldOffset + float3( i.color.xy, normZ ) * WorldScale;

	float OccludedHeight = WorldPos.z;
	[branch]
	if ( IsOccluded(WorldPos, WorldPos.z) )
	{
		float BaseZ = WorldPos.z;
		float StepZ = 256.0f; // POWER OF TWO PLZ!

		[unroll]
		for ( int i=0; i<13; ++i )
		{
			if ( IsOccluded( WorldPos, BaseZ + StepZ ) )
			{
				BaseZ = BaseZ + StepZ;
				StepZ *= 0.5f;
			}
			else
			{
				StepZ *= 0.5f;
			}
		}

		OccludedHeight = BaseZ;  
	}

	// save the terrain height (as reference and the highest occluded height).
	// rescale to [0,1] range
	return ( OccludedHeight.xxxx - WorldOffset.zzzz ) / WorldScale.zzzz;
}

#endif
