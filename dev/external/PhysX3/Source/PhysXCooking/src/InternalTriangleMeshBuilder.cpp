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

#include "GuRTreeCooking.h"
#include "InternalTriangleMeshBuilder.h"
#include "GuEdgeList.h"

using namespace physx;
using namespace Gu;

InternalTriangleMeshBuilder::InternalTriangleMeshBuilder(InternalTriangleMesh* m, const PxCookingParams& params) :
	mesh(m),
	edgeList(NULL),
	mParams(params)
{
}

InternalTriangleMeshBuilder::~InternalTriangleMeshBuilder()
{
	releaseEdgeList();
}

void InternalTriangleMeshBuilder::remapTopology(const PxU32* order)
{
	PX_ASSERT(mesh);

	Gu::InternalTriangleMeshData& data = mesh->mData;

	if(!data.mNumTriangles)
		return;

	// Remap one array at a time to limit memory usage

	Gu::TriangleT<PxU32>* newTopo = (Gu::TriangleT<PxU32>*)PX_ALLOC(data.mNumTriangles * sizeof(Gu::TriangleT<PxU32>), PX_DEBUG_EXP("Gu::TriangleT<PxU32>"));
	for(PxU32 i=0;i<data.mNumTriangles;i++)
		newTopo[i]	= ((Gu::TriangleT<PxU32>*)data.mTriangles)[order[i]];
	PX_FREE_AND_RESET(data.mTriangles);
	data.mTriangles = newTopo;

	if(mesh->mMaterialIndices)
	{
		PxMaterialTableIndex* newMat = PX_NEW(PxMaterialTableIndex)[data.mNumTriangles];
		for(PxU32 i=0;i<data.mNumTriangles;i++)
			newMat[i] = mesh->mMaterialIndices[order[i]];
		PX_DELETE_POD(mesh->mMaterialIndices);
		mesh->mMaterialIndices = newMat;
	}

	if(!mParams.suppressTriangleMeshRemapTable)
	{
		PxU32* newMap = PX_NEW(PxU32)[data.mNumTriangles];
		for(PxU32 i=0;i<data.mNumTriangles;i++)
		{
//			PxU32 index = order[i];
//			newMap[index] = mesh->mFaceRemap ? mesh->mFaceRemap[i] : i;
			newMap[i] = mesh->mFaceRemap ? mesh->mFaceRemap[order[i]] : order[i];	// PT: fixed August, 15, 2004
		}
		PX_DELETE_POD(mesh->mFaceRemap);
		mesh->mFaceRemap = newMap;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct RTreeCookerRemap : RTreeCooker::RemapCallback 
{
	PxU32 mNumTrisPerLeaf;
	PxU32 mNumTris;
	RTreeCookerRemap(PxU32 numTris, PxU32 numTrisPerLeaf) : mNumTrisPerLeaf(numTrisPerLeaf), mNumTris(numTris)
	{
	}

	virtual void remap(PxU32* val, PxU32 start, PxU32 leafCount)
	{
		PX_ASSERT(leafCount > 0);
		PX_ASSERT(leafCount <= 16); // sanity check
		PX_ASSERT(start < mNumTris);
		PX_ASSERT(start+leafCount <= mNumTris);
		PX_ASSERT(val);
		LeafTriangles lt;
		// here we remap from ordered leaf index in the rtree to index in post-remap in triangles
		// this post-remap will happen later
		lt.SetData(leafCount, start);
		*val = lt.Data;
	}
};

bool InternalTriangleMeshBuilder::createRTree()
{
	mesh->setupMeshInterface();

	Gu::InternalTriangleMeshData& triData = mesh->mData;

	Array<PxU32> resultPermute;
	const PxU32 numTrisPerLeaf = 4;
	Gu::RTree& rtree = const_cast<Gu::RTree&>(mesh->getOpcodeModel().mRTree);

	RTreeCookerRemap rc(triData.mNumTriangles, numTrisPerLeaf);
	RTreeCooker::buildFromTriangles(
		rtree,
		triData.mVertices, triData.mNumVertices,
		(triData.mFlags & PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES) ? reinterpret_cast<PxU16*>(triData.mTriangles) : NULL,
		!(triData.mFlags & PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES) ? reinterpret_cast<PxU32*>(triData.mTriangles) : NULL,
		triData.mNumTriangles, numTrisPerLeaf, resultPermute, &rc);

	PX_ASSERT(resultPermute.size() == triData.mNumTriangles);

	remapTopology(resultPermute.begin());

	return true;
}

#include "PsSort.h"

struct RemapVertex
{
	PxVec3 vert;
	PxU32 index;
};

struct RemapTriangle
{
	PxU32 v0;
	PxU32 v1;
	PxU32 v2;
	PxU32 index;
};

struct SortVertsPredicate
{
	SortVertsPredicate()
	{
	}

	bool operator()(const RemapVertex& v0, const RemapVertex& v1) const
	{
		if(v0.vert.x < v1.vert.x)
			return true;
		else if (v0.vert.x == v1.vert.x)
		{
			if(v0.vert.y < v1.vert.y)
				return true;
			else if (v0.vert.y == v1.vert.y)
			{
				if(v0.vert.z < v1.vert.z)
					return true;
			}
		}
		return false;
	}
};

static void vertexWelding(PxVec3* vertices, PxU32& numVertices, Gu::TriangleT<PxU32>* triangles, PxU32& numTriangles, PxU32* triangleRemapTable,
						  const PxMeshPreprocessingFlags meshPreprocessParams, const PxF32 meshWeldTolerance)
{
	if(!(meshPreprocessParams & PxMeshPreprocessingFlag::eWELD_VERTICES))
		return;

	if(meshWeldTolerance == 0.f)
	{
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "TriangleMesh: Enable mesh welding with 0 weld tolerance!");
		return;
	}

	const PxF32 weldTolerance = 1.0f / meshWeldTolerance;
	//(1) snap to grid
	RemapVertex* tmpVertices = (RemapVertex*)PX_ALLOC(numVertices * sizeof(RemapVertex), PX_DEBUG_EXP("RemapVertex"));
	PxI32* remapIndex = (PxI32*)PX_ALLOC(numVertices * sizeof(PxI32), PX_DEBUG_EXP("Remap"));
	
	for(PxU32 a = 0; a < numVertices; ++a)
	{
		PxVec3 clampedVert = vertices[a];
		//const PxF32 tempX = sign(clampedVert.x)*((PxF32)((PxU32)((abs(clampedVert.x)*100.f)+0.5f)));
		clampedVert.x = PxFloor(clampedVert.x*weldTolerance + 0.5f);///100.f;
		clampedVert.y = PxFloor(clampedVert.y*weldTolerance + 0.5f);///100.f;
		clampedVert.z = PxFloor(clampedVert.z*weldTolerance + 0.5f);///100.f;
		tmpVertices[a].vert = clampedVert;
		tmpVertices[a].index = a;
		remapIndex[a] = a;
	}

	SortVertsPredicate pred;
	Ps::sort<RemapVertex, SortVertsPredicate>(tmpVertices, numVertices, pred);

	for(PxU32 a = 1; a < numVertices; ++a)
	{
		if(tmpVertices[a-1].vert == tmpVertices[a].vert)
		{
			//PX_ASSERT(tmpVertices[a-1].index == remapIndex[tmpVertices[a-1].index]);
			remapIndex[tmpVertices[a].index] = remapIndex[tmpVertices[a-1].index];
		}
	}

	//remap the triangle index and remove degenerated triangle
	for(PxU32 a = 0; a < numTriangles; ++a)
	{
		//PxU32 origIndex0 = triangles[a].v[0], origIndex1 = triangles[a].v[1], origIndex2 = triangles[a].v[2];

		triangles[a].v[0] = remapIndex[triangles[a].v[0]];
		triangles[a].v[1] = remapIndex[triangles[a].v[1]];
		triangles[a].v[2] = remapIndex[triangles[a].v[2]];
#if 1
		//sliver or just a vertex
		if(triangles[a].v[0]==triangles[a].v[1] || triangles[a].v[0] == triangles[a].v[2] || triangles[a].v[1] == triangles[a].v[2])
		{
			//get rid of this triangle
			triangles[a] = triangles[numTriangles-1];
			triangleRemapTable[a] = triangleRemapTable[numTriangles-1];
			numTriangles--;
			a--;
		}
#endif
	}

	
	//build add a dead vertex list and alive vertex list

	PxU32* dead = (PxU32*)PX_ALLOC(numVertices * sizeof(PxU32), PX_DEBUG_EXP("RemapDead"));
	PxU32* alive = (PxU32*)PX_ALLOC(numVertices * sizeof(PxU32), PX_DEBUG_EXP("RemapAlive"));
	
	PxU32 aliveCount=0, deadCount = 0;
	for(PxU32 a=0; a<numVertices; ++a)
	{
		if(remapIndex[a] == a)
		{
			//add to the alive list
			alive[aliveCount++] = a;
		}
		else
		{
			dead[deadCount++] = a;
		}
	}
	//Force the last deadCount to be past end of array
	dead[deadCount] = numVertices;


	PxI32 start = 0;
	PxI32 end = aliveCount;
	PxI32 deadIndex=0;

	for(PxU32 a = 0; a < numVertices; ++a)
	{
		remapIndex[a] = a;
	}

	while(start < end)
	{
		
		if(alive[start] > dead[deadIndex])
		{
			remapIndex[alive[--end]] = dead[deadIndex++];
		}
		else
		{
			start++;
		}
		
	}

	if(meshPreprocessParams & PxMeshPreprocessingFlag::eREMOVE_UNREFERENCED_VERTICES)
	{
		//remove duplicate vertices
		for(PxU32 a=0; a <numVertices; ++a)
		{
			if(a != remapIndex[a])
			{
				vertices[remapIndex[a]] = vertices[a];
			}
		}
	
		numVertices = aliveCount;
		

		//remap the triangle index again
		for(PxU32 a = 0; a < numTriangles; ++a)
		{
			triangles[a].v[0] = remapIndex[triangles[a].v[0]];
			triangles[a].v[1] = remapIndex[triangles[a].v[1]];
			triangles[a].v[2] = remapIndex[triangles[a].v[2]];
		}
	}
	////verify
	//for(PxU32 a = 0; a< numTriangles; ++a)
	//{
	//	PX_ASSERT(triangles[a].v[0] < numVertices);
	//	PX_ASSERT(triangles[a].v[1] < numVertices);
	//	PX_ASSERT(triangles[a].v[2] < numVertices);
	//}
	PX_FREE(dead);
	PX_FREE(alive);
	PX_FREE(tmpVertices);
	PX_FREE(remapIndex);
}

struct SortTrianglePredicate
{
	SortTrianglePredicate()
	{
	}

	bool operator()(RemapTriangle& triangle0, RemapTriangle& triangle1) const
	{
		if(triangle0.v0 < triangle1.v0)
			return true;
		else if (triangle0.v0 == triangle1.v0)
		{
			if(triangle0.v1 < triangle1.v1)
				return true;
			else if (triangle0.v1 == triangle1.v1)
			{
				if(triangle0.v2< triangle1.v2)
					return true;
			}
		}
		return false;
	}
};

static void removeDuplicatedTriangles(Gu::TriangleT<PxU32>* triangles, PxU32& numTriangles, PxVec3* verts, PxU32 /*numVerts*/, PxU32* triangleRemapTable, PxU32 meshPreprocessParams)
{
#if 0
	//This was already done inside the "detectDuplicateVertices"
	//get rid of degenerated triangles(silver/vertex)
	for(PxU32 a = 0; a < numTriangles; ++a)
	{
		//sliver or just a vertex
		if(triangles[a].v[0]==triangles[a].v[1] || triangles[a].v[0] == triangles[a].v[2] || triangles[a].v[1] == triangles[a].v[2])
		{
			//get rid of this triangle
			triangles[a] = triangles[numTriangles-1];
			triangleRemapTable[a] = triangleRemapTable[numTriangles-1];
			numTriangles--;
			a--;
		}
	}
#endif

	if(!(meshPreprocessParams & PxMeshPreprocessingFlag::eREMOVE_DUPLICATED_TRIANGLES))
		return;

	//get rid of the duplicate triangles
	RemapTriangle* remapTriangles =  (RemapTriangle*)PX_ALLOC(numTriangles * sizeof(RemapTriangle), PX_DEBUG_EXP("RemapTriangle"));
	
	//sort the remap triangle by indices
	for(PxU32 a = 0; a<numTriangles; ++a)
	{
		PxU32 v0, v1, v2;
		PxU32 tV0 = triangles[a].v[0];
		PxU32 tV1 = triangles[a].v[1];
		PxU32 tV2 = triangles[a].v[2];
		if((tV0 > tV1) && (tV0 > tV2))
		{
			v0 = tV0;
			
			if(tV1 > tV2)
			{
				v1 = tV1;
				v2 = tV2;
			}
			else
			{
				v1 = tV2;
				v2 = tV1;
			}
		}
		else if(tV1 > tV2)
		{
			v0 = tV1;
			if(tV0 > tV2)
			{
				v1 = tV0;
				v2 = tV2;
			}
			else
			{
				v1 = tV2;
				v2 = tV0;
			}
		}
		else
		{
			v0 = tV2;
			if(tV0 > tV1)
			{
				v1 = tV0;
				v2 = tV1;
			}
			else
			{
				v1 = tV1;
				v2 = tV0;
			} 
		}
		PX_ASSERT(v0 > v1 && v1 > v2);
		remapTriangles[a].v0 = v0;
		remapTriangles[a].v1 = v1;
		remapTriangles[a].v2 = v2;
		remapTriangles[a].index = a;

	}

	const SortTrianglePredicate triPred;
	Ps::sort<RemapTriangle, SortTrianglePredicate>(remapTriangles, numTriangles, triPred);

		
	//build add a dead vertex list and alive vertex list
	PxU32* deadTriangles = (PxU32*)PX_ALLOC(numTriangles * sizeof(PxU32), PX_DEBUG_EXP("RemapDeadTriangle"));
	PxU32* aliveTriangles = (PxU32*)PX_ALLOC(numTriangles * sizeof(PxU32), PX_DEBUG_EXP("RemapAliveTriangle"));
	PxU32* remapTriangleIndices = (PxU32*)PX_ALLOC(numTriangles * sizeof(PxU32), PX_DEBUG_EXP("RemapTriangleIndices"));

	PxU32 aliveTriangleCount=1, deadTriangleCount = 0;
	aliveTriangles[0] = remapTriangles[0].index;
	
	for(PxU32 a = 1; a < numTriangles; ++a)
	{
		RemapTriangle& r0 = remapTriangles[a-1];
		RemapTriangle& r1 = remapTriangles[a];
		if(r0.v0==r1.v0 && r0.v1 == r1.v1 && r0.v2 == r1.v2)
		{
			//Check winding order/normals...

			PxVec3 v0 = verts[triangles[r1.index].v[0]];
			PxVec3 v1 = verts[triangles[r1.index].v[1]];
			PxVec3 v2 = verts[triangles[r1.index].v[2]];

			PxVec3 n = (v1 - v0).cross(v2 - v0);

			PxVec3 v00 = verts[triangles[r0.index].v[0]];
			PxVec3 v10 = verts[triangles[r0.index].v[1]];
			PxVec3 v20 = verts[triangles[r0.index].v[2]];

			PxVec3 n0 = (v10 - v00).cross(v20 - v00);

			if(n0.dot(n) > 0.f)
			{
				deadTriangles[deadTriangleCount++] = r1.index;
			}
			else
			{
				//These are facing opposite directions so must be double-sided surface
				aliveTriangles[aliveTriangleCount++]=r1.index;
			}
		}
		else
		{
			aliveTriangles[aliveTriangleCount++]=r1.index;
		}
	}

	deadTriangles[deadTriangleCount] = numTriangles;
	

	Ps::sort(deadTriangles, deadTriangleCount);
	Ps::sort(aliveTriangles, aliveTriangleCount);

	for(PxU32 a = 0; a < numTriangles; ++a)
	{
		remapTriangleIndices[a] = a;
	}

	PxU32 startTriangle = 0;
	PxU32 endTriangle = aliveTriangleCount;
	PxU32 deadTriangleIndex=0;
	while(startTriangle < endTriangle)
	{
		if(aliveTriangles[startTriangle] > deadTriangles[deadTriangleIndex])
		{
			PX_ASSERT(deadTriangles[deadTriangleIndex] < aliveTriangleCount);
			remapTriangleIndices[aliveTriangles[--endTriangle]] = deadTriangles[deadTriangleIndex++];
		}
		else
		{
			startTriangle++;
		}
	}

	for(PxU32 a=0; a <numTriangles; ++a)
	{
		if(a != remapTriangleIndices[a])
		{
			PX_ASSERT(remapTriangleIndices[a] < aliveTriangleCount);
			triangles[remapTriangleIndices[a]] = triangles[a];
			triangleRemapTable[remapTriangleIndices[a]] = triangleRemapTable[a];
		}
	}

	numTriangles = aliveTriangleCount;

	
	PX_FREE(remapTriangles);
	PX_FREE(deadTriangles);
	PX_FREE(aliveTriangles);
	PX_FREE(remapTriangleIndices);
}

static void validateMesh(const Gu::TriangleT<PxU32>* triangles, const PxU32 numTriangles, const PxVec3* vertices, const PxU32 numVertices, const PxF32 meshWeldTolerance)
{
	if(meshWeldTolerance == 0.f)
		return;

	//snap to grid validation
	const PxF32 weldTolerance =  1.f / meshWeldTolerance;
	
	RemapVertex* tmpVertices = (RemapVertex*)PX_ALLOC(numVertices * sizeof(RemapVertex), PX_DEBUG_EXP("RemapVertex"));
	PxU32* refCount = (PxU32*)PX_ALLOC(numVertices * sizeof(PxU32), PX_DEBUG_EXP("RefCount"));
	
	for(PxU32 a = 0; a < numVertices; ++a)
	{
		PxVec3 clampedVert = vertices[a];
		clampedVert.x = PxFloor(clampedVert.x*weldTolerance + 0.5f);
		clampedVert.y = PxFloor(clampedVert.y*weldTolerance + 0.5f);
		clampedVert.z = PxFloor(clampedVert.z*weldTolerance + 0.5f);
		tmpVertices[a].vert = clampedVert;
		tmpVertices[a].index = a;
		refCount[a] = 0;
	}

	SortVertsPredicate pred;
	Ps::sort<RemapVertex, SortVertsPredicate>(tmpVertices, numVertices, pred);

	for(PxU32 a = 0; a < numTriangles; ++a)
	{
		refCount[triangles[a].v[0]]++;
		refCount[triangles[a].v[1]]++;
		refCount[triangles[a].v[2]]++;
	}



	PxReal meshWeldToleranceSq = meshWeldTolerance * meshWeldTolerance;

	bool notWeld = false;
	for(PxU32 a = 1; a < numVertices; ++a)
	{
		if(tmpVertices[a-1].vert == tmpVertices[a].vert)
		{
			if(refCount[tmpVertices[a].index] > 0 && 
				refCount[tmpVertices[a-1].index] > 0)
			{
				//get the original vertices
				const PxVec3 v0 = vertices[tmpVertices[a-1].index];
				const PxVec3 v1 = vertices[tmpVertices[a].index];
				const PxReal dif = (v0 - v1).magnitudeSquared();
				if(dif < meshWeldToleranceSq)
				{
					notWeld = true;
					break;
				}
			}
		}
	}

	if(notWeld)
	{
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "TriangleMesh: the mesh isn't welded!");
	}

	PX_FREE(tmpVertices);
	PX_FREE(refCount);
}




