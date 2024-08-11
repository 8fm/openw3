/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderProxyMesh.h"
#include "renderSkinningData.h"
#include "renderTerrainShadows.h"
#include "renderShadowManager.h"
#include "renderElementMap.h"
#include "renderElementMeshChunk.h"
#include "renderScene.h"
#include "renderHelpers.h"
#include "renderMaterial.h"
#include "renderMesh.h"
#include "renderTexture.h"
#include "renderOcclusion.h"
#include "renderVisibilityQueryManager.h"
#include "renderDynamicDecalChunk_Mesh.h"
#include "renderDecalMesh.h"
#include "../engine/mesh.h"
#include "../core/dataError.h"
#include "../core/configVar.h"
#include "../engine/materialInstance.h"
#include "../engine/meshComponent.h"
#include "../engine/mergedMeshComponent.h"
#include "../engine/normalBlendComponent.h"

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
#endif // USE_ANSEL

// Comment me out to enable checking all chunks for consistent volume chunks. Checking a single chunk will
// have less chance for any performance hit, but won't catch mixed volume/non-volume chunks.
//#define CHECK_SINGLE_CHUNK_FOR_VOLUME_MESH

// Comment me out to use a regular assert instead of Fatal. There may be some meshes that have mixed
// volume/non-volume chunks, and the fatal assert could help catch those, but might make it difficult to
// otherwise test stuff.
//#define USE_FATAL_ASSERT_FOR_INCONSISTENT_VOLUME_CHUNKS


#if defined(USE_FATAL_ASSERT_FOR_INCONSISTENT_VOLUME_CHUNKS) && defined(CHECK_SINGLE_CHUNK_FOR_VOLUME_MESH)
# error USE_FATAL_ASSERT and CHECK_SINGLE_CHUNK don't make much sense. Won't get the fatal assert anywhere.
#endif


namespace Config
{
	TConfigVar< Bool > cvLoadLodRange						( "Rendering", "PrefetchMeshLodRange",				false );
	TConfigVar< Bool > cvFakeMeshesNotReady					( "Rendering", "FakeMeshesNotReady",				false );
}

static const Uint32 MAX_NUM_CHUNKS = 256;

namespace Config
{
	TConfigVar< Bool >				cvHackyDisableMergedChunks( "Rendering/Hacks", "DisableMergedChunks", false );
	TConfigVar< Bool >				cvHackyDisableAdditionalAlphaShadowMeshParts( "Rendering/Hacks", "DisableAdditionalAlphaShadowMeshParts", false );

	TConfigVar< Float >				cvMeshShadowDistanceCutoff0( "Rendering/Shadows", "MeshDistanceCutoffNear", 0.15f ); // objects below that size are not rendered into any shadowmaps
	TConfigVar< Float >				cvMeshShadowDistanceCutoff1( "Rendering/Shadows", "MeshDistanceCutoffFar", 0.4f ); // objects below that size are only rendered in cascade 1
}

const CMeshTypeResource* CRenderProxy_Mesh::ExtractMeshTypeResource( const RenderProxyInitInfo& initInfo )
{
	if ( const CMeshTypeComponent* mtc = Cast< CMeshTypeComponent >( initInfo.m_component ) )
	{
		return mtc->GetMeshTypeResource();
	}
	else if ( initInfo.m_packedData )
	{
		const RenderProxyMeshInitData* meshData = static_cast< const RenderProxyMeshInitData* >( initInfo.m_packedData );
		return meshData->m_mesh;
	}

	return nullptr;
}

Int32 CRenderProxy_Mesh::ExtractForceLODLevel( const RenderProxyInitInfo& initInfo )
{
	if ( initInfo.m_component && initInfo.m_component->IsA< CMeshComponent >() )
	{
		const CMeshComponent* mc = static_cast< const CMeshComponent* >( initInfo.m_component );
		return mc->GetForcedLODLevel();
	}
	else if ( initInfo.m_packedData )
	{
		const RenderProxyMeshInitData* meshData = static_cast< const RenderProxyMeshInitData* >( initInfo.m_packedData );
		return meshData->m_forcedLODLevel;
	}


	return -1;
}

Bool CRenderProxy_Mesh::ExtractForceNoAutohide( const RenderProxyInitInfo& initInfo )
{
#ifndef NO_EDITOR
	// Editor only global flag
	if( CDrawableComponent::IsForceNoAutohideDebug() ) return true;
#endif

	if ( initInfo.m_component && initInfo.m_component->IsA< CDrawableComponent >() )
	{
		const CDrawableComponent* dc = static_cast< const CDrawableComponent* >( initInfo.m_component );
		return dc->IsForceNoAutohide();
	}
	else if ( initInfo.m_packedData )
	{
		const RenderProxyMeshInitData* meshData = static_cast< const RenderProxyMeshInitData* >( initInfo.m_packedData );
		return meshData->m_noAutoHide;
	}

	return false;
}

Float CRenderProxy_Mesh::ExtractAutohideDistance( const RenderProxyInitInfo& initInfo )
{
	if ( initInfo.m_component && initInfo.m_component->IsA< CDrawableComponent >() )
	{
		const CMeshComponent* mc = static_cast< const CMeshComponent* >( initInfo.m_component );
		return mc->GetAutoHideDistance();
	}
	else if ( initInfo.m_packedData )
	{
		const RenderProxyMeshInitData* meshData = static_cast< const RenderProxyMeshInitData* >( initInfo.m_packedData );
		return meshData->m_autoHideDistance;
	}

	return false;
}

