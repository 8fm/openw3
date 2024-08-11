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

#include "NxApex.h"
#include "NxApexDefs.h"

#if defined(APEX_CUDA_SUPPORT)

#include "NiApexSDK.h"
#include "NiApexScene.h"
#include "Modifier.h"
#include "NxIofxActor.h"
#include "IofxManagerGPU.h"
#include "IofxAsset.h"
#include "IofxScene.h"

#include "ModuleIofx.h"
#include "IofxActorGPU.h"

#include "PxGpuTask.h"
#include "ApexCutil.h"

#include "RandStateHelpers.h"

#include "IofxRenderData.h"

#ifdef APEX_TEST
#include "IofxManagerTestData.h"
#endif

#if APEX_CUDA_CHECK_ENABLED
#define CUDA_CHECK(msg) \
	{ \
		physx::PxTaskManager* tm = mIofxScene.mApexScene->getTaskManager(); \
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

#define CUDA_OBJ(name) SCENE_CUDA_OBJ(mIofxScene, name)

namespace physx
{
namespace apex
{
namespace iofx
{

class IofxAssetSceneInstGPU : public IofxAssetSceneInst
{
public:
	IofxAssetSceneInstGPU(IofxAsset* asset, PxU32 assetID, PxU32 semantics, IofxScene* scene)
		: IofxAssetSceneInst(asset, assetID, semantics)
		, _modifierConstMemGroup(SCENE_CUDA_OBJ(*scene, modifierConstMem))
	{
		_totalRandomCount = 0;

		APEX_CUDA_CONST_MEM_GROUP_SCOPE(_modifierConstMemGroup)

		AssetParams* assetParams = _assetParamsHandle.alloc(_storage_);
		buildModifierList(assetParams->spawnModifierList, _asset->mSpawnModifierStack);
		buildModifierList(assetParams->continuousModifierList, _asset->mContinuousModifierStack);
	}
	virtual ~IofxAssetSceneInstGPU() {}

	InplaceHandle<AssetParams> getAssetParamsHandle() const
	{
		return _assetParamsHandle;
	}

private:

	void buildModifierList(ModifierList& list, const ModifierStack& stack)
	{
		InplaceStorage& _storage_ = _modifierConstMemGroup.getStorage();

		class Mapper : public ModifierParamsMapperGPU
		{
		public:
			InplaceStorage* storage;

			InplaceHandleBase paramsHandle;
			physx::PxU32 paramsRandomCount;

			virtual InplaceStorage& getStorage()
			{
				return *storage;
			}

			virtual void  onParams(InplaceHandleBase handle, physx::PxU32 randomCount)
			{
				paramsHandle = handle;
				paramsRandomCount = randomCount;
			}

		} mapper;
		mapper.storage = &_storage_;

		list.resize(_storage_, stack.size());
		ModifierListElem* listElems = list.getElems(_storage_);

		PxU32 index = 0;
		for (ModifierStack::ConstIterator it = stack.begin(); it != stack.end(); ++it)
		{
			PxU32 type = (*it)->getModifierType();
			//NxU32 usage = (*it)->getModifierUsage();
			//if ((usage & usageStage) == usageStage && (usage & usageClass) == usageClass)
			{
				const Modifier* modifier = Modifier::castFrom(*it);
				modifier->mapParamsGPU(mapper);

				ModifierListElem& listElem = listElems[index++];
				listElem.type = type;
				listElem.paramsHandle = mapper.paramsHandle;

				_totalRandomCount += mapper.paramsRandomCount;
			}
		}
	}

	ApexCudaConstMemGroup		_modifierConstMemGroup;
	InplaceHandle<AssetParams>	_assetParamsHandle;
	physx::PxU32				_totalRandomCount;
};


IofxAssetSceneInst* IofxManagerGPU::createAssetSceneInst(IofxAsset* asset, PxU32 assetID, PxU32 semantics)
{
	return PX_NEW(IofxAssetSceneInstGPU)(asset, assetID, semantics, &mIofxScene);
}

class IofxManagerLaunchTask : public physx::PxGpuTask, public physx::UserAllocated
{
public:
	IofxManagerLaunchTask(IofxManagerGPU* actor) : mActor(actor) {}
	const char* getName() const
	{
		return "IofxManagerLaunchTask";
	}
	void         run()
	{
		PX_ALWAYS_ASSERT();
	}
	bool         launchInstance(CUstream stream, int kernelIndex)
	{
		return mActor->cudaLaunch(stream, kernelIndex);
	}
	physx::PxGpuTaskHint::Enum getTaskHint() const
	{
		return physx::PxGpuTaskHint::Kernel;
	}

protected:
	IofxManagerGPU* mActor;
};

IofxManagerGPU::IofxManagerGPU(NiApexScene& scene, const NiIofxManagerDesc& desc, IofxManager& mgr)
	: mManager(mgr)
	, mIofxScene(*mgr.mIofxScene)
	, mCopyQueue(*scene.getTaskManager()->getGpuDispatcher())
	, mCuSpawnSeed(scene)
	, mCuBlockPRNGs(scene)
	, mCuSortedActorIDs(scene)
	, mCuSortedStateIDs(scene)
	, mCuSortTempKeys(scene)
	, mCuSortTempValues(scene)
	, mCuSortTemp(scene)
	, mCuMinBounds(scene)
	, mCuMaxBounds(scene)
	, mCuTempMinBounds(scene)
	, mCuTempMaxBounds(scene)
	, mCuTempActorIDs(scene)
	, mCuActorStart(scene)
	, mCuActorEnd(scene)
	, mCuActorVisibleEnd(scene)
	, mCurSeed(0)
	, mTargetBufDevPtr(NULL)
	, mCountActorIDs(0)
	, mNumberVolumes(0)
	, mNumberActorClasses(0)
	, mEmptySimulation(false)
	, mVolumeConstMemGroup(CUDA_OBJ(volumeConstMem))
	, mModifierConstMemGroup(CUDA_OBJ(modifierConstMem))
	, mOutputToBuffer(false)
{
#ifdef APEX_TEST
	const ApexMirroredPlace::Enum defaultPlace = ApexMirroredPlace::CPU_GPU;
#else
	const ApexMirroredPlace::Enum defaultPlace = ApexMirroredPlace::GPU;
#endif

	mTaskLaunch = PX_NEW(IofxManagerLaunchTask)(this);

	const PxU32 maxObjectCount = desc.maxObjectCount;
	const PxU32 maxInStateCount = desc.maxInStateCount;
	PxU32 usageClass = 0;
	PxU32 blockSize = 0;

	if (mManager.mIsMesh)
	{
		usageClass = ModifierUsage_Mesh;
		blockSize = CUDA_OBJ(meshModifiersKernel).getBlockDim().x;
	}
	else
	{
		usageClass = ModifierUsage_Sprite;
		blockSize = CUDA_OBJ(spriteModifiersKernel).getBlockDim().x;
	}

	mCuSpawnSeed.reserve(mManager.mOutStateOffset + maxObjectCount, ApexMirroredPlace::GPU);

	mCuSortedActorIDs.reserve(maxInStateCount, defaultPlace);
	mCuSortedStateIDs.reserve(maxInStateCount, defaultPlace);

	mCuSortTempKeys.reserve(maxInStateCount, ApexMirroredPlace::GPU);
	mCuSortTempValues.reserve(maxInStateCount, ApexMirroredPlace::GPU);
	mCuSortTemp.reserve(32 * 16, ApexMirroredPlace::GPU);

	mCuTempMinBounds.reserve(WARP_SIZE * 2, ApexMirroredPlace::GPU);
	mCuTempMaxBounds.reserve(WARP_SIZE * 2, ApexMirroredPlace::GPU);
	mCuTempActorIDs.reserve(WARP_SIZE * 2, ApexMirroredPlace::GPU);

	// alloc volumeConstMem
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mVolumeConstMemGroup)

		mVolumeParamsArrayHandle.alloc(_storage_);
		mActorIDBitmapArrayHandle.alloc(_storage_);
	}

	// alloc modifierConstMem
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mModifierConstMemGroup)

		mAssetParamsHandleArrayHandle.alloc(_storage_);

		if (mManager.mIsMesh)
		{
			mMeshOutputLayoutHandle.alloc(_storage_);
		}
		else
		{
			mSpriteOutputLayoutHandle.alloc(_storage_);
		}
	}

	InitDevicePRNGs(scene, blockSize, mRandThreadLeap, mRandGridLeap, mCuBlockPRNGs);
}

