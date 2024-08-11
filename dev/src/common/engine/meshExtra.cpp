/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mesh.h"
#include "meshDataBuilder.h"

#include "../redSystem/numericalLimits.h"
#include "../../common/core/gatheredResource.h"

#include "../../../../bin/shaders/include/globalConstantsVS.fx"
#include "navigationObstacle.h"
#include "renderCommands.h"
#include "../core/depot.h"
#include "simplygonHelpers.h"
#include "../core/feedback.h"
#include "../core/configVar.h"
#include "../engine/renderVertices.h"
#include "../engine/materialGraph.h"

#include "collisionMesh.h"
#include "skeletonProvider.h"
#include "renderProxy.h"
#include "materialDefinition.h"

// vcache_optimizer generates a couple of warnings, so just temporarily disable those.
#pragma warning( push )
#pragma warning( disable:4512 4244 )
#include "../../../external/vcache_optimizer/vcache_optimizer/vcache_optimizer.hpp"
#pragma warning( pop )

IMPLEMENT_RTTI_ENUM( EMeshShadowImportanceBias );

//////////////////////////////////////////////////////////////////////////

CGatheredResource resDefaultMeshMaterial( TXT("engine\\materials\\defaults\\mesh.w2mg"), RGF_Startup );

//////////////////////////////////////////////////////////////////////////

namespace MeshUtilities
{
	Uint32 GetSkinningMatricesAndBoxMS( const ISkeletonDataProvider* provider, const TDynArray< Int16 >& boneMapping, const Matrix* rigMatrices, const Float* vertexEpsilons, void* skinningMatrices, ESkinningDataMatrixType outMatricesType, Box& outBoxMS )
	{
		// Get the bones
		ISkeletonDataProvider::SBonesData bonesData( outBoxMS );
		bonesData.m_boneIndices = boneMapping.TypedData();
		bonesData.m_numBones = boneMapping.Size();
		bonesData.m_rigMatrices = rigMatrices;
		bonesData.m_vertexEpsilons = vertexEpsilons;
		bonesData.m_outMatricesArray = skinningMatrices;
		bonesData.m_outMatricesType = outMatricesType;
		provider->GetBoneMatricesAndBoundingBoxModelSpace( bonesData );

		// Return number of skinning matrices generated
		return boneMapping.Size();
	}

	Bool UpdateTransformAndSkinningDataMS( const ISkeletonDataProvider* provider, const CMesh* mesh, const TDynArray< Int16 >& boneMapping, const Matrix& localToWorld, IRenderProxy* proxy, IRenderSkinningData* skinningData, Box& boxMS )
	{
		if ( provider && mesh && mesh->GetBoneCount() )
		{
			if ( GRender && proxy )
			{
				// Get mesh skeleton bones
				const Matrix* rigMatrices = mesh->GetBoneRigMatrices();
				const Float* vertexEpsilons = mesh->GetBoneVertexEpsilons();

				if ( skinningData )
				{
					void* skinningMatrices = skinningData->GetWriteData();

					// We can fill
					boxMS.Clear();
					GetSkinningMatricesAndBoxMS( provider, boneMapping, rigMatrices, vertexEpsilons, skinningMatrices, skinningData->GetMatrixType(), boxMS );

					const Box boxWS = localToWorld.TransformBox( boxMS );

					// Send new bounding box
					RenderProxyUpdateInfo updateInfo;
					updateInfo.m_localToWorld = &localToWorld;
					updateInfo.m_boundingBox = &boxWS;

					// Send new data to render thread
					( new CRenderCommand_UpdateSkinningDataAndRelink( proxy, skinningData, updateInfo ) )->Commit();

					return true;
				}
			}
		}

		return false;
	}

	Float CalcFovDistanceMultiplierNoClamp( Float fov )
	{
		// assuming that default fov is 60 here
		const Float tanOfDefaultFOV = 0.57735f; // tan( 0.5f * DEG2RAD( 60.f ) ); //...0.700207531f
		const Float fovFactor = tan( DEG2RAD(0.5f * fov) ) / tanOfDefaultFOV;
		const Float ret = fovFactor * fovFactor;
		return ret;
	}

	Float CalcFovDistanceMultiplier( Float fov )
	{
		// Limit to 1.0f, so there's no effect for small FOVs. It only affects when FOV is made wider than default.
		const Float ret = CalcFovDistanceMultiplierNoClamp( fov );
		return Max< Float >( 1.0f, ret );
	}

}

//////////////////////////////////////////////////////////////////////////

// Optimization stuff
struct MeshOpt_Triangle
{
	Uint16 vertices[3];

	explicit MeshOpt_Triangle() {}

	explicit MeshOpt_Triangle( Uint16 i1, Uint16 i2, Uint16 i3)
	{
		vertices[0] = i1;
		vertices[1] = i2;
		vertices[2] = i3;
	}

	Uint16 operator[]( Uint32 index ) const
	{
		return vertices[index];
	}
};

struct MeshOpt_Traits
{
	// submesh id will always be 0, since we optimize each submesh separately
	typedef Uint16 submesh_id_t;
	// It's just the index of the vertex in the original data.
	typedef Uint16 vertex_t;
	typedef MeshOpt_Triangle triangle_t;
	typedef Uint16 vertex_index_t;
	typedef Uint32 triangle_index_t;
};

struct MeshOpt_Mesh
{
	typedef TDynArray< MeshOpt_Traits::triangle_t >	triangles_t;
	typedef TDynArray< MeshOpt_Traits::vertex_index_t >	vertices_t;

	triangles_t triangles;
	vertices_t vertices;
};

MeshOpt_Traits::triangle_t create_new_triangle( MeshOpt_Mesh& mesh, MeshOpt_Traits::vertex_index_t vtx1, MeshOpt_Traits::vertex_index_t vtx2, MeshOpt_Traits::vertex_index_t vtx3 )
{
	return MeshOpt_Triangle( vtx1, vtx2, vtx3 );
}

std::size_t get_num_triangles( const MeshOpt_Mesh& mesh, MeshOpt_Traits::submesh_id_t /*submesh_id*/ )
{
	return mesh.triangles.Size();
}

std::size_t get_num_vertices( const MeshOpt_Mesh& mesh, MeshOpt_Traits::submesh_id_t /*submesh_id*/ )
{
	return mesh.vertices.Size();
}

MeshOpt_Traits::triangle_t get_triangle( const MeshOpt_Mesh& mesh, MeshOpt_Traits::submesh_id_t /*submesh_id*/, MeshOpt_Traits::triangle_index_t index )
{
	return mesh.triangles[index];
}

MeshOpt_Traits::vertex_t get_vertex( const MeshOpt_Mesh& mesh, MeshOpt_Traits::submesh_id_t /*submesh_id*/, MeshOpt_Traits::vertex_index_t index )
{
	return mesh.vertices[index];
}

void set_triangle( MeshOpt_Mesh& mesh, MeshOpt_Traits::submesh_id_t /*submesh_id*/, MeshOpt_Traits::triangle_index_t index, MeshOpt_Traits::triangle_t new_triangle )
{
	mesh.triangles[index] = new_triangle;
}

void set_vertex( MeshOpt_Mesh& mesh, MeshOpt_Traits::submesh_id_t /*submesh_id*/, MeshOpt_Traits::vertex_index_t index, MeshOpt_Traits::vertex_t new_vertex )
{
	mesh.vertices[index] = new_vertex;
}

#ifdef TEST_OPTIMIZATION

static Uint32 TestMeshChunkInd( const Uint16* indices, Uint32 numIndices )
{
	Uint32 numMisses = 0;

	const Uint32 fakeCacheSize = 32;

	TStaticArray< Uint16, fakeCacheSize > fakeCache;

	for ( Uint32 i = 0; i < numIndices; ++i )
	{
		Uint16 idx = indices[i];
		if ( !fakeCache.Exist( idx ) )
		{
			if ( fakeCache.Size() == fakeCacheSize )
			{
				fakeCache.RemoveAt( 0 );
			}
			++numMisses;
		}
		else
		{
			fakeCache.Remove( idx );
		}
		fakeCache.PushBack( idx );
	}

	return numMisses;
}

#endif // TEST_OPTIMIZATION

//////////////////////////////////////////////////////////////////////////

#ifndef NO_RESOURCE_IMPORT

Uint32 CMesh::CountLODTriangles( Uint32 level ) const
{
	Uint32 numTriangles = 0;

	const auto& chunks = GetChunks();

	// Count chunks in given LOD
	if ( level < m_lodLevelInfo.Size() )
	{
		const LODLevel& lodLevel = m_lodLevelInfo[ level ];
		for ( Uint32 i=0; i<lodLevel.m_chunks.Size(); i++ )
		{
			const auto& chunk = chunks[ lodLevel.m_chunks[i] ];
			numTriangles += chunk.m_numIndices / 3;
		}
	}

	// Return triangle count
	return numTriangles;
}

Uint32 CMesh::CountLODVertices( Uint32 level ) const
{
	Uint32 numVertices = 0;

	const auto& chunks = GetChunks();

	// Count chunks in given LOD
	if ( level < m_lodLevelInfo.Size() )
	{
		const LODLevel& lodLevel = m_lodLevelInfo[ level ];
		for ( Uint32 i=0; i<lodLevel.m_chunks.Size(); i++ )
		{
			const auto& chunk = chunks[ lodLevel.m_chunks[i] ];
			numVertices += chunk.m_numVertices; 
		}
	}

	// Return vertex count
	return numVertices;
}

Uint32 CMesh::CountLODBones( Uint32 level ) const
{
	CMeshData data( this );
	const auto& chunks = data.GetChunks(); 
	
	// Count bones in given LOD
	if ( !chunks.Empty() && level < m_lodLevelInfo.Size() )
	{
		THashSet< Uint8 > set( GetBoneCount() );

		const LODLevel& lodLevel = m_lodLevelInfo[ level ];
		for ( Uint32 i=0; i<lodLevel.m_chunks.Size(); i++ )
		{
			const auto& chunk = chunks[ lodLevel.m_chunks[i] ];

			for ( Uint32 j=0; j<chunk.m_numVertices; ++j )
			{
				const auto& vertex = chunk.m_vertices[ j ];
				for ( Uint32 k=0; k<4; ++k )
				{
					const Uint8 boneIdx = vertex.m_indices[ k ];
					set.Insert( boneIdx );
				}
			}
		}

		return set.Size();
	}

	return 0;
}

Uint32 CMesh::CountLODChunks( Uint32 level ) const
{
	// Count chunks in given LOD
	if ( level < m_lodLevelInfo.Size() )
	{
		const LODLevel& lodLevel = m_lodLevelInfo[ level ];
		return lodLevel.m_chunks.Size();
	}

	// No valid LOD index
	return 0;
}

Uint32 CMesh::CountLODMaterials( Uint32 level ) const
{
	const auto& chunks = GetChunks(); 

	// Count chunks in given LOD
	if ( level < m_lodLevelInfo.Size() )
	{		
		TDynArray< Uint32 > uniqueMaterials;

		const LODLevel& lodLevel = m_lodLevelInfo[ level ];
		for ( Uint32 i=0; i<lodLevel.m_chunks.Size(); i++ )
		{
			const auto& chunk = chunks[ lodLevel.m_chunks[i] ];
			uniqueMaterials.PushBackUnique( chunk.m_materialID );
		}

		return uniqueMaterials.Size();
	}

	// No valid LOD index
	return 0;
}

