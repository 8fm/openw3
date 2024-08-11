#include "common.fx"
#include "include_utilities.fx"
#include "postfx_common.fx"

#define DEFERRED
#include "include_constants.fx"

#define vChannelWeights0		PSC_Custom_0
#define vChannelWeights1		PSC_Custom_1
#define VMasks					PSC_Custom_2.xy
#define fBlendFactor			PSC_Custom_2.z
#define iFaceIndex				PSC_Custom_3.x

TEXTURE2D_ARRAY			TextureShadow		: register( t0 );
TEXTURE2D_ARRAY<float>	TextureDepth		: register( t1 );


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

	o.pos   = i.pos;

	return o;
}
#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const uint2 pixelCoord = i.pos.xy;

	const float zw = TextureDepth[ uint3( pixelCoord, iFaceIndex ) ].x;

	const bool isSky = IsSkyByProjectedDepthRevProjAware( zw );

	float shadow = 1.0;
	[branch]
	if ( !isSky )
	{
		const float3 worldSpacePosition = PositionFromDepthRevProjAware(zw,pixelCoord);

		float4 orig_color = TextureShadow[ uint3( pixelCoord, iFaceIndex ) ];

		// ace_ibl_todo: actually no blending of shadow term will be needed when i'll make blending of the cubemaps

		float shadow0 = (((int)dot( vChannelWeights0, orig_color )) & (int)VMasks.x ? 1 : 0);
		float shadow1 = (((int)dot( vChannelWeights1, orig_color )) & (int)VMasks.y ? 1 : 0);
		
		shadow = lerp( shadow0, shadow1, fBlendFactor );

		shadow *= CalculateCloudsShadow( worldSpacePosition );
	}
	
	float4 result = 1;
	result[GLOBAL_SHADOW_BUFFER_CHANNEL_SHADOW] = shadow;
	return result;
}

#endif
