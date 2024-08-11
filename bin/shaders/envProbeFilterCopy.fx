#include "postfx_common.fx"


Texture2D		t_Texture		: register( t0 );

#define iSourceMipIndex		((int)PSC_Custom_0.z)
#define fAlpha				(PSC_Custom_0.w)

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
float4 ps_main( float4 vpos : SYS_POSITION ) : SYS_TARGET_OUTPUT0
{
	uint2 pixelCoord = vpos.xy;
	return float4 ( SAMPLE_MIPMAPS( t_Texture, iSourceMipIndex, pixelCoord ).xyz, fAlpha );
}
#endif
