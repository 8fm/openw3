#if MSAA_NUM_SAMPLES > 1
# error Not supported at this moment
#endif

#define DEFERRED

#if !IS_ENVPROBE_GEN
#define ENABLE_BILLBOARDS_LIGHT_BLEED
#define BILLBOARDS_LIGHT_BLEED_WRAP
#endif

#include "common.fx"
#include "commonCS.fx"
#include "include_constants.fx"
#include "include_dimmers.fx"

#define ENABLE_TRANSPRERENT_LIST_OUTPUT			!IS_ENVPROBE_GEN
#define ENABLE_SSAO								!IS_ENVPROBE_GEN
#define ENABLE_LIGHTS_INTERIOR_EXTERIOR			!IS_ENVPROBE_GEN
#define ENABLE_ENVPROBE_INTERIOR_EXTERIOR		!IS_ENVPROBE_GEN

#define ENABLE_DIMMER_APPLY_CODE				1

#if IS_PURE_DEFERRED_COMPUTE
# define ENABLE_LIGHTS_CODE						0
# define ENABLE_LIGHT_APPLY_CODE				0
# define ENABLE_CAMERA_LIGHTS_MODIFIER			0
#else
# define ENABLE_LIGHTS_CODE						1
# define ENABLE_LIGHT_APPLY_CODE				1
#if IS_ENVPROBE_GEN
# define ENABLE_CAMERA_LIGHTS_MODIFIER			0
#else
# define ENABLE_CAMERA_LIGHTS_MODIFIER			1
#endif
#endif

RW_TEXTURE2D<float4> OutputTexture		: register(u0);

#if ENABLE_TRANSPRERENT_LIST_OUTPUT
	RW_BYTEBUFFER LightListTransparentOutput	: register(u2);
#endif

#if IS_ENVPROBE_GEN
	TEXTURE2D_ARRAY<float> DepthTexture			: register(t0);
	TEXTURE2D_ARRAY<float4> GBufferSurface0		: register(t1);
	TEXTURE2D_ARRAY<float4> GBufferSurface1		: register(t2);
#else
	Texture2D<float> DepthTexture				: register(t0);
	Texture2D<float4> GBufferSurface0			: register(t1);
	Texture2D<float4> GBufferSurface1			: register(t2);
#endif

#if !IS_ENVPROBE_GEN
	Texture2D<uint2> StencilTexture			: register(t3);
	Texture2D<float4> GBufferSurface2		: register(t4);
#endif

Texture2D<float4> ShadowSurface				: register(t16);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if IS_ENVPROBE_GEN
	CS_CUSTOM_CONSTANT_BUFFER
		float4 envProbeDimmerFactorAndLightScale;
		float4 envProbeFaceIndex;
	END_CS_CUSTOM_CONSTANT_BUFFER
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float4 StripGBufferSurface0( float4 value )
{
#if IS_ENVPROBE_GEN
	// Stripping is to make sure the unavailable data has the value we want.
	value.zw = 0;
#endif
	return value;
}