Uint32 CMesh::EstimateMemoryUsageCPU( const Int32 lodLevelIndex ) const
{
	Uint32 size = 0;

	size += sizeof(CMesh);
	size += (Uint32)m_usedBones.DataSize();

	const auto& chunks = GetChunks(); 

	if ( !chunks.Empty() )
	{
		if ( lodLevelIndex >= 0 && lodLevelIndex < (Int32)m_lodLevelInfo.Size() )
		{
			const LODLevel& lodLevel = m_lodLevelInfo[ lodLevelIndex ];
			for ( Uint32 i=0; i<lodLevel.m_chunks.Size(); i++ )
			{
				const auto& chunk = chunks[ lodLevel.m_chunks[i] ];
				size += chunk.m_numBonesPerVertex * sizeof(Uint16);
			}
		}
	}

	return size;
}

Uint32 CMesh::EstimateMemoryUsageGPU( const Int32 lodLevelIndex ) const
{
	Uint32 size = 0;

	if ( lodLevelIndex < 0 )
		return 0;

	const auto& chunks = GetChunks(); 

	// Count chunks in given LOD
	if ( lodLevelIndex < (Int32)m_lodLevelInfo.Size() )
	{
		const LODLevel& lodLevel = m_lodLevelInfo[ lodLevelIndex ];
		for ( Uint32 i=0; i<lodLevel.m_chunks.Size(); i++ )
		{
			const auto& chunk = chunks[ lodLevel.m_chunks[i] ];

			// TODO: how to get approximate vertex size per type ?
			const Uint32 vertexSizePerType = sizeof(SMeshVertex);
			const Uint32 indexSizePerType = sizeof(Uint16);

			size += chunk.m_numVertices * vertexSizePerType;
			size += chunk.m_numIndices * indexSizePerType;
		}
	}

	return size;
}

Bool CMesh::UpdateLODSettings( Uint32 level, const SMeshTypeResourceLODLevel& lodSettings )
{
	// We can set distances only for valid LOD levels
	if ( level < m_lodLevelInfo.Size() )
	{
		LODLevel& lodLevel = m_lodLevelInfo[ level ];
		lodLevel.m_meshTypeLOD = lodSettings;
		return true;
	}

	// Not set
	return false;
}

void CMesh::RebuildMaterialMap()
{
	// Clamp if too many
	const Uint32 numMaterials = m_materials.Size();
	if ( m_materialNames.Size() > numMaterials )
	{
		m_materialNames.Resize( numMaterials );
	}

	// Add if too few
	while ( m_materialNames.Size() < numMaterials )
	{
		String materialName = String::Printf( TXT("Material%i"), m_materialNames.Size() );
		m_materialNames.PushBack( materialName );
	}
}

#ifndef NO_RESOURCE_IMPORT

Bool CMesh::ForceRenderMaskOnAllChunks( const Uint8 newRenderMask, const Bool recreateRenderingData /*= true*/ )
{
	// change chunks
	Bool dataModified = false;
	for ( auto& chunk : m_chunks )
	{
		if ( chunk.m_renderMask != newRenderMask )
		{
			if ( !MarkModified() )
				return false;

			chunk.m_renderMask = newRenderMask;
			dataModified = true;
		}
	}

	// recreate rendering mesh
	if ( dataModified && recreateRenderingData )
	{
		ClearUnpackedData();

		ReleaseRenderResource();
		CreateRenderResource();

		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}

	return true;
}

Bool CMesh::SetRenderMaskOnAllChunks( const Uint8 bitsToSet, const Bool recreateRenderingData /*= true*/ )
{
	// change chunks
	Bool dataModified = false;
	for ( auto& chunk : m_chunks )
	{
		const auto newMask = chunk.m_renderMask | bitsToSet;
		if ( chunk.m_renderMask != newMask)
		{
			if ( !MarkModified() )
				return false;

			chunk.m_renderMask = newMask;
			dataModified = true;
		}
	}

	// recreate rendering mesh
	if ( dataModified && recreateRenderingData )
	{
		ClearUnpackedData();

		ReleaseRenderResource();
		CreateRenderResource();

		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}

	return true;
}

Bool CMesh::ClearRenderMaskOnAllChunks( const Uint8 bitsToClear, const Bool recreateRenderingData /*= true*/ )
{
	Bool dataModified = false;

	// change chunks
	for ( auto& chunk : m_chunks )
	{
		const auto newMask = chunk.m_renderMask & ~bitsToClear;
		if ( chunk.m_renderMask != newMask)
		{
			chunk.m_renderMask = newMask;
			dataModified = true;
		}
	}

	// recreate rendering mesh
	if ( dataModified && recreateRenderingData )
	{
		ClearUnpackedData();

		ReleaseRenderResource();
		CreateRenderResource();

		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}

	return true;
}

Bool CMesh::ForceRenderMaskOnChunk( const Uint32 chunkIndex, const Uint8 newRenderMask, const Bool recreateRenderingData /*= true*/ )
{
	if ( chunkIndex >= m_chunks.Size() )
		return false;

	auto& chunk = m_chunks[ chunkIndex ];
	if ( chunk.m_renderMask == newRenderMask )
		return true;

	if ( !MarkModified() )
		return false;

	chunk.m_renderMask = newRenderMask;

	if ( recreateRenderingData )
	{
		ClearUnpackedData();

		ReleaseRenderResource();
		CreateRenderResource();

		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}

	return true;
}

Bool CMesh::ForceShadowMeshFlagOnChunk( const Uint32 chunkIndex, const Bool flag )
{
	if ( chunkIndex >= m_chunks.Size() )
		return false;

	auto& chunk = m_chunks[ chunkIndex ];
	if ( chunk.m_useForShadowmesh == flag )
		return true;

	if ( !MarkModified() )
		return false;

	chunk.m_useForShadowmesh = flag;
	return true;
}

#endif

void CMesh::RecomputeDefaultRenderMask()
{
	// if it's a normal mesh it appears in cascad1&2 + all of the local lights
	// automatic proxies are not rendered into shadows any more - this is handled by the merged shadow mesh
	// by default DO NOT allow stuff to be directly rendered into cascade 3 - this is reserved for special crap
	Uint8 renderMask = 0;
	if ( m_entityProxy )
	{
		renderMask = MCR_Scene;
	}
	else
	{
		renderMask = MCR_Cascade1 | MCR_Cascade2 | MCR_LocalShadows | MCR_Scene;
	}

	for ( auto& chunk : m_chunks )
	{
		Uint8 chunkRenderMask = renderMask;

		// transparency check
		if ( chunk.m_materialID < m_materials.Size() )
		{
			IMaterial* material = m_materials[ chunk.m_materialID ];
			if ( material )
			{
				CMaterialGraph* base = Cast< CMaterialGraph >( material->GetBaseMaterial() );
				if ( base && base->GetRenderingBlendMode() != RBM_None )
				{
					// it's a transparent material, don't render it into the shadows
					chunkRenderMask &= ~(MCR_Cascade1 | MCR_Cascade2 | MCR_LocalShadows);
				}
			}
		}

		// volume material check
		if ( chunk.m_materialID < m_materialNames.Size() )
		{
			const String& name = m_materialNames[chunk.m_materialID];
			if ( name == TXT("volume") )
			{
				// it's a volume material, don't render it into the shadows
				chunkRenderMask &= ~(MCR_Cascade1 | MCR_Cascade2 | MCR_LocalShadows);
			}
		}

		chunk.m_renderMask = chunkRenderMask;
	}
}

Uint32 CMesh::MapMaterial( const String& materialName )
{
	ASSERT( m_materialNames.Size() == m_materials.Size() );

	// Try to use existing material 
	for ( Uint32 i=0; i<m_materials.Size(); i++ )
	{
		const String& existingName = m_materialNames[i];
		if ( existingName == materialName )
		{
			return i;
		}
	}

	// Try to auto find specialized material
	IMaterial* foundMaterial  = NULL;
	{
		// Search for matching shader
		TDynArray< CDiskFile* > materialsFound;
		
		const String materialInstanceFileName = materialName + TXT(".w2mi");
		const String materialFileName = materialName + TXT(".w2mg");
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
		GDepot->Search( materialInstanceFileName, materialsFound );
		if ( materialsFound.Empty() )
		{
			GDepot->Search( materialFileName, materialsFound );
		}
#endif
		const String& unwantedExt( TXT("link") );
		materialsFound.Erase(
			RemoveIf( materialsFound.Begin(), materialsFound.End(), [&unwantedExt]( CDiskFile* file ){ return CFilePath( file->GetDepotPath() ).GetExtension() == unwantedExt; } ),
			materialsFound.End()
			);
		// We have found something
		if ( materialsFound.Size() )
		{
			// There are more than one matching material
			if ( materialsFound.Size() > 1 )
			{
				WARN_ENGINE( TXT("More than 1 material definitions found for material \"%s\". The first match that has been found will be used."), materialFileName.AsChar() );
			}

			// Get material from file
			materialsFound[0]->Load();
			foundMaterial = Cast< IMaterial >( materialsFound[0]->GetResource() );
		}

		// Material not found, use default one
		if ( !foundMaterial )
		{
			foundMaterial = resDefaultMeshMaterial.LoadAndGet< IMaterial >();
			ASSERT( foundMaterial );
		}
	}

	// Add new one
	const Uint32 materialIndex = m_materialNames.Size();
	m_materialNames.PushBack( materialName );
	m_materials.PushBack( foundMaterial );

	// Return mapped material index
	return materialIndex;
}

Bool CMesh::ValidateMeshData( const TDynArray< SMeshChunk >& chunks, const TLODLevelArray& lodData, const Uint32 numMaterials )
{
	// Validate data
	Bool validData = true;

	// validate chunk indices
	for ( Uint32 i=0; i<lodData.Size(); ++i )
	{
		for ( Uint16 index : lodData[i].m_chunks )
		{
			if ( index >= chunks.Size() )
			{
				ERR_ENGINE( TXT("Chunk index %d in LOD %d lies outside allowed range (%d)"), index, i, chunks.Size() );
				validData = false;
			}
		}
	}

	// validate material indices
	for ( Uint32 i=0; i<chunks.Size(); ++i )
	{
		if ( chunks[i].m_materialID >= numMaterials )
		{
			ERR_ENGINE( TXT("Chunk %d uses material %d that lies outside allowed range (%d)"), i, chunks[i].m_materialID, numMaterials );
			validData = false;
		}
	}

	// validate indices in chunks
	for ( Uint32 i=0; i<chunks.Size(); ++i )
	{
		const Uint16* index = chunks[i].m_indices.TypedData();
		const Uint32 numIndices = chunks[i].m_indices.Size();
		const Uint32 maxIndex = chunks[i].m_vertices.Size();
		if ( (numIndices % 3) != 0)
		{
			ERR_ENGINE( TXT("Chunk %d has invalid number of indices (%d)"), i, numIndices );
			validData = false;
		}

		for ( Uint32 j=0; j<numIndices; ++j, ++index )
		{
			if ( *index >= maxIndex )
			{
				ERR_ENGINE( TXT("Chunk %d has invalid vertex index %d (max %d) at position %d"), i, *index, maxIndex, j );
				validData = false;
				break;
			}
		}
	}

	// do not set if invalid
	if ( !validData )
	{
		ERR_ENGINE( TXT("Unable to flush mesh content because data is invalid") );
		return false;
	}

	// valid
	return true;
}

