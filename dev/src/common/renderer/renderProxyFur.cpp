#include "build.h"
#include "renderCollector.h"
#include "renderProxyFur.h"
#include "renderSkinningData.h"
#include "renderTexture.h"
#include "renderShader.h"
#include "renderShaderPair.h"
#include "renderScene.h"
#include "renderVisibilityQueryManager.h"

#include "renderFurMesh.h"

#ifdef RED_PLATFORM_WINPC
#ifdef USE_NVIDIA_FUR
#include "../../../external/NvidiaHair/include/GFSDK_HairWorks.h"
#endif // USE_NVIDIA_FUR
#endif // RED_PLATFORM_WINPC

#include "../engine/renderFragment.h"
#include "../engine/furComponent.h"
#include "../engine/furMeshResource.h"
#include "../engine/bitmapTexture.h"


#ifdef RED_LOGGING_ENABLED
// Uncomment to enable some extra tests of shader cache state. Disabled, so the log doesn't get filled up with
// stuff every time a fur proxy is created.
//#define ENABLE_FUR_SHADER_DEBUG_LOGGING
#endif

namespace
{
#ifdef USE_NVIDIA_FUR
	void SetTextureResource( GFSDK_HairSDK* sdk, GFSDK_HairInstanceID instanceID, const CRenderTexture* inTex, GFSDK_HAIR_TEXTURE_TYPE type)
	{
		ID3D11ShaderResourceView* d3dSrv = nullptr;
		if ( inTex )
		{
			if ( inTex->HasHiResLoaded() )
			{
				d3dSrv = GpuApi::Hacks::GetTextureSRV( inTex->GetHiResTextureRef() );
			}
			else
			{
				d3dSrv = GpuApi::Hacks::GetTextureSRV( inTex->GetTextureRef() );
			}
		}
		sdk->SetTextureSRV( instanceID, type, d3dSrv );
	}

#ifdef ENABLE_FUR_SHADER_DEBUG_LOGGING
	void TestMaterialInShaderCache( GFSDK_HairSDK* sdk, GFSDK_HairInstanceID instanceID )
	{
		int shaderStatus = -1;
		sdk->CheckShaderCacheStatus( instanceID, shaderStatus );
		switch ( shaderStatus )
		{
		case 0:		RED_LOG( Hairworks, TXT(" Shaders not found in cache. Will use default unoptimized.") ); break;
		case 1:		RED_LOG_WARNING( Hairworks, TXT(" Shaders are in cache, but shader objects not created yet. Will be created on first use.") ); break;
		case 2:		RED_LOG( Hairworks, TXT(" Shaders are in cache, and already created! Enjoy better hairworks performance!") ); break;
		default:	RED_HALT( "Unknown shader status: %d", shaderStatus );
		}
	}

	void TestInShaderCache( GFSDK_HairSDK* sdk, GFSDK_HairInstanceID instanceID, const String& resourcePath, Bool includeWetness )
	{
		RED_LOG( Hairworks, TXT("Testing default material shader for %ls"), resourcePath.AsChar() );
		sdk->SetCurrentMaterial( instanceID, 0 );
		TestMaterialInShaderCache( sdk, instanceID );

		if( includeWetness )
		{
			RED_LOG( Hairworks, TXT("Testing wet material shader for %ls"), resourcePath.AsChar() );
			sdk->SetCurrentMaterial( instanceID, 1, 1.0f );
			TestMaterialInShaderCache( sdk, instanceID );

			RED_LOG( Hairworks, TXT("Testing mixed material shader for %ls"), resourcePath.AsChar() );
			sdk->SetCurrentMaterial( instanceID, 1, 0.5f );
			TestMaterialInShaderCache( sdk, instanceID );

			// Revert back to material 0.
			sdk->SetCurrentMaterial( instanceID, 0 );
		}
	}
#endif

#endif
}


CRenderProxy_Fur::CRenderProxy_Fur( const RenderProxyInitInfo& info ) 
	: IRenderProxyDrawable( RPT_Fur, info )
	, IRenderEntityGroupProxy( info.m_entityGroup )
#ifdef USE_NVIDIA_FUR
	, m_asset( nullptr )
	, m_defaultParams( nullptr )
	, m_wetnessMaterialParams( nullptr )
	, m_furTextures( nullptr )
	, m_instanceID( GFSDK_HairInstanceID_NULL )
	, m_wetnessMaterialWeight( 0.f )
