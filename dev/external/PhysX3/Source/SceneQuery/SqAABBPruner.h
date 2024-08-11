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

#ifndef SQ_DYNAMICPRUNER2_H
#define SQ_DYNAMICPRUNER2_H

#include "SqPruningPool.h"
#include "OPC_TreeBuilders.h"
#include "SqBucketPruner.h"
#include "SqAABBTreeUpdateMap.h"

#include "PsHashSet.h"

namespace physx
{
namespace Gu { class AABBTree; }

namespace Sq
{
	enum BuildStatus
	{
		BUILD_NOT_STARTED,
		BUILD_INIT,
		BUILD_IN_PROGRESS,
		BUILD_FINISHED,

		BUILD_FORCE_DWORD	= 0xffffffff
	};

	class AABBPruner
#if !PX_IS_SPU
		: public Pruner
#endif
	{
		public:
												AABBPruner(bool incrementalUpdate);
#if __SPU__
												PX_FORCE_INLINE ~AABBPruner() {}
#else
		virtual									~AABBPruner();
#endif

		// PrunerInterface
						bool					addObjects(PrunerHandle* results, const PxBounds3* bounds, const PrunerPayload* userData, PxU32 count = 1);
						void					removeObjects(const PrunerHandle* handles, PxU32 count = 1);
						void					updateObjects(const PrunerHandle* handles, const PxBounds3* newBounds, PxU32 count = 1);

						PxAgain					raycast(const PxVec3& origin, const PxVec3& unitDir, PxReal& inOutDistance, PrunerCallback&)			const;
						PxAgain					sweep(const ShapeData& queryVolume, const PxVec3& unitDir, PxReal& inOutDistance, PrunerCallback&)		const;
						PxAgain					overlap(const ShapeData& queryVolume, PrunerCallback&) const;

						const PrunerPayload&	getPayload(const PrunerHandle& h) const { return mPool.getPayload(h); }
						void					preallocate(PxU32 entries) { mPool.preallocate(entries); };

						void					commit();
						void					shiftOrigin(const PxVec3& shift);
						void					visualize(Cm::RenderOutput& out, PxU32 color) const;

		// non-pruner interface
						void					setRebuildRateHint(PxU32 nbStepsForRebuild);	// Besides the actual rebuild steps, 3 additional steps are needed.
						bool					buildStep();	// returns true if finished
						void					purge();		// gets rid of internal accel struct

		// direct access for SPU and test code

		PX_FORCE_INLINE	PxU32					getNbAddedObjects()	const				{ return mAdded0.size() + mAdded1.size();	}
		PX_FORCE_INLINE	const Gu::AABBTree*		getAABBTree()	const					{ PX_ASSERT(!mDirty); return mAABBTree;	}
		PX_FORCE_INLINE	Gu::AABBTree*			getAABBTree()							{ PX_ASSERT(!mDirty); return mAABBTree;	}
		PX_FORCE_INLINE	void					setAABBTree(Gu::AABBTree* tree)			{ mAABBTree = tree; }
		PX_FORCE_INLINE	const Gu::AABBTree*		hasAABBTree()	const					{ return mAABBTree;	}
				
		// local functions
		private:
						Gu::AABBTree*			mAABBTree;
						Gu::AABBTreeBuilder		mBuilder;
						Gu::AABBTree*			mNewTree;
						PxBounds3*				mCachedBoxes;
						PxU32					mNbCachedBoxes;
						BuildStatus				mProgress;
						PxU32					mNbCalls;
						typedef Ps::HashSet<PrunerHandle> PrunerHandleSet;
						PrunerHandleSet*		mAddedObjects0;
						PrunerHandleSet*		mAddedObjects1;
						PrunerHandleSet			mAdded0;
						PrunerHandleSet			mAdded1;

						BucketPrunerCore		mBucketPruner;

						PxU32					mRebuildRateHint;		// Fraction of the total number of primitives that should be updated per step.
						PxU32					mTotalWorkUnits;		// Estimate for how much work has to be done to rebuild the tree.
						PxI32					mAdaptiveRebuildTerm;	// Term to correct the work unit estimate if the rebuild rate is not matched.

						PruningPool				mPool;
						AABBTreeUpdateMap		mTreeMap;

						bool					mIncrementalUpdate;
						bool					mDirty;					// A rebuild can be triggered even when the Pruner is not dirty
						bool					mNeedsNewTree;			// A new AABB tree is built if an object was added, removed or updated
																		// Changing objects during a build will trigger another rebuild right afterwards
						struct Record
						{
							Record(PxU32 a,PxU32 b):indexA(a),indexB(b){}
							PxU32 indexA;
							PxU32 indexB;
						};
						Ps::Array<Record>		mRecords;
						bool					mIsBuilding;

		// Internal methods
						bool					rebuildAABBTree();
						void					release();
						void					refit();
						void					updateBucketPruner();
	};

} // namespace Sq

}

#endif // SQ_DYNAMICPRUNER2_H
