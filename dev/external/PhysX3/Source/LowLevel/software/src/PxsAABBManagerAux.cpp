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


#include "PxsAABBManagerAux.h"
#include "PxsRigidBody.h"
#include "PxvGeometry.h"
#include "PsAtomic.h"
#include "GuConvexMeshData.h"
#include "GuTriangleMeshData.h"
#include "GuHeightFieldData.h"

#ifdef __SPU__
#include "CmPS3MemFetch.h"
#include "CellComputeAABBTask.h"
#endif

using namespace physx;

///////////////////////////////////////////////////////////////////////////////

PxBounds3 computeAABBNoCCD(const PxcAABBDataDynamic& aabbData, const PxsComputeAABBParams& /*params*/)
{
	const PxsShapeCore* PX_RESTRICT shapeCore = aabbData.mShapeCore;
	const PxBounds3* PX_RESTRICT localSpaceBounds = aabbData.mLocalSpaceAABB;

	// PT: sad but true: writing this with one line gets rid of an LHS
	PxBounds3 bounds;
	if(aabbData.mBodyAtom)
	{
		const PxsBodyCore* PX_RESTRICT bodyCore = static_cast<const PxsBodyCore* PX_RESTRICT>(aabbData.mRigidCore);
		shapeCore->geometry.computeBounds(bounds, bodyCore->body2World * bodyCore->body2Actor.getInverse() *shapeCore->transform, shapeCore->contactOffset, localSpaceBounds);
	}
	else 
	{
		const PxsRigidCore* PX_RESTRICT rigidCore = aabbData.mRigidCore;
		shapeCore->geometry.computeBounds(bounds, rigidCore->body2World * shapeCore->transform, shapeCore->contactOffset, localSpaceBounds);
	}
	return bounds;
}

//Returns true if this is fast-moving
PxF32 computeSweptBounds(const PxsComputeAABBParams & /*params*/, const PxcAABBDataDynamic& aabbData, PxBounds3& destBounds)
{
	const PxsShapeCore* PX_RESTRICT shapeCore = aabbData.mShapeCore;
	const PxsRigidBody* PX_RESTRICT bodyAtom = (PxsRigidBody*)aabbData.mBodyAtom;
	const PxsBodyCore* PX_RESTRICT bodyCore = static_cast<const PxsBodyCore* PX_RESTRICT>(aabbData.mRigidCore);
	const PxBounds3* PX_RESTRICT localSpaceBounds = aabbData.mLocalSpaceAABB;

	//NEW: faster, more approximate mode: take world bounds at start and end of motion, and make a bound around both.
	//also, if we're not moving fast enough, non-swept bounds are returned.

	PX_ASSERT(bodyAtom);	//only call for dynamics!

	//Hmm, core and localSpaceBounds passed from spu because we 
	//can't patch up mBodyAtom's reference to PxsBodyCore on spu.
	//Need to handle the case on spu where core is null because we have  
	//a static or kinematic object.
	PX_ASSERT(bodyCore);

	PxTransform endShape2world = bodyCore->body2World * bodyCore->body2Actor.getInverse() * shapeCore->transform; 
	PxVec3 endOrigin, endExtent;
	PxReal ccdThreshold = shapeCore->geometry.computeBoundsWithCCDThreshold(endOrigin, endExtent, endShape2world, localSpaceBounds);//hopefully this does not do too much compute...this should really be cached!!

	PxBounds3 bounds = PxBounds3::centerExtents(endOrigin, endExtent);

	//KS - Always inflate bounds here
	PxTransform startShape2World = bodyAtom->getLastCCDTransform() * bodyCore->body2Actor.getInverse() * shapeCore->transform;	//build shape 2 world

	PxBounds3 startBounds;
	shapeCore->geometry.computeBounds(startBounds, startShape2World, 0.0f, localSpaceBounds);//hopefully this does not do too much compute...this should really be cached!!

	bounds.include(startBounds);

	//AM: we fatten by the contact offset here to be able to do dist based CG.
	const PxVec3 offset(shapeCore->contactOffset);

	destBounds.minimum = bounds.minimum - offset;
	destBounds.maximum = bounds.maximum + offset;

	const PxVec3 startOrigin = startBounds.getCenter();
	const PxVec3 trans = startOrigin - endOrigin;

	return physx::intrinsics::fsel(trans.magnitudeSquared() - (ccdThreshold * ccdThreshold),  1.0f, 0.0f);
	//return (trans.magnitudeSquared() >= (ccdThreshold * ccdThreshold) ? 1 : 0);
}

