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

#ifndef __PARTICLE_IOS_ACTOR_GPU_H__
#define __PARTICLE_IOS_ACTOR_GPU_H__

#include "NxApex.h"

#include "ParticleIosActor.h"
#include "ParticleIosAsset.h"
#include "NiInstancedObjectSimulation.h"
#include "ParticleIosScene.h"
#include "ApexActor.h"
#include "ApexContext.h"
#include "ApexFIFO.h"
#include "NiFieldSamplerQuery.h"

#include "PxGpuTask.h"

namespace physx
{
namespace apex
{

namespace iofx
{
class NxIofxActor;
class NxApexRenderVolume;
}
	
namespace pxparticleios
{

class ParticleIosActorGPU;

class ParticleIosActorGPU : public ParticleIosActor
{
public:
	ParticleIosActorGPU(NxResourceList&, ParticleIosAsset&, ParticleIosScene&, NxIofxAsset&);
	~ParticleIosActorGPU();

	virtual physx::PxTaskID		submitTasks(physx::PxTaskManager* tm, physx::PxTaskID taskFinishBeforeID);
	virtual void						setTaskDependencies();
	virtual void						fetchResults();

private:
	bool								launch(CUstream stream, int kernelIndex);

	physx::PxGpuCopyDescQueue		mCopyQueue;

	ApexMirroredArray<physx::PxU32>		mHoleScanSum;
	ApexMirroredArray<physx::PxU32>		mMoveIndices;

	ApexMirroredArray<physx::PxU32>		mTmpReduce;
	ApexMirroredArray<physx::PxU32>		mTmpHistogram;
	ApexMirroredArray<physx::PxU32>		mTmpScan;
	ApexMirroredArray<physx::PxU32>		mTmpScan1;

	ApexMirroredArray<physx::PxU32>		mTmpOutput;	// 0:STATUS_LASTACTIVECOUNT, ...
	ApexMirroredArray<physx::PxU32>		mTmpBoundParams;	// min, max
#if defined(APEX_TEST)
	ApexMirroredArray<physx::PxU32>		mTestMirroredArray;

	ApexCudaConstMemGroup				mTestConstMemGroup;
	InplaceHandle<int>					mTestITHandle;
#endif

	class LaunchTask : public physx::PxGpuTask
	{
	public:
		LaunchTask(ParticleIosActorGPU& actor) : mActor(actor) {}
		const char*	getName() const
		{
			return "ParticleIosActorGPU::LaunchTask";
		}
		void		run()
		{
			PX_ALWAYS_ASSERT();
		}
		bool		launchInstance(CUstream stream, int kernelIndex)
		{
			return mActor.launch(stream, kernelIndex);
		}
		physx::PxGpuTaskHint::Enum getTaskHint() const
		{
			return physx::PxGpuTaskHint::Kernel;
		}

	protected:
		ParticleIosActorGPU& mActor;

	private:
		LaunchTask& operator=(const LaunchTask&);
	};

	static PX_CUDA_CALLABLE PX_INLINE PxMat44 inverse(const PxMat44& in);
	static PxReal distance(PxVec4 a, PxVec4 b);

	LaunchTask							mLaunchTask;
};

}
}
} // namespace physx::apex

#endif // __PARTICLE_IOS_ACTOR_GPU_H__
