#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"

SamplerState	s_Texture	: register( s0 );
Texture2D		t_Texture	: register( t0 );

#if ENABLE_EXPONENT
    #define fExponent PSC_Custom_0
#endif

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

	o.pos = mul( mul( i.pos, VSC_LocalToWorld ), VSC_WorldToScreen );
	o.uv = i.uv;
#if XENON	
	o.color = i.color.wxyz;
#else
	o.color = i.color;
#endif	

	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
    float4 result = i.color * t_Texture.Sample( s_Texture, i.uv ).wwww;
#if ENABLE_EXPONENT
    result.xyz    = pow( abs(result.xyz), fExponent.xxx );
#endif
	return result;
}

#endif
