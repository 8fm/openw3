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

#ifndef GU_PCM_CONTACT_CONVEX_COMMON_H
#define GU_PCM_CONTACT_CONVEX_COMMON_H

#define	PCM_MAX_CONTACTPATCH_SIZE	32

#include "GuContactBuffer.h"
#include "GuVecCapsule.h"
#include "GuPCMTriangleContactGen.h"
#include "Ice/IceContainer.h"

#ifdef	PCM_LOW_LEVEL_DEBUG
#include "CmRenderOutput.h"
extern physx::Cm::RenderOutput* gRenderOutPut;
#endif

#define PCM_LOCAL_CONTACTS_SIZE		1088

namespace physx
{

namespace Gu
{

class PCMMeshContactGeneration
{
public:
	PCMContactPatch							mContactPatch[PCM_MAX_CONTACTPATCH_SIZE];
	PCMContactPatch*						mContactPatchPtr[PCM_MAX_CONTACTPATCH_SIZE];
	const Ps::aos::FloatV					mContactDist;
	const Ps::aos::FloatV					mReplaceBreakingThreshold;
	const Ps::aos::PsTransformV&			mConvexTransform;
	const Ps::aos::PsTransformV&			mMeshTransform;
	Gu::MultiplePersistentContactManifold&	mMultiManifold;
	Gu::ContactBuffer&						mContactBuffer;

	Ps::aos::FloatV							mAcceptanceEpsilon;
	Ps::aos::FloatV							mSqReplaceBreakingThreshold;
	Ps::aos::PsMatTransformV				mMeshToConvex;
	Gu::PersistentContact*					mManifoldContacts;
	PxU32									mNumContacts;
	PxU32									mNumContactPatch;
	PxU32									mNumCalls;

	PCMMeshContactGeneration(
		const Ps::aos::FloatVArg	contactDist,
		const Ps::aos::FloatVArg	replaceBreakingThreshold,
		const Ps::aos::PsTransformV& convexTransform,
		const Ps::aos::PsTransformV& meshTransform,
		Gu::MultiplePersistentContactManifold& multiManifold,
		Gu::ContactBuffer& contactBuffer

	) :
		mContactDist(contactDist),
		mReplaceBreakingThreshold(replaceBreakingThreshold),
		mConvexTransform(convexTransform),
		mMeshTransform(meshTransform),
		mMultiManifold(multiManifold),
		mContactBuffer(contactBuffer)
		
	{
		using namespace Ps::aos;
		mNumContactPatch = 0;
		mNumContacts = 0;
		mNumCalls = 0;

		mMeshToConvex = mConvexTransform.transformInv(mMeshTransform);

		//Assign the PCMContactPatch to the PCMContactPathPtr
		for(PxU32 i=0; i<PCM_MAX_CONTACTPATCH_SIZE; ++i)
		{
			mContactPatchPtr[i] = &mContactPatch[i];
		}
		mManifoldContacts = PX_CP_TO_PCP(contactBuffer.contacts);

		mSqReplaceBreakingThreshold = FMul(replaceBreakingThreshold, replaceBreakingThreshold);

		mAcceptanceEpsilon = FLoad(0.996);//5 degree
		//mAcceptanceEpsilon = FloatV_From_F32(0.9999);//5 degree
	}

