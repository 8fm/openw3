cbuffer cb0 : register( b0 )
{
	row_major float4x4 mWorldViewProj;
	float fTime;
};


cbuffer cb1 : register( b1 )
{
	float4 objectPos;
};

struct VS_INPUT
{
	float3 Position   : POSITION;
};

struct VS_OUTPUT
{
	float4 Position   : SV_Position;
	float4 Diffuse    : COLOR0;
};

VS_OUTPUT vs_main( VS_INPUT input )
{
	VS_OUTPUT Output;

	float fSin, fCos;
	float x = length( input.Position ) * sin( fTime ) * 15.0f;

	sincos( x, fSin, fCos );

	float4 OutPos = mul( float4( input.Position.x, fSin * 0.1f, input.Position.y, 0.0f ) + float4(objectPos.xyz,1), mWorldViewProj );

	Output.Position = OutPos;
	Output.Diffuse = 0.5f - 0.5f * fCos;

	return Output;
}

float4 ps_main( VS_OUTPUT input ) : SV_Target
{
	return input.Diffuse;
}
