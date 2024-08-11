/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "mesh.h"
#include "material.h"
#include "mergedMeshBuilder.h"
#include "materialDefinition.h"
#include "meshComponent.h"
#include "materialGraph.h"
#include "../core/gatheredResource.h"

CGatheredResource	resMergedMeshMaterial( TXT("engine\\materials\\defaults\\mergedmesh.w2mg"), 0 );

#ifndef NO_RESOURCE_IMPORT

MergedMeshSourceData::MergedMeshSourceData( const Uint32 renderMask )
	: m_numVertices( 0 )
	, m_numTriangles( 0 )
	, m_mergeMask( renderMask )
{
	m_worldBox.Clear();
}

void MergedMeshSourceData::AddComponent( const CMeshComponent* component, const TDynArray< Uint32 >& chunkIndices )
{
	const CMesh* mesh = component->GetMeshNow();
	if ( !mesh )
		return;

	// process chunks to see if it's worth merging
	Bool hasMergableData = false;
	for ( Uint32 i=0; i<chunkIndices.Size(); ++i )
	{
		const auto& chunk = mesh->GetChunks()[ chunkIndices[i] ];
		if ( chunk.m_renderMask & m_mergeMask )
		{
			m_numTriangles += chunk.m_numIndices / 3;
			m_numVertices += chunk.m_numVertices;

			hasMergableData = true;
			break;
		}
	}

	// no data to merge
	if ( !hasMergableData )
		return;

	// add info
	ObjectInfo info;
	info.m_id = GlobalVisID( mesh, component );
	info.m_mesh = mesh;
	info.m_chunksToUse = chunkIndices;
	info.m_renderMask = m_mergeMask;
	info.m_localToWorld = component->GetLocalToWorld();
	m_objects.PushBack( info );

	// add stuff to box
	const Box localBox = component->GetLocalToWorld().TransformBox( mesh->GetBoundingBox() ) ;
	m_worldBox.AddBox( localBox );
}

void MergedMeshSourceData::InsertObjects( CMeshData& outMeshData, const CMeshData::IMergeGeometryModifier* geometryModifier, const EMeshVertexType vertexType, TDynArray<String>& outCorruptedMeshPaths  ) const
{
	for ( const ObjectInfo& object : m_objects )
	{
		// add matching chunks
		const CMeshData sourceMeshData( object.m_mesh );
		const auto& sourceChunks = sourceMeshData.GetChunks();
		const auto& sourceLODs = sourceMeshData.GetLODs();

		// merge matching chunks only
		for ( const auto& sourceChunkIndex : object.m_chunksToUse )
		{
			const auto& sourceChunk = sourceChunks[ sourceChunkIndex ];
			if ( sourceChunk.m_renderMask & m_mergeMask )
			{
				if ( sourceChunk.m_materialID < object.m_mesh->GetMaterials().Size() )
				{
					IMaterial* sourceMaterial = object.m_mesh->GetMaterials()[ sourceChunk.m_materialID ];
					if ( !sourceMaterial )
						continue;

					CMaterialGraph* baseSourceMaterial = Cast< CMaterialGraph >( sourceMaterial->GetMaterialDefinition() );
					if ( !baseSourceMaterial )
						continue;

					// transparency check - do not merge transparent chunks
					if ( baseSourceMaterial->GetRenderingBlendMode() != RBM_None )
						continue;

					// masked check - do not merge masked materials
					if ( sourceMaterial->IsMasked() )
						continue;

					// volume material check
					if ( sourceChunk.m_materialID < object.m_mesh->GetMaterialNames().Size() )
					{
						const String& name = object.m_mesh->GetMaterialNames()[sourceChunk.m_materialID];
						if ( name == TXT("volume") )
							continue;
					}

					// add chunk
					const Uint8 mergedRenderMask = sourceChunk.m_renderMask & m_mergeMask;
					if ( mergedRenderMask != 0 )
					{
						if( !outMeshData.AppendChunk( sourceChunk, 0, object.m_localToWorld, geometryModifier, mergedRenderMask, 0, vertexType ) )
						{
							// skip & log corrupted meshes, so they can be fixed later on
							outCorruptedMeshPaths.PushBackUnique( object.m_mesh->GetDepotPath() );
						}
					}
				}
			}
		}
	}
}

//----------------------------

MergedMeshBuilder::MergedMeshBuilder( THandle< CMesh > mesh, EMeshVertexType vertexType )
	: m_mesh( mesh )
	, m_vertexType( vertexType )
	, m_data( mesh )
{
	// remove existing stuff from the mesh, leave one LOD
	m_data.Clear();

	// add default material if not present
	mesh->GetMaterials().Resize(0);
	mesh->GetMaterials().PushBack( resMergedMeshMaterial.LoadAndGet< IMaterial >() );
	mesh->GetMaterialNames().Resize(0);
	mesh->GetMaterialNames().PushBack( TXT("merged_material") );
}

void MergedMeshBuilder::AddData( const MergedMeshSourceData& source, const CMeshData::IMergeGeometryModifier* geometryModifier, TDynArray<String> &outCorruptedMeshDepotPaths, Vector& outOrigin )
{	
	// pack data	

	CTimeCounter counter;
	source.InsertObjects( m_data, geometryModifier, m_vertexType, outCorruptedMeshDepotPaths );	

	outOrigin = source.GetWorldBox().GetMassCenter();

	// Move all chunks near the origin
	auto& chunks = m_data.GetChunks();
	for( auto& ch : chunks )
	{
		for( auto& v : ch.m_vertices )
		{
			v.m_position[0] -= outOrigin.X;
			v.m_position[1] -= outOrigin.Y;
			v.m_position[2] -= outOrigin.Z;
		}
	}

	// stats
	LOG_ENGINE( TXT("Inserted %d object into mesh in %1.2f ms, %d vertices, %d triangles, %d chunks"), 
		source.GetNumObjects(), 
		counter.GetTimePeriodMS(),
		m_data.GetNumVertices(), m_data.GetNumTriangles(), m_data.GetChunks().Size() );	
}

void MergedMeshBuilder::SetRenderMaskForAllChunks( const Uint8 renderMask )
{
	for ( auto& chunk : m_data.GetChunks() )
	{
		chunk.m_renderMask = renderMask;
	}
}

Bool MergedMeshBuilder::Flush()
{
	// check triangle count
	if ( m_data.GetNumTriangles() == 0 )
		return false;

	// we do not reoptimize mesh here
	m_data.FlushChanges( /*optimized*/ false );
	return true;
}

#endif