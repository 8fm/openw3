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
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

#include "NxApex.h"
#include "NiApexScene.h"
#include "ModuleDestructible.h"
#include "DestructibleScene.h"
#include "DestructibleAsset.h"
#include "DestructibleActor.h"
#include "DestructibleActorProxy.h"
#include "DestructibleActorJointProxy.h"
#if APEX_USE_PARTICLES
#include "NxApexEmitterActor.h"
#include "NxEmitterGeoms.h"
#endif

#if NX_SDK_VERSION_MAJOR == 2
#include <NxScene.h>
#include <NxConvexMeshDesc.h>
#include <NxConvexShapeDesc.h>
#elif NX_SDK_VERSION_MAJOR == 3
#include "PsArray.h"
#include "PxScene.h"
#include "PxConvexMeshDesc.h"
#include "PxConvexMeshGeometry.h"
#include "PxBoxGeometry.h"
#include "PxSphereGeometry.h"
#endif

#if APEX_RUNTIME_FRACTURE
#include "SimScene.h"
#include "Actor.h"
#endif

#include "ApexRand.h"
#include "ModulePerfScope.h"
#include "PsString.h"
#include "PsTime.h"

#include "NiApexRenderMeshAsset.h"

#if APEX_USE_GRB
#if NX_SDK_VERSION_MAJOR == 2
#include "GrbScene.h"
#include "GrbSceneDesc.h"
#else
#include "GrbScene3.h"
#include "GrbPhysics3.h"
#include "GrbSceneDesc3.h"
#endif
#endif


namespace physx
{
namespace apex
{

#if NX_SDK_VERSION_MAJOR == 2
PX_INLINE NxMat34 NxFromPxMat34(const physx::PxTransform& in)
{
	physx::PxMat34Legacy temp1(in);
	NxMat34 temp2;
	NxFromPxMat34(temp2, temp1);
	return temp2;
}
#elif NX_SDK_VERSION_MAJOR == 3
PX_INLINE const physx::PxTransform& NxFromPxMat34(const physx::PxTransform& in)
{
	return in;
}
#endif

namespace destructible
{
namespace
{
	void unfortunateCompilerWorkaround(physx::PxU32)
	{
	}
};

#if NX_SDK_VERSION_MAJOR == 2
class OverlapSphereShapesReport : public NxUserEntityReport< NxShape* >
{
public:

	OverlapSphereShapesReport(const DestructibleScene* inScene) : scene(inScene) {}
	virtual ~OverlapSphereShapesReport() {}

	bool onEvent(physx::PxU32 numHits, NxShape** hits)
	{
		NxShape** stop = hits + numHits;
		while (hits < stop)
		{
			NxShape* shape = *hits++;
			if (scene->mModule->owns((NxActor*)&shape->getActor()))	// BRG OPTIMIZE
			{
				DestructibleStructure::Chunk* chunk = scene->getChunk(shape);	// BRG OPTIMIZE
				if (chunk != NULL)
				{
					destructibles.pushBack(scene->mDestructibles.direct(chunk->destructibleID));
				}
			}
		}
		return false;
	}

	const DestructibleScene*		scene;
	physx::Array<DestructibleActor*>	destructibles;
};
#endif //NX_SDK_VERSION_MAJOR == 2

PX_INLINE physx::PxVec3 transformToScreenSpace(const physx::PxVec3& pos, const physx::PxF32* viewProjectionMatrix)
{
	const physx::PxF32 v[4] = { pos.x, pos.y, pos.z, 1.0f };

	physx::PxF32 u[4] = { 0, 0, 0, 0 };

	const physx::PxF32* row = viewProjectionMatrix;
	for (physx::PxU32 i = 0; i < 4; ++i, ++row)
	{
		for (physx::PxU32 j = 0; j < 4; ++j)
		{
			u[i] += row[j << 2] * v[j];
		}
	}

	const physx::PxF32 recipW = 1.0f / u[3];

	return physx::PxVec3(u[0] * recipW, u[1] * recipW, u[2] * recipW);
}

// Put an actor to sleep based upon input velocities.
PX_INLINE void handleSleeping(NxActor* actor, physx::PxVec3 linearVelocity, physx::PxVec3 angularVelocity)
{
	// In PhysX3, we only use an energy threshold.  In PhysX2, we use either energy or velocity
#if NX_SDK_VERSION_MAJOR == 2
	NxPhysicsSDK* physicsSDK = NxGetApexSDK()->getPhysXSDK();
	if (actor->readBodyFlag(NX_BF_ENERGY_SLEEP_TEST))
	{
		// Using energy test
		const physx::PxF32 apiSleepEnergyThreshold = actor->getSleepEnergyThreshold();
		const physx::PxF32 energySleepThreshold = apiSleepEnergyThreshold >= 0.0f ? apiSleepEnergyThreshold : physicsSDK->getParameter(NX_DEFAULT_SLEEP_ENERGY);
		// Calculate kinetic energy
		const physx::PxF32 mass = actor->getMass();
		const physx::PxF32 linearKE = 0.5f*mass*linearVelocity.magnitudeSquared();
		const physx::PxF32 rotationalKE = 0.5f*NxFromPxVec3Fast(angularVelocity).dot(actor->getGlobalInertiaTensor()*NxFromPxVec3Fast(angularVelocity));
		const physx::PxF32 totalKE = linearKE + rotationalKE;
		// Put to sleep if below threshold
		if (totalKE <= energySleepThreshold)
		{
			actor->setLinearVelocity(NxVec3(0.0f));
			actor->setAngularVelocity(NxVec3(0.0f));
			actor->putToSleep();
		}
	}
	else
	{
		// Using velocity test
		const physx::PxF32 apiSleepLinVelThreshold = actor->getSleepLinearVelocity();
		const physx::PxF32 sleepLinVel2Threshold = apiSleepLinVelThreshold >= 0.0f ? apiSleepLinVelThreshold*apiSleepLinVelThreshold : physicsSDK->getParameter(NX_DEFAULT_SLEEP_LIN_VEL_SQUARED);
		const physx::PxF32 apiSleepAngVelThreshold = actor->getSleepAngularVelocity();
		const physx::PxF32 sleepAngVel2Threshold = apiSleepAngVelThreshold >= 0.0f ? apiSleepAngVelThreshold*apiSleepAngVelThreshold : physicsSDK->getParameter(NX_DEFAULT_SLEEP_ANG_VEL_SQUARED);
		// Put to sleep if below threshold
		if (linearVelocity.magnitudeSquared() <= sleepLinVel2Threshold && angularVelocity.magnitudeSquared() <= sleepAngVel2Threshold)
		{
			actor->setLinearVelocity(NxVec3(0.0f));
			actor->setAngularVelocity(NxVec3(0.0f));
			actor->putToSleep();
		}
	}
#elif NX_SDK_VERSION_MAJOR == 3
	PxRigidDynamic* rigidDynamic	= actor->isRigidDynamic();
	if (rigidDynamic == NULL)
	{
		return;
	}
	// Calculate kinetic energy
	const physx::PxF32 mass = rigidDynamic->getMass();
	const physx::PxF32 linearKE = 0.5f*mass*linearVelocity.magnitudeSquared();
	const PxTransform globalToMassT = rigidDynamic->getCMassLocalPose().transform(rigidDynamic->getGlobalPose());
	const physx::PxVec3 massSpaceAngularVelocity = globalToMassT.rotateInv(angularVelocity);
	const physx::PxF32 rotationalKE = 0.5f*massSpaceAngularVelocity.dot(rigidDynamic->getMassSpaceInertiaTensor().multiply(massSpaceAngularVelocity));
	const physx::PxF32 totalKE = linearKE + rotationalKE;
	// Put to sleep if below threshold
	if (totalKE <= rigidDynamic->getSleepThreshold()*mass)
	{
		rigidDynamic->setLinearVelocity(physx::PxVec3(0.0f));
		rigidDynamic->setAngularVelocity(physx::PxVec3(0.0f));
		rigidDynamic->putToSleep();
	}
#endif
}

/****************************
* ApexDamageEventReportData *
*****************************/

physx::PxU32
ApexDamageEventReportData::addFractureEvent(const DestructibleStructure::Chunk& chunk, physx::PxU32 flags)
{
	PX_ASSERT(m_destructible != NULL);
	if (m_destructible == NULL)
	{
		return 0xFFFFFFFF;
	}

	// Find flags to see if we record this event
	PX_ASSERT(!chunk.isDestroyed());
	if (chunk.state & ChunkDynamic)
	{
		flags |= NxApexChunkFlag::DYNAMIC;
	}
	if (chunk.flags & ChunkExternallySupported)
	{
		flags |= NxApexChunkFlag::EXTERNALLY_SUPPORTED;
	}
	if (chunk.flags & ChunkCrumbled)
	{
		flags |= NxApexChunkFlag::DESTROYED_CRUMBLED;
	}

	// return invalid index if we don't record this event
	if ((m_chunkReportBitMask & flags) == 0)
	{
		return 0xFFFFFFFF;
	}

	PX_ASSERT(m_destructible->getID() == chunk.destructibleID);
	const DestructibleAssetParametersNS::Chunk_Type& source = m_destructible->getAsset()->mParams->chunks.buf[chunk.indexInAsset];

	physx::PxBounds3 chunkWorldBoundsOnStack;
	physx::PxBounds3* chunkWorldBounds = &chunkWorldBoundsOnStack;

	physx::PxU32 fractureEventIndex = 0xFFFFFFFF;

	if (source.depth <= m_chunkReportMaxFractureEventDepth)
	{
		fractureEventIndex = m_fractureEvents.size();
		NxApexChunkData& fractureEvent = m_fractureEvents.insert();
		fractureEventList = &m_fractureEvents[0];
		fractureEventListSize = m_fractureEvents.size();
		fractureEvent.index = chunk.indexInAsset;
		fractureEvent.depth = source.depth;
		fractureEvent.damage = chunk.damage;
		chunkWorldBounds = &fractureEvent.worldBounds;	// Will be filled in below
		fractureEvent.flags = flags;
	}

	// Adjust bounds to world coordinates
	const physx::PxVec3 scale = m_destructible->getScale();
	const physx::PxBounds3& bounds = m_destructible->getAsset()->getChunkShapeLocalBounds(chunk.indexInAsset);
	chunkWorldBounds->minimum = bounds.minimum.multiply(scale);
	chunkWorldBounds->maximum = bounds.maximum.multiply(scale);
	PxBounds3Transform(*chunkWorldBounds, m_destructible->getChunkPose(chunk.indexInAsset));

	worldBounds.include(*chunkWorldBounds);
	minDepth = physx::PxMin(minDepth, source.depth);
	maxDepth = physx::PxMax(maxDepth, source.depth);
	++totalNumberOfFractureEvents;

	return fractureEventIndex;
}

void
ApexDamageEventReportData::setDestructible(DestructibleActor* inDestructible)
{
	m_destructible = inDestructible;
	if (m_destructible != NULL)
	{
		m_chunkReportBitMask = m_destructible->getStructure()->dscene->mModule->m_chunkReportBitMask;
		m_chunkReportMaxFractureEventDepth = m_destructible->getStructure()->dscene->mModule->m_chunkReportMaxFractureEventDepth;
		destructible = m_destructible->getAPI();
		minDepth = (physx::PxU16)(m_destructible->getAsset()->mParams->depthCount > 0 ? m_destructible->getAsset()->mParams->depthCount - 1 : 0);
		maxDepth = 0;
	}
	else
	{
		clear();
	}
}

void
ApexDamageEventReportData::clearChunkReports()
{
	PX_ASSERT(m_destructible != NULL);
	if (m_destructible == NULL)
	{
		return;
	}

	for (physx::PxU32 i = 0; i < m_fractureEvents.size(); ++i)
	{
		NxApexChunkData& fractureEvent = m_fractureEvents[i];
		DestructibleStructure::Chunk& chunk = m_destructible->getStructure()->chunks[fractureEvent.index + m_destructible->getFirstChunkIndex()];
		chunk.reportID = (physx::PxU32)DestructibleScene::InvalidReportID;
	}
}

/********************
* DestructibleScene *
********************/

class DestructibleBeforeTick : public physx::PxTask, public physx::UserAllocated
{
public:
	DestructibleBeforeTick(DestructibleScene& scene) : mScene(&scene), mDeltaTime(0.0f) {}
	const char* getName() const
	{
		return "DestructibleScene::BeforeTick";
	}
	void setDeltaTime(physx::PxF32 deltaTime)
	{
		mDeltaTime = deltaTime;
	}
	void run()
	{
		mScene->tasked_beforeTick(mDeltaTime);
	}
	DestructibleScene* mScene;
	physx::PxF32 mDeltaTime;
};

#if APEX_USE_GRB
class DestructiblePostPhysX : public physx::PxTask, public physx::UserAllocated
{
public:
	DestructiblePostPhysX(DestructibleScene& scene) : mScene(&scene) {}
	const char* getName() const
	{
		return "DestructibleScene::postPhysX";
	}

	void setTiming(physx::PxF32 maxTimestep, physx::PxU32 maxIter)
	{
		mMaxTimestep = maxTimestep;
		mMaxIter = maxIter;
	}

	void run()
	{
		mScene->postPhysX(mMaxTimestep, mMaxIter);
	}
	DestructibleScene* mScene;
	physx::PxF32 mMaxTimestep;
	physx::PxU32 mMaxIter;
};
#endif

static int comparePointers(const void* a, const void* b)
{
	return (uintptr_t)a == (uintptr_t)b ? 0 : ((uintptr_t)a < (uintptr_t)b ? -1 : 1);
}

#if NX_SDK_VERSION_MAJOR == 3
static int compareOverlapHitShapePointers(const void* a, const void* b)
{
	const physx::PxOverlapHit* overlapA = (const physx::PxOverlapHit*)a;
	const physx::PxOverlapHit* overlapB = (const physx::PxOverlapHit*)b;

	return comparePointers(overlapA->shape, overlapB->shape);
}
#endif

DestructibleScene::DestructibleScene(ModuleDestructible& module, NiApexScene& scene, NiApexRenderDebug* debugRender, NxResourceList& list) :
	mLodSumBenefit(0.0f),
	mLodRelativeBenefit(0.0f),
	mCurrentCostOfEssentialChunks(0.0f),
	mCurrentCostOfAllChunks(0.0f),
	mCurrentBudget(0.0f),
	mLodTotalValidChunks(0),
#if NX_SDK_VERSION_MAJOR == 2
	mUserNotify(module),
#elif NX_SDK_VERSION_MAJOR == 3
	mUserNotify(module, this),
#endif
	mMassScale(1.0f),
	mMassScaleInv(1.0f),
	mScaledMassExponent(0.5f),
	mScaledMassExponentInv(2.0f),
	mPreviousVisibleDestructibleChunkCount(0),
	mPreviousDynamicDestructibleChunkIslandCount(0),
	mDynamicActorFIFONum(0),
	mTotalChunkCount(0),
	mFractureEventCount(0),
	m_worldSupportPhysXScene(NULL),
	mDebugRender(debugRender),
	mSyncParams(module.getSyncParams())
{
	list.add(*this);
	mModule = &module;
	mApexScene = &scene;
	mPhysXScene = NULL;
#if NX_SDK_VERSION_MAJOR == 2
	mContactReport.destructibleScene = this;
	mApexScene->addModuleUserContactReport(mContactReport);
#else
	mOverlapHits.resize(MAX_SHAPE_COUNT);
#endif
	mApexScene->addModuleUserContactModify(mContactModify);
	mContactModify.destructibleScene = this;
	mNumFracturesProcessedThisFrame = 0;
	mNumActorsCreatedThisFrame = 0;
	mApexScene->addModuleUserNotifier(mUserNotify);

#if APEX_USE_GRB
	mGrbSimulating = false;
	mGrbScene = NULL;
	mPostPhysXTask = NULL;
	mGrbInteropEnabled = false;
	mGrbSimulationEnabled = scene.isGrbPhysicsEnabled();
	mPhysXGpuIndicator = NULL;
#endif

#if APEX_RUNTIME_FRACTURE
	mRTScene = NULL;
#endif

	mBeforeTickTask = PX_NEW(DestructibleBeforeTick)(*this);

	/* Initialize reference to DestructibleDebugRenderParams */
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
			memberHandle.initParamRef(DestructibleDebugRenderParams::staticClassName(), true);
		}
	}

	/* Load reference to DestructibleDebugRenderParams */
	NxParameterized::Interface* refPtr = NULL;
	memberHandle.getParamRef(refPtr);
	mDestructibleDebugRenderParams = DYNAMIC_CAST(DestructibleDebugRenderParams*)(refPtr);
	PX_ASSERT(mDestructibleDebugRenderParams);

	/* Initialize module defaults */
	setMassScaling(mModule->m_massScale, mModule->m_scaledMassExponent);