PxF32 physx::PxsComputeAABB
(const PxsComputeAABBParams& /*params*/, const bool /*secondBroadphase*/, const PxcAABBDataStatic& aabbData, 
 PxBounds3& updatedBodyShapeBounds)
{
	PX_ASSERT(aabbData.mShapeCore);
	const PxsShapeCore* PX_RESTRICT shapeCore = aabbData.mShapeCore;
	const PxsRigidCore* PX_RESTRICT rigidCore = aabbData.mRigidCore;

#ifndef __SPU__

	PX_ALIGN(16, PxTransform globalPose);

	const Vec3V body2WorldPos = V3LoadU(rigidCore->body2World.p);
	const QuatV body2WorldRot = QuatVLoadU(&rigidCore->body2World.q.x);

	const Vec3V body2ActorPos = V3LoadU(shapeCore->transform.p);
	const QuatV body2ActorRot = QuatVLoadU(&shapeCore->transform.q.x);

	const Vec3V trnsl0 = QuatRotate(body2WorldRot,body2ActorPos);
	const Vec3V trnsl1 = V3Add(trnsl0,body2WorldPos);
	const QuatV rot1 = QuatMul(body2WorldRot,body2ActorRot);

	V3StoreA(trnsl1,globalPose.p);
	V4StoreA(rot1,&globalPose.q.x);

	shapeCore->geometry.computeBounds(updatedBodyShapeBounds, globalPose, shapeCore->contactOffset, NULL);

#else

	shapeCore->geometry.computeBounds(updatedBodyShapeBounds, rigidCore->body2World * shapeCore->transform, shapeCore->contactOffset, NULL);

#endif

	PX_ASSERT(updatedBodyShapeBounds.minimum.x <= updatedBodyShapeBounds.maximum.x
	 	 &&	  updatedBodyShapeBounds.minimum.y <= updatedBodyShapeBounds.maximum.y
		 &&	  updatedBodyShapeBounds.minimum.z <= updatedBodyShapeBounds.maximum.z);

	return 0.0f;
}

PxF32 physx::PxsComputeAABB
(const PxsComputeAABBParams& params, const bool secondBroadphase, const PxcAABBDataDynamic& aabbData, 
 PxBounds3& updatedBodyShapeBounds)
{
	PX_ASSERT(aabbData.mShapeCore);

	const PxsShapeCore* PX_RESTRICT shapeCore = aabbData.mShapeCore;
	const PxsRigidCore* PX_RESTRICT rigidCore = aabbData.mRigidCore;
	const PxBounds3* PX_RESTRICT localSpaceBounds = aabbData.mLocalSpaceAABB;
	PX_ASSERT(aabbData.mBodyAtom);

	if(!secondBroadphase || 0==rigidCore->hasCCD())				
	{
		const PxsBodyCore* PX_RESTRICT bodyCore = static_cast<const PxsBodyCore* PX_RESTRICT>(rigidCore);

		//bodyCore->body2World * bodyCore->body2Actor.getInverse() *shapeCore->transform

		PX_ALIGN(16, PxTransform globalPose);
		const PxVec3 negBody2Actor = -bodyCore->body2Actor.p;

		const Vec3V body2WorldPos = V3LoadU(bodyCore->body2World.p);
		const QuatV body2WorldRot = QuatVLoadU(&bodyCore->body2World.q.x);

		const Vec3V body2ActorPos = V3LoadU(negBody2Actor);
		const QuatV body2ActorRot = QuatVLoadU(&bodyCore->body2Actor.q.x);		

		const Vec3V body2ActorPosInv = QuatRotateInv(body2ActorRot,body2ActorPos);
		const QuatV body2ActorRotInv = QuatConjugate(body2ActorRot);

		const Vec3V shape2ActorPos = V3LoadU(shapeCore->transform.p);
		const QuatV shape2ActorRot = QuatVLoadU(&shapeCore->transform.q.x);

		const Vec3V trnsl0 = QuatRotate(body2WorldRot,body2ActorPosInv);
		const Vec3V trnsl1 = V3Add(trnsl0,body2WorldPos);
		const QuatV rot1 = QuatMul(body2WorldRot,body2ActorRotInv);

		const Vec3V trnsl2 = QuatRotate(rot1,shape2ActorPos);
		const Vec3V trnsl3 = V3Add(trnsl2,trnsl1);
		const QuatV rot3 = QuatMul(rot1,shape2ActorRot);

		V3StoreA(trnsl3,globalPose.p);
		V4StoreA(rot3,&globalPose.q.x);		

		shapeCore->geometry.computeBounds(updatedBodyShapeBounds, globalPose, shapeCore->contactOffset, localSpaceBounds);
		return 0.0f;
	}
	else
	{
		PX_ASSERT(secondBroadphase);
		PX_ASSERT(aabbData.mBodyAtom);//TODO: mUseSweptBounds doesn't make sense for statics but somehow it's still possible to enable!  We should reject that scenario!!
		return computeSweptBounds(params, aabbData, updatedBodyShapeBounds);	//this already includes the fat.  Note: if we do this when global second bp is off, then the bounds will be trailing at the end of the frame because we move out of them at integration, but this is OK.
	}
}

#ifdef __SPU__

struct EAAABBDataStatic
{
	PxU8 mShapeCore[sizeof(PxsShapeCore)];
	PxU8 mBodyCore[sizeof(PxsBodyCore) + 48];
	Cm::MemFetchGatherList<3> mList;
	PxcAABBDataStatic mAABBData;
};

