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

#ifndef __BASIC_IOS_ACTOR_CPU_H__
#define __BASIC_IOS_ACTOR_CPU_H__

#include "NxApex.h"

#include "BasicIosActor.h"
#include "BasicIosAsset.h"
#include "NiInstancedObjectSimulation.h"
#include "BasicIosScene.h"
#include "ApexActor.h"
#include "ApexContext.h"
#include "ApexFIFO.h"

#include "PxTask.h"

namespace physx
{
namespace apex
{

class NxApexRenderVolume;

namespace basicios
{

class BasicIosActorCPU;

class BasicIosActorCPU : public BasicIosActor
{
public:
	BasicIosActorCPU(NxResourceList&, BasicIosAsset&, BasicIosScene&, physx::apex::NxIofxAsset&);
	~BasicIosActorCPU();

	virtual void						submitTasks();
	virtual void						setTaskDependencies();

#ifdef APEX_TEST
	virtual void						copyTestData() const;
#endif

private:
	/* Internal utility functions */
	void								simulateParticles();

	static const physx::PxU32 HISTOGRAM_BIN_COUNT = 1024;
	physx::PxU32						computeHistogram(physx::PxU32 dataCount, physx::PxF32 dataMin, physx::PxF32 dataMax, physx::PxU32& bound);

	/* particle data (output to the IOFX actors, and some state) */

	physx::Array<physx::PxU32>			mNewIndices;

	class SimulateTask : public physx::PxTask
	{
	public:
		SimulateTask(BasicIosActorCPU& actor) : mActor(actor) {}

		const char* getName() const
		{
			return "BasicIosActorCPU::SimulateTask";
		}
		void run()
		{
			mActor.simulateParticles();
		}

	protected:
		BasicIosActorCPU& mActor;

	private:
		SimulateTask& operator=(const SimulateTask&);
	};
	SimulateTask						mSimulateTask;

	ApexCpuInplaceStorage				mSimulationStorage;

	friend class BasicIosAsset;
};

}
}
} // namespace physx::apex

#endif // __BASIC_IOS_ACTOR_CPU_H__
