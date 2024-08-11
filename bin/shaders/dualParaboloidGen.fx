#include "postfx_common.fx"

SamplerState	s_cube		: register( s0 );
TextureCube		t_cube		: register( t0 );

#define RENDER_RESOLUTION		PSC_Custom_0.x
#define IS_FRONT_RENDER			PSC_Custom_0.y


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
	o.pos = i.pos;	
	return o;
}
#endif

#ifdef PIXELSHADER
float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	uint2 vpos = (uint2)i.pos.xyz;
	float2 uv = vpos / (float2)(RENDER_RESOLUTION-1) * 2.0f - 1.0f;

	float3 dir;
	dir.x = 2.0f * uv.x;
	dir.y = 2.0f * uv.y;
	dir.z = -1.f + dot(uv,uv);
	dir *= (IS_FRONT_RENDER ? 1.f : -1.f);
	dir /= (dot(uv,uv) + 1.f);

	float alpha = (dot(uv,uv) < 1) ? 1 : 0;

	//return float4 ( t_cube.SampleLevel( s_cube, dir, 0 ).xyz, 1.f ) * alpha;
	return float4 ( SAMPLE_LEVEL( t_cube, s_cube, dir, 0 ).xyz, 1.f * alpha );
}
#endif
