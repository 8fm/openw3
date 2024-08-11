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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef GU_TRIANGLEMESH_H
#define GU_TRIANGLEMESH_H

#include "CmPhysXCommon.h"
#include "GuInternalTriangleMesh.h"
#include "CmRefCountable.h"
#include "PxTriangle.h"
#include "PxTriangleMesh.h"
#include "CmRenderOutput.h"

namespace physx
{

class GuMeshFactory;
class PxMeshScale;
class PxInputStream;


namespace Gu
{


enum InternalMeshSerialFlag
{
	IMSF_MATERIALS		=	(1<<0),
	IMSF_FACE_REMAP		=	(1<<1),
	IMSF_8BIT_INDICES	=	(1<<2),
	IMSF_16BIT_INDICES	=	(1<<3),
	IMSF_ADJACENCIES	=	(1<<4),
};

// 1: support stackless collision trees for non-recursive collision queries
// 2: height field functionality not supported anymore
// 3: mass struct removed
// 4: bounding sphere removed
// 5: RTree added, opcode tree still in the binary image, physx 3.0
// 6: opcode tree removed from binary image
// 7: convex decomposition is out
// 8: adjacency information added
// 9: removed leaf triangles and most of opcode data, changed rtree layout
// 10: float rtrees
// 11: new build, isLeaf added to page
// 12: isLeaf is now the lowest bit in ptrs

#define PX_MESH_VERSION 12 // search for PX_MESH_VERSION_BC for backwards compabitility code

class TriangleMesh : public PxTriangleMesh, public Ps::UserAllocated, public Cm::RefCountable
{
public:
// PX_SERIALIZATION
		PX_PHYSX_COMMON_API 										TriangleMesh(PxBaseFlags baseFlags)	: PxTriangleMesh(baseFlags), Cm::RefCountable(PxEmpty), mMesh(PxEmpty)	{}
		PX_PHYSX_COMMON_API virtual		void						onRefCountZero();
		PX_PHYSX_COMMON_API virtual		void						exportExtraData(PxSerializationContext&);
		PX_PHYSX_COMMON_API				void						importExtraData(PxDeserializationContext&);
		PX_INLINE						void						setMeshFactory(GuMeshFactory* f)	{ mMeshFactory = f;					}
		PX_PHYSX_COMMON_API	static		TriangleMesh*				createObject(PxU8*& address, PxDeserializationContext& context);
		PX_PHYSX_COMMON_API static		void						getBinaryMetaData(PxOutputStream& stream);
										void						resolveReferences(PxDeserializationContext& ) {}
							virtual		void						requires(PxProcessPxBaseCallback&){}
	//~PX_SERIALIZATION
		PX_PHYSX_COMMON_API 										TriangleMesh();

		PX_PHYSX_COMMON_API bool									load(PxInputStream& stream);
// PxTriangleMesh

		PX_PHYSX_COMMON_API virtual		PxU32						getNbVertices()						const	{ return mMesh.getNumVertices();	}
		PX_PHYSX_COMMON_API virtual		const PxVec3*				getVertices()						const	{ return mMesh.getVertices();		}
		PX_PHYSX_COMMON_API virtual		const PxU32*				getTrianglesRemap()					const	{ return mMesh.getFaceRemap();		}
		PX_PHYSX_COMMON_API virtual		PxU32						getNbTriangles()					const	{ return mMesh.getNumTriangles();	}
		PX_PHYSX_COMMON_API virtual		const void*					getTriangles()						const	{ return mMesh.getTriangles();		}
		PX_PHYSX_COMMON_API virtual		PxTriangleMeshFlags			getTriangleMeshFlags()				const	{ return mMesh.getTriangleMeshFlags();	}

		PX_PHYSX_COMMON_API virtual		void						release();
		PX_PHYSX_COMMON_API virtual		PxMaterialTableIndex		getTriangleMaterialIndex(PxTriangleID triangleIndex)	const	{ return hasPerTriangleMaterials() ?  mMesh.getMaterials()[triangleIndex] : 0xffff;	}
		PX_PHYSX_COMMON_API virtual		PxBounds3					getLocalBounds()					const	{ PX_ASSERT(mMesh.mData.mAABB.isValid()); return mMesh.mData.mAABB; }
		PX_PHYSX_COMMON_API virtual		PxU32						getReferenceCount()					const;
//~PxTriangleMesh

	PX_FORCE_INLINE	const PxU32*				getFaceRemap()						const	{ return mMesh.getFaceRemap();		}

	PX_FORCE_INLINE	bool						has16BitIndicesFast()				const	{ return mMesh.has16BitIndices();	}
	PX_FORCE_INLINE	bool						hasPerTriangleMaterials()			const	{ return mMesh.getMaterials()!=NULL;}
	PX_FORCE_INLINE	PxU32						getNumVerticesFast()				const	{ return mMesh.getNumVertices();	}
	PX_FORCE_INLINE	PxU32						getNumTrianglesFast()				const	{ return mMesh.getNumTriangles();	}
	PX_FORCE_INLINE	const void*					getTrianglesFast()					const	{ return mMesh.getTriangles();		}
	PX_FORCE_INLINE	const PxVec3*				getVerticesFast()					const	{ return mMesh.getVertices();		}

//	PX_FORCE_INLINE	PxU32						getTrigSharedEdgeFlags(PxU32 trigIndex) const			{ return mMesh.getTrigSharedEdgeFlags(trigIndex);		}
	PX_FORCE_INLINE	void						setTrigSharedEdgeFlag(PxU32 trigIndex, PxU32 edgeIndex)	{ mMesh.setTrigSharedEdgeFlag(trigIndex, edgeIndex);	}