float4 StripGBufferSurface1( float4 value )
{
#if IS_ENVPROBE_GEN
	// Stripping is to make sure the unavailable data has the value we want.
	value.w = 0;
#endif
	return value;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Shared memory
GROUPSHARED uint TileMinZ;
GROUPSHARED uint TileMaxZ;

// Light list for the tile
GROUPSHARED uint TileLightListOpaque[MAX_LIGHTS_PER_TILE];
GROUPSHARED uint NumTileLightsOpaque;

#if ENABLE_TRANSPRERENT_LIST_OUTPUT
	GROUPSHARED uint TileLightListTransparent[MAX_LIGHTS_PER_TILE];
	GROUPSHARED uint NumTileLightsTransparent;
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CullLight( uint lightIndex, float4 frustumPlanes[7] )
{
	float3 lightPosition = lights[lightIndex].positionAndRadius.xyz;
	float cutoffRadius = lights[lightIndex].positionAndRadius.w;

	// 5 frustum planes are common, 1 is a znear for solids, 1 is znear for transparent
	float d0 = dot(frustumPlanes[0], float4(lightPosition, 1.0f));
	float d1 = dot(frustumPlanes[1], float4(lightPosition, 1.0f));
	float d2 = dot(frustumPlanes[2], float4(lightPosition, 1.0f));
	float d3 = dot(frustumPlanes[3], float4(lightPosition, 1.0f));
	float d4 = dot(frustumPlanes[4], float4(lightPosition, 1.0f));
	float d5 = dot(frustumPlanes[5], float4(lightPosition, 1.0f));
	float d6 = dot(frustumPlanes[6], float4(lightPosition, 1.0f));

	// test if the grid does contain this light (more conservative, for transparent pixels)
    bool inFrustumTransparent = (d0 >= -cutoffRadius) && (d1 >= -cutoffRadius) && (d2 >= -cutoffRadius)
			&& (d3 >= -cutoffRadius) && (d4 >= -cutoffRadius) && (d6 >= -cutoffRadius);

	// test if the grid does contain this light
    bool inFrustum = inFrustumTransparent && (d5 >= -cutoffRadius);
		
	// spot light culling - no branching, all of the ALU is nicelly eaten
	if ( lights[lightIndex].colorAndType.w >= 0.8f )
	{
		// spot lights			
		float3 lightPositionCull = lights[lightIndex].positionAndRadiusCull.xyz;
		float cutoffRadiusCull = lights[lightIndex].positionAndRadiusCull.w;

		// plane tests
		float d0 = dot(frustumPlanes[0], float4(lightPositionCull, 1.0f));
		float d1 = dot(frustumPlanes[1], float4(lightPositionCull, 1.0f));
		float d2 = dot(frustumPlanes[2], float4(lightPositionCull, 1.0f));
		float d3 = dot(frustumPlanes[3], float4(lightPositionCull, 1.0f));
		float d4 = dot(frustumPlanes[4], float4(lightPositionCull, 1.0f));
		float d5 = dot(frustumPlanes[5], float4(lightPositionCull, 1.0f));
		float d6 = dot(frustumPlanes[6], float4(lightPositionCull, 1.0f));

		// test if the grid does contain this light
		if ( inFrustum )
		{
			inFrustum = (d0 >= -cutoffRadiusCull) && (d1 >= -cutoffRadiusCull) && (d2 >= -cutoffRadiusCull)
				&& (d3 >= -cutoffRadiusCull) && (d4 >= -cutoffRadiusCull) && (d5 >= -cutoffRadiusCull);
		}

		// test if the grid does contain this light (more conservative, for transparent pixels)
		if ( inFrustumTransparent )
		{
			inFrustumTransparent = (d0 >= -cutoffRadiusCull) && (d1 >= -cutoffRadiusCull) && (d2 >= -cutoffRadiusCull)
				&& (d3 >= -cutoffRadiusCull) && (d4 >= -cutoffRadiusCull) && (d6 >= -cutoffRadiusCull);
		}
	}

	// if the light is visible emit it to the light list
	if ( inFrustumTransparent )
	{
	#if ENABLE_TRANSPRERENT_LIST_OUTPUT
		{
			uint listIndexTransparent;
			INTERLOCKED_ADD( NumTileLightsTransparent, 1, listIndexTransparent );
			if ( listIndexTransparent < MAX_LIGHTS_PER_TILE )
			{
				TileLightListTransparent[listIndexTransparent] = lightIndex;
			}
		}
	#endif
			
		if ( inFrustum )
		{
			uint listIndex;
			INTERLOCKED_ADD(NumTileLightsOpaque, 1, listIndex);
			if ( listIndex < MAX_LIGHTS_PER_TILE )
			{
				TileLightListOpaque[listIndex] = lightIndex;
			}
		}
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float4 TransformPlane( float4x4 mat, float4 plane )
{
	// ace_optimize
	float3 p = mul( mat, float4 ( -plane.xyz * plane.w, 1 ) ).xyz;
	float3 n = mul( (float3x3)mat, plane.xyz );
	return float4 ( n, -dot(n,p) );
}

void PerformCulling( uint2 groupId, uint groupThreadIdx, float zw )
{
	zw = TransformDepthRevProjAware( zw ); // ace_todo: we loose some precision here, but it might be more efficient when revProjAware is not compiled in (not 100% sure though)

	// Initialize shared memory
    if(groupThreadIdx == 0)
	{
		NumTileLightsOpaque = 0;
#if !ENABLE_LIGHTS_CODE
		TileLightListOpaque[0] = 0;
#endif
#if ENABLE_TRANSPRERENT_LIST_OUTPUT
#if !ENABLE_LIGHTS_CODE
		TileLightListTransparent[0] = 0;
#endif
		NumTileLightsTransparent = 0;
#endif
        TileMinZ = 0x7F7FFFFF;      // Max float
        TileMaxZ = 0;
    }

    GROUP_BARRIER_GROUP_SYNC;

	INTERLOCKED_MIN(TileMinZ, asuint(zw));
	INTERLOCKED_MAX(TileMaxZ, asuint(zw));

    GROUP_BARRIER_GROUP_SYNC;

    float minTileZ = DeprojectDepth( asfloat(TileMinZ) );
    float maxTileZ = DeprojectDepth( asfloat(TileMaxZ) );

	// Work out scale/bias from [0, 1]
    float2 tileScale = float2(screenDimensions.xy) * rcp(2.0f * float2(TILE_SIZE, TILE_SIZE));
    float2 tileBias = tileScale - float2(groupId.xy);

    // Now work out composite projection matrix
    // Relevant matrix columns for this tile frusta
    float4 c1 = float4(projectionMatrix._11 * tileScale.x, 0.0f, tileBias.x, 0.0f);
    float4 c2 = float4(0.0f, -projectionMatrix._22 * tileScale.y, tileBias.y, 0.0f);
    float4 c4 = float4(0.0f, 0.0f, 1.0f, 0.0f);

	/*
	float4 c1 = float4( projectionMatrix._11 * tileScale.x,	 projectionMatrix._21,					tileBias.x,	projectionMatrix._41 );
	float4 c2 = float4( projectionMatrix._12,				-projectionMatrix._22 * tileScale.y,	tileBias.y, projectionMatrix._42 );
	float4 c4 = float4( projectionMatrix._14,				 projectionMatrix._24, 					1.0f,		projectionMatrix._44 );
	*/

    // Derive frustum planes
    float4 frustumPlanes[7];

    // Sides
    frustumPlanes[0] = c4 - c1;
    frustumPlanes[1] = c4 + c1;
    frustumPlanes[2] = c4 - c2;
    frustumPlanes[3] = c4 + c2;

    // Near/far
    frustumPlanes[4] = float4(0.0f, 0.0f, -1.0f,  maxTileZ);
	frustumPlanes[5] = float4(0.0f, 0.0f,  1.0f, -minTileZ);
	
	// For transparent
	frustumPlanes[6] = float4(0.0f, 0.0f,  1.0f, 0.0f );    

    // Normalize frustum planes (near/far already normalized)
	frustumPlanes[0] /= length(frustumPlanes[0].xyz);
	frustumPlanes[1] /= length(frustumPlanes[1].xyz);
	frustumPlanes[2] /= length(frustumPlanes[2].xyz);
	frustumPlanes[3] /= length(frustumPlanes[3].xyz);

	float4 frustumPlanesWorld[7];
	frustumPlanesWorld[0] = TransformPlane( viewToWorld, frustumPlanes[0] );
	frustumPlanesWorld[1] = TransformPlane( viewToWorld, frustumPlanes[1] );
	frustumPlanesWorld[2] = TransformPlane( viewToWorld, frustumPlanes[2] );
	frustumPlanesWorld[3] = TransformPlane( viewToWorld, frustumPlanes[3] );
	frustumPlanesWorld[4] = TransformPlane( viewToWorld, frustumPlanes[4] );
	frustumPlanesWorld[5] = TransformPlane( viewToWorld, frustumPlanes[5] );
	frustumPlanesWorld[6] = TransformPlane( viewToWorld, frustumPlanes[6] );
	
	// Cull lights for this tile
	#if ENABLE_LIGHTS_CODE
		[loop]
		for (uint lightIndex = groupThreadIdx; lightIndex < lightNum; lightIndex += TILE_SIZE*TILE_SIZE)
		{
			CullLight( lightIndex, frustumPlanesWorld );
		}
	#endif

	GROUP_BARRIER_GROUP_SYNC;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OutputTileLists( uint2 groupId, uint groupThreadIdx )
{
#if ENABLE_TRANSPRERENT_LIST_OUTPUT

#if ENABLE_LIGHTS_CODE
	// Outputing lights lists
    if(groupThreadIdx < MAX_LIGHTS_PER_TILE)
    {
        // Write out the indices
        uint tileIdx = groupId.y * numTiles.x + groupId.x;
        uint bufferIdx = tileIdx * MAX_LIGHTS_PER_TILE + groupThreadIdx;
		uint lightIdx;
		if ( groupThreadIdx < NumTileLightsTransparent )
		{
			lightIdx = TileLightListTransparent[groupThreadIdx];
		}
		else
		{	
			lightIdx = MAX_LIGHTS_PER_TILE;
		}

        LightListTransparentOutput.Store(bufferIdx*4, lightIdx);
    }
#endif

#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float FetchDepthTexture( uint2 pixelCoord )
{
#if IS_ENVPROBE_GEN
	return DepthTexture[ uint3(pixelCoord, (uint)envProbeFaceIndex.x) ].x;
#else
	return DepthTexture[ pixelCoord ].x;
#endif
}

float4 FetchGBuffer0Texture( uint2 pixelCoord )
{
#if IS_ENVPROBE_GEN
	return StripGBufferSurface0( GBufferSurface0[ uint3(pixelCoord, (uint)envProbeFaceIndex.x) ] );
#else
	return StripGBufferSurface0( GBufferSurface0[ pixelCoord ] );
#endif
}

float4 FetchGBuffer1Texture( uint2 pixelCoord )
{
#if IS_ENVPROBE_GEN
	return StripGBufferSurface1( GBufferSurface1[ uint3(pixelCoord, (uint)envProbeFaceIndex.x) ] );
#else
	return StripGBufferSurface1( GBufferSurface1[ pixelCoord ] );
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

[NUMTHREADS(TILE_SIZE, TILE_SIZE, 1)]
void cs_main(uint3 GroupID : SYS_GROUP_ID, uint3 GroupThreadID : SYS_GROUP_THREAD_ID)
{
	const uint2 pixelCoord		= GroupID.xy * uint2(TILE_SIZE, TILE_SIZE) + GroupThreadID.xy;
	const uint groupThreadIdx	= GroupThreadID.y * TILE_SIZE + GroupThreadID.x;
	const float zw				= FetchDepthTexture( pixelCoord );
	const bool isSky			= IsSkyByProjectedDepthRevProjAware( zw );

	PerformCulling( GroupID.xy, groupThreadIdx, zw );
	
	#if IS_ENVPROBE_GEN
		const float3 worldSpacePosition = PositionFromDepthRevProjAware(zw,pixelCoord);
		const float3 V = normalize(cameraPosition.xyz-worldSpacePosition);
	#endif

	float3 ambient = float3(0,0,0);
	float3 diffuse = float3(0,0,0);
	float3 specular = float3(0,0,0);
	float3 reflection = float3(0,0,0);
	
	[branch]
	if ( !isSky )
	{
		#ifdef __PSSL__
			const SCompileInLightingParams extraLightingParams;
		#else
			const SCompileInLightingParams extraLightingParams = (SCompileInLightingParams)0;
		#endif

		float4 gbuff0 = FetchGBuffer0Texture( pixelCoord );
		float4 gbuff1 = FetchGBuffer1Texture( pixelCoord );
	
		float3 N = normalize( gbuff1.xyz - 0.5 );

		#if !IS_ENVPROBE_GEN
			const float3 worldSpacePosition = PositionFromDepthRevProjAware(zw,pixelCoord);
			const float3 V = normalize(cameraPosition.xyz-worldSpacePosition);
		#endif
		
		const float  translucency			= gbuff0.w;

#if IS_ENVPROBE_GEN
		const float  dimmerFactor			= envProbeDimmerFactorAndLightScale.x;
#elif ENABLE_DIMMER_APPLY_CODE
		const float  dimmerFactor			= DecodeGlobalShadowBufferDimmers( ShadowSurface[pixelCoord] );
#else
		const float  dimmerFactor			= 1;
#endif

		#if IS_ENVPROBE_GEN
			float4 gbuff2						= 0;
			const float3 specularity			= 0;		
			const float  roughness				= 1;
			const float  materialFresnelGain	= 0;
		#else
			float4 gbuff2						= GBufferSurface2[pixelCoord];
			const float3 specularity			= pow( gbuff2.xyz, 2.2 );
			const float  roughness				= gbuff1.w;
			const float  materialFresnelGain	= (GBUFF_MATERIAL_FLAG_GRASS | GBUFF_MATERIAL_FLAG_TREES | GBUFF_MATERIAL_FLAG_BILLBOARDS) & DecodeGBuffMaterialFlagsMask( gbuff2.w ) ? 0.f : 1.f;
		#endif
				
		const float3 N_flat = N;
		const float extraEnvProbeFresnelScale = materialFresnelGain;
		
		const float curr_shadow = DecodeGlobalShadowBufferShadow( ShadowSurface[pixelCoord] );
		float3 shadowFactorFinal = curr_shadow;
		{
			// global light
			{				
				LightingData ld = CalcGlobalLightPBRPipeline( V, N, translucency, specularity, roughness, worldSpacePosition, (float2)pixelCoord / screenDimensions.xy, 0, curr_shadow, extraLightingParams, gbuff2.w );
				shadowFactorFinal = min( ld.preNormDiffuse, shadowFactorFinal );

			#if IS_ENVPROBE_GEN
				const float globalLightScale = envProbeDimmerFactorAndLightScale.y;
			#else
				const float globalLightScale = 1;
			#endif
				
				diffuse += globalLightScale * ld.diffuse;
				specular += globalLightScale * ld.specular;
			}

			//
		#if ENABLE_LIGHTS_INTERIOR_EXTERIOR || ENABLE_ENVPROBE_INTERIOR_EXTERIOR
			const float interiorFactor = DecodeGlobalShadowBufferInteriorFactor( ShadowSurface[pixelCoord] );
		#endif
	
			// local lights
		#if ENABLE_LIGHTS_CODE && ENABLE_LIGHT_APPLY_CODE
		#if ENABLE_CAMERA_LIGHTS_MODIFIER
			const float lightsCharacterModifier = 0 != ( GetStencilValue( StencilTexture[ pixelCoord ] ) & LC_Characters ) ? 1 : localLightsExtraParams.x;
		#endif
			for( uint tLightIdx = 0; tLightIdx < NumTileLightsOpaque; ++tLightIdx )
			{
				uint lIdx = TileLightListOpaque[tLightIdx];
				
				float3 L = lights[lIdx].positionAndRadius.xyz - worldSpacePosition;
				float attenuation = CalcLightAttenuation( lIdx, worldSpacePosition, L );

				#if IS_ENVPROBE_GEN
					attenuation *= envProbeDimmerFactorAndLightScale.z;
				#endif

				#if ENABLE_LIGHTS_INTERIOR_EXTERIOR
					attenuation *= GetLocalLightsAttenuationInteriorScale( lights[lIdx].colorAndType.w, interiorFactor );
				#endif
		
				[branch]
				if ( attenuation > 0 )
				{
					#if ENABLE_CAMERA_LIGHTS_MODIFIER
						attenuation *= IsLightFlagsCharacterModifierEnabled( lights[lIdx].colorAndType.w ) ? lightsCharacterModifier : 1;
					#endif

					L = normalize( L );
					diffuse += attenuation * (DiffuseLightingPBRPipeline( L, N, V, translucency, specularity, roughness, extraLightingParams ).xyz * lights[lIdx].colorAndType.xyz);
					#if !IS_ENVPROBE_GEN
					specular += attenuation * (SpecularLightingPBRPipeline( L, N, V, specularity, roughness, translucency, extraLightingParams ).xyz * lights[lIdx].colorAndType.xyz);
					#endif
				}
			}
		#endif
		
			// envprobes (ambient + reflection)
			{
				const float envprobe_distance_scale		= CalcEnvProbesDistantBrightness( worldSpacePosition );

		#if IS_ENVPROBE_GEN
				const float  envprobe_diffuse_scale		= 1;
				const float3 envprobe_ambient_scale		= envprobe_distance_scale * lerp( pbrSimpleParams0.yyy, pbrSimpleParams0.xxx, shadowFactorFinal );
				const float  envprobe_specular_scale	= 1;
				const float3 envprobe_reflection_scale	= envprobe_distance_scale * lerp( pbrSimpleParams1.yyy, pbrSimpleParams1.xxx, shadowFactorFinal );
		#else			
				const float  envprobe_diffuse_scale		= pbrSimpleParams0.z;
				const float3 envprobe_ambient_scale		= envprobe_distance_scale * lerp( pbrSimpleParams0.yyy, pbrSimpleParams0.xxx, shadowFactorFinal );
				const float  envprobe_specular_scale	= pbrSimpleParams0.w;
				const float3 envprobe_reflection_scale	= envprobe_distance_scale * lerp( pbrSimpleParams1.yyy, pbrSimpleParams1.xxx, shadowFactorFinal );
		#endif

				{
					float3 envProbeAmbient  = float3 ( 0, 0, 0 );
					float3 envProbeSpecular = float3 ( 0, 0, 0 );
				#if !IS_ENVPROBE_GEN
					{
						float reflectionMipIndex = 0;
						{
							float4 gbuff1_10 = FetchGBuffer1Texture( pixelCoord + int2(1,0) );
							float4 gbuff1_01 = FetchGBuffer1Texture( pixelCoord + int2(0,1) );
							float3 N_10 = normalize(gbuff1_10.xyz - 0.5);
							float3 N_01 = normalize(gbuff1_01.xyz - 0.5);

							// mip computed based on N instead of R direction. a bit cheaper, didn't notice quality degradation.
							reflectionMipIndex = ENVPROBE_MIPLEVEL_SCALE * CalcCubeMipLevelExplicitNormals( REFLECTION_CUBE_RESOLUTION, N, N_10, N_01 );
						}

						// ace_ibl_fix: remove old gbuffer encoding if we'll stick to the gbuffer based derivation
						
						//float4 _gbuff2 = gbuff2;						
						//_gbuff2.z = mipLevel / REFLECTION_CUBE_NUM_MIPS; // because decodeMipLevel multiplies by REFLECTION_CUBE_NUM_MIPS
						//envProbeSpecular = CalcEnvProbeReflection( worldSpacePosition, V, N, _gbuff2, roughness );

					#if ENABLE_ENVPROBE_INTERIOR_EXTERIOR
						const float envProbesInteriorFactor = interiorFactor;
					#else
						const float envProbesInteriorFactor = -1;
					#endif

						const float3 R = reflect( -V, N );
						CalcEnvProbes_MipLod( envProbeAmbient, envProbeSpecular, worldSpacePosition, N, R, 0, reflectionMipIndex, roughness, true, true, true, envProbesInteriorFactor );
					}
				
					// Characters lighting boost
					{
						const uint stencilValue = GetStencilValue( StencilTexture[ pixelCoord ] );
						ApplyCharactersLightingBoostStencilBased( stencilValue, shadowFactorFinal, envProbeAmbient.xyz, envProbeSpecular.xyz );
					}
				#endif

					diffuse  *= envprobe_diffuse_scale;
					specular *= envprobe_specular_scale;
					ambient  += NormalizeAmbientPBRPipeline( envprobe_ambient_scale * envProbeAmbient, translucency, V, N, specularity, roughness, N_flat, extraEnvProbeFresnelScale );
					reflection += envprobe_reflection_scale * envProbeSpecular * CalcFresnelPBRPipelineEnvProbeReflection( V, N, specularity, roughness, translucency, N_flat, extraEnvProbeFresnelScale );
				}
			}

			// apply AO
			{
			#if ENABLE_SSAO
				const float3 ssaoMod				= ModulateSSAOByTranslucency( ProcessSampledSSAO( DecodeGlobalShadowBufferSSAO( ShadowSurface[pixelCoord] ) ), translucency );
			#else
				const float3 ssaoMod				= 1;
			#endif

				const float3 AO_probe				= GetProbesSSAOScaleByDimmerFactor( dimmerFactor ) * ssaoMod;				
				const float3 AO_nonProbe			= ssaoParams.x * ssaoMod + ssaoParams.y;		

				ambient		*= AO_probe;
				reflection	*= AO_probe;
				diffuse		*= AO_nonProbe;
				specular	*= AO_nonProbe;		
			}
		}
	}

	// Output result colors
	#if IS_ENVPROBE_GEN
		{
			float3 resultColor = 0;

			[branch]
			if ( !isSky )
			{
				const float3 albedo = pow( DecodeTwoChannelColorArray( GBufferSurface0, (uint)envProbeFaceIndex.x, pixelCoord ), 2.2 );
				resultColor = albedo * (ambient.xyz + diffuse.xyz) + specular.xyz + reflection.xyz;
				OutputTexture[pixelCoord] = float4( resultColor, 1 );
			}
		}
	#else
		{			
			float3 albedo = pow( GBufferSurface0[pixelCoord].xyz, 2.2 );

			float debug_output_add = 0;
			
			#if 0
			{
				if ( pixelCoord.x < 5 )
				{
				#if IS_PURE_DEFERRED_COMPUTE
					albedo = float3( 0.25, 1, 0.25 );
				#else
					albedo = float3( 1, 0.25, 0.25 );
				#endif
				}

				#if IS_PURE_DEFERRED_COMPUTE
				{
					const uint2  tileIdx		= pixelCoord.xy / TILE_SIZE.xx;
					const uint   bufferIdx		= ((tileIdx.y * (int)numTiles.x) + tileIdx.x) * MAX_LIGHTS_PER_TILE;
	
					uint totalLights = 0;

					[loop]
					for(int tileLightIdx = 0; tileLightIdx < MAX_LIGHTS_PER_TILE; ++tileLightIdx)
					{
						uint lIdx = LightListTransparentOutput.Load((bufferIdx + tileLightIdx)*4);

						[branch]
						if( lIdx >= MAX_LIGHTS_PER_TILE )
							break;

						totalLights += 1;
					}		

					debug_output_add = 0.2 * totalLights;
				}
				#elif !IS_ENVPROBE_GEN
				{
					debug_output_add = 0.2 * NumTileLightsTransparent;
				}
				#endif
			}
			#endif

			OutputTexture[pixelCoord] = float4( debug_output_add + albedo * max( 0, ambient.xyz + diffuse.xyz ) + max( 0, specular.xyz + reflection.xyz ), 1 );
		}
	#endif

	// Output light buffers	
	OutputTileLists( GroupID.xy, groupThreadIdx );
}


