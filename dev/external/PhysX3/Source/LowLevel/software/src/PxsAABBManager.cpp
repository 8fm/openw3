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

#include "PxvConfig.h"

#include "PxsAABBManager.h"
#include "PxsBroadPhaseConfig.h"
#include "Ice/IceRevisitedRadix2.h"
#include "PsUtilities.h"
#include "PsAllocator.h"
#include "CmTmpMem.h"
#include "PxsBodyShapeBPHandle.h"
#include "PxsBroadPhaseSap.h"
#include "PxsRigidBody.h"

#include "CmEventProfiler.h"

using namespace physx;

#ifdef PX_PS3
	#include "PS3Support.h"
	#include "CellComputeAABBTask.h"
	#include "CellSPUProfiling.h"
	#include "CellTimerMarker.h"
#endif //PX_PS3

#define DEFAULT_CREATEDDELETED_PAIR_ARRAY_CAPACITY 64

///////////////////////////////////////////////////

namespace physx
{
class AABBUpdateTask: public physx::PxLightCpuTask
{
public:

	AABBUpdateTask(Cm::EventProfiler& eventProfiler, PxsAABBManager* AABBMgr, const PxsComputeAABBParams& params, const bool secondBroadPhase)
		: mEventProfiler(eventProfiler),
	      mAABBMgr(AABBMgr),
		  mBPStart(0),
		  mBPCount(0),
		  mCompoundStart(0),
		  mCompoundCount(0),
		  mParams(params),
		  mSecondBroadPhase(secondBroadPhase),
		  mNumFastMovingShapes(0)
	{
	}

	virtual ~AABBUpdateTask()
	{}


	void setBPCount(const PxU32 bpStart, const PxU32 bpCount)
	{
		mBPStart = bpStart;
		mBPCount = bpCount;
	}

	void setCompoundCount(const PxU32 compoundStart, const PxU32 compoundCount)
	{
		mCompoundStart = compoundStart;
		mCompoundCount = compoundCount;
	}

	virtual void run()
	{
		PxcBpHandle updatedElemIdStatics[2048];
		PxcBpHandle updatedElemIdDynamics[2048];

		PxU32 numFastMovingShapes=0;

		if(mBPCount>0)
		{
			PxU32 updatedElemIdsStaticSize=0;
			PxU32 updatedElemIdsDynamicSize=0;

			const PxcBpHandle* PX_RESTRICT updatedElemIds = mAABBMgr->mBPUpdatedElemIds;
			const PxcBpHandle* groups = mAABBMgr->mBPElems.getGroups();
			const PxcBpHandle* PX_RESTRICT aabbDataHandles = mAABBMgr->mBPElems.getAABBDataHandles();
			const PxcAABBDataStatic* PX_RESTRICT staticAABBData = mAABBMgr->mBPElems.getStaticAABBDataArray();
			const PxcAABBDataDynamic* dynamicAABBData = mAABBMgr->mBPElems.getDynamicAABBDataArray();
			IntegerAABB* PX_RESTRICT bounds = mAABBMgr->mBPElems.getBounds();
			const PxU32 boundsCapacity = mAABBMgr->mBPElems.getCapacity();

			const PxU32 start = mBPStart;
			const PxU32 end = mBPStart + mBPCount;
			for(PxU32 i=start;i<end;i++)
			{
				const PxcBpHandle id=updatedElemIds[i];
				if(0==groups[id])
				{
					updatedElemIdStatics[updatedElemIdsStaticSize]=id;
					updatedElemIdsStaticSize++;
					if(2048==updatedElemIdsStaticSize)
					{
						updateBodyShapeAABBs(updatedElemIdStatics, updatedElemIdsStaticSize, aabbDataHandles, staticAABBData, mParams, mSecondBroadPhase, bounds, boundsCapacity);
						updatedElemIdsStaticSize=0;
					}
				}
				else
				{
					updatedElemIdDynamics[updatedElemIdsDynamicSize]=id;
					updatedElemIdsDynamicSize++;
					if(2048==updatedElemIdsDynamicSize)
					{
						numFastMovingShapes+=updateBodyShapeAABBs(updatedElemIdDynamics, updatedElemIdsDynamicSize, aabbDataHandles, dynamicAABBData, mParams, mSecondBroadPhase, bounds, boundsCapacity);
						updatedElemIdsDynamicSize=0;
					}
				}
			}

			if(updatedElemIdsStaticSize>0)
			{
				updateBodyShapeAABBs(updatedElemIdStatics, updatedElemIdsStaticSize, aabbDataHandles, staticAABBData, mParams, mSecondBroadPhase, bounds, boundsCapacity);
			}

			if(updatedElemIdsDynamicSize>0)
			{
				numFastMovingShapes+=updateBodyShapeAABBs(updatedElemIdDynamics, updatedElemIdsDynamicSize, aabbDataHandles, dynamicAABBData, mParams, mSecondBroadPhase, bounds, boundsCapacity);
			}
		}

		if(mCompoundCount>0)
		{
			PxU32 updatedElemIdsStaticSize=0;
			PxU32 updatedElemIdsDynamicSize=0;

			const PxcBpHandle* PX_RESTRICT updatedElemIds = mAABBMgr->mCompoundUpdatedElemIds;
			const PxcBpHandle* groups = mAABBMgr->mCompoundElems.getGroups();
			const PxcBpHandle* PX_RESTRICT aabbDataHandles = mAABBMgr->mCompoundElems.getAABBDataHandles();
			const PxcAABBDataStatic* PX_RESTRICT staticAABBData = mAABBMgr->mCompoundElems.getStaticAABBDataArray();
			const PxcAABBDataDynamic* dynamicAABBData = mAABBMgr->mCompoundElems.getDynamicAABBDataArray();
			IntegerAABB* PX_RESTRICT bounds = mAABBMgr->mCompoundElems.getBounds();
			const PxU32 boundsCapacity = mAABBMgr->mCompoundElems.getCapacity();

			const PxU32 start = mCompoundStart;
			const PxU32 end = mCompoundStart + mCompoundCount;
			for(PxU32 i=start;i<end;i++)
			{
				const PxcBpHandle id=updatedElemIds[i];
				if(0==groups[id])
				{
					updatedElemIdStatics[updatedElemIdsStaticSize]=id;
					updatedElemIdsStaticSize++;
					if(2048==updatedElemIdsStaticSize)
					{
						updateBodyShapeAABBs(updatedElemIdStatics, updatedElemIdsStaticSize, aabbDataHandles, staticAABBData, mParams, mSecondBroadPhase, bounds, boundsCapacity);
						updatedElemIdsStaticSize=0;
					}
				}
				else
				{
					updatedElemIdDynamics[updatedElemIdsDynamicSize]=id;
					updatedElemIdsDynamicSize++;
					if(2048==updatedElemIdsDynamicSize)
					{
						numFastMovingShapes+=updateBodyShapeAABBs(updatedElemIdDynamics, updatedElemIdsDynamicSize, aabbDataHandles, dynamicAABBData, mParams, mSecondBroadPhase, bounds, boundsCapacity);
						updatedElemIdsDynamicSize=0;
					}
				}
			}

			if(updatedElemIdsStaticSize>0)
			{
				updateBodyShapeAABBs(updatedElemIdStatics, updatedElemIdsStaticSize, aabbDataHandles, staticAABBData, mParams, mSecondBroadPhase, 
					bounds, boundsCapacity);
			}

			if(updatedElemIdsDynamicSize>0)
			{
				numFastMovingShapes+=updateBodyShapeAABBs(updatedElemIdDynamics, updatedElemIdsDynamicSize, aabbDataHandles, dynamicAABBData, mParams, mSecondBroadPhase, 
					bounds, boundsCapacity);
			}
		}

		mNumFastMovingShapes+=numFastMovingShapes;
	}

	virtual const char* getName() const { return "AABBUpdateTask"; }

	PX_FORCE_INLINE PxU32 getNumFastMovingShapes() const
	{
		return mNumFastMovingShapes;
	}

	PX_FORCE_INLINE void setNumFastMovingShapes(const PxU32 numFastMovingShapes)
	{
		mNumFastMovingShapes = numFastMovingShapes;
	}

private:
	AABBUpdateTask& operator=(const AABBUpdateTask&);
	Cm::EventProfiler& mEventProfiler;
	PxsAABBManager* mAABBMgr;
	PxU32 mBPStart;
	PxU32 mBPCount;
	PxU32 mCompoundStart;
	PxU32 mCompoundCount;
	PxsComputeAABBParams mParams;
	bool mSecondBroadPhase;
	PxU32 mNumFastMovingShapes;
};

void AABBUpdateWorkTask::updateNumFastMovingShapes() const
{
	PxU32 numFastMovingShapes=0;
	for(PxU32 i=0;i<eMAX_NUM_TASKS;i++)
	{
		numFastMovingShapes+=mAABBUpdateTask[i]->getNumFastMovingShapes();
	}
	*mParams.numFastMovingShapes = numFastMovingShapes;
}

void AABBUpdateWorkTask::setNumFastMovingShapes(const PxU32 task, const PxU32 numFastMovingShapes)
{
	PX_ASSERT(task<eMAX_NUM_TASKS);
	mAABBUpdateTask[task]->setNumFastMovingShapes(numFastMovingShapes);
}

} //namespace physx

#ifdef PX_PS3

namespace physx
{
class AABBUpdateTaskSPU : public physx::PxSpuTask
{
public:

	AABBUpdateTaskSPU (PxsAABBManager* context,  const PxsComputeAABBParams& params, bool secondBroadPhase, PxU32 numSpusToUse) : 
	  PxSpuTask(gPS3GetElfImage(SPU_ELF_COMPUTEAABB_TASK), gPS3GetElfSize(SPU_ELF_COMPUTEAABB_TASK), numSpusToUse),
		  mContext(context)
	  {
		  mCellComputeAABBSPUInput.mParams							= params;
		  mCellComputeAABBSPUInput.mSecondBroadPhase				= secondBroadPhase;

		  //bp elems.
		  {
			  mCellComputeAABBSPUInput.mBPUpdatedElems				= context->mBPUpdatedElemIds;
			  mCellComputeAABBSPUInput.mBPUpdatedElemsSize			= context->mBPUpdatedElemIdsSize;

			  mCellComputeAABBSPUInput.mBPElemsWordStarts			= context->mBPUpdatedElemWordStarts;
			  mCellComputeAABBSPUInput.mBPElemsWordEnds				= context->mBPUpdatedElemWordEnds;
			  mCellComputeAABBSPUInput.mBPElemsWordCount			= context->mBPUpdatedElemWordCount;


			  mCellComputeAABBSPUInput.mBPElemsBoxMinMaxXYZ			= context->mBPElems.getBounds();		
			  mCellComputeAABBSPUInput.mBPElemsAABBDataHandles		= context->mBPElems.getAABBDataHandles();
			  mCellComputeAABBSPUInput.mBPElemsCapacity				= context->mBPElems.getCapacity();

			  mCellComputeAABBSPUInput.mBPElemsAABBDataStatic		= context->mBPElems.getStaticAABBDataArray();
			  mCellComputeAABBSPUInput.mBPElemsAABBDataStaticCapacity= context->mBPElems.getStaticAABBDataArrayCapacity();
			  mCellComputeAABBSPUInput.mBPElemsAABBDataDynamic		= context->mBPElems.getDynamicAABBDataArray();
			  mCellComputeAABBSPUInput.mBPElemsAABBDataDynamicCapacity= context->mBPElems.getDynamicAABBDataArrayCapacity();
		  }

		  //compound elems
		  {
			  mCellComputeAABBSPUInput.mCompoundUpdatedElems		= context->mCompoundUpdatedElemIds;
			  mCellComputeAABBSPUInput.mCompoundUpdatedElemsSize	= context->mCompoundUpdatedElemIdsSize;

			  mCellComputeAABBSPUInput.mCompoundElemsWordStarts		= context->mCompoundUpdatedElemWordStarts;
			  mCellComputeAABBSPUInput.mCompoundElemsWordEnds		= context->mCompoundUpdatedElemWordEnds;
			  mCellComputeAABBSPUInput.mCompoundElemsWordCount		= context->mCompoundUpdatedElemWordCount;

			  mCellComputeAABBSPUInput.mCompoundElemsBounds			= context->mCompoundElems.getBounds();		
			  mCellComputeAABBSPUInput.mCompoundElemsAABBDataHandles= context->mCompoundElems.getAABBDataHandles();
			  mCellComputeAABBSPUInput.mCompoundElemsCapacity		= context->mCompoundElems.getCapacity();

			  mCellComputeAABBSPUInput.mCompoundElemsAABBDataStatic	= context->mCompoundElems.getStaticAABBDataArray();
			  mCellComputeAABBSPUInput.mCompoundElemsAABBDataStaticCapacity = context->mCompoundElems.getStaticAABBDataArrayCapacity();
			  mCellComputeAABBSPUInput.mCompoundElemsAABBDataDynamic	= context->mCompoundElems.getDynamicAABBDataArray();
			  mCellComputeAABBSPUInput.mCompoundElemsAABBDataDynamicCapacity = context->mCompoundElems.getDynamicAABBDataArrayCapacity();
		  }

		  mCellComputeAABBSPUInput.mActiveTaskCount					= numSpusToUse;	
		  mCellComputeAABBSPUInput.mTotalTaskCount					= numSpusToUse;	

#if SPU_PROFILE
		  for(PxU32 i=0;i<MAX_NUM_SPU_PROFILE_ZONES;i++)
		  {
			  mProfileCounters[i]=0;
		  }
		  mCellComputeAABBSPUInput.mProfileZones					= mProfileCounters;
#endif

		  // set SPU arguments
		  for (PxU32 uiTask=0; uiTask < numSpusToUse; uiTask++) 
		  {
			  mCellComputeAABBSPUOutput[uiTask].mNumFastMovingShapes	=	0;
			  setArgs(uiTask, uiTask | (unsigned int)&mCellComputeAABBSPUOutput[uiTask], (unsigned int)&mCellComputeAABBSPUInput);
		  }
	  }

	  virtual const char* getName() const { return "AABBUpdateWorkTaskSPU"; }

	  virtual void release()
	  {
		  for (PxU32 uiTask=0; uiTask < getSpuCount(); uiTask++) 
		  {
			  mContext->mAABBUpdateWorkTask.setNumFastMovingShapes(uiTask, mCellComputeAABBSPUOutput[uiTask].mNumFastMovingShapes);
		  }
		  PxSpuTask::release();
	  }

private:

	CellComputeAABBSPUInput			PX_ALIGN(128, mCellComputeAABBSPUInput);				
	CellComputeAABBSPUOutput		PX_ALIGN(128, mCellComputeAABBSPUOutput[6]);	
	PxU64							PX_ALIGN(16,  mProfileCounters[MAX_NUM_SPU_PROFILE_ZONES]);

	PxsAABBManager* mContext;
};
}//physx
#endif // PX_PS3


AABBUpdateWorkTask::AABBUpdateWorkTask(Cm::EventProfiler& eventProfiler)
: mEventProfiler(eventProfiler)
{
#ifdef PX_PS3
	mAABBUpdateTaskSPU = (AABBUpdateTaskSPU*)physx::shdfnd::AlignedAllocator<128>().allocate(sizeof(AABBUpdateTaskSPU), __FILE__, __LINE__);
#endif

	for(PxU32 i=0;i<eMAX_NUM_TASKS;i++)
	{
		mAABBUpdateTask[i] = (AABBUpdateTask*)physx::shdfnd::AlignedAllocator<128>().allocate(sizeof(AABBUpdateTask), __FILE__, __LINE__);
	}
}

AABBUpdateWorkTask::~AABBUpdateWorkTask()
{
#ifdef PX_PS3
	physx::shdfnd::AlignedAllocator<128>().deallocate(mAABBUpdateTaskSPU);
#endif

	for(PxU32 i=0;i<eMAX_NUM_TASKS;i++)
	{
		physx::shdfnd::AlignedAllocator<128>().deallocate(mAABBUpdateTask[i]);
	}
}

