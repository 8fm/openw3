#include "postfx_common.fx"

#define vTexCoordTransformColor VSC_Custom_0
#define vTexCoordTransformDepth VSC_Custom_1
#define vViewSpaceVecTransform 	VSC_Custom_2
#define vCamForwardVS 			VSC_Custom_3
#define vCamRightVS 			VSC_Custom_4
#define vCamUpVS 				VSC_Custom_5

SamplerState s_TextureColor		: register( s0 );
Texture2D    t_TextureColor		: register( t0 );

SamplerState s_TextureDepth     : register( s1 );
Texture2D	 t_TextureDepth		: register( t1 );

#define vFlareParams0			PSC_Custom_0
#define vFlarePosition			PSC_Custom_1
#define vFlareBloomColor		PSC_Custom_2
#define vAspectRatio			PSC_Custom_3
#define vTexelSize				PSC_Custom_9
#define vPointSpotParams		PSC_Custom_5
#define vCameraPosAndDistance	PSC_Custom_6
#define vWorldSpacePos			PSC_Custom_7
#define vSpotDirection			PSC_Custom_8
#define vSimpleTonemapping  	PSC_Custom_4


struct VS_INPUT
{
	float4 pos		   : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos	   		: SYS_POSITION;
	float2 coordColor  	: TEXCOORD0;
	float2 coordDepth  	: TEXCOORD1;
	float2 normalized	: TEXCOORD2;
	float3 worldVec		: TEXCOORD3;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   		= i.pos;
	o.normalized	= i.pos.xy * float2( 0.5f, -0.5f ) + float2( 0.5f, 0.5f );
	o.coordColor 	= ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformColor );
	o.coordDepth 	= ApplyTexCoordTransform( i.pos.xy, vTexCoordTransformDepth );
	float3 viewVec  = ApplyViewSpaceVecTransform( i.pos.xy, vViewSpaceVecTransform );
	o.worldVec		= mul( viewVec, float3x3( vCamRightVS.xyz, vCamUpVS.xyz, vCamForwardVS.xyz ) );
	
	return o;
}

#endif

#ifdef PIXELSHADER


float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	float4 ret = 0;

	// Normalized UV spans from 0 to 1 over whole effect
	const float2 normalizedCoord = i.normalized.xy;

	// Calculate edge mask
	float edgeMask = 1.0f - normalizedCoord.x * (1.0f - normalizedCoord.x) * normalizedCoord.y * (1.0f - normalizedCoord.y) * 8.0f;
	edgeMask = edgeMask * edgeMask * edgeMask * edgeMask;
	
	float4 offset = float4( vTexelSize.x, vTexelSize.y, 0.0f, 0.0f);

    // Downsample scene color
    float3 sceneColor0 = t_TextureColor.Sample( s_TextureColor, i.coordColor + offset.zw ).xyz;
    float3 sceneColor1 = t_TextureColor.Sample( s_TextureColor, i.coordColor + offset.xz ).xyz;
    float3 sceneColor2 = t_TextureColor.Sample( s_TextureColor, i.coordColor + offset.zy ).xyz;
    float3 sceneColor3 = t_TextureColor.Sample( s_TextureColor, i.coordColor + offset.xy ).xyz;
    float3 downsampledColor = 0.25f * ( sceneColor0.xyz + sceneColor1.xyz + sceneColor2.xyz + sceneColor3.xyz );

    // Gather scene depths
    float sceneDepth0 = t_TextureDepth.Sample( s_TextureDepth, i.coordDepth + offset.zw).x;
    float sceneDepth1 = t_TextureDepth.Sample( s_TextureDepth, i.coordDepth + offset.xz).x;
    float sceneDepth2 = t_TextureDepth.Sample( s_TextureDepth, i.coordDepth + offset.zy).x;
    float sceneDepth3 = t_TextureDepth.Sample( s_TextureDepth, i.coordDepth + offset.xy).x;
    float4 sceneDepths = float4( sceneDepth0, sceneDepth1, sceneDepth2, sceneDepth3 );    

	// Only bloom colors over BloomThreshold
	float luminance = max( dot( downsampledColor, RGB_LUMINANCE_WEIGHTS_LINEAR_FlareGrab), 6.10352e-5 );
	float adjustedLuminance = max( luminance - vFlareParams0.y, 0.0f );
	float3 bloomColor = vFlareParams0.x * downsampledColor / luminance * adjustedLuminance * 2.0f;
	
