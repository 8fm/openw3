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

#ifndef Gu_PERSISTENTCONTACTMANIFOLD_H
#define Gu_PERSISTENTCONTACTMANIFOLD_H

#include "CmPhysXCommon.h"
#include "PsVecTransform.h"
#include "PxUnionCast.h"

namespace physx
{

#ifndef __SPU__  
#define VISUALIZE_PERSISTENT_CONTACT	1   
#endif
#define GU_MANIFOLD_CACHE_SIZE 4
#define GU_SINGLE_MANIFOLD_CACHE_SIZE 6
#define GU_SPHERE_MANIFOLD_CACHE_SIZE 1
#define GU_CAPSULE_MANIFOLD_CACHE_SIZE 3
#define GU_MAX_MANIFOLD_SIZE 4

namespace Cm
{
	class RenderOutput;
}

//class PxTransform;
  
namespace Gu
{
	struct ContactPoint;
	class ContactBuffer;

extern const Ps::aos::FloatV invalidateThresholds[5]; 
extern const Ps::aos::FloatV invalidateQuatThresholds[5];
extern const Ps::aos::FloatV invalidateThresholds2[3]; 
extern const Ps::aos::FloatV invalidateQuatThresholds2[3];


Ps::aos::Mat33V findRotationMatrixFromZAxis(const Ps::aos::Vec3VArg to);


class PersistentContact
{
public:
	PersistentContact()
	{
	}

	PersistentContact(Ps::aos::Vec3V _localPointA, Ps::aos::Vec3V _localPointB, Ps::aos::Vec4V _localNormalPen) : 
	mLocalPointA(_localPointA), mLocalPointB(_localPointB), mLocalNormalPen(_localNormalPen)
	{

	}

	Ps::aos::Vec3V mLocalPointA;
	Ps::aos::Vec3V mLocalPointB;
	Ps::aos::Vec4V mLocalNormalPen; // the (x, y, z) is the local normal, and the w is the penetration depth
};

struct PCMContactPatch
{
public:
	PCMContactPatch()
	{
		mNextPatch = NULL;
		mEndPatch = NULL;
		mRoot = this;
		mPatchMaxPen = Ps::aos::FMax();
	}
	Ps::aos::Vec3V mPatchNormal;
	PCMContactPatch* mNextPatch;
	PCMContactPatch* mEndPatch;
	PCMContactPatch* mRoot;
	Ps::aos::FloatV mPatchMaxPen;
	PxU32 mStartIndex;
	PxU32 mEndIndex;
	PxU32 mTotalSize;
};

class PersistentContactManifold
{
public:

	PersistentContactManifold(): mNumContacts(0), mNumWarmStartPoints(0)
	{
		mRelativeTransform.Invalidate();
	}

	PX_FORCE_INLINE PxU32	getNumContacts() const { return mNumContacts;}

	PX_FORCE_INLINE bool	isEmpty() { return mNumContacts==0; }

	PX_FORCE_INLINE PersistentContact& getContactPoint(const PxU32 index)
	{
		PX_ASSERT(index < GU_MANIFOLD_CACHE_SIZE);
		return mContactPoints[index];
	}
	
	PX_FORCE_INLINE Ps::aos::FloatV maxTransformdelta(const Ps::aos::PsTransformV& curTransform)
	{
		using namespace Ps::aos;
		const Vec4V p0 = Vec4V_From_Vec3V(mRelativeTransform.p);
		const Vec4V q0 = mRelativeTransform.q;
		const Vec4V p1 = Vec4V_From_Vec3V(curTransform.p);
		const Vec4V q1 = curTransform.q;

		const Vec4V dp = V4Abs(V4Sub(p1, p0));
		const Vec4V dq = V4Abs(V4Sub(q1, q0));

		const Vec4V max0 = V4Max(dp, dq);

		//need to work out max from a single vector...
		return V4ExtractMax(max0);
	}

