/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderDynamicDecalChunk_Apex.h"
#include "renderDynamicDecal.h"
#include "renderShaderPair.h"
#include "apexRenderInterface.h"
#include "renderProxyApex.h"

using namespace physx::apex;

#ifdef USE_APEX


struct RenderResourceInfo
{
	Uint32						m_vbOffset;				// Offset in VB for this render resource.
	Uint32						m_numVertices;			// Number of vertices for this render resource.
	GpuApi::VertexLayoutRef		m_originalLayout;		// The vertex layout used by this render resource in the original render proxy.
	GpuApi::VertexLayoutRef		m_decalLayout;			// The modified vertex layout for decal rendering.

	RenderResourceInfo()
		: m_vbOffset( 0 )
		, m_numVertices( 0 )
	{
	}

	RenderResourceInfo( const RenderResourceInfo& info )
		: m_vbOffset( info.m_vbOffset )
		, m_numVertices( info.m_numVertices )
		, m_originalLayout( info.m_originalLayout )
		, m_decalLayout( info.m_decalLayout )
	{
		GpuApi::AddRefIfValid( m_originalLayout );
		GpuApi::AddRefIfValid( m_decalLayout );
	}

	RenderResourceInfo& operator =( const RenderResourceInfo& info )
	{
		m_vbOffset = info.m_vbOffset;
		m_numVertices = info.m_numVertices;
		GpuApi::SafeRefCountAssign( m_originalLayout, info.m_originalLayout );
		GpuApi::SafeRefCountAssign( m_decalLayout, info.m_decalLayout );
		return *this;
	}

	~RenderResourceInfo()
	{
		GpuApi::SafeRelease( m_originalLayout );
		GpuApi::SafeRelease( m_decalLayout );
	}
};


//////////////////////////////////////////////////////////////////////////


// Custom Apex renderer that allows us to generate additional vertex data, and then render with that new buffer.
class ApexDecalRenderer : public physx::apex::NxUserRenderer
{
protected:
	CRenderDynamicDecalChunk_Apex*	m_decalChunk;

	Matrix							m_localToWorld;		// L2W transform used during processing. Either the matrix given by Apex, or coming from a proxy.

	Uint32							m_current;			// While processing a proxy, there can be multiple render resources. This tracks which we're currently on.


public:

	ApexDecalRenderer( CRenderDynamicDecalChunk_Apex* decalChunk )
		: m_decalChunk( decalChunk )
	{}

	void RenderDecal()
	{
		PC_SCOPE_RENDER_LVL1(RenderDynamicDecalChunk_Apex);

		if ( m_decalChunk == nullptr || m_decalChunk->GetOwnerDecal() == nullptr || m_decalChunk->m_renderProxy == nullptr )
		{
			RED_HALT( "Invalid apex decal chunk!" );
			return;
		}

		physx::apex::NxApexRenderable* renderable = m_decalChunk->m_renderProxy->GetApexRenderable();
		if ( renderable == nullptr )
		{
			WARN_RENDERER( TXT("Can't render decal chunk when its proxy has no renderable set.") );
			return;
		}

		// Bind default quantization. Decals use the same shaders for meshes and apex, so we need to account for quantization
		GetRenderer()->GetStateManager().SetVertexConst( VSC_QS, Vector::ONES );
		GetRenderer()->GetStateManager().SetVertexConst( VSC_QB, Vector::ZEROS );

		renderable->lockRenderResources();

		m_current = 0;
		renderable->dispatchRenderResources( *this );

		renderable->unlockRenderResources();
	}


public:

