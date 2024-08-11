cbuffer CameraShaderConsts : register(b0)
{
	row_major float4x4	VSC_Local;
	row_major float4x4	VSC_WorldToView;
	row_major float4x4	VSC_ViewToScreen;
	float4				Color;
};

RWBuffer<uint> IndirectArgs : register(u0);
RWByteAddressBuffer InputVB : register(u1);
RWByteAddressBuffer InputIB : register(u2);
RWByteAddressBuffer IBufferOut : register(u3);

bool IsPointInsideRectangle( float4 v )
{
	const float w = v.w;
	return ( v.x < w && v.x > -w && v.y < w && v.y > -w );
}

bool LineRectangleIntersect( float4 v0, float4 v1 ) 
{
	if( IsPointInsideRectangle(v0) || IsPointInsideRectangle(v1) )
		return true;

	v0 /= v0.w;
	v1 /= v1.w;

	float a = (v1.y-v0.y) / (v1.x-v0.x);
	float b = v0.y - a*v0.x;

	float x_top = (1.0f-b)/a;
	float x_bottom = -(1.0f+b)/a;

	float y_min = min(v0.y, v1.y);
	float y_max = max(v0.y, v1.y);

	if( abs(x_top) < 1.0f )
	{
		if( y_min < 1.0f && y_max > 1.0f )
			return true;
	}

	if( abs(x_bottom) < 1.0f )
	{
		if( y_min < -1.0f && y_max > -1.0f )
			return true;
	}

	float y_left = -a + b;
	float y_right = a + b;

	float x_min = min(v0.x, v1.x);
	float x_max = min(v0.x, v1.x);

	if( abs(y_right) < 1.0f )
	{
		if( x_min < 1.0f && x_max > 1.0f )
			return true;
	}

	if( abs(y_left) < 1.0f )
	{
		if( x_min < -1.0f && x_max > -1.0f )
			return true;
	}

	return false;
}

[numthreads(512, 1, 1)]
void cs_main( uint3 DTid : SV_DispatchThreadID )
{
	if (DTid.x == 0 && DTid.y == 0 && DTid.z == 0)
    {
        IndirectArgs[0] = 3;
        IndirectArgs[1] = 0;
        IndirectArgs[2] = 0;
        IndirectArgs[3] = 0;
    }

	uint2 tmp = asuint(InputIB.Load2(DTid.x * 6));
	int3 i = int3( tmp.x & 0xffff, tmp.x >> 16, tmp.y & 0xffff );
	
	uint originalVal;
	InterlockedAdd(IndirectArgs[1], 1, originalVal);

	IBufferOut.Store3(DTid.x * 12, i);

	// Get vertices
	//float3 v0 = asfloat(InputVB.Load3(i.x*12));
	//float3 v1 = asfloat(InputVB.Load3(i.y*12));
	//float3 v2 = asfloat(InputVB.Load3(i.z*12));

	//// Calculate position in view space
	//float4 viewV0 = mul( mul( float4(v0,1), VSC_Local ), VSC_WorldToView );
	//float4 viewV1 = mul( mul( float4(v1,1), VSC_Local ), VSC_WorldToView );
	//float4 viewV2 = mul( mul( float4(v2,1), VSC_Local ), VSC_WorldToView );

	//// Back face culling
	//float4 dx = viewV1 - viewV0;
	//float4 dy = viewV2 - viewV0;
	//float3 N = cross(dx.xyz, dy.xyz);

	//if( dot( -viewV0.xyz, N ) < 0 )
	//{
	//	// Calculate position in screen space
	//	float4 s0 = mul( viewV0, VSC_ViewToScreen );
	//	float4 s1 = mul( viewV1, VSC_ViewToScreen );
	//	float4 s2 = mul( viewV2, VSC_ViewToScreen );

	//	// Intersects the screen
	//	if( LineRectangleIntersect(s0, s1) || 
	//		LineRectangleIntersect(s1, s2) || 
	//		LineRectangleIntersect(s2, s0) )
	//	{
	//		uint originalVal;
	//		InterlockedAdd(IndirectArgs[1], 1, originalVal);

	//		IBufferOut.Store3(DTid.x * 12, i);
	//	}

	//	// TODO?: Clipping - Cohen-Shutterland	
	//}
}