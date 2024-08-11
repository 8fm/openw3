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
#include "OPC_LSSCollider.h"
#include "CmMatrix34.h"

using namespace physx;
using namespace Cm;
using namespace Gu;

void LSSCollider::InitQuery(const Gu::Capsule& lss, const Cm::Matrix34* worldl, const Cm::Matrix34* worldm)
{
	VolumeCollider::InitQuery();

	// Compute LSS in model space:
	// - Precompute R^2
	mRadius = lss.radius;
	mRadius2 = lss.radius * lss.radius;
	// - Compute segment
	mSeg.p0 = lss.p0;
	mSeg.p1 = lss.p1;
	// -> to world space
	if(worldl)
	{
		mSeg.p0 = worldl->transform(mSeg.p0);
		mSeg.p1 = worldl->transform(mSeg.p1);
	}
	// -> to model space
	if(worldm)
	{
		// Invert model matrix
		Cm::Matrix34 InvWorldM = worldm->getInverseRT();

		mSeg.p0 = InvWorldM.transform(mSeg.p0);
		mSeg.p1 = InvWorldM.transform(mSeg.p1);
	}

	// Precompute segment data
	mSDir = 0.5f * (mSeg.p1 - mSeg.p0);
	mFDir.x = PxAbs(mSDir.x);
	mFDir.y = PxAbs(mSDir.y);
	mFDir.z = PxAbs(mSDir.z);
	mSCen = 0.5f * (mSeg.p1 + mSeg.p0);

	mOBB.create(Gu::Capsule(mSeg, mRadius));	// Create box around transformed capsule
}


bool HybridLSSCollider::processLeafTriangles(PxU32 count, const PxU32* PX_RESTRICT buf)
{
	const Ps::IntBool NoPrimitiveTests = mFlags & OPC_NO_PRIMITIVE_TESTS;
	const Ps::IntBool LoosePrimitiveTests = mFlags & OPC_LOOSE_PRIMITIVE_TESTS;

	while(count--)
	{
		const PxU32 leafData = *buf++;

		// Each leaf box has a set of triangles
		PxU32 NbTris = LeafGetNbTriangles(leafData);
		PxU32 BaseIndex = LeafGetTriangleIndex(leafData);

		// Each leaf box has a set of triangles
		// Loop through triangles and test each of them
		if(!NoPrimitiveTests)
		{
			if(LoosePrimitiveTests)
			{
				while(NbTris--)
					if(!PerformLooseLSSPrimOverlapTest(BaseIndex++, OPC_CONTACT))
						return true;
			}
			else
			{
				while(NbTris--)
					if(!PerformLSSPrimOverlapTest(BaseIndex++, OPC_CONTACT))
						return true;
			}
		}
		else
		{
			while(NbTris--)
				if(!setContact(BaseIndex++, OPC_CONTACT))
					return true;
		}
	}
	return true;
}

	//template <int DoPrimTests, int LoosePrimitiveTests>
	struct LSSColliderProcessingCallback : Gu::RTree::CallbackRaycast
	{
		HybridLSSCollider*			parent;
		const RTreeMidphaseData*	model;
		ReportCapsuleCallback		filteredCallback;
		PxU32						totalResults;
		PxU32						doPrimTests;
		PxU32						loosePrimitiveTests;

		LSSColliderProcessingCallback(HybridLSSCollider* parent, const RTreeMidphaseData* model, ReportCapsuleCallback fc, PxU32 DoPrimTests, PxU32 LoosePrimitiveTests)
			: parent(parent), model(model), filteredCallback(fc), totalResults(0), doPrimTests(DoPrimTests), loosePrimitiveTests(LoosePrimitiveTests)
		{
		}
		virtual ~LSSColliderProcessingCallback() {}

		virtual bool processResults(PxU32 count, PxU32* buf, PxF32& newMaxT)
		{
			PX_UNUSED(newMaxT);

			MemFetchSmallBuffer buf0, buf1, buf2;
			PxU32* has16BitIndices = memFetchAsync<PxU32>(MemFetchPtr(parent->mIMesh)+PX_OFFSET_OF(MeshInterface, mHas16BitIndices), 5, buf0);
			MemFetchPtr* mTris = memFetchAsync<MemFetchPtr>(MemFetchPtr(parent->mIMesh)+PX_OFFSET_OF(MeshInterface, mTris), 5, buf1);
			MemFetchPtr* mVerts = memFetchAsync<MemFetchPtr>(MemFetchPtr(parent->mIMesh)+PX_OFFSET_OF(MeshInterface, mVerts), 5, buf2);
			memFetchWait(5);

			// new buffer for trig results, can be bigger than touched leaves so we can't reuse Touched in place
			//const PxU32 trigBufferSize = 32;
			//PxVec3 trigBuffer[trigBufferSize*3];
			//PxU32 indexBuffer[trigBufferSize];
			//PxU32 numTrigsHit = 0;
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

					if(!doPrimTests
						|| ((loosePrimitiveTests && !PX_IS_SPU)
								? parent->LooseLSSTriOverlap(v0, v1, v2)
								: parent->LSSTriOverlap(v0, v1, v2)))
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

bool HybridLSSCollider::Collide(ReportCapsuleCallback cb, void* userData,
								const Gu::Capsule& lss, const RTreeMidphaseData& model,
								const Cm::Matrix34* worldl, const Cm::Matrix34* worldm)
{
	if(!Setup(&model))
		return false;

	InitQuery(lss, worldl, worldm);

	mCallback = cb;
	mUserData = userData;

	//void* callbackBuf = PxAlloca(sizeof(LSSColliderProcessingCallback<0,0>));

	//LSSColliderProcessingCallback<0,0> callbackNoPrimNoLoose(this, &model, cb);
	//LSSColliderProcessingCallback<0,1> callbackNoPrimLoose(this, &model, cb);
	//LSSColliderProcessingCallback<1,0> callbackPrimNoLoose(this, &model, cb);
	//LSSColliderProcessingCallback<1,1> callbackPrimLoose(this, &model, cb);

	const Ps::IntBool NoPrimitiveTests = mFlags & OPC_NO_PRIMITIVE_TESTS;
	const Ps::IntBool LoosePrimitiveTests = mFlags & OPC_LOOSE_PRIMITIVE_TESTS;

	LSSColliderProcessingCallback callback(this, &model, cb,!NoPrimitiveTests,LoosePrimitiveTests);
	
	//Gu::RTree::Callback* callback;
	//if (NoPrimitiveTests && !LoosePrimitiveTests)
	//	callback = new(callbackBuf) LSSColliderProcessingCallback<0,0>(this, &model, cb);
	//else if (NoPrimitiveTests && LoosePrimitiveTests)
	//	callback = new(callbackBuf) LSSColliderProcessingCallback<0,1>(this, &model, cb);
	//else if (LoosePrimitiveTests)
	//	callback = new(callbackBuf) LSSColliderProcessingCallback<1,1>(this, &model, cb);
	//else
	//	callback = new(callbackBuf) LSSColliderProcessingCallback<0,0>(this, &model, cb);

	const PxU32 bufSize = 32;
	PxU32 buf[bufSize];
	PxVec3 radius3(mRadius);
	model.mRTree->traverseRay<1>(mSeg.p0, mSeg.p1-mSeg.p0, bufSize, buf, &callback, &radius3, 1.0f);
	return true;
}
