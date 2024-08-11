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

#include "PxcNpMemBlockPool.h"
#include "PsUserAllocated.h"

using namespace physx;

PxcNpMemBlockPool::PxcNpMemBlockPool(PxcScratchAllocator& allocator):
  mNpCacheActiveStream(0),
  mFrictionActiveStream(0),
  mCCDCacheActiveStream(0),
  mAllocatedBlocks(0),
  mMaxBlocks(0),
  mUsedBlocks(0),
  mMaxUsedBlocks(0),
  mScratchBlockAddr(0),
  mNbScratchBlocks(0),
  mScratchAllocator(allocator),
  mPeakConstraintAllocations(0),
  mConstraintAllocations(0)
#ifdef PX_PS3
  ,mMaxSpuContactBlocks(0)
  ,mMaxSpuFrictionBlocks(0)
  ,mMaxSpuConstraintBlocks(0)
  ,mMaxSpuNpCacheBlocks(0)
#endif
{
}

void PxcNpMemBlockPool::init(PxU32 initialBlockCount, PxU32 maxBlocks)
{
	mMaxBlocks = maxBlocks;

	PxU32 reserve = PxMax<PxU32>(initialBlockCount, 64);

	mConstraints.reserve(reserve);
	mExceptionalConstraints.reserve(16);

	mFriction[0].reserve(reserve);
	mFriction[1].reserve(reserve);
	mNpCache[0].reserve(reserve);
	mNpCache[1].reserve(reserve);
	mUnused.reserve(reserve);

	setBlockCount(initialBlockCount);
}

PxU32 PxcNpMemBlockPool::getUsedBlockCount() const
{
	return mUsedBlocks;
}

PxU32 PxcNpMemBlockPool::getMaxUsedBlockCount() const
{
	return mMaxUsedBlocks;
}

PxU32 PxcNpMemBlockPool::getPeakConstraintBlockCount() const
{
	return mPeakConstraintAllocations;
}


void PxcNpMemBlockPool::setBlockCount(PxU32 blockCount)
{
	Ps::Mutex::ScopedLock lock(mLock);
	PxU32 current = getUsedBlockCount();
	for(PxU32 i=current;i<blockCount;i++)
	{
		mUnused.pushBack(reinterpret_cast<PxcNpMemBlock *>(PX_ALLOC(PxcNpMemBlock::SIZE, PX_DEBUG_EXP("PxcNpMemBlock"))));
		mAllocatedBlocks++;
	}
}

void PxcNpMemBlockPool::releaseUnusedBlocks()
{
	Ps::Mutex::ScopedLock lock(mLock);
	while(mUnused.size())
	{
		PX_FREE(mUnused.popBack());
		mAllocatedBlocks--;
	}
}


PxcNpMemBlockPool::~PxcNpMemBlockPool()
{
	// swapping twice guarantees all blocks are released from the stream pairs
	swapFrictionStreams();
	swapFrictionStreams();

	swapNpCacheStreams();
	swapNpCacheStreams();

	releaseConstraintMemory();
	releaseContacts();

	PX_ASSERT(mUsedBlocks == 0);

	flushUnused();
}

void PxcNpMemBlockPool::acquireConstraintMemory()
{
	PxU32 size;
	void* addr = mScratchAllocator.allocAll(size);
	size = size&~(PxcNpMemBlock::SIZE-1);

	PX_ASSERT(mScratchBlocks.size()==0);
	mScratchBlockAddr = reinterpret_cast<PxcNpMemBlock*>(addr);
	mNbScratchBlocks =  size/PxcNpMemBlock::SIZE;

	mScratchBlocks.resize(mNbScratchBlocks);
	for(PxU32 i=0;i<mNbScratchBlocks;i++)
		mScratchBlocks[i] = mScratchBlockAddr+i;
}

