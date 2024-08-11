cbuffer CameraShaderConsts : register(b0)
{
	row_major float4x4  VSC_Local;
	row_major float4x4	VSC_WorldToView;
	row_major float4x4	VSC_ViewToScreen;
};

struct VSInput
{
	float4 pos : POSITION;
	float4 color : COLOR;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float4 col : COLOR;
};

struct PSOutput
{
	float4 color0 : SV_TARGET0;
	float4 color1 : SV_TARGET1;
	float4 color2 : SV_TARGET2;
	float4 color3 : SV_TARGET3;
};

PSInput vs_main(VSInput i)
{
	PSInput o;
	o.pos = mul( mul( mul( i.pos, VSC_Local ), VSC_WorldToView ), VSC_ViewToScreen );
	o.col = i.color;

	return o;
}

PSOutput ps_main(PSInput i)
{
	PSOutput o;
	o.color0 = i.col;
	o.color1 = i.col;
	o.color2 = i.col;
	o.color3 = i.col;
	return o;
}
