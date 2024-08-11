#include "postfx_common.fx"
#include "include_constants.fx"

#include "common.fx"

#define vTexCoordTransformScene		VSC_Custom_0
#define vTexCoordClamp				PSC_Custom_0

#define flowIndex_rain 0
#define flowIndex_splash 1
#define flowIndex_ice 2
#define flowIndex_burn 3

Texture2D				tTextureColor : register (t0);
SamplerState			sClampNoMip : register (s0);

// array with all base surface flow textures
TEXTURE2D_ARRAY			tTextureFlow : register (t4);
SamplerState			sTextureFlow : register (s4);

Texture2D				tUnderwater : register (t6);
SamplerState			sUnderwater : register (s6);

float applyRainMask( float mask, float fracTime )
{
	float d = lerp(-1,1, saturate( lerp( 0.0f, 5.0f, fracTime ) * mask ) );
	d = d - 1.0f * d*d;
	d *= ( 1.0f - fracTime );

	return saturate( d );
}

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

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{	
	const uint2 pixelCoord = (uint2)i.pos.xy;
	float4 result = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float effectStrength = saturate(PSC_Custom_2.x);
			
	float3 sceneColor = tTextureColor.Sample( sClampNoMip, i.coordScene.xy ).xyz;
	float depth = SYS_SAMPLE_TEXEL( PSSMP_SceneDepth, pixelCoord ).x;
	float3 normal = float3(0.0f,0.0f,1.0f);
	float3 wPos = 0;

	// get world pos
	wPos = PositionFromDepthRevProjAware( depth, i.pos.xy );
	float distToCamera = length(wPos.xyz-PSC_CameraPosition.xyz);

	if( distToCamera > 40.0f ) 
	{
		return float4( sceneColor.xyz, 1.0f );
	}
	
	float resultFade = 0.0f; 					
	
	// interior cut
	resultFade = CalculateVolumeCutByPixelCoord( pixelCoord, wPos );

	if( resultFade < 0.01f )
	{
		return float4( sceneColor.xyz, 1.0f );
	}
	
	// cancel effect if underwater
	effectStrength *= ( 1.0f - SAMPLE_LEVEL( tUnderwater, sClampNoMip, i.coordScene.xy, 0 ).x );	

	if( effectStrength < 0.01f )
	{
		return float4( sceneColor.xyz, 1.0f );
	}

	float2 verticalWorldUV = 0;	
	float2 horizontalWorldUV = 0;
							 //V  //H
	float2 uvScale = float2( 2.0f, 4.0f );
	float2 timeScale = float2(0.0f, 0.065f * PSC_TimeVector.x);
	
	/////// 
		float3 N_flat = normal;			

		float pixelOffset = 4.5f;

		const float2 pixelCoord2 = pixelCoord + float2(pixelOffset, 0.0f);
		float3 a = PositionFromDepthRevProjAware( SYS_SAMPLE_TEXEL( PSSMP_SceneDepth, pixelCoord2 ).x, pixelCoord2 );

		const float2 pixelCoord3 = pixelCoord + float2(-pixelOffset, 0.0f);
		float3 b = PositionFromDepthRevProjAware( SYS_SAMPLE_TEXEL( PSSMP_SceneDepth, pixelCoord3 ).x, pixelCoord3 );

		const float2 pixelCoord4 = pixelCoord + float2(0.0f, pixelOffset);
		float3 c = PositionFromDepthRevProjAware( SYS_SAMPLE_TEXEL( PSSMP_SceneDepth, pixelCoord4 ).x, pixelCoord4 );

		const float2 pixelCoord5 = pixelCoord + float2(0.0f, -1.0f*pixelOffset);
		float3 d = PositionFromDepthRevProjAware( SYS_SAMPLE_TEXEL( PSSMP_SceneDepth, pixelCoord5 ).x, pixelCoord5 );

		N_flat = ( cross( a - b, d - c ) );
		normal = normalize(N_flat);
	////////////////
		
	float3 noiseTexV = float3( 0.0f, 0.0f, 3.0f );				
	float resultingVerticalFlow = 0.0f;
	
	// go horizontal
	if( normal.z > 0.7f )
	{
		noiseTexV.x = wPos.x;
		noiseTexV.y = wPos.y;
	}
	else
	// go vertical
	{
		if( normal.z > -0.6f )
		{
			// do a pseudo cylindrycal map
			noiseTexV.y = wPos.z;
			float2 ch = (0,0);
			ch.x = ((int)(abs(normal.x) * 5)) * 0.2f;
			ch.y = ((int)(abs(normal.y) * 5)) * 0.2f;

			noiseTexV.x = (wPos.x) * pow((abs(ch.y)),0.5f);
			float add = (wPos.y) * pow( abs(ch.x),0.5f);
			if ((normal.x * normal.y) > 0) 
			{
				add *= (-1); 
			}
				noiseTexV.x += add;
			}
			else 
			{
				resultFade = 0.0f;
			}
			resultingVerticalFlow = 1.0f;				
	}

	verticalWorldUV.xy = noiseTexV.xy;

	verticalWorldUV.xy *= uvScale.x;

	verticalWorldUV += 1.2f*timeScale;	
	
	horizontalWorldUV.xy = wPos.xy;
	horizontalWorldUV.xy *= uvScale.y;			
	
	float resultingHorizontalFlow = (1.0f - resultingVerticalFlow);	

	float4 horizontalMask = tTextureFlow.Sample( sTextureFlow, float3( 0.5f*horizontalWorldUV.xy, flowIndex_splash ) );
	
	// noise
	float noiseFade = tTextureFlow.Sample( sTextureFlow, float3( 0.05f*verticalWorldUV.x + 0.5f*timeScale.y, 0.05f*verticalWorldUV.y + 0.8f*timeScale.y, flowIndex_rain ) ).z;
	float2 timescaleNoise = float2(0.05f, 0.2f );
	float noiseUV = tTextureFlow.Sample( sTextureFlow, float3( 0.1f*verticalWorldUV.x + timescaleNoise.x*timeScale.y, 0.02f*verticalWorldUV.y + timescaleNoise.y*timeScale.y, flowIndex_rain ) ).z;

	// circle thickness
	float scale = 400.0f;
	//randomize time start on each tile
	float rand_time = frac(sin(dot(floor(fmod(noiseTexV.xy * 2.0f , 100.0f))/100 ,float2(12.9898f,78.233f))) * 43758.5453f) + 0.5f * PSC_TimeVector.x;

	float horizontalRainMask = (saturate(( scale * - pow((horizontalMask.w - frac( rand_time ) ) , 2.0f) + 1.0f)) + saturate(( scale * - pow((horizontalMask.w - frac( rand_time ) + 0.99f) , 2.0f) + 1.0f)) )* horizontalMask.z; 
	horizontalRainMask = saturate(horizontalRainMask * depth * 30.0f) * saturate(floor((noiseFade+0.52f) * 3)/3);
	
	float4 horizontalNormal = float4((horizontalMask.xy - 0.5f) * 2.0f * horizontalRainMask, 1.0f, 0.0f);

	// STRING MASK	
	float4 verticalNormal = tTextureFlow.Sample( sTextureFlow, float3( verticalWorldUV.xy * lerp( 0.985f, 1.08f, noiseUV ) + timeScale, flowIndex_rain ) );			
	verticalNormal.z = 0.0f;
	
	float4 horizontalNormalModDecompressed = normalize( horizontalNormal );
	float4 verticalNormalModDecompressed = normalize( 2.0f*(verticalNormal - 0.5f) );
	
	const float depthRevProjAware = TransformDepthRevProjAware( depth );
	
	float2 horizontalRef = horizontalNormalModDecompressed.xy;
	horizontalRef *= (1.0f-depthRevProjAware);

	float2 verticalRef = verticalNormalModDecompressed.xy;
	verticalRef *= 1.9f*( (verticalNormal.xy) * (noiseFade));	
	verticalRef *= (1.0f-depthRevProjAware);
	
	float3 sceneColorHorizontalRef = tTextureColor.Sample( sClampNoMip, i.coordScene.xy + horizontalRef.xy * 0.2f ).xyz;	
	float3 sceneColorVerticalRef = tTextureColor.Sample( sClampNoMip, i.coordScene.xy + verticalRef * 0.2f ).xyz; 
	
	sceneColorHorizontalRef = lerp(sceneColorHorizontalRef , sceneColorHorizontalRef * 1.6f + 0.02f , horizontalRainMask );
	sceneColorVerticalRef = lerp(sceneColorVerticalRef , sceneColorVerticalRef * 6.2f + 0.02f, verticalNormal.w * noiseFade);
				
	
	result.xyz = lerp( sceneColor, sceneColorVerticalRef, effectStrength*saturate( resultingVerticalFlow ) );
	result.xyz = lerp( result.xyz, sceneColorHorizontalRef, effectStrength*resultingHorizontalFlow );
	
	// dampen the effect based on artist setup
	resultFade *= saturate(1.0f + PSC_WaterShadingParams.w);
			
	// fade the result based on min distances beteween volumes
	result.xyz = lerp( sceneColor, result.xyz, resultFade );	

	


	return result;
}
#endif


