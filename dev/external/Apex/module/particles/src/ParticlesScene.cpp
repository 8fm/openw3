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
#include "PsShare.h"
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

#include "NxApex.h"
#include "ParticlesScene.h"
#include "NiApexScene.h"
#include "ModulePerfScope.h"
#include "ModuleParticles.h"
#include "PxScene.h"
#include "EffectPackageActor.h"
#include "ApexEmitterAssetParameters.h"
#include "NxApexEmitterAsset.h"
#include "NxApexEmitterActor.h"

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

namespace physx
{


namespace apex
{

namespace particles
{

ParticlesScene::ParticlesScene(ModuleParticles& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list)
	: mRenderDebug(renderDebug)
	, mUpdateTask(*this)
	, mSimTime(0)
	, mCheckTime(0)
{
	mModule = &module;
	mApexScene = &scene;
	mPhysXScene = NULL;

	list.add(*this);		// Add self to module's list of ParticlesScenes

	/* Initialize reference to ParticlesDebugRenderParams */
	mDebugRenderParams = DYNAMIC_CAST(DebugRenderParams*)(mApexScene->getDebugRenderParams());
	PX_ASSERT(mDebugRenderParams);
	NxParameterized::Handle handle(*mDebugRenderParams), memberHandle(*mDebugRenderParams);
	int size;

	if (mDebugRenderParams->getParameterHandle("moduleName", handle) == NxParameterized::ERROR_NONE)
	{
		handle.getArraySize(size, 0);
		handle.resizeArray(size + 1);
		if (handle.getChildHandle(size, memberHandle) == NxParameterized::ERROR_NONE)
		{
			memberHandle.initParamRef(ParticlesDebugRenderParams::staticClassName(), true);
		}
	}

	/* Load reference to ParticlesDebugRenderParams */
	NxParameterized::Interface* refPtr = NULL;
	memberHandle.getParamRef(refPtr);
	mParticlesDebugRenderParams = DYNAMIC_CAST(ParticlesDebugRenderParams*)(refPtr);
	PX_ASSERT(mParticlesDebugRenderParams);
}

ParticlesScene::~ParticlesScene()
{
}

// Called by scene task graph between LOD and PhysX::simulate()
void ParticlesScene::TaskUpdate::run()
{
#if 1
	setProfileStat((PxU16) mOwner.mActorArray.size());
	physx::PxF32 dt = mOwner.mApexScene->getElapsedTime();
	mOwner.updateActors(dt);

#endif
}

void ParticlesScene::updateFromSimulate(physx::PxF32 dt)
{
	PX_UNUSED(dt);
}

// Called by updateTask between LOD and PhysX simulate.  Any writes
// to render data must be protected by acquiring the actor's render data lock
void ParticlesScene::updateActors(physx::PxF32 dt)
{
	PX_PROFILER_PERF_SCOPE("ParticlesScene::updateActors");

	//_ASSERTE (dt <= 1.0f /60.0f);
	SCOPED_PHYSX_LOCK_WRITE(*mApexScene);
	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		ParticlesBase* db = static_cast< ParticlesBase*>(mActorArray[i]);
		switch (db->getParticlesType())
		{
		case ParticlesBase::DST_EFFECT_PACKAGE_ACTOR:
		{
			EffectPackageActor* actor = static_cast<EffectPackageActor*>(db);
			actor->updateParticles(dt);
		}
		break;
		}
	}

}

// submit the task that updates the dynamicsystem actors
// called from ApexScene::simulate()
void ParticlesScene::submitTasks(PxF32 elapsedTime, PxF32 /*substepSize*/, PxU32 /*numSubSteps*/)
{
	PX_UNUSED(elapsedTime);
#if 1
	mSimTime += elapsedTime;
	mCheckTime += elapsedTime;
	physx::PxTaskManager* tm = mApexScene->getTaskManager();
	tm->submitUnnamedTask(mUpdateTask);
	mUpdateTask.startAfter(tm->getNamedTask(AST_LOD_COMPUTE_BENEFIT));
	mUpdateTask.finishBefore(tm->getNamedTask(AST_PHYSX_SIMULATE));
#else
	updateActors(elapsedTime);
#endif
}

void ParticlesScene::fetchResults()
{

}

// Called by ApexScene::fetchResults() with all actors render data locked.
void ParticlesScene::fetchResultsPostRenderUnlock()
{
	PX_PROFILER_PERF_SCOPE("ParticlesSceneFetchResults");

	bool screenCulling = mModule->getEnableScreenCulling();
	bool znegative = mModule->getZnegative();

	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		ParticlesBase* db = static_cast< ParticlesBase*>(mActorArray[i]);

		switch (db->getParticlesType())
		{
		case ParticlesBase::DST_EFFECT_PACKAGE_ACTOR:
		{
			EffectPackageActor* actor = static_cast< EffectPackageActor*>(db);
			actor->updatePoseAndBounds(screenCulling, znegative);
		}
		break;
		}
	}

