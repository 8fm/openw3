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
#include "PsArray.h"
#include "ApexInterface.h"
#include "NiApexScene.h"
#include "ModuleIofx.h"
#include "IofxManager.h"
#include "IofxScene.h"
#include "IofxAsset.h"
#include "IosObjectData.h"
#include "IofxRenderData.h"

#include "IofxActorCPU.h"

#ifdef APEX_TEST
#include "IofxManagerTestData.h"
#endif

#ifdef APEX_CUDA_SUPPORT
#include "ApexCuda.h" // APEX_CUDA_MEM_ALIGN_UP_32BIT
#include "IofxManagerGPU.h"
#include "ApexMirroredArray.h"
#endif

#define BASE_SPRITE_SEMANTICS (1<<NxRenderSpriteSemantic::POSITION) | \
	(1<<NxRenderSpriteSemantic::VELOCITY) | \
	(1<<NxRenderSpriteSemantic::LIFE_REMAIN)

#define BASE_MESH_SEMANTICS   (1<<NxRenderInstanceSemantic::POSITION) | \
	(1<<NxRenderInstanceSemantic::ROTATION_SCALE) | \
	(1<<NxRenderInstanceSemantic::VELOCITY_LIFE)

namespace physx
{
namespace apex
{
namespace iofx
{

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

IofxAssetSceneInst::IofxAssetSceneInst(IofxAsset* asset, PxU32 assetID, PxU32 semantics)
{
	_asset = asset;
	_assetID = assetID;
	_semantics = semantics;
}

IofxAssetSceneInst::~IofxAssetSceneInst()
{
}


IofxManager::IofxManager(IofxScene& scene, const NiIofxManagerDesc& desc, bool isMesh)
	: mNextAssetID(0)
	, mPostUpdateTaskID(0)
	, mIofxScene(&scene)
	, mIosAssetName(desc.iosAssetName)
	, mWorkingIosData(NULL)
	, mResultIosData(NULL)
	, mStagingIosData(NULL)
	, mRenderIosData(NULL)
	, mTargetSemantics(0)
	, mIsInteropEnabled(false)
	, positionMass(*scene.mApexScene, NV_ALLOC_INFO("positionMass", PARTICLES))
	, velocityLife(*scene.mApexScene, NV_ALLOC_INFO("velocityLife", PARTICLES))
	, collisionNormalFlags(*scene.mApexScene, NV_ALLOC_INFO("collisionNormalFlags", PARTICLES))
	, density(*scene.mApexScene, NV_ALLOC_INFO("density", PARTICLES))
	, actorIdentifiers(*scene.mApexScene, NV_ALLOC_INFO("actorIdentifiers", PARTICLES))
	, inStateToInput(*scene.mApexScene, NV_ALLOC_INFO("inStateToInput", PARTICLES))
	, outStateToInput(*scene.mApexScene, NV_ALLOC_INFO("outStateToInput", PARTICLES))
	, userData(*scene.mApexScene, NV_ALLOC_INFO("userData", PARTICLES))
	, pubStateSize(0)
	, privStateSize(0)
	, mStateSwap(false)
	, mTotalElapsedTime(0)
	, mIsMesh(isMesh)
	, mDistanceSortingEnabled(false)
	, mCudaIos(desc.iosOutputsOnDevice)
	, mCudaModifiers(false)
	, mCudaPipeline(NULL)
	, mSimulateTask(*this)
	, mCopyQueue(*scene.mApexScene->getTaskManager()->getGpuDispatcher())
	, mSimulatedParticlesCount(0)
	, mOnStartCallback(NULL)
	, mOnFinishCallback(NULL)
#ifdef APEX_TEST
	, mTestData(NULL)
#endif
{
	scene.mActorManagers.add(*this);

	mBounds.setEmpty();

	mInStateOffset = 0;
	mOutStateOffset = desc.maxObjectCount;

	// The decision whether to use GPU IOFX Modifiers is separate from whether the IOS
	// outputs will come from the GPU or not
#if defined(APEX_CUDA_SUPPORT)
	physx::PxGpuDispatcher* gd = scene.mApexScene->getTaskManager()->getGpuDispatcher();
	if (gd && gd->getCudaContextManager()->contextIsValid() && !scene.mModule->mCudaDisabled)
	{
		mCudaModifiers = true;
		// detect interop
		mIsInteropEnabled = (gd->getCudaContextManager()->getInteropMode() != PxCudaInteropMode::NO_INTEROP) && !scene.mModule->mInteropDisabled;
		const PxU32 dataCount = (mIsInteropEnabled ? 3 : 2);
		for (PxU32 i = 0 ; i < dataCount ; i++)
		{
			IofxOutputData* outputData = mIsMesh ? static_cast<IofxOutputData*>(PX_NEW(IofxOutputDataMesh)()) : PX_NEW(IofxOutputDataSprite)();
			IosObjectGpuData* gpuIosData = PX_NEW(IosObjectGpuData)(i, outputData);
			mObjData.pushBack(gpuIosData);
		}

		mOutStateOffset = APEX_CUDA_MEM_ALIGN_UP_32BIT(desc.maxObjectCount);
		mCudaPipeline = PX_NEW(IofxManagerGPU)(*mIofxScene->mApexScene, desc, *this);
	}
	else
#endif
	{
		for (PxU32 i = 0 ; i < 2 ; i++)
		{
			IofxOutputData* outputData = mIsMesh ? static_cast<IofxOutputData*>(PX_NEW(IofxOutputDataMesh)()) : PX_NEW(IofxOutputDataSprite)();
			IosObjectCpuData* cpuIosData = PX_NEW(IosObjectCpuData)(i, outputData);
			mObjData.pushBack(cpuIosData);
		}
	}

	mWorkingIosData = mObjData[0];
	mResultIosData = mObjData[1];

	// Create & Assign Shared Render Data
	if (mIsMesh)
	{
		mSharedRenderData = PX_NEW(IofxSharedRenderDataMesh)(0);
	}
	else
	{
		mSharedRenderData = PX_NEW(IofxSharedRenderDataSprite)(0);
	}

#if defined(APEX_CUDA_SUPPORT)
	if (mIsInteropEnabled)
	{
		mResultReadyState = RESULT_WAIT_FOR_INTEROP;

		mStagingIosData = mObjData[2];

		mInteropRenderData.resize( mObjData.size() );
		for (PxU32 i = 0 ; i < mObjData.size() ; i++)
		{
			IofxSharedRenderData* renderData;
			if (mIsMesh)
			{
				renderData = PX_NEW(IofxSharedRenderDataMesh)(i + 1);
			}
			else
			{
				renderData = PX_NEW(IofxSharedRenderDataSprite)(i + 1);
			}
			renderData->setUseInterop(true);

			mInteropRenderData[i] = renderData;
			mObjData[i]->renderData = renderData;
		}
	}
	else
#endif
	{
		mResultReadyState = RESULT_WAIT_FOR_NEW;
		for (PxU32 i = 0 ; i < mObjData.size() ; i++)
		{
			mObjData[i]->renderData = mSharedRenderData;
		}
	}

	ApexMirroredPlace::Enum place = ApexMirroredPlace::CPU;
#if defined(APEX_CUDA_SUPPORT)
	if (mCudaIos || mCudaModifiers)
	{
		place =  ApexMirroredPlace::CPU_GPU;
	}
#endif
	{
		positionMass.setSize(desc.maxInputCount, place);
		velocityLife.setSize(desc.maxInputCount, place);
		if (desc.iosSupportsCollision)
		{
			collisionNormalFlags.setSize(desc.maxInputCount, place);
		}
		if (desc.iosSupportsDensity)
		{
			density.setSize(desc.maxInputCount, place);
		}
		actorIdentifiers.setSize(desc.maxInputCount, place);
		inStateToInput.setSize(desc.maxInStateCount, place);
		outStateToInput.setSize(desc.maxObjectCount, place);

		userData.setSize(desc.maxInputCount, place);

		mSimBuffers.pmaPositionMass = &positionMass;
		mSimBuffers.pmaVelocityLife = &velocityLife;
		mSimBuffers.pmaCollisionNormalFlags = desc.iosSupportsCollision ? &collisionNormalFlags : NULL;
		mSimBuffers.pmaDensity = desc.iosSupportsDensity ? &density : NULL;
		mSimBuffers.pmaActorIdentifiers = &actorIdentifiers;
		mSimBuffers.pmaInStateToInput = &inStateToInput;
		mSimBuffers.pmaOutStateToInput = &outStateToInput;
		mSimBuffers.pmaUserData = &userData;
	}

	if (!mCudaModifiers)
	{
		mOutputToState.resize(desc.maxObjectCount);
	}

	/* Initialize IOS object data structures */
	for (PxU32 i = 0 ; i < mObjData.size() ; i++)
	{
		mObjData[i]->pmaPositionMass = mSimBuffers.pmaPositionMass;
		mObjData[i]->pmaVelocityLife = mSimBuffers.pmaVelocityLife;
		mObjData[i]->pmaCollisionNormalFlags = mSimBuffers.pmaCollisionNormalFlags;
		mObjData[i]->pmaDensity = mSimBuffers.pmaDensity;
		mObjData[i]->pmaActorIdentifiers = mSimBuffers.pmaActorIdentifiers;
		mObjData[i]->pmaInStateToInput = mSimBuffers.pmaInStateToInput;
		mObjData[i]->pmaOutStateToInput = mSimBuffers.pmaOutStateToInput;
		mObjData[i]->pmaUserData = mSimBuffers.pmaUserData;

		mObjData[i]->iosAssetName = desc.iosAssetName;
		mObjData[i]->iosOutputsOnDevice = desc.iosOutputsOnDevice;
		mObjData[i]->iosSupportsDensity = desc.iosSupportsDensity;
		mObjData[i]->iosSupportsCollision = desc.iosSupportsCollision;
		mObjData[i]->maxObjectCount = desc.maxObjectCount;
		mObjData[i]->maxInputCount = desc.maxInputCount;
		mObjData[i]->maxInStateCount = desc.maxInStateCount;
	}
}

IofxManager::~IofxManager()
{
	for (PxU32 i = 0; i < pubState.slices.size(); ++i)
	{
		delete pubState.slices[i];
	}

	for (PxU32 i = 0; i < privState.slices.size(); ++i)
	{
		delete privState.slices[i];
	}
}

void IofxManager::destroy()
{
#if defined(APEX_CUDA_SUPPORT)
	if (mCudaPipeline)
	{
		mCudaPipeline->release();
	}
	for (PxU32 i = 0 ; i < mInteropRenderData.size() ; i++)
	{
		PX_DELETE(mInteropRenderData[i]);
	}
#endif
	if (mSharedRenderData != NULL)
	{
		PX_DELETE(mSharedRenderData);
	}
	for (PxU32 i = 0 ; i < mObjData.size() ; i++)
	{
		PX_DELETE(mObjData[i]);
	}

	delete this;
}


void IofxManager::release()
{
	mIofxScene->releaseIofxManager(this);
}

#if !defined(APEX_CUDA_SUPPORT)
/* Stubs for console builds */
void IofxManager::fillMapUnmapArraysForInterop(physx::Array<CUgraphicsResource> &, physx::Array<CUgraphicsResource> &) {}
void IofxManager::mapBufferResults(bool, bool) {}
#endif

void IofxManager::prepareRenderResources()
{
	if (mResultReadyState != RESULT_READY)
	{
		mRenderIosData = NULL;
		return;
	}
	mResultReadyState = RESULT_WAIT_FOR_NEW;

	mRenderIosData = mResultIosData;
#if defined(APEX_CUDA_SUPPORT)
	if (mIsInteropEnabled)
	{
		physx::swap(mStagingIosData, mResultIosData);

		if (mRenderIosData->renderData->getBufferIsMapped())
		{
			APEX_INTERNAL_ERROR("IofxManager: CUDA Interop Error - render data is still mapped to CUDA memory!");
			PX_ASSERT(0);
		}
	}
	else
#endif
	{
		mRenderIosData->renderData->alloc(mRenderIosData, NULL);
	}

	bool bNeedUpdate = false;

	// mLiveRenderVolumesLock is allready locked in IofxScene::prepareRenderResources
	for (PxU32 i = 0 ; i < mIofxScene->mLiveRenderVolumes.size() ; i++)
	{
		ApexRenderVolume* vol = mIofxScene->mLiveRenderVolumes[i];
		// all render volumes are allready locked in IofxScene::prepareRenderResources

		PxU32 iofxActorCount;
		NxIofxActor* const* iofxActorList = vol->getIofxActorList(iofxActorCount);
		for (PxU32 iofxActorIndex = 0; iofxActorIndex < iofxActorCount; ++iofxActorIndex)
		{
			IofxActor* iofxActor = DYNAMIC_CAST(IofxActor*)( iofxActorList[iofxActorIndex] );
			if (&iofxActor->mMgr == this)
			{
				bNeedUpdate |= iofxActor->prepareRenderResources(mRenderIosData);
			}
		}
	}

	if (bNeedUpdate)
	{
		mRenderIosData->prepareRenderDataUpdate();
	}
}

void IofxManager::postPrepareRenderResources()
{
	if (mRenderIosData != NULL)
	{
		mRenderIosData->executeRenderDataUpdate();
		mRenderIosData = NULL;
	}
}


PxF32 IofxManager::getObjectRadius() const
{
	return mObjData[0] ? mObjData[0]->radius : 0.0f;
}

void IofxManager::setSimulationParameters(PxF32 radius, const PxVec3& up, PxF32 gravity, PxF32 restDensity)
{
	/* Initialize IOS object data structures */
	for (PxU32 i = 0 ; i < mObjData.size() ; i++)
	{
		mObjData[i]->radius = radius;
		mObjData[i]->upVector = up;
		mObjData[i]->gravity = gravity;
		mObjData[i]->restDensity = restDensity;
	}
}

void IofxManager::createSimulationBuffers(NiIosBufferDesc& outDesc)
{
	outDesc = mSimBuffers;
}

/* Called by owning IOS actor during simulation startup, only if
 * the IOS is going to simulate this frame, so it is safe to submit
 * tasks from here. postUpdateTaskID is the ID for an IOS task
 * that should run after IOFX modifiers.  If the IOFX Manager adds
 * no dependencies, postUpdateTaskID task will run right after
 * updateEffectsData() returns.  If updateEffectsData() will be completely
 * synchronous, it is safe to return 0 here.
 */
PxTaskID IofxManager::getUpdateEffectsTaskID(PxTaskID postUpdateTaskID)
{
	physx::PxTaskManager* tm = mIofxScene->mApexScene->getTaskManager();
	mPostUpdateTaskID = postUpdateTaskID;
	if (mCudaModifiers)
	{
		return mCudaPipeline->launchGpuTasks();
	}
	else
	{
		tm->submitUnnamedTask(mSimulateTask);
		mSimulateTask.finishBefore(tm->getNamedTask(AST_PHYSX_FETCH_RESULTS));
		return mSimulateTask.getTaskID();
	}
}


void TaskUpdateEffects::run()
{
	setProfileStat((PxU16) mOwner.mWorkingIosData->numParticles);
	mOwner.cpuModifiers();
}

/// \brief Called by IOS actor before TaskUpdateEffects is scheduled to run
void IofxManager::updateEffectsData(PxF32 deltaTime, PxU32 numObjects, PxU32 maxInputID, PxU32 maxStateID)
{
	PX_PROFILER_PLOT((PxU32)numObjects, "IofxManagerUpdateEffectsData");

	PX_ASSERT(maxStateID >= maxInputID && maxInputID >= numObjects);

	mSimulatedParticlesCount = numObjects;

	if (mCudaIos && !mCudaModifiers)
	{
#if defined(APEX_CUDA_SUPPORT)
		/* Presumably, updateEffectsData() is being called from a DtoH GPU task */
		mCopyQueue.reset(0, 8);
		positionMass.copyDeviceToHostQ(mCopyQueue);
		velocityLife.copyDeviceToHostQ(mCopyQueue);
		if (collisionNormalFlags.getSize() > 0)
		{
			collisionNormalFlags.copyDeviceToHostQ(mCopyQueue);
		}
		if (density.getSize() > 0)
		{
			density.copyDeviceToHostQ(mCopyQueue);
		}
		actorIdentifiers.copyDeviceToHostQ(mCopyQueue);
		inStateToInput.copyDeviceToHostQ(mCopyQueue);
		mCopyQueue.flushEnqueued();

		mIofxScene->mApexScene->getTaskManager()->getGpuDispatcher()->addCompletionPrereq(mSimulateTask);
#else
		PX_ALWAYS_ASSERT();
#endif
	}

	/* Data from the IOS */
	mWorkingIosData->maxInputID = maxInputID;
	mWorkingIosData->maxStateID = maxStateID;
	mWorkingIosData->numParticles = numObjects;

	/* Data from the scene */
	mWorkingIosData->eyePosition = mIofxScene->mApexScene->getEyePosition();
	mWorkingIosData->eyeDirection = mIofxScene->mApexScene->getEyeDirection();

	PxMat44 viewMtx = mIofxScene->mApexScene->getViewMatrix();
	PxMat44 projMtx = mIofxScene->mApexScene->getProjMatrix();
	mWorkingIosData->eyeAxisX = PxVec3(viewMtx.column0.x, viewMtx.column1.x, viewMtx.column2.x);
	mWorkingIosData->eyeAxisY = PxVec3(viewMtx.column0.y, viewMtx.column1.y, viewMtx.column2.y);
	mWorkingIosData->zNear = physx::PxAbs(projMtx.column3.z / projMtx.column2.z);

	mWorkingIosData->deltaTime = deltaTime;
	// TODO: Convert into PxU32 elapsed milliseconds
	mTotalElapsedTime = numObjects ? mTotalElapsedTime + mWorkingIosData->deltaTime : 0;
	mWorkingIosData->elapsedTime = mTotalElapsedTime;

	/* IOFX data */
	mWorkingIosData->writeBufferCalled = false;

	if (mCudaModifiers)
	{
		mCudaPipeline->launchPrep(); // calls allocOutputs
	}
	else
	{
		//wait for outputData copy to render resource
		mWorkingIosData->waitForRenderDataUpdate();

		mWorkingIosData->allocOutputs();
	}
}

void IofxManager::cpuModifiers()
{
	if (mOnStartCallback)
	{
		(*mOnStartCallback)(NULL);
	}
	PxU32 maxInputID, maxStateID, numObjects;

	maxInputID = mWorkingIosData->maxInputID;
	maxStateID = mWorkingIosData->maxStateID;
	numObjects = mWorkingIosData->numParticles;

	PX_UNUSED(numObjects);

	/* Swap state buffer pointers */

	IosObjectCpuData* md = DYNAMIC_CAST(IosObjectCpuData*)(mWorkingIosData);

	md->inPubState = mStateSwap ? &pubState.a[0] : &pubState.b[0];
	md->outPubState = mStateSwap ? &pubState.b[0] : &pubState.a[0];

	md->inPrivState = mStateSwap ? &privState.a[0] : &privState.b[0];
	md->outPrivState = mStateSwap ? &privState.b[0] : &privState.a[0];

	swapStates();

	/* Sort sprites */

	if (!mIsMesh)
	{
		DYNAMIC_CAST(IosObjectCpuData*)(mWorkingIosData)->sortingKeys =
			mDistanceSortingEnabled ? &mSortingKeys.front() : NULL;
	}

	/* Volume Migration (1 pass) */

	mCountPerActor.clear();
	mCountPerActor.resize(mActorClassTable.size() * mVolumeTable.size() / NiIofxActorID::ASSETS_PER_MATERIAL, 0);
	for (PxU32 input = 0 ; input < maxInputID ; input++)
	{
		NiIofxActorID& id = mWorkingIosData->pmaActorIdentifiers->get(input);
		if (id.getActorClassID() == NiIofxActorID::INV_ACTOR || id.getActorClassID() >= mActorClassTable.size())
		{
			id.set(NiIofxActorID::NO_VOLUME, NiIofxActorID::INV_ACTOR);
		}
		else
		{
			const PxVec3& pos = mWorkingIosData->pmaPositionMass->get(input).getXYZ();
			PxU32 curPri = 0;
			PxU16 curVID = NiIofxActorID::NO_VOLUME;

			for (PxU16 i = 0 ; i < mVolumeTable.size() ; i++)
			{
				VolumeData& vd = mVolumeTable[ i ];
				if (vd.vol == NULL)
				{
					continue;
				}

				const PxU32 bit = mActorClassTable.size() * i + id.getActorClassID();

				// This volume owns this particle if:
				//  1. The volume bounds contain the particle
				//  2. The volume affects the particle's IOFX Asset
				//  3. This volume has the highest priority or was the previous owner
				if (vd.mBounds.contains(pos) &&
				    (mVolumeActorClassBitmap[ bit >> 5 ] & (1u << (bit & 31))) &&
				    (curVID == NiIofxActorID::NO_VOLUME || vd.mPri > curPri || (vd.mPri == curPri && id.getVolumeID() == i)))
				{
					curVID = i;
					curPri = vd.mPri;
				}
			}

			id.setVolumeID(curVID);
		}

		// Count particles in each actor
		if (id.getVolumeID() != NiIofxActorID::NO_VOLUME)
		{
			const PxU32 aid = ((id.getVolumeID() * mActorClassTable.size()) + id.getActorClassID()) / NiIofxActorID::ASSETS_PER_MATERIAL;
			++mCountPerActor[aid];
		}
	}

	/* Prefix sum */
	mStartPerActor.clear();
	mStartPerActor.resize(mCountPerActor.size(), 0);
	PxU32 sum = 0;
	for (PxU32 i = 0 ; i < mStartPerActor.size() ; i++)
	{
		mStartPerActor[ i ] = sum;
		sum += mCountPerActor[ i ];
	}

	IosObjectCpuData* objData = DYNAMIC_CAST(IosObjectCpuData*)(mWorkingIosData);
	objData->outputToState = &mOutputToState.front();

	/* Generate outputToState (1 pass) */
	mBuildPerActor.clear();
	mBuildPerActor.resize(mStartPerActor.size(), 0);
	PxU32 homeless = 0;
	for (PxU32 state = 0 ; state < maxStateID ; state++)
	{
		PxU32 input = objData->pmaInStateToInput->get(state);
		if (input == NiIosBufferDesc::NOT_A_PARTICLE)
		{
			continue;
		}

		input &= ~NiIosBufferDesc::NEW_PARTICLE_FLAG;

		const NiIofxActorID id = objData->pmaActorIdentifiers->get(input);
		if (id.getVolumeID() == NiIofxActorID::NO_VOLUME)
		{
			objData->pmaOutStateToInput->get(sum + homeless) = input;
			++homeless;
		}
		else
		{
			PX_ASSERT(id.getActorClassID() != NiIofxActorID::INV_ACTOR && id.getActorClassID() < mActorClassTable.size());
			const PxU32 aid = ((id.getVolumeID() * mActorClassTable.size()) + id.getActorClassID()) / NiIofxActorID::ASSETS_PER_MATERIAL;
			objData->outputToState[ mStartPerActor[aid] + mBuildPerActor[ aid ]++ ] = state;
		}
	}

	/* Step IOFX Actors */
	PxU32 aid = 0;
	PxTaskManager* tm = mIofxScene->mApexScene->getTaskManager();
	for (PxU32 i = 0 ; i < mVolumeTable.size() ; i++)
	{
		VolumeData& d = mVolumeTable[ i ];
		if (d.vol == 0)
		{
			aid += mActorClassTable.size() / NiIofxActorID::ASSETS_PER_MATERIAL;
			continue;
		}

		for (PxU32 j = 0 ; j < mActorClassTable.size() / NiIofxActorID::ASSETS_PER_MATERIAL; j++)
		{
			if (d.mActors[ j ] == DEFERRED_IOFX_ACTOR &&
			        (mIofxScene->mModule->mDeferredDisabled || mCountPerActor[ aid ]))
			{
				const ActorClassData& acd = mActorClassTable[ j * NiIofxActorID::ASSETS_PER_MATERIAL ];
				PX_ASSERT(acd.renderAsset != 0);
				IofxActor* iofxActor = PX_NEW(IofxActorCPU)(acd.renderAsset, mIofxScene, *this);
				if (d.vol->addIofxActor(*iofxActor))
				{
					d.mActors[ j ] = iofxActor;

					iofxActor->addSelfToContext(*this);
					iofxActor->mActorClassID = j;
					iofxActor->mRenderVolume = d.vol;
					iofxActor->mSemantics = 0;
					iofxActor->mDistanceSortingEnabled = false;
					for (PxU32 k = 0; k < NiIofxActorID::ASSETS_PER_MATERIAL; ++k)
					{
						IofxAssetSceneInst* iofxAssetSceneInst = mActorClassTable[ j * NiIofxActorID::ASSETS_PER_MATERIAL + k ].iofxAssetSceneInst;
						if (iofxAssetSceneInst != 0)
						{
							iofxActor->mSemantics |= iofxAssetSceneInst->getSemantics();
							iofxActor->mDistanceSortingEnabled |= iofxAssetSceneInst->getAsset()->isSortingEnabled();
						}
					}
				}
				else
				{
					iofxActor->release();
				}
			}

			IofxActorCPU* cpuIofx = DYNAMIC_CAST(IofxActorCPU*)(d.mActors[ j ]);
			if (cpuIofx && cpuIofx != DEFERRED_IOFX_ACTOR)
			{
				if (mCountPerActor[ aid ])
				{
					ObjectRange range;
					range.objectCount = mCountPerActor[ aid ];
					range.startIndex = mStartPerActor[ aid ];
					PX_ASSERT(range.startIndex + range.objectCount <= numObjects);

					cpuIofx->mWorkingRange = range;

					cpuIofx->mModifierTask.setContinuation(*tm, tm->getTaskFromID(mPostUpdateTaskID));
					cpuIofx->mModifierTask.removeReference();
				}
				else
				{
					cpuIofx->mWorkingVisibleCount = 0;
					cpuIofx->mWorkingRange.objectCount = 0;
					cpuIofx->mWorkingRange.startIndex = 0;
					cpuIofx->mWorkingBounds.setEmpty();
				}
			}

			aid++;
		}
	}

	if (mOnFinishCallback)
	{
		(*mOnFinishCallback)(NULL);
	}
#if APEX_TEST
	if (mTestData != NULL)
	{
		mTestData->mIsCPUTest = true;
		mTestData->mOutStateToInput.resize(objData->pmaOutStateToInput->getSize());
		mTestData->mInStateToInput.resize(objData->pmaInStateToInput->getSize());
		for (PxU32 i = 0; i < objData->pmaOutStateToInput->getSize(); i++)
		{
			mTestData->mOutStateToInput[i] = objData->pmaOutStateToInput->get(i);
		}
		for (PxU32 i = 0; i < objData->pmaInStateToInput->getSize(); i++)
		{
			mTestData->mInStateToInput[i] = objData->pmaInStateToInput->get(i);
		}
		mTestData->mMaxInputID = objData->maxInputID;
		mTestData->mMaxStateID = objData->maxStateID;
		mTestData->mNumParticles = objData->numParticles;

		mTestData->mCountPerActor.resize(mCountPerActor.size());
		mTestData->mStartPerActor.resize(mStartPerActor.size());
		for (PxU32 i = 0; i < mCountPerActor.size(); i++)
		{
			mTestData->mCountPerActor[i] = mCountPerActor[i];
		}
		for (PxU32 i = 0; i < mStartPerActor.size(); i++)
		{
			mTestData->mStartPerActor[i] = mStartPerActor[i];
		}
	}
#endif

}

void IofxManager::outputHostToDevice()
{
	if (mCudaIos && !mCudaModifiers)
	{
#if defined(APEX_CUDA_SUPPORT)
		physx::PxTaskManager* tm = mIofxScene->mApexScene->getTaskManager();
		physx::PxCudaContextManager* ctx = tm->getGpuDispatcher()->getCudaContextManager();
		physx::PxScopedCudaLock s(*ctx);

		mCopyQueue.reset(0, 2);
		actorIdentifiers.copyHostToDeviceQ(mCopyQueue);
		outStateToInput.copyHostToDeviceQ(mCopyQueue);
		mCopyQueue.flushEnqueued();
#else
		PX_ALWAYS_ASSERT();
#endif
	}
}


void IofxManager::submitTasks()
{
	/* Discover new volumes, removed volumes */
	for (PxU32 i = 0 ; i < mVolumeTable.size() ; i++)
	{
		mVolumeTable[ i ].mFlags = 0;
	}

	for (PxU32 i = 0 ; i < mIofxScene->mLiveRenderVolumes.size() ; i++)
	{
		getVolumeID(mIofxScene->mLiveRenderVolumes[ i ]);
	}

	for (PxU32 i = 0 ; i < mVolumeTable.size() ; i++)
	{
		if (mVolumeTable[ i ].mFlags == 0)
		{
			mVolumeTable[ i ].vol = 0;
		}
	}

	/* Trim Volume and ActorClassID tables */
	while (mVolumeTable.size() && mVolumeTable.back().vol == 0)
	{
		mVolumeTable.popBack();
	}

	PxI32 lastValidAsset = -1;
	for (PxU32 cur = 0; cur < mActorClassTable.size(); cur += NiIofxActorID::ASSETS_PER_MATERIAL)
	{
		if (mActorClassTable[ cur ].renderAsset != NULL)
		{
			lastValidAsset = (PxI32) cur;
		}
	}
	if (lastValidAsset == -1)
	{
		mActorClassTable.clear();
	}
	else
	{
		mActorClassTable.resize(PxU32(lastValidAsset) + NiIofxActorID::ASSETS_PER_MATERIAL);
	}

	const physx::PxU32 volumeActorClassBitmapSize = (mVolumeTable.size() * mActorClassTable.size() + 31) >> 5;
	mVolumeActorClassBitmap.resize(volumeActorClassBitmapSize);
	for (PxU32 i = 0 ; i < volumeActorClassBitmapSize ; i++)
	{
		mVolumeActorClassBitmap[ i ] = 0;
	}

	/* Add new IofxActors as necessary */
	for (PxU32 i = 0 ; i < mVolumeTable.size() ; i++)
	{
		VolumeData& d = mVolumeTable[ i ];

		// First, ensure per-volume actor array can hold all ClassIDs
		d.mActors.resize(mActorClassTable.size() / NiIofxActorID::ASSETS_PER_MATERIAL, 0);

		if (d.vol == NULL)
		{
			continue;
		}

		d.mBounds = d.vol->getOwnershipBounds();
		d.mPri = d.vol->getPriority();

		d.vol->renderDataLock(); // for safety during affectsIofxAsset() calls

		for (PxU32 cur = 0; cur < mActorClassTable.size(); cur += NiIofxActorID::ASSETS_PER_MATERIAL)
		{
			if (mActorClassTable[ cur ].renderAsset != NULL)
			{
				const PxU32 actorIdx = cur / NiIofxActorID::ASSETS_PER_MATERIAL;
				if (!d.mActors[ actorIdx ])
				{
					d.mActors[ actorIdx ] = DEFERRED_IOFX_ACTOR;
				}
				for (PxU32 j = 0; j < NiIofxActorID::ASSETS_PER_MATERIAL; ++j)
				{
					const ActorClassData& acd = mActorClassTable[ cur + j ];
					if (acd.iofxAssetSceneInst != NULL)
					{
						IofxAsset* iofxAsset = acd.iofxAssetSceneInst->getAsset();
						if (iofxAsset && d.vol->affectsIofxAsset(*iofxAsset))
						{
							const PxU32 bit = mActorClassTable.size() * i + (cur + j);
							mVolumeActorClassBitmap[ bit >> 5 ] |= (1u << (bit & 31));
						}
					}
				}
			}
		}

		d.vol->renderDataUnLock(); // for safety during affectsIofxAsset() calls
	}

	PxU32 targetSemantics = 0;
	mDistanceSortingEnabled = false;
	{
		for (AssetHashMap_t::Iterator it = mAssetHashMap.getIterator(); !it.done(); ++it)
		{
			IofxAsset* iofxAsset = it->first;
			IofxAssetSceneInst* iofxAssetSceneInst = it->second;

			targetSemantics |= iofxAssetSceneInst->getSemantics();
			if (!mDistanceSortingEnabled && iofxAsset->isSortingEnabled())
			{
				mDistanceSortingEnabled = true;
				if (!mCudaModifiers)
				{
					mSortingKeys.resize(mOutputToState.size());
				}
			}
		}
	}

	mTargetSemanticsLock.lockWriter();
	mTargetSemantics = targetSemantics;
	mTargetSemanticsLock.unlockWriter();

	if (!mIsInteropEnabled)
	{
		mWorkingIosData->updateSemantics(targetSemantics);
	}

	if (mCudaModifiers)
	{
		mCudaPipeline->submitTasks();
	}

	if (!addedAssets.empty())
	{
		/* Calculate state sizes required by new assets */
		PxU32 newPubStateSize = 0, newPrivStateSize = 0;
		for (PxU32 i = 0; i < addedAssets.size(); ++i)
		{
			newPubStateSize = PxMax(newPubStateSize, addedAssets[i]->getPubStateSize());
			newPrivStateSize = PxMax(newPrivStateSize, addedAssets[i]->getPrivStateSize());
		}

		PxU32 maxObjectCount = outStateToInput.getSize(),
			totalCount = mOutStateOffset + maxObjectCount;

		// Allocate data for pubstates
		while (newPubStateSize > pubStateSize)
		{
			pubStateSize += sizeof(IofxSlice);

			SliceArray* slice = new SliceArray(*mIofxScene->mApexScene, NV_ALLOC_INFO("slice", PARTICLES));

#if defined(APEX_CUDA_SUPPORT)
			if (mCudaModifiers)
			{
				//slice->reserve(totalCount, ApexMirroredPlace::GPU); Recalculated on GPU
			}
			else
#endif
			{
				slice->reserve(totalCount, ApexMirroredPlace::CPU);
			}

			pubState.slices.pushBack(slice);

			IofxSlice* p;
#if defined(APEX_CUDA_SUPPORT)
			p = mCudaModifiers
				? pubState.slices.back()->getGpuPtr()
				: pubState.slices.back()->getPtr();
#else
			p = pubState.slices.back()->getPtr();
#endif
			pubState.a.pushBack(p + mInStateOffset);
			pubState.b.pushBack(p + mOutStateOffset);
		}

		// Allocate data for privstates
		while (newPrivStateSize > privStateSize)
		{
			privStateSize += sizeof(IofxSlice);

			SliceArray* slice = new SliceArray(*mIofxScene->mApexScene, NV_ALLOC_INFO("slice", PARTICLES));

#if defined(APEX_CUDA_SUPPORT)
			if (mCudaModifiers)
			{
				slice->reserve(totalCount, ApexMirroredPlace::GPU);
			}
			else
#endif
			{
				slice->reserve(totalCount, ApexMirroredPlace::CPU);
			}

			privState.slices.pushBack(slice);

			IofxSlice* p;
#if defined(APEX_CUDA_SUPPORT)
			p = mCudaModifiers 
				? privState.slices.back()->getGpuPtr()
				: privState.slices.back()->getPtr();
#else
			p = privState.slices.back()->getPtr();
#endif
			privState.a.pushBack(p + mInStateOffset);
			privState.b.pushBack(p + mOutStateOffset);
		}

		addedAssets.clear();
	}
}

void IofxManager::swapStates()
{
	mStateSwap = !mStateSwap;
	swap(mInStateOffset, mOutStateOffset);
}

void IofxManager::fetchResults()
{
	if (!mPostUpdateTaskID)
	{
		return;
	}
	mPostUpdateTaskID = 0;

	if (mCudaModifiers)
	{
		mCudaPipeline->fetchResults();
	}
	else
	{
		for (PxU32 i = 0 ; i < mVolumeTable.size() ; i++)
		{
			VolumeData& d = mVolumeTable[ i ];
			for (PxU32 j = 0 ; j < mActorClassTable.size() / NiIofxActorID::ASSETS_PER_MATERIAL ; j++)
			{
				IofxActorCPU* cpuIofx = DYNAMIC_CAST(IofxActorCPU*)(d.mActors[ j ]);
				if (cpuIofx && cpuIofx != DEFERRED_IOFX_ACTOR)
				{
					cpuIofx->mResultBounds = cpuIofx->mWorkingBounds;
					cpuIofx->mResultRange = cpuIofx->mWorkingRange;
					cpuIofx->mResultVisibleCount = cpuIofx->mWorkingVisibleCount;
				}
			}
		}
	}

	//build bounds
	{
		mBounds.setEmpty();
		for (PxU32 i = 0 ; i < mVolumeTable.size() ; i++)
		{
			VolumeData& d = mVolumeTable[ i ];
			for (PxU32 j = 0 ; j < mActorClassTable.size() / NiIofxActorID::ASSETS_PER_MATERIAL ; j++)
			{
				IofxActor* iofx = d.mActors[ j ];
				if (iofx && iofx != DEFERRED_IOFX_ACTOR)
				{
					mBounds.include(iofx->mResultBounds);
				}
			}
		}
	}

	//swap ObjectData
	switch (mResultReadyState)
	{
	case RESULT_WAIT_FOR_NEW:
	case RESULT_READY:
		physx::swap(mResultIosData, mWorkingIosData);
		mResultReadyState = RESULT_READY;
		break;
	case RESULT_WAIT_FOR_INTEROP:
		break;
	case RESULT_INTEROP_READY:
		physx::swap(mStagingIosData, mWorkingIosData);
		mResultReadyState = RESULT_WAIT_FOR_NEW;
		break;
	};
}

PxBounds3 IofxManager::getBounds() const
{
	return mBounds;
}

PxU16 IofxManager::getActorClassID(NxIofxAsset* asset, PxU16 meshID)
{
	IofxAsset* iofxAsset = static_cast<IofxAsset*>(asset);

	NxApexAsset* renderAsset = NULL;
	if (mIsMesh)
	{
		const char* rmName = iofxAsset->getMeshAssetName(meshID);
		bool isOpaqueMesh = iofxAsset->isOpaqueMesh(meshID);
		renderAsset = iofxAsset->mRenderMeshAssetTracker.getMeshAssetFromName(rmName, isOpaqueMesh);
		if (renderAsset == NULL)
		{
			APEX_INVALID_PARAMETER("IofxManager: ApexRenderMeshAsset with name \"%s\" not found.", rmName);
			return NiIofxActorID::INV_ACTOR;
		}
	}
	else
	{
		const char* mtlName = iofxAsset->getSpriteMaterialName();
		renderAsset = iofxAsset->mSpriteMaterialAssetTracker.getAssetFromName(mtlName);
		if (renderAsset == NULL)
		{
			APEX_INVALID_PARAMETER("IofxManager: SpriteMaterial with name \"%s\" not found.", mtlName);
			return NiIofxActorID::INV_ACTOR;
		}
	}

	bool found = false;
	PxU32 pos = PxU32(-1);
	for (PxU32 cur = 0; cur < mActorClassTable.size(); cur += NiIofxActorID::ASSETS_PER_MATERIAL)
	{
		const ActorClassData& d = mActorClassTable[ cur ];
		if (d.renderAsset == renderAsset)
		{
			found = true;
			pos = cur;
			break;
		}

		if (pos == PxU32(-1) && d.renderAsset == NULL)
		{
			pos = cur;
		}
	}

	if (!found)
	{
		if (pos == PxU32(-1))
		{
			/* Asset is not in table, append it */
			pos = mActorClassTable.size();
			if (pos + NiIofxActorID::ASSETS_PER_MATERIAL > NiIofxActorID::INV_ACTOR)
			{
				APEX_INVALID_OPERATION("IofxManager: out of space for ActorClassID.");
				return NiIofxActorID::INV_ACTOR;
			}
			mActorClassTable.resize(pos + NiIofxActorID::ASSETS_PER_MATERIAL);
		}

		/* init entries for material/mesh */
		for (PxU16 i = 0 ; i < NiIofxActorID::ASSETS_PER_MATERIAL ; i++)
		{
			ActorClassData& d = mActorClassTable[ pos + i ];

			d.renderAsset = renderAsset;
			d.iofxAssetSceneInst = NULL;
		}
	}

	PxU32 result = PxU32(-1);
	for (PxU16 i = 0 ; i < NiIofxActorID::ASSETS_PER_MATERIAL ; i++)
	{
		ActorClassData& d = mActorClassTable[ pos + i ];
		if (d.iofxAssetSceneInst != NULL && d.iofxAssetSceneInst->getAsset() == iofxAsset)
		{
			return PxU16(pos + i);
		}
		if (result == -1 && d.iofxAssetSceneInst == NULL)
		{
			result = pos + i;
		}
	}

	if (result == PxU32(-1))
	{
		APEX_INVALID_OPERATION("IofxManager: too many IofxAssets with the same material/mesh.");
		return NiIofxActorID::INV_ACTOR;
	}

	ActorClassData& d = mActorClassTable[ result ];

	IofxAssetSceneInst* &iofxAssetSceneInst = mAssetHashMap[iofxAsset];
	if (iofxAssetSceneInst == NULL)
	{
		iofxAssetSceneInst = createAssetSceneInst(asset);

		// Update state sizes later in submitTasks
		addedAssets.pushBack(iofxAsset);
	}

	d.iofxAssetSceneInst = iofxAssetSceneInst;
	d.iofxAssetSceneInst->addActorClassId(PxU16(result));
	return PxU16(result);
}

void IofxManager::releaseAssetID(NxIofxAsset* asset)
{
	// TODO: free unused memory in states

	IofxAsset* iofxAsset = static_cast<IofxAsset*>(asset);

	const AssetHashMap_t::Entry* assetEntry = mAssetHashMap.find(iofxAsset);
	if (assetEntry == NULL)
	{
		APEX_INVALID_PARAMETER("IofxManager: releaseAssetID is called with not registered asset.");
		return;
	}
	IofxAssetSceneInst* iofxAssetSceneInst = assetEntry->second;
	const Array<PxU16>& actorClassIdList = iofxAssetSceneInst->getActorClassIdList();

	for (PxU32 i = 0; i < actorClassIdList.size(); ++i)
	{
		const PxU32 actorClassId = actorClassIdList[i];
		const PxU32 actorClassId0 = actorClassId - (actorClassId & (NiIofxActorID::ASSETS_PER_MATERIAL - 1));

		PX_ASSERT(mActorClassTable[actorClassId].iofxAssetSceneInst == iofxAssetSceneInst);
		mActorClassTable[actorClassId].iofxAssetSceneInst = NULL;

		PxU32 assetCount = 0;
		for (PxU32 id = actorClassId0; id < actorClassId0 + NiIofxActorID::ASSETS_PER_MATERIAL; ++id)
		{
			if (mActorClassTable[id].iofxAssetSceneInst != NULL)
			{
				++assetCount;
			}
		}
		if (assetCount == 0)
		{
			const PxU32 actorIdx = (actorClassId0 >> NiIofxActorID::ASSETS_PER_MATERIAL_BITS);
			for (PxU16 j = 0 ; j < mVolumeTable.size() ; j++)
			{
				if (!mVolumeTable[ j ].vol)
				{
					continue;
				}

				if (mVolumeTable[ j ].mActors.size() > actorIdx)
				{
					IofxActor* iofx = mVolumeTable[ j ].mActors[ actorIdx ];
					if (iofx && iofx != DEFERRED_IOFX_ACTOR)
					{
						iofx->release();
						//IofxManager::removeActorAtIndex should zero the actor in mActors
						PX_ASSERT(mVolumeTable[ j ].mActors[ actorIdx ] == 0);
					}
					mVolumeTable[ j ].mActors[ actorIdx ] = 0;
				}
			}

			/* reset entries for material/mesh */
			for (PxU32 id = actorClassId0; id < actorClassId0 + NiIofxActorID::ASSETS_PER_MATERIAL; ++id)
			{
				mActorClassTable[id].renderAsset = NULL;
			}
			/* renderAsset will be released by material/mesh ApexAssetTracker in IofxAsset */
		}
	}

	mAssetHashMap.erase(iofxAsset);

	// release IofxAssetSceneInst
	releaseAssetSceneInst(iofxAssetSceneInst);
}

PxU16 IofxManager::getVolumeID(NxApexRenderVolume* vol)
{
	PxI32 hole = -1;
	for (PxU16 i = 0 ; i < mVolumeTable.size() ; i++)
	{
		if (vol == mVolumeTable[ i ].vol)
		{
			mVolumeTable[ i ].mFlags = 1;
			return i;
		}
		else if (hole == -1 && !mVolumeTable[ i ].vol)
		{
			hole = (PxI32) i;
		}
	}
	if (hole == -1)
	{
		mVolumeTable.insert();
		hole = mVolumeTable.size() - 1;
	}
	VolumeData& d = mVolumeTable[ hole ];
	d.vol = DYNAMIC_CAST(ApexRenderVolume*)(vol);
	d.mFlags = 1;
	d.mActors.clear(); //Iofx Actors are released in ApexRenderVolume destructor!
	return (PxU16) hole;
}


void IofxManager::removeActorAtIndex(PxU32 index)
{
	IofxActor* iofx = DYNAMIC_CAST(IofxActor*)(mActorArray[ index ]);

	for (PxU32 i = 0 ; i < mVolumeTable.size() ; i++)
	{
		if (mVolumeTable[ i ].vol == iofx->mRenderVolume)
		{
			PX_ASSERT(iofx == mVolumeTable[ i ].mActors[ iofx->mActorClassID ]);
			mVolumeTable[ i ].mActors[ iofx->mActorClassID ] = 0;
		}
	}

	ApexContext::removeActorAtIndex(index);
}


IofxAssetSceneInst* IofxManager::createAssetSceneInst(NxIofxAsset* nxAsset)
{
	IofxAsset* asset = DYNAMIC_CAST(IofxAsset*)( nxAsset );

	PxU32 semantics = mIsMesh ? BASE_MESH_SEMANTICS : BASE_SPRITE_SEMANTICS;
	if( mObjData[0]->iosSupportsDensity ) {
		semantics |= mIsMesh ? (PxU32)NxRenderInstanceSemantic::DENSITY : (PxU32)NxRenderSpriteSemantic::DENSITY;
	}
	semantics |= mIsMesh ? asset->getMeshSemanticsBitmap() : asset->getSpriteSemanticsBitmap();

	IofxAssetSceneInst* iofxAssetSceneInst = 0;
#if defined(APEX_CUDA_SUPPORT)
	if (mCudaModifiers)
	{
		iofxAssetSceneInst = mCudaPipeline->createAssetSceneInst(asset, mNextAssetID++, semantics);
	}
	else
#endif
	{
		iofxAssetSceneInst = PX_NEW(IofxAssetSceneInstCPU)(asset, mNextAssetID++, semantics, mIofxScene);
	}
	PX_ASSERT(iofxAssetSceneInst != 0);
	return iofxAssetSceneInst;
}

void IofxManager::releaseAssetSceneInst(IofxAssetSceneInst* iofxAssetSceneInst)
{
	PX_ASSERT(iofxAssetSceneInst != 0);
	PX_DELETE(iofxAssetSceneInst);
}

#ifdef APEX_TEST
IofxManagerTestData* IofxManager::createTestData()
{
	mTestData = new IofxManagerTestData();
	return mTestData;
}

void IofxManager::copyTestData() const
{
	if (mTestData == NULL)
	{
		return;
	}

	mTestData->mNOT_A_PARTICLE = NiIosBufferDesc::NOT_A_PARTICLE;
	mTestData->mNEW_PARTICLE_FLAG = NiIosBufferDesc::NEW_PARTICLE_FLAG;
	mTestData->mACTOR_ID_START_BIT = NiIofxActorID::ASSETS_PER_MATERIAL_BITS;
	mTestData->mSTATE_ID_MASK = STATE_ID_MASK;
}
void IofxManager::clearTestData()
{
	mTestData->mInStateToInput.reset();
	mTestData->mOutStateToInput.reset();
	mTestData->mCountPerActor.reset();
	mTestData->mStartPerActor.reset();
	mTestData->mPositionMass.reset();
	mTestData->mSortedActorIDs.reset();
	mTestData->mSortedStateIDs.reset();
	mTestData->mActorStart.reset();
	mTestData->mActorEnd.reset();
	mTestData->mActorVisibleEnd.reset();
	mTestData->mMinBounds.reset();
	mTestData->mMaxBounds.reset();
}
#endif

}
}
} // end namespace physx::apex