#endif
	, m_skinningData( nullptr )
	, m_useShadowDistances( false )
{
	PC_SCOPE_PIX( CreateFurProxy );

	RED_ASSERT( info.m_component );
	RED_ASSERT( info.m_component->IsA< CFurComponent >() );

#ifdef USE_NVIDIA_FUR

	if ( info.m_component->IsA< CFurComponent >() )
	{
		const CFurComponent* component = static_cast< const CFurComponent* >( info.m_component );
		RED_ASSERT( component );

		if ( component->GetFurMesh() )
		{
			const CFurMeshResource& meshRes = *(component->GetFurMesh());

			m_isTwoSided = meshRes.IsTwoSided();
			m_boundingBox = meshRes.GetBoundingBox();

			GFSDK_HairSDK* hairSDK = GetRenderer()->GetHairSDK();
			if ( !hairSDK )
			{
				RED_LOG_ERROR( CNAME( CRenderProxy_Fur ), TXT( "HairWorks library not initialized" ) );
				return;
			}

			CRenderFurMesh_Hairworks* asset = static_cast< CRenderFurMesh_Hairworks* >( meshRes.GetRenderResource() );
			if ( asset == nullptr )
			{
				RED_LOG_ERROR( CNAME( CRenderProxy_Fur ), TXT( "Failed to get render resource" ) );
				return;
			}

			m_asset = asset;
			m_asset->AddRef();


			RED_ASSERT( m_furTextures == nullptr );
			m_furTextures = new CRenderTexture*[ GFSDK_HAIR_NUM_TEXTURES ];

			ExtractRenderResource( meshRes.m_physicalMaterials.m_volume.m_densityTex.Get(),				m_furTextures[ GFSDK_HAIR_TEXTURE_DENSITY ] );
			ExtractRenderResource( meshRes.m_graphicalMaterials.m_color.m_rootColorTex.Get(),			m_furTextures[ GFSDK_HAIR_TEXTURE_ROOT_COLOR ] );
			ExtractRenderResource( meshRes.m_graphicalMaterials.m_color.m_tipColorTex.Get(),			m_furTextures[ GFSDK_HAIR_TEXTURE_TIP_COLOR ] );
			ExtractRenderResource( meshRes.m_graphicalMaterials.m_color.m_strandTex.Get(),				m_furTextures[ GFSDK_HAIR_TEXTURE_STRAND ] );

			ExtractRenderResource( meshRes.m_physicalMaterials.m_strandWidth.m_rootWidthTex.Get(),		m_furTextures[ GFSDK_HAIR_TEXTURE_ROOT_WIDTH ] );
			ExtractRenderResource( meshRes.m_physicalMaterials.m_strandWidth.m_tipWidthTex.Get(),		m_furTextures[ GFSDK_HAIR_TEXTURE_TIP_WIDTH ] );

			ExtractRenderResource( meshRes.m_physicalMaterials.m_stiffness.m_stiffnessTex.Get(),		m_furTextures[ GFSDK_HAIR_TEXTURE_STIFFNESS ] );
			ExtractRenderResource( meshRes.m_physicalMaterials.m_stiffness.m_rootStiffnessTex.Get(),	m_furTextures[ GFSDK_HAIR_TEXTURE_ROOT_STIFFNESS ] );

			ExtractRenderResource( meshRes.m_physicalMaterials.m_clumping.m_clumpScaleTex.Get(),		m_furTextures[ GFSDK_HAIR_TEXTURE_CLUMP_SCALE ] );
			ExtractRenderResource( meshRes.m_physicalMaterials.m_clumping.m_clumpRoundnessTex.Get(),	m_furTextures[ GFSDK_HAIR_TEXTURE_CLUMP_ROUNDNESS ] );
			ExtractRenderResource( meshRes.m_physicalMaterials.m_clumping.m_clumpNoiseTex.Get(),		m_furTextures[ GFSDK_HAIR_TEXTURE_CLUMP_NOISE ] );

			ExtractRenderResource( meshRes.m_physicalMaterials.m_waveness.m_waveScaleTex.Get(),			m_furTextures[ GFSDK_HAIR_TEXTURE_WAVE_SCALE ] );
			ExtractRenderResource( meshRes.m_physicalMaterials.m_waveness.m_waveFreqTex.Get(),			m_furTextures[ GFSDK_HAIR_TEXTURE_WAVE_FREQ ] );

			ExtractRenderResource( meshRes.m_physicalMaterials.m_volume.m_lengthTex.Get(),				m_furTextures[ GFSDK_HAIR_TEXTURE_LENGTH ] );

			// create instance descriptor
			if (m_defaultParams == nullptr)
			{
				m_defaultParams = new GFSDK_HairInstanceDescriptor;
			}

			meshRes.CreateDefaultHairInstanceDesc( m_defaultParams );
			//m_defaultParams->m_distanceLODEnd = GetAutoHideDistance();	// proxy override for test

			if ( meshRes.IsUsingWetness() )
			{
				if( m_wetnessMaterialParams == nullptr )
				{
					m_wetnessMaterialParams = new GFSDK_HairInstanceDescriptor;
				}
				meshRes.CreateTargetHairInstanceDesc( m_wetnessMaterialParams, 0 );
			}

			// create instance
			GFSDK_HairInstanceID instID;
			GFSDK_HAIR_RETURNCODES createResult = hairSDK->CreateHairInstance( m_asset->GetAssetID(), &instID);
			m_instanceID = static_cast<Uint16>(instID);
			RED_ASSERT( createResult == GFSDK_RETURN_OK, TXT("Fur instance creation failed") );


			// Set rendering control variables
			GFSDK_HAIR_RETURNCODES updateResult = GFSDK_RETURN_OK;
			updateResult |= hairSDK->UpdateInstanceDescriptor( instID, *m_defaultParams, 0 );

			if( m_wetnessMaterialParams != nullptr )
			{
				updateResult |= hairSDK->UpdateInstanceDescriptor( instID, *m_wetnessMaterialParams, 1 );
			}

			RED_ASSERT( updateResult == GFSDK_RETURN_OK, TXT("Fur instance update failed") );

			gfsdk_float4x4* dataPtr = static_cast<gfsdk_float4x4*>( RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Debug, meshRes.m_boneCount * sizeof(gfsdk_float4x4), 16 ) );

			Matrix identity = Matrix::IDENTITY;

			for ( Uint32 matI = 0; matI < meshRes.m_boneCount; ++matI )
			{
				Red::System::MemoryCopy( &dataPtr[matI]._11, identity.AsFloat(), sizeof(Matrix) );
			}

			GFSDK_HAIR_RETURNCODES skinningResult = hairSDK->UpdateSkinningMatrices( (GFSDK_HairInstanceID)m_instanceID, meshRes.m_boneCount, dataPtr );
			RED_ASSERT( skinningResult == GFSDK_RETURN_OK, TXT("Fur skinning failed") );

			RED_MEMORY_FREE( MemoryPool_Default, MC_Debug, dataPtr );


			m_psConstantBuffer = GpuApi::CreateBuffer( sizeof(GFSDK_HairShaderConstantBuffer), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
			GpuApi::SetBufferDebugPath( m_psConstantBuffer, "fur cbuffer" );

#ifdef ENABLE_FUR_SHADER_DEBUG_LOGGING
			// Pass texture resources to hairworks. Only needed here for TestInShaderCache to work. Otherwise, they get set during render.
			for ( int idx = 0; idx < GFSDK_HAIR_NUM_TEXTURES; ++idx )
			{
				SetTextureResource( hairSDK, instID, m_furTextures[ idx ], (GFSDK_HAIR_TEXTURE_TYPE)idx );
			}

			TestInShaderCache( hairSDK, instID, meshRes.GetDepotPath(), m_wetnessMaterialParams != nullptr );
#endif // ENABLE_FUR_SHADER_DEBUG_LOGGING
		}
	}
#endif // USE_NVIDIA_FUR
}

