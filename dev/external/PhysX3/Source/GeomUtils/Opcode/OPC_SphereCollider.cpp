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
#include "GuSphere.h"
#include "OPC_SphereCollider.h"
#include "CmMatrix34.h"

using namespace physx;
using namespace Cm;
using namespace Gu;

SphereCollider::SphereCollider()
{
	mCenter = PxVec3(0);
	mRadius2 = 0.0f;
	mRadius = 0.0f;
}

void SphereCollider::InitQuery(const Gu::Sphere& sphere, const Cm::Matrix34* worlds, const Cm::Matrix34* worldm)
{
	VolumeCollider::InitQuery();

	// Compute sphere in model space:
	// - Precompute R^2
	mRadius2 = sphere.radius * sphere.radius;
	mRadius = sphere.radius;
	// - Compute center position
	mCenter = sphere.center;
	// -> to world space
	if(worlds)
	{
		mCenter = worlds->transform(mCenter);
	}
	// -> to model space
	if(worldm)
	{
		// Invert model matrix
		Cm::Matrix34 InvWorldM = worldm->getInverseRT();

		mCenter = InvWorldM.transform(mCenter);
	}
}

#ifndef OPC_SUPPORT_VMX128

#define LOOSE_SPHERE_PRIM_SETUP
#define LOOSE_SPHERE_PRIM_FAST(prim_index, flag) PerformLooseSpherePrimOverlapTest(prim_index, flag);

#else

#define LOOSE_SPHERE_PRIM_SETUP													\
	Vec3V LSP_center = V3LoadU(mCenter);								\
	Vec3V LSP_radius = V3Load(mRadius);									\
	Vec3V LSP_sphereMin = V3Sub(LSP_center, LSP_radius);						\
	Vec3V LSP_sphereMax = V3Add(LSP_center, LSP_radius);		

//! OBB-triangle test
#define LOOSE_SPHERE_PRIM_FAST(prim_index, flag)								\
	/* Request vertices from the app */											\
	VertexPointers VP;	mIMesh->GetTriangle(VP, prim_index);					\
	/* Perform triangle-sphere overlap test */									\
	if(LooseSphereTriOverlap(*VP.vertex[0], *VP.vertex[1], *VP.vertex[2],		\
		LSP_center, LSP_radius, LSP_sphereMin, LSP_sphereMax))					\
	{																			\
		setContact(prim_index, flag);											\
	}

#endif



struct SphereColliderProcessingCallback : Gu::RTree::Callback
{
	HybridSphereCollider*		parent;
	const RTreeMidphaseData*	model;
	ReportSphereCallback		filteredCallback;
	PxU32						totalResults;
	PxU32						doPrimTests;

	SphereColliderProcessingCallback(
		HybridSphereCollider* parent, const RTreeMidphaseData* model, ReportSphereCallback fc, PxU32 DoPrimTests)
		: parent(parent), model(model), filteredCallback(fc), totalResults(0), doPrimTests(DoPrimTests)
	{
	}
	virtual ~SphereColliderProcessingCallback() {}

	virtual bool processResults(PxU32 count, PxU32* buf)
	{
		MemFetchSmallBuffer buf0, buf1, buf2;
		PxU32* has16BitIndices = memFetchAsync<PxU32>(MemFetchPtr(parent->mIMesh)+PX_OFFSET_OF(MeshInterface, mHas16BitIndices), 5, buf0);
		MemFetchPtr* mTris = memFetchAsync<MemFetchPtr>(MemFetchPtr(parent->mIMesh)+PX_OFFSET_OF(MeshInterface, mTris), 5, buf1);
		MemFetchPtr* mVerts = memFetchAsync<MemFetchPtr>(MemFetchPtr(parent->mIMesh)+PX_OFFSET_OF(MeshInterface, mVerts), 5, buf2);
		memFetchWait(5);

		while(count--)
		{
			// Each leaf box has a set of triangles
			LeafTriangles CurrentLeaf;
			CurrentLeaf.Data = *buf++;
			PxU32 NbTris = CurrentLeaf.GetNbTriangles();
			PxU32 BaseIndex = CurrentLeaf.GetTriangleIndex();

			while(NbTris--)
			{
				const PxU32 TriangleIndex = BaseIndex++;
#ifdef PX_X360
				if(NbTris)
					MeshInterface::prefetchTriangleVerts(*has16BitIndices, (void*)*mTris, (PxVec3*)*mVerts, BaseIndex);
#endif
				PxVec3 v0, v1, v2;
				MeshInterface::GetTriangleVerts(*has16BitIndices, *mTris, *mVerts, TriangleIndex, v0, v1, v2);

				if(!doPrimTests || parent->SphereTriOverlap(v0,v1,v2))
				{
					parent->mFlags |= OPC_CONTACT;

					if(!filteredCallback(TriangleIndex, parent->mUserData))
							return false;
				}
			}
		}

		return true;
	}
};


bool HybridSphereCollider::Collide(	ReportSphereCallback cb, void* userData,
									const Gu::Sphere& sphere, const RTreeMidphaseData& model,
									const Cm::Matrix34* worlds, const Cm::Matrix34* worldm)
{
	if(!Setup(&model))
		return false;

	InitQuery(sphere, worlds, worldm);

	mCallback = cb;
	mUserData = userData;

	const Ps::IntBool NoPrimitiveTests = mFlags & OPC_NO_PRIMITIVE_TESTS;
	
	SphereColliderProcessingCallback callback(this, &model, cb, !NoPrimitiveTests);

	const PxU32 bufSize = 32;
	PxU32 buf[bufSize];
	model.mRTree->traverseAABB(mCenter-PxVec3(mRadius), mCenter+PxVec3(mRadius), bufSize, buf, &callback);
	return true;
}