void AABBUpdateWorkTask::computeTaskWork(const PxU32 numUpdatedAABBs, const PxU32 numTasks, PxU32* starts, PxU32* counts)
{
	PX_ASSERT(numUpdatedAABBs>eTASK_UNIT_WORK_SIZE);
	PX_ASSERT(numTasks>1);
	PX_ASSERT(numTasks<=eMAX_NUM_TASKS);

	const PxU32 numBlocks = (numUpdatedAABBs + (eTASK_UNIT_WORK_SIZE-1))/eTASK_UNIT_WORK_SIZE;
	const PxU32 numBlocksPerTask = numBlocks/numTasks;
	const PxU32 remainderBlocks = numBlocks - numBlocksPerTask*numTasks;

	PxU32 count=0;
	PxU32 lastStart=0;

	//Some tasks don't take on an extra block.
	PxU32 numToProcess=eTASK_UNIT_WORK_SIZE*numBlocksPerTask;
	for(PxU32 i=0;i<(numTasks-remainderBlocks);i++)
	{
		starts[i]=count;
		counts[i]=numToProcess;
		lastStart=count;
		count+=numToProcess;
	}
	//Some tasks need to do one extra block.
	numToProcess=eTASK_UNIT_WORK_SIZE*(numBlocksPerTask+1);
	for(PxU32 i=(numTasks-remainderBlocks);i<numTasks;i++)
	{
		starts[i]=count;
		counts[i]=numToProcess;
		lastStart=count;
		count+=numToProcess;
	}
	PX_ASSERT((count>=numUpdatedAABBs) && (count<=(numUpdatedAABBs+eTASK_UNIT_WORK_SIZE)));
	counts[numTasks-1] = numUpdatedAABBs - lastStart;

	//Some tasks do no work at all.
	for(PxU32 i=numTasks;i<eMAX_NUM_TASKS;i++)
	{
		starts[i]=0;
		counts[i]=0;
	}
}

void AABBUpdateWorkTask::run()
{
#ifdef PX_PS3
	startTimerMarker(eCOMPUTE_AABB); 
#endif
	CM_PROFILE_START_CROSSTHREAD(mEventProfiler,Cm::ProfileEventId::Sim::GetupdateVolumes());

#if SPU_AABB
#if FORCE_SINGLE_SPU_AABB
	const PxU32 numSpus = 1;
#else
	const PxU32 numSpus = mNumSpus;
#endif
#endif

	//Reset the number of fast moving shapes for each task.
	for(PxU32 i=0;i<eMAX_NUM_TASKS;i++)
	{
		mAABBUpdateTask[i]->setNumFastMovingShapes(0);
	}

#if SPU_AABB
	if(numSpus>0 && 
		(mAABBMgr->mCompoundUpdatedElemIdsSize>0 || mAABBMgr->mBPUpdatedElemIdsSize>0) && 
		(mAABBMgr->mCompoundUpdatedElemIdsSize<=MAX_NUM_BP_SPU_SAP_AABB && mAABBMgr->mBPUpdatedElemIdsSize<=MAX_NUM_BP_SPU_SAP_AABB) &&
		(mAABBMgr->mCompoundElems.getCapacity()<=MAX_NUM_BP_SPU_SAP_AABB && mAABBMgr->mBPElems.getCapacity()<=MAX_NUM_BP_SPU_SAP_AABB)
		)
	{
		// Create new aabb update task
		PX_ASSERT(mAABBUpdateTaskSPU);
		PX_PLACEMENT_NEW(mAABBUpdateTaskSPU, AABBUpdateTaskSPU)(mAABBMgr, mParams, mSecondBroadPhase, numSpus);

		// Set continuation and run immediately
		mAABBUpdateTaskSPU->setContinuation(mCont);
		mAABBUpdateTaskSPU->removeReference();
	}
	else
#endif
	{
		//Reset all tasks.
		for(PxU32 i=0;i<eMAX_NUM_TASKS;i++)
		{
			mAABBUpdateTask[i] = PX_PLACEMENT_NEW(mAABBUpdateTask[i], AABBUpdateTask)(mEventProfiler,mAABBMgr,mParams,mSecondBroadPhase);
		}

		const PxU32 numTasks = PxMin((PxU32)eMAX_NUM_TASKS, mNumCpuTasks);

		//Divide bp aabbs up among all tasks.
		const PxU32 numUpdatedBPElems=mAABBMgr->mBPUpdatedElemIdsSize;
		if(numUpdatedBPElems>0)
		{
			//If there are is only a single thread or just a few aabbs to update then we can
			//proceed immediately with the aabb computation on the current thread.
			if(numTasks>1 && numUpdatedBPElems>eTASK_UNIT_WORK_SIZE)
			{
				PxU32 starts[eMAX_NUM_TASKS];
				PxU32 counts[eMAX_NUM_TASKS];
				computeTaskWork(numUpdatedBPElems, numTasks, starts, counts);
				for(PxU32 i=0;i<eMAX_NUM_TASKS;i++)
				{
					mAABBUpdateTask[i]->setBPCount(starts[i],counts[i]);
				}
			}
			else
			{
				mAABBUpdateTask[0]->setBPCount(0, numUpdatedBPElems);
			}
		}

		//Divide compound aabbs up among all tasks.
		const PxU32 numUpdatedCompoundElems=mAABBMgr->mCompoundUpdatedElemIdsSize;
		if(numUpdatedCompoundElems>0)
		{
			//If there are is only a single thread or just a few aabbs to update then we can 
			//proceed immediately with the aabb computation on the current thread.
			if(numTasks>1 && numUpdatedCompoundElems>eTASK_UNIT_WORK_SIZE)
			{
				PxU32 starts[eMAX_NUM_TASKS];
				PxU32 counts[eMAX_NUM_TASKS];
				computeTaskWork(numUpdatedCompoundElems, numTasks, starts, counts);
				for(PxU32 i=0;i<eMAX_NUM_TASKS;i++)
				{
					mAABBUpdateTask[i]->setCompoundCount(starts[i],counts[i]);
				}
			}
			else
			{
				mAABBUpdateTask[0]->setCompoundCount(0, numUpdatedCompoundElems);
			}
		}

		//If we have more than 1 thread and have more than one block of work to perform then run in parallel.
		//If we have only 1 thread or just a single block of work then run without spawning another task.
		if((numTasks > 1) && (mAABBMgr->mBPUpdatedElemIdsSize>eTASK_UNIT_WORK_SIZE || mAABBMgr->mCompoundUpdatedElemIdsSize>eTASK_UNIT_WORK_SIZE))
		{
			//Run in parallel with tasks.
			for(PxU32 i=0;i<eMAX_NUM_TASKS;i++)
			{
				mAABBUpdateTask[i]->setContinuation(mCont);
			}
			for(PxU32 i=0;i<eMAX_NUM_TASKS;i++)
			{
				mAABBUpdateTask[i]->removeReference();
			}
		}
		else
		{
			//Do all the work immediately without spawning another task.
			mAABBUpdateTask[0]->run();
		}
	}
}

void AABBUpdateWorkEndTask::run()
{
	//Tidy up any loose ends needed to finish off the aabb update calculations.
	//(eg compute the aggregate bounds from the shape bounds).
	mAABBMgr->prepareBP();

	//Prepare a data structure to pass to the broadphase.
	PxcBroadPhaseUpdateData updateData(
		mAABBMgr->mBPCreatedElems.getElems(), mAABBMgr->mBPCreatedElems.getElemsSize(),
		mAABBMgr->mBPUpdatedElems.getElems(), mAABBMgr->mBPUpdatedElems.getElemsSize(),
		mAABBMgr->mBPRemovedElems.getElems(), mAABBMgr->mBPRemovedElems.getElemsSize(),
		mAABBMgr->mBPElems.getBounds(), mAABBMgr->mBPElems.getGroups(), mAABBMgr->mBPElems.getCapacity());

	//Update the broadphase.

#ifdef PX_PS3
	startTimerMarker(eBROADPHASE_SAP); 
#endif

	CM_PROFILE_START_CROSSTHREAD(mEventProfiler, Cm::ProfileEventId::Sim::GetbroadPhase());

	mAABBMgr->mBP->update(mNumCpuTasks,mNumSpus,updateData,mCont);
}

void BPUpdateFinalizeTask::run()
{
#ifdef PX_PS3
	stopTimerMarker(eBROADPHASE_SAP); 
#endif

	CM_PROFILE_STOP_CROSSTHREAD(mEventProfiler, Cm::ProfileEventId::Sim::GetbroadPhase());

	mAABBMgr->finalizeUpdate(mNumSpusSap);
}

///////////////////////////////////////////////////


/*
	- what happens to bitmaps when a subpart gets deleted? (i.e. "nb" changes)
	Very tedious to handle this case, all bitmaps from all pairs containing the compound must be upgraded

	Different cases:

	I) object gets added to an existing compound. "nb++", and the new element becomes the *head* of the
	linked list, so the mapping between the bitmaps and the objects changes.

		1) The compound's "selfCollBitmap" must be reallocated with (nb+1)*(nb+1) bits and the previous
		(nb)*(nb) bitmap must be transferred to the larger one. Tedious but not very difficult.

		2) all "compoundCollBitmap" of all CompoundPair involving the modified compound must be updated.
		This requires parsing the mCompoundPairs array, as in 'purgeCompoundPairs'. Each bitmap of each
		pair involving the modified compound must be upgraded.

			a) if the pair is compound-compound, the bitmap must be upgraded from (nb0)*(nb1) to (nb0+1)*(nb1)
			b) if the pair is single-compound, the bitmap must be upgraded from (nb0) to (nb0+1)

	II) object gets removed from an existing compound. "nb--" and the removed element can be anywhere in the
	linked list. We need to know the element's position in order to upgrade the bitmaps correctly. Contrary
	to the previous case where the new element was always inserted in the same position (the head), the hole
	left by the removed element can be anywhere, and the remapping code depends on the hole index.

		1) The compound's "selfCollBitmap" must be reallocated with (nb-1)*(nb-1) bits and the previous
		(nb)*(nb) bitmap must be transferred to the smaller one.

		2) all "compoundCollBitmap" of all CompoundPair involving the modified compound must be updated.
		This requires parsing the mCompoundPairs array, as in 'purgeCompoundPairs'. Each bitmap of each
		pair involving the modified compound must be upgraded.

			a) if the pair is compound-compound, the bitmap must be upgraded from (nb0)*(nb1) to (nb0-1)*(nb1)
			b) if the pair is single-compound, the bitmap must be upgraded from (nb0) to (nb0-1)

	So in short, this is hideous.

	Moreover:
	- we want to minimize the remaps I guess. If we remove 2 objects from a compound, do we do the remap twice
	(more expensive), or do we do the remap once, taking into account both objects (more complicated)?

	Notes:
	- for removals, maybe we can avoid "filling the hole". We may simply invalidate the linked list entry without
	actually deleting it, and just keep the bitmap until the compound is deleted (self CD bitmap) or until the
	pair separates. After all, removing an object from one of those dynamic compounds (e.g. ragdoll) shouldn't
	happen. When it does, the compound parts might go far away from each other, in which case we have bigger issues
	(e.g. the compound bounds become very large and the whole thing becomes very inefficient).
	- maybe we should just ban the whole operation.

	--------------------------------------------------------------------------

	- mix of 2 solutions? pass compound ID down to Sim creation, then save it in ActorSim

	- reoptimize away useless imul/idiv (if any) introduced by SPU changes + useless CMPs in IDToElem
	- "compound" viz is confusing
	- test bitmap promotion
	- new design:
		- refiltering pain
		- serial
		- self collisions bitmap
		- arrays of aggregates in npscene or something
	- redo compound allocation/management
	- use a bitmap iterator?
	- refactor small pieces of code like bitmap allocation, etc
	- pass integer bounds as-is to underlying BP
	- optimize:
		- skip self-cd etc also when new bounds are "the same" as before?
		- use timestamps for deletion?
	- use box pruning?
	- remove linked lists, do a shared sorted buffer
	- better memory buffer usage. When it's all done, compare memory usage to normal code.
	- fix debug colors!
*/

// PT: one non-inlined resize function for everybody please
void* physx::resizePODArray(const PxU32 oldMaxNb, const PxU32 newMaxNb, const PxU32 elemSize, void* elements)
{
	PX_ASSERT(newMaxNb > oldMaxNb);
	PX_ASSERT(newMaxNb*elemSize > 0);
	PX_ASSERT(0==((newMaxNb*elemSize) & 15)); 
	PxU8* newElements = (PxU8*)PX_ALLOC(elemSize*newMaxNb, PX_DEBUG_EXP("PODArray Elements"));
	PX_ASSERT(0==((uintptr_t)newElements & 0x0f));
	copyPodArray(newElements, elements, newMaxNb, oldMaxNb, elemSize);
	PX_FREE(elements);
	return newElements;
}

///////////////////////////////////////////////////////////////////////////////

PxsAABBManager::PxsAABBManager(Cm::EventProfiler& eventProfiler,PxcScratchAllocator& allocator) :
	mCompoundCache			(allocator),
	mBPUpdatedElemIds		(NULL),
	mBPUpdatedElemIdsSize	(0),
	mCompoundUpdatedElemIds	(NULL),
	mCompoundUpdatedElemIdsSize(0),
	mBP						(NULL),
	mCreatedPairs			(NULL),
	mCreatedPairsSize		(0),
	mCreatedPairsCapacity	(0),
	mDeletedPairs			(NULL),
	mDeletedPairsSize		(0),
	mDeletedPairsCapacity	(0),
	mCompoundPairs			(NULL),
	mCompoundPairsSize		(0),
	mCompoundPairsCapacity	(0),
	mEventProfiler			(eventProfiler),
	mAABBUpdateWorkTask		(eventProfiler),
	mAABBUpdateWorkEndTask	(eventProfiler),
	mBPUpdateFinalizeTask	(eventProfiler)
{
	for(PxU32 i=0;i<32;i++)
		mBitmasks[i] = 1<<i;

#ifdef PX_PS3
	mBPUpdatedElemWordStarts = NULL;
	mBPUpdatedElemWordEnds = NULL;
	mCompoundUpdatedElemWordStarts = NULL;
	mCompoundUpdatedElemWordEnds = NULL;
#endif

	preAllocate(PX_DEFAULT_BOX_ARRAY_CAPACITY,PX_DEFAULT_BOX_ARRAY_CAPACITY);
}

PxsAABBManager::~PxsAABBManager()
{
	for(PxU32 i=0;i<mCompoundPairsSize;i++)
	{
		if(mCompoundPairs[i].compoundCollBitmap)
		{
			PX_DELETE(mCompoundPairs[i].compoundCollBitmap);
			mCompoundPairs[i].compoundCollBitmap=NULL;
		}
	}

	PX_FREE(mCreatedPairs);
	PX_FREE(mDeletedPairs);
	PX_FREE(mCompoundPairs);

	if(mBPUpdatedElemIds != mBPUpdatedElemIdsBuffer)
		PX_FREE(mBPUpdatedElemIds);

#ifdef PX_PS3
	if(mBPUpdatedElemWordStarts != mBPUpdatedElemWordStartsBuffer)
		PX_FREE(mBPUpdatedElemWordStarts);

	if(mBPUpdatedElemWordEnds != mBPUpdatedElemWordEndsBuffer)
		PX_FREE(mBPUpdatedElemWordEnds);
#endif

	if(mCompoundUpdatedElemIds != mCompoundUpdatedElemIdsBuffer)
		PX_FREE(mCompoundUpdatedElemIds);

#ifdef PX_PS3
	if(mCompoundUpdatedElemWordStarts != mCompoundUpdatedElemWordStartsBuffer)
		PX_FREE(mCompoundUpdatedElemWordStarts);

	if(mCompoundUpdatedElemWordEnds != mCompoundUpdatedElemWordEndsBuffer)
		PX_FREE(mCompoundUpdatedElemWordEnds);
#endif
}

