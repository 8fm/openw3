#include "postfx_common.fx"


#define vTexCoordTransformColor  VSC_Custom_0
#define vTexCoordTransformBlur   VSC_Custom_1
#define vTexCoordTransformDepth  VSC_Custom_2
#define vViewSpaceVecTransform	 VSC_Custom_3
#define vCamForwardVS 			 VSC_Custom_4
#define vCamRightVS 			 VSC_Custom_5
#define vCamUpVS 				 VSC_Custom_6
#define vDofParams  			 PSC_Custom_0
#define vDofSkyParams  			 PSC_Custom_1
#define vDofParams2  			 PSC_Custom_2

SamplerState	s_TextureColor		: register( s0 );
Texture2D		t_TextureColor		: register( t0 );

SamplerState	s_TextureBlur       : register( s1 );
Texture2D		t_TextureBlur		: register( t1 );

SamplerState	s_TextureDepth      : register( s2 );
Texture2D		t_TextureDepth		: register( t2 );

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
	float2 coordColor  : TEXCOORD0;
	float2 coordBlur   : TEXCOORD1;
	float2 coordDepth  : TEXCOORD2;
	float3 worldVec	   : TEXCOORD3;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	float3 viewVec = ApplyViewSpaceVecTransform( i.pos.xy, vViewSpaceVecTransform );
	
	o.pos   = i.pos;
	o.coordColor = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformColor );
	o.coordBlur  = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformBlur );
	o.coordDepth = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformDepth );	
	o.worldVec	 = mul( viewVec, float3x3( vCamRightVS.xyz, vCamUpVS.xyz, vCamForwardVS.xyz ) );
	
	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	float near 		= vDofParams.x;
	float far  		= vDofParams.y;
	float intensity	= vDofParams.z;
	float invRange	= vDofParams.w;

	float near2 	= vDofParams2.x;
	float far2  	= vDofParams2.y;
	float invRange2	= vDofParams2.w;
	
    float4 orig_color = t_TextureColor.Sample( s_TextureColor, i.coordColor );
	float4 blur_color = t_TextureBlur.Sample( s_TextureBlur,  i.coordBlur );
	float depth      = DeprojectDepthRevProjAware( t_TextureDepth.Sample( s_TextureDepth, i.coordDepth ).x );	
	
	float f = saturate( (depth - near) * invRange );
	f = sqrt(f);
	f = f * intensity;
	
	if ( IsSkyByLinearDepth( depth ) )
	{
		float skyDofStart    = vDofSkyParams.x;
		float skyDofRangeInv = vDofSkyParams.y;

		float3 v = normalize( i.worldVec.xyz );
		f *= saturate( 1 - (v.y - skyDofStart) * skyDofRangeInv );
	}
	
	return lerp( orig_color, blur_color, f );
}
#endif