void CMesh::RegenerateMeshData()
{
	if ( MarkModified() )
	{
		CMeshData data( this );
		data.FlushChanges( true );
	}
}

Bool CMesh::OptimizeMeshChunk( const SMeshChunk& sourceChunk, TDynArray< Uint16 >& optimizedIndices, TDynArray< SMeshVertex >& optimizedVertices )
{
	optimizedVertices.Clear();
	optimizedIndices.Clear();

	// Run the chunk through the optimizer.
	MeshOpt_Mesh optMesh;
	optMesh.vertices.Resize( sourceChunk.m_vertices.Size() );
	for ( Uint32 j = 0; j < sourceChunk.m_vertices.Size(); ++j )
	{
		optMesh.vertices[ j ] = (Uint16)j;
	}

	optMesh.triangles.Resize( sourceChunk.m_indices.Size() / 3 );
	for ( Uint32 j = 0; j < sourceChunk.m_indices.Size(); j += 3 )
	{
		optMesh.triangles[ j / 3 ] = MeshOpt_Triangle( sourceChunk.m_indices[ j ], sourceChunk.m_indices[ j + 1 ], sourceChunk.m_indices[ j + 2 ] );
	}

	vcache_optimizer::vcache_optimizer < MeshOpt_Mesh, Float, MeshOpt_Traits > optimizer;
	optimizer( optMesh, 0, true );

	// Store out the reordered vertex and index buffers.
	optimizedVertices.Resize( sourceChunk.m_vertices.Size() );
	for ( Uint32 j = 0; j < sourceChunk.m_vertices.Size(); ++j )
	{
		optimizedVertices[j] = sourceChunk.m_vertices[ optMesh.vertices[j] ];
	}

	optimizedIndices.Resize( sourceChunk.m_indices.Size() );
	for ( Uint32 j = 0; j < sourceChunk.m_indices.Size(); j += 3 )
	{
		optimizedIndices[j+0] = optMesh.triangles[j/3][0];
		optimizedIndices[j+1] = optMesh.triangles[j/3][1];
		optimizedIndices[j+2] = optMesh.triangles[j/3][2];
	}
	return true;
}

Bool CMesh::SetMeshData( const TDynArray< SMeshChunk >& chunks, const TLODLevelArray& lodData, const Bool recreateRenderingData /*= true*/, const Bool optimizeMeshData /*= true*/ )
{
	// validate the data
	if ( !ValidateMeshData( chunks, lodData, m_materials.Size() ) )
		return false;

	// cleanup cached unpacked data (we are going to replace it anyway)
	ClearUnpackedData();

	// calculate memory needed for all of the vertices and indices
	Uint32 numTotalVertices = 0;
	Uint32 numTotalIndices = 0;
	for ( Uint32 i=0; i<chunks.Size(); ++i )
	{
		numTotalIndices += chunks[i].m_indices.Size();
		numTotalVertices += chunks[i].m_vertices.Size();
	}

	// stats (just in case of crash)
	LOG_ENGINE( TXT("Total mesh data: %d vertices, %d indices, %d chunks"), numTotalVertices, numTotalIndices, chunks.Size() );

	// setup new data
	TDynArray< SMeshVertex > optimizedVertices, finalVertices;
	finalVertices.Resize( numTotalVertices );
	TDynArray< Uint16 > optimizedIndices, finalIndices;
	finalIndices.Resize( numTotalIndices );

	// copy data to internal chunks
	Uint32 currentVertexOffset = 0;
	Uint32 currentIndexOffset = 0;
	m_chunks.Resize( chunks.Size() );
	for ( Uint32 i=0; i<chunks.Size(); ++i )
	{
		SMeshChunkPacked& destChunk = m_chunks[i];
		const SMeshChunk& srcChunk = chunks[i];

		// sort vertices inside skinning chunks in order of decreasing weights
		SortSkinningForChunk( const_cast< SMeshChunk& >( srcChunk ) );

		// optimize chunk vertices
		if ( optimizeMeshData )
		{
			OptimizeMeshChunk( srcChunk, optimizedIndices, optimizedVertices );
		}

		// copy chunk data
		destChunk.m_vertexType = srcChunk.m_vertexType;
		destChunk.m_materialID = srcChunk.m_materialID;
		destChunk.m_numBonesPerVertex = srcChunk.m_numBonesPerVertex;
		destChunk.m_renderMask = srcChunk.m_renderMask;

		// failsafe
		if ( destChunk.m_renderMask == 0 )
		{
			destChunk.m_renderMask = MCR_Scene | MCR_Cascade1 | MCR_Cascade2 | MCR_LocalShadows;
		}

		// copy vertices
		if ( optimizeMeshData )
		{
			destChunk.m_firstVertex = currentVertexOffset;
			destChunk.m_numVertices = optimizedVertices.Size();
			Red::MemoryCopy( &finalVertices[ currentVertexOffset ], optimizedVertices.Data(), optimizedVertices.DataSize() );
			currentVertexOffset += optimizedVertices.Size();
		}
		else
		{
			destChunk.m_firstVertex = currentVertexOffset;
			destChunk.m_numVertices = srcChunk.m_vertices.Size();
			Red::MemoryCopy( &finalVertices[ currentVertexOffset ], srcChunk.m_vertices.Data(), srcChunk.m_vertices.DataSize() );
			currentVertexOffset += srcChunk.m_vertices.Size();
		}

		// copy indices
		if ( optimizeMeshData )
		{
			destChunk.m_firstIndex = currentIndexOffset;
			destChunk.m_numIndices = optimizedIndices.Size();
			Red::MemoryCopy( &finalIndices[ currentIndexOffset ], optimizedIndices.Data(), optimizedIndices.DataSize() );
			currentIndexOffset += optimizedIndices.Size();
		}
		else
		{
			destChunk.m_firstIndex = currentIndexOffset;
			destChunk.m_numIndices = srcChunk.m_indices.Size();
			Red::MemoryCopy( &finalIndices[ currentIndexOffset ], srcChunk.m_indices.Data(), srcChunk.m_indices.DataSize() );
			currentIndexOffset += srcChunk.m_indices.Size();
		}
	}

	// setup new data
	m_rawVertices.SetBufferContent( finalVertices.Data(), (const Uint32)finalVertices.DataSize() );
	m_rawIndices.SetBufferContent( finalIndices.Data(), (const Uint32)finalIndices.DataSize() );
	m_finalIndices = finalIndices;

	// copy LODs
	m_lodLevelInfo = lodData;

	// update static flag on mesh
	{
		m_isStatic = false;
		for ( Uint32 i=0; i<m_lodLevelInfo.Size() && !m_isStatic; ++i )
		{
			const LODLevel& lod = m_lodLevelInfo[i];
			for ( Uint32 j = 0; j < lod.m_chunks.Size(); ++j )
			{
				const SMeshChunk& c = chunks[ lod.m_chunks[j] ];
				if ( c.m_vertexType == MVT_StaticMesh )
				{
					m_isStatic = true;
					break;
				}
			}
		}
	}

	// update bounding box
	{
		m_boundingBox.Clear();

		// Add all vertices
		for ( Uint32 i=0; i<chunks.Size(); ++i )
		{
			const SMeshChunk& ch = chunks[i];
			for ( Uint32 j=0; j<ch.m_vertices.Size(); j++ )
			{
				const SMeshVertex& v = ch.m_vertices[j];
				m_boundingBox.AddPoint( Vector( v.m_position[0], v.m_position[1], v.m_position[2] ) );
			}
		}
	}

	// recalculate the bone distance epsilons
	{
		// Reset vertex epsilons for bones
		TDynArray< Vector > bonePositions;
		for ( Uint32 i=0; i<m_boneRigMatrices.Size(); i++ )
		{
			const Matrix& rigMatrix = m_boneRigMatrices[i];

			// Bone center is encoded in the bone rig matrix
			Matrix invMatrix = rigMatrix.FullInverted();
			bonePositions.PushBack( invMatrix.GetTranslation() );
		}

		// Update bones vertex epsilons
		for ( Uint32 i=0; i<chunks.Size(); i++ )
		{
			const SMeshChunk& chunk = chunks[i];

			// Process only skinned chunks
			if ( IsSkinnedMeshVertexType( chunk.m_vertexType ) )
			{
				// Process vertices from chunk
				for ( Uint32 j=0; j<chunk.m_vertices.Size(); j++ )
				{
					const SMeshVertex& vertex = chunk.m_vertices[j];

					// Calculate distance to each used bones
					for ( Uint32 k=0; k<4; k++ )
					{
						if ( vertex.m_weights[k] > 0.0f && vertex.m_indices[k] < bonePositions.Size() )
						{
							const Uint32 boneIndex = vertex.m_indices[k];
							const Vector vertexPosition( vertex.m_position[0], vertex.m_position[1], vertex.m_position[2], 0.0f );
							Float distToBone = vertexPosition.DistanceTo( bonePositions[ boneIndex ] );

							// Grow by 20%
							distToBone *= 1.2f;

							// Get max
							m_boneVertexEpsilons[ boneIndex ] = Max< Float >( m_boneVertexEpsilons[ boneIndex ], distToBone );
						}
					}
				}
			}
		}
	}

	// update list of used bones
	{
		m_usedBones.Clear();
		for ( Uint32 chunk_i=0; chunk_i<chunks.Size(); ++chunk_i )
		{
			const auto& chunk = chunks[chunk_i];
			const bool chunkSkinned = IsSkinnedMeshVertexType( chunk.m_vertexType );

			// Update used bones
			if ( chunkSkinned )
			{
				for	( Uint32 j=0; j<chunk.m_vertices.Size(); j++ )
				{
					const SMeshVertex& v = chunk.m_vertices[j];
					for ( Uint32 k=0; k<4; k++ )
					{
						if ( v.m_weights[k] > 0 )
						{
							Uint32 boneIndex = v.m_indices[k];
							m_usedBones.PushBackUnique( boneIndex );
						}
					}
				}
			}
		}
	}

	// repair rig mapping
	{
		for ( Uint32 i=0; i<m_boneRigMatrices.Size(); i++ )
		{
			Matrix& rigMatrix = m_boneRigMatrices[i];
			rigMatrix.V[0].W = 0.0f;
			rigMatrix.V[1].W = 0.0f;
			rigMatrix.V[2].W = 0.0f;
			rigMatrix.V[3].W = 1.0f;
		}
	}

	// recreate rendering resources
	if ( recreateRenderingData )
	{
		ReleaseRenderResource();
	}

	// invalidate skeleton caches
	m_skeletonMappingCache.Reset();

	// done
	return true;
}


