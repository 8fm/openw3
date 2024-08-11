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

#include "NxApexDefs.h"
#include "MinPhysxSdkVersion.h"

#if NX_SDK_VERSION_MAJOR == 3

#include "PsSort.h"
#include "FieldSamplerPhysXMonitor.h"
#include "NxApexReadWriteLock.h"
#include "FieldSamplerScene.h"
#include "NiFieldSamplerManager.h"
#include "NiFieldSamplerQuery.h"

#include "PxParticleDeviceExclusive.h"
#include "extensions/PxShapeExt.h"

#if defined(APEX_CUDA_SUPPORT)
#define CUDA_OBJ(name) SCENE_CUDA_OBJ(mFieldSamplerScene, name)
#endif

namespace physx
{
namespace apex
{
namespace fieldsampler
{

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

FieldSamplerPhysXMonitor::FieldSamplerPhysXMonitor(FieldSamplerScene& scene)
	: mFieldSamplerScene(&scene)
#ifdef APEX_TEST
	, mIsFirstCopy(true)
	, mTestPosition(	scene.getApexScene(), NV_ALLOC_INFO("mTestPosition", APEX))
	, mTestVelocity(	scene.getApexScene(), NV_ALLOC_INFO("mTestVelocities", APEX))
#endif
	, mNumPS(0)
	, mOutField(		scene.getApexScene(), NV_ALLOC_INFO("mOutField", APEX))
	, mNumRB(0)
	, mRBInPosition(	scene.getApexScene(), NV_ALLOC_INFO("mRBInPosition", APEX))
	, mRBInVelocities(	scene.getApexScene(), NV_ALLOC_INFO("mRBInVelocities", APEX))
	, mRBOutField(		scene.getApexScene(), NV_ALLOC_INFO("mRBOutField", APEX))
	, mEnable(true)
	, mTaskRunAfterActorUpdate(*this)
{
	mScene = mFieldSamplerScene->getModulePhysXScene();
	mParams = static_cast<FieldSamplerPhysXMonitorParams*>(NiGetApexSDK()->getParameterizedTraits()->createNxParameterized(FieldSamplerPhysXMonitorParams::staticClassName()));
	mFieldSamplerManager = mFieldSamplerScene->getManager();
	if (mFieldSamplerManager)
	{
		mFieldSamplerQuery.resize(mParams->maxPSCount);
		mFieldSamplerTaskID.resize(mParams->maxPSCount, ~0U);
	}
	mRBIndex.reserve(mParams->maxRBCount);
	mPSMass.resize(mParams->maxPSCount);
	mParticleReadData.resize(mParams->maxPSCount);
	mParticleSystems.resize(mParams->maxPSCount);
	mIndices.resize(mParams->maxParticleCount);
	for(PxU32 i = 0; i < mParams->maxParticleCount; i++)
	{
		mIndices[i] = i;
	}
}

void FieldSamplerPhysXMonitor::commonInitArray()
{
	mOutField.reserve(mParams->maxParticleCount, mApexMirroredPlace);
	mOutField.setSize(mParams->maxParticleCount);

	mOutVelocities.resize(mParams->maxParticleCount);

	mRBInPosition.reserve(mParams->maxRBCount, mApexMirroredPlace);
	mRBInVelocities.reserve(mParams->maxRBCount, mApexMirroredPlace);
	mRBOutField.reserve(mParams->maxRBCount, mApexMirroredPlace);
	
	mRBInPosition.setSize(mParams->maxRBCount);
	mRBInVelocities.setSize(mParams->maxRBCount);
	mRBOutField.setSize(mParams->maxRBCount);

#ifdef APEX_TEST
	mTestPosition.reserve(mParams->maxParticleCount, mApexMirroredPlace);
	mTestVelocity.reserve(mParams->maxParticleCount, mApexMirroredPlace);	
	mTestPosition.setSize(mParams->maxParticleCount);
	mTestVelocity.setSize(mParams->maxParticleCount);
#endif

	mRBActors.resize(mParams->maxRBCount);
}


FieldSamplerPhysXMonitor::~FieldSamplerPhysXMonitor()
{
	for (physx::PxU32 i = 0; i < mFieldSamplerQuery.size(); i++)
	{
		if (mFieldSamplerQuery[i])
		{
			mFieldSamplerQuery[i]->release();
		}
	}
	if(mParams)
	{
		mParams->destroy();
	}
}


void FieldSamplerPhysXMonitor::setPhysXScene(PxScene* scene)
{
	mScene = scene;
}


void FieldSamplerPhysXMonitor::addPhysXFilterData(physx::PxFilterData filterData)
{
	if(mFilterData.find(filterData) == mFilterData.end())
	{
		mFilterData.pushBack(FsFilterData(filterData, mFilterData.size()));

		NxGroupsMask64 groupMask; 
		groupMask.bits0 = filterData.word0;
		groupMask.bits1 = filterData.word1;
		NiFieldSamplerQueryDesc queryDesc;
		queryDesc.samplerFilterData = groupMask;
		queryDesc.maxCount = mParams->maxRBCount;
		mFieldSamplerQuery.pushBack(mFieldSamplerManager->createFieldSamplerQuery(queryDesc));
		mFieldSamplerTaskID.pushBack(~0U);
	}
}


void FieldSamplerPhysXMonitor::removePhysXFilterData(physx::PxFilterData filterData)
{
	PxU32 index = mFilterData.find(filterData)->index;
	mFilterData.replaceWithLast(index);
	index += mParams->maxPSCount;
	mFieldSamplerQuery[index]->release();
	mFieldSamplerQuery.replaceWithLast(index);
	mFieldSamplerTaskID.replaceWithLast(index);
	//mFilterData.findAndReplaceWithLast(filterData);
}


void FieldSamplerPhysXMonitorCPU::getParticles(physx::PxU32 taskId)
{
	SCOPED_PHYSX3_LOCK_READ(mFieldSamplerScene->getApexScene().getPhysXScene());
	physx::PxF32 deltaTime = mFieldSamplerScene->getApexScene().getPhysXSimulateTime();
	mPCount = 0;
	mNumPS = mScene->getActors(physx::PxActorTypeSelectionFlag::ePARTICLE_SYSTEM, &mParticleSystems[0], mParams->maxPSCount);
	for(PxU32 i = 0; i < mNumPS; i++)
	if (!mFieldSamplerManager->isUnhandledParticleSystem(mParticleSystems[i]))
	{
		if (mFieldSamplerQuery[i] == NULL)
		{
			NiFieldSamplerQueryDesc queryDesc;
			queryDesc.maxCount = mParams->maxParticleCount;
			mFieldSamplerQuery[i] = mFieldSamplerManager->createFieldSamplerQuery(queryDesc);
		}

		PxParticleSystem* particleSystem = DYNAMIC_CAST(PxParticleSystem*)((mParticleSystems[i]));
		mParticleReadData[i] = particleSystem->lockParticleReadData();
		physx::PxU32 numParticles;
		if (mParticleReadData[i])
		{
			numParticles = mParticleReadData[i]->validParticleRange;
			if(mPCount + numParticles >= mParams->maxParticleCount) break;
		
			NiFieldSamplerQueryData queryData;
			queryData.timeStep = deltaTime;
			queryData.count = numParticles;
			queryData.isDataOnDevice = false;
			
			//hack for PhysX particle stride calculation
			physx::PxStrideIterator<const physx::PxVec3> positionIt(mParticleReadData[i]->positionBuffer);
#ifdef WIN64
			queryData.strideBytes = (PxU32)(-(PxI64)&*positionIt + (PxI64)&*(++positionIt));
#else
			queryData.strideBytes = (PxU32)(-(PxI32)&*positionIt + (PxI32)&*(++positionIt));
#endif
			queryData.massStrideBytes = 0;
			queryData.pmaInPosition = (PxF32*)&*(mParticleReadData[i]->positionBuffer);
			queryData.pmaInVelocity = (PxF32*)&*(mParticleReadData[i]->velocityBuffer);
			mPSMass[i] = particleSystem->getParticleMass();
			queryData.pmaInMass = &mPSMass[i];
			queryData.pmaOutField = mOutField.getPtr() + mPCount;
			mFieldSamplerTaskID[i] = mFieldSamplerQuery[i]->submitFieldSamplerQuery(queryData, taskId);

			mPCount += numParticles;
		}
	}
}

#if defined(APEX_CUDA_SUPPORT)
void FieldSamplerPhysXMonitorGPU::getParticles(physx::PxU32 taskId)
{
	SCOPED_PHYSX_LOCK_READ(mFieldSamplerScene->getApexScene());
	physx::PxF32 deltaTime = mFieldSamplerScene->getApexScene().getPhysXSimulateTime();
	mPCount = 0;
	mNumPS = mScene->getActors(physx::PxActorTypeSelectionFlag::ePARTICLE_SYSTEM, &mParticleSystems[0], mParams->maxPSCount);
	for(PxU32 i = 0; i < mNumPS; i++)
	if (!mFieldSamplerManager->isUnhandledParticleSystem(mParticleSystems[i]))
	{
		if (mFieldSamplerQuery[i] == NULL) 
		{
			NiFieldSamplerQueryDesc queryDesc;
			queryDesc.maxCount = mParams->maxParticleCount;
			mFieldSamplerQuery[i] = mFieldSamplerManager->createFieldSamplerQuery(queryDesc);
		}

		PxParticleSystem* particleSystem = DYNAMIC_CAST(PxParticleSystem*)((mParticleSystems[i]));
		PxU32 numParticles = particleSystem->getMaxParticles();
			
		if(mPCount + numParticles >= mParams->maxParticleCount) break;
		
		PxCudaReadWriteParticleBuffers buffers;
		PxParticleDeviceExclusive::getReadWriteCudaBuffers(*mParticleSystems[i]->isParticleBase(), buffers);

		NiFieldSamplerQueryData queryData;
		queryData.timeStep = deltaTime;
		queryData.count = numParticles;
		queryData.isDataOnDevice = true;
		queryData.strideBytes = sizeof(PxVec4);
		queryData.massStrideBytes = 0;
		queryData.pmaInPosition = (PxF32*)buffers.positions;
		queryData.pmaInVelocity = (PxF32*)buffers.velocities;
		mPSMass[i] = particleSystem->getParticleMass();
		queryData.pmaInMass = &mPSMass[i];
		queryData.pmaOutField = mOutField.getGpuPtr() + mPCount;
		mFieldSamplerTaskID[i] = mFieldSamplerQuery[i]->submitFieldSamplerQuery(queryData, taskId);

		mPCount += numParticles;		
	}
}
#endif

void FieldSamplerPhysXMonitor::updateParticles()
{
	PxU32 pCount = 0;
	for(PxU32 i = 0; i < mNumPS; i++)
	if (!mFieldSamplerManager->isUnhandledParticleSystem(mParticleSystems[i]))
	{
		PxParticleSystem* particleSystem = DYNAMIC_CAST(PxParticleSystem*)((mParticleSystems[i]));
		PxU32 numParticles = PxMin(mParticleReadData[i]->validParticleRange, mParams->maxParticleCount);
		physx::PxStrideIterator<const physx::PxVec3> velocityIt(mParticleReadData[i]->velocityBuffer);

		for(PxU32 j = 0; j < numParticles; j++, ++velocityIt)
		{
			mOutVelocities[j] = *velocityIt + mOutField[pCount + j].getXYZ();
		}
		pCount += numParticles;
		// return ownership of the buffers back to the SDK
		mParticleReadData[i]->unlock();
	
		if(pCount <= mParams->maxParticleCount)
		{
			physx::PxStrideIterator<PxU32> indices(&mIndices[0]);
			physx::PxStrideIterator<PxVec3> outVelocities(&mOutVelocities[0]);
			particleSystem->setVelocities(numParticles, indices, outVelocities);
		}		
	}
}


void FieldSamplerPhysXMonitor::getRigidBodies(physx::PxU32 taskId, bool isDataOnDevice)
{
	SCOPED_PHYSX_LOCK_READ(mFieldSamplerScene->getApexScene());
	physx::PxF32 deltaTime = mFieldSamplerScene->getApexScene().getPhysXSimulateTime();

	NiFieldSamplerQueryData queryData;
	queryData.timeStep = deltaTime;

	PxU32 rbCount = mScene->getActors(physx::PxActorTypeSelectionFlag::eRIGID_DYNAMIC, &mRBActors[0], mParams->maxRBCount);
	Array<PxShape*> shapes;
	mNumRB = 0;
	mRBIndex.clear();
	for (PxU32 i = 0; i < rbCount; i++)
	{
		physx::PxRigidDynamic* rb = (physx::PxRigidDynamic*)mRBActors[i];
		if (rb->getRigidDynamicFlags() == PxRigidDynamicFlag::eKINEMATIC) 
		{
			continue;
		}
		const physx::PxVec3& cmPos = rb->getGlobalPose().p;
		const physx::PxVec3& velocity = rb->getLinearVelocity();
		const physx::PxVec3& rotation = rb->getAngularVelocity();
		physx::PxF32 mass = rb->getMass();

		const PxU32 numShapes = rb->getNbShapes();
		shapes.resize(numShapes);
		if (numShapes == 0) 
		{
			continue;
		}
		rb->getShapes(&shapes[0], numShapes);
		for (PxU32 j = 0; j < numShapes && mNumRB < mParams->maxRBCount; j++)
		{
			PxFilterData filterData = shapes[j]->getQueryFilterData(); //>getSimulationFilterData();
			if (mFilterData.find(filterData) != mFilterData.end())
			{
				mRBIndex.pushBack(FsFilterData(filterData, i));
				//PxVec3 pos = shapes[j]->getWorldBounds().getCenter();
				PxVec3 pos = PxShapeExt::getWorldBounds(*shapes[j], *rb).getCenter();
				PxVec3 vel = velocity + rotation.cross(pos - cmPos);

				mRBInPosition[mNumRB] = physx::PxVec4(pos, mass / numShapes);
				mRBInVelocities[mNumRB] = physx::PxVec4(vel, 0.f);
				++mNumRB;
			}
		}		
	}

	if (mNumRB == 0) 
	{
		return;
	}

	sort(&mRBIndex[0], mNumRB, FsFilterData::sortPredicate);


	for (PxU32 i = mNumPS; i < mFieldSamplerTaskID.size(); i++)
	{
		mFieldSamplerTaskID[i] = ~0U;
	}

	PxFilterData current(mRBIndex[0].pxData);
	PxU32 count = 0;
	PxU32 displacement = 0;
	for (PxU32 i = 0; i <= mNumRB; i++)
	{
		if (i == mNumRB || current != mRBIndex[i].pxData)
		{
			queryData.count = count;
			queryData.isDataOnDevice = isDataOnDevice;
			queryData.massStrideBytes = sizeof(PxVec4);
			queryData.strideBytes = sizeof(PxVec4);			
			if (isDataOnDevice)
			{
#if defined(APEX_CUDA_SUPPORT)
				queryData.pmaInPosition = (PxF32*)(mRBInPosition.getGpuPtr() + displacement);
				queryData.pmaInVelocity = (PxF32*)(mRBInVelocities.getGpuPtr() + displacement);
				queryData.pmaInMass = &(mRBInPosition.getGpuPtr() + displacement)->w;
				queryData.pmaOutField = mRBOutField.getGpuPtr() + displacement;
#endif
			}
			else
			{
				queryData.pmaInPosition = (PxF32*)(mRBInPosition.getPtr() + displacement);
				queryData.pmaInVelocity = (PxF32*)(mRBInVelocities.getPtr() + displacement);
				queryData.pmaInMass = &(mRBInPosition.getPtr() + displacement)->w;
				queryData.pmaOutField = mRBOutField.getPtr() + displacement;
			}
	
			PxU32 index = mFilterData.find(current)->index + mParams->maxPSCount;;
			mFieldSamplerTaskID[index] = mFieldSamplerQuery[index]->submitFieldSamplerQuery(queryData, taskId);
			
			displacement += count;
			if (i != mNumRB) 
			{
				current = mRBIndex[i].pxData;
				count = 1;
			}
		}
		else
		{
			count++;
		}
	}
}

void FieldSamplerPhysXMonitor::updateRigidBodies()
{
	SCOPED_PHYSX_LOCK_WRITE(mFieldSamplerScene->getApexScene());

	for(PxU32 i = 0; i < mNumRB; i++)
	{
		physx::PxRigidDynamic* rb = (physx::PxRigidDynamic*)mRBActors[mRBIndex[i].index];
		physx::PxVec3 resVelocity = rb->getLinearVelocity() + mRBOutField[mRBIndex[i].index].getXYZ();
		physx::PxVec3 resRotation = rb->getAngularVelocity() + mRBOutField[mRBIndex[i].index].getXYZ().cross(mRBInPosition[mRBIndex[i].index].getXYZ() - rb->getGlobalPose().p);
		rb->setLinearVelocity(resVelocity);
		rb->setAngularVelocity(resRotation);
	}
}


void FieldSamplerPhysXMonitor::update(bool isDataOnDevice)
{	
	PX_UNUSED(isDataOnDevice);
	physx::PxTaskManager* tm = mFieldSamplerScene->getApexScene().getTaskManager();
	physx::PxU32 taskId = tm->getNamedTask(FSST_PHYSX_MONITOR_LOAD);
	if(mScene)
	{
		getParticles(taskId);

		getRigidBodies(taskId, isDataOnDevice);

		//getCloth(task, isDataOnDevice);
	}
	if(mNumPS > 0 || mNumRB > 0)
	{	
		tm->submitNamedTask(&mTaskRunAfterActorUpdate, FSST_PHYSX_MONITOR_UPDATE);
		mTaskRunAfterActorUpdate.startAfter(tm->getNamedTask(FSST_PHYSX_MONITOR_FETCH));
		mTaskRunAfterActorUpdate.finishBefore(tm->getNamedTask(AST_PHYSX_SIMULATE));
	}	
}

void FieldSamplerPhysXMonitor::updatePhysX()
{
	if (mApexMirroredPlace == physx::apex::ApexMirroredPlace::CPU)
	{
		updateParticles();
	}
	updateRigidBodies();
}

FieldSamplerPhysXMonitorCPU::FieldSamplerPhysXMonitorCPU(fieldsampler::FieldSamplerScene& scene)
	: FieldSamplerPhysXMonitor(scene)	
{
	mApexMirroredPlace = physx::apex::ApexMirroredPlace::CPU;

	commonInitArray();
}

FieldSamplerPhysXMonitorCPU::~FieldSamplerPhysXMonitorCPU()
{
}

void FieldSamplerPhysXMonitorCPU::update()
{
	FieldSamplerPhysXMonitor::update(false);
}

#ifdef APEX_TEST

bool FieldSamplerPhysXMonitor::setPhysXMonitorParticlesData(physx::PxU32 numParticles, physx::PxVec4** positions, physx::PxVec4** velocities)
{
	mNumTestParticles = numParticles;
	*positions = mTestPosition.getPtr();
	*velocities = mTestVelocity.getPtr();
	return true;
}

void FieldSamplerPhysXMonitor::getPhysXMonitorParticlesData(physx::PxVec4** velocities)
{
	*velocities = mOutField.getPtr();
}

#endif

#if defined(APEX_CUDA_SUPPORT)

FieldSamplerPhysXMonitorGPU::FieldSamplerPhysXMonitorGPU(fieldsampler::FieldSamplerScene& scene)
	: FieldSamplerPhysXMonitor(scene)
	, mTaskLaunchBeforeActorUpdate(*this)
	, mGpuTaskLaunchAfterActorUpdate(*this)
	, mCopyQueueHtD(*scene.getApexScene().getTaskManager()->getGpuDispatcher())
	, mCopyQueueDtH(*scene.getApexScene().getTaskManager()->getGpuDispatcher())
{
	mApexMirroredPlace = physx::apex::ApexMirroredPlace::CPU_GPU;

	commonInitArray();
}

FieldSamplerPhysXMonitorGPU::~FieldSamplerPhysXMonitorGPU()
{
}

void FieldSamplerPhysXMonitorGPU::update()
{
	FieldSamplerPhysXMonitor::update(true);

	if(mNumPS > 0 || mNumRB > 0)
	{
		physx::PxTaskManager* tm = mFieldSamplerScene->getApexScene().getTaskManager();
		tm->submitUnnamedTask(mTaskLaunchBeforeActorUpdate, physx::PxTaskType::TT_GPU);

		// Just in case one of the below conditions doesn't set a bounding dependency, let's not let it dangle
		mTaskLaunchBeforeActorUpdate.finishBefore(tm->getNamedTask(AST_PHYSX_SIMULATE));
		for (physx::PxU32 i = 0; i < mNumPS; i++)
		{
			if (mNumPS > 0 && !mFieldSamplerManager->isUnhandledParticleSystem(mParticleSystems[i]))
			{
				mTaskLaunchBeforeActorUpdate.finishBefore(mFieldSamplerTaskID[i]);
			}
		}
		for (physx::PxU32 i = mNumPS; i < mFieldSamplerTaskID.size(); i++)
		{
			if (mFieldSamplerTaskID[i] != ~0U)
			{
				mTaskLaunchBeforeActorUpdate.finishBefore(mFieldSamplerTaskID[i]);
			}
		}

		tm->submitUnnamedTask(mGpuTaskLaunchAfterActorUpdate, physx::PxTaskType::TT_GPU);
		mGpuTaskLaunchAfterActorUpdate.startAfter(tm->getNamedTask(FSST_PHYSX_MONITOR_FETCH));
		mGpuTaskLaunchAfterActorUpdate.finishBefore(mTaskRunAfterActorUpdate.getTaskID());
	}
}

bool FieldSamplerPhysXMonitorGPU::launchBeforeActorUpdate(CUstream stream, int kernelIndex)
{
	PX_UNUSED(stream);
	PX_UNUSED(kernelIndex);

	switch(kernelIndex)
	{
	case 0:
		mRBInPosition.copyHostToDeviceQ(mCopyQueueHtD, mParams->maxRBCount);
		mRBInVelocities.copyHostToDeviceQ(mCopyQueueHtD, mParams->maxRBCount);
#ifdef APEX_TEST
		mOutField.copyHostToDeviceQ(mCopyQueueHtD, mParams->maxParticleCount);
		if(mIsFirstCopy)
		{
			mTestPosition.copyHostToDeviceQ(mCopyQueueHtD, mParams->maxParticleCount);
			mTestVelocity.copyHostToDeviceQ(mCopyQueueHtD, mParams->maxParticleCount);
		}
#endif
		mCopyQueueHtD.flushEnqueued();
		return true;
#ifdef APEX_TEST
	case 1:
		if(mIsFirstCopy && mNumPS)
		{
			PxCudaReadWriteParticleBuffers buffers;
			PxParticleBase* pb = mParticleSystems[0]->isParticleBase();
			PxParticleDeviceExclusive::getReadWriteCudaBuffers(*pb, buffers);
			CUDA_OBJ(testParticleKernel)(stream, mNumTestParticles, 
									(float4*)buffers.positions, 
									(float4*)buffers.velocities,
									(physx::PxU32*)buffers.flags,
									(float4*)mTestPosition.getGpuPtr(),
									(float4*)mTestVelocity.getGpuPtr());
			mIsFirstCopy = false;
			return false;
		}
#endif
	default:
		return false;
	}
}

bool FieldSamplerPhysXMonitorGPU::launchAfterActorUpdate(CUstream stream, int kernelIndex)
{
	switch(kernelIndex)
	{
	case 0:
#ifdef APEX_TEST
		mOutField.copyDeviceToHostQ(mCopyQueueDtH, mParams->maxParticleCount);
#endif
		mRBOutField.copyDeviceToHostQ(mCopyQueueDtH, mParams->maxRBCount);
		mCopyQueueDtH.flushEnqueued();
		return true;
	default:
		if((PxU32)kernelIndex <= mNumPS)
		{
			SCOPED_PHYSX_LOCK_READ(mFieldSamplerScene->getApexScene());
			PxCudaReadWriteParticleBuffers buffers;
			PxParticleBase* pb = mParticleSystems[kernelIndex - 1]->isParticleBase();
			PxParticleDeviceExclusive::getReadWriteCudaBuffers(*pb, buffers);
			if(pb->getMaxParticles() > 0 && !mFieldSamplerManager->isUnhandledParticleSystem(mParticleSystems[kernelIndex - 1]))
			{
				CUDA_OBJ(applyParticlesKernel)(stream, pb->getMaxParticles(), (float4*)buffers.velocities, (float4*)mOutField.getGpuPtr());
			}
			return true;
		}
		else
		{
			mFieldSamplerScene->getApexScene().getTaskManager()->getGpuDispatcher()->addCompletionPrereq(mTaskRunAfterActorUpdate);
			return false;
		}
	}
}

#endif //defined(APEX_CUDA_SUPPORT)

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

