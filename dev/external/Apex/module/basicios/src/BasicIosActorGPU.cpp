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
#if defined(APEX_CUDA_SUPPORT)

#include "NxApex.h"
#include "NiApexScene.h"
#include "NiApexSDK.h"

#include "NxBasicIosActor.h"
#include "BasicIosActorGPU.h"
#include "BasicIosAsset.h"
#include "NxIofxAsset.h"
#include "NxIofxActor.h"
#include "ModuleBasicIos.h"
#include "BasicIosScene.h"
#include "NiApexRenderDebug.h"
#include "NiApexAuthorableObject.h"

#include "foundation/PxMath.h"
#ifdef APEX_TEST
#include "BasicIosActorTestData.h"
#endif

//CUDA
#include "PxGpuTask.h"
#include "ApexCutil.h"

#define CUDA_OBJ(name) SCENE_CUDA_OBJ(*mBasicIosScene, name)

#if APEX_CUDA_CHECK_ENABLED
#define CUDA_CHECK(msg) \
	{ \
		physx::PxTaskManager* tm = mBasicIosScene->getApexScene().getTaskManager(); \
		physx::PxCudaContextManager* ctx = tm->getGpuDispatcher()->getCudaContextManager(); \
		physx::PxScopedCudaLock s(*ctx); \
		CUresult ret = cuCtxSynchronize(); \
		if( CUDA_SUCCESS != ret ) { \
			APEX_INTERNAL_ERROR("Cuda Error %d, %s", ret, msg); \
			PX_ASSERT(!ret); \
		} \
	}
#else
#define CUDA_CHECK(msg)
#endif