	createModuleStats();
}

DestructibleScene::~DestructibleScene()
{
#if APEX_USE_GRB
	if (mGrbScene)
	{
#if NX_SDK_VERSION_MAJOR == 2
		mModule->mGrbPhysicsSDK->releaseScene(*mGrbScene);
#else
		mGrbScene->release();
#endif	
		mGrbInteropEnabled = true;
		mGrbSimulationEnabled = true;

		NiApexSDK* apexSdk = NiGetApexSDK();
		apexSdk->unregisterPhysXIndicatorGpuClient(mPhysXGpuIndicator);
		mPhysXGpuIndicator = NULL;
	}
	delete(DestructiblePostPhysX*) mPostPhysXTask;
#endif
	delete(DestructibleBeforeTick*) mBeforeTickTask;

#if APEX_RUNTIME_FRACTURE
		PX_DELETE(mRTScene);
#endif

	destroyModuleStats();
}

void DestructibleScene::destroy()
{
	removeAllActors();
	reset();
	PX_ASSERT(mAwakeActors.size() == 0); // if there are actors left in here... thats very bad indeed.
	mApexScene->removeModuleUserNotifier(mUserNotify);
#if NX_SDK_VERSION_MAJOR == 2
	mApexScene->removeModuleUserContactReport(mContactReport);
#endif
	mApexScene->removeModuleUserContactModify(mContactModify);
	mApexScene->moduleReleased(*this);
	delete this;
}

#if APEX_USE_GRB
bool DestructibleScene::isGrbSimulationEnabled() const
{
	return mModule->mGrbInitialized && mGrbSimulationEnabled;
}

void DestructibleScene::setGrbSimulationEnabled(bool enable)
{
	if (enable && !mGrbScene)
	{
		mGrbSimulationEnabled = enable;
		createGRBScene();
	}
	else
	if (mGrbScene)
	{
		NiApexSDK* apexSdk = NiGetApexSDK();
#if		NX_SDK_VERSION_MAJOR == 2
		GrbScene * gs = (GrbScene*)mGrbScene;
#else
		GrbScene3 * gs = (GrbScene3*)mGrbScene;
#endif

		// turn on the GPU PhysX indicator
		if (!mPhysXGpuIndicator && enable)
		{
			mPhysXGpuIndicator = apexSdk->registerPhysXIndicatorGpuClient();
		}
		else if (mPhysXGpuIndicator && !enable)
		{
			// turn off the GPU PhysX indicator
			apexSdk->unregisterPhysXIndicatorGpuClient(mPhysXGpuIndicator);
			mPhysXGpuIndicator = NULL;
		}

		//When GRB simulation is disabled, we want to stop PhysX from generating
		//interop events. If we don't do this then there will be a big backlog
		//of interop events when GRB is renabled. Here we use setNxScene to
		//enable or disable PhysX interop event generation.
		if(!mGrbInteropEnabled && enable)
		{
#if NX_SDK_VERSION_MAJOR == 2
			gs->setNxScene(mPhysXScene);
#else
			gs->setPxScene(mPhysXScene);
#endif
			mGrbInteropEnabled = true;
		}
		else if(mGrbInteropEnabled && !enable)
		{
#if NX_SDK_VERSION_MAJOR == 2
			gs->setNxScene(0);
#else
			gs->setPxScene(0);
#endif
			mGrbInteropEnabled = false;
		}
		mGrbSimulationEnabled = enable;
	}
	else
	{
		mGrbSimulationEnabled = false;
	}
}

void DestructibleScene::createGRBScene()
{
	if (mPhysXScene && mModule->mGrbInitialized && !mGrbScene && mModule->mGrbPhysicsSDK && mGrbSimulationEnabled)
	{
		mGrbContactReport.destructibleScene = this;
		mPostPhysXTask = PX_NEW(DestructiblePostPhysX)(*this);

#if NX_SDK_VERSION_MAJOR == 2
		GrbSceneDesc sceneDesc;
		mPhysXScene->getGravity(sceneDesc.gravity);

		sceneDesc.nonPenSolverPosIterCount = mModule->mGrbNonPenSolverPosIterCount;
		sceneDesc.frictionSolverPosIterCount = mModule->mGrbFrictionSolverPosIterCount;
		sceneDesc.frictionSolverVelIterCount = mModule->mGrbFrictionSolverVelIterCount;

		sceneDesc.meshCellSize = mModule->mGrbMeshCellSize;
		//sceneDesc.skinWidth = mModule->mSkinWidth;
		//sceneDesc.skinWidth = mModule->mParticleSpacing/10.0f;
		sceneDesc.maxLinAcceleration = mModule->mGrbMaxLinAcceleration;

		//Make GRB have same substepping parameters as PhysX scene
		mPhysXScene->getTiming(sceneDesc.maxTimestep, sceneDesc.maxIter, sceneDesc.timeStepMethod, 0);

		sceneDesc.skinWidth = mPhysXScene->getPhysicsSDK().getParameter(NX_SKIN_WIDTH);

		mGrbScene = mModule->mGrbPhysicsSDK->createScene(mPhysXScene, sceneDesc);
		mGrbInteropEnabled = true;

		((GrbScene*)mGrbScene)->setGrbUserContactReport(&mGrbContactReport);
		// changing pair reporting mode to make it report contacts when PhysX bodies touch APEX destructible actors (while they are still in the kinematic state)
		((GrbScene*)mGrbScene)->setGrbPairReportingMode(NX_PAIR_REPORTING_INTEROP_KINEMATICS);
		GrbScene* grbScene = ((GrbScene*)mGrbScene);
#else
		SCOPED_PHYSX3_LOCK_READ(mPhysXScene);

		GrbSceneDesc3 sceneDesc(mModule->mGrbPhysicsSDK->getTolerancesScale());
		sceneDesc.gravity = mPhysXScene->getGravity();

		sceneDesc.nonPenSolverPosIterCount = mModule->mGrbNonPenSolverPosIterCount;
		sceneDesc.frictionSolverPosIterCount = mModule->mGrbFrictionSolverPosIterCount;
		sceneDesc.frictionSolverVelIterCount = mModule->mGrbFrictionSolverVelIterCount;
		sceneDesc.gpuMemSceneSize = mModule->mGrbMemSceneSize;
		sceneDesc.gpuMemTempDataSize = mModule->mGrbMemTempDataSize;
		sceneDesc.meshCellSize = mModule->mGrbMeshCellSize;
		sceneDesc.maxLinAcceleration = mModule->mGrbMaxLinAcceleration;

		mGrbScene = static_cast<GrbPhysics3 *>(mModule->mGrbPhysicsSDK)->createScene(sceneDesc, mPhysXScene);
		mGrbInteropEnabled = true;

		((GrbScene3*)mGrbScene)->setGrbUserContactReport(&mGrbContactReport);
		// changing pair reporting mode to make it report contacts when PhysX bodies touch APEX destructible actors (while they are still in the kinematic state)
		((GrbScene3*)mGrbScene)->setGrbPairReportingMode(PxPairReportingMode::eINTEROP_KINEMATICS);
		GrbScene3* grbScene = ((GrbScene3*)mGrbScene);
#endif
		
		// set the sphere and capsule GRB convex representation parameters
		grbScene->setGrbSphereSlices(mModule->getModuleParameters()->gpuRigidBodySettings.sphereSlices);
		grbScene->setGrbSphereStacks(mModule->getModuleParameters()->gpuRigidBodySettings.sphereStacks);
		grbScene->setGrbCapsuleSlices(mModule->getModuleParameters()->gpuRigidBodySettings.capsuleSlices);
		grbScene->setGrbCapsuleStacks(mModule->getModuleParameters()->gpuRigidBodySettings.capsuleStacks);

		NiApexSDK* apexSdk = NiGetApexSDK();
		mPhysXGpuIndicator = apexSdk->registerPhysXIndicatorGpuClient();
	}
}

void DestructibleScene::releaseGRBScene()
{
	if (mGrbScene)
	{
#if NX_SDK_VERSION_MAJOR == 2
		mModule->mGrbPhysicsSDK->releaseScene(*mGrbScene);
#else
		mGrbScene->release();
#endif

		// turn off the GPU PhysX indicator
		NiApexSDK* apexSdk = NiGetApexSDK();
		apexSdk->unregisterPhysXIndicatorGpuClient(mPhysXGpuIndicator);
		mPhysXGpuIndicator = NULL;

		mGrbInteropEnabled = false;
		mGrbScene = NULL;
	}
}
#endif // #if APEX_USE_GRB

void DestructibleScene::setModulePhysXScene(NxScene* nxScene)
{
	if (nxScene == mPhysXScene)
	{
		return;
	}

	mPhysXScene = nxScene;

	if (nxScene)
	{
		// Actors will make calls back to add themselves to structures
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			mActorArray[i]->setPhysXScene(nxScene);
		}

#if APEX_USE_GRB
		createGRBScene();
#endif
	}
	else
	{
#if APEX_USE_GRB
		releaseGRBScene();
#endif
		/* Release all destructible structures.  TODO - this is not an optimal way to do this */
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			mActorArray[i]->setPhysXScene(0);
		}

		tasked_beforeTick(0.0f);
	}
}

#if APEX_RUNTIME_FRACTURE
::physx::fracture::SimScene* DestructibleScene::getDestructibleRTScene(bool create)
{
	if(mRTScene == NULL && create)
	{
	#if APEX_USE_GRB
		if(getGrbScene())
		{
			mRTScene = physx::fracture::SimScene::createSimScene(&mGrbScene->getPhysics(),NxGetApexSDK()->getCookingInterface(),mGrbScene,true,0.02f,NULL,NULL);
			mRTScene->clear();
		}
		else
	#endif
		{
			mRTScene = physx::fracture::SimScene::createSimScene(&mApexScene->getPhysXScene()->getPhysics(),NxGetApexSDK()->getCookingInterface(),mPhysXScene,false,0.02f,NULL,NULL);
			mRTScene->clear();
		}		
	}
	return mRTScene;
}
#endif

NxDestructibleActorJoint* DestructibleScene::createDestructibleActorJoint(const NxDestructibleActorJointDesc& destructibleActorJointDesc)
{
	return PX_NEW(DestructibleActorJointProxy)(destructibleActorJointDesc, mDestructibleActorJointList);
}

PX_INLINE physx::PxF32 square(physx::PxF32 x)
{
	return x * x;
}

bool DestructibleScene::insertDestructibleActor(NxDestructibleActor* nxdestructible)
{
	//NiGetApexSDK()->getErrorHandler().print("GrbScene scene insertDestructibleActor");

	DestructibleActor* destructible = &((DestructibleActorProxy*)nxdestructible)->impl;
	if (destructible->getStructure())
	{
		return false;
	}

	const physx::PxF32 paddingFactor = destructible->getAsset()->mParams->neighborPadding;

	physx::Array<DestructibleStructure*> overlappedStructures;

	const physx::PxF32 padding = (destructible->getOriginalBounds().maximum - destructible->getOriginalBounds().minimum).magnitude() * paddingFactor;

	// TODO: Enable deserialization into compound structures when GRB is enabled
	const bool formsExtendedStructures =  !destructible->isInitiallyDynamic()     &&
	                                       destructible->formExtendedStructures() &&
	                                     !(destructible->initializedFromState()   && isGrbSimulationEnabled());

	physx::PxU32 destructibleCount = 0;
	DestructibleStructure* structureToUse = NULL;

	if (formsExtendedStructures)
	{
		// Static actor
		const physx::PxBounds3& box = destructible->getOriginalBounds();

		// Find structures that this actor touches
		for (physx::PxU32 structureNum = 0; structureNum < mStructures.usedCount(); ++structureNum)
		{
			DestructibleStructure* structure = mStructures.getUsed(structureNum);
			if (structure->destructibles.size() == 0)
			{
				continue;
			}
			DestructibleActor* firstDestructible = structure->destructibles[0];
			if (firstDestructible->isInitiallyDynamic() || !firstDestructible->formExtendedStructures())
			{
				continue;
			}
			// TODO: Support structure selection by hashing structure when serializing actor
			// We can hook up with this structure
			for (physx::PxU32 destructibleIndex = 0; destructibleIndex < structure->destructibles.size(); ++destructibleIndex)
			{
				DestructibleActor* existingDestructible = structure->destructibles[destructibleIndex];
				const physx::PxBounds3& existingDestructibleBox = existingDestructible->getOriginalBounds();
				if (box.intersects(existingDestructibleBox))
				{
					if (DestructibleAsset::chunksInProximity(*destructible->getAsset(), 0, destructible->getInitialGlobalPose(), destructible->getScale(),
					        *existingDestructible->getAsset(), 0, existingDestructible->getInitialGlobalPose(), existingDestructible->getScale(), padding))
					{
						// Record individual actor touches for neighbor list
						if (structureToUse == NULL || structureToUse->destructibles.size() < structure->destructibles.size())
						{
							structureToUse = structure;
						}
						overlappedStructures.pushBack(structure);
						destructibleCount += structure->destructibles.size();
						break;
					}
				}
			}
		}
	}

	physx::Array<DestructibleActor*> destructiblesToAdd;

	if (structureToUse == NULL)
	{
		// Need to make a structure for this destructible
		destructiblesToAdd.pushBack(destructible);
		physx::PxU32 structureID = PX_MAX_U32;
		if (!mStructures.useNextFree(structureID))
		{
			PX_ASSERT(!"Could not create a new structure ID.\n");
			return false;
		}
		structureToUse = PX_NEW(DestructibleStructure)(this, structureID);
		mStructures.direct(structureID) = structureToUse;
	}
	else
	{
		// We may re-use one of the overlappedStructures, and delete the rest
		destructiblesToAdd.resize(destructibleCount - structureToUse->destructibles.size() + 1);	// Exclude the ones in the structure already, include the one we're adding
		physx::PxU32 destructibleIndexOffset = 0;
		for (physx::PxU32 i = 0; i < overlappedStructures.size(); ++i)
		{
			DestructibleStructure* structure = overlappedStructures[i];
			if (structure == structureToUse)
			{
				continue;
			}
			memcpy(&destructiblesToAdd[destructibleIndexOffset], structure->destructibles.begin(), structure->destructibles.size()*sizeof(DestructibleActor*));
			destructibleIndexOffset += structure->destructibles.size();
			mStructures.free(structure->ID);
			mStructures.direct(structure->ID) = NULL;
			delete structure;
		}
		PX_ASSERT(destructibleIndexOffset == destructiblesToAdd.size() - 1);
		// Finally, add our new destructible to the list
		destructiblesToAdd[destructibleIndexOffset] = destructible;
	}

	return structureToUse->addActors(destructiblesToAdd);
}

void DestructibleScene::reset()
{
	//===SyncParams===
	mDeprioritisedFractureBuffer.erase();

	// Fracture buffer
	mFractureBuffer.erase();

	// Damage buffer
	mDamageBuffer.erase();

	// FIFO
	mDynamicActorFIFONum = 0;
	mTotalChunkCount = 0;
	mActorFIFO.reset();

	// Dormant list
	mDormantActors.clear();

	// Level-specific arrays
	mStructureKillList.reset();
	for (physx::PxU32 actorKillIndex = 0; actorKillIndex < mActorKillList.size(); ++actorKillIndex)
	{
		NxActor* actor = mActorKillList[actorKillIndex];
		PX_ASSERT(actor);
		if (actor)
		{
			releasePhysXActor(*actor);
		}
	}
	mActorKillList.reset();
	mDamageEventReportData.reset();
	mChunkReportHandles.reset();

	// Structure container
	for (physx::PxU32 i = mStructures.usedCount(); i--;)
	{
		physx::PxU32 index = mStructures.usedIndices()[i];
		delete mStructures.direct(index);
		mStructures.free(index);
	}

	mStructures.~NxBank<DestructibleStructure*, physx::PxU32>();
	PX_PLACEMENT_NEW(&mStructures, (NxBank<DestructibleStructure*, physx::PxU32>))();

	physx::PxU32 apexActorKillIndex = mApexActorKillList.getSize();
	while (apexActorKillIndex--)
	{
		DestructibleActorProxy* proxy = DYNAMIC_CAST(DestructibleActorProxy*)(mApexActorKillList.getResource(apexActorKillIndex));
		PX_ASSERT(proxy);
		delete proxy;
	}

	mChunkKillList.clear();
}


physx::PxF32	DestructibleScene::getBenefit()
{
	return mLodSumBenefit;
}

physx::PxF32	DestructibleScene::setResource(physx::PxF32 suggested, physx::PxF32, physx::PxF32 relativeBenefit)
{
	for (physx::PxU32 i = 0; i < mStructures.usedCount(); ++i)
	{
		DestructibleStructure* structure = mStructures.getUsed(i);
		if( !structure ) continue;

		if (structure->supportGraphNeedsRebuilding)
		{
			structure->buildSupportGraph();
			structure->supportGraphNeedsRebuilding = false;
		}
		structure->processActorDistancesSquaredFromViewportBuffer();
	}

	mLodRelativeBenefit = relativeBenefit;
	const physx::PxF32 resourceBudget = suggested;


	const physx::PxF32 taken = processEventBuffers(resourceBudget);

	return taken;
}

void DestructibleScene::resetEmitterActors()
{
# if APEX_USE_PARTICLES
	for (physx::PxU32 structureNum = 0; structureNum < mStructures.usedCount(); ++structureNum)
	{
		DestructibleStructure* structure = mStructures.getUsed(structureNum);
		if (structure)
		{
			for (physx::PxU32 destructibleIndex = 0; destructibleIndex < structure->destructibles.size(); ++destructibleIndex)
			{
				DestructibleActor* destructible = structure->destructibles[destructibleIndex];
				if (!destructible)
				{
					continue;
				}

				// stop all of the crumble and dust emitters in the destructible actors
				if (destructible->getCrumbleEmitter() && destructible->getCrumbleEmitter()->isExplicitGeom())
				{
					destructible->getCrumbleEmitter()->isExplicitGeom()->resetParticleList();
				}
				if (destructible->getDustEmitter() && destructible->getDustEmitter()->isExplicitGeom())
				{
					destructible->getDustEmitter()->isExplicitGeom()->resetParticleList();
				}
			}
		}
	}
#endif
}

void DestructibleScene::submitTasks(PxF32 elapsedTime, PxF32 substepSize, PxU32 numSubSteps)
{
	PX_PROFILER_PERF_SCOPE("DestructibleScene/submitTasks");
	PX_PROFILER_PLOT(mFractureEventCount, "Destructible/FractureEventCount");
	mFractureEventCount = 0;
	physx::PxTaskManager* taskManager = mApexScene->getTaskManager();
	taskManager->submitNamedTask(mBeforeTickTask, mBeforeTickTask->getName());
	mBeforeTickTask->setDeltaTime(elapsedTime);
#if APEX_USE_GRB
	if (mGrbScene && mPostPhysXTask)
	{
		taskManager->submitNamedTask(mPostPhysXTask, mPostPhysXTask->getName());
		mPostPhysXTask->setTiming(substepSize,numSubSteps);

		/* From this function it is safe to set dependencies on scene tasks */
		mPostPhysXTask->finishBefore(taskManager->getNamedTask(AST_PHYSX_FETCH_RESULTS));
	}
#else
	PX_UNUSED(substepSize)
	PX_UNUSED(numSubSteps)
#endif
}

void DestructibleScene::setTaskDependencies()
{
//	const PxTaskID lodTick = taskManager->getNamedTask(AST_LOD_COMPUTE_BENEFIT);
//	const PxTaskID physxTick = taskManager->getNamedTask(AST_PHYSX_SIMULATE);
//	mBeforeTickTask->startAfter(lodTick);
//	mBeforeTickTask->finishBefore(physxTick);
#if APEX_USE_GRB
	if (mGrbScene && mPostPhysXTask)
	{
		physx::PxTaskManager* taskManager = mApexScene->getTaskManager();
		/* From this function it is safe to set dependencies on other module tasks */
		mPostPhysXTask->startAfter(taskManager->getNamedTask(AST_PHYSX_SIMULATE));
	}
#endif
}

#if APEX_USE_GRB
void DestructibleScene::simulate(physx::PxF32)
{
}

// Called by ApexScene after NxScene::checkResults() returns true (so PhysX is complete),
// but before the game thread runs ApexScene::fetchResults().
void DestructibleScene::postPhysX(physx::PxF32 maxTimestep, physx::PxU32 maxIter)
{
	PX_UNUSED(maxTimestep);
	PX_UNUSED(maxIter);

	if (mGrbScene)
	{
		if (!isGrbSimulationEnabled())
		{
			return;
		}

		// Let's fetchResult for previous frame

		if (mGrbSimulating)
		{
			PX_PROFILER_PERF_SCOPE("GrbScene_fetchResults");

#if NX_SDK_VERSION_MAJOR == 2
			mGrbScene->fetchResults(NX_ALL_FINISHED, true);
#else
			mGrbScene->fetchResults(true);
#endif
			mGrbSimulating = false;

			// Warning, any code placed here (between fetchResults and simulate)
			// is in the critical path. This is the time when the GPU is idle,
			// so any code placed here will reduce X factor.
			// However, if you want to read, modify and write the same piece of
			// scene state in a single frame, then you must put that here.
			// The maxChunkVelocity capping is an example of a read/modify/write
			// operation.
			// If you put r/m/w operations anywhere other than here then state
			// writes from GRB might get overwritten and this can cause jitter.

			// We know GRB simulate has finished at this point, so record how long it took
			// We have to query the GRB API to find out its simulate time, because it 
			// may have finished before we call fetchResults.

#if NX_SDK_VERSION_MAJOR == 2
			const NxSceneStats2 * grbStats = mGrbScene->getStats2();
#else
			const GrbSimulationStatistics3 * grbStats = ((GrbScene3*)mGrbScene)->getSimulationStatistics();
#endif

			physx::PxI32 timeInMicroseconds = 0;
			physx::PxU32 i = 0;
			while(i != grbStats->numStats && strcmp(grbStats->stats[i].name, "SimulateCpuTimeIncludingGpuWait")!=0) i++; 

			if(i != grbStats->numStats)
			{
				ApexStatValue timeInMilliseconds;

				timeInMicroseconds = grbStats->stats[i].curValue;
				timeInMilliseconds.Float = (float)timeInMicroseconds/1000.0f;

				setStatValue(GpuRbSimulationTime,timeInMilliseconds);
			}

			//Calculate throughput
			i=0;
			while(i != grbStats->numStats && strcmp(grbStats->stats[i].name, "TotalPairs")!=0) i++;

			if(i != grbStats->numStats)
			{
				ApexStatValue throughput;

				physx::PxI32 numPairs = grbStats->stats[i].curValue;
				throughput.Float = (PxF32)numPairs / timeInMicroseconds; // divide by microseconds because we want Mpair/sec
				setStatValue(RbThroughput, throughput);
			}

			{
				PX_PROFILER_PERF_SCOPE("GrbScene_syncPhysXsdk");

#if NX_SDK_VERSION_MAJOR == 2
				((GrbScene*)mGrbScene)->syncPhysXsdk(!isGrbSimulationEnabled());
#else
				((GrbScene3*)mGrbScene)->syncPhysXsdk(!isGrbSimulationEnabled());
#endif
			}

			const physx::PxU32 actorCount = mActorFIFO.size();
			for (physx::PxU32 FIFOIndex = 0; FIFOIndex < actorCount; ++FIFOIndex)
			{
				ActorFIFOEntry& entry = mActorFIFO[FIFOIndex];
				// Cap the linear velocity here if it's a GRB
				if (entry.actor != NULL && entry.maxSpeed < PX_MAX_F32 && (entry.flags & ActorFIFOEntry::IsGRB) != 0)
				{
					const NxVec3 chunkVel = entry.actor->getLinearVelocity();
					const physx::PxF32 chunkSpeed2 = chunkVel.magnitudeSquared();
					if (chunkSpeed2 > entry.maxSpeed * entry.maxSpeed)
					{
						entry.actor->setLinearVelocity((entry.maxSpeed * NxRecipSqrt(chunkSpeed2))*chunkVel);
					}
				}
			}
		}

		{
			PX_PROFILER_PERF_SCOPE("GrbScene_simulate");

#if NX_SDK_VERSION_MAJOR == 2
			mGrbScene->simulate(mApexScene->getElapsedTime());
#else
			//TODO imitate stepper
			mGrbScene->simulate(mApexScene->getElapsedTime());
#endif
			mGrbSimulating = true;
		}
	}
}

