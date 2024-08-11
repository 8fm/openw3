
#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"
#include "include_constants.fx"

#define PAT_PI 3.1415926535897932384626433f

Texture2D		t_tex : register( t0 );
SamplerState	s_tex : register( s0 );

Texture2D		t_input : register( t1 );
SamplerState	s_input : register( s1 );

Texture2D		t_order : register( t2 );
SamplerState	s_order : register( s2 );

Texture2D		t_indices : register( t3 );
SamplerState	s_indices : register( s3 );

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

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;
	o.pos = i.pos;
	o.color = i.color;
	o.uv = i.uv;
	return o;
}

#ifdef COMPUTESHADER

float2 complex_mul( float2 a, float2 b )
{
	return float2( a.x*b.x-a.y*b.y, a.x*b.y+b.x*a.y );
}

#ifdef PHILLIPS_ANIMATION

RW_TEXTURE2D<float2> t_output : register( u4 );

#include "commonCS.fx"

CS_CUSTOM_CONSTANT_BUFFER
	float4 dataPhillips;
	float4 dataAnimation;
END_CS_CUSTOM_CONSTANT_BUFFER

float2 Philips( float2 p )
{
	float2 k = ( p - float2(0.5f,0.5f) ) * PAT_PI;
			
	const float2 wdir_sincos = dataPhillips.xy; // cos/-sin of 6.2830384f * wdir
	float L2 = dataPhillips.z;	// ( wspeed * wspeed / 9.81f ) ^ 2
	float amplitude = dataPhillips.w;

	float k_sq = dot(k,k);
	float k_len = sqrt(k_sq);
	float2 k_ = k / k_len;
	
	float w_dot_k = dot( k_ , wdir_sincos );
	float w_dot_k_sq = w_dot_k * w_dot_k;
	
	float m = amplitude * exp( -1.0f / (k_sq*L2) ) / (k_sq*k_sq) * w_dot_k_sq;
	
	// bi-directional movement
	float mul = 1.0f;
	mul -= step( w_dot_k < 0.0f, 0.0f )*( k_len );	

	return ( sqrt( m ) * k_len ) * mul;
}

#define INVTEXTURESIZEX  1.f / 512.f
#define INVTEXTURESIZEY  1.f / 512.f

[NUMTHREADS(TILE_SIZE, TILE_SIZE, 1)]
void cs_main( uint3 ThreadID : SYS_DISPATCH_THREAD_ID )
{
	const uint2 pixelCoord = ThreadID.xy;
	float2 uv = (float2(pixelCoord) + 0.5f) / 512.f;

	float2 p = frac( uv.xy + float2( 0.5f, 0.5f ) );
	float2 x0 = t_input[ p * 512 ].xy;
	float2 x = x0 * Philips( p );
	//x = x0;

	float dt = dataAnimation.z;
	float2 t2 = dt * (p * 2.0f - float2(1.0f,1.0f)); // dt * k
												// directional, bi-directional
	float2 waveDirectionalVelocity = normalize( float2( dataAnimation.x, dataAnimation.y ) );

	// overal water sim speed	
	float l = 5.0f*(1.0f - length( uv.xy - float2(0.5f, 0.5f) ));
	
	float len = 0.0001f * length( x ) + 0.000001f;
	float c1, s1;
	sincos(l * dt, s1, c1);

	float2 complex = waveDirectionalVelocity.x * float2( s1, c1 ) + float2( waveDirectionalVelocity.y * cos( t2.x ), sin( t2.y ) ); 
	
	t_output[pixelCoord].xy = complex_mul( len * complex, x );
}

#endif

#ifdef ORDERING

RW_TEXTURE2D<float2> t_output : register( u4 );

#define ORDERING_TEXTURE_SIZE 512

[NUMTHREADS(TILE_SIZE, TILE_SIZE, 1)]
void cs_main( uint3 GroupID : SYS_GROUP_ID, uint3 ThreadID : SYS_GROUP_THREAD_ID )
{
	const uint2 pixelCoord = GroupID.xy * uint2(TILE_SIZE, TILE_SIZE) + ThreadID.xy;
	float c = t_order[uint2(pixelCoord.x, 0)].x; // the order texture is 1 tall and 512 wide
#ifdef TRANSPOSE
	uint2 u = uint2( pixelCoord.y , c * ORDERING_TEXTURE_SIZE );
#else
	uint2 u = uint2( c * ORDERING_TEXTURE_SIZE , pixelCoord.y );
#endif
	//t_output[ pixelCoord ] = t_tex[pixelCoord].xy;
	t_output[ pixelCoord ] = t_tex[u].xy;
}
#endif

