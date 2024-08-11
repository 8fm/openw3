#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"
#include "include_sharedConsts.fx"
#include "include_constants.fx"

TEXTURE2D<float> DepthTexture				: register(t0);
TEXTURE2D<float4> GBufferSurface0			: register(t1);
TEXTURE2D<float4> GBufferSurface1			: register(t2);
TEXTURE2D<float4> GBufferSurface2			: register(t3);
Texture2D<float4> ShadowSurface				: register(t4);
#if IS_CAMERA_LIGHTS_MODIFIER_ALLOWED
	Texture2D<uint2> StencilTexture			: register(t5);
#endif

#if IS_PURE_DEFERRED_LIGHT_SPOT
#define fTargetRadAngle			VSC_Custom_0.x
#endif

#define iLightIndex				((int)PSC_Custom_0.x)

struct VS_INPUT
{
	float4 pos : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos : SYS_POSITION;
};

#ifdef VERTEXSHADER
VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	float3 pos = i.pos.xyz;
	
#if IS_PURE_DEFERRED_LIGHT_SPOT
	// Apply cone angle
	{
		const float curAngle		= acos( i.pos.y );
		const float targetAngle		= fTargetRadAngle * curAngle / (0.5f * PI);
		const float dist			= length( pos.xyz );

		if ( dist > 0 )
		{
			pos.y = cos( targetAngle );		
			pos.xz = sin( targetAngle ) * pos.xz / max( 0.001, dist );
			pos = dist * normalize( pos );
		}
		else
		{
			pos.y = min( 0, cos( targetAngle ) );		
		}
	}
#endif

	o.pos = mul( mul( float4 ( pos, 1 ), VSC_LocalToWorld ), VSC_WorldToScreen );
	return o;
}
#endif

#ifdef PIXELSHADER
float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const int2 pixelCoord = i.pos.xy;
	const float zw = DepthTexture[ pixelCoord ].x;
	const float3 worldSpacePosition = PositionFromDepthRevProjAware(zw,pixelCoord);
	const LightParams lightParams = lights[iLightIndex];
	const float3 lightVec = lightParams.positionAndRadius.xyz - worldSpacePosition;
	const float lightDist = length( lightVec );

	float3 result = 0;
	[branch]
	if ( lightDist < lightParams.positionAndRadius.w )
	{
		float4 shadowSurf	= ShadowSurface[pixelCoord];

		float4 gbuff0		= GBufferSurface0[ pixelCoord ];
		float4 gbuff1		= GBufferSurface1[ pixelCoord ];	
		float4 gbuff2		= GBufferSurface2[ pixelCoord ];	
			
		const float interiorFactor			= DecodeGlobalShadowBufferInteriorFactor( shadowSurf );
		float attenuation = CalcLightAttenuation( iLightIndex, worldSpacePosition, lightVec );

		attenuation *= GetLocalLightsAttenuationInteriorScale( lightParams.colorAndType.w, interiorFactor );
		
		[branch]
		if ( attenuation > 0 )
		{	
			#if IS_CAMERA_LIGHTS_MODIFIER_ALLOWED
			{
				const float lightsCharacterModifier = 0 != ( GetStencilValue( StencilTexture[ pixelCoord ] ) & LC_Characters ) ? 1 : localLightsExtraParams.x;
				attenuation *= IsLightFlagsCharacterModifierEnabled( lightParams.colorAndType.w ) ? lightsCharacterModifier : 1;
			}
			#endif

			const float3 albedo					= pow( gbuff0.xyz, 2.2 );
			const float3 specularity			= pow( gbuff2.xyz, 2.2 );
			const float  roughness				= gbuff1.w;
			const float  translucency			= gbuff0.w;
			const float3 N						= normalize( gbuff1.xyz - 0.5 );
			const float3 L						= lightVec / lightDist;
			const float3 V						= normalize( cameraPosition.xyz - worldSpacePosition.xyz );
		
			#ifdef __PSSL__
				const SCompileInLightingParams extraLightingParams;
			#else
				const SCompileInLightingParams extraLightingParams = (SCompileInLightingParams)0;
			#endif

			const float  envprobe_diffuse_scale		= pbrSimpleParams0.z;
			const float  envprobe_specular_scale	= pbrSimpleParams0.w;
			
			float3 diffuse = envprobe_diffuse_scale * attenuation * (DiffuseLightingPBRPipeline( L, N, V, translucency, specularity, roughness, extraLightingParams ).xyz * lightParams.colorAndType.xyz);
			float3 specular = envprobe_specular_scale * attenuation * (SpecularLightingPBRPipeline( L, N, V, specularity, roughness, translucency, extraLightingParams ).xyz * lightParams.colorAndType.xyz);

			result = albedo * diffuse + specular;

			// apply AO
			{
				const float  dimmerFactor			= DecodeGlobalShadowBufferDimmers( shadowSurf );
				const float3 ssaoMod				= ModulateSSAOByTranslucency( ProcessSampledSSAO( DecodeGlobalShadowBufferSSAO( shadowSurf ) ), translucency );
				const float3 AO_probe				= GetProbesSSAOScaleByDimmerFactor( dimmerFactor ) * ssaoMod;
				const float3 AO_nonProbe			= ssaoParams.x * ssaoMod + ssaoParams.y;
				result *= AO_nonProbe;
			}
		}

		/*
		#if IS_PURE_DEFERRED_LIGHT_SHADOWED
		result *= float3 ( 1, 0, 0 );
		#endif
		*/
	}

	return float4 ( result, 1 );
}
#endif