#endif	// APEX_USE_GRB


class IRLess
{
public:
	bool operator()(IndexedReal& ir1, IndexedReal& ir2) const
	{
		return ir1.value < ir2.value;
	}
};

void DestructibleScene::tasked_beforeTick(physx::PxF32 elapsedTime)
{
	SCOPED_PHYSX_LOCK_WRITE(*mApexScene);

	for (physx::PxU32 actorKillIndex = 0; actorKillIndex < mActorKillList.size(); ++actorKillIndex)
	{
		NxActor* actor = mActorKillList[actorKillIndex];
		PX_ASSERT(actor);
		NiApexPhysXObjectDesc* actorObjDesc = mModule->mSdk->getGenericPhysXObjectInfo(actor);
		if (actorObjDesc != NULL)
		{
			const physx::PxU32 dActorCount = actorObjDesc->mApexActors.size();
			for (physx::PxU32 i = 0; i < dActorCount; ++i)
			{
				const NxDestructibleActor* dActor = static_cast<const NxDestructibleActor*>(actorObjDesc->mApexActors[i]);
				if (dActor != NULL)
				{
					if (actorObjDesc->mApexActors[i]->getOwner()->getObjTypeID() == DestructibleAsset::getAssetTypeID())
					{
						DestructibleActor& destructibleActor = const_cast<DestructibleActor&>(static_cast<const DestructibleActorProxy*>(dActor)->impl);
#if NX_SDK_VERSION_MAJOR == 2
						if (!actor->isSleeping())
#elif NX_SDK_VERSION_MAJOR == 3
						const PxRigidDynamic*	rigidDynamic = actor->isRigidDynamic();
						if (rigidDynamic && rigidDynamic->isSleeping())
#endif
						{
							destructibleActor.decrementWakeCount();
						}
						if (destructibleActor.getStructure() != NULL)
						{
							if (actor == destructibleActor.getStructure()->actorForStaticChunks)
							{
								destructibleActor.getStructure()->actorForStaticChunks = NULL;
							}
						}
					}
				}
			}
			actorObjDesc->mApexActors.clear();
			releasePhysXActor(*actor);
		}
	}
	mActorKillList.reset();

	if (mApexScene->getDebugRenderParams() && mDebugRenderParams->LodBenefits == 0.0f)
	{
		mLODDeletionDataList.reset();
		mLODReductionDataList.reset();
	}

	mNumFracturesProcessedThisFrame = 0;	//reset this counter
	mNumActorsCreatedThisFrame = 0;			//reset this counter

	resetEmitterActors();

	{
		PX_PROFILER_PERF_DSCOPE("DestructibleRemoveChunksForBudget", mChunkKillList.size());

		// Remove chunks which need eliminating to keep budget
		for (physx::PxU32 i = 0; i < mChunkKillList.size(); ++i)
		{
			const IntPair& deadChunk = mChunkKillList[i];
			DestructibleStructure* structure = mStructures.direct(deadChunk.i0);
			if (structure != NULL)
			{
				DestructibleStructure::Chunk& chunk = structure->chunks[deadChunk.i1];
				if (!chunk.isDestroyed())
				{
					structure->removeChunk(chunk);
				}
			}
		}
		mChunkKillList.clear();
	}

	//===SyncParams=== prepare user's fracture event buffer, if available
	UserFractureEventHandler * callback = NULL;
	callback = mModule->getSyncParams().getUserFractureEventHandler();
	const physx::Array<SyncParams::UserFractureEvent> * userSource = NULL;
	if(NULL != callback)
	{
		mSyncParams.onPreProcessReadData(*callback, userSource);
	}

	//===SyncParams=== give the user the fracture event buffer. fracture event buffer must be fully populated and locked during this call
	if(NULL != callback)
	{
		mSyncParams.onProcessWriteData(*callback, mFractureBuffer);
	}
	
	// Clear as much of the queue as we can
	processFractureBuffer();
	//===SyncParams=== process user's fracture events
	processFractureBuffer(userSource);

	//===SyncParams=== done with user's fracture event buffer, if available
	if(NULL != callback)
	{
		mSyncParams.onPostProcessReadData(*callback);
	}
	callback = NULL;

	// Process Damage coloring from forceDamageColoring()
	processDamageColoringBuffer();

	{
		PX_PROFILER_PERF_DSCOPE("DestructibleKillStructures", mStructureKillList.size());

		for (physx::PxU32 structureKillIndex = 0; structureKillIndex < mStructureKillList.size(); ++structureKillIndex)
		{
			DestructibleStructure* structure = mStructureKillList[structureKillIndex];
			if (structure)
			{
				for (physx::PxU32 destructibleIndex = structure->destructibles.size(); destructibleIndex--;)
				{
					DestructibleActor*& destructible = structure->destructibles[destructibleIndex];
					if (destructible)
					{
						destructible->setStructure(NULL);
						mDestructibles.direct(destructible->getID()) = NULL;
						mDestructibles.free(destructible->getID());
						destructible = NULL;
					}
				}
			}
			mStructures.free(structure->ID);
			mStructures.direct(structure->ID) = NULL;
			delete structure;
		}
		mStructureKillList.reset();
	}

#if DISABLE_RENDER_LOCKS==0
	{
		PX_PROFILER_PERF_SCOPE("DestructibleBeforeTickLockRenderables");
		// Lock renderables
		for (physx::PxU32 i = 0; i < mActorArray.size(); ++i)
		{
			mActorArray[i]->renderDataLock();
		}
	}
#endif

	const physx::PxU32 actorCount = mActorFIFO.size();

	if (mDynamicActorFIFONum > 0 && (((mModule->m_dynamicActorFIFOMax > 0 && mDynamicActorFIFONum > mModule->m_dynamicActorFIFOMax) ||
		(mModule->m_chunkFIFOMax > 0 && mTotalChunkCount > mModule->m_chunkFIFOMax)) && mModule->m_sortByBenefit))
	{
		if (mActorBenefitSortArray.size() < mActorFIFO.size())
		{
			mActorBenefitSortArray.resize(mActorFIFO.size());
		}
		for (physx::PxU32 i = 0; i < mActorFIFO.size(); ++i)
		{
			IndexedReal& ir = mActorBenefitSortArray[i];
			ActorFIFOEntry& entry = mActorFIFO[i];
			ir.value = entry.benefitCache;
			ir.index = i;
		}
		if (mActorFIFO.size() > 1)
		{
			shdfnd::sort(&mActorBenefitSortArray[0], mActorFIFO.size(), IRLess());
		}
		physx::PxU32 sortIndex = 0;
		if (mModule->m_dynamicActorFIFOMax > 0 && mDynamicActorFIFONum > mModule->m_dynamicActorFIFOMax)
		{
			while (sortIndex < mActorFIFO.size() && mDynamicActorFIFONum > mModule->m_dynamicActorFIFOMax)
			{
				IndexedReal& ir = mActorBenefitSortArray[sortIndex++];
				ActorFIFOEntry& entry = mActorFIFO[ir.index];
				if (entry.actor)
				{
					entry.flags |= ActorFIFOEntry::ForceLODRemove;
					--mDynamicActorFIFONum;
				}
			}
		}
		physx::PxU32 estTotalChunkCount = mTotalChunkCount;	// This will get decremented again, in the FIFO loop below
		if (mModule->m_chunkFIFOMax > 0 && estTotalChunkCount > mModule->m_chunkFIFOMax)
		{
			while (sortIndex < mActorFIFO.size() && estTotalChunkCount > mModule->m_chunkFIFOMax)
			{
				IndexedReal& ir = mActorBenefitSortArray[sortIndex++];
				ActorFIFOEntry& entry = mActorFIFO[ir.index];
				if (entry.actor)
				{
					entry.flags |= ActorFIFOEntry::ForceLODRemove;
					const physx::PxU32 chunkCount =  entry.actor->getNbShapes();
					estTotalChunkCount = estTotalChunkCount > chunkCount ? estTotalChunkCount - chunkCount : 0;
				}
			}
		}
	}

	mDynamicActorFIFONum = 0;
	for (physx::PxU32 FIFOIndex = 0; FIFOIndex < actorCount; ++FIFOIndex)
	{
		ActorFIFOEntry& entry = mActorFIFO[FIFOIndex];
		if (!entry.actor)
		{
			continue;
		}
		NiApexPhysXObjectDesc* actorObjDesc = mModule->mSdk->getGenericPhysXObjectInfo(entry.actor);
		if (!actorObjDesc)
		{
			continue;
		}

		entry.benefitCache = 0.0f;
		entry.age += elapsedTime;
		physx::PxU32 shapeCount;
		if (actorObjDesc->userData != NULL && (shapeCount = entry.actor->getNbShapes()) != 0)
		{
			physx::PxU32 reasonToDestroy = 0;
			const DestructibleActor* destructible = NULL;
			entry.maxSpeed = PX_MAX_F32;
			physx::PxF32 sleepVelocityFrameDecayConstant = 0.0f;
			bool useHardSleeping = false;
			//++LAVA
			bool hasTimeout = false;
			bool isFadingOut = false;
			physx::PxF32 fadeOutTime = PX_MAX_F32;
			//--LAVA
			for (physx::PxU32 i = 0; i < actorObjDesc->mApexActors.size(); ++i)
			{
				const DestructibleActorProxy* proxy = static_cast<const DestructibleActorProxy*>(actorObjDesc->mApexActors[i]);
				if (proxy == NULL)
				{
					continue;
				}
				destructible = &proxy->impl;
				const NxDestructibleParameters& parameters = destructible->getDestructibleParameters();
				physx::PxVec3 islandPos;
				PxFromNxVec3(islandPos, entry.actor->getCMassGlobalPosition());
				if ((parameters.flags & NxDestructibleParametersFlag::USE_VALID_BOUNDS) != 0 &&
					!parameters.validBounds.contains(islandPos - destructible->getInitialGlobalPose().getPosition()))
				{
					reasonToDestroy = NxApexChunkFlag::DESTROYED_LEFT_VALID_BOUNDS;
				}
				else if ((entry.flags & ActorFIFOEntry::ForceLODRemove) != 0)
				{
					reasonToDestroy = NxApexChunkFlag::DESTROYED_FIFO_FULL;
				}
				else if ((entry.flags & ActorFIFOEntry::IsDebris) != 0)
				{
					//++LAVA
					hasTimeout = hasTimeout || (parameters.flags & NxDestructibleParametersFlag::DEBRIS_TIMEOUT) != 0;
					//--LAVA

					// Check if too old or too far
					if ((parameters.flags & NxDestructibleParametersFlag::DEBRIS_TIMEOUT) != 0 &&
						entry.age > (parameters.debrisLifetimeMax - parameters.debrisLifetimeMin)*mModule->m_maxChunkSeparationLOD + parameters.debrisLifetimeMin)
					{
						reasonToDestroy = NxApexChunkFlag::DESTROYED_TIMED_OUT;
					}
					else if ((parameters.flags & NxDestructibleParametersFlag::DEBRIS_MAX_SEPARATION) != 0 &&
						(entry.origin - islandPos).magnitudeSquared() >
						square((parameters.debrisMaxSeparationMax - parameters.debrisMaxSeparationMin)*mModule->m_maxChunkSeparationLOD + parameters.debrisMaxSeparationMin))
					{
						reasonToDestroy = NxApexChunkFlag::DESTROYED_EXCEEDED_MAX_DISTANCE;
					}
				}
				else if ( entry.actor->getIsToBeDestroyed() )
				{
					reasonToDestroy = NxApexChunkFlag::DESTROYED_EXCEEDED_MAX_DISTANCE;
				}

				//++LAVA
				// If we've timed out or reached maximum separation, we want to fade out instead of just destroying immediately.
				// Also if we've already started fading out, want to continue with that.
				if (reasonToDestroy == NxApexChunkFlag::DESTROYED_TIMED_OUT || reasonToDestroy == NxApexChunkFlag::DESTROYED_EXCEEDED_MAX_DISTANCE ||
					(entry.flags & ActorFIFOEntry::IsFading) != 0)
				{
					fadeOutTime = physx::PxMin( fadeOutTime, destructible->getParams()->defaultBehaviorGroup.fadeOut );
					isFadingOut = true;

					// Don't destroy until we're faded out.
					if ((entry.flags & ActorFIFOEntry::IsFadeComplete) == 0)
					{
						reasonToDestroy = 0;
					}
					// If we're faded out, but don't have a specific reason, then we must have just been fading out previously.
					// This is possible for the max separation case, if we fell back within range.
					else if (reasonToDestroy == 0)
					{
						reasonToDestroy = NxApexChunkFlag::DESTROYED_EXCEEDED_MAX_DISTANCE;
					}
				}
				//--LAVA

				if (reasonToDestroy)
				{
					//++LAVA
					isFadingOut = false;
					//--LAVA
					destroyActorChunks(*entry.actor, reasonToDestroy);	// places the actor on the kill list
					entry.actor = NULL;
					break;
				}
				if (parameters.maxChunkSpeed > 0.0f)
				{
					entry.maxSpeed = physx::PxMin(entry.maxSpeed, parameters.maxChunkSpeed);
				}
				sleepVelocityFrameDecayConstant = physx::PxMax(sleepVelocityFrameDecayConstant, destructible->getSleepVelocityFrameDecayConstant());
				useHardSleeping = useHardSleeping || destructible->useHardSleeping();
			}

			//++LAVA
			if (isFadingOut)
			{
				entry.flags |= ActorFIFOEntry::IsFading;

				const PxF32 fadeSpeed = fadeOutTime <= 0.0f ? PX_MAX_F32 : ( 1.0f / fadeOutTime );
				entry.fadeAlpha = entry.fadeAlpha - elapsedTime * fadeSpeed;
				if ( entry.fadeAlpha <= 0.0f )
				{
					entry.fadeAlpha = 0.0f;
					entry.flags |= ActorFIFOEntry::IsFadeComplete;
				}
			}
			//--LAVA

			if (reasonToDestroy)
			{
				continue;
			}

			// Smooth velocities and see if the actor should be put to sleep, if it's not a GRB
			if (sleepVelocityFrameDecayConstant > 1.0f && (entry.flags & ActorFIFOEntry::IsGRB) == 0)
			{
				// Create smoothed velocities
				const physx::PxF32 sleepVelocitySmoothingFactor = 1.0f-1.0f/sleepVelocityFrameDecayConstant;
				const physx::PxF32 sleepVelocitySmoothingFactorComplement = 1.0f-sleepVelocitySmoothingFactor;
#if NX_SDK_VERSION_MAJOR == 2
				const physx::PxVec3 currentLinearVelocity = PxFromNxVec3Fast(entry.actor->getLinearVelocity());
				const physx::PxVec3 currentAngularVelocity = PxFromNxVec3Fast(entry.actor->getAngularVelocity());
#else
				const physx::PxVec3 currentLinearVelocity = entry.actor->getLinearVelocity();
				const physx::PxVec3 currentAngularVelocity = entry.actor->getAngularVelocity();
#endif
				if (!entry.actor->isSleeping())
				{
					entry.averagedLinearVelocity = sleepVelocitySmoothingFactor*entry.averagedLinearVelocity + sleepVelocitySmoothingFactorComplement*currentLinearVelocity;
					entry.averagedAngularVelocity = sleepVelocitySmoothingFactor*entry.averagedAngularVelocity + sleepVelocitySmoothingFactorComplement*currentAngularVelocity;
					handleSleeping(entry.actor, entry.averagedLinearVelocity, entry.averagedAngularVelocity);
				}
				else
				{
					// Initialize smoothed velocity so that the actor may wake up again
#if NX_SDK_VERSION_MAJOR == 2
					mPhysXScene->getGravity(NxFromPxVec3Fast(entry.averagedLinearVelocity));
#else
					entry.averagedLinearVelocity = mPhysXScene->getGravity();
#endif
					entry.averagedAngularVelocity = physx::PxVec3(0.0f, 0.0f, PxTwoPi);
				}
			}
			// Cap the linear velocity here if it's not a GRB
			if (entry.maxSpeed < PX_MAX_F32 && (entry.flags & ActorFIFOEntry::IsGRB) == 0)
			{
				const NxVec3 chunkVel = entry.actor->getLinearVelocity();
				const physx::PxF32 chunkSpeed2 = chunkVel.magnitudeSquared();
				if (chunkSpeed2 > entry.maxSpeed * entry.maxSpeed)
				{
					entry.actor->setLinearVelocity((entry.maxSpeed * NxRecipSqrt(chunkSpeed2))*chunkVel);
				}
			}
			if (actorObjDesc->userData == NULL)	// Signals that shapes have changed, need to recalculate the mass properties
			{
				PX_ASSERT(destructible != NULL);
			}
			if( destructible )
			{
				entry.benefitCache += destructible->getBenefit() * (physx::PxF32)shapeCount / (physx::PxF32)physx::PxMax<physx::PxU32>(destructible->getVisibleDynamicChunkShapeCount(), 1);
			}
			if ((entry.flags & ActorFIFOEntry::MassUpdateNeeded) != 0)
			{
				PX_ASSERT(entry.unscaledMass > 0.0f);
				if (entry.unscaledMass > 0.0f)
				{
					entry.actor->updateMassFromShapes(0.0f, scaleMass(entry.unscaledMass));
				}
				entry.flags &= ~(physx::PxU32)ActorFIFOEntry::MassUpdateNeeded;
				entry.unscaledMass = 0.0f;
			}

			//++LAVA -- check IsFading and hasTimeout
			// If we're fading out, don't remove from the dynamic object list. Need to keep getting updates.
			// Don't remove either if we have a debris timeout, or else we won't actually use that.
			if (useHardSleeping && entry.actor->isSleeping() && (entry.flags & ActorFIFOEntry::IsFading) == 0 && !hasTimeout)
			//--LAVA
			{
				if (!entry.actor->readBodyFlag(NX_BF_KINEMATIC))
				{
					entry.actor->raiseBodyFlag(NX_BF_KINEMATIC);
					physx::PxU32 dormantID = 0;
					if (mDormantActors.useNextFree(dormantID))
					{
						actorObjDesc->userData = (void*)~(uintptr_t)dormantID;
						mDormantActors.direct(dormantID) = DormantActorEntry(entry.actor, entry.unscaledMass, entry.flags);
					}
					else
					{
						actorObjDesc->userData = NULL;
					}
				}
				entry.actor = NULL;
				continue;
			}
			if (mDynamicActorFIFONum != FIFOIndex)
			{
				actorObjDesc->userData = (void*)~(uintptr_t)mDynamicActorFIFONum;
				mActorFIFO[mDynamicActorFIFONum] = entry;
			}
			++mDynamicActorFIFONum;
		}
		else
		{
			scheduleNxActorForDelete(*actorObjDesc);
			entry.actor = NULL;
		}
	}
	mActorFIFO.resize(mDynamicActorFIFONum);

	// Unlock renderables
	for (physx::PxU32 i = 0; i < mActorArray.size(); ++i)
	{
		mActorArray[i]->renderDataUnLock();
	}

	// Tick structures
	mTotalChunkCount = 0;

	for (physx::PxU32 structureNum = 0; structureNum < mStructures.usedCount(); ++structureNum)
	{
		DestructibleStructure* structure = mStructures.getUsed(structureNum);
		if (structure)
		{
			structure->tick(elapsedTime);
			for (physx::PxU32 i = 0; i < structure->destructibles.size(); ++i)
			{
				DestructibleActor* destructible = structure->destructibles[i];
				if (destructible != NULL)
				{
					mTotalChunkCount += destructible->getNumVisibleChunks();
					destructible->getAge() += elapsedTime;
				}
			}
		}
	}

	for (physx::PxU32 jointIndex = mDestructibleActorJointList.getSize(); jointIndex--;)
	{
		DestructibleActorJointProxy* jointProxy =
		    DYNAMIC_CAST(DestructibleActorJointProxy*)(mDestructibleActorJointList.getResource(jointIndex));
		PX_ASSERT(jointProxy != NULL);
		if (jointProxy != NULL)
		{
			bool result = jointProxy->impl.updateJoint();
			if (!result)
			{
				jointProxy->release();
			}
		}
	}
}

