#if MSAA_NUM_SAMPLES > 1
# error Not supported at this moment
#endif

#include "postfx_common.fx"
#include "include_utilities.fx"
#include "include_constants.fx"

Texture2D TextureDepth		: register( t0 );
Texture2D TextureScene		: register( t1 );
Texture2D ShadowSurface		: register( t2 );

#define vFogImpactParams		PSC_Custom_0
#define vFogImpactParams2		PSC_Custom_1

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

float3 ApplyFogWithSSAOImpact( in float3 color, in bool isSky, in bool isClouds, in float3 fragPosWorldSpace, in float ssaoImpact )
{
	SFogData fogData = CalculateFogFull( isSky, isClouds, fragPosWorldSpace );
	fogData.paramsFog.w *= ssaoImpact;
	fogData.paramsAerial.w *= ssaoImpact;
	return ApplyFogDataFull( color.xyz, fogData );
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	uint2 pixelCoord = i.pos.xy;

	const float zw = TextureDepth[pixelCoord].x;
	const bool isSky = IsSkyByProjectedDepthRevProjAware( zw );	
	const float3 worldSpacePosition = PositionFromDepthRevProjAware(zw,pixelCoord);

	float3 resultColor = float3(0,0,0);

	[branch]
	if ( !isSky )
	{
		resultColor = TextureScene[pixelCoord].xyz;

		float ssaoFogImpact = 1;
		{
			const float ssaoValue = max( vFogImpactParams2.x, DecodeGlobalShadowBufferSSAO( ShadowSurface[pixelCoord] ) );
			const float ssaoImpactFactor = lerp( vFogImpactParams.z, vFogImpactParams.w, saturate( (length(worldSpacePosition - cameraPosition.xyz) - vFogImpactParams.x) / (vFogImpactParams.y - vFogImpactParams.x) ) );
			ssaoFogImpact = lerp( 1, ssaoValue, ssaoImpactFactor );
		}

		resultColor = ApplyFogWithSSAOImpact( resultColor, isSky, false, worldSpacePosition, ssaoFogImpact ).xyz;
	}

	return float4 ( resultColor.xyz, 1 );
}
#endif
