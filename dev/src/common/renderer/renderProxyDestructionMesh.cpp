/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderProxyDestructionMesh.h"
#include "renderDestructionMesh.h"
#include "../engine/physicsDestructionResource.h"
#include "renderScene.h"

#include "../engine/meshTypeComponent.h"
#include "../engine/material.h"
#include "../core/dataError.h"
#include "../engine/meshComponent.h"


namespace Config
{
	extern TConfigVar< Bool >				cvLoadLodRange;
	extern TConfigVar< Bool >				cvFakeMeshesNotReady;

	extern TConfigVar< Bool >				cvHackyDisableMergedChunks;
	extern TConfigVar< Bool >				cvHackyDisableAdditionalAlphaShadowMeshParts;
	extern TConfigVar< Float >				cvMeshShadowDistanceCutoff0;
	extern TConfigVar< Float >				cvMeshShadowDistanceCutoff1;
}

CRenderProxy_DestructionMesh::CRenderProxy_DestructionMesh( const RenderProxyInitInfo& initInfo )
	: CRenderProxy_Mesh( initInfo, INIT_SUBCLASS )
	, m_needUpdateBoneIndices( false )
{
	RED_ASSERT( 1.f == GetShadowFadeFraction() );

	SetMeshProxyFlagsFromComponent( initInfo.m_component );
	SetDefaultEffectParameters( initInfo.m_component );

	// Initialize from mesh component or destruction system component
	const CMesh* meshResource = Cast< CMesh >( ExtractMeshTypeResource( initInfo ) );
	if ( meshResource != nullptr )
	{

		SetupUmbra( meshResource, initInfo.m_component );
	
		SetupAutohideDistance( meshResource, initInfo );

		Uint8 allowedRenderMask = CalculateAllowedRenderMask( meshResource );

		// Render mesh
		CRenderMesh* originalRenderMesh = static_cast< CRenderMesh* >( meshResource->GetRenderResource() );
		RED_ASSERT( originalRenderMesh != nullptr, TXT("No CRenderMesh") );
		Int32 forcedLOD = ExtractForceLODLevel( initInfo );

		// Setup render mesh
		if ( originalRenderMesh )
		{
			m_mesh = new CRenderDestructionMesh( originalRenderMesh );
			CMeshTypeResourceMaterialProvider materialProvider( meshResource, m_mesh );
			CreateLodGroups( materialProvider, forcedLOD, allowedRenderMask );
		}

		// TODO : Find out if this can just be included with CalculateAllowedRenderMask. In original code,
		// this was done after creating lod groups, so I left it there...
		allowedRenderMask = PatchAllowedRenderMask( allowedRenderMask );
		SetupRenderMask( allowedRenderMask );		
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
}

CRenderProxy_DestructionMesh::~CRenderProxy_DestructionMesh()
{
}

void CRenderProxy_DestructionMesh::UpdateActiveIndices( TDynArray<Uint16>&& indices, TDynArray< Uint32 >&& newOffsets, TDynArray< Uint32 >&& newNumIndices )
{
	m_activeIndices			= Move( indices );
	m_chunkOffsets			= Move( newOffsets );
	m_chunkNumIndices		= Move( newNumIndices );
	m_needUpdateBoneIndices = true;
}

/// Chain for update function that should happen only once per frame
const EFrameUpdateState CRenderProxy_DestructionMesh::UpdateOncePerFrame( const CRenderCollector& collector )
{
	// Compute basic stuff
	const auto ret = CRenderProxy_Mesh::UpdateOncePerFrame( collector );
	if ( ret == FUS_AlreadyUpdated )
		return ret;

	if ( m_needUpdateBoneIndices || !m_mesh->IsFullyLoaded() )
	{
		collector.m_scene->RequestCollectedTickAsync( this );
	}

	return ret;
}

void CRenderProxy_DestructionMesh::CollectedTick( CRenderSceneEx* scene )
{
	RED_ASSERT( m_mesh, TXT("CRenderProxy_DestructionMesh getting CollectedTick, but doesn't have mesh") );

	CRenderDestructionMesh* renderMesh = static_cast< CRenderDestructionMesh* >( m_mesh );

	if( !renderMesh->IsFullyLoaded() )
	{
		renderMesh->FinalizeLoading( );
	}

	// Try to apply new active bone indices
	if ( m_needUpdateBoneIndices && renderMesh != nullptr )
	{
		m_needUpdateBoneIndices = !( renderMesh->UpdateActiveIndices( m_activeIndices, m_chunkOffsets, m_chunkNumIndices ) );
	}
}

void CRenderProxy_DestructionMesh::CollectElements( CRenderCollector& collector )
{
	// Update stuff that should be updated once per frame
	UpdateOncePerFrame( collector );

	CRenderProxy_Mesh::CollectElements( collector );
}

void CRenderProxy_DestructionMesh::CollectCascadeShadowElements( SMergedShadowCascades& cascades, Uint32 perCascadeTestResults )
{
	// Update stuff that should be updated once per frame
	UpdateOncePerFrame( *cascades.m_collector );

	CRenderProxy_Mesh::CollectCascadeShadowElements( cascades, perCascadeTestResults );
}

void CRenderProxy_DestructionMesh::CollectLocalShadowElements( const CRenderCollector& collector, const CRenderCamera& shadowCamera, SShadowCascade& elementCollector )
{
	// Update stuff that should be updated once per frame
	UpdateOncePerFrame( collector );

	CRenderProxy_Mesh::CollectLocalShadowElements( collector, shadowCamera, elementCollector );
}

//! Collect local shadow elements for STATIC shadows ( highest LOD, no dissolve )
void CRenderProxy_DestructionMesh::CollectStaticShadowElements( const CRenderCollector& collector, const CRenderCamera& shadowCamera, SShadowCascade& elementCollector )
{
	// Update stuff that should be updated once per frame
	UpdateOncePerFrame( collector );

	CRenderProxy_Mesh::CollectStaticShadowElements( collector, shadowCamera, elementCollector );
}

//! Collect elements for rendering hires shadows
void CRenderProxy_DestructionMesh::CollectHiResShadowsElements( CRenderCollector& collector, CRenderCollector::HiResShadowsCollector& elementCollector, Bool forceHighestLOD )
{
	// Update stuff that should be updated once per frame
	UpdateOncePerFrame( collector );

	CRenderProxy_Mesh::CollectHiResShadowsElements( collector, elementCollector, forceHighestLOD );
}