Bool CRenderProxy_Mesh::ExtractCastShadowsEvenIfNotVisible( const RenderProxyInitInfo& initInfo )
{
	if ( initInfo.m_component && initInfo.m_component->IsA< CDrawableComponent >() )
	{
		const CDrawableComponent* dc = static_cast< const CDrawableComponent* >( initInfo.m_component );
		return dc->IsCastingShadowsEvenIfNotVisible();
	}
	else if ( initInfo.m_packedData )
	{
		const RenderProxyMeshInitData* meshData = static_cast< const RenderProxyMeshInitData* >( initInfo.m_packedData );
		return meshData->m_castingShadowsAlways;
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////

IMaterial* GRenderProxyGlobalMaterial = nullptr;

CMeshTypeResourceMaterialProvider::CMeshTypeResourceMaterialProvider( const CMeshTypeResource* mesh, const CRenderMesh* renderMesh )
	: m_mesh( mesh )
	, m_renderMesh( renderMesh )
{
	RED_FATAL_ASSERT( m_mesh != nullptr && m_renderMesh != nullptr, "CMesh and CRenderMesh must be non-null! Caller should be dealing with this" );
}


Bool CMeshTypeResourceMaterialProvider::GetMaterialForChunk( Uint8 chunkIndex, CRenderMaterial*& outMaterial, CRenderMaterialParameters*& outMaterialParams ) const
{
	if ( chunkIndex >= m_renderMesh->GetNumChunks() )
	{
		return false;
	}

	IMaterial* material = GRenderProxyGlobalMaterial;
	if ( material == nullptr )
	{
		const Uint32 materialId = m_renderMesh->GetChunks()[ chunkIndex ].m_materialId;

		if ( materialId < m_mesh->GetMaterials().Size() )
		{
			material = m_mesh->GetMaterials()[ materialId ];
		}
	}

	if ( material == nullptr )
	{
		return false;
	}

	if ( material->GetMaterialDefinition() == nullptr )
	{
		DATA_HALT( DES_Major, m_mesh, TXT("Material"),
			TXT("Can't find material '%ls' definition on mesh '%ls'"), material->GetFriendlyName().AsChar(), m_mesh->GetFriendlyName().AsChar() ); 
		return false;
	}

	ExtractRenderResource( material, outMaterialParams );
	ExtractRenderResource( material->GetMaterialDefinition(), outMaterial );
	return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////

CRenderProxy_Mesh::CRenderProxy_Mesh( const RenderProxyInitInfo& initInfo, INIT_SUBCLASS_TAG )
	: IRenderProxyDissolvable( RPT_Mesh, initInfo )
	, IRenderEntityGroupProxy( initInfo.m_entityGroup )
	, m_meshProxyFlags( 0 )
	, m_skinningData( nullptr )
	, m_replacedMaterialLodGroup( nullptr )
	, m_normalBlendLodGroup( nullptr )
	, m_normalBlendAreas( nullptr )
	, m_normalBlendWeights( nullptr )
	, m_renderMask( MCR_Scene | MCR_LocalShadows | MCR_Cascade1 | MCR_Cascade2 )
	, m_localVisId( 0 )
	, m_renderGroup( nullptr )
	, m_mesh( nullptr )
	, m_skipOcclusionTest( false )
{
	// Do nothing. We assume that the subclass knows what it's doing and will initialize everything :)


	// Lame check for volume chunks.
	if ( const CMeshTypeResource* mtr = ExtractMeshTypeResource( initInfo ) )
	{
		Bool hasNonVolumeMaterial = false;
		Bool hasVolumeMaterial = false;
		const CMeshTypeResource::TMaterials& mtls = mtr->GetMaterials();
		for ( const THandle<IMaterial>& mtl : mtls )
		{
			if ( mtl != nullptr )
			{
				if ( IMaterialDefinition* defn = mtl->GetMaterialDefinition() )
				{
					hasNonVolumeMaterial = !defn->IsUsedByVolumeRendering();
					hasVolumeMaterial = defn->IsUsedByVolumeRendering();

#ifndef CHECK_SINGLE_CHUNK_FOR_VOLUME_MESH
					SetMeshProxyFlag(RMPF_IsVolumeMesh, hasVolumeMaterial);
					break;
#endif
				}
			}
		}

#ifdef USE_FATAL_ASSERT_FOR_INCONSISTENT_VOLUME_CHUNKS
		RED_FATAL_ASSERT( !hasNonVolumeMaterial || !hasVolumeMaterial, "Inconsistent Volume chunks in %ls", mtr->GetDepotPath().AsChar() );
#else
		RED_ASSERT( !hasNonVolumeMaterial || !hasVolumeMaterial, TXT("Inconsistent Volume chunks in %ls"), mtr->GetDepotPath().AsChar() )
#endif
	}
}

CRenderProxy_Mesh::CRenderProxy_Mesh( const RenderProxyInitInfo& initInfo )
	: IRenderProxyDissolvable( RPT_Mesh, initInfo )
	, IRenderEntityGroupProxy( initInfo.m_entityGroup )
	, m_meshProxyFlags( 0 )
	, m_skinningData( nullptr )
	, m_replacedMaterialLodGroup( nullptr )
	, m_normalBlendLodGroup( nullptr )
	, m_normalBlendAreas( nullptr )
	, m_normalBlendWeights( nullptr )
	, m_renderMask( MCR_Scene | MCR_LocalShadows | MCR_Cascade1 | MCR_Cascade2 )
	, m_localVisId( 0 )
	, m_renderGroup( nullptr )
	, m_mesh( nullptr )
	, m_skipOcclusionTest( false )
{
	RED_ASSERT( 1.f == GetShadowFadeFraction() );


	//
	// NOTE : If anything here changes, check places where the INIT_SUBCLASS_TAG constructor is used (Find In Files for "INIT_SUBCLASS").
	// Those places probably have some variation of this duplicated, and may need to be changed as well.
	//


	SetMeshProxyFlagsFromComponent( initInfo.m_component );
	SetDefaultEffectParameters( initInfo.m_component );


	const CMesh* meshResource = Cast< CMesh >( ExtractMeshTypeResource( initInfo ) );
	if ( meshResource != nullptr )
	{
		SetupUmbra( meshResource, initInfo.m_component );

		W3HACK_ApplyUmbraHacks( meshResource );

		if ( meshResource->IsEntityProxy() )
		{
			SetMeshProxyFlag( RMPF_IsEntityProxy );

#if HACKY_ENTITY_PROXY_SHADOWS_FIX
			// If it can go into merged shadows, then it should cast shadows...
			if ( meshResource->CanMergeIntoGlobalShadowMesh() )
			{
				m_drawableFlags |= RPF_CastingShadows;
			}
#endif
		}


		SetupAutohideDistance( meshResource, initInfo );
		W3HACK_ApplyAutohideDistanceHack( meshResource );


		Uint8 allowedRenderMask = CalculateAllowedRenderMask( meshResource );

		CRenderMesh* renderMesh = static_cast< CRenderMesh* >( meshResource->GetRenderResource() );

		// NOTE : At least the proxy flag needs to be set before setting up render elements in InitializeRenderMesh.
		if ( renderMesh != nullptr && initInfo.m_usesVertexCollapse )
		{
			renderMesh->InitBonePositionTexture( meshResource );
			SetMeshProxyFlag( RMPF_UsesVertexCollapse );
		}


		const Int32 forcedLOD = ExtractForceLODLevel( initInfo );
		InitializeRenderMesh( renderMesh, meshResource, forcedLOD, allowedRenderMask );

		// TODO : Find out if this can just be included with CalculateAllowedRenderMask. In original code,
		// this was done after creating lod groups, so I left it there...
		allowedRenderMask = PatchAllowedRenderMask( allowedRenderMask );
		SetupRenderMask( allowedRenderMask );

#if HACKY_ENTITY_PROXY_SHADOWS_FIX
		// If it can go into merged shadows, then it should cast shadows...
		if ( CheckMeshProxyFlag( RMPF_IsEntityProxy ) && meshResource->CanMergeIntoGlobalShadowMesh() )
		{
			if ( ( m_renderMask & MCR_Cascade1 ) == 0 )
			{
				m_renderMask |= MCR_Cascade1;
				SetMeshProxyFlag( RMPF_HACK_EntityProxyNeedsShadows );
			}
		}
#endif
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

	if ( initInfo.m_packedData )
	{
		const RenderProxyMeshInitData* meshData = static_cast< const RenderProxyMeshInitData* >( initInfo.m_packedData );

		// Custom visibility reference point
		if ( meshData->m_useCustomReferencePoint )
		{
			SetCustomReferencePoint( meshData->m_customReferencePoint );
		}

		if ( meshData->m_forceNoUmbraCulling )
		{
			SetMeshProxyFlag( RMPF_ForceDynamic );
			m_umbraProxyId = GlobalVisID();
		}
	}

	W3HACK_ApplyProxyMeshUmbraHack();
	W3HACK_ApplyVolumeMeshFlag();
}


void CRenderProxy_Mesh::SetMeshProxyFlagsFromComponent( const CComponent* component )
{
	if ( component == nullptr )
	{
		return;
	}

	if ( component->IsA< CMergedMeshComponent >() )
	{
		SetMeshProxyFlag( RMPF_MergedMesh );
	}

	if ( const CDrawableComponent* dc = Cast< CDrawableComponent >( component ) )
	{
		SetMeshProxyFlag( RMPF_ForceHighestLOD, dc->IsForcedHighestLOD() );
	}

	if ( const CMeshTypeComponent* mtc = Cast< CMeshTypeComponent >( component ) )
	{
		SetMeshProxyFlag( RMPF_IsCharacterShadowFallback, mtc->IsCharacterShadowFallback() );
	}
}

void CRenderProxy_Mesh::SetDefaultEffectParameters( const CComponent* component )
{
	if ( const CMeshTypeComponent* mtc = Cast< CMeshTypeComponent >( component ) )
	{
		Vector effectParam0 = mtc->GetDefaultEffectParams();
		Vector effectParam1 = mtc->GetDefaultEffectColor().ToVector();

		if ( effectParam1 != Vector::ZEROS || effectParam0 != Vector::ZEROS )
		{
			m_effectParams = new SRenderProxyDrawableEffectParams();
			m_effectParams->m_customParam0 = effectParam0;
			m_effectParams->m_customParam1 = effectParam1;
		}
	}
}


void CRenderProxy_Mesh::SetupUmbra( const CMeshTypeResource* meshTypeResource, const CComponent* component )
{
#ifdef USE_UMBRA
	if ( component )
	{
		m_umbraProxyId = GlobalVisID( meshTypeResource, component );
	}
	else
	{
		m_umbraProxyId = GlobalVisID( meshTypeResource, GetLocalToWorld() );
	}

	if ( component && component->GetTransformParent() )
	{
		SetMeshProxyFlag( RMPF_ForceDynamic );
	}
#endif
}


void CRenderProxy_Mesh::SetupAutohideDistance( const CMeshTypeResource* meshTypeResource, const RenderProxyInitInfo& initInfo )
{
	if ( ExtractForceNoAutohide( initInfo ) )
	{
		m_autoHideDistance = 2000.0f; 
	}
	else
	{
		m_autoHideDistance = ExtractAutohideDistance( initInfo );
	}
	m_autoHideDistanceSquared = m_autoHideDistance * m_autoHideDistance;
}



void CRenderProxy_Mesh::InitializeRenderMesh( CRenderMesh* renderMesh, const CMeshTypeResource* meshTypeResource, Int32 forcedLOD, Uint8 allowedRenderMask )
{
	m_mesh = renderMesh;
	if ( m_mesh != nullptr )
	{
		m_mesh->AddRef();

		CMeshTypeResourceMaterialProvider materialProvider( meshTypeResource, m_mesh );
		CreateLodGroups( materialProvider, forcedLOD, allowedRenderMask );
	}
}

void CRenderProxy_Mesh::CreateLodGroups( const IRenderMeshChunkMaterialProvider& materialProvider, Int32 forcedLOD, Uint8 renderMask )
{
	RED_FATAL_ASSERT( m_mesh != nullptr, "Caller should not call this is there's no render mesh!" );
	RED_FATAL_ASSERT( m_lodGroups.Empty(), "Already have LOD groups populated!" );

	const Uint32 numLODs = m_mesh->GetNumLODs();

	// LOD is forced on this mesh, create chunks for this LOD only
	if ( forcedLOD >= 0 && forcedLOD < (Int32)numLODs )
	{
		m_lodGroups.Reserve( 1 );
		new ( m_lodGroups ) MeshLodGroup( this, m_mesh, materialProvider, forcedLOD, 0, renderMask );

		GetLodSelector().SetupSingle( m_autoHideDistanceSquared );
	}
	else
	{
		m_lodGroups.Reserve( numLODs );
		for ( Uint32 i = 0; i < numLODs; ++i )
		{
			new ( m_lodGroups ) MeshLodGroup( this, m_mesh, materialProvider, i, i, renderMask );
		}

		GetLodSelector().SetupFromMesh( m_mesh, m_autoHideDistance, m_autoHideDistanceSquared );
	}
}


void CRenderProxy_Mesh::SetupRenderMask( Uint8 allowedRenderMask )
{
	m_renderMask = 0;
	Bool hasAdditionalShadowElements = false;
	for ( const auto& lod : m_lodGroups )
	{
		for ( Uint32 i=0, end = lod.GetChunkCount(); i<end; ++i )
		{
			const auto* chunk = lod.GetChunk( i );

			// Get the merged visibility mask
			m_renderMask |= chunk->GetBaseRenderMask() | chunk->GetMergedRenderMask(); // any allowed configuration counts

			// Check if we need additional chunks to be drawn in the shadow cascades (masked chunks)
			if ( chunk->HasFlag( RMCF_AdditionalShadowElement ) )
			{
				hasAdditionalShadowElements = true;
			}
		}
	}

	// use the more expensive shadow drawing on proxies that require it
	if ( hasAdditionalShadowElements )
	{
		SetMeshProxyFlag( RMPF_HasAdditionalShadowElements );
	}

	// Final mask out - remove cascades if they were filtered out by the performance profile
	m_renderMask &= allowedRenderMask;

	// hack!!
	if ( CheckMeshProxyFlag( RMPF_MergedMesh ) )
	{
		m_renderMask = MCR_Cascade2 | MCR_Cascade3 | MCR_Cascade4;
	}
}


Uint8 CRenderProxy_Mesh::CalculateAllowedRenderMask( const CMeshTypeResource* meshTypeResource ) const
{
	Uint8 allowedRenderMask = MCR_Scene | MCR_LocalShadows | MCR_Cascade1 | MCR_Cascade2 | MCR_Cascade3;
	if ( !CheckMeshProxyFlag( RMPF_MergedMesh ) )
	{
		if ( const CMesh* mesh = Cast< CMesh >( meshTypeResource ) )
		{
			const Float generalizedRadius = mesh->GetGeneralizedMeshRadius();
			if ( generalizedRadius >= Config::cvMeshShadowDistanceCutoff0.Get() )
			{
				allowedRenderMask |= MCR_Cascade1;
				if ( generalizedRadius >= Config::cvMeshShadowDistanceCutoff1.Get() )
				{
					allowedRenderMask |= MCR_Cascade2;
				}
			}
		}
	}

	return allowedRenderMask;
}


Uint8 CRenderProxy_Mesh::PatchAllowedRenderMask( Uint8 allowedRenderMask ) const
{
	// Extra masking out for shadows
	if ( !CanCastShadows() )
	{
		// all shadows are disabled
		allowedRenderMask &= ~( MCR_Cascade1 | MCR_Cascade2 | MCR_Cascade3 | MCR_Cascade4 | MCR_LocalShadows );
	}
	else if ( IsCastingShadowsFromLocalLightsOnly() )
	{
		// global shadows are disabled
		allowedRenderMask &= ~( MCR_Cascade1 | MCR_Cascade2 | MCR_Cascade3 | MCR_Cascade4 );
	}

	return allowedRenderMask;
}


//*********************************************************************************************//
// HACKY HACKY HACKY !!!!!!
void CRenderProxy_Mesh::W3HACK_ApplyAutohideDistanceHack( const CMeshTypeResource* meshResource )
{
	const String path = meshResource->GetDepotPath();
	const Uint32 pathHash = Red::System::CalculateAnsiHash32( path.AsChar() );
	const Uint32 meshesNameHashes[] = 
	{
		545725439U ,			// environment\architecture\human\redania\novigrad\passiflora_brothel\passiflora_brothel_new\passiflora_building_ext_wall_fa.w2mesh
		4128311076U ,			// environment\\architecture\\human\\redania\\novigrad\\rich\\ochman_house\\ochman_house_e_exterior_new.w2mesh
		3666457333U,			// dlc\bob\data\environment\terrain_surroundings\background\bcg_a.w2mesh
		2612767942U,			// dlc\bob\data\environment\terrain_surroundings\background\bcg_b.w2mesh
		157917146U,				// dlc\bob\data\environment\terrain_surroundings\background\bob_background_fairytail_merged.w2mesh
		340362028U,				// dlc\bob\data\environment\terrain_surroundings\background\background_b.w2mesh
		0,
	};
	const Uint32* meshesNameHashesPtr = meshesNameHashes; 
	while( *meshesNameHashesPtr )
	{
		if( pathHash == *meshesNameHashesPtr )
		{
			m_autoHideDistance += 10.0f; // LOL
			m_autoHideDistanceSquared = m_autoHideDistance * m_autoHideDistance;

			SetMeshProxyFlag( RMPF_ForceDynamic );
			m_umbraProxyId = GlobalVisID();

			break;
		}
		++meshesNameHashesPtr;
	}

	// if( path == L"" )
	// {
	// 	RED_LOG(RED_LOG_CHANNEL(HACKTEST),TXT("hash=%u"),pathHash);
	// }

}

void CRenderProxy_Mesh::W3HACK_ApplyProxyMeshUmbraHack()
{
	// If it's a proxy mesh, force dynamic and clear umbra ID. This way it won't be falsely occluded in distant
	// areas where there is no occlusion data loaded.
	if ( m_lodGroups.Size() >= 2 && m_lodGroups[0].GetChunkCount() == 0 )
	{
		SetMeshProxyFlag( RMPF_ForceDynamic );
#ifdef USE_ANSEL
		// UBERHACK for Blood and Wine castle shadowmesh
		if ( m_umbraProxyId.GetKey() != 11465474488251405214 )
		{
			m_umbraProxyId = GlobalVisID();
		}
#endif // USE_ANSEL
	}
}

void CRenderProxy_Mesh::W3HACK_ApplyVolumeMeshFlag()
{
	if ( m_mesh == nullptr )
	{
		return;
	}

	// This would maybe be better to store in the mesh itself, but instead we'll just check if we have a chunk
	// with a volume material.
	Bool isFirstChunk = true;

	for ( const MeshLodGroup& lodGroup : m_lodGroups )
	{
		const Uint32 numChunks = lodGroup.GetChunkCount();
		for ( Uint32 i = 0; i < numChunks; ++i )
		{
			const Bool isVolume = lodGroup.GetChunk( i )->GetSortGroup() == RSG_Volumes;

			if ( !isFirstChunk )
			{
#ifdef USE_FATAL_ASSERT_FOR_INCONSISTENT_VOLUME_CHUNKS
				RED_FATAL_ASSERT( isVolume == CheckMeshProxyFlag( RMPF_IsVolumeMesh ), "Inconsistent Volume chunks in %ls", m_mesh->GetMeshDepotPath().AsChar() );
#elif !RED_FINAL_BUILD
				RED_ASSERT( isVolume == CheckMeshProxyFlag( RMPF_IsVolumeMesh ), TXT("Inconsistent Volume chunks in %ls"), m_mesh->GetMeshDepotPath().AsChar() )
#endif
			}
			isFirstChunk = false;

			if ( isVolume )
			{
				SetMeshProxyFlag( RMPF_IsVolumeMesh );
			}

#ifdef CHECK_SINGLE_CHUNK_FOR_VOLUME_MESH
			// Stop once we've found the first chunk.
			return;
#endif
		}
	}
}

void CRenderProxy_Mesh::W3HACK_ApplyUmbraHacks( const CMeshTypeResource* meshTypeResource )
{
	const String& depotPath = meshTypeResource->GetDepotPath();
	const Uint32 pathHash = Red::System::CalculateAnsiHash32( depotPath.AsChar() );
	const Uint32 meshesNameHashes[] = 
	{
		2754086624U,	// dlc\bob\data\quests\main_quests\quest_files\q702_hunt\entities\meshes\q702_poor_wardrobe_door_a.w2mesh
		1042796587U,	// dlc\bob\data\quests\main_quests\quest_files\q702_hunt\entities\meshes\q702_poor_wardrobe_door_b.w2mesh
		
		// TTP#154004:
		2487623647U,	// dlc\bob\data\environment\entity_proxy\bob_farm_01_proxy.w2mesh
		424409948U,		// dlc\bob\data\environment\entity_proxy\bob_farm_02_proxy.w2mesh
		1644535945U,	// dlc\bob\data\environment\entity_proxy\bob_castle_ravello_walls_03_proxy.w2mesh
		34937312U,		// dlc\bob\data\environment\entity_proxy\bob_castle_ravello_walls_02_proxy.w2mesh
		0,
	};
	const Uint32* meshesNameHashesPtr = meshesNameHashes; 
	while ( *meshesNameHashesPtr )
	{
		if ( pathHash == *meshesNameHashesPtr )
		{
			m_skipOcclusionTest = true;
			break;
		}
		++meshesNameHashesPtr;
	}
}

// HACKY HACKY HACKY !!!!!!
//*********************************************************************************************//



CRenderProxy_Mesh::~CRenderProxy_Mesh()
{
	if ( m_replacedMaterialLodGroup )
	{
		delete m_replacedMaterialLodGroup;
		m_replacedMaterialLodGroup = nullptr;
	}

	if (m_normalBlendLodGroup)
	{
		delete m_normalBlendLodGroup;
		m_normalBlendLodGroup = nullptr;
	}

	RED_MEMORY_FREE( MemoryPool_Default, MC_RenderNormalBlendingData, m_normalBlendAreas );
	m_normalBlendAreas = nullptr;

	RED_MEMORY_FREE( MemoryPool_Default, MC_RenderNormalBlendingData, m_normalBlendWeights );
	m_normalBlendWeights = nullptr;

	SAFE_RELEASE( m_skinningData );
	SAFE_RELEASE( m_mesh );
}

CRenderProxy_Mesh::MeshLodGroup::MeshLodGroup()
	: m_chunks( nullptr )	
	, m_numChunks( 0 )
{
}


CRenderProxy_Mesh::MeshLodGroup::MeshLodGroup( CRenderProxy_Mesh* proxy, CRenderMesh* mesh, const IRenderMeshChunkMaterialProvider& materialProvider, Int32 sourceLodGroupIndex, Int32 lodIndex, const Uint8 renderFilterMask /*= 0xFF*/ )
	: m_chunks( nullptr )
	, m_numChunks( 0 )
{
	Append( proxy, mesh, materialProvider, sourceLodGroupIndex, lodIndex, renderFilterMask );
}


CRenderProxy_Mesh::MeshLodGroup::~MeshLodGroup()
{
	// Release chunk elements
	for ( Uint8 i=0; i<m_numChunks; i++ )
	{
		SAFE_RELEASE( m_chunks[i] );
	}

	RED_MEMORY_FREE_HYBRID( MemoryPool_Default, MC_RenderMeshChunks , m_chunks );

	m_chunks = nullptr;
}


void CRenderProxy_Mesh::MeshLodGroup::Append( CRenderProxy_Mesh* proxy, CRenderMesh* renderMesh, const IRenderMeshChunkMaterialProvider& materialProvider, Int32 sourceLodGroupIndex, Int32 lodIndex, const Uint8 renderFilterMask /*= 0xFF*/ )
{
	// TEMP CHUNK LIST
	CRenderElement_MeshChunk* tempChunkList[ MAX_NUM_CHUNKS ];
	Uint32 numChunksInTempList = 0;

	if ( renderMesh )
	{
		// Create rendering elements for each chunk defined at given LOD
		const Uint32 numChunks = renderMesh->GetNumChunks();
		for ( Uint32 i=0; i<numChunks; i++ )
		{
			const CRenderMesh::Chunk& chunk = renderMesh->GetChunks()[ i ];

			// Is this a chunk for this LOD ?
			if ( (chunk.m_lodMask & ( 1 << sourceLodGroupIndex )) == 0 )
				continue;

			// Can we actually render it
			if ( (chunk.m_baseRenderMask & renderFilterMask) == 0 && 
				(chunk.m_mergedRenderMask & renderFilterMask) == 0 )
				continue;

			CRenderMaterial* material = nullptr;
			CRenderMaterialParameters* materialParameters = nullptr;

			if ( !materialProvider.GetMaterialForChunk( i, material, materialParameters ) )
			{
				continue;
			}
			if ( material == nullptr || materialParameters == nullptr )
			{
				SAFE_RELEASE( material );
				SAFE_RELEASE( materialParameters );
				ERR_RENDERER( TXT("Material provider returned null material") );
				continue;
			}

			// Create new element
			if ( numChunksInTempList < MAX_NUM_CHUNKS )
			{
				CRenderElement_MeshChunk* renderChunk = new CRenderElement_MeshChunk( proxy, renderMesh, i, material, materialParameters, lodIndex );
				if ( renderChunk && renderChunk->GetMaterial() && renderChunk->GetMaterialParams() )
				{
					// Add to chunk list
					tempChunkList[ numChunksInTempList ] = renderChunk;
					numChunksInTempList += 1;
				}
				else
				{
					RED_FATAL_ASSERT( false, "There is something unexpectedly wrong with this chunk!" );
				}
			}

			SAFE_RELEASE( material );
			SAFE_RELEASE( materialParameters );
		}

		// Add collected chunks
		if ( numChunksInTempList > 0 )
		{
			Append( tempChunkList, static_cast< Uint8 >( numChunksInTempList ) );
		}
	}
}


void CRenderProxy_Mesh::MeshLodGroup::Append( CRenderElement_MeshChunk* const* newChunks, Uint8 numChunks )
{
	RED_ASSERT( m_numChunks + numChunks < MAX_NUM_CHUNKS, TXT("Too many mesh chunks: %d"), m_numChunks + numChunks );

#ifndef RED_FINAL_BUILD
	for ( Uint8 i = 0; i < numChunks; ++i )
	{
		if ( newChunks[i] == nullptr )
		{
			RED_HALT( "Trying to append a null mesh chunk! Aborting!" );
			return;
		}
		if ( newChunks[i]->GetMaterial() == nullptr || newChunks[i]->GetMaterialParams() == nullptr )
		{
			RED_HALT( "Trying to append a mesh chunk with no material! Aborting!" );
			return;
		}
	}
#endif

	Uint32 oldNumChunks = m_numChunks;

	Reallocate( m_numChunks + numChunks );

	// Copy new chunks to end of array.
	Red::System::MemoryCopy( m_chunks + oldNumChunks, newChunks, sizeof( CRenderElement_MeshChunk* ) * numChunks );
}


CRenderProxy_Mesh::MeshLodGroup* CRenderProxy_Mesh::MeshLodGroup::CreateCopyWithMaterialOverride( CRenderMaterial* material, CRenderMaterialParameters* parameters ) const
{
	MeshLodGroup* newGroup = new MeshLodGroup();
	
	if ( m_numChunks )
	{
		newGroup->Reallocate( m_numChunks );
		// If we have a material, then make copies of the chunks with the new material.
		if ( material && parameters )
		{
			// Copy original chunks and substitute material
			for ( Uint32 i = 0; i < newGroup->m_numChunks; ++i )
			{
				if ( m_chunks[i] )
				{
					if ( material->CanReplace( m_chunks[i]->GetMaterial() ) )
					{
						newGroup->m_chunks[i] = m_chunks[i]->CreateCopyWithMaterialOverride( material, parameters );
					}
					else
					{
						newGroup->m_chunks[i] = new CRenderElement_MeshChunk( *m_chunks[i] );
					}
				}
			}
		}
	}

	return newGroup;
}

RED_INLINE CRenderElement_MeshChunk* CRenderProxy_Mesh::MeshLodGroup::GetChunk( Uint32 index ) const
{
	RED_FATAL_ASSERT( index < m_numChunks, "Out Of Bound chunk acess." );
	return m_chunks[ index ];
}

RED_INLINE CRenderElement_MeshChunk**  CRenderProxy_Mesh::MeshLodGroup::GetChunks() const
{
	return m_chunks;
}

RED_INLINE void CRenderProxy_Mesh::MeshLodGroup::ReleaseChunk( Uint32 index )
{
	RED_FATAL_ASSERT( index < m_numChunks, "Out Of Bound chunk acess." );
	SAFE_RELEASE( m_chunks[ index ] );
}

RED_INLINE void CRenderProxy_Mesh::MeshLodGroup::SetChunk( Uint32 index,  CRenderElement_MeshChunk* chunk )
{
	ReleaseChunk( index );
	m_chunks[ index ] = chunk;
}

void CRenderProxy_Mesh::MeshLodGroup::Reallocate( Uint32 count )
{
	RED_FATAL_ASSERT( count >= m_numChunks, "MeshLodGroup cannot shrink." );

	if( count != m_numChunks  )
	{
		void * newMemory = nullptr;
		Uint32 newMemorySize = count * sizeof( CRenderElement_MeshChunk* );
		Uint32 oldMemorySize = m_numChunks * sizeof( CRenderElement_MeshChunk* );
		
		newMemory = RED_MEMORY_ALLOCATE_ALIGNED_HYBRID( MemoryPool_Default, MC_RenderMeshChunks, newMemorySize, __alignof( CRenderElement_MeshChunk* ) );
		
		Red::System::MemoryCopy( newMemory, m_chunks, oldMemorySize );

		RED_MEMORY_FREE_HYBRID( MemoryPool_Default, MC_RenderMeshChunks, m_chunks );
		
		m_chunks = static_cast< CRenderElement_MeshChunk** >( newMemory );
		m_numChunks = count;
	}

}

void CRenderProxy_Mesh::OnCollectElement( CRenderCollector* collector, Bool isShadow, CRenderElement_MeshChunk* chunk )
{
	// WHAT A BULLSHIT!
}

Bool CRenderProxy_Mesh::ShouldCollectElement( CRenderCollector* collector, Bool isShadow, const CRenderElement_MeshChunk* chunk ) const
{
	// NOTE : If this changes, check if you need to make similar changes to CRenderProxy_MorphedMesh!

	// debug shit
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
		if ( !collector->GetRenderFrameInfo().IsShowFlagOn( SHOW_MergedMeshes ) && CheckMeshProxyFlag( RMPF_MergedMesh ) )
		{
			return false;
		}
		if ( !collector->GetRenderFrameInfo().IsShowFlagOn( SHOW_GeometryProxies ) && CheckMeshProxyFlag( RMPF_IsEntityProxy ) )
		{
			return false;
		}
	}
#endif

	// Don't collect if the mesh isn't ready.
	if ( !chunk->GetMesh()->IsFullyLoaded() )
	{
		return false;
	}

	if ( isShadow )
	{
		return IsSortGroupCastingShadow( chunk->GetSortGroup() );
	}

	return true;
}

void CRenderProxy_Mesh::OnNotVisibleFromAutoHide( CRenderCollector& collector )
{
	// proxy not visible
	if ( !IsVisible() )
		return;

	// merged mesh
	if ( CheckMeshProxyFlag( RMPF_MergedMesh ) )
		return;

	// Update stuff that should be updated once per frame
	UpdateOncePerFrame( collector );
}


void CRenderProxy_Mesh::CollectElements( CRenderCollector& collector )
{
	RED_FATAL_ASSERT( m_scene != nullptr, "Trying to collect proxy that is not in a scene - hacky hacky rendering" );

	// proxy not visible
	if ( !IsVisible() )
		return;

	// merged mesh
	if ( CheckMeshProxyFlag( RMPF_MergedMesh ) )
		return;

	// Update stuff that should be updated once per frame
	UpdateOncePerFrame( collector );

	// filter by render mask	
	Uint8 proxyRenderMask = m_renderMask;
	if ( m_localVisId != 0 )
	{
#ifndef RED_FINAL_BUILD
		if ( collector.GetRenderFrameInfo().IsShowFlagOn( SHOW_UseVisibilityMask ) )
#endif
		{
			proxyRenderMask &= collector.GetVisibilityFilter().GetMaskFilterForLocalID( m_localVisId );
		}
	}

	// should not be rendered at all ?
	if ( !(proxyRenderMask & MCR_Scene) )
		return;

	// Collect the related entity group
	if ( GetEntityGroup() != nullptr )
	{
		GetEntityGroup()->CollectElements( collector );
	}

	// Collection mask
	const Uint8 renderMask = MCR_Scene;

	// We are overriding mesh material
	if ( m_replacedMaterialLodGroup )
	{
		UpdateVisibilityQueryState( CRenderVisibilityQueryManager::VisibleScene );

		for ( Uint8 i=0; i<m_replacedMaterialLodGroup->GetChunkCount(); i++ )
		{
			CRenderElement_MeshChunk* chunk = m_replacedMaterialLodGroup->GetChunk(i);
			if ( chunk->CheckRenderMask( renderMask, false /* no merged data for normal rendering */ ) )
			{
				if ( ShouldCollectElement( &collector, false, chunk ) )
				{
					collector.PushElement( chunk );
				}
			}
		}

		// We don't want the original to be drawn
		if ( !CheckMeshProxyFlag( RMPF_DrawOriginalWhileReplacingMaterial ) )
		{
			return;
		}
	}

	// Should we dissolve the rendering ?
	Bool isDissolved = IRenderProxyFadeable::IsFading( collector.GetDissolveSynchronizer() ) || CheckMeshProxyFlag( RMPF_DissolveOverride );

	// Get the LODs to use on the meshes
	Int32 baseLOD=-1, nextLOD=-1;
	GetLOD( collector, baseLOD, nextLOD, &isDissolved /* dissolve during the LOD change */ ); 

	// Draw base LOD (if not empty)
	if ( DissolveHelpers::IsLodVisible(baseLOD) )
	{
#ifndef RED_FINAL_BUILD
		++collector.m_collectedMeshes;
#endif // RED_FINAL_BUILD

		// Update the scene visibility query
		UpdateVisibilityQueryState( CRenderVisibilityQueryManager::VisibleScene );

		// Base LOD
		PushLodGroupElements( collector, baseLOD, isDissolved, false, renderMask );
	}

	// Draw next LOD (we are during LOD transition)
	if ( nextLOD != baseLOD && DissolveHelpers::IsLodVisible(nextLOD) )
	{
		PushLodGroupElements( collector, nextLOD, isDissolved, false, renderMask );
	}
}

void CRenderProxy_Mesh::PushLodGroupElements( CRenderCollector& collector, Int32 lodGroupIndex, const Bool isDissolved, const Bool useMergedGeometry, const Uint8 renderMask )
{	
	RED_FATAL_ASSERT( lodGroupIndex >= 0 && lodGroupIndex < m_lodGroups.SizeInt(), "Invalid LOD index" );
	
	const MeshLodGroup& group = m_lodGroups[ lodGroupIndex ];
	for ( Uint8 i=0, end = group.GetChunkCount(); i< end; i++ )
	{
		CRenderElement_MeshChunk* chunk;

		// For LOD 0, use a normal blend chunk if it's available.
		if ( lodGroupIndex == 0 && m_normalBlendLodGroup && m_normalBlendLodGroup->GetChunk(i) )
		{
			chunk = m_normalBlendLodGroup->GetChunk(i);
		}
		else
		{
			chunk = group.GetChunk(i);
		}

		if ( chunk->CheckRenderMask( renderMask, useMergedGeometry ) )
		{
			if ( ShouldCollectElement( &collector, false, chunk ) )
			{
				chunk->SetMeshChunkFlag( RMCF_UsesDissolve, isDissolved ); // HACK - this should be directly passed to the collector
				collector.PushElement( chunk );
			}
		}
	}
}

void CRenderProxy_Mesh::SetDissolveOverride(Bool useDissolve)
{
	SetMeshProxyFlag( RMPF_DissolveOverride, useDissolve );
}

void CRenderProxy_Mesh::SetMeshProxyFlag( ERenderMeshProxyFlags flag, Bool flagValue /*= true*/ )
{
	if ( flagValue )
	{
		m_meshProxyFlags |= flag;
	}
	else
	{
		m_meshProxyFlags &= ~flag;
	}
}

void CRenderProxy_Mesh::ClearMeshProxyFlag( ERenderMeshProxyFlags flag )
{
	m_meshProxyFlags &= ~flag;
}

void CRenderProxy_Mesh::CollectForcedCascadeShadowElements( SMergedShadowCascades& cascades, Uint32 perCascadeTestFlags )
{
#ifndef RED_FINAL_BUILD
	RED_FATAL_ASSERT( cascades.m_collector != nullptr, "Cascades cannot be collected without render collector" );
	RED_FATAL_ASSERT( m_scene != nullptr, "Trying to collect proxy that is not in a scene - hacky hacky rendering" );

	extern SceneRenderingStats GRenderingStats;

	// Do not render merged meshes in to the forced shadows
	if ( CheckMeshProxyFlag( RMPF_MergedMesh ) )
		return;

	// Never use character shadows fallback
	if ( CheckMeshProxyFlag( RMPF_IsCharacterShadowFallback ) )
		return;
	
	// Use LOD0 always
	for ( Uint16 cascadeIndex=0; cascadeIndex<cascades.m_numCascades; ++cascadeIndex, perCascadeTestFlags >>= 2 )
	{
		SShadowCascade* cascade = &cascades.m_cascades[ cascadeIndex ];

		// Test the visibility of the fragment in this cascade
		const Uint32 visResult = perCascadeTestFlags & 3;
		if ( visResult == 0 )
		{
			continue;
		}

		// stats
		GRenderingStats.m_numCascadeProxies[ cascadeIndex ] += 1;
		++cascades.m_collector->m_collectedMeshes;

		// Draw base LOD
		if ( !m_lodGroups.Empty() )
		{
			// Current LOD
			const MeshLodGroup& group = m_lodGroups[ 0 ];
			for ( Uint32 i=0, end = group.GetChunkCount(); i<end; ++i )
			{
				CRenderElement_MeshChunk* chunk = group.GetChunk(i);

				if ( ShouldCollectElement( cascades.m_collector, true, chunk ) )
				{	
					const Bool isDissolved = false; //< we are never dissolving shadows in high quality mode (it's a reference mode)
					const Bool hasDiscard = IsElementMasked( chunk ) | chunk->HasFlag( RMCF_UsesUVDissolve );
					chunk->SetMeshChunkFlag( RMCF_UsesCascadesDissolve, isDissolved ); // HACK - this should be directly passed to the collector
					cascade->PushShadowElement( chunk, hasDiscard );
				}
			}
		}

		// Do not add to bigger collect if we are fully inside the smaller cascade
		// THIS WORKS ONLY IF BLENDING BETWEEN CASCADES IS DISABLED
		if ( visResult == 2 )
		{
			break;
		}
	}
#endif
}

void CRenderProxy_Mesh::SetUseShadowDistances( Bool enable )
{
	SetMeshProxyFlag( RMPF_UseShadowDistances, enable );

	if ( !enable )
	{
		SetShadowFadeOne();
	}
}

Bool CRenderProxy_Mesh::IsProxyReadyForRendering() const
{
	// not ready
	if ( Config::cvFakeMeshesNotReady.Get() )
		return false;

	// mesh is not yet loaded
	if ( m_mesh && !m_mesh->IsFullyLoaded() )
		return false;

	// data is loaded
	return true;
}

void CRenderProxy_Mesh::CollectCascadeShadowElements( SMergedShadowCascades& cascades, Uint32 perCascadeTestFlags )
{
	RED_FATAL_ASSERT( cascades.m_collector != nullptr, "Cascades cannot be collected without render collector" );
	RED_FATAL_ASSERT( m_scene != nullptr, "Trying to collect proxy that is not in a scene - hacky hacky rendering" );

#ifndef RED_FINAL_BUILD
	extern SceneRenderingStats GRenderingStats;
#endif

	// proxy not visible, even in shadows
	if ( !IsVisible() && !CheckMeshProxyFlag( RMPF_CastingShadowsEvenIfNotVisible ) )
		return;

#ifndef RED_FINAL_BUILD
	// stats
	GRenderingStats.m_numCascadeProxiesTotal += 1;
#endif

	// Special mode of rendering shadows - high quality preview
#ifndef RED_FINAL_BUILD
	if ( cascades.m_collector->GetRenderFrameInfo().IsShowFlagOn( SHOW_ForceAllShadows ) )
	{
		CollectForcedCascadeShadowElements( cascades, perCascadeTestFlags );
		return;
	}
#endif

	// filter by render mask
	const Uint8 originalRenderMask = m_renderMask;
	Uint8 proxyRenderMask = m_renderMask;
	if ( m_localVisId != 0 )
	{
#ifndef RED_FINAL_BUILD
		if ( cascades.m_collector->GetRenderFrameInfo().IsShowFlagOn( SHOW_UseVisibilityMask ) )
#endif
		{
			proxyRenderMask &= cascades.m_collector->GetVisibilityFilter().GetMaskFilterForLocalID( m_localVisId );
		}
	}

	// should not be rendered at all ?
	const Uint8 cascadeMask = MCR_Cascade1 | MCR_Cascade2 | MCR_Cascade3 | MCR_Cascade4;
	if ( !(proxyRenderMask & cascadeMask) )
		return;

	// Update stuff that should be updated once per frame
	UpdateOncePerFrame( *cascades.m_collector );

	// Skip if shadow faded out
	if ( GetShadowFadeAlpha().IsZero() )
	{
		return;
	}

#ifndef RED_FINAL_BUILD
	// stats
	GRenderingStats.m_numCascadeProxiesTotal += 1;
#endif

	// Should we be dissolving ?
	Bool isDissolved = !GetShadowFadeAlpha().IsOne() || GetGenericDissolve().IsFading( cascades.m_collector->GetDissolveSynchronizer() ) || CheckMeshProxyFlag( RMPF_DissolveOverride );

	// Get the LODs to use on the meshes
	Int32 baseLOD=-1, nextLOD=-1;
	GetLOD( *cascades.m_collector, baseLOD, nextLOD, &isDissolved /* lod transition is also forcing us to use dissolved rendering */ );

	if( !DissolveHelpers::IsLodVisible(baseLOD) && !DissolveHelpers::IsLodVisible(nextLOD) )
	{
		return;
	}

	// Do not allow shadow dissolves for merged meshes
	if ( CheckMeshProxyFlag( RMPF_MergedMesh ) )
		isDissolved = false;

	// Due to the nature of the merged geometry we cannot use the dissolves on it
#ifdef RED_FINAL_BUILD
	const Bool useMergedData = !isDissolved;
	const Bool hasAdditionalElements = CheckMeshProxyFlag( RMPF_HasAdditionalShadowElements ); 
#else
	// use more hacky versions for more debugging
	const Bool useMergedData = !isDissolved && !Config::cvHackyDisableMergedChunks.Get();
	const Bool hasAdditionalElements = CheckMeshProxyFlag( RMPF_HasAdditionalShadowElements ) && !Config::cvHackyDisableAdditionalAlphaShadowMeshParts.Get();
#endif

#if HACKY_ENTITY_PROXY_SHADOWS_FIX
	const Bool needEntityProxyCascadeHack = CheckMeshProxyFlag( RMPF_HACK_EntityProxyNeedsShadows );
#endif

	// Process all cascades
	Bool visibleInCascades = false;
	const Uint16 numCascades = cascades.m_numCascades;
	const Uint16 numFullDetailCascades = numCascades;
	for ( Uint16 cascadeIndex=0; cascadeIndex<numFullDetailCascades; ++cascadeIndex, perCascadeTestFlags >>= 2 )
	{
		SShadowCascade* cascade = &cascades.m_cascades[ cascadeIndex ];

		// Test the visibility of the fragment in this cascade
		const Uint32 visResult = perCascadeTestFlags & 3;

		// Render mask for this cascade
		Bool renderOnlyAdditionalElements = false;
#if HACKY_ENTITY_PROXY_SHADOWS_FIX
		const Uint8 cascadeRenderMask = ( needEntityProxyCascadeHack && cascadeIndex == 0 ) ? MCR_Scene : ( (Uint8)MCR_Cascade1 << cascadeIndex );
#else
		const Uint8 cascadeRenderMask = ( (Uint8)MCR_Cascade1 << cascadeIndex );
#endif

		if ( !(cascadeRenderMask & proxyRenderMask ) )
		{
			// this object was culled due to the visibility exclusion
			// do not bother rendering extra alpha shit in other cascade than 1, in cascade 0 we are using full meshes any way and cascades >1 are to far away
			if ( hasAdditionalElements && (cascadeIndex == 1) )
			{
				// we cannot totally forget about this LOD, try to draw it regardless
				renderOnlyAdditionalElements = true;
			}
			else
			{
				// we don't have any special elements, we can skip whole step
				if ( cascadeRenderMask & originalRenderMask )
				{
#ifndef RED_FINAL_BUILD
					GRenderingStats.m_numCascadeProxiesExlusionFiltered += 1;
#endif
				}

				continue;
			}
		}

		// We are not in the cascade
		if ( visResult == 0 )
		{
			continue;
		}

#ifndef RED_FINAL_BUILD
		// stats
		GRenderingStats.m_numCascadeProxies[ cascadeIndex ] += 1;
		++cascades.m_collector->m_collectedMeshes;
#endif

		// Draw base LOD
		if ( DissolveHelpers::IsLodVisible(baseLOD) )
		{
			// Current LOD
			const MeshLodGroup& group = m_lodGroups[ baseLOD ];
			for ( Uint32 i=0, end = group.GetChunkCount(); i<end; ++i )
			{
				CRenderElement_MeshChunk* chunk = group.GetChunk(i);

				if ( chunk->CheckRenderMask( cascadeRenderMask, useMergedData ) )
				{
					if ( ShouldCollectElement( cascades.m_collector, true, chunk ) )
					{
						if ( !renderOnlyAdditionalElements || chunk->HasFlag( RMCF_AdditionalShadowElement ) )
						{
							if ( CheckMeshProxyFlag( RMPF_MergedMesh ) )
							{
								RED_ASSERT( !isDissolved );
								cascade->PushMergedShadowElement( chunk );
							}
							else
							{
								const Bool hasDiscard = isDissolved || IsElementMasked( chunk ) || chunk->HasFlag( RMCF_UsesUVDissolve );
								chunk->SetMeshChunkFlag( RMCF_UsesCascadesDissolve, isDissolved ); // HACK - this should be directly passed to the collector
								cascade->PushShadowElement( chunk, hasDiscard );
							}
						}
					}
				}
			}

			// We are visible in a cascade
			visibleInCascades = true;
		}

		// Next LOD ( when dissolving )
		if ( DissolveHelpers::IsLodVisible(nextLOD) && nextLOD != baseLOD )
		{
			const MeshLodGroup& group = m_lodGroups[ nextLOD ];
			for ( Uint32 i=0, end = group.GetChunkCount(); i<end; ++i )
			{
				CRenderElement_MeshChunk* chunk = group.GetChunk(i);
				if ( chunk->CheckRenderMask( cascadeRenderMask, useMergedData ) )
				{
					if ( ShouldCollectElement( cascades.m_collector, true, chunk ) )
					{
						if ( !renderOnlyAdditionalElements || chunk->HasFlag( RMCF_AdditionalShadowElement ) )
						{
							if ( CheckMeshProxyFlag( RMPF_MergedMesh ) )
							{
								RED_ASSERT( !isDissolved );
								cascade->PushMergedShadowElement( chunk );
							}
							else
							{
								const Bool hasDiscard = isDissolved || IsElementMasked( chunk ) || chunk->HasFlag( RMCF_UsesUVDissolve );
								chunk->SetMeshChunkFlag( RMCF_UsesCascadesDissolve, isDissolved ); // HACK - this should be directly passed to the collector
								cascade->PushShadowElement( chunk, hasDiscard );
							}
						}
					}
				}
			}
		}

		// Do not add to bigger collect if we are fully inside the smaller cascade
		// THIS WORKS ONLY IF BLENDING BETWEEN CASCADES IS DISABLED
		if ( visResult == 2 )
		{
#ifndef RED_FINAL_BUILD
			GRenderingStats.m_numCascadeProxiesOptimized += 1;
#endif
			break;
		}
	}

	// Update the scene visibility query
	if ( visibleInCascades )
	{
		UpdateVisibilityQueryState( CRenderVisibilityQueryManager::VisibleMainShadows );
	}
}

void CRenderProxy_Mesh::CollectShadowLodsForced( Uint8 lodIndex, CRenderCollector::SRenderElementContainer& elementCollector )
{
	RED_FATAL_ASSERT( m_scene != nullptr, "Trying to collect proxy that is not in a scene - hacky hacky rendering" );

	// proxy not visible, even in shadows
	if ( !IsVisible() && !CheckMeshProxyFlag( RMPF_CastingShadowsEvenIfNotVisible ) )
		return;

	// Never use character shadows fallback
	if ( CheckMeshProxyFlag( RMPF_IsCharacterShadowFallback ) )
		return;

	MeshLodGroup& group = m_lodGroups[ lodIndex ];
	for ( Uint32 i=0, end = group.GetChunkCount(); i<end; ++i )
	{
		CRenderElement_MeshChunk* chunk = group.GetChunk(i);
		if ( chunk->CheckRenderMask( MCR_Scene, false ) )
		{
			chunk->SetMeshChunkFlag( RMCF_UsesDissolve, false );
			elementCollector.PushMeshChunk( chunk );
		}
	}
}

// When drawing static shadows collect geometry from highest LOD
void CRenderProxy_Mesh::CollectStaticShadowElements( const CRenderCollector& collector, const CRenderCamera& shadowCamera, SShadowCascade& elementCollector )
{
	RED_FATAL_ASSERT( m_scene != nullptr, "Trying to collect proxy that is not in a scene - hacky hacky rendering" );

	// proxy not visible, even in shadows
	if ( !IsVisible() && !CheckMeshProxyFlag( RMPF_CastingShadowsEvenIfNotVisible ) )
		return;

	// Never use character shadows fallback
	if ( CheckMeshProxyFlag( RMPF_IsCharacterShadowFallback ) )
		return;

	// Nothing to collect when we don't have any lodGroups
	if ( m_lodGroups.Empty() )
		return;
		
	UpdateOncePerFrame( collector );

	// Highest LOD only
	MeshLodGroup& group = m_lodGroups[ 0 ];
	for ( Uint32 i=0, end = group.GetChunkCount(); i<end; ++i )
	{
		CRenderElement_MeshChunk* chunk = group.GetChunk(i);

		// Due to the nature of the merged geometry we cannot use the dissolves on it
		if ( ShouldCollectElement( nullptr, true, chunk ) )
		{
			const Bool hasDiscard = /*dissolved ||*/ IsElementMasked( chunk ) || chunk->HasFlag( RMCF_UsesUVDissolve );
			elementCollector.PushShadowElement( chunk, hasDiscard );
		}
	}
}

void CRenderProxy_Mesh::CollectHiResShadowsElements( CRenderCollector& collector, CRenderCollector::HiResShadowsCollector& elementCollector, Bool forceHighestLOD )
{
	RED_FATAL_ASSERT( m_scene != nullptr, "Trying to collect proxy that is not in a scene - hacky hacky rendering" );

	// proxy not visible, even in shadows
	if ( !IsVisible() && !CheckMeshProxyFlag( RMPF_CastingShadowsEvenIfNotVisible ) )
		return;

	// Never use merged meshes in hi-res shadows
	if ( CheckMeshProxyFlag( RMPF_MergedMesh ) )
		return;

	// Never use character shadows fallback
	if ( CheckMeshProxyFlag( RMPF_IsCharacterShadowFallback ) )
		return;

	// Update once-per frame stuff
	UpdateOncePerFrame( collector );

	// Should we be dissolving ?
	Bool isDissolved = IRenderProxyFadeable::IsFading( collector.GetDissolveSynchronizer() ) || CheckMeshProxyFlag( RMPF_DissolveOverride );

	// ignore m_shadowFade for hires shadows

	// Get the LODs to use on the meshes
	Int32 baseLOD=-1, nextLOD=-1;
	GetLOD( collector, baseLOD, nextLOD, &isDissolved /* LOD change is causing a dissolve to */ );

	// We can use merged geometry only if we are not dissolving
	const Bool useMergedGeometry = !isDissolved;

	// Draw base LOD
	if ( DissolveHelpers::IsLodVisible(baseLOD) )
	{
		// Update the scene visibility query
		UpdateVisibilityQueryState( CRenderVisibilityQueryManager::VisibleMainShadows );

		// Current LOD
		const MeshLodGroup& group = m_lodGroups[ baseLOD ];
		for ( Uint32 i=0, end = group.GetChunkCount(); i<end; ++i )
		{
			CRenderElement_MeshChunk* chunk = group.GetChunk(i);
			if ( chunk->CheckRenderMask( MCR_LocalShadows, useMergedGeometry ) )
			{
				if ( ShouldCollectElement( &collector, true, chunk ) )
				{
					OnCollectElement( &collector, true, chunk );
					const Bool isHair = (chunk->GetSortGroup() == RSG_Hair);
					const Bool hasDiscards = IsElementMasked( chunk ) || isDissolved || chunk->HasFlag( RMCF_UsesUVDissolve );
					elementCollector.PushMeshChunk( chunk, hasDiscards, isHair );
				}
			}
		}
	}

	// Draw next LOD
	if ( DissolveHelpers::IsLodVisible(nextLOD) && nextLOD != baseLOD )
	{
		const MeshLodGroup& group = m_lodGroups[ nextLOD ];
		for ( Uint32 i=0, end = group.GetChunkCount(); i<end; ++i )
		{
			CRenderElement_MeshChunk* chunk = group.GetChunk(i); //< normalBlend only for first LOD
			if ( chunk->CheckRenderMask( MCR_LocalShadows, useMergedGeometry ) )
			{
				if ( ShouldCollectElement( &collector, true, chunk ) )
				{
					OnCollectElement( &collector, true, chunk );
					const Bool isHair = (chunk->GetSortGroup() == RSG_Hair);
					const Bool hasDiscards = IsElementMasked( chunk ) || isDissolved || chunk->HasFlag( RMCF_UsesUVDissolve );
					elementCollector.PushMeshChunk( chunk, hasDiscards, isHair );
				}
			}
		}
	}
}

void CRenderProxy_Mesh::CollectLocalShadowElements( const CRenderCollector& collector, const CRenderCamera& shadowCamera, SShadowCascade& elementCollector )
{
	RED_FATAL_ASSERT( m_scene != nullptr, "Trying to collect proxy that is not in a scene - hacky hacky rendering" );

	// proxy not visible, even in shadows
	if ( !IsVisible() && !CheckMeshProxyFlag( RMPF_CastingShadowsEvenIfNotVisible ) )
		return;

	// Never use merged meshes in hi-res shadows
	if ( CheckMeshProxyFlag( RMPF_MergedMesh ) )
		return;

	// Update once-per frame stuff
	UpdateOncePerFrame( collector );

	// Skip if shadow faded out
	if ( GetShadowFadeAlpha().IsZero() )
	{
		return;
	}

	// Should we be dissolving ?
	Bool isDissolved = !GetShadowFadeAlpha().IsOne() || IRenderProxyFadeable::IsFading( collector.GetDissolveSynchronizer() ) || CheckMeshProxyFlag( RMPF_DissolveOverride );

	// Get the LODs to use on the meshes
	Int32 baseLOD=-1, nextLOD=-1;
	GetLOD( collector, baseLOD, nextLOD, &isDissolved /* LOD change is causing a dissolve to */ );

	// We can use merged geometry only if we are not dissolving
	const Bool useMergedGeometry = !isDissolved;

	// Draw base LOD
	if ( DissolveHelpers::IsLodVisible(baseLOD) )
	{
		// Update the scene visibility query
		UpdateVisibilityQueryState( CRenderVisibilityQueryManager::VisibleAdditionalShadows );

		// Current LOD
		const MeshLodGroup& group = m_lodGroups[ baseLOD ];
		for ( Uint32 i = 0, end = group.GetChunkCount(); i < end; ++i )
		{
			CRenderElement_MeshChunk* chunk = group.GetChunk(i);
			if ( chunk->CheckRenderMask( MCR_LocalShadows, useMergedGeometry ) )
			{
				if ( ShouldCollectElement( nullptr, true, chunk ) )
				{
					OnCollectElement( nullptr, true, chunk );
					const Bool hasDiscard = isDissolved || IsElementMasked( chunk ) || chunk->HasFlag( RMCF_UsesUVDissolve );
					elementCollector.PushShadowElement( chunk, hasDiscard );
				}
			}
		}
	}

	// Next LOD ( when dissolving )
	if ( DissolveHelpers::IsLodVisible(nextLOD) && nextLOD != baseLOD )
	{
		const MeshLodGroup& group = m_lodGroups[ nextLOD ];
		for ( Uint32 i = 0, end = group.GetChunkCount(); i < end; ++i )
		{
			CRenderElement_MeshChunk* chunk = group.GetChunk(i);
			if ( chunk->CheckRenderMask( MCR_LocalShadows, useMergedGeometry ) )
			{
				if ( ShouldCollectElement( nullptr, true, chunk ) )
				{
					OnCollectElement( nullptr, true, chunk );
					const Bool isMasked = IsElementMasked( chunk );
					const Bool hasDiscard = isDissolved || IsElementMasked( chunk ) || chunk->HasFlag( RMCF_UsesUVDissolve );
					elementCollector.PushShadowElement( chunk, hasDiscard );
				}
			}
		}
	}
}

void CRenderProxy_Mesh::UpdateLOD( const CRenderCollector& collector, const Bool wasVisibleLastFrame )
{
	/*if ( HasCustomReferencePoint() )
	{
		int a, b;
		m_lodSelector.GetLODIndices(a, b);

		LOG_ENGINE( TXT("Proxy[0x%016llX]: DIST %f, SQDIST %f, LOD %d, NEXT LOD %d, AH %f, REF [%f,%f,%f]"),
			(Uint64)this, MSqrt(GetCachedDistanceSquared()), GetCachedDistanceSquared(), a, b, m_autoHideDistance );
	}*/

	// Calculate preferred LOD, returns the LOD distance
#ifndef NO_EDITOR
	// Editor forced LOD
	const Int8 forceLOD = (Int8)m_scene->GetForcedLOD();
	if ( forceLOD != -1 )
	{
		const Int8 lod = Min< Int8 >( forceLOD, GetLodSelector().GetNumLODs()-1 );
		GetLodSelector().ForceLOD( lod );
		return;
	}
#else
	#ifdef USE_ANSEL
	if ( isAnselSessionActive )
	{
		const Int8 forceLOD = (Int8)m_scene->GetForcedLOD();
		if ( forceLOD != -1 )
		{
			const Int8 lod = Min< Int8 >( 0, GetLodSelector().GetNumLODs() - 1 );
			GetLodSelector().ForceLOD( lod );
			return;
		}
	}
	#endif // USE_ANSEL
#endif // NO_EDITOR

	// Because of this hack with replacedMaterial, similar one have to exist on the gameplay side in CMimicComponent::GetLodBoneNum()!
	// If you ever change condition for hack here, remember to update also condition on the gameplay side.
	// If you are removing this hack, please also remove the one in CMimicComponent::GetLodBoneNum().
	const Bool forceLod0 = ( m_replacedMaterialLodGroup != nullptr ) || CheckMeshProxyFlag( RMPF_ForceHighestLOD );

	if ( HasCustomReferencePoint() && !forceLod0 )
	{
		Float lodDistance = GetCachedDistanceSquared();
		Float visDistance = GetCachedDistanceSquared();

		// rendering consistency: preserve the low res model visibility while waiting for the high res object to stream
		Int32 minLOD = -1, maxLOD = 100;
		if ( m_renderGroup && !m_renderGroup->IsGroupReadyForRendering(collector) )
		{
			if ( CheckMeshProxyFlag( RMPF_IsEntityProxy ) )
			{
				// keep the low-res proxy visible until the group is ready
				minLOD = 1;
			}
			else
			{
				// keep the high res proxy hidden until all of the are loaded
				minLOD = -1;
				maxLOD = -1;
			}
		}

		// W3 hack: our beloved environmental artist require for some large buildings (castles) that the distances for the visibility and the LOD distances are different
		// So, for LOD we ALWAYS use the true distance
		const Float trueDistnaceSq = GetBoundingBox().CalcCenter().DistanceSquaredTo( collector.GetRenderCamera().GetPosition() );
		lodDistance = trueDistnaceSq;

		IRenderProxyDissolvable::UpdateLOD( lodDistance, collector, wasVisibleLastFrame, false, Min<Int32>( minLOD , m_lodGroups.SizeInt()-1 ), maxLOD );
	}
	else
	{
		IRenderProxyDissolvable::UpdateLOD( collector, wasVisibleLastFrame, forceLod0 );
	}
}

void CRenderProxy_Mesh::UpdateFades( const CRenderCollector& collector, const Bool wasVisibleLastFrame )
{
	IRenderProxyDissolvable::UpdateFades( collector , wasVisibleLastFrame );

	// update shadow fade alpha
	if ( CheckMeshProxyFlag( RMPF_UseShadowDistances ) )
	{
		if ( GetEntityGroup() )
		{
			// Update entity group shadow fade
			GetEntityGroup()->UpdateOncePerFrame( collector );

			// Get appropriate shadow fade
			RED_ASSERT( IsCharacterShadowFallback() == CheckMeshProxyFlag( RMPF_IsCharacterShadowFallback ) );
			SetShadowfadeAlpha( GetEntityGroup()->GetShadowFadeAlpha( collector.GetDissolveSynchronizer(), CheckMeshProxyFlag( RMPF_IsCharacterShadowFallback ) ) );
		}
		else
		{
			SetShadowFadeOne();
		}
	}

	IRenderProxyFadeable::UpdateDistanceFade( GetCachedDistanceSquared(), m_autoHideDistanceSquared, wasVisibleLastFrame && !collector.WasLastFrameCameraInvalidated() );
}

const EFrameUpdateState CRenderProxy_Mesh::UpdateOncePerFrame( const CRenderCollector& collector )
{
	// Compute basic stuff
	const auto ret = IRenderProxyDrawable::UpdateOncePerFrame( collector );
	if ( ret == FUS_AlreadyUpdated )
		return ret;

	// Update LOD
	const Bool wasVisibleLastFrame = (ret == FUS_UpdatedLastFrame);
	UpdateLOD( collector, wasVisibleLastFrame );
	UpdateFades( collector, wasVisibleLastFrame );

	// We were updated
	return ret;
}

Bool CRenderProxy_Mesh::IsAllowedForEnvProbes() const
{
	// ace_ibl_fix: pimp this, it doesn't include everything it should (for instance geralt's swords, and I bet some other shit also)
	return IRenderProxyDrawable::IsAllowedForEnvProbes() && !CheckMeshProxyFlag( RMPF_IsCharacterProxy );
}

Bool CRenderProxy_Mesh::IsCharacterShadowFallback() const
{
	return CheckMeshProxyFlag( RMPF_IsCharacterShadowFallback );
}

void CRenderProxy_Mesh::SetSkinningData( IRenderObject* skinningData )
{
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

	// If we've just added skinning data, we become a dynamic object. Won't do the other way, since we might still be dynamic for some other reason
	// and we don't want to mess that up.
	if ( m_skinningData != nullptr )
	{
		m_lightChannels |= LC_DynamicObject;
	}
}

void CRenderProxy_Mesh::SetReplacedMaterial( IRenderResource* material, IRenderResource* parameters, Bool drawOriginal )
{
	// Delete current replacement group
	if ( m_replacedMaterialLodGroup )
	{
		delete m_replacedMaterialLodGroup;
		m_replacedMaterialLodGroup = nullptr;
	}
	
	// There are no LOD groups here
	if ( m_lodGroups.Size() == 0 )
	{
		WARN_RENDERER( TXT("Unable to replace material when mesh has no lod groups!") );
		return;
	}

	// Create new replacement LOD group
	if( material != nullptr && parameters != nullptr )
	{
		m_replacedMaterialLodGroup = m_lodGroups[0].CreateCopyWithMaterialOverride( static_cast< CRenderMaterial* >( material ), static_cast< CRenderMaterialParameters* >( parameters ) );
	}
	else
	{
		RED_HALT("Material replacement created without proper material");
	}

	// Set the draw original flag
	SetMeshProxyFlag( RMPF_DrawOriginalWhileReplacingMaterial, drawOriginal );

	// Force lod 0
	GetLodSelector().ForceLOD(0);
}

Bool CRenderProxy_Mesh::HasReplacedMaterial()
{
	return m_replacedMaterialLodGroup != nullptr;
}

void CRenderProxy_Mesh::DisableMaterialReplacement()
{
	if ( m_replacedMaterialLodGroup )
	{
		delete m_replacedMaterialLodGroup;
		m_replacedMaterialLodGroup = nullptr;
	}
}

void CRenderProxy_Mesh::SetNormalBlendMaterial( IRenderResource* nbMaterial, IRenderResource* parameters, IRenderResource* sourceMaterial, IRenderResource* sourceTexture )
{
	//
	// Build normal-blend LOD group, replacing any instances of sourceMaterial that use sourceTexture with nbMaterial.
	//

	// If we have no LOD groups (no mesh), we can't do anything.
	if ( m_lodGroups.Size() == 0 )
	{
		return;
	}

	// Make an empty LOD group if we don't already have it.
	if ( !m_normalBlendLodGroup )
	{
		// If we don't have the LOD group created, and are clearing a normal-blend material (nbMaterial == NULL), we don't need to do anything!
		if ( !nbMaterial )
		{
			return;
		}

		// NULL material and parameter so that no chunks will be created initially. We fill them in below only for chunks that we want to replace.
		m_normalBlendLodGroup = m_lodGroups[0].CreateCopyWithMaterialOverride( nullptr, nullptr );
	}

	// Check through our LOD 0 chunks, create new normal-blend chunks where appropriate.
	for ( Uint32 i = 0, end = m_lodGroups[0].GetChunkCount(); i < end; ++i )
	{
		CRenderElement_MeshChunk* chunk = m_lodGroups[0].GetChunk(i);
		// If the source material matches...
		if ( !sourceMaterial || chunk->GetMaterial() == sourceMaterial )
		{
			// ... and the source uses the texture...
			if ( !sourceTexture || chunk->GetMaterialParams()->m_textures.Exist( sourceTexture ) )
			{
				// ... then create a new chunk with the new normal-blend material.
				// Delete any existing chunk first.
				m_normalBlendLodGroup->ReleaseChunk(i);

				// If we've got a material to replace with, we create a new chunk.
				if ( nbMaterial )
				{
					auto * newChunk = m_lodGroups[0].GetChunk(i)->CreateCopyWithMaterialOverride( static_cast< CRenderMaterial* >( nbMaterial ), static_cast< CRenderMaterialParameters* >( parameters ) );
					newChunk->SetMeshChunkFlag( RMCF_NormalBlendMaterial, true );
					m_normalBlendLodGroup->SetChunk( i, newChunk );
				}
			}
		}
	}

	// Check if we still need our LOD group. If it's all NULL, we can delete it.
	Bool allNull = true;
	for ( Uint32 i = 0, end = m_normalBlendLodGroup->GetChunkCount(); i < end; ++i )
	{
		if ( m_normalBlendLodGroup->GetChunk(i) )
		{
			allNull = false;
			break;
		}
	}

	if ( allNull )
	{
		delete m_normalBlendLodGroup;
		m_normalBlendLodGroup = nullptr;
	}
}

void CRenderProxy_Mesh::SetNormalBlendAreas( Uint32 firstArea, Uint32 numAreas, const Float* areas )
{
	if ( nullptr == m_normalBlendAreas )
	{
		m_normalBlendAreas = static_cast< Float* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_RenderNormalBlendingData , NUM_NORMALBLEND_AREAS * sizeof( Float ) * 4) );
	}
	
	if ( firstArea > NUM_NORMALBLEND_AREAS )
	{
		return;
	}
	numAreas = Min(numAreas, NUM_NORMALBLEND_AREAS - firstArea);

	// Rearrange areas so they're more friendly for the shader. They end up like:
	// minX0 minX1 minX2 minX3
	// ...
	// minY0 minY1 minY2 minY3
	// ...
	// maxX0 maxX1 maxX2 maxX3
	// ...
	// maxY0 maxY1 maxY2 maxY3
	// ...
	for ( Uint32 i = firstArea; i < firstArea + numAreas; ++i )
	{
		m_normalBlendAreas[ 0 * NUM_NORMALBLEND_AREAS + i ] = areas[ ( i - firstArea ) * 4 + 0 ];
		m_normalBlendAreas[ 1 * NUM_NORMALBLEND_AREAS + i ] = areas[ ( i - firstArea ) * 4 + 1 ];
		m_normalBlendAreas[ 2 * NUM_NORMALBLEND_AREAS + i ] = areas[ ( i - firstArea ) * 4 + 2 ];
		m_normalBlendAreas[ 3 * NUM_NORMALBLEND_AREAS + i ] = areas[ ( i - firstArea ) * 4 + 3 ];
	}
}

void CRenderProxy_Mesh::SetNormalBlendWeights( Uint32 firstWeight, Uint32 numWeights, const Float *weights )
{
	if ( nullptr == m_normalBlendWeights )
	{
		m_normalBlendWeights = static_cast< Float* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_RenderNormalBlendingData , NUM_NORMALBLEND_AREAS * sizeof( Float )) );
	}

	if ( firstWeight > NUM_NORMALBLEND_AREAS )
	{
		return;
	}

	numWeights = Min(numWeights, NUM_NORMALBLEND_AREAS - firstWeight);

	for ( Uint32 i=0; i<numWeights; ++i )
	{
		m_normalBlendWeights[firstWeight + i] = Max( 0.f, weights[i] );
	}
}