CMesh* CMesh::Create( const FactoryInfo& data )
{
	CMesh* obj = data.CreateResource();
	ASSERT( !data.m_reuse || obj == data.m_reuse );

	obj->m_authorName			= data.m_authorName;
	obj->m_baseResourceFilePath	= data.m_baseResourceFilePath;
	obj->m_entityProxy			= data.m_entityProxy;

	if ( !data.m_reuseMesh )
	{
		// Basic data
		obj->m_boneNames			= data.m_boneNames;
		obj->m_boneRigMatrices		= data.m_boneRigMatrices;
		obj->m_boneVertexEpsilons	= data.m_boneVertexEpsilons;

		// Create materials
		TDynArray< Uint32 > materialRemappingTable( data.m_materialNames.Size() );
		for ( Uint32 i=0; i<data.m_materialNames.Size(); i++ )
		{
			// Map imported material 
			const String& newMaterialName = data.m_materialNames[i];
			Uint32 materialIndex = obj->MapMaterial( newMaterialName );

			// Remember remapped index
			materialRemappingTable[ i ] = materialIndex;
		}

		// Remap imported chunk materials
		for ( Uint32 i=0; i<data.m_chunks.Size(); i++ )
		{
			SMeshChunk& chunk = const_cast< SMeshChunk& >( data.m_chunks[i] );
			ASSERT( chunk.m_materialID < materialRemappingTable.Size() );
			chunk.m_materialID = materialRemappingTable[ chunk.m_materialID ];
			ASSERT( chunk.m_materialID < obj->GetMaterials().Size() );
		}

		// Create mesh data, must be last (we need mapped materials first)
		if ( !obj->SetMeshData( data.m_chunks, data.m_lodLevels, false ) )
		{
			return false;
		}
	}

	if ( !data.m_reuseVolumes )
	{
		obj->RemoveCollision();

		// Build or import collision
		if ( data.m_buildConvexCollision || ( data.m_importCollision && data.m_collisionMeshes.Empty() ) )
		{
			// Build convex ( forced )
			obj->AddConvexCollision();
		}
		else if ( data.m_importCollision && !data.m_collisionMeshes.Empty() )
		{
			// Create new collision
			CCollisionMesh* collisionMesh = ::CreateObject< CCollisionMesh>( obj );
			ASSERT( collisionMesh );

			// Add parts
			for ( Uint32 i=0; i<data.m_collisionMeshes.Size(); i++ )
			{
				const CollisionMesh& partMesh = data.m_collisionMeshes[i];
				if ( !partMesh.m_vertices.Empty() )
				{
					const Vector* vertices = partMesh.m_vertices.TypedData();
					if ( partMesh.m_type == CMesh::CollisionMesh::ECMT_Convex )
					{
						collisionMesh->AddConvex( Matrix::IDENTITY, vertices, partMesh.m_vertices.Size(), partMesh.m_physicalMaterials[ 0 ] );
					}
					else if ( partMesh.m_type == CMesh::CollisionMesh::ECMT_Box )
					{
						collisionMesh->AddBox( Matrix( vertices[ 0 ], vertices[ 1 ], vertices[ 2 ], vertices[ 3 ] ), vertices[ 4 ], partMesh.m_physicalMaterials[ 0 ] );
					}
					else if ( partMesh.m_type == CMesh::CollisionMesh::ECMT_Sphere )
					{
						collisionMesh->AddSphere( Matrix( vertices[ 0 ], vertices[ 1 ], vertices[ 2 ], vertices[ 3 ] ), vertices[ 4 ].X, partMesh.m_physicalMaterials[ 0 ] );
					}
					else if ( partMesh.m_type == CMesh::CollisionMesh::ECMT_Capsule )
					{
						collisionMesh->AddCapsule( Matrix( vertices[ 0 ], vertices[ 1 ], vertices[ 2 ], vertices[ 3 ] ), vertices[ 4 ].X, vertices[ 4 ].Z, partMesh.m_physicalMaterials[ 0 ] );
					}
					else if ( !partMesh.m_indices.Empty() )
					{
						const Uint32* indices = partMesh.m_indices.TypedData();
						if( partMesh.m_physicalMaterialIndexes.Empty() )
						{
							collisionMesh->AddTriMesh( Matrix::IDENTITY, vertices, partMesh.m_vertices.Size(), indices, partMesh.m_indices.Size() / 3 );
						}
						else
						{
							collisionMesh->AddTriMesh( Matrix::IDENTITY, vertices, partMesh.m_vertices.Size(), indices, partMesh.m_indices.Size() / 3, partMesh.m_physicalMaterialIndexes, partMesh.m_physicalMaterials );
						}
					}
				}
			}

			// Set as new collision mesh for imported mesh
			obj->m_collisionMesh = collisionMesh;
		}

	#ifndef NO_OBSTACLE_MESH_DATA
		obj->m_navigationObstacle = data.m_navigationObstacle;
	#endif
	}

	// Rebuild shadow info
	obj->RecomputeDefaultRenderMask();
	obj->RecomputeShadowFadeDistance();

	// Recreate rendering data
	obj->CreateRenderResource();
	return obj;	
}

Bool CMesh::GenerateLODWithSimplygon( Int32 lodIndex, const SLODPresetDefinition& lodDefinition, Uint32& numOfRemovedChunks, String& message, Bool showProgress )
{
	if ( !MarkModified() )
	{
		message = TXT("Failed to check out resource");
		return false;
	}

	if ( m_lodLevelInfo.Empty() )
	{
		message = TXT("No LODs in mesh, can't create LOD levels without initial data");
		return false;
	}

	CMeshData data( this );
	if ( !data.GenerateLODWithSimplygon( lodIndex, lodDefinition, numOfRemovedChunks, message, showProgress ) )
		return false;

	data.FlushChanges( true );
	return true;
}

static Bool GetMeshCollisionData( const CMesh &mesh, TDynArray< Vector >* vertices, TDynArray< Uint32 >* indices, TDynArray< Uint16 >* materialIndices )
{
	// Calculate how many data will be added
	Uint32 collisionVerticesCount = 0;
	Uint32 collisionIndicesCount  = 0;

	// Extract data
	const CMeshData data( &mesh );
	const auto& chunks = data.GetChunks();

	for ( Uint32 chunk_i=0; chunk_i<chunks.Size(); ++chunk_i )
	{
		const SMeshChunk &chunk = chunks[chunk_i];
		if ( IsCollisionMeshVertexType( chunk.m_vertexType ) )
		{
			ASSERT( 0 == chunk.m_indices.Size() % 3 );
			collisionVerticesCount	+= chunk.m_vertices.Size();			
			collisionIndicesCount	+= chunk.m_indices.Size();	
		}
	}

	// Allocate space for vertices
	if ( NULL != vertices )
	{
		vertices->Resize( collisionVerticesCount );
	}

	// Allocate space for indices
	if ( NULL != indices )
	{
		indices->Resize( collisionIndicesCount );
	}

	// Allocate space for material indices
	if ( NULL != materialIndices )
	{
		materialIndices->Resize( collisionIndicesCount / 3 );	
	}

	// Emit data
	Uint32 verticesOffset = 0;	
	Uint32 indicesOffset = 0;
	Bool added = false;
	for ( Uint32 chunk_i=0; chunk_i<chunks.Size(); chunk_i++ )
	{
		const SMeshChunk& chunk = chunks[chunk_i];
		if ( !IsCollisionMeshVertexType( chunk.m_vertexType ) )
		{
			continue;
		}

		// copy vertices
		if ( NULL != vertices )
		{
			for ( Uint32 vertex_i=0; vertex_i<chunk.m_vertices.Size(); ++vertex_i )
			{
				const SMeshVertex& vertex = chunk.m_vertices[vertex_i];
				(*vertices)[verticesOffset + vertex_i].Set3( vertex.m_position[0], vertex.m_position[1], vertex.m_position[2] );			
				added = true;
			}
		}

		// copy indices
		if ( NULL != indices )
		{
			for ( Uint32 index_i=0; index_i<chunk.m_indices.Size(); ++index_i )
			{
				(*indices)[indicesOffset + index_i] = verticesOffset + chunk.m_indices[index_i];
				added = true;
			}
		}

		// copy materials
		if ( NULL != materialIndices )
		{
			Uint32 materialIndicesOff = indicesOffset / 3;
			Uint32 numMaterialIndices = chunk.m_indices.Size() / 3;
			for ( Uint32 i=0; i<numMaterialIndices; ++i )
			{
				(*materialIndices)[materialIndicesOff + i] = (Uint16)chunk.m_materialID;
				added = true;
			}		
		}

		// update offsets
		verticesOffset += chunk.m_vertices.Size();
		indicesOffset  += chunk.m_indices.Size();
	}
	ASSERT( NULL==vertices || vertices->Size() == verticesOffset );
	ASSERT( NULL==indices  || indices->Size() == indicesOffset );	

	// Return true if anything was added
	return added;
}


Bool CMesh::GetCollisionData( TDynArray< Vector >& vertices ) const
{
	return GetMeshCollisionData( *this, &vertices, NULL, NULL );	
}

Bool CMesh::GetCollisionData( TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices, TDynArray< Uint16 >& materialIndices ) const
{
	return GetMeshCollisionData( *this, &vertices, &indices, &materialIndices );
}

void CMesh::RemoveCollision()
{	
	// Remove collision mesh
	if ( m_collisionMesh )
	{
		m_collisionMesh->RemoveAll();
		m_collisionMesh->Discard();
		m_collisionMesh = NULL;
	}
}

void CMesh::RemoveCollisionShape( Uint32 index )
{
	if ( m_collisionMesh )
	{
		m_collisionMesh->RemoveShape( index );
	}
}


Uint32 CMesh::AddBoxCollision( Float scale )
{
	if ( !m_collisionMesh )
	{
		// Create new collision mesh
		m_collisionMesh = ::CreateObject< CCollisionMesh >( this );
	}

	if ( m_collisionMesh )
	{
		// Build the scale matrix. Need to add the bounding box offset in here too, since we just pass extents to the collision mesh.
		Matrix matScale;
		matScale.SetIdentity();
		matScale.SetScale33( Vector( scale, scale, scale ) );
		matScale.SetTranslation( m_boundingBox.CalcCenter() );

		// Add this mesh as a source of convex data. CalcExtents() actually returns half-extents (from center to corner), which is what we need.
		m_collisionMesh->AddBox( matScale, m_boundingBox.CalcExtents() );

		return m_collisionMesh->GetShapes().Size() - 1;
	}
	return (Uint32)-1;
}

Uint32 CMesh::AddConvexCollision( Float scale )
{
	if ( !m_collisionMesh )
	{
		// Create new collision mesh
		m_collisionMesh = ::CreateObject< CCollisionMesh >( this );
	}

	if ( m_collisionMesh )
	{
		// Build the scale matrix
		Matrix matScale;
		matScale.SetIdentity();
		matScale.SetScale33( Vector( scale, scale, scale ) );

		// Add this mesh as a source of convex data
		if ( m_collisionMesh->AddConvex( matScale, this ) )
		{
			return m_collisionMesh->GetShapes().Size() - 1;
		}
	}
	return (Uint32)-1;
}

