/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "renderCollector.h"
#include "renderProxyStripe.h"
#include "renderMesh.h"
#include "renderTexture.h"
#include "renderShaderPair.h"
#include "../engine/renderFragment.h"
#include "../engine/stripeComponent.h"
#include "../engine/renderSettings.h"

#ifndef NO_EDITOR
// Make sure this is in sync with stripeComponent.h
#define RENDER_SCEF_VISUALIZE_BLEND		0x0002
#endif

//////////////////////////////////////////////////////////////////////////

static CRenderShaderPair* GetShaderPairForProperties( const SRenderProxyStripeProperties& properties, Bool msaa )
{
#ifndef NO_EDITOR
	extern Uint32 GStripeComponentEditorFlags;

	if ( GStripeComponentEditorFlags & SCEF_VISUALIZE_BLEND )
	{
		return properties.m_projectToTerrain ? GetRenderer()->m_shaderStripeProjectedBlendVis : GetRenderer()->m_shaderStripeRegularBlendVis;
	}
#endif

	CRenderShaderPair*	const shaderLookup[] =
	{
		GetRenderer()->m_shaderStripeRegularSingle,							// 0 0 0 0 0
		GetRenderer()->m_shaderStripeRegularSingle,							// 0 0 0 0 1
		GetRenderer()->m_shaderStripeRegularSingleNormals,					// 0 0 0 1 0
		GetRenderer()->m_shaderStripeRegularSingleNormals,					// 0 0 0 1 1
		GetRenderer()->m_shaderStripeRegularDouble,							// 0 0 1 0 0
		GetRenderer()->m_shaderStripeRegularDoubleSecondNormals,			// 0 0 1 0 1
		GetRenderer()->m_shaderStripeRegularDoubleFirstNormals,				// 0 0 1 1 0
		GetRenderer()->m_shaderStripeRegularDoubleBothNormals,				// 0 0 1 1 1

		GetRenderer()->m_shaderStripeRegularSingle_MSAA,					// 0 1 0 0 0
		GetRenderer()->m_shaderStripeRegularSingle_MSAA,					// 0 1 0 0 1
		GetRenderer()->m_shaderStripeRegularSingleNormals_MSAA,				// 0 1 0 1 0
		GetRenderer()->m_shaderStripeRegularSingleNormals_MSAA,				// 0 1 0 1 1
		GetRenderer()->m_shaderStripeRegularDouble_MSAA,					// 0 1 1 0 0
		GetRenderer()->m_shaderStripeRegularDoubleSecondNormals_MSAA,		// 0 1 1 0 1
		GetRenderer()->m_shaderStripeRegularDoubleFirstNormals_MSAA ,		// 0 1 1 1 0
		GetRenderer()->m_shaderStripeRegularDoubleBothNormals_MSAA ,		// 0 1 1 1 1

		GetRenderer()->m_shaderStripeProjectedSingle,						// 1 0 0 0 0
		GetRenderer()->m_shaderStripeProjectedSingle,						// 1 0 0 0 1
		GetRenderer()->m_shaderStripeProjectedSingleNormals,				// 1 0 0 1 0
		GetRenderer()->m_shaderStripeProjectedSingleNormals,				// 1 0 0 1 1
		GetRenderer()->m_shaderStripeProjectedDouble,						// 1 0 1 0 0
		GetRenderer()->m_shaderStripeProjectedDoubleSecondNormals,			// 1 0 1 0 1
		GetRenderer()->m_shaderStripeProjectedDoubleFirstNormals,			// 1 0 1 1 0
		GetRenderer()->m_shaderStripeProjectedDoubleBothNormals,			// 1 0 1 1 1

		GetRenderer()->m_shaderStripeProjectedSingle_MSAA,					// 1 1 0 0 0
		GetRenderer()->m_shaderStripeProjectedSingle_MSAA,					// 1 1 0 0 1
		GetRenderer()->m_shaderStripeProjectedSingleNormals_MSAA,			// 1 1 0 1 0
		GetRenderer()->m_shaderStripeProjectedSingleNormals_MSAA,			// 1 1 0 1 1
		GetRenderer()->m_shaderStripeProjectedDouble_MSAA,					// 1 1 1 0 0
		GetRenderer()->m_shaderStripeProjectedDoubleSecondNormals_MSAA,		// 1 1 1 0 1
		GetRenderer()->m_shaderStripeProjectedDoubleFirstNormals_MSAA ,		// 1 1 1 1 0
		GetRenderer()->m_shaderStripeProjectedDoubleBothNormals_MSAA ,		// 1 1 1 1 1
	};

	Uint32 shaderMask = 0;
	shaderMask |= FLAG(4) * properties.m_projectToTerrain;
	shaderMask |= FLAG(3) * msaa;
	shaderMask |= FLAG(2) * ( properties.m_blendTexture && properties.m_diffuseTexture2 );
	shaderMask |= FLAG(1) * ( properties.m_normalTexture != nullptr );
	shaderMask |= FLAG(0) * ( properties.m_normalTexture2 != nullptr );

	RED_FATAL_ASSERT( shaderMask < ARRAY_COUNT(shaderLookup) , "Invalid id of stripe shader" );
	
	return shaderLookup[shaderMask];
}