	PX_FORCE_INLINE Ps::aos::Vec4V maxTransformdelta2(const Ps::aos::PsTransformV& curTransform)
	{
		using namespace Ps::aos;
		const Vec4V p0 = Vec4V_From_Vec3V(mRelativeTransform.p);
		const Vec4V q0 = mRelativeTransform.q;
		const Vec4V p1 = Vec4V_From_Vec3V(curTransform.p);
		const Vec4V q1 = curTransform.q;

		const Vec4V dp = V4Abs(V4Sub(p1, p0));
		const Vec4V dq = V4Abs(V4Sub(q1, q0));

		const Vec4V max0 = V4Max(dp, dq);

		//need to work out max from a single vector...
		return max0;
	}

	PX_FORCE_INLINE Ps::aos::FloatV maxTransformPositionDelta(const Ps::aos::Vec3V& curP)
	{
		using namespace Ps::aos;
	
		const Vec3V deltaP = V3Sub(curP, mRelativeTransform.p);
		const Vec4V delta = Vec4V_From_Vec3V(V3Abs(deltaP));
		//need to work out max from a single vector...
		return V4ExtractMax(delta);
	}

	PX_FORCE_INLINE Ps::aos::FloatV maxTransformQuatDelta(const Ps::aos::QuatV& curQ)
	{
		using namespace Ps::aos;
	
		const Vec4V deltaQ = V4Sub(curQ, mRelativeTransform.q);
		const Vec4V delta = V4Abs(deltaQ);
		//need to work out max from a single vector...
		return V4ExtractMax(delta);
	}

	PX_FORCE_INLINE void setRelativeTransform(const Ps::aos::PsTransformV& transform)
	{
		mRelativeTransform = transform;
	}

	PX_FORCE_INLINE PxU32 invalidate(const Ps::aos::PsTransformV& curRTrans, const Ps::aos::FloatVArg minMargin)
	{
		using namespace Ps::aos;
		const BoolV bTrue = BTTTT();
		PX_ASSERT(mNumContacts <= 4);
		const FloatV ratio = invalidateThresholds[mNumContacts];
		const FloatV thresholdP = FMul(minMargin, ratio);
		const FloatV deltaP = maxTransformPositionDelta(curRTrans.p);
	/*	const FloatV thresholdQ = invalidateQuatThresholds[numContacts];
		const FloatV deltaQ = maxTransformQuatDelta(curRTrans.q);
		const BoolV con = BOr(FIsGrtr(deltaP, thresholdP), FIsGrtr(deltaQ, thresholdQ));*/

		const FloatV thresholdQ = invalidateQuatThresholds[mNumContacts];
		const FloatV deltaQ = QuatDot(curRTrans.q, mRelativeTransform.q);
		const BoolV con = BOr(FIsGrtr(deltaP, thresholdP), FIsGrtr(thresholdQ, deltaQ));

		return BAllEq(con, bTrue);
	}

	PX_FORCE_INLINE PxU32 invalidate2(const Ps::aos::PsTransformV& curRTrans, const Ps::aos::FloatVArg minMargin)
	{
		using namespace Ps::aos;
		const BoolV bTrue = BTTTT();
		PX_ASSERT(mNumContacts <= 2);
		const FloatV ratio = invalidateThresholds2[mNumContacts];
		//const FloatV ratio = FloatV_From_F32(0.05f);
		const FloatV thresholdP = FMul(minMargin, ratio);
		const FloatV deltaP = maxTransformPositionDelta(curRTrans.p);
		/*const FloatV thresholdQ = invalidateQuatThresholds[numContacts*2];
		const FloatV deltaQ = maxTransformQuatDelta(curRTrans.q);
		const BoolV con = BOr(FIsGrtr(deltaP, thresholdP), FIsGrtr(deltaQ, thresholdQ));*/

		const FloatV thresholdQ = invalidateQuatThresholds2[mNumContacts];
		const FloatV deltaQ = QuatDot(curRTrans.q, mRelativeTransform.q);
		const BoolV con = BOr(FIsGrtr(deltaP, thresholdP), FIsGrtr(thresholdQ, deltaQ));

		return BAllEq(con, bTrue);
	}


	PX_FORCE_INLINE void removeContactPoint (int index)
	{
		mNumContacts--;
		mContactPoints[index] = mContactPoints[mNumContacts];
	}      
    
	bool validContactDistance(const PersistentContact& pt, const Ps::aos::FloatVArg breakingThreshold) const
	{
		using namespace Ps::aos;
		const FloatV dist = V4GetW(pt.mLocalNormalPen);
		return FAllGrtr(breakingThreshold, dist) != 0;
		//return FAllGrtr(breakingThreshold, pt.dist) != 0;
	}