CRenderProxy_Fur::~CRenderProxy_Fur()
{
#ifdef USE_NVIDIA_FUR

	if ( m_defaultParams != nullptr )
	{
		delete m_defaultParams;
		m_defaultParams = nullptr;
	}

	if ( m_wetnessMaterialParams != nullptr )
	{
		delete m_wetnessMaterialParams;
		m_wetnessMaterialParams = nullptr;
	}


	GFSDK_HairSDK* hairSDK = GetRenderer()->GetHairSDK();
	if ( hairSDK )
	{
		hairSDK->FreeHairInstance( (GFSDK_HairInstanceID)m_instanceID );
	}

	GpuApi::SafeRelease( m_psConstantBuffer );
	if ( m_furTextures )
	{
		for ( int idx = 0; idx < GFSDK_HAIR_NUM_TEXTURES; ++idx )
		{
			SAFE_RELEASE( m_furTextures[ idx ] );
		}
		delete[] m_furTextures;
	}
	m_furTextures = nullptr;

	SAFE_RELEASE( m_skinningData );

	SAFE_RELEASE( m_asset );

#endif // USE_NVIDIA_FUR
}


void CRenderProxy_Fur::Prefetch( CRenderFramePrefetch* prefetch ) const
{
#ifdef USE_NVIDIA_FUR
	if ( m_furTextures == nullptr )
	{
		return;
	}

	const Float distanceSq = CalcCameraDistanceSq( prefetch->GetCameraPosition(), prefetch->GetCameraFovMultiplierUnclamped() );
	for ( Uint32 i = 0; i < GFSDK_HAIR_NUM_TEXTURES; ++i )
	{
		prefetch->AddTextureBind( m_furTextures[ i ], distanceSq );
	}
#endif // USE_NVIDIA_FUR
}


