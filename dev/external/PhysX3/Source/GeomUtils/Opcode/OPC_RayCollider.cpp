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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PsIntrinsics.h"
#include "OPC_RayCollider.h"
#include "GuIntersectionRayBox.h"
#include "CmMemFetch.h"
#include "CmMatrix34.h"

using namespace physx;
using namespace Cm;
using namespace Gu;

class SimpleRayTriOverlap
{
public:
	PX_FORCE_INLINE SimpleRayTriOverlap(const PxVec3& origin, const PxVec3& dir, bool bothSides, PxReal geomEpsilon)
		: mOrigin(origin), mDir(dir), mBothSides(bothSides), mGeomEpsilon(geomEpsilon)
	{
	}

	PX_FORCE_INLINE Ps::IntBool overlap(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2, PxRaycastHit& hit) const
	{
		// AP: looks like this computation is based on "Fast Minimum Storage Ray Triangle Intersection" paper by Moeller et al
		const PxVec3 edge1 = vert1 - vert0;
		const PxVec3 edge2 = vert2 - vert0;

		const PxVec3 p = mDir.cross(edge2);
		const PxReal det = edge1.dot(p);
		const PxReal epsilon = mGeomEpsilon;

		if (!mBothSides)
		{
			if (det<epsilon)
				return Ps::IntFalse;

			//epsilon *= PxAbs(det); // scale the epsilon to improve precision

			const PxVec3 t = mOrigin - vert0;
			const PxReal u = t.dot(p); // (origin-v0).dot(dir x e2)

			if(u < -epsilon || u > det + epsilon)	
				return Ps::IntFalse;

			const PxVec3 q = t.cross(edge1);

			const PxReal v = mDir.dot(q);
			if(v < -epsilon || (u + v) > det + epsilon)	
				return Ps::IntFalse;

			const PxReal d = edge2.dot(q);
			if(d<-epsilon) // test if the ray intersection t is really negative
				return Ps::IntFalse;

			const float oneOverDet = 1.0f / det;
			hit.distance	= oneOverDet * d;
			hit.u			= oneOverDet * u;
			hit.v			= oneOverDet * v;
		} else
		{
			if (PxAbs(det)<epsilon)
				return Ps::IntFalse;
			PxReal invDet = 1.0f/det;

			const PxVec3 t = mOrigin - vert0;
			const PxReal u = t.dot(p)*invDet; // (origin-v0).dot(dir x e2)

			if(u < -epsilon || u > 1.0f + epsilon)	
				return Ps::IntFalse;

			const PxVec3 q = t.cross(edge1);

			const PxReal v = mDir.dot(q)*invDet;
			if(v < -epsilon || (u + v) > 1.0f + epsilon)	
				return Ps::IntFalse;

			const PxReal d = edge2.dot(q)*invDet;
			if(d<-epsilon) // test if the ray intersection t is really negative
				return Ps::IntFalse;

			hit.distance	= d;
			hit.u			= u;
			hit.v			= v;
		}

		return Ps::IntTrue;
	}

	PxVec3	mOrigin;
	PxVec3	mDir;
	bool	mBothSides;
	PxReal	mGeomEpsilon;
};

using Gu::RTree;

// This callback comes from RTree and decodes LeafTriangle indices stored in rtree into actual triangles
// This callback is needed because RTree doesn't know that it stores triangles since it's a general purpose spatial index
template <int tInflate>
struct RayRTreeCallback : public RTree::CallbackRaycast
{
	const RTreeMidphaseData& model;
	MeshHitCallback<PxRaycastHit>& outerCallback;
	PxU32 has16BitIndices;
	MemFetchPtr* mTris;
	const MemFetchPtr* mVerts;
	const PxVec3* mInflate;
	const SimpleRayTriOverlap rayCollider;
	PxReal maxT;
	PxRaycastHit closestHit;
	PxVec3 cv0, cv1, cv2;
	bool hadClosestHit;