	void clearManifold()
	{
		mNumWarmStartPoints = 0;
		mNumContacts = 0;
		mRelativeTransform.Invalidate();
	}

	void initialize()
	{
		clearManifold();
	}

	bool replaceManifoldPoint(const Ps::aos::Vec3VArg localPointA, const Ps::aos::Vec3VArg localPointB, const Ps::aos::Vec4VArg localNormalPen, const Ps::aos::FloatVArg replaceBreakingThreshold);
	bool replaceManifoldPoint2(const Ps::aos::Vec3VArg localPointA, const Ps::aos::Vec3VArg localPointB, const Ps::aos::Vec4VArg localNormalPen, const Ps::aos::FloatVArg replaceBreakingThreshold);

	PxU32 addManifoldPoint( const Ps::aos::Vec3VArg localPointA, const Ps::aos::Vec3VArg localPointB, const Ps::aos::Vec4VArg localNormalAPen, const Ps::aos::FloatVArg replaceBreakingThreshold);
	PxU32 addManifoldPoint2( const Ps::aos::Vec3VArg localPointA, const Ps::aos::Vec3VArg localPointB, const Ps::aos::Vec4VArg localNormalAPen, const Ps::aos::FloatVArg replaceBreakingThreshold);//max two points of contacts  
	void addBatchManifoldContacts2( const PersistentContact* manifoldPoints, const PxU32 numPoints);//max two points of contacts             
	void addBatchManifoldContacts(const PersistentContact* manifoldPoints, const PxU32 numPoints);
	void removeDuplidateManifoldContacts(PersistentContact* manifoldPoints, PxU32& numPoints, const Ps::aos::FloatVArg replaceBreakingThreshold);
	void reduceBatchContacts2(const PersistentContact* manifoldPoints, const PxU32 numPoints);
	void reduceBatchContacts(const PersistentContact* manifoldPoints, const PxU32 numPoints);

	PxU32 reduceContactsForPCM(const Ps::aos::Vec3VArg localPointA, const Ps::aos::Vec3VArg localPointB, const Ps::aos::Vec4VArg localNormalPen);
	PxU32 reduceContactSegment(const Ps::aos::Vec3VArg localPointA, const Ps::aos::Vec3VArg localPointB, const Ps::aos::Vec4VArg localNormalPen);
  
	//void refreshContactPoints(const Ps::aos::PsTransformV& relTra, const Ps::aos::FloatVArg projectBreakingThreshold, const Ps::aos::FloatVArg contactOffset);
	void refreshContactPoints(const Ps::aos::PsMatTransformV& relTra, const Ps::aos::FloatVArg projectBreakingThreshold, const Ps::aos::FloatVArg contactOffset);
	void addManifoldContactsToContactBuffer(Gu::ContactBuffer& contactBuffer, const Ps::aos::Vec3VArg normal, const Ps::aos::PsTransformV& transf1);
	void addManifoldContactsToContactBuffer(Gu::ContactBuffer& contactBuffer, const Ps::aos::Vec3VArg normal, const Ps::aos::PsTransformV& transf0, const Ps::aos::FloatVArg radius);


	Ps::aos::Vec3V getWorldNormal(const Ps::aos::PsTransformV& trB);
	Ps::aos::Vec3V getLocalNormal();
	//Ps::aos::Vec3V getWorldNormal(const Ps::aos::PsMatTransformV& trB);
	
	void drawShrinkVertice(Cm::RenderOutput& out, const PxVec3* __restrict vertices, const PxU32 numVertices, const PxTransform& transf, const PxF32 size, const PxU32 colorIndex);
	void drawManifold(Cm::RenderOutput& out, const Ps::aos::PsTransformV& trA, const Ps::aos::PsTransformV& trB);
	void drawManifold(const PersistentContact& m, Cm::RenderOutput& out, const Ps::aos::PsTransformV& trA, const Ps::aos::PsTransformV& trB);
	static void drawPoint(Cm::RenderOutput& out, const Ps::aos::Vec3VArg p, const PxF32 size, const PxU32 color);
	static void drawLine(Cm::RenderOutput& out, const Ps::aos::Vec3VArg p0, const Ps::aos::Vec3VArg p1, const PxU32 color = 0xff00ffff);
	static void drawPolygon( Cm::RenderOutput& out, const Ps::aos::PsTransformV& transform,  Ps::aos::Vec3V* points, const PxU32 numVerts, const PxU32 color = 0xff00ffff);

