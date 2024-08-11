/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderElementMap.h"
#include "renderElementMeshChunk.h"
#include "renderProxyMesh.h"
#include "renderHelpers.h"
#include "renderMaterial.h"
#include "renderMesh.h"
#include "renderMorphedMesh.h"
#include "../engine/meshEnum.h"

////////////////////////////////////////////////////////////////////////////////////////////

static Uint32 CalculateMeshElementFlags( const CRenderElement_MeshChunk* element )
{
	Uint32 flags = 0;

	Bool isMorphed = false;

	if ( IRenderProxyDrawable* proxy = element->GetProxy() )
	{
		// This mesh element is owned by a mesh proxy ( not every mesh element is owned by a mesh proxy )
		if ( proxy->GetType() == RPT_Mesh )
		{
			// Get some proxy related crap
			const CRenderProxy_Mesh* meshProxy = static_cast< const CRenderProxy_Mesh* >( proxy );

			// Mesh is marked as vertex collapse
			if ( meshProxy->UsesVertexCollapse() )
			{
				flags |= RMCF_UsesVertexCollapse;
			}

			// We have skinning data
			if ( element->IsSkinnable() )
			{
				flags |= RMCF_UsesSkinning;
			}

			// No instancing supported for morphed meshes.
			if ( meshProxy->IsMorphedMesh() )
			{
				isMorphed = true;
				flags |= RMCF_NoInstancing;
			}
		}
	}

	if ( CRenderMaterial* renderMaterial = element->GetMaterial() )
	{
		RED_ASSERT( element->GetMaterialParams(), TXT("Element has a material but does not have parameters!!!") );

		// This is a mimics material
		if ( renderMaterial->IsMimicMaterial() )
		{
			flags |= RMCF_MimicsMaterial;
		}

		// We are using color shifts on this material
		if ( renderMaterial->IsUsingColorShift() )
		{
			flags |= RMCF_UseColorShift;
		}

		// We are using effect parameter on this material
		if ( renderMaterial->IsUsingEffectParam0() )
		{
			flags |= RMCF_UseEffectParam0;
		}

		// We are using second effect parameter on this material
		if ( renderMaterial->IsUsingEffectParam1() )
		{
			flags |= RMCF_UseEffectParam1;
		}
	}

	if ( CRenderMaterialParameters* materialParameters = element->GetMaterialParams() )
	{
		RED_ASSERT( element->GetMaterial(), TXT("Element has material parameters but does not have a Material!!!") );

		// This is a masked material
		if ( materialParameters->IsMasked() )
		{
			flags |= RMCF_IsMaskedMaterial;
			flags |= RMCF_AdditionalShadowElement; // that's fairly easy for now - if material is masked than it's not part of the shadow mesh - draw it regardless
		}
	}

	// Extra streams
	CRenderMesh* mesh = element->GetMesh();
	if ( mesh )
	{
		const CRenderMesh::Chunk& chunk = mesh->GetChunks()[ element->GetChunkIndex() ];
		if ( chunk.m_chunkVertices.type == GpuApi::BCT_VertexMeshStaticExtended || chunk.m_chunkVertices.type == GpuApi::BCT_VertexMeshSkinnedExtended )
		{
			flags |= RMCF_UsesExtraStreams;
		}


		if ( isMorphed )
		{
			CRenderMorphedMesh* morphedMesh = static_cast< CRenderMorphedMesh* >( mesh );
			if ( morphedMesh->IsChunkFromTarget( element->GetChunkIndex() ) )
			{
				flags |= RMCF_MorphTarget;
			}
			if ( morphedMesh->IsChunkMaterialBlended( element->GetChunkIndex() ) )
			{
				flags |= RMCF_MorphBlendMaterial;
			}
		}
	}

	// Return computed flags
	return flags;
}

////////////////////////////////////////////////////////////////////////////////////////////

CRenderElement_MeshChunk::CRenderElement_MeshChunk( IRenderProxyDrawable* proxy, CRenderMesh* mesh, Uint32 chunkIndex, IMaterial* material, Int32 lodGroupIndex, const Uint8 filterRenderMask )
	: IRenderElement( RET_MeshChunk, proxy, material )
	, m_mesh( mesh )
	, m_chunkIndex( (Uint8)chunkIndex )
	, m_baseRenderMask( mesh->GetChunks()[ chunkIndex ].m_baseRenderMask & filterRenderMask )
	, m_mergedRenderMask( mesh->GetChunks()[ chunkIndex ].m_mergedRenderMask & filterRenderMask )
	, m_lodGroupIndex( (Int8)lodGroupIndex )
	, m_canUseInstancing( false )
{
	ASSERT( chunkIndex < 255 );
	ASSERT( lodGroupIndex < 127 );

	// Get original vertex type and if it's skinned convert it to a non skinned crap
	m_vertexFactory = (EMaterialVertexFactory)m_mesh->GetChunks()[ m_chunkIndex ].m_vertexFactory;

	// Add reference to mesh
	if ( m_mesh )
	{
		m_mesh->AddRef();
	}

	if ( m_material )
	{
		// Flags
		m_elementFlags = CalculateMeshElementFlags( this );
	}

	RefreshInstancingFlag();
}

