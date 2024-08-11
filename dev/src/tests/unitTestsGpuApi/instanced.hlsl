cbuffer globalvs
{
    row_major float4x4 viewMatrix;
	row_major float4x4 projMatrix;
	float4 lightPos;
};


struct VSInput
{
	float3  pos 			: POSITION;
	row_major float4x4 worldMatrix	: INSTANCE_TRANSFORM;
	float4   Color			: COLOR;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float4 surfaceColor : TEXCOORD2;
};

PSInput vs_main(VSInput i)
{
	PSInput o = (PSInput)0;
	matrix worldView = mul(i.worldMatrix, viewMatrix);
	float4 viewPos = float4(i.pos, 1.0f);
	viewPos = mul(viewPos, worldView);
	o.pos = mul(viewPos, projMatrix);

	o.surfaceColor = i.Color;
	return o;
}

float4 ps_main(PSInput i) : SV_TARGET
{
	return i.surfaceColor;
}