	PersistentContact mContactPoints[GU_MANIFOLD_CACHE_SIZE];
	Ps::aos::PsTransformV mRelativeTransform;//aToB
	//PxU32 triangleIndex[4];
	PxU8 mNumContacts;
	PxU8 mNumWarmStartPoints;
	PxU8 mAIndice[4];
	PxU8 mBIndice[4];  
};


class SinglePersistentContactManifold
{
public:

	SinglePersistentContactManifold(): mNumContacts(0)
	{
	}

	PX_FORCE_INLINE PxU32	getNumContacts() const { return mNumContacts;}

	PX_FORCE_INLINE bool	isEmpty() { return mNumContacts==0; }

	PX_FORCE_INLINE PersistentContact& getContactPoint(const PxU32 index)
	{
		PX_ASSERT(index < GU_SINGLE_MANIFOLD_CACHE_SIZE);
		return mContactPoints[index];
	}

	PX_FORCE_INLINE void removeContactPoint (int index)
	{
		mNumContacts--;
		mContactPoints[index] = mContactPoints[mNumContacts];
	}  

	PX_FORCE_INLINE void clearManifold()
	{
		mNumContacts = 0;
	}

	PX_FORCE_INLINE void initialize()
	{
		clearManifold();
	}


	PX_FORCE_INLINE Ps::aos::Vec3V getWorldNormal(const Ps::aos::PsTransformV& trB)
	{
		using namespace Ps::aos;
		//const FloatV numContactsV = FloatV_From_F32((PxF32)numContacts);
		Vec4V nPen = mContactPoints[0].mLocalNormalPen;
		for(PxU32 i =1; i < mNumContacts; ++i)
		{
			nPen = V4Add(nPen, mContactPoints[i].mLocalNormalPen);
		}

		const Vec3V n = Vec3V_From_Vec4V(nPen);
		return V3Normalize(trB.rotate(n));
	}

	PX_FORCE_INLINE Ps::aos::Vec3V getLocalNormal()
	{
		using namespace Ps::aos;
		//const FloatV numContactsV = FloatV_From_F32((PxF32)numContacts);
		Vec4V nPen = mContactPoints[0].mLocalNormalPen;
		for(PxU32 i =1; i < mNumContacts; ++i)
		{
			nPen = V4Add(nPen, mContactPoints[i].mLocalNormalPen);
		}
		return V3Normalize(Vec3V_From_Vec4V(nPen));
	}
	
	
	Ps::aos::FloatV reduceBatchContactsConvex(const PersistentContact* manifoldContactExt, const PxU32 numContacts, PCMContactPatch& patch);
	Ps::aos::FloatV reduceBatchContactsSphere(const PersistentContact* manifoldContactExt, const PxU32 numContacts, PCMContactPatch& patch);
	Ps::aos::FloatV reduceBatchContactsCapsule(const PersistentContact* manifoldContactExt, const PxU32 numContacts, PCMContactPatch& patch);

	Ps::aos::FloatV addBatchManifoldContactsConvex(const PersistentContact* manifoldContact, const PxU32 numContacts, PCMContactPatch& patch, const Ps::aos::FloatVArg replaceBreakingThreshold);
	Ps::aos::FloatV addBatchManifoldContactsSphere(const PersistentContact* manifoldContact, const PxU32 numContacts, PCMContactPatch& patch, const Ps::aos::FloatVArg replaceBreakingThreshold);
	Ps::aos::FloatV addBatchManifoldContactsCapsule(const PersistentContact* manifoldContact, const PxU32 numContacts, PCMContactPatch& patch, const Ps::aos::FloatVArg replaceBreakingThreshold);
	void reduceBatchContactsKeepAllDeepestContacts(const PersistentContact* manifoldContactExt, const PxU32 numContacts, PCMContactPatch& patch);
	static PxU32 reduceContacts(PersistentContact* manifoldContactExt, PxU32 numContacts);