	virtual void renderResource( const NxApexRenderContext& context )
	{
		CApexRenderResource* resource = static_cast< CApexRenderResource* >( context.renderResource );

		Vector c0( context.local2world.column0.x, context.local2world.column0.y, context.local2world.column0.z, context.local2world.column0.w );
		Vector c1( context.local2world.column1.x, context.local2world.column1.y, context.local2world.column1.z, context.local2world.column1.w );
		Vector c2( context.local2world.column2.x, context.local2world.column2.y, context.local2world.column2.z, context.local2world.column2.w );
		Vector c3( context.local2world.column3.x, context.local2world.column3.y, context.local2world.column3.z, context.local2world.column3.w );
		m_localToWorld = Matrix(c0, c1, c2, c3);


		//
		// Render the decal. This assumes we get the render resources in the same order as the ApexDecalGenerator did.
		//

		// If there's a different number of render resources, we shouldn't draw -- different LOD or a broken up destructible...
		if ( m_current >= m_decalChunk->m_resourceInfo.Size() )
		{
			return;
		}

		RenderResourceInfo& resourceInfo = m_decalChunk->m_resourceInfo[m_current];

		Uint32 numVertices = resource->m_numVertices;

		// If there's a different number of vertices than we were generated with, then we shouldn't draw -- probably from a different LOD.
		// TODO : Maybe regenerate decal instead of not drawing? Or force cloth LOD to not change after decal applied?
		if ( numVertices != resourceInfo.m_numVertices )
		{
			return;
		}

		// We can assume that we're only going to be drawing things that have an index buffer. Only the debug geometry doesn't use one, and
		// we aren't putting decals on that!
		RED_ASSERT( resource->m_indexBuffer != nullptr );
		resource->m_indexBuffer->Bind();

		if ( resource->m_boneBuffer != nullptr )
		{
			resource->m_boneBuffer->Bind();
		}

		GetRenderer()->GetStateManager().SetLocalToWorld( &m_localToWorld );

		GpuApi::BufferRef buffers[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS];
		Uint32 strides[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS] = { 0 };
		Uint32 offsets[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS] = { 0 };

		RED_ASSERT( resource->m_numVB <= GPUAPI_VERTEX_LAYOUT_MAX_SLOTS-1, TXT("Unsupported number of VBs: %d"), resource->m_numVB );
		if ( resource->m_numVB > GPUAPI_VERTEX_LAYOUT_MAX_SLOTS-1 ) return;

		for ( Uint32 i = 0; i < resource->m_numVB; ++i )
		{
			buffers[i] = resource->m_vertexBuffers[i]->m_vb;
			strides[i] = resource->m_vertexBuffers[i]->m_stride;
			offsets[i] = resource->m_vertexBuffers[i]->m_stride * resource->m_firstVertex;
		}

		buffers[resource->m_numVB] = m_decalChunk->m_vertexBuffer;
		strides[resource->m_numVB] = GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexDynamicDecalGenOut );
		offsets[resource->m_numVB] = resourceInfo.m_vbOffset;


		// If we don't have a layout yet, or the layout from the resource is different from what we know (it's changed), rebuild decal
		// layout. The layout can change when a destructible breaks apart (gains a bone index), or when cloth switches between simulated
		// and skinned. Using the same decal vertex buffer seems to still work nicely, so that's cool.
		if ( resourceInfo.m_decalLayout.isNull() || resource->m_vertexLayout != resourceInfo.m_originalLayout )
		{
			// Create a modified layout based on the render resource's original one. We just need to add TexCoord4 to hold the decal
			// information. TexCoord4 is used to ensure that it's not going to collide with anything that might otherwise be in use.
			GpuApi::VertexLayoutDesc modifiedLayoutDesc = *GpuApi::GetVertexLayoutDesc( resource->m_vertexLayout );

			// NOTE : This used to just add a new TEXCOORD4 element to the end of the desc. It's been changed as a workaround, there
			// seems to be something wrong with PS4's semantic remapping or fetch shaders, or something. When TEXCOORD4 was just added
			// to the end, I was seeing in razor that the ES stage was still reading in elements that didn't really match anything
			// (float2, half4, float4, float4). Replacing TEXCOORD0 with the decal's TEXCOORD4 seems to work.
			for ( Uint32 i = 0; i < GPUAPI_VERTEX_LAYOUT_MAX_ELEMENTS; ++i )
			{
				// If we've reached the end, just append texcoord4.
				if ( modifiedLayoutDesc.m_elements[ i ].IsEmpty() )
				{
					modifiedLayoutDesc.m_elements[ i ].m_type = GpuApi::VertexPacking::PT_Float16_4;
					modifiedLayoutDesc.m_elements[ i ].m_usage = GpuApi::VertexPacking::PS_TexCoord;
					modifiedLayoutDesc.m_elements[ i ].m_usageIndex = 4;
					modifiedLayoutDesc.m_elements[ i ].m_slot = (Uint8)resource->m_numVB;
					modifiedLayoutDesc.m_elements[ i ].m_slotType = GpuApi::VertexPacking::ST_PerVertex;
					break;
				}

				// Replace texcoord0 with the appropriate decal data.
				if ( modifiedLayoutDesc.m_elements[ i ].m_usage == GpuApi::VertexPacking::PS_TexCoord && modifiedLayoutDesc.m_elements[i].m_usageIndex == 0 )
				{
					modifiedLayoutDesc.m_elements[ i ].m_usageIndex = 4;
					modifiedLayoutDesc.m_elements[ i ].m_type = GpuApi::VertexPacking::PT_Float16_4;
					modifiedLayoutDesc.m_elements[ i ].m_slot = (Uint8)resource->m_numVB;
					modifiedLayoutDesc.m_elements[ i ].m_slotType = GpuApi::VertexPacking::ST_PerVertex;
					break;
				}
			}
			modifiedLayoutDesc.UpdateStrides();

			GpuApi::SafeRelease( resourceInfo.m_decalLayout );
			resourceInfo.m_decalLayout = GpuApi::CreateVertexLayout( modifiedLayoutDesc );
			GpuApi::SafeRefCountAssign( resourceInfo.m_originalLayout, resource->m_vertexLayout );
		}