	PX_FORCE_INLINE	void						computeWorldTriangle(PxTriangle& worldTri, PxTriangleID triangleIndex, const Cm::Matrix34& worldMatrix, PxU32* PX_RESTRICT vertexIndices=NULL,PxU32* PX_RESTRICT adjacencyIndices=NULL) const;
	PX_FORCE_INLINE void						getLocalTriangle(PxTriangle& localTri, PxTriangleID triangleIndex) const;
	PX_FORCE_INLINE	PxReal						getGeomEpsilon()					const	{ return mMesh.mData.mOpcodeModel.mGeomEpsilon;	}
	PX_FORCE_INLINE	const PxBounds3&			getLocalBoundsFast()				const	{ return mMesh.mData.mAABB;						}

	PX_FORCE_INLINE	const Gu::RTreeMidphase&	getOpcodeModel()					const	{ return mMesh.getOpcodeModel();				}

					InternalTriangleMesh		mMesh;

					GuMeshFactory*				mMeshFactory;					// PT: changed to pointer for serialization
protected:
	PX_PHYSX_COMMON_API virtual										~TriangleMesh();

#if PX_ENABLE_DEBUG_VISUALIZATION
public:
	/**
	\brief Perform triangle mesh geometry debug visualization

	\param[out] Debug renderer.
	\param[out] World position.
	*/
	PX_PHYSX_COMMON_API virtual			void	debugVisualize(Cm::RenderOutput& out, const Cm::Matrix34& absPose, const PxMeshScale& scaling, const PxBounds3& cullbox,
													const PxU64 mask, const PxReal fscale, const PxU32 numMaterials) const;
#endif
};

} // namespace Gu

PX_FORCE_INLINE void Gu::TriangleMesh::computeWorldTriangle(PxTriangle& worldTri, PxTriangleID triangleIndex, const Cm::Matrix34& worldMatrix, PxU32* PX_RESTRICT vertexIndices, PxU32* PX_RESTRICT adjacencyIndices) const
{
	PxU32 vref0, vref1, vref2;
	if(mMesh.has16BitIndices())
	{
		const Gu::TriangleT<PxU16>& T = ((const Gu::TriangleT<PxU16>*)getTrianglesFast())[triangleIndex];
		vref0 = T.v[0];
		vref1 = T.v[1];
		vref2 = T.v[2];
	}
	else
	{
		const Gu::TriangleT<PxU32>& T = ((const Gu::TriangleT<PxU32>*)getTrianglesFast())[triangleIndex];
		vref0 = T.v[0];
		vref1 = T.v[1];
		vref2 = T.v[2];
	}
	const PxVec3* PX_RESTRICT vertices = getVerticesFast();
	worldTri.verts[0] = worldMatrix.transform(vertices[vref0]);
	worldTri.verts[1] = worldMatrix.transform(vertices[vref1]);
	worldTri.verts[2] = worldMatrix.transform(vertices[vref2]);

	if(vertexIndices)
	{
		vertexIndices[0] = vref0;
		vertexIndices[1] = vref1;
		vertexIndices[2] = vref2;
	}

	if(adjacencyIndices)
	{
		if(mMesh.getAdjacencies())
		{
			adjacencyIndices[0] = mMesh.getAdjacencies()[triangleIndex*3 + 0];
			adjacencyIndices[1] = mMesh.getAdjacencies()[triangleIndex*3 + 1];
			adjacencyIndices[2] = mMesh.getAdjacencies()[triangleIndex*3 + 2];
		}
		else
		{
			adjacencyIndices[0] = 0xffffffff;
			adjacencyIndices[1] = 0xffffffff;
			adjacencyIndices[2] = 0xffffffff;
		}
	}
}

PX_FORCE_INLINE void Gu::TriangleMesh::getLocalTriangle(PxTriangle& localTri, PxTriangleID triangleIndex) const
{
	PxU32 vref0, vref1, vref2;
	if(mMesh.has16BitIndices())
	{
		const Gu::TriangleT<PxU16>& T = ((const Gu::TriangleT<PxU16>*)getTrianglesFast())[triangleIndex];
		vref0 = T.v[0];
		vref1 = T.v[1];
		vref2 = T.v[2];
	}
	else
	{
		const Gu::TriangleT<PxU32>& T = ((const Gu::TriangleT<PxU32>*)getTrianglesFast())[triangleIndex];
		vref0 = T.v[0];
		vref1 = T.v[1];
		vref2 = T.v[2];
	}
	const PxVec3* PX_RESTRICT vertices = getVerticesFast();
	localTri.verts[0] = vertices[vref0];
	localTri.verts[1] = vertices[vref1];
	localTri.verts[2] = vertices[vref2];
}

}

#endif