void DestructibleScene::fetchResults()
{
	PX_PROFILER_PERF_SCOPE("DestructibleFetchResults");

	// beforeTick() should have deleted all of the NxActors.  It should be safe now to delete the Apex actors
	// since there should be no more NxApexPhysXObjectDesc which reference this actor.
	physx::PxU32 apexActorKillIndex = mApexActorKillList.getSize();
	while (apexActorKillIndex--)
	{
		DestructibleActorProxy* proxy = DYNAMIC_CAST(DestructibleActorProxy*)(mApexActorKillList.getResource(apexActorKillIndex));
		PX_ASSERT(proxy);
		delete proxy;
	}

#if NX_SDK_VERSION_MAJOR == 2
	// no need to update bone poses if the physx scene did not step
	physx::PxF32 maxTimeStep;
	physx::PxU32 nbSubSteps, maxIter;
	NxTimeStepMethod method;
	mPhysXScene->getTiming(maxTimeStep, maxIter, method, &nbSubSteps);
	if (nbSubSteps)
#endif
	{
		PX_PROFILER_PERF_DSCOPE("DestructibleUpdateRenderMeshBonePoses", mAwakeActors.size());

		// Reset instanced mesh buffers
		for (physx::PxU32 i = 0; i < mActorArray.size(); ++i)
		{
			DestructibleActor* actor = DYNAMIC_CAST(DestructibleActor*)(mActorArray[ i ]);
			if( !actor->isVisible() )
			{
				continue;
			}
			if (actor->m_listIndex == 0)
			{
				for (physx::PxU32 j = 0; j < actor->getAsset()->m_chunkInstanceBufferData.size(); ++j)
				{
					actor->getAsset()->m_chunkInstanceBufferData[j].resize(0);
				}
				for (physx::PxU32 j = 0; j < actor->getAsset()->m_scatterMeshInstanceInfo.size(); ++j)
				{
					actor->getAsset()->m_scatterMeshInstanceInfo[j].m_instanceBufferData.resize(0);
				}
				actor->getAsset()->m_instancingRepresentativeActorIndex = -1;	// not set
			}
		}

		//===SyncParams=== prepare user's chunk motion buffer, if available
		UserChunkMotionHandler * callback = NULL;
		callback = mModule->getSyncParams().getUserChunkMotionHandler();
		if(NULL != callback)
		{
			mSyncParams.onPreProcessReadData(*callback);
		}

        //update actors' pose
		for (physx::PxU32 i = 0; i < mActorArray.size(); ++i)
		{
			DestructibleActor* actor = DYNAMIC_CAST(DestructibleActor*)(mActorArray[ i ]);
			if( !actor->isVisible() )
			{
				continue;
			}
			actor->fillInstanceBuffers();
			const bool processChunkPoseForSyncing = ((NULL != callback) && (	actor->getSyncParams().isSyncFlagSet(NxDestructibleActorSyncFlags::CopyChunkTransform) ||
																				actor->getSyncParams().isSyncFlagSet(NxDestructibleActorSyncFlags::ReadChunkTransform)));
            actor->setRenderTMs(processChunkPoseForSyncing);
		}

		//===SyncParams=== give the user the chunk motion buffer. chunk motion buffer must be fully populated and locked during this call
		if(NULL != callback)
		{
			mSyncParams.onProcessWriteData(*callback);
		}

		//===SyncParams=== done with user's chunk event motion, if available
		if(NULL != callback)
		{
			mSyncParams.onPostProcessReadData(*callback);
		}
		callback = NULL;

		//===SyncParams=== allow user from changing sync params again. do this just after the last encounter of working with sync params in the program
		PX_ASSERT(mSyncParams.lockSyncParams);
		mSyncParams.lockSyncParams = false;

#if 0
		/* Update bone boses in the render mesh, update world bounds */
		for (physx::PxU32 i = 0 ; i < mAwakeActors.size() ; i++)
		{
			DestructibleActor* actor = DYNAMIC_CAST(DestructibleActor*)(mAwakeActors[ i ]);
			if (actor->getStructure())
			{
				actor->setRenderTMs();
			}
		}
#endif
	}

	if (mModule->m_chunkReport)
	{
		PX_PROFILER_PERF_SCOPE("DestructibleChunkReport");
		for (physx::PxU32 reportNum = 0; reportNum < mDamageEventReportData.size(); ++reportNum)
		{
			ApexDamageEventReportData& data = mDamageEventReportData[reportNum];
			if (data.totalNumberOfFractureEvents > 0)
			{
				mModule->m_chunkReport->onDamageNotify(data);
			}
			data.clearChunkReports();
		}
	}
	mDamageEventReportData.reset();
	mChunkReportHandles.reset();

	// here all scenes have finished beforeTick for sure, so we know all the NxActors have been deleted
	// TODO: can that happen: beforeTick, releaseActor, releaseAsset, fetchResults ? that would still cause a problem.
	mModule->releaseBufferedConvexMeshes();
}

PX_INLINE void visualizeNxActorBenefit(NiApexRenderDebug* debugRender, const physx::PxVec3& eyePos, const physx::PxVec3& eyeDir, NxActor& actor, physx::PxF32 benefit, const physx::PxF32* m, physx::PxF32 e, physx::PxF32 recip_a)
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(debugRender);
	PX_UNUSED(eyePos);
	PX_UNUSED(eyeDir);
	PX_UNUSED(actor);
	PX_UNUSED(benefit);
	PX_UNUSED(m);
	PX_UNUSED(e);
	PX_UNUSED(recip_a);
#else
	const physx::PxVec3 pos = PXFROMNXVEC3(actor.getCMassGlobalPosition());
	const physx::PxF32 z = (pos - eyePos).dot(eyeDir);
	if (z > 0)
	{
		physx::PxVec3 boxCorners[4];
		const physx::PxVec3 screenPos = transformToScreenSpace(pos, m);
		const physx::PxF32 screenSizeX = e * physx::PxSqrt(0.01f * benefit);	// 1/100 is now a hack; benefit units are not screen area percentage
		const physx::PxF32 screenSizeY = recip_a * screenSizeX;
		boxCorners[0] = physx::PxVec3(screenPos.x + screenSizeX, screenPos.y + screenSizeY, 0.0f);
		boxCorners[1] = physx::PxVec3(screenPos.x - screenSizeX, screenPos.y + screenSizeY, 0.0f);
		boxCorners[2] = physx::PxVec3(screenPos.x - screenSizeX, screenPos.y - screenSizeY, 0.0f);
		boxCorners[3] = physx::PxVec3(screenPos.x + screenSizeX, screenPos.y - screenSizeY, 0.0f);
		debugRender->debugPolygon(4, boxCorners);
		const physx::PxF32 textSize = physx::PxMax(0.07f, 0.5f * screenSizeY);
		debugRender->setCurrentTextScale(textSize);
		if (screenSizeX > 0.025f)
		{
			static char buf[128];
			physx::string::sprintf_s(buf, sizeof(buf), "%4.2f", benefit);
			debugRender->debugText(PxVec3(screenPos.x - 0.9f * screenSizeX, screenPos.y + screenSizeY - 0.5f * textSize, 0.0f), buf);
		}
	}
#endif
}

void DestructibleScene::visualize()
{
#ifdef WITHOUT_DEBUG_VISUALIZE
#else
	if (!mDestructibleDebugRenderParams->VISUALIZE_DESTRUCTIBLE_ACTOR)
	{
		return;
	}

	// save the rendering state
	mDebugRender->pushRenderState();
	if (mDestructibleDebugRenderParams->VISUALIZE_DESTRUCTIBLE_BOUNDS > 0)
	{
		mDebugRender->setCurrentColor(mDebugRender->getDebugColor(physx::DebugColors::Red));
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			DestructibleActor* actor = DYNAMIC_CAST(DestructibleActor*)(mActorArray[ i ]);
			mDebugRender->setCurrentUserPointer((void*)(NxApexActor*)(mActorArray[i]));
			physx::PxBounds3 bounds = actor->getBounds();
			mDebugRender->debugBound(bounds.minimum, bounds.maximum);
			mDebugRender->setCurrentUserPointer(NULL);
		}
	}

	if (mDestructibleDebugRenderParams->VISUALIZE_DESTRUCTIBLE_SUPPORT > 0)
	{
		for (physx::PxU32 structureNum = 0; structureNum < mStructures.usedCount(); ++structureNum)
		{
			DestructibleStructure* structure = mStructures.getUsed(structureNum);
			if (structure)
			{
				structure->visualizeSupport(mDebugRender);
			}
		}
	}

	// debug visualization
	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		DestructibleActor* actor = DYNAMIC_CAST(DestructibleActor*)(mActorArray[ i ]);
		mDebugRender->setCurrentUserPointer((void*)(NxApexActor*)(mActorArray[i]));
		const physx::PxU16*	visibleChunkIndexContainer	= actor->getVisibleChunks();
		const physx::PxU32 visibleChunkIndexCount = actor->getNumVisibleChunks();
		const physx::PxVec3&	eyePos						= mApexScene->getEyePosition();

		for (physx::PxU32 chunkIndex = 0 ; chunkIndex < visibleChunkIndexCount; ++chunkIndex)
		{
			physx::PxMat34Legacy	chunkPose	= actor->getChunkPose(visibleChunkIndexContainer[chunkIndex]);
			physx::PxF32	disToEye	= (-eyePos + chunkPose.t).magnitude();

			if (visibleChunkIndexCount == 1 && visibleChunkIndexContainer[0] == 0)
			{
				// visualize actor pose
				if (mDestructibleDebugRenderParams->VISUALIZE_DESTRUCTIBLE_ACTOR_POSE &&
				        disToEye < mDestructibleDebugRenderParams->THRESHOLD_DISTANCE_DESTRUCTIBLE_ACTOR_POSE)
				{
					mDebugRender->debugAxes(chunkPose, 1, 1);
				}

				// visualize actor name
				if (mDestructibleDebugRenderParams->VISUALIZE_DESTRUCTIBLE_ACTOR_NAME &&
				        disToEye < mDestructibleDebugRenderParams->THRESHOLD_DISTANCE_DESTRUCTIBLE_ACTOR_NAME)
				{
					PxF32 shiftDistance		= actor->getBounds().getExtents().magnitude();
					PxVec3 shiftDirection	= -1 * mApexScene->getViewMatrix(0).column1.getXYZ();
					PxVec3 textLocation		= chunkPose.t + (shiftDistance * shiftDirection);

					mDebugRender->addToCurrentState(physx::DebugRenderState::CameraFacing);
					mDebugRender->debugText(textLocation, "Destructible");
					mDebugRender->removeFromCurrentState(physx::DebugRenderState::CameraFacing);
				}
			}
			else
			{
				// visualize actor pose (fragmented)
				if (mDestructibleDebugRenderParams->VISUALIZE_DESTRUCTIBLE_FRAGMENT_POSE &&
				        disToEye < mDestructibleDebugRenderParams->THRESHOLD_DISTANCE_DESTRUCTIBLE_FRAGMENT_POSE)
				{
#if 0
					mDebugRender->debugAxes(chunkPose, 0.5, 0.7);
#endif //lionel: todo: chunk pose is incorrect
				}
			}
		}
		mDebugRender->setCurrentUserPointer(NULL);
	}

	// visualize render mesh actor
	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		DestructibleActor* currentActor = NULL;
		currentActor = DYNAMIC_CAST(DestructibleActor*)(mActorArray[i]);
		PX_ASSERT(currentActor != NULL);

		for (physx::PxU32 meshTypeIndex = 0; meshTypeIndex < NxDestructibleActorMeshType::Count; ++meshTypeIndex)
		{
			const NiApexRenderMeshActor* currentMeshActor = NULL;
			currentMeshActor = static_cast<NiApexRenderMeshActor*>(const_cast<NxRenderMeshActor*>(currentActor->getRenderMeshActor(static_cast<NxDestructibleActorMeshType::Enum>(meshTypeIndex)))); //lionel: const_cast bad!
			if (NULL == currentMeshActor)
			{
				continue;
			}
			PX_ASSERT(mDebugRender != NULL);
			currentMeshActor->visualize(*mDebugRender, mDebugRenderParams);
		}

		// Instancing
		if (currentActor->m_listIndex == 0)
		{
			for (physx::PxU32 j = 0; j < currentActor->getAsset()->m_instancedChunkRenderMeshActors.size(); ++j)
			{
				PX_ASSERT(j < currentActor->getAsset()->m_chunkInstanceBufferData.size());
				if (currentActor->getAsset()->m_instancedChunkRenderMeshActors[j] != NULL && currentActor->getAsset()->m_chunkInstanceBufferData[j].size() > 0)
				{
					physx::PxU32 count = currentActor->getAsset()->m_chunkInstanceBufferData[j].size();
					physx::PxMat33* scaledRotations = &currentActor->getAsset()->m_chunkInstanceBufferData[j][0].scaledRotation;
					physx::PxVec3* translations = &currentActor->getAsset()->m_chunkInstanceBufferData[j][0].translation;

					const NiApexRenderMeshActor* currentMeshActor = NULL;
					currentMeshActor = static_cast<NiApexRenderMeshActor*>(const_cast<NxRenderMeshActor*>(currentActor->getAsset()->m_instancedChunkRenderMeshActors[j]));
					if (NULL == currentMeshActor)
					{
						continue;
					}
					PX_ASSERT(mDebugRender != NULL);
					currentMeshActor->visualize(*mDebugRender, mDebugRenderParams, scaledRotations, translations, sizeof(DestructibleAsset::ChunkInstanceBufferDataElement), count);
				}
			}
		}
	}

	// Visualize lod benefit
	physx::PxF32 benefitScale = mDebugRenderParams->LodBenefits;
	if (benefitScale > 0.0f)
	{
		mDebugRender->setCurrentColor(mDebugRender->getDebugColor(physx::DebugColors::Green));
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			DestructibleActor* actor = DYNAMIC_CAST(DestructibleActor*)(mActorArray[ i ]);
			mDebugRender->debugSphere(actor->getOriginalBounds().getCenter(), actor->getOriginalBounds().getExtents().magnitude());
		}

		const physx::PxF32* m = mDebugRender->getViewProjectionMatrix();
		const physx::PxF32* p = mDebugRender->getProjectionMatrix();
		const physx::PxF32 e = p[0];	// p(0,0) = 1/tan(FOV/2)
		const physx::PxF32 recip_a = p[5] / e;	// p(1,1) = p(0,0)/(screen height/width)
		const physx::PxVec3& eyePos = mApexScene->getEyePosition();
		const physx::PxVec3& eyeDir = mApexScene->getEyeDirection();
		mDebugRender->addToCurrentState(physx::DebugRenderState::ScreenSpace);

		// Visualize current chunk islands
		for (physx::PxU32 i = 0; i < mActorFIFO.size(); i++)
		{
			ActorFIFOEntry& entry = mActorFIFO[i];
			if (entry.actor && entry.benefitCache < PX_MAX_F32) //can have PX_MAX_F32 special
			{
				visualizeNxActorBenefit(mDebugRender, eyePos, eyeDir, *entry.actor, entry.benefitCache, m, e, recip_a);
			}
		}

		physx::PxVec3 boxCorners[4];

		// Visualize LOD-deleted chunks
		mDebugRender->setCurrentColor(mDebugRender->getDebugColor(physx::DebugColors::Red));
		mDebugRender->setCurrentDisplayTime(0.5f);
		while (mLODDeletionDataList.size())
		{
			LODReductionData& deletionData = mLODDeletionDataList.back();
			const physx::PxF32 z = (deletionData.pos - eyePos).dot(eyeDir);
			if (z > 0)
			{
				const physx::PxVec3 screenPos = transformToScreenSpace(deletionData.pos, m);
				const physx::PxF32 relativeBenefit = mLodRelativeBenefit * deletionData.benefit;
				const physx::PxF32 screenSizeX = e * physx::PxSqrt(0.01f * relativeBenefit);	// 1/100 is not a hack; benefit units are in screen area percentage
				const physx::PxF32 screenSizeY = recip_a * screenSizeX;
				boxCorners[0] = physx::PxVec3(screenPos.x + screenSizeX, screenPos.y + screenSizeY, 0.0f);
				boxCorners[1] = physx::PxVec3(screenPos.x - screenSizeX, screenPos.y + screenSizeY, 0.0f);
				boxCorners[2] = physx::PxVec3(screenPos.x - screenSizeX, screenPos.y - screenSizeY, 0.0f);
				boxCorners[3] = physx::PxVec3(screenPos.x + screenSizeX, screenPos.y - screenSizeY, 0.0f);
				// Draw a box with an "X" through it
				mDebugRender->debugPolygon(4, boxCorners);
				mDebugRender->debugLine(boxCorners[0], boxCorners[2]);
				mDebugRender->debugLine(boxCorners[1], boxCorners[3]);
			}
			mLODDeletionDataList.popBack();
		}

		// Visualize LOD-reduced fracture events
		mDebugRender->setCurrentColor(mDebugRender->getDebugColor(physx::DebugColors::Yellow));
		mDebugRender->setCurrentDisplayTime(0.5f);
		while (mLODReductionDataList.size())
		{
			LODReductionData& reductionData = mLODReductionDataList.back();
			const physx::PxF32 z = (reductionData.pos - eyePos).dot(eyeDir);
			if (z > 0)
			{
				const physx::PxVec3 screenPos = transformToScreenSpace(reductionData.pos, m);
				const physx::PxF32 relativeBenefit = mLodRelativeBenefit * reductionData.benefit;
				const physx::PxF32 screenSizeX = e * physx::PxSqrt(0.01f * relativeBenefit);	// 1/100 is not a hack; benefit units are in screen area percentage
				const physx::PxF32 screenSizeY = recip_a * screenSizeX;
				boxCorners[0] = physx::PxVec3(screenPos.x, screenPos.y + screenSizeY, 0.0f);
				boxCorners[1] = physx::PxVec3(screenPos.x - screenSizeX, screenPos.y, 0.0f);
				boxCorners[2] = physx::PxVec3(screenPos.x, screenPos.y - screenSizeY, 0.0f);
				boxCorners[3] = physx::PxVec3(screenPos.x + screenSizeX, screenPos.y, 0.0f);
				// Draw a diamond
				mDebugRender->debugPolygon(4, boxCorners);
			}
			mLODReductionDataList.popBack();
		}

		// Budget bar
		const physx::PxF32 barTextY = -0.9f;
		mDebugRender->setCurrentTextScale(0.125f);
		mDebugRender->setCurrentColor(mDebugRender->getDebugColor(physx::DebugColors::Blue));
		mDebugRender->debugText(PxVec3(-1.0f, barTextY, 0.0f), "Essential");
		mDebugRender->setCurrentColor(mDebugRender->getDebugColor(physx::DebugColors::Yellow));
		mDebugRender->debugText(PxVec3(-0.7f, barTextY, 0.0f), "Nonessential");
		mDebugRender->setCurrentColor(mDebugRender->getDebugColor(physx::DebugColors::White));
		mDebugRender->debugText(PxVec3(-0.3f, barTextY, 0.0f), "Unused");
		mDebugRender->setCurrentColor(mDebugRender->getDebugColor(physx::DebugColors::Red));
		mDebugRender->debugText(PxVec3(0.0f, barTextY, 0.0f), "Overrun");
		mDebugRender->addToCurrentState(physx::DebugRenderState::SolidShaded);
		const physx::PxF32 barY = -0.95f;
		const physx::PxF32 barThickness = 0.025f;
		mDebugRender->setCurrentColor(mDebugRender->getDebugColor(physx::DebugColors::Blue));
		const physx::PxF32 essentialLen = physx::PxMin(1.0f, mCurrentCostOfEssentialChunks / mCurrentBudget);
		mDebugRender->debugCylinder(physx::PxVec3(-1.0f, barY, 0.0f), physx::PxVec3(-1.0f + essentialLen, barY, 0.0f), barThickness);
		const physx::PxF32 allLen = physx::PxMin(1.0f, mCurrentCostOfAllChunks / mCurrentBudget);
		if (allLen > essentialLen)
		{
			mDebugRender->setCurrentColor(mDebugRender->getDebugColor(physx::DebugColors::Yellow));
			mDebugRender->debugCylinder(physx::PxVec3(-1.0f + essentialLen, barY, 0.0f), physx::PxVec3(-1.0f + allLen, barY, 0.0f), barThickness);
		}
		if (allLen < 1.0f)
		{
			mDebugRender->setCurrentColor(mDebugRender->getDebugColor(physx::DebugColors::White));
			mDebugRender->debugCylinder(physx::PxVec3(-1.0f + allLen, barY, 0.0f), physx::PxVec3(0.0f, barY, 0.0f), barThickness);
		}
		else
		{
			const physx::PxF32 costLen = physx::PxMin(2.0f, mCurrentCostOfAllChunks / mCurrentBudget);
			mDebugRender->setCurrentColor(mDebugRender->getDebugColor(physx::DebugColors::Red));
			mDebugRender->debugCylinder(physx::PxVec3(0.0f, barY, 0.0f), physx::PxVec3(-1.0f + costLen, barY, 0.0f), barThickness);
		}
	}
	// restore the rendering state
	mDebugRender->popRenderState();
