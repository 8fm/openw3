cbuffer globalvs
{
    row_major float4x4 viewMatrix;
	row_major float4x4 projMatrix;
	row_major float4x4 worldMatrix;
	float uvScale;
};


SamplerState colorSamplerState : register( s0 );
Texture2D colorSampler : register( t0 );

struct VS_INPUT
{
	float3 Position		: POSITION0;
	float4 Color		: COLOR0;
	float2 UV			: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float2 UV			: TEXCOORD1;
};

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT Output;

	matrix worldView = mul(worldMatrix, viewMatrix);
	float4 viewPos = float4(i.Position, 1.0f);
	viewPos = mul(viewPos, worldView);

	Output.Position = mul(viewPos, projMatrix);
	Output.UV = i.UV * uvScale;

	return Output;
}

float4 ps_main( VS_OUTPUT Input ) : SV_TARGET
{
	return colorSampler.Sample( colorSamplerState, Input.UV );
}