void IofxManagerGPU::release()
{
	delete this;
}

IofxManagerGPU::~IofxManagerGPU()
{
	delete mTaskLaunch;
}


void IofxManagerGPU::submitTasks()
{
	mNumberActorClasses = mManager.mActorClassTable.size();
	mNumberVolumes = mManager.mVolumeTable.size();
	mCountActorIDs = (mNumberActorClasses >> NiIofxActorID::ASSETS_PER_MATERIAL_BITS) * mNumberVolumes;

	// update volumeConstMem
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mVolumeConstMemGroup)

		VolumeParamsArray& volumeParamsArray = *mVolumeParamsArrayHandle.resolve(_storage_);
		volumeParamsArray.resize(_storage_, mNumberVolumes);
		VolumeParams* volumeParams = volumeParamsArray.getElems(_storage_);

		ActorIDBitmapArray& actorIDBitmapArray = *mActorIDBitmapArrayHandle.resolve(_storage_);
		actorIDBitmapArray.resize(_storage_, mManager.mVolumeActorClassBitmap.size());
		physx::PxU32* actorIDBitmap = actorIDBitmapArray.getElems(_storage_);
		memcpy(actorIDBitmap, &mManager.mVolumeActorClassBitmap.front(), actorIDBitmapArray.getSize() * sizeof(PxU32));

		for (PxU32 i = 0 ; i < mNumberVolumes ; i++)
		{
			IofxManager::VolumeData& vd = mManager.mVolumeTable[ i ];
			if (vd.vol)
			{
				volumeParams[i].bounds = vd.mBounds;
				volumeParams[i].priority = vd.mPri;
			}
			else
			{
				volumeParams[i].bounds.setEmpty();
				volumeParams[i].priority = 0;
			}
		}

	}

	// update modifierConstMem
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mModifierConstMemGroup)

		AssetParamsHandleArray& assetParamsHandleArray = *mAssetParamsHandleArrayHandle.resolve(_storage_);

		assetParamsHandleArray.resize(_storage_, mNumberActorClasses);

		InplaceHandle<AssetParams>* assetParamsHandles = assetParamsHandleArray.getElems(_storage_);
		for (physx::PxU32 i = 0 ; i < mNumberActorClasses ; ++i)
		{
			IofxAssetSceneInstGPU* iofxAssetSceneInstGPU = static_cast<IofxAssetSceneInstGPU*>(mManager.mActorClassTable[i].iofxAssetSceneInst);
			assetParamsHandles[i].setNull();
			if (iofxAssetSceneInstGPU != 0)
			{
				assetParamsHandles[i] = iofxAssetSceneInstGPU->getAssetParamsHandle();
			}
		}

		if (mManager.mIsMesh)
		{
			MeshOutputLayout& meshOutputLayout = *mMeshOutputLayoutHandle.resolve(_storage_);

			IosObjectGpuData* mWorkingData = DYNAMIC_CAST(IosObjectGpuData*)(mManager.mWorkingIosData);
			IofxOutputDataMesh* meshOutputData = DYNAMIC_CAST(IofxOutputDataMesh*)(mWorkingData->outputData);
			IofxSharedRenderDataMesh* meshRenderData = DYNAMIC_CAST(IofxSharedRenderDataMesh*)(mWorkingData->renderData);

			const NxUserRenderInstanceBufferDesc& instanceBufferDesc = 
				mManager.mIsInteropEnabled ? meshRenderData->getInstanceBufferDesc() : meshOutputData->getVertexDesc();

			mOutputDWords = instanceBufferDesc.stride >> 2;
			meshOutputLayout.stride = instanceBufferDesc.stride;
			::memcpy(meshOutputLayout.offsets, instanceBufferDesc.semanticOffsets, sizeof(meshOutputLayout.offsets));
		}
		else
		{
			SpriteOutputLayout& spriteOutputLayout = *mSpriteOutputLayoutHandle.resolve(_storage_);

			IosObjectGpuData* mWorkingData = DYNAMIC_CAST(IosObjectGpuData*)(mManager.mWorkingIosData);
			IofxOutputDataSprite* spriteOutputData = DYNAMIC_CAST(IofxOutputDataSprite*)(mWorkingData->outputData);
			IofxSharedRenderDataSprite* spriteRenderData = DYNAMIC_CAST(IofxSharedRenderDataSprite*)(mWorkingData->renderData);

			const NxUserRenderSpriteBufferDesc& spriteBufferDesc = 
				mManager.mIsInteropEnabled ? spriteRenderData->getSpriteBufferDesc() : spriteOutputData->getVertexDesc();
			
			mOutputDWords = spriteBufferDesc.stride >> 2;
			spriteOutputLayout.stride = spriteBufferDesc.stride;
			::memcpy(spriteOutputLayout.offsets, spriteBufferDesc.semanticOffsets, sizeof(spriteOutputLayout.offsets));
		}
	}

}


#pragma warning(push)
#pragma warning(disable:4312) // conversion from 'CUdeviceptr' to 'PxU32 *' of greater size

PxTaskID IofxManagerGPU::launchGpuTasks()
{
	physx::PxTaskManager* tm = mIofxScene.mApexScene->getTaskManager();
	tm->submitUnnamedTask(*mTaskLaunch, PxTaskType::TT_GPU);
	mTaskLaunch->finishBefore(mManager.mPostUpdateTaskID);
	return mTaskLaunch->getTaskID();
}