	RayRTreeCallback(
		const RTreeMidphaseData& model, MeshHitCallback<PxRaycastHit>& callback,
		PxU32 has16BitIndices, MemFetchPtr* tris, MemFetchPtr* verts,
		const PxVec3& origin, const PxVec3& dir, PxReal maxT, bool bothSides, const PxVec3* inflate, PxReal geomEpsilon)
		:	model(model), outerCallback(callback), has16BitIndices(has16BitIndices),
			mTris(tris), mVerts(verts), mInflate(inflate), rayCollider(origin, dir, bothSides, geomEpsilon), maxT(maxT)
	{
		PX_ASSERT(closestHit.distance == PX_MAX_REAL);
		hadClosestHit = false;
	}

	PX_FORCE_INLINE void getVertIndices(PxU32 triIndex, PxU32& i0, PxU32 &i1, PxU32 &i2)
	{
		if(has16BitIndices)
		{
			const PxU16* p = reinterpret_cast<const PxU16*>(*mTris + triIndex*3*sizeof(PxU16));
			i0 = p[0]; i1 = p[1]; i2 = p[2];
		}
		else
		{
			const PxU32* p = reinterpret_cast<const PxU32*>(*mTris + triIndex*3*sizeof(PxU32));
			i0 = p[0]; i1 = p[1]; i2 = p[2];
		}
	}