Uint32 CRenderProxy_Fur::GetNumTextures() const
{
#ifdef USE_NVIDIA_FUR
	return GFSDK_HAIR_NUM_TEXTURES;
#else
	return 0;
#endif
}


void CRenderProxy_Fur::UpdateFurSkinning()
{
	PC_SCOPE_PIX( UpdateFurSkinning );

#ifdef USE_NVIDIA_FUR
	GFSDK_HairSDK* hairSDK = GetRenderer()->GetHairSDK();

	RED_ASSERT( hairSDK, TXT("No HairWorks sdk") );

	if ( !hairSDK )
	{
		return;
	}

	// default material
	if ( m_defaultParams )
	{
		if( HasClippingEllipse() )
		{
			m_defaultParams->m_useCullSphere = true;
			memcpy( &m_defaultParams->m_cullSphereInvTransform, m_clippingEllipseParams->m_localToEllipse.AsFloat(), sizeof(gfsdk_float4x4) ); 
		}
		else
		{
			m_defaultParams->m_useCullSphere = false; 
		}
		// model-to-world matrix should be set before StepSimulation()
		memcpy( &m_defaultParams->m_modelToWorld, m_localToWorld.AsFloat(), sizeof(gfsdk_float4x4) );
		GFSDK_HAIR_RETURNCODES updateResult = hairSDK->UpdateInstanceDescriptor( (GFSDK_HairInstanceID)m_instanceID, *m_defaultParams, 0 );
		RED_ASSERT( updateResult == GFSDK_RETURN_OK, TXT("Fur update default instance desc failed") );
	}

	// first material for blend. If need more just we need to have more blocks like this
	if ( m_wetnessMaterialParams != nullptr )
	{
		if( HasClippingEllipse() )
		{
			m_wetnessMaterialParams->m_useCullSphere = true;
			memcpy( &m_wetnessMaterialParams->m_cullSphereInvTransform, m_clippingEllipseParams->m_localToEllipse.AsFloat(), sizeof(gfsdk_float4x4) ); 
		}
		else
		{
			m_wetnessMaterialParams->m_useCullSphere = false; 
		}
		// model-to-world matrix should be set before StepSimulation()
		memcpy( &m_wetnessMaterialParams->m_modelToWorld, m_localToWorld.AsFloat(), sizeof(gfsdk_float4x4) );
		GFSDK_HAIR_RETURNCODES updateResult = hairSDK->UpdateInstanceDescriptor( (GFSDK_HairInstanceID)m_instanceID, *m_wetnessMaterialParams, 1 );
		RED_ASSERT( updateResult == GFSDK_RETURN_OK, TXT("Fur update target instance desc failed") );
	}

	if ( !m_skinningData )
	{
		return;
	}

	// Update skinning
	if ( m_skinningData->GetMatrixType() == SDMT_4x4 )
	{
		Uint32 numBones = m_skinningData->GetNumMatrices();
		const gfsdk_float4x4* dataPtr = reinterpret_cast<const gfsdk_float4x4*>( m_skinningData->GetReadData() );
		gfsdk_float4x4* matrices = new gfsdk_float4x4[ numBones ];
		memcpy( matrices, dataPtr, sizeof(gfsdk_float4x4)*numBones );
		GFSDK_HAIR_RETURNCODES skinningResult = hairSDK->UpdateSkinningMatrices( (GFSDK_HairInstanceID)m_instanceID, numBones, matrices );
		RED_ASSERT( skinningResult == GFSDK_RETURN_OK, TXT("Fur skinning failed") );
		delete[] matrices;
	}
	RED_ASSERT( m_skinningData->GetMatrixType() == SDMT_4x4, TXT("Fur skinning matrix format mismatch") );
#endif // USE_NVIDIA_FUR
}