void PxsAABBManager::init(PxvBroadPhase* bp)
{
	mBP = bp;
}

void PxsAABBManager::preAllocate(const PxU32 nbStaticShapes, const PxU32 nbDynamicShapes)
{
	const PxU32 nbStaticShapes32 = ((nbStaticShapes  + 31) & ~31);
	const PxU32 nbDynamicShapes32 = ((nbDynamicShapes  + 31) & ~31);

	const PxU32 nbElems = nbStaticShapes32 + nbDynamicShapes32;

	if(!nbElems)
		return;

	if(mBPElems.getCapacity() < nbElems)
	{
		mBPElems.setDefaultCapacity(nbStaticShapes32, nbDynamicShapes32);

		//these arrays needs to at least track the number of bp elems
		mBPUpdatedElems.setDefaultElemsCapacity(nbElems);
		mBPCreatedElems.setDefaultElemsCapacity(nbElems);
		mBPRemovedElems.setDefaultElemsCapacity(nbElems);
		mBPUpdatedElems.growBitmap(nbElems);
		mBPCreatedElems.growBitmap(nbElems);
		mBPRemovedElems.growBitmap(nbElems);
	}

	if(mCompoundElems.getCapacity() < nbElems)
	{
		mCompoundElems.resize(nbElems);
	}

	if(mSingleManager.getCapacity()<nbElems)
	{
		mSingleManager.resize(nbElems);
	}
}

void PxsAABBManager::destroyV()
{
	mBP->destroy();
	delete this;
}

void PxsAABBManager::freeCreatedOverlaps()
{
	if(mCreatedPairsCapacity > DEFAULT_CREATEDDELETED_PAIR_ARRAY_CAPACITY)
	{
		PX_FREE(mCreatedPairs);
		mCreatedPairs = (PxvBroadPhaseOverlap*)PX_ALLOC(sizeof(PxvBroadPhaseOverlap)*DEFAULT_CREATEDDELETED_PAIR_ARRAY_CAPACITY, "PxsAABBManager");
		mCreatedPairsCapacity = DEFAULT_CREATEDDELETED_PAIR_ARRAY_CAPACITY;
	}
}

void PxsAABBManager::freeDestroyedOverlaps()
{
	if(mDeletedPairsCapacity > DEFAULT_CREATEDDELETED_PAIR_ARRAY_CAPACITY)
	{
		PX_FREE(mDeletedPairs);
		mDeletedPairs = (PxvBroadPhaseOverlap*)PX_ALLOC(sizeof(PxvBroadPhaseOverlap)*DEFAULT_CREATEDDELETED_PAIR_ARRAY_CAPACITY, "PxsAABBManager");
		mDeletedPairsCapacity = DEFAULT_CREATEDDELETED_PAIR_ARRAY_CAPACITY;
	}
}

///////////////////////////////////////////////////////////////////////////////