	Ps::aos::FloatV refreshContactPoints(const Ps::aos::PsMatTransformV& relTra, const Ps::aos::FloatVArg projectBreakingThreshold, const Ps::aos::FloatVArg contactOffset);
	//PxU32 addManifoldContactsToContactBuffer(Gu::ContactBuffer& contactBuffer, const Ps::aos::Vec3VArg normal, const Ps::aos::PsTransformV& transf1);

	void drawManifold(Cm::RenderOutput& out, const Ps::aos::PsTransformV& trA, const Ps::aos::PsTransformV& trB);

	PersistentContact mContactPoints[GU_SINGLE_MANIFOLD_CACHE_SIZE];//192 bytes
	PxU32 mNumContacts;//208 bytes
	
};

class MultiplePersistentContactManifold
{
public:
	MultiplePersistentContactManifold():mNumManifolds(0), mNumTotalContacts(0)
	{
		mRelativeTransform.Invalidate();
	}

	PX_FORCE_INLINE void setRelativeTransform(const Ps::aos::PsTransformV& transform)
	{
		mRelativeTransform = transform;
	}

	PX_FORCE_INLINE Ps::aos::FloatV maxTransformPositionDelta(const Ps::aos::Vec3V& curP)
	{
		using namespace Ps::aos;
	
		const Vec3V deltaP = V3Sub(curP, mRelativeTransform.p);
		const Vec4V delta = Vec4V_From_Vec3V(V3Abs(deltaP));
		//need to work out max from a single vector...
		return V4ExtractMax(delta);
	}

	PX_FORCE_INLINE Ps::aos::FloatV maxTransformQuatDelta(const Ps::aos::QuatV& curQ)
	{
		using namespace Ps::aos;
	
		const Vec4V deltaQ = V4Sub(curQ, mRelativeTransform.q);
		const Vec4V delta = V4Abs(deltaQ);
		//need to work out max from a single vector...
		return V4ExtractMax(delta);
	}

	PX_FORCE_INLINE PxU32 invalidate(const Ps::aos::PsTransformV& curRTrans, const Ps::aos::FloatVArg minMargin, const Ps::aos::FloatVArg ratio)
	{
		using namespace Ps::aos;
		const BoolV bTrue = BTTTT();
		//const FloatV ratio = FloatV_From_F32(0.02f);
		const FloatV thresholdP = FMul(minMargin, ratio);
		const FloatV deltaP = maxTransformPositionDelta(curRTrans.p);
		//const FloatV thresholdQ = FloatV_From_F32(0.99999f);
		const FloatV thresholdQ = FLoad(0.9998f);//about 1 degree
		const FloatV deltaQ = QuatDot(curRTrans.q, mRelativeTransform.q);
		const BoolV con = BOr(FIsGrtr(deltaP, thresholdP), FIsGrtr(thresholdQ, deltaQ));

		return BAllEq(con, bTrue);
	}

	PX_FORCE_INLINE PxU32 invalidate(const Ps::aos::PsTransformV& curRTrans, const Ps::aos::FloatVArg minMargin)
	{
		using namespace Ps::aos;
		return invalidate(curRTrans, minMargin, FLoad(0.2f));
	}

	
	
	PX_FORCE_INLINE void refineContactPatchConnective(PCMContactPatch** contactPatch, PxU32 numContactPatch, PersistentContact* manifoldContacts, const Ps::aos::FloatVArg acceptanceEpsilon)
	{
		PX_UNUSED(manifoldContacts);

		using namespace Ps::aos;
	
		//const FloatV distanceTolerance = FloatV_From_F32(0.04f);
		//work out the contact patch connectivity, the patchNormal should be in the local space of mesh
		for(PxU32 i=0; i<numContactPatch; ++i)
		{
			PCMContactPatch* patch = contactPatch[i];
			patch->mRoot = patch;
			patch->mEndPatch = patch;
			patch->mTotalSize = patch->mEndIndex - patch->mStartIndex;
			patch->mNextPatch = NULL;
	
			for(PxU32 j=i; j>0; --j)
			{
				PCMContactPatch* other = contactPatch[j-1];
				const FloatV d = V3Dot(patch->mPatchNormal, other->mRoot->mPatchNormal);
				//const FloatV dist = FAbs(V3Dot(patch->normal, V3Sub(manifoldContacts[patch->startIndex].localPointB, manifoldContacts[other->root->startIndex].localPointB)));
				if(FAllGrtrOrEq(d, acceptanceEpsilon))// && FAllGrtr(distanceTolerance, dist))//less than 5 degree
				{
					
					other->mNextPatch = patch;
					other->mRoot->mEndPatch = patch;
					patch->mRoot = other->mRoot;
					other->mRoot->mTotalSize += patch->mEndIndex - patch->mStartIndex;
					//other.root->maxPen = FMin(other.root->maxPen, patch.maxPen);
					break;
				}
			}
		}
	}

