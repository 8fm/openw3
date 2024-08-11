cbuffer globalvs
{
    float4 colors[3];
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
};

PSInput vs_main(uint vertexId : SV_VertexID)
{
	PSInput verts[3] = 
    {
        { float4( -0.5f, -0.5f, 0, 1 ), colors[0] }, 
		{ float4(  0.0f,  0.5f, 0, 1 ), colors[1] },
        { float4(  0.5f, -0.5f, 0, 1 ), colors[2] },
    };

	return verts[vertexId];
}

float4 ps_main(PSInput i) : SV_TARGET
{
	return i.col;
}