//////////////////////////////////////////////////////////////////////////

CRenderProxy_Stripe::CRenderProxy_Stripe( const RenderProxyInitInfo& info ) 
	: IRenderProxyDrawable( RPT_Stripe, info )
{
#ifdef USE_UMBRA
	if ( info.m_component )
	{
		const CStripeComponent* stripeComponent = static_cast< const CStripeComponent* >( info.m_component );
		if ( stripeComponent )
		{
			m_umbraProxyId = GlobalVisID( stripeComponent->GetOcclusionId(), GetLocalToWorld() );
		}
	}
#endif
}

CRenderProxy_Stripe::~CRenderProxy_Stripe()
{
	SAFE_RELEASE( m_properties.m_depthTexture );
	SAFE_RELEASE( m_properties.m_blendTexture );
	SAFE_RELEASE( m_properties.m_normalTexture );
	SAFE_RELEASE( m_properties.m_normalTexture2 );
	SAFE_RELEASE( m_properties.m_diffuseTexture );
	SAFE_RELEASE( m_properties.m_diffuseTexture2 );
	GpuApi::SafeRelease( m_indexBuffer );
	GpuApi::SafeRelease( m_vertexBuffer );
}

void CRenderProxy_Stripe::UpdateProperties( const SRenderProxyStripeProperties& properties )
{
	SAFE_COPY( m_properties.m_diffuseTexture,	properties.m_diffuseTexture );
	SAFE_COPY( m_properties.m_diffuseTexture2,	properties.m_diffuseTexture2 );
	SAFE_COPY( m_properties.m_normalTexture,	properties.m_normalTexture );
	SAFE_COPY( m_properties.m_normalTexture2,	properties.m_normalTexture2 );
	SAFE_COPY( m_properties.m_blendTexture,		properties.m_blendTexture );
	SAFE_COPY( m_properties.m_depthTexture,		properties.m_depthTexture );

	m_properties.m_boundingBox					= properties.m_boundingBox;
	m_properties.m_projectToTerrain				= properties.m_projectToTerrain;

	m_properties.m_vertices						= properties.m_vertices;
	m_properties.m_indices						= properties.m_indices;
}

void CRenderProxy_Stripe::Prefetch( CRenderFramePrefetch* prefetch ) const
{
	const Float distanceSq = CalcCameraDistanceSqForTextures( prefetch->GetCameraPosition(), prefetch->GetCameraFovMultiplierUnclamped() );

	prefetch->AddTextureBind( static_cast< CRenderTextureBase* >( m_properties.m_diffuseTexture ),	distanceSq );
	prefetch->AddTextureBind( static_cast< CRenderTextureBase* >( m_properties.m_diffuseTexture2 ),	distanceSq );
	prefetch->AddTextureBind( static_cast< CRenderTextureBase* >( m_properties.m_normalTexture ),	distanceSq );
	prefetch->AddTextureBind( static_cast< CRenderTextureBase* >( m_properties.m_normalTexture2 ),	distanceSq );
	prefetch->AddTextureBind( static_cast< CRenderTextureBase* >( m_properties.m_blendTexture ),	distanceSq );
	prefetch->AddTextureBind( static_cast< CRenderTextureBase* >( m_properties.m_depthTexture ),	distanceSq );
}