#include "CmRenderOutput.h"
void PxsAABBManager::visualize(Cm::RenderOutput& out)
{
	PxTransform idt = PxTransform(PxIdentity);
	out << idt;

	const PxU32 N=mCompoundManager.getCompoundsCapacity();
	for(PxU32 i=0;i<N;i++)
	{
		const Compound* compound=mCompoundManager.getCompound(i);
		if(compound->nbElems)
		{
			if(!mCompoundsUpdated.isInList((PxcBpHandle)i))
			{
				out << PxU32(PxDebugColor::eARGB_GREEN);
			}
			else
			{
				out << PxU32(PxDebugColor::eARGB_RED);
			}

			PxBounds3 decoded;
			const IntegerAABB iaabb=mBPElems.getAABB(compound->bpElemId);
			iaabb.decode(decoded);

			out << Cm::DebugBox(decoded, true);
			PxU32 elem = compound->headID;
			while(PX_INVALID_BP_HANDLE!=elem)
			{
				out << PxU32(PxDebugColor::eARGB_CYAN);
				const IntegerAABB elemBounds = mCompoundElems.getAABB(elem);
				elemBounds.decode(decoded);
				out << Cm::DebugBox(decoded, true);
				elem = mCompoundElems.getNextId(elem);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

PxU32 PxsAABBManager::createCompound(void* userData, const bool selfCollisions)
{
	//Create a compound and return the encoded compound id.
	const PxU32 oldCapacity=mCompoundManager.getCompoundsCapacity();
	const PxcBpHandle compoundId = mCompoundManager.createCompound(userData, selfCollisions);
	const PxU32 newCapacity=mCompoundManager.getCompoundsCapacity();
	if(newCapacity>oldCapacity)
	{
		mCompoundsUpdated.growBitmap(newCapacity);
	}
	return encodeCompoundForClient(compoundId);
}

void PxsAABBManager::deleteCompound(const PxU32 encodedCompoundId)
{
	PX_ASSERT(isClientVolumeCompound((PxcBpHandle)encodedCompoundId));
	const PxcBpHandle compoundId=decodeCompoundFromClient((PxcBpHandle)encodedCompoundId);
	PX_ASSERT(0==mCompoundManager.getCompound(compoundId)->nbElems);
	PX_ASSERT(PX_INVALID_BP_HANDLE==mCompoundManager.getCompound(compoundId)->headID);
	PX_ASSERT(PX_INVALID_BP_HANDLE==mCompoundManager.getCompound(compoundId)->bpElemId);
	PX_ASSERT(PX_INVALID_BP_HANDLE!=mCompoundManager.getCompound(compoundId)->group);
	mCompoundManager.reuseCompound(compoundId);
}

AABBMgrId PxsAABBManager::createVolume(const PxU32 encodedCompoundId, const PxcBpHandle encodedSingleOrCompoundId, const PxU32 group, void* userdata, const PxBounds3& bounds)
{
	if(encodedCompoundId==PX_INVALID_U32)
	{
		//The incoming object should be handled as a singleton ("fast" path).

		//Encode the bounds.
		IntegerAABB iaabb;
		iaabb.encode(bounds);

		//Get the bp elem id.
		const PxU32 bpElemId = createBPElem();
		PX_ASSERT(bpElemId < mBPElems.getCapacity());

		//Add the created bp elem id to the list of created elems.
		mBPCreatedElems.addElem((PxcBpHandle)bpElemId);

		//Set up the bp elem as a single.
		mBPElems.initAsSingle(bpElemId, userdata,(PxcBpHandle) group, iaabb); 

		//Connect the bp elem to the structure that manages all bp elems shared by the same rigid body.
		PxcBpHandle singleId=PX_INVALID_BP_HANDLE;
		if(PX_INVALID_BP_HANDLE==encodedSingleOrCompoundId)
		{
			//This is the first bp entry of a rigid body (a rigid body can have multiple shapes).
			singleId=mSingleManager.createSingle();
			Single* single=mSingleManager.getSingle(singleId);
			mBPElems.setSingleOwnerId(bpElemId,singleId);
			mBPElems.setNextId(bpElemId, PX_INVALID_BP_HANDLE);
			single->headID=(PxcBpHandle)bpElemId;
		}
		else 
		{
			//This isn't the first bp entry of a rigid body.  
			PX_ASSERT(!isClientVolumeCompound(encodedSingleOrCompoundId));
			singleId=decodeSingleFromClient(encodedSingleOrCompoundId);
			Single* single=mSingleManager.getSingle(singleId);
			mBPElems.setSingleOwnerId(bpElemId,singleId);
			const PxcBpHandle headId=single->headID;
			mBPElems.setNextId(bpElemId, headId);
			single->headID=(PxcBpHandle)bpElemId;
		}

		//Return an encoded id (internal id + isCompound).
		if(canEncodeForClient(bpElemId) && canEncodeForClient(singleId))
		{
			return AABBMgrId(encodeSingleForClient(bpElemId), encodeSingleForClient(singleId));
		}
		else
		{
			return AABBMgrId(PX_INVALID_BP_HANDLE, PX_INVALID_BP_HANDLE);
		}
	}

	PX_ASSERT(encodedSingleOrCompoundId == encodedCompoundId || encodedSingleOrCompoundId == PX_INVALID_BP_HANDLE);
	PX_ASSERT(isClientVolumeCompound((PxcBpHandle)encodedCompoundId));
	const PxcBpHandle compoundId = decodeCompoundFromClient((PxcBpHandle)encodedCompoundId);
	Compound* compound = mCompoundManager.getCompound(compoundId);

	if(PX_INVALID_BP_HANDLE == compound->headID)
	{
		//Incoming object is a compound but we've not yet added an element to the compound.
		//Alternatively we have added to the compound then removed all compound elements and are now adding again to an empty compound.

		//Encode the bounds (we need this later to initialise the bp elem bounds and to set the compound elem bounds).
		IntegerAABB encoded;
		encoded.encode(bounds);

		//Get the compound.
		Compound* compound=mCompoundManager.getCompound(compoundId);
		PX_ASSERT(0 == compound->nbActive);
		PX_ASSERT(PX_INVALID_BP_HANDLE == compound->bpElemId);
		PX_ASSERT(PX_INVALID_BP_HANDLE != compound->group);

		//Create a bp elem for the compound and set it up so the compound
		//knows the bp elem id and the bp elem knows the compound id.
		//Also set the bounds of the bp elem.
		const PxcBpHandle bpElemId = (PxcBpHandle)createBPElem();
		compound->bpElemId=bpElemId;
		mBPElems.setCompoundOwnerId(bpElemId, compoundId);
		mBPElems.setGroup(bpElemId, compound->group);
		mBPElems.initAsCompound(bpElemId,encoded);

		//Add to the created list of bp elems.
		mBPCreatedElems.addElem(bpElemId);

		//Create a compound elem for the aabb and set it up.
		const PxcBpHandle compoundElemID = (PxcBpHandle)createCompoundElem();
		PX_ASSERT(compoundElemID < mCompoundElems.getCapacity());
		mCompoundElems.init(compoundElemID,userdata,(PxcBpHandle)group,encoded,compoundId,PX_INVALID_BP_HANDLE);

		//Set up the compound.
		if(!mCompoundsUpdated.isInList(compoundId))
		{
			mCompoundsUpdated.addElem(compoundId);
		}
		compound->headID = compoundElemID;
		compound->nbElems++;
		compound->nbActive++;

		if(canEncodeForClient(compoundElemID) && canEncodeForClient(compoundId))
		{
			return AABBMgrId(encodeCompoundForClient(compoundElemID), encodeCompoundForClient(compoundId));
		}
		else
		{
			return AABBMgrId(PX_INVALID_BP_HANDLE, PX_INVALID_BP_HANDLE);
		}
	}
	else 
	{
		IntegerAABB encoded;
		encoded.encode(bounds);

		//Create a compound elem for the aabb.
		PxcBpHandle compoundElemID;
		PxU32 internalIndex=mCompoundManager.getAvailableElem(compoundId);
		if(PX_INVALID_BP_HANDLE != internalIndex)
		{
			//Remember that internalIndex was computed by counting 
			//backwards from the end of the list because the start of the 
			//list can change if we add an extra element to the aggreagate.
			//We need to first get a list of all elements in the compound
			//so we can get the reusable element by counting backwards from the end.
			PxcBpHandle componentElemIds[MAX_COMPOUND_BOUND_SIZE];
			compoundElemID = compound->headID;
			PxU32 index=0;
			while(compoundElemID!=PX_INVALID_BP_HANDLE)
			{
				componentElemIds[index]=compoundElemID;
				index++;
				compoundElemID = mCompoundElems.getNextId(compoundElemID);
			}
			PX_ASSERT(index == compound->nbElems);

			//Select the one that can be reused.
			const PxU32 componentElemIndex = compound->nbElems - 1 - internalIndex;
			PX_ASSERT(componentElemIndex < MAX_COMPOUND_BOUND_SIZE);
			compoundElemID = componentElemIds[componentElemIndex];

			//Reuse it.
			mCompoundElems.reinit(compoundElemID,userdata,(PxcBpHandle)group,encoded,compoundId,compound->headID);

			//Set up the compound.
			compound->nbActive++;

			//Add the compound to the updated list.
			if(!mCompoundsUpdated.isInList(compoundId))
			{
				mCompoundsUpdated.addElem(compoundId);
			}
		}
		else if(compound->nbElems < MAX_COMPOUND_BOUND_SIZE)
		{
			promoteBitmaps(compound);

			compoundElemID = (PxcBpHandle)createCompoundElem();
			mCompoundElems.init(compoundElemID,userdata,(PxcBpHandle)group,encoded,compoundId,compound->headID);

			//Set up the compound.
			compound->headID = compoundElemID;
			compound->nbElems++;
			compound->nbActive++;

			//Add the compound to the updated list.
			if(!mCompoundsUpdated.isInList(compoundId))
			{
				mCompoundsUpdated.addElem(compoundId);
			}
		}
		else
		{
			Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "A PxAggregate has exceeded the limit of 128 PxShapes. Not all shapes of the aggregate will be added to the broapdhase");
			return AABBMgrId();
		}

		// Merge/update bounds in the BP
		const PxcBpHandle bpElemId=compound->bpElemId;
		PX_ASSERT(bpElemId < mBPElems.getCapacity());

		IntegerAABB iaabb=mBPElems.getAABB(bpElemId);
		if(!encoded.isInside(iaabb))
		{
			iaabb.include(encoded);
			mBPElems.setAABB(bpElemId,iaabb);

			//If the element is not in the created list then we need to think about adding it to the updated list.
			//If it is in the created list then just leave it there.
			if(!mBPCreatedElems.isInList(bpElemId))
			{
				if(!mBPUpdatedElems.isInList(bpElemId))
				{
					mBPUpdatedElems.addElem(bpElemId);
				}
			}
		}

		if(canEncodeForClient(compoundElemID) && canEncodeForClient(compoundId))
		{
			return AABBMgrId(encodeCompoundForClient(compoundElemID),encodeCompoundForClient(compoundId));
		}
		else
		{
			return AABBMgrId(PX_INVALID_BP_HANDLE, PX_INVALID_BP_HANDLE);
		}
	}
}

void PxsAABBManager::setStaticAABBData(const PxcBpHandle volume, const PxcAABBDataStatic& aabbData)
{
	if(!isClientVolumeCompound(volume))
		mBPElems.setStaticAABBData(decodeSingleFromClient(volume),aabbData);
	else
		mCompoundElems.setStaticAABBData(decodeCompoundFromClient(volume),aabbData);
}

void PxsAABBManager::setDynamicAABBData(const PxcBpHandle volume, const PxcAABBDataDynamic& aabbData)
{
	if(!isClientVolumeCompound(volume))
		mBPElems.setDynamicAABBData(decodeSingleFromClient(volume),aabbData);
	else
		mCompoundElems.setDynamicAABBData(decodeCompoundFromClient(volume),aabbData);
}

bool PxsAABBManager::releaseVolume(const PxcBpHandle volume)
{
	bool removingLastShape = false;
	if(!isClientVolumeCompound(volume))
	{
		const PxcBpHandle bpElemId = decodeSingleFromClient(volume);

		purgeCompoundPairs(bpElemId);

		//If the volume is in the updated list then remove it.
		if(mBPUpdatedElems.isInList(bpElemId))
		{
			mBPUpdatedElems.removeElem(bpElemId);
		}

		//If the volume is in the created list then remove it.
		//If the volume wasn't in the created list then add it to the removed list.
		//Remember that we only need to remove elements from the bp that have already been added and sorted.
		//If the element has never been inserted into the bp then don't add it to the list to be removed from the bp.
		if(mBPCreatedElems.isInList(bpElemId))
		{
			mBPCreatedElems.removeElem(bpElemId);
		}
		else
		{
			mBPRemovedElems.addElem(bpElemId);
		}

		//Patch up the linked list of singles with common ownership.
		const PxcBpHandle singleId = mBPElems.getSingleOwnerId(bpElemId);
		Single* single=mSingleManager.getSingle(singleId);
		PxcBpHandle idPrev=single->headID;
		PX_ASSERT(idPrev!=PX_INVALID_BP_HANDLE);
		PxcBpHandle idCurr=mBPElems.getNextId(idPrev);
		if(bpElemId==idPrev)
		{
			//The first element in the linked list is to be removed.
			if(PX_INVALID_BP_HANDLE==idCurr)
			{
				//The linked list has only a single entry so the list will be empty after removing the entry.
				mSingleManager.clearSingle(singleId);
				mSingleManager.reuseSingle(singleId);
				mBPElems.setNextId(idPrev, PX_INVALID_BP_HANDLE);
				removingLastShape = true;
			}
			else
			{
				//Just set the headId to be the second entry in the linked list.
				single->headID=idCurr;
			}
		}
		else
		{
			//Find the entry in the list that is to be removed.
			while(idCurr!=bpElemId)
			{
				idPrev=idCurr;
				idCurr=mBPElems.getNextId(idCurr);
			}
			//Connect the previous entry to the next entry.
			mBPElems.setNextId(idPrev, mBPElems.getNextId(idCurr));
			//Invalidate the removed entry.
			mBPElems.setNextId(idCurr, PX_INVALID_BP_HANDLE);
		}

		//Don't free the id yet because it could then be immediately reused, leading to confusion
		//because the same id would appear in the created and removed lists.
		//Only reuse the id when the next simulate is called during updateAABBsAndBP.
		//mBPElems.freeElem(bpElemId);
	}
	else
	{
		//Get the compound elem id.
		PxU32 compoundElemId = decodeCompoundFromClient(volume);

		//Get the compound owner id of the compound element and then the compound itself.
		//Decrement nbActive and update the timestamp.
		const PxcBpHandle compoundOwnerId = mCompoundElems.getCompoundOwnerId(compoundElemId);
		Compound* PX_RESTRICT compound = mCompoundManager.getCompound(compoundOwnerId);
		compound->nbActive--;	
		if(!mCompoundsUpdated.isInList(compoundOwnerId))
		{
			mCompoundsUpdated.addElem(compoundOwnerId);
		}

		//We need to preserve the position of all the elements in the linked list to preserve the meaning of the overlap bitmaps.
		//This means we need to keep the removed element in the linked list but flag it so that in the future we neglect it when 
		//updating bp bounds, overlapping single-compound etc.
		//We obviously can't reuse the element we're trying to remove because we need to preserve the flag that marks it to be neglected.
		//Mark the element to be removed with an invalid group id.
		//Set the aabb to empty just to help us with debugging.
		mCompoundElems.setGroup(compoundElemId, PX_INVALID_BP_HANDLE);	
		mCompoundElems.setEmptyAABB(compoundElemId);	

		//Mark the element so that it can be reused after all bitmaps have been cleared.
		//Because we keep changing the head id when we add new elements to the aggregate
		//we need to count backwards from the end of the linked list rather than forwards
		//from the start.  As a consequence we need to mark (compound->nbElems - 1 - internalIndex)
		//rather than just internalIndex.  
		PxcBpHandle headId = compound->headID;
		PxU32 internalIndex = 0;
		while(headId != compoundElemId)
		{
			internalIndex++;
			headId = mCompoundElems.getNextId(headId);
		}
		mCompoundManager.releaseElem(compoundOwnerId, (PxcBpHandle)(compound->nbElems - 1 - internalIndex));

		//If there are no active elements remaining in the compound then take action by removing the compound from the bp.
		//We can now free all the compound's elems.
		if(!compound->nbActive)
		{
			//Get the bp elem id of the compound.
			const PxcBpHandle bpElemId=compound->bpElemId;

			//Remove all overlap pairs involving the bpElem
			purgeCompoundPairs(bpElemId);

			//If the bp element is in the updated list then remove it from the updated list.
			if(mBPUpdatedElems.isInList(bpElemId))
			{
				mBPUpdatedElems.removeElem(bpElemId);
			}

			//If the volume is in the created list then remove it from the created list.
			//If the volume wasn't in the created list then add it to the removed list.
			//Remember that we only need to remove elements from the bp that have already been added and sorted.
			//If the element has never been inserted into the bp then don't add it to the list to be removed from the bp.
			if(mBPCreatedElems.isInList(bpElemId))
			{
				mBPCreatedElems.removeElem(bpElemId);
			}
			else
			{
				mBPRemovedElems.addElem(bpElemId);
			}

			//Don't free the id yet because it could then be immediately reused, leading to confusion
			//because the same id would appear in the created and removed lists.
			//Only reuse the id when the next simulate is called during updateAABBsAndBP.
			//mBPElems.freeElem(bpElemId);

			//Now free all the elements of the compound (including the volume of the compound that is being freed).
			PxcBpHandle compoundId=compound->headID;
			while(PX_INVALID_BP_HANDLE!=compoundId)
			{
				const PxcBpHandle nextCompoundId=mCompoundElems.getNextId(compoundId);
				mCompoundElems.freeElem(compoundId);
				PX_ASSERT(PX_INVALID_BP_HANDLE==mCompoundElems.getNextId(compoundId));
				compoundId=nextCompoundId;
			}

			//Now clear the compound itself.
			//Don't reuse the compound it yet because we only do this in deleteCompound.
			mCompoundManager.clearCompound(compoundOwnerId);
		}
	}

	return removingLastShape;
}

void PxsAABBManager::setBPElemVolumeBounds(const PxcBpHandle singleID, const IntegerAABB& iaabb)
{
	mBPElems.setAABB(singleID,iaabb);

	if(!mBPCreatedElems.isInList(singleID) && !mBPUpdatedElems.isInList(singleID))
	{
		mBPUpdatedElems.addElem(singleID);
	}
}

void PxsAABBManager::setCompoundElemVolumeBounds(const PxcBpHandle elemId, const IntegerAABB& iaabb)
{
	mCompoundElems.setAABB(elemId,iaabb);

	const PxcBpHandle compoundId = mCompoundElems.getCompoundOwnerId(elemId);
	if(!mCompoundsUpdated.isInList(compoundId))
	{
		mCompoundsUpdated.addElem(compoundId);
	}
}

void PxsAABBManager::setVolumeBounds(const PxcBpHandle volume, const PxBounds3& bounds)
{
	IntegerAABB iaabb;
	iaabb.encode(bounds);

	if(!isClientVolumeCompound(volume))
	{
		const PxcBpHandle singleID = decodeSingleFromClient(volume);
		setBPElemVolumeBounds(singleID, iaabb);
	}
	else
	{
		const PxcBpHandle elemId = decodeCompoundFromClient(volume);
		setCompoundElemVolumeBounds(elemId, iaabb);
	}
}

PxBounds3 PxsAABBManager::getBPBounds(const PxcBpHandle volume) const
{
	PxBounds3 ret;
	const IntegerAABB iaabb = mBPElems.getAABB(volume);
	iaabb.decode(ret);
	return ret;
}

Ps::IntBool PxsAABBManager::isCompoundId(const PxcBpHandle singleOrCompoundId) const
{
	PX_ASSERT(singleOrCompoundId != PX_INVALID_BP_HANDLE);
	return isClientVolumeCompound(singleOrCompoundId);
}

PxU32 PxsAABBManager::getCompoundRigidBodies(const PxcBpHandle compoundId, PxcRigidBody** rigidBodies, const PxU32 rigidBodiesArrayCapacity) const
{
	PX_ASSERT(rigidBodiesArrayCapacity == MAX_COMPOUND_BOUND_SIZE);
	PX_UNUSED(rigidBodiesArrayCapacity);
	PX_ASSERT(compoundId != PX_INVALID_BP_HANDLE);
	PX_ASSERT(isClientVolumeCompound(compoundId));

	const Compound* compound=mCompoundManager.getCompound(decodeCompoundFromClient(compoundId));
	if(compound->nbElems>0)
	{
		PxcBpHandle id=compound->headID;

		//Record the rigid bodies in the array.
		PxU32 count=0;
		while(id!=PX_INVALID_BP_HANDLE)
		{
			//Make sure the element hasn't been removed.
			if(PX_INVALID_BP_HANDLE!=mCompoundElems.getGroup(id))
			{
				PX_ASSERT(count < rigidBodiesArrayCapacity);
				rigidBodies[count] = (PxcRigidBody*)mCompoundElems.getBodyAtom(id);
				count++;
			}
			
			//get the next shape.
			id=mCompoundElems.getNextId(id);
		}
		return count;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

void PxsAABBManager::updateAABBsAndBP(PxsContext* /*context*/, const PxU32 numCpuTasks, const PxU32 numSpusAABB, const PxU32 numSpusSap, PxBaseTask* continuation,
PxU32* PX_RESTRICT changedShapeWords, const PxU32 changedShapeWordCount, 						
const PxF32 dt, const bool secondBroadPhase, PxI32* numFastMovingShapes)
{

	// need to compute the update lists before we release the handles
	const PxsComputeAABBParams params = {dt, numFastMovingShapes};
	computeAABBUpdateLists(changedShapeWords, changedShapeWordCount, params, secondBroadPhase);				

	//The removed ids can be reused now.
	const PxcBpHandle* removedHandles=mBPRemovedElems.getElems();
	const PxU32 numRemovedHandles=mBPRemovedElems.getElemsSize();
	for(PxU32 i=0;i<numRemovedHandles;i++)
	{
		mBPElems.freeElem(removedHandles[i]);
	}


	mBPUpdateFinalizeTask.setAABBMgr(this);
	mAABBUpdateWorkEndTask.setAABBMgr(this);
	mAABBUpdateWorkTask.setAABBMgr(this);

	mBPUpdateFinalizeTask.set(numCpuTasks, numSpusSap);
	mAABBUpdateWorkEndTask.set(numCpuTasks,numSpusSap);
	mAABBUpdateWorkTask.set(numCpuTasks, numSpusAABB, params, secondBroadPhase);

	mBPUpdateFinalizeTask.setContinuation(continuation);
	mAABBUpdateWorkEndTask.setContinuation(&mBPUpdateFinalizeTask);
	mAABBUpdateWorkTask.setContinuation(&mAABBUpdateWorkEndTask);

	mBPUpdateFinalizeTask.removeReference();
	mAABBUpdateWorkEndTask.removeReference();
	mAABBUpdateWorkTask.removeReference();
}

void PxsAABBManager::computeAABBUpdateLists(
PxU32* PX_RESTRICT changedShapeWords, const PxU32 changedShapeWordCount,
const PxsComputeAABBParams& params, const bool secondBroadPhase)
{
	PX_ASSERT(changedShapeWords);
	PX_UNUSED(params);
	PX_UNUSED(secondBroadPhase);

	//Allocate some memory for the updated elem ids.
	mBPUpdatedElemIds = mBPUpdatedElemIdsBuffer;
	if(mBPElems.getCapacity() > MAX_NUM_BP_SPU_SAP_AABB)
	{
		mBPUpdatedElemIds = (PxcBpHandle*)PX_ALLOC(sizeof(PxcBpHandle)*mBPElems.getCapacity(), PX_DEBUG_EXP("PxBpHandle"));
	}
	mCompoundUpdatedElemIds = mCompoundUpdatedElemIdsBuffer;
	if(mCompoundElems.getCapacity() > MAX_NUM_BP_SPU_SAP_AABB)
	{
		mCompoundUpdatedElemIds = (PxcBpHandle*)PX_ALLOC(sizeof(PxcBpHandle)*mCompoundElems.getCapacity(), PX_DEBUG_EXP("PxBpHandle"));
	}

	//Set up the bitmap of changed shapes so we can easily find the last set bit.
	PxU8 changedShapesBitmapBuffer[sizeof(Cm::BitMap)];
	Cm::BitMap* changedShapesBitmap=(Cm::BitMap*)changedShapesBitmapBuffer;
	changedShapesBitmap->setWords(changedShapeWords, changedShapeWordCount);
	const PxU32 lastSetBit = changedShapesBitmap->findLast();

	//Work with local variables to avoid lhs (just set the member variables once at the end).
	PxU32 BPUpdatedElemIdsSize = 0;
	PxcBpHandle* PX_RESTRICT BPUpdatedElemIds=mBPUpdatedElemIds;
	PxU32 CompoundUpdatedElemIdsSize = 0;
	PxcBpHandle* PX_RESTRICT CompoundUpdatedElemIds=mCompoundUpdatedElemIds;

	//Gather lists of all bp and compound elems that need updated.
	for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
	{
		for(PxU32 b = changedShapeWords[w]; b; b &= b-1)
		{
			const PxU32 index = (w<<5|Ps::lowestSetBit(b));
			const PxcBpHandle volume = (PxcBpHandle)index;
			if (volume == PX_INVALID_BP_HANDLE)
				continue;

			if(!isClientVolumeCompound(volume))
			{
				const PxcBpHandle singleId=decodeSingleFromClient(volume);
				const Single* single=mSingleManager.getSingle(singleId);
				const PxcBpHandle headId=single->headID;
				PxcBpHandle id=headId;
				while(id!=PX_INVALID_BP_HANDLE)
				{
					BPUpdatedElemIds[BPUpdatedElemIdsSize]=id;
					BPUpdatedElemIdsSize++;
					if(!mBPCreatedElems.isInList(id) && !mBPUpdatedElems.isInList(id))
					{
						mBPUpdatedElems.addElem(id);
					}
					id=mBPElems.getNextId(id);
				}
			}
			else
			{
				const PxcBpHandle compoundId=decodeCompoundFromClient(volume);
				const Compound* compound=mCompoundManager.getCompound(compoundId);

				PX_ASSERT(compound->nbElems>0 || PX_INVALID_BP_HANDLE==compound->bpElemId);
				PX_ASSERT(compound->nbElems>0 || PX_INVALID_BP_HANDLE==compound->headID);

				//Only consider compounds that have elements.
				if(compound->nbElems>0)
				{
					PxcBpHandle id=compound->headID;
					while(id!=PX_INVALID_BP_HANDLE)
					{
						if(PX_INVALID_BP_HANDLE!=mCompoundElems.getGroup(id))
						{
							CompoundUpdatedElemIds[CompoundUpdatedElemIdsSize]=id;
							CompoundUpdatedElemIdsSize++;
						}
						id=mCompoundElems.getNextId(id);
					}

					//Add to the list of compounds that need updated.
					if(!mCompoundsUpdated.isInList(compoundId))
					{
						mCompoundsUpdated.addElem(compoundId);
					}

					//Add to the list of bp elems that will have been updated after we've updated the compounds.
					const PxcBpHandle bpElemId = compound->bpElemId;
					if(!mBPCreatedElems.isInList(bpElemId) && !mBPUpdatedElems.isInList(bpElemId))
					{
						mBPUpdatedElems.addElem(bpElemId);
					}
				}
			}
		}
	}
	mBPUpdatedElemIdsSize = BPUpdatedElemIdsSize;
	mCompoundUpdatedElemIdsSize = CompoundUpdatedElemIdsSize;


#ifdef PX_PS3

	mBPUpdatedElemWordCount = 0;
	mCompoundUpdatedElemWordCount = 0;

	if(BPUpdatedElemIdsSize>0)
	{
		Cm::BitMap bm;
		for(PxU32 i=0;i<BPUpdatedElemIdsSize;i++)
		{
			bm.growAndSet(BPUpdatedElemIds[i]);
		}

		mBPUpdatedElemWordStarts = mBPUpdatedElemWordStartsBuffer;
		mBPUpdatedElemWordEnds = mBPUpdatedElemWordEndsBuffer;
		mBPUpdatedElemWordCount = bm.getWordCount();
		if(bm.getWordCount() > (MAX_NUM_BP_SPU_SAP_AABB>>5))
		{
			mBPUpdatedElemWordStarts = (PxcBpHandle*)PX_ALLOC(sizeof(PxcBpHandle)*bm.getWordCount(), PX_DEBUG_EXP("PxBpHandle"));
			mBPUpdatedElemWordEnds = (PxcBpHandle*)PX_ALLOC(sizeof(PxcBpHandle)*bm.getWordCount(), PX_DEBUG_EXP("PxBpHandle"));
		}
		PxcBpHandle* PX_RESTRICT BPUpdatedElemWordStarts = mBPUpdatedElemWordStarts;
		PxcBpHandle* PX_RESTRICT BPUpdatedElemWordEnds = mBPUpdatedElemWordEnds;

		const PxcBpHandle* PX_RESTRICT groupIds=mBPElems.getGroups();

		BPUpdatedElemIdsSize = 0;
		for(PxU32 w = 0; w <= bm.findLast() >> 5; ++w)
		{
			BPUpdatedElemWordStarts[w]=BPUpdatedElemIdsSize;
			for(PxU32 b = bm.getWords()[w]; b; b &= b-1)
			{
				const PxU32 index = (w<<5|Ps::lowestSetBit(b));
				if(0!=groupIds[index])
				{
					BPUpdatedElemIds[BPUpdatedElemIdsSize]=index;
					BPUpdatedElemIdsSize++;
				}
				else
				{
					PxBounds3 bounds;
					const PxcAABBDataStatic& staticAABBData=mBPElems.getStaticAABBData(index);
					PxsComputeAABB(params,secondBroadPhase,staticAABBData,bounds);
					IntegerAABB iaabb;
					iaabb.encode(bounds);
					mBPElems.setAABB(index,iaabb);
				}
			}
			BPUpdatedElemWordEnds[w]=BPUpdatedElemIdsSize;
		}
	}

	if(CompoundUpdatedElemIdsSize>0)
	{
		Cm::BitMap bitmap;
		for(PxU32 i=0;i<CompoundUpdatedElemIdsSize;i++)
		{
			bitmap.growAndSet(CompoundUpdatedElemIds[i]);
		}

		mCompoundUpdatedElemWordStarts = mCompoundUpdatedElemWordStartsBuffer;
		mCompoundUpdatedElemWordEnds = mCompoundUpdatedElemWordEndsBuffer;
		mCompoundUpdatedElemWordCount = bitmap.getWordCount();
		if(bitmap.getWordCount() > (MAX_NUM_BP_SPU_SAP_AABB>>5))
		{
			mCompoundUpdatedElemWordStarts = (PxcBpHandle*)PX_ALLOC(sizeof(PxcBpHandle)*bitmap.getWordCount(), PX_DEBUG_EXP("PxBpHandle"));
			mCompoundUpdatedElemWordEnds = (PxcBpHandle*)PX_ALLOC(sizeof(PxcBpHandle)*bitmap.getWordCount(), PX_DEBUG_EXP("PxBpHandle"));
		}
		PxcBpHandle* PX_RESTRICT CompoundUpdatedElemWordStarts = mCompoundUpdatedElemWordStarts;
		PxcBpHandle* PX_RESTRICT CompoundUpdatedElemWordEnds = mCompoundUpdatedElemWordEnds;

		const PxcBpHandle* PX_RESTRICT groupIds=mCompoundElems.getGroups();

		CompoundUpdatedElemIdsSize=0;
		for(PxU32 w = 0; w <= bitmap.findLast() >> 5; ++w)
		{
			CompoundUpdatedElemWordStarts[w]=CompoundUpdatedElemIdsSize;
			for(PxU32 b = bitmap.getWords()[w]; b; b &= b-1)
			{
				const PxU32 index = (w<<5|Ps::lowestSetBit(b));
				if(0!=groupIds[index])
				{
					CompoundUpdatedElemIds[CompoundUpdatedElemIdsSize]=index;
					CompoundUpdatedElemIdsSize++;
				}
				else
				{
					PxBounds3 bounds;
					const PxcAABBDataStatic& staticAABBData=mCompoundElems.getStaticAABBData(index);
					PxsComputeAABB(params,secondBroadPhase,staticAABBData,bounds);
					IntegerAABB iaabb;
					iaabb.encode(bounds);
					mCompoundElems.setAABB(index,iaabb);
				}
			}
			CompoundUpdatedElemWordEnds[w]=CompoundUpdatedElemIdsSize;
		}
	}
#endif

	mBPUpdatedElems.computeList();
	mBPCreatedElems.computeList();
	mBPRemovedElems.computeList();
	mCompoundsUpdated.computeList();
}

void PxsAABBManager::mergeCompoundBounds()
{
	const PxU32 N = mCompoundsUpdated.getElemsSize();
	const PxcBpHandle* updatedCompounds = mCompoundsUpdated.getElems();
	for(PxU32 i=0;i<N;i++)
	{
		const PxcBpHandle compoundId = updatedCompounds[i];
		Compound& compound = *mCompoundManager.getCompound(compoundId);
		if(compound.nbElems)
		{
			IntegerAABB mergedBounds;
			mergedBounds.setEmpty();
			PxU32 elem = compound.headID;
			while(PX_INVALID_BP_HANDLE!=elem)
			{
				mergedBounds.include(mCompoundElems.getAABB(elem));
				elem = mCompoundElems.getNextId(elem);
			}
			mBPElems.setAABB(compound.bpElemId, mergedBounds);
		}
	}
}

void PxsAABBManager::prepareBP()
{
	mAABBUpdateWorkTask.updateNumFastMovingShapes();

	mergeCompoundBounds();

#ifdef PX_PS3
	stopTimerMarker(eCOMPUTE_AABB); 
#endif
	CM_PROFILE_STOP_CROSSTHREAD(mEventProfiler,Cm::ProfileEventId::Sim::GetupdateVolumes());
}

void PxsAABBManager::finalizeUpdate(const PxU32 /*numSpusSap*/)
{
	// Handle out-of-bounds objects
	{
		PxU32 nb = mBP->getNumOutOfBoundsObjects();
		const PxU32* PX_RESTRICT handles = mBP->getOutOfBoundsObjects();
		while(nb--)
		{
			const PxcBpHandle h = (PxcBpHandle)(*handles++);
			if(mBPElems.isSingle(h))
			{
				void* userData = mBPElems.getUserData(h);
				mOutOfBoundsObjects.pushBack(userData);
			}
			else
			{
				Compound* compound = mCompoundManager.getCompound(mBPElems.getCompoundOwnerId(h));
				mOutOfBoundsAggregates.pushBack(compound->userData);
			}
		}
	}

	mCompoundCache.prepare();

	// Reset the number of created and deleted pairs.
	mCreatedPairsSize = 0;
	mDeletedPairsSize = 0;

	// Self-collide all the compounds.
	selfCollideCompoundBounds();

	// Now for the main chunk of work: take all the pairs reported by the bp 
	// and add all the pairs from single-single single-compound and compound-compound overlaps.

	const PxU32 nbCreatedPairs = mBP->getNumCreatedPairs();
	const PxU32 nbDeletedPairs = mBP->getNumDeletedPairs();
	PxcBroadPhasePair* createdPairs = mBP->getCreatedPairs();
	PxcBroadPhasePair* deletedPairs = mBP->getDeletedPairs();

	// Directly add all freshly created single-single pairs.
	// Add all created single-compound or compound-compound pairs to the bp created pair array.
	PxU32 numCreatedCompoundPairs = 0;
	PxcBroadPhasePair* createdCompoundPairs = createdPairs;
	for(PxU32 i=0; i<nbCreatedPairs; i++)
	{
		// Get the bp elem ids.
		const PxcBpHandle volA = createdPairs[i].mVolA;
		const PxcBpHandle volB = createdPairs[i].mVolB;

		// Work out if the bp elems are singles or compounds.
		const bool s0 = mBPElems.isSingle(volA);
		const bool s1 = mBPElems.isSingle(volB);

		// If both are singles then we can add them as a created pair immediately.
		// If either is a compound then store in an array for later use cos we need to consider the compound elems.
		if(s0 && s1)
		{
			addCreatedPair(mBPElems.getUserData(volA), mBPElems.getUserData(volB));
		}
		else 
		{
			PX_ASSERT(numCreatedCompoundPairs<=i);
			createdCompoundPairs[numCreatedCompoundPairs].mVolA = volA;
			createdCompoundPairs[numCreatedCompoundPairs].mVolB = volB;
			numCreatedCompoundPairs++;
		}
	}

	// Directly add all freshly deleted single-single pairs.
	// Add all deleted single-compound or compound-compound pairs to the bp deleted pair array.
	PxU32 numDeletedCompoundPairs = 0;
	PxcBroadPhasePair* deletedCompoundPairs = deletedPairs;
	for(PxU32 i=0;i<nbDeletedPairs;i++)
	{
		// Get the bp elem ids.
		const PxcBpHandle volA = deletedPairs[i].mVolA;
		const PxcBpHandle volB = deletedPairs[i].mVolB;

		// Work out if the bp elems are singles or compounds.
		const bool s0 = mBPElems.isSingle(volA);
		const bool s1 = mBPElems.isSingle(volB);

		// If both are singles then we can add them as a deleted pair immediately.
		// If either is a compound then store in an array for later use cos we need to consider the compound elems.
		if(s0 && s1)
		{
			addDeletedPair(mBPElems.getUserData(volA), mBPElems.getUserData(volB));
		}
		else
		{
			PX_ASSERT(numDeletedCompoundPairs<=i);
			deletedCompoundPairs[numDeletedCompoundPairs].mVolA = volA;
			deletedCompoundPairs[numDeletedCompoundPairs].mVolB = volB;
			numDeletedCompoundPairs++;
		}
	}

	// Handle the deleted single-compound and compound-compound pairs.
	// Do this before considering the added pairs to avoid unnecessarily growing the arrays.
	for(PxU32 i=0;i<numDeletedCompoundPairs;i++)
	{
		//Get the bp elem ids.
		const PxcBpHandle volA = deletedCompoundPairs[i].mVolA;
		const PxcBpHandle volB = deletedCompoundPairs[i].mVolB;
		removeCompoundPair(volA, volB);
	}

	// Now add the freshly created single-compound and compound-compound pairs
	// to a special array of compound pairs.
	for(PxU32 i=0;i<numCreatedCompoundPairs;i++)
	{
		//Get the bp elem ids.
		const PxcBpHandle volA = createdCompoundPairs[i].mVolA;
		const PxcBpHandle volB = createdCompoundPairs[i].mVolB;
		addCompoundPair(volA, volB);
	}

	// Process all compound pairs (existing and freshly added).
	processCompoundPairs();

	// Free unnecessary buffers.
	// Don't need the arrays of created/deleted bp pairs any longer.
	mBP->freeBuffers();
	// Don't need the arrays of created/updated/removed bp elems any longer.
	mBPCreatedElems.free();
	mBPUpdatedElems.free();
	mBPRemovedElems.free();
	mCompoundsUpdated.free();
	if(mBPUpdatedElemIds != mBPUpdatedElemIdsBuffer)
	{
		PX_FREE(mBPUpdatedElemIds);
		mBPUpdatedElemIds=NULL;
	}
	if(mCompoundUpdatedElemIds != mCompoundUpdatedElemIdsBuffer)
	{
		PX_FREE(mCompoundUpdatedElemIds);
		mCompoundUpdatedElemIds=NULL;
	}
#ifdef PX_PS3
	if(mBPUpdatedElemWordStarts != mBPUpdatedElemWordStartsBuffer)
	{
		PX_FREE(mBPUpdatedElemWordStarts);
		mBPUpdatedElemWordStarts=NULL;
	}
	if(mBPUpdatedElemWordEnds != mBPUpdatedElemWordEndsBuffer)
	{
		PX_FREE(mBPUpdatedElemWordEnds);
		mBPUpdatedElemWordEnds=NULL;
	}
	if(mCompoundUpdatedElemWordStarts != mCompoundUpdatedElemWordStartsBuffer)
	{
		PX_FREE(mCompoundUpdatedElemWordStarts);
		mCompoundUpdatedElemWordStarts=NULL;
	}
	if(mCompoundUpdatedElemWordEnds != mCompoundUpdatedElemWordEndsBuffer)
	{
		PX_FREE(mCompoundUpdatedElemWordEnds);
		mCompoundUpdatedElemWordEnds=NULL;
	}
#endif

	mCompoundManager.markReleasedCompoundElemsAsAvailable();
	mCompoundCache.flush();
}

///////////////////////////////////////////////////////////////////////////////

PxU32 PxsAABBManager::createBPElem()
{
	PxU32 elemId = PX_INVALID_BP_HANDLE;

	if(mBPElems.getFirstFreeElem() != PX_INVALID_BP_HANDLE)
	{
		elemId = mBPElems.useFirstFreeElem();
	}
	else
	{
		{
			const PxU32 newCapacity = mBPElems.getCapacity() ? mBPElems.getCapacity()*2 : 32;
			mBPElems.grow(newCapacity);

			//these array needs to track the number of bp elems
			mBPUpdatedElems.growBitmap(newCapacity);
			mBPCreatedElems.growBitmap(newCapacity);
			mBPRemovedElems.growBitmap(newCapacity);
		}

		PX_ASSERT(mBPElems.getFirstFreeElem()!=PX_INVALID_BP_HANDLE);
		elemId = mBPElems.useFirstFreeElem();
	}

	PX_ASSERT(elemId != PX_INVALID_BP_HANDLE);
	return elemId;
}

PxU32 PxsAABBManager::createCompoundElem()
{
	PxU32 elemId = PX_INVALID_BP_HANDLE;

	if(mCompoundElems.getFirstFreeElem() != PX_INVALID_BP_HANDLE)
	{
		elemId = mCompoundElems.useFirstFreeElem();
	}
	else
	{
		{
			const PxU32 newCapacity = mCompoundElems.getCapacity() ? mCompoundElems.getCapacity()*2 : 32;
			mCompoundElems.grow(newCapacity);
			mCompoundsUpdated.growBitmap(newCapacity);
		}

		PX_ASSERT(mCompoundElems.getFirstFreeElem()!=PX_INVALID_BP_HANDLE);
		elemId = mCompoundElems.useFirstFreeElem();
	}

	PX_ASSERT(elemId != PX_INVALID_BP_HANDLE);
	return elemId;
}

///////////////////////////////////////////////////////////////////////////////

CompoundPair& PxsAABBManager::addCompoundPair(const PxcBpHandle bpElemId0, const PxcBpHandle bpElemId1)
{
	if(mCompoundPairsSize==mCompoundPairsCapacity)
	{
		const PxU32 oldCapacity=mCompoundPairsCapacity;
		const PxU32 newCapacity=mCompoundPairsCapacity ? mCompoundPairsCapacity*2 : 32;
		mCompoundPairs = (CompoundPair*)resizePODArray(oldCapacity, newCapacity, sizeof(CompoundPair), mCompoundPairs);
		mCompoundPairsCapacity = newCapacity;
	}

	CompoundPair& cp = mCompoundPairs[mCompoundPairsSize++];
	cp.mBPElemId0 = PxMin(bpElemId0, bpElemId1);
	cp.mBPElemId1 = PxMax(bpElemId0, bpElemId1);
	cp.compoundCollBitmap	= NULL;

	return cp;
}

bool PxsAABBManager::removeCompoundPair(const PxcBpHandle vol0, const PxcBpHandle vol1)
{
	const PxcBpHandle bpElemId0 = PxMin(vol0,vol1);
	const PxcBpHandle bpElemId1 = PxMax(vol0,vol1);

#ifdef PX_DEBUG
	bool aggregateOutOfBounds = false;
	PxU32 nb = mBP->getNumOutOfBoundsObjects();
	const PxU32* PX_RESTRICT handles = mBP->getOutOfBoundsObjects();
	while(nb--)
	{
		const PxcBpHandle h = (PxcBpHandle)(*handles++);
		if(!mBPElems.isSingle(h))
		{
			if(h==bpElemId0 || h==bpElemId1)
				aggregateOutOfBounds = true;
		}
	}
#endif

	for(PxU32 i=0;i<mCompoundPairsSize;i++)
	{
		if(mCompoundPairs[i].mBPElemId0==bpElemId0 && mCompoundPairs[i].mBPElemId1==bpElemId1)
		{
			if(mCompoundPairs[i].compoundCollBitmap)
			{
				PX_ASSERT(aggregateOutOfBounds || mBPRemovedElems.isInList(bpElemId0) || false == mBPElems.getAABB(bpElemId0).intersectsOrTouches(mBPElems.getAABB(bpElemId1)));

				// PT: the correct function could be looked up and stored inside "CompoundPair"...
				Compound* c0 = (mBPElems.isSingle(bpElemId0)) ? NULL : mCompoundManager.getCompound(mBPElems.getCompoundOwnerId(bpElemId0));
				Compound* c1 = (mBPElems.isSingle(bpElemId1)) ? NULL : mCompoundManager.getCompound(mBPElems.getCompoundOwnerId(bpElemId1));

				if(c0)
				{
					if(c1)
					{
						// Compound-compound
						collideCompoundCompoundRemovePair(c0, c1, mCompoundPairs[i].compoundCollBitmap);
					}
					else
					{
						// Compound-single
						collideSingleCompoundRemovePair(bpElemId1, c0, mCompoundPairs[i].compoundCollBitmap);
					}
				}
				else
				{
					if(c1)
					{
						// Single-compound
						collideSingleCompoundRemovePair(bpElemId0, c1, mCompoundPairs[i].compoundCollBitmap);
					}
					else
					{
						// Single-single, shouldn't happen
						PX_ASSERT(0);
					}
				}

				PX_DELETE(mCompoundPairs[i].compoundCollBitmap);
				mCompoundPairs[i].compoundCollBitmap=NULL;
			}

			mCompoundPairs[i] = mCompoundPairs[--mCompoundPairsSize];
			return true;
		}
	}
	PX_ASSERT(0);
	return false;
}


// PT: TODO: try to use timestamps instead of this
void PxsAABBManager::purgeCompoundPairs(const PxcBpHandle bpElemId)
{
	PxU32 nb = mCompoundPairsSize;
	PxU32 i = 0;
	while(nb--)
	{
		// PT: TODO: try to use one test only
		if(bpElemId==mCompoundPairs[i].mBPElemId0 || bpElemId==mCompoundPairs[i].mBPElemId1)
		{
			// PT: we don't issue "deleted pairs" here because the PhysX broadphase also doesn't. But in theory we should.
			if(mCompoundPairs[i].compoundCollBitmap)
			{
				PX_DELETE(mCompoundPairs[i].compoundCollBitmap);
				mCompoundPairs[i].compoundCollBitmap=NULL;
			}

			mCompoundPairs[i] = mCompoundPairs[--mCompoundPairsSize];
		}
		else i++;
	}
}

///////////////////////////////////////////////////////////////////////////////

PX_FORCE_INLINE void PxsAABBManager::addCreatedPair(void* userdata0, void* userdata1)
{
	if(mCreatedPairsSize==mCreatedPairsCapacity)
	{
		const PxU32 oldCapacity=mCreatedPairsCapacity;
		const PxU32 newCapacity=mCreatedPairsCapacity ? mCreatedPairsCapacity*2 : 32;
		mCreatedPairs = (PxvBroadPhaseOverlap*)resizePODArray(oldCapacity, newCapacity, sizeof(PxvBroadPhaseOverlap), mCreatedPairs);
		mCreatedPairsCapacity = newCapacity;
	}

	PxvBroadPhaseOverlap& pair = mCreatedPairs[mCreatedPairsSize++];
	pair.userdata0 = userdata0;
	pair.userdata1 = userdata1;
}

PX_FORCE_INLINE void PxsAABBManager::addDeletedPair(void* userdata0, void* userdata1)
{
	if(mDeletedPairsSize==mDeletedPairsCapacity)
	{
		const PxU32 oldCapacity=mDeletedPairsCapacity;
		const PxU32 newCapacity=mDeletedPairsCapacity ? mDeletedPairsCapacity*2 : 32;
		mDeletedPairs = (PxvBroadPhaseOverlap*)resizePODArray(oldCapacity, newCapacity, sizeof(PxvBroadPhaseOverlap), mDeletedPairs);
		mDeletedPairsCapacity = newCapacity;
	}

	PxvBroadPhaseOverlap& pair = mDeletedPairs[mDeletedPairsSize++];
	pair.userdata0 = userdata0;
	pair.userdata1 = userdata1;
}


///////////////////////////////////////////////////////////////////////////////

#ifdef PX_DEBUG
static int gNbChanges=0;
#endif

PX_FORCE_INLINE	void PxsAABBManager::overlapTest(const IntegerAABB& b0, const IntegerAABB& b1, void* PX_RESTRICT userdata0, void* PX_RESTRICT userdata1, Cm::BitMap* PX_RESTRICT bitmap, PxU32 bitIndex, bool flag)
{
	const Ps::IntBool b = testBitmap(*bitmap, mBitmasks, bitIndex);
	if(flag && b0.intersects(b1))
	{
		if(!b)
		{
			setBitmap(*bitmap, mBitmasks, bitIndex);
			addCreatedPair(userdata0, userdata1);
#ifdef PX_DEBUG
			gNbChanges++;
#endif
		}
	}
	else
	{
		if(b)
		{
			resetBitmap(*bitmap, mBitmasks, bitIndex);
			addDeletedPair(userdata0, userdata1);
#ifdef PX_DEBUG
			gNbChanges++;
#endif
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void PxsAABBManager::selfCollideCompound(Compound& compound)
{
	if(!compound.selfCollide)
		return;

	Cm::BitMap bitmap;
	bitmap.setWords(compound.selfCollBitMapWords, MAX_COMPOUND_WORD_COUNT);
	PxU32 bitIndex = 0;

	const PxU32 nb = compound.nbElems;
	PxU32 elemId0 = compound.headID;

	for(PxU32 i=0;i<nb;i++)
	{
		if(mCompoundElems.getGroup(elemId0) != PX_INVALID_BP_HANDLE)
		{
			PxU32 elemId1 = compound.headID;
			for(PxU32 j=0;j<i+1;j++)
				elemId1 = mCompoundElems.getNextId(elemId1);

			const IntegerAABB& elemBounds0 = mCompoundElems.getAABB(elemId0);

			for(PxU32 j=i+1;j<nb;j++)
			{
				if(mCompoundElems.getGroup(elemId1) != PX_INVALID_BP_HANDLE)
				{
					if(mCompoundElems.getGroup(elemId0) != mCompoundElems.getGroup(elemId1))
					{
						const IntegerAABB& elemBounds1 = mCompoundElems.getAABB(elemId1);
						overlapTest(elemBounds0, elemBounds1, mCompoundElems.getUserData(elemId0), mCompoundElems.getUserData(elemId1), &bitmap, bitIndex + j, true);
					}
				}
				else
				{
					resetBitmap(bitmap, mBitmasks, bitIndex + j);
					PX_ASSERT(!testBitmap(bitmap, mBitmasks, bitIndex + j));
				}
				elemId1 = mCompoundElems.getNextId(elemId1);
			}
			PX_ASSERT(PX_INVALID_BP_HANDLE==elemId1);
		}
		else
		{
			for(PxU32 j=i+1;j<nb;j++)
			{
				resetBitmap(bitmap, mBitmasks, bitIndex + j);
				PX_ASSERT(!testBitmap(bitmap, mBitmasks, bitIndex + j));
			}
		}

		bitIndex += nb;
		elemId0 = mCompoundElems.getNextId(elemId0);
	}
	PX_ASSERT(PX_INVALID_BP_HANDLE==elemId0);
}

//////////////////////////////////////////////////////////////////////////

void PxsAABBManager::selfCollideCompoundBipartite(Compound& compound,CompoundCache::CompoundData* PX_RESTRICT cData)
{
	if(!compound.selfCollide)
		return;

	Cm::BitMap bitmap;
	bitmap.setWords(compound.selfCollBitMapWords, MAX_COMPOUND_WORD_COUNT);		

	const PxU32 nb = compound.nbElems;
	PxU32 elemId0 = compound.headID;
	PxU32 bitIndex = 0;

	PxU32 maxWordCount = nb*nb/32 + 1;	
	if(maxWordCount > MAX_COMPOUND_WORD_COUNT)
		maxWordCount  = MAX_COMPOUND_WORD_COUNT;

	if(!cData->compound)
	{
		fillCompoundData(&compound,cData);
	}

	if(nb != cData->numValidElements)
	{
		for (PxU32 i = 0; i < nb; i++)
		{
			if(mCompoundElems.getGroup(elemId0) == PX_INVALID_BP_HANDLE)
			{
				for(PxU32 j=i+1;j<nb;j++)
				{
					resetBitmap(bitmap, mBitmasks, bitIndex + j);
					PX_ASSERT(!testBitmap(bitmap, mBitmasks, bitIndex + j));
				}
			}		
			bitIndex += nb;
			elemId0 = mCompoundElems.getNextId(elemId0);
		}
	}

	// parse the pairs and store new bitmap for compare
	Cm::BitMap compareBitMap;
	PxU32* bmMem = mCompoundCache.getBitmapMemory();
	intrinsics::memSet(bmMem,0,sizeof(PxU32)*maxWordCount);
	compareBitMap.setWords(bmMem,maxWordCount);

	CompoundCache::CompoundPairsArray& pairs = mCompoundCache.getPairsArray(); 
	if(completeBoxPruning(cData->elemData,cData->ranks,cData->numValidElements,pairs))
	{
		for (PxU32 i = 0; i < pairs.size();  i+=2)
		{
			PxU32 index0 = 0;	
			PxU32 index1 = 0;	

			if(pairs[i] < pairs[i+1])
			{
				index0 = pairs[i];	
				index1 = pairs[i+1];	
			}
			else
			{
				index1 = pairs[i];	
				index0 = pairs[i+1];	
			}

			const PxU32& elem1Id = cData->elemData[index1].elemId;
			const PxU32& elem1Index = cData->elemData[index1].elemIndex;
			const PxU32& elem0Id = cData->elemData[index0].elemId;
			const PxU32& elem0Index = cData->elemData[index0].elemIndex;
			PxU32 index = elem0Index*nb + elem1Index;

			if(mCompoundElems.getGroup(elem0Id) != mCompoundElems.getGroup(elem1Id))
			{				
				const Ps::IntBool b = testBitmap(bitmap, mBitmasks, index);
				if(!b)
				{
					setBitmap(bitmap, mBitmasks, index);						
					addCreatedPair(mCompoundElems.getUserData(elem0Id), mCompoundElems.getUserData(elem1Id));
#ifdef PX_DEBUG
					gNbChanges++;
#endif
				}
				setBitmap(compareBitMap, mBitmasks, index);
			}
		}
	}

	// compare the bitmaps and delete the non colliding
	for (PxU32 i = 0; i < maxWordCount; i++)
	{
		compareBitMap.getWords()[i] = bitmap.getWords()[i] & ~(compareBitMap.getWords()[i]);
	}

	Cm::BitMap::Iterator it(compareBitMap);
	PxU32 index = it.getNext();
	while(index != Cm::BitMap::Iterator::DONE)
	{
		resetBitmap(bitmap, mBitmasks, index);
		void* userdata0 = NULL;
		void* userdata1 = NULL;

		PxU32 index0 = index/nb;
		PxU32 index1 = index % nb;

		if(index1 < index0)
		{
			PxU32 tempI = index0;
			index0 = index1;
			index1 = tempI;
		}

		if(index0 < cData->numValidElements && cData->elemData[index0].elemIndex == index0)
		{
			userdata0 = mCompoundElems.getUserData(cData->elemData[index0].elemId);
		}
		else
		{
			for (PxU32 i = 0; i < cData->numValidElements; i++)
			{
				if(cData->elemData[i].elemIndex == index0)
				{
					userdata0 = mCompoundElems.getUserData(cData->elemData[i].elemId);
					break;
				}
			}
		}

		if(index1 < cData->numValidElements && cData->elemData[index1].elemIndex == index1)
		{
			userdata1 = mCompoundElems.getUserData(cData->elemData[index1].elemId);
		}
		else
		{
			for (PxU32 i = 0; i < cData->numValidElements; i++)
			{
				if(cData->elemData[i].elemIndex == index1)
				{
					userdata1 = mCompoundElems.getUserData(cData->elemData[i].elemId);
					break;
				}
			}
		}

		PX_ASSERT(userdata0);
		PX_ASSERT(userdata1);

		addDeletedPair(userdata0, userdata1);
#ifdef PX_DEBUG
		gNbChanges++;
#endif
		index=it.getNext();
	}
}
//////////////////////////////////////////////////////////////////////////

void PxsAABBManager::collideCompoundCompound(Compound* PX_RESTRICT c0, Compound* PX_RESTRICT c1, Cm::BitMap* PX_RESTRICT compoundCollBitmap, bool flag)
{
	PX_ASSERT(c0);
	PX_ASSERT(c1);

	PxU32 elemId0 = c0->headID;
	const PxU32 nb0 = c0->nbElems;
	const PxU32 nb1 = c1->nbElems;
	PxU32 bitIndex = 0;

	for(PxU32 i=0;i<nb0;i++)
	{
		if(mCompoundElems.getGroup(elemId0) != PX_INVALID_BP_HANDLE)
		{
			PxU32 elemId1 = c1->headID;

			const IntegerAABB& elemBounds0 = mCompoundElems.getAABB(elemId0);

			for(PxU32 j=0;j<nb1;j++)
			{
				if(mCompoundElems.getGroup(elemId1) != PX_INVALID_BP_HANDLE)
				{
					if(mCompoundElems.getGroup(elemId0) != mCompoundElems.getGroup(elemId1))
					{
						const IntegerAABB& elemBounds1 = mCompoundElems.getAABB(elemId1);
						overlapTest(elemBounds0, elemBounds1, mCompoundElems.getUserData(elemId0), mCompoundElems.getUserData(elemId1), compoundCollBitmap, bitIndex+j, flag);
					}
				}
				else
				{
					resetBitmap(*compoundCollBitmap,mBitmasks,bitIndex+j);
					PX_ASSERT(!testBitmap(*compoundCollBitmap,mBitmasks,bitIndex+j));
				}
				elemId1 = mCompoundElems.getNextId(elemId1);
			}
			PX_ASSERT(PX_INVALID_BP_HANDLE==elemId1);
		}
		else
		{
			for(PxU32 j=0;j<nb1;j++)
			{
				resetBitmap(*compoundCollBitmap, mBitmasks, bitIndex+j);
				PX_ASSERT(!testBitmap(*compoundCollBitmap, mBitmasks, bitIndex+j));
			}
		}
		bitIndex += nb1;
		elemId0 = mCompoundElems.getNextId(elemId0);
	}
	PX_ASSERT(PX_INVALID_BP_HANDLE==elemId0);
}

void PxsAABBManager::collideCompoundCompoundRemovePair(Compound* PX_RESTRICT c0, Compound* PX_RESTRICT c1, Cm::BitMap* PX_RESTRICT compoundCollBitmap)
{
	PX_ASSERT(c0);
	PX_ASSERT(c1);

	PxU32 elemId0 = c0->headID;
	PxU32 elemId1 = c1->headID;
	const PxU32 nb0 = c0->nbElems;
	const PxU32 nb1 = c1->nbElems;
	PxU32 bitIndex = 0;

	PX_ALLOCA(c0eleIds,PxU32,nb0);
	PX_ALLOCA(c1eleIds,PxU32,nb1);
	
	for (PxU32 i = 0; i < nb0; i++)
	{
		if(mCompoundElems.getGroup(elemId0) == PX_INVALID_BP_HANDLE)
		{
			for(PxU32 j=0;j<nb1;j++)
			{
				resetBitmap(*compoundCollBitmap, mBitmasks, bitIndex+j);
				PX_ASSERT(!testBitmap(*compoundCollBitmap, mBitmasks, bitIndex+j));
			}
		}
		c0eleIds[i]= elemId0;
		bitIndex += nb1;
		elemId0 = mCompoundElems.getNextId(elemId0);
	}

	PxU32 nb1Valid = 0;
	for (PxU32 i = 0; i < nb1; i++)
	{
		if(mCompoundElems.getGroup(elemId1) != PX_INVALID_BP_HANDLE)
		{			
			nb1Valid++;
		}
		c1eleIds[i]= elemId1;
		elemId1 = mCompoundElems.getNextId(elemId1);
	}


	// handle case where not all are valid we have to iterate again 
	if(nb1Valid != nb1)
	{
		bitIndex = 0;
		elemId0 = c0->headID;
		for(PxU32 i=0;i<nb0;i++)
		{
			if(mCompoundElems.getGroup(elemId0) != PX_INVALID_BP_HANDLE)
			{
				elemId1 = c1->headID;

				for(PxU32 j=0;j<nb1;j++)
				{
					if(mCompoundElems.getGroup(elemId1) == PX_INVALID_BP_HANDLE)
					{
						resetBitmap(*compoundCollBitmap,mBitmasks,bitIndex+j);
						PX_ASSERT(!testBitmap(*compoundCollBitmap,mBitmasks,bitIndex+j));
					}
					elemId1 = mCompoundElems.getNextId(elemId1);
				}
				PX_ASSERT(PX_INVALID_BP_HANDLE==elemId1);
			}
			bitIndex += nb1;
			elemId0 = mCompoundElems.getNextId(elemId0);
		}
	}

	Cm::BitMap::Iterator it(*compoundCollBitmap);
	PxU32 index = it.getNext();
	while(index != Cm::BitMap::Iterator::DONE)
	{
		const PxU32& index0 = index/nb1;
		const PxU32& index1 = index % nb1;

		resetBitmap(*compoundCollBitmap, mBitmasks, index);
		addDeletedPair(mCompoundElems.getUserData(c0eleIds[index0]),mCompoundElems.getUserData(c1eleIds[index1]));
#ifdef PX_DEBUG
		gNbChanges++;
#endif		
		index = it.getNext();
	}
}

void PxsAABBManager::fillCompoundData(Compound* PX_RESTRICT c, CompoundCache::CompoundData* PX_RESTRICT cData)
{
	PxU32 elemId = c->headID;
	const PxU16 nb = c->nbElems;

	CompoundCache::SortedData* sortedData = mCompoundCache.getSortedData();

	PX_ASSERT(nb < 256);

	PxU16 nbValid = 0;
	for (PxU16 i = 0; i < nb; i++)
	{
		if(mCompoundElems.getGroup(elemId) != PX_INVALID_BP_HANDLE)
		{	
			CompoundCache::ElementData& elemData = cData->elemData[nbValid];
			const IntegerAABB& elemBounds0 = mCompoundElems.getAABB(elemId);			
			elemData.elemId = elemId;
			elemData.elemIndex = i;
			elemData.bounds[0] = elemBounds0.mMinMax[0];
			elemData.bounds[1] = elemBounds0.mMinMax[1];
			elemData.bounds[2] = elemBounds0.mMinMax[2];
			elemData.bounds[3] = elemBounds0.mMinMax[3];
			elemData.bounds[4] = elemBounds0.mMinMax[4];
			elemData.bounds[5] = elemBounds0.mMinMax[5];
			sortedData[nbValid].elemIndex = nbValid;
			sortedData[nbValid].bounds = elemBounds0.mMinMax[0];
			nbValid++;
		}
		elemId = mCompoundElems.getNextId(elemId);
	}	

	cData->compound = c;
	cData->numValidElements = nbValid;	

	// sort the data
	Ps::sort(sortedData,nbValid);

	for (PxU32 i = 0; i < nbValid; i++)
	{
		cData->ranks[i] = sortedData[i].elemIndex;
	}
}

void PxsAABBManager::collideCompoundCompoundBipartite(Compound* PX_RESTRICT c0, CompoundCache::CompoundData* PX_RESTRICT c0Data, Compound* PX_RESTRICT c1, CompoundCache::CompoundData* PX_RESTRICT c1Data, Cm::BitMap* PX_RESTRICT compoundCollBitmap)
{
	PX_ASSERT(c0);
	PX_ASSERT(c1);
	PX_ASSERT(c0Data);
	PX_ASSERT(c1Data);

	if(!c0Data->compound)
	{
		fillCompoundData(c0, c0Data);
	}

	if(!c1Data->compound)
	{
		fillCompoundData(c1, c1Data);
	}

	PxU32 elemId0 = c0->headID;
	PxU32 elemId1 = c1->headID;
	const PxU32 nb0 = c0->nbElems;
	const PxU32 nb1 = c1->nbElems;
	PxU32 bitIndex = 0;

	// handle case where not all are valid we have to iterate again 
	if(nb0 != c0Data->numValidElements)
	{
		for (PxU32 i = 0; i < nb0; i++)
		{
			if(mCompoundElems.getGroup(elemId0) == PX_INVALID_BP_HANDLE)
			{
				for(PxU32 j=0;j<nb1;j++)
				{
					resetBitmap(*compoundCollBitmap, mBitmasks, bitIndex+j);
					PX_ASSERT(!testBitmap(*compoundCollBitmap, mBitmasks, bitIndex+j));
				}
			}
			bitIndex += nb1;
			elemId0 = mCompoundElems.getNextId(elemId0);
		}
	}

	bitIndex = 0;
	elemId0 = c0->headID;
	// handle case where not all are valid we have to iterate again 
	if(nb1 != c1Data->numValidElements)
	{
		for(PxU32 i=0;i<nb0;i++)
		{
			if(mCompoundElems.getGroup(elemId0) != PX_INVALID_BP_HANDLE)
			{
				elemId1 = c1->headID;

				for(PxU32 j=0;j<nb1;j++)
				{
					if(mCompoundElems.getGroup(elemId1) == PX_INVALID_BP_HANDLE)
					{
						resetBitmap(*compoundCollBitmap,mBitmasks,bitIndex+j);
						PX_ASSERT(!testBitmap(*compoundCollBitmap,mBitmasks,bitIndex+j));
					}
					elemId1 = mCompoundElems.getNextId(elemId1);
				}
				PX_ASSERT(PX_INVALID_BP_HANDLE==elemId1);
			}
			bitIndex += nb1;
			elemId0 = mCompoundElems.getNextId(elemId0);
		}
	}

	// parse the pairs and store new bitmap for compare
	Cm::BitMap bitMap;
	PxU32* bmMem = mCompoundCache.getBitmapMemory();
	intrinsics::memSet(bmMem,0,sizeof(PxU32)*compoundCollBitmap->getWordCount());
	bitMap.setWords(bmMem,compoundCollBitmap->getWordCount());

	CompoundCache::CompoundPairsArray& pairs = mCompoundCache.getPairsArray(); 
	if(bipartiteBoxPruning(c0Data->elemData,c0Data->ranks,c0Data->numValidElements,c1Data->elemData, c1Data->ranks,c1Data->numValidElements,pairs))
	{
		for (PxU32 i = 0; i < pairs.size();  i+=2)
		{
			const PxU32& index0 = pairs[i];
			const PxU32& index1 = pairs[i+1];

			const PxU32& elem0Id = c0Data->elemData[index0].elemId;
			const PxU32& elem0Index = c0Data->elemData[index0].elemIndex;
			const PxU32& elem1Id = c1Data->elemData[index1].elemId;
			const PxU32& elem1Index = c1Data->elemData[index1].elemIndex;

			if(mCompoundElems.getGroup(elem0Id) != mCompoundElems.getGroup(elem1Id))
			{
				bitIndex = nb1*elem0Index + elem1Index;

				const Ps::IntBool b = testBitmap(*compoundCollBitmap, mBitmasks, bitIndex);
				if(!b)
				{
					setBitmap(*compoundCollBitmap, mBitmasks, bitIndex);				
					addCreatedPair(mCompoundElems.getUserData(elem0Id), mCompoundElems.getUserData(elem1Id));
	#ifdef PX_DEBUG
					gNbChanges++;
	#endif
				}
				setBitmap(bitMap, mBitmasks, bitIndex);
			}
		}
	}

	// compare the bitmaps and delete the non colliding
	for (PxU32 i = 0; i < bitMap.getWordCount(); i++)
	{
		bitMap.getWords()[i] = compoundCollBitmap->getWords()[i] & ~(bitMap.getWords()[i]);
	}

	Cm::BitMap::Iterator it(bitMap);
	PxU32 index = it.getNext();
	while(index != Cm::BitMap::Iterator::DONE)
	{
		resetBitmap(*compoundCollBitmap, mBitmasks, index);
		void* userdata0 = NULL;
		void* userdata1 = NULL;

		const PxU32& index0 = index/nb1;
		const PxU32& index1 = index % nb1;

		if(index0 < c0Data->numValidElements && c0Data->elemData[index0].elemIndex == index0)
		{
			userdata0 = mCompoundElems.getUserData(c0Data->elemData[index0].elemId);
		}
		else
		{
			for (PxU32 i = 0; i < c0Data->numValidElements; i++)
			{
				if(c0Data->elemData[i].elemIndex == index0)
				{
					userdata0 = mCompoundElems.getUserData(c0Data->elemData[i].elemId);
					break;
				}
			}
		}

		if(index1 < c1Data->numValidElements && c1Data->elemData[index1].elemIndex == index1)
		{
			userdata1 = mCompoundElems.getUserData(c1Data->elemData[index1].elemId);
		}
		else
		{
			for (PxU32 i = 0; i < c1Data->numValidElements; i++)
			{
				if(c1Data->elemData[i].elemIndex == index1)
				{
					userdata1 = mCompoundElems.getUserData(c1Data->elemData[i].elemId);
					break;
				}
			}
		}

		PX_ASSERT(userdata0);
		PX_ASSERT(userdata1);

		addDeletedPair(userdata0, userdata1);
#ifdef PX_DEBUG
		gNbChanges++;
#endif
		index=it.getNext();
	}
	
}

void PxsAABBManager::collideSingleCompoundBipartite(const PxcBpHandle s0, Compound* PX_RESTRICT c1,CompoundCache::CompoundData* PX_RESTRICT c1Data, Cm::BitMap* PX_RESTRICT compoundCollBitmap)
{
	PX_ASSERT(c1);
	PX_ASSERT(c1Data);

	const IntegerAABB elemBounds0 = mBPElems.getAABB(s0);
	void* PX_RESTRICT userData0 = mBPElems.getUserData(s0);
	const PxcBpHandle group0 = mBPElems.getGroup(s0);
	
	PxU32 elemId1 = c1->headID;	
	const PxU32 nb1 = c1->nbElems;
	
	if(!c1Data->compound)
	{
		fillCompoundData(c1, c1Data);
	}

	if(nb1 != c1Data->numValidElements)
	{
		for (PxU32 i = 0; i < nb1; i++)
		{
			if(mCompoundElems.getGroup(elemId1) == PX_INVALID_BP_HANDLE)
			{
				for(PxU32 j=0;j<nb1;j++)
				{
					resetBitmap(*compoundCollBitmap, mBitmasks, i);
					PX_ASSERT(!testBitmap(*compoundCollBitmap, mBitmasks, i));
				}
			}		
			elemId1 = mCompoundElems.getNextId(elemId1);
		}
	}

	// parse the pairs and store new bitmap for compare
	Cm::BitMap bitMap;
	PxU32* bmMem = mCompoundCache.getBitmapMemory();
	intrinsics::memSet(bmMem,0,sizeof(PxU32)*compoundCollBitmap->getWordCount());
	bitMap.setWords(bmMem,compoundCollBitmap->getWordCount());

	CompoundCache::ElementData data0;
	intrinsics::memCopy(&data0.bounds[0], &elemBounds0.mMinMax[0],sizeof(PxcBPValType)*6);
	data0.elemIndex = 0;
	data0.elemId = s0;
	PxU32 ranks = 0;

	CompoundCache::CompoundPairsArray& pairs = mCompoundCache.getPairsArray(); 
	if(bipartiteBoxPruning(&data0,&ranks,1,c1Data->elemData,c1Data->ranks,c1Data->numValidElements,pairs))	
	{
		for (PxU32 i = 0; i < pairs.size();  i+=2)
		{
			const PxU32& index1 = pairs[i+1];	
			const PxU32& elem1Id = c1Data->elemData[index1].elemId;
			const PxU32& elem1Index = c1Data->elemData[index1].elemIndex;


			if(group0 != mCompoundElems.getGroup(elem1Id))
			{				
				const Ps::IntBool b = testBitmap(*compoundCollBitmap, mBitmasks, elem1Index);
				if(!b)
				{
					setBitmap(*compoundCollBitmap, mBitmasks, elem1Index);				
					addCreatedPair(userData0, mCompoundElems.getUserData(elem1Id));
#ifdef PX_DEBUG
					gNbChanges++;
#endif
				}
				setBitmap(bitMap, mBitmasks, elem1Index);
			}
		}
	}

	// compare the bitmaps and delete the non colliding
	for (PxU32 i = 0; i < bitMap.getWordCount(); i++)
	{
		bitMap.getWords()[i] = compoundCollBitmap->getWords()[i] & ~(bitMap.getWords()[i]);
	}

	Cm::BitMap::Iterator it(bitMap);
	PxU32 index = it.getNext();
	while(index != Cm::BitMap::Iterator::DONE)
	{
		resetBitmap(*compoundCollBitmap, mBitmasks, index);
		void* userdata1 = NULL;

		if(index < c1Data->numValidElements && c1Data->elemData[index].elemIndex == index)
		{
			userdata1 = mCompoundElems.getUserData(c1Data->elemData[index].elemId);
		}
		else
		{
			for (PxU32 i = 0; i < c1Data->numValidElements; i++)
			{
				if(c1Data->elemData[i].elemIndex == index)
				{
					userdata1 = mCompoundElems.getUserData(c1Data->elemData[i].elemId);
					break;
				}
			}
		}
		
		PX_ASSERT(userdata1);

		addDeletedPair(userData0, userdata1);
#ifdef PX_DEBUG
		gNbChanges++;
#endif
		index=it.getNext();
	}
}

void PxsAABBManager::collideSingleCompoundRemovePair(const PxcBpHandle s0, Compound* PX_RESTRICT c1, Cm::BitMap* PX_RESTRICT compoundCollBitmap)
{
	PX_ASSERT(c1);

	void* PX_RESTRICT userData0 = mBPElems.getUserData(s0);

	PxU32 elemId1 = c1->headID;
	const PxU32 nb1 = c1->nbElems;

	PX_ALLOCA(c1eleIds,PxU32,nb1);

	for (PxU32 i = 0; i < nb1; i++)
	{
		if(mCompoundElems.getGroup(elemId1) == PX_INVALID_BP_HANDLE)
		{
			resetBitmap(*compoundCollBitmap, mBitmasks, i);
			PX_ASSERT(!testBitmap(*compoundCollBitmap, mBitmasks, i));
		}
		c1eleIds[i]= elemId1;		
		elemId1 = mCompoundElems.getNextId(elemId1);
	}

	Cm::BitMap::Iterator it(*compoundCollBitmap);
	PxU32 index = it.getNext();
	while(index != Cm::BitMap::Iterator::DONE)
	{
		resetBitmap(*compoundCollBitmap, mBitmasks, index);
		addDeletedPair(userData0,mCompoundElems.getUserData(c1eleIds[index]));
#ifdef PX_DEBUG
		gNbChanges++;
#endif		
		index = it.getNext();
	}
}

void PxsAABBManager::collideSingleCompound(const PxcBpHandle s0, Compound* PX_RESTRICT c1, Cm::BitMap* PX_RESTRICT compoundCollBitmap, bool flag)
{
	PX_ASSERT(c1);

	const IntegerAABB elemBounds0 = mBPElems.getAABB(s0);
	void* PX_RESTRICT userData0 = mBPElems.getUserData(s0);
	const PxcBpHandle group0 = mBPElems.getGroup(s0);

	PxU32 elemId1 = c1->headID;
	for(PxU32 i = 0;i < c1->nbElems; i++)
	{
		if(mCompoundElems.getGroup(elemId1) != PX_INVALID_BP_HANDLE)
		{
			if(group0 != mCompoundElems.getGroup(elemId1))
			{
				const IntegerAABB& elemBounds1 = mCompoundElems.getAABB(elemId1);
				void* PX_RESTRICT userData1 = mCompoundElems.getUserData(elemId1);
				overlapTest(elemBounds0, elemBounds1, userData0, userData1, compoundCollBitmap, i, flag);
			}
		}
		else
		{
			resetBitmap(*compoundCollBitmap, mBitmasks, i);
			PX_ASSERT(!testBitmap(*compoundCollBitmap, mBitmasks, i));
		}
		elemId1 = mCompoundElems.getNextId(elemId1);
	}
	PX_ASSERT(PX_INVALID_BP_HANDLE==elemId1);
}

bool PxsAABBManager::bipartiteBoxPruning(const CompoundCache::ElementData* PX_RESTRICT data0,const PxU32* PX_RESTRICT ranks0, PxU32 nb0, const CompoundCache::ElementData* PX_RESTRICT data1,const PxU32* PX_RESTRICT ranks1, PxU32 nb1, CompoundCache::CompoundPairsArray& pairs)
{
	pairs.clear();
	// Checkings
	if(nb0 == 0 || nb1 == 0)
		return false;

	const PxU32* Sorted0 = ranks0; 
	const PxU32* Sorted1 = ranks1; 

	// 3) Prune the lists
	PxU32 Index0, Index1;

	const PxU32* const LastSorted0 = &Sorted0[nb0];
	const PxU32* const LastSorted1 = &Sorted1[nb1];
	const PxU32* RunningAddress0 = Sorted0;
	const PxU32* RunningAddress1 = Sorted1;

#define BPHANDLE_INTERSECT(bounds0,bounds1) \
	(bounds0[5] >= bounds1[2] && bounds1[5] >= bounds0[2] && bounds0[4] >= bounds1[1] && bounds1[4] >= bounds0[1])

	while(RunningAddress1<LastSorted1 && Sorted0<LastSorted0)
	{
		Index0 = *Sorted0++;

		while(RunningAddress1<LastSorted1 && data1[(*RunningAddress1)].bounds[0]<data0[Index0].bounds[0])	RunningAddress1++;

		const PxU32* RunningAddress2_1 = RunningAddress1;

		while(RunningAddress2_1<LastSorted1 && data1[Index1 = *RunningAddress2_1++].bounds[0]<=data0[Index0].bounds[3])
		{
			const PxcBPValType* ind0 = &data0[Index0].bounds[0];
			const PxcBPValType* ind1 = &data1[Index1].bounds[0];
			if(BPHANDLE_INTERSECT(ind0,ind1))
			{
				pairs.pushBack(Ps::to16(Index0));
				pairs.pushBack(Ps::to16(Index1));
			}
		}
	}

	////

	while(RunningAddress0<LastSorted0 && Sorted1<LastSorted1)
	{
		Index0 = *Sorted1++;

		while(RunningAddress0<LastSorted0 && data0[*RunningAddress0].bounds[0]<=data1[Index0].bounds[0])	RunningAddress0++;

		const PxU32* RunningAddress2_0 = RunningAddress0;

		while(RunningAddress2_0<LastSorted0 && data0[Index1 = *RunningAddress2_0++].bounds[0]<=data1[Index0].bounds[3])
		{
			const PxcBPValType* ind0 = &data0[Index1].bounds[0];
			const PxcBPValType* ind1 = &data1[Index0].bounds[0];

			if(BPHANDLE_INTERSECT(ind0,ind1))
			{
				pairs.pushBack(Ps::to16(Index1));
				pairs.pushBack(Ps::to16(Index0));
			}
		}
	}

#undef BPHANDLE_INTERSECT

	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool PxsAABBManager::completeBoxPruning(const CompoundCache::ElementData* PX_RESTRICT data,const PxU32* PX_RESTRICT ranks, PxU32 nb,CompoundCache::CompoundPairsArray& pairs)
{
	pairs.clear();

	// Checkings
	if(!nb)
		return false;
	
	const PxU32* Sorted = ranks;

#define BPHANDLE_INTERSECT(bounds0,bounds1) \
	(bounds0[5] >= bounds1[2] && bounds1[5] >= bounds0[2] && bounds0[4] >= bounds1[1] && bounds1[4] >= bounds0[1])

	// 3) Prune the list
	const PxU32* const LastSorted = &Sorted[nb];
	const PxU32* RunningAddress = Sorted;
	PxU32 Index0, Index1;
	while(RunningAddress<LastSorted && Sorted<LastSorted)
	{
		Index0 = *Sorted++;

		while(RunningAddress<LastSorted && data[*RunningAddress++].bounds[0]<data[Index0].bounds[0]);

		const PxU32* RunningAddress2 = RunningAddress;

		while(RunningAddress2<LastSorted && data[Index1 = *RunningAddress2++].bounds[0]<=data[Index0].bounds[3])
		{
			if(Index0!=Index1)
			{
				const PxcBPValType* ind0 = &data[Index0].bounds[0];
				const PxcBPValType* ind1 = &data[Index1].bounds[0];

				if(BPHANDLE_INTERSECT(ind0,ind1))
				{
					pairs.pushBack(Ps::to16(Index0));
					pairs.pushBack(Ps::to16(Index1));
				}
			}
		}
	}	

#undef BPHANDLE_INTERSECT
	return true;
}

///////////////////////////////////////////////////////////////////////////////

void PxsAABBManager::processCompoundPairs()
{
	// Process all active compound pairs to complete the number of normal created/deleted pairs
	// A compound pair can be (Single, Compound) or (Compound, Compound)

	for(PxU32 i = 0; i < mCompoundPairsSize; i++)
	{
		CompoundPair& p=mCompoundPairs[i];

		//Get the compound for each element of the pair (one might be a single).
		const PxcBpHandle bpElemId0=p.mBPElemId0;
		const PxcBpHandle bpElemId1=p.mBPElemId1;
		Compound* c0 = mBPElems.isSingle(bpElemId0) ? NULL : mCompoundManager.getCompound(mBPElems.getCompoundOwnerId(bpElemId0));
		Compound* c1 = mBPElems.isSingle(bpElemId1) ? NULL : mCompoundManager.getCompound(mBPElems.getCompoundOwnerId(bpElemId1));

		PX_ASSERT(c0 || c1);

		PX_ASSERT(true == mBPElems.getAABB(bpElemId0).intersects(mBPElems.getAABB(bpElemId1)));

		if(c0)
		{
			CompoundCache::CompoundData* c0Data = mCompoundCache.getCompoundData(mBPElems.getCompoundOwnerId(bpElemId0),c0->nbElems);

			if(c1)
			{
				CompoundCache::CompoundData* c1Data = mCompoundCache.getCompoundData(mBPElems.getCompoundOwnerId(bpElemId1),c1->nbElems);
				// Compound-compound
				if(!p.compoundCollBitmap)
				{
					p.compoundCollBitmap = PX_NEW(Cm::BitMap);
					p.compoundCollBitmap->clear(c0->nbElems*c1->nbElems);
				}
				
				if(c0Data && c1Data)
				{
					collideCompoundCompoundBipartite(c0,c0Data, c1,c1Data, mCompoundPairs[i].compoundCollBitmap);
				}
				else
				{
					collideCompoundCompound(c0,c1,p.compoundCollBitmap, true);
				}
			}
			else
			{
				// Compound-single
				if(!p.compoundCollBitmap)
				{
					p.compoundCollBitmap = PX_NEW(Cm::BitMap);
					p.compoundCollBitmap->clear(c0->nbElems);
				}

				if(c0Data)
				{
					collideSingleCompoundBipartite(bpElemId1, c0,c0Data, p.compoundCollBitmap);
				}
				else
				{
					collideSingleCompound(bpElemId1,c0,p.compoundCollBitmap, true);
				}
			}
		}
		else
		{
			if(c1)
			{
				CompoundCache::CompoundData* c1Data = mCompoundCache.getCompoundData(mBPElems.getCompoundOwnerId(bpElemId1),c1->nbElems);
				// Single-compound
				if(!p.compoundCollBitmap)
				{
					p.compoundCollBitmap = PX_NEW(Cm::BitMap);
					p.compoundCollBitmap->clear(c1->nbElems);
				}

				if(c1Data)
				{
					collideSingleCompoundBipartite(bpElemId0, c1,c1Data, p.compoundCollBitmap);
				}
				else
				{
					collideSingleCompound(bpElemId0,c1,p.compoundCollBitmap, true);
				}
			}
			else
			{
				// Single-single, shouldn't happen
				PX_ASSERT(0);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void PxsAABBManager::selfCollideCompoundBounds()
{
	const PxU32 N = mCompoundsUpdated.getElemsSize();
	const PxcBpHandle* updatedCompounds = mCompoundsUpdated.getElems();
	for(PxU32 i=0;i<N;i++)
	{
		const PxcBpHandle compoundId = updatedCompounds[i];
		Compound& compound = *mCompoundManager.getCompound(compoundId);

		CompoundCache::CompoundData* cData = mCompoundCache.getCompoundData(compoundId,compound.nbElems);
		if(compound.nbElems)
		{
			if(cData)
			{
				selfCollideCompoundBipartite(compound,cData);				
			}
			else
			{
				selfCollideCompound(compound);
			}

#ifdef PX_DEBUG
			gNbChanges = 0;
			selfCollideCompound(compound);
			PX_ASSERT(gNbChanges==0);
#endif
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

Cm::BitMap* PxsAABBManager::promoteBitmap(Cm::BitMap* bitmap, PxU32 nbX, PxU32 nbY, PxU32 newXIndex, PxU32 newYIndex) const
{
	if(!bitmap)
		return NULL;

	Cm::BitMap* newBitmap = PX_NEW(Cm::BitMap);
	const PxU32 newX = newXIndex != PX_INVALID_U32 ? nbX+1 : nbX;
	const PxU32 newY = newYIndex != PX_INVALID_U32 ? nbY+1 : nbY;
	const PxU32 bitSize = newX*newY;
	const PxU32 bitSize128 = (bitSize & 127) ?  ((bitSize + 128) & ~127) : bitSize;
	PX_ASSERT(bitSize128>=bitSize && (0==(bitSize128 & 127)));
	newBitmap->clear(bitSize128);

	PxU32 srcIndex = 0;
	PxU32 dstIndex = 0;
	for(PxU32 j=0;j<nbY;j++)
	{
		if(j==newYIndex)
			dstIndex += newX;
		for(PxU32 i=0;i<nbX;i++)
		{
			if(i==newXIndex)
				dstIndex++;
			if(testBitmap(*bitmap, mBitmasks, srcIndex++))
				setBitmap(*newBitmap, mBitmasks, dstIndex);
			dstIndex++;
		}
	}

	PX_DELETE(bitmap);
	return newBitmap;
}

void PxsAABBManager::promoteBitmap(PxU32* bitMapWords, PxU32 nbX, PxU32 nbY, PxU32 newXIndex, PxU32 newYIndex) const
{
	PxU32 newBitMapWords[MAX_COMPOUND_WORD_COUNT];
	PxMemSet(newBitMapWords, 0, sizeof(PxU32)*MAX_COMPOUND_WORD_COUNT);
	Cm::BitMap newBitMap;
	newBitMap.setWords(newBitMapWords,MAX_COMPOUND_WORD_COUNT);

	Cm::BitMap bitMap;
	bitMap.setWords(bitMapWords,MAX_COMPOUND_WORD_COUNT);

	const PxU32 newX = newXIndex != PX_INVALID_U32 ? nbX+1 : nbX;

	PxU32 srcIndex = 0;
	PxU32 dstIndex = 0;
	for(PxU32 j=0;j<nbY;j++)
	{
		if(j==newYIndex)
			dstIndex += newX;
		for(PxU32 i=0;i<nbX;i++)
		{
			if(i==newXIndex)
				dstIndex++;
			if(testBitmap(bitMap, mBitmasks, srcIndex++))
				setBitmap(newBitMap, mBitmasks, dstIndex);
			dstIndex++;
		}
	}

	PxMemCopy(bitMapWords, newBitMapWords, sizeof(PxU32)*MAX_COMPOUND_WORD_COUNT);
}


void PxsAABBManager::promoteBitmaps(Compound* compound)
{

	// 1) Promote self-CD bitmap
	promoteBitmap(compound->selfCollBitMapWords, compound->nbElems, compound->nbElems, 0, 0);


	// 2) Promote CD bitmaps
	PxU32 nb = mCompoundPairsSize;
	CompoundPair* PX_RESTRICT compoundPairs = mCompoundPairs;
	while(nb--)
	{
		CompoundPair& p = *compoundPairs++;

		// PT: the correct function could be looked up and stored inside "CompoundPair"...
		Compound* c0 = mBPElems.isSingle(p.mBPElemId0) ? NULL : mCompoundManager.getCompound(mBPElems.getCompoundOwnerId(p.mBPElemId0));
		Compound* c1 = mBPElems.isSingle(p.mBPElemId1) ? NULL : mCompoundManager.getCompound(mBPElems.getCompoundOwnerId(p.mBPElemId1));
						
		if(c0)
		{
			if(c1)
			{
				// Compound-compound
				if(c0==compound)
				{
					PX_ASSERT(p.compoundCollBitmap);
					PX_ASSERT(p.compoundCollBitmap->getWords());
					p.compoundCollBitmap = promoteBitmap(p.compoundCollBitmap, compound->nbElems, c1->nbElems, 0, PX_INVALID_U32);
				}
				else if(c1==compound)
				{
					PX_ASSERT(p.compoundCollBitmap);
					PX_ASSERT(p.compoundCollBitmap->getWords());
					p.compoundCollBitmap = promoteBitmap(p.compoundCollBitmap, c0->nbElems, compound->nbElems, PX_INVALID_U32, 0);
				}
			}
			else
			{
				// Compound-single
				if(c0==compound)
				{
					PX_ASSERT(p.compoundCollBitmap);
					PX_ASSERT(p.compoundCollBitmap->getWords());
					p.compoundCollBitmap = promoteBitmap(p.compoundCollBitmap, compound->nbElems, 1, 0, PX_INVALID_U32);
				}
			}
		}
		else
		{
			if(c1)
			{
				// Single-compound
				if(c1==compound)
				{
					PX_ASSERT(p.compoundCollBitmap);
					PX_ASSERT(p.compoundCollBitmap->getWords());
					p.compoundCollBitmap = promoteBitmap(p.compoundCollBitmap, compound->nbElems, 1, 0, PX_INVALID_U32);
				}
			}
			else
			{
				// Single-single, shouldn't happen
				PX_ASSERT(0);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void PxsAABBManager::shiftOrigin(const PxVec3& shift)
{
	const PxU32 compoundsCapacity = mCompoundManager.getCompoundsCapacity();
	for(PxU32 i=0; i < compoundsCapacity; i++)
	{
		const Compound* compound = mCompoundManager.getCompound(i);
		if(compound->nbElems)
		{
			IntegerAABB iaabb = mBPElems.getAABB(compound->bpElemId);
			iaabb.shift(shift);

			setBPElemVolumeBounds(compound->bpElemId, iaabb);

			PxU32 elem = compound->headID;
			while(PX_INVALID_BP_HANDLE != elem)
			{
				IntegerAABB iaabbElem = mCompoundElems.getAABB(elem);
				iaabbElem.shift(shift);

				mCompoundElems.setAABB(elem, iaabbElem);

				elem = mCompoundElems.getNextId(elem);
			}
		}
	}

	const PxU32 singleCapacity = mSingleManager.getCapacity();
	for(PxU32 i=0; i < singleCapacity; i++)
	{
		const Single* single = mSingleManager.getSingle(i);
		PxcBpHandle elem = single->headID;
		while(PX_INVALID_BP_HANDLE != elem)
		{
			IntegerAABB iaabb = mBPElems.getAABB(elem);
			iaabb.shift(shift);

			setBPElemVolumeBounds(elem, iaabb);

			elem = mBPElems.getNextId(elem);
		}
	}

	mBP->shiftOrigin(shift);
}