struct Indices
{
	PxU32 mRef[3];

	PX_FORCE_INLINE bool operator!=(const Indices&v) const	{ return mRef[0] != v.mRef[0] || mRef[1] != v.mRef[1] || mRef[2] != v.mRef[2]; }
};

static PX_FORCE_INLINE PxU32 getHashValue(const PxVec3& v)
{
	const PxU32* h = (const PxU32*)(&v.x);
	const PxU32 f = (h[0]+h[1]*11-(h[2]*17)) & 0x7fffffff;	// avoid problems with +-0
	return (f>>22)^(f>>12)^(f);
}

static PX_FORCE_INLINE PxU32 getHashValue(const Indices& v)
{
//	const PxU32* h = v.mRef;
//	const PxU32 f = (h[0]+h[1]*11-(h[2]*17)) & 0x7fffffff;	// avoid problems with +-0
//	return (f>>22)^(f>>12)^(f);

	PxU32 a = v.mRef[0];
	PxU32 b = v.mRef[1];
	PxU32 c = v.mRef[2];
	a=a-b;  a=a-c;  a=a^(c >> 13);
	b=b-c;  b=b-a;  b=b^(a << 8); 
	c=c-a;  c=c-b;  c=c^(b >> 13);
	a=a-b;  a=a-c;  a=a^(c >> 12);
	b=b-c;  b=b-a;  b=b^(a << 16);
	c=c-a;  c=c-b;  c=c^(b >> 5);
	a=a-b;  a=a-c;  a=a^(c >> 3);
	b=b-c;  b=b-a;  b=b^(a << 10);
	c=c-a;  c=c-b;  c=c^(b >> 15);
	return c;
}

