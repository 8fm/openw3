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


#ifndef PXS_CONTACTCACHE_H
#define PXS_CONTACTCACHE_H

#include "PxvConfig.h"
#include "CmBitMap.h"
#include "PxTransform.h"
#include "CmMatrix34.h"
#include "PxcThreadCoherantCache.h"
#include "PxsThresholdTable.h"
#include "PxcSolverBody.h"
#include "PsAllocator.h"
#include "PsAlignedMalloc.h"
#include "PxcNpThreadContext.h"
#include "PxcSolverConstraintDesc.h"
#include "PxvDynamics.h"
#include "PxcArticulation.h"

namespace physx
{
class PxsRigidBody;
struct PxcSolverBody;
class PxsSolverBodyPool;
class PxsContactManager;
struct PxsIndexedContactManager;

/*!
Cache information specific to the software implementation(non common).

See PxcgetThreadContext.

Not thread-safe, so remember to have one object per thread!

TODO! refactor this and rename(it is a general per thread cache). Move transform cache into its own class.
*/
class PxsThreadContext : 
	public PxcThreadCoherantCache<PxsThreadContext>::EntryBase, public PxcNpThreadContext
{
public:

	// PX_ENABLE_SIM_STATS
	struct ThreadSimStats
	{
		void clear()
		{
#if PX_ENABLE_SIM_STATS
			numActiveConstraints = 0;
			numActiveDynamicBodies = 0;
			numActiveKinematicBodies = 0;
			numAxisSolverConstraints = 0;
#endif
		}

		PxU32 numActiveConstraints;
		PxU32 numActiveDynamicBodies;
		PxU32 numActiveKinematicBodies;
		PxU32 numAxisSolverConstraints;

	};

	//TODO: tune cache size based on number of active objects.
	PxsThreadContext(PxsContext *context);
	void reset(PxU32 shapeCount, PxU32 cmCount);
	void resizeArrays(PxU32 bodyCount, PxU32 cmCount, PxU32 contactConstraintDescCount, PxU32 frictionConstraintDescCount, PxU32 articulationCount);


	PX_FORCE_INLINE void										addLocalNewTouchCount(PxU32 newTouchCMCount)	{ mLocalNewTouchCount += newTouchCMCount;	}
	PX_FORCE_INLINE void										addLocalLostTouchCount(PxU32 lostTouchCMCount)	{ mLocalLostTouchCount += lostTouchCMCount;	}
	PX_FORCE_INLINE PxU32										getLocalNewTouchCount()					const	{ return mLocalNewTouchCount;				}
	PX_FORCE_INLINE PxU32										getLocalLostTouchCount()				const	{ return mLocalLostTouchCount;				}

	PX_FORCE_INLINE Cm::BitMap&									getLocalChangeTouch()							{ return mLocalChangeTouch;					}
	PX_FORCE_INLINE Cm::BitMap&									getLocalChangedShapes()							{ return mLocalChangedShapes;				}

	PX_FORCE_INLINE	Ps::Array<PxcArticulationSolverDesc>&		getArticulations()								{ return mArticulations;					}
	PX_FORCE_INLINE PxsThresholdStream&							getThresholdStream()							{ return mThresholdStream;					}
	PX_FORCE_INLINE PxsThresholdStream&							getAccumulatedThresholdStream()					{ return mAccumulatedThresholdStream;					}
	PX_FORCE_INLINE void										setCreateContactStream(bool to)					{ mCreateContactStream = to;				}

					void										saveThresholdStream()							
					{
						mThresholdStream.resize(mThresholdPairCount);
						mAccumulatedThresholdStream.append(mThresholdStream);
						mThresholdStream.clear();
					}


	// PX_ENABLE_SIM_STATS
	PX_FORCE_INLINE ThreadSimStats& getSimStats()
	{
		return mThreadSimStats;
	}


	// this stuff is just used for reformatting the solver data. Hopefully we should have a more
	// sane format for this when the dust settles - so it's just temporary. If we keep this around
	// here we should move these from public to private

	PX_ALIGN(16, PxU32					mConstraintsPerPartition[33]);
	PxU32 mSuccessfulSpuConstraintPartitionCount;
	PxU32 mNumDifferentBodyConstraints;
	PxU32 mNumDifferentBodyFrictionConstraints;
	PX_ALIGN(16, PxU32					mFrictionConstraintsPerPartition[33]);
	PxU32 mNumSelfConstraints;
	PxU32 mNumSelfFrictionConstraints;
	PxU32 mNumSelfConstraintBlocks;
	PxU32 mNumSelfConstraintFrictionBlocks;

	Ps::Array<PxsBodyCore*>				bodyCoreArray;
	Ps::Array<Cm::SpatialVector>		accelerationArray;
	Ps::Array<Cm::SpatialVector>		motionVelocityArray;
	Ps::Array<PxcSolverConstraintDesc>	contactConstraintDescArray;
	Ps::Array<PxcSolverConstraintDesc>	frictionConstraintDescArray;
	Ps::Array<PxcSolverConstraintDesc>	orderedContactConstraints;
	Ps::Array<PxsConstraintBatchHeader> contactConstraintBatchHeaders;
	Ps::Array<PxsConstraintBatchHeader> frictionConstraintBatchHeaders;
	Ps::Array<PxsCompoundContactManager> compoundConstraints;
	Ps::Array<const PxsIndexedContactManager*> orderedContactList;
	Ps::Array<const PxsIndexedContactManager*> tempContactList;
	Ps::Array<PxU32>					sortIndexArray;

	PxU32 numDifferentBodyBatchHeaders;
	PxU32 numSelfConstraintBatchHeaders;

	
	PxU32								mOrderedContactDescCount;
	PxU32								mOrderedFrictionDescCount;

	PxU32 mAxisConstraintCount;
	bool mSuccessfulSpuConstraintPartition;
	PxcFsSelfConstraintBlock* mSelfConstraintBlocks;
	
	PxcFsSelfConstraintBlock* mSelfConstraintFrictionBlocks;
	
	
	

	PxU32 mMaxPartitions;
	PxU32 mMaxFrictionPartitions;
	PxU32 mMaxSolverPositionIterations;
	PxU32 mMaxSolverVelocityIterations;
	PxU32 mThresholdPairCount;
	PxU32 mMaxArticulationLength;
	
	PxcSolverConstraintDesc* mContactDescPtr;
	PxcSolverConstraintDesc* mStartContactDescPtr;
	PxcSolverConstraintDesc* mFrictionDescPtr;

	PxI32 mSolverProgressCounters[6];
	

private:

	// Solver caches

	// first two of these are transient, we should just temp allocate them
	PxsThresholdStream						mThresholdStream;
	PxsThresholdStream						mAccumulatedThresholdStream;

	Ps::Array<PxcArticulationSolverDesc>	mArticulations;

	// change touch handling.
	Cm::BitMap mLocalChangeTouch;
	PxU32 mLocalNewTouchCount;
	PxU32 mLocalLostTouchCount;

	//Shapes changed handling(keeps track of which shapes and therefore their volumes changed within a particular thread).
	Cm::BitMap mLocalChangedShapes;

	// PX_ENABLE_SIM_STATS
	ThreadSimStats				mThreadSimStats;


};

}

#endif
