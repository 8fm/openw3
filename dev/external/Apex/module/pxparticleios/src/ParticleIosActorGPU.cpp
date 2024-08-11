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
#if NX_SDK_VERSION_MAJOR == 3
#if defined(APEX_CUDA_SUPPORT)

#include "NxApex.h"
#include "NiApexScene.h"
#include "NiApexSDK.h"
#include "NxApexReadWriteLock.h"
#include "NxParticleIosActor.h"
#include "ParticleIosActorGPU.h"
#include "ParticleIosAsset.h"
#include "NxIofxAsset.h"
#include "NxIofxActor.h"
#include "ModuleParticleIos.h"
#include "ParticleIosScene.h"
#include "NiApexRenderDebug.h"
#include "NiApexAuthorableObject.h"

#include "foundation/PxMath.h"

//CUDA
#include "PxGpuTask.h"
#include "ApexCutil.h"

#define CUDA_OBJ(name) SCENE_CUDA_OBJ(*mParticleIosScene, name)

#include "PxParticleBase.h"
#include "PxParticleSystem.h"
#include "PxParticleDeviceExclusive.h"

namespace physx
{
namespace apex
{
namespace pxparticleios
{

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

ParticleIosActorGPU::ParticleIosActorGPU(
    NxResourceList& list,
    ParticleIosAsset& asset,
    ParticleIosScene& scene,
	NxIofxAsset& iofxAsset)
	: ParticleIosActor(list, asset, scene, iofxAsset, true)
	, mCopyQueue(*scene.getApexScene().getTaskManager()->getGpuDispatcher())
	, mHoleScanSum(scene.getApexScene(), NV_ALLOC_INFO("mHoleScanSum", PARTICLES))
	, mMoveIndices(scene.getApexScene(), NV_ALLOC_INFO("mMoveIndices", PARTICLES))
	, mTmpReduce(scene.getApexScene(), NV_ALLOC_INFO("mTmpReduce", PARTICLES))
	, mTmpHistogram(scene.getApexScene(), NV_ALLOC_INFO("mTmpHistogram", PARTICLES))
	, mTmpScan(scene.getApexScene(), NV_ALLOC_INFO("mTmpScan", PARTICLES))
	, mTmpScan1(scene.getApexScene(), NV_ALLOC_INFO("mTmpScan1", PARTICLES))
	, mTmpOutput(scene.getApexScene(), NV_ALLOC_INFO("mTmpOutput", PARTICLES))
	, mTmpBoundParams(scene.getApexScene(), NV_ALLOC_INFO("mTmpBoundParams", PARTICLES))
	, mLaunchTask(*this)
#if defined(APEX_TEST)
	, mTestMirroredArray(scene.getApexScene(), NV_ALLOC_INFO("mTestMirroredArray", PARTICLES))
	, mTestConstMemGroup(CUDA_OBJ(simulateConstMem))
#endif
{
	initStorageGroups(CUDA_OBJ(simulateConstMem));

#if defined(APEX_TEST)
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mTestConstMemGroup)
		mTestITHandle;
		int* pi = mTestITHandle.alloc(_storage_);
		*pi = 2;
	}
#endif

	const ApexMirroredPlace::Enum defaultPlace = ApexMirroredPlace::GPU;
	
	mTmpOutput.setSize(4, ApexMirroredPlace::CPU_GPU);
	mTmpBoundParams.setSize(2, defaultPlace);

	const unsigned int ScanWarpsPerBlock = CUDA_OBJ(scanKernel).getBlockDim().x / WARP_SIZE;
	physx::PxCudaContextManager* ctxMgr = mParticleIosScene->getApexScene().getTaskManager()->getGpuDispatcher()->getCudaContextManager();
	int mMPCount = ctxMgr->getMultiprocessorCount();
	mTmpReduce.reserve(WARP_SIZE * 5, defaultPlace);
	mTmpHistogram.reserve(mMPCount * HISTOGRAM_SIMULATE_BIN_COUNT, defaultPlace);
	mTmpScan.reserve(mMPCount * ScanWarpsPerBlock, defaultPlace);
	mTmpScan1.reserve(mMPCount * ScanWarpsPerBlock, defaultPlace);
	
	mField.reserve(mMaxParticleCount, defaultPlace);
	mLifeTime.reserve(mMaxParticleCount, defaultPlace);

	mLifeSpan.reserve(mMaxTotalParticleCount, ApexMirroredPlace::CPU_GPU);
	mInjector.reserve(mMaxTotalParticleCount, ApexMirroredPlace::CPU_GPU);
	mBenefit.reserve(mMaxTotalParticleCount, ApexMirroredPlace::CPU_GPU);

