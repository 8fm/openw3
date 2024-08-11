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

#include "PxsMBPTasks.h"
#include "PxsContext.h"
#include "PxsAABBManager.h"
#include "PxsBroadPhaseMBP.h"
#include "CmFlushPool.h"
#include "PxcScratchAllocator.h"
#include "PsTime.h"

//	#define DUMP_TOTAL_MBP_TIME
// 256 convex stacks: ~880 most of the time, jumps to 1400 max
// pot pourri box: ~2700 to ~3300
// boxes: ~1400 to ~1900
//	#define DUMP_MBP_TIME

using namespace physx;

///////////////////////////////////////////////////////////////////////////////

#ifdef DUMP_TOTAL_MBP_TIME
	static PxU64 gStartTime = shdfnd::Time::getCurrentCounterValue();
#endif

///////////////////////////////////////////////////////////////////////////////

MBPUpdateWorkTask::MBPUpdateWorkTask(PxcScratchAllocator& scratchAllocator)
	: mScratchAllocator(scratchAllocator)
{
#ifdef PX_PS3
	mSPUTask = (MBPTaskSPU*)physx::shdfnd::AlignedAllocator<128>().allocate(sizeof(MBPTaskSPU), __FILE__, __LINE__);
	#ifdef MBP_SINGLE_SPU
	const PxU32 nbSPUs = 1;
	#else
	const PxU32 nbSPUs = 4;	// ### don't hardcode it
//	const PxU32 nbSPUs = 6;	// ### don't hardcode it
//	const PxU32 nbSPUs = mNumSpus;
//	printf("nbSPUs: %d\n", nbSPUs);
	#endif
	PX_PLACEMENT_NEW(mSPUTask, MBPTaskSPU)(nbSPUs);
#endif
}

MBPUpdateWorkTask::~MBPUpdateWorkTask()
{
#ifdef PX_PS3
	physx::shdfnd::AlignedAllocator<128>().deallocate(mSPUTask);
#endif
}

MBPPostUpdateWorkTask::MBPPostUpdateWorkTask(PxcScratchAllocator& scratchAllocator)
	: mScratchAllocator(scratchAllocator)
{
}


class MyTestTask : public PxLightCpuTask
{
public:
	MyTestTask()	{}

	virtual void run()
	{
		printf("I'm alive!\n");
	}

	virtual const char* getName() const
	{
		return "MyTestTask";
	}
};


void MBPUpdateWorkTask::run()
{
#ifdef DUMP_MBP_TIME
	PxU64 startTime = shdfnd::Time::getCurrentCounterValue();
#endif

#ifdef PX_PS3
	if(mNumSpus)
	{
		//printf("nbSPUs: %d\n", mNumSpus);
//		mSPUTask->setSpuCount(mNumSpus);
		mSPUTask->setNbSPUs(mNumSpus);

		mSPUTask->init(mScratchAllocator, *mMBP);
		mSPUTask->setContinuation(getContinuation());
		mSPUTask->removeReference();

//		mMBP->updatePPU(getContinuation());
	}
	else
#endif
	{
#if 0
		if(0)
		{
			const PxU32 numTasks = 4;
			MyTestTask* tasks = (MyTestTask*)mContext->getTaskPool().allocate(sizeof(MyTestTask) * numTasks);
			for(PxU32 i=0; i<numTasks; i++)
			{
				MyTestTask* pTask = PX_PLACEMENT_NEW((&tasks[i]), MyTestTask)();
				pTask->setContinuation(getContinuation());
				pTask->removeReference();
			}
		}
#endif

/*
	const PxU32 maxTasks = task.getTaskManager()->getCpuDispatcher()->getWorkerCount();
	const PxU32 numTasks = PxMin((bodyCount/IntegrationPerThread) + 1, maxTasks);

	if(numTasks > 1)
	{
		PxsAtomIntegrateTask* tasks = (PxsAtomIntegrateTask*)mContext->getTaskPool().allocate(sizeof(PxsAtomIntegrateTask) * numTasks);
		PxI32* atomicTesters = (PxI32*)mContext->getTaskPool().allocate(sizeof(PxI32) * 2);

		PxI32* pCounter = atomicTesters;
		volatile PxI32* pNumIntegrated = &atomicTesters[1];

		*pCounter = 0;

		*pNumIntegrated = 0;

		for(PxU32 a = 0; a < numTasks; ++a)
		{
			const PxU32 remainingBodies = PxMin(bodyCount - a, IntegrationPerThread);
			PX_UNUSED(remainingBodies);
			PxsAtomIntegrateTask* pTask = PX_PLACEMENT_NEW((&tasks[a]), PxsAtomIntegrateTask)(*this, bodyArray,
							originalBodyArray, solverBodyPool, solverBodyDataPool, motionVelocityArray, accelerationArray, dt,bodyCount, pCounter, pNumIntegrated,
							&maxSolverPositionIterations, &maxSolverVelocityIterations);

			pTask->setContinuation(&task);
			pTask->removeReference();
		}
	}
	else*/

		mMBP->updatePPU(getContinuation());
	}
#ifdef DUMP_MBP_TIME
	PxU64 endTime = shdfnd::Time::getCurrentCounterValue();
	printf("MBPUpdateWorkTask Time: %llu\n", endTime - startTime);
#endif
}

///////////////////////////////////////////////////////////////////////////////

void MBPPostUpdateWorkTask::run()
{
#ifdef DUMP_MBP_TIME
	PxU64 startTime = shdfnd::Time::getCurrentCounterValue();
#endif
	{
		mMBP->postUpdatePPU(getContinuation());
	}
#ifdef DUMP_MBP_TIME
	PxU64 endTime = shdfnd::Time::getCurrentCounterValue();
	printf("MBPPostUpdateWorkTask Time: %llu\n", endTime - startTime);
#endif

#ifdef DUMP_TOTAL_MBP_TIME
	PxU64 endTime = shdfnd::Time::getCurrentCounterValue();
	printf("MBP Time: %llu\n", endTime - gStartTime);
#endif
}

///////////////////////////////////////////////////////////////////////////////