void PxcNpMemBlockPool::releaseConstraintMemory()
{
	Ps::Mutex::ScopedLock lock(mLock);

	mPeakConstraintAllocations = mConstraintAllocations = 0;
	
	while(mConstraints.size())
	{
		PxcNpMemBlock* block = mConstraints.popBack();
		if(mScratchAllocator.isScratchAddr(block))
			mScratchBlocks.pushBack(block);
		else
		{
			mUnused.pushBack(block);
			mUsedBlocks--;
		}
	}

	for(PxU32 i=0;i<mExceptionalConstraints.size();i++)
		PX_FREE(mExceptionalConstraints[i]);
	mExceptionalConstraints.clear();

	PX_ASSERT(mScratchBlocks.size()==mNbScratchBlocks); // check we released them all
	mScratchBlocks.clear();

	if(mScratchBlockAddr)
	{
		mScratchAllocator.free(mScratchBlockAddr);
		mScratchBlockAddr = 0;
		mNbScratchBlocks = 0;
	}
}


PxcNpMemBlock* PxcNpMemBlockPool::acquire(PxcNpMemBlockArray& trackingArray, PxU32* allocationCount, PxU32* peakAllocationCount, bool isScratchAllocation)
{
	Ps::Mutex::ScopedLock lock(mLock);
	if(allocationCount && peakAllocationCount)
	{
		*peakAllocationCount = PxMax(*allocationCount + 1, *peakAllocationCount);
		(*allocationCount)++;
	}

	// this is a bit of hack - the logic would be better placed in acquireConstraintBlock, but then we'd have to grab the mutex
	// once there to check the scratch block array and once here if we fail - or, we'd need a larger refactor to separate out
	// locking and acquisition.

	if(isScratchAllocation && mScratchBlocks.size()>0)
	{
		PxcNpMemBlock* block = mScratchBlocks.popBack();
		trackingArray.pushBack(block);
		return block;
	}

	
	if(mUnused.size())
	{
		PxcNpMemBlock* block = mUnused.popBack();
		trackingArray.pushBack(block);
		mMaxUsedBlocks = PxMax<PxU32>(mUsedBlocks+1, mMaxUsedBlocks);
		mUsedBlocks++;
		return block;
	}

		
			


	if(mAllocatedBlocks == mMaxBlocks)
		return NULL;

	// increment here so that if we hit the limit in separate threads we won't overallocated
	mAllocatedBlocks++;
	
	PxcNpMemBlock* block = reinterpret_cast<PxcNpMemBlock*>(PX_ALLOC(sizeof(PxcNpMemBlock), PX_DEBUG_EXP("PxcNpMemBlock")));

	if(block)
	{
		trackingArray.pushBack(block);
		mMaxUsedBlocks = PxMax<PxU32>(mUsedBlocks+1, mMaxUsedBlocks);
		mUsedBlocks++;
	}
	else
		mAllocatedBlocks--;

	return block;
}

PxU8* PxcNpMemBlockPool::acquireExceptionalConstraintMemory(PxU32 size)
{
	PxU8* memory = reinterpret_cast<PxU8*>(PX_ALLOC(size, PX_DEBUG_EXP("PxcNpExceptionalMemory")));
	if(memory)
	{
		Ps::Mutex::ScopedLock lock(mLock);
		mExceptionalConstraints.pushBack(memory);
	}
	return memory;
}

void PxcNpMemBlockPool::release(PxcNpMemBlockArray& deadArray, PxU32* allocationCount)
{
	Ps::Mutex::ScopedLock lock(mLock);
	mUsedBlocks -= deadArray.size();
	if(allocationCount)
	{
		*allocationCount -= deadArray.size();
	}
	while(deadArray.size())
	{
		PxcNpMemBlock* block = deadArray.popBack();
		for(PxU32 a = 0; a < mUnused.size(); ++a)
		{
			PX_ASSERT(mUnused[a] != block);
		}
		mUnused.pushBack(block);
	}
}

void PxcNpMemBlockPool::flushUnused()
{
	while(mUnused.size())
		PX_FREE(mUnused.popBack());
}


PxcNpMemBlock* PxcNpMemBlockPool::acquireConstraintBlock()
{
	// we track the scratch blocks in the constraint block array, because the code in acquireMultipleConstraintBlocks
	// assumes that acquired blocks are listed there.

	return acquire(mConstraints);
}

PxcNpMemBlock* PxcNpMemBlockPool::acquireConstraintBlock(PxcNpMemBlockArray& memBlocks)
{
	return acquire(memBlocks, &mConstraintAllocations, &mPeakConstraintAllocations, true);
}

