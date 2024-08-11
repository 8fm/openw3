
#include "common.fx"
#include "globalConstantsPS.fx"
#include "globalConstantsVS.fx"
#include "include_constants.fx"

Texture2D    t_tex : register( t0 );
SamplerState s_tex : register( s0 );

Texture2D		t_helperMask : register( t1 );
SamplerState    s_helperMask : register( s1 );

#define DIFFUSIONMAPRESOLUTION 512.0f
#define DIFFUSIONMAP_CH_RESOLUTION 256.0f

struct VS_INPUT
{
	float4 pos   : POSITION0;
	float4 color : COLOR0;
    float4 uv    : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos   : SYS_POSITION;
	float4 color : COLOR0;
    float4 uv    : TEXCOORD0;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;
    o.pos = i.pos;
    o.color = i.color;
    o.uv = i.uv;
	return o;
}
#endif

#ifdef PIXELSHADER

struct PS_OUTPUT
{
	float4 color 		: SYS_TARGET_OUTPUT0;	
};

#ifdef __PSSL__
#pragma PSSL_target_output_format(default FMT_32_ABGR)
#endif

#ifdef DIFFUSION2D


PS_OUTPUT ps_main( VS_OUTPUT v )
{
    PS_OUTPUT outp; 
    outp.color = float4(0.0f,0.0f,0.0f,0.0f);
	float offset = 1.0f/DIFFUSIONMAPRESOLUTION;
	float2 uv = v.uv.xy;

	float2 coor = floor( uv/float2(0.5f,0.5f) );
	float ran  = 0.65f + 0.35f * CalculateNoise2D( int(uv.x*150 + PSC_Custom_0.z*300.0f), int(uv.y*150 + PSC_Custom_0.w*300.0f) );
	
	float err1 = saturate( 1.0f - dot( coor-float2(0.0f,0.0f), coor-float2(0.0f,0.0f) ) );
	float err2 = saturate( 1.0f - dot( coor-float2(1.0f,0.0f), coor-float2(1.0f,0.0f) ) );
	float err3 = saturate( 1.0f - dot( coor-float2(0.0f,1.0f), coor-float2(0.0f,1.0f) ) );
	float err4 = saturate( 1.0f - dot( coor-float2(1.0f,1.0f), coor-float2(1.0f,1.0f) ) );	
	
	float4 mask = float4( err1, err2, err3, err4 );
	
	float2 impuv = frac(float2(2.0f,2.0f)*uv);
	float2 uv1 = saturate( uv+float2( offset,0.0f) );
	float2 uv2 = saturate( uv+float2(-offset,0.0f) );
	float2 uv3 = saturate( uv+float2(0.0f, offset) );
	float2 uv4 = saturate( uv+float2(0.0f,-offset) );
	
    float4 imp = t_helperMask.Sample( s_helperMask, impuv ).xyzw;    
    float2 col = t_tex.Sample( s_tex, uv ).xyzw;
	
	float2 col1 = t_tex.Sample( s_tex, uv1 ).xyzw;
	float2 col2 = t_tex.Sample( s_tex, uv2 ).xyzw;
	float2 col3 = t_tex.Sample( s_tex, uv3 ).xyzw;
	float2 col4 = t_tex.Sample( s_tex, uv4 ).xyzw;
	float2 cols = (col1+col2+col3+col4)*0.25f;
	
	offset = 1.0f/DIFFUSIONMAP_CH_RESOLUTION;
	float4 imp1 = t_helperMask.Sample( s_helperMask, impuv+float2( offset,0.0f) ).xyzw;  
	float4 imp2 = t_helperMask.Sample( s_helperMask, impuv+float2(-offset,0.0f) ).xyzw;  
	float4 imp3 = t_helperMask.Sample( s_helperMask, impuv+float2(0.0f, offset) ).xyzw;  
	float4 imp4 = t_helperMask.Sample( s_helperMask, impuv+float2(0.0f,-offset) ).xyzw;  
	
	float2 rang = saturate( max( abs(imp1.xy-imp2.xy), abs(imp3.xy-imp4.xy) ) );
	
	float dImpMask = dot(imp,mask);

	col.y += dImpMask*0.15f;
	col.y += dImpMask*rang.x*0.35f;
	col.y += dImpMask*rang.y*0.35f;
	
	
	col.y += (cols.x-col.x)*0.9f*ran;	
	col.y += 0.2f*col.x * (-1.2f)*ran;
	col.x += col.y;
	
	// dampen the diffusion based on camera forward delta
	col *= lerp( 0.7f, 0.86f, pow( saturate( PSC_Custom_0.x ), 100.0f ) );
	
	outp.color.xy = col;	
	return outp;
}
#endif




#endif