CRenderElement_MeshChunk::CRenderElement_MeshChunk( IRenderProxyDrawable* proxy, CRenderMesh* mesh, Uint32 chunkIndex, CRenderMaterial* material, CRenderMaterialParameters* parameters, Int32 lodGroupIndex, const Uint8 filterRenderMask /*= 0xFF*/ )
	: IRenderElement( (RET_MeshChunk), proxy, material, parameters )
	, m_mesh( mesh )
	, m_chunkIndex( (Uint8)chunkIndex )
	, m_lodGroupIndex( (Int8)lodGroupIndex )
	, m_baseRenderMask( mesh->GetChunks()[ chunkIndex ].m_baseRenderMask & filterRenderMask )
	, m_mergedRenderMask( mesh->GetChunks()[ chunkIndex ].m_mergedRenderMask & filterRenderMask )
	, m_canUseInstancing( false )
{
	ASSERT( chunkIndex <= 255 );
	ASSERT( lodGroupIndex < 128 );

	// Get original vertex type and if it's skinned convert it to a non skinned crap
	m_vertexFactory = (EMaterialVertexFactory)m_mesh->GetChunks()[ m_chunkIndex ].m_vertexFactory;

	// Add reference to mesh
	if ( m_mesh )
	{
		m_mesh->AddRef();
	}

	// Flags
	m_elementFlags = CalculateMeshElementFlags( this );

	RefreshInstancingFlag();
}

CRenderElement_MeshChunk::CRenderElement_MeshChunk( const CRenderElement_MeshChunk& toCopy )
	: IRenderElement( RET_MeshChunk, toCopy.m_proxy, toCopy.GetMaterial(), toCopy.GetMaterialParams() )
	, m_mesh( toCopy.m_mesh )
	, m_chunkIndex( toCopy.m_chunkIndex )
	, m_lodGroupIndex( toCopy.m_lodGroupIndex )
	, m_vertexFactory( toCopy.m_vertexFactory )
	, m_elementFlags( toCopy.m_elementFlags )
	, m_baseRenderMask( toCopy.m_baseRenderMask )
	, m_mergedRenderMask( toCopy.m_mergedRenderMask )
	, m_canUseInstancing( false )
{
	// Add reference to mesh
	if ( m_mesh )
	{
		m_mesh->AddRef();
	}

	RefreshInstancingFlag();
}

CRenderElement_MeshChunk::~CRenderElement_MeshChunk()
{
	SAFE_RELEASE( m_mesh );
}

void CRenderElement_MeshChunk::ReplaceMesh( CRenderMesh* newMesh )
{
	SAFE_COPY( m_mesh, newMesh );
}

void CRenderElement_MeshChunk::SetMeshChunkFlag( ERenderMeshChunkFlags flag, Bool flagValue /*= true*/ )
{
	if ( flag == RMCF_UsesSkinning && !IsSkinnable() )
	{
		flagValue = false;
	}

	if ( flagValue )
	{
		m_elementFlags |= flag;
	}
	else
	{
		m_elementFlags &= ~flag;
	}

	RefreshInstancingFlag();
}

CRenderElement_MeshChunk* CRenderElement_MeshChunk::CreateCopyWithMaterialOverride( CRenderMaterial* material, CRenderMaterialParameters* parameters ) const
{
	RED_ASSERT( material && parameters );

	CRenderElement_MeshChunk* newChunk = new CRenderElement_MeshChunk( *this );

	SAFE_COPY( newChunk->m_material, material );
	SAFE_COPY( newChunk->m_parameters, parameters );

	newChunk->RefreshInstancingFlag();

	newChunk->m_elementFlags = CalculateMeshElementFlags( newChunk );
	newChunk->m_baseRenderMask = m_baseRenderMask;
	newChunk->m_mergedRenderMask = m_mergedRenderMask;
	newChunk->m_sortGroup = material->GetRenderSortGroup();
	newChunk->m_elementFlags |= RMCF_MaterialOverride;

	return newChunk;
}