PxcNpMemBlock* PxcNpMemBlockPool::acquireContactBlock()
{
	return acquire(mContacts, NULL, NULL, true);
}


void PxcNpMemBlockPool::releaseConstraintBlocks(PxcNpMemBlockArray& memBlocks)
{
	Ps::Mutex::ScopedLock lock(mLock);
	
	while(memBlocks.size())
	{
		PxcNpMemBlock* block = memBlocks.popBack();
		if(mScratchAllocator.isScratchAddr(block))
			mScratchBlocks.pushBack(block);
		else
		{
			mUnused.pushBack(block);
			mUsedBlocks--;
		}
	}
}

void PxcNpMemBlockPool::releaseContacts()
{
	releaseConstraintBlocks(mContacts);
}

PxcNpMemBlock* PxcNpMemBlockPool::acquireFrictionBlock()
{
	return acquire(mFriction[mFrictionActiveStream]);
}

void PxcNpMemBlockPool::swapFrictionStreams()
{
	release(mFriction[1-mFrictionActiveStream]);
	mFrictionActiveStream = 1-mFrictionActiveStream;
}

PxcNpMemBlock* PxcNpMemBlockPool::acquireNpCacheBlock()
{
	return acquire(mNpCache[mNpCacheActiveStream]);
}

void PxcNpMemBlockPool::swapNpCacheStreams()
{
	release(mNpCache[1-mNpCacheActiveStream]);
	mNpCacheActiveStream = 1-mNpCacheActiveStream;
}

#ifdef PX_PS3
const PxcNpMemBlock* const* PxcNpMemBlockPool::acquireMultipleConstraintBlocks(const PxU32 numRequestedBlocks, PxU32& acquiredBlockStart, PxU32& numAcquiredBlocks)
{
	if(0==numRequestedBlocks) return NULL;
	acquiredBlockStart = mConstraints.size();
	acquireMultipleBlocks(numRequestedBlocks, mConstraints, numAcquiredBlocks);
	return numAcquiredBlocks ? &mConstraints[acquiredBlockStart] : NULL;
}

void PxcNpMemBlockPool::releaseMultipleConstraintBlocks(const PxU32 rangeStart, const PxU32 rangeEnd)
{
	releaseMultipleBlocks(mConstraints,rangeStart,rangeEnd);
}


const PxcNpMemBlock* const* PxcNpMemBlockPool::acquireMultipleFrictionBlocks(const PxU32 numRequestedBlocks, PxU32& acquiredBlockStart, PxU32& numAcquiredBlocks)
{
	acquiredBlockStart = mFriction[mFrictionActiveStream].size();
	acquireMultipleBlocks(numRequestedBlocks, mFriction[mFrictionActiveStream], numAcquiredBlocks);
	return numAcquiredBlocks ? &mFriction[mFrictionActiveStream][acquiredBlockStart] : NULL;
}

void PxcNpMemBlockPool::releaseMultipleFrictionBlocks(const PxU32 rangeStart, const PxU32 rangeEnd)
{
	releaseMultipleBlocks(mFriction[mFrictionActiveStream],rangeStart,rangeEnd);
}


const PxcNpMemBlock* const* PxcNpMemBlockPool::acquireMultipleCacheBlocks(const PxU32 numRequestedBlocks, PxU32& acquiredBlockStart, PxU32& numAcquiredBlocks)
{
	acquiredBlockStart=mNpCache[mNpCacheActiveStream].size();
	acquireMultipleBlocks(numRequestedBlocks, mNpCache[mNpCacheActiveStream], numAcquiredBlocks);
	return numAcquiredBlocks ? &mNpCache[mNpCacheActiveStream][acquiredBlockStart] : NULL;
}

void PxcNpMemBlockPool::releaseMultipleCacheBlocks(const PxU32 rangeStart, const PxU32 rangeEnd)
{
	releaseMultipleBlocks(mNpCache[mNpCacheActiveStream],rangeStart,rangeEnd);
}