Bool CRenderProxy_Mesh::IsAnyNormalBlendAreaActive() const
{
	Bool isAnyActive = false;

	// No normal-blend areas can actually be active if we don't even have the LOD group...
	if ( m_normalBlendLodGroup && m_normalBlendWeights )
	{
		for ( Uint32 i = 0; i < NUM_NORMALBLEND_AREAS; ++i )
		{
			ASSERT( m_normalBlendWeights[i] >= 0.f && "Should have been tested during weights update" );
			if ( 0.f != m_normalBlendWeights[i] )
			{
				isAnyActive = true;
				break;
			}
		}
	}

	return isAnyActive;
}

void CRenderProxy_Mesh::AttachToScene( CRenderSceneEx* scene )
{
	IRenderProxyDrawable::AttachToScene( scene );

	// If mesh has some skinnable parts assume that proxy is dynamic.
	Bool dynamicGeometry = false;
	for ( Uint32 i=0; i<m_lodGroups.Size(); ++i )
	{
		const MeshLodGroup& group = m_lodGroups[i];
		for ( Uint32 j=0, end = group.GetChunkCount(); j<end; ++j )
		{
			if ( group.GetChunk(j)->IsSkinnable() )
			{
				dynamicGeometry = true;
				m_drawableFlags |= RPF_Dynamic;
				break;
			}
		}
	}

	// Add to entity group proxy list
	IRenderEntityGroupProxy::AttachToScene();

	// If proxy is not dynamic than register it in the visibility filtering map
	if ( !CheckMeshProxyFlag( RMPF_ForceDynamic ) && !dynamicGeometry && m_umbraProxyId.IsValid() && scene->GetVisibilityExclusionMap() && m_renderMask )
	{
		RED_FATAL_ASSERT( m_localVisId == 0, "Proxy already registered in visibility filtering, why ?" );
		m_localVisId = scene->GetVisibilityExclusionMap()->RegisterProxy( this, m_umbraProxyId );
	}

	// Entity proxy related meshes requires group
	if ( HasCustomReferencePoint() )
	{
		const Uint64 groupHash = UmbraHelpers::CalculatePositionHash64( GetCustomReferencePoint() );
		CRenderProxyObjectGroup* group = m_scene->GetRenderingGroup( groupHash );
		if ( group )
		{
			m_renderGroup = group;

			// register high res meshes in the group (render all or nothing)
			if ( !CheckMeshProxyFlag( RMPF_IsEntityProxy ) )
			{
				m_renderGroup->RegisterProxy( this );
			}
		}
	}
}

