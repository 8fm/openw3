#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"

struct VS_INPUT
{
	float4 pos : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos : SYS_POSITION;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos = mul( mul( i.pos, VSC_LocalToWorld ), VSC_WorldToScreen );

	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	return PSC_ConstColor;
}

#endif
