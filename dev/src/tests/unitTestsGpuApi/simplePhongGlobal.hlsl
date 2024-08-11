cbuffer globalvs : register( b0 )
{
	row_major float4x4 viewMatrix;
	row_major float4x4 projMatrix;
	float4 lightPos;
};


cbuffer customvs : register( b1 )
{
	row_major float4x4 worldMatrix;
}

cbuffer customps : register( b0 )
{
	float4 surfaceColor;
}

struct VSInput
{
	float3 pos : POSITION;
	float3 norm : NORMAL0;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float3 viewVec : TEXCOORD0;
	float3 lightVec0 : TEXCOORD1;
};

PSInput vs_main(VSInput i)
{
	PSInput o = (PSInput)0;
	matrix worldView = mul(worldMatrix, viewMatrix);
	float4 viewPos = float4(i.pos, 1.0f);
	viewPos = mul(viewPos, worldView);
	o.pos = mul(viewPos, projMatrix);

	o.norm = mul(float4(i.norm, 0.0f), worldMatrix).xyz;
	o.norm = normalize(o.norm);
	
	o.viewVec = normalize(-viewPos.xyz);
	o.lightVec0 = normalize((mul(lightPos, viewMatrix) - viewPos).xyz);
	return o;
}

static const float3 ambientColor = float3(0.2f, 0.2f, 0.2f);
static const float3 lightColor = float3(1.0f, 1.0f, 1.0f);
static const float3 kd = 0.5, ks = 0.5f, m = 100.0f;

float4 ps_main(PSInput i) : SV_TARGET
{
	float3 viewVec = normalize(i.viewVec);
	float3 normal = normalize(i.norm);
	float3 lightVec = normalize(i.lightVec0);
	float3 halfVec = normalize(viewVec + lightVec);
	float3 color = surfaceColor * ambientColor;
	color += lightColor * surfaceColor * kd * saturate(dot(normal, lightVec)) +
			 lightColor * ks * pow(saturate(dot(normal, halfVec)), m);

	return float4(saturate(2.0f*color), surfaceColor.a);
}