	//

	if (!mEmitterPool.empty())
	{
		if (mCheckTime > 1)   // only check once every second
		{
			mCheckTime = 0;
			EmitterPool* source = &mEmitterPool[0];
			EmitterPool* dest = source;
			PxU32 incount = mEmitterPool.size();
			PxU32 outcount = 0;
			for (PxU32 i = 0; i < incount; i++)
			{
				bool alive = source->process(mSimTime);
				if (!alive)   // there must be at least one emitter sharing the same asset!
				{
					alive = true; // by default we cannot delete it unless there is at least once emitter in the pool sharing the same IOS
					EmitterPool* scan = &mEmitterPool[0];
					for (PxU32 i = 0; i < outcount; i++)
					{
						if (scan != source)
						{
							if (source->mEmitter->getEmitterAsset() == scan->mEmitter->getEmitterAsset())
							{
								alive = false;
								break;
							}
						}
						scan++;
					}
				}
				if (alive)
				{
					*dest = *source;
					dest++;
					outcount++;
				}
				else
				{
					source->releaseEmitter();
				}
				source++;
			}
			if (outcount != incount)
			{
				mEmitterPool.resize(outcount);
			}
		}
	}
	//
}

void ParticlesScene::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mParticlesDebugRenderParams->VISUALIZE_HEAT_SOURCE_ACTOR &&  !mParticlesDebugRenderParams->VISUALIZE_EFFECT_PACKAGE_ACTOR)
	{
		return;
	}
	mRenderDebug->pushRenderState();
	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		ParticlesBase* db = static_cast< ParticlesBase*>(mActorArray[i]);
		switch (db->getParticlesType())
		{
		case ParticlesBase::DST_EFFECT_PACKAGE_ACTOR:
			if (mParticlesDebugRenderParams->VISUALIZE_EFFECT_PACKAGE_ACTOR)
			{
				EffectPackageActor* hsa = static_cast< EffectPackageActor*>(db);
				hsa->visualize(mRenderDebug, false);
			}
			break;
		}
	}
	mRenderDebug->popRenderState();
#endif
}

void ParticlesScene::destroy()
{
	removeAllActors();
	resetEmitterPool();
	mApexScene->moduleReleased(*this);
	delete this;
}

#if NX_SDK_VERSION_MAJOR == 3
void ParticlesScene::setModulePhysXScene(PxScene* nxScene)
{
	mPhysXScene = nxScene;
}
#else
void ParticlesScene::setModulePhysXScene(NxScene* nxScene)
{
	mPhysXScene = nxScene;
}
#endif

void ParticlesScene::resetEmitterPool()
{
	for (PxU32 i = 0; i < mEmitterPool.size(); i++)
	{
		mEmitterPool[i].releaseEmitter();
	}
	mEmitterPool.clear();
}

void ParticlesScene::addToEmitterPool(NxApexEmitterActor* emitterActor)
{
	EmitterPool ep(emitterActor, mSimTime);
	emitterActor->stopEmit();
	mEmitterPool.pushBack(ep);
}

NxApexEmitterActor* ParticlesScene::getEmitterFromPool(NxApexEmitterAsset* asset)
{
	NxApexEmitterActor* ret = NULL;

	PxU32 ecount = mEmitterPool.size();
	if (ecount)
	{
		for (PxU32 i = 0; i < ecount; i++)
		{
			if (mEmitterPool[i].mEmitter->getEmitterAsset() == asset)
			{
				ret = mEmitterPool[i].mEmitter;
				for (PxU32 j = i + 1; j < ecount; j++)
				{
					mEmitterPool[j - 1] = mEmitterPool[j];
				}
				mEmitterPool.resize(ecount - 1);
				break;
			}
		}
	}
	return ret;
}

bool EmitterPool::process(PxF32 simTime)
{
	bool ret = true;
	if (simTime > mEmitterTime)
	{
		mEmitterTime = simTime + EMITTER_FORGET_TIME;
		ret = false;
	}
	return ret;
}

void EmitterPool::releaseEmitter()
{
	PX_ASSERT(mEmitter);
	if (mEmitter)
	{
		mEmitter->release();
		mEmitter = NULL;
	}
}

EmitterPool::EmitterPool(NxApexEmitterActor* emitterActor, PxF32 simTime)
{
	mEmitter = emitterActor;
	mEmitterTime = simTime + EMITTER_FORGET_TIME;
}

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
