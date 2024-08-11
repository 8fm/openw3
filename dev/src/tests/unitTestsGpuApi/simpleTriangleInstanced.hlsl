cbuffer globalvs
{
    float4 colors[3];
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float4 col : COLOR;
};

PSInput vs_main( uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID )
{
	PSInput o;
	float4 verts[3] = 
    {
        { float4( -0.1f, -0.1f, 0, 1 ) }, 
		{ float4(  0.0f,  0.1f, 0, 1 ) },
        { float4(  0.1f, -0.1f, 0, 1 ) },
    };

	o.pos = float4( -0.5f + verts[vertexId].x + ((instanceId % 8) / 6.0f), -0.5f + verts[vertexId].y + ((instanceId / 8) / 6.0f), 0, 1);
	o.col = colors[vertexId];

	return o;
}

float4 ps_main(PSInput i) : SV_TARGET
{
	return i.col;
}
