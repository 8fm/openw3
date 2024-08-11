#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"

//- - - - - - - - - - - - - - - - - - - - - - - - - - - 
#if TEX_ARRAY

	#ifndef CUBE
		TEXTURE2D_ARRAY<float4>	sTexture	: register( t0 );
	#else
		TEXTURECUBE_ARRAY<float4>sTexture	: register( t0 );
	#endif
//- - - - - - - - - - - - - - - - - - - - - - - - - - - 
#else // Not Array

	#ifndef CUBE
		Texture2D<float4>		sTexture	: register( t0 );
	#else
		TextureCube<float4>		sTexture	: register( t0 );
	#endif
		
#endif
//- - - - - - - - - - - - - - - - - - - - - - - - - - - 

SamplerState samp					: register( s0 );

#if ENABLE_EXPONENT
    #define fExponent PSC_Custom_0
#endif

struct VS_INPUT
{
	float4 pos		: POSITION0;
	float4 color	: COLOR0;
	float2 uv		: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos		: SYS_POSITION;
	float4 color	: COLOR0;
	float2 uv		: TEXCOORD0;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos = mul( mul( i.pos, VSC_LocalToWorld ), VSC_WorldToScreen );
	o.uv = i.uv;

#if XENON	
	o.color = i.color.wxyz;
#else
	o.color = i.color;
#endif	

	return o;
}

#endif

#ifdef PIXELSHADER

#ifdef CUBE

static const float3 upVec[6] = 
{
	float3( 0.0f, 0.0f, 1.0f ), 
	float3( 0.0f, 0.0f, 1.0f ), 
	float3( 0.0f, 0.0f, 1.0f ), 
	float3( 0.0f, 0.0f,-1.0f ), 
	float3( 1.0f, 0.0f, 0.0f ), 
	float3(-1.0f, 0.0f, 0.0f )
};

static const float3 forwardVec[6] = 
{
	float3( 1.0f, 0.0f, 0.0f ), 
	float3(-1.0f, 0.0f, 0.0f ), 
	float3( 0.0f, 1.0f, 0.0f ), 
	float3( 0.0f,-1.0f, 0.0f ), 
	float3( 0.0f, 0.0f,-1.0f ), 
	float3( 0.0f, 0.0f, 1.0f )
};

#endif

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const float slice	= PSC_Custom_2.y;
	const float mip		= PSC_Custom_2.x;

#ifdef CUBE
	// Get virtual mapping for just one cubemap face
	const int cubeside	= int(PSC_Custom_2.z);
	const float2 cuv = i.uv * 2.0 - 1.0;
	const float3 f = forwardVec[cubeside];
	const float3 u = upVec[cubeside];
	const float3 r = cross(f,u);
	const float3 cubeUV	= cuv.x * r + cuv.y * u + f;
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - 
#if TEX_ARRAY

	#ifndef CUBE
		float4 result = SAMPLE_LEVEL( sTexture, samp, float3( i.uv, slice ), mip );
	#else
		// float4 result = float4( cubeUV *0.5 + 0.5, 1.0 );
		float4 result = SAMPLE_LEVEL( sTexture, samp, float4( cubeUV, slice ), mip );
	#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - 
#else // Not Array

	#ifndef CUBE
		float4 result = SAMPLE_LEVEL( sTexture, samp, i.uv, mip );	
	#else
		float4 result = SAMPLE_LEVEL( sTexture, samp, cubeUV, mip );	
	#endif
    
#endif
//- - - - - - - - - - - - - - - - - - - - - - - - - - - 

	result -= ( PSC_Custom_0 );
	result /= ( PSC_Custom_1 - PSC_Custom_0 );

	result = lerp( float4(0,0,0,1), result, PSC_Custom_3 );

	return result;
}

#endif
