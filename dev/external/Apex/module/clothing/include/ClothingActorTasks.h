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

#ifndef CLOTHING_ACTOR_TASKS_H
#define CLOTHING_ACTOR_TASKS_H

#include "PxTask.h"

#ifdef PX_PS3
#include "PxSpuTask.h"
#include "ClothingSpuPrograms.h"
#endif

namespace physx
{
namespace apex
{
namespace clothing
{

class ClothingActor;
class ClothingActorData;


class ClothingActorBeforeTickTask : public physx::PxLightCpuTask
{
public:
	ClothingActorBeforeTickTask(ClothingActor* actor) : mActor(actor), mDeltaTime(0.0f), mSubstepSize(0.0f), mNumSubSteps(0) {}

	PX_INLINE void setDeltaTime(PxF32 simulationDelta, PxF32 substepSize, PxU32 numSubSteps)
	{
		mDeltaTime = simulationDelta;
		mSubstepSize = substepSize;
		mNumSubSteps = numSubSteps;
	}

	virtual void        run();
	virtual const char* getName() const;

private:
	ClothingActor* mActor;
	PxF32 mDeltaTime;
	PxF32 mSubstepSize;
	PxU32 mNumSubSteps;
};



class ClothingActorDuringTickTask : public physx::PxTask
{
public:
	ClothingActorDuringTickTask(ClothingActor* actor) : mActor(actor) {}

	virtual void		run();
	virtual const char*	getName() const;

private:
	ClothingActor* mActor;
};



class ClothingActorFetchResultsTask : public physx::PxLightCpuTask
{
public:
	ClothingActorFetchResultsTask(ClothingActor* actor) : mActor(actor) {}

	virtual void		run();
	virtual const char*	getName() const;

private:
	ClothingActor* mActor;
};


#ifdef PX_PS3


class ClothingActorFetchResultsTaskSpu : public physx::PxSpuTask
{
public:
	ClothingActorFetchResultsTaskSpu() : PxSpuTask(gSpuClothingFetchResults.elfStart, gSpuClothingFetchResults.elfSize) {}

	virtual const char* getName() const;
};



class ClothingActorFetchResultsTaskSimpleSpu : public physx::PxSpuTask
{
public:
	ClothingActorFetchResultsTaskSimpleSpu() : PxSpuTask(gSpuClothingFetchResultsSimple.elfStart, gSpuClothingFetchResultsSimple.elfSize) {}

	virtual const char* getName() const;
};



class ClothingActorSkinPhysicsTaskSimpleSpu : public physx::PxSpuTask
{
public:
	ClothingActorSkinPhysicsTaskSimpleSpu() : PxSpuTask(gSpuClothingSkinPhysicsSimple.elfStart, gSpuClothingSkinPhysicsSimple.elfSize)
	{
	}

	virtual const char* getName() const;
};



class ClothingActorLockingTasks : public physx::PxLightCpuTask
{
public:
	ClothingActorLockingTasks(ClothingActor* actor) : mActor(actor) {}

	virtual void		run();
	virtual const char*	getName() const;

private:
	ClothingActor* mActor;
};

#endif // PX_PS3

}
} // namespace apex
} // namespace physx

#endif