	PX_FORCE_INLINE PxU32 reduceManifoldContacts(PCMContactPatch* contactPatch, PxU32 numContactPatch, PersistentContact* manifoldContacts, PxU32 numContacts, const Ps::aos::FloatVArg sqReplaceBreaking, const Ps::aos::FloatVArg acceptanceEpsilon)
	{
		using namespace Ps::aos;
		//const FloatV eps = FloatV_From_F32(0.996f);

		for(PxU32 i=0; i<numContactPatch; ++i)
		{
			PCMContactPatch& patch = contactPatch[i];
			for(PxU32 k = patch.mStartIndex; k<patch.mEndIndex; ++k)
			{
			/*	for(PxU32 j = k+1; j < patch.endIndex; ++j)
				{
					Vec3V contact1 = manifoldContacts[j].localPointB;
					Vec3V dif = V3Sub(contact1, contact0);
					FloatV d = V3Dot(dif, dif);
					PX_ASSERT(!FAllGrtr(sqReplaceBreaking, d));
				}*/
				for(PxU32 j=i+1; j<numContactPatch; ++j)  
				{
					PCMContactPatch& other = contactPatch[j];
					const FloatV d = V3Dot(patch.mPatchNormal, other.mPatchNormal);
					if(FAllGrtrOrEq(d, acceptanceEpsilon))//less than 5 degree
					{
						for(PxU32 l = other.mStartIndex; l < other.mEndIndex; ++l)
						{
							Vec3V dif = V3Sub(manifoldContacts[l].mLocalPointB, manifoldContacts[k].mLocalPointB);
							FloatV d = V3Dot(dif, dif);
							if(FAllGrtr(sqReplaceBreaking, d))
							{
								manifoldContacts[l] = manifoldContacts[other.mEndIndex-1];
								other.mEndIndex--;
								numContacts--;
								l--;
							}
							//PX_ASSERT(FAllGrtr(d, FMul(mReplaceBreakingThreshold, mReplaceBreakingThreshold)));
						}
					}
				}
			}
		}
		return numContacts;
	}

	//PX_FORCE_INLINE void refreshManifold(const Ps::aos::PsMatTransformV& relTra, const Ps::aos::FloatVArg projectBreakingThreshold, const Ps::aos::FloatVArg contactDist)
	//{
	//	using namespace Ps::aos;
	//
	//	//refresh manifold contacts
	//	for(PxU32 i=mNumManifolds; i>0; --i)
	//	{
	//		PxU32 ind = mManifoldIndices[i-1];
	//		PX_ASSERT(mManifoldIndices[i-1] < GU_MAX_MANIFOLD_SIZE);
	//		FloatV _maxPen = mManifolds[ind].refreshContactPoints(relTra, projectBreakingThreshold, contactDist);

	//		if(mManifolds[ind].isEmpty())
	//		{
	//			//swap the index with the next manifolds
	//			PxU32 index = mManifoldIndices[--mNumManifolds];
	//			mManifoldIndices[mNumManifolds] = ind;
	//			mManifoldIndices[i-1] = index;
	//		}
	//		else
	//		{
	//			//mMaxPen[ind] =_maxPen;
	//			PxF32_From_FloatV(_maxPen, &mMaxPen[ind]);
	//		}
	//	}

	///*	for(PxU32 i=0; i<numManifolds; ++i)
	//	{
	//		PX_ASSERT(manifoldIndices[i] < PXC_MAX_MANIFOLD_SIZE);
	//	}*/
	//}

