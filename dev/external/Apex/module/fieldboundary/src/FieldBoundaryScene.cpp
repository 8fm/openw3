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
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2

#include "NxApex.h"
#include "ApexRenderable.h"
#include "FieldBoundaryActor.h"
#include "FieldBoundaryScene.h"
#include "NiApexRenderDebug.h"
#include "NiApexScene.h"
#include "ModulePerfScope.h"

// these two lines are to circumvent Nx header errors
class NxForceField;
#include <NxForceFieldShape.h>

#include <NxBoxForceFieldShape.h>
#include <NxCapsuleForceFieldShape.h>
#include <NxConvexForceFieldShape.h>
#include <NxConvexForceFieldShapeDesc.h>
#include <NxConvexMeshDesc.h>
#include <NxForceFieldShape.h>
#include <NxForceFieldShapeGroup.h>
#include <NxSphereForceFieldShape.h>
#include "NiModuleFieldSampler.h"
#include "FieldBoundaryDrawer.h"
#include "PsShare.h"

#include "NxFromPx.h"
#include "NxSegment.h"

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

namespace physx
{
namespace apex
{
namespace fieldboundary
{

FieldBoundaryScene::FieldBoundaryScene(ModuleFieldBoundary& module, NiApexScene& scene, NiApexRenderDebug* renderDebug, NxResourceList& list)
	: mModule(&module)
	, mApexScene(&scene)
	, mRenderDebug(renderDebug)
	, mFieldSamplerManager(0)
	, mUpdateTask(*this)
{
	list.add(*this);		// Add self to module's list of FieldBoundaryScenes

	/* Initialize reference to FieldBoundaryDebugRenderParams */
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
			memberHandle.initParamRef(FieldBoundaryDebugRenderParams::staticClassName(), true);
		}
	}

	/* Load reference to FieldBoundaryDebugRenderParams */
	NxParameterized::Interface* refPtr = NULL;
	memberHandle.getParamRef(refPtr);
	mFieldBoundaryDebugRenderParams = DYNAMIC_CAST(FieldBoundaryDebugRenderParams*)(refPtr);
	PX_ASSERT(mFieldBoundaryDebugRenderParams);
}

FieldBoundaryScene::~FieldBoundaryScene()
{
}

// Called by scene task graph between LOD and PhysX::simulate()
void FieldBoundaryScene::TaskUpdate::run()
{
	setProfileStat((PxU16) mOwner.mActorArray.size());
	physx::PxF32 dt = mOwner.mApexScene->getElapsedTime();
	mOwner.updateActors(dt);
}

// Called by updateTask between LOD and PhysX simulate.  Any writes
// to render data must be protected by acquiring the actor's render data lock
void FieldBoundaryScene::updateActors(physx::PxF32 dt)
{
	mApexScene->acquirePhysXLock();
	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		FieldBoundaryActor* actor = DYNAMIC_CAST(FieldBoundaryActor*)(mActorArray[ i ]);
		PX_UNUSED(actor);
		PX_UNUSED(dt);
		// Do something.  This example is too simple.  Nothing to do before PhysX runs.
	}
	mApexScene->releasePhysXLock();
}

// submit the task that updates the field boundary actors
// called from ApexScene::simulate()
void FieldBoundaryScene::submitTasks(PxF32 /*elapsedTime*/, PxF32 /*substepSize*/, PxU32 /*numSubSteps*/)
{
	physx::PxTaskManager* tm = mApexScene->getTaskManager();
	tm->submitUnnamedTask(mUpdateTask);
	mUpdateTask.startAfter(tm->getNamedTask(AST_LOD_COMPUTE_BENEFIT));
	mUpdateTask.finishBefore(tm->getNamedTask(AST_PHYSX_SIMULATE));
}

// Called by ApexScene::fetchResults() with all actors render data locked.
void FieldBoundaryScene::fetchResults()
{
	PX_PROFILER_PERF_SCOPE("FieldBoundarySceneFetchResults");

	for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
	{
		FieldBoundaryActor* actor = DYNAMIC_CAST(FieldBoundaryActor*)(mActorArray[ i ]);
		actor->updatePoseAndBounds();
	}
}

