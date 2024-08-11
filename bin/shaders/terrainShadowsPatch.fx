#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"

TEXTURE2D_ARRAY<float>	sHeightMap	: register( t0 );
SamplerState samp					: register( s0 );

struct VS_INPUT
{
	float2 pos : POSITION0;
	float2 uv : TEXCOORD0;

	float4 Data0 : EXTRA_DATA0;
	float4 Data1 : EXTRA_DATA1;
	float4 Data2 : EXTRA_DATA2;
};

struct VS_OUTPUT
{
	float4 pos : SYS_POSITION;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	const float3 WorldOffset = i.Data0.xyz;
	const float3 WorldScale = i.Data1.xyz;

	const float ClipMapLevel = i.Data1.w;
	const float2 UVOffset = i.Data2.xy;
	const float2 UVScale = i.Data2.zw;

	float2 uv = i.uv * UVScale + UVOffset;
	float z = SAMPLE_LEVEL( sHeightMap, samp, float3( uv, ClipMapLevel ), 0 );

	float3 worldPos = WorldOffset + float3( i.pos, z ) * WorldScale;

	o.pos = mul( float4( worldPos, 1 ), VSC_WorldToScreen );
	return o;
}

#endif

#ifdef PIXELSHADER

void ps_main( VS_OUTPUT i )
{
	// empty (we have null rendertarget anyway, so this way we get rid of debugDevice warning)
}

#endif