#endif
}

#if APEX_USE_GRB
void DestructibleGrbContactReport::onContactNotify(NxU32 arraySizes, void** shape0Array, void** shape1Array, void** actor0Array, void** actor1Array, float* positionArray, float* normalArray)
{
	PX_PROFILER_PERF_SCOPE("GrbContactReport_onContactNotify");

	for (physx::PxU32 i = 0; i < arraySizes; ++i)
	{
#if NX_SDK_VERSION_MAJOR == 2
		GrbShape* shapes[2];
		shapes[0] = (GrbShape*) *shape0Array++;
		shapes[1] = (GrbShape*) *shape1Array++;

		NxActor* actors[2] = { (NxActor *)*actor0Array++, (NxActor *)*actor1Array++ };
#else
		GrbShape3* shapes[2];
		shapes[0] = (GrbShape3*) *shape0Array++;
		shapes[1] = (GrbShape3*) *shape1Array++;

		PxRigidActor* actors[2] = { (PxRigidActor *)*actor0Array++, (PxRigidActor *)*actor1Array++ };
#endif
		NxVec3 position(*positionArray, *(positionArray + 1), *(positionArray + 2));
		positionArray += 3;
		NxVec3 normal(*normalArray, *(normalArray + 1), *(normalArray + 2));
		normalArray += 3;

		DestructibleActor* destructibles[2];
		physx::PxI32 chunkIndices[2];

		physx::PxF32							minImpactVelocityThresholdsSq =	NX_MAX_REAL;

		for (int i = 0; i < 2; ++i)
		{
			DestructibleActorProxy* proxy = (DestructibleActorProxy*)destructibleScene->getDestructibleAndChunk(shapes[i], chunkIndices + i);
			destructibles[i] = proxy != NULL ? &proxy->impl : NULL;
		}

		if (destructibles[0] == destructibles[1])
		{
			continue;
		}

#if NX_SDK_VERSION_MAJOR == 2
		bool kinematicActors[2] =
		{
			actors[0]->isDynamic() && actors[0]->readBodyFlag(NX_BF_KINEMATIC) != 0,
			actors[1]->isDynamic() && actors[1]->readBodyFlag(NX_BF_KINEMATIC) != 0
		};
		bool bothKinematics = kinematicActors[0] && kinematicActors[1];
		bool dynamicActors[2] =
		{
			actors[0]->isDynamic() && actors[0]->readBodyFlag(NX_BF_KINEMATIC) == 0,
			actors[1]->isDynamic() && actors[1]->readBodyFlag(NX_BF_KINEMATIC) == 0
		};
		const physx::PxF32 masses[2] =
		{
			(dynamicActors[0] || bothKinematics) ? actors[0]->getMass() : 0,
			(dynamicActors[1] || bothKinematics) ? actors[1]->getMass() : 0
		};
#else
		const bool kinematicActors[2] =
		{
			actors[0]->isRigidDynamic() && (NxU32)(static_cast<PxRigidDynamic *>(actors[0])->getRigidDynamicFlags() & PxRigidDynamicFlag::eKINEMATIC) != 0,
			actors[1]->isRigidDynamic() && (NxU32)(static_cast<PxRigidDynamic *>(actors[1])->getRigidDynamicFlags() & PxRigidDynamicFlag::eKINEMATIC) != 0
		};
		bool bothKinematics = kinematicActors[0] && kinematicActors[1];
		bool dynamicActors[2] =
		{
			actors[0]->isRigidDynamic() && (NxU32)(static_cast<PxRigidDynamic *>(actors[0])->getRigidDynamicFlags() & PxRigidDynamicFlag::eKINEMATIC) == 0,
			actors[1]->isRigidDynamic() && (NxU32)(static_cast<PxRigidDynamic *>(actors[1])->getRigidDynamicFlags() & PxRigidDynamicFlag::eKINEMATIC) == 0
		};
		const physx::PxF32 masses[2] =
		{
			 (dynamicActors[0] || bothKinematics) ? static_cast<PxRigidDynamic *>(actors[0])->getMass() : 0,
			 (dynamicActors[1] || bothKinematics) ? static_cast<PxRigidDynamic *>(actors[1])->getMass() : 0
		};
#endif

		physx::PxF32 reducedMass;
		if (masses[0] == 0.0f)
		{
			reducedMass = masses[1];
		}
		else if (masses[1] == 0.0f)
		{
			reducedMass = masses[0];
		}
		else
		{
			reducedMass = masses[0] * masses[1] / (masses[0] + masses[1]);
		}

		for (int i = 0; i < 2; ++i)
		{
			if (destructibles[i] == NULL)
			{
				continue;
			}
			physx::PxF32 ivts = destructibles[i]->getDestructibleParameters().impactVelocityThreshold;
			ivts *= ivts;
			if (ivts < minImpactVelocityThresholdsSq)
			{
				minImpactVelocityThresholdsSq = ivts;
			}
		}

#if NX_SDK_VERSION_MAJOR == 2
		const NxVec3 velocities[2] =
		{
			(dynamicActors[0] || bothKinematics) ? actors[0]->getLinearVelocity() : NxVec3(0.0f),
			(dynamicActors[1] || bothKinematics) ? actors[1]->getLinearVelocity() : NxVec3(0.0f)
		};
#else
		const NxVec3 velocities[2] =
		{
			(dynamicActors[0] || bothKinematics) ? static_cast<PxRigidDynamic *>(actors[0])->getLinearVelocity() : NxVec3(0.0f),
			(dynamicActors[1] || bothKinematics) ? static_cast<PxRigidDynamic *>(actors[1])->getLinearVelocity() : NxVec3(0.0f)
		};
#endif

		const NxVec3 velocityDelta = velocities[0] - velocities[1];

		if (velocityDelta.magnitudeSquared() >= minImpactVelocityThresholdsSq)
		{
			for (int i = 0; i < 2; ++i)
			{
				if (destructibles[i] != NULL)
				{
					// this is not really physically correct, but at least its determenistic...
					NxVec3 force = (normal * normal.dot(velocityDelta)) * reducedMass * (i ? 1.0f : -1.0f);

					if (!force.isZero())
					{
						destructibles[i]->takeImpact(PXFROMNXVEC3(force), PXFROMNXVEC3(position), (physx::PxI16)chunkIndices[i], actors[i^1]);
					}
				}
			}
		}
	}
}
#endif // #if APEX_USE_GRB

//	Private interface, used by Destructible* classes

NxDestructibleActor* DestructibleScene::getDestructibleAndChunk(const NxShape* shape, physx::PxI32* chunkIndex) const
{
	if (chunkIndex)
	{
		*chunkIndex = NxModuleDestructibleConst::INVALID_CHUNK_INDEX;
	}

#if NX_SDK_VERSION_MAJOR == 2
	if (!mModule->owns(&shape->getActor()))
#elif NX_SDK_VERSION_MAJOR == 3
	if (!mModule->owns(shape->getActor()))
#endif	
	{
		return NULL;
	}

	DestructibleStructure::Chunk* chunk = getChunk((const NxShape*)shape);
	if (chunk == NULL)
	{
		return NULL;
	}

	DestructibleActor* destructible = mDestructibles.direct(chunk->destructibleID);

	if (destructible != NULL)
	{
		if (chunkIndex)
		{
			*chunkIndex = (physx::PxI32)chunk->indexInAsset;
		}
		return destructible->getAPI();
	}

	return NULL;
}

bool DestructibleScene::removeStructure(DestructibleStructure* structure, bool immediate)
{
	if (structure)
	{
		if (!immediate)
		{
			mStructureKillList.pushBack(structure);
		}
		else
		{
			for (physx::PxU32 destructibleIndex = structure->destructibles.size(); destructibleIndex--;)
			{
				DestructibleActor*& destructible = structure->destructibles[destructibleIndex];
				if (destructible)
				{
					destructible->setStructure(NULL);
					mDestructibles.direct(destructible->getID()) = NULL;
					mDestructibles.free(destructible->getID());
					destructible = NULL;
				}
			}
			mStructures.free(structure->ID);
			mStructures.direct(structure->ID) = NULL;
			delete structure;
		}
		return true;
	}

	return false;
}

void DestructibleScene::addActor(NiApexPhysXObjectDesc& desc, NxActor& actor, physx::PxF32 unscaledMass, bool isDebris, bool isGRB)
{
	uintptr_t& cindex = (uintptr_t&)desc.userData;

	if (actor.readBodyFlag(NX_BF_KINEMATIC) && cindex != 0)
	{
		// Remove from dormant list
		const physx::PxU32 index = (physx::PxU32)~cindex; 
		if (mDormantActors.free(index))
		{
			mDormantActors.direct(index).actor = NULL;
		}
		cindex = 0;
	}

	if (cindex != 0)
	{
		PX_ASSERT(!"Attempting to add an actor twice.\n");
		return;
	}

	// Add to FIFO
	cindex = ~(uintptr_t)mActorFIFO.size();
	physx::PxU32 entryFlags = 0;
	if (isDebris)
	{
		entryFlags |= ActorFIFOEntry::IsDebris;
	}
	if (isGRB)
	{
		entryFlags |= ActorFIFOEntry::IsGRB;
	}
	mActorFIFO.pushBack(ActorFIFOEntry(&actor, unscaledMass, entryFlags));

	// Initialize smoothed velocity so as not to get immediate sleeping
	ActorFIFOEntry& fifoEntry = mActorFIFO.back();
#if NX_SDK_VERSION_MAJOR == 2
	mPhysXScene->getGravity(NxFromPxVec3Fast(fifoEntry.averagedLinearVelocity));
#else
	SCOPED_PHYSX3_LOCK_READ(mPhysXScene);
	fifoEntry.averagedLinearVelocity = mPhysXScene->getGravity();
#endif
	fifoEntry.averagedAngularVelocity = physx::PxVec3(0.0f, 0.0f, PxTwoPi);

	++mDynamicActorFIFONum;
}

void DestructibleScene::capDynamicActorCount()
{
	if ((mModule->m_dynamicActorFIFOMax > 0 || mModule->m_chunkFIFOMax > 0) && !mModule->m_sortByBenefit)
	{
		while (mDynamicActorFIFONum > 0 && ((mModule->m_dynamicActorFIFOMax > 0 && mDynamicActorFIFONum > mModule->m_dynamicActorFIFOMax) ||
		        (mModule->m_chunkFIFOMax > 0 && mTotalChunkCount > mModule->m_chunkFIFOMax)))
		{
			ActorFIFOEntry& entry = mActorFIFO[mActorFIFO.size() - mDynamicActorFIFONum--];
			if (entry.actor != NULL)
			{
				destroyActorChunks(*entry.actor, NxApexChunkFlag::DESTROYED_FIFO_FULL);
			}
		}
	}
}

void DestructibleScene::removeReferencesToActor(DestructibleActor& destructible)
{
	if (destructible.getStructure() == NULL)
	{
		return;
	}

#if NX_SDK_VERSION_MAJOR == 3
	mApexScene->lockRead(__FILE__, __LINE__);
#endif

	// Remove from FIFO or dormant list
	const physx::PxU16* chunkIndexPtr = destructible.getVisibleChunks();
	const physx::PxU16* chunkIndexPtrStop = chunkIndexPtr + destructible.getNumVisibleChunks();
	while (chunkIndexPtr < chunkIndexPtrStop)
	{
		physx::PxU16 chunkIndex = *chunkIndexPtr++;
		DestructibleStructure::Chunk& chunk = destructible.getStructure()->chunks[chunkIndex + destructible.getFirstChunkIndex()];

		NxActor* actor = destructible.getStructure()->getChunkActor(chunk);
		if (actor == NULL)
		{
			continue;
		}

		if ((chunk.state & ChunkDynamic) != 0)
		{
			const NxApexPhysXObjectDesc* desc = mModule->mSdk->getPhysXObjectInfo(actor);
			PX_ASSERT(desc != NULL);

			physx::PxU32 index = ~(physx::PxU32)(uintptr_t)desc->userData;

			if (!actor->readBodyFlag(NX_BF_KINEMATIC))
			{
				// find the entry in the fifo
				if (index < mDynamicActorFIFONum)
				{
					mActorFIFO[index].actor = NULL;
				}
			}
			else
			{
				// find the actor in the dormant list
				if (mDormantActors.free(index))
				{
					mDormantActors.direct(index).actor = NULL;
				}
			}
		}
	}
#if NX_SDK_VERSION_MAJOR == 3
	mApexScene->unlockRead();
#endif


	destructible.setPhysXScene(NULL);

	destructible.removeSelfFromStructure();

	// Remove from kill list
	for (physx::PxU32 killListIndex = mActorKillList.size(); killListIndex--;)
	{
		bool removeActorFromList = false;
		NxActor* killListActor = mActorKillList[killListIndex];
		if (killListActor != NULL)
		{
			NiApexPhysXObjectDesc* actorObjDesc = (NiApexPhysXObjectDesc*)mModule->mSdk->getPhysXObjectInfo(killListActor);
			if (actorObjDesc != NULL)
			{
				for (physx::PxU32 i = actorObjDesc->mApexActors.size(); i--;)
				{
					if (actorObjDesc->mApexActors[i] == destructible.getAPI())
					{
						actorObjDesc->mApexActors.replaceWithLast(i);
					}
				}
				if (actorObjDesc->mApexActors.size() == 0)
				{
					releasePhysXActor(*killListActor);
					removeActorFromList = true;
				}
			}
			else
			{
				removeActorFromList = true;
			}
		}
		else
		{
			removeActorFromList = true;
		}

		if (removeActorFromList)
		{
			mActorKillList.replaceWithLast(killListIndex);
		}
	}

	PX_ASSERT(mDestructibles.usedCount() == 0 ? mActorKillList.size() == 0 : true);

	// Remove from damage and fracture buffers
	for (NxRingBuffer<DamageEvent>::It i(mDamageBuffer); i; ++i)
	{
		DamageEvent& e = *i;
		if (e.destructibleID == destructible.getID())
		{
			e.flags |= (physx::PxU32)DamageEvent::Invalid;
			//e.destructibleID = (physx::PxU32)DestructibleStructure::InvalidID;
		}
	}

	for (NxRingBuffer<FractureEvent>::It i(mFractureBuffer); i; ++i)
	{
		FractureEvent& e = *i;
		if (e.destructibleID == destructible.getID() && e.chunkIndexInAsset < destructible.getAsset()->getChunkCount())
		{
			e.flags |= (physx::PxU32)FractureEvent::Invalid;
			//e.destructibleID = (physx::PxU32)DestructibleStructure::InvalidID;
		}
	}

	for (NxRingBuffer<FractureEvent>::It i(mDeprioritisedFractureBuffer); i; ++i)
	{
		FractureEvent& e = *i;
		if (e.destructibleID == destructible.getID() && e.chunkIndexInAsset < destructible.getAsset()->getChunkCount())
		{
			e.flags |= (physx::PxU32)FractureEvent::Invalid;
			//e.destructibleID = (physx::PxU32)DestructibleStructure::InvalidID;
		}
	}
}

bool DestructibleScene::destroyActorChunks(NxActor& actor, physx::PxU32 chunkFlag)
{
	NiApexPhysXObjectDesc* actorObjDesc = (NiApexPhysXObjectDesc*) mModule->mSdk->getPhysXObjectInfo(&actor);
	if (!actorObjDesc)
	{
		return false;
	}
	if ((uintptr_t&)actorObjDesc->userData == 0)
	{
		return false;
	}

	// First collect a list of invariant destructible and chunk IDs
	physx::Array<IntPair> chunks;
	for (physx::PxU32 i = actor.getNbShapes(); i--;)
	{
		NxShape* shape = getShape(actor, i);
		DestructibleStructure::Chunk* chunk = getChunk(shape);
		if (chunk != NULL && chunk->isFirstShape(shape))	// BRG OPTIMIZE
		{
			getChunkReportData(*chunk, chunkFlag);
			if (chunk->state & ChunkVisible)
			{
				IntPair chunkIDs;
				chunkIDs.set((physx::PxI32)chunk->destructibleID, (physx::PxI32)chunk->indexInAsset);
				chunks.pushBack(chunkIDs);
			}
		}
	}

	// Now release the list of chunks
	for (physx::PxU32 i = 0; i < chunks.size(); ++i)
	{
		const IntPair& chunkIDs = chunks[i];
		DestructibleActor* dactor = mDestructibles.direct((physx::PxU32)chunkIDs.i0);
		if (dactor != NULL && dactor->getStructure() != NULL)
		{
			dactor->getStructure()->removeChunk(dactor->getStructure()->chunks[dactor->getFirstChunkIndex()+(physx::PxU16)chunkIDs.i1]);
			dactor->getStructure()->removeNxActorIslandReferences(actor);
		}
	}

	scheduleNxActorForDelete(*actorObjDesc);
	return true;
}

