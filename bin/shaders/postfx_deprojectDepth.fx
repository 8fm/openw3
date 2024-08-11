#include "postfx_common.fx"

#define vTexCoordTransform VSC_Custom_0
#define vTexCoordClamp PSC_Custom_0

#if MULTISAMPLED_DEPTH
TEXTURE2D_MS<float>		sTexture		: register( t0 );
#else
TEXTURE2D<float>		sTexture		: register( t0 );
#endif


struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos			: SYS_POSITION;
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

#ifdef __PSSL__
#pragma PSSL_target_output_format(default FMT_32_AR)
#endif

float4 ps_main( float4 vpos : SYS_POSITION ) : SYS_TARGET_OUTPUT0
{
	const uint2 pixelCoord = vpos.xy;
#if MULTISAMPLED_DEPTH
	float orig_depth = sTexture.Load(pixelCoord, 0).x;
#else
	float orig_depth = sTexture[pixelCoord].x;
#endif
	float deproj_depth = DeprojectDepthRevProjAware( orig_depth );
	return deproj_depth;
}
#endif
