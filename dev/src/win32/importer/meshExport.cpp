#include "Build.h"

#include "meshExport.h"

#include "../../common/core/scopedPtr.h"
#include "../../common/engine/meshDataBuilder.h"
#include "ReFileHelpers.h"

static const Float EXPORT_SCALE = 100.f;

#ifndef NO_RESOURCE_IMPORT

namespace MeshReExport
{
	void ExportLODToRE( ReFileArchive& archive, Int32 lodIdx, const CMesh::LODLevel& lod, const CMesh& mesh )
	{
		// note: archive becomes an owner of the buffer
		ReFileBuffer* mbuff = archive.Append( new ReFileBuffer( 'mesh', lodIdx ) );

		Red::TScopedPtr< ReFileMesh > destMesh( new ReFileMesh() );

		Int32 numChunks = lod.m_chunks.Size();
		Int32 numBones = mesh.GetBoneCount();

		destMesh->set( StringAnsi::Printf( "LOD%d", lodIdx ).AsChar(), numChunks, numBones );

		// Do bones
		for ( Int32 i=0; i<numBones; ++i )
		{
			destMesh->mRigMatrices[i].setName( mesh.GetBoneNames()[i].AsAnsiChar() );
			const Matrix& srcM = mesh.GetBoneRigMatrices()[i];
			auto& destM = destMesh->mRigMatrices[i].mRigMatrix;

			destM[0] = srcM.GetRow(0).X;
			destM[1] = srcM.GetRow(0).Y;
			destM[2] = srcM.GetRow(0).Z;

			destM[3] = srcM.GetRow(1).X;
			destM[4] = srcM.GetRow(1).Y;
			destM[5] = srcM.GetRow(1).Z;

			destM[6] = srcM.GetRow(2).X;
			destM[7] = srcM.GetRow(2).Y;
			destM[8] = srcM.GetRow(2).Z;

			destM[9 ] = srcM.GetRow(3).X * EXPORT_SCALE;
			destM[10] = srcM.GetRow(3).Y * EXPORT_SCALE;
			destM[11] = srcM.GetRow(3).Z * EXPORT_SCALE;
		}

		destMesh->mTransformMatrix[0] = 1.0f;
		destMesh->mTransformMatrix[1] = 0.0f;
		destMesh->mTransformMatrix[2] = 0.0f;

		destMesh->mTransformMatrix[3] = 0.0f;
		destMesh->mTransformMatrix[4] = 1.0f;
		destMesh->mTransformMatrix[5] = 0.0f;

		destMesh->mTransformMatrix[6] = 0.0f;
		destMesh->mTransformMatrix[7] = 0.0f;
		destMesh->mTransformMatrix[8] = 1.0f;

		destMesh->mTransformMatrix[9] = 0.0f;
		destMesh->mTransformMatrix[10] = 0.0f;
		destMesh->mTransformMatrix[11] = 0.0f;

		// do geometry
		const CMeshData data( &mesh );
		const auto& chunks = data.GetChunks();

		for ( Int32 i = 0; i < numChunks; ++i )
		{
			Int32 chind = lod.m_chunks[i];

			const SMeshChunk& srcChunk = chunks[chind];
			ReFileMeshChunk& destChunk = destMesh->mMeshChunks[i];

			//Int32 matid  = srcChunk.m_materialID;
			Int32 numfac = srcChunk.m_numIndices/3;
			Int32 numv   = srcChunk.m_numVertices;

			const AnsiChar* matName = 
				( chunks[chind].m_materialID >= 0 )
				? UNICODE_TO_ANSI( mesh.GetMaterialNames()[chunks[chind].m_materialID].AsChar() )
				: "unknown";

			//destMesh->chunks[i].material.set
			ReFileMaterial reMaterial( matName, "diff", "nor", "ble" );
			destChunk.setMeshChunkData( numv, numfac, reMaterial );

			for ( Int32 v=0; v < numv; ++v )
			{
				const SMeshVertex& srcV = chunks[chind].m_vertices[v];

				// In W3 we support only 4 bones skinning so please
				// use ReFileFatVertex with care regarding getExtra4Weights and getExtra4BoneIndices
				ReFileFatVertex& destV  = destChunk.mVertices[v];

				float3 value( srcV.m_position[0], srcV.m_position[1], srcV.m_position[2] );
				value *= EXPORT_SCALE;
				destV.setPosition( value );

				value.x = srcV.m_normal[0];
				value.y = srcV.m_normal[1];
				value.z = srcV.m_normal[2];
				destV.setNormal( value );

				value.x = srcV.m_tangent[0];
				value.y = srcV.m_tangent[1];
				value.z = srcV.m_tangent[2];
				destV.setTangent( value );

				value.x = srcV.m_binormal[0];
				value.y = srcV.m_binormal[1];
				value.z = srcV.m_binormal[2];
				destV.setBinormal( value );

				float2 valueUV( srcV.m_uv0[0], ( 1.f - srcV.m_uv0[1] ) );
				destV.setUV1( valueUV );

				valueUV.x = srcV.m_uv1[0];
				valueUV.y =  1.0f - srcV.m_uv1[1];
				destV.setUV2( valueUV );

				Uint32 col = srcV.m_color;
				Uint8* ccc = (Uint8*)&col;

				float4 tempVal( ccc[0], ccc[1], ccc[2], ccc[3] );
				tempVal.x /= 255.f;
				tempVal.y /= 255.f;
				tempVal.z /= 255.f;
				tempVal.w /= 255.f;
				destV.setColor( tempVal );

				tempVal.x = srcV.m_weights[0];
				tempVal.y = srcV.m_weights[1];
				tempVal.z = srcV.m_weights[2];
				tempVal.w = srcV.m_weights[3];

				destV.setWeights( tempVal );

				tempVal.x = srcV.m_indices[0];
				tempVal.y = srcV.m_indices[1];
				tempVal.z = srcV.m_indices[2];
				tempVal.w = srcV.m_indices[3];

				destV.setBoneIndices( tempVal );
			}

			for( Int32 f=0; f < numfac; ++f )
			{
				ReFileFace& destF = destChunk.mFaces[f];

				destF.mIndices[0] = srcChunk.m_indices[(f*3)+2];
				destF.mIndices[1] = srcChunk.m_indices[(f*3)+1];
				destF.mIndices[2] = srcChunk.m_indices[(f*3)+0];
				destF.mId = srcChunk.m_materialID;
			}
		}

		destMesh->write( mbuff );
	}
}

#endif // NO_RESOURCE_IMPORT