class MeshCleaner
{
	public:
		MeshCleaner(PxU32 nbVerts, const PxVec3* verts, PxU32 nbTris, const PxU32* indices, PxF32 meshWeldTolerance);
		~MeshCleaner();

		PxU32	mNbVerts;
		PxU32	mNbTris;
		PxVec3*	mVerts;
		PxU32*	mIndices;
		PxU32*	mRemap;
};

MeshCleaner::MeshCleaner(PxU32 nbVerts, const PxVec3* srcVerts, PxU32 nbTris, const PxU32* srcIndices, PxF32 meshWeldTolerance)
{
	PxVec3* cleanVerts = (PxVec3*)PX_ALLOC(sizeof(PxVec3)*nbVerts, PX_DEBUG_EXP("MeshCleaner"));
	PX_ASSERT(cleanVerts);

	PxU32* indices = (PxU32*)PX_ALLOC(sizeof(PxU32)*nbTris*3, PX_DEBUG_EXP("MeshCleaner"));

	PxU32* remapTriangles = (PxU32*)PX_ALLOC(sizeof(PxU32)*nbTris, PX_DEBUG_EXP("MeshCleaner"));

	PxU32* vertexIndices = NULL;
	if(meshWeldTolerance!=0.0f)
	{
		vertexIndices = (PxU32*)PX_ALLOC(sizeof(PxU32)*nbVerts, PX_DEBUG_EXP("MeshCleaner"));
		const PxF32 weldTolerance = 1.0f / meshWeldTolerance;
		// snap to grid
		for(PxU32 i=0; i<nbVerts; i++)
		{
			vertexIndices[i] = i;
			cleanVerts[i] = PxVec3(	PxFloor(srcVerts[i].x*weldTolerance + 0.5f),
									PxFloor(srcVerts[i].y*weldTolerance + 0.5f),
									PxFloor(srcVerts[i].z*weldTolerance + 0.5f));
		}
	}
	else
	{
		memcpy(cleanVerts, srcVerts, nbVerts*sizeof(PxVec3));
	}

	const PxU32 maxNbElems = PxMax(nbTris, nbVerts);
	const PxU32 hashSize = Ps::nextPowerOfTwo(maxNbElems);
	const PxU32 hashMask = hashSize-1;
	PxU32* hashTable = (PxU32*)PX_ALLOC(sizeof(PxU32)*(hashSize + maxNbElems), PX_DEBUG_EXP("MeshCleaner"));
	PX_ASSERT(hashTable);
	memset(hashTable, 0xff, hashSize * sizeof(PxU32));
	PxU32* const next = hashTable + hashSize;

	PxU32* remapVerts = (PxU32*)PX_ALLOC(sizeof(PxU32)*nbVerts, PX_DEBUG_EXP("MeshCleaner"));
	memset(remapVerts, 0xff, nbVerts * sizeof(PxU32));

	for(PxU32 i=0;i<nbTris*3;i++)
	{
		const PxU32 vref = srcIndices[i];
		if(vref<nbVerts)
			remapVerts[vref] = 0;
	}

	PxU32 nbCleanedVerts = 0;
	for(PxU32 i=0;i<nbVerts;i++)
	{
		if(remapVerts[i]==0xffffffff)
			continue;

		const PxVec3& v = cleanVerts[i];
		const PxU32 hashValue = getHashValue(v) & hashMask;
		PxU32 offset = hashTable[hashValue];

		while(offset!=0xffffffff && cleanVerts[offset]!=v)
			offset = next[offset];

		if(offset==0xffffffff)
		{
			remapVerts[i] = nbCleanedVerts;
			cleanVerts[nbCleanedVerts] = v;
			if(vertexIndices)
				vertexIndices[nbCleanedVerts] = i;
			next[nbCleanedVerts] = hashTable[hashValue];
			hashTable[hashValue] = nbCleanedVerts++;
		}
		else remapVerts[i] = offset;
	}

	PxU32 nbCleanedTris = 0;
	for(PxU32 i=0;i<nbTris;i++)
	{
		PxU32 vref0 = *srcIndices++;
		PxU32 vref1 = *srcIndices++;
		PxU32 vref2 = *srcIndices++;
		if(vref0>=nbVerts || vref1>=nbVerts || vref2>=nbVerts)
			continue;

		// PT: you can still get zero-area faces when the 3 vertices are perfectly aligned
		const PxVec3& p0 = srcVerts[vref0];
		const PxVec3& p1 = srcVerts[vref1];
		const PxVec3& p2 = srcVerts[vref2];
		const float area2 = ((p0 - p1).cross(p0 - p2)).magnitudeSquared();
		if(area2==0.0f)
			continue;

		vref0 = remapVerts[vref0];
		vref1 = remapVerts[vref1];
		vref2 = remapVerts[vref2];
		if(vref0==vref1 || vref1==vref2 || vref2==vref0)
			continue;

		indices[nbCleanedTris*3+0] = vref0;
		indices[nbCleanedTris*3+1] = vref1;
		indices[nbCleanedTris*3+2] = vref2;
		nbCleanedTris++;
	}
	PX_FREE(remapVerts);

	PxU32 nbToGo = nbCleanedTris;
	nbCleanedTris = 0;
	memset(hashTable, 0xff, hashSize * sizeof(PxU32));

	Indices* I = reinterpret_cast<Indices*>(indices);
	bool idtRemap = true;
	for(PxU32 i=0;i<nbToGo;i++)
	{
		const Indices& v = I[i];
		const PxU32 hashValue = getHashValue(v) & hashMask;
		PxU32 offset = hashTable[hashValue];

		while(offset!=0xffffffff && I[offset]!=v)
			offset = next[offset];

		if(offset==0xffffffff)
		{
			remapTriangles[nbCleanedTris] = i;
			if(i!=nbCleanedTris)
				idtRemap = false;
			I[nbCleanedTris] = v;
			next[nbCleanedTris] = hashTable[hashValue];
			hashTable[hashValue] = nbCleanedTris++;
		}
	}
	PX_FREE(hashTable);

	if(vertexIndices)
	{
		for(PxU32 i=0;i<nbCleanedVerts;i++)
			cleanVerts[i] = srcVerts[vertexIndices[i]];
		PX_FREE(vertexIndices);
	}
	mNbVerts	= nbCleanedVerts;
	mNbTris		= nbCleanedTris;
	mVerts		= cleanVerts;
	mIndices	= indices;
	if(idtRemap)
	{
		PX_FREE(remapTriangles);
		mRemap	= NULL;
	}
	else
	{
		mRemap	= remapTriangles;
	}
}

