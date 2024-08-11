#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"

TextureCube sTexture		: register( t0 );
SamplerState sSampler		: register( s0 );

struct VS_INPUT
{
	float4 pos : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos   : SYS_POSITION;
	float3 norm  : TEXCOORD0;
	float4 wpos  : TEXCOORD1;	
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.wpos = mul( i.pos, VSC_LocalToWorld );
	o.pos  = mul( o.wpos, VSC_WorldToScreen );
	o.norm = i.pos.xyz;

	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	float3 sampleVec = i.norm;
	//sampleVec = reflect( i.wpos.xyz - PSC_CameraPosition.xyz, normalize( i.norm ) );
	//return SAMPLE_LEVEL( sTexture, sSampler, sampleVec, floor( fmod(i.pos / 700.0, 1.0) * 10 ) );
	//return SAMPLE_LEVEL( sTexture, sSampler, sampleVec, 0 );
	return sTexture.Sample( sSampler, sampleVec );	
}

#endif