Uint32 CMesh::AddTriMeshCollision( Float scale )
{
	if ( !m_collisionMesh )
	{
		// Create new collision mesh
		m_collisionMesh = ::CreateObject< CCollisionMesh >( this );
	}

	if ( m_collisionMesh )
	{
		// Build the scale matrix
		Matrix matScale;
		matScale.SetIdentity();
		matScale.SetScale33( Vector( scale, scale, scale ) );

		// Add this mesh as a source of mesh data
		m_collisionMesh->AddTriMesh( matScale, this );

		return m_collisionMesh->GetShapes().Size() - 1;
	}
	return (Uint32)-1;
}

Uint32 CMesh::AddSphereCollision( Float scale )
{
	if ( !m_collisionMesh )
	{
		// Create new collision mesh
		m_collisionMesh = ::CreateObject< CCollisionMesh >( this );
	}

	if ( m_collisionMesh )
	{
		// Build the scale matrix. Need to add the bounding box offset in here too, since we just pass extents to the collision mesh.
		Matrix matScale;
		matScale.SetIdentity();
		matScale.SetScale33( Vector( scale, scale, scale ) );
		matScale.SetTranslation( m_boundingBox.CalcCenter() );

		// Add a sphere that approximates the mesh. Use the largest extent for the radius.
		m_collisionMesh->AddSphere( matScale, m_boundingBox.CalcExtents().Upper3() );

		return m_collisionMesh->GetShapes().Size() - 1;
	}
	return (Uint32)-1;
}

Uint32 CMesh::AddCapsuleCollision( Float scale )
{
	if ( !m_collisionMesh )
	{
		// Create new collision mesh
		m_collisionMesh = ::CreateObject< CCollisionMesh >( this );
	}

	if ( m_collisionMesh )
	{
		// Add a capsule that roughly approximates the mesh's bounding box.
		Vector boxSize = m_boundingBox.CalcSize();
		// Find which axis is longest. Since Max returns exact copies of the input values, we don't need to worry about
		// Float equality.
		Float majorAxisLength = Max( boxSize.X, boxSize.Y, boxSize.Z );
		Uint32 majorAxis = ( boxSize.X == majorAxisLength ? 0 : ( boxSize.Y == majorAxisLength ? 1 : 2 ) );

		// 'radius' is the maximum of the two remaining axes. Halve it since we want radius, not diameter.
		Float radius = ( majorAxis == 0 ? Max( boxSize.Y, boxSize.Z ) : ( majorAxis == 1 ? Max( boxSize.X, boxSize.Z ) : Max( boxSize.X, boxSize.Y ) ) ) * 0.5f;
		// 'height' of the capsule is just the length of the largest axis, minus twice the radius. This puts the ends of the mesh at the ends of capsule
		Float height = majorAxisLength - 2 * radius;


		Matrix transform;
		transform.SetIdentity();
		// Depending on the major axis, we may need to rotate the capsule. It will be created so it is oriented along the X axis,
		// so we need to get from there to whatever the major axis is.
		if ( majorAxis == 1 )
		{
			transform.SetRotZ33( M_PI * 0.5f );
		}
		else if ( majorAxis == 2 )
		{
			transform.SetRotY33( M_PI * 0.5f );
		}

		// Next apply scaling. SetScale33 multiplies with existing values, so the rotation will remain.
		transform.SetScale33( Vector( scale, scale, scale ) );
		// Finally, set the translation to position the capsule on the mesh.
		transform.SetTranslation( m_boundingBox.CalcCenter() );

		m_collisionMesh->AddCapsule( transform, radius, height );

		return m_collisionMesh->GetShapes().Size() - 1;
	}
	return (Uint32)-1;
}

RED_INLINE Float CalcVertexDist( const SMeshVertex& v, const SMeshVertex& nv )
{
	const Float dx = v.m_position[0] - nv.m_position[0];
	const Float dy = v.m_position[1] - nv.m_position[1];
	const Float dz = v.m_position[2] - nv.m_position[2];
	return sqrtf( dx*dx + dy*dy + dz*dz );
}

Uint32 CMesh::RemoveUnusedMaterials()
{
	ASSERT( m_materials.Size() == m_materialNames.Size() );

	// Create list of used materials
	TDynArray< Uint32 > usedMaterials;
	for ( const SMeshChunkPacked& meshChunk : m_chunks )
	{
		ASSERT( meshChunk.m_materialID < m_materials.Size() );
		usedMaterials.PushBackUnique( meshChunk.m_materialID );
	}

	Uint32 materialsToRemove = m_materials.Size() - usedMaterials.Size();

	// Update only if something really changes
	if ( materialsToRemove > 0 )
	{
		Sort( usedMaterials.Begin(), usedMaterials.End() ); // have to keep the order

		// Create reindex table
		TDynArray< Uint32 > newMaterialIndices( m_materials.Size() );
		Red::System::MemorySet( newMaterialIndices.Data(), 0, newMaterialIndices.DataSize() );
		for ( Uint32 i=0; i<usedMaterials.Size(); i++ )
		{
			const Uint32 materialIndex = usedMaterials[i];
			newMaterialIndices[ materialIndex ] = i;
		}

		// Remove unused materials
		for ( Int32 i=(m_materials.SizeInt()-1); i>=0; i-- )
		{
			// This material index is not used, remove it
			if ( !usedMaterials.Exist( i ) )
			{
				// Remove entry
				m_materialNames.Erase( m_materialNames.Begin() + i );
				m_materials.Erase( m_materials.Begin() + i );
			}
		}

		// Make sure size of both list matches
		ASSERT( m_materials.Size() == m_materialNames.Size() );

		CMeshData data( this );
		TDynArray< SMeshChunk >& chunks = data.GetChunks();

		// Reindex chunk material indices
		for ( SMeshChunk& meshChunk : chunks )
		{
			ASSERT( meshChunk.m_materialID < newMaterialIndices.Size() );
			meshChunk.m_materialID = newMaterialIndices[ meshChunk.m_materialID ];
			ASSERT( meshChunk.m_materialID < m_materials.Size() );
		}

		data.FlushChanges( false );
	}

	return materialsToRemove;
}

void CMesh::RemoveUnusedBones()
{
	Uint32 numRigBones = m_boneNames.Size();

	if ( numRigBones==0 )
	{
		return;
	}

	// Gather all used bones
	TDynArray< Uint32 > usedBones( numRigBones );
	Red::System::MemorySet( usedBones.TypedData(), 0, usedBones.DataSize() );

	CMeshData data( this );
	auto& chunks = data.GetChunks();

	// Process chunks and mark used bones
	for ( Uint32 i=0; i<chunks.Size(); i++ )
	{
		const auto& chunk = chunks[i];

		// Process vertices from chunk
		for ( Uint32 j=0; j<chunk.m_vertices.Size(); j++ )
		{
			const SMeshVertex& vertex = chunk.m_vertices[j];

			// Gather used bones
			for ( Uint32 k=0; k<4; k++ )
			{
				ASSERT( vertex.m_weights[k] >= 0.0f );
				if ( vertex.m_weights[k] > 0.0f )
				{
					const Uint32 boneIndex = vertex.m_indices[k];
					++usedBones[ boneIndex ];
				}
			}
		}
	}

	// Bone reindex table
	TDynArray< Int32 > newBoneIndices( numRigBones );
	for ( Uint32 i=0; i<numRigBones; i++ )
	{
		newBoneIndices[i] = -1;
	}

	// Create new bone table with used bones
	TSkeletonBoneNameArray newBoneNames;
	TSkeletonBoneMatrixArray newRigMatrices;
	TSkeletonBoneEpsilonArray newVertexEpsilons;

	for ( Uint32 i=0; i<numRigBones; i++ )
	{
		// Copy this bone only if it's used
		if ( usedBones[i] )
		{
			// Add new bone
			const Uint32 newBoneIndex = newBoneNames.Size();

			newBoneNames.PushBack( m_boneNames[i] );
			newRigMatrices.PushBack( m_boneRigMatrices[i] );
			newVertexEpsilons.PushBack( m_boneVertexEpsilons[i] );

			// Remember new mapping
			newBoneIndices[i] = newBoneIndex;
		}
	}

	// Update only if something realy changes
	if ( newBoneNames.Size() != m_boneNames.Size() )
	{
		// Reindex chunks
		for ( Uint32 i=0; i<chunks.Size(); i++ )
		{
			SMeshChunk& chunk = chunks[i];

			// Remap indices
			for ( Uint32 j=0; j<chunk.m_vertices.Size(); j++ )
			{
				SMeshVertex& vertex = chunk.m_vertices[j];

				// Gather used bones
				for ( Uint32 k=0; k<4; k++ )
				{
					if ( vertex.m_weights[k] > 0.0f )
					{
						// Get bone index in the new bone table
						const Uint32 oldBoneIndex = vertex.m_indices[k];
						const Int32 newBoneIndex = newBoneIndices[ oldBoneIndex ];
						// Change
						vertex.m_indices[k] = (Uint8) newBoneIndex;
					}
					else
					{
						vertex.m_weights[k] = 0.0f;
						vertex.m_indices[k] = 0;
					}
				}
			}
		}

		// Show info
		LOG_ENGINE( TXT("RemoveUnusedBones: %i->%i"), m_boneNames.Size(), newBoneNames.Size() );
		m_boneNames = newBoneNames;
		m_boneRigMatrices = newRigMatrices;
		m_boneVertexEpsilons = newVertexEpsilons;

		// Store modified data back in mesh
		data.FlushChanges( false );
	}
}

Bool CMesh::RemoveSkinningFromChunk( Uint32 index )
{
	CMeshData data( this );
	auto& chunks = data.GetChunks();

	ASSERT ( index < chunks.Size() );

	SMeshChunk& chunk = chunks[ index ];

	// Remove skinning data form skinning chunks
	if ( chunk.m_vertexType == MVT_SkinnedMesh || chunk.m_vertexType == MVT_DestructionMesh )
	{
		// Change type
		chunk.m_vertexType = MVT_StaticMesh;

		// Process vertices from chunk
		for ( Uint32 j=0; j<chunk.m_vertices.Size(); j++ )
		{
			SMeshVertex& vertex = chunk.m_vertices[j];

			// Delete skinning data
			vertex.m_indices[0] = 0;
			vertex.m_indices[1] = 0;
			vertex.m_indices[2] = 0;
			vertex.m_indices[3] = 0;
			vertex.m_weights[0] = 0.0f;
			vertex.m_weights[1] = 0.0f;
			vertex.m_weights[2] = 0.0f;
			vertex.m_weights[3] = 0.0f;
		}

		data.FlushChanges( false );
		return true;
	}

	return false;
}

Bool CMesh::RemoveSkinningFromLOD( Uint32 index )
{
	ASSERT ( index < m_lodLevelInfo.Size() );

	// we need a copy here, as flushChanges() can be called in RemoveSkinningFromChunk
	const LODLevel lod = m_lodLevelInfo[index];

	Bool removed = false;

	BatchOperation batch( this );

	for ( auto chunkIdxIt = lod.m_chunks.Begin(); chunkIdxIt != lod.m_chunks.End(); ++chunkIdxIt )
	{
		if ( RemoveSkinningFromChunk( *chunkIdxIt ) )
		{
			removed = true;
		}
	}

	return removed;
}

