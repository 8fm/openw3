#include "commonPS.fx"
#ifdef VERTEXSHADER
#include "commonVS.fx"
#endif
#include "commonCS.fx"


SYS_SAMPLER( SourceTexture, 0 );


struct VS_OUTPUT
{
	float4 pos : SYS_POSITION;
	float2 tex : TEXCOORD0;
};


#ifdef VERTEXSHADER

#define SourceRect	VSC_Custom_0

VS_OUTPUT vs_main( uint num : SYS_VERTEX_ID )
{
	const float4 verts[4] = {
		{  1, 1, 0, 1 },
		{  1,-1, 0, 1 },
		{ -1, 1, 0, 1 },
		{ -1,-1, 0, 1 }
	};

	const float2 texs[4] = {
		SourceRect.zy,
		SourceRect.zw,
		SourceRect.xy,
		SourceRect.xw
	};

	VS_OUTPUT o;

	o.pos = verts[num];
	o.tex = texs[num];

	return o;
}

#endif


#ifdef PIXELSHADER

#if defined(OUTPUT_32_RGBA)
#pragma PSSL_target_output_format(default FMT_32_ABGR)
#elif defined(OUTPUT_32_RG)
#pragma PSSL_target_output_format(default FMT_32_GR)
#elif defined(OUTPUT_32_R)

// it seems to me that FMT_32_AR or FMT_32_GR should be usable for R32 output BUT it gives a Razor warning
// so this is the workaround for now.

#pragma PSSL_target_output_format(default FMT_32_AR)	
#endif

#define ColorScale	PSC_Custom_0

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
#ifndef SCALE_COLOR
	return SYS_SAMPLE_LEVEL( SourceTexture, i.tex, 0 );
#else
	return SYS_SAMPLE_LEVEL( SourceTexture, i.tex, 0 ) * ColorScale;
#endif

}

#endif



#ifdef COMPUTESHADER

// Must match ThreadCount in CRenderInterface::StretchRect()
#define ThreadCount 16


CS_CUSTOM_CONSTANT_BUFFER
	uint4 DestRect;				// x,y,w,h
	float4 DestToSourceParams;	// scale/offset to transform from dest rect to source rect
END_CS_CUSTOM_CONSTANT_BUFFER

RW_TEXTURE2D<float4> OutputTexture	: register(u0);


[NUMTHREADS(ThreadCount, ThreadCount, 1)]
void cs_main( uint3 GroupID : SYS_GROUP_ID, uint3 GroupThreadID : SYS_GROUP_THREAD_ID )
{
	uint2 destLocalCoord = GroupID.xy * uint2(ThreadCount, ThreadCount) + GroupThreadID.xy;

	// Don't go outside the copy region.
	if ( any( destLocalCoord >= DestRect.zw ) )
	{
		return;
	}

	uint2 destCoord = DestRect.xy + destLocalCoord;

	float2 srcCoord = (float2)destLocalCoord * DestToSourceParams.xy + DestToSourceParams.zw;

	OutputTexture[destCoord] = SYS_SAMPLE_LEVEL( SourceTexture, srcCoord, 0 );
}

#endif