void FieldBoundaryScene::visualize(void)
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mFieldBoundaryDebugRenderParams->VISUALIZE_FIELD_BOUNDARY_ACTOR)
	{
		return;
	}

	if (mFieldBoundaryDebugRenderParams->VISUALIZE_FIELD_BOUNDARIES)
	{
		PX_ASSERT(mRenderDebug != NULL);
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			FieldBoundaryActor* actor = DYNAMIC_CAST(FieldBoundaryActor*)(mActorArray[ i ]);
			mRenderDebug->setCurrentUserPointer((void*)(NxApexActor*)(mActorArray[i]));

			physx::PxU32 color;
			if (actor->mType)
			{
				color = mRenderDebug->getDebugColor(physx::DebugColors::Red); // Exclude Boundary
			}
			else
			{
				color = mRenderDebug->getDebugColor(physx::DebugColors::Green); // Include Boundary
			}

			physx::PxU32 nbShapes = actor->mShapeGroup->getNbShapes();
			actor->mShapeGroup->resetShapesIterator();
			for (physx::PxU32 j = 0; j < nbShapes; j++)
			{
				NxForceFieldShape* fs = actor->mShapeGroup->getNextShape();
				physx::PxMat34Legacy globalPose;
				PxFromNxMat34(globalPose, fs->getPose());

				// save the render state.
				mRenderDebug->pushRenderState();
				{
					mRenderDebug->setCurrentColor(color);

					switch (fs->getType())
					{
					case NX_SHAPE_SPHERE:
					{
						physx::PxF32 radius = fs->isSphere()->getRadius();
						FieldBoundaryDrawer::drawFieldBoundarySphere(*mRenderDebug, globalPose.t, radius, 16);
					}
					break;
					case NX_SHAPE_BOX:
					{
						physx::PxVec3 dims = PXFROMNXVEC3(fs->isBox()->getDimensions());
						physx::PxVec3 bmin = globalPose.t - dims;
						physx::PxVec3 bmax = globalPose.t + dims;
						FieldBoundaryDrawer::drawFieldBoundaryBox(*mRenderDebug, bmin, bmax);
					}
					break;
					case NX_SHAPE_CAPSULE:
					{
						NxSegment worldSegment;

						physx::PxF32 radius = fs->isCapsule()->getRadius();
						physx::PxF32 height = fs->isCapsule()->getHeight();
						FieldBoundaryDrawer::drawFieldBoundaryCapsule(*mRenderDebug, radius, height, 2, PxMat44(globalPose.M, globalPose.t));
					}
					break;
					case NX_SHAPE_CONVEX:
					{
						NxConvexForceFieldShapeDesc convexDesc;
						fs->isConvex()->saveToDesc(convexDesc);
						NxConvexMesh* mesh = convexDesc.meshData;
						NxConvexMeshDesc meshDesc;
						mesh->saveToDesc(meshDesc);

						FieldBoundaryDrawer::drawFieldBoundaryConvex(*mRenderDebug, globalPose.t, meshDesc.numVertices, meshDesc.numTriangles, meshDesc.pointStrideBytes, meshDesc.triangleStrideBytes, meshDesc.points, meshDesc.triangles);
#if 0
						const void* triangleBase	= meshDesc.triangles;
						physx::PxU32 triangleNum			= meshDesc.numTriangles;
						physx::PxU32 triangleStride		= meshDesc.triangleStrideBytes;
						const void* pointsBase		= meshDesc.points;
#if _DEBUG
						physx::PxU32 pointsNum				= meshDesc.numVertices;
						physx::PxU32 pointsStride			= meshDesc.pointStrideBytes;
#endif
						PX_ASSERT(triangleNum != 0u);
						PX_ASSERT((triangleStride == (3 * sizeof(physx::PxU32))) ||
						          (triangleStride == (3 * sizeof(physx::PxU16))));
						PX_ASSERT(pointsNum != 0);
						PX_ASSERT(pointsStride == sizeof(physx::PxVec3));
						for (physx::PxU32 k = 0; k < triangleNum; k++)
						{
							physx::PxU32 triIndices[3];
							physx::PxVec3 triPoints[3];
							if (triangleStride == (3 * sizeof(physx::PxU32)))
							{
								triIndices[0] = *((physx::PxU32*)triangleBase + 3 * k + 0);
								triIndices[1] = *((physx::PxU32*)triangleBase + 3 * k + 1);
								triIndices[2] = *((physx::PxU32*)triangleBase + 3 * k + 2);
							}
							else
							{
								triIndices[0] = *((physx::PxU16*)triangleBase + 3 * k + 0);
								triIndices[1] = *((physx::PxU16*)triangleBase + 3 * k + 1);
								triIndices[2] = *((physx::PxU16*)triangleBase + 3 * k + 2);
							}
							PX_ASSERT(triIndices[0] < pointsNum);
							PX_ASSERT(triIndices[1] < pointsNum);
							PX_ASSERT(triIndices[2] < pointsNum);
							triPoints[0] = *((physx::PxVec3*)pointsBase + triIndices[0]);
							triPoints[1] = *((physx::PxVec3*)pointsBase + triIndices[1]);
							triPoints[2] = *((physx::PxVec3*)pointsBase + triIndices[2]);
							triPoints[0] += globalPose.t;
							triPoints[1] += globalPose.t;
							triPoints[2] += globalPose.t;
							mRenderDebug->debugPolygon(3, triPoints);
						}
#endif
					}
					break;
					default:
						break;
					}
				}
				mRenderDebug->popRenderState();

				//visualize actor names
				if (mFieldBoundaryDebugRenderParams->VISUALIZE_FIELD_BOUNDARY_ACTOR_NAME &&
				        mFieldBoundaryDebugRenderParams->THRESHOLD_DISTANCE_FIELD_BOUNDARY_ACTOR_NAME >
				        (-mApexScene->getEyePosition(0) + globalPose.t).magnitude())
				{
					mRenderDebug->pushRenderState();
					mRenderDebug->setCurrentTextScale(2.0f);

					PxMat44 cameraFacingPose((mApexScene->getViewMatrix(0)).inverseRT());
					physx::PxVec3 textLocation = globalPose.t + cameraFacingPose.column1.getXYZ();
					cameraFacingPose.setPosition(textLocation);
					mRenderDebug->debugOrientedText(cameraFacingPose, " %s %s", actor->getOwner()->getObjTypeName(), actor->getOwner()->getName());
					mRenderDebug->popRenderState();
				}
			}
			mRenderDebug->setCurrentUserPointer(NULL);
		}
	}