void IofxManagerGPU::launchPrep()
{
	IosObjectGpuData* mWorkingData = DYNAMIC_CAST(IosObjectGpuData*)(mManager.mWorkingIosData);

	if (!mWorkingData->numParticles)
	{
		mEmptySimulation = true;
		return;
	}

	mCurSeed = static_cast<PxU32>(mIofxScene.mApexScene->getSeed());
	mTargetBufDevPtr = 0;
	mOutputToBuffer = true;
	mTargetTextureCount = 0;

	physx::PxTaskManager* tm = mIofxScene.mApexScene->getTaskManager();
	physx::PxCudaContextManager* ctx = tm->getGpuDispatcher()->getCudaContextManager();
	if (mManager.mIsInteropEnabled)
	{
		bool isBufferResolved = false;

		if (mWorkingData->renderData->getBufferIsMapped())
		{
			physx::PxScopedCudaLock s(*ctx);

			CUdeviceptr renderableDevicePtr;
			if ( mWorkingData->renderData->resolveResourceList(renderableDevicePtr, mTargetTextureCount, mTargetCuArrayList) )
			{
				mTargetBufDevPtr = reinterpret_cast<PxU32 *>(renderableDevicePtr);
				PX_ASSERT( mTargetBufDevPtr != NULL || mTargetTextureCount > 0 );

				if (!mManager.mIsMesh)
				{
					IofxSharedRenderDataSprite* renderDataSprite = static_cast<IofxSharedRenderDataSprite*>(mWorkingData->renderData);

					//alloc/release TextureBuffers
					for( PxU32 i = 0; i < mTargetTextureCount; ++i ) {
						mTargetTextureBufferList[i].alloc( ctx, renderDataSprite->getTextureDesc(i), mTargetCuArrayList[i] );
					}
					for( PxU32 i = mTargetTextureCount; i < NxUserRenderSpriteBufferDesc::MAX_SPRITE_TEXTURES; ++i ) {
						mTargetTextureBufferList[i].release();
					}
				}

				//we've resolved interop buffer - so disable writeBuffer fall-back
				isBufferResolved = true;
			}
		}
		else if (mWorkingData->renderData->getAllocSemantics() == 0)
		{
			//buffer is not allocated yet - just skip writing to it and skip rendering
			//this happens for the first 2 frames - we don't use writeBuffer fall-back
			isBufferResolved = true;
		}

		mOutputToBuffer = false;

		if (!isBufferResolved)
		{
			//here we are in case interop was broken somehow, so use Shared Render Data
			APEX_INTERNAL_ERROR("IofxManager: CUDA Interop Error - failed to resolve mapped CUDA memory!");
			PX_ASSERT(0);

			//TEMP: disable writeBuffer fallback
#if 0
			mWorkingData->renderData = mManager.mSharedRenderData;
			mOutputToBuffer = true;
#endif
		}
	}

	if (mOutputToBuffer)
	{
		//wait for outputData copy to render resource
		mWorkingData->waitForRenderDataUpdate();

		physx::PxScopedCudaLock s(*ctx);

		mWorkingData->allocOutputs(ctx);

		if (!mManager.mIsMesh)
		{
			IofxOutputDataSprite* spriteOutputData = DYNAMIC_CAST(IofxOutputDataSprite*)(mWorkingData->outputData);
			mTargetTextureCount = spriteOutputData->getTextureCount();

			for( PxU32 i = 0; i < mTargetTextureCount; ++i ) {
				mTargetTextureBufferList[i].alloc( ctx, spriteOutputData->getVertexDesc().textureDescs[i] );
			}
			for( PxU32 i = mTargetTextureCount; i < NxUserRenderSpriteBufferDesc::MAX_SPRITE_TEXTURES; ++i ) {
				mTargetTextureBufferList[i].release();
			}
		}

		if (mTargetTextureCount == 0)
		{
			mTargetOutputBuffer.realloc(mWorkingData->outputData->getDefaultBuffer().getCapacity(), ctx);
			mTargetBufDevPtr = static_cast<PxU32*>( mTargetOutputBuffer.getGpuPtr() );
		}

		if (mWorkingData->outputDWords == 0)
		{
			PX_ALWAYS_ASSERT();
			mEmptySimulation = true;
			return;
		}
	}
	else
	{
		mWorkingData->writeBufferCalled = true;

		mWorkingData->outputData->getDefaultBuffer().release();
		mTargetOutputBuffer.release();
	}

	const physx::PxU32 numActorIDValues = mCountActorIDs + 2;
	mCuActorStart.setSize(numActorIDValues, ApexMirroredPlace::CPU_GPU);
	mCuActorEnd.setSize(numActorIDValues, ApexMirroredPlace::CPU_GPU);
	mCuActorVisibleEnd.setSize(numActorIDValues, ApexMirroredPlace::CPU_GPU);
	mCuMinBounds.setSize(numActorIDValues, ApexMirroredPlace::CPU_GPU);
	mCuMaxBounds.setSize(numActorIDValues, ApexMirroredPlace::CPU_GPU);

#if APEX_TEST
	mCuSortedActorIDs.setSize(mWorkingData->maxStateID, ApexMirroredPlace::CPU_GPU);
	mCuSortedStateIDs.setSize(mWorkingData->maxStateID, ApexMirroredPlace::CPU_GPU);
#endif

	mManager.positionMass.setSize(mWorkingData->maxInputID, ApexMirroredPlace::CPU_GPU);
	mManager.velocityLife.setSize(mWorkingData->maxInputID, ApexMirroredPlace::CPU_GPU);
	mManager.actorIdentifiers.setSize(mWorkingData->maxInputID, ApexMirroredPlace::CPU_GPU);
	mManager.inStateToInput.setSize(mWorkingData->maxStateID, ApexMirroredPlace::CPU_GPU);
	mManager.outStateToInput.setSize(mWorkingData->numParticles, ApexMirroredPlace::CPU_GPU);
	if (mWorkingData->iosSupportsCollision)
	{
		mManager.collisionNormalFlags.setSize(mWorkingData->maxInputID, ApexMirroredPlace::CPU_GPU);
	}
	if (mWorkingData->iosSupportsDensity)
	{
		mManager.density.setSize(mWorkingData->maxInputID, ApexMirroredPlace::CPU_GPU);
	}

	mEmptySimulation = false;
}

#pragma warning(pop)


///
PX_INLINE PxU32 getHighestBitShift(physx::PxU32 x)
{
	PX_ASSERT(isPowerOfTwo(x));
	return highestSetBit(x);
}

