#ifdef USE_OLD_DYNWATER


#include "common.fx"
#include "globalConstantsPS.fx"
#include "globalConstantsVS.fx"
#include "include_constants.fx"

Texture2D    t_tex : register( t0 );
SamplerState s_tex : register( s0 );

Texture2D		t_texi : register( t1 );
SamplerState    s_texi : register( s1 );

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
    o.pos = mul( i.pos, VSC_LocalToWorld );
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

#ifdef DYNAMICWATERIMPULSE
PS_OUTPUT ps_main( VS_OUTPUT v )
{
	PS_OUTPUT outp; 
		
	const float m = saturate( length( ( v.uv.xy ) - float2(0.5f,0.5f) ) * 2.0f );
    const float vv = 1.0f - m;

	const float ran = frac(sin(dot( float2( (v.uv.x + PSC_Custom_0.z) , (v.uv.y + PSC_Custom_0.w )), float2(12.9898f,78.233f))) * 43758.5453f);
	
    outp.color.xy = ( vv ) * PSC_Custom_0.xz * float2(0.05f,-0.05f)*(ran*0.5f+0.5f);
	outp.color.z  = 0.0f;
	outp.color.w  = 0.0f;
	return outp;
}
#endif

#ifdef DYNAMICWATER
PS_OUTPUT ps_main( VS_OUTPUT v )
{
	PS_OUTPUT outp;   

	const float rain = saturate(PSC_Custom_1.x);
    const float dt = min( ( PSC_Custom_2.x ) * 20.0f , 0.9f ); 	
	const float3 cam = PSC_Custom_1.yzw;
    const float2 delta = PSC_Custom_0.xy;
    
    const float2 p = v.uv.xy + delta; 
	const float2 imp = t_texi.Sample( s_texi, v.uv.xy ).xy*100.0f*dt;

	float4 x = t_tex.Sample( s_tex, p );
			
	const float offset = GlobalWater.dynamicWaterResolutionInv*0.888;	
	const float finalOffset = PSC_Custom_0.y * offset + offset;
			
	float4 sr = float4(0.0 , 0.0 , 0.0 , 0.0);
	{
		sr += t_tex.Sample( s_tex , p + float2(finalOffset,0.0) );
		sr += t_tex.Sample( s_tex , p + float2(-finalOffset,0.0) );
		sr += t_tex.Sample( s_tex , p + float2(0.0,finalOffset) );
		sr += t_tex.Sample( s_tex , p + float2(0.0,-finalOffset) );
		
		const float finalOffset2 = finalOffset * 0.70710678118;

		sr += t_tex.Sample( s_tex , p + float2(finalOffset2,finalOffset2) );
		sr += t_tex.Sample( s_tex , p + float2(finalOffset2,-finalOffset2) );
		sr += t_tex.Sample( s_tex , p + float2(-finalOffset2,finalOffset2) );
		sr += t_tex.Sample( s_tex , p + float2(-finalOffset2,-finalOffset2) );
	}
	sr *= 0.125;
	
	// randomize
	const float ran = frac(sin(dot( float2( (v.uv.x + PSC_Custom_0.z) , (v.uv.y + PSC_Custom_0.w )), float2(12.9898f,78.233f))) * 43758.5453f);
		
	x += clamp( float4(imp, imp ), -10.1, 10.1 );
	x.w += step( ran, lerp( 0.0001f, 0.1f, rain) ) * -0.5 * rain * dt;
			
	x *= saturate( 1.0 - dt * float4( 0.01, 0.02, 0.01, 0.02 ) );
		
	x.xy += ( x.x * ran * dt ) * PSC_Custom_0.zx * float2(-0.035,-0.01);
			
	x.y += ( -0.2 * (x.x) + ( sr.x - x.x ) * 0.75 ) * dt;
			
	x.x += ( x.y ) * dt;
	x.y += -x.y*0.04f;
			
	x.w += ( -0.1 * (x.z) + ( sr.z - x.z ) * 0.75 + sr.w * 0.02 ) * dt;    
			
	x.z += ( (sr.z - x.z ) * 0.05 + x.w ) * dt;	
			
	const float2 mask = step( float2(0.01,0.01), p ) * step( p , float2(0.99,0.99) );
			
	x.xy *= ( mask.x * mask.y );

    outp.color = x*step(abs(x),100000.0f);
	return outp;
}
#endif