struct EAAABBDataDynamic
{
	PxU8 mShapeCore[sizeof(PxsShapeCore)];
	PxU8 mRigidBodyPlusBodyCorePlusLocalSpaceAABB[sizeof(PxcRigidBody) + sizeof(PxsBodyCore) + 48];
	Cm::MemFetchGatherList<5> mList;
	PxcAABBDataDynamic mAABBData;
};


void initEAABBBData(EAAABBDataStatic& eaAABBData)
{
	eaAABBData.mList.init();

	eaAABBData.mList.setSize(0, sizeof(PxsShapeCore));

	eaAABBData.mAABBData.mShapeCore=(PxsShapeCore*)eaAABBData.mShapeCore;
}

void initEAABBBData(EAAABBDataDynamic& eaAABBData)
{
	eaAABBData.mList.init();

	eaAABBData.mList.setSize(0, sizeof(PxsShapeCore));

	eaAABBData.mAABBData.mShapeCore=(PxsShapeCore*)eaAABBData.mShapeCore;
}

PX_FORCE_INLINE void prefetchAsync(const PxcAABBDataStatic*& aabbData, EAAABBDataStatic* eaAABBData)
{
	PX_ASSERT(aabbData->mShapeCore);
	eaAABBData->mList.setEA(0, (uintptr_t)aabbData->mShapeCore);
	PX_ASSERT(eaAABBData->mList.getSize(0) == sizeof(PxsShapeCore));
	PX_ASSERT(eaAABBData->mAABBData.mShapeCore==(PxsShapeCore*)eaAABBData->mShapeCore);

	eaAABBData->mAABBData.mRigidCore=NULL;
	PxU32 listTide=1;
	PxU32 dataTide=0;

	eaAABBData->mList.setEA(1, (uintptr_t)aabbData->mRigidCore);
	eaAABBData->mList.setSize(1, sizeof(PxsRigidCore));
	eaAABBData->mAABBData.mRigidCore=(PxsRigidCore*)(eaAABBData->mBodyCore + 0);
	listTide=2;
	dataTide=sizeof(PxsRigidCore);

	Cm::memFetchGatherListAsync(Cm::MemFetchPtr(eaAABBData->mShapeCore), eaAABBData->mList, listTide, 1);

	aabbData=&eaAABBData->mAABBData;
}

PX_FORCE_INLINE void prefetchAsync(const PxcAABBDataDynamic*& aabbData, EAAABBDataDynamic* eaAABBData)
{
	PX_ASSERT(aabbData->mShapeCore);
	eaAABBData->mList.setEA(0, (uintptr_t)aabbData->mShapeCore);
	PX_ASSERT(eaAABBData->mList.getSize(0) == sizeof(PxsShapeCore));
	PX_ASSERT(eaAABBData->mAABBData.mShapeCore==(PxsShapeCore*)eaAABBData->mShapeCore);

	eaAABBData->mAABBData.mBodyAtom=NULL;
	eaAABBData->mAABBData.mRigidCore=NULL;
	eaAABBData->mAABBData.mLocalSpaceAABB=NULL;
	PxU32 listTide=1;
	PxU32 dataTide=0;

	if(aabbData->mBodyAtom)
	{
		PX_ASSERT(aabbData->mRigidCore);

		eaAABBData->mList.setEA(1, (uintptr_t)aabbData->mBodyAtom);
		eaAABBData->mList.setSize(1, sizeof(PxcRigidBody));
		eaAABBData->mAABBData.mBodyAtom=(PxcRigidBody*)(eaAABBData->mRigidBodyPlusBodyCorePlusLocalSpaceAABB + 0);

		eaAABBData->mList.setEA(2, (uintptr_t)aabbData->mRigidCore);
		eaAABBData->mList.setSize(2, sizeof(PxsBodyCore));
		eaAABBData->mAABBData.mRigidCore=(PxsBodyCore*)(eaAABBData->mRigidBodyPlusBodyCorePlusLocalSpaceAABB + sizeof(PxcRigidBody));

		listTide=3;
		dataTide=sizeof(PxcRigidBody) + sizeof(PxsBodyCore);
	}
	else
	{
		eaAABBData->mList.setEA(1, (uintptr_t)aabbData->mRigidCore);
		eaAABBData->mList.setSize(1, sizeof(PxsRigidCore));
		eaAABBData->mAABBData.mRigidCore=(PxsRigidCore*)(eaAABBData->mRigidBodyPlusBodyCorePlusLocalSpaceAABB + 0);
		listTide=2;
		dataTide=sizeof(PxsRigidCore);
	}

	if(aabbData->mLocalSpaceAABB)
	{
		//ea might not be 16-byte aligned. work out the 16-byte aligned address.
		const uintptr_t ea = (uintptr_t)aabbData->mLocalSpaceAABB;
		const uintptr_t ea16 = (ea & ~0x0f);
		eaAABBData->mList.setEA(listTide, ea16);
		eaAABBData->mList.setSize(listTide, 48);
		eaAABBData->mAABBData.mLocalSpaceAABB=(PxBounds3*)(eaAABBData->mRigidBodyPlusBodyCorePlusLocalSpaceAABB + dataTide + ea - ea16);
		listTide++;
		dataTide+=48;
	}

	Cm::memFetchGatherListAsync(Cm::MemFetchPtr(eaAABBData->mShapeCore), eaAABBData->mList, listTide, 1);

	aabbData=&eaAABBData->mAABBData;
}