#ifdef BUTTERFLY

RW_TEXTURE2D<float2> t_output : register( u4 );

#include "commonCS.fx"

CS_CUSTOM_CONSTANT_BUFFER
	float4 data;
END_CS_CUSTOM_CONSTANT_BUFFER

[NUMTHREADS(TILE_SIZE, TILE_SIZE, 1)]
void cs_main( uint3 GroupID : SYS_GROUP_ID, uint3 ThreadID : SYS_GROUP_THREAD_ID )
{
	const uint2 pixelCoord = GroupID.xy * uint2(TILE_SIZE, TILE_SIZE) + ThreadID.xy;
	float step = data.x;

	float4 s = t_indices[ uint2( pixelCoord.x, step ) ];

	float2 e_uv = float2(s.x * 512.f, (float)pixelCoord.y);
	float2 o_uv = float2(s.y * 512.f, (float)pixelCoord.y);

	float2 ev = t_tex[uint2(e_uv)].xy;
	float2 ov = t_tex[uint2(o_uv)].xy;

	float2 wyn = ev + complex_mul( s.zw, ov );

	t_output[ pixelCoord ] = wyn;
}
#endif

#ifdef FINALIZE

RW_TEXTURE2D<float4> t_output : register( u4 );

#include "commonCS.fx"

CS_CUSTOM_CONSTANT_BUFFER
	float4 data;
END_CS_CUSTOM_CONSTANT_BUFFER

[NUMTHREADS(TILE_SIZE, TILE_SIZE, 1)]
void cs_main( uint3 GroupID : SYS_GROUP_ID, uint3 ThreadID : SYS_GROUP_THREAD_ID )
{
	const uint2 pixelCoord = GroupID.xy * uint2(TILE_SIZE, TILE_SIZE) + ThreadID.xy;

	float sq = data.x; // 1.0 / resolution
	float offset = data.y;	// ( 1 + 8 * windScale );
	
	float x = sq * t_tex[ pixelCoord ].x;
	
	uint2 bottomright = (pixelCoord + 1) % 512;
	uint2 topleft = (pixelCoord - 1) % 512;

	float x1 = t_tex[ uint2( bottomright.x, pixelCoord.y ) ].x;
	float x2 = t_tex[ uint2( topleft.x,		pixelCoord.y ) ].x;
	float x3 = t_tex[ uint2( pixelCoord.x,	bottomright.y ) ].x;
	float x4 = t_tex[ uint2( pixelCoord.x,	topleft.y ) ].x;

	uint2 bottomright_offset = (pixelCoord + offset) % 512;
	uint2 topleft_offset = (pixelCoord - offset) % 512;

	float4 xx = float4(
		t_tex[ uint2( bottomright_offset.x, pixelCoord.y) ].x,
		t_tex[ uint2( topleft_offset.x,		pixelCoord.y) ].x,
		t_tex[ uint2( pixelCoord.x,			bottomright_offset.y) ].x,
		t_tex[ uint2( pixelCoord.x,			topleft_offset.y) ].x);

	float nx = sq * ( x2-x1 );
	float ny = sq * ( x4-x3 );

	float delta = 1.0f + 3000.0f*( x - sq * dot(xx,1.f)/4.0f );

	t_output[ pixelCoord ] = float4( nx, ny, delta, x );
}
#endif

#endif//computeshader


#ifdef PIXELSHADER

struct PS_OUTPUT
{
	float4 color 		: SYS_TARGET_OUTPUT0;	
};

float2 complex_mul( float2 a, float2 b )
{
	return float2( a.x*b.x-a.y*b.y, a.x*b.y+b.x*a.y );
}

#ifdef PHILLIPS_ANIMATION

float2 Philips( float2 p )
{
	float2 x = t_input.Sample( s_input, p ).xy;

	float2 k = ( p - float2(0.5f,0.5f) ) * PAT_PI;
			
	const float2 wdir_sincos = PSC_Custom_0.xy; // cos/-sin of 6.2830384f * wdir
	float L2 = PSC_Custom_0.z;	// ( wspeed * wspeed / 9.81f ) ^ 2
	float amplitude = PSC_Custom_0.w;	

	float k_sq = dot(k,k);
	float k_len = sqrt(k_sq);
	float2 k_ = k / k_len;
	
	float w_dot_k = dot( k_ , wdir_sincos );
	float w_dot_k_sq = w_dot_k * w_dot_k;
	
	float m = amplitude * exp( -1.0f / (k_sq*L2) ) / (k_sq*k_sq) * w_dot_k_sq;
	
	// bi-directional movement
	float mul = 1.0f;
	mul -= step( w_dot_k < 0.0f, 0.0f )*( k_len );

	return x * ( sqrt( m ) * k_len ) * mul;
}

