#include "common.fx"

struct VSInput
{
	float4 pos : POSITION;
};

struct PSInput
{
	float4 pos : SYS_POSITION;
};

#ifdef VERTEXSHADER

#include "commonVS.fx"

PSInput vs_main(VSInput input)
{
	PSInput o;

	float4 pos = float4(input.pos.xyz * VSC_QS.xyz + VSC_QB.xyz, 1.0f);
	o.pos = mul( mul( pos, VSC_LocalToWorld), VSC_WorldToScreen);

	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main(PSInput input) : SYS_TARGET_OUTPUT0
{
	return 0;
}

#endif