namespace physx
{
namespace apex
{
namespace basicios
{

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

BasicIosActorGPU::BasicIosActorGPU(
    NxResourceList& list,
    BasicIosAsset& asset,
    BasicIosScene& scene,
    physx::apex::NxIofxAsset& iofxAsset)
	: BasicIosActor(list, asset, scene, iofxAsset, true)
	, mCopyQueue(*scene.getApexScene().getTaskManager()->getGpuDispatcher())
	, mHoleScanSum(scene.getApexScene())
	, mMoveIndices(scene.getApexScene())
	, mTmpReduce(scene.getApexScene())
	, mTmpHistogram(scene.getApexScene())
	, mTmpScan(scene.getApexScene())
	, mTmpScan1(scene.getApexScene())
	, mTmpOutput(scene.getApexScene())
	, mTmpOutput1(scene.getApexScene())
	, mLaunchTask(*this)
{
	initStorageGroups(CUDA_OBJ(simulateConstMem));

	//CUDA
	physx::PxCudaContextManager* ctxMgr = mBasicIosScene->getApexScene().getTaskManager()->getGpuDispatcher()->getCudaContextManager();
	int mMPCount = ctxMgr->getMultiprocessorCount();

#if defined(APEX_TEST) || APEX_CUDA_CHECK_ENABLED
	const ApexMirroredPlace::Enum defaultPlace = ApexMirroredPlace::CPU_GPU;
#else
	const ApexMirroredPlace::Enum defaultPlace = ApexMirroredPlace::GPU;
#endif

	mTmpOutput.setSize(4, ApexMirroredPlace::CPU_GPU);
	mTmpOutput1.setSize(2, ApexMirroredPlace::CPU_GPU);

	const unsigned int ScanWarpsPerBlock = CUDA_OBJ(scanKernel).getBlockDim().x / WARP_SIZE;
	mTmpReduce.reserve(WARP_SIZE * 4, defaultPlace);
	mTmpHistogram.reserve(mMPCount * HISTOGRAM_SIMULATE_BIN_COUNT, defaultPlace);
	mTmpScan.reserve(mMPCount * ScanWarpsPerBlock, defaultPlace);
	mTmpScan1.reserve(mMPCount * ScanWarpsPerBlock, defaultPlace);

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

	if (mAsset->mParams->collisionWithConvex)
	{
		mConvexPlanes.reserve(MAX_CONVEX_PLANES_COUNT, ApexMirroredPlace::CPU_GPU);
		mConvexVerts.reserve(MAX_CONVEX_VERTS_COUNT, ApexMirroredPlace::CPU_GPU);
		mConvexPolygonsData.reserve(MAX_CONVEX_POLYGONS_DATA_SIZE, ApexMirroredPlace::CPU_GPU);
	}
	if (mAsset->mParams->collisionWithTriangleMesh)
	{
		mTrimeshVerts.reserve(MAX_TRIMESH_VERTS_COUNT, ApexMirroredPlace::CPU_GPU);
		mTrimeshIndices.reserve(MAX_TRIMESH_INDICES_COUNT, ApexMirroredPlace::CPU_GPU);
	}

	mHoleScanSum.reserve(mMaxTotalParticleCount, defaultPlace);
	mMoveIndices.reserve(mMaxTotalParticleCount, defaultPlace);
}

BasicIosActorGPU::~BasicIosActorGPU()
{
}

void BasicIosActorGPU::submitTasks()
{
	BasicIosActor::submitTasks();

	mInjectorsCounters.setSize(mInjectorList.getSize(), ApexMirroredPlace::CPU_GPU); 
	physx::PxTaskManager* tm = mBasicIosScene->getApexScene().getTaskManager();
	tm->submitUnnamedTask(mLaunchTask, physx::PxTaskType::TT_GPU);
}

void BasicIosActorGPU::setTaskDependencies()
{
	BasicIosActor::setTaskDependencies(&mLaunchTask, true);

	physx::PxTaskManager* tm = mBasicIosScene->getApexScene().getTaskManager();
	if (tm->getGpuDispatcher()->getCudaContextManager()->supportsArchSM20())
	{
		/* For Fermi devices, it pays to launch all IOS together.  This also forces
		 * The IOFX managers to step at the same time.
		 */
		PxTaskID interlock = tm->getNamedTask("IOS::StepInterlock");
		mLaunchTask.startAfter(interlock);
	}
}

bool BasicIosActorGPU::launch(CUstream stream, int kernelIndex)
{
	physx::PxF32 deltaTime = mBasicIosScene->getApexScene().getPhysXSimulateTime();

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

	BasicIosSceneGPU* sceneGPU = static_cast<BasicIosSceneGPU*>(mBasicIosScene);
	bool useSyncKernels = !sceneGPU->mGpuDispatcher->getCudaContextManager()->supportsArchSM20();

	switch (kernelIndex)
	{
	case 0:
		if (!mFieldSamplerQuery && mOnStartCallback)
		{
			(*mOnStartCallback)(stream);
		}

		mCopyQueue.reset(stream, 12);
		if (mInjectedCount > 0)
		{
			mBufDesc.pmaPositionMass->copyHostToDeviceQ(mCopyQueue, mInjectedCount, mParticleCount);
			mBufDesc.pmaVelocityLife->copyHostToDeviceQ(mCopyQueue, mInjectedCount, mParticleCount);
			mBufDesc.pmaActorIdentifiers->copyHostToDeviceQ(mCopyQueue, mInjectedCount, mParticleCount);
			mLifeSpan.copyHostToDeviceQ(mCopyQueue, mInjectedCount, mParticleCount);
			mInjector.copyHostToDeviceQ(mCopyQueue, mInjectedCount, mParticleCount);
			mBenefit.copyHostToDeviceQ(mCopyQueue, mInjectedCount, mParticleCount);
			mBufDesc.pmaUserData->copyHostToDeviceQ(mCopyQueue, mInjectedCount, mParticleCount);
		}
		if (mAsset->mParams->collisionWithConvex)
		{
			mConvexPlanes.copyHostToDeviceQ(mCopyQueue);
			mConvexVerts.copyHostToDeviceQ(mCopyQueue);
			mConvexPolygonsData.copyHostToDeviceQ(mCopyQueue);
		}
		if (mAsset->mParams->collisionWithTriangleMesh)
		{
			mTrimeshVerts.copyHostToDeviceQ(mCopyQueue);
			mTrimeshIndices.copyHostToDeviceQ(mCopyQueue);
		}
		mCopyQueue.flushEnqueued();
		CUDA_CHECK("BasicIosActorGPU::copyToDevice");
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
					mTmpOutput1.getGpuPtr(),
					mTmpHistogram.getGpuPtr()
				);
			}
			else
			{
				int histogramGridSize =
					CUDA_OBJ(histogram1Kernel)(
						stream, totalCount,
						mBenefit.getGpuPtr(), boundCount,
						benefitMin, benefitMax,
						mTmpOutput1.getGpuPtr(),
						mTmpHistogram.getGpuPtr()
					);

				//launch just 1 block
				CUDA_OBJ(histogram2Kernel)(
					stream, CUDA_OBJ(histogram2Kernel).getBlockDim().x,
					mBenefit.getGpuPtr(), boundCount,
					benefitMin, benefitMax,
					mTmpOutput1.getGpuPtr(),
					mTmpHistogram.getGpuPtr(),
					histogramGridSize
				);
			}
		}
		CUDA_CHECK("BasicIosActorGPU::histogram");
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
					mTmpOutput1.getGpuPtr(),
					mTmpScan.getGpuPtr(), mTmpScan1.getGpuPtr()
				);
			}
			else
			{
				int scanGridSize = 
					CUDA_OBJ(scan1Kernel)(
						stream, totalCount,
						benefitMin, benefitMax,
						mHoleScanSum.getGpuPtr(), mBenefit.getGpuPtr(),
						mTmpOutput1.getGpuPtr(),
						mTmpScan.getGpuPtr(), mTmpScan1.getGpuPtr()
					);

				//launch just 1 block
				CUDA_OBJ(scan2Kernel)(
					stream, CUDA_OBJ(scan2Kernel).getBlockDim().x,
					benefitMin, benefitMax,
					mHoleScanSum.getGpuPtr(), mBenefit.getGpuPtr(),
					mTmpOutput1.getGpuPtr(),
					mTmpScan.getGpuPtr(), mTmpScan1.getGpuPtr(),
					scanGridSize
				);

				CUDA_OBJ(scan3Kernel)(
					stream, totalCount,
					benefitMin, benefitMax,
					mHoleScanSum.getGpuPtr(), mBenefit.getGpuPtr(),
					mTmpOutput1.getGpuPtr(),
					mTmpScan.getGpuPtr(), mTmpScan1.getGpuPtr()
				);
			}
		}
		CUDA_CHECK("BasicIosActorGPU::scan");
		return true;

	case 3:
	{
		if (totalCount > 0)
		{
			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefCompactScanSum, mHoleScanSum);

			const PxU32 injectorCount = mInjectorList.getSize();

			CUDA_OBJ(compactKernel)(
				stream, PxMax(totalCount, injectorCount),
				targetCount, totalCount, injectorCount, mMoveIndices.getGpuPtr(), mTmpScan.getGpuPtr(), mInjectorsCounters.getGpuPtr()
			);
		}
		CUDA_CHECK("BasicIosActorGPU::compact");
		return true;
	}


	case 4:
		if (targetCount > 0)
		{
			int histogramGridSize = 0;
			{
				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefMoveIndices, mMoveIndices);

				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefPositionMass, *mBufDesc.pmaPositionMass);
				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefVelocityLife, *mBufDesc.pmaVelocityLife);
				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefIofxActorIDs, *mBufDesc.pmaActorIdentifiers);
				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefLifeSpan, mLifeSpan);
				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefLifeTime, mLifeTime);
				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefInjector, mInjector);

				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefUserData, *mBufDesc.pmaUserData);

				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefConvexPlanes, mConvexPlanes);
				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefConvexVerts, mConvexVerts);
				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefConvexPolygonsData, mConvexPolygonsData);

				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefTrimeshVerts, mTrimeshVerts);
				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefTrimeshIndices, mTrimeshIndices);

				physx::PxVec3 gravity = -mUp;
				const physx::PxVec3& eyePos = mBasicIosScene->getApexScene().getEyePosition();
			
				if (mFieldSamplerQuery != NULL)
				{
					APEX_CUDA_TEXTURE_SCOPE_BIND(texRefField, mField);

					histogramGridSize = CUDA_OBJ(simulateApplyFieldKernel)(stream, 
						targetCount,
						lastCount,
						deltaTime,
						gravity,
						eyePos,
						sceneGPU->mInjectorConstMemGroup.getStorage().mappedHandle(sceneGPU->mInjectorParamsArrayHandle), mInjectorsCounters.getSize(),
						createApexCudaMemRef(mHoleScanSum, targetCount, ApexCudaMemRefBase::IN),
						createApexCudaMemRef(mTmpScan, 1, ApexCudaMemRefBase::IN),
						createApexCudaMemRef(mTmpHistogram, targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef(mInjectorsCounters, mInjectorsCounters.getSize(), ApexCudaMemRefBase::OUT),
						createApexCudaMemRef((float4*)mBufDesc.pmaPositionMass->getGpuPtr(), targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef((float4*)mBufDesc.pmaVelocityLife->getGpuPtr(), targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef((float4*)mBufDesc.pmaCollisionNormalFlags->getGpuPtr(), targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef(mBufDesc.pmaUserData->getGpuPtr(), targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef(mLifeSpan, targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef(mLifeTime, targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef(mInjector, targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef(*(mBufDesc.pmaActorIdentifiers),  targetCount, ApexCudaMemRefBase::OUT),
						createApexCudaMemRef(mBenefit, targetCount, ApexCudaMemRefBase::OUT),
						mSimulationStorageGroup.getStorage().mappedHandle(mSimulationParamsHandle)
						);
				}
				else
				{
					histogramGridSize = CUDA_OBJ(simulateKernel)(stream, targetCount,
											 lastCount, deltaTime, gravity, eyePos,
											 sceneGPU->mInjectorConstMemGroup.getStorage().mappedHandle(sceneGPU->mInjectorParamsArrayHandle), mInjectorsCounters.getSize(),											 mHoleScanSum.getGpuPtr(), mTmpScan.getGpuPtr(), mTmpHistogram.getGpuPtr(), mInjectorsCounters.getGpuPtr(),
											 (float4*)mBufDesc.pmaPositionMass->getGpuPtr(),
											 (float4*)mBufDesc.pmaVelocityLife->getGpuPtr(),
											 (float4*)mBufDesc.pmaCollisionNormalFlags->getGpuPtr(),
											 mBufDesc.pmaUserData->getGpuPtr(),
											 mLifeSpan.getGpuPtr(), mLifeTime.getGpuPtr(), mInjector.getGpuPtr(), mBufDesc.pmaActorIdentifiers->getGpuPtr(),
											 mBenefit.getGpuPtr(), mSimulationStorageGroup.getStorage().mappedHandle(mSimulationParamsHandle)
											);
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
					BasicIOSAssetParam* gridParams = (BasicIOSAssetParam*)(mAsset->getAssetNxParameterized());
					mGridDensityParams.GridSize = gridParams->GridDensity.GridSize;
					mGridDensityParams.GridMaxCellCount = gridParams->GridDensity.MaxCellCount;
				}
				// extract frustum
				if (mBasicIosScene->getApexScene().getNumProjMatrices() > 0)
				{
					PxMat44 matDen = PxMat44::createIdentity();
					GridDensityFrustumParams frustum;
					PxMat44 matModel = mBasicIosScene->getApexScene().getViewMatrix();
					PxMat44 matProj  = mBasicIosScene->getApexScene().getProjMatrix();
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
					// see how skewed the eye point is
					PxVec3 eye;
					{
						// find the eye point
						PxVec4 a4 = matInv.transform(PxVec4(1.f,1.f,0.00f,1.f));
						PxVec4 b4 = matInv.transform(PxVec4(1.f,1.f,0.01f,1.f));
						PxVec4 c4 = matInv.transform(PxVec4(-1.f,-1.f,0.00f,1.f));
						PxVec4 d4 = matInv.transform(PxVec4(-1.f,-1.f,0.01f,1.f));
						PxVec3 a3 = a4.getXYZ()/a4.w;
						PxVec3 b3 = b4.getXYZ()/b4.w;
						PxVec3 c3 = c4.getXYZ()/c4.w;
						PxVec3 d3 = d4.getXYZ()/d4.w;
						PxVec3 a = b3-a3;
						PxVec3 b = d3-c3;
						PxVec3 c = a.cross(b);
						PxVec3 d = a3-c3;
						PxMat33 m(a,b,c);
						PxMat33 mInv = m.getInverse();
						PxVec3 coord = mInv.transform(d);
						eye = c3 + (d3-c3)*coord.y;		
					}
					// build scale,rotation,translation matrix
					PxMat44 mat1Inv = PxMat44::createIdentity();
					mat1Inv.column0 = PxVec4(basisX3,0.f);
					mat1Inv.column1 = PxVec4(basisY3,0.f);
					mat1Inv.column2 = PxVec4(basisZ3,0.f);
					mat1Inv.column3 = PxVec4(origin3,1.f);
					PxMat44 mat1 = inverse(mat1Inv);
					PxVec3 eyeOffset = mat1.transform(eye);
					// do perspective transform
					PxMat44 mat2 = PxMat44::createIdentity();
					{
						PxReal xshift = -2.f*(eyeOffset.x-0.5f);
						PxReal yshift = -2.f*(eyeOffset.y-0.5f);
						PxReal left		= -3.0f + xshift;
						PxReal right	= 1.0f + xshift;
						PxReal top		= 1.0f + yshift;
						PxReal bottom	= -3.0f + yshift;
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
					PxReal factor = PxMin((PxReal)(mGridDensityParams.GridResolution-4) / (mGridDensityParams.GridResolution),0.75f);
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
		CUDA_CHECK("BasicIosActorGPU::simulate");
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
						mBenefit.getGpuPtr(), (float4*)mTmpOutput.getGpuPtr(), mTmpReduce.getGpuPtr()
					);

				//launch just 1 block
				CUDA_OBJ(reduce2Kernel)(
					stream, CUDA_OBJ(reduce2Kernel).getBlockDim().x,
					mBenefit.getGpuPtr(), (float4*)mTmpOutput.getGpuPtr(), mTmpReduce.getGpuPtr(),
					reduceGridSize
				);
			}
		}
		CUDA_CHECK("BasicIosActorGPU::reduce");
		return true;

	case 6:
		if (totalCount > 0)
		{
			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefHoleScanSum, mHoleScanSum);
			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefMoveIndices, mMoveIndices);

			CUDA_OBJ(stateKernel)(stream, totalCount,
			                      lastCount, targetCount,
			                      mTmpScan.getGpuPtr(),
			                      mBufDesc.pmaInStateToInput->getGpuPtr(), mBufDesc.pmaOutStateToInput->getGpuPtr()
			                     );
		}
		CUDA_CHECK("BasicIosActorGPU::state");
		return true;

	case 7:
		mTmpOutput.copyDeviceToHostQ(mCopyQueue);
		mInjectorsCounters.copyDeviceToHostQ(mCopyQueue);

#if defined(APEX_TEST) || APEX_CUDA_CHECK_ENABLED
		mTmpOutput1.copyDeviceToHostQ(mCopyQueue);
		mTmpScan.copyDeviceToHostQ(mCopyQueue, 1);
		mHoleScanSum.copyDeviceToHostQ(mCopyQueue, totalCount);
		mMoveIndices.copyDeviceToHostQ(mCopyQueue, totalCount);
		mTmpHistogram.copyDeviceToHostQ(mCopyQueue, HISTOGRAM_BIN_COUNT);
		mBenefit.copyDeviceToHostQ(mCopyQueue, totalCount);
		mBufDesc.pmaInStateToInput->copyDeviceToHostQ(mCopyQueue, totalCount);
#endif
#if APEX_CUDA_CHECK_ENABLED
		mBufDesc.pmaPositionMass->copyDeviceToHostQ(mCopyQueue, targetCount);
#endif

		mCopyQueue.flushEnqueued();

		CUDA_CHECK("BasicIosActorGPU::copyToHost");

		/* Oh! Manager of the IOFX! do your thing */
		mIofxMgr->updateEffectsData(deltaTime, targetCount, targetCount, totalCount);

		return false;
	}

	return false;
}

void BasicIosActorGPU::fetchResults()
{
	BasicIosActor::fetchResults();
#ifdef APEX_TEST
	if (mTestData != NULL)
	{
		physx::PxU32 lastCount = mParticleCount;
		physx::PxU32 totalCount = lastCount + mInjectedCount;

		mTestData->mHoleScanSum.resize(totalCount);
		mTestData->mMoveIndices.resize(mTmpScan[0]);
		mTestData->mBenefit.resize(totalCount);
		mTestData->mInStateToInput.resize(totalCount);
		//mTestData->mHistogram.resize(HISTOGRAM_BIN_COUNT);

		for (physx::PxU32 i = 0; i < totalCount; i++)
		{
			mTestData->mHoleScanSum[i] = mHoleScanSum[i];
		}
		for (physx::PxU32 i = 0; i < mTmpScan[0]; i++)
		{
			mTestData->mMoveIndices[i] = mMoveIndices[i];
		}
		for (physx::PxU32 i = 0; i < totalCount; i++)
		{
			mTestData->mBenefit[i] = mBenefit[i];
		}
		//for(physx::PxU32 i = 0; i < HISTOGRAM_BIN_COUNT; i++)
		//	mTestData->mHistogram[i] = mTmpHistogram[i];
		for (physx::PxU32 i = 0; i < totalCount; ++i)
		{
			mTestData->mInStateToInput[i] = mBufDesc.pmaInStateToInput->get(i);
		}

		mTestData->mTmpScan = mTmpScan[0];
		mTestData->mHistogramBeg = mTmpHistogram[ mTmpOutput1[1] ];
		mTestData->mHistogramBack = mTmpHistogram[ HISTOGRAM_BIN_COUNT - 1 ];

		mTestData->mBoundHistorgram = 0;
		PxU32 activeCount = mLastActiveCount + mInjectedCount;
		if (activeCount > mParticleBudget)
		{
			mTestData->mBoundHistorgram = activeCount - mParticleBudget;
		}

		mTestData->mParticleBudget = mParticleBudget;
		mTestData->mInjectedCount = mInjectedCount;
//		mTestData->mParticleCountOld = mParticleCount;
//		mTestData->mActiveParticleCount = mLastActiveCount;
//		mTestData->mBenefitMin = PxMin(mLastBenefitMin, mInjectedBenefitMin);
//		mTestData->mBenefitMax = PxMax(mLastBenefitMax, mInjectedBenefitMax) * 1.00001f;
	}
#endif
	//this can be done only after TaskLaunchAfterLod is finished!!!

#if APEX_CUDA_CHECK_ENABLED
	{
		physx::PxU32 lastCount = mParticleCount;
		physx::PxU32 totalCount = lastCount + mInjectedCount;

		PxU32 validInputCount = 0;
		for (PxU32 i = 0; i < totalCount; ++i)
		{
			PxU32 inputId = mBufDesc.pmaInStateToInput->get(i);
			if (inputId != NiIosBufferDesc::NOT_A_PARTICLE)
			{
				validInputCount++;
			}

			PxF32 benefit = mBenefit[i];
			if (benefit != -FLT_MAX)
			{
				PX_ASSERT(benefit >= 0.0f && benefit <= 1.0f);
			}
		}
		PX_ASSERT(mParticleBudget == 0 || validInputCount == mParticleBudget);
	}
#endif

	physx::PxU32 targetCount = mParticleBudget;
	if (targetCount > 0)
	{
		mLastActiveCount = mTmpOutput[0];
		float* pTmpOutput = (float*)mTmpOutput.getPtr();
		mLastBenefitSum = pTmpOutput[1];
		mLastBenefitMin = pTmpOutput[2];
		mLastBenefitMax = pTmpOutput[3];
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

#ifdef APEX_TEST
void BasicIosActorGPU::copyTestData() const
{
	if (mTestData == NULL)
	{
		return;
	}
	mTestData->mIsGPUTest = true;
	//Copy base data
	BasicIosActor::copyTestData();

	mTestData->mHOLE_SCAN_FLAG = HOLE_SCAN_FLAG;
	mTestData->mHOLE_SCAN_MASK = HOLE_SCAN_MASK;

//	mTestData->mInjectedCount = mInjectedCount;
//	mTestData->mActiveParticleCount = mActiveParticleCount;
}
#endif

PxMat44 BasicIosActorGPU::inverse(const PxMat44& in)
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

PxReal BasicIosActorGPU::distance(PxVec4 a, PxVec4 b)
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