void CRenderProxy_Stripe::Render( const class RenderingContext& context, const CRenderFrameInfo& frameInfo, CRenderCollector* renderCollector )
{
	// not collected this frame
	if ( m_frameTracker.GetLastUpdateFrameIndex() < renderCollector->m_frameIndex )
		return;

	// check visibility distance
	if ( !IsVisibleInCurrentFrame() )
		return;

	ASSERT( sizeof(GpuApi::SystemVertex_Stripe) == sizeof(SRenderProxyStripeProperties::Vertex), TXT("Stripe vertices do no match! This will either crash or show garbage!") );

	// Check light channels
	if ( !context.CheckLightChannels( m_lightChannels ) )
	{
		return;
	}

	// Vertices arrived, we need to rebuild the vertex and index buffers
	if ( !m_properties.m_vertices.Empty() )
	{
		// Release the previous buffers (if any)
		GpuApi::SafeRelease( m_indexBuffer );
		GpuApi::SafeRelease( m_vertexBuffer );

		// Update bounding box
		m_boundingBox = m_properties.m_boundingBox;

		// Create vertex buffer
		{
			m_vertexCount = m_properties.m_vertices.Size();
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = m_properties.m_vertices.TypedData();
			m_vertexBuffer = GpuApi::CreateBuffer( m_vertexCount*sizeof(GpuApi::SystemVertex_Stripe), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
			if ( !m_vertexBuffer )
			{
				LOG_RENDERER( TXT("Failed to create the vertex buffer for the strip") );
				return;
			}
			GpuApi::SetBufferDebugPath( m_vertexBuffer, "stripeVB" );
		}
		
		// Create index buffer
		{
			m_indexCount = m_properties.m_indices.Size();
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = m_properties.m_indices.TypedData();
			m_indexBuffer = GpuApi::CreateBuffer( m_indexCount*sizeof(Uint16), GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );
			if ( !m_indexBuffer )
			{
				GpuApi::SafeRelease( m_vertexBuffer );
				LOG_RENDERER( TXT("Failed to create the index buffer for he strip") );
				return;
			}
			GpuApi::SetBufferDebugPath( m_indexBuffer, "stripeIB" );
		}

		// Clear the vertices, we don't need them anymore
		m_properties.m_vertices.Clear();
	}

	// Make sure we have actual geometry to render
	if ( !m_properties.m_diffuseTexture || !m_indexBuffer || !m_vertexBuffer || !m_indexCount || !m_vertexCount )
	{
		return;
	}

	// Get the proper shader pair
	CRenderShaderPair* shaderPair = GetShaderPairForProperties( m_properties, GRender->IsMSAAEnabled( frameInfo ) );
	
	// Make sure the pair is valid
	if ( !shaderPair->IsValid() )
	{
		return;
	}

	// Bind the pair
	shaderPair->Bind();

	const Float textureDistance = AdjustCameraDistanceSqForTextures( GetCachedDistanceSquared() );

	// Bind the textures
	static_cast< CRenderTexture* >( m_properties.m_diffuseTexture )->Bind( 0, RST_PixelShader, textureDistance );
	if ( m_properties.m_normalTexture )
	{
		static_cast< CRenderTexture* >( m_properties.m_normalTexture )->Bind( 1, RST_PixelShader, textureDistance );
	}
	if ( m_properties.m_blendTexture )
	{
		if ( m_properties.m_diffuseTexture2 )
		{
			static_cast< CRenderTexture* >( m_properties.m_diffuseTexture2 )->Bind( 2, RST_PixelShader, textureDistance );
			if ( m_properties.m_normalTexture2 )
			{
				static_cast< CRenderTexture* >( m_properties.m_normalTexture2 )->Bind( 3, RST_PixelShader, textureDistance );
			}
		}
		static_cast< CRenderTexture* >( m_properties.m_blendTexture )->Bind( 4, RST_PixelShader, textureDistance );
	}
	if ( m_properties.m_depthTexture )
	{
		static_cast< CRenderTexture* >( m_properties.m_depthTexture )->Bind( 5, RST_PixelShader, textureDistance );
	}
	else
	{
		GpuApi::TextureRef nulltex = GpuApi::TextureRef::Null();
		GpuApi::BindTextures( 5, 1, &nulltex, GpuApi::PixelShader );
	}

	// Set parameters
	GetRenderer()->GetStateManager().SetLocalToWorld( &m_localToWorld );

	// Bind buffers
	Uint32 vbSstride = GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexSystemStripe );
	Uint32 vbOffset = 0;
	GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemStripe );
	GpuApi::BindVertexBuffers( 0, 1, &m_vertexBuffer, &vbSstride, &vbOffset );
	GpuApi::BindIndexBuffer( m_indexBuffer );

	// Draw the triangles
	GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, m_vertexCount, 0, m_indexCount / 3 );
}

void CRenderProxy_Stripe::CollectElements( CRenderCollector& collector )
{
	UpdateOncePerFrame( collector );
}

void CRenderProxy_Stripe::CollectTextures( TDynArray< CRenderTextureBase* >& outTextures ) const
{
	IRenderResource* texs[] = {
		m_properties.m_diffuseTexture,
		m_properties.m_diffuseTexture2,
		m_properties.m_normalTexture,
		m_properties.m_normalTexture2,
		m_properties.m_blendTexture,
		m_properties.m_depthTexture
	};

	for ( IRenderResource* tex : texs )
	{
		if ( tex != nullptr )
		{
			outTextures.PushBack( static_cast< CRenderTextureBase* >( tex ) );
		}
	}
}