//! Detach from scene
void CRenderProxy_Mesh::DetachFromScene( CRenderSceneEx* scene )
{
	// Pass to base class
	IRenderProxyDrawable::DetachFromScene( scene );

	// unregister from local visibility filtering
	if ( m_localVisId != 0 )
	{
		RED_FATAL_ASSERT( m_umbraProxyId.IsValid(), "We have local VisID but no GlobalVisID - how did we get registered ?" );
		scene->GetVisibilityExclusionMap()->UnregisterProxy( this, m_umbraProxyId, m_localVisId );
		m_localVisId = 0;
	}

	// unregister from group
	if ( m_renderGroup != nullptr )
	{
		m_renderGroup->UnregisterProxy( this );
		m_renderGroup = nullptr;
	}

	// Remove from entity group if we were in one
	IRenderEntityGroupProxy::DetachFromScene();

	ClearDynamicDecalChunks();
}

void CRenderProxy_Mesh::Relink( const Box& boundingBox, const Matrix& localToWorld )
{
	//we are casting static shadows invalidate the static lights
	if ( !IsDynamic() && ( CanCastShadows() || IsCastingShadowsFromLocalLightsOnly() ) && m_scene && m_scene->IsWorldScene() )
	{
		GetRenderer()->GetShadowManager()->InvalidateStaticLights( boundingBox );
		GetRenderer()->GetShadowManager()->InvalidateStaticLights( GetBoundingBox() );		
	}

	// Pass to base class
	IRenderProxyDrawable::Relink( boundingBox, localToWorld );
}