MeshCleaner::~MeshCleaner()
{
	PX_FREE_AND_RESET(mRemap);
	PX_FREE_AND_RESET(mIndices);
	PX_FREE_AND_RESET(mVerts);
}

/*	PX_INLINE void	StartProfile(PxU32& val)
	{
		__asm{
			cpuid
			rdtsc
			mov		ebx, val
			mov		[ebx], eax
		}
	}

	PX_INLINE void	EndProfile(PxU32& val)
	{
		__asm{
			cpuid
			rdtsc
			mov		ebx, val
			sub		eax, [ebx]
			mov		[ebx], eax
		}
	}*/

bool InternalTriangleMeshBuilder::cleanMesh()
{
	PX_ASSERT(mesh);

	Gu::InternalTriangleMeshData& data = mesh->mData;

	// PT: new (faster) mesh cleaning code
	const bool useNewWelding = true;
	{
		PxF32 meshWeldTolerance = 0.0f;
		if(useNewWelding && mParams.meshPreprocessParams & PxMeshPreprocessingFlag::eWELD_VERTICES)
		{
			if(mParams.meshWeldTolerance == 0.f)
			{
				Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "TriangleMesh: Enable mesh welding with 0 weld tolerance!");
			}
			else
			{
				meshWeldTolerance = mParams.meshWeldTolerance;
			}
		}
//PxU32 time;
//StartProfile(time);
		MeshCleaner cleaner(data.mNumVertices, data.mVertices, data.mNumTriangles, (const PxU32*)data.mTriangles, meshWeldTolerance);
//EndProfile(time);
//printf("New: %d\n", time/1024);
		if(!cleaner.mNbTris)
			return false;

		// PT: deal with the remap table
		{
			PX_DELETE_POD(mesh->mFaceRemap); 

			// PT: TODO: optimize this
			if(cleaner.mRemap && mParams.suppressTriangleMeshRemapTable == false)
			{
				const PxU32 newNbTris = cleaner.mNbTris;
				mesh->mFaceRemap = PX_NEW(PxU32)[newNbTris];
				PxMemCopy(mesh->mFaceRemap, cleaner.mRemap, newNbTris*sizeof(PxU32));

				// Remap material array
				if(mesh->mMaterialIndices)
				{
					PxMaterialTableIndex* tmp = PX_NEW(PxMaterialTableIndex)[newNbTris];
					for(PxU32 i=0;i<newNbTris;i++)
						tmp[i] = mesh->mMaterialIndices[mesh->mFaceRemap[i]];

					PX_DELETE_POD(mesh->mMaterialIndices);
					mesh->mMaterialIndices = tmp;
				}
			}
		}

		// PT: deal with geometry
		{
			if(data.mNumVertices!=cleaner.mNbVerts)
			{
				PX_FREE_AND_RESET(data.mVertices);
				mesh->allocateVertices(cleaner.mNbVerts);
			}
			PxMemCopy(data.mVertices, cleaner.mVerts, data.mNumVertices*sizeof(PxVec3));
		}

		// PT: deal with topology
		{
			PX_ASSERT(!(data.mFlags & PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES));
			if(data.mNumTriangles!=cleaner.mNbTris)
			{
				PX_FREE_AND_RESET(data.mTriangles);
				mesh->allocateTriangles(cleaner.mNbTris, true);
			}

			const float testLength = 500.0f*500.0f*mParams.scale.length*mParams.scale.length;
			bool bigTriangle = false;
			const PxVec3* v = data.mVertices;
			for(PxU32 i=0;i<data.mNumTriangles;i++)
			{
				const PxU32 vref0 = cleaner.mIndices[i*3+0];
				const PxU32 vref1 = cleaner.mIndices[i*3+1];
				const PxU32 vref2 = cleaner.mIndices[i*3+2];
				PX_ASSERT(vref0!=vref1 && vref0!=vref2 && vref1!=vref2);

				reinterpret_cast<Gu::TriangleT<PxU32>*>(data.mTriangles)[i].v[0] = vref0;
				reinterpret_cast<Gu::TriangleT<PxU32>*>(data.mTriangles)[i].v[1] = vref1;
				reinterpret_cast<Gu::TriangleT<PxU32>*>(data.mTriangles)[i].v[2] = vref2;

				if(		(v[vref0] - v[vref1]).magnitudeSquared() >= testLength
					||	(v[vref1] - v[vref2]).magnitudeSquared() >= testLength
					||	(v[vref2] - v[vref0]).magnitudeSquared() >= testLength
					)
					bigTriangle = true;
			}
			if(bigTriangle)
				Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "TriangleMesh: triangles are too big, reduce their size to increase simulation stability!");
		}
	}

	// Additional cleanup for PCM
	if(!useNewWelding && mParams.meshPreprocessParams)
	{
		PxU32* triangleRemapTable = (PxU32*)PX_ALLOC(sizeof(PxU32)*data.mNumTriangles, "RemapTable");
		for(PxU32 a = 0; a < data.mNumTriangles; ++a)
			triangleRemapTable[a] = mesh->mFaceRemap ? mesh->mFaceRemap[a] : a;

//PxU32 time;
//StartProfile(time);
		vertexWelding(data.mVertices, data.mNumVertices, (Gu::TriangleT<PxU32>*)data.mTriangles, data.mNumTriangles, triangleRemapTable, mParams.meshPreprocessParams, mParams.meshWeldTolerance);
		removeDuplicatedTriangles((Gu::TriangleT<PxU32>*)data.mTriangles, data.mNumTriangles, data.mVertices, data.mNumVertices, triangleRemapTable, mParams.meshPreprocessParams);
		validateMesh((Gu::TriangleT<PxU32>*)data.mTriangles, data.mNumTriangles, data.mVertices, data.mNumVertices, mParams.meshWeldTolerance);
//EndProfile(time);
//printf("Old: %d\n", time/1024);

		if(mParams.suppressTriangleMeshRemapTable == false)
		{
			bool valid=false;
			for(PxU32 i=0;i<data.mNumTriangles;i++)
			{
				if(triangleRemapTable[i]!=i)
				{
					valid=true;
					break;
				}
			}
			if(valid)
			{
				if(mesh->mFaceRemap)
					PX_DELETE_POD(mesh->mFaceRemap);
				mesh->mFaceRemap = PX_NEW(PxU32)[data.mNumTriangles];
				PxMemCopy(mesh->mFaceRemap, triangleRemapTable, data.mNumTriangles*sizeof(PxU32));
			}
		}
		PX_FREE(triangleRemapTable);
	}
	return true;
}

	//2) build edge table:
	class Edge : public Ps::UserAllocated
	{
	public:
		PxU32 vertex1, vertex2;
		//PxReal angle;
		PxU32 trig1Ref, trig2Ref;	//2 hi bits are edge index: 0, 1, or 2. lower bits are triangle index.

		void init(PxU32 v1, PxU32 v2, PxU32 t, PxU32 edgeIndex)
		{	
			//sort the vertex indices, needed for redundancy removal:
			if (v1 < v2)
			{
				vertex1 = v1;
				vertex2 = v2;
			}
			else
			{
				vertex1 = v2;
				vertex2 = v1;
			}
			trig1Ref = (edgeIndex << 30)| t;
			trig2Ref = 0xffffffff;
		}
		bool differentSideEffect(Edge & other, InternalTriangleMeshBuilder & mesh)
		{
			if (vertex1 != other.vertex1 || vertex2 != other.vertex2)
			{
				return true;
			}
			else
			{
				//same, other will be deleted:
				if (trig2Ref == 0xffffffff)
					trig2Ref = other.trig1Ref;
				else
				{
					//error, an edge is shared between 3 or more trigs!
					//PxCollision::Physics::error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__);
					//TODO: log a warning.
					//					static int badIndex = 0;

					//					printf("%d-%d: bad edge #%d!\n", vertex1, vertex2, badIndex++);

					//ignore this edge:
					PxU32 trig2Index = other.trig1Ref & 0x3FFFFFFF;	//ignore top 2 bits.
					PxU32 trig2EdgeIndex = other.trig1Ref >> 30;	//ignore top 2 bits.
					mesh.mesh->setTrigSharedEdgeFlag(trig2Index, trig2EdgeIndex);
				}
				return false;
			}
		}

		bool isSmaller(const Edge* e2)
		{
			const Edge* e1 = this;
			//sort edges:  greater(e1, e2) = { e1.v1 < e2.v1 ? e1 : (e1.v2 < e2.v2 ? e1 : e1 : e2)  : e2 }	//less, equal, greater
			//return s1 < s2 ? 1 : 0;
			if (e1->vertex1 < e2->vertex1)
				return true;
			else if (e1->vertex1 > e2->vertex1)
				return false;
			else
			{
				//1st vertex the same, sort based on second:
				return (e1->vertex2 < e2->vertex2);
			}
		}
		static void quickSort(Edge *entries, int l, int r)
		{
			int i,j, mi;
			Edge k, m;

			i = l; j = r; mi = (l + r)/2;
			m = entries[mi];
			while (i <= j) 
			{
				while(entries[i].isSmaller(&m)) i++;
				while(m.isSmaller(&entries[j])) j--;
				if (i <= j) 
				{
					k = entries[i]; entries[i] = entries[j]; entries[j] = k;
					i++; j--;
				}
			}
			if (l < j) quickSort(entries, l, j);
			if (i < r) quickSort(entries, i, r);
		}

		void processUniqueEdge(InternalTriangleMeshBuilder & mesh, bool buildTriangleAdjacencies)
		{
			// store the triangle adjacencies if required 
			if(buildTriangleAdjacencies)
			{
				PxU32 trig1Index = trig1Ref & 0x3FFFFFFF;	//ignore top 2 bits.
				PxU32 trig1EdgeIndex = trig1Ref >> 30;	
				if(trig1EdgeIndex == 1)
				{
					trig1EdgeIndex = 2;
				}
				else
				{
					if(trig1EdgeIndex == 2)
					{
						trig1EdgeIndex = 1;
					}
				}
				PxU32 trig2Index = 0xffffffff;
				PxU32 trig2EdgeIndex = 0xffffffff; 

				if(trig2Ref != 0xffffffff)
				{
					trig2Index = trig2Ref & 0x3FFFFFFF;	//ignore top 2 bits.
					trig2EdgeIndex = trig2Ref >> 30;	
					if(trig2EdgeIndex == 1)
					{
						trig2EdgeIndex = 2;
					}
					else
					{
						if(trig2EdgeIndex == 2)
						{
							trig2EdgeIndex = 1;
						}
					}
				}

				mesh.mesh->setTriangleAdjacency(trig1Index, trig2Index,trig1EdgeIndex);
				if(trig2Index != 0xffffffff)
				{
					mesh.mesh->setTriangleAdjacency(trig2Index, trig1Index,trig2EdgeIndex);
				}
			}

			//ML : this logic is redundant now, we only use active edge flag in all contact gen, but not concave flag 
#if	0
			/*
			rules:
			If the edge is not shared between 2 trigs, it is a boundary edge, and we need to use it for stabs. (i.e. don't flag to ignore)
			Else if it is shared:	
			* first flag to ignore in one of the trigs, the other trig will take care of its colldet.
			* make sure that the two adjacent trigs have the same winding, i.e. that the normal doesn't "flip" -- that is an error.
			if the angle between the two trigs is:
			0 : this is a flat edge which is unimportant for colldet, flag to ignore in other trig too.
			concave: this edge won't be collided with, flag to ignore in other trig too.
			convex: this will need to be collided with, so do nothing.

			Also, later:
			if a any of a vertex' adjacent edges are concave, it should be ignored.
			if a all of a vertex' adjacent edges are flat, it should be ignored.
			*/
			if (trig2Ref != 0xffffffff)
			{
				PxU32 trig1Index = trig1Ref & 0x3FFFFFFF;	//ignore top 2 bits.
				PxU32 trig1EdgeIndex = trig1Ref >> 30;	//ignore top 2 bits.
				PxU32 trig2Index = trig2Ref & 0x3FFFFFFF;	//ignore top 2 bits.
				PxU32 trig2EdgeIndex = trig2Ref >> 30;	//ignore top 2 bits.
				//shared edge
				//find the second trig, and flag this edge in it as ignored:
				mesh.mesh->setTrigSharedEdgeFlag(trig2Index, trig2EdgeIndex);

				// Compute normal on-the-fly
				const Gu::TriangleT<PxU32>* trigs = reinterpret_cast<const Gu::TriangleT<PxU32>*>(mesh.mesh->getTriangles());
				const PxPoint * vertices = mesh.mesh->getVertices();
				PxVec3 trig1Normal;
				{
					const PxPoint& v0 =vertices[trigs[trig1Index].v[0]];
					const PxPoint& v1 =vertices[trigs[trig1Index].v[1]];
					const PxPoint& v2 =vertices[trigs[trig1Index].v[2]];
					trig1Normal = (v1 - v0).cross(v2 - v0);
					trig1Normal.normalize();
				}
				PxVec3 trig2Normal;
				{
					const PxPoint& v0 =vertices[trigs[trig2Index].v[0]];
					const PxPoint& v1 =vertices[trigs[trig2Index].v[1]];
					const PxPoint& v2 =vertices[trigs[trig2Index].v[2]];
					trig2Normal = (v1 - v0).cross(v2 - v0);
					trig2Normal.normalize();
				}

				//find the concavity of the edge:
				//it is flat if the two trig normals' dot product is (almost) 1.
				//				PxVec3 & trig1N = mesh.extraTrigData[trig1Index].normal;
				const PxVec3& trig1N = trig1Normal;
				//				if (trig1N.dot(mesh.extraTrigData[trig2Index].normal) > 0.999f)//tolerance of +-2.56 degrees.
				//				if (trig1N.dot(mesh.extraTrigData[trig2Index].normal) > 1.0f - mesh.getConvexEdgeThreshold())//tolerance of +-2.56 degrees.
				if (trig1N.dot(trig2Normal) > 1.0f - mesh.mesh->getConvexEdgeThreshold())//tolerance of +-2.56 degrees.
				{
					//flat -- ignore it:
				mesh.mesh->setTrigSharedEdgeFlag(trig1Index, trig1EdgeIndex);
				}
				else
				{
					/*
					* pretend that trig1 is lying flat on the ground, with normal pointing upward.
					* trig2's vertex that is not on the common edge is now either above or below ground.
					*  if it is above ground, then:
					* trig2's normal should point toward trig1, across the edge, otherwise it has bad refersed winding.
					* the edge is concave
					*  else it is below ground, then: 
					* trig2's normal should point away from trig1 and the edge, otherwise it has bad refersed winding.
					* the edge is convex
					*/

					//find an edge of trig2 that points TO the vertex unused in the current edge.
					const Gu::TriangleT<PxU32>& trig2 = reinterpret_cast<const Gu::TriangleT<PxU32>*>(mesh.mesh->getTriangles())[trig2Index];
					PxU32 vertexOnEdge=0, trig2s3rdVertex=0;
					switch (trig2EdgeIndex)
					{
					case 0://0->1
						vertexOnEdge = 0;
						trig2s3rdVertex = 2;
						break;
					case 1://0->2
						vertexOnEdge = 0;
						trig2s3rdVertex = 1;
						break;
					case 2://1->2
						vertexOnEdge = 1;
						trig2s3rdVertex = 0;
						break;
					default:
						PX_ASSERT(0);
					}
					PxPoint trig2sEdgeTo3rdVertex = mesh.mesh->getVertices()[trig2.v[trig2s3rdVertex]] - mesh.mesh->getVertices()[trig2.v[vertexOnEdge]];
					bool concave = trig1N.dot(trig2sEdgeTo3rdVertex) > 0;
					if (concave)
					{
						//ignore it:
						mesh.mesh->setTrigSharedEdgeFlag(trig1Index, trig1EdgeIndex);
					}
				}
			}
#endif
		}
	};