PX_FORCE_INLINE void wait()
{
	Cm::memFetchWait(1);
}

#else

PX_FORCE_INLINE void prefetchAsync(const PxcAABBDataDynamic* PX_RESTRICT aabbData)
{
	PX_ASSERT(aabbData->mShapeCore);

	Ps::prefetchLine(aabbData->mShapeCore);
	Ps::prefetchLine(aabbData->mBodyAtom);
	Ps::prefetchLine(aabbData->mRigidCore);
	Ps::prefetchLine(aabbData->mLocalSpaceAABB);
}

PX_FORCE_INLINE void prefetchAsync(const PxcAABBDataStatic* PX_RESTRICT aabbData)
{
	PX_ASSERT(aabbData->mShapeCore);
	Ps::prefetchLine(aabbData->mShapeCore);
	Ps::prefetchLine(aabbData->mRigidCore);
}


PX_FORCE_INLINE void wait()
{
}

#endif

PxU32 physx::updateBodyShapeAABBs
(const PxcBpHandle* PX_RESTRICT updatedAABBHandles, const PxU32 numUPdatedAABBHandles, 
 const PxcBpHandle* PX_RESTRICT aabbDataHandles, const PxcAABBDataDynamic* PX_RESTRICT aabbData,
 const PxsComputeAABBParams& params, const bool secondBroadPhase,
 IntegerAABB* bounds, const PxU32 /*maxNumBounds*/)
{
	PX_ASSERT(updatedAABBHandles);
	PX_ASSERT(numUPdatedAABBHandles>0);

#ifdef __SPU__
	EAAABBDataDynamic PX_ALIGN(16, eaaabbData0);
	EAAABBDataDynamic PX_ALIGN(16, eaaabbData1);
	EAAABBDataDynamic* currEaAABBData=&eaaabbData0;
	EAAABBDataDynamic* nextEaAABBData=&eaaabbData1;
	initEAABBBData(*currEaAABBData);
	initEAABBBData(*nextEaAABBData);
#endif //__SPU__

	//Prefetch the very first aabb data.
	const PxU32 nextHandle=updatedAABBHandles[0];
	const PxcAABBDataDynamic* nextAABBData = &aabbData[aabbDataHandles[nextHandle]];
#ifdef __SPU__
	prefetchAsync(nextAABBData, nextEaAABBData);
#else
	prefetchAsync(nextAABBData);
#endif

	//Update aabbs in blocks of four where we can update the aabb and prefetch the data for the next one.
	const PxU32 num = (numUPdatedAABBHandles & 3) ? numUPdatedAABBHandles & ~3 : numUPdatedAABBHandles-4;

	PxF32 numFastMovingObjects = 0;
	for(PxU32 i=0;i<num;i+=4)
	{
		//Hoping the compiler will unroll the loop here to save me copying the same code out 4 times.
		for(PxU32 j=0;j<4;j++)
		{
			const PxcAABBDataDynamic* PX_RESTRICT currAABBData=nextAABBData;

			PX_ASSERT((i+j+1) < numUPdatedAABBHandles);
			const PxU32 nextHandle=updatedAABBHandles[i+j+1];
			nextAABBData = &aabbData[aabbDataHandles[nextHandle]];
			wait();
#ifdef __SPU__
			EAAABBDataDynamic* tmp=currEaAABBData;
			currEaAABBData=nextEaAABBData;
			nextEaAABBData=tmp;
			prefetchAsync(nextAABBData, nextEaAABBData);
#else
			prefetchAsync(nextAABBData);
#endif
			PxBounds3 updatedBodyShapeBounds;
			numFastMovingObjects+=PxsComputeAABB(params, secondBroadPhase, *currAABBData, updatedBodyShapeBounds);
			bounds[updatedAABBHandles[i+j]].encode(updatedBodyShapeBounds);
		}
	}

	//Remaining updateAABB + prefetch of next.
	for(PxU32 i=num;i<(numUPdatedAABBHandles-1);i++)
	{ 
		const PxcAABBDataDynamic* PX_RESTRICT currAABBData=nextAABBData;

		PX_ASSERT((i+1) < numUPdatedAABBHandles);
		const PxU32 nextHandle=updatedAABBHandles[i+1];
		nextAABBData = &aabbData[aabbDataHandles[nextHandle]];
		wait();
#ifdef __SPU__
		EAAABBDataDynamic* tmp=currEaAABBData;
		currEaAABBData=nextEaAABBData;
		nextEaAABBData=tmp;
		prefetchAsync(nextAABBData, nextEaAABBData);
#else
		prefetchAsync(nextAABBData);
#endif
		PxBounds3 updatedBodyShapeBounds;
		numFastMovingObjects+=PxsComputeAABB(params, secondBroadPhase, *currAABBData, updatedBodyShapeBounds);
		bounds[updatedAABBHandles[i]].encode(updatedBodyShapeBounds);
	}

	//Very last update aabb (no prefetch obviously).
	const PxcAABBDataDynamic* PX_RESTRICT currAABBData=nextAABBData;
	PxBounds3 updatedBodyShapeBounds;
	wait();
	numFastMovingObjects+=PxsComputeAABB(params, secondBroadPhase, *currAABBData, updatedBodyShapeBounds);
	bounds[updatedAABBHandles[numUPdatedAABBHandles-1]].encode(updatedBodyShapeBounds);

	//OK, write this value back to the params
	const PxU32 numFastMovingObjectsU32=(PxU32)numFastMovingObjects;
	return numFastMovingObjectsU32;
}