void CRenderProxy_Fur::Render( const CRenderCollector &collector, const RenderingContext& context )
{
	PC_SCOPE_PIX( FurRender );

	if ( !context.CheckLightChannels( m_lightChannels ) )
	{
		return;
	}

	// Get frame info
	if ( !collector.m_info )
	{
		return;
	}
	const CRenderFrameInfo& frameInfo = *collector.m_info;

#ifdef USE_NVIDIA_FUR
	// Skip shadows rendering if possible
	if ( context.m_pass == RP_ShadowDepthSolid )
	{
		// If shadow cast is false in shadow pass, then skip rendering shadow
		if ( !m_defaultParams->m_castShadows )
		{
			return;
		}

		// Skip based on entity group (we don't want hair to be visible if all the rest disappears)
		if ( m_useShadowDistances && GetEntityGroup() )
		{
			GetEntityGroup()->UpdateOncePerFrame( collector );
			
			const Bool isVisibleForShadow = GetEntityGroup()->GetShadowFadeAlpha( collector.GetDissolveSynchronizer(), false ).IsOne();

			if ( !isVisibleForShadow )
			{
				return;
			}
		}
	}

	if ( m_instanceID != GFSDK_HairInstanceID_NULL )
	{
		GFSDK_HairSDK* hairSDK = GetRenderer()->GetHairSDK();

		if ( !hairSDK )
		{
			return;
		}

		ID3D11Device* devD3D = GpuApi::Hacks::GetDevice();
		ID3D11DeviceContext* ctxD3D = GpuApi::Hacks::GetDeviceContext();

		if (devD3D && ctxD3D)
		{
			gfsdk_float4x4 viewMat;
			gfsdk_float4x4 projMat;
			memcpy(&viewMat, context.GetCamera().GetWorldToView().AsFloat(), sizeof(gfsdk_float4x4));
			memcpy(&projMat, context.GetCamera().GetViewToScreenRevProjAware().AsFloat(), sizeof(gfsdk_float4x4));

			GFSDK_HAIR_RETURNCODES matrixSetResult = hairSDK->SetViewProjection( &viewMat, &projMat, context.GetCamera().GetFOV() );
			RED_ASSERT( matrixSetResult == GFSDK_RETURN_OK, TXT("Fur viewProjection setting failed") );

			// Set character light boost
			const bool isCharactersLightingBoostRender = ( ( context.m_pass != RP_GBuffer ) && ( context.m_pass != RP_ShadowDepthSolid ) ) ? true : false;
			if ( isCharactersLightingBoostRender )
			{
				const CEnvGlobalLightParametersAtPoint &globalLightParams = frameInfo.m_envParametersArea.m_globalLight;
				Vector charactersLightingBoostValue (
					globalLightParams.m_charactersLightingBoostAmbientLight.GetScalarClampMin( 0.f ), 		
					globalLightParams.m_charactersLightingBoostAmbientShadow.GetScalarClampMin( 0.f ),
					globalLightParams.m_charactersLightingBoostReflectionLight.GetScalarClampMin( 0.f ), 
					globalLightParams.m_charactersLightingBoostReflectionShadow.GetScalarClampMin( 0.f ) );

				GetRenderer()->GetStateManager().SetPixelConst( PSC_CharactersLightingBoost, charactersLightingBoostValue );
				GpuApi::BindMainConstantBuffers();

				// UpdateConstantBuffers() is just a hack to re-bind character light boost params.
				// It should use the previous draw's bound constant buffers, but it is not working for now.
				// This call should be removed when we are able to reuse the previous state.
				GpuApi::Hacks::UpdateConstantBuffers();
			}

			// set material blending for skinned and non skinned
			if ( m_wetnessMaterialWeight > 0.f )
			{
				hairSDK->SetCurrentMaterial( (GFSDK_HairInstanceID)m_instanceID, 1, m_wetnessMaterialWeight);
			}
			else
			{
				hairSDK->SetCurrentMaterial( (GFSDK_HairInstanceID)m_instanceID, 0, 0.f);
			}

			const CGpuApiScopedTwoSidedRender scopedForcedTwoSided( m_isTwoSided );
			// Determine which pass we are in
			bool normalPass = false;
			if ( context.m_pass == RP_GBuffer )
			{
				if ( GetRenderer()->m_shaderHairGbuffer ) GetRenderer()->m_shaderHairGbuffer->Bind();
				// When to UnBind()?
			}
			else if ( context.m_pass == RP_ShadowDepthSolid )
			{
				if ( GetRenderer()->m_shaderHairShadow ) GetRenderer()->m_shaderHairShadow->Bind();
				GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_PixelShader );
			}
			else
			{
				if ( GetRenderer()->m_shaderHair ) GetRenderer()->m_shaderHair->Bind();
				normalPass = true;
			}

			// Just use cached distance directly here, rather than calling AdjustCameraDistanceSqForTextures. Fur is relatively
			// small, so this should be close enough.
			const Float textureDistance = GetCachedDistanceSquared();

			// Pass texture resources to hairworks
			for ( int idx = 0; idx < GFSDK_HAIR_NUM_TEXTURES; ++idx )
			{
				if ( m_furTextures[ idx ] == nullptr ) continue;

				// TODO : Is this all needed? hair*.fx use 0,1,2 explicitly, so maybe don't need the last two (bind all to 2, and set
				// to hairworks) or maybe we can just let hairworks bind them all, and just update distance here. Certainly the binding
				// everything to 2 seems doubtful...
				switch ( idx )
				{
				case GFSDK_HAIR_TEXTURE_ROOT_COLOR:
					m_furTextures[ idx ]->Bind( 0, RST_PixelShader, textureDistance );
					break;
				case GFSDK_HAIR_TEXTURE_TIP_COLOR:
					m_furTextures[ idx ]->Bind( 1, RST_PixelShader, textureDistance );
					break;
				case GFSDK_HAIR_TEXTURE_STRAND:
					m_furTextures[ idx ]->Bind( 2, RST_PixelShader, textureDistance );
					break;
				default:
					// Internal HairWorks texture is not loaded in proper resolution without this
					m_furTextures[ idx ]->Bind( 3, RST_PixelShader, textureDistance );
				}
				SetTextureResource( hairSDK, (GFSDK_HairInstanceID)m_instanceID, m_furTextures[ idx ], (GFSDK_HAIR_TEXTURE_TYPE)idx );
			}

			// Get the constant buffer content from HairWorks and bind into pixel shader
			{
				// Let the HairWorks fill the internal part of the constant buffer
				GFSDK_HairShaderConstantBuffer hairWorksConstantBuffer;
				GFSDK_HAIR_RETURNCODES prepareShaderConstantBufferResult = hairSDK->PrepareShaderConstantBuffer( (GFSDK_HairInstanceID)m_instanceID, (GFSDK_HairShaderConstantBuffer*)&hairWorksConstantBuffer);
				RED_ASSERT( prepareShaderConstantBufferResult == GFSDK_RETURN_OK, TXT("Fur prepare shader constant buffer failed") );
				void* bufferContent = GpuApi::LockBuffer( m_psConstantBuffer, GpuApi::BLF_Discard, 0, sizeof(hairWorksConstantBuffer) );
				Red::System::MemoryCopy( bufferContent, &hairWorksConstantBuffer, sizeof(hairWorksConstantBuffer) );
				GpuApi::UnlockBuffer( m_psConstantBuffer );
				GpuApi::BindConstantBuffer( 10, m_psConstantBuffer, GpuApi::PixelShader );
			}

			// Set hairworks shader settings
			GFSDK_HairShaderSettings settings;
			{
				settings.m_useCustomShaderResources = true; // We set shader resources ourselves
				settings.m_useCustomConstantBuffer = true; // We set our own constant buffer

				settings.m_optimizeShader = true; // Turn optimization on
				settings.m_usePixelShaderInterpolation = true;

				if (normalPass)
				{
					settings.m_shaderAttributeMask = GFSDK_HAIR_SHADER_EMIT_ALL;
					settings.m_depthFuncLess = !context.GetCamera().IsReversedProjection();
				}
				else
				{
					settings.m_shadowPass =  ( context.m_pass == RP_ShadowDepthSolid );
					settings.m_shaderAttributeMask = GFSDK_HAIR_SHADER_EMIT_NONE;
					settings.m_depthFuncLess = context.GetCamera().IsReversedProjection();
				}
			}

			// We bind additional hairworks internal resources for optimization for pixel shader side interpolation of tangent and normals
			if (normalPass)
			{
				GFSDK_HairInstanceID instanceID = (GFSDK_HairInstanceID)m_instanceID;

				int numResourceViews = 0;
				ID3D11ShaderResourceView* ppSRV[] = { 0, 0, 0 };

				hairSDK->GetShaderSRV(instanceID, GFSDK_HAIR_SHADER_RESOUCE_HAIR_INDICES, &ppSRV[numResourceViews++]);
				hairSDK->GetShaderSRV(instanceID, GFSDK_HAIR_SHADER_RESOUCE_TANGENTS, &ppSRV[numResourceViews++]);
				hairSDK->GetShaderSRV(instanceID, GFSDK_HAIR_SHADER_RESOUCE_NORMALS, &ppSRV[numResourceViews++]);

				const int additionalShaderResourceSlot = 3; // 3 slots are already used above
				ctxD3D->PSSetShaderResources( additionalShaderResourceSlot, numResourceViews, ppSRV);
			}

			// Call hairworks API to render actual hairs
			GFSDK_HAIR_RETURNCODES renderResult = hairSDK->RenderHairs( (GFSDK_HairInstanceID)m_instanceID, &settings );
			RED_ASSERT( renderResult == GFSDK_RETURN_OK, TXT("Fur rendering failed") );

#ifndef RED_FINAL_BUILD
			if (normalPass)
			{
				hairSDK->RenderVisualization( (GFSDK_HairInstanceID)m_instanceID );
			}
#endif //RED_FINAL_BUILD
		}
	}
