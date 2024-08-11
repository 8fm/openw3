#include "commonPS.fx"
#include "commonCS.fx"
#ifdef VERTEXSHADER
#include "commonVS.fx"
#endif


struct VS_OUTPUT
{
	float4 _pos : SYS_POSITION;
};


#ifdef VERTEXSHADER

#ifdef CLEARDEPTH
#define ClearDepth VSC_Custom_0.x
#endif

VS_OUTPUT vs_main( uint num : SYS_VERTEX_ID )
{
	const float4 verts[4] = {
		{  1, 1, 0, 1 },
		{  1,-1, 0, 1 },
		{ -1, 1, 0, 1 },
		{ -1,-1, 0, 1 }
	};

	VS_OUTPUT o;

	o._pos  = verts[num];
#ifdef CLEARDEPTH
	o._pos.z = ClearDepth;
#endif

	return o;
}

#endif


#ifdef PIXELSHADER

#define ClearColor PSC_Custom_0

#ifdef OUTPUT32
#pragma PSSL_target_output_format(default FMT_32_ABGR)
#endif

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	return ClearColor;
}

#endif



#ifdef COMPUTESHADER

#ifdef CLEAR_BUFFER

// Must match ThreadCount in CRenderInterface::ClearBufferUAV_Uint()
#define ThreadCount 64


CS_CUSTOM_CONSTANT_BUFFER
	uint4 ClearValue;
END_CS_CUSTOM_CONSTANT_BUFFER

RW_STRUCTBUFFER(uint)	OutputBuffer	: register(u0);

[NUMTHREADS(ThreadCount,1,1)]
void cs_main(uint3 position : SYS_DISPATCH_THREAD_ID)
{
	const uint idx = position.x;
	OutputBuffer[ idx ] = ClearValue[ idx % 4 ];
}


#else //CLEAR_BUFFER

// Must match ThreadCount in CRenderInterface::ClearColorTarget()
#define ThreadCount 16


CS_CUSTOM_CONSTANT_BUFFER
	float4 ClearColor;
END_CS_CUSTOM_CONSTANT_BUFFER

RW_TEXTURE2D<float4> OutputTexture	: register(u0);


[NUMTHREADS(ThreadCount, ThreadCount, 1)]
void cs_main( uint3 GroupID : SYS_GROUP_ID, uint3 GroupThreadID : SYS_GROUP_THREAD_ID )
{
	uint2 pixelCoord = GroupID.xy * uint2(ThreadCount, ThreadCount) + GroupThreadID.xy;
	OutputTexture[pixelCoord] = ClearColor;
}

#endif //CLEAR_BUFFER

#endif //COMPUTESHADER