Bool CRenderElement_MeshChunk::IsSkinnable() const
{
	return MVF_MeshSkinned == m_vertexFactory || MVF_MeshDestruction == m_vertexFactory;
}

Bool CRenderElement_MeshChunk::IsStaticGeometryChunk() const
{
	GpuApi::eBufferChunkType chunkType = GetMesh()->GetChunks()[0].m_chunkVertices.type;
	return ( chunkType == GpuApi::BCT_VertexMeshStaticSimple ) || 
		( chunkType == GpuApi::BCT_VertexMeshStaticSimpleInstanced ) ||
		( chunkType == GpuApi::BCT_VertexMeshStaticExtended ) ||
		( chunkType == GpuApi::BCT_VertexMeshStaticExtendedInstanced ) ||
		( chunkType == GpuApi::BCT_VertexMeshStaticPositionOnly ) ||
		( chunkType == GpuApi::BCT_VertexMeshStaticPositionOnlyInstanced );
}

void CRenderElement_MeshChunk::SetChunkIndex( Uint32 newChunkIndex )
{
	if ( m_mesh && newChunkIndex < m_mesh->GetNumChunks() )
	{
		ASSERT( newChunkIndex < 255 );
		m_chunkIndex = (Uint8)newChunkIndex;
	}
}

CRenderTexture* CRenderElement_MeshChunk::GetUVDissolveTexture() const
{
	if ( m_proxy && m_proxy->GetType() == RPT_Mesh )
	{
		const CRenderProxy_Mesh* proxy = static_cast< const CRenderProxy_Mesh* >( m_proxy );
		return proxy->GetUVDissolveTexture( this );
	}
	return nullptr;
}

Vector CRenderElement_MeshChunk::GetUVDissolveValues() const
{
	if ( m_proxy && m_proxy->GetType() == RPT_Mesh )
	{
		const CRenderProxy_Mesh* proxy = static_cast< const CRenderProxy_Mesh* >( m_proxy );
		return proxy->GetUVDissolveValues( this );
	}
	else
	{
		return Vector::ZEROS;
	}
}

Bool CRenderElement_MeshChunk::DoesUVDissolveUseSeparateTexCoord() const
{
	if ( m_proxy && m_proxy->GetType() == RPT_Mesh )
	{
		const CRenderProxy_Mesh* proxy = static_cast< const CRenderProxy_Mesh* >( m_proxy );
		return proxy->DoesUVDissolveUseSeparateTexCoord( this );
	}
	else
	{
		return false;
	}
}



#ifndef NO_MESH_PROXY_DISSOLVE

Vector CRenderElement_MeshChunk::GetDissolveValues() const
{
	RED_FATAL_ASSERT( m_proxy != nullptr, "Invalid state" );
	RED_FATAL_ASSERT(  m_proxy->GetType() == RPT_Mesh, "Mesh chunk not owned by mesh proxy" );

	const CRenderProxy_Mesh* proxy = static_cast< const CRenderProxy_Mesh* >( m_proxy );
	return proxy->CalcDissolveValues( m_lodGroupIndex );
}

Vector CRenderElement_MeshChunk::GetShadowDissolveValues() const
{
	RED_FATAL_ASSERT( m_proxy != nullptr, "Invalid state" );
	RED_FATAL_ASSERT(  m_proxy->GetType() == RPT_Mesh, "Mesh chunk not owned by mesh proxy" );

	const CRenderProxy_Mesh* proxy = static_cast< const CRenderProxy_Mesh* >( m_proxy );
	return proxy->CalcShadowDissolveValues( m_lodGroupIndex );
}

#endif

void CRenderElement_MeshChunk::RefreshInstancingFlag()
{
	m_canUseInstancing = ( m_vertexFactory == MVF_MeshStatic || ( ( m_vertexFactory == MVF_MeshSkinned || m_vertexFactory == MVF_MeshDestruction ) && m_material->CanUseSkinnedInstancing() ) )

		&& !HasFlag( RMCF_NoInstancing )

		// We could maybe group by dissolve texture, and if either not using separate UV or using same extra UV data, could instance.
		&& !HasFlag( RMCF_UsesUVDissolve )

		// Could include the ellipse matrix in instance buffer, but would either require a larger size, or an extra buffer for
		// clipped objects. Would also require even more buffer chunk types (or runtime-created custom layouts). And this is mainly
		// used for skinned meshes anyways.
		&& !m_proxy->HasClippingEllipse()

		&& !HasFlag( RMCF_NormalBlendMaterial );
}