PS_OUTPUT ps_main( VS_OUTPUT v )
{

/*
#ifdef __PSSL__
#pragma PSSL_target_output_format(default FMT_FP16_ABGR)
#endif
*/

	PS_OUTPUT outp;

	float2 p = frac( v.uv.xy + float2(0.5f,0.5f) );
	float2 x = Philips( p );

	float dt = PSC_Custom_1.z;
	float2 t2 = dt * (p * 2.0f - float2(1.0f,1.0f)); // dt * k
												// directional, bi-directional
	float2 waveDirectionalVelocity = normalize( float2( PSC_Custom_1.x, PSC_Custom_1.y ) );

	// overal water sim speed	
	float l = 5.0f*(1.0f - length( v.uv.xy - float2(0.5f, 0.5f) ));

	float len = 0.0001f * length( x ) + 0.000001f;
	float c1, s1;
	sincos(l * dt, s1, c1);
	float2 complex = waveDirectionalVelocity.x * float2( s1, c1 ) + float2( waveDirectionalVelocity.y * cos( t2.x ), sin( t2.y ) ); 
			
	float2 xx = complex_mul( len * complex, x );

	outp.color = float4(xx,0.0f,1.0f);
	return outp;
}

#endif

#ifdef ORDERING
PS_OUTPUT ps_main( VS_OUTPUT v )
{
	PS_OUTPUT outp;
	float2 p = v.uv.xy;
	float c = t_order.Sample( s_order, p).x;
#ifdef TRANSPOSE
	float2 u = float2( p.y , c );
#else
	float2 u = float2( c , p.y );
#endif
	float2 col = t_tex.Sample( s_tex, u ).xy;
	outp.color = float4(col,0.0f,1.0f);
	return outp;
}
#endif

#ifdef BUTTERFLY
PS_OUTPUT ps_main( VS_OUTPUT v )
{
	PS_OUTPUT outp;
	float2 p = v.uv.xy;
	float step = PSC_Custom_0.x;

	float4 s = t_indices.Sample( s_indices, float2(p.x,step)).xyzw;

	float2 e_uv = float2(s.x,p.y);
	float2 o_uv = float2(s.y,p.y);
	float2 ev = t_tex.Sample( s_tex, e_uv ).xy;
	float2 ov = t_tex.Sample( s_tex, o_uv ).xy;

	float2 wyn = ev + complex_mul(s.zw,ov);

	outp.color = float4(wyn,0.0f,1.0f);
	return outp;
}
#endif

#ifdef FINALIZE
PS_OUTPUT ps_main( VS_OUTPUT v )
{
	PS_OUTPUT outp;
	float2 p = v.uv.xy;

	float sq = PSC_Custom_0.x; // 1.0 / resolution
	float offsx = 0.5f* PSC_Custom_0.y;	// sq * ( 2.0f + 16.0f * windScale );
	
	float x = sq * t_tex.Sample( s_tex, p).x;
  
	float x1 = t_tex.Sample( s_tex, float2(p.x+sq,p.y)).x;
	float x2 = t_tex.Sample( s_tex, float2(p.x-sq,p.y)).x;
	float x3 = t_tex.Sample( s_tex, float2(p.x,p.y+sq)).x;
	float x4 = t_tex.Sample( s_tex, float2(p.x,p.y-sq)).x;

	float4 xx = float4(
		t_tex.Sample( s_tex, float2(p.x+offsx,p.y)).x,
		t_tex.Sample( s_tex, float2(p.x-offsx,p.y)).x,
		t_tex.Sample( s_tex, float2(p.x,p.y+offsx)).x,
		t_tex.Sample( s_tex, float2(p.x,p.y-offsx)).x);

	float nx = sq * ( x2-x1 );
	float ny = sq * ( x4-x3 );

	float delta = 1.0f + 3000.0f*( x - sq * (dot(xx,1))/4.0f );

	outp.color = float4( nx , ny , delta, x );
	
	return outp;
}
#endif

#ifdef COPYHEIGHT
PS_OUTPUT ps_main( VS_OUTPUT v )
{
	PS_OUTPUT outp;	
	float x = t_tex.Sample( s_tex, v.uv.xy ).w;
	outp.color = float4(x, 0.0f, 0.0f, 0.0f);

	return outp;
}
#endif


#endif