void IofxManagerGPU::cudaLaunchRadixSort(CUstream stream, unsigned int numElements, unsigned int keyBits, unsigned int startBit, bool useSyncKernels)
{
	if (useSyncKernels)
	{
		//we use OLD Radix Sort on Tesla (SM < 2), because it is faster
		CUDA_OBJ(radixSortKernel)(
			stream, numElements,
			mCuSortedActorIDs.getGpuPtr(), mCuSortedStateIDs.getGpuPtr(),
			mCuSortTempKeys.getGpuPtr(), mCuSortTempValues.getGpuPtr(),
			mCuSortTemp.getGpuPtr(), keyBits, startBit
		);
	}
	else
	{
#if 1
		//NEW Radix Sort
		if (numElements <= CUDA_OBJ(newRadixSortBlockKernel).getBlockDim().x * SORT_NEW_VECTOR_SIZE)
		{
			//launch just a single block for small sizes
			CUDA_OBJ(newRadixSortBlockKernel)(
				stream, CUDA_OBJ(newRadixSortBlockKernel).getBlockDim().x,
				numElements, keyBits, startBit,
				mCuSortedActorIDs.getGpuPtr(), mCuSortedStateIDs.getGpuPtr()
			);
		}
		else
		{
			unsigned int totalThreads = (numElements + SORT_NEW_VECTOR_SIZE - 1) / SORT_NEW_VECTOR_SIZE;

			for (unsigned int bit = startBit; bit < startBit + keyBits; bit += RADIX_SORT_NBITS)
			{
				int gridSize = 
					CUDA_OBJ(newRadixSortStep1Kernel)(
						stream, totalThreads,
						numElements, bit,
						mCuSortedActorIDs.getGpuPtr(), mCuSortedStateIDs.getGpuPtr(),
						mCuSortTempKeys.getGpuPtr(), mCuSortTempValues.getGpuPtr(),
						mCuSortTemp.getGpuPtr()
					);

				//launch just a single block
				CUDA_OBJ(newRadixSortStep2Kernel)(
					stream, CUDA_OBJ(newRadixSortStep2Kernel).getBlockDim().x,
					numElements, gridSize,
					mCuSortTemp.getGpuPtr()
				);

				CUDA_OBJ(newRadixSortStep3Kernel)(
					stream, totalThreads,
					numElements, bit,
					mCuSortedActorIDs.getGpuPtr(), mCuSortedStateIDs.getGpuPtr(),
					mCuSortTempKeys.getGpuPtr(), mCuSortTempValues.getGpuPtr(),
					mCuSortTemp.getGpuPtr()
				);

				mCuSortedActorIDs.swapGpuPtr(mCuSortTempKeys);
				mCuSortedStateIDs.swapGpuPtr(mCuSortTempValues);
			}
		}
#else
		//OLD Radix Sort
		for (unsigned int startBit = 0; startBit < keyBits; startBit += RADIX_SORT_NBITS)
		{
			int gridSize = 
				CUDA_OBJ(radixSortStep1Kernel)(
					stream, numElements,
					mCuSortedActorIDs.getGpuPtr(), mCuSortedStateIDs.getGpuPtr(),
					mCuSortTempKeys.getGpuPtr(), mCuSortTempValues.getGpuPtr(),
					mCuSortTemp.getGpuPtr(), startBit
				);

			//launch just 1 block
			CUDA_OBJ(radixSortStep2Kernel)(
				stream, CUDA_OBJ(radixSortStep2Kernel).getBlockDim().x,
				mCuSortTemp.getGpuPtr(), gridSize
			);

			CUDA_OBJ(radixSortStep3Kernel)(
				stream, numElements,
				mCuSortedActorIDs.getGpuPtr(), mCuSortedStateIDs.getGpuPtr(),
				mCuSortTempKeys.getGpuPtr(), mCuSortTempValues.getGpuPtr(),
				mCuSortTemp.getGpuPtr(), startBit
			);
		}
#endif
	}
}