#endif
}

void FieldBoundaryScene::destroy()
{
	removeAllActors();
	mApexScene->moduleReleased(*this);
	delete this;
}

void FieldBoundaryScene::setModulePhysXScene(NxScene* nxScene)
{
	if (nxScene)
	{
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			FieldBoundaryActor* actor = DYNAMIC_CAST(FieldBoundaryActor*)(mActorArray[ i ]);
			actor->setPhysXScene(nxScene);
		}
	}
	else
	{
		for (physx::PxU32 i = 0 ; i < mActorArray.size() ; i++)
		{
			FieldBoundaryActor* actor = DYNAMIC_CAST(FieldBoundaryActor*)(mActorArray[ i ]);
			actor->setPhysXScene(NULL);
		}
	}

	mPhysXScene = nxScene;
}

NiFieldSamplerManager* FieldBoundaryScene::getNiFieldSamplerManager()
{
	if (mFieldSamplerManager == NULL)
	{
		NiModuleFieldSampler* moduleFieldSampler = mModule->getNiModuleFieldSampler();
		if (moduleFieldSampler != NULL)
		{
			mFieldSamplerManager = moduleFieldSampler->getNiFieldSamplerManager(*mApexScene);
			PX_ASSERT(mFieldSamplerManager != NULL);
		}
	}
	return mFieldSamplerManager;
}

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