void physx::updateBodyShapeAABBs
(const PxcBpHandle* PX_RESTRICT updatedAABBHandles, const PxU32 numUPdatedAABBHandles, 
 const PxcBpHandle* PX_RESTRICT aabbDataHandles, const PxcAABBDataStatic* PX_RESTRICT aabbData,
 const PxsComputeAABBParams& params, const bool secondBroadPhase,
 IntegerAABB* bounds, const PxU32 /*maxNumBounds*/)
{
	PX_ASSERT(updatedAABBHandles);
	PX_ASSERT(numUPdatedAABBHandles>0);

#ifdef __SPU__
	EAAABBDataStatic PX_ALIGN(16, eaaabbData0);
	EAAABBDataStatic PX_ALIGN(16, eaaabbData1);
	EAAABBDataStatic* currEaAABBData=&eaaabbData0;
	EAAABBDataStatic* nextEaAABBData=&eaaabbData1;
	initEAABBBData(*currEaAABBData);
	initEAABBBData(*nextEaAABBData);
#endif //__SPU__

	//Prefetch the very first aabb data.
	const PxU32 nextHandle=updatedAABBHandles[0];
	const PxcAABBDataStatic* nextAABBData = &aabbData[aabbDataHandles[nextHandle]];
#ifdef __SPU__
	prefetchAsync(nextAABBData, nextEaAABBData);
#else
	prefetchAsync(nextAABBData);
#endif

	//Update aabbs in blocks of four where we can update the aabb and prefetch the data for the next one.
	const PxU32 num = (numUPdatedAABBHandles & 3) ? numUPdatedAABBHandles & ~3 : numUPdatedAABBHandles-4;

	for(PxU32 i=0;i<num;i+=4)
	{
		//Hoping the compiler will unroll the loop here to save me copying the same code out 4 times.
		for(PxU32 j=0;j<4;j++)
		{
			const PxcAABBDataStatic* PX_RESTRICT currAABBData=nextAABBData;

			PX_ASSERT((i+j+1) < numUPdatedAABBHandles);
			const PxU32 nextHandle=updatedAABBHandles[i+j+1];
			nextAABBData = &aabbData[aabbDataHandles[nextHandle]];
			wait();
#ifdef __SPU__
			EAAABBDataStatic* tmp=currEaAABBData;
			currEaAABBData=nextEaAABBData;
			nextEaAABBData=tmp;
			prefetchAsync(nextAABBData, nextEaAABBData);
#else
			prefetchAsync(nextAABBData);
#endif
			PxBounds3 updatedBodyShapeBounds;
			PxsComputeAABB(params, secondBroadPhase, *currAABBData, updatedBodyShapeBounds);
			bounds[updatedAABBHandles[i+j]].encode(updatedBodyShapeBounds);
		}
	}

	//Remaining updateAABB + prefetch of next.
	for(PxU32 i=num;i<(numUPdatedAABBHandles-1);i++)
	{ 
		const PxcAABBDataStatic* PX_RESTRICT currAABBData=nextAABBData;

		PX_ASSERT((i+1) < numUPdatedAABBHandles);
		const PxU32 nextHandle=updatedAABBHandles[i+1];
		nextAABBData = &aabbData[aabbDataHandles[nextHandle]];
		wait();
#ifdef __SPU__
		EAAABBDataStatic* tmp=currEaAABBData;
		currEaAABBData=nextEaAABBData;
		nextEaAABBData=tmp;
		prefetchAsync(nextAABBData, nextEaAABBData);
#else
		prefetchAsync(nextAABBData);
#endif
		PxBounds3 updatedBodyShapeBounds;
		PxsComputeAABB(params, secondBroadPhase, *currAABBData, updatedBodyShapeBounds);
		bounds[updatedAABBHandles[i]].encode(updatedBodyShapeBounds);
	}

	//Very last update aabb (no prefetch obviously).
	const PxcAABBDataStatic* PX_RESTRICT currAABBData=nextAABBData;
	PxBounds3 updatedBodyShapeBounds;
	wait();
	PxsComputeAABB(params, secondBroadPhase, *currAABBData, updatedBodyShapeBounds);
	bounds[updatedAABBHandles[numUPdatedAABBHandles-1]].encode(updatedBodyShapeBounds);
}