// PT: eeerrrr.... isn't this whole function totally useless now???
void InternalTriangleMeshBuilder::createSharedEdgeData(bool buildTriangleAdjacencies)
{
	PX_ASSERT(mesh);

	PX_DELETE_POD(mesh->mData.mExtraTrigData); 
	PX_DELETE_POD(mesh->mAdjacencies);

	PxU32 nTrigs = mesh->getNumTriangles();
	const Gu::TriangleT<PxU32>* trigs = reinterpret_cast<const Gu::TriangleT<PxU32>*>(mesh->getTriangles());

	if(0x40000000 <= nTrigs)
	{
		//mesh is too big for this algo, need to be able to express trig indices in 30 bits, and still have an index reserved for "unused":
		Ps::getFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, "TriangleMesh: mesh is too big for this algo!");
		return;
	}

	mesh->mData.mExtraTrigData = PX_NEW(PxU8)[nTrigs];
	memset(mesh->mData.mExtraTrigData, 0, sizeof(PxU8)*nTrigs);

	if(buildTriangleAdjacencies)
	{
		mesh->mAdjacencies = PX_NEW(PxU32)[nTrigs*3];
		memset(mesh->mAdjacencies, 0, sizeof(PxU32)*nTrigs*3);		
	}

	/*
	we'd like to store, for each trig:
	*?? face normal (3 floats, saves a cross and a normalize)
	* edge type (just an ignore bit is enough; ignore concaves too!)
	* vertex flag: ignore bit.
	*/