void CRenderProxy_Mesh::CreateDynamicDecalChunks( CRenderDynamicDecal* decal, DynamicDecalChunkList& outChunks )
{
	// If shadow fallback, or otherwise not visible, no point in creating chunks.
	if ( CheckMeshProxyFlag( RMPF_IsCharacterShadowFallback ) || !IsVisible() )
	{
		return;
	}

	// Create decal mesh for just the lowest LOD. The decals are already drawn with some allowance for geometry differences,
	// so this should be good enough, while minimizing the triangle count for the decals.
	Int32 lastNonEmptyLod = -1;
	for ( Int32 i = m_lodGroups.SizeInt() - 1; i >= 0; --i )
	{
		if ( m_lodGroups[ i ].GetChunkCount() > 0 )
		{
			lastNonEmptyLod = i;
			break;
		}
	}
	// Can't create a decal on an empty mesh!
	if ( lastNonEmptyLod < 0 )
	{
		WARN_RENDERER( TXT("No non-empty LOD found. Can't create a decal mesh") );
		return;
	}


	// Create a decal mesh for the chosen LOD.
	const auto& lodGroup = m_lodGroups[ lastNonEmptyLod ];
	CRenderDecalMesh* decalMesh = CRenderDecalMesh::Create( decal, lodGroup.GetChunks(), lodGroup.GetChunkCount(), m_localToWorld, m_skinningData, 0 );
	if ( decalMesh == nullptr )
	{
		WARN_RENDERER( TXT("Failed to create CRenderDecalMesh") );
		return;
	}


	// Now create a decal element for each chunk in the resulting decal mesh.
	const Uint32 numDecalChunks = decalMesh->GetNumChunks();
	outChunks.Reserve( outChunks.Size() + numDecalChunks );
	for ( Uint32 i = 0; i < numDecalChunks; ++i )
	{
		CRenderDynamicDecalChunk_Mesh* newChunk = new CRenderDynamicDecalChunk_Mesh( decal, this, decalMesh, i );
		outChunks.PushBack( newChunk );
	}

	decalMesh->Release();
}