	template <PxU32 TriangleCount, typename Derived>
	bool processTriangleCache(Gu::TriangleCache<TriangleCount>& cache)
	{
		PxU32 count = cache.mNumTriangles;
		PxVec3* verts = cache.mVertices;
		PxU32* vertInds = cache.mIndices;
		PxU32* triInds = cache.mTriangleIndex;
		PxU8* edgeFlags = cache.mEdgeFlags;
		while(count--)
		{
			((Derived*)(this))->processTriangle(verts, *triInds, *edgeFlags, vertInds);
			verts += 3;
			vertInds += 3;
			triInds++;
			edgeFlags++;
		}
		return true;
	}
	void insertionSort();
	void addManifoldPointToPatch(const Ps::aos::Vec3VArg currentPatchNormal, const Ps::aos::FloatVArg maxPen, const PxU32 previousNumContacts);
	void processContacts(const PxU8 maxContactPerManifold, const bool isNotLastPatch = true);
private:
	PCMMeshContactGeneration& operator=(const PCMMeshContactGeneration&);
};

PX_FORCE_INLINE void PCMMeshContactGeneration::addManifoldPointToPatch(const Ps::aos::Vec3VArg currentPatchNormal, const Ps::aos::FloatVArg maxPen, const PxU32 previousNumContacts)
{
	using namespace Ps::aos;

	bool foundPatch = false;
	if(mNumContactPatch > 0)
	{
		if(FAllGrtr(V3Dot(mContactPatch[mNumContactPatch-1].mPatchNormal, currentPatchNormal), mAcceptanceEpsilon))
		{
			//get the last patch
			PCMContactPatch& patch = mContactPatch[mNumContactPatch-1];

			//remove duplicate contacts
			for(PxU32 i = patch.mStartIndex; i<patch.mEndIndex; ++i)
			{
				for(PxU32 j = previousNumContacts; j<mNumContacts; ++j)
				{
					Vec3V dif = V3Sub(mManifoldContacts[j].mLocalPointB, mManifoldContacts[i].mLocalPointB);
					FloatV d = V3Dot(dif, dif);
					if(FAllGrtr(mSqReplaceBreakingThreshold, d))
					{
						if(FAllGrtr(V4GetW(mManifoldContacts[i].mLocalNormalPen), V4GetW(mManifoldContacts[j].mLocalNormalPen)))
						{
							//The new contact is deeper than the old contact so we keep the deeper contact
							mManifoldContacts[i] = mManifoldContacts[j];
						}
						mManifoldContacts[j] = mManifoldContacts[mNumContacts-1];
						mNumContacts--;
						j--;
					}
				}
			}
			patch.mEndIndex = mNumContacts;
			patch.mPatchMaxPen = FMin(patch.mPatchMaxPen, maxPen);
			foundPatch = true;
		}
	}
	if(!foundPatch)
	{
		mContactPatch[mNumContactPatch].mStartIndex = previousNumContacts;
		mContactPatch[mNumContactPatch].mEndIndex = mNumContacts;
		mContactPatch[mNumContactPatch].mPatchMaxPen = maxPen;
		mContactPatch[mNumContactPatch++].mPatchNormal = currentPatchNormal;
	}
}

PX_FORCE_INLINE  void PCMMeshContactGeneration::insertionSort()
{
	using namespace Ps::aos;
	//sort the contact patch based on the max penetration
	for(PxU32 i=1; i<mNumContactPatch; ++i)
	{
		const PxU32 indexi = i-1;
		if(FAllGrtr(mContactPatchPtr[indexi]->mPatchMaxPen, mContactPatchPtr[i]->mPatchMaxPen))
		{
			//swap
			PCMContactPatch* temp = mContactPatchPtr[indexi];
			mContactPatchPtr[indexi] = mContactPatchPtr[i];
			mContactPatchPtr[i] = temp;

			for(PxI32 j=i-2; j>=0; j--)
			{
				const PxU32 indexj = j+1;
				if(FAllGrtrOrEq(mContactPatchPtr[indexj]->mPatchMaxPen, mContactPatchPtr[j]->mPatchMaxPen))
					break;
				//swap
				PCMContactPatch* temp = mContactPatchPtr[indexj];
				mContactPatchPtr[indexj] = mContactPatchPtr[j];
				mContactPatchPtr[j] = temp;
			}
		}
	}

	//for(PxU32 i=1; i<mNumContactPatch; i++)
	//{
	//	PX_ASSERT(FAllGrtrOrEq(mContactPatchPtr[i]->maxPen, mContactPatchPtr[i-1]->maxPen));
	//}

}


PX_FORCE_INLINE void PCMMeshContactGeneration::processContacts(const PxU8 maxContactPerManifold, bool isNotLastPatch)
{
	using namespace Ps::aos;
	
	mMultiManifold.reduceManifoldContacts(mContactPatch, mNumContactPatch, mManifoldContacts, mNumContacts, mSqReplaceBreakingThreshold, mAcceptanceEpsilon);
	insertionSort();
	mMultiManifold.refineContactPatchConnective(mContactPatchPtr, mNumContactPatch, mManifoldContacts, mAcceptanceEpsilon);
	mMultiManifold.addManifoldContactPoints(mManifoldContacts, mNumContacts, mContactPatchPtr, mNumContactPatch, mSqReplaceBreakingThreshold, mAcceptanceEpsilon, maxContactPerManifold);
	
	mNumContacts = 0;
	mNumContactPatch = 0;

	if(isNotLastPatch)
	{
		//remap the contact patch pointer to contact patch
		for(PxU32 i=0; i<PCM_MAX_CONTACTPATCH_SIZE; ++i)
		{
			mContactPatchPtr[i] = &mContactPatch[i];
		}
	}
}

struct PCMDeferredPolyData
{
public:
	PxVec3	mVerts[3];
	PxU32	mInds[3];
	PxU32	mTriangleIndex;
	PxU32	mFeatureIndex;
	PxU8	triFlags;
};

#define MAX_CACHE_SIZE	128

class PCMConvexVsMeshContactGeneration : public PCMMeshContactGeneration
{
public:
	//Gu::CacheMap<Gu::CachedVertex, MAX_CACHE_SIZE>				mVertexCache;
	Gu::CacheMap<Gu::CachedEdge, MAX_CACHE_SIZE>				mEdgeCache;
	Ps::aos::Vec3V									mHullCenterMesh;

