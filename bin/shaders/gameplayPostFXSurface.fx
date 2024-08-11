#include "postfx_common.fx"
#include "include_constants.fx"

#define vTexCoordTransform VSC_Custom_0
#define vTexCoordClamp     PSC_Custom_0

#define burn 1
#define frost 0

Texture2D				tTextureColor : register (t0);
SamplerState			sTextureColor : register (s0);

// array with all base surface flow textures
TEXTURE2D_ARRAY			tTextureFlow : register (t4);
SamplerState			sTextureFlow : register (s4);

START_CB( frostGroups, 5 )
	float4 effectData[ 16 ];
END_CB

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
	o.coord = ApplyTexCoordTransform( i.pos.xy, vTexCoordTransform );

	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{	
	
	const int2 pixelCoord = (int2)i.pos.xy;
	float2 uv = i.pos.xy * PSC_ViewportSize.zw;
	
	float3 sceneColor = tTextureColor.Sample( sTextureColor, uv.xy ).xyz;
	float4 result = float4( sceneColor.xyz, 1 );

	float depth = SYS_SAMPLE( PSSMP_SceneDepth, uv ).x;
	
	//early out
	if( IsSkyByProjectedDepthRevProjAware( depth ) )
	{
		return result;
	}
		
	float3 normal = float3(0.0f, 0.0f, 1.0f);
	
	// TODO
	const float3 frostBaseColor = float3( 0.89f, 0.9f, 1.00f );
	const float3 burnBaseColor = float3( 100.0f, 10.3f, 0.3f );

	float3 surfaceBaseColor = float3(0,0,0);

	float3 WorldPosition = float3(0.0f, 0.0f, 0.0f);

									//V  //H
	float2 uvScale = float2( 0.5f, 0.3f );	
	
	// get world pos for uv mapping
	WorldPosition = PositionFromDepthRevProjAware( depth, i.pos.xy );
	
	//early out
	const float3 viewVector = PSC_CameraPosition.xyz - WorldPosition.xyz;
	if( length( viewVector ) > 35.0f )
	return result;
	
	const int textureArrayIndexOffset = 2;
	const float frostContrast = 3.6f;
	const float frostAmount = 1.6f;
	const float burnContrast = 0.5f;
	const float burnAmount = 1.0f;
	const float normalGain = 2.0f;

	float	resultingFrostBlend		= 0.0f;
	float	resultingBurnBlend		= 0.0f;
	float	resultingEffectBlend	= 0.0f;
	int		resultingEffectType		= 0;

	for(int it=0; it<PSC_Custom_1.x; it++)	
	{			
		float effectRangeBlend = 0.0f;		
		float effectFalloff = 0.0f;
		float effectAmount = 0.0f;
		float effectContrast = 0.0f;
		float effectBurnInfluence = 0.0f;
		float effectFrostInfluence = 0.0f;

		float3 effectPivot = effectData[it].xyz;
		//effect type and effect range are encoded in effectData[it].w
		float effectRange = clamp( effectData[it].w % 1000 * frac( effectData[it].w ), 0.0001f, 100.0f );
		//effectType: 1 -> burn, 0 -> frost
		float effectType = ((int)floor( effectData[it].w/1000.0f )) - 1 ;

		if( effectType == burn )
		{
			effectAmount = burnAmount;
			effectContrast = burnContrast;
			effectBurnInfluence = 1.0f;
		}
		else if( effectType == frost )
		{
			effectAmount = frostAmount;
			effectContrast = frostContrast;
			effectFrostInfluence = 1.0f;
		}

		effectRangeBlend = 1.0f - saturate( length( WorldPosition - effectPivot ) / effectRange );
		effectFalloff = saturate( effectAmount * pow( effectRangeBlend, effectContrast ) );

		resultingBurnBlend = max( resultingBurnBlend, effectFalloff * effectBurnInfluence );
		resultingFrostBlend = max( resultingFrostBlend, effectFalloff * effectFrostInfluence );
	}
	
	resultingFrostBlend = saturate( resultingFrostBlend );
	resultingBurnBlend = saturate( resultingBurnBlend  );

	if( resultingFrostBlend < resultingBurnBlend ) 
	{
		resultingEffectType = burn;
		resultingEffectBlend = resultingBurnBlend - resultingFrostBlend;
	}
	else
	{
		resultingEffectType = frost;
		resultingEffectBlend = resultingFrostBlend - resultingBurnBlend;
	}
	
	[branch]
	if( (resultingFrostBlend + resultingBurnBlend) < 0.01f ) 
	{			
		return float4( sceneColor.xyz, 1.0f );
	}

	//Reconstructing normal
	float pixelOffset = 1.0f;

	//Sample A
	const float2 offsetA = float2( pixelOffset, 0.0f );
	const float depthA = SYS_SAMPLE( PSSMP_SceneDepth, uv.xy + offsetA * PSC_ViewportSize.zw ).x;
	float3 a = PositionFromDepthRevProjAware( depthA, i.pos.xy + offsetA );

	//Sample B
	const float2 offsetB = float2( -1.0f * pixelOffset, 0.0f );
	const float depthB = SYS_SAMPLE( PSSMP_SceneDepth, uv.xy + offsetB * PSC_ViewportSize.zw ).x;
	float3 b = PositionFromDepthRevProjAware( depthB, i.pos.xy + offsetB );

	//Sample C
	const float2 offsetC = float2( 0.0f, pixelOffset );
	const float depthC = SYS_SAMPLE( PSSMP_SceneDepth, uv.xy + offsetC * PSC_ViewportSize.zw ).x;
	float3 c = PositionFromDepthRevProjAware( depthC, i.pos.xy + offsetC );

	//Sample D
	const float2 offsetD = float2( 0.0f, -1.0f * pixelOffset );
	const float depthD = SYS_SAMPLE( PSSMP_SceneDepth, uv.xy + offsetD * PSC_ViewportSize.zw ).x;
	float3 d = PositionFromDepthRevProjAware( depthD, i.pos.xy + offsetD );
			
	normal = normalize( cross( a - b, d - c ) );

	//Triplanar mapping
	float2 triplanarUV = float2( 0.0f, 0.0f );

	float3 normalAbs = abs( normal );
	if( normalAbs.z > 0.707f )
	{
		triplanarUV = frac( WorldPosition.xy * uvScale.y );
	}
	else
	{
		if( normalAbs.x > 0.8f )
		{
			triplanarUV = frac( WorldPosition.yz * uvScale.x );
		}
		else
		{
			triplanarUV = frac( WorldPosition.xz * uvScale.x );
		}
	}

		float4 effectTexture = tTextureFlow.Sample( sTextureFlow, float3( triplanarUV.xy, textureArrayIndexOffset + resultingEffectType ) );
		float3 effectTextureNormal = 2.0f * ( effectTexture.xyz - 0.5f);

		// calc world space normals
		float3 effectWorldSpaceNormal = normal.xyz;

		if( normalAbs.z > 0.5f ) 
		{
			effectWorldSpaceNormal.xy = normalGain * effectTextureNormal * sign( normal.z );
		}
		else
		{
			if( normalAbs.x > normalAbs.y )
			{			
				effectWorldSpaceNormal.x += normalGain*effectTextureNormal.x;
				effectWorldSpaceNormal.y = 0.3f;
				effectWorldSpaceNormal.z += normalGain*effectTextureNormal.y;
			}
			else
			{	
				effectWorldSpaceNormal.x = 0.3f;
				effectWorldSpaceNormal.y += normalGain*effectTextureNormal.x;
				effectWorldSpaceNormal.z += normalGain*effectTextureNormal.y;
			}
		}

		effectWorldSpaceNormal = normalize( effectWorldSpaceNormal );

		if ( resultingEffectType ==  frost )
		{
			float3 ambient = clamp( pow(pbrSimpleParams0.y, 4.0f), 0.1f, 1.0f) * lerp( 0.4f, 1.0f, saturate( PSC_GlobalLightDir.w ) );
			float3 limitedLightDir = normalize( float3( PSC_GlobalLightDir.x, PSC_GlobalLightDir.y, sign(PSC_GlobalLightDir.z)*clamp(abs(PSC_GlobalLightDir.z), 0.15f, 0.4f) ) );

			float globalLight = lerp( 0.4f, 5.0f, pow( saturate( dot( limitedLightDir.xyz, effectWorldSpaceNormal.xyz ) ), 3.0f ) );			

			float3 frostFullyLit = 3*ambient*( globalLight ) * ( lerp( float3(0.01f,0.01f,0.15f), 3.0f*frostBaseColor.xyz, saturate(pow(saturate(resultingFrostBlend-0.15f), 4.0f)) ) );
			// hack get some color from manipulated normalmap
			frostFullyLit.xy *= effectTexture.z;

			float effectMask = saturate( pow(( 1.0f - effectTexture.w ), 2.0f ) + resultingEffectBlend - 1.0f );

			return float4( lerp( sceneColor.xyz, frostFullyLit, effectMask ), 0.0f );
		}
		else if ( resultingEffectType ==  burn )
		{
			float effectMask = saturate( pow( ( 1.0f - effectTexture.w ), 8.0f ) + resultingEffectBlend - 1.0f );
			return float4( lerp( sceneColor.xyz, burnBaseColor, effectMask ), 0.0f );
		}
		else
		{
			return float4( sceneColor.xyz, 0.0f );
		}
}
#endif
