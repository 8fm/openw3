
#pragma once

#include "mesh.h"

struct SLODPreset
{
	String m_name;
	TDynArray< SLODPresetDefinition > m_definitions;
	Uint32 m_faceLimit;
};

//! Deducts the LOD generation preset that would lead to the actual mesh LOD setup
inline SLODPreset DeductLODsPresetFromMesh( const CMesh* mesh )
{
	const auto& chunks = mesh->GetChunks();
	
	CMesh::TLODLevelArray lods = mesh->GetMeshLODLevels();
	ASSERT ( lods.Size() >= 1 );

	SLODPreset preset;
	preset.m_name = TXT("deducted");
	preset.m_faceLimit = 0; // this doesn't matter, the number of LODs is the answer

	if ( lods.Size() > 1 )
	{
		Uint32 baseTriangleCount = mesh->CountLODTriangles( 0 );

		for ( Uint32 i=1; i<lods.Size(); ++i )
		{
			Uint32 chunkFaceLimit = NumericLimits< Uint32 >::Max();

			const CMesh::LODLevel& lod = lods[i];

			Bool hasSkinning = false;

			for ( auto chunkIt = lod.m_chunks.Begin(); chunkIt < lod.m_chunks.End(); ++chunkIt )
			{
				Uint32 chunkIdx = *chunkIt;
				Uint32 chunkTriangleCount = chunks[ chunkIdx ].m_numIndices / 3;

				if ( chunkTriangleCount-1 < chunkFaceLimit )
				{
					chunkFaceLimit = chunkTriangleCount-1;
				}

				hasSkinning = hasSkinning || chunks[ chunkIdx ].m_vertexType == MVT_SkinnedMesh || chunks[ chunkIdx ].m_vertexType == MVT_DestructionMesh;
			}

			Uint32 triangleCount = mesh->CountLODTriangles( i );

			Float reduction = static_cast< Float >( triangleCount ) / baseTriangleCount;
			Float distance = lod.GetDistance();

			// we're not expecting exactly the same mesh after reimporting, so we might get slightly different triangles count
			// - the triangles count shouldn't be limited so strictly
			Int32 finalChunkFaceLimit = (Int32)MFloor( 0.95f * chunkFaceLimit );

			preset.m_definitions.PushBack( SLODPresetDefinition( reduction, distance, finalChunkFaceLimit, !hasSkinning ) );
		}
	}

	return preset;
}

#ifdef USE_SIMPLYGON
//! Creates a bunch of LODs for the mesh basing on the given preset. All previously existing LODs are removed.
template < typename MsgFunct >
Bool GenerateLODsForMesh( CMesh* mesh, const SLODPreset& preset, String& outErrorMsg, MsgFunct msgCallback )
{
	CMesh::BatchOperation batch( mesh );

	for ( Int32 i = mesh->GetNumLODLevels()-1; i > 0 ; --i )
	{
		mesh->RemoveLOD( i );
	}

	if ( mesh->CountLODTriangles( 0 ) > preset.m_faceLimit )
	{
		for ( auto defI = preset.m_definitions.Begin(); defI != preset.m_definitions.End(); ++defI )
		{
			Uint32 removedChunks;
			if ( !mesh->GenerateLODWithSimplygon( mesh->GetNumLODLevels(), *defI, removedChunks, outErrorMsg, false ) )
			{
				return false;
			}

			Uint32 lodIdx = mesh->GetNumLODLevels()-1;
			Uint32 triangleCount = mesh->CountLODTriangles( lodIdx );

			if ( triangleCount < preset.m_faceLimit )
			{
				mesh->RemoveLOD( lodIdx );
				msgCallback( TXT( "    Triangle limit hit" ) );
				break;
			}
			else
			{
				String msg = String::Printf( TXT( "    LOD [%i] created, %i triangles" ), lodIdx, triangleCount );
				if ( removedChunks != 0 )
				{
					msg += String::Printf( TXT( " (%i chunk(s) removed)" ), removedChunks );
				}

				if ( defI->m_removeSkinning && mesh->RemoveSkinningFromLOD( lodIdx ) )
				{
					msg += TXT( " (skinning removed)" );
				}

				msgCallback( msg );
			}
		}
	}

	return true;
}
#endif