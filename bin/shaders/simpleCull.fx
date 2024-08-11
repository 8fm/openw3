#include "common.fx"

START_CB( CameraShaderConsts, 3 )
	float4x4	CSC_LocalToWorld;
	float4x4	CSC_WolrdToScreen;
	float4		CSC_QS;
	float4		CSC_QB;
	float4		CSC_Count;
END_CB

// Input
DATABUFFER(float4) InputVB : register(t0);
DATABUFFER(uint)  InputIB : register(t1);

// Output
RW_DATABUFFER(uint) IndirectArgs : register(u0);
RW_DATABUFFER(uint)  IBufferOut : register(u1);

bool IsPointInsideRectangle( float4 v )
{
	return abs( v.xy ) < v.w;
}

//[UNUSED ATM]
bool LineRectangleIntersect( float4 v0i, float4 v1i ) 
{
	float4 v0 = v0i / v0i.w;
	float4 v1 = v1i / v1i.w;

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

#define THREADS_COUNT 1024

#define NUM_TRIANGLES (int)CSC_Count.x
#define INDEX_OFFSET  (int)CSC_Count.y
#define VERTEX_OFFSET (int)CSC_Count.z
#define INDEX_START	  (int)CSC_QS.w

[NUMTHREADS(THREADS_COUNT, 1, 1)]
void cs_main( uint3 DTid : SYS_DISPATCH_THREAD_ID, uint3 Gid : SYS_GROUP_ID, uint3 GTid : SYS_GROUP_THREAD_ID )
{	
	uint idx = Gid.x * THREADS_COUNT + GTid.x;

	if( idx == 0 && INDEX_START == 0 )
	{
		// Zero out indirect arg counter
		IndirectArgs[0] = 0;
	}

	GROUP_BARRIER_GROUP_SYNC;

	if( idx < NUM_TRIANGLES )
	{
		uint offset = INDEX_OFFSET+INDEX_START+3*idx;
		uint3 i = uint3( InputIB[offset], InputIB[offset+1], InputIB[offset+2] );
		uint3 iOff = i + VERTEX_OFFSET;

		float4 v0U = InputVB[iOff.x];
		float4 v1U = InputVB[iOff.y];
		float4 v2U = InputVB[iOff.z];

		float4 v0 = float4(v0U.xyz * CSC_QS.xyz + CSC_QB.xyz, 1.0f);
		float4 v1 = float4(v1U.xyz * CSC_QS.xyz + CSC_QB.xyz, 1.0f);
		float4 v2 = float4(v2U.xyz * CSC_QS.xyz + CSC_QB.xyz, 1.0f);

		{
			float4x4 LocalToScreen = mul( CSC_LocalToWorld, CSC_WolrdToScreen );

			float4 s0 = mul( v0, LocalToScreen );
			float4 s1 = mul( v1, LocalToScreen );
			float4 s2 = mul( v2, LocalToScreen );

		    // Is at least one triangle point inside the screen
			if( IsPointInsideRectangle(s0) || IsPointInsideRectangle(s1) || IsPointInsideRectangle(s2) )
			{
				//[UNUSED ATM] - no visual diff on VS but significant perf drop on CS
				/*if( LineRectangleIntersect(s0, s1) || 
					LineRectangleIntersect(s1, s2) || 
					LineRectangleIntersect(s2, s0) )*/
				
				// Backface culling in clip space
				float4 e1 = s1-s0;
				float4 e2 = s2-s0;
				float crossZ = e1.x*e2.y - e1.y*e2.x;
				if( crossZ < 0 )
				{
					// check whether this triangle actually contributes to anything in pixelspace
					float l1 = length(e1.xy);
					float l2 = length(e2.xy);
					const float minL = 1.0f / 256.0f;
					if( l1 > minL || l2 > minL )
					{
						uint originalVal;
						INTERLOCKED_ADD(IndirectArgs[0], 3, originalVal);

						IBufferOut[ originalVal ] = i.x;
						IBufferOut[ originalVal+1 ] = i.y;
						IBufferOut[ originalVal+2 ] = i.z;
					}
				}
			}
		}
	}
}