////add the space to destBounds that is spanned by a particle at globalStartPoint and rotating by rot around pivot.  Rot is also given in part by the angularDirection (normalized).
////the angle must be below 180 degrees.
//
//#ifndef PX_VMX	//AM: I realize we're not supposed to use #ifdef, but this is the way this macro is branched for all over the place so far, xbox folks should fix.
//
//void PxsBodyShape::temporalPointBounds(const PxVec3& globalStartPoint, 
//						 const PxQuat& rot, const PxVec3& pivot, const PxVec3& angularDirection, 
//						 PxVec3& destBoundsMin, PxVec3& destBoundsMax)
//	{
//
//	PxVec3 offset = globalStartPoint - pivot;
//	PxReal angDirDot = angularDirection.dot(offset);
//
//	PxVec3 center = pivot + angularDirection * angDirDot;
//
//	PxVec3 startRadius = globalStartPoint - center;
//	PxReal radiusSqr = startRadius.magnitudeSquared();
//
//
//	PxVec3 globalEndPoint = rot.rotate(startRadius);		//TODO: optimize to use a matrix instead??
//
//	//from MrE. for rotations < 180 degrees
//
//	PxVec3 v1 = startRadius.cross(angularDirection); //TODO: optimize away these 2 cross products
//	PxVec3 v2 = globalEndPoint.cross(angularDirection);
//
//	globalEndPoint += center;
//
//	PxReal a,b;
//
//	for (PxU32  i = 0; i < 3; i++ ) 
//		{
//		// if the derivative changes sign along this axis during the rotation from start to end
//		if ( PX_SIGN_BITMASK & (PX_IR(v1[i]) ^ PX_IR(v2[i])) ) 
//			{
//			PxReal sq = PxSqrt( radiusSqr * fabs( 1.0f - angularDirection[i] * angularDirection[i] ) );
//			if ( 0.5f * ( globalStartPoint[i] + globalEndPoint[i] ) > center[i] )
//				{
//				a = PxMin( globalStartPoint[i], globalEndPoint[i] );
//				b = center[i] + sq;
//				} 
//			else 
//				{
//				a = center[i] - sq;
//				b = PxMax( globalStartPoint[i], globalEndPoint[i] );
//				}
//			} 
//		else if ( globalStartPoint[i] > globalEndPoint[i] ) 
//			{
//			a = globalEndPoint[i];
//			b = globalStartPoint[i];
//			} 
//		else 
//			{
//			a = globalStartPoint[i];
//			b = globalEndPoint[i];
//			}
//	
//		if (a < destBoundsMin[i])
//			destBoundsMin[i] = a;
//
//		if (b > destBoundsMax[i])
//			destBoundsMax[i]= b;
//		}
//	}
//#else 
//// TODO: give this to XBOX folks to test, it was I guess tested a long time ago.
//
//void PxsBodyShape::temporalPointBounds(const PxVec3& globalStartPoint, 
//						 const PxQuat& rot, const PxVec3& pivot, const PxVec3& angularDirection, 
//						 PxVec3& destBoundsMin, PxVec3& destBoundsMax)
//{
//	Cm::PxSimd::Vector4 globalStartPoint4 = Cm::PxSimd::load(globalStartPoint);
//	Cm::PxSimd::Vector4 pivot4 = Cm::PxSimd::load(pivot);
//	Cm::PxSimd::Vector4 angularDirection4 = Cm::PxSimd::load(angularDirection);
//
//	//assumes layout out matrix(row major).
//	Cm::PxSimd::Vector4 rot4 = Cm::PxSimd::load(rot);	
//	
///*	PxVec3 offset = globalStartPoint - pivot;
//	PxReal angDirDot = angularDirection.dot(offset);*/
//
//	Cm::PxSimd::Vector4 offset = Cm::PxSimd::subtract(globalStartPoint4, pivot4);
//	Cm::PxSimd::Vector4 angDirDot = Cm::PxSimd::dot(angularDirection4, offset);
//
///*	PxVec3 center = pivot + angularDirection * angDirDot;
//
//	PxVec3 startRadius = globalStartPoint - center;
//	PxReal radiusSqr = startRadius.magnitudeSquared();*/
//		
//	Cm::PxSimd::Vector4 center = Cm::PxSimd::multiplyAdd(angularDirection4, angDirDot, pivot4);
//	Cm::PxSimd::Vector4 startRadius = Cm::PxSimd::subtract(globalStartPoint4, center);
//	Cm::PxSimd::Vector4 radiusSq = Cm::PxSimd::dot(startRadius, startRadius);
//
//
//	/*PxVec3 globalEndPoint = rotm*startRadius; 
//	*/
//
//	Cm::PxSimd::Vector4 globalEndPoint = Cm::PxSimd::rotateQuat(rot4, startRadius);
//
//	/*PxVec3 v1; v1.cross(startRadius, angularDirection); //TODO: optimize away these 2 cross products
//	PxVec3 v2; v2.cross(globalEndPoint, angularDirection);*/
//
//	Cm::PxSimd::Vector4 v1 = Cm::PxSimd::cross(startRadius, angularDirection4);
//	Cm::PxSimd::Vector4 v2 = Cm::PxSimd::cross(globalEndPoint, angularDirection4);
//
//	/*globalEndPoint += center;
//
//	PxReal a,b;*/
//
//	globalEndPoint = Cm::PxSimd::add(globalEndPoint, center);
//
//	Cm::PxSimd::Vector4 a,b;
//
//	/*for (PxU32  i = 0; i < 3; i++ ) 
//		{
//		// if the derivative changes sign along this axis during the rotation from start to end
//		if ( PX_SIGN_BITMASK & (PX_IR(v1[i]) ^ PX_IR(v2[i])) ) 
//			{
//			PxReal sq = PxSqrt( radiusSqr * fabs( 1.0f - angularDirection[i] * angularDirection[i] ) );
//			if ( 0.5f * ( globalStartPoint[i] + globalEndPoint[i] ) > center[i] )
//				{
//				a = PxMin( globalStartPoint[i], globalEndPoint[i] );
//				b = center[i] + sq;
//				} 
//			else 
//				{
//				a = center[i] - sq;
//				b = PxMax( globalStartPoint[i], globalEndPoint[i] );
//				}
//			} 
//		
//		*/
//
//	Cm::PxSimd::Vector4 signChange = Cm::PxSimd::and4(Cm::PxSimd::xor4(v1, v2), Cm::PxSimd::signMask());
//	signChange = Cm::PxSimd::intNotEqual(signChange, Cm::PxSimd::zero());
//
//	Cm::PxSimd::Vector4 aChange = Cm::PxSimd::zero();
//	Cm::PxSimd::Vector4 bChange = Cm::PxSimd::zero();
//
//	//if any sign changes on an axis. - is this branch worth it?
//	if(Cm::PxSimd::intNotEqualBool(signChange, Cm::PxSimd::zero()))
//	{
//		//compute result for sign change on all axis at once.
//
//		Cm::PxSimd::Vector4 tmp = Cm::PxSimd::subtract(Cm::PxSimd::one(), Cm::PxSimd::multiply(angularDirection4, angularDirection4));
//		Cm::PxSimd::Vector4 sq;
//		Cm::PxSimd::sqrtAndRcpSqrtEst(Cm::PxSimd::multiply(radiusSq, Cm::PxSimd::abs(tmp)),sq,tmp);
//	
//	
//		Cm::PxSimd::Vector4 t = Cm::PxSimd::multiply(Cm::PxSimd::half(), Cm::PxSimd::add(globalStartPoint4, globalEndPoint));
//		Cm::PxSimd::Vector4 mask = Cm::PxSimd::greater(t, center);
//
//		Cm::PxSimd::Vector4 a_0 = Cm::PxSimd::minimum(globalStartPoint4, globalEndPoint);
//		Cm::PxSimd::Vector4 b_0 = Cm::PxSimd::add(center, sq);
//
//		Cm::PxSimd::Vector4 a_1 = Cm::PxSimd::subtract(center, sq);
//		Cm::PxSimd::Vector4 b_1 = Cm::PxSimd::maximum(globalStartPoint4, globalEndPoint);
//
//		aChange = Cm::PxSimd::select(a_1, a_0, mask);
//		bChange = Cm::PxSimd::select(b_1, b_0, mask);
//	}
//
//	/*
//			else if ( globalStartPoint[i] > globalEndPoint[i] ) 
//			{
//			a = globalEndPoint[i];
//			b = globalStartPoint[i];
//			} 
//		else 
//			{
//			a = globalStartPoint[i];
//			b = globalEndPoint[i];
//			}
//*/
//	Cm::PxSimd::Vector4 gsMask = Cm::PxSimd::greater(globalStartPoint4, globalEndPoint);
//	
//	a = Cm::PxSimd::select(globalStartPoint4, globalEndPoint, gsMask);
//	b = Cm::PxSimd::select(globalEndPoint, globalStartPoint4, gsMask);
//
//	a = Cm::PxSimd::select(a, aChange, signChange);
//	b = Cm::PxSimd::select(b, bChange, signChange);
//
///*		if (a < destBoundsMin[i])
//			destBoundsMin[i] = a;
//
//		if (b > destBoundsMax[i])
//			destBoundsMax[i]= b;
//		}
//		}*/
//
//	Cm::PxSimd::Vector4 boundsMin = Cm::PxSimd::load(destBoundsMin);
//	Cm::PxSimd::Vector4 boundsMax = Cm::PxSimd::load(destBoundsMax);
//
//	boundsMin = Cm::PxSimd::minimum(a, boundsMin);
//	boundsMax = Cm::PxSimd::maximum(b, boundsMax);
//
//	Cm::PxSimd::store(destBoundsMin, boundsMin);
//	Cm::PxSimd::store(destBoundsMax, boundsMax);
//
//}
//#endif
///*
//	//bounds is not emptied so it should be empty when called
//static void PX_INLINE PxsBodyShape::temporalPointBoundsSimple (const PxVec3& globalStartPoint, 
//								const PxVec3& angularDirection, 
//								const PxVec3& pivot, PxBounds3& destBounds)
//	{
//	PxVec3 center = pivot + angularDirection * angularDirection.dot(globalStartPoint - pivot);
//	PxVec3 startRadius = globalStartPoint - center;
//	PxReal radiusSqr = startRadius.magnitudeSquared();
//
//	PxReal radius = PxSqrt(radiusSqr) + gPlueckerEpsilon;
//	PxVec3 minimum = center, maximum = center;
//	minimum.x -= radius;
//	minimum.y -= radius;
//	minimum.z -= radius;
//
//	maximum.x += radius;
//	maximum.y += radius;
//	maximum.z += radius;
//
//	destBounds.include(minimum);
//	destBounds.include(maximum);
//	}
//*/
//
//void PxsBodyShape::boundsofRotSweptOBB(const Gu::Box& startingBox, PxReal angle, 
//								  const PxVec3& angularDir, const PxVec3& pivot,
//								  PxVec3& destBoundsMin, PxVec3& destBoundsMax)
//{
//	// main code is only for rotations < 180 degrees
//	//PX_ASSERT(angle < 3.1416f);
//	if (angle > 3.1416f)
//	{
////		printf("my angle is huge: %f!\n", angle);	//TODO: weird
//		angle = 3.1416f;
//	}
//
//	PxVec3 boxVertices[8];
//	startingBox.computeBoxPoints(boxVertices);
//
//	PxQuat qrot(angle, angularDir);
//
//	//MSvanfeldt, TODO: Possibly a speedup could be the result if the simpler bounds
//	//calculation was employed when pivot is inside the box (or some other condition)
//	//Results in bigger bounds but is _way_ faster to compute
//
//	for (PxU32 j = 0; j < 8;)
//	{
//		temporalPointBounds(boxVertices[j++], qrot, pivot, angularDir, destBoundsMin, destBoundsMax);
//		temporalPointBounds(boxVertices[j++], qrot, pivot, angularDir, destBoundsMin, destBoundsMax);
//		temporalPointBounds(boxVertices[j++], qrot, pivot, angularDir, destBoundsMin, destBoundsMax);
//		temporalPointBounds(boxVertices[j++], qrot, pivot, angularDir, destBoundsMin, destBoundsMax);
//	}
//
//}

