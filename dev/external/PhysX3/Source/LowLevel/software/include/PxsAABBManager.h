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


#ifndef PXS_COMPOUND_MANAGER_H
#define PXS_COMPOUND_MANAGER_H

#include "PxsBroadPhaseCommon.h"
#include "PxsAABBManagerAux.h"
#include "PxsAABBManagerId.h"

#ifndef __SPU__
#include "PxTask.h"
#endif

namespace physx
{
namespace Cm
{	
	class RenderOutput;
	class EventProfiler;
}

class PxBaseTask;
class PxLightCpuTask;

class BPTasks;
class PxvBroadPhase;
struct PxsBodyShapeBPHandle;
class PxsRigidBody;
class PxsContext;

class PxsAABBManager : public Ps::UserAllocated
{
	public:

		friend class AABBUpdateWorkTask;
		friend class AABBUpdateTask;
		friend class AABBUpdateWorkEndTask;
		friend class AABBUpdateTaskSPU;
		friend class BPUpdateFinalizeTask;

		PxsAABBManager(Cm::EventProfiler& eventProfiler,PxcScratchAllocator& allocator);
		~PxsAABBManager();

		//init/destroy/etc
		void init(PxvBroadPhase* bp);
		void preAllocate(const PxU32 nbStaticShapes, const PxU32 nbDynamicShapes);
		void destroyV();

		void visualize(Cm::RenderOutput& out);

		//create/delete compound
		PxU32 createCompound(void* userData, const bool selfCollisions);
		void deleteCompound(const PxU32 id);

		//create/init/release volume
		//compoundId stored in actorCore - PX_INVALID_BP_HANDLE for non-compound volume, value generated by createCompound. 
		//singleOrCompoundId stored in PxcRigidBody - PX_INVALID_BP_HANDLE for first shape of a compound or non-compound rigid body, 
		AABBMgrId createVolume(const PxU32 compoundId, const PxcBpHandle singleOrCompoundId, const PxU32 group, void* userdata, const PxBounds3& bounds);
		void setDynamicAABBData(const PxcBpHandle aabbMgrHandle, const PxcAABBDataDynamic& aabbData);
		void setStaticAABBData(const PxcBpHandle aabbMgrHandle, const PxcAABBDataStatic& aabbData);
		
		//return true if released last shape
		bool releaseVolume(const PxcBpHandle aabbMgrHandle);
		void setVolumeBounds(const PxcBpHandle aabbMgrHandle, const PxBounds3& bounds);

		//query compound
		PX_FORCE_INLINE bool getCompoundSelfCollisions(const PxU32 id)
		{
			return mCompoundManager.getCompound(id)->selfCollide;
		}

		//query volume
		PxBounds3 getBPBounds(const PxcBpHandle volume) const;

		//Return true if an id represents a compound, false if it represents a single.
		Ps::IntBool isCompoundId(const PxcBpHandle singleOrCompoundId) const;

		//Compute array of all rigid bodies associated with a compound.
		PxU32 getCompoundRigidBodies(const PxcBpHandle compoundId, PxcRigidBody** rigidBodies, const PxU32 rigidBodiesArrayCapacity) const;

		//Update aabbs and sap.
		void updateAABBsAndBP(
			PxsContext* context,
			const PxU32 numCpuTasks, const PxU32 numSpusAABB, const PxU32 numSpusSap, PxBaseTask* continuation,
			PxU32* PX_RESTRICT changedShapeWords, const PxU32 changedShapeWordCount, 						
			const PxF32 dt, const bool secondBroadPhase, PxI32* numFastMovingShapes);

		//Query the results of the broadphase update reported back to the high-level for filtering.
		PX_FORCE_INLINE	PxU32						getCreatedOverlapsCount()	const	{ return mCreatedPairsSize;		}
		PX_FORCE_INLINE	const PxvBroadPhaseOverlap* getCreatedOverlaps()		const	{ return mCreatedPairs;			}
						void						freeCreatedOverlaps();

		PX_FORCE_INLINE	PxU32						getDestroyedOverlapsCount()	const	{ return mDeletedPairsSize;		}
		PX_FORCE_INLINE	const PxvBroadPhaseOverlap*	getDestroyedOverlaps()		const	{ return mDeletedPairs;			}
						void						freeDestroyedOverlaps();

		PX_FORCE_INLINE	PxvBroadPhase*				getBroadPhase()				const	{ return mBP;					}
		PX_FORCE_INLINE	Ps::Array<void*>&			getOutOfBoundsObjects()				{ return mOutOfBoundsObjects;	}
		PX_FORCE_INLINE	Ps::Array<void*>&			getOutOfBoundsAggregates()			{ return mOutOfBoundsAggregates;}