#endif //USE_NVIDIA_FUR
}

void CRenderProxy_Fur::SetSkinningData( IRenderObject* skinningData )
{
#ifdef USE_NVIDIA_FUR
	const Bool hadSkinningData = m_skinningData != nullptr;

	Bool keepSkinningData = m_skinningData == skinningData;
	// Release previous data
	if ( !keepSkinningData )
	{
		SAFE_RELEASE( m_skinningData );
	}

	// Set new skinning data
	m_skinningData = static_cast< CRenderSkinningData* >( skinningData );
	m_skinningData->AdvanceRead();

	// Add internal reference to keep the object alive
	if ( m_skinningData && !keepSkinningData )
	{
		m_skinningData->AddRef();
	}
#endif // USE_NVIDIA_FUR
}


void CRenderProxy_Fur::SetUseShadowDistances( Bool enable )
{
	m_useShadowDistances = enable;
}

void CRenderProxy_Fur::CalculateLOD( const CRenderCollector& collector, const Bool wasVisibleLastFrame )
{
#ifdef USE_NVIDIA_FUR
	Bool drawHairs = ( GetTemporalFadeFraction() > 0.5f);
	m_defaultParams->m_drawRenderHairs = drawHairs;
	if( m_wetnessMaterialParams != nullptr )
	{
		m_wetnessMaterialParams->m_drawRenderHairs = drawHairs;
	}
#endif //USE_NVIDIA_FUR
}

