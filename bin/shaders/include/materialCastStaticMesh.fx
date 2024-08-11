struct VertexBufferData
{
	float2	m_position;			// UV.xy * TargetTexSize.xy
	float2	m_uv0;				// First uv set
	float4	m_normal;			// Vertex normal
	float4	m_tangent;			// Vertex tangent
	float4  m_color;
	float2  m_uv1;
	float2  m_pad;
};

STRUCTBUFFER(VertexBufferData)	g_vertexBuffer		: register( t0 );

VS_INPUT ExtractVertexStream( uint vId )
{
	uint vIdDiv = vId / 3;
	uint vIdMod = vId % 3;
	VertexBufferData v = g_vertexBuffer[vIdDiv];
	
	float2 pos = v.m_position + float2(0,1);
	// Transform pos to clip space 
	float2 texHalfSize = VSC_Custom_0.xy / 2.0f;
	pos /= texHalfSize;
	pos += float2(-1,-1);
	
	const float pixelSizeInClipSpace = 2.0f / VSC_Custom_0.x;
	const float halfPixelSize = pixelSizeInClipSpace / 2.0f;
	const float EPS = halfPixelSize / 3.0f;

	if( vIdMod == 0 )
	{
		pos += float2(halfPixelSize,-EPS);
	}
	else if( vIdMod == 1 )
	{
		pos += float2(EPS,-pixelSizeInClipSpace+EPS);
	}
	else if( vIdMod == 2 )
	{
		pos += float2(pixelSizeInClipSpace-EPS,-pixelSizeInClipSpace+EPS);
	}
	pos.y *= -1.0f; // flip y

	VS_INPUT In;
	In.Position = float3(pos,0);
	In.UV = v.m_uv0;
	In.Normal = v.m_normal.xyz;
	In.Tangent = v.m_tangent;
#ifdef MS_HAS_EXTRA_STREAMS
	In.UV2 = v.m_uv1;
	In.Color = v.m_color;
#endif

	return In;
}