void CMesh::RemoveSkinning()
{
	BatchOperation batch( this );

	// Change chunks
	for ( Uint32 i=0; i<m_lodLevelInfo.Size(); i++ )
	{
		RemoveSkinningFromLOD( i );
	}

	// Remove skeleton
	m_boneNames.Clear();
	m_boneRigMatrices.Clear();
	m_boneVertexEpsilons.Clear();
}

Bool CMesh::RemoveChunk( Uint32 index )
{
	if ( !MarkModified() )
	{
		return false;
	}

	CMeshData data( this );
	data.RemoveChunk( index );
	data.FlushChanges( false );
	return true;
}

Bool CMesh::RemoveLOD( Uint32 lodIndex )
{
	if ( !MarkModified() )
	{
		return false;
	}

	// Remove all the chunks
	CMeshData data( this );
	data.RemoveLOD( lodIndex );
	data.FlushChanges( false );

	return true;
}

namespace 
{
	template < typename T, size_t N, size_t M >
	void AssignArray( T (&destArray)[N], const T (&sourceArray)[M] )
	{
		for ( size_t i=0; i < Min( N, M ); ++i )
		{
			destArray[i] = sourceArray[i];
		}
	}
}

void CMesh::AddMesh( const CMesh& source, const Matrix& transform, Bool removeRedundantLODs, Bool mergeChunks /*= true*/ )
{
	if ( !MarkModified() )
	{
		return;
	}

	BatchOperation scopedBatch( this );

	// = MATERIALS =

	Int32 matIdOffset = m_materials.Size();

	for ( THandle< IMaterial > mat : source.GetMaterials() )
	{
		if ( mat->GetParent() == &source )
		{ // local instance - make a clone
			m_materials.PushBack( SafeCast< IMaterial >( mat->Clone( this ) ) );
		}
		else
		{
			m_materials.PushBack( mat );
		}
	}

	for ( const String& matName : source.GetMaterialNames() )
	{
		m_materialNames.PushBack( matName );
	}

	// = GEOMETRY =

	const CMeshData srcData( &source );
	const auto& srcChunks = srcData.GetChunks();
	const TLODLevelArray& sourceLods = source.GetMeshLODLevels();

	// = adjust number of LOD levels =

	CMeshData data( this );

	Uint32 numCommonLods;

	if ( GetNumLODLevels() == 0 )
	{
		numCommonLods = sourceLods.Size();

		for ( const LODLevel& sourceLod : sourceLods )
		{
			data.AddLOD( sourceLod.m_meshTypeLOD.m_distance, -1 );
		}
	}
	else
	{
		numCommonLods = Min( data.GetLODs().Size(), sourceLods.Size() );

		if ( removeRedundantLODs )
		{
			while ( data.GetLODs().Size() > numCommonLods )
			{
				data.RemoveLOD( data.GetLODs().Size()-1 );
			}
		}
	}

	for ( Uint32 lodIdx = 0; lodIdx < numCommonLods; ++lodIdx )
	{
		const LODLevel& sourceLod = sourceLods[ lodIdx ];

		for ( Uint32 srcChunkIdx : sourceLod.m_chunks )
		{
			const auto& srcChunk = srcChunks[ srcChunkIdx ];
			Uint32 dstChunkIdx;
			SMeshChunk* dstChunk = data.AddChunkToLOD( lodIdx, srcChunk.m_materialID + matIdOffset, srcChunk.m_vertexType, dstChunkIdx );

			dstChunk->m_renderMask        = srcChunk.m_renderMask;
			dstChunk->m_numBonesPerVertex = srcChunk.m_numBonesPerVertex;
			dstChunk->m_numVertices       = srcChunk.m_numVertices;
			dstChunk->m_numIndices        = srcChunk.m_numIndices;
			dstChunk->m_indices           = srcChunk.m_indices;

			dstChunk->m_vertices.Resize( srcChunk.m_vertices.Size() );
			for ( Uint32 vertIdx = 0; vertIdx < srcChunk.m_vertices.Size(); ++vertIdx )
			{
				const SMeshVertex& srcV = srcChunk.m_vertices[ vertIdx ];
				SMeshVertex& dstV = dstChunk->m_vertices[ vertIdx ];

				dstV.m_color = srcV.m_color;
				AssignArray( dstV.m_position,   transform.TransformPoint( srcV.m_position ).A );
				AssignArray( dstV.m_indices,    srcV.m_indices );
				AssignArray( dstV.m_weights,    srcV.m_weights );
				AssignArray( dstV.m_normal,     transform.TransformVector( srcV.m_normal ).A );
				AssignArray( dstV.m_uv0,        srcV.m_uv0 );
				AssignArray( dstV.m_uv1,        srcV.m_uv1 );
				AssignArray( dstV.m_tangent,    transform.TransformVector( srcV.m_tangent ).A );
				AssignArray( dstV.m_binormal,   transform.TransformVector( srcV.m_binormal ).A );
				AssignArray( dstV.m_extraData0, srcV.m_extraData0 );
				AssignArray( dstV.m_extraData1, srcV.m_extraData1 );
				AssignArray( dstV.m_extraData2, srcV.m_extraData2 );
				AssignArray( dstV.m_extraData3, srcV.m_extraData3 );
			}
		}
	}

	// = Collision =

	if ( source.m_collisionMesh )
	{
		if ( !m_collisionMesh )
		{
			m_collisionMesh = ::CreateObject< CCollisionMesh >( this );
		}

		m_collisionMesh->Append( *source.m_collisionMesh, transform );
	}

	// = Navigation ==

	for ( const SNavigationObstacleShape& obst : source.m_navigationObstacle.GetShapes() )
	{
		TDynArray< Vector2 > verts;
		verts.Reserve( obst.m_verts.Size() );
		for ( const Vector2& v : obst.m_verts )
		{
			verts.PushBack( transform.TransformPoint( v ) );
		}
		m_navigationObstacle.PushShape( Move( verts ), obst.m_bbox );
	}

	// = Other properties =
	m_autoHideDistance = Max( m_autoHideDistance, source.m_autoHideDistance );

	// apply data
	data.FlushChanges( false );

	// optimize out possible duplicated chunks and materials
	if ( mergeChunks )
	{
		MergeChunks();
		RemoveUnusedMaterials();
	}
}

CMesh* CMesh::ExtractCollapsed( EMeshVertexType vertexType, Int32 lodLevel ) const
{
// 	FactoryInfo factoryInfo;
// 
// 	factoryInfo.m_materialNames.PushBack( TXT("pbr_std" ) );
// 	factoryInfo.m_lodLevels.PushBack( LODLevel( 0.f ) );
// 	factoryInfo.m_lodLevels.Back().m_chunks.PushBack( 0 );
// 	factoryInfo.m_chunks.PushBack( SMeshChunk() );
//	SMeshChunk& dstChunk = factoryInfo.m_chunks.Back();
// 	dstChunk.m_materialID = 0;
// 	dstChunk.m_vertexType = vertexType;

	CMesh* result = CreateObject< CMesh >();
	CMeshData dstData( result );

	Uint32 dstChunkIdx;
	dstData.AddLOD( 0.f, -1 );
	SMeshChunk& dstChunk = *dstData.AddChunkToLOD( 0, 0, vertexType, dstChunkIdx );
	ASSERT( dstChunkIdx == 0 );

	result->m_materialNames.PushBack( TXT("default") );
	result->m_materials.PushBack( resDefaultMeshMaterial.LoadAndGet< IMaterial >() );

	const CMeshData srcData( this );
	const auto& srcChunks = srcData.GetChunks();
	const TLODLevelArray& sourceLods = GetMeshLODLevels();

	// get specified (or the lowest) LOD from the source
	const LODLevel& sourceLod = ( lodLevel >= 0 && lodLevel < sourceLods.SizeInt() ) ? sourceLods[ lodLevel ] : sourceLods.Back();

	Uint32 dstIndexIdx  = 0;
	Uint32 dstVertexIdx = 0;
	for ( Uint32 srcChunkIdx : sourceLod.m_chunks )
	{
		const auto& srcChunk = srcChunks[ srcChunkIdx ];

		dstChunk.m_numIndices += srcChunk.m_numIndices;
		dstChunk.m_indices.Resize( dstChunk.m_numIndices );
		for ( Uint16 srcI : srcChunk.m_indices )
		{
			dstChunk.m_indices[ dstIndexIdx++ ] = srcI + dstChunk.m_numVertices;
		}

		dstChunk.m_numVertices += srcChunk.m_numVertices;
		dstChunk.m_vertices.Resize( dstChunk.m_numVertices );
		for ( const SMeshVertex& srcV : srcChunk.m_vertices )
		{ 
			ASSERT( dstVertexIdx < 0xffff );
			SMeshVertex& dstV = dstChunk.m_vertices[ dstVertexIdx++ ];
			dstV.m_color = srcV.m_color;
			AssignArray( dstV.m_position,   srcV.m_position );
			AssignArray( dstV.m_indices,    srcV.m_indices );
			AssignArray( dstV.m_weights,    srcV.m_weights );
			AssignArray( dstV.m_normal,     srcV.m_normal );
			AssignArray( dstV.m_uv0,        srcV.m_uv0 );
			AssignArray( dstV.m_uv1,        srcV.m_uv1 );
			AssignArray( dstV.m_tangent,    srcV.m_tangent );
			AssignArray( dstV.m_binormal,   srcV.m_binormal );
			AssignArray( dstV.m_extraData0, srcV.m_extraData0 );
			AssignArray( dstV.m_extraData1, srcV.m_extraData1 );
			AssignArray( dstV.m_extraData2, srcV.m_extraData2 );
			AssignArray( dstV.m_extraData3, srcV.m_extraData3 );
		}
	}

	dstData.FlushChanges( false );

	//result->CalculateShadowInfo();

	return result;
}

Bool CMesh::MergeChunks( const TDynArray< Uint32 >& chunkIndices, Uint32 materialIndex, String* outErrorStr, Bool optimizeMesh /*= true*/ )
{
	if ( !MarkModified() )
	{
		return false;
	}

	CMeshData data( this );

	if ( !data.MergeChunks( chunkIndices, materialIndex, outErrorStr ) )
	{
		return false;
	}

	return data.FlushChanges( optimizeMesh );
}

Int32 CMesh::MergeChunks( Uint32 lodIndex, Bool countOnly, String* outErrorStr, Bool optimizeMesh /*= true*/ )
{
	if ( !countOnly && !MarkModified() )
	{
		return 0;
	}

	CMeshData data( this );

	Int32 chunksMerged = data.MergeChunks( lodIndex, countOnly, outErrorStr );

	if ( !countOnly && chunksMerged > 0 )
	{
		data.FlushChanges( optimizeMesh );
	}

	return chunksMerged;
}

Int32 CMesh::MergeChunks( Bool countOnly, String* outErrorStr, Bool optimizeMesh /*= true*/ )
{
	if ( !countOnly && !MarkModified() )
	{
		return 0;
	}

	CMeshData data( this );

	Int32 chunksMerged = data.MergeChunks( countOnly, outErrorStr );

	if ( !countOnly && chunksMerged > 0 )
	{
		data.FlushChanges( optimizeMesh );
	}

	return chunksMerged;
}