#if POINT_LIGHT_SHAFTS
	float3 WorldPosition00 = (i.worldVec * sceneDepth0).xzy + vCameraPosAndDistance.xyz;
	float3 WorldPosition10 = (i.worldVec * sceneDepth1).xzy + vCameraPosAndDistance.xyz;
	float3 WorldPosition01 = (i.worldVec * sceneDepth2).xzy + vCameraPosAndDistance.xyz;
	float3 WorldPosition11 = (i.worldVec * sceneDepth3).xzy + vCameraPosAndDistance.xyz;

	float3 WorldLightVector00 = WorldPosition00 - vWorldSpacePos.xyz;
	float3 WorldLightVector10 = WorldPosition10 - vWorldSpacePos.xyz;
	float3 WorldLightVector01 = WorldPosition01 - vWorldSpacePos.xyz;
	float3 WorldLightVector11 = WorldPosition11 - vWorldSpacePos.xyz;

	// 1 at the radius of the light, 0 at the center of the light
	float4 Distances = float4(
		length(WorldLightVector00),
		length(WorldLightVector10),
		length(WorldLightVector01),
		length(WorldLightVector11));

	float4 NormalizedDistances = Distances / vPointSpotParams.z;
	// Setup a mask that only allows occlusion between the camera and the light's center
	// This prevents pixels behind the light from occluding light shafts
	float InFrontOfLightMask = 1.0f - saturate((vCameraPosAndDistance.w - length(WorldPosition00 - vCameraPosAndDistance.xyz)) / vPointSpotParams.z);
	// Filter the occlusion mask instead of the depths
	float OcclusionMask = max(dot(saturate(NormalizedDistances), .25f), InFrontOfLightMask);

	float SpotLightAttenuation = 1.0f;
#if SPOT_LIGHT_SHAFTS
	
	// Setup a mask that only allows occlusion or blooming from pixels in the spotlight's cone
	float4 SpotDotProducts = float4(
		dot(WorldLightVector00, vSpotDirection.xyz),
		dot(WorldLightVector10, vSpotDirection.xyz),
		dot(WorldLightVector01, vSpotDirection.xyz),
		dot(WorldLightVector11, vSpotDirection.xyz));

	// Filter the spotlight mask instead of the depths
	SpotLightAttenuation = pow(dot(saturate((SpotDotProducts / Distances - vPointSpotParams.wwww) * vPointSpotParams.xxxx), .25f), 2.0f );

#endif
	
	// Only allow blooming from pixels within 1/3rd of the light's radius
	//@todo - this excludes translucency which may be composing the majority of the light's emissive component
	float BlurOriginDistanceMask = 1.0f - dot(saturate(NormalizedDistances * 3.0f), .25f);
	// Calculate bloom color with masks applied
	ret.rgb = bloomColor * (1.0f - edgeMask) * (1.0f - vPointSpotParams.y) * BlurOriginDistanceMask * BlurOriginDistanceMask * SpotLightAttenuation;

#else
	// Filter the occlusion mask instead of the depths
	float invOcclusionDepthRange = vFlareParams0.z;
	float occlusionMask = dot( saturate( sceneDepths * invOcclusionDepthRange), 0.25f );

	// Only allow bloom from pixels whose depth are in the far half of OcclusionDepthRange
	float bloomDistanceMask = dot( saturate(( sceneDepths - 0.5f / invOcclusionDepthRange) * invOcclusionDepthRange), 0.25f );

	// Setup a mask that is 0 at TextureSpaceBlurOrigin and increases to 1 over distance
	const float2 vTextureSpaceBlurOrigin = vFlarePosition.xy;
	float blurOriginDistanceMask = 1.0f - saturate( length( (vTextureSpaceBlurOrigin.xy - normalizedCoord) * vAspectRatio.xy ) * 5.0f / vFlareParams0.w );

	// Calculate bloom color with masks applied
	ret.rgb = bloomColor * bloomDistanceMask * sqrt(sqrt(1.0f - edgeMask)) * (1.0f - vPointSpotParams.y) * blurOriginDistanceMask * blurOriginDistanceMask;
#endif

	return ret;
}

#endif