void DestructibleScene::releasePhysXActor(NxActor& actor)
{
	for (physx::PxU32 shapeN = 0; shapeN < actor.getNbShapes(); ++shapeN)
	{
		NxShape* shape = getShape(actor, shapeN);
		if (mModule->mSdk->getPhysXObjectInfo(shape))
		{
			mModule->mSdk->releaseObjectDesc(shape);
		}
	}

	for ( PxU32 damageReportIndex = 0; damageReportIndex < mDamageEventReportData.size(); ++damageReportIndex )
	{
		ApexDamageEventReportData& damageReport = mDamageEventReportData[damageReportIndex];

		if ( damageReport.impactDamageActor == &actor )
		{
			damageReport.impactDamageActor = NULL;
		}
	}

	if (getModule()->m_destructiblePhysXActorReport != NULL)
	{
		getModule()->m_destructiblePhysXActorReport->onPhysXActorRelease(actor);
	}

	mModule->mSdk->releaseObjectDesc(&actor);
#if NX_SDK_VERSION_MAJOR == 2
	actor.getScene().releaseActor(actor);
#elif NX_SDK_VERSION_MAJOR == 3
	SCOPED_PHYSX3_LOCK_WRITE(actor.getScene());
	actor.release();
#endif
}

bool DestructibleScene::scheduleChunkShapesForDelete(DestructibleStructure::Chunk& chunk)
{
	DestructibleActor* destructible = mDestructibles.direct(chunk.destructibleID);

	if ((chunk.state & ChunkVisible) == 0)
	{
//		PX_ASSERT(!"Cannot schedule shape for release from invisible chunk.\n");
		chunk.clearShapes();
		ChunkClearDescendents chunkOp(chunk.indexInAsset + destructible->getFirstChunkIndex());
		forSubtree(chunk, chunkOp, true);
		return true;
	}

	if (chunk.isDestroyed())
	{
		return false;
	}

	NxActor* actor = destructible->getStructure()->getChunkActor(chunk);

	NiApexPhysXObjectDesc* actorObjDesc = (NiApexPhysXObjectDesc*)mModule->mSdk->getPhysXObjectInfo(actor);
	if (!actorObjDesc)
	{
		PX_ASSERT(!"Cannot schedule actor for release that is not owned by APEX.\n");
		return false;
	}
	bool owned = false;
	for (physx::PxU32 i = 0; i < actorObjDesc->mApexActors.size(); ++i)
	{
		if (actorObjDesc->mApexActors[i] && actorObjDesc->mApexActors[i]->getOwner()->getObjTypeID() == DestructibleAsset::mAssetTypeID)
		{
			owned = true;
			break;
		}
	}
	if (!owned)
	{
		PX_ASSERT(!"Cannot schedule actor for release that is not owned by this module.\n");
		return false;
	}

#if USE_DESTRUCTIBLE_RWLOCK
	DestructibleScopedWriteLock destructibleWriteLock(*destructible);
#endif

#if !DISABLE_RENDER_LOCKS
	destructible->renderDataLock();
#endif

#if USE_CHUNK_RWLOCK
	DestructibleStructure::ChunkScopedWriteLock chunkWriteLock(chunk);
#endif

	// Update the mass
	physx::PxF32 mass = unscaleMass(actor->getMass());
	mass -= destructible->getChunkMass(chunk.indexInAsset);
	if (mass <= 0.0f)
	{
		mass = 1.0f;	// This should only occur if the last shape is deleted.  In this case, the mass won't matter.
	}

	physx::Array<NxShape*>&	shapes = destructible->getStructure()->getChunkShapes(chunk);
	for (physx::PxU32 i = 0; i < shapes.size(); ++i)
	{
		NxShape* shape = shapes[i];

		const NiApexPhysXObjectDesc* shapeObjDesc = (const NiApexPhysXObjectDesc*)mModule->mSdk->getPhysXObjectInfo(shape);
		if (!shapeObjDesc)
		{
			PX_ASSERT(!"Cannot schedule object for release that is not owned by APEX.\n");
			continue;
		}

		mModule->mSdk->releaseObjectDesc(shape);
#if NX_SDK_VERSION_MAJOR == 2
		actor->releaseShape(*shape);
#elif NX_SDK_VERSION_MAJOR == 3
		SCOPED_PHYSX3_LOCK_WRITE(mPhysXScene);
		((physx::PxRigidActor*)actor)->detachShape(*shape);
#endif
	}

	if (mTotalChunkCount > 0)
	{
		--mTotalChunkCount;
	}

	if (actor->getNbShapes() == 0)
	{
		uintptr_t& cindex = (uintptr_t&)actorObjDesc->userData;
		if (actor->readBodyFlag(NX_BF_KINEMATIC))
		{
			if (cindex != 0)
			{
				// Dormant actor
				const physx::PxU32 index = (physx::PxU32)~cindex; 
				if (mDormantActors.free(index))
				{
					mDormantActors.direct(index).actor = NULL;
				}
			}
			destructible->getStructure()->removeNxActorIslandReferences(*actor);
			scheduleNxActorForDelete(*actorObjDesc);
		}
		else// if(0 != (uintptr_t&)actorObjDesc->userData)
		{
			PX_ASSERT(cindex != 0);
			PX_ASSERT(mActorFIFO[(physx::PxU32)~cindex].actor == NULL || mActorFIFO[(physx::PxU32)~cindex].actor == actor);
			cindex = 0;
		}
	}
	else if (!actor->readBodyFlag(NX_BF_KINEMATIC))
	{
		if (actor->getNbShapes() > 0)
		{
			const uintptr_t cindex = (uintptr_t)actorObjDesc->userData;
			if (cindex != 0)
			{
				PX_ASSERT(mActorFIFO[(physx::PxU32)~cindex].actor == NULL || mActorFIFO[(physx::PxU32)~cindex].actor == actor);
				ActorFIFOEntry& FIFOEntry = mActorFIFO[(physx::PxU32)~cindex];
				FIFOEntry.unscaledMass = mass;
				FIFOEntry.flags |= ActorFIFOEntry::MassUpdateNeeded;
			}
		}
	}
	chunk.clearShapes();
	ChunkClearDescendents chunkOp(chunk.indexInAsset + destructible->getFirstChunkIndex());
	forSubtree(chunk, chunkOp, true);

	chunk.state &= ~(physx::PxU32)ChunkVisible;

	destructible->renderDataUnLock();
	return true;
}

bool DestructibleScene::scheduleNxActorForDelete(NiApexPhysXObjectDesc& actorDesc)
{
#if NX_SDK_VERSION_MAJOR == 3
	bool inScene = false;
	if (mPhysXScene)
	{
		mPhysXScene->lockRead();
		inScene = ((NxActor*)actorDesc.mPhysXObject)->getScene() != NULL;
		mPhysXScene->unlockRead();
	}
	if (inScene)
#endif
	{
		((NxActor*)actorDesc.mPhysXObject)->wakeUp();
	}
	mActorKillList.pushBack((NxActor*)actorDesc.mPhysXObject);
	return true;
}

void DestructibleScene::applyRadiusDamage(physx::PxF32 damage, physx::PxF32 momentum, const physx::PxVec3& position, physx::PxF32 radius, bool falloff)
{
	// Apply scene-based damage actor-based damage.  Those will split off islands.
	DamageEvent& damageEvent = mDamageBuffer.pushFront();
	damageEvent.damage = damage;
	damageEvent.momentum = momentum;
	damageEvent.position = position;
	damageEvent.direction = physx::PxVec3(0.0f); // not used
	damageEvent.radius = radius;
	damageEvent.chunkIndexInAsset = NxModuleDestructibleConst::INVALID_CHUNK_INDEX;
	damageEvent.flags = DamageEvent::UseRadius;
	if (falloff)
	{
		damageEvent.flags |= DamageEvent::HasFalloff;
	}
}


bool DestructibleScene::isActorCreationRateExceeded()
{
	return mNumActorsCreatedThisFrame >= mModule->m_maxActorsCreateablePerFrame;
}

bool DestructibleScene::isFractureBufferProcessRateExceeded()
{
	return mNumFracturesProcessedThisFrame >= mModule->m_maxFracturesProcessedPerFrame;
}

void DestructibleScene::addToAwakeList(DestructibleActor& actor)
{
	mAwakeActors.pushBack(&actor);
}

void DestructibleScene::removeFromAwakeList(DestructibleActor& actor)
{
	const physx::PxU32 numAwakeActors = mAwakeActors.size();
	physx::PxU32       found          = numAwakeActors;
	for (physx::PxU32 i = 0; i < numAwakeActors; i++)
	{
		if (&actor == mAwakeActors[i])
		{
			found = i;
			break;
		}
	}
	PX_ASSERT(found < numAwakeActors);
	if (found < numAwakeActors)
	{
		mAwakeActors.replaceWithLast(found);
	}
}

bool DestructibleScene::setMassScaling(physx::PxF32 massScale, physx::PxF32 scaledMassExponent)
{
	if (massScale > 0.0f && scaledMassExponent > 0.0f && scaledMassExponent <= 1.0f)
	{
		mMassScale = massScale;
		mMassScaleInv = 1.0f / mMassScale;
		mScaledMassExponent = scaledMassExponent;
		mScaledMassExponentInv = 1.0f / scaledMassExponent;
		return true;
	}

	return false;
}

physx::PxF32 DestructibleScene::setDamageBufferBudget(physx::PxF32 budget, physx::NxRingBuffer<DamageEvent> * userDamageBuffer /*= NULL*/)
{
	mCurrentBudget = budget;

	processFIFOForLOD();

	physx::PxF32 minCost, maxCost;
	calculateDamageBufferCostProfiles(minCost, maxCost, mDamageBuffer);

	//===SyncParams===
	if(NULL != userDamageBuffer)
	{
		physx::PxF32 minCostDummy, maxCostDummy;
		calculateDamageBufferCostProfiles(minCost, maxCost, *userDamageBuffer);
		PX_UNUSED(minCostDummy);
		PX_UNUSED(maxCostDummy);
	}

	physx::PxF32 totalCost = maxCost + mCurrentCostOfAllChunks;

	if (totalCost > budget)
	{
		const bool visualize = mApexScene->getDebugRenderParams() && mDebugRenderParams->LodBenefits != 0.0f;

		// Gather nonessential chunks from the destructibles

		const physx::PxF32 chunkUnitCost = mModule->getLODUnitCost();

		mNonessentialChunks.clear();
		for (physx::PxU32 destructibleNum = 0; destructibleNum < mDestructibles.usedCount(); ++destructibleNum)
		{
			DestructibleActor* destructible = mDestructibles.getUsed(destructibleNum);
			if (destructible != NULL)
			{
				const physx::PxU16* chunkIndexPtr = destructible->getVisibleChunks();
				const physx::PxU16* chunkIndexPtrStop = chunkIndexPtr + destructible->getNumVisibleChunks();
				DestructibleAssetParametersNS::Chunk_Type* assetChunks = destructible->getAsset()->mParams->chunks.buf;
				const physx::PxU16 essentialDepth = (physx::PxU16)destructible->getDestructibleParameters().essentialDepth;
				while (chunkIndexPtr < chunkIndexPtrStop)
				{
					physx::PxU16 chunkIndex = *chunkIndexPtr++;
					// We know it's visible by this iterator - if it's non-essential and dynamic, consider it for release
					if(assetChunks[chunkIndex].depth > essentialDepth && (destructible->getStructure()->chunks[chunkIndex + destructible->getFirstChunkIndex()].state & ChunkDynamic) != 0)
					{
						ChunkLODRef& chunkRef = mNonessentialChunks.insert();
						chunkRef.structureID = destructible->getStructure()->ID;
						chunkRef.chunkIndex = destructible->getFirstChunkIndex() + chunkIndex;
						const physx::PxU32 chunkShapeCount = destructible->getAsset()->getChunkHullCount(chunkIndex);
						chunkRef.benefit = destructible->getBenefit()*(physx::PxF32)chunkShapeCount/(physx::PxF32)physx::PxMax<physx::PxU32>(destructible->getVisibleDynamicChunkShapeCount(), 1);
						chunkRef.cost = chunkShapeCount * chunkUnitCost;
					}
				}
			}
		}

		// Sort benefit of chunks which can be eliminated
		if (mNonessentialChunks.size() > 1)
		{
			qsort(mNonessentialChunks.begin(), mNonessentialChunks.size(), sizeof(ChunkLODRef), ChunkLODRef::cmpBenefitDecreasing);
		}

		do
		{
			// Find event with minimum benefit/cost change when decrementing its process depth
			bool costReducingTransitionFound = false;
			DamageEvent* eventPtr = NULL;
			physx::PxF32 eventDeltaCost = NX_MAX_REAL;
			physx::PxF32 minEventBenefitToCost = NX_MAX_REAL;
			for (NxRingBuffer<DamageEvent>::It i(mDamageBuffer); i; ++i)
			{
				DamageEvent& event = *i;
				if (event.flags & DamageEvent::Invalid)
				{
					continue;
				}
				const physx::PxU32 processDepth = event.getProcessDepth();
				if (processDepth > event.minDepth)
				{
					const physx::PxF32 deltaBenefit = event.getBenefit(processDepth - 1) - event.getBenefit(processDepth);
					const physx::PxF32 deltaCost = event.getCost(processDepth - 1) - event.getCost(processDepth);
					if (deltaCost < 0)
					{
						const physx::PxF32 benefitToCost = deltaBenefit / deltaCost;
						if (benefitToCost < minEventBenefitToCost)
						{
							minEventBenefitToCost = benefitToCost;
							eventDeltaCost = deltaCost;
							eventPtr = &event;
							costReducingTransitionFound = true;
						}
					}
					if (!costReducingTransitionFound && deltaCost < eventDeltaCost)
					{
						eventDeltaCost = deltaCost;
						eventPtr = &event;
					}
				}
			}

			// Compare benefit/cost ratio with that obtained from eliminating min. benefit present chunk
			ChunkLODRef* ref = mNonessentialChunks.size() > 0 ? &mNonessentialChunks.back() : NULL;
			if (eventPtr == NULL && ref == NULL)
			{
				// Nothing left to reduce
				break;
			}

			if (eventPtr == NULL || (ref != NULL && ref->benefit < minEventBenefitToCost * ref->cost))
			{
				// Eliminating a chunk to reduce cost
				DestructibleStructure* structure = mStructures.direct(ref->structureID);
				DestructibleStructure::Chunk& chunk = structure->chunks[ref->chunkIndex];
				if (visualize && !chunk.isDestroyed())
				{
					LODReductionData& deletionData = mLODDeletionDataList.insert();
					deletionData.benefit = ref->benefit;
					deletionData.pos = structure->getChunkGlobalPose(chunk) * PXFROMNXVEC3(chunk.localSphere.center);
				}
				// Remove chunk
				IntPair& deadChunk = mChunkKillList.insert();
				deadChunk.i0 = ref->structureID;
				deadChunk.i1 = ref->chunkIndex;
				mNonessentialChunks.popBack();
				// Update cost
				totalCost -= ref->cost;
			}
			else
			{
				// Using a transition in the fracture buffer to reduce cost
				if (visualize)
				{
					LODReductionData& reductionData = mLODReductionDataList.insert();
					reductionData.benefit = minEventBenefitToCost / eventDeltaCost;
					reductionData.pos = eventPtr->position;
				}
				// Make the transition
				--eventPtr->processDepth;
				// Update cost
				totalCost += eventDeltaCost;
			}
		}
		while (totalCost > budget);	// Continue if we're still over-budget
	}

	return totalCost;
}

void DestructibleScene::processFractureBuffer()
{
	PX_PROFILER_PERF_DSCOPE("DestructibleProcessFractureBuffer", mFractureBuffer.size());

	mDamageEventReportIndices.resize(mDestructibles.usedCount());
	if (mDamageEventReportIndices.size() > 0)
	{
		memset(&mDamageEventReportIndices[0], 0xFF, sizeof(mDamageEventReportIndices[0])*mDamageEventReportIndices.size());
	}

	for (;
		(mFractureBuffer.size() > 0) && (!isFractureBufferProcessRateExceeded() && !isActorCreationRateExceeded());
		mFractureBuffer.popFront())
	{
		FractureEvent& fractureEvent = mFractureBuffer.front();
		PX_ASSERT(0 == (FractureEvent::SyncDirect & fractureEvent.flags) && (0 == (FractureEvent::SyncDerived & fractureEvent.flags)) && (0 == (FractureEvent::Manual & fractureEvent.flags)));

		if (fractureEvent.flags & FractureEvent::Invalid)
		{
			continue;
		}

		PX_ASSERT(fractureEvent.destructibleID < mDestructibles.capacity());
		DestructibleActor* destructible = mDestructibles.direct(fractureEvent.destructibleID);
		if (destructible && destructible->getStructure())
		{
			physx::PxU32& damageEventReportIndex = mDamageEventReportIndices[mDestructibles.getRank(fractureEvent.destructibleID)];
			if (damageEventReportIndex == 0xffffffff && mModule->m_chunkReport != NULL)
			{
				damageEventReportIndex = mDamageEventReportData.size();
				ApexDamageEventReportData& damageEventReport = mDamageEventReportData.insert();
				damageEventReport.setDestructible(destructible);
				damageEventReport.hitDirection = fractureEvent.hitDirection;
				damageEventReport.impactDamageActor = fractureEvent.impactDamageActor;
				damageEventReport.hitPosition = fractureEvent.position;
			}
			fractureEvent.damageEventIndex = damageEventReportIndex;

			destructible->getStructure()->fractureChunk(fractureEvent);
			++mNumFracturesProcessedThisFrame;
		}
	}
}