void CMesh::SetChunkMaterialId( Uint32 chunkIdx, Uint32 materialID )
{
	ASSERT ( chunkIdx < m_chunks.Size() );

	CMeshData data( this );
	data.GetChunks()[ chunkIdx ].m_materialID = materialID;

	data.FlushChanges( false );
}

//////////////////////////////////////////////////////////////////////////

namespace Dex
{
	/// Point cache for mesh
	template< Uint32 bucketCount >
	class MeshPointCache
	{
	protected:
		///! Entry in the cache
		struct Entry
		{
			Vector		m_point;
			Int32		m_next;

			RED_INLINE Entry( const Vector& point )
				: m_point( point )
				, m_next( -1 )
			{};
		};

	protected:
		Int32					m_cacheHeads[ bucketCount ];
		TDynArray< Entry >		m_points;

	public:
		//! Get number of points
		RED_INLINE Uint32 getNumPoints() const { return m_points.Size(); }

		//! Get point coordinates
		RED_INLINE const Vector& getPoint( Uint32 index ) const { return m_points[ index ].m_point; }

	public:
		MeshPointCache()
		{
			for ( Uint32 i=0; i<bucketCount; ++i )
			{
				m_cacheHeads[i] = -1;
			}

			// Preallocate some entries
			//m_points.reserve( bucketCount * 4 );
		}

		Uint32 mapPoint( const Vector& point )
		{
			// Calculate hash
			const Uint32 hash = calcVectorHash( point );

			// Check if such a point is already defined
			Int32 entryIndex = m_cacheHeads[ hash ];
			while ( entryIndex != -1 )
			{
				// Is it the same point ?
				const Entry& entry = m_points[ entryIndex ];
				if ( entry.m_point == point )
				{
					// Found
					return entryIndex;
				}

				// Go to the next entry with the same hash
				entryIndex = entry.m_next;
			}

			// Create new entry
			Uint32 newEntryIndex = m_points.Size();
			Entry newEntry( point );

			// Link the entry in the hash table
			newEntry.m_next = m_cacheHeads[ hash ];
			m_cacheHeads[ hash ] = newEntryIndex;

			// Add to table
			m_points.PushBack( newEntry );

			// Return the entry index ( mapped point )
			return newEntryIndex;
		}

		Uint32 calcVectorHash( const Vector& point ) const
		{
			const Uint32 x = ( ( Uint32& ) point.X );
			const Uint32 y = ( ( Uint32& ) point.Y );
			const Uint32 z = ( ( Uint32& ) point.Z );
			const Uint32 mix = (x >> 10) + (y >> 10) + (z >> 10);
			return ( mix + ( mix >> 10 ) + (mix >> 20) )  % bucketCount;
		}
	};

	struct Triangle
	{
		Uint32					points[3];
		Triangle*				next;
		Triangle**				prev;
		bool					processed;

		Triangle()
			: next( NULL )
			, prev( NULL )
			, processed( false )
		{};

		void link( Triangle*& list )
		{
			if ( list ) list->prev = &next;
			next = list;
			prev = &list;
			list = this;
		}

		void unlink()
		{
			if ( next ) next->prev = prev;
			if ( prev ) *prev = next;
			next = NULL;
			prev = NULL;
		}
	};

	class Connectivity
	{
	public:
		Connectivity()
		{
			m_meshBox.Clear();
		}

		const Uint32 GetNumPoints() const
		{
			return m_points.getNumPoints();
		}

		const Uint32 GetNumTriangles() const
		{
			return m_triangles.Size();
		}

		const Uint32 GetNumClusters() const
		{
			return m_clustersRoots.Size();
		}

		void AddMesh( const Float* coords, Uint32 numVertices, Uint32 vertexStride, const Uint16* indices, Uint32 numIndices )
		{
			// reserve space for triangles data
			const Uint32 firstTriangle = m_triangles.Size();
			m_triangles.Resize( m_triangles.Size() + (numIndices/3) );

			// process vertices
			const Float* read = coords;
			const Float vertexGrid = 100.0f;
			TDynArray< Uint32 >	pointMap( numVertices );
			for ( Uint32 i=0; i<numVertices; ++i, read += vertexStride )
			{
				// this helps by welding vertices together more efficiently
				const Float snappedX = (Int32) (read[0] * vertexGrid) / vertexGrid;
				const Float snappedY = (Int32) (read[1] * vertexGrid) / vertexGrid;
				const Float snappedZ = (Int32) (read[2] * vertexGrid) / vertexGrid;

				Vector point( snappedX, snappedY, snappedZ, 1.0f );
				pointMap[ i ] = m_points.mapPoint( point );
				m_meshBox.AddPoint( point );
			}

			// calculate connectivity ( triangles at points )
			const Uint16* readI = indices;
			for ( Uint32 i=0; i<numIndices; i += 3 )
			{
				// initialize triangle data
				Triangle& tri = m_triangles[ firstTriangle + i/3 ];
				tri.points[0] = pointMap[ *readI++ ];
				tri.points[1] = pointMap[ *readI++ ];
				tri.points[2] = pointMap[ *readI++ ];
			}
		}

		void CreateConnectivity()
		{
			// count number of triangles at each point
			TDynArray< Uint32 > numTrianglesAtPoint( m_points.getNumPoints() );
			Red::MemoryZero( numTrianglesAtPoint.Data(), numTrianglesAtPoint.DataSize() );
			for ( const Triangle& tri : m_triangles )
			{
				numTrianglesAtPoint[ tri.points[0] ] += 1;
				numTrianglesAtPoint[ tri.points[1] ] += 1;
				numTrianglesAtPoint[ tri.points[2] ] += 1;
			}

			// create map data
			Uint32 numTriangleLinks = 0;
			m_triangleLinksOffsets.Resize( m_points.getNumPoints() );
			for ( Uint32 i = 0; i < m_points.getNumPoints(); ++i )
			{
				m_triangleLinksOffsets[i] = numTriangleLinks;
				numTriangleLinks += numTrianglesAtPoint[i] + 1; // one extra entry (NULL) to indicate end of the list
			}

			// resize the data table where we will be putting triangle indices
			m_triangleLinks.Resize( numTriangleLinks );
			Red::MemorySet( m_triangleLinks.Data(), -1, m_triangleLinks.DataSize() );

			// fill the indices table
			for ( Uint32 triangleIndex = 0; triangleIndex < m_triangles.Size(); ++triangleIndex )
			{
				const Triangle& tri = m_triangles[triangleIndex];

				for ( Uint32 i=0; i<3; ++i )
				{
					const Uint32 pointIndex = tri.points[i];
					const Uint32 index = --numTrianglesAtPoint[ pointIndex ]; // counts down, indicates our writing position in the list
					m_triangleLinks[ m_triangleLinksOffsets[pointIndex] + index ] = triangleIndex;
				}
			}

			// we should process all of the links
			for ( Uint32 i=0; i<m_triangleLinksOffsets.Size(); ++i )
			{
				RED_FATAL_ASSERT( numTrianglesAtPoint[i] == 0, "Conectivity calculation failed!" );
			}
		}

		void CreateClusters()
		{
			// create initial list of triangles not yet visited
			Triangle* triangleFreeList = NULL;
			for ( Uint32 i=0; i<m_triangles.Size(); ++i )
			{
				Triangle& tri = m_triangles[i];
				tri.link( triangleFreeList );
			}

			// reserve space in the cluster triangle list (we will have one entry for each triangle)
			m_clustersTriangles.Reserve( m_triangles.Size() );
			m_clustersRoots.Reserve( 128 );

			// process triangles, start from the one that is not yet processed
			while ( triangleFreeList != NULL )
			{
				// stack (unwinded recursion)
				TDynArray< Triangle* > stack( 256 );
				stack.ClearFast();
				stack.PushBack( triangleFreeList );

				// setup cluster root
				m_clustersRoots.PushBack( m_clustersTriangles.Size() );

				// process the triangles
				while ( !stack.Empty() )
				{
					// remove the triangle from stack and from the free list
					Triangle* tri = stack.PopBack();
					tri->processed = true;
					tri->unlink();

					// add triangle to the cluster
					const Uint32 triangleIndex = (Uint32) ( tri - m_triangles.TypedData() );
					m_clustersTriangles.PushBack( triangleIndex );

					// try to visit all triangles that share the same vertices
					// we could use edges but that's simpler
					for ( Uint32 i=0; i<3; ++i )
					{
						const Uint32 pointIndex = tri->points[i];
						Uint32 linkIndex = m_triangleLinksOffsets[ pointIndex ];

						// visit other triangles
						Int32 otherTriangleIndex = m_triangleLinks[ linkIndex++ ];
						while ( otherTriangleIndex != -1 )
						{
							// of the adjacent triangle is not yet processed add it to the list
							Triangle* nextTri = &m_triangles[ otherTriangleIndex ];
							if ( !nextTri->processed )
							{
								nextTri->processed = true;
								stack.PushBack( nextTri );
							}

							// go to next entry, eventually we will reach -1
							otherTriangleIndex = m_triangleLinks[ linkIndex++ ];
						}
					}
				}
			}

			// special end of the list marker
			RED_FATAL_ASSERT( m_clustersTriangles.Size() == m_triangles.Size(), "Clustering error" );
			m_clustersRoots.PushBack( m_clustersTriangles.Size() );
		}

	public:
		Box							m_meshBox;
		MeshPointCache< 2048 >		m_points;
		TDynArray< Uint32 >			m_triangleLinksOffsets; // offsets in table, per point
		TDynArray< Int32 >			m_triangleLinks; // links to other triangles
		TDynArray< Uint32 >			m_clustersRoots;
		TDynArray< Uint32 >			m_clustersTriangles;
		TDynArray< Triangle >		m_triangles;
	};

	class GeneralizedRadiusCalculator
	{
	public:
		void CalculateClustersCOMAndRadius( const Connectivity& connectivity )
		{
			// Prepare data
			m_clusterCOM.Resize( connectivity.m_clustersRoots.Size() );
			m_clusterRadius.Resize( connectivity.m_clustersRoots.Size() );

			// Process clusters
			const Uint32 numClusters = connectivity.m_clustersRoots.Size() - 1;
			for ( Uint32 clusterIndex = 0; clusterIndex < numClusters; ++clusterIndex )
			{
				const Uint32 firstClusterTriangle = connectivity.m_clustersRoots[ clusterIndex ];
				const Uint32 lastClusterTriangle = connectivity.m_clustersRoots[ clusterIndex+1 ]; // always valid

				// Calculate COM for each cluster
				Vector com = Vector::ZEROS;
				for ( Uint32 clusterTriangleIndex = firstClusterTriangle; clusterTriangleIndex < lastClusterTriangle; ++clusterTriangleIndex )
				{
					const Triangle& tri = connectivity.m_triangles[ connectivity.m_clustersTriangles[ clusterTriangleIndex ] ];
					com += connectivity.m_points.getPoint( tri.points[0] ); 
					com += connectivity.m_points.getPoint( tri.points[1] );
					com += connectivity.m_points.getPoint( tri.points[2] );
				}

				// Average
				const Uint32 numPoints = (lastClusterTriangle - firstClusterTriangle) * 3;
				com /= (Float)numPoints;
				m_clusterCOM[ clusterIndex ] = com;

				// Having the COM, compute the largest distance
				Float largestDistanceSq = 0.0f;
				for ( Uint32 clusterTriangleIndex = firstClusterTriangle; clusterTriangleIndex < lastClusterTriangle; ++clusterTriangleIndex )
				{
					const Triangle& tri = connectivity.m_triangles[ connectivity.m_clustersTriangles[ clusterTriangleIndex ] ];
					for ( Uint32 i=0; i<3; ++i )
					{
						const Vector& pos = connectivity.m_points.getPoint( tri.points[i] ); 
						largestDistanceSq = Max< Float >( largestDistanceSq, com.DistanceSquaredTo(pos) );
					}
				}

				// Save best cluster radius
				m_clusterRadius[ clusterIndex ] = sqrtf( largestDistanceSq );
			}
		}

