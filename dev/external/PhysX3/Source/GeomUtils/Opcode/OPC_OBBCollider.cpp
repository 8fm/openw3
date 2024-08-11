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
#include "CmPhysXCommon.h"
#include "CmMemFetch.h"
#include "OPC_AABBCollider.h"
#include "OPC_OBBCollider.h"
#include "OPC_OBBColliderOverlap.h"
#include "CmMatrix34.h"

using namespace physx;
using namespace Cm;
using namespace Gu;

template<int nullWorldBM>
Ps::IntBool OBBCollider::InitQuery(const Gu::Box& box, const Cm::Matrix34* worldb, const Cm::Matrix34* worldm)
{
	// 1) Call the base method
	VolumeCollider::InitQuery();

	// 2) Compute obb in world space
	mBoxExtents = box.extents;

	Cm::Matrix34 WorldB;

	if(worldb && !nullWorldBM)
	{
		const PxMat33 tmp = PxMat33(worldb->base0, worldb->base1, worldb->base2) * box.rot;
		WorldB.base0 = tmp.column0;
		WorldB.base1 = tmp.column1;
		WorldB.base2 = tmp.column2;
		WorldB.base3 = worldb->transform(box.center);
	}
	else
	{
		WorldB.base0 = box.rot.column0;
		WorldB.base1 = box.rot.column1;
		WorldB.base2 = box.rot.column2;
		WorldB.base3 = box.center;
	}

	// Setup matrices
	if(worldm && !nullWorldBM)
	{
		Cm::Matrix34 WorldToModel	= worldm->getInverseRT();
		Cm::Matrix34 WorldToBox		= WorldB.getInverseRT();

		Cm::Matrix34 ModelToBox = WorldToBox * *worldm;
		Cm::Matrix34 BoxToModel = WorldToModel * WorldB;

		mRModelToBox.column0	= ModelToBox.base0;
		mRModelToBox.column1	= ModelToBox.base1;
		mRModelToBox.column2	= ModelToBox.base2;
		mTModelToBox			= ModelToBox.base3;

		mRBoxToModel.column0	= BoxToModel.base0;
		mRBoxToModel.column1	= BoxToModel.base1;
		mRBoxToModel.column2	= BoxToModel.base2;
		mTBoxToModel			= BoxToModel.base3;
	}
	else
	{
		Cm::Matrix34 ModelToBox	= WorldB.getInverseRT();

		mRModelToBox.column0	= ModelToBox.base0;
		mRModelToBox.column1	= ModelToBox.base1;
		mRModelToBox.column2	= ModelToBox.base2;
		mTModelToBox			= ModelToBox.base3;

		mRBoxToModel.column0	= WorldB.base0;
		mRBoxToModel.column1	= WorldB.base1;
		mRBoxToModel.column2	= WorldB.base2;
		mTBoxToModel			= WorldB.base3;
	}

	return Ps::IntFalse;
}

template <int DoPrimTests, int ReportVerts, int ParallelSPUFetch=1> // use 1 for parallel fetch unless strapped for space
struct OBBRTreeCallback : Gu::RTree::Callback
{
	HybridOBBCollider*		parent;
	const RTreeMidphaseData*	model;
	VolumeColliderTrigCallback*	filteredCallback;
	PxU32				totalResults;
	//PxU32				totalTris;
	OBBRTreeCallback(HybridOBBCollider* parent, const RTreeMidphaseData* model, VolumeColliderTrigCallback* fc)
		: parent(parent), model(model), filteredCallback(fc), totalResults(0)//, totalTris(0)
	{
	}

	virtual void profile()
	{
		#if 0
		static PxU32 totalTrigs1 = 0;
		if (totalTris > 0)
		{
			static int counter1 = 0;
			counter1 ++;
			totalTrigs1 += totalTris;
			if (counter1 == 5000)
				printf("avg trigs = %.2f\n", PxF32(totalTrigs1) / 5000);
		}
		#endif
	}

