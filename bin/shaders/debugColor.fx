#include "common.fx"
#include "globalConstantsVS.fx"

struct VS_INPUT
{
	float4 pos : POSITION0;
	float4 color : COLOR0;
};

struct VS_OUTPUT
{
	float4 pos : SYS_POSITION;
	float4 color : COLOR0;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos = mul( mul( i.pos, VSC_LocalToWorld ), VSC_WorldToScreen );
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
	return i.color;
}

#endif