const EFrameUpdateState CRenderProxy_Fur::UpdateOncePerFrame( const CRenderCollector& collector )
{
	const auto ret = IRenderProxyDrawable::UpdateOncePerFrame( collector );
	if ( ret == FUS_AlreadyUpdated )
		return ret;

	const Bool wasVisibleLastFrame = ( ret == FUS_UpdatedLastFrame );
	CalculateLOD( collector, wasVisibleLastFrame );

	return ret;
}

void CRenderProxy_Fur::CollectElements( CRenderCollector& collector )
{
	UpdateOncePerFrame( collector );

	// check auto hide distance visibility
	if ( !IsVisibleInCurrentFrame() )
		return;

	collector.m_renderCollectorData->m_furProxies.PushBack( this );

	/* Not needed for the hair, because we're interested 
	   in group based shadow removal in case of hair.

	// Collect the related entity group
	if ( GetEntityGroup() != nullptr )
	{
		collector.CollectEntityGroup( GetEntityGroup() );
	}
	*/

	// query update
	UpdateVisibilityQueryState( CRenderVisibilityQueryManager::VisibleScene );
}

void CRenderProxy_Fur::CollectCascadeShadowElements( SMergedShadowCascades& cascades, Uint32 perCascadeTestResults )
{
	PC_SCOPE_PIX( RenderProxyFur_CollectCascadeShadowElements )
	if ( !cascades.m_collector )
	{
		return;
	}

	CRenderCollector &collector = *cascades.m_collector;
	if ( !collector.m_frame->GetFrameInfo().IsShowFlagOn( SHOW_HairAndFur ) )
	{
		return;
	}

	UpdateOncePerFrame( collector );

	// Process all cascades
	const Uint32 numCascades = cascades.m_numCascades;
	for ( Uint32 cascadeIndex = 0; cascadeIndex < numCascades; ++cascadeIndex, perCascadeTestResults >>= 2 )
	{
		// Test the visibility of the fragment in this cascade
		const Uint32 visResult = perCascadeTestResults & 3;
		if ( visResult != 0 )
		{
			// We are in a cascade
			collector.m_renderCollectorData->m_furShadowProxies.PushBack( this );
			UpdateVisibilityQueryState( CRenderVisibilityQueryManager::VisibleMainShadows );
			break;
		}
	}
}