	virtual bool processResults(PxU32 Nb, PxU32* Touched)
	{
		totalResults += Nb;

		MemFetchSmallBuffer buf0, buf1, buf2;
		PxU32* has16BitIndices = memFetchAsync<PxU32>(MemFetchPtr(parent->mIMesh)+PX_OFFSET_OF(MeshInterface, mHas16BitIndices), 5, buf0);
		MemFetchPtr* mTris = memFetchAsync<MemFetchPtr>(MemFetchPtr(parent->mIMesh)+PX_OFFSET_OF(MeshInterface, mTris), 5, buf1);
		MemFetchPtr* mVerts = memFetchAsync<MemFetchPtr>(MemFetchPtr(parent->mIMesh)+PX_OFFSET_OF(MeshInterface, mVerts), 5, buf2);
		memFetchWait(5);

		// new buffer for trig results, can be bigger than touched leaves so we can't reuse Touched in place
		const PxU32 trigBufferSize = PX_IS_SPU ? 16 : 64;
		PxU32 indexBuffer[trigBufferSize];
		PxVec3 trigBuffer[trigBufferSize*3];
		PxU32 vertIndexBuffer[trigBufferSize*3];
		PX_UNUSED(vertIndexBuffer);
		PxU32 numTrigsHit = 0;
		while(Nb--)
		{
			PxU32 leafData = *Touched++;

			// Each leaf box has a set of triangles
			PxU32 NbTris = LeafGetNbTriangles(leafData);
			PxU32 BaseIndex = LeafGetTriangleIndex(leafData);
			//pxPrintf("triNb,baseIdx=%d,%d\n", NbTris, BaseIndex);
			//totalTris += NbTris;

			if (!DoPrimTests && !ReportVerts)
			{
				if (numTrigsHit + NbTris >= trigBufferSize)
				{
					// flush the buffer to make sure we have space for Duff's device
					filteredCallback->processResults(numTrigsHit, trigBuffer, indexBuffer, vertIndexBuffer);
					numTrigsHit = 0;
				}
				const int NBITS = 4;
				const int N = 16;
				PX_ASSERT(NbTris < 64 && NbTris < trigBufferSize);
			    int nExtraBlocks = NbTris >> NBITS;
				PxU32* to = indexBuffer+numTrigsHit;
				char entry = NbTris & (N-1);
				to += entry;
				// a variation on Duff's device http://en.wikipedia.org/wiki/Duff's_device
				// used to speed up initializing the return array with sequential indices
				switch (entry)
				{
					case 16: do{ to[-16] = BaseIndex+15;
					case 15:     to[-15] = BaseIndex+14;
					case 14:     to[-14] = BaseIndex+13;
					case 13:     to[-13] = BaseIndex+12;
					case 12:     to[-12] = BaseIndex+11;
					case 11:     to[-11] = BaseIndex+10;
					case 10:     to[-10] = BaseIndex+9;
					case 9:      to[-9]  = BaseIndex+8;
					case 8:      to[-8]  = BaseIndex+7;
					case 7:      to[-7]  = BaseIndex+6;
					case 6:      to[-6]  = BaseIndex+5;
					case 5:      to[-5]  = BaseIndex+4;
					case 4:      to[-4]  = BaseIndex+3;
					case 3:      to[-3]  = BaseIndex+2;
					case 2:      to[-2]  = BaseIndex+1;
					case 1:      to[-1]  = BaseIndex+0;
					case 0:
								to += N;  BaseIndex += N;
						} while (--nExtraBlocks>=0);
				}
				numTrigsHit += NbTris;
			} else
			{
				#if __SPU__// && ParallelSPUFetch
				const PxU32 N = 4;
				for(PxU32 iTri = 0; iTri < NbTris; iTri+=N)
				{
					PxVec3 v[N][3];
					PxU32 i[N][3];
					PxU32 countLeft = iTri+N > NbTris ? NbTris-iTri : N;
					if (ReportVerts || DoPrimTests)
						MeshInterface::getTriangleVertsN<N>(*has16BitIndices, *mTris, *mVerts, BaseIndex+iTri, countLeft, v, i);
					for(PxU32 jj = 0; jj < countLeft; jj++)
					{
						const PxVec3& v0 = v[jj][0];
						const PxVec3& v1 = v[jj][1];
						const PxVec3& v2 = v[jj][2];
						if(!DoPrimTests || parent->TriBoxOverlap(v0, v1, v2))
						{
							if (ReportVerts)
							{
								trigBuffer[numTrigsHit*3+0] = v0;
								trigBuffer[numTrigsHit*3+1] = v1;
								trigBuffer[numTrigsHit*3+2] = v2;
								vertIndexBuffer[numTrigsHit*3+0] = i[jj][0];
								vertIndexBuffer[numTrigsHit*3+1] = i[jj][1];
								vertIndexBuffer[numTrigsHit*3+2] = i[jj][2];
							}
							indexBuffer[numTrigsHit++] = BaseIndex+iTri+jj;
							if (numTrigsHit == trigBufferSize)
							{
								// flush partial result, reset trig count
								filteredCallback->processResults(numTrigsHit, trigBuffer, indexBuffer, vertIndexBuffer);
								numTrigsHit = 0;
							}
						}
					}
				}
				#else
				for(PxU32 iTri = 0; iTri < NbTris; iTri++)
				{
					PxU32 TriangleIndex = BaseIndex++;
					PxVec3 v0, v1, v2;
					PxU32 i0, i1, i2;
					if (ReportVerts || DoPrimTests)
						MeshInterface::GetTriangleVerts(*has16BitIndices, *mTris, *mVerts, TriangleIndex, v0, v1, v2, i0, i1, i2);
					if(!DoPrimTests || parent->TriBoxOverlap(v0, v1, v2))
					{
						if (ReportVerts)
						{
							PxU32 nbTrigsHit = numTrigsHit*3;
							trigBuffer[nbTrigsHit+0] = v0;
							trigBuffer[nbTrigsHit+1] = v1;
							trigBuffer[nbTrigsHit+2] = v2;
							vertIndexBuffer[nbTrigsHit+0] = i0;
							vertIndexBuffer[nbTrigsHit+1] = i1;
							vertIndexBuffer[nbTrigsHit+2] = i2;
						}
						indexBuffer[numTrigsHit++] = TriangleIndex;
						if (numTrigsHit == trigBufferSize)
						{
							// flush partial result, reset trig count
							filteredCallback->processResults(numTrigsHit, trigBuffer, indexBuffer, vertIndexBuffer);
							numTrigsHit = 0;
						}
					}
				}
				#endif
			}
		}

		if (!numTrigsHit)
			return true;

		//for (PxU32 ii = 0; ii < numTrigsHit; ii++)
		//	for (PxU32 jj = 0; jj < 3; jj++)
		//		pxPrintf("trigBuffer[%d][%d] = %.5f %.5f %.5f\n", ii, jj,
		//			trigBuffer[ii*3+jj].x, trigBuffer[ii*3+jj].y, trigBuffer[ii*3+jj].z);
		// flush any outstanding results
		return filteredCallback->processResults(numTrigsHit, trigBuffer, indexBuffer, vertIndexBuffer);

	} // processResults
};