//////////////////////////////////////////////////////////////////////////

CompoundCache::CompoundCache(PxcScratchAllocator& allocator)
	: mAllocator(allocator),mCurrentIndex(0),mMaxSize(0),mData(NULL),mSortedData(NULL),mBitmapMemory(NULL)
{
	mPairsArray.reserve(128);
}

//////////////////////////////////////////////////////////////////////////

CompoundCache::~CompoundCache()
{	
}

//////////////////////////////////////////////////////////////////////////

void CompoundCache::prepare()
{
	if(mMaxSize)
	{
		mData =  static_cast<CompoundData*> (mAllocator.alloc(sizeof(CompoundData)*mMaxSize));
	}
}

//////////////////////////////////////////////////////////////////////////

void CompoundCache::flush()
{
	mMaxSize = mCurrentIndex;
	mCurrentIndex = 0;

	if(mData)
	{
		mAllocator.free(mData);
	}
	mData = NULL;

	if(mBitmapMemory)
	{
		mAllocator.free(mBitmapMemory);
	}
	mBitmapMemory = NULL;

	CompoundCacheMap::Iterator it = mCacheMap.getIterator();
	while (!it.done())
	{
		CompoundData* data = it->second;
		mAllocator.free(data->elemData);
		if(data->ownMemory)
		{
			mAllocator.free(data);
		}
		++it;
	}

	mCacheMap.clear();
	if(mSortedData)
	{
		mAllocator.free(mSortedData);
	}
	mSortedData = NULL;
}