#ifdef DYNAMICWATERFINALIZE
PS_OUTPUT ps_main( VS_OUTPUT v )
{
    PS_OUTPUT outp;

    const float2 p = v.uv.xy;    
    const float offset = GlobalWater.dynamicWaterResolutionInv;

    const float2 x1 = (t_tex.Sample( s_tex, float2(p.x+offset,p.y)) ).xz;
    const float2 x2 = (t_tex.Sample( s_tex, float2(p.x-offset,p.y)) ).xz;
    const float2 x3 = (t_tex.Sample( s_tex, float2(p.x,p.y+offset)) ).xz;
    const float2 x4 = (t_tex.Sample( s_tex, float2(p.x,p.y-offset)) ).xz;
			
	const float a = 2.0;
	const float2 d = -abs( 2.0*a*(p - 0.5) ) + a;			
	
	const float mask = saturate(d.x) * saturate(d.y);
	const float4 N = clamp( 0.05f*float4(x2-x1,x4-x3) , -0.5, 0.5 ) * mask;
	
	outp.color = float4( N.xz , N.yw );		
	return outp;
}
#endif


#endif



#else // USE_OLD_DYNWATER


#include "common.fx"

// Keep in sync with renderSimDynamicWater.cpp
#define TILE_SIZE 16


#ifdef DYNAMICWATERIMPULSE

struct VS_OUTPUT
{
	float4 pos	: SYS_POSITION;
	float2 uv	: TEXCOORD0;

	// Could also just bind the instance data as a cbuffer, address with SYS_INSTANCE_ID?
	NOINTERPOLATION float4 randomNums : TEXCOORD1;
};

struct VS_INPUT
{
	float4x4 transform	: INSTANCE_TRANSFORM;
	float4 randomNums	: EXTRA_DATA;
};

struct PS_OUTPUT
{
	float4 color 		: SYS_TARGET_OUTPUT0;
};


#ifdef VERTEXSHADER


VS_OUTPUT vs_main( VS_INPUT i, uint vertexID : SYS_VERTEX_ID )
{
	const float4 VertexPositions[] = {
		float4(  1, -1, 0, 1 ),
		float4( -1, -1, 0, 1 ),
		float4(  1,  1, 0, 1 ),
		float4( -1,  1, 0, 1 )
	};
	const float2 VertexUVs[] = {
		float2( 1, 1 ),
		float2( 0, 1 ),
		float2( 1, 0 ),
		float2( 0, 0 )
	};

	VS_OUTPUT o;

	// Actually need transform's transpose, so reversed the multiply
	o.pos			= mul( i.transform, VertexPositions[ vertexID ] );
	o.uv			= VertexUVs[ vertexID ];
	o.randomNums	= i.randomNums;

	return o;
}

#endif // VERTEXSHADER


#ifdef PIXELSHADER

PS_OUTPUT ps_main( VS_OUTPUT v )
{
	float4 randomNums = v.randomNums;

	PS_OUTPUT outp;

	const float m = saturate( length( ( v.uv.xy ) - float2(0.5f,0.5f) ) * 2.0f );
	const float vv = 1.0f - m;

	const float ran = frac(sin(dot( float2( (v.uv.x + randomNums.z) , (v.uv.y + randomNums.w )), float2(12.9898f,78.233f))) * 43758.5453f);

	outp.color.xy = ( vv ) * randomNums.xz * float2(0.05f,-0.05f)*(ran*0.5f+0.5f);
	outp.color.z  = 0.0f;
	outp.color.w  = 0.0f;
	return outp;
}

#endif // PIXELSHADER

#endif // DYNAMICWATERIMPULSE



#ifdef DYNAMICWATER

#ifndef COMPUTESHADER
#error New Finalize only supports compute shader
#endif


#include "commonCS.fx"

TEXTURE2D< float4 >		t_input : register( t0 );
TEXTURE2D< float2 >		t_impulses : register( t1 );
RW_TEXTURE2D< float4 >	t_output : register( u0 );


CS_CUSTOM_CONSTANT_BUFFER
	struct SimulateData {
		float2	cameraDelta;
		float2	randoms;
		float	rainIntensity;
		float	scaledDT;
		//float2 padding
	} DynWaterSimulateData;
END_CS_CUSTOM_CONSTANT_BUFFER


