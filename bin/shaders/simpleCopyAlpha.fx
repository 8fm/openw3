#include "commonPS.fx"
#ifdef VERTEXSHADER
#include "commonVS.fx"
#endif

SYS_SAMPLER( SourceTexture, 0 );
SYS_SAMPLER( SourceTextureAlpha, 1 );


struct VS_OUTPUT
{
	float4 pos 		: SYS_POSITION;
	float2 tex 		: TEXCOORD0;
	float2 texAlpha : TEXCOORD1;
};


#ifdef VERTEXSHADER

#define SourceRect	VSC_Custom_0
#define SourceRectAlpha	VSC_Custom_1

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

	const float2 texsAlpha[4] = {
		SourceRectAlpha.zy,
		SourceRectAlpha.zw,
		SourceRectAlpha.xy,
		SourceRectAlpha.xw
	};

	VS_OUTPUT o;

	o.pos = verts[num];
	o.tex = texs[num];
	o.texAlpha = texsAlpha[num];

	return o;
}

#endif


#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	float3 rgb = SYS_SAMPLE_LEVEL( SourceTexture, i.tex, 0 ).rgb;
	float  a   = SYS_SAMPLE_LEVEL( SourceTextureAlpha, i.texAlpha, 0 ).a;

	return float4( rgb , a );
}

#endif
