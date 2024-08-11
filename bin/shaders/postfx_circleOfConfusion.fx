#include "blurSamples.fx"
#include "postfx_common.fx"

#define vTexCoordTransformFull	VSC_Custom_0
#define vTexCoordTransformHalf	VSC_Custom_1

#define vTexCoordClamp			PSC_Custom_0
#define vTexCoordDim			PSC_Custom_1
#define vTexCameraParams		PSC_Custom_2

#define vDofParamsRange			PSC_Custom_4
#define vDofParamsIntensity		PSC_Custom_5

SamplerState	s_TextureColor				: register( s0 );
Texture2D		t_TextureColor				: register( t0 );
SamplerState	s_SceneDepth				: register( s1 );
Texture2D		t_SceneDepth				: register( t1 );

#define planeInFocus		vTexCameraParams.x
#define focalLength			vTexCameraParams.y
#define apertureDiameter	vTexCameraParams.z
#define maxCoCsize			vTexCameraParams.w
#define bokehRatio			PSC_Custom_3.xy

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
	float2 coord       : TEXCOORD0;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;
	o.pos   = i.pos;
	o.coord = ApplyTexCoordTransform( i.pos.xy , vTexCoordTransformFull );
	return o;
}

#endif

#ifdef PIXELSHADER

float CalcBlurWeight( float depth )
{
#ifndef NO_FAR
	return	
		-saturate( (vDofParamsRange.x - depth) * vDofParamsRange.y ) +
		 saturate( (depth - vDofParamsRange.z) * vDofParamsRange.w );
#else
	return	
		-saturate( (vDofParamsRange.x - depth) * vDofParamsRange.y );
#endif
}


float CalculateCircleOfConfusion (  float d, float p,  float f, float a )
{
	// a - apertureDiameter - size of the "hole" in the apperture called "entrance pupil", related to the f-stops
	// p - planeInFocus - distance from the camera to the plane that is sharp
	// f - focalLength - property ot the lenses
	// d - depth - distance from the camera to point for which we calculate the Coc size
	return ( a * ( ( f * ( p - d) ) / ( d * ( p - f ) ) ) ); // We dont need ABS( ), bcoz near plane is < 0.0
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	float2 uv = i.coord;
	float sceneDepth = t_SceneDepth[ int2( i.pos.xy ) ].x;
	float depth =  DeprojectDepthRevProjAware( sceneDepth );

#ifdef PHYSICAL
	float blurWeight = CalculateCircleOfConfusion( depth , planeInFocus , focalLength , apertureDiameter ) * vDofParamsIntensity.x;
#else
	float blurWeight = CalcBlurWeight ( depth ) * vDofParamsIntensity.x;
#endif

	return float4( min( vDofParamsIntensity.y * blurWeight * blurWeight / 24.0 , 0.04 ) , 0.0 , ( blurWeight * 0.5f + 0.5f ) , 0 );
}
#endif
