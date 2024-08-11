cbuffer CameraShaderConsts : register(b0)
{
	row_major float4x4  VSC_Local;
	row_major float4x4	VSC_WorldToView;
	row_major float4x4	VSC_ViewToScreen;
	float4				Color;
};

ByteAddressBuffer AppendBuffer : register(t0);
ByteAddressBuffer InputVB : register(t1);

struct VSInput
{
	float4 pos : POSITION;
	uint VertexId   : SV_VertexID;
	uint InstanceId : SV_InstanceID;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float4 col : COLOR;
};

PSInput vs_main(VSInput input)
{
	PSInput o;

	// indices of the on-screen triangles
	int3 i = asint(AppendBuffer.Load3(input.InstanceId * 12));

	float3 pos;
	if( input.VertexId == 0 )
	{
		pos = asfloat(InputVB.Load3(i.x*12));
	}
	else if( input.VertexId == 1 )
	{
		pos = asfloat(InputVB.Load3(i.y*12));
	}
	else if( input.VertexId == 2 )
	{
		pos = asfloat(InputVB.Load3(i.z*12));
	}

	o.pos = mul( mul( mul( float4(pos, 1), VSC_Local ), VSC_WorldToView ), VSC_ViewToScreen );
	o.col = Color;

	return o;
}

float4 ps_main(PSInput i) : SV_TARGET
{
	return i.col;
}