bool IofxManagerGPU::cudaLaunch(CUstream stream, int kernelIndex)
{
	physx::PxTaskManager* tm = mIofxScene.mApexScene->getTaskManager();

	if (mEmptySimulation)
	{
		return false;
	}

	const physx::PxU32 numActorIDValues = mCountActorIDs + 2;
	//value <  mCountActorIDs     - valid particle with volume
	//value == mCountActorIDs     - homeless particle (no volume or invalid actor class)
	//value == mCountActorIDs + 1 - NOT_A_PARTICLE


	IofxSceneGPU* sceneGPU = static_cast<IofxSceneGPU*>(&mIofxScene);
	bool useSyncKernels = !sceneGPU->mGpuDispatcher->getCudaContextManager()->supportsArchSM20();

	IosObjectGpuData* mWorkingData = DYNAMIC_CAST(IosObjectGpuData*)(mManager.mWorkingIosData);

	switch (kernelIndex)
	{
	case 0:
		if (mManager.mOnStartCallback)
		{
			(*mManager.mOnStartCallback)(stream);
		}
		mCopyQueue.reset(stream, 14);
		if (mIofxScene.copyDirtySceneData(mCopyQueue))
		{
			/* Inform the GPU dispatcher that no kernels in any stream should be allowed
			 * to launch until the copies issued here have completed
			 */
			mTaskLaunch->requestSyncPoint();
		}
		if (mManager.mCudaIos == true || mWorkingData->maxInputID == 0)
		{
			mCopyQueue.flushEnqueued();

			CUDA_CHECK("IofxManagerGPU::copyToDevice 1");
			break;
		}

		mManager.positionMass.copyHostToDeviceQ(mCopyQueue);
		mManager.velocityLife.copyHostToDeviceQ(mCopyQueue);
		mManager.actorIdentifiers.copyHostToDeviceQ(mCopyQueue);
		mManager.inStateToInput.copyHostToDeviceQ(mCopyQueue);
		if (mWorkingData->iosSupportsCollision)
		{
			mManager.collisionNormalFlags.copyHostToDeviceQ(mCopyQueue);
		}
		if (mWorkingData->iosSupportsDensity)
		{
			mManager.density.copyHostToDeviceQ(mCopyQueue);
		}
		mCopyQueue.flushEnqueued();

		CUDA_CHECK("IofxManagerGPU::copyToDevice 2");
		break;

	case 1:
		/* Volume Migration (input space) */
		CUDA_OBJ(volumeMigrationKernel)(stream,
		                                PxMax(mWorkingData->maxInputID, numActorIDValues),
		                                CUDA_OBJ(volumeConstMem).mappedHandle(mVolumeParamsArrayHandle),
		                                CUDA_OBJ(volumeConstMem).mappedHandle(mActorIDBitmapArrayHandle),
		                                mNumberActorClasses, mNumberVolumes,
		                                mManager.actorIdentifiers.getGpuPtr(), mWorkingData->maxInputID,
		                                (const float4*)mManager.positionMass.getGpuPtr(),
		                                mCuActorStart.getGpuPtr(), mCuActorEnd.getGpuPtr(), mCuActorVisibleEnd.getGpuPtr()
		                               );

		CUDA_CHECK("IofxManagerGPU::volumeMigration");
		break;

	case 2:
		{
			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefRemapPositions,      mManager.positionMass)
			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefRemapActorIDs,       mManager.actorIdentifiers)
			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefRemapInStateToInput, mManager.inStateToInput)

			/* if mDistanceSortingEnabled, sort on camera distance first, else directly make ActorID keys */
			CUDA_OBJ(makeSortKeys)(stream, mWorkingData->maxStateID,
								   mManager.inStateToInput.getGpuPtr(), mWorkingData->maxInputID,
								   mManager.actorIdentifiers.getGpuPtr(), mNumberActorClasses, mCountActorIDs,
								   (const float4*)mManager.positionMass.getGpuPtr(), mManager.mDistanceSortingEnabled,
								   mWorkingData->eyePosition, mWorkingData->eyeDirection, mWorkingData->zNear,
								   mCuSortedActorIDs.getGpuPtr(), mCuSortedStateIDs.getGpuPtr());

			if (mManager.mDistanceSortingEnabled)
			{
				cudaLaunchRadixSort(stream, mWorkingData->maxStateID, 32, 0, useSyncKernels);

				/* Generate ActorID sort keys, using distance sorted stateID values */
				CUDA_OBJ(remapKernel)(stream, mWorkingData->maxStateID,
									  mManager.inStateToInput.getGpuPtr(), mWorkingData->maxInputID,
									  mManager.actorIdentifiers.getGpuPtr(), mNumberActorClasses, mCountActorIDs,
									  mCuSortedStateIDs.getGpuPtr(), mCuSortedActorIDs.getGpuPtr());
			}
		}
		CUDA_CHECK("IofxManagerGPU::makeSortKeys");
		break;

	case 3:
		/* ActorID Sort (output state space) */
		// input: mCuSortedActorIDs == actorIDs, in distance sorted order
		// input: mCuSortedStateIDs == stateIDs, in distance sorted order

		// output: mCuSortedActorIDs == sorted ActorIDs
		// output: mCuSortedStateIDs == output-to-input state
		{
			//SortedActorIDs could contain values from 0 to mCountActorIDs + 1 (included),
			//so keybits should cover at least mCountActorIDs + 2 numbers
			PxU32 keybits = 0;
			while ((1U << keybits) < numActorIDValues)
			{
				++keybits;
			}

			cudaLaunchRadixSort(stream, mWorkingData->maxStateID, keybits, NiIofxActorID::ASSETS_PER_MATERIAL_BITS, useSyncKernels);
		}
		CUDA_CHECK("IofxManagerGPU::radixSort");
		break;

	case 4:
		/* Per-IOFX actor particle range detection */
		CUDA_OBJ(actorRangeKernel)(stream, mWorkingData->numParticles,
		                           mCuSortedActorIDs.getGpuPtr(), mCountActorIDs,
		                           mCuActorStart.getGpuPtr(), mCuActorEnd.getGpuPtr(), mCuActorVisibleEnd.getGpuPtr(),
								   mCuSortedStateIDs.getGpuPtr()
		                          );

		CUDA_CHECK("IofxManagerGPU::actorRange");
		break;

	case 5:
		/* Modifiers (output state space) */
		{
			PX_PROFILER_PERF_SCOPE("IofxManagerGPUModifiers");
			ModifierCommonParams commonParams = mWorkingData->getCommonParams();

			PX_ASSERT(mAssetParamsHandleArrayHandle.resolve(CUDA_OBJ(modifierConstMem))->getSize() == mNumberActorClasses);

			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefPositionMass,     mManager.positionMass)
			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefVelocityLife,     mManager.velocityLife)

			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefInStateToInput,   mManager.inStateToInput)
			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefStateSpawnSeed,   mCuSpawnSeed)

			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefUserData,         mManager.userData)

			if (mWorkingData->iosSupportsCollision)
			{
				CUDA_OBJ(texRefCollisionNormalFlags).bindTo(mManager.collisionNormalFlags);
			}
			if (mWorkingData->iosSupportsDensity)
			{
				CUDA_OBJ(texRefDensity).bindTo(mManager.density);
			}

			PRNGInfo rand;
			rand.g_stateSpawnSeed = mCuSpawnSeed.getGpuPtr();
			rand.g_randBlock = mCuBlockPRNGs.getGpuPtr();
			rand.randGrid = mRandGridLeap;
			rand.randThread = mRandThreadLeap;
			rand.seed = mCurSeed;

			if (mManager.mIsMesh)
			{
				// 3x3 matrix => 9 float scalars => 3 slices

				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefMeshPrivState0, *mManager.privState.slices[0]);
				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefMeshPrivState1, *mManager.privState.slices[1]);
				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefMeshPrivState2, *mManager.privState.slices[2]);

				MeshPrivateStateArgs meshPrivStateArgs;
				meshPrivStateArgs.g_state[0] = mManager.privState.a[0];
				meshPrivStateArgs.g_state[1] = mManager.privState.a[1];
				meshPrivStateArgs.g_state[2] = mManager.privState.a[2];

				CUDA_OBJ(meshModifiersKernel)(MAX_SMEM_BANKS * mOutputDWords, WARP_SIZE * physx::PxMax<int>(mOutputDWords, 4), 
											  stream, mWorkingData->numParticles,
											  mManager.mInStateOffset, mManager.mOutStateOffset,
											  CUDA_OBJ(modifierConstMem).mappedHandle(mAssetParamsHandleArrayHandle),
											  commonParams, mNumberActorClasses * mNumberVolumes,
											  mCuSortedActorIDs.getGpuPtr(), mCuSortedStateIDs.getGpuPtr(),
											  mManager.outStateToInput.getGpuPtr(),
											  meshPrivStateArgs,
											  rand, mTargetBufDevPtr,
											  CUDA_OBJ(modifierConstMem).mappedHandle(mMeshOutputLayoutHandle)
											 );
			}
			else
			{
				// 1 float scalar => 1 slice

				APEX_CUDA_TEXTURE_SCOPE_BIND(texRefSpritePrivState0, *mManager.privState.slices[0]);

				SpritePrivateStateArgs spritePrivStateArgs;
				spritePrivStateArgs.g_state[0] = mManager.privState.a[0];

				//IofxSharedRenderDataSprite* renderDataSprite = static_cast<IofxSharedRenderDataSprite*>(mWorkingData->renderData);
				//IofxOutputDataSprite* outputDataSprite = static_cast<IofxOutputDataSprite*>(mWorkingData->outputData);

				//PxU32 targetTextureCount = mOutputToBuffer ? outputDataSprite->getTextureCount() : renderDataSprite->getTextureCount();
				//const NxUserRenderSpriteTextureDesc* targetTextureDescArray = mOutputToBuffer ? outputDataSprite->getTextureDescArray() : renderDataSprite->getTextureDescArray();

				if (mTargetTextureCount > 0)
				{
					/*
					if (mSpriteData->textureCount > 0) CUDA_OBJ(surfRefOutput0).bindTo( mTargetCuArrayList[0] );
					if (mSpriteData->textureCount > 1) CUDA_OBJ(surfRefOutput1).bindTo( mTargetCuArrayList[1] );
					if (mSpriteData->textureCount > 2) CUDA_OBJ(surfRefOutput2).bindTo( mTargetCuArrayList[2] );
					if (mSpriteData->textureCount > 3) CUDA_OBJ(surfRefOutput3).bindTo( mTargetCuArrayList[3] );
					*/

					SpriteTextureOutputLayout outputLayout;
					outputLayout.textureCount = mTargetTextureCount;
					for (PxU32 i = 0; i < outputLayout.textureCount; ++i)
					{
						outputLayout.textureData[i].layout = static_cast<PxU16>(mTargetTextureBufferList[i].getLayout());

						PxU32 width = mTargetTextureBufferList[i].getWidth();
						PxU32 pitch = mTargetTextureBufferList[i].getPitch();
						//width should be a power of 2 and a multiply of WARP_SIZE
						PX_ASSERT(isPowerOfTwo(width));
						PX_ASSERT(isPowerOfTwo(pitch));
						PX_ASSERT((width & (WARP_SIZE - 1)) == 0);
						outputLayout.textureData[i].widthShift = static_cast<PxU8>(highestSetBit(width));
						outputLayout.textureData[i].pitchShift = static_cast<PxU8>(highestSetBit(pitch));

						outputLayout.texturePtr[i] = mTargetTextureBufferList[i].getPtr();
					}

					CUDA_OBJ(spriteTextureModifiersKernel)(0, WARP_SIZE * 4, 
														   stream, mWorkingData->numParticles,
														   mManager.mInStateOffset, mManager.mOutStateOffset,
														   CUDA_OBJ(modifierConstMem).mappedHandle(mAssetParamsHandleArrayHandle),
														   commonParams, mNumberActorClasses * mNumberVolumes,
														   mCuSortedActorIDs.getGpuPtr(), mCuSortedStateIDs.getGpuPtr(),
														   mManager.outStateToInput.getGpuPtr(),
														   spritePrivStateArgs,
														   rand, outputLayout
														  );

				}
				else
				{
					CUDA_OBJ(spriteModifiersKernel)(MAX_SMEM_BANKS * mOutputDWords, WARP_SIZE * physx::PxMax<int>(mOutputDWords, 4), 
													stream, mWorkingData->numParticles,
													mManager.mInStateOffset, mManager.mOutStateOffset,
													CUDA_OBJ(modifierConstMem).mappedHandle(mAssetParamsHandleArrayHandle),
													commonParams, mNumberActorClasses * mNumberVolumes,
													mCuSortedActorIDs.getGpuPtr(), mCuSortedStateIDs.getGpuPtr(),
													mManager.outStateToInput.getGpuPtr(),
													spritePrivStateArgs,
													rand, mTargetBufDevPtr,
													CUDA_OBJ(modifierConstMem).mappedHandle(mSpriteOutputLayoutHandle)
												   );
				}
			}

			if (mWorkingData->iosSupportsCollision)
			{
				CUDA_OBJ(texRefCollisionNormalFlags).unbind();
			}
			if (mWorkingData->iosSupportsDensity)
			{
				CUDA_OBJ(texRefDensity).unbind();
			}

		}
		CUDA_CHECK("IofxManagerGPU::modifiers");
		break;

	case 6:
		if (mCountActorIDs > 0)
		{
			/* Per-IOFX actor BBox generation */
			APEX_CUDA_TEXTURE_SCOPE_BIND(texRefBBoxPositions, mManager.positionMass)

			if (useSyncKernels)
			{
				CUDA_OBJ(bboxKernel)(
					stream, mWorkingData->numParticles,
					mCuSortedActorIDs.getGpuPtr(),
					mManager.outStateToInput.getGpuPtr(),
					(const float4*)mManager.positionMass.getGpuPtr(),
					(float4*)mCuMinBounds.getGpuPtr(), (float4*)mCuMaxBounds.getGpuPtr(),
					mCuTempActorIDs.getGpuPtr(),
					(float4*)mCuTempMinBounds.getGpuPtr(), (float4*)mCuTempMaxBounds.getGpuPtr()
				);
			}
			else
			{
				int bboxGridSize =
					CUDA_OBJ(bbox1Kernel)(
						stream, mWorkingData->numParticles,
						mCuSortedActorIDs.getGpuPtr(),
						mManager.outStateToInput.getGpuPtr(),
						(const float4*)mManager.positionMass.getGpuPtr(),
						(float4*)mCuMinBounds.getGpuPtr(), (float4*)mCuMaxBounds.getGpuPtr(),
						mCuTempActorIDs.getGpuPtr(),
						(float4*)mCuTempMinBounds.getGpuPtr(), (float4*)mCuTempMaxBounds.getGpuPtr()
					);

				CUDA_OBJ(bbox2Kernel)(
					stream, CUDA_OBJ(bbox2Kernel).getBlockDim().x,
					mCuSortedActorIDs.getGpuPtr(),
					mManager.outStateToInput.getGpuPtr(),
					(const float4*)mManager.positionMass.getGpuPtr(),
					(float4*)mCuMinBounds.getGpuPtr(), (float4*)mCuMaxBounds.getGpuPtr(),
					mCuTempActorIDs.getGpuPtr(),
					(float4*)mCuTempMinBounds.getGpuPtr(), (float4*)mCuTempMaxBounds.getGpuPtr(),
					bboxGridSize
				);
			}
		}
		CUDA_CHECK("IofxManagerGPU::bbox");
		break;

	case 7:
		if (mTargetTextureCount > 0)
		{
			if (mOutputToBuffer)
			{
				IofxOutputDataSprite* spriteOutputData = DYNAMIC_CAST(IofxOutputDataSprite*)(mWorkingData->outputData);
				PX_ASSERT(spriteOutputData->getTextureCount() == mTargetTextureCount);

				for (PxU32 i = 0; i < mTargetTextureCount; ++i)
				{
					mTargetTextureBufferList[i].copyDeviceToHostQ(spriteOutputData->getTextureBuffer(i), mCopyQueue);
				}
			}
			else
			{
				for (PxU32 i = 0; i < mTargetTextureCount; ++i)
				{
					mTargetTextureBufferList[i].copyToArray(mTargetCuArrayList[i], stream, mWorkingData->numParticles);
				}
			}
		}
		else
		{
			if (mOutputToBuffer)
			{
				mTargetOutputBuffer.copyDeviceToHostQ(mWorkingData->outputData->getDefaultBuffer(), mCopyQueue);
			}
		}
		if (mCountActorIDs > 0)
		{
			mCuMinBounds.copyDeviceToHostQ(mCopyQueue);
			mCuMaxBounds.copyDeviceToHostQ(mCopyQueue);
		}
		mCuActorStart.copyDeviceToHostQ(mCopyQueue);
		mCuActorEnd.copyDeviceToHostQ(mCopyQueue);
		mCuActorVisibleEnd.copyDeviceToHostQ(mCopyQueue);

