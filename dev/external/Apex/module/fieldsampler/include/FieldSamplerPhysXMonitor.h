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

#ifndef __FIELD__SAMPLER_PHYSX_MONITOR_H___
#define __FIELD__SAMPLER_PHYSX_MONITOR_H___

#include "NxApex.h"
#include <PsArray.h>
#include <ApexMirroredArray.h>

#include "NiFieldSamplerScene.h"
#include "FieldSamplerPhysXMonitorParams.h"

#if NX_SDK_VERSION_MAJOR == 3
#include <PxScene.h>
#endif

namespace physx
{
namespace apex
{

class NiFieldSamplerQuery;
class NiFieldSamplerManager;

namespace fieldsampler
{

class FieldSamplerScene;

struct FsFilterData //: public UserAllocated
{
#if NX_SDK_VERSION_MAJOR == 3
	PxFilterData	pxData;
	PxU32			index;

	FsFilterData(const PxFilterData& d, PxU32 i = (PxU32)~0) : pxData(d), index(i) {}

	static bool sortPredicate (const FsFilterData& d1, const FsFilterData& d2)
	{
		if (d1.pxData.word3 != d2.pxData.word3) return d1.pxData.word3 < d2.pxData.word3;
		if (d1.pxData.word2 != d2.pxData.word2) return d1.pxData.word2 < d2.pxData.word2;
		if (d1.pxData.word1 != d2.pxData.word1) return d1.pxData.word1 < d2.pxData.word1;
		return d1.pxData.word0 < d2.pxData.word0;
	}
#endif
};


#if NX_SDK_VERSION_MAJOR == 3
PX_INLINE bool operator != (const FsFilterData& d1, const FsFilterData& d2)
{
	return d1.pxData.word0 != d2.pxData.word0 || d1.pxData.word1 != d2.pxData.word1 || d1.pxData.word2 != d2.pxData.word2 || d1.pxData.word3 != d2.pxData.word3;
}
#endif

class FieldSamplerPhysXMonitor : public UserAllocated
{
public:
	FieldSamplerPhysXMonitor(FieldSamplerScene& scene);
	virtual ~FieldSamplerPhysXMonitor();

#if NX_SDK_VERSION_MAJOR == 3
	virtual void	update() = 0;
	virtual void	updatePhysX();
	void	update(bool isDataOnDevice);

	/* PhysX scene management */
	void	setPhysXScene(PxScene* scene);
	PX_INLINE PxScene*	getPhysXScene() const
	{
		return mScene;
	}

	/* Toggle PhysX Monitor on/off */
	void enablePhysXMonitor(bool enable)
	{
		mEnable = enable;
	}

	/* Is PhysX Monitor enabled */
	bool isEnable()
	{
		return mEnable;
	}

	void addPhysXFilterData(physx::PxFilterData filterData);
	void removePhysXFilterData(physx::PxFilterData filterData);

#ifdef APEX_TEST
	bool setPhysXMonitorParticlesData(physx::PxU32 numParticles, physx::PxVec4** positions, physx::PxVec4** velocities);
	void getPhysXMonitorParticlesData(physx::PxVec4** velocities);

	physx::PxU32 mNumTestParticles;
	bool mIsFirstCopy;
	physx::ApexMirroredArray<physx::PxVec4> mTestPosition;
	physx::ApexMirroredArray<physx::PxVec4> mTestVelocity;
#endif

private:
	FieldSamplerPhysXMonitor& operator=(const FieldSamplerPhysXMonitor&);

	virtual void getParticles(physx::PxU32 taskId) = 0;
	void updateParticles();
	void getRigidBodies(physx::PxU32 taskId, bool isDataOnDevice);
	void updateRigidBodies();
	//void getCloth(physx::PxTask& task, bool isDataOnDevice);
	//void updateCloth();

protected:
	void commonInitArray();

	FieldSamplerScene*				mFieldSamplerScene;
	NiFieldSamplerManager*			mFieldSamplerManager;

	PxScene*						mScene;
	Array<PxParticleReadData*>		mParticleReadData;
	Array<PxF32>					mPSMass;
	Array<NiFieldSamplerQuery*>		mFieldSamplerQuery;	
	Array<physx::PxTaskID>	mFieldSamplerTaskID;

	ApexMirroredPlace::Enum			mApexMirroredPlace;

	FieldSamplerPhysXMonitorParams*	mParams;

	//Filter Data
	//typedef HashMap <PxFilterData, NiFieldSamplerQuery*> FDHashMap;
	Array<FsFilterData>		mFilterData;

	//Particles
	physx::PxU32					mNumPS;  //Number of particle systems
	physx::PxU32					mPCount; //Number of particles in buffer
	Array<PxActor*>					mParticleSystems;
	Array<PxU32>					mIndices;
	ApexMirroredArray<PxVec4>		mOutField;
	Array<PxVec3>					mOutVelocities;