template <int DoPrimTests, int ReportVerts, int CompactVersion>
void HybridOBBCollider::Collide(
	const Gu::Box& box, const RTreeMidphaseData& model, VolumeColliderTrigCallback* parentCallback,
	const Cm::Matrix34* worldb, const Cm::Matrix34* worldm)
{
	// Checkings
	if(!Setup(&model))
		return;

	// Init collision query
	//Note does not do precompute for box-box(done in InitTraversal())
	if(InitQuery<CompactVersion>(box, worldb, worldm))
		return;

	//Perform additional setup here(avoids overhead when we can early out).
	InitTraversal();

	OBBRTreeCallback<DoPrimTests, ReportVerts, !CompactVersion> callback(this, &model, parentCallback);

	Gu::Box meshSpaceBox;
	if (worldm && !CompactVersion)
	{
		Cm::Matrix34 invWorldM = worldm->getInverseRT();
		meshSpaceBox.rot.column0 = invWorldM.rotate(box.rot.column0);
		meshSpaceBox.rot.column1 = invWorldM.rotate(box.rot.column1);
		meshSpaceBox.rot.column2 = invWorldM.rotate(box.rot.column2);
		meshSpaceBox.center = invWorldM.transform(box.center);
		meshSpaceBox.extents = box.extents;
	} else
		meshSpaceBox = box;

	const PxU32 bufSize = 32;
	PxU32 buf[bufSize];

	PxQuat q(meshSpaceBox.rot);
	if (!PX_IS_SPU && PxAbs(q.w) > 0.99999f) // use AABB collider for axis aligned OBBs
	{
		PxVec3 boxMin = meshSpaceBox.center-meshSpaceBox.extents;
		PxVec3 boxMax = meshSpaceBox.center+meshSpaceBox.extents;
		model.mRTree->traverseAABB(boxMin, boxMax, bufSize, buf, &callback);
	}
	else
		model.mRTree->traverseOBB(meshSpaceBox, bufSize, buf, &callback);

	//callback->profile();
}

namespace physx { namespace Gu {
// explicit instantiations for HybridOBBCollider
#define EX_INST(a,b,c) \
	template PX_PHYSX_COMMON_API void HybridOBBCollider::Collide<a,b,c>( \
			const Gu::Box& box, const RTreeMidphaseData& model, VolumeColliderTrigCallback* callback, \
			const Cm::Matrix34* worldb, const Cm::Matrix34* worldm);
	EX_INST(0,0,0)
	EX_INST(0,0,1)
	EX_INST(0,1,0)
	EX_INST(0,1,1)
	EX_INST(1,0,0)
	EX_INST(1,0,1)
	EX_INST(1,1,0)
	EX_INST(1,1,1)
#undef EX_INST
}}