	PX_FORCE_INLINE void refreshManifold(const Ps::aos::PsMatTransformV& relTra, const Ps::aos::FloatVArg projectBreakingThreshold, const Ps::aos::FloatVArg contactDist)
	{
		using namespace Ps::aos;
	
		//refresh manifold contacts
		for(PxU32 i=0; i < mNumManifolds; ++i)
		{
			PxU8 ind = mManifoldIndices[i];
			PX_ASSERT(mManifoldIndices[i] < GU_MAX_MANIFOLD_SIZE);
			PxU32 nextInd = PxMin(i, mNumManifolds-2u)+1;
			Ps::prefetchLine(&mManifolds[mManifoldIndices[nextInd]]);
			Ps::prefetchLine(&mManifolds[mManifoldIndices[nextInd]],128);
			Ps::prefetchLine(&mManifolds[mManifoldIndices[nextInd]],256);
			FloatV _maxPen = mManifolds[ind].refreshContactPoints(relTra, projectBreakingThreshold, contactDist);

			if(mManifolds[ind].isEmpty())
			{
				//swap the index with the next manifolds
				PxU8 index = mManifoldIndices[--mNumManifolds];
				mManifoldIndices[mNumManifolds] = ind;
				mManifoldIndices[i] = index;
				i--;
			}
			else
			{
				//mMaxPen[ind] =_maxPen;
				FStore(_maxPen, &mMaxPen[ind]);
			}
		}

	/*	for(PxU32 i=0; i<numManifolds; ++i)
		{
			PX_ASSERT(manifoldIndices[i] < PXC_MAX_MANIFOLD_SIZE);
		}*/
	}


	PX_FORCE_INLINE void initialize()
	{
		mNumManifolds = 0;
		mNumTotalContacts = 0;
		mRelativeTransform.Invalidate();
		for(PxU8 i=0; i<GU_MAX_MANIFOLD_SIZE; ++i)
		{
			mManifolds[i].initialize();
			mManifoldIndices[i] = i;
		}
	}

	PX_FORCE_INLINE void clearManifold()
	{
		for(PxU8 i=0; i<mNumManifolds; ++i)
		{
			mManifolds[i].clearManifold();
			//mActive[i] = false;
		}
		mNumManifolds = 0;
		mNumTotalContacts = 0;
		mRelativeTransform.Invalidate();
	}

	PX_FORCE_INLINE SinglePersistentContactManifold* getManifold(const PxU32 index)
	{
		PX_ASSERT(index < GU_MAX_MANIFOLD_SIZE);
		return &mManifolds[mManifoldIndices[index]];
	}

	PX_FORCE_INLINE SinglePersistentContactManifold* getEmptyManifold()
	{
		if(mNumManifolds < GU_MAX_MANIFOLD_SIZE)
			return  &mManifolds[mManifoldIndices[mNumManifolds]];
		return NULL;
	}

	void addManifoldContactPoints(PersistentContact* manifoldContact, PxU32 numManifoldContacts, PCMContactPatch** contactPatch, const PxU32 numPatch, 
		const Ps::aos::FloatVArg sqReplaceBreakingThreshold, const Ps::aos::FloatVArg acceptanceEpsilon, PxU8 maxContactsPerManifold);
	bool addManifoldContactsToContactBuffer(Gu::ContactBuffer& contactBuffer, const Ps::aos::PsTransformV& transf1);
	bool addManifoldContactsToContactBuffer(Gu::ContactBuffer& contactBuffer, const Ps::aos::PsTransformV& trA, const Ps::aos::PsTransformV& trB, const Ps::aos::FloatVArg radius);
	void drawManifold(Cm::RenderOutput& out, const Ps::aos::PsTransformV& trA, const Ps::aos::PsTransformV& trB);
	static void drawLine(Cm::RenderOutput& out, const Ps::aos::Vec3VArg p0, const Ps::aos::Vec3VArg p1, const PxU32 color = 0xff00ffff);
	static void drawLine(Cm::RenderOutput& out, const PxVec3 p0, const PxVec3 p1, const PxU32 color = 0xff00ffff);
	static void drawPoint(Cm::RenderOutput& out, const Ps::aos::Vec3VArg p, const PxF32 size, const PxU32 color = 0x00ff0000);
	static void drawPolygon( Cm::RenderOutput& out, const Ps::aos::PsTransformV& transform,  Ps::aos::Vec3V* points, const PxU32 numVerts, const PxU32 color = 0xff00ffff);

