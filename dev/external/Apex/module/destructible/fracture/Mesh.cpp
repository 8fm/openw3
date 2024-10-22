// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

#include "RTdef.h"
#if RT_COMPILE
#include <NxRenderMeshAsset.h>

#include "Mesh.h"

namespace physx
{
namespace fracture
{

void Mesh::gatherPartMesh(Array<physx::PxVec3>& vertices,
					Array<physx::PxU32>&  indices,
					Array<physx::PxVec3>& normals,
					Array<physx::PxVec2>& texcoords,
					Array<SubMesh>& subMeshes,
					const NxRenderMeshAsset& renderMeshAsset,
					physx::PxU32 partIndex)
					
{
	if (partIndex >= renderMeshAsset.getPartCount())
	{
		vertices.resize(0);
		indices.resize(0);
		normals.resize(0);
		texcoords.resize(0);
		subMeshes.resize(0);
		return;
	}

	subMeshes.resize(renderMeshAsset.getSubmeshCount());

	// Pre-count vertices and indices so we can allocate once
	physx::PxU32 vertexCount = 0;
	physx::PxU32 indexCount = 0;
	for (physx::PxU32 submeshIndex = 0; submeshIndex < renderMeshAsset.getSubmeshCount(); ++submeshIndex)
	{
		const NxRenderSubmesh& submesh = renderMeshAsset.getSubmesh(submeshIndex);
		vertexCount += submesh.getVertexCount(partIndex);
		indexCount += submesh.getIndexCount(partIndex);
	}

	vertices.resize(vertexCount);
	normals.resize(vertexCount);
	texcoords.resize(vertexCount);
	indices.resize(indexCount);

	vertexCount = 0;
	indexCount = 0;
	for (physx::PxU32 submeshIndex = 0; submeshIndex < renderMeshAsset.getSubmeshCount(); ++submeshIndex)
	{
		const NxRenderSubmesh& submesh = renderMeshAsset.getSubmesh(submeshIndex);
		const PxU32 submeshVertexCount = submesh.getVertexCount(partIndex);
		if (submeshVertexCount > 0)
		{
			const NxVertexBuffer& vertexBuffer = submesh.getVertexBuffer();
			const NxVertexFormat& vertexFormat = vertexBuffer.getFormat();

			enum { MESH_SEMANTIC_COUNT = 3 };
			struct { 
				NxRenderVertexSemantic::Enum semantic; 
				NxRenderDataFormat::Enum format; 
				PxU32 sizeInBytes;
				void* dstBuffer;
			} semanticData[MESH_SEMANTIC_COUNT] = {
				{ NxRenderVertexSemantic::POSITION,  NxRenderDataFormat::FLOAT3, sizeof(PxVec3), &vertices[vertexCount]  },
				{ NxRenderVertexSemantic::NORMAL,    NxRenderDataFormat::FLOAT3, sizeof(PxVec3), &normals[vertexCount]   },
				{ NxRenderVertexSemantic::TEXCOORD0, NxRenderDataFormat::FLOAT2, sizeof(PxVec2), &texcoords[vertexCount] } 
			};

			for (PxU32 i = 0; i < MESH_SEMANTIC_COUNT; ++i)
			{
				const physx::PxI32 bufferIndex = vertexFormat.getBufferIndexFromID(vertexFormat.getSemanticID(semanticData[i].semantic));
				if (bufferIndex >= 0)
					vertexBuffer.getBufferData(semanticData[i].dstBuffer, 
											   semanticData[i].format, 
											   semanticData[i].sizeInBytes, 
											   bufferIndex, 
											   submesh.getFirstVertexIndex(partIndex), 
											   submesh.getVertexCount(partIndex));
				else
					memset(semanticData[i].dstBuffer, 0, submesh.getVertexCount(partIndex)*semanticData[i].sizeInBytes);
			}

			/*
			const PxU32 firstVertexIndex       = submesh.getFirstVertexIndex(partIndex);
			fillBuffer<PxVec3>(vertexBuffer, vertexFormat, NxRenderVertexSemantic::POSITION, NxRenderDataFormat::FLOAT3,
			                   firstVertexIndex, submeshVertexCount, &vertices[vertexCount]);
			fillBuffer<PxVec3>(vertexBuffer, vertexFormat, NxRenderVertexSemantic::NORMAL, NxRenderDataFormat::FLOAT3,
			                   firstVertexIndex, submeshVertexCount, &normals[vertexCount]);
			fillBuffer<PxVec2>(vertexBuffer, vertexFormat, NxRenderVertexSemantic::TEXCOORD0, NxRenderDataFormat::FLOAT2,
			                   firstVertexIndex, submeshVertexCount, &texcoords[vertexCount]);*/

			const physx::PxU32* partIndexBuffer = submesh.getIndexBuffer(partIndex);
			const physx::PxU32 partIndexCount = submesh.getIndexCount(partIndex);
			subMeshes[submeshIndex].firstIndex = partIndexCount;
			for (physx::PxU32 indexNum = 0; indexNum < partIndexCount; ++indexNum)
			{
				indices[indexCount++] = partIndexBuffer[indexNum] + vertexCount - submesh.getFirstVertexIndex(partIndex);
			}
			vertexCount += submeshVertexCount;
		}
	}
}


void Mesh::loadFromRenderMesh(const NxRenderMeshAsset& mesh, PxU32 partIndex)
{
	gatherPartMesh(mVertices, mIndices, mNormals, mTexCoords, mSubMeshes, mesh, partIndex);
}

}
}
#endif