/*	//1) Compute face normals, and init flags to zero.
//	const Gu::TriangleT<PxU32>* t = trigs;
	unsigned i = 0;
	const PxVec3 * vertices = mesh->getVertices();
	for(i = 0; i < nTrigs; i++)		
	{
//		const Gu::TriangleT<PxU32>& trig = *t++;//T = [a,b,c]

		//compute normal for trig
//		PxPoint trig1Normal;
//		const PxPoint& v0 = vertices[trig.v[0]];
//		const PxPoint& v1 = vertices[trig.v[1]];
//		const PxPoint& v2 = vertices[trig.v[2]];

//		const PxVec3 edge1MeshSpace = v1 - v0;
//		const PxVec3 edge2MeshSpace = v2 - v0;

		//compute face normal:
		PxU8& ed = mesh->mData.mExtraTrigData[i];
		ed = 0;
	}
*/

	/* would it be faster to sort the indices and not the data directly? (Less data to copy, more data to cache)
	struct EdgeIndex : public Ps::UserAllocated
	{
	PxU32 i;
	};
	EdgeIndex * edgeIndices = PX_NEW EdgeIndex[3 * nTrigs];
	//std::vector<Edge *, Ps::STLAllocator<Edge *>>edgeList;
	//edgeList.reserve(3 * nTrigs);
	*/
	PxU32 maxEdges = 3 * nTrigs;
	Edge * edges = PX_NEW(Edge)[maxEdges];
	Edge * edge = edges;
	const Gu::TriangleT<PxU32>* t = trigs;
	for (PxU32 i = 0; i < nTrigs; i++)
	{
		const Gu::TriangleT<PxU32>& trig = *t++;//T = [a,b,c]

		//edges are:
		// 0-->1, 0-->2, 1-->2
		edge->init(trig.v[0], trig.v[1],i, 0);
		edge++;
		edge->init(trig.v[0], trig.v[2],i, 1);
		edge++;
		edge->init(trig.v[1], trig.v[2],i, 2);
		edge++;
	}

	//3) sort edge table to make dupe removal simple
	//pros to pierre's radix sorter: probably fast
	//cons: can't handle strides, not typesafe, uses lots of memory, can't sort anything but 2 dwords, we don't need its temporal coherence here, for small meshes prolly slower than quicksort,
	//can't do weird compare that we use below.
	/*
	Ice::RadixSort sorter;	//could make static for coherence, but we don't need coherence.
	PxU32* Sorted0 = sorter.Sort(MinPosList0, nb0).GetRanks();
	*/
	//STL: sort(edgeList.start(), edgeList.end(), Edge::isSmaller);
	Edge::quickSort(edges, 0, maxEdges - 1);


	//go through sorted edge list and throw out duplicates:
	PxU32 lastValidEdgeIndex = 0;	//numEdges - 1
	for (PxU32 i = 1; i < maxEdges; i++)
	{
		if (edges[lastValidEdgeIndex].differentSideEffect(edges[i], *this))
		{
			lastValidEdgeIndex++;
			edges[lastValidEdgeIndex] = edges[i];
		}
	}

	//classify the edges -- TODO: this could be done as part of the prev. loop, with added confusion
	for (PxU32 i = 0; i <= lastValidEdgeIndex; i++)
	{
		edges[i].processUniqueEdge(*this, buildTriangleAdjacencies);
	}

	//initially all vertices are flagged to ignore them. (we assume them to be flat)
	//for all NONFLAT edges, incl boundary
	//unflag 2 vertices in up to 2 trigs as perhaps interesting
	//for all CONCAVE edges
	//flag 2 vertices in up to 2 trigs to ignore them.


	//delete all dynamic arrays!
	PX_DELETE_ARRAY(edges);
}

void InternalTriangleMeshBuilder::createEdgeList()
{
	Gu::EDGELISTCREATE create;
	create.NbFaces		= mesh->getNumTriangles();
	if(mesh->has16BitIndices())
	{
		create.DFaces		= NULL;
		create.WFaces		= (PxU16*)mesh->getTriangles();
	}
	else
	{
		create.DFaces		= (PxU32*)mesh->getTriangles();
		create.WFaces		= NULL;
	}
	create.FacesToEdges	= true;
	create.EdgesToFaces	= true;
	create.Verts		= mesh->getVertices();
	//create.Epsilon = 0.1f;
	//	create.Epsilon		= convexEdgeThreshold;
	edgeList = PX_NEW(Gu::EdgeListBuilder);
	if(!edgeList->Init(create))
	{
		PX_DELETE(edgeList);
		edgeList = 0;
	}
}

void InternalTriangleMeshBuilder::releaseEdgeList()
{
	PX_DELETE(edgeList);
	edgeList = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