	Gu::Container&									mDeferredContacts;
	const Gu::PolygonalData&						mPolyData;
	SupportLocal*									mPolyMap;
	const Cm::FastVertex2ShapeScaling&				mConvexScaling;  
	bool											mIdtConvexScale;
	
				
	PCMConvexVsMeshContactGeneration(
		const Ps::aos::FloatVArg				contactDistance,
		const Ps::aos::FloatVArg				replaceBreakingThreshold,
		const Ps::aos::PsTransformV&			convexTransform, 
		const Ps::aos::PsTransformV&			meshTransform,
		Gu::MultiplePersistentContactManifold&	multiManifold,
		Gu::ContactBuffer&						contactBuffer,

		const Gu::PolygonalData&				polyData,
		SupportLocal*							polyMap,
		Gu::Container&							delayedContacts,
		const Cm::FastVertex2ShapeScaling&		convexScaling,
		bool									idtConvexScale
		
	) : PCMMeshContactGeneration(contactDistance, replaceBreakingThreshold, convexTransform, meshTransform, multiManifold, contactBuffer),
		mDeferredContacts(delayedContacts),
		mPolyData(polyData),
		mPolyMap(polyMap),
		mConvexScaling(convexScaling),
		mIdtConvexScale(idtConvexScale)
	{
		using namespace Ps::aos;
	
		// Hull center in local space
		const Vec3V hullCenterLocal = V3LoadU(mPolyData.mCenter);
		// Hull center in mesh space
		mHullCenterMesh = mMeshToConvex.transformInv(hullCenterLocal);

	}

	bool processTriangle(const PxVec3* verts, PxU32 triangleIndex, PxU8 triFlags, const PxU32* vertInds);  
	bool generateTriangleFullContactManifold(Gu::TriangleV& localTriangle, const PxU32 triangleIndex, const PxU32* triIndices, const PxU8 triFlags, const Gu::PolygonalData& polyData,  Gu::SupportLocalImpl<Gu::TriangleV>* localTriMap, Gu::SupportLocal* polyMap, Gu::PersistentContact* manifoldContacts, PxU32& numContacts,
		const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& patchNormal);