	virtual bool processResults(PxU32 NumTouched, PxU32* Touched, PxF32& newMaxT)
	{
		//pxPrintf("in processResults, opccollider\n");
		PX_ASSERT(NumTouched > 0);
		// Loop through touched leaves
		PxRaycastHit tempHit;
		for(PxU32 leaf = 0; leaf<NumTouched; leaf++)
		{
			// Each leaf box has a set of triangles
			LeafTriangles currentLeaf;
			currentLeaf.Data = Touched[leaf];
			PxU32 nbLeafTris = currentLeaf.GetNbTriangles();			
			PxU32 baseLeafTriIndex = currentLeaf.GetTriangleIndex();

#if PX_IS_SPU
			// on SPU we fetch verts on 8 parallel DMA channels
			const PxU32 N = 8;
			for(PxU32 iTri = 0; iTri < nbLeafTris; iTri+=N)
			{
				PxVec3 v[N][3];
				PxU32 inds[N][3];
				PxU32 countLeft = iTri+N > nbLeafTris ? nbLeafTris-iTri : N;
				MeshInterface::getTriangleVertsN<N>(has16BitIndices, *mTris, *mVerts, baseLeafTriIndex+iTri, countLeft, v, inds);
				for(PxU32 jj = 0; jj < countLeft; jj++)
				{
					const PxU32 triangleIndex = baseLeafTriIndex+iTri+jj;
					const PxVec3& v0 = v[jj][0];
					const PxVec3& v1 = v[jj][1];
					const PxVec3& v2 = v[jj][2];

#else // #if PX_IS_SPU
			for(PxU32 i = 0; i < nbLeafTris; i++)
			{{ // double brace to make the inner loop compatible with the double loop SPU section above, without code duplication
				PxU32 i0, i1, i2;
				const PxU32 triangleIndex = baseLeafTriIndex+i;
				getVertIndices(triangleIndex, i0, i1, i2);

				const PxVec3* verts = reinterpret_cast<PxVec3*>(*mVerts);
				PxVec3 v0 = verts[i0], v1 = verts[i1], v2 = verts[i2];
#endif // #if PX_IS_SPU

				Ps::IntBool overlap;
				if (tInflate)
				{
					PxVec3 minB = v0; minB = minB.minimum(v1); minB = minB.minimum(v2);
					PxVec3 maxB = v0; maxB = maxB.maximum(v1); maxB = maxB.maximum(v2);
					PxReal tNear, tFar;
					overlap = Gu::intersectRayAABB2(
						minB-*mInflate, maxB+*mInflate, rayCollider.mOrigin, rayCollider.mDir, maxT, tNear, tFar);
					if (overlap)
					{
						// can't clip to tFar here because hitting the AABB doesn't guarantee that we can clip
						// (since we can still miss the actual tri)
						tempHit.distance = maxT;
						tempHit.faceIndex = triangleIndex;
						tempHit.u = tempHit.v = 0.0f;
					}
				} else
					overlap = rayCollider.overlap(v0, v1, v2, tempHit) && tempHit.distance <= maxT;
				if(!overlap)
					continue;
				tempHit.faceIndex = triangleIndex;
				tempHit.flags = PxHitFlag::ePOSITION|PxHitFlag::eDISTANCE;
				// Intersection point is valid if dist < segment's length
				// We know dist>0 so we can use integers
				if (outerCallback.inClosestMode())
				{
					if(tempHit.distance < closestHit.distance)
					{
						closestHit = tempHit;
						newMaxT = PxMin(tempHit.distance, newMaxT);
						cv0 = v0; cv1 = v1; cv2 = v2;
						hadClosestHit = true;
					}
				} else
				{
					PxReal shrunkMaxT = newMaxT;
					//pxPrintf("calling processHit\n");
					PxAgain again = outerCallback.processHit(tempHit, v0, v1, v2, has16BitIndices != 0, mTris, shrunkMaxT);
					if (!again)
						return false;
					if (shrunkMaxT < newMaxT)
					{
						newMaxT = shrunkMaxT;
						maxT = shrunkMaxT;
					}
				}

				if (outerCallback.inAnyMode()) // early out if in ANY mode
					return false;
			}} // for SPU code sharing

		} // for(PxU32 leaf = 0; leaf<NumTouched; leaf++)

		return true;
	}

	virtual ~RayRTreeCallback()
	{
		if (hadClosestHit)
		{
			PX_ASSERT(outerCallback.inClosestMode());
			outerCallback.processHit(closestHit, cv0, cv1, cv2, has16BitIndices != 0, mTris, maxT);
		}
	}

private:
	RayRTreeCallback& operator=(const RayRTreeCallback&);
};

template <int tInflate>
void MeshRayCollider<tInflate>::Collide(
	const PxVec3& orig, const PxVec3& dir, PxReal maxT, bool bothSides,
	const RTreeMidphaseData& model, MeshHitCallback<PxRaycastHit>& callback,
	PxReal geomEpsilon, const Cm::Matrix34* world, const PxVec3* inflate)
{
	const MeshInterface* mi = model.mIMesh;
	MemFetchSmallBuffer buf0, buf1, buf2;
	PxU32* has16BitIndices = memFetchAsync<PxU32>(MemFetchPtr(mi)+PX_OFFSET_OF(MeshInterface, mHas16BitIndices), 5, buf0);
	MemFetchPtr* mTris = memFetchAsync<MemFetchPtr>(MemFetchPtr(mi)+PX_OFFSET_OF(MeshInterface, mTris), 5, buf1);
	MemFetchPtr* mVerts = memFetchAsync<MemFetchPtr>(MemFetchPtr(mi)+PX_OFFSET_OF(MeshInterface, mVerts), 5, buf2);
	memFetchWait(5);

	//pxPrintf("mesh collider, world=%x\n", PxU32(world));
	PxVec3 dir1 = world ? world->rotateTranspose(dir) : dir;
	PxVec3 origin1 = world ? world->getInverseRT().transform(orig) : orig;

	RayRTreeCallback<tInflate> rTreeCallback(
		model, callback, *has16BitIndices, mTris, mVerts, origin1, dir1, maxT, bothSides, inflate, geomEpsilon);

	const PxU32 maxResults = Gu::RTreePage::SIZE; // maxResults=rtree page size for more efficient early out
	PxU32 buf[maxResults];
	model.mRTree->traverseRay<tInflate>(origin1, dir1, maxResults, buf, &rTreeCallback, inflate, maxT);
	//pxPrintf("returning from meshCollider\n");
}


template void MeshRayCollider<1>::Collide(
	const PxVec3& orig, const PxVec3& dir, PxReal maxT, bool bothSides,
	const RTreeMidphaseData& model, MeshHitCallback<PxRaycastHit>& callback,
	PxReal geomEpsilon, const Cm::Matrix34* world, const PxVec3* inflate);

template void MeshRayCollider<0>::Collide(
	const PxVec3& orig, const PxVec3& dir, PxReal maxT, bool bothSides,
	const RTreeMidphaseData& model, MeshHitCallback<PxRaycastHit>& callback,
	PxReal geomEpsilon, const Cm::Matrix34* world, const PxVec3* inflate);