CompoundCache::CompoundData* CompoundCache::getCompoundData(const PxU32& key, const PxU32 nbElements)
{
	CompoundCache::CompoundData* compoundData = NULL;

	const CompoundCacheEntry* entry = mCacheMap.find(key);

	if(entry)
	{
		return entry->second;
	}

	// not found in cache create new one
	if(mCurrentIndex < mMaxSize && mData)
	{
		compoundData = &mData[mCurrentIndex];
		compoundData->ownMemory = false;
		mCurrentIndex++;
	}
	else
	{		
		mCurrentIndex++;
		compoundData = static_cast<CompoundData*> (mAllocator.alloc(sizeof(CompoundData)));
		if(compoundData)
		{
			compoundData->ownMemory = true;			
		}
	}

	if(!compoundData)
		return NULL;	

	PxU8* data = static_cast<PxU8*> (mAllocator.alloc(sizeof(ElementData)*nbElements + sizeof(PxU32)*nbElements));

	if(!data)
	{
		if(compoundData->ownMemory)
		{
			mAllocator.free(compoundData);
		}
		return NULL;
	}

	mCacheMap.insert(key,compoundData);

	compoundData->numElements = nbElements;	
	compoundData->elemData = reinterpret_cast<ElementData*> (data);
	compoundData->ranks = reinterpret_cast<PxU32*> (&data[sizeof(ElementData)*nbElements]);
	compoundData->compound = NULL;
	compoundData->elemId = key;
	compoundData->numValidElements = 0;

	return compoundData;
}