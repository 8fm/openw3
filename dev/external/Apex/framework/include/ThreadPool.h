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

#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include "PxTask.h"
#include "PxTaskManager.h"
#include "PxCpuDispatcher.h"
#include "PsSList.h"
#include "PsSync.h"
#include "PsThread.h"
#include "PsShare.h"

namespace PVD
{
	class PvdBinding;
}

namespace physx
{
namespace apex
{

PxCpuDispatcher* createDefaultThreadPool(unsigned int numThreads);

class SharedQueueEntry : public shdfnd::SListEntry
{
public:
	SharedQueueEntry(void* objectRef) : mObjectRef(objectRef), mPooledEntry(false) {}
	SharedQueueEntry() : mObjectRef(NULL), mPooledEntry(true) {}

public:
	void* mObjectRef;
	bool mPooledEntry; // True if the entry was preallocated in a pool
};

template<class Alloc = typename shdfnd::AllocatorTraits<SharedQueueEntry>::Type >
class SharedQueueEntryPool : private Alloc
{
public:
	SharedQueueEntryPool(PxU32 poolSize, const Alloc& alloc = Alloc());
	~SharedQueueEntryPool();

	SharedQueueEntry* getEntry(void* objectRef);
	void putEntry(SharedQueueEntry& entry);

private:
	SharedQueueEntry*					mTaskEntryPool;
	shdfnd::SList						mTaskEntryPtrPool;
};

template <class Alloc>
SharedQueueEntryPool<Alloc>::SharedQueueEntryPool(PxU32 poolSize, const Alloc& alloc)
	: Alloc(alloc)
{
	shdfnd::AlignedAllocator<PX_SLIST_ALIGNMENT, Alloc> alignedAlloc;

	mTaskEntryPool = poolSize ? (SharedQueueEntry*)alignedAlloc.allocate(sizeof(SharedQueueEntry) * poolSize, __FILE__, __LINE__) : NULL;

	if (mTaskEntryPool)
	{
		for (PxU32 i = 0; i < poolSize; i++)
		{
			PX_ASSERT((size_t(&mTaskEntryPool[i]) & (PX_SLIST_ALIGNMENT - 1)) == 0); // The SList entry must be aligned according to PX_SLIST_ALIGNMENT

			PX_PLACEMENT_NEW(&mTaskEntryPool[i], SharedQueueEntry)();
			PX_ASSERT(mTaskEntryPool[i].mPooledEntry == true);
			mTaskEntryPtrPool.push(mTaskEntryPool[i]);
		}
	}
}


template <class Alloc>
SharedQueueEntryPool<Alloc>::~SharedQueueEntryPool()
{
	if (mTaskEntryPool)
	{
		shdfnd::AlignedAllocator<PX_SLIST_ALIGNMENT, Alloc> alignedAlloc;
		alignedAlloc.deallocate(mTaskEntryPool);
	}
}


template <class Alloc>
SharedQueueEntry* SharedQueueEntryPool<Alloc>::getEntry(void* objectRef)
{
	SharedQueueEntry* e = static_cast<SharedQueueEntry*>(mTaskEntryPtrPool.pop());
	if (e)
	{
		PX_ASSERT(e->mPooledEntry == true);
		e->mObjectRef = objectRef;
		return e;
	}
	else
	{
		shdfnd::AlignedAllocator<PX_SLIST_ALIGNMENT, Alloc> alignedAlloc;
		e = (SharedQueueEntry*)alignedAlloc.allocate(sizeof(SharedQueueEntry), __FILE__, __LINE__);
		if (e)
		{
			PX_PLACEMENT_NEW(e, SharedQueueEntry)(objectRef);
			PX_ASSERT(e->mPooledEntry == false);
		}

		return e;
	}
}


template <class Alloc>
void SharedQueueEntryPool<Alloc>::putEntry(SharedQueueEntry& entry)
{
	if (entry.mPooledEntry)
	{
		entry.mObjectRef = NULL;
		mTaskEntryPtrPool.push(entry);
	}
	else
	{
		shdfnd::AlignedAllocator<PX_SLIST_ALIGNMENT, Alloc> alignedAlloc;
		alignedAlloc.deallocate(&entry);
	}
}

#define TASK_QUEUE_ENTRY_POOL_SIZE 128

class TaskQueueHelper
{
public:
	static PxBaseTask* fetchTask(shdfnd::SList& taskQueue, SharedQueueEntryPool<>& entryPool)
	{
		SharedQueueEntry* entry = static_cast<SharedQueueEntry*>(taskQueue.pop());
		if (entry)
		{
			PxBaseTask* task = reinterpret_cast<PxBaseTask*>(entry->mObjectRef);
			entryPool.putEntry(*entry);
			return task;
		}
		else
		{
			return NULL;
		}
	}
};


class DefaultCpuDispatcher;

class CpuWorkerThread : public shdfnd::Thread
{
public:
	CpuWorkerThread();
	~CpuWorkerThread();

	void					initialize(DefaultCpuDispatcher* ownerDispatcher);
	void					execute();
	bool					tryAcceptJobToLocalQueue(PxBaseTask& task, shdfnd::Thread::Id taskSubmitionThread);
	PxBaseTask*		giveUpJob();

protected:
	SharedQueueEntryPool<>			mQueueEntryPool;
	DefaultCpuDispatcher*			mOwner;
	shdfnd::SList      				mLocalJobList;
	shdfnd::Thread::Id				mThreadId;
	PVD::PvdBinding					*mPvdBinding;
};

/*
 * Default CpuDispatcher implementation, if none is provided
 */
class DefaultCpuDispatcher : public PxCpuDispatcher, public shdfnd::UserAllocated
{
	friend class TaskQueueHelper;

private:
	DefaultCpuDispatcher() : mQueueEntryPool(0) {}
	~DefaultCpuDispatcher();

public:
	DefaultCpuDispatcher(PxU32 numThreads, PxU32* affinityMasks);
	void					submitTask(PxBaseTask& task);
	void					flush( PxBaseTask& task, PxI32 targetRef);
	PxU32					getWorkerCount() const;
	void					release();

	PxBaseTask*				getJob();
	PxBaseTask*				stealJob();
	void					waitForWork()
	{
		mWorkReady.wait();
	}
	void					resetWakeSignal();
	static PxU32			getAffinityMask(PxU32 affinityMask);

protected:
	CpuWorkerThread*				mWorkerThreads;
	SharedQueueEntryPool<>			mQueueEntryPool;
	shdfnd::SList					mJobList;
	shdfnd::Sync					mWorkReady;
	PxU32							mNumThreads;
	bool							mShuttingDown;
	PVD::PvdBinding					*mPvdBinding;
};

} // end pxtask namespace
} // end physx namespace

#endif // _THREAD_POOL_H_