		// shift origin of AABBs
		void shiftOrigin(const PxVec3& shift);

	private:

		//Called form createVolume/releaseVolume
		PxU32 createBPElem();
		PxU32 createCompoundElem();


		////////////////////////////////////////////////////
		//Overlap bp results (single - compound, compound - compound) and self-collision
		////////////////////////////////////////////////////

		CompoundPair& addCompoundPair(const PxcBpHandle bpElemId0, const PxcBpHandle bpElemId1);
		bool removeCompoundPair(const PxcBpHandle bpElemId0, const PxcBpHandle bpElemId1);
		void purgeCompoundPairs(const PxcBpHandle bpElemId);

		PX_FORCE_INLINE	void addCreatedPair(void* userdata0, void* userdata1);
		PX_FORCE_INLINE	void addDeletedPair(void* userdata0, void* userdata1);

		void selfCollideCompound(Compound& compound);
		void selfCollideCompoundBipartite(Compound& compound,CompoundCache::CompoundData* PX_RESTRICT cData);
		void collideCompoundCompound(Compound* PX_RESTRICT c0, Compound* PX_RESTRICT c1, Cm::BitMap* PX_RESTRICT compoundCollBitmap, bool flag);
		void collideCompoundCompoundBipartite(Compound* PX_RESTRICT c0, CompoundCache::CompoundData* PX_RESTRICT c0Data, Compound* PX_RESTRICT c1, CompoundCache::CompoundData* PX_RESTRICT c1Data, Cm::BitMap* PX_RESTRICT compoundCollBitmap);
		void collideCompoundCompoundRemovePair(Compound* PX_RESTRICT c0, Compound* PX_RESTRICT c1, Cm::BitMap* PX_RESTRICT compoundCollBitmap);
		void collideSingleCompound(const PxcBpHandle s0, Compound* PX_RESTRICT c1, Cm::BitMap* PX_RESTRICT compoundCollBitmap, bool flag);
		void collideSingleCompoundBipartite(const PxcBpHandle s0, Compound* PX_RESTRICT c1,CompoundCache::CompoundData* PX_RESTRICT c1Data, Cm::BitMap* PX_RESTRICT compoundCollBitmap);
		void collideSingleCompoundRemovePair(const PxcBpHandle s0, Compound* PX_RESTRICT c1, Cm::BitMap* PX_RESTRICT compoundCollBitmap);
		bool bipartiteBoxPruning(const CompoundCache::ElementData* PX_RESTRICT data0,const PxU32* PX_RESTRICT ranks0, PxU32 nb0, const CompoundCache::ElementData* PX_RESTRICT data1,const PxU32* PX_RESTRICT ranks1, PxU32 nb1, CompoundCache::CompoundPairsArray& pairs);		
		bool completeBoxPruning(const CompoundCache::ElementData* PX_RESTRICT data0,const PxU32* PX_RESTRICT ranks0, PxU32 nb0,CompoundCache::CompoundPairsArray& pairs);
		void fillCompoundData(Compound* PX_RESTRICT c0, CompoundCache::CompoundData* PX_RESTRICT c0Data);
		void processCompoundPairs();
		void processCompoundPairs(const PxcBroadPhasePair* PX_RESTRICT bpPairs, const PxU32 numBPPairs);

		PX_FORCE_INLINE	void overlapTest(const IntegerAABB& b0, const IntegerAABB& b1, void* PX_RESTRICT userdata0, void* PX_RESTRICT userdata1, Cm::BitMap* PX_RESTRICT bitmap, PxU32 bitIndex, bool flag);

		void promoteBitmaps(Compound* compound);
		void promoteBitmap(PxU32* bitMapWords, PxU32 nbX, PxU32 nbY, PxU32 newXIndex, PxU32 newYIndex) const;
		Cm::BitMap*	promoteBitmap(Cm::BitMap* bitmap, PxU32 nbX, PxU32 nbY, PxU32 newXIndex, PxU32 newYIndex) const;

		void selfCollideCompoundBounds();

		////////////////////////////////////////////////////

		CompoundManager				mCompoundManager;
		PxU32						mBitmasks[32];
		UpdatedCreatedRemovedList	mCompoundsUpdated;
		CompoundCache				mCompoundCache;

		SingleManager				mSingleManager;

