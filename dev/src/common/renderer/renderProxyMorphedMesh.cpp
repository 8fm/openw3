/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderProxyMorphedMesh.h"
#include "renderElementMeshChunk.h"
#include "renderScene.h"
#include "renderMorphedMesh.h"
#include "../engine/morphedMeshComponent.h"
#include "../engine/mesh.h"
#include "../engine/material.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////


class CRenderMorphedMeshMaterialProvider : public IRenderMeshChunkMaterialProvider
{
private:
	const CRenderMorphedMesh* m_mesh;

public:
	CRenderMorphedMeshMaterialProvider( const CRenderMorphedMesh* mesh )
		: m_mesh( mesh )
	{
	}


	virtual Bool GetMaterialForChunk( Uint8 chunkIndex, CRenderMaterial*& outMaterial, CRenderMaterialParameters*& outMaterialParams ) const
	{
		if ( chunkIndex >= m_mesh->GetNumChunks() )
		{
			return false;
		}

		SAFE_COPY( outMaterial, m_mesh->GetChunkMaterial( chunkIndex ) );
		SAFE_COPY( outMaterialParams, m_mesh->GetChunkMaterialParameters( chunkIndex ) );

		return true;
	}
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////


CRenderProxy_MorphedMesh::CRenderProxy_MorphedMesh( const RenderProxyInitInfo& initInfo )
	: CRenderProxy_Mesh( initInfo, INIT_SUBCLASS )
	, m_morphRatio( 0.f )
{

	RED_ASSERT( 1.f == GetShadowFadeFraction() );

	SetMeshProxyFlagsFromComponent( initInfo.m_component );
	SetDefaultEffectParameters( initInfo.m_component );

	const CMorphedMeshComponent* component = static_cast< const CMorphedMeshComponent* >( initInfo.m_component );
	if ( component != nullptr )
	{
		CMesh* morphSource = component->GetMorphSource();
		CMesh* morphTarget = component->GetMorphTarget();
		CMesh* meshForSetup = morphSource ? morphSource : morphTarget;

		if ( meshForSetup != nullptr )
		{
			SetupUmbra( meshForSetup, initInfo.m_component );

			SetupAutohideDistance( meshForSetup, initInfo );


			Uint8 allowedRenderMask = CalculateAllowedRenderMask( meshForSetup );

			const Int32 forcedLOD = ExtractForceLODLevel( initInfo );

			CRenderMorphedMesh* morphedMesh = CRenderMorphedMesh::Create( component, 0 );
			if ( morphedMesh != nullptr )
			{
				m_mesh = morphedMesh;

				// NOTE : At least the proxy flag needs to be set before setting up render elements in CreateLodGroups.
				if ( initInfo.m_usesVertexCollapse )
				{
					m_mesh->InitBonePositionTexture( meshForSetup );
					SetMeshProxyFlag( RMPF_UsesVertexCollapse );
				}

				CRenderMorphedMeshMaterialProvider materialProvider( morphedMesh );
				CreateLodGroups( materialProvider, forcedLOD, allowedRenderMask );
			}

			// TODO : Find out if this can just be included with CalculateAllowedRenderMask. In original code,
			// this was done after creating lod groups, so I left it there...
			allowedRenderMask = PatchAllowedRenderMask( allowedRenderMask );
			SetupRenderMask( allowedRenderMask );

			m_morphRatio = component->GetMorphRatio();
		}
	}

#ifndef RED_FINAL_BUILD
	if ( m_renderMask == 0 && initInfo.m_component )
	{
		WARN_RENDERER( TXT("Render mesh proxy for '%ls' is not going to be rendered (no render mask)"), 
			initInfo.m_component->GetFriendlyName().AsChar() );
	}
#endif

	// Initialize always casting flag
	SetMeshProxyFlag( RMPF_CastingShadowsEvenIfNotVisible, ExtractCastShadowsEvenIfNotVisible( initInfo ) );

	// Morphed meshes are always dynamic.
	m_lightChannels |= LC_DynamicObject;
}

CRenderProxy_MorphedMesh::~CRenderProxy_MorphedMesh()
{
}

/// Chain for update function that should happen only once per frame
const EFrameUpdateState CRenderProxy_MorphedMesh::UpdateOncePerFrame( const CRenderCollector& collector )
{
	// Compute basic stuff
	const auto ret = CRenderProxy_Mesh::UpdateOncePerFrame( collector );
	if ( ret == FUS_AlreadyUpdated )
		return ret;
	
	if ( m_needMorphGenerate )
	{
		collector.m_scene->RequestCollectedTickAsync( this );
	}

	return ret;
}

void CRenderProxy_MorphedMesh::CollectedTick( CRenderSceneEx* scene )
{
	RED_ASSERT( m_mesh, TXT("CRenderProxy_MorphedMesh getting CollectedTick, but doesn't have both meshes") );

	// Try to apply the new morph ratio. If it fails (maybe source or target mesh isn't fully loaded yet), we'll try again next time.
	if ( m_mesh != nullptr && static_cast< CRenderMorphedMesh* >( m_mesh )->ApplyMorphRatio( m_morphRatio ) )
	{
		m_needMorphGenerate = false;
	}
}


Bool CRenderProxy_MorphedMesh::ShouldCollectElement( CRenderCollector* collector, const Bool isShadow, const CRenderElement_MeshChunk* chunk ) const
{
	// HACK : Copied from CRenderProxy_Mesh. For normal meshes, we're checking if mesh->IsFullyLoaded, and not collected when that's
	// false. For morphed meshes, IsFullyLoaded is true once the interpolated buffer has been generated at least once, but we still want
	// to collect in that case, as long as both source and target mesh are ready.

#ifndef RED_FINAL_BUILD
	if ( collector != nullptr )
	{
		if ( !collector->GetRenderFrameInfo().IsShowFlagOn( SHOW_GeometryStatic ) && m_skinningData == nullptr )
		{
			return false;
		}
		if ( !collector->GetRenderFrameInfo().IsShowFlagOn( SHOW_GeometrySkinned ) && m_skinningData != nullptr )
		{
			return false;
		}
		if ( !collector->GetRenderFrameInfo().IsShowFlagOn( SHOW_Meshes ) )
		{
			return false;
		}
	}
#endif

	// If morph source/target mesh aren't ready, we can't collect yet. We aren't checking chunk->GetMesh()->IsFullyLoaded (like is done
	// for CRenderProxy_Mesh), because the morphed mesh doesn't mark as ready until it's been generated at least once. And we need to
	// be collected before that can happen! As long as the source and target are loaded, we'll be able to generate when we get to the
	// collected tick.
	RED_ASSERT( chunk->GetMesh() == m_mesh, TXT("Have a chunk whose mesh is not our morphed mesh!") );
	if ( m_mesh == nullptr || !static_cast< const CRenderMorphedMesh* >( m_mesh )->AreBaseMeshesFullyLoaded() )
	{
		return false;
	}


	if ( isShadow )
	{
		return IsSortGroupCastingShadow( chunk->GetSortGroup() );
	}

	// End of stuff from CRenderProxy_Mesh


	// If we have a morph material, we only created one element for that mesh chunk, so we can draw it.
	if ( chunk->HasFlag( RMCF_MorphBlendMaterial ) )
	{
		return true;
	}

	// When morphRatio == 0, we can shortcut by just drawing the source mesh.
	// When drawing a material override, we don't use UV dissolve, and can just draw elements for the source mesh.
	if ( m_morphRatio == 0.0f || chunk->HasFlag( RMCF_MaterialOverride ) )
	{
		return !chunk->HasFlag( RMCF_MorphTarget );
	}
	// When morphRatio == 1, we can just draw the target mesh.
	else if ( m_morphRatio == 1.0f )
	{
		return chunk->HasFlag( RMCF_MorphTarget );
	}

	// Otherwise, we need to draw all elements, for both meshes.
	return true;
}

void CRenderProxy_MorphedMesh::CollectElements( CRenderCollector& collector )
{
	// Update stuff that should be updated once per frame
	UpdateOncePerFrame( collector );

	CRenderProxy_Mesh::CollectElements( collector );
}

void CRenderProxy_MorphedMesh::CollectCascadeShadowElements( SMergedShadowCascades& cascades, Uint32 perCascadeTestResults )
{
	// Update stuff that should be updated once per frame
	UpdateOncePerFrame( *cascades.m_collector );

	CRenderProxy_Mesh::CollectCascadeShadowElements( cascades, perCascadeTestResults );
}

void CRenderProxy_MorphedMesh::CollectLocalShadowElements( const CRenderCollector& collector, const CRenderCamera& shadowCamera, SShadowCascade& elementCollector )
{
	// Update stuff that should be updated once per frame
	UpdateOncePerFrame( collector );

	CRenderProxy_Mesh::CollectLocalShadowElements( collector, shadowCamera, elementCollector );
}

//! Collect local shadow elements for STATIC shadows ( highest LOD, no dissolve )
void CRenderProxy_MorphedMesh::CollectStaticShadowElements( const CRenderCollector& collector, const CRenderCamera& shadowCamera, SShadowCascade& elementCollector )
{
	// Update stuff that should be updated once per frame
	UpdateOncePerFrame( collector );

	CRenderProxy_Mesh::CollectStaticShadowElements( collector, shadowCamera, elementCollector );
}

//! Collect elements for rendering hires shadows
void CRenderProxy_MorphedMesh::CollectHiResShadowsElements( CRenderCollector& collector, CRenderCollector::HiResShadowsCollector& elementCollector, Bool forceHighestLOD )
{
	// Update stuff that should be updated once per frame
	UpdateOncePerFrame( collector );

	CRenderProxy_Mesh::CollectHiResShadowsElements( collector, elementCollector, forceHighestLOD );
}


void CRenderProxy_MorphedMesh::OnCollectElement( CRenderCollector* collector, Bool isShadow, CRenderElement_MeshChunk* chunk )
{
	Bool useUVDissolve = false;

	Bool isMaterialReplace = chunk->HasFlag( RMCF_MaterialOverride );

	// If ratio is 0 or 1, just push elements from one mesh and no UV dissolve.
	// If we're drawing the material replacement group, we don't need the UV dissolve.
	useUVDissolve = ( m_morphRatio > 0.0f && m_morphRatio < 1.0f && !isMaterialReplace && !chunk->HasFlag( RMCF_MorphBlendMaterial ) );

	chunk->SetMeshChunkFlag( RMCF_UsesUVDissolve, useUVDissolve );

	CRenderProxy_Mesh::OnCollectElement( collector, isShadow, chunk );
}

void CRenderProxy_MorphedMesh::AttachToScene( CRenderSceneEx* scene )
{
	CRenderProxy_Mesh::AttachToScene( scene );

	// Set up for morphing. If we don't have both meshes, we won't be morphing so things are simpler.
	if ( m_mesh != nullptr )
	{
		m_needMorphGenerate = true;
	}
}



CRenderTexture* CRenderProxy_MorphedMesh::GetUVDissolveTexture( const CRenderElement_MeshChunk* chunk ) const
{
	return static_cast< const CRenderMorphedMesh* >( m_mesh )->GetChunkControlTexture( chunk->GetChunkIndex() );
}


Vector CRenderProxy_MorphedMesh::GetUVDissolveValues( const CRenderElement_MeshChunk* chunk ) const
{
	Bool firstMesh = !chunk->HasFlag( RMCF_MorphTarget );
	if ( firstMesh )
	{
		return Vector( 1.f, -m_morphRatio, 0.0f, 0.0f );
	}
	else
	{
		return Vector( -1.f, m_morphRatio, 0.0f, 0.0f );
	}
}

Bool CRenderProxy_MorphedMesh::DoesUVDissolveUseSeparateTexCoord( const CRenderElement_MeshChunk* chunk ) const
{
	// Only target mesh chunks need the extra texcoord. Source mesh (or when there is only one mesh) already have
	// the correct coords since the morphed vertex data original came from there.
	return chunk->HasFlag( RMCF_MorphTarget );
}


void CRenderProxy_MorphedMesh::SetMorphRatio( Float ratio )
{
	// Make sure the ratio is in proper range.
	ratio = Clamp( ratio, 0.0f, 1.0f );

	if ( ratio != m_morphRatio )
	{
		m_morphRatio = ratio;
		m_needMorphGenerate = true;
	}
}
