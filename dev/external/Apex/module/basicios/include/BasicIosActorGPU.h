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

#ifndef __BASIC_IOS_ACTOR_GPU_H__
#define __BASIC_IOS_ACTOR_GPU_H__

#include "NxApex.h"

#include "BasicIosActor.h"
#include "BasicIosAsset.h"
#include "NiInstancedObjectSimulation.h"
#include "BasicIosScene.h"
#include "ApexActor.h"
#include "ApexContext.h"
#include "ApexFIFO.h"
#include "NiFieldSamplerQuery.h"

#include "PxGpuTask.h"

namespace physx
{
namespace apex
{

namespace IOFX
{

class NxIofxActor;
class NxApexRenderVolume;

}

namespace basicios
{

class BasicIosActorGPU;

class BasicIosActorGPU : public BasicIosActor
{
public:
	BasicIosActorGPU(NxResourceList&, BasicIosAsset&, BasicIosScene&, physx::apex::NxIofxAsset&);
	~BasicIosActorGPU();

	virtual void						submitTasks();
	virtual void						setTaskDependencies();
	virtual void						fetchResults();

#ifdef APEX_TEST
	virtual void						copyTestData() const;
#endif

private:
	bool								launch(CUstream stream, int kernelIndex);

	physx::PxGpuCopyDescQueue		mCopyQueue;

	ApexMirroredArray<physx::PxU32>		mHoleScanSum;
	ApexMirroredArray<physx::PxU32>		mMoveIndices;

	ApexMirroredArray<physx::PxU32>		mTmpReduce;
	ApexMirroredArray<physx::PxU32>		mTmpHistogram;
	ApexMirroredArray<physx::PxU32>		mTmpScan;
	ApexMirroredArray<physx::PxU32>		mTmpScan1;

	ApexMirroredArray<physx::PxU32>		mTmpOutput;
	ApexMirroredArray<physx::PxU32>		mTmpOutput1;

	class LaunchTask : public physx::PxGpuTask
	{
	public:
		LaunchTask(BasicIosActorGPU& actor) : mActor(actor) {}
		const char* getName() const
		{
			return "BasicIosActorGPU::LaunchTask";
		}
		void         run()
		{
			PX_ALWAYS_ASSERT();
		}
		bool         launchInstance(CUstream stream, int kernelIndex)
		{
			return mActor.launch(stream, kernelIndex);
		}
		physx::PxGpuTaskHint::Enum getTaskHint() const
		{
			return physx::PxGpuTaskHint::Kernel;
		}

	protected:
		BasicIosActorGPU& mActor;

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

#endif // __BASIC_IOS_ACTOR_GPU_H__
