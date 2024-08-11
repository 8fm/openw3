#define USE_GLOBAL_SHADOW_PASS
#define USE_SHADOW_BLEND_DISSOLVE

#include "postfx_common.fx"
#include "include_utilities.fx"

#define DEFERRED
#include "include_constants.fx"

#if MSAA_NUM_SAMPLES > 1
TEXTURE2D_MS<float> TextureDepth      : register( t0 );
TEXTURE2D_MS<float4> GBufferSurface1  : register( t1 );
TEXTURE2D_MS<float4> GBufferSurface2  : register( t2 );
#else
TEXTURE2D<float> TextureDepth        : register( t0 );
TEXTURE2D<float4> GBufferSurface1    : register( t1 );
TEXTURE2D<float4> GBufferSurface2    : register( t2 );
#endif


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
	uint2 pixelCoord = i.pos.xy;

#if MSAA_NUM_SAMPLES > 1
	float zw = TextureDepth.Load( pixelCoord, 0 ).x;
#else
	float zw = TextureDepth[pixelCoord].x;
#endif

	const bool isSky = IsSkyByProjectedDepthRevProjAware( zw );

	float shadow = 1.0;
	[branch]
	if ( !isSky )
	{
#if MSAA_NUM_SAMPLES > 1
		const float4 gbuff1 = GBufferSurface1.Load( pixelCoord, 0 );			
		const float4 gbuff2 = GBufferSurface2.Load( pixelCoord, 0 );			
#else
		const float4 gbuff1 = GBufferSurface1[pixelCoord];
		const float4 gbuff2 = GBufferSurface2[pixelCoord];
#endif

		const float3 worldSpacePosition = PositionFromDepthRevProjAware(zw,pixelCoord);
		const float3 normalWorldSpace = normalize(gbuff1.xyz - 0.5);

		const bool isSpeedTreeReceiver = 0 != ((GBUFF_SPEEDTREE_SHADOWS | GBUFF_MATERIAL_FLAG_BILLBOARDS | GBUFF_MATERIAL_FLAG_TREES) & DecodeGBuffMaterialFlagsMask( gbuff2.w ));

		shadow = CalcGlobalShadow( normalWorldSpace, worldSpacePosition, pixelCoord, 0, isSpeedTreeReceiver );
	}
	
	return shadow;
}

#endif