	PxF32 mMaxPen[GU_MAX_MANIFOLD_SIZE];
	Ps::aos::PsTransformV mRelativeTransform;//aToB
	PxU8 mManifoldIndices[GU_MAX_MANIFOLD_SIZE];
	PxU8 mNumManifolds;
	PxU8 mNumTotalContacts;
	SinglePersistentContactManifold mManifolds[GU_MAX_MANIFOLD_SIZE];
	
	
};


PX_FORCE_INLINE Ps::aos::Vec3V PersistentContactManifold::getWorldNormal(const Ps::aos::PsTransformV& trB)
{
	using namespace Ps::aos;
	//const FloatV numContactsV = FloatV_From_F32((PxF32)numContacts);
	Vec4V nPen = mContactPoints[0].mLocalNormalPen;
	for(PxU32 i =1; i < mNumContacts; ++i)
	{
		nPen = V4Add(nPen, mContactPoints[i].mLocalNormalPen);
	}

	const Vec3V n = Vec3V_From_Vec4V(nPen);
	return V3Normalize(trB.rotate(n));
}

PX_FORCE_INLINE Ps::aos::Vec3V PersistentContactManifold::getLocalNormal()
{
	using namespace Ps::aos;
	//const FloatV numContactsV = FloatV_From_F32((PxF32)numContacts);
	Vec4V nPen = mContactPoints[0].mLocalNormalPen;
	for(PxU32 i =1; i < mNumContacts; ++i)
	{
		nPen = V4Add(nPen, mContactPoints[i].mLocalNormalPen);
	}
	return V3Normalize(Vec3V_From_Vec4V(nPen));
}


PX_FORCE_INLINE void PersistentContactManifold::refreshContactPoints(const Ps::aos::PsMatTransformV& aToB, const Ps::aos::FloatVArg projectBreakingThreshold, const Ps::aos::FloatVArg contactOffset)
{
	using namespace Ps::aos;
	const FloatV sqProjectBreakingThreshold =  FMul(projectBreakingThreshold, projectBreakingThreshold); 
	const BoolV bTrue = BTTTT();

	//PX_ASSERT(numContacts <= PXC_MANIFOLD_CACHE_SIZE);


	// first refresh worldspace positions and distance
	for (PxU32 i=mNumContacts; i > 0; --i)
	{
		PersistentContact& manifoldPoint = mContactPoints[i-1];
		const Vec3V localAInB = aToB.transform( manifoldPoint.mLocalPointA ); // from a to b
		const Vec3V localBInB = manifoldPoint.mLocalPointB;
		const Vec3V v = V3Sub(localAInB, localBInB); 

		//const Vec3V localNormal = V3Normalize(Vec3V_From_Vec4V(manifoldPoint.localNormalPen)); // normal in b space
		const Vec3V localNormal = Vec3V_From_Vec4V(manifoldPoint.mLocalNormalPen); // normal in b space
		const FloatV dist= V3Dot(v, localNormal);

		const Vec3V projectedPoint = V3NegScaleSub(localNormal,  dist, localAInB);//manifoldPoint.worldPointA - manifoldPoint.worldPointB * manifoldPoint.m_distance1;
		const Vec3V projectedDifference = V3Sub(localBInB, projectedPoint);

		const FloatV distance2d = V3Dot(projectedDifference, projectedDifference);
		const BoolV con = BOr(FIsGrtr(dist, contactOffset), FIsGrtr(distance2d, sqProjectBreakingThreshold));
		if(BAllEq(con, bTrue))
		{
			removeContactPoint(i-1);
		} 
		else
		{
			manifoldPoint.mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(localNormal), dist);
		}
	}
}

#ifdef PX_PS3
PX_FORCE_INLINE PersistentContact* PX_CP_TO_PCP(physx::Gu::ContactPoint* contactPoint)
{
	return physx::PxUnionCast<Gu::PersistentContact*, Gu::ContactPoint*>(contactPoint);
}
#else
#define PX_CP_TO_PCP(contactPoint)				((Gu::PersistentContact*)(contactPoint))
#endif

}
}

#endif
