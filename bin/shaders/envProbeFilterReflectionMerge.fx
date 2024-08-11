#include "postfx_common.fx"
#include "include_dimmers.fx"


Texture2D		t_TextureBase		: register( t0 );
Texture2D		t_TextureAlbedo		: register( t1 );
Texture2D		t_TextureNormal		: register( t2 );
Texture2D		t_TextureDepthAndSky: register( t3 );
Texture2D		t_TextureAmbient	: register( t4 );

SamplerState 	s_ClampLinearMip	: register(s0);

#define iSegmentSize			((int)PSC_Custom_0.x)
#define iMargin					((int)PSC_Custom_0.y)
#define cAmbient				PSC_Custom_1.xyz
#define colorSceneMul			PSC_Custom_2.xyz
#define colorSceneAdd			PSC_Custom_3.xyz
#define colorSkyMul				PSC_Custom_4.xyz
#define colorSkyAdd				PSC_Custom_5.xyz
#define colorRemapOffset		PSC_Custom_6.x
#define colorRemapStrength		PSC_Custom_6.y
#define colorRemapClamp			PSC_Custom_6.z
#define DEFINE_CUSTOM_PARAMS_TABLE	float4 customParamsTable[CUBE_ARRAY_CAPACITY] = { PSC_Custom_7, PSC_Custom_8, PSC_Custom_9, PSC_Custom_10, PSC_Custom_11, PSC_Custom_12, PSC_Custom_13 }
#define tCustomParamsTable			customParamsTable

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos			: SYS_POSITION;
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
float4 ps_main( float4 vpos : SYS_POSITION ) : SYS_TARGET_OUTPUT0
{
	const int2 pixelCoord = vpos.xy;
	const int2 segmentIndex = pixelCoord / iSegmentSize;
	const int2 segmentCoord = pixelCoord - segmentIndex * iSegmentSize;
	const float2 paraboloidUV = (segmentCoord - iMargin + 0.5) / (iSegmentSize - 2 * iMargin);
	const float4 cubeDirData = ParaboloidToCube( paraboloidUV, segmentIndex.x );
	
	float3 result = 0;

	bool shouldShade = true;
	{
		float2 uv = paraboloidUV * 2 - 1;
		shouldShade = length( uv ) <= 1 + 2.0 / (iSegmentSize - 2 * iMargin);
	}

	const float2 depthAndSky = t_TextureDepthAndSky[pixelCoord].xy;
	const float depth = depthAndSky.x;
	const bool isSky = 1 == depthAndSky.y;
	if ( depth <= 0 )
	{
		shouldShade = false;
	}
	
	[branch]
	if ( shouldShade )
	{
		const float3 sceneColor = t_TextureBase[pixelCoord].xyz;
		const float3 albedo = t_TextureAlbedo[pixelCoord].xyz;
		const float3 normal = normalize( t_TextureNormal[pixelCoord].xyz * 2 - 1 );
		
		const float3 probePos = commonEnvProbeParams[segmentIndex.y].probePos.xyz;
		const float3 worldSpacePosition = probePos + cubeDirData.xyz * depth;		
		
		result = sceneColor;
		
		DEFINE_CUSTOM_PARAMS_TABLE;
		const float4 currCustomParams = tCustomParamsTable[segmentIndex.y];
			
		if ( !isSky )
		{			
			float dimmerFactor = currCustomParams.x;
			result.xyz += albedo * cAmbient * GetProbesSSAOScaleByDimmerFactor( dimmerFactor ) * CalcEnvProbesDistantBrightnessByHorizDist( length( worldSpacePosition.xy - probePos.xy ) ) * SampleEnvProbeAmbient( t_TextureAmbient, segmentIndex.y, s_ClampLinearMip, normal, AMBIENT_CUBE_NUM_MIPS-1 );
		}

		// apply fog
		{
			SFogData fogData = CalculateFogFullCustomCamera( isSky, false, worldSpacePosition, probePos, 0 );
			fogData.paramsFog.w = currCustomParams.y * lerp( fogData.paramsFog.w * fogData.paramsFog.w, fogData.paramsFog.w, currCustomParams.y );
			fogData.paramsAerial.w = currCustomParams.y * fogData.paramsAerial.w; //< let's be cheaper here. aerial is meant to be subtle.
			result = ApplyFogDataFull( result.xyz, fogData );
		}
			
		result.xyz *= isSky ? colorSkyMul.xyz : colorSceneMul.xyz;
		result.xyz += isSky ? colorSkyAdd.xyz : colorSceneAdd.xyz;

		{
			const float currLum = dot( RGB_LUMINANCE_WEIGHTS_LINEAR, result.xyz );
			const float offLum = currLum - colorRemapOffset;
			const float targetLum = min( (offLum > 0 ? colorRemapOffset + offLum / (1 + colorRemapStrength * offLum) : currLum), colorRemapClamp );
			result.xyz *= targetLum / max( 0.00001, currLum );
		}

		result.xyz = lerp( result.xyz, sceneColor, currCustomParams.z );
	}

	return float4 ( result.xyz, 1 );
}
#endif