#ifndef NO_EDITOR
void CRenderProxy_Mesh::UpdateMeshRenderParams(const SMeshRenderParams& params)
{
	// VERY FISHY LOGIC - to refactor!

	/*m_autoHideDistance *= params.m_meshRenderDist;

	const Uint32 lodGroupCount = m_lodGroups.Size();
	for( Uint32 i=0; i<lodGroupCount; ++i )
	{
		MeshLodGroup& lod = m_lodGroups[i];
		lod.m_lodDistanceSquared *= params.m_meshLODRenderDist;
	}*/
}
#endif

void CRenderProxy_Mesh::RefreshInstancingFlag()
{

	if ( m_replacedMaterialLodGroup )
	{
		for ( Uint8 i = 0, end = m_replacedMaterialLodGroup->GetChunkCount(); i < end; ++i )
		{
			m_replacedMaterialLodGroup->GetChunk(i)->RefreshInstancingFlag();
		}
	}

	for ( Uint8 lodIndex = 0, end = m_lodGroups.Size(); lodIndex < end; ++lodIndex )
	{
		const MeshLodGroup& group = m_lodGroups[ lodIndex ];
		const Bool includeNB = lodIndex == 0 && m_normalBlendLodGroup != nullptr;

		for ( Uint8 i = 0, end = group.GetChunkCount(); i < end; ++i )
		{
			if ( includeNB && m_normalBlendLodGroup->GetChunk(i) )
			{
				m_normalBlendLodGroup->GetChunk(i)->RefreshInstancingFlag();
			}

			group.GetChunk(i)->RefreshInstancingFlag();
		}
	}
}