void DestructibleScene::processFractureBuffer(const physx::Array<SyncParams::UserFractureEvent> * userSource)
{
	// if we still have pending local fractures, we push any new incoming fractures into the deprioritised fracture buffer
	if(mFractureBuffer.size() > 0)
	{
		if(NULL != userSource)
		{
			for(physx::Array<SyncParams::UserFractureEvent>::ConstIterator iter = userSource->begin(); iter != userSource->end(); ++iter)
			{
				FractureEvent & fractureEvent = mSyncParams.interpret(*iter);
				PX_ASSERT(mSyncParams.assertUserFractureEventOk(fractureEvent, *this));
				PX_ASSERT(0 == (FractureEvent::SyncDirect & fractureEvent.flags));
				fractureEvent.flags |= FractureEvent::SyncDirect;
				mDeprioritisedFractureBuffer.pushBack() = fractureEvent;
			}
		}
	}
	// process deprioritised fractures only if local fractures have been processed, and any new incoming fractures only after clearing the backlog
	else
	{
		// reset damage event report data indices
		mDamageEventReportIndices.resize(mDestructibles.usedCount());
		if(!mDamageEventReportIndices.empty())
		{
			memset(&mDamageEventReportIndices[0], 0xFF, sizeof(mDamageEventReportIndices[0]) * mDamageEventReportIndices.size());
		}

		// process fractures from the sync buffer
#define DEPRIORITISED_CONDITION (mDeprioritisedFractureBuffer.size() > 0)
		PX_ASSERT(0 == mFractureBuffer.size());
		bool processingDeprioritised = true;
		const physx::PxU32 userSourceCount = (NULL != userSource) ? userSource->size() : 0;
		physx::PxU32 userSourceIndex = 0;
		for (;
			(DEPRIORITISED_CONDITION || (userSourceIndex < userSourceCount)) && (!isFractureBufferProcessRateExceeded() && !isActorCreationRateExceeded());
			processingDeprioritised ? mDeprioritisedFractureBuffer.popFront() : unfortunateCompilerWorkaround(++userSourceIndex))
		{
			processingDeprioritised = DEPRIORITISED_CONDITION;
#undef DEPRIORITISED_CONDITION
			FractureEvent & fractureEvent = processingDeprioritised ? mDeprioritisedFractureBuffer.front() : mSyncParams.interpret((*userSource)[userSourceIndex]);
			PX_ASSERT(!processingDeprioritised ? mSyncParams.assertUserFractureEventOk(fractureEvent, *this) :true);
			if(!processingDeprioritised)
			{
				PX_ASSERT(0 == (FractureEvent::SyncDirect & fractureEvent.flags));
				fractureEvent.flags |= FractureEvent::SyncDirect;
			}

			const bool usingEditFeature = false;
			if(usingEditFeature)
			{
				if(!processingDeprioritised)
				{
					const DestructibleActor::SyncParams & actorParams = mDestructibles.direct(fractureEvent.destructibleID)->getSyncParams();
					mSyncParams.interceptEdit(fractureEvent, actorParams);
				}
			}

			if(0 != (FractureEvent::Invalid & fractureEvent.flags))
			{
				continue;
			}

			PX_ASSERT(fractureEvent.destructibleID < mDestructibles.capacity());
			DestructibleActor * destructible = mDestructibles.direct(fractureEvent.destructibleID);
			if(NULL != destructible && NULL != destructible->getStructure())
			{
				// set damage event report data and fracture event chunk report data
				PX_ASSERT(mDestructibles.isUsed(fractureEvent.destructibleID) ? mDestructibles.getRank(fractureEvent.destructibleID) < mDamageEventReportIndices.size() : false);
				physx::PxU32 & damageEventReportIndex = mDamageEventReportIndices[mDestructibles.getRank(fractureEvent.destructibleID)];
				if(0xFFFFFFFF == damageEventReportIndex && NULL != mModule->m_chunkReport)
				{
					damageEventReportIndex = mDamageEventReportData.size();
					ApexDamageEventReportData & damageEventReport = mDamageEventReportData.insert();
					damageEventReport.setDestructible(destructible);
					damageEventReport.hitDirection = fractureEvent.hitDirection;
				}
				fractureEvent.damageEventIndex = damageEventReportIndex;

				// perform the fracturing
				destructible->getStructure()->fractureChunk(fractureEvent);
				++mNumFracturesProcessedThisFrame;
			}
		}

		// if we still have any incoming fractures unprocessed, we push them into the deprioritised fracture buffer
		if(NULL != userSource ? userSourceIndex < userSourceCount : false)
		{
			for(; userSourceIndex < userSourceCount; ++userSourceIndex)
			{
				FractureEvent & fractureEvent = mSyncParams.interpret((*userSource)[userSourceIndex]);
				PX_ASSERT(mSyncParams.assertUserFractureEventOk(fractureEvent, *this));
				PX_ASSERT(0 == (FractureEvent::SyncDirect & fractureEvent.flags));
				fractureEvent.flags |= FractureEvent::SyncDirect;
				mDeprioritisedFractureBuffer.pushBack() = fractureEvent;
			}
		}
	}

	// Move fracture event from Deferred fracture buffer to Deprioritised fracture buffer
	if(mDeferredFractureBuffer.size() > 0)
	{
		mDeprioritisedFractureBuffer.reserve(mDeprioritisedFractureBuffer.size() + mDeferredFractureBuffer.size());
		do
		{
			mDeprioritisedFractureBuffer.pushBack() = mDeferredFractureBuffer.front();
			mDeferredFractureBuffer.popFront();
		}
		while(mDeferredFractureBuffer.size() > 0);
	}
}

void DestructibleScene::processDamageColoringBuffer()
{
	for (; (mSyncDamageEventCoreDataBuffer.size() > 0) && (!isFractureBufferProcessRateExceeded() && !isActorCreationRateExceeded()); mSyncDamageEventCoreDataBuffer.popFront())
	{
		SyncDamageEventCoreDataParams& syncDamageEventCoreDataParams = mSyncDamageEventCoreDataBuffer.front();

		PX_ASSERT(syncDamageEventCoreDataParams.destructibleID < mDestructibles.capacity());
		DestructibleActor * destructible = mDestructibles.direct(syncDamageEventCoreDataParams.destructibleID);
		if (NULL != destructible && destructible->useDamageColoring())
		{
			destructible->applyDamageColoring_immediate(	syncDamageEventCoreDataParams.chunkIndexInAsset, 
														syncDamageEventCoreDataParams.position, 
														syncDamageEventCoreDataParams.damage, 
														syncDamageEventCoreDataParams.radius );
		}
	}
}
//-----------------------------------------------------------------------------
// DestructibleScene::processFIFOForLOD
// ------------------------------------
//
// Output:
// Destructibles[]->mLodVolumeAccumulator.mVolume			-
// Destructibles[]->mLodVolumeAccumulator.mCentroid			-
// Destructibles[]->mLodVolumeAccumulator.mVariance			-
// Destructibles[]->mLodVolumeAccumulator.mScalarAverage[]	-
// nonessentialChunks										-
// nonessentialChunks[].structureID							-
// nonessentialChunks[].chunkIndex							-
// nonessentialChunks[].benefit								-
// nonessentialChunks[].cost								-
// structures[]->actorForStaticChunksBenefitCache			-
// actorFIFO[].benefitCache                                 -
// mCurrentCostOfAllChunks                                  -
// mCurrentCostOfEssentialChunks                            -
// mLodSumBenefit
//
// Input:
// destructibles
// apexScene->mViewMatrices[]->viewMatrix					- used to get eye position
// actorFIFO
// structures
// structures[]->actorForStaticChunks						- PhysX or GRB actor
// structures[]->actorForStaticChunks->shapes				- PhysX or GRB shape list
// structures[]->actorForStaticChunksBenefitCache			-
// getChunk :: NxShape -> Chunk								-
// getChunk()->destructibleID								-
// getChunk()->localSphere.center					        -
// getChunk()->localSphere.radius							-
// getChunk()->shapes										- PhysX or GRB shape list
// getChunk()->shapes[0]->getGlobalPose()					-
// getChunk()->indexInAsset									-
// destructibles[]->mAge									-
// destructibles[]->->mInternalLodWeights					-
// destructibles[]->asset->mParams->chunks.buf[].depth		-
// destructibles[]->destructibleParameters.essentialDepth	-
// destructibles[]->structure->ID                           -
// destructibles[]->firstChunkIndex                         -
// actorFIFO												- fifo of PhysX or GRB actors
// actorFIFO.actor->shapes									- PhysX or GRB shape list
// actorFIFO.age                                            -
//
// Algorithm
// ---------
//
// for all used destructibles
//   clear mLodVolumeAccumulator
//
// Calculate benefit of static chunks, update lodVolumeAccumulator and
// actorForStaticChunksBenefitCache:
//
//   for all used structures that have an actorForStaticChunks
//     for all shapes in structure
//       get the shape's chunk
//       get the chunk's destructible
//       get the chunk's world centroid
//         (gets the globalPose of the first shape in the chunk from PhysX or
//          GRBand multiplies it by the chunk's local sphere center)
//       update the destructible's lodVolumeAccumulator
//       calculate the chunk's benefit
//         (involves a second call to NxShape::getGlobalPose())
//       update the structure's actorForStaticChunksBenefitCache
//
// Update actor fifo, gather nonessential chunks, update lodVolumeAccumulator:
//
//   for each entry in actor fifo
//     for each shape in actor
//       get the shape's chunk
//       get the chunk's destructible
//       update scene's mCurrentCostOfAllChunks
//       calculate chunkBenefit (involves a call to shape::getGlobalPose())
//       update entry's benefitCache
//       update scene's mCurrentCostOfEssentialChunks
//       if chunk is nonessential
//         add a reference to chunk to nonessentialChunks
//         get the chunk's world centroid
//           (involves a second call to NxShape::getGlobalPose()
//         update destructible's lodVolumeAccumulator
//
//   for all used destructibles
//     finalize lodVolumeAccumulator (calc centroid, variance and scalar average)
//     calculate benefit from lodVolumeAccumulator
//     update scene's mLodSumBenefit
//-----------------------------------------------------------------------------
void DestructibleScene::processFIFOForLOD()
{
	PX_PROFILER_PERF_SCOPE("DestructibleProcessFIFOForLOD");
	//if (!mModule->getLODEnabled())
	//	return;

	const physx::PxVec3 eyePos = mApexScene->getEyePosition();

	const physx::PxF32 chunksShapeCost = mModule->getLODUnitCost();

	mCurrentCostOfEssentialChunks = 0.0f;
	mCurrentCostOfAllChunks = 0.0f;

	{
		PX_PROFILER_PERF_SCOPE("DestructibleCalculateBenefit");

		mLodSumBenefit = 0.0f;
		for (physx::PxU32 destructibleNum = 0; destructibleNum < mDestructibles.usedCount(); ++destructibleNum)
		{
			DestructibleActor* destructible = mDestructibles.getUsed(destructibleNum);
			if (destructible != NULL)
			{
				destructible->setBenefit(calculateBenefit(destructible->getLODWeights(), destructible->getOriginalBounds().getCenter(), destructible->getOriginalBounds().getExtents().magnitudeSquared(), eyePos, destructible->getAge()));
				mLodSumBenefit += destructible->getBenefit();
				mCurrentCostOfAllChunks += destructible->getVisibleDynamicChunkShapeCount() * chunksShapeCost;
				mCurrentCostOfEssentialChunks += destructible->getEssentialVisibleDynamicChunkShapeCount() * chunksShapeCost;
			}
		}
	}
}


void DestructibleScene::calculateDamageBufferCostProfiles(physx::PxF32& minCost, physx::PxF32& maxCost, physx::NxRingBuffer<DamageEvent> & subjectDamageBuffer)
{
	physx::Array<physx::PxU8*> trail;
	physx::Array<physx::PxU8> undo;

	const physx::PxVec3 eyePos = mApexScene->getEyePosition();

	minCost = 0;
	maxCost = 0;

	for (NxRingBuffer<DamageEvent>::It i(subjectDamageBuffer); i; ++i)
	{
		DamageEvent& e = *i;

		if (e.flags & DamageEvent::Invalid)
		{
			continue;
		}

		// Calculate costs and benefits for this event
		physx::PxF32 realCost = 0;
		physx::PxF32 realBenefit = 0;
		for (physx::PxU32 depth = 0; depth <= e.getMaxDepth(); ++depth)
		{
			physx::PxF32 depthCost = realCost;	// Cost & benefit at depth include only real events at lower depths
			physx::PxF32 depthBenefit = realBenefit;
			physx::Array<FractureEvent>& fractures = e.fractures[depth];
			for (physx::PxU32 j = 0; j < fractures.size(); ++j)
			{
				FractureEvent& fractureEvent = fractures[j];
				physx::PxF32 eventCost = 0;
				physx::PxF32 eventBenefit = 0;
				physx::PxU32 oldTrailSize = trail.size();
				calculatePotentialCostAndBenefit(eventCost, eventBenefit, trail, undo, fractureEvent);
				depthCost += eventCost;	// Cost & benefit at depth include virtual events at this depth
				depthBenefit += eventBenefit;
				if ((fractureEvent.flags & FractureEvent::Virtual) == 0)
				{
					// Add real cost
					realCost += eventCost;
					realBenefit += eventBenefit;
				}
				else
				{
					// Undo chunk temporary state changes from virtual event
					physx::PxU8** trailMark = trail.begin() + oldTrailSize;
					physx::PxU8** trailEnd = trail.end();
					physx::PxU8* undoEnd = undo.end();
					while (trailEnd-- > trailMark)
					{
						PX_ASSERT(undoEnd > undo.begin());
						*(*trailEnd) = (physx::PxU8)(((**trailEnd)&~(physx::PxU8)ChunkTempMask) | (*--undoEnd));
					}
					trail.resize((physx::PxU32)(trailMark - trail.begin()));
				}
				undo.reset();
			}
			e.cost[depth] = depthCost;
			e.benefit[depth] = depthBenefit;
		}

		minCost += e.getCost(e.getMinDepth());
		maxCost += e.getCost(e.getMaxDepth());
		e.processDepth = e.getMaxDepth();

		// Cover trail
		for (physx::PxU8** statePtr = trail.begin(); statePtr < trail.end(); ++statePtr)
		{
			*(*statePtr) &= ~(physx::PxU8)ChunkTempMask;
		}
		trail.reset();
	}
}

physx::PxF32 DestructibleScene::processEventBuffers(physx::PxF32 resourceBudget)
{
	physx::PxF32 taken = 0.0f;

	//===SyncParams=== prevent user from changing sync params during this phase. do this just before the first encounter of working with sync params in the program
	//PX_ASSERT(!mSyncParams.lockSyncParams);
	mSyncParams.lockSyncParams = true;

	//===SyncParams=== prepare user's damage event buffer, if available
	UserDamageEventHandler * callback = NULL;
	callback = mModule->getSyncParams().getUserDamageEventHandler();
	const physx::Array<SyncParams::UserDamageEvent> * userSource = NULL;
	if(NULL != callback)
	{
		mSyncParams.onPreProcessReadData(*callback, userSource);
	}

	//process damage buffer's damageEvents
	physx::NxRingBuffer<DamageEvent> userDamageBuffer;
	const physx::NxRingBuffer<DamageEvent> * const prePopulateAddress = &userDamageBuffer;
	PX_ASSERT(0 == userDamageBuffer.size());
	generateFractureProfilesInDamageBuffer(userDamageBuffer, userSource);
	PX_ASSERT((NULL != userSource) ? (0 != userDamageBuffer.size()) : (0 == userDamageBuffer.size()));
	userSource = NULL;
	const physx::NxRingBuffer<DamageEvent> * const postPopulateAddress = &userDamageBuffer;
	PX_ASSERT(prePopulateAddress == postPopulateAddress);
	PX_UNUSED(prePopulateAddress);
	PX_UNUSED(postPopulateAddress);
	taken += setDamageBufferBudget(resourceBudget, (0 != userDamageBuffer.size()) ? &userDamageBuffer : NULL);

	//===SyncParams=== give the user the damage event buffer. damage event buffer must be fully populated and locked during this call
	if(NULL != callback)
	{
		mSyncParams.onProcessWriteData(*callback, mDamageBuffer);
	}

	//pop damage buffer, push fracture buffer. note that local fracture buffer may be 'contaminated' with non-local fractureEvents, which are derivatives of the user's damageEvents
	fillFractureBufferFromDamage((0 != userDamageBuffer.size()) ? &userDamageBuffer : NULL);
	userDamageBuffer.clear();

	//===SyncParams=== done with user's damage event buffer, if available
	if(NULL != callback)
	{
		mSyncParams.onPostProcessReadData(*callback);
	}
	callback = NULL;

	return taken;
}

void DestructibleScene::generateFractureProfilesInDamageBuffer(physx::NxRingBuffer<DamageEvent> & userDamageBuffer, const physx::Array<SyncParams::UserDamageEvent> * userSource /*= NULL*/)
{
	
#define LOCAL_CONDITION (eventN < mDamageBuffer.size())
	bool processingLocal = true;
	for (physx::PxU32 eventN = 0 , userEventN = 0, userEventCount = (NULL != userSource && NULL != &userDamageBuffer) ? userSource->size() : 0;
		 LOCAL_CONDITION || (userEventN < userEventCount);
		 processingLocal ? ++eventN : ++userEventN)
	{
		processingLocal = LOCAL_CONDITION;
#undef LOCAL_CONDITION
		//===SyncParams===
		DamageEvent& damageEvent = processingLocal ? mDamageBuffer[eventN] : mSyncParams.interpret((*userSource)[userEventN]);
		PX_ASSERT(!processingLocal ? mSyncParams.assertUserDamageEventOk(damageEvent, *this) : true);
		const bool usingEditFeature = false;
		if(usingEditFeature)
		{
			if(!processingLocal)
			{
				const DestructibleActor::SyncParams & actorParams = mDestructibles.direct(damageEvent.destructibleID)->getSyncParams();
				mSyncParams.interceptEdit(damageEvent, actorParams);
			}
		}

		for (physx::PxU32 i = DamageEvent::MaxDepth + 1; i--;)
		{
			damageEvent.fractures[i].reset();
		}

		if (damageEvent.flags & DamageEvent::Invalid)
		{
			continue;
		}

#if APEX_RUNTIME_FRACTURE
#if NX_SDK_VERSION_MAJOR==3
		if ((damageEvent.flags & DamageEvent::IsFromImpact) == 0)
#endif
		{
			// TODO: Remove this, and properly process the damage events
			for (physx::PxU32 i = 0; i < mDestructibles.usedCount(); ++i)
			{
				physx::fracture::Actor* rtActor = mDestructibles.getUsed(i)->getRTActor();
				if (rtActor != NULL && rtActor->patternFracture(damageEvent))
				{
					break;
				}
			}
		}
#endif

		if (damageEvent.destructibleID == (physx::PxU32)DestructibleActor::InvalidID)
		{
			// Scene-based damage.  Must find destructibles.
#if NX_SDK_VERSION_MAJOR == 2
			OverlapSphereShapesReport overlapReport(this);
			NxShape* shapesArray[100];
			NxSphere sphere(NXFROMPXVEC3(damageEvent.position), damageEvent.radius);
			mPhysXScene->overlapSphereShapes(sphere, NX_DYNAMIC_SHAPES, 100, shapesArray, &overlapReport);
			if (overlapReport.destructibles.size())
			{
				qsort(overlapReport.destructibles.begin(), overlapReport.destructibles.size(), sizeof(DestructibleActor*), comparePointers);
				DestructibleActor* lastDestructible = NULL;
				for (physx::PxU32 i = 0; i < overlapReport.destructibles.size(); ++i)
				{
					DestructibleActor* overlapDestructible = overlapReport.destructibles[i];
					if (overlapDestructible != lastDestructible)
					{
						if (overlapDestructible->getStructure() != NULL)
						{
							// Expand damage buffer
							DamageEvent& newEvent 	= processingLocal ? mDamageBuffer.pushBack()	: userDamageBuffer.pushBack();
							newEvent 				= processingLocal ? mDamageBuffer[eventN]		: userDamageBuffer[userEventN];	// Need to use indexed access again; damageEvent may now be invalid if the buffer resized
							newEvent.destructibleID = overlapDestructible->getID();
							const PxU32 maxLOD = overlapDestructible->getAsset()->getDepthCount() > 0 ? overlapDestructible->getAsset()->getDepthCount() - 1 : 0;
							newEvent.minDepth = physx::PxMin(overlapDestructible->getLOD(), maxLOD);
							newEvent.maxDepth = damageEvent.minDepth;
						}
						lastDestructible = overlapDestructible;
					}
				}
			}
#elif NX_SDK_VERSION_MAJOR == 3
			PxSphereGeometry sphere(damageEvent.radius);

			PxOverlapBuffer ovBuffer(&mOverlapHits[0], MAX_SHAPE_COUNT);
			mPhysXScene->lockRead();
			mPhysXScene->overlap(sphere, PxTransform(damageEvent.position), ovBuffer);
			mPhysXScene->unlockRead();
			PxI32	nbHits	= ovBuffer.getNbAnyHits();
			nbHits	= nbHits >= 0 ? nbHits : MAX_SHAPE_COUNT;

			if (nbHits)
			{
				qsort(&mOverlapHits[0], nbHits, sizeof(PxOverlapHit), compareOverlapHitShapePointers);
				DestructibleActor* lastDestructible = NULL;
				for (physx::PxI32 i = 0; i < nbHits; ++i)
				{
					DestructibleActor* overlapDestructible = NULL;
					DestructibleStructure::Chunk*	chunk	= getChunk((const NxShape*)mOverlapHits[i].shape);
					if (chunk != NULL)
					{
						overlapDestructible	= mDestructibles.direct(chunk->destructibleID);
					}

					if (overlapDestructible != lastDestructible)
					{
						if (overlapDestructible->getStructure() != NULL)
						{
							// Expand damage buffer
							DamageEvent& newEvent 	= processingLocal ? mDamageBuffer.pushBack()	: userDamageBuffer.pushBack();
							newEvent 				= processingLocal ? mDamageBuffer[eventN]		: userDamageBuffer[userEventN];	// Need to use indexed access again; damageEvent may now be invalid if the buffer resized
							newEvent.destructibleID = overlapDestructible->getID();
							const physx::PxU32 maxLOD = overlapDestructible->getAsset()->getDepthCount() > 0 ? overlapDestructible->getAsset()->getDepthCount() - 1 : 0;
							newEvent.minDepth = physx::PxMin(overlapDestructible->getLOD(), maxLOD);
							newEvent.maxDepth = damageEvent.minDepth;
						}
						lastDestructible = overlapDestructible;
					}
				}
			}
#endif
		}
		else
		{
			// Actor-based damage
			DestructibleActor* destructible = mDestructibles.direct(damageEvent.destructibleID);
			if (destructible == NULL)
			{
				continue;
			}

#if !DISABLE_RENDER_LOCKS
			destructible->renderDataLock();
#endif
			if(!processingLocal)
			{
				PX_ASSERT(0 == (DamageEvent::SyncDirect & damageEvent.flags));
				damageEvent.flags |= DamageEvent::SyncDirect;
			}
			const PxU32 maxLOD = destructible->getAsset()->getDepthCount() > 0 ? destructible->getAsset()->getDepthCount() - 1 : 0;
			damageEvent.minDepth = physx::PxMin(destructible->getLOD(), maxLOD);
			damageEvent.maxDepth = damageEvent.minDepth;
			if ((damageEvent.flags & DamageEvent::UseRadius) == 0)
			{
                destructible->applyDamage_immediate(damageEvent);
			}
			else
			{
				destructible->applyRadiusDamage_immediate(damageEvent);
			}
			destructible->renderDataUnLock();
		}

		//===SyncParams===
		if(!processingLocal)
		{
			PX_ASSERT(0 != (DamageEvent::SyncDirect & damageEvent.flags));
			userDamageBuffer.pushBack() = damageEvent;
		}
	}
}