		const Float GetLargestRadius() const
		{
			if ( m_clusterRadius.Empty() )
				return 0.0f;

			Float best = m_clusterRadius[0];
			for ( Uint32 i=1; i<m_clusterRadius.Size(); ++i )
			{
				if ( m_clusterRadius[i] > best )
				{
					best = m_clusterRadius[i];
				}
			}

			return best;
		}

	public:
		TDynArray< Vector >		m_clusterCOM;
		TDynArray< Float >		m_clusterRadius;
	};
}

Float CMesh::GetChunkSurfaceArea( Uint32 chunk_index ) const
{
	const CMeshData data( this );

	const auto& chunks = data.GetChunks();
	if ( chunk_index >= chunks.Size() )
		return 0.0f;

	return GetChunkSurfaceArea( chunks[chunk_index] );
}

Float CMesh::GetChunkSurfaceArea( const SMeshChunk& chunkData )
{
	const Uint32 numtris = chunkData.m_numIndices/3;

	Float area = 0.0f;
	for( Uint32 j=0;j<numtris;++j )
	{
		Int32 facx = chunkData.m_indices[ (j*3)+0 ];
		Int32 facy = chunkData.m_indices[ (j*3)+1 ];
		Int32 facz = chunkData.m_indices[ (j*3)+2 ];
		Vector v0 = chunkData.m_vertices[facx].m_position;
		Vector v1 = chunkData.m_vertices[facy].m_position;
		Vector v2 = chunkData.m_vertices[facz].m_position;
		Vector row1 = v1 - v0;
		Vector row2 = v2 - v0;
		Vector row3 = Vector::Cross( row1, row2 );
		Float ar = row3.Mag3() * 0.5f;
		area += ar;
	}

	return area;
}

void CMesh::RecomputePerChunkShadowMaskOptions()
{
	for ( Uint32 i=0; i<m_lodLevelInfo.Size(); ++i )
	{
		const bool isLastLOD = (i == m_lodLevelInfo.Size()-1);

		const auto& lod = m_lodLevelInfo[i];
		for ( Uint32 j=0; j<lod.m_chunks.Size(); ++j )
		{
			const Uint32 chunkIndex = lod.m_chunks[j];
			m_chunks[ chunkIndex ].m_useForShadowmesh = isLastLOD;
		}
	}
}

void CMesh::RecomputeShadowFadeDistance()
{
	CTimeCounter timer;

	// load mesh data
	const CMeshData data(this);
	const auto& chunks = data.GetChunks();

	// mesh processing
	Dex::Connectivity connectivity;
	Dex::GeneralizedRadiusCalculator radiusCalculator;

	// check only chunks that are casting shadows into the cascades
	const Uint8 cascadeRenderMask = MCR_Cascade1 | MCR_Cascade2 | MCR_Cascade3 | MCR_Cascade4;
	for ( const auto& chunk : data.GetChunks() )
	{
		// no shadows at all ?
		if ( (chunk.m_renderMask & cascadeRenderMask) == 0 )
			continue;

		// get data
		const SMeshVertex* vertices = chunk.m_vertices.TypedData();
		const Uint16* indices = chunk.m_indices.TypedData();

		// calculate generalized chunk extent
		connectivity.AddMesh(
			(const Float*)&vertices->m_position, chunk.m_vertices.Size(), sizeof(SMeshVertex)/sizeof(Float),
			indices, chunk.m_indices.Size() );
	}

	// compute connectivity
	{
		CTimeCounter localTimer;
		connectivity.CreateConnectivity();
		LOG_ENGINE( TXT("Mesh connectivity computed in %1.2fms (%d points, %d triangles)"), 
			localTimer.GetTimePeriodMS(), connectivity.GetNumPoints(), connectivity.GetNumTriangles() );
	}

	// compute clusters
	{
		CTimeCounter localTimer;
		connectivity.CreateClusters();
		LOG_ENGINE( TXT("Mesh clusters computed in %1.2fms (%d clusters)"), 
			localTimer.GetTimePeriodMS(), connectivity.GetNumClusters() );
	}

	// compute COMs
	{
		CTimeCounter localTimer;
		radiusCalculator.CalculateClustersCOMAndRadius( connectivity );
		LOG_ENGINE( TXT("Mesh COMs computed in %1.2fms"), 
			localTimer.GetTimePeriodMS() );
	}

	// compute final shadow distance
	m_generalizedMeshRadius = radiusCalculator.GetLargestRadius();
	LOG_ENGINE( TXT("Generalized mesh radius: %1.2fm (computed in %1.2fms)"), m_generalizedMeshRadius, timer.GetTimePeriodMS() );
}

void CMesh::SortSkinningForChunk( SMeshChunk& chunk )
{
	// We only care about sorting for skinned chunks.
	if ( chunk.m_vertexType != MVT_SkinnedMesh && chunk.m_vertexType != MVT_DestructionMesh )
	{
		return;
	}

	for ( SMeshVertex& vertex : chunk.m_vertices )
	{
		Uint8 bones[4];
		Float weights[4];

		Red::System::MemoryCopy( bones, vertex.m_indices, 4*sizeof( Uint8 ) );
		Red::System::MemoryCopy( weights, vertex.m_weights, 4*sizeof( Float ) );

		Uint32 inds[4] = { 0, 1, 2, 3 };
		Sort( inds, inds + 4, [&]( Uint32 a, Uint32 b ) { return weights[a] > weights[b]; } );

		for ( Uint32 i = 0; i < 4; ++i )
		{
			vertex.m_indices[i] = bones[inds[i]];
			vertex.m_weights[i] = weights[inds[i]];
		}
	}
}

IRenderResource* CMesh::CompileDebugMesh( const EMeshDebugStyle renderStyle, const Int32 lod, const Uint8 renderMask, const Color& debugColor ) const
{
	CMeshData sourceData( this );

	// Invalid source LOD :)
	if ( lod < 0 || lod >= sourceData.GetLODs().SizeInt() )
		return nullptr;

	// No renderer
	if ( !GRender )
		return nullptr;

	// Compute number of triangles/vertices in the source geometry
	Uint32 numSourceTriangles = 0;
	Uint32 numSourceVertices = 0;

	TDynArray< const SMeshChunk* > sourceChunks;
	for ( const auto chunkIndex : sourceData.GetLODs()[ lod ].m_chunks )
	{
		const SMeshChunk* sourceChunk = &sourceData.GetChunks()[ chunkIndex ];
		if ( sourceChunk->m_renderMask & renderMask )
		{
			numSourceVertices += sourceChunk->m_numVertices;
			numSourceTriangles += sourceChunk->m_numIndices / 3;
			sourceChunks.PushBack( sourceChunk );
		}
	}

	// assemble debug mesh
	TDynArray< DebugVertex > debugVertices;
	TDynArray< Uint32 > debugIndices;
	if ( renderStyle == MDS_FaceNormals || renderStyle == MDS_FaceFakeLighting )
	{
		debugVertices.Reserve( numSourceTriangles*3 );
		debugIndices.Reserve( numSourceTriangles*3 );

		Vector lAmbient( 40.0f, 40.0f, 40.0f );
		Vector lDir1( 1.0f, 2.0f, 3.0f );
		Vector lColor1( debugColor.R, debugColor.G, debugColor.B, 255 );
		Vector lDir2( -1.0f, -1.0f, -2.0f );
		Vector lColor2( 128, 128, 128 );
		lDir1.Normalize3();
		lDir2.Normalize3();

		for ( const auto* chunk : sourceChunks )
		{
			const auto* triangle = chunk->m_indices.TypedData();
			for ( Uint32 i=0; i<chunk->m_indices.Size(); i += 3, triangle += 3 )
			{
				const auto& v0 = chunk->m_vertices[ triangle[0] ];
				const auto& v1 = chunk->m_vertices[ triangle[1] ];
				const auto& v2 = chunk->m_vertices[ triangle[2] ];

				// compute normal and color
				Color vertexColor(0,0,0);
				{
					const Vector vba = v1.GetPosition() - v0.GetPosition();
					const Vector vca = v2.GetPosition() - v0.GetPosition();
					Vector n = Vector::Cross( vca, vba );
					if ( n.Normalize3() > 0.0000001f )
					{
						if ( renderStyle == MDS_FaceNormals )
						{
							vertexColor.R = (Uint8) Clamp< Float >( 127.5f + 127.5f*n.X, 0.0f, 255.0f );
							vertexColor.G = (Uint8) Clamp< Float >( 127.5f + 127.5f*n.X, 0.0f, 255.0f );
							vertexColor.B = (Uint8) Clamp< Float >( 127.5f + 127.5f*n.X, 0.0f, 255.0f );
							vertexColor.A = debugColor.A;
						}
						else if ( renderStyle == MDS_FaceFakeLighting )
						{
							const Float nDotL1 = Clamp< Float >( Vector::Dot3( n, lDir1 ), 0.0f, 1.0f );
							const Float nDotL2 = Clamp< Float >( Vector::Dot3( n, lDir2 ), 0.0f, 1.0f );
							vertexColor.R = (Uint8) Clamp< Float >( lAmbient.X + lColor1.X * nDotL1 + lColor2.X * nDotL2, 0.0f, 255.0f );
							vertexColor.G = (Uint8) Clamp< Float >( lAmbient.Y + lColor1.Y * nDotL1 + lColor2.Y * nDotL2, 0.0f, 255.0f );
							vertexColor.B = (Uint8) Clamp< Float >( lAmbient.Z + lColor1.Z * nDotL1 + lColor2.Z * nDotL2, 0.0f, 255.0f );
							vertexColor.A = debugColor.A;
						}
					}
				}

				// setup vertex
				debugIndices.PushBack( debugVertices.Size() );
				new ( debugVertices ) DebugVertex( v0.GetPosition(), vertexColor.ToUint32() );

				// setup vertex
				debugIndices.PushBack( debugVertices.Size() );
				new ( debugVertices ) DebugVertex( v1.GetPosition(), vertexColor.ToUint32() );

				// setup vertex
				debugIndices.PushBack( debugVertices.Size() );
				new ( debugVertices ) DebugVertex( v2.GetPosition(), vertexColor.ToUint32() );
			}
		}
	}

	// no data
	if ( debugVertices.Empty() || debugIndices.Empty() )
		return nullptr;

	// create debug mesh for rendering
	return GRender->UploadDebugMesh( debugVertices, debugIndices );
}

#endif

//////////////////////////////////////////////////////////////////////////