	bool generatePolyDataContactManifold(Gu::TriangleV& localTriangle, const PxU32 featureIndex, const PxU8 triFlags, Gu::PersistentContact* manifoldContacts, PxU32& numContacts, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& patchNormal);
	void generateLastContacts();
	void addContactsToPatch(const Ps::aos::Vec3VArg patchNormal, const PxU32 previousNumContacts, const PxU32 _numContacts);
};



class PCMSphereVsMeshContactGeneration : public PCMMeshContactGeneration
{
public:
	Ps::aos::Vec3V	mSphereCenter;
	Ps::aos::FloatV mSphereRadius;
	Ps::aos::FloatV	mSqInflatedSphereRadius;

				
	PCMSphereVsMeshContactGeneration(
		const Ps::aos::Vec3VArg		sphereCenter,
		const Ps::aos::FloatVArg	sphereRadius,
		const Ps::aos::FloatVArg	contactDist,
		const Ps::aos::FloatVArg	replaceBreakingThreshold,
		const Ps::aos::PsTransformV& sphereTransform,
		const Ps::aos::PsTransformV& meshTransform,
		Gu::MultiplePersistentContactManifold& multiManifold,
		Gu::ContactBuffer& contactBuffer

	) : PCMMeshContactGeneration(contactDist, replaceBreakingThreshold, sphereTransform, meshTransform, multiManifold, contactBuffer),
		mSphereCenter(sphereCenter),
		mSphereRadius(sphereRadius)
	{
		using namespace Ps::aos;
		const FloatV inflatedSphereRadius = FAdd(sphereRadius, contactDist);
		mSqInflatedSphereRadius = FMul(inflatedSphereRadius, inflatedSphereRadius);
	}


	bool processTriangle(const PxVec3* verts, PxU32 triangleIndex, PxU8 triFlags, const PxU32* vertInds);
};

class PCMCapsuleVsMeshContactGeneration : public PCMMeshContactGeneration
{
public:
	Ps::aos::FloatV mInflatedRadius;
	Ps::aos::FloatV	mSqInflatedRadius;
	const CapsuleV&	mCapsule;

				
	PCMCapsuleVsMeshContactGeneration( 
		const CapsuleV&				capsule,
		const Ps::aos::FloatVArg	contactDist,
		const Ps::aos::FloatVArg	replaceBreakingThreshold,
		const Ps::aos::PsTransformV& sphereTransform,
		const Ps::aos::PsTransformV& meshTransform,
		Gu::MultiplePersistentContactManifold& multiManifold,
		Gu::ContactBuffer& contactBuffer

	) : PCMMeshContactGeneration(contactDist, replaceBreakingThreshold, sphereTransform, meshTransform, multiManifold, contactBuffer),
		mCapsule(capsule)
	{
		using namespace Ps::aos;
		mInflatedRadius = FAdd(capsule.radius, contactDist);
		mSqInflatedRadius = FMul(mInflatedRadius, mInflatedRadius);
	}

	void generateEEContacts(const Ps::aos::Vec3VArg a, const Ps::aos::Vec3VArg b,const Ps::aos::Vec3VArg c, const Ps::aos::Vec3VArg normal);
	void generateEE(const Ps::aos::Vec3VArg p, const Ps::aos::Vec3VArg q, const Ps::aos::Vec3VArg normal, const Ps::aos::Vec3VArg a, const Ps::aos::Vec3VArg b);
	void generateContacts(const Ps::aos::Vec3VArg a, const Ps::aos::Vec3VArg b,const Ps::aos::Vec3VArg c, const Ps::aos::Vec3VArg normal);
	bool processTriangle(const PxVec3* verts, PxU32 triangleIndex, PxU8 triFlags, const PxU32* vertInds);
};

}
}

#endif