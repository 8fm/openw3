#include "postfx_common.fx"
#include "include_msaa.fx"

// old technique seems faster (non filter based) a very little bit.
#define USE_OLD_TECHNIQUE 1

#if USE_OLD_TECHNIQUE
	Texture2D				t_CoverageMaskTexture	: register(t0);
#else
	#define vTexCoordSize	PSC_Custom_0
	SamplerState	s_CoverageMaskTexture	: register(s0);
	Texture2D		t_CoverageMaskTexture : register(t0);
#endif


struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
};

#ifdef VERTEXSHADER
VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   = i.pos;

	return o;
}
#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	uint2 pixelCoord = i.pos.xy;
	
#if USE_OLD_TECHNIQUE
	// mark full quad for better coherency (faster rendering:))
	pixelCoord -= pixelCoord % 2;
	if ( !t_CoverageMaskTexture[pixelCoord].x &&
		 !t_CoverageMaskTexture[pixelCoord + uint2(1,0)].x &&
		 !t_CoverageMaskTexture[pixelCoord + uint2(0,1)].x &&
		 !t_CoverageMaskTexture[pixelCoord + uint2(1,1)].x )
	{
		discard;
	}
#else
	// mark full quad for better coherency (faster:))
	float2 crd = ((pixelCoord - pixelCoord%2) + 1.0 ) * vTexCoordSize.zw;	//< coord in the middle of the quad
	if ( !(t_CoverageMaskTexture.Sample( s_CoverageMaskTexture, crd ).x > 0) )
	{
		discard;
	}
#endif

	return 0;
}



#endif
