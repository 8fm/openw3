#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"
#include "include_constants.fx"
#include "include_utilities.fx"
#include "include_envProbe.fx"

TEXTURE2D			sTextureAtlasAmbient		: register( t0 );
TEXTURE2D			sTextureAtlasReflection		: register( t1 );
SamplerState sSampler							: register( s0 );


#define MODE_INDEX					PSC_Custom_0.w

#define CUBE_ARRAY_SLOT_INDEX		PSC_Custom_0.x
#define CUBE_AMBIENT_WEIGHT			PSC_Custom_0.y
#define CUBE_REFLECTION_WEIGHT		PSC_Custom_0.z
#define CUBE_AMBIENT_CUSTOM_MIP		PSC_Custom_1.x
#define CUBE_REFLECTION_CUSTOM_MIP	PSC_Custom_1.y

#define MODE12_WORLD_SPACE_POS		PSC_Custom_2.xyz
#define MODE12_RESULT_SCALE			PSC_Custom_3.xyz
#define MODE12_SAMPLE_DIR_SCALE		float3( 1, -1, -1 )
#define MODE12_VIEW_ROTATION		float3x3( PSC_Custom_4.xyz, PSC_Custom_5.xyz, PSC_Custom_6.xyz )
#define MODE12_CUSTOM_MIP			PSC_Custom_0.z


struct VS_INPUT
{
	float4 pos : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos   : SYS_POSITION;
	float3 norm  : TEXCOORD0;
	float4 wpos  : TEXCOORD1;	
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.wpos = mul( i.pos, VSC_LocalToWorld );
	o.pos  = mul( o.wpos, VSC_WorldToScreen );
	o.norm = i.pos.xyz;

	return o;
}

#endif

#ifdef PIXELSHADER
float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const float2 vpos = i.pos.xy;
	const float3 sampleDir = normalize( i.norm );

	float3 result = 0;

	if ( 0 == MODE_INDEX )
	{
		const float3 ambientColor = SampleEnvProbeAmbient( sTextureAtlasAmbient, CUBE_ARRAY_SLOT_INDEX, sSampler, sampleDir, CUBE_AMBIENT_CUSTOM_MIP );
		const float3 reflectionColor = SampleEnvProbeReflection( sTextureAtlasReflection, CUBE_ARRAY_SLOT_INDEX, sSampler, sampleDir, (CUBE_REFLECTION_CUSTOM_MIP >= 0 ? CUBE_REFLECTION_CUSTOM_MIP : CalcEnvProbeMipLevel( sampleDir, vpos )) );

		// (x>0 tested so that nan's won't bleed between ambient and reflection)
		result = 
			(CUBE_AMBIENT_WEIGHT > 0    ? CUBE_AMBIENT_WEIGHT    * ambientColor    : 0) +
			(CUBE_REFLECTION_WEIGHT > 0 ? CUBE_REFLECTION_WEIGHT * reflectionColor : 0);
	}
	else if ( 1 == MODE_INDEX )
	{
		float3 _sampleDir = sampleDir * MODE12_SAMPLE_DIR_SCALE;
		const float interiorFactor = CalcDimmersFactorAndInteriorFactorTransparency( MODE12_WORLD_SPACE_POS, vpos ).y;
		result = MODE12_RESULT_SCALE * CalcEnvProbeAmbient( MODE12_WORLD_SPACE_POS, mul( MODE12_VIEW_ROTATION, _sampleDir ), true, MODE12_CUSTOM_MIP, interiorFactor );
	}
	else if ( 2 == MODE_INDEX )
	{
		float3 _sampleDir = sampleDir * MODE12_SAMPLE_DIR_SCALE;
		float3 R = mul( MODE12_VIEW_ROTATION, _sampleDir );
		const float interiorFactor = CalcDimmersFactorAndInteriorFactorTransparency( MODE12_WORLD_SPACE_POS, vpos ).y;
		result = MODE12_RESULT_SCALE * CalcEnvProbeReflection_MipLod( MODE12_WORLD_SPACE_POS, normalize( R ), MODE12_CUSTOM_MIP, 0, true, interiorFactor );
	}
	else
	{
		result = float3 ( 1, 1, 0 );
	}

	return float4 ( result, 1.0 );
}

#endif