		GpuApi::SetVertexFormatRaw( resourceInfo.m_decalLayout, false );
		GpuApi::BindVertexBuffers( 0, resource->m_numVB+1, buffers, strides, offsets );


		// Since the apex decal chunk's GetShader will return null, the batcher has not set a shader for us. So we need to bind it ourselves.
		CRenderShaderTriple* shader = m_decalChunk->SelectShader( resource->m_boneBuffer != nullptr, resourceInfo.m_decalLayout );
		RED_ASSERT( shader != nullptr, TXT("Could not select a shader for dynamic decal chunk") );
		if ( shader == nullptr )
		{
			return;
		}
		shader->Bind();
#ifdef DYNAMIC_DECAL_NO_GS_CULLING
		GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );
#endif

		Uint32 numPrims = GpuApi::MapDrawElementCountToPrimitiveCount( resource->m_primitiveType, resource->m_numIndices );
		GpuApi::DrawIndexedPrimitive( resource->m_primitiveType, 0, 0, resource->m_numVertices, resource->m_firstIndex, numPrims );

		++m_current;
	}

};


//////////////////////////////////////////////////////////////////////////


// Custom Apex renderer that allows us to generate additional vertex data, and then render with that new buffer.
class ApexDecalGenerator : public physx::apex::NxUserRenderer
{
protected:

	enum Phase { PHASE_CountVertices, PHASE_Generate };

	CRenderDynamicDecal*			m_decal;
	CRenderProxy_Apex*				m_renderProxy;

	Uint32							m_current;			// While processing a proxy, there can be multiple render resources. This tracks which we're currently on.
	Uint32							m_totalVertices;	// While counting vertices, this just tracks the total number of vertices.

	Phase							m_phase;			// What type of processing should be done.


	TDynArray< RenderResourceInfo >	m_resourceInfo;
	GpuApi::BufferRef				m_vertexBuffer;


public:

	ApexDecalGenerator( CRenderDynamicDecal* decal, CRenderProxy_Apex* renderProxy )
		: m_decal( decal )
		, m_renderProxy( renderProxy )
	{
	}

	virtual ~ApexDecalGenerator()
	{
		RED_FATAL_ASSERT( !m_vertexBuffer, "m_vertexBuffer should be null here!" );
	}