#if APEX_TEST
		{
			mManager.inStateToInput.copyDeviceToHostQ(mCopyQueue);
			mManager.actorIdentifiers.copyDeviceToHostQ(mCopyQueue);
			mManager.outStateToInput.copyDeviceToHostQ(mCopyQueue);
			mManager.positionMass.copyDeviceToHostQ(mCopyQueue);

			mCuSortedActorIDs.copyDeviceToHostQ(mCopyQueue);
			mCuSortedStateIDs.copyDeviceToHostQ(mCopyQueue);
		}
#else
		if (!mManager.mCudaIos)
		{
			mManager.actorIdentifiers.copyDeviceToHostQ(mCopyQueue);
			mManager.outStateToInput.copyDeviceToHostQ(mCopyQueue);
		}
#endif

		mCopyQueue.flushEnqueued();

		if (mManager.mOnFinishCallback)
		{
			(*mManager.mOnFinishCallback)(stream);
		}

		tm->getGpuDispatcher()->addCompletionPrereq(*tm->getTaskFromID(mManager.mPostUpdateTaskID));

		CUDA_CHECK("IofxManagerGPU::copyToHost");
		return false;

	default:
		PX_ALWAYS_ASSERT();
		return false;
	}

	return true;
}

void IofxManagerGPU::fetchResults()
{
	IosObjectGpuData* mWorkingData = DYNAMIC_CAST(IosObjectGpuData*)(mManager.mWorkingIosData);
	PX_UNUSED(mWorkingData);

#ifdef APEX_TEST
	IofxManagerTestData* testData = mManager.mTestData;
	if (testData != NULL)
	{
		testData->mIsGPUTest = true;

		testData->mCountActorIDs = mCountActorIDs;
		testData->mMaxInputID = mWorkingData->maxInputID;
		testData->mMaxStateID = mWorkingData->maxStateID;
		testData->mNumParticles = mWorkingData->numParticles;

		testData->mInStateToInput.resize(mWorkingData->maxStateID);
		testData->mSortedActorIDs.resize(mWorkingData->maxStateID);
		testData->mSortedStateIDs.resize(mWorkingData->maxStateID);

		testData->mOutStateToInput.resize(mWorkingData->numParticles);
		testData->mPositionMass.resize(mWorkingData->numParticles);

		const PxU32 numActorIDValues = mCountActorIDs + 2;
		testData->mMinBounds.resize(numActorIDValues);
		testData->mMaxBounds.resize(numActorIDValues);
		testData->mActorStart.resize(numActorIDValues);
		testData->mActorEnd.resize(numActorIDValues);
		testData->mActorVisibleEnd.resize(numActorIDValues);

		for (PxU32 i = 0; i < mWorkingData->maxStateID; i++)
		{
			testData->mSortedActorIDs[i] = mCuSortedActorIDs[i];
			testData->mSortedStateIDs[i] = mCuSortedStateIDs[i];
			testData->mInStateToInput[i] = mManager.inStateToInput[i];
		}
		for (PxU32 i = 0; i < mWorkingData->numParticles; i++)
		{
			testData->mOutStateToInput[i] = mManager.outStateToInput[i];
			testData->mPositionMass[i] = mManager.positionMass[i];
		}
		for (PxU32 i = 0; i < numActorIDValues; ++i)
		{
			testData->mMinBounds[i] = mCuMinBounds[i];
			testData->mMaxBounds[i] = mCuMaxBounds[i];
			testData->mActorStart[i] = mCuActorStart[i];
			testData->mActorEnd[i] = mCuActorEnd[i];
			testData->mActorVisibleEnd[i] = mCuActorVisibleEnd[i];
		}
	}
#endif

#if 0
	{
		ApexMirroredArray<PxU32> actorID(*mIofxScene.mApexScene);
		ApexMirroredArray<PxVec4> outMinBounds(*mIofxScene.mApexScene);
		ApexMirroredArray<PxVec4> outMaxBounds(*mIofxScene.mApexScene);
		ApexMirroredArray<PxVec4> outDebugInfo(*mIofxScene.mApexScene);
		ApexMirroredArray<PxU32> tmpLastActorID(*mIofxScene.mApexScene);
		tmpLastActorID.setSize(64, ApexMirroredPlace::CPU_GPU);

		const PxU32 NE = 2000;
		actorID.setSize(NE, ApexMirroredPlace::CPU_GPU);

		Array<PxU32> actorCounts;
		actorCounts.reserve(1000);

		PxU32 NA = 0;
		for (PxU32 ie = 0; ie < NE; ++NA)
		{
			PxU32 num_ie = rand(1, 100); // We need to use QDSRand here s.t. seed could be preset during tests!
			PxU32 next_ie = PxMin(ie + num_ie, NE);

			actorCounts.pushBack(next_ie - ie);

			for (; ie < next_ie; ++ie)
			{
				actorID[ie] = NA;
			}
		}
		outMinBounds.setSize(NA, ApexMirroredPlace::CPU_GPU);
		outMaxBounds.setSize(NA, ApexMirroredPlace::CPU_GPU);
		outDebugInfo.setSize(NA, ApexMirroredPlace::CPU_GPU);

		for (PxU32 ia = 0; ia < NA; ++ia)
		{
			outMinBounds[ia].setZero();
			outMaxBounds[ia].setZero();
		}

		physx::PxTaskManager* tm = mIofxScene.mApexScene->getTaskManager();
		physx::PxCudaContextManager* ctx = tm->getGpuDispatcher()->getCudaContextManager();
		physx::PxScopedCudaLock s(*ctx);

		mCopyQueue.reset(0, 4);

		actorID.copyHostToDeviceQ(mCopyQueue);
		outMinBounds.copyHostToDeviceQ(mCopyQueue);
		outMaxBounds.copyHostToDeviceQ(mCopyQueue);
		mCopyQueue.flushEnqueued();

		CUDA_OBJ(bboxKernel2)(0, NE, actorID.getGpuPtr(), NULL, 0, (float4*)outDebugInfo.getGpuPtr(), (float4*)outMinBounds.getGpuPtr(), (float4*)outMaxBounds.getGpuPtr()/*, tmpLastActorID.getGpuPtr()*/);

		outMinBounds.copyDeviceToHostQ(mCopyQueue);
		outMaxBounds.copyDeviceToHostQ(mCopyQueue);
		outDebugInfo.copyDeviceToHostQ(mCopyQueue);
		tmpLastActorID.copyDeviceToHostQ(mCopyQueue);
		mCopyQueue.flushEnqueued();

		CUT_SAFE_CALL(cuCtxSynchronize());

		PxU32 errors = 0;
		PxF32 totCount = 0;
		for (PxU32 ie = 0; ie < NE; ++ie)
		{
			PxU32 id = actorID[ie];
			if (ie == 0 || actorID[ie - 1] != id)
			{
				PxU32 count = actorCounts[id];
				const PxVec4& bounds = outMinBounds[id];
				if (bounds.x != count)
				{
					++errors;
				}
				if (bounds.y != count * 2)
				{
					++errors;
				}
				if (bounds.z != count * 3)
				{
					++errors;
				}
				totCount += count;
			}
		}

	}
#endif

#if 0
	{
		physx::PxTaskManager* tm = mIofxScene.mApexScene->getTaskManager();
		physx::PxCudaContextManager* ctx = tm->getGpuDispatcher()->getCudaContextManager();

		physx::PxScopedCudaLock s(*ctx);

		CUT_SAFE_CALL(cuCtxSynchronize());
	}
#endif

	/* Swap input/output state offsets */
	mManager.swapStates();

	if (mEmptySimulation)
	{
		for (PxU32 i = 0 ; i < mNumberVolumes ; i++)
		{
			IofxManager::VolumeData& d = mManager.mVolumeTable[ i ];
			if (d.vol == 0)
			{
				continue;
			}

			for (PxU32 j = 0 ; j < mNumberActorClasses / NiIofxActorID::ASSETS_PER_MATERIAL ; j++)
			{
				IofxActor* iofx = d.mActors[ j ];
				if (iofx && iofx != DEFERRED_IOFX_ACTOR)
				{
					iofx->mResultBounds.setEmpty();
					iofx->mResultRange.startIndex = 0;
					iofx->mResultRange.objectCount = 0;
					iofx->mResultVisibleCount = 0;
				}
			}
		}
	}
	else
	{
		PX_ASSERT(mCuActorStart.cpuPtrIsValid() && mCuActorEnd.cpuPtrIsValid());
		if (!mCuActorStart.cpuPtrIsValid() || !mCuActorEnd.cpuPtrIsValid())
		{
			// Workaround for issue seen by a customer
			APEX_INTERNAL_ERROR("Bad cpuPtr in IofxManagerGPU::fetchResults");
			return;
		}
//#ifndef NDEBUG
#if 0 // Temporarily commented out by JWR
		//check Actor Ranges
		{
			PxU32 totalCount = 0;
			//range with the last index (= mCountActorIDs) contains homeless particles!
			for (PxU32 i = 0 ; i <= mCountActorIDs ; i++)
			{
				const PxU32 rangeStart = mCuActorStart[ i ];
				const PxU32 rangeEnd = mCuActorEnd[ i ];
				const PxU32 rangeVisibleEnd = mCuActorVisibleEnd[ i ];

				PX_ASSERT(rangeStart < mWorkingData->numParticles);
				PX_ASSERT(rangeEnd <= mWorkingData->numParticles);
				PX_ASSERT(rangeStart <= rangeEnd);
				PX_ASSERT(rangeStart <= rangeVisibleEnd && rangeVisibleEnd <= rangeEnd);

				const PxU32 rangeCount = rangeEnd - rangeStart;
				totalCount += rangeCount;
			}
			PX_ASSERT(totalCount == mWorkingData->numParticles);
		}
#endif

		PxU32 aid = 0;
		for (PxU32 i = 0 ; i < mNumberVolumes ; i++)
		{
			IofxManager::VolumeData& d = mManager.mVolumeTable[ i ];
			if (d.vol == 0)
			{
				aid += mNumberActorClasses / NiIofxActorID::ASSETS_PER_MATERIAL;
				continue;
			}

			for (PxU32 j = 0 ; j < mNumberActorClasses / NiIofxActorID::ASSETS_PER_MATERIAL ; j++)
			{
				const PxU32 rangeStart = mCuActorStart[ aid ];
				const PxU32 rangeEnd = mCuActorEnd[ aid ];
				const PxU32 rangeVisibleEnd = mCuActorVisibleEnd[ aid ];

				const PxU32 rangeCount = rangeEnd - rangeStart;
				const PxU32 visibleCount = rangeVisibleEnd - rangeStart;

				if (d.mActors[ j ] == DEFERRED_IOFX_ACTOR &&
				        (mIofxScene.mModule->mDeferredDisabled || rangeCount))
				{
					const IofxManager::ActorClassData& acd = mManager.mActorClassTable[ j * NiIofxActorID::ASSETS_PER_MATERIAL ];
					PX_ASSERT(acd.renderAsset != 0);
					IofxActor* iofx = PX_NEW(IofxActorGPU)(acd.renderAsset, &mIofxScene, mManager);
					if (d.vol->addIofxActor(*iofx))
					{
						d.mActors[ j ] = iofx;

						iofx->addSelfToContext(mManager);
						iofx->mActorClassID = j;
						iofx->mRenderVolume = d.vol;
						iofx->mSemantics = 0;
						iofx->mDistanceSortingEnabled = false;
						for (PxU32 k = 0; k < NiIofxActorID::ASSETS_PER_MATERIAL; ++k)
						{
							IofxAssetSceneInst* iofxAssetSceneInst = mManager.mActorClassTable[ j * NiIofxActorID::ASSETS_PER_MATERIAL + k ].iofxAssetSceneInst;
							if (iofxAssetSceneInst != 0)
							{
								iofx->mSemantics |= iofxAssetSceneInst->getSemantics();
								iofx->mDistanceSortingEnabled |= iofxAssetSceneInst->getAsset()->isSortingEnabled();
							}
						}

						// lock this renderable because the APEX scene will unlock it after this method is called
						iofx->renderDataLock();
					}
					else
					{
						iofx->release();
					}
				}

				IofxActor* iofx = d.mActors[ j ];
				if (iofx && iofx != DEFERRED_IOFX_ACTOR)
				{
					iofx->mResultBounds.setEmpty();
					if (rangeCount > 0)
					{
						iofx->mResultBounds.minimum = mCuMinBounds[ aid ].getXYZ();
						iofx->mResultBounds.maximum = mCuMaxBounds[ aid ].getXYZ();
					}
					PX_ASSERT(iofx->mRenderBounds.isFinite());
					iofx->mResultRange.startIndex = rangeStart;
					iofx->mResultRange.objectCount = rangeCount;
					iofx->mResultVisibleCount = visibleCount;
				}

				aid++;
			}
		}
	}

}