const PxcNpMemBlock* const* PxcNpMemBlockPool::acquireMultipleContactBlocks(const PxU32 numRequestedBlocks, PxU32& acquiredBlockStart, PxU32& numAcquiredBlocks)
{
	if(0==numRequestedBlocks) return NULL;
	acquiredBlockStart = mContacts.size();
	acquireMultipleBlocks(numRequestedBlocks, mContacts, numAcquiredBlocks);
	return mContacts.size() ? &mContacts[acquiredBlockStart] : NULL;
}

void PxcNpMemBlockPool::releaseMultipleContactBlocks(const PxU32 rangeStart, const PxU32 rangeEnd)
{
	releaseMultipleBlocks(mContacts,rangeStart,rangeEnd);
}

void PxcNpMemBlockPool::acquireMultipleBlocks(const PxU32 numRequestedBlocks, PxcNpMemBlockArray& trackingArray, PxU32& numAcquiredBlocks)
{
	numAcquiredBlocks = 0;
	{
		Ps::Mutex::ScopedLock lock(mLock);
		while(numAcquiredBlocks < numRequestedBlocks)
		{
			if(acquire(trackingArray))
			{
				numAcquiredBlocks++;
			}
			else
			{
				break;
			}
		}
		mMaxUsedBlocks = PxMax<PxU32>(mMaxUsedBlocks, mUsedBlocks);
	}	
}

void PxcNpMemBlockPool::releaseMultipleBlocks(PxcNpMemBlockArray& deadArray, const PxU32 rangeStart, const PxU32 rangeEnd)
{
	Ps::Mutex::ScopedLock lock(mLock);
	if(rangeEnd <= rangeStart)
		return;

	PX_ASSERT(deadArray.size() >= rangeEnd);
	PX_ASSERT(rangeEnd > rangeStart);

	for(PxU32 i = rangeStart; i< rangeEnd; i++)
	{
		PxcNpMemBlock* block = deadArray[i];
		if(block>=mScratchBlockAddr && block < mScratchBlockAddr+mNbScratchBlocks)
			mScratchBlocks.pushBack(block);
		else
		{
			mUnused.pushBack(block);
			mUsedBlocks--;
		}
	}

	deadArray.removeRange(rangeStart, rangeEnd - rangeStart);

}

void PxcNpMemBlockPool::getSpuMemBlockCounters(PxU32& numContactBlocks, PxU32& numFrictionBlocks, PxU32& numConstraintBlocks,
											   PxU32& numNpCacheBlocks)
{
	numContactBlocks = mMaxSpuContactBlocks;
	numFrictionBlocks = mMaxSpuFrictionBlocks;
	numConstraintBlocks = mMaxSpuConstraintBlocks;
	numNpCacheBlocks = mMaxSpuNpCacheBlocks;
}

void PxcNpMemBlockPool::updateSpuNpCacheBlockCount()
{
	// PPU uses npCache blocks whereas SPU uses friction blocks for the same data
	// adding up the two leads to a better guess
	PxU32 newNbNpCacheBlocks = mNpCache[mNpCacheActiveStream].size();
	mMaxSpuNpCacheBlocks = PxMax(mMaxSpuNpCacheBlocks,newNbNpCacheBlocks);
}

void PxcNpMemBlockPool::updateSpuFrictionBlockCount()
{
	// PPU uses npCache blocks whereas SPU uses friction blocks for the same data
	// adding up the two leads to a better guess
	PxU32 newNbFrictionBlocks = mFriction[mFrictionActiveStream].size();
	mMaxSpuFrictionBlocks = PxMax(mMaxSpuFrictionBlocks,newNbFrictionBlocks);
}

void PxcNpMemBlockPool::updateSpuConstraintBlockCount()
{
	PxU32 currentNbConstraintBlocks = mConstraints.size();
	mMaxSpuConstraintBlocks = PxMax(mMaxSpuConstraintBlocks, currentNbConstraintBlocks);
}

void PxcNpMemBlockPool::updateSpuContactBlockCount()
{
	mCurrentSpuContactBlocks = mContacts.size(); //This is actually contact buffer stuff!
	mMaxSpuContactBlocks = PxMax(mMaxSpuContactBlocks, mCurrentSpuContactBlocks);
}

#endif //PX_PS3