void DestructibleScene::fillFractureBufferFromDamage(physx::NxRingBuffer<DamageEvent> * userDamageBuffer /*= NULL*/)
{
#define LOCAL_CONDITION (mDamageBuffer.size() > 0)
	bool processingLocal = true;
	for (physx::PxU32 userEventN = 0, userEventCount = (NULL != userDamageBuffer) ? userDamageBuffer->size() : 0;
		 LOCAL_CONDITION || (userEventN < userEventCount);
		 processingLocal ? mDamageBuffer.popFront() : unfortunateCompilerWorkaround(++userEventN))
	{
        processingLocal = LOCAL_CONDITION;
#undef LOCAL_CONDITION
		//===SyncParams===
		DamageEvent& damageEvent = processingLocal ? mDamageBuffer.front() : (*userDamageBuffer)[userEventN];

		if (damageEvent.flags & DamageEvent::Invalid)
		{
			continue;
		}
		for (physx::PxU32 depth = 0; depth <= damageEvent.getProcessDepth(); ++depth)
		{
			const bool atProcessDepth = depth == damageEvent.getProcessDepth();
			physx::Array<FractureEvent>& buffer = damageEvent.fractures[depth];
			for (physx::PxU32 i = 0; i < buffer.size(); ++i)
			{
				FractureEvent& fractureEvent = buffer[i];
				if (atProcessDepth || (fractureEvent.flags & FractureEvent::Virtual) == 0)
				{
					fractureEvent.hitDirection = damageEvent.direction;
					fractureEvent.impactDamageActor = damageEvent.impactDamageActor;
					fractureEvent.position = damageEvent.position;
					fractureEvent.appliedDamageUserData = damageEvent.appliedDamageUserData;

					//===SyncParams===
					PX_ASSERT(0 == (FractureEvent::DeleteChunk & fractureEvent.flags));
					if(mDestructibles.direct(damageEvent.destructibleID)->mInDeleteChunkMode)
					{
						fractureEvent.flags |= FractureEvent::DeleteChunk;
					}

					if(!processingLocal)
					{
						PX_ASSERT(0 != (FractureEvent::SyncDerived & fractureEvent.flags));
						mDeprioritisedFractureBuffer.pushBack() = fractureEvent;
					}
					else
					{
						PX_ASSERT(0 == (FractureEvent::SyncDirect & fractureEvent.flags) && (0 == (FractureEvent::SyncDerived & fractureEvent.flags)) && (0 == (FractureEvent::Manual & fractureEvent.flags)));
						mFractureBuffer.pushBack() = fractureEvent;
					}
				}
			}
		}
    }
}

static ApexStatsInfo DestructionStatsData[] =
{
	{"VisibleDestructibleChunkCount",		ApexStatDataType::INT, {{0}} },
	{"DynamicDestructibleChunkIslandCount",	ApexStatDataType::INT, {{0}} },
	{"NumberOfShapes",						ApexStatDataType::INT, {{0}} },
	{"NumberOfAwakeShapes",					ApexStatDataType::INT, {{0}} },
	{"NumberOfGPUSimulatedShapes",			ApexStatDataType::INT, {{0}} },
	{"NumberOfGpuShapePairs",				ApexStatDataType::INT, {{0}} },
	{"GpuRbSimulationTime",					ApexStatDataType::FLOAT, {{0}} },
	{"RbThroughput(Mpair/sec)",				ApexStatDataType::FLOAT, {{0}} },
	{"GpuSceneMemoryUsed (MB)",				ApexStatDataType::FLOAT, {{0}} },
	{"GpuTempMemoryUsed (MB)",				ApexStatDataType::FLOAT, {{0}} },
	{"GpuRbStaticConvexMaxEdgeCount",		ApexStatDataType::INT, {{0}} },
	{"GpuRbDynamicConvexMaxEdgeCount",		ApexStatDataType::INT, {{0}} },
	{"GpuRbKinematicConvexMaxEdgeCount",	ApexStatDataType::INT, {{0}} },
	{"NumberOfGpuDynamicActorsAwake",		ApexStatDataType::INT, {{0}} },
	{"NumberOfGpuDynamicActorsSleeping",	ApexStatDataType::INT, {{0}} }
};

void DestructibleScene::createModuleStats(void)
{
	mModuleSceneStats.numApexStats		= NumberOfStats;
	mModuleSceneStats.ApexStatsInfoPtr	= (ApexStatsInfo*)PX_ALLOC(sizeof(ApexStatsInfo) * NumberOfStats, PX_DEBUG_EXP("ApexStatsInfo"));

	for (physx::PxU32 i = 0; i < NumberOfStats; i++)
	{
		mModuleSceneStats.ApexStatsInfoPtr[i] = DestructionStatsData[i];
	}
}

void DestructibleScene::destroyModuleStats(void)
{
	mModuleSceneStats.numApexStats = 0;
	if (mModuleSceneStats.ApexStatsInfoPtr)
	{
		PX_FREE_AND_RESET(mModuleSceneStats.ApexStatsInfoPtr);
	}
}

void DestructibleScene::setStatValue(physx::PxI32 index, ApexStatValue dataVal)
{
	if (mModuleSceneStats.ApexStatsInfoPtr)
	{
		mModuleSceneStats.ApexStatsInfoPtr[index].StatCurrentValue = dataVal;
	}
}

NxApexSceneStats* DestructibleScene::getStats()
{
	{	// CPU/GPU agnositic stats
		unsigned totalVisibleChunkCount = 0;
		unsigned totalDynamicIslandCount = 0;
		for (unsigned i = 0; i < mStructures.usedCount(); ++i)
		{
			DestructibleStructure* s = mStructures.getUsed(i);
			for (unsigned j = 0; j < s->destructibles.size(); ++j)
			{
				DestructibleActor* a = s->destructibles[j];
				const unsigned visibleChunkCount = a->getNumVisibleChunks();
				totalVisibleChunkCount += visibleChunkCount;
				#if NX_SDK_VERSION_MAJOR == 2
						NxActor** buffer;
				#elif NX_SDK_VERSION_MAJOR == 3
						physx::PxRigidDynamic** buffer;
				#endif
				unsigned bufferSize;
				if (a->acquirePhysXActorBuffer(buffer, bufferSize, NxDestructiblePhysXActorQueryFlags::Dynamic, true))
				{
					totalDynamicIslandCount += bufferSize;
					a->releasePhysXActorBuffer();
				}
			}
		}

#if 0	// Issue warnings if these don't align with updated values
		if (mTotalChunkCount != totalVisibleChunkCount)
		{
			APEX_DEBUG_WARNING("mTotalChunkCount = %d, actual total visible chunk count = %d.\n", mTotalChunkCount, totalVisibleChunkCount);
		}
		if (mDynamicActorFIFONum != totalDynamicIslandCount)
		{
			APEX_DEBUG_WARNING("mDynamicActorFIFONum = %d, actual total dynamic chunk island count = %d.\n", mDynamicActorFIFONum, totalDynamicIslandCount);
		}
		if (mModule->m_chunkFIFOMax > 0 && mTotalChunkCount > mModule->m_chunkFIFOMax)
		{
			APEX_DEBUG_WARNING("mTotalChunkCount = %d, mModule->m_chunkFIFOMax = %d.\n", mTotalChunkCount, mModule->m_chunkFIFOMax);
		}
		if (mModule->m_dynamicActorFIFOMax > 0 && mDynamicActorFIFONum > mModule->m_dynamicActorFIFOMax)
		{
			APEX_DEBUG_WARNING("mDynamicActorFIFONum = %d, mModule->m_dynamicActorFIFOMax = %d.\n", mDynamicActorFIFONum, mModule->m_dynamicActorFIFOMax);
		}
#endif

		ApexStatValue dataVal;

		dataVal.Int = (int)totalVisibleChunkCount - mPreviousVisibleDestructibleChunkCount;
		mPreviousVisibleDestructibleChunkCount = (int)totalVisibleChunkCount;
		setStatValue(VisibleDestructibleChunkCount, dataVal);

		dataVal.Int = (int)totalDynamicIslandCount - mPreviousDynamicDestructibleChunkIslandCount;
		mPreviousDynamicDestructibleChunkIslandCount = (int)totalDynamicIslandCount;
		setStatValue(DynamicDestructibleChunkIslandCount, dataVal);
	}

#if APEX_USE_GRB
	if (mGrbScene && isGrbSimulationEnabled())
	{
		ApexStatValue dataVal;

#if NX_SDK_VERSION_MAJOR==2
		const NxSceneStats2* grbStats = mGrbScene->getStats2();
#else
		const GrbSimulationStatistics3 * grbStats = ((GrbScene3*)mGrbScene)->getSimulationStatistics();
#endif

		physx::PxI32 numConvexes = 0, numPairs = 0, sEdgeCount = 0, dEdgeCount = 0, kEdgeCount = 0;
		physx::PxI32 numAwakeActors = 0, numSleepingActors = 0;
		physx::PxI32 memSceneUsed = 0, memTempDataUsed = 0;
		for (physx::PxU32 i = 0; i < grbStats->numStats; ++i)
		{
			if (0 == strcmp(grbStats->stats[i].name, "TotalConvexShapes"))
			{
				numConvexes = grbStats->stats[i].curValue;
			}
			else if (0 == strcmp(grbStats->stats[i].name, "TotalPairs"))
			{
				numPairs = grbStats->stats[i].curValue;
			}
			else if (0 == strcmp(grbStats->stats[i].name, "GpuSceneMemoryUsed"))
			{
				memSceneUsed = grbStats->stats[i].curValue;
			}
			else if (0 == strcmp(grbStats->stats[i].name, "GpuTempMemoryUsed"))
			{
				memTempDataUsed = grbStats->stats[i].curValue;
			}
			else if (0 == strcmp(grbStats->stats[i].name, "StaticConvexMaxEdgeCount"))
			{
				sEdgeCount = grbStats->stats[i].curValue;
			}
			else if (0 == strcmp(grbStats->stats[i].name, "DynamicConvexMaxEdgeCount"))
			{
				dEdgeCount = grbStats->stats[i].curValue;
			}
			else if (0 == strcmp(grbStats->stats[i].name, "KinematicConvexMaxEdgeCount"))
			{
				kEdgeCount = grbStats->stats[i].curValue;
			}
			else if (0 == strcmp(grbStats->stats[i].name, "TotalDynamicActorsInAwakeGroups"))
			{
				numAwakeActors = grbStats->stats[i].curValue;
		}
			else if (0 == strcmp(grbStats->stats[i].name, "TotalDynamicActorsSleeping"))
			{
				numSleepingActors = grbStats->stats[i].curValue;
			}
		}

		dataVal.Int = numConvexes;
		setStatValue(NumberOfShapes, dataVal);
		//setStatValue(NumberOfGpuSimulatedShapes,dataVal);
		setStatValue(NumberOfAwakeShapes, dataVal);

		dataVal.Int = numPairs;
		setStatValue(NumberOfGpuShapePairs, dataVal);

		dataVal.Float = (float)memSceneUsed/(1024.0f*1024.0f);
		setStatValue(GpuSceneMemoryUsed, dataVal);
		
		dataVal.Float = (float)memTempDataUsed/(1024.0f*1024.0f);
		setStatValue(GpuTempMemoryUsed, dataVal);

		dataVal.Int = sEdgeCount;
		setStatValue(GpuRbStaticConvexMaxEdgeCount, dataVal);

		dataVal.Int = dEdgeCount;
		setStatValue(GpuRbDynamicConvexMaxEdgeCount, dataVal);

		dataVal.Int = kEdgeCount;
		setStatValue(GpuRbKinematicConvexMaxEdgeCount, dataVal);

		dataVal.Int = numAwakeActors;
		setStatValue(NumberOfGpuDynamicActorsAwake, dataVal);

		dataVal.Int = numSleepingActors;
		setStatValue(NumberOfGpuDynamicActorsSleeping, dataVal);
	}
	else
#endif
	{
		ApexStatValue dataVal;
		dataVal.Int = 0;
		setStatValue(NumberOfShapes, dataVal);
		setStatValue(NumberOfGPUSimulatedShapes, dataVal);
		setStatValue(NumberOfAwakeShapes, dataVal);
		setStatValue(NumberOfGpuShapePairs, dataVal);
		setStatValue(RbThroughput, dataVal);
		setStatValue(GpuSceneMemoryUsed, dataVal);
		setStatValue(GpuTempMemoryUsed, dataVal);
		setStatValue(GpuRbStaticConvexMaxEdgeCount, dataVal);
		setStatValue(GpuRbDynamicConvexMaxEdgeCount, dataVal);
		setStatValue(GpuRbKinematicConvexMaxEdgeCount, dataVal);
		setStatValue(NumberOfGpuDynamicActorsAwake, dataVal);
		setStatValue(NumberOfGpuDynamicActorsSleeping, dataVal);
	}

	return &mModuleSceneStats;
}

///////////////////////////////////////////////////////////////////////////

bool DestructibleScene::ChunkLoadState::execute(DestructibleStructure* structure, DestructibleStructure::Chunk& chunk)
{
	DestructibleScene& scene        = *structure->dscene;
	DestructibleActor& destructible = *scene.mDestructibles.direct(chunk.destructibleID);

	const physx::PxU16 chunkIndex  = chunk.indexInAsset;
	const physx::PxU16 parentIndex = destructible.getAsset()->getChunkParentIndex(chunkIndex);
	const bool chunkDynamic        = destructible.getInitialChunkDynamic(chunkIndex);
	const bool chunkVisible        = destructible.getInitialChunkVisible(chunkIndex);
	const bool chunkDestroyed      = destructible.getInitialChunkDestroyed(chunkIndex);
	bool chunkRecurse              = true;

	// Destroy the chunk if necessary
	if (chunkDestroyed && (!chunk.isDestroyed() || chunkVisible))
	{
		structure->removeChunk(chunk);
		structure->supportInvalid = true;
		chunkRecurse = true;
	}
	// Create the chunk actor or append the chunk shapes to the island's actor
	else if (chunkVisible && chunk.getShapeCount() == 0)
	{
		NxActor* chunkActor = NULL;
		bool     chunkActorCreated = false;

		// Visible chunks not in an island require their own NxActor
		if (chunk.islandID == (physx::PxU32)DestructibleStructure::InvalidID)
		{
			chunk.state &= ~(physx::PxU8)ChunkVisible;
			chunkActor = scene.createRoot(chunk, physx::PxMat34Legacy(destructible.getInitialChunkGlobalPose(chunkIndex)), chunkDynamic, NULL, false);
			chunkActorCreated = true;
		}
		else
		{
			// Check if the island actor has already been created
			NxActor* actor = NULL;
			if (!chunkDynamic && structure->actorForStaticChunks != NULL)
			{
				actor = structure->actorForStaticChunks;
				chunk.islandID = structure->actorToIsland[actor];
			}
			else
			{
				actor = structure->islandToActor[chunk.islandID];
			}
			if (NULL == actor)
			{
				// Create the island's actor
				chunk.state &= ~(physx::PxU8)ChunkVisible;
				actor = scene.createRoot(chunk, physx::PxMat34Legacy(destructible.getInitialChunkGlobalPose(chunkIndex)), chunkDynamic, NULL, false);
				structure->islandToActor[chunk.islandID] = actor;
				if (!chunkDynamic)
				{
					structure->actorForStaticChunks = actor;
				}
				chunkActorCreated = true;
			}
			else
			{
				// Append the chunk shapes to the existing island actor
				physx::PxMat34Legacy islandPose, relTM;
				PxFromNxMat34(islandPose, actor->getGlobalPose());
				relTM.multiplyInverseRTLeft(islandPose, physx::PxMat34Legacy(destructible.getInitialChunkGlobalPose(chunkIndex)));
					
#if		APEX_USE_GRB
				chunk.setGRB(scene.isGrbSimulationEnabled());
#else
				chunk.setGRB(false);
#endif
				
				chunk.state &= ~(physx::PxU8)ChunkVisible;
				if (scene.appendShapes(chunk, chunkDynamic, &relTM, actor))
				{
					NiApexPhysXObjectDesc* objDesc = scene.mModule->mSdk->getGenericPhysXObjectInfo(actor);
					if(objDesc->mApexActors.find(destructible.getAPI()) == objDesc->mApexActors.end())
					{
						objDesc->mApexActors.pushBack(destructible.getAPI());
						destructible.incrementWakeCount();
					}
				}
			}
			chunkActor = actor;
		}

		PX_ASSERT(!chunk.isDestroyed() && NULL != chunkActor);
		if (!chunk.isDestroyed() && NULL != chunkActor)
		{
			// BRG - must decouple this now since we have "dormant" kinematic chunks
			// which are chunks that have been freed but turned kinematic for "hard" sleeping
//			PX_ASSERT(chunkDynamic != chunkActor->readBodyFlag(NX_BF_KINEMATIC));

			SCOPED_PHYSX_LOCK_WRITE(*scene.mApexScene);
			SCOPED_PHYSX_LOCK_READ(*scene.mApexScene);

			destructible.setChunkVisibility(chunkIndex, true);

			// Only set shape local poses on newly created actors (appendShapes properly positions otherwise)
			if (chunkActorCreated)
			{
				for (physx::PxU32 i = 0; i < chunk.getShapeCount(); ++i)
				{
					chunk.getShape(i)->setLocalPose(NxFromPxMat34(destructible.getInitialChunkLocalPose(chunkIndex)));
				}
			}

			// Initialize velocities for newly created dynamic actors
			if (chunkActorCreated && chunkDynamic)
			{
				chunkActor->setLinearVelocity( NXFROMPXVEC3(destructible.getInitialChunkLinearVelocity( chunkIndex)));
				chunkActor->setAngularVelocity(NXFROMPXVEC3(destructible.getInitialChunkAngularVelocity(chunkIndex)));
			}

			// Don't recurse visible chunks
			chunkRecurse = false;
			if (parentIndex != DestructibleAsset::InvalidChunkIndex)
			{
				structure->chunks[parentIndex].flags |= ChunkMissingChild;
			}
		}
		structure->supportInvalid = true;
	}

	return chunkRecurse;
}

}
}
} // end namespace physx::apex

#endif