	CRenderDynamicDecalChunk_Apex* GenerateDecal()
	{
		// TODO : Sadly, especially for large destructibles, this is quite wasteful. We can end up with far more geometry than we actually need.
		// Since the entire destructible is in a single apex renderable, and apparently even a single render resource, the decal geometry covers
		// the entire destructible, even it if only exists on a small section.
		// We could possibly run a second pass over the generated geometry, and use a SO query to track how many triangles are actually in the
		// decal's area, and re-generate the geometry when the query is ready... wouldn't work for cloth, but could help with destructibles. Would
		// give a large initial buffer, but then it would be shrunk down shortly after with just what it needs.
		// The re-generated geometry would need to include full vertex data, not just the decal coordinates.
		// On the other hand, my tests were with a constant stream of decals spawning on the same place, so maybe in real-world situations it won't
		// be a problem.

		PC_SCOPE_RENDER_LVL1(GenerateDynamicDecalChunk_Apex);
		
		RED_ASSERT( m_renderProxy != nullptr, TXT("Can't generate an apex decal for null apex proxy!") );
		RED_ASSERT( m_decal != nullptr, TXT("Can't generate an apex decal for null decal!") );
		if ( m_renderProxy == nullptr || m_decal == nullptr )
		{
			return nullptr;
		}

		physx::apex::NxApexRenderable* renderable = m_renderProxy->GetApexRenderable();
		if ( renderable == nullptr )
		{
			WARN_RENDERER( TXT("Apex proxy has no renderable set. Skipping decal generation.") );
			return nullptr;
		}

		CRenderDynamicDecalChunk_Apex* decalChunk = nullptr;

		renderable->lockRenderResources();

		// First a quick pass, to count up how big a vertex buffer we'll need to create.
		m_phase = PHASE_CountVertices;
		m_resourceInfo.ClearFast();
		m_totalVertices = 0;
		renderable->dispatchRenderResources( *this );

		// If we have vertices to generate, create the buffer and do another pass through the renderable. This time,
		// we'll be passing the vertices through a SO shader, outputting the decal data.
		if ( m_totalVertices > 0 )
		{
			m_vertexBuffer = GpuApi::CreateBuffer( m_totalVertices * GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexDynamicDecalGenOut ), GpuApi::BCC_Vertex, GpuApi::BUT_StreamOut, 0 );
			RED_ASSERT( m_vertexBuffer, TXT("Failed to create vertex buffer for dynamic decal apex chunk") );
			if ( m_vertexBuffer )
			{
				// Bind default quantization. Decals use the same shaders for meshes and apex, so we need to account for quantization
				GetRenderer()->GetStateManager().SetVertexConst( VSC_QS, Vector::ONES );
				GetRenderer()->GetStateManager().SetVertexConst( VSC_QB, Vector::ZEROS );

				Matrix mtx = m_decal->GetWorldToDecalParent() * m_decal->GetParentToDecal();
				m_decal->BindGenerateParameters( mtx );

				m_phase = PHASE_Generate;
				m_current = 0;
				renderable->dispatchRenderResources( *this );

				// Clear out SO buffers and shader.
				GpuApi::BindStreamOutBuffers( 0, nullptr, nullptr );
				GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_GeometryShader );


				decalChunk = new CRenderDynamicDecalChunk_Apex( m_decal, m_renderProxy );

				// Swap resource infos. We don't need ours anymore, and we can avoid any extra memory operations.
				decalChunk->m_resourceInfo.SwapWith( m_resourceInfo );

				// Transfer ownership of vertex buffer to the decal chunk.
				decalChunk->m_vertexBuffer = m_vertexBuffer;
				m_vertexBuffer = GpuApi::BufferRef::Null();
			}
		}

		renderable->unlockRenderResources();


		return decalChunk;
	}

protected:

	void CountVertices( CApexRenderResource* resource )
	{
		// Count vertices for this render resource, and update the buffer offset/size arrays.
		RenderResourceInfo* info = new ( m_resourceInfo ) RenderResourceInfo();
		info->m_vbOffset		= m_totalVertices * GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexDynamicDecalGenOut );
		info->m_numVertices		= resource->m_numVertices;
		GpuApi::SafeRefCountAssign( info->m_originalLayout, resource->m_vertexLayout );

		m_totalVertices += resource->m_numVertices;
	}

	void Generate( CApexRenderResource* resource, const Matrix& localToWorld )
	{
		// Generate decal tex coords for this render resource. Since Generate comes after CountVertices, we already have the
		// stream-out vertex buffer created, and know where to start writing.
		// This assumes that Apex is giving us the render resources in the same order as before, but that seems to be the case.

		CApexBoneBuffer* bb = ( CApexBoneBuffer* )resource->m_boneBuffer;
		if ( bb )
		{
			bb->Bind();
		}

		GpuApi::BufferRef buffers[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS];
		Uint32 strides[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS] = { 0 };
		Uint32 offsets[GPUAPI_VERTEX_LAYOUT_MAX_SLOTS] = { 0 };

		RED_ASSERT( resource->m_numVB <= GPUAPI_VERTEX_LAYOUT_MAX_SLOTS-1, TXT("Unsupported number of VBs: %d"), resource->m_numVB );
		if ( resource->m_numVB > GPUAPI_VERTEX_LAYOUT_MAX_SLOTS-1 ) return;

		for ( Uint32 i = 0; i < resource->m_numVB; ++i )
		{
			buffers[i] = resource->m_vertexBuffers[i]->m_vb;
			strides[i] = resource->m_vertexBuffers[i]->m_stride;
			offsets[i] = resource->m_vertexBuffers[i]->m_stride * resource->m_firstVertex;
		}
		GpuApi::SetVertexFormatRaw( resource->m_vertexLayout, false );
		GpuApi::BindVertexBuffers( 0, resource->m_numVB, buffers, strides, offsets );

		GpuApi::BindStreamOutBuffers( 1, &m_vertexBuffer, &m_resourceInfo[ m_current ].m_vbOffset );


		// Apex decals are always applied in world space.
		/*
		if ( m_decalChunk->GetOwnerDecal()->ApplyInLocalSpace() )
		{
			GetRenderer()->m_shaderDynamicDecalGen->Bind();
			GetRenderer()->GetStateManager().SetLocalToWorld( &Matrix::IDENTITY );
		}
		else
		*/
		{
			// Apply in current global pose.
			if ( bb != nullptr )
			{
				const GpuApi::VertexLayoutDesc* layoutDesc = GpuApi::GetVertexLayoutDesc( resource->m_vertexLayout );
				RED_ASSERT( layoutDesc != nullptr );
				if ( layoutDesc->GetUsageOffset( GpuApi::VertexPacking::PS_SkinWeights, 0 ) < 0 )
				{
					GetRenderer()->m_shaderDynamicDecalSingleSkinnedGen->Bind();
				}
				else
				{
					GetRenderer()->m_shaderDynamicDecalSkinnedGen->Bind();
				}
			}
			else
			{
				GetRenderer()->m_shaderDynamicDecalGen->Bind();
			}
			GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld );
		}
		GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_PixelShader );

		// Point list, since we're going to use the existing vertex buffers and index buffer, so we just need to write out per-vertex
		// information, not a full triangle list.
		GpuApi::DrawPrimitive( GpuApi::PRIMTYPE_PointList, resource->m_firstVertex, resource->m_numVertices );

		++m_current;
	}