/**
 * Called from render thread context, just before renderer calls update/dispatch on any IOFX
 * actors.  Map/Unmap render resources as required.  "Mapped" means the graphics buffer has been
 * mapped into our CUDA context where our kernels can write directly into it.
 */
void IofxManager::fillMapUnmapArraysForInterop(physx::Array<CUgraphicsResource> &toMapArray, physx::Array<CUgraphicsResource> &toUnmapArray)
{
	if (mIsInteropEnabled)
	{
		physx::PxGpuDispatcher *gd = mIofxScene->mApexScene->getTaskManager()->getGpuDispatcher();

		mTargetSemanticsLock.lockReader();
		const PxU32 targetSemantics = mTargetSemantics;
		mTargetSemanticsLock.unlockReader();

		if (mResultReadyState == RESULT_WAIT_FOR_INTEROP)
		{
			mResultIosData->updateSemantics(targetSemantics);
			mResultIosData->renderData->alloc( mResultIosData, gd->getCudaContextManager() );
			mResultIosData->renderData->addToMapArray(toMapArray);

			mStagingIosData->updateSemantics(targetSemantics);
			mStagingIosData->renderData->alloc( mStagingIosData, gd->getCudaContextManager() );
			mStagingIosData->renderData->addToMapArray(toMapArray);
		}
		else if (mResultReadyState == RESULT_READY)
		{
			PX_ASSERT(mResultIosData->renderData->getBufferIsMapped());
			mResultIosData->renderData->addToUnmapArray(toUnmapArray);

			mStagingIosData->updateSemantics(targetSemantics);
			mStagingIosData->renderData->alloc( mStagingIosData, gd->getCudaContextManager() );
			mStagingIosData->renderData->addToMapArray(toMapArray);
		}
	}
}


void IofxManager::mapBufferResults(bool mapSuccess, bool unmapSuccess)
{
	if (mIsInteropEnabled)
	{
		if (mResultReadyState == RESULT_WAIT_FOR_INTEROP)
		{
			if (mapSuccess)
			{
				mResultIosData->renderData->onMapSuccess();
				mStagingIosData->renderData->onMapSuccess();

				mResultReadyState = RESULT_INTEROP_READY;
			}
		}
		else if (mResultReadyState == RESULT_READY)
		{
			if (unmapSuccess)
			{
				mResultIosData->renderData->onUnmapSuccess();
			}
			if (mapSuccess)
			{
				mStagingIosData->renderData->onMapSuccess();
			}
		}
	}
}

}
}
} // namespace physx::apex

#endif