	//Rigid bodies
	physx::PxU32					mNumRB; //Number of rigid bodies
	Array<PxActor*>					mRBActors;
	Array<FsFilterData>				mRBIndex;
	ApexMirroredArray<PxVec4>		mRBInPosition;
	ApexMirroredArray<PxVec4>		mRBInVelocities;
	ApexMirroredArray<PxVec4>		mRBOutField;

	//Enable or disable PhysX Monitor
	bool mEnable;

public:
	class RunAfterActorUpdateTask : public physx::PxTask
	{
	public:
		RunAfterActorUpdateTask(FieldSamplerPhysXMonitor& owner) : mOwner(owner) {}
		const char* getName() const
		{
			return FSST_PHYSX_MONITOR_UPDATE;
		}
		void run()
		{
			mOwner.updatePhysX();
		}

	protected:
		FieldSamplerPhysXMonitor& mOwner;

	private:
		RunAfterActorUpdateTask operator=(const RunAfterActorUpdateTask&);
	};
	RunAfterActorUpdateTask				mTaskRunAfterActorUpdate;
#endif
};

class FieldSamplerPhysXMonitorCPU : public FieldSamplerPhysXMonitor
{
public:
	FieldSamplerPhysXMonitorCPU(FieldSamplerScene& scene);
	virtual ~FieldSamplerPhysXMonitorCPU();
#if NX_SDK_VERSION_MAJOR == 3
	virtual void	getParticles(physx::PxU32 taskid);
	virtual void	update();
#endif

};

}
}
} // end namespace physx::apex


#if defined(APEX_CUDA_SUPPORT)

#include "PxGpuTask.h"

namespace physx
{
namespace apex
{
namespace fieldsampler
{

class FieldSamplerPhysXMonitorGPU : public FieldSamplerPhysXMonitor
{
public:
	FieldSamplerPhysXMonitorGPU(FieldSamplerScene& scene);
	virtual ~FieldSamplerPhysXMonitorGPU();
#if NX_SDK_VERSION_MAJOR == 3
	virtual void	getParticles(physx::PxU32 taskid);
	virtual void	update();
	virtual bool	launchBeforeActorUpdate(CUstream stream, int kernelIndex);
	virtual bool	launchAfterActorUpdate(CUstream stream, int kernelIndex);

	class LaunchBeforeActorUpdateTask : public physx::PxGpuTask
	{
	public:
		LaunchBeforeActorUpdateTask(FieldSamplerPhysXMonitorGPU& owner) : mOwner(owner) {}
		const char* getName() const
		{
			return "FieldSamplerPhysXMonitorGPU::LaunchBeforeActorUpdateTask";
		}
		void run()
		{
			PX_ALWAYS_ASSERT();
		}
		bool launchInstance(CUstream stream, int kernelIndex)
		{
			return mOwner.launchBeforeActorUpdate(stream, kernelIndex);
		}
		physx::PxGpuTaskHint::Enum getTaskHint() const
		{
			return physx::PxGpuTaskHint::Kernel;
		}

	protected:
		FieldSamplerPhysXMonitorGPU& mOwner;

	private:
		LaunchBeforeActorUpdateTask& operator=(const LaunchBeforeActorUpdateTask&);
	};
	LaunchBeforeActorUpdateTask		mTaskLaunchBeforeActorUpdate;

	class LaunchAfterActorUpdateGPUTask : public physx::PxGpuTask
	{
	public:
		LaunchAfterActorUpdateGPUTask(FieldSamplerPhysXMonitorGPU& owner) : mOwner(owner) {}
		const char* getName() const
		{
			return "FieldSamplerPhysXMonitorGPU::LaunchAfterActorUpdateGPUTask";
		}
		void run()
		{
			PX_ALWAYS_ASSERT();
		}
		bool launchInstance(CUstream stream, int kernelIndex)
		{
			return mOwner.launchAfterActorUpdate(stream, kernelIndex);
		}
		physx::PxGpuTaskHint::Enum getTaskHint() const
		{
			return physx::PxGpuTaskHint::Kernel;
		}
	protected:
		FieldSamplerPhysXMonitorGPU& mOwner;

	private:
		LaunchAfterActorUpdateGPUTask& operator=(const LaunchAfterActorUpdateGPUTask&);
	};
	LaunchAfterActorUpdateGPUTask	mGpuTaskLaunchAfterActorUpdate;

	physx::PxGpuCopyDescQueue mCopyQueueHtD;
	physx::PxGpuCopyDescQueue mCopyQueueDtH;
#endif
};

}
}
} // end namespace physx::apex

#endif //defined(APEX_CUDA_SUPPORT)

#endif

