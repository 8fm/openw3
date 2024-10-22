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

#ifndef NI_APEX_SCENE_H
#define NI_APEX_SCENE_H

#define APEX_CHECK_STAT_TIMER(name)// { PX_PROFILER_PERF_SCOPE(name); }

#include "NxApexScene.h"
#include "PsShare.h"

#if NX_SDK_VERSION_MAJOR == 2
class NxUserNotify;
class NxUserContactReport;
class NxUserContactModify;
#elif NX_SDK_VERSION_MAJOR == 3


// PH prevent PxScene.h from including PxPhysX.h, it will include sooo many files that it will break the clothing embedded branch
#define PX_PHYSICS_NX_PHYSICS
#include "PxScene.h"
#undef PX_PHYSICS_NX_PHYSICS

namespace physx
{
class PxSimulationEventCallback;
class PxContactModifyCallback;
}
#endif

namespace physx
{
namespace apex
{

class NiModuleScene;
class ApexContext;
class NiApexRenderDebug;
class NxApexPhysX3Interface;
class ApexCudaTestManager;

/**
 * Framework interface to ApexScenes for use by modules
 */
class NiApexScene : public NxApexScene
{
public:
	/**
	When a module has been released by the end-user, the module must release
	its NiModuleScenes and notify those ApexScenes that their module
	scenes no longer exist
	*/
	virtual void moduleReleased(NiModuleScene& moduleScene) = 0;

#if NX_SDK_VERSION_MAJOR == 2
	/**
	 * Manages module specific user callbacks.
	 */
	virtual void addModuleUserNotifier(NxUserNotify& notify) = 0;
	virtual void removeModuleUserNotifier(NxUserNotify& notify) = 0;
	virtual void addModuleUserContactReport(NxUserContactReport& report) = 0;
	virtual void removeModuleUserContactReport(NxUserContactReport& report) = 0;
	virtual void addModuleUserContactModify(NxUserContactModify& contactModify) = 0;
	virtual void removeModuleUserContactModify(NxUserContactModify& contactModify) = 0;
#elif NX_SDK_VERSION_MAJOR == 3
	virtual void addModuleUserNotifier(physx::PxSimulationEventCallback& notify) = 0;
	virtual void removeModuleUserNotifier(physx::PxSimulationEventCallback& notify) = 0;
	virtual void addModuleUserContactModify(physx::PxContactModifyCallback& notify) = 0;
	virtual void removeModuleUserContactModify(physx::PxContactModifyCallback& notify) = 0;
	virtual NxApexPhysX3Interface* getApexPhysX3Interface()	const = 0;
#endif

	virtual ApexContext* getApexContext() = 0;
	virtual physx::PxF32 getElapsedTime() const = 0;

	/* Get total elapsed simulation time, in integer milliseconds */
	virtual physx::PxU32 getTotalElapsedMS() const = 0;

	virtual bool isSimulating() const = 0;
	virtual bool physXElapsedTime(physx::PxF32& dt) const = 0;

	virtual physx::PxF32 getPhysXSimulateTime() const = 0;

	virtual physx::PxVec3 getGravity() const = 0;

#if NX_SDK_VERSION_MAJOR == 3
	virtual void lockRead(const char *fileName,PxU32 lineo) = 0;
	virtual void lockWrite(const char *fileName,PxU32 lineno) = 0;
	virtual void unlockRead(void) = 0;
	virtual void unlockWrite(void) = 0;
#else
	virtual void acquirePhysXLock() = 0;
	virtual void releasePhysXLock() = 0;
#endif
	virtual bool isFinalStep() const = 0;

	virtual physx::PxU32 getSeed() = 0; // Not necessarily const

#if APEX_USE_GRB
	virtual bool isGrbPhysicsEnabled() const = 0;
#endif

	enum ApexStatsDataEnum
	{
		NumberOfActors,
		NumberOfShapes,
		NumberOfAwakeShapes,
		//NumberOfGPUSimulatedShapes,
		NumberOfGpuShapePairs,
		NumberOfCpuShapePairs,
		ApexBeforeTickTime,
		ApexDuringTickTime,
		ApexPostTickTime,
		PhysXSimulationTime,
		ClothingSimulationTime,
		GpuRbSimulationTime,
		PhysXFetchResultTime,
		UserDelayedFetchTime,
		RbThroughput,
		GpuSceneMemoryUsed,
		GpuTempMemoryUsed,
		GpuRbStaticConvexMaxEdgeCount,
		GpuRbDynamicConvexMaxEdgeCount,
		GpuRbKinematicConvexMaxEdgeCount,
		GpuParticleTime,
		GpuTurbulenceTime,
		NumberOfGpuDynamicActorsAwake,
		NumberOfGpuDynamicActorsSleeping,
		SimulatedSpriteParticlesCount,
		SimulatedMeshParticlesCount,
		VisibleDestructibleChunkCount,
		DynamicDestructibleChunkIslandCount,

		// insert new items before this line
		NumberOfApexStats	// The number of stats
	};

	virtual void setApexStatValue(PxI32 index, ApexStatValue dataVal) = 0;

#if defined(APEX_CUDA_SUPPORT)
	virtual ApexCudaTestManager& getApexCudaTestManager() = 0;
	virtual bool isUsingCuda() const = 0;
#endif
	virtual NiModuleScene* getNiModuleScene(const char* moduleName) = 0;
};

/* ApexScene task names */
#define APEX_DURING_TICK_TIMING_FIX 1

#define AST_LOD_COMPUTE_BENEFIT			"ApexScene::LODComputeBenefit"
#define AST_PHYSX_SIMULATE				"ApexScene::PhysXSimulate"
#define AST_PHYSX_BETWEEN_STEPS			"ApexScene::PhysXBetweenSteps"

#if APEX_DURING_TICK_TIMING_FIX
#	define AST_DURING_TICK_COMPLETE		"ApexScene::DuringTickComplete"
#endif

#define AST_PHYSX_CHECK_RESULTS			"ApexScene::CheckResults"
#define AST_PHYSX_FETCH_RESULTS			"ApexScene::FetchResults"




}
} // end namespace physx::apex


#endif // NI_APEX_SCENE_H