void CRenderProxy_Mesh::SetClippingEllipseMatrix( const Matrix& localToEllipse )
{
	IRenderProxyDrawable::SetClippingEllipseMatrix( localToEllipse );

	RefreshInstancingFlag();	
}

void CRenderProxy_Mesh::ClearClippingEllipse()
{
	IRenderProxyDrawable::ClearClippingEllipse();

	RefreshInstancingFlag();
}

void CRenderProxy_Mesh::Prefetch( CRenderFramePrefetch* prefetch ) const
{
	const Float distanceSq = CalcCameraDistanceSq( prefetch->GetCameraPosition(), prefetch->GetCameraFovMultiplierUnclamped() );
	const Float texDistanceSq = AdjustCameraDistanceSqForTexturesNoMultiplier( distanceSq );

	if ( m_replacedMaterialLodGroup )
	{
		for ( Uint8 i = 0, end = m_replacedMaterialLodGroup->GetChunkCount(); i < end; ++i )
		{
			CRenderElement_MeshChunk* chunk = m_replacedMaterialLodGroup->GetChunk(i);
			prefetch->AddMaterialBind( chunk->GetMaterial(), chunk->GetMaterialParams(), texDistanceSq );
		}
	}
	else
	{
		// HACK : Without material replacement, we prefetch materials for the target LOD, plus neighboring LODs, so that if there's
		// any change we should be ready...
		// TODO : Better way to deal with LOD changes?
		// TODO : See if this is needed... investigate the problematic case a bit more, since it was still not right even with this


		// Calculate target lod index
		const Int8 targetLod = GetLodSelector().ComputeBestLOD( distanceSq, distanceSq );
		// No LOD -- probably beyond auto-hide distance.
		if ( ! DissolveHelpers::IsLodVisible(targetLod) )
		{
			return;
		}
		const Int8 rangeMin = Config::cvLoadLodRange.Get() ? Max< Int8 >( targetLod - 1, 0 ) : targetLod;
		const Int8 rangeMax = Config::cvLoadLodRange.Get() ? Min< Int8 >( targetLod + 1, m_lodGroups.SizeInt() - 1 ) : targetLod;
		for ( Int8 lodIndex = rangeMin; lodIndex <= rangeMax; ++lodIndex )
		{
			const MeshLodGroup& group = m_lodGroups[ lodIndex ];
			const Bool includeNB = lodIndex == 0 && m_normalBlendLodGroup != nullptr;

			for ( Uint8 i = 0, end = group.GetChunkCount(); i < end; ++i )
			{
				CRenderElement_MeshChunk* chunk;
				// For LOD 0, use a normal blend chunk if it's available.
				if ( includeNB && m_normalBlendLodGroup->GetChunk(i) )
				{
					chunk = m_normalBlendLodGroup->GetChunk(i);
				}
				else
				{
					chunk = group.GetChunk(i);
				}

				prefetch->AddMaterialBind( chunk->GetMaterial(), chunk->GetMaterialParams(), texDistanceSq );
			}
		}
	}
}