	{
		PxU32 size = mGridDensityParams.GridResolution;
		if(size > 0)
		{
			mGridDensityGrid.setSize(size*size*size,ApexMirroredPlace::GPU);
			mGridDensityGridLowPass.setSize(size*size*size,ApexMirroredPlace::GPU);
		}
	}

	mHoleScanSum.reserve(mMaxTotalParticleCount, defaultPlace);
	mMoveIndices.reserve(mMaxTotalParticleCount, defaultPlace);
#if defined(APEX_TEST)
	mTestMirroredArray.reserve(16, ApexMirroredPlace::CPU_GPU);
	mTestMirroredArray.setSize(16);
	for(PxU32 i = 0; i<16; i++)
		mTestMirroredArray.getPtr()[i] = i;
#endif
}

ParticleIosActorGPU::~ParticleIosActorGPU()
{
}

physx::PxTaskID ParticleIosActorGPU::submitTasks(physx::PxTaskManager* tm, physx::PxTaskID taskFinishBeforeID)
{
	ParticleIosActor::submitTasks(tm, taskFinishBeforeID);
	mInjectorsCounters.setSize(mInjectorList.getSize(), ApexMirroredPlace::CPU_GPU); 

	physx::PxTaskID	taskID	= tm->submitUnnamedTask(mLaunchTask, physx::PxTaskType::TT_GPU);
	physx::PxTask*	task	= tm->getTaskFromID(taskID);

	if (taskFinishBeforeID != 0)
	{
		task->finishBefore(taskFinishBeforeID);
	}

	SCOPED_PHYSX3_LOCK_WRITE(mParticleIosScene->getModulePhysXScene());
	/* This call may need to be moved to a task that runs between LOD and PhysX::Simulate
	 * if this GPU task is ever allowed to overlap with the PhysX simulation step
	 */
	PxParticleDeviceExclusive::setValidParticleRange(*mParticleActor->isParticleBase(), mParticleCount);

	return taskID;
}

void ParticleIosActorGPU::setTaskDependencies()
{
	ParticleIosActor::setTaskDependencies(mLaunchTask, true);

	physx::PxTaskManager* tm = mParticleIosScene->getApexScene().getTaskManager();
	if (tm->getGpuDispatcher()->getCudaContextManager()->supportsArchSM20())
	{
		/* For Fermi devices, it pays to launch all IOS together.  This also forces
		 * The IOFX managers to step at the same time.
		 */
		PxTaskID interlock = tm->getNamedTask("IOS::StepInterlock");
		mLaunchTask.startAfter(interlock);
	}
}

bool ParticleIosActorGPU::launch(CUstream stream, int kernelIndex)
{
	physx::PxF32 deltaTime = mParticleIosScene->getApexScene().getPhysXSimulateTime();

	physx::PxU32 targetCount = mParticleBudget;
	if (targetCount == 0)
	{
		//skip simulation & just call IofxManager
		mIofxMgr->updateEffectsData(deltaTime, 0, 0, 0);
		return false;
	}

	physx::PxU32 lastCount = mParticleCount;
	physx::PxU32 injectCount = mInjectedCount;

	physx::PxU32 activeCount = mLastActiveCount + mInjectedCount;
	physx::PxU32 totalCount = lastCount + injectCount;
	PX_ASSERT(targetCount <= totalCount);

	physx::PxU32 boundCount = 0;
	if (activeCount > targetCount)
	{
		boundCount = activeCount - targetCount;
	}

	ParticleIosSceneGPU* sceneGPU = static_cast<ParticleIosSceneGPU*>(mParticleIosScene);
	bool useSyncKernels = !sceneGPU->mGpuDispatcher->getCudaContextManager()->supportsArchSM20();

	switch (kernelIndex)
	{
	case 0:
		if (!mFieldSamplerQuery && mOnStartCallback)
		{
			(*mOnStartCallback)(stream);
		}

		// Copy particle data for newly injected particles
		mCopyQueue.reset(stream, 14);
		if (mInjectedCount > 0)
		{
			mBufDesc.pmaPositionMass->copyHostToDeviceQ(mCopyQueue, mInjectedCount, mParticleCount);
			mBufDesc.pmaVelocityLife->copyHostToDeviceQ(mCopyQueue, mInjectedCount, mParticleCount);
			mBufDesc.pmaActorIdentifiers->copyHostToDeviceQ(mCopyQueue, mInjectedCount, mParticleCount);
			mLifeSpan.copyHostToDeviceQ(mCopyQueue, mInjectedCount, mParticleCount);
			mInjector.copyHostToDeviceQ(mCopyQueue, mInjectedCount, mParticleCount);
			mBenefit.copyHostToDeviceQ(mCopyQueue, mInjectedCount, mParticleCount);
			mBufDesc.pmaUserData->copyHostToDeviceQ(mCopyQueue,mInjectedCount,mParticleCount);
#if defined(APEX_TEST)
			mTestMirroredArray.copyHostToDeviceQ(mCopyQueue, 16);
#endif
			mCopyQueue.flushEnqueued();
		}
		return true;

	case 1:
		if (totalCount > 0)
		{
			PxF32 benefitMin = PxMin(mLastBenefitMin, mInjectedBenefitMin);
			PxF32 benefitMax = PxMax(mLastBenefitMax, mInjectedBenefitMax);
			PX_ASSERT(benefitMin <= benefitMax);
			benefitMax *= 1.00001f;

			if (useSyncKernels)
			{
				CUDA_OBJ(histogramKernel)(
					stream, totalCount,
					mBenefit.getGpuPtr(), boundCount,
					benefitMin, benefitMax,
					mTmpBoundParams.getGpuPtr(),
					mTmpHistogram.getGpuPtr()
				);
			}
			else
			{
				int histogramGridSize =
					CUDA_OBJ(histogram1Kernel)(
						stream, totalCount,
						createApexCudaMemRef(mBenefit, ApexCudaMemRefBase::IN), 
						boundCount,	benefitMin, benefitMax,
						createApexCudaMemRef(mTmpBoundParams, ApexCudaMemRefBase::IN),						
						createApexCudaMemRef(mTmpHistogram, ApexCudaMemRefBase::OUT)
					);

				//launch just 1 block
				CUDA_OBJ(histogram2Kernel)(
					stream, CUDA_OBJ(histogram2Kernel).getBlockDim().x,
					createApexCudaMemRef(mBenefit, ApexCudaMemRefBase::IN), boundCount,
					benefitMin, benefitMax,
					createApexCudaMemRef(mTmpBoundParams, ApexCudaMemRefBase::OUT),
					createApexCudaMemRef(mTmpHistogram, ApexCudaMemRefBase::IN_OUT),
					histogramGridSize
				);
			}
		}
		return true;

	case 2:
		if (totalCount > 0)
		{
			PxF32 benefitMin = PxMin(mLastBenefitMin, mInjectedBenefitMin);
			PxF32 benefitMax = PxMax(mLastBenefitMax, mInjectedBenefitMax);
			PX_ASSERT(benefitMin <= benefitMax);
			benefitMax *= 1.00001f;

			if (useSyncKernels)
			{
				CUDA_OBJ(scanKernel)(
					stream, totalCount,
					benefitMin, benefitMax,
					mHoleScanSum.getGpuPtr(), mBenefit.getGpuPtr(),
					mTmpBoundParams.getGpuPtr(),
					mTmpScan.getGpuPtr(), mTmpScan1.getGpuPtr()
				);
			}
			else
			{
				int scanGridSize = 
					CUDA_OBJ(scan1Kernel)(
						stream, totalCount,
						benefitMin, benefitMax,
						createApexCudaMemRef(mHoleScanSum, ApexCudaMemRefBase::IN), 
						createApexCudaMemRef(mBenefit, ApexCudaMemRefBase::IN),
						createApexCudaMemRef(mTmpBoundParams, ApexCudaMemRefBase::IN),
						createApexCudaMemRef(mTmpScan, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef(mTmpScan1, ApexCudaMemRefBase::OUT)
					);

				//launch just 1 block
				CUDA_OBJ(scan2Kernel)(
					stream, CUDA_OBJ(scan2Kernel).getBlockDim().x,
					benefitMin, benefitMax,
					createApexCudaMemRef(mHoleScanSum, ApexCudaMemRefBase::IN), 
					createApexCudaMemRef(mBenefit, ApexCudaMemRefBase::IN),
					createApexCudaMemRef(mTmpBoundParams, ApexCudaMemRefBase::IN),
					createApexCudaMemRef(mTmpScan, ApexCudaMemRefBase::IN_OUT),
					createApexCudaMemRef(mTmpScan1, ApexCudaMemRefBase::IN_OUT),
					scanGridSize
				);

				CUDA_OBJ(scan3Kernel)(
					stream, totalCount,
					benefitMin, benefitMax,
					createApexCudaMemRef(mHoleScanSum, ApexCudaMemRefBase::OUT),
					createApexCudaMemRef(mBenefit, ApexCudaMemRefBase::IN),
					createApexCudaMemRef(mTmpBoundParams, ApexCudaMemRefBase::IN),
					createApexCudaMemRef(mTmpScan, ApexCudaMemRefBase::IN), 
					createApexCudaMemRef(mTmpScan1, ApexCudaMemRefBase::IN)
				);
			}
		}
		return true;

	case 3:
	{
		if (totalCount > 0)
		{
			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefCompactScanSum, mHoleScanSum);
			const PxU32 injectorCount = mInjectorList.getSize();

			CUDA_OBJ(compactKernel)(
				stream,
				PxMax(totalCount, injectorCount),
				targetCount,
				totalCount,
				injectorCount,
				createApexCudaMemRef(mMoveIndices, ApexCudaMemRefBase::OUT),
				createApexCudaMemRef(mTmpScan, ApexCudaMemRefBase::OUT),
				createApexCudaMemRef(mInjectorsCounters, ApexCudaMemRefBase::OUT)
			);
		}
		return true;
	}

	case 4:
		if (targetCount > 0)
		{
			int histogramGridSize = 0;
			{
			APEX_CUDA_TEXTURE_SCOPE_BIND_SIZE(texRefMoveIndices, mMoveIndices, totalCount);

			APEX_CUDA_TEXTURE_SCOPE_BIND_SIZE(texRefPositionMass, *mBufDesc.pmaPositionMass, totalCount);
			APEX_CUDA_TEXTURE_SCOPE_BIND_SIZE(texRefVelocityLife, *mBufDesc.pmaVelocityLife, totalCount);
			APEX_CUDA_TEXTURE_SCOPE_BIND_SIZE(texRefIofxActorIDs, *mBufDesc.pmaActorIdentifiers, totalCount);
			APEX_CUDA_TEXTURE_SCOPE_BIND_SIZE(texRefLifeSpan, mLifeSpan, totalCount);
			APEX_CUDA_TEXTURE_SCOPE_BIND_SIZE(texRefLifeTime, mLifeTime, totalCount);
			APEX_CUDA_TEXTURE_SCOPE_BIND_SIZE(texRefInjector, mInjector, totalCount);

			APEX_CUDA_TEXTURE_SCOPE_BIND_SIZE(texRefUserData,*mBufDesc.pmaUserData, totalCount);

			SCOPED_PHYSX_LOCK_READ(mParticleIosScene->getApexScene());

			PxCudaReadWriteParticleBuffers buffers;
			memset(&buffers, 0, sizeof(buffers));

				const physx::PxVec3& eyePos = mParticleIosScene->getApexScene().getEyePosition();
				ParticleIosSceneGPU* sceneGPU = static_cast<ParticleIosSceneGPU*>(mParticleIosScene);

				PxParticleDeviceExclusive::getReadWriteCudaBuffers(*mParticleActor->isParticleBase(), buffers);
				PX_ASSERT( buffers.positions && buffers.velocities && buffers.collisionNormals && buffers.flags);
			
				APEX_CUDA_TEXTURE_SCOPE_BIND_PTR(texRefPxPosition,  (float4*)buffers.positions,        lastCount);
				APEX_CUDA_TEXTURE_SCOPE_BIND_PTR(texRefPxVelocity,  (float4*)buffers.velocities,       lastCount);
				APEX_CUDA_TEXTURE_SCOPE_BIND_PTR(texRefPxCollision, (float4*)buffers.collisionNormals, lastCount);
				if(buffers.densities)
				{
					CUDA_OBJ(texRefPxDensity).bindTo(buffers.densities, lastCount);
				}
				APEX_CUDA_TEXTURE_SCOPE_BIND_PTR(texRefPxFlags,     (unsigned int*)buffers.flags,      lastCount);

				if (mFieldSamplerQuery != NULL)
				{
					APEX_CUDA_TEXTURE_SCOPE_BIND_SIZE(texRefField, mField, totalCount);

					histogramGridSize = CUDA_OBJ(simulateApplyFieldKernel)(stream,
						targetCount,
						lastCount,
						deltaTime,
						eyePos,
						sceneGPU->mInjectorConstMemGroup.getStorage().mappedHandle(sceneGPU->mInjectorParamsArrayHandle),
						mInjectorsCounters.getSize(),
						createApexCudaMemRef(mHoleScanSum, targetCount, ApexCudaMemRefBase::IN),
						createApexCudaMemRef(mInputIdToParticleIndex, ApexCudaMemRefBase::IN),
						createApexCudaMemRef(mTmpScan, 1, ApexCudaMemRefBase::IN), //g_moveCount
						createApexCudaMemRef(mTmpHistogram, targetCount, ApexCudaMemRefBase::OUT),   //targetCount ????
						createApexCudaMemRef(mInjectorsCounters, mInjectorsCounters.getSize(), ApexCudaMemRefBase::OUT),
						createApexCudaMemRef((float4*)mBufDesc.pmaPositionMass->getGpuPtr(), targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef((float4*)mBufDesc.pmaVelocityLife->getGpuPtr(), targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef((float4*)mBufDesc.pmaCollisionNormalFlags->getGpuPtr(), targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef((unsigned int*)mBufDesc.pmaUserData->getGpuPtr(), targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef(mLifeSpan, targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef(mLifeTime, targetCount, ApexCudaMemRefBase::OUT),
						mBufDesc.pmaDensity != NULL ? createApexCudaMemRef((float*)mBufDesc.pmaDensity->getGpuPtr(), targetCount, ApexCudaMemRefBase::OUT) : ApexCudaMemRef<float>(NULL, 0),
						createApexCudaMemRef(mInjector, targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef(*(mBufDesc.pmaActorIdentifiers), targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef(mBenefit, targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef((float4*)buffers.positions, targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef((float4*)buffers.velocities, targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef((float4*)buffers.collisionNormals, targetCount, ApexCudaMemRefBase::IN),
						buffers.densities != NULL ? createApexCudaMemRef((float*)buffers.densities, targetCount, ApexCudaMemRefBase::OUT) : ApexCudaMemRef<float>(NULL, 0),
						createApexCudaMemRef((unsigned int*)buffers.flags, targetCount, ApexCudaMemRefBase::OUT),
						mGridDensityParams
						);
				}
				else
				{
					histogramGridSize = CUDA_OBJ(simulateKernel)(stream,
						targetCount,
						lastCount,
						deltaTime,
						eyePos,
						sceneGPU->mInjectorConstMemGroup.getStorage().mappedHandle(sceneGPU->mInjectorParamsArrayHandle),
						mInjectorsCounters.getSize(),
						mHoleScanSum.getGpuPtr(),
						mInputIdToParticleIndex.getGpuPtr(),
						mTmpScan.getGpuPtr(),
						mTmpHistogram.getGpuPtr(),
						mInjectorsCounters.getGpuPtr(),
						(float4*)mBufDesc.pmaPositionMass->getGpuPtr(),
						(float4*)mBufDesc.pmaVelocityLife->getGpuPtr(),
						(float4*)mBufDesc.pmaCollisionNormalFlags->getGpuPtr(),
						(unsigned int*)mBufDesc.pmaUserData->getGpuPtr(),
						mLifeSpan.getGpuPtr(),
						mLifeTime.getGpuPtr(),
						mBufDesc.pmaDensity != NULL ? (float*)mBufDesc.pmaDensity->getGpuPtr() : NULL,
						mInjector.getGpuPtr(),
						mBufDesc.pmaActorIdentifiers->getGpuPtr(),
						mBenefit.getGpuPtr(),
						(float4*)buffers.positions,
						(float4*)buffers.velocities,
						(float4*)buffers.collisionNormals,
						buffers.densities != NULL ? (float*)buffers.densities : NULL,
						(unsigned int*) buffers.flags,
						mGridDensityParams
						);
				}
				if(buffers.densities)
				{
					CUDA_OBJ(texRefPxDensity).unbind();
				}
			}
			//new kernel invocation - to merge temp histograms 
			{
				if(mInjectorsCounters.getSize() <= HISTOGRAM_SIMULATE_BIN_COUNT)
				{
					CUDA_OBJ(mergeHistogramKernel)(stream, CUDA_OBJ(mergeHistogramKernel).getBlockDim().x,
						mInjectorsCounters.getGpuPtr(),
						mTmpHistogram.getGpuPtr(),
						histogramGridSize,
						mInjectorsCounters.getSize()
						);
				}

			}
			// calculate grid grid density
			if (mGridDensityParams.Enabled)
			{
				mGridDensityParams.DensityOrigin = mDensityOrigin;
				const unsigned int dim = mGridDensityParams.GridResolution;
				// refreshed non-shared params
				{
					ParticleIosAssetParam* params = (ParticleIosAssetParam*)(mAsset->getAssetNxParameterized());
					const SimpleParticleSystemParams* gridParams = static_cast<SimpleParticleSystemParams*>(params->particleType);
					mGridDensityParams.GridSize = gridParams->GridDensity.GridSize;
					mGridDensityParams.GridMaxCellCount = gridParams->GridDensity.MaxCellCount;
				}
				// extract frustum
				if(mParticleIosScene->getApexScene().getNumProjMatrices() > 0)
				{
					PxMat44 matDen = PxMat44::createIdentity();
					GridDensityFrustumParams frustum;
					PxMat44 matModel = mParticleIosScene->getApexScene().getViewMatrix();
					PxMat44 matProj  = mParticleIosScene->getApexScene().getProjMatrix();
					PxMat44 mat = matProj*matModel;
					PxMat44 matInv = inverse(mat);
					const PxReal targetDepth = mGridDensityParams.GridSize;
					// for debug vis
					mDensityDebugMatInv = matInv;
					// to calculate w transform
					PxReal nearDimX = distance(matInv.transform(PxVec4(-1.f,0.f,0.f,1.f)),matInv.transform(PxVec4(1.f,0.f,0.f,1.f)));
					PxReal farDimX	= distance(matInv.transform(PxVec4(-1.f,0.f,1.f,1.f)),matInv.transform(PxVec4(1.f,0.f,1.f,1.f)));
					PxReal nearDimY	= distance(matInv.transform(PxVec4(0.f,-1.f,0.f,1.f)),matInv.transform(PxVec4(0.f,1.f,0.f,1.f)));
					PxReal farDimY	= distance(matInv.transform(PxVec4(0.f,-1.f,1.f,1.f)),matInv.transform(PxVec4(0.f,1.f,1.f,1.f)));
					PxReal dimZ		= distance(matInv.transform(PxVec4(0.f, 0.f,0.f,1.f)),matInv.transform(PxVec4(0.f,0.f,1.f,1.f)));
					PxReal myFarDimX = nearDimX*(1.f-targetDepth/dimZ) + farDimX*(targetDepth/dimZ);
					PxReal myFarDimY = nearDimY*(1.f-targetDepth/dimZ) + farDimY*(targetDepth/dimZ);
					// grab necessary frustum coordinates
					PxVec4 origin4 = matInv.transform(PxVec4(-1.f, 1.f,0.f,1.f));
					PxVec4 basisX4 = matInv.transform(PxVec4( 1.f, 1.f,0.f,1.f));
					PxVec4 basisY4 = matInv.transform(PxVec4(-1.f,-1.f,0.f,1.f));
					PxVec4 zDepth4 = matInv.transform(PxVec4(-1.f, 1.f,1.f,1.f));
					// create vec3 versions
					PxVec3 origin3(origin4.x/origin4.w,origin4.y/origin4.w,origin4.z/origin4.w);
					PxVec3 basisX3(basisX4.x/basisX4.w,basisX4.y/basisX4.w,basisX4.z/basisX4.w);
					PxVec3 basisY3(basisY4.x/basisY4.w,basisY4.y/basisY4.w,basisY4.z/basisY4.w);
					PxVec3 zDepth3(zDepth4.x/zDepth4.w,zDepth4.y/zDepth4.w,zDepth4.z/zDepth4.w);
					// make everthing relative to origin
					basisX3 -= origin3;
					basisY3 -= origin3;
					zDepth3 -= origin3;
					// find third basis
					PxVec3 basisZ3(basisX3.cross(basisY3));
					basisZ3.normalize();
					basisZ3*= targetDepth;
					// build scale,rotation,translation matrix
					PxMat44 mat1Inv = PxMat44::createIdentity();
					mat1Inv.column0 = PxVec4(basisX3,0.f);
					mat1Inv.column1 = PxVec4(basisY3,0.f);
					mat1Inv.column2 = PxVec4(basisZ3,0.f);
					mat1Inv.column3 = PxVec4(origin3,1.f);
					PxMat44 mat1 = inverse(mat1Inv);
					// do perspective transform
					PxMat44 mat2 = PxMat44::createIdentity();
					{
						PxReal left		= -3.0f;
						PxReal right	= 1.0f;
						PxReal top		= 1.0f;
						PxReal bottom	= -3.0f;
						PxReal nearVal	= nearDimX/(0.5f*(myFarDimX-nearDimX));
						//PxReal farVal	= nearVal + 1.f;
						// build matrix
						mat2.column0.x = -2.f*nearVal/(right-left);
						mat2.column1.y = -2.f*nearVal/(top-bottom);
						mat2.column2.x = (right+left)/(right-left);
						mat2.column2.y = (top+bottom)/(top-bottom);
						//mat2.column2.z = -(farVal+nearVal)/(farVal-nearVal);
						mat2.column2.w = -1.f;
						//mat2.column3.z = -(2.f*farVal*nearVal)/(farVal-nearVal);
						mat2.column3.w = 0.f;
					}
					// shrink to calculate density just outside of frustum
					PxMat44 mat3 = PxMat44::createIdentity();
					PxReal factor = (PxReal)(mGridDensityParams.GridResolution-4) / (mGridDensityParams.GridResolution);
					{			
						mat3.column0.x = factor;
						mat3.column1.y = factor;
						mat3.column2.z = factor;
						mat3.column3.x = (1.0f-factor)/2.0f;
						mat3.column3.y = (1.0f-factor)/2.0f;
						mat3.column3.z = (1.0f-factor)/2.0f;
					}
					// create final matrix
					matDen = mat3*mat2*mat1;
					// create frustum info
					frustum.nearDimX = factor*nearDimX;
					frustum.farDimX  = factor*myFarDimX;
					frustum.nearDimY = factor*nearDimY;
					frustum.farDimY	 = factor*myFarDimY;
					frustum.dimZ     = factor*targetDepth;
					// launch frustum kernels
					CUDA_OBJ(gridDensityGridClearKernel)(stream, dim*dim*dim,
						mGridDensityGrid.getGpuPtr(),
						mGridDensityParams
					);
					CUDA_OBJ(gridDensityGridFillFrustumKernel)(stream, targetCount,
						(float4*)mBufDesc.pmaPositionMass->getGpuPtr(),
						mGridDensityGrid.getGpuPtr(),
						mGridDensityParams,
						matDen,
						frustum
						);
					CUDA_OBJ(gridDensityGridLowPassKernel)(stream, dim*dim*dim,
						mGridDensityGrid.getGpuPtr(),
						mGridDensityGridLowPass.getGpuPtr(),
						mGridDensityParams
						);
					CUDA_OBJ(gridDensityGridApplyFrustumKernel)(stream, targetCount,
						mBufDesc.pmaDensity != NULL ? (float*)mBufDesc.pmaDensity->getGpuPtr() : NULL,
						(float4*)mBufDesc.pmaPositionMass->getGpuPtr(),
						mGridDensityGridLowPass.getGpuPtr(),
						mGridDensityParams,
						matDen,
						frustum
						);
				}
			}
		}
		return true;

	case 5:
		if (targetCount > 0)
		{
			if (useSyncKernels)
			{
				CUDA_OBJ(reduceKernel)(
					stream, targetCount,
					mBenefit.getGpuPtr(), (float4*)mTmpOutput.getGpuPtr(), mTmpReduce.getGpuPtr()
				);
			}
			else
			{
				int reduceGridSize =
					CUDA_OBJ(reduce1Kernel)(
						stream, targetCount,
						createApexCudaMemRef(mBenefit, ApexCudaMemRefBase::IN),
						createApexCudaMemRef((float4*)mTmpOutput.getGpuPtr(), 1, ApexCudaMemRefBase::IN),
						createApexCudaMemRef(mTmpReduce, ApexCudaMemRefBase::OUT)
					);

				//launch just 1 block
				CUDA_OBJ(reduce2Kernel)(
					stream, CUDA_OBJ(reduce2Kernel).getBlockDim().x,
					createApexCudaMemRef(mBenefit, ApexCudaMemRefBase::IN),
					createApexCudaMemRef((float4*)mTmpOutput.getGpuPtr(), 1, ApexCudaMemRefBase::OUT),
					createApexCudaMemRef(mTmpReduce, ApexCudaMemRefBase::IN),
					reduceGridSize
				);
			}
		}
		return true;

	case 6:
		if (totalCount > 0)
		{
			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefHoleScanSum, mHoleScanSum);
			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefMoveIndices, mMoveIndices);

			CUDA_OBJ(stateKernel)(stream, totalCount,
			                      lastCount, targetCount,
								  createApexCudaMemRef(mTmpScan, ApexCudaMemRefBase::IN),
								  createApexCudaMemRef(*mBufDesc.pmaInStateToInput, ApexCudaMemRefBase::OUT),
								  createApexCudaMemRef(*mBufDesc.pmaOutStateToInput, ApexCudaMemRefBase::IN)
			                     );
		}
		return true;

	case 7:
#if defined(APEX_TEST)
		{
			//Test kernel
			PxU32 scalarVar = 1;
			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefHoleScanSum, mTestMirroredArray);
			CUDA_OBJ(testKernel)(stream, 16, scalarVar, createApexCudaMemRef(mTestMirroredArray), mTestConstMemGroup.getStorage().mappedHandle(mTestITHandle));
			return true;
		}
	case 8:
		mTestMirroredArray.copyDeviceToHostQ(mCopyQueue);
#endif
		mTmpOutput.copyDeviceToHostQ(mCopyQueue);
		mInjectorsCounters.copyDeviceToHostQ(mCopyQueue);
		mCopyQueue.flushEnqueued();
		
		/* Oh! Manager of the IOFX! do your thing */
		mIofxMgr->updateEffectsData(deltaTime, targetCount, targetCount, totalCount);
		return false;
	}
	return false;
}



void ParticleIosActorGPU::fetchResults()
{
	ParticleIosActor::fetchResults();
	physx::PxU32 targetCount = mParticleBudget;
	if (targetCount > 0)
	{
		float* pTmpOutput = (float*)mTmpOutput.getPtr();

		mLastActiveCount = mTmpOutput[STATUS_LAST_ACTIVE_COUNT];
		mLastBenefitSum  = pTmpOutput[STATUS_LAST_BENEFIT_SUM];
		mLastBenefitMin  = pTmpOutput[STATUS_LAST_BENEFIT_MIN];
		mLastBenefitMax  = pTmpOutput[STATUS_LAST_BENEFIT_MAX];
	}
	else
	{
		//this is the case when we skiped the simulation
		mLastActiveCount = 0;
		mLastBenefitSum = 0;
		mLastBenefitMin = +FLT_MAX;
		mLastBenefitMax = -FLT_MAX;
	}
	mParticleCount = targetCount;

	mIofxMgr->outputHostToDevice();
}


PxMat44 ParticleIosActorGPU::inverse(const PxMat44& in)
{
	PxMat44 ret;
	PxReal inv[16];
	PxReal* invOut = &ret.column0.x;
	const PxReal* m = &in.column0.x;
	int i;

    inv[0] = m[5]  * m[10] * m[15] - 
             m[5]  * m[11] * m[14] - 
             m[9]  * m[6]  * m[15] + 
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] - 
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] + 
              m[4]  * m[11] * m[14] + 
              m[8]  * m[6]  * m[15] - 
              m[8]  * m[7]  * m[14] - 
              m[12] * m[6]  * m[11] + 
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] - 
             m[4]  * m[11] * m[13] - 
             m[8]  * m[5] * m[15] + 
             m[8]  * m[7] * m[13] + 
             m[12] * m[5] * m[11] - 
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] + 
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] - 
               m[8]  * m[6] * m[13] - 
               m[12] * m[5] * m[10] + 
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] + 
              m[1]  * m[11] * m[14] + 
              m[9]  * m[2] * m[15] - 
              m[9]  * m[3] * m[14] - 
              m[13] * m[2] * m[11] + 
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] - 
             m[0]  * m[11] * m[14] - 
             m[8]  * m[2] * m[15] + 
             m[8]  * m[3] * m[14] + 
             m[12] * m[2] * m[11] - 
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] + 
              m[0]  * m[11] * m[13] + 
              m[8]  * m[1] * m[15] - 
              m[8]  * m[3] * m[13] - 
              m[12] * m[1] * m[11] + 
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] - 
              m[0]  * m[10] * m[13] - 
              m[8]  * m[1] * m[14] + 
              m[8]  * m[2] * m[13] + 
              m[12] * m[1] * m[10] - 
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] - 
             m[1]  * m[7] * m[14] - 
             m[5]  * m[2] * m[15] + 
             m[5]  * m[3] * m[14] + 
             m[13] * m[2] * m[7] - 
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] + 
              m[0]  * m[7] * m[14] + 
              m[4]  * m[2] * m[15] - 
              m[4]  * m[3] * m[14] - 
              m[12] * m[2] * m[7] + 
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] - 
              m[0]  * m[7] * m[13] - 
              m[4]  * m[1] * m[15] + 
              m[4]  * m[3] * m[13] + 
              m[12] * m[1] * m[7] - 
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] + 
               m[0]  * m[6] * m[13] + 
               m[4]  * m[1] * m[14] - 
               m[4]  * m[2] * m[13] - 
               m[12] * m[1] * m[6] + 
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + 
              m[1] * m[7] * m[10] + 
              m[5] * m[2] * m[11] - 
              m[5] * m[3] * m[10] - 
              m[9] * m[2] * m[7] + 
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - 
             m[0] * m[7] * m[10] - 
             m[4] * m[2] * m[11] + 
             m[4] * m[3] * m[10] + 
             m[8] * m[2] * m[7] - 
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + 
               m[0] * m[7] * m[9] + 
               m[4] * m[1] * m[11] - 
               m[4] * m[3] * m[9] - 
               m[8] * m[1] * m[7] + 
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - 
              m[0] * m[6] * m[9] - 
              m[4] * m[1] * m[10] + 
              m[4] * m[2] * m[9] + 
              m[8] * m[1] * m[6] - 
              m[8] * m[2] * m[5];

    PxReal det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
		return PxMat44::createIdentity();

    det = 1.0f / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

	return ret;
}

PxReal ParticleIosActorGPU::distance(PxVec4 a, PxVec4 b)
{
	PxVec3 a3(a.x/a.w,a.y/a.w,a.z/a.w);
	PxVec3 b3(b.x/b.w,b.y/b.w,b.z/b.w);
	PxVec3 diff(b3-a3);
	return diff.magnitude();
}

}
}
} // namespace physx::apex

#endif //defined(APEX_CUDA_SUPPORT)
#endif // NX_SDK_VERSION_MAJOR == 3
