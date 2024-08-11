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

#ifndef __PARTICLE_IOS_ACTOR_CPU_H__
#define __PARTICLE_IOS_ACTOR_CPU_H__

#include "NxApex.h"

#include "ParticleIosActor.h"
#include "ParticleIosAsset.h"
#include "NiInstancedObjectSimulation.h"
#include "ParticleIosScene.h"
#include "ApexActor.h"
#include "ApexContext.h"
#include "ApexFIFO.h"

#include "PxTask.h"

namespace physx
{
namespace apex
{

namespace iofx
{
class NxApexRenderVolume;
class NxIofxAsset;
}

namespace pxparticleios
{

class ParticleIosActorCPU : public ParticleIosActor
{
public:
	ParticleIosActorCPU(NxResourceList&, ParticleIosAsset&, ParticleIosScene&, NxIofxAsset&);
	~ParticleIosActorCPU();

	virtual physx::PxTaskID		submitTasks(physx::PxTaskManager* tm, physx::PxTaskID taskFinishBeforeID);
	virtual void						setTaskDependencies();

private:
	/* Internal utility functions */
	void								simulateParticles();

	static const physx::PxU32 HISTOGRAM_BIN_COUNT = 1024;
	physx::PxU32						computeHistogram(physx::PxU32 dataCount, physx::PxF32 dataMin, physx::PxF32 dataMax, physx::PxU32& bound);

	/* particle data (output to the IOFX actors, and some state) */

	struct NewParticleData
	{
		PxU32  destIndex;
		PxVec3 position;
		PxVec3 velocity;
	};
	physx::Array<physx::PxU32>			mNewIndices;
	physx::Array<physx::PxU32>			mRemovedParticleList;
	physx::Array<NewParticleData>		mAddedParticleList;
	PxParticleExt::IndexPool*			mIndexPool;

	/* Field sampler update velocity */
	physx::Array<physx::PxU32>			mUpdateIndexBuffer;
	physx::Array<physx::PxVec3>			mUpdateVelocityBuffer;

	class SimulateTask : public physx::PxTask
	{
	public:
		SimulateTask(ParticleIosActorCPU& actor) : mActor(actor) {}

		const char* getName() const
		{
			return "ParticleIosActorCPU::SimulateTask";
		}
		void run()
		{
			mActor.simulateParticles();
		}

	protected:
		ParticleIosActorCPU& mActor;

	private:
		SimulateTask& operator=(const SimulateTask&);
	};
	SimulateTask						mSimulateTask;

	ApexCpuInplaceStorage				mSimulationStorage;

	friend class ParticleIosAsset;
};

}
}
} // namespace physx::apex

#endif // __PARTICLE_IOS_ACTOR_CPU_H__
