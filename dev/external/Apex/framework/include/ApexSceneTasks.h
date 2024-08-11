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

#ifndef APEX_SCENE_TASKS_H
#define APEX_SCENE_TASKS_H

#include "ApexScene.h"

#include "PsAllocator.h"

namespace physx
{
namespace apex
{

class LODComputeBenefitTask : public physx::PxTask, public UserAllocated
{
public:
	LODComputeBenefitTask(ApexScene& scene) : mScene(&scene) {}

	const char* getName() const;
	void run();

protected:
	ApexScene* mScene;
};



class PhysXSimulateTask : public physx::PxTask, public UserAllocated
{
public:
	PhysXSimulateTask(ApexScene& scene, CheckResultsTask& checkResultsTask); 
	~PhysXSimulateTask();
	
	const char* getName() const;
	void run();
	void setElapsedTime(PxF32 elapsedTime);
	void setFollowingTask(physx::PxBaseTask* following);

#if NX_SDK_VERSION_MAJOR == 3
	void setScratchBlock(void* scratchBlock, PxU32 size)
	{
		mScratchBlock = scratchBlock;
		mScratchBlockSize = size;
	}
#endif

protected:
	ApexScene* mScene;
	PxF32 mElapsedTime;

	physx::PxBaseTask* mFollowingTask;
	CheckResultsTask& mCheckResultsTask;

#if NX_SDK_VERSION_MAJOR == 3
	void*			mScratchBlock;
	PxU32			mScratchBlockSize;
#endif

private:
	PhysXSimulateTask& operator=(const PhysXSimulateTask&);
};



class CheckResultsTask : public physx::PxTask, public UserAllocated
{
public:
	CheckResultsTask(ApexScene& scene) : mScene(&scene) {}

	const char* getName() const;
	void run();

protected:
	ApexScene* mScene;
};



class FetchResultsTask : public physx::PxTask, public UserAllocated
{
public:
	FetchResultsTask(ApexScene& scene) 
	:	mScene(&scene)
	,	mFollowingTask(NULL)
	{}

	const char* getName() const;
	void run();

	/**
	* \brief Called by dispatcher after Task has been run.
	*
	* If you re-implement this method, you must call this base class
	* version before returning.
	*/
	void release();

	void setFollowingTask(physx::PxTask* following);

protected:
	ApexScene*					mScene;
	physx::PxBaseTask*	mFollowingTask;
};


/**  
*	This task is solely meant to record the duration of APEX's "during tick" tasks.
*	It could be removed and replaced with only the check results task if it is found
*	to be a performance issue.
*/
#if APEX_DURING_TICK_TIMING_FIX
class DuringTickCompleteTask : public physx::PxTask, public UserAllocated
{
public:
	DuringTickCompleteTask(ApexScene& scene) : mScene(&scene) {}

	const char* getName() const;
	void run();

protected:
	ApexScene* mScene;
};
#endif

/* This tasks loops all intermediate steps until the final fetchResults can be called */
class PhysXBetweenStepsTask : public physx::PxLightCpuTask, public UserAllocated
{
public:
	PhysXBetweenStepsTask(ApexScene& scene) : mScene(scene), mSubStepSize(0.0f),
		mNumSubSteps(0), mSubStepNumber(0), mLast(NULL) {}

	const char* getName() const;
	void run();
	void setSubstepSize(PxF32 substepSize, PxU32 numSubSteps);
	void setFollower(PxU32 substepNumber, physx::PxTask* last);

protected:
	ApexScene& mScene;
	PxF32 mSubStepSize;
	PxU32 mNumSubSteps;

	PxU32 mSubStepNumber;
	physx::PxTask* mLast;

private:
	PhysXBetweenStepsTask& operator=(const PhysXBetweenStepsTask&);
};

}
}

#endif
