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



#ifndef PXC_NP_MEM_BLOCK_POOL_H
#define PXC_NP_MEM_BLOCK_POOL_H

#include "PxvConfig.h"
#include "PsMutex.h"
#include "PxAssert.h"
#include "PsArray.h"
#include "PxcScratchAllocator.h"

namespace physx
{

struct PxcNpMemBlock
{
	enum
	{
		SIZE = 16384
	};
	PxU8 data[SIZE];
};

typedef Ps::Array<PxcNpMemBlock*> PxcNpMemBlockArray;

class PxcNpMemBlockPool
{
	PX_NOCOPY(PxcNpMemBlockPool)
public:
	PxcNpMemBlockPool(PxcScratchAllocator& allocator);
	~PxcNpMemBlockPool();

	void			init(PxU32 initial16KDataBlocks, PxU32 maxBlocks);
	void			flush();
	void			setBlockCount(PxU32 count);
	PxU32			getUsedBlockCount() const;
	PxU32			getMaxUsedBlockCount() const;
	PxU32			getPeakConstraintBlockCount() const;
	void			releaseUnusedBlocks();

	PxcNpMemBlock*	acquireConstraintBlock();
	PxcNpMemBlock*	acquireConstraintBlock(PxcNpMemBlockArray& memBlocks);
	PxcNpMemBlock*	acquireContactBlock();
	PxcNpMemBlock*	acquireFrictionBlock();
	PxcNpMemBlock*	acquireNpCacheBlock();

	PxU8*			acquireExceptionalConstraintMemory(PxU32 size);

#ifdef PX_PS3
	const PxcNpMemBlock* const* acquireMultipleConstraintBlocks(const PxU32 numRequestedBlocks, PxU32& acquiredBlockStart, PxU32& numAcquiredBlocks);
	const PxcNpMemBlock* const* acquireMultipleFrictionBlocks(const PxU32 numRequestedBlocks, PxU32& acquiredBlockStart, PxU32& numAcquiredBlocks);
	const PxcNpMemBlock* const* acquireMultipleCacheBlocks(const PxU32 numRequestedBlocks, PxU32& acquiredBlockStart, PxU32& numAcquiredBlocks);
	const PxcNpMemBlock* const* acquireMultipleContactBlocks(const PxU32 numRequestedBlocks, PxU32& acquiredBlockStart, PxU32& numAcquiredBlocks);
	void releaseMultipleConstraintBlocks(const PxU32 rangeStart, const PxU32 rangeEnd);
	void releaseMultipleFrictionBlocks(const PxU32 rangeStart, const PxU32 rangeEnd);
	void releaseMultipleCacheBlocks(const PxU32 rangeStart, const PxU32 rangeEnd);
	void releaseMultipleContactBlocks(const PxU32 rangeStart, const PxU32 rangeEnd);
	void getSpuMemBlockCounters(PxU32& numContactBlocks, PxU32& numFrictionBlocks, PxU32& numConstraintBlocks,
		PxU32& numNpCacheBlocks);
	/*
	 * KS - we now have a single buffer for constraints and a single buffer for contact stream. The memory is allocated "mixed"
	 * so there is no way of differentiating between memory allocated for contacts and joints (shaders). However, there are simulation
	 * statistics that tell us how much memory was required for contact constraints in total (in bytes) so this allows us to get an approximate
	 * overhead for the joints in the scene as the total constraint memory = (contacts constraints + shader constraints).
	 *
	 * We also now have separate buffers for NpCache and friction blocks. As such, we need another function to keep track of these buffers
	*/
	void updateSpuContactBlockCount();
	void updateSpuFrictionBlockCount();
	void updateSpuConstraintBlockCount();
	void updateSpuNpCacheBlockCount();
#endif

	void			acquireConstraintMemory();
	void			releaseConstraintMemory();
	void			releaseConstraintBlocks(PxcNpMemBlockArray& memBlocks);
	void			releaseContacts();
	void			swapFrictionStreams();
	void			swapNpCacheStreams();

	void			flushUnused();
	
private:


	Ps::Mutex				mLock;
	PxcNpMemBlockArray		mConstraints;
	PxcNpMemBlockArray		mContacts;
	PxcNpMemBlockArray		mFriction[2];
	PxcNpMemBlockArray		mNpCache[2];
	PxcNpMemBlockArray		mScratchBlocks;
	Ps::Array<PxU8*>		mExceptionalConstraints;

	PxcNpMemBlockArray		mUnused;

	PxU32					mNpCacheActiveStream;
	PxU32					mFrictionActiveStream;
	PxU32					mCCDCacheActiveStream;
	PxU32					mAllocatedBlocks;
	PxU32					mMaxBlocks;
	PxU32					mUsedBlocks;
	PxU32					mMaxUsedBlocks;
	PxcNpMemBlock*			mScratchBlockAddr;
	PxU32					mNbScratchBlocks;
	PxcScratchAllocator&	mScratchAllocator;

	PxU32					mPeakConstraintAllocations;
	PxU32					mConstraintAllocations;

	PxcNpMemBlock*	acquire(PxcNpMemBlockArray& trackingArray, PxU32* allocationCount = NULL, PxU32* peakAllocationCount = NULL, bool isScratchAllocation = false);
	void			release(PxcNpMemBlockArray& deadArray, PxU32* allocationCount = NULL);

#ifdef PX_PS3
	PxU32			mMaxSpuContactBlocks;
	PxU32			mCurrentSpuContactBlocks;
	PxU32			mMaxSpuFrictionBlocks;
	PxU32			mMaxSpuConstraintBlocks;
	PxU32			mMaxSpuNpCacheBlocks;
	void			acquireMultipleBlocks(const PxU32 numRequestedBlocks, PxcNpMemBlockArray& mConstraints, PxU32& numAcquiredBlocks);
	void			releaseMultipleBlocks(PxcNpMemBlockArray& deadArray, const PxU32 rangeStart, const PxU32 rangeEnd);
#endif
};

}

#endif