[NUMTHREADS(TILE_SIZE, TILE_SIZE, 1)]
void cs_main( uint3 ThreadID : SYS_DISPATCH_THREAD_ID )
{
	float2 textureSize;
	t_input.GetDimensions( textureSize.x, textureSize.y );


	const uint2 pixelCoord = ThreadID.xy;
	const float2 uv = pixelCoord / textureSize;

	const float2 srcUV = uv.xy + DynWaterSimulateData.cameraDelta;
	const uint2 srcPixelCoord = srcUV * textureSize;

	float4 sr = float4( 0.0f, 0.0f, 0.0f, 0.0f );
	{
		// TODO : Could maybe use groupshared memory here, reduce the number of texture reads
		sr += t_input.Load( int3(srcPixelCoord, 0), int2(-1,-1) );
		sr += t_input.Load( int3(srcPixelCoord, 0), int2( 0,-1) );
		sr += t_input.Load( int3(srcPixelCoord, 0), int2( 1,-1) );
		sr += t_input.Load( int3(srcPixelCoord, 0), int2(-1, 0) );
		sr += t_input.Load( int3(srcPixelCoord, 0), int2( 1, 0) );
		sr += t_input.Load( int3(srcPixelCoord, 0), int2(-1, 1) );
		sr += t_input.Load( int3(srcPixelCoord, 0), int2( 0, 1) );
		sr += t_input.Load( int3(srcPixelCoord, 0), int2( 1, 1) );
	}
	sr *= 0.125;

	// randomize
	const float ran = frac( sin( dot( uv + DynWaterSimulateData.randoms, float2( 12.9898f, 78.233f ) ) ) * 43758.5453f );

	const float dt = DynWaterSimulateData.scaledDT;

	const float2 impulse = t_impulses.Load( int3( pixelCoord, 0 ) ) * 100.0f * dt;

	float4 x = t_input.Load( int3( srcPixelCoord, 0 ) );

	x += clamp( impulse, -10.1, 10.1 ).xyxy;
	x.w += step( ran, lerp( 0.0001f, 0.1f, DynWaterSimulateData.rainIntensity) ) * -0.5 * DynWaterSimulateData.rainIntensity * dt;

	x *= saturate( 1.0 - dt * float4( 0.01, 0.02, 0.01, 0.02 ) );

	x.xy += ( x.x * ran * dt ) * DynWaterSimulateData.randoms * float2(-0.035,-0.01);

	x.y += ( -0.2 * x.x + ( sr.x - x.x ) * 0.75 ) * dt;

	x.x += x.y * dt;
	x.y += -x.y * 0.04f;

	x.w += ( -0.1 * x.z + ( sr.z - x.z ) * 0.75 + sr.w * 0.02 ) * dt;
	x.z += ( ( sr.z - x.z ) * 0.05 + x.w ) * dt;

	const float2 mask = step( float2(0.01,0.01), srcUV ) * step( srcUV , float2(0.99,0.99) );

	x.xy *= ( mask.x * mask.y );


	t_output[ pixelCoord ] = x * step( abs(x), 100000.0f );
}

#endif // DYNAMICWATER



#ifdef DYNAMICWATERFINALIZE

#ifndef COMPUTESHADER
#error New Finalize only supports compute shader
#endif


TEXTURE2D< float4 >		t_input : register( t0 );
RW_TEXTURE2D< float4 >	t_output : register( u0 );

[NUMTHREADS(TILE_SIZE, TILE_SIZE, 1)]
void cs_main( uint3 ThreadID : SYS_DISPATCH_THREAD_ID )
{
	const uint2 pixelCoord = ThreadID.xy;

	// TODO : Could maybe use groupshared memory here, reduce the number of texture reads?
	const float2 x1 = t_input.Load( int3( pixelCoord, 0 ), int2( 1, 0) ).xz;
	const float2 x2 = t_input.Load( int3( pixelCoord, 0 ), int2(-1, 0) ).xz;
	const float2 x3 = t_input.Load( int3( pixelCoord, 0 ), int2( 0, 1) ).xz;
	const float2 x4 = t_input.Load( int3( pixelCoord, 0 ), int2( 0,-1) ).xz;

	float2 textureSize;
	t_input.GetDimensions( textureSize.x, textureSize.y );
	const float2 p = float2(pixelCoord) / textureSize;

	const float a = 2.0;
	const float2 d = -abs( 2.0*a*(p - 0.5) ) + a;

	const float mask = saturate(d.x) * saturate(d.y);
	const float4 N = clamp( 0.05f*float4(x2-x1,x4-x3) , -0.5, 0.5 ) * mask;

	t_output[ pixelCoord ] = N.xzyw;
}

#endif // DYNAMICWATERFINALIZE


#endif // !USE_OLD_DYNWATER