void CRenderProxy_Fur::AttachToScene( CRenderSceneEx* scene )
{
	IRenderProxyDrawable::AttachToScene( scene );
	IRenderEntityGroupProxy::AttachToScene();
}

void CRenderProxy_Fur::DetachFromScene( CRenderSceneEx* scene )
{
	IRenderProxyDrawable::DetachFromScene( scene );
	IRenderEntityGroupProxy::DetachFromScene();
}

// static
void CRenderProxy_Fur::RestoreState( const CRenderFrameInfo &info )
{
	const CCascadeShadowResources &cascadeShadowResources = GetRenderer()->GetGlobalCascadesShadowResources(); // ace_ibl_hack

	// The fur rendering tramples on some PS constants and subsequent post-processing fails without these calls.
	// We may need to fix additional states here.
	GetRenderer()->GetStateManager().Reset();

	GetRenderer()->BindForwardConsts( info, cascadeShadowResources, GetRenderer()->GetSurfaces(), true, GpuApi::PixelShader );
	GetRenderer()->BindForwardConsts( info, cascadeShadowResources, GetRenderer()->GetSurfaces(), true, GpuApi::VertexShader );
}

void CRenderProxy_Fur::UpdateFurParams( const Vector& wind, Float wetness )
{
#ifdef USE_NVIDIA_FUR
	if ( m_defaultParams )
	{
		m_defaultParams->m_wind.x = wind.X;
		m_defaultParams->m_wind.y = wind.Y;
		m_defaultParams->m_wind.z = wind.Z;
	}

	if ( m_wetnessMaterialParams )
	{
		m_wetnessMaterialWeight = wetness;
		m_wetnessMaterialParams->m_wind.x = wind.X;
		m_wetnessMaterialParams->m_wind.y = wind.Y;
		m_wetnessMaterialParams->m_wind.z = wind.Z;
	}
#endif
}

#ifndef NO_EDITOR
#ifdef USE_NVIDIA_FUR
void CRenderProxy_Fur::EditorSetFurParams( struct GFSDK_HairInstanceDescriptor* newParams, Uint32 index )
{
	if( GFSDK_HairSDK* hairSDK = GetRenderer()->GetHairSDK() )
	{
		// Set rendering control variables
		GFSDK_HAIR_RETURNCODES updateResult = GFSDK_RETURN_OK;

		switch (index)
		{
		case 0:
			delete m_defaultParams;
			m_defaultParams = newParams;
			if ( m_defaultParams )
			{
				updateResult |= hairSDK->UpdateInstanceDescriptor( (GFSDK_HairInstanceID)m_instanceID, *m_defaultParams, 0 );
			}
			break;
		case 1:
			delete m_wetnessMaterialParams;
			m_wetnessMaterialParams = newParams;
			if ( m_wetnessMaterialParams )
			{
				updateResult |= hairSDK->UpdateInstanceDescriptor( (GFSDK_HairInstanceID)m_instanceID, *m_wetnessMaterialParams, 1 );
			}
			break;
		default:
			RED_HALT( "please check this call" );
			delete newParams;
		}

		RED_ASSERT( updateResult == GFSDK_RETURN_OK, TXT("Fur update instance desc failed") );
	}
}
#endif //USE_NVIDIA_FUR
#endif //NO_EDITOR