public:

	virtual void renderResource( const NxApexRenderContext& context ) override
	{
		// Redirect to the appropriate function based on the current phase.

		CApexRenderResource* resource = static_cast< CApexRenderResource* >( context.renderResource );

		// Pass off to proper function.
		switch ( m_phase )
		{
		case PHASE_CountVertices:
			CountVertices( resource );
			break;

		case PHASE_Generate:
			{
				Vector c0( context.local2world.column0.x, context.local2world.column0.y, context.local2world.column0.z, context.local2world.column0.w );
				Vector c1( context.local2world.column1.x, context.local2world.column1.y, context.local2world.column1.z, context.local2world.column1.w );
				Vector c2( context.local2world.column2.x, context.local2world.column2.y, context.local2world.column2.z, context.local2world.column2.w );
				Vector c3( context.local2world.column3.x, context.local2world.column3.y, context.local2world.column3.z, context.local2world.column3.w );
				Matrix localToWorld = Matrix(c0, c1, c2, c3);
				Generate( resource, localToWorld );
			}
			break;

		default:
			RED_HALT( "Invalid phase: %d", m_phase );
			break;
		}
	}

};


//////////////////////////////////////////////////////////////////////////


CRenderDynamicDecalChunk_Apex* CRenderDynamicDecalChunk_Apex::Create( CRenderDynamicDecal* ownerDecal, CRenderProxy_Apex* renderProxy )
{
	ApexDecalGenerator genRenderer( ownerDecal, renderProxy );
	return genRenderer.GenerateDecal();
}


CRenderDynamicDecalChunk_Apex::CRenderDynamicDecalChunk_Apex( CRenderDynamicDecal* ownerDecal, CRenderProxy_Apex* renderProxy )
	: CRenderDynamicDecalChunk( ownerDecal, renderProxy )
	, m_renderProxy( renderProxy )
{
}

CRenderDynamicDecalChunk_Apex::~CRenderDynamicDecalChunk_Apex()
{
	GpuApi::SafeRelease( m_vertexBuffer );
}


void CRenderDynamicDecalChunk_Apex::Render( const RenderingContext& context, const CRenderFrameInfo& frameInfo )
{
	RED_ASSERT( m_vertexBuffer, TXT("Dynamic decal apex chunk without any vertex buffer! How can this be?") );
	if ( m_vertexBuffer )
	{
		ApexDecalRenderer renderer( this );
		renderer.RenderDecal();
	}
}


Uint32 CRenderDynamicDecalChunk_Apex::GetUsedVideoMemory() const
{
	if ( m_vertexBuffer.isNull() )
	{
		return 0;
	}

	return GpuApi::GetBufferDesc( m_vertexBuffer ).CalcUsedVideoMemory();
}


#endif