		//AABBs that have bp entries: either single aabbs or aggregated compound aabbs.
		BPElems						mBPElems;
		//AABBs that have no bp entries: must be in a compound.
		CompoundElems				mCompoundElems;

		//Blocks of aabbs that need their bounds updated (decoded from bitmap of handles that need updated).
		//Need to store this on the heap rather than temporarily on the stack because we need to perform a dma from spu.
		PxcBpHandle*				mBPUpdatedElemIds;
		PxcBpHandle				PX_ALIGN(16, mBPUpdatedElemIdsBuffer[MAX_NUM_BP_SPU_SAP_AABB]);
		PxU32					mBPUpdatedElemIdsSize;
		PxcBpHandle*				mCompoundUpdatedElemIds;
		PxcBpHandle				PX_ALIGN(16, mCompoundUpdatedElemIdsBuffer[MAX_NUM_BP_SPU_SAP_AABB]);
		PxU32					mCompoundUpdatedElemIdsSize;

#ifdef PX_PS3
		PxcBpHandle*				mBPUpdatedElemWordStarts;
		PxcBpHandle*				mBPUpdatedElemWordEnds;
		PxcBpHandle				PX_ALIGN(16, mBPUpdatedElemWordStartsBuffer[MAX_NUM_BP_SPU_SAP_AABB>>5]);
		PxcBpHandle				PX_ALIGN(16, mBPUpdatedElemWordEndsBuffer[MAX_NUM_BP_SPU_SAP_AABB>>5]);
		PxU32					mBPUpdatedElemWordCount;
		PxcBpHandle*				mCompoundUpdatedElemWordStarts;
		PxcBpHandle*				mCompoundUpdatedElemWordEnds;
		PxcBpHandle				PX_ALIGN(16, mCompoundUpdatedElemWordStartsBuffer[MAX_NUM_BP_SPU_SAP_AABB>>5]);
		PxcBpHandle				PX_ALIGN(16, mCompoundUpdatedElemWordEndsBuffer[MAX_NUM_BP_SPU_SAP_AABB>>5]);
		PxU32					mCompoundUpdatedElemWordCount;
#endif

		//bp element lists that have been updated/created/removed.
		UpdatedCreatedRemovedList mBPUpdatedElems;
		UpdatedCreatedRemovedList mBPCreatedElems;
		UpdatedCreatedRemovedList mBPRemovedElems;


		// Broadphase
		PxvBroadPhase*			mBP;

		// Broadphase results.
		PxvBroadPhaseOverlap*	mCreatedPairs;
		PxU32					mCreatedPairsSize;
		PxU32					mCreatedPairsCapacity;
		PxvBroadPhaseOverlap*	mDeletedPairs;
		PxU32					mDeletedPairsSize;
		PxU32					mDeletedPairsCapacity;
		CompoundPair*			mCompoundPairs;
		PxU32					mCompoundPairsSize;
		PxU32					mCompoundPairsCapacity;

		//Used internally by setVolumeBounds 
		void setBPElemVolumeBounds(const PxcBpHandle singleId, const IntegerAABB& bounds);
		void setCompoundElemVolumeBounds(const PxcBpHandle elemId, const IntegerAABB& bounds);

		///////////////////////////////////////////////////////////

		Cm::EventProfiler& mEventProfiler;

		///////////////////////////////////////////////////////////

#ifndef __SPU__
		AABBUpdateWorkTask		mAABBUpdateWorkTask;
		AABBUpdateWorkEndTask	mAABBUpdateWorkEndTask;
		BPUpdateFinalizeTask	mBPUpdateFinalizeTask;
#endif

		Ps::Array<void*>		mOutOfBoundsObjects;
		Ps::Array<void*>		mOutOfBoundsAggregates;

		//Compute the lists of bp and compound elems that need to be updated.
		void computeAABBUpdateLists(PxU32* PX_RESTRICT changedShapeWords, const PxU32 changedShapeWordCount, const PxsComputeAABBParams& params, const bool secondBroadPhase); 						
		//Update the aabbs from the lists of bp and compound elems that need updated.
		//Recompute the compound bounds after updating the elements of the compound bounds.
		void mergeCompoundBounds();
		//Pass the updated bounds data to the bp.
		//Called from SapUpdateStartTask.
		void prepareBP();
		//After the sap we need to consider single-compound and compound-compound overlaps reported by the sap.
		//Called from AABBUpdateFinalizeTask.
		void finalizeUpdate(const PxU32 numSpusSap);
};

} //namespace physx

#endif //PXS_COMPOUND_MANAGER_H
