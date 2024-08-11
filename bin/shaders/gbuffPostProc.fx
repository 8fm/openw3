#include "postfx_common.fx"
#include "include_constants.fx"

#include "common.fx"

#define vTexCoordTransformScene		VSC_Custom_0
#define vTexCoordClamp				PSC_Custom_0

Texture2D				tTextureColor : register (t0);
SamplerState			sClampNoMip : register (s0);

Texture2D				tNormals : register (t1);
SamplerState			sNormals : register (s1);

Texture2D				tSpecular : register (t2);
SamplerState			sSpecular : register (s2);

Texture2D				tUnderwater : register (t6);
SamplerState			sUnderwater : register (s6);


struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		      : SYS_POSITION;
	float2 coordScene     : TEXCOORD0;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   = i.pos;
	o.coordScene = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformScene );

	return o;
}

#endif

#ifdef PIXELSHADER

struct PS_OUTPUT
{
	float4 specular 				: SYS_TARGET_OUTPUT0;
	float4 normal					: SYS_TARGET_OUTPUT1;	
};

PS_OUTPUT ps_main( VS_OUTPUT i )// : SYS_TARGET_OUTPUT0
{
	PS_OUTPUT o = (PS_OUTPUT)0;

	const uint2 pixelCoord = (uint2)i.pos.xy;	
	float effectStrength = saturate(PSC_Custom_2.x);
	
	// this comes as rain/snow strength and should not be 1:1 as specular bump strength
	// will expose this soon
	effectStrength *= 0.15f;

	// cancel effect if underwater
	float underwaterCut = ( 1.0f - SAMPLE_LEVEL( tUnderwater, sClampNoMip, i.coordScene.xy, 0 ).x );
	effectStrength *= underwaterCut;	
	
	float4 gbNormal = tNormals[pixelCoord];
	o.normal = gbNormal;	

	float roughness = gbNormal.w;

	float metalness = 0.5f;
	float4 specularTex = tSpecular[pixelCoord].xyzw;

	float porosity = 0.0f;
	float factor = 0.0f;
	//float3 sceneColor = float3(0.0f, 0.0f, 0.0f);	
	float depth = SYS_SAMPLE_TEXEL( PSSMP_SceneDepth, pixelCoord ).x;
	o.specular = specularTex;	
	
	if( IsSkyByProjectedDepthRevProjAware( depth ) ) 
	{		
		return o;
	}

	// get world pos
	float3 wPos = PositionFromDepthRevProjAware( depth, i.pos.xy );
	
	float resultFade = 0.0f; 					

	resultFade = CalculateVolumeCutByPixelCoord( pixelCoord, wPos );	

	{
		float3 specular_ycbcr = PackYCbCr( specularTex.xyz );
		specular_ycbcr.x = lerp( specular_ycbcr.x * max(underwaterCut, 0.3f), CalculateWetnessSpecular( resultFade * effectStrength, roughness, specularTex.xyz ).x, effectStrength );
		specular_ycbcr = saturate( specular_ycbcr );
		o.specular.xyz = UnpackYCbCr( specular_ycbcr );
	}

	o.normal.w = lerp( o.normal.w, CalculateWetnessRoughness( resultFade * effectStrength, roughness ).x, effectStrength );

	return o; 
}
#endif
