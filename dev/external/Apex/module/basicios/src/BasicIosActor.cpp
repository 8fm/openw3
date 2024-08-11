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
#include "NiApexScene.h"
#include "NiApexSDK.h"

#include "NxBasicIosActor.h"
#include "BasicIosActor.h"
#include "BasicIosAsset.h"
#include "NxIofxAsset.h"
#include "NxIofxActor.h"
#include "ModuleBasicIos.h"
#include "BasicIosScene.h"
#include "NiApexRenderDebug.h"
#include "NiApexAuthorableObject.h"
#include "NiModuleIofx.h"
#include "NiFieldSamplerManager.h"
#include "NiFieldSamplerQuery.h"
#include "ApexMirroredArray.h"
#include "ApexResourceHelper.h"
#include "NxApexReadWriteLock.h"
#include "PxAsciiConversion.h"

#include "PxTask.h"

#if NX_SDK_VERSION_MAJOR == 2
#include <NxScene.h>

#include <NxSphereShape.h>
#include <NxCapsuleShape.h>
#include <NxBoxShape.h>
#include <NxPlaneShape.h>
#include <NxConvexShape.h>

#include <NxConvexMesh.h>
#include <NxConvexMeshDesc.h>

#include <NxTriangleMeshShape.h>
#include <NxTriangleMesh.h>
#include <NxTriangleMeshDesc.h>

#include <NxMaterial.h>

#elif NX_SDK_VERSION_MAJOR == 3
#include <PxScene.h>

#include <PxShape.h>
#include <geometry/PxBoxGeometry.h>
#include <geometry/PxSphereGeometry.h>
#include <geometry/PxCapsuleGeometry.h>
#include <geometry/PxPlaneGeometry.h>
#include <geometry/PxTriangleMeshGeometry.h>
#include <geometry/PxTriangleMesh.h>

#include <PxMaterial.h>

#include <PxRigidActor.h>
#include <PxRigidBody.h>
#include <extensions/PxShapeExt.h>

#include <PxAsciiConversion.h>

#endif

#ifdef APEX_TEST
#include "BasicIosActorTestData.h"
#endif

namespace physx
{
	namespace apex
	{
		namespace basicios
		{

			class BasicIosInjectTask : public physx::PxTask, public physx::UserAllocated
			{
			public:
				BasicIosInjectTask(BasicIosActor* actor) : mActor(actor) {}

				const char* getName() const
				{
					return "BasicIosActor::InjectTask";
				}
				void run()
				{
					mActor->injectNewParticles();
				}

			protected:
				BasicIosActor* mActor;
			};

			void BasicIosActor::initStorageGroups(InplaceStorage& storage)
			{
				mSimulationStorageGroup.init(storage);
			}

			BasicIosActor::BasicIosActor(
				NxResourceList& list,
				BasicIosAsset& asset,
				BasicIosScene& scene,
				physx::apex::NxIofxAsset& iofxAsset,
				bool isDataOnDevice)
				:
#ifdef APEX_TEST
			mTestData(NULL),
#endif
				mAsset(&asset)
				, mBasicIosScene(&scene)
				, mIofxMgr(NULL)
				, mTotalElapsedTime(0.0f)
				, mParticleCount(0)
				, mParticleBudget(0)
				, mInjectedCount(0)
				, mLastActiveCount(0)
				, mLastBenefitSum(0)
				, mLastBenefitMin(+FLT_MAX)
				, mLastBenefitMax(-FLT_MAX)
				, mLifeSpan(scene.getApexScene(), NV_ALLOC_INFO("mLifeSpan", PARTICLES))
				, mLifeTime(scene.getApexScene(), NV_ALLOC_INFO("mLifeTime", PARTICLES))
				, mInjector(scene.getApexScene(), NV_ALLOC_INFO("mInjector", PARTICLES))
				, mBenefit(scene.getApexScene(), NV_ALLOC_INFO("mBenefit", PARTICLES))
				, mConvexPlanes(scene.getApexScene(), NV_ALLOC_INFO("mConvexPlanes", PARTICLES))
				, mConvexVerts(scene.getApexScene(), NV_ALLOC_INFO("mConvexVerts", PARTICLES))
				, mConvexPolygonsData(scene.getApexScene(), NV_ALLOC_INFO("mConvexPolygonsData", PARTICLES))
				, mTrimeshVerts(scene.getApexScene(), NV_ALLOC_INFO("mTrimeshVerts", PARTICLES))
				, mTrimeshIndices(scene.getApexScene(), NV_ALLOC_INFO("mTrimeshIndices", PARTICLES))
				, mInjectorsCounters(scene.getApexScene(), NV_ALLOC_INFO("mInjectorsCounters", PARTICLES))
				, mGridDensityGrid(scene.getApexScene(), NV_ALLOC_INFO("mGridDensityGrid", PARTICLES))
				, mGridDensityGridLowPass(scene.getApexScene(), NV_ALLOC_INFO("mGridDensityGridLowPass", PARTICLES))
				, mFieldSamplerQuery(NULL)
				, mField(scene.getApexScene(), NV_ALLOC_INFO("mField", PARTICLES))
				, mDensityOrigin(0.f,0.f,0.f)
			{
				list.add(*this);

				mMaxParticleCount = mAsset->mParams->maxParticleCount;
				physx::PxF32 maxInjectCount = mAsset->mParams->maxInjectedParticleCount;
				mMaxTotalParticleCount = mMaxParticleCount + physx::PxU32(maxInjectCount <= 1.0f ? mMaxParticleCount * maxInjectCount : maxInjectCount);


				NiIofxManagerDesc desc;
				desc.iosAssetName         = mAsset->getName();
				desc.iosSupportsDensity   = mAsset->getSupportsDensity();
				desc.iosSupportsCollision = true;
				desc.iosOutputsOnDevice   = isDataOnDevice;
				desc.maxObjectCount       = mMaxParticleCount;
				desc.maxInputCount        = mMaxTotalParticleCount;
				desc.maxInStateCount      = mMaxTotalParticleCount;

				NiModuleIofx* moduleIofx = mAsset->mModule->getNiModuleIofx();
				if (moduleIofx)
				{
					mIofxMgr = moduleIofx->createActorManager(*mBasicIosScene->mApexScene, iofxAsset, desc);
					mIofxMgr->createSimulationBuffers(mBufDesc);
				}

#if NX_SDK_VERSION_MAJOR == 2
				mCollisionGroup = ApexResourceHelper::resolveCollisionGroup(mAsset->mParams->collisionFilterDataName);
				mCollisionGroupsMask = ApexResourceHelper::resolveCollisionGroup128(mAsset->mParams->collisionFilterDataName);
#elif NX_SDK_VERSION_MAJOR == 3
				mCollisionFilterData = ApexResourceHelper::resolveCollisionGroup128(mAsset->mParams->collisionFilterDataName);
#endif

				NiFieldSamplerManager* fieldSamplerManager = mBasicIosScene->getNiFieldSamplerManager();
				if (fieldSamplerManager)
				{
					NiFieldSamplerQueryDesc queryDesc;
					queryDesc.maxCount = mMaxParticleCount;
					queryDesc.samplerFilterData = ApexResourceHelper::resolveCollisionGroup64(mAsset->mParams->fieldSamplerFilterDataName);

					mFieldSamplerQuery = fieldSamplerManager->createFieldSamplerQuery(queryDesc);

					if (isDataOnDevice)
					{
#if defined(APEX_CUDA_SUPPORT)
						mField.reserve(mMaxParticleCount, ApexMirroredPlace::GPU);
#endif
					}
					else
					{
						mField.reserve(mMaxParticleCount, ApexMirroredPlace::CPU);
					}
				}

				mInjectTask = PX_NEW(BasicIosInjectTask)(this);

				// Pull Grid Density Parameters
				{
					if(mBufDesc.pmaDensity)
					{
						BasicIOSAssetParam* gridParams = (BasicIOSAssetParam*)(mAsset->getAssetNxParameterized());
						mGridDensityParams.Enabled = gridParams->GridDensity.Enabled;
						mGridDensityParams.GridSize = gridParams->GridDensity.GridSize;
						mGridDensityParams.GridMaxCellCount = gridParams->GridDensity.MaxCellCount;
						mGridDensityParams.GridResolution = general_string_parsing2::PxAsc::strToU32(&gridParams->GridDensity.Resolution[4],NULL);
						mGridDensityParams.DensityOrigin = mDensityOrigin;
					}		
					else
					{
						mGridDensityParams.Enabled = false;
						mGridDensityParams.GridSize = 1.f;
						mGridDensityParams.GridMaxCellCount = 1u;
						mGridDensityParams.GridResolution = 8;
						mGridDensityParams.DensityOrigin = mDensityOrigin;
					}
				}

				addSelfToContext(*scene.mApexScene->getApexContext());		// add self to NxApexScene
				addSelfToContext(*DYNAMIC_CAST(ApexContext*)(&scene));		// add self to BasicIosScene
			}

			BasicIosActor::~BasicIosActor()
			{
				PX_DELETE(mInjectTask);
			}

			void BasicIosActor::release()
			{
				if (mInRelease)
				{
					return;
				}
				mInRelease = true;
				mAsset->releaseIosActor(*this);
			}

			void BasicIosActor::destroy()
			{
				ApexActor::destroy();

				setPhysXScene(NULL);

				// remove ourself from our asset's resource list, in case releasing our emitters
				// causes our asset's resource count to reach zero and for it to be released.
				ApexResource::removeSelf();

				// Release all injectors, releasing all emitters and their IOFX asset references
				while (mInjectorList.getSize())
				{
					BasicParticleInjector* inj = DYNAMIC_CAST(BasicParticleInjector*)(mInjectorList.getResource(mInjectorList.getSize() - 1));
					inj->release();
				}

				if (mIofxMgr)
				{
					mIofxMgr->release();
				}
				if (mFieldSamplerQuery)
				{
					mFieldSamplerQuery->release();
				}

				delete this;
			}

#if NX_SDK_VERSION_MAJOR == 2
			void BasicIosActor::setPhysXScene(NxScene* scene)
			{
				if (scene)
				{
					putInScene(scene);
				}
				else
				{
					removeFromScene();
				}
			}
			NxScene* BasicIosActor::getPhysXScene() const
			{
				return NULL;
			}
			void BasicIosActor::putInScene(NxScene* scene)
			{
				NxVec3 up;
				scene->getGravity(up);

				physx::PxVec3 gravity = PXFROMNXVEC3(up);
				setGravity(gravity);
			}
#elif NX_SDK_VERSION_MAJOR == 3
			void BasicIosActor::setPhysXScene(PxScene* scene)
			{
				if (scene)
				{
					putInScene(scene);
				}
				else
				{
					removeFromScene();
				}
			}
			PxScene* BasicIosActor::getPhysXScene() const
			{
				return NULL;
			}
			void BasicIosActor::putInScene(PxScene* scene)
			{
				SCOPED_PHYSX3_LOCK_READ(scene);
				physx::PxVec3 gravity = scene->getGravity();
				setGravity(gravity);
			}
#endif


			void BasicIosActor::getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly)
			{
				PX_UNUSED(min);
				PX_UNUSED(max);
				PX_UNUSED(intOnly);
				APEX_INVALID_OPERATION("not implemented");
			}


			physx::PxF32 BasicIosActor::getActivePhysicalLod()
			{
				APEX_INVALID_OPERATION("NxBasicIosActor does not support this operation");
				return -1.0f;
			}


			void BasicIosActor::forcePhysicalLod(physx::PxF32 lod)
			{
				PX_UNUSED(lod);
				APEX_INVALID_OPERATION("not implemented");
			}


			void BasicIosActor::removeFromScene()
			{
				mParticleCount = 0;
			}


			const physx::PxVec3* BasicIosActor::getRecentPositions(physx::PxU32& count, physx::PxU32& stride) const
			{
				APEX_INVALID_OPERATION("not implemented");

				count = 0;
				stride = 0;
				return NULL;
			}

			physx::PxVec3 BasicIosActor::getGravity() const
			{
				return mGravityVec;
			}

			void BasicIosActor::setGravity(physx::PxVec3& gravity)
			{
				mGravityVec = gravity;
				mUp = mGravityVec;

				// apply asset's scene gravity scale and external acceleration
				mUp *= mAsset->getSceneGravityScale();
				mUp += mAsset->getExternalAcceleration();

				mGravity = mUp.magnitude();
				if (!physx::PxIsFinite(mGravity))
				{
					// and they could set both to 0,0,0
					mUp = physx::PxVec3(0.0f, -1.0f, 0.0f);
					mGravity = 1.0f;
				}
				mUp *= -1.0f;

				mIofxMgr->setSimulationParameters(getObjectRadius(), mUp, mGravity, getObjectDensity());
			}

			NiIosInjector* BasicIosActor::allocateInjector(const char* iofxAssetName)
			{
				NiResourceProvider* nrp = mAsset->mModule->mSdk->getInternalResourceProvider();

				NiApexAuthorableObject* AO = mAsset->mModule->mSdk->getAuthorableObject(NX_IOFX_AUTHORING_TYPE_NAME);
				if (!AO)
				{
					APEX_INTERNAL_ERROR("Unknown authorable type: %s, please load the IOFX module.", NX_IOFX_AUTHORING_TYPE_NAME);
					return NULL;
				}
				NxResID iofxnsid = AO->getResID();

				if (iofxnsid == INVALID_RESOURCE_ID)
				{
					return NULL;
				}

				BasicParticleInjector* inj = 0;
				//createInjector
				{
					physx::PxU32 injectorID = mBasicIosScene->getInjectorAllocator().allocateInjectorID();
					if (injectorID != BasicIosInjectorAllocator::NULL_INJECTOR_INDEX)
					{
						inj = PX_NEW(BasicParticleInjector)(mInjectorList, *this, injectorID);
					}
				}
				if (inj == 0)
				{
					APEX_INTERNAL_ERROR("Failed to create new BasicIos injector.");
					return NULL;
				}

				NxResID iofxAssetResID = nrp->createResource(iofxnsid, iofxAssetName);
				if (iofxAssetResID == INVALID_RESOURCE_ID)
				{
					inj->release();
					return NULL;
				}

				physx::apex::NxIofxAsset* iofxAsset = DYNAMIC_CAST(physx::apex::NxIofxAsset*)(nrp->getResource(iofxAssetResID));
				if (iofxAsset == 0)
				{
					inj->release();
					return NULL;
				}

				inj->init(iofxAssetResID, iofxAsset);

				/* Add reference to IOFX asset table */
				for (PxU32 a = 0; a < mIofxAssets.size() ; a++)
				{
					if (mIofxAssets[a] == iofxAsset)
					{
						mIofxAssetRefs[a]++;
						return inj;
					}
				}

				/** New IOFX Asset */
				mIofxAssets.pushBack(iofxAsset);
				mIofxAssetRefs.pushBack(1);
				return inj;
			}

			void BasicIosActor::releaseInjector(NiIosInjector& injector)
			{
				BasicParticleInjector* inj = DYNAMIC_CAST(BasicParticleInjector*)(&injector);

				for (PxU32 i = 0 ; i < mIofxAssets.size() ; i++)
				{
					if (mIofxAssets[i] != inj->mIofxAsset)
					{
						continue;
					}

					if (--mIofxAssetRefs[ i ] == 0)
					{
						mIofxMgr->releaseAssetID(inj->mIofxAsset);
						mIofxAssets.replaceWithLast(i);
						mIofxAssetRefs.replaceWithLast(i);
					}
				}

				//destroyInjector
				{
					//set mLODBias to FLT_MAX to mark released injector
					//all particles from released injectors will be removed in simulation
					InjectorParams& injParams = mBasicIosScene->getInjectorParams(inj->mInjectorID);
					injParams.mLODBias = FLT_MAX;

					mBasicIosScene->getInjectorAllocator().releaseInjectorID(inj->mInjectorID);
					inj->destroy();
				}

				if (mInjectorList.getSize() == 0)
				{
					//if we have no injectors - release self
					release();
				}
			}


#if NX_SDK_VERSION_MAJOR == 2
			template <typename T>
			PxU32 BasicIosActor::extractConvexPolygons(const NxConvexMeshDesc& convexMeshDesc)
			{
				mTempConvexPolygons.clear();

				const PxU8* pointData = static_cast<const PxU8*>(convexMeshDesc.points);
				const T* triangleData = static_cast<const T*>(convexMeshDesc.triangles);
				const physx::PxU32 numTriangles = convexMeshDesc.numTriangles;

				PxU32  refIndex0 = PX_MAX_U32; 
				PxVec3 refNormal(0.0f);

				for (PxU32 triIndex = 0; triIndex < numTriangles; triIndex++)
				{
					PxU32 index0 = triangleData[3 * triIndex + 0];
					PxU32 index1 = triangleData[3 * triIndex + 1];
					PxU32 index2 = triangleData[3 * triIndex + 2];

					const PxVec3& vert0 = *reinterpret_cast<const PxVec3*>(pointData + index0 * convexMeshDesc.pointStrideBytes);
					const PxVec3& vert1 = *reinterpret_cast<const PxVec3*>(pointData + index1 * convexMeshDesc.pointStrideBytes);
					const PxVec3& vert2 = *reinterpret_cast<const PxVec3*>(pointData + index2 * convexMeshDesc.pointStrideBytes);

					PxVec3 normal = (vert1 - vert0).cross(vert2 - vert0);
					normal.normalize();

					const float normalEps = 1e-5f;
					if (index0 != refIndex0 || normal.dot(refNormal) < 1 - normalEps)
					{
						if (!mTempConvexPolygons.empty())
						{
							mTempConvexPolygons.back().endTriIndex = triIndex;
						}

						refIndex0 = index0;
						refNormal = normal;

						ConvexPolygon polygon;
						polygon.plane.n = normal;
						polygon.plane.d = -normal.dot(vert0);
						polygon.begTriIndex = triIndex;
						polygon.endTriIndex = numTriangles;
						mTempConvexPolygons.pushBack( polygon );
					}
				}

				PxU32 polygonsDataSize = 0;

				PxU32 numPolygons = mTempConvexPolygons.size();
				for (PxU32 i = 0; i < numPolygons; i++)
				{
					const ConvexPolygon& polygon = mTempConvexPolygons[i];
					PxU32 polygonTriCount = (polygon.endTriIndex - polygon.begTriIndex);
					PxU32 polygonVertCount = (2 + polygonTriCount);

					polygonsDataSize += (1 + polygonVertCount);
				}

				return polygonsDataSize;
			}

			template <typename T>
			void BasicIosActor::fillConvexPolygonsData(const NxConvexMeshDesc& convexMeshDesc, PxU32 polygonsDataSize)
			{
				PxU32 numPolygons = mTempConvexPolygons.size();

				PxPlane* convexPlanes = mConvexPlanes.getPtr() + mConvexPlanes.getSize();
				PxVec4*  convexVerts = mConvexVerts.getPtr() + mConvexVerts.getSize();
				PxU32*   convexPolygonsData = mConvexPolygonsData.getPtr() + mConvexPolygonsData.getSize();

				mConvexPlanes.setSize(mConvexPlanes.getSize() + numPolygons);
				mConvexVerts.setSize(mConvexVerts.getSize() + convexMeshDesc.numVertices);
				mConvexPolygonsData.setSize(mConvexPolygonsData.getSize() + polygonsDataSize);

				//copy Convex Planes & Polygon Data
				for (PxU32 i = 0; i < numPolygons; i++)
				{
					const ConvexPolygon& polygon = mTempConvexPolygons[i];
					*convexPlanes++ = polygon.plane;

					const PxU32 polygonTriCount = (polygon.endTriIndex - polygon.begTriIndex);
					const PxU32 polygonVertCount = (2 + polygonTriCount);

					const T* triangleData = static_cast<const T*>(convexMeshDesc.triangles) + 3 * polygon.begTriIndex;

					*convexPolygonsData++ = polygonVertCount;
					*convexPolygonsData++ = *triangleData++; //first vertex index
					*convexPolygonsData++ = *triangleData++; //second vertex index
					for (PxU32 j = 0; j < polygonTriCount; j++)
					{
						*convexPolygonsData++ = *triangleData; //third vertex index
						triangleData += 3;
					}
				}
				//copy Convex Vertices
				const PxU8* pointData = static_cast<const PxU8*>(convexMeshDesc.points);
				for(PxU32 i = 0; i < convexMeshDesc.numVertices; i++)
				{
					const PxF32* point = reinterpret_cast<const PxF32*>(pointData);
					pointData += convexMeshDesc.pointStrideBytes;

					*convexVerts++ = PxVec4(point[0], point[1], point[2], 0);
				}
			}
#endif

			void BasicIosActor::visualize()
			{
#ifndef WITHOUT_DEBUG_VISUALIZE
				if(mBasicIosScene->mBasicIosDebugRenderParams->VISUALIZE_BASIC_IOS_GRID_DENSITY)
				{
					physx::RenderDebug* renderer = mBasicIosScene->mDebugRender;
					if(mGridDensityParams.Enabled)
					{					
						renderer->setCurrentColor(0x0000ff);
						PxF32 factor = PxMin((PxReal)(mGridDensityParams.GridResolution-4) / (mGridDensityParams.GridResolution),0.75f);
						PxU32 onScreenRes = (PxU32)(factor*mGridDensityParams.GridResolution);
						for (PxU32 i = 0 ; i <= onScreenRes; i++)
						{     
							PxF32 u = 2.f*((PxF32)i/(onScreenRes))-1.f;
							PxVec4 a = mDensityDebugMatInv.transform(PxVec4(u,-1.f,0.1f,1.f));
							PxVec4 b = mDensityDebugMatInv.transform(PxVec4(u, 1.f,0.1f,1.f));
							PxVec4 c = mDensityDebugMatInv.transform(PxVec4(-1.f,u,0.1f,1.f));
							PxVec4 d = mDensityDebugMatInv.transform(PxVec4( 1.f,u,0.1f,1.f));
							renderer->debugLine(PxVec3(a.getXYZ()/a.w),PxVec3(b.getXYZ()/b.w));
							renderer->debugLine(PxVec3(c.getXYZ()/c.w),PxVec3(d.getXYZ()/d.w));
						}
					}
				}
				if(mBasicIosScene->mBasicIosDebugRenderParams->VISUALIZE_BASIC_IOS_COLLIDE_SHAPES)
				{
					INPLACE_STORAGE_GROUP_SCOPE(mSimulationStorageGroup);
					SimulationParams* simParams = mSimulationParamsHandle.resolve(_storage_);

					physx::RenderDebug* renderer = mBasicIosScene->mDebugRender;
					renderer->setCurrentColor(mBasicIosScene->mDebugRender->getDebugColor(physx::DebugColors::Blue));

					for(PxU32 i = 0; i < (PxU32)simParams->boxes.getSize(); ++i)
					{
						physx::PxMat34Legacy pose = simParams->boxes.getElems(_storage_)[i].pose;  
						PxVec3 position = pose.t;
						PxVec3 halfSize = simParams->boxes.getElems(_storage_)[i].halfSize;
						renderer->debugOrientedBound(-halfSize, halfSize, position, pose.M.toQuat());
					}

					for(PxU32 i = 0; i < (PxU32)simParams->spheres.getSize(); ++i)
					{
						PxF32 r = simParams->spheres.getElems(_storage_)[i].radius;
						PxVec3 pos = simParams->spheres.getElems(_storage_)[i].pose.t;
						renderer->debugSphere(pos, r);
					}

					for(PxU32 i = 0; i < (PxU32)simParams->capsules.getSize(); ++i)
					{
						CollisionCapsuleData capsule = simParams->capsules.getElems(_storage_)[i];
						PxF32 r = capsule.radius, h = capsule.halfHeight;
						physx::PxMat34Legacy pose = capsule.pose;
						renderer->debugOrientedCapsule(r, 2 * h, 0, pose);
					}
				}
#endif
			}

			void BasicIosActor::submitTasks()
			{
				physx::PxTaskManager* tm = mBasicIosScene->getApexScene().getTaskManager();
				tm->submitUnnamedTask(*mInjectTask);

				//compile a list of actually colliding objects and process them to be used by the simulation
				INPLACE_STORAGE_GROUP_SCOPE(mSimulationStorageGroup);

				SimulationParams* simParams = mSimulationParamsHandle.allocOrResolve(_storage_);
				PX_ASSERT(simParams);
				if (simParams)
				{
					simParams->collisionThreshold = mAsset->mParams->collisionThreshold;
					simParams->collisionDistance = mAsset->mParams->particleRadius * mAsset->mParams->collisionDistanceMultiplier;

					int numBoxes = 0;
					int numSpheres = 0;
					int numCapsules = 0;
					int numHalfSpaces = 0;
					int numConvexMeshes = 0;
					int numTriMeshes = 0;

					mConvexPlanes.setSize(0);
					mConvexVerts.setSize(0);
					mConvexPolygonsData.setSize(0);

					mTrimeshVerts.setSize(0);
					mTrimeshIndices.setSize(0);

					simParams->convexPlanes = mConvexPlanes.getGpuPtr() ? mConvexPlanes.getGpuPtr() : mConvexPlanes.getPtr();
					simParams->convexVerts = mConvexVerts.getGpuPtr() ? mConvexVerts.getGpuPtr() : mConvexVerts.getPtr();
					simParams->convexPolygonsData = mConvexPolygonsData.getGpuPtr() ? mConvexPolygonsData.getGpuPtr() : mConvexPolygonsData.getPtr();

					simParams->trimeshVerts = mTrimeshVerts.getGpuPtr() ? mTrimeshVerts.getGpuPtr() : mTrimeshVerts.getPtr();
					simParams->trimeshIndices = mTrimeshIndices.getGpuPtr() ? mTrimeshIndices.getGpuPtr() : mTrimeshIndices.getPtr();

					if (!(mAsset->mParams->staticCollision || mAsset->mParams->dynamicCollision))
					{
						return;
					}

					PxBounds3 bounds = mIofxMgr->getBounds();
					if (bounds.isEmpty())
					{
						return;
					}

					const PxF32 collisionRadius = simParams->collisionThreshold + simParams->collisionDistance;
					PX_ASSERT(!bounds.isEmpty());
					bounds.fattenFast(collisionRadius);

					const PxU32 maxCollidingObjects = mAsset->mParams->maxCollidingObjects;

#if NX_SDK_VERSION_MAJOR == 2
					mOverlappedShapes.resize(maxCollidingObjects);
					NxScene* physxScene = mBasicIosScene->getModulePhysXScene();

					NxBounds3 nxBounds;
					NxFromPxBounds3(nxBounds, bounds);

					PxU32 groupsMaskBits = mCollisionGroupsMask.bits0 | mCollisionGroupsMask.bits1 | mCollisionGroupsMask.bits2 | mCollisionGroupsMask.bits3;
					PxU32 nxShapesType = 0;
					if (mAsset->mParams->staticCollision) nxShapesType |= NX_STATIC_SHAPES;
					if (mAsset->mParams->dynamicCollision) nxShapesType |= NX_DYNAMIC_SHAPES;

					PxU32 numCollidingObjects = physxScene->overlapAABBShapes(nxBounds, static_cast<NxShapesType>(nxShapesType), mOverlappedShapes.size(), &mOverlappedShapes[0], NULL, NxU32(-1),
						groupsMaskBits ? &mCollisionGroupsMask : NULL);
					// clamp to size of mOverlappedShapes
					if (numCollidingObjects > mOverlappedShapes.size())
					{
						numCollidingObjects = mOverlappedShapes.size();
					}

					for (PxU32 iShape = 0; iShape < numCollidingObjects; iShape++)
					{
						NxShape* shape = mOverlappedShapes[iShape];

						switch (shape->getType())
						{
						case NX_SHAPE_BOX:
							{
								++numBoxes;
							}
							break;
						case NX_SHAPE_SPHERE:
							{
								++numSpheres;
							}
							break;
						case NX_SHAPE_CAPSULE:
							{
								++numCapsules;
							}
							break;
						case NX_SHAPE_PLANE:
							{
								++numHalfSpaces;
							}
							break;
						case NX_SHAPE_CONVEX:
							{
								if (mAsset->mParams->collisionWithConvex)
								{
									++numConvexMeshes;
								}
							}
							break;
						case NX_SHAPE_MESH:
							{
								if (mAsset->mParams->collisionWithTriangleMesh)
								{
									++numTriMeshes;
								}
							}
							break;
						default:
							break;
						}
					}

					simParams->boxes.resize(_storage_ , numBoxes);
					simParams->spheres.resize(_storage_ , numSpheres);
					simParams->capsules.resize(_storage_ , numCapsules);
					simParams->halfSpaces.resize(_storage_ , numHalfSpaces);
					simParams->convexMeshes.resize(_storage_ , numConvexMeshes);
					simParams->trimeshes.resize(_storage_ , numTriMeshes);

					numBoxes = 0;
					numSpheres = 0;
					numCapsules = 0;
					numHalfSpaces = 0;
					numConvexMeshes = 0;
					numTriMeshes = 0;

					for (PxU32 iShape = 0; iShape < numCollidingObjects; iShape++)
					{
						NxShape* shape = mOverlappedShapes[iShape];

						//check legacy-32 PhysX collision
						if (mCollisionGroup < 32 &&
							physxScene->getGroupCollisionFlag(shape->getGroup(), mCollisionGroup) == false)
						{
							continue;
						}

						NxBounds3 shapeWorldBounds;
						shape->getWorldBounds(shapeWorldBounds);
						shapeWorldBounds.fatten(collisionRadius);

						NxMat34 shapeGlobalPose = shape->getGlobalPose();

						CollisionData* baseData = 0;
						if (const NxSphereShape* sphereShape = shape->isSphere())
						{
							CollisionSphereData& data = simParams->spheres.getElems(_storage_)[ numSpheres++ ];
							baseData = &data;

							PxFromNxBounds3(data.aabb, shapeWorldBounds);
							PxFromNxMat34(data.pose, shapeGlobalPose);
							data.inversePose = data.pose.getInverseRT();

							data.radius = sphereShape->getRadius();
							//extend
							data.radius += simParams->collisionDistance;
						}
						else if (const NxCapsuleShape* capsuleShape = shape->isCapsule())
						{
							CollisionCapsuleData& data = simParams->capsules.getElems(_storage_)[ numCapsules++ ];
							baseData = &data;

							PxFromNxBounds3(data.aabb, shapeWorldBounds);
							PxFromNxMat34(data.pose, shapeGlobalPose);
							data.inversePose = data.pose.getInverseRT();

							data.halfHeight = capsuleShape->getHeight() * 0.5f;
							data.radius = capsuleShape->getRadius();
							//extend
							data.radius += simParams->collisionDistance;
						}
						else if (const NxBoxShape* boxShape = shape->isBox())
						{
							CollisionBoxData& data = simParams->boxes.getElems(_storage_)[ numBoxes++ ];
							baseData = &data;

							PxFromNxBounds3(data.aabb, shapeWorldBounds);
							PxFromNxMat34(data.pose, shapeGlobalPose);
							data.inversePose = data.pose.getInverseRT();

							PxFromNxVec3(data.halfSize, boxShape->getDimensions());
							//extend
						}
						else if (const NxPlaneShape* planeShape = shape->isPlane())
						{
							CollisionHalfSpaceData& data = simParams->halfSpaces.getElems(_storage_)[ numHalfSpaces++ ];
							baseData = &data;

							NxPlane plane = planeShape->getPlane();
							plane.d = -plane.d; //fix PhysX2 inconsistency, see NxPlaneShape docs

							PxFromNxVec3(data.normal, plane.normal);
							data.normal.normalize();
							PxFromNxVec3(data.origin, plane.pointInPlane());
							//extend
							data.origin += data.normal * simParams->collisionDistance;
						}
						else if (NxConvexShape* convexShape = shape->isConvexMesh())
						{
							if (mAsset->mParams->collisionWithConvex)
							{
								CollisionConvexMeshData& data = simParams->convexMeshes.getElems(_storage_)[ numConvexMeshes++ ];
								baseData = &data;

								PxFromNxBounds3(data.aabb, shapeWorldBounds);
								PxFromNxMat34(data.pose, shapeGlobalPose);
								data.inversePose = data.pose.getInverseRT();

								//get ConvexMesh desc
								NxConvexMeshDesc convexMeshDesc;
								convexShape->getConvexMesh().saveToDesc(convexMeshDesc);

								PxU32 polygonsDataSize;
								if (convexMeshDesc.flags & NX_CF_16_BIT_INDICES)
								{
									polygonsDataSize = extractConvexPolygons<PxU16>(convexMeshDesc);
								}
								else
								{
									polygonsDataSize = extractConvexPolygons<PxU32>(convexMeshDesc);
								}
								PxU32 numPolygons = mTempConvexPolygons.size();
								PxU32 numVertices = convexMeshDesc.numVertices;

								if (mConvexPlanes.getSize() + numPolygons <= mConvexPlanes.getCapacity() &&
									mConvexVerts.getSize() + numVertices <= mConvexVerts.getCapacity() &&
									mConvexPolygonsData.getSize() + polygonsDataSize <= mConvexPolygonsData.getCapacity())
								{
									data.numPolygons = numPolygons;
									data.firstPlane = mConvexPlanes.getSize();
									data.firstVertex = mConvexVerts.getSize();
									data.polygonsDataOffset = mConvexPolygonsData.getSize();

									if (convexMeshDesc.flags & NX_CF_16_BIT_INDICES)
									{
										fillConvexPolygonsData<PxU16>(convexMeshDesc, polygonsDataSize);
									}
									else
									{
										fillConvexPolygonsData<PxU32>(convexMeshDesc, polygonsDataSize);
									}
								}
								else
								{
									APEX_DEBUG_WARNING("BasicIosActor: out of memory to store Convex data");

									data.numPolygons = 0;
									data.firstPlane = 0;
									data.firstVertex = 0;
									data.polygonsDataOffset = 0;
								}
							}
						}
						else if (NxTriangleMeshShape* trimeshShape = shape->isTriangleMesh())
						{
							if (mAsset->mParams->collisionWithTriangleMesh)
							{
								CollisionTriMeshData& data = simParams->trimeshes.getElems(_storage_)[ numTriMeshes++ ];
								baseData = &data;

								PxFromNxBounds3(data.aabb, shapeWorldBounds);
								PxFromNxMat34(data.pose, shapeGlobalPose);
								data.inversePose = data.pose.getInverseRT();

								//get TriangleMesh desc
								NxTriangleMeshDesc trimeshDesc;
								trimeshShape->getTriangleMesh().saveToDesc(trimeshDesc);

								const PxU32 numTrimeshIndices = trimeshDesc.numTriangles * 3;
								const PxU32 numTrimeshVerts = trimeshDesc.numVertices;

								if (mTrimeshIndices.getSize() + numTrimeshIndices <= mTrimeshIndices.getCapacity() &&
									mTrimeshVerts.getSize() + numTrimeshVerts <= mTrimeshVerts.getCapacity())
								{
									data.numTriangles = trimeshDesc.numTriangles;
									data.firstIndex = mTrimeshIndices.getSize();
									data.firstVertex = mTrimeshVerts.getSize();

									mTrimeshIndices.setSize(data.firstIndex + numTrimeshIndices);
									//copy TriangleMesh indices
									physx::PxU32* trimeshIndices = mTrimeshIndices.getPtr() + data.firstIndex;
									if (trimeshDesc.flags & NX_MF_16_BIT_INDICES)
									{
										const PxU16* src = static_cast<const PxU16*>(trimeshDesc.triangles);
										for( PxU32 i = 0; i < numTrimeshIndices; i++)
										{
											trimeshIndices[i] = src[i];
										}
									}
									else
									{
										const PxU32* src = static_cast<const PxU32*>(trimeshDesc.triangles);
										for( PxU32 i = 0; i < numTrimeshIndices; i++)
										{
											trimeshIndices[i] = src[i];
										}
									}

									mTrimeshVerts.setSize(data.firstVertex + numTrimeshVerts);
									//copy TriangleMesh vertices
									PxVec4* trimeshVerts = mTrimeshVerts.getPtr() + data.firstVertex;
									const PxU8* src = static_cast<const PxU8*>(trimeshDesc.points);
									for( PxU32 i = 0; i < numTrimeshVerts; i++)
									{
										const PxF32* vert = reinterpret_cast<const PxF32*>(src);
										src += trimeshDesc.pointStrideBytes;

										trimeshVerts[i] = PxVec4(vert[0], vert[1], vert[2], 0);
									}
								}
								else
								{
									APEX_DEBUG_WARNING("BasicIosActor: out of memory to store TriangleMesh data");

									data.numTriangles = 0;
									data.firstIndex = 0;
									data.firstVertex = 0;
								}

							}
						}

						if (baseData)
						{
							NxActor& nxActor = shape->getActor();
							if (nxActor.isDynamic())
							{
								PxFromNxVec3(baseData->bodyCMassPosition, nxActor.getCMassGlobalPosition());
								PxFromNxVec3(baseData->bodyLinearVelocity, nxActor.getLinearVelocity());
								PxFromNxVec3(baseData->bodyAngluarVelocity, nxActor.getAngularVelocity());

								baseData->materialRestitution = mAsset->mParams->restitutionForDynamicShapes;
							}
							else
							{
								baseData->bodyCMassPosition = PxVec3(0.0);
								baseData->bodyLinearVelocity = PxVec3(0.0);
								baseData->bodyAngluarVelocity = PxVec3(0.0);

								baseData->materialRestitution = mAsset->mParams->restitutionForStaticShapes;
							}
							//NxMaterial* nxMaterial = mBasicIosScene->getModulePhysXScene()->getMaterialFromIndex(shape->getMaterial());
							//baseData->materialRestitution = nxMaterial->getRestitution();
						}
					}
#elif NX_SDK_VERSION_MAJOR == 3
					mOverlapHits.resize(maxCollidingObjects);
					PxBoxGeometry overlapGeom(bounds.getExtents());
					PxTransform overlapPose(bounds.getCenter());

					PxSceneQueryFilterData overlapFilterData;
					overlapFilterData.data = mCollisionFilterData;

					overlapFilterData.flags = PxSceneQueryFilterFlags();
					if (mAsset->mParams->staticCollision) overlapFilterData.flags |= PxSceneQueryFilterFlag::eSTATIC;
					if (mAsset->mParams->dynamicCollision) overlapFilterData.flags |= PxSceneQueryFilterFlag::eDYNAMIC;

					SCOPED_PHYSX3_LOCK_READ(mBasicIosScene->getModulePhysXScene());
					PxOverlapBuffer ovBuffer(&mOverlapHits[0], mOverlapHits.size());
					mBasicIosScene->getModulePhysXScene()->overlap(overlapGeom, overlapPose, ovBuffer, overlapFilterData);
					PxI32 numCollidingObjects = ovBuffer.getNbAnyHits();
					if (numCollidingObjects < 0)
					{
						numCollidingObjects = mOverlapHits.size();
						APEX_DEBUG_WARNING("BasicIosActor: too many collision shapes, clamping to %d", numCollidingObjects);
					}

					for (PxI32 iShape = 0; iShape < numCollidingObjects; iShape++)
					{
						PxShape* shape = mOverlapHits[iShape].shape;

						switch (shape->getGeometryType())
						{
						case PxGeometryType::eBOX:
							{
								++numBoxes;
							}
							break;
						case PxGeometryType::eSPHERE:
							{
								++numSpheres;
							}
							break;
						case PxGeometryType::eCAPSULE:
							{
								++numCapsules;
							}
							break;
						case PxGeometryType::ePLANE:
							{
								++numHalfSpaces;
							}
							break;
						case PxGeometryType::eCONVEXMESH:
							{
								if (mAsset->mParams->collisionWithConvex)
								{
									++numConvexMeshes;
								}
							}
							break;
						case PxGeometryType::eTRIANGLEMESH:
							{
								if (mAsset->mParams->collisionWithTriangleMesh)
								{
									++numTriMeshes;
								}
							}
							break;
						default:
							break;
						}
					}

					simParams->boxes.resize(_storage_ , numBoxes);
					simParams->spheres.resize(_storage_ , numSpheres);
					simParams->capsules.resize(_storage_ , numCapsules);
					simParams->halfSpaces.resize(_storage_ , numHalfSpaces);
					simParams->convexMeshes.resize(_storage_ , numConvexMeshes);
					simParams->trimeshes.resize(_storage_ , numTriMeshes);

					numBoxes = 0;
					numSpheres = 0;
					numCapsules = 0;
					numHalfSpaces = 0;
					numConvexMeshes = 0;
					numTriMeshes = 0;

					for (PxI32 iShape = 0; iShape < numCollidingObjects; iShape++)
					{
						PxShape* shape = mOverlapHits[iShape].shape;
						PxRigidActor* actor = mOverlapHits[iShape].actor;

						PxBounds3 shapeWorldBounds = PxShapeExt::getWorldBounds(*shape, *actor);
						PxTransform actorGlobalPose = shape->getActor()->getGlobalPose();
						PxTransform shapeGlobalPose = actorGlobalPose.transform(shape->getLocalPose());

						PX_ASSERT(!shapeWorldBounds.isEmpty());
						shapeWorldBounds.fattenFast(collisionRadius);

						CollisionData* baseData = 0;

						switch (shape->getGeometryType())
						{
						case PxGeometryType::eBOX:
							{
								PxBoxGeometry boxGeom;
								shape->getBoxGeometry(boxGeom);

								CollisionBoxData& data = simParams->boxes.getElems(_storage_)[numBoxes++]; 
								baseData = &data;

								data.aabb = shapeWorldBounds;
								data.pose = PxMat34Legacy(shapeGlobalPose);
								data.inversePose = data.pose.getInverseRT();

								data.halfSize = boxGeom.halfExtents;
							}
							break;
						case PxGeometryType::eSPHERE:
							{
								PxSphereGeometry sphereGeom;
								shape->getSphereGeometry(sphereGeom);

								CollisionSphereData& data = simParams->spheres.getElems(_storage_)[numSpheres++];

								baseData = &data;

								data.aabb = shapeWorldBounds;
								data.pose = PxMat34Legacy(shapeGlobalPose);
								data.inversePose = data.pose.getInverseRT();

								data.radius = sphereGeom.radius;
								//extend
								data.radius += simParams->collisionDistance;
							}
							break;
						case PxGeometryType::eCAPSULE:
							{
								PxCapsuleGeometry capsuleGeom;
								shape->getCapsuleGeometry(capsuleGeom);

								CollisionCapsuleData& data = simParams->capsules.getElems(_storage_)[numCapsules++]; 
								baseData = &data;

								data.aabb = shapeWorldBounds;
								data.pose = PxMat34Legacy(shapeGlobalPose);
								data.inversePose = data.pose.getInverseRT();

								data.halfHeight = capsuleGeom.halfHeight;
								data.radius = capsuleGeom.radius;
								//extend
								data.radius += simParams->collisionDistance;
							}
							break;
						case PxGeometryType::ePLANE:
							{
								//PxPlaneGeometry planeGeom;
								//shape->getPlaneGeometry(planeGeom);

								CollisionHalfSpaceData& data = simParams->halfSpaces.getElems(_storage_)[numHalfSpaces++]; 
								baseData = &data;

								data.origin = shapeGlobalPose.p;
								data.normal = shapeGlobalPose.rotate(PxVec3(1, 0, 0));
								//extend
								data.origin += data.normal * simParams->collisionDistance;
							}
							break;
						case PxGeometryType::eCONVEXMESH:
							{
								if (mAsset->mParams->collisionWithConvex)
								{
									PxConvexMeshGeometry convexGeom;
									shape->getConvexMeshGeometry(convexGeom);

									CollisionConvexMeshData& data = simParams->convexMeshes.getElems(_storage_)[numConvexMeshes++]; 
									baseData = &data;

									data.aabb = shapeWorldBounds;
									data.pose = PxMat34Legacy(shapeGlobalPose);
									data.inversePose = data.pose.getInverseRT();

									//get ConvexMesh
									const PxConvexMesh* convexMesh = convexGeom.convexMesh;

									PxU32 numPolygons = convexMesh->getNbPolygons();
									PxU32 numVertices = convexMesh->getNbVertices();
									PxU32 polygonsDataSize = 0;
									for (PxU32 i = 0; i < numPolygons; i++)
									{
										PxHullPolygon polygon;
										bool polygonDataTest = convexMesh->getPolygonData(i, polygon);
										PX_UNUSED( polygonDataTest );
										PX_ASSERT( polygonDataTest );

										polygonsDataSize += (1 + polygon.mNbVerts);
									}

									if (mConvexPlanes.getSize() + numPolygons <= mConvexPlanes.getCapacity() &&
										mConvexVerts.getSize() + numVertices <= mConvexVerts.getCapacity() &&
										mConvexPolygonsData.getSize() + polygonsDataSize <= mConvexPolygonsData.getCapacity())
									{
										data.numPolygons = numPolygons;
										data.firstPlane = mConvexPlanes.getSize();
										data.firstVertex = mConvexVerts.getSize();
										data.polygonsDataOffset = mConvexPolygonsData.getSize();

										PxPlane* convexPlanes = mConvexPlanes.getPtr() + data.firstPlane;
										PxVec4*  convexVerts = mConvexVerts.getPtr() + data.firstVertex;
										PxU32*   convexPolygonsData = mConvexPolygonsData.getPtr() + data.polygonsDataOffset;

										mConvexPlanes.setSize(data.firstPlane + numPolygons);
										mConvexVerts.setSize(data.firstVertex + numVertices);
										mConvexPolygonsData.setSize(data.polygonsDataOffset + polygonsDataSize);

										//copy Convex Planes & Polygon Data
										const physx::PxU8* srcIndices = convexMesh->getIndexBuffer();
										for (PxU32 i = 0; i < numPolygons; i++)
										{
											PxHullPolygon polygon;
											bool polygonDataTest = convexMesh->getPolygonData(i, polygon);
											PX_UNUSED( polygonDataTest );
											PX_ASSERT( polygonDataTest );

											*convexPlanes++ = PxPlane(polygon.mPlane[0], polygon.mPlane[1], polygon.mPlane[2], polygon.mPlane[3]);

											const PxU32 polygonVertCount = polygon.mNbVerts;
											const physx::PxU8* polygonIndices = srcIndices + polygon.mIndexBase;

											*convexPolygonsData++ = polygonVertCount;
											for (PxU32 j = 0; j < polygonVertCount; ++j)
											{
												*convexPolygonsData++ = *polygonIndices++; 
											}
										}

										//copy Convex Vertices
										const PxVec3* srcVertices = convexMesh->getVertices();
										for (PxU32 i = 0; i < numVertices; i++)
										{
											*convexVerts++ = PxVec4(*srcVertices++, 0);
										}
									}
									else
									{
										APEX_DEBUG_WARNING("BasicIosActor: out of memory to store Convex data");

										data.numPolygons = 0;
										data.firstPlane = 0;
										data.firstVertex = 0;
										data.polygonsDataOffset = 0;
									}
								}
								break;
							}
						case PxGeometryType::eTRIANGLEMESH:
							{
								if (mAsset->mParams->collisionWithTriangleMesh)
								{
									PxTriangleMeshGeometry trimeshGeom;
									shape->getTriangleMeshGeometry(trimeshGeom);

									CollisionTriMeshData& data = simParams->trimeshes.getElems(_storage_)[numTriMeshes++]; 
									baseData = &data;

									data.aabb = shapeWorldBounds;
									PX_ASSERT(!data.aabb.isEmpty());
									data.aabb.fattenFast( simParams->collisionDistance + simParams->collisionThreshold );
									data.pose = PxMat34Legacy(shapeGlobalPose);
									data.inversePose = data.pose.getInverseRT();

									//triangle mesh data
									const PxTriangleMesh* trimesh = trimeshGeom.triangleMesh;

									const PxU32 numTrimeshIndices = trimesh->getNbTriangles() * 3;
									const PxU32 numTrimeshVerts = trimesh->getNbVertices();

									if (mTrimeshIndices.getSize() + numTrimeshIndices <= mTrimeshIndices.getCapacity() &&
										mTrimeshVerts.getSize() + numTrimeshVerts <= mTrimeshVerts.getCapacity())
									{
										data.numTriangles = trimesh->getNbTriangles();
										data.firstIndex = mTrimeshIndices.getSize();
										data.firstVertex = mTrimeshVerts.getSize();

										mTrimeshIndices.setSize(data.firstIndex + numTrimeshIndices);
										//copy TriangleMesh indices
										physx::PxU32* trimeshIndices = mTrimeshIndices.getPtr() + data.firstIndex;

										const bool has16BitIndices = (trimesh->getTriangleMeshFlags() & PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES);
										if (has16BitIndices)
										{
											const PxU16* srcIndices = static_cast<const PxU16*>(trimesh->getTriangles());
											for( PxU32 i = 0; i < numTrimeshIndices; i++)
											{
												trimeshIndices[i] = srcIndices[i];
											}
										}
										else
										{
											const PxU32* srcIndices = static_cast<const PxU32*>(trimesh->getTriangles());
											for( PxU32 i = 0; i < numTrimeshIndices; i++)
											{
												trimeshIndices[i] = srcIndices[i];
											}
										}

										mTrimeshVerts.setSize(data.firstVertex + numTrimeshVerts);
										//copy TriangleMesh vertices
										PxVec4* trimeshVerts = mTrimeshVerts.getPtr() + data.firstVertex;
										const physx::PxVec3* srcVertices = trimesh->getVertices();
										for( PxU32 i = 0; i < numTrimeshVerts; i++)
										{
											trimeshVerts[i] = PxVec4(srcVertices[i], 0);
										}
									}
									else
									{
										APEX_DEBUG_WARNING("BasicIosActor: out of memory to store TriangleMesh data");

										data.numTriangles = 0;
										data.firstIndex = 0;
										data.firstVertex = 0;
									}
								}
								break;
							}
						default:
							break;
						}

						if (baseData)
						{
							if (PxRigidBody* pxBody = shape->getActor()->isRigidBody())
							{
								baseData->bodyCMassPosition = actorGlobalPose.transform(pxBody->getCMassLocalPose().p);
								baseData->bodyLinearVelocity = pxBody->getLinearVelocity();
								baseData->bodyAngluarVelocity = pxBody->getAngularVelocity();

								baseData->materialRestitution = mAsset->mParams->restitutionForDynamicShapes;
							}
							else
							{
								baseData->bodyCMassPosition = actorGlobalPose.p;
								baseData->bodyLinearVelocity = PxVec3(0, 0, 0);
								baseData->bodyAngluarVelocity = PxVec3(0, 0, 0);

								baseData->materialRestitution = mAsset->mParams->restitutionForStaticShapes;
							}
							//PxMaterial* pxMaterial = shape->getMaterialFromInternalFaceIndex(0);
							//PX_ASSERT(pxMaterial);
							//baseData->materialRestitution = pxMaterial->getRestitution();
						}
					}
#endif
				}
			}

			void BasicIosActor::setTaskDependencies(physx::PxTask* iosTask, bool isDataOnDevice)
			{
				physx::PxTaskManager* tm = mBasicIosScene->getApexScene().getTaskManager();

				physx::PxTaskID lodTaskID = tm->getNamedTask(AST_LOD_COMPUTE_BENEFIT);
				mInjectTask->finishBefore(lodTaskID);
				iosTask->startAfter(lodTaskID);

				if (mFieldSamplerQuery != NULL)
				{
					physx::PxF32 deltaTime = mBasicIosScene->getApexScene().getPhysXSimulateTime();

					NiFieldSamplerQueryData queryData;
					queryData.timeStep = deltaTime;
					queryData.count = mParticleCount;
					queryData.isDataOnDevice = isDataOnDevice;
					queryData.strideBytes = sizeof(physx::PxVec4);
					queryData.massStrideBytes = sizeof(physx::PxVec4);		
					if (isDataOnDevice)
					{
#if defined(APEX_CUDA_SUPPORT)
						queryData.pmaInPosition = (PxF32*)mBufDesc.pmaPositionMass->getGpuPtr();
						queryData.pmaInVelocity = (PxF32*)mBufDesc.pmaVelocityLife->getGpuPtr();
						queryData.pmaInMass = &mBufDesc.pmaPositionMass->getGpuPtr()->w;
						queryData.pmaOutField = mField.getGpuPtr();
#endif
					}
					else
					{
						queryData.pmaInPosition = (PxF32*)mBufDesc.pmaPositionMass->getPtr();
						queryData.pmaInVelocity = (PxF32*)mBufDesc.pmaVelocityLife->getPtr();
						queryData.pmaInMass = &mBufDesc.pmaPositionMass->getPtr()->w;
						queryData.pmaOutField = mField.getPtr();
					}
					mFieldSamplerQuery->submitFieldSamplerQuery(queryData, iosTask->getTaskID());
				}

				physx::PxTaskID postIofxTaskID = tm->getNamedTask(AST_PHYSX_FETCH_RESULTS);
				physx::PxTaskID iofxTaskID = mIofxMgr->getUpdateEffectsTaskID(postIofxTaskID);
				if (iofxTaskID == 0)
				{
					iofxTaskID = postIofxTaskID;
				}
				iosTask->finishBefore(iofxTaskID);
			}

			void BasicIosActor::fetchResults()
			{
				for(PxU32 i = 0; i < mInjectorList.getSize(); ++i)
				{
					BasicParticleInjector* inj = DYNAMIC_CAST(BasicParticleInjector*)(mInjectorList.getResource(i));
					inj->assignSimParticlesCount(mInjectorsCounters.get(i));
				}
			}

			physx::PxF32 BasicIosActor::getBenefit()
			{
				physx::PxF32 totalBenefit = mLastBenefitSum + mInjectedBenefitSum;
				physx::PxU32 totalCount = mLastActiveCount + mInjectedCount;

				physx::PxF32 averageBenefit = 0.0f;
				if (totalCount > 0)
				{
					averageBenefit = totalBenefit / totalCount;
				}
				return averageBenefit * mInjectorList.getSize();
			}

			physx::PxF32 BasicIosActor::setResource(physx::PxF32 resourceBudget, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit)
			{
				PX_UNUSED(maxRemaining);
				PX_UNUSED(relativeBenefit);

				physx::PxF32 unitCost = mBasicIosScene->mModule->getLODUnitCost();
				if (mBasicIosScene->mModule->getLODEnabled())
				{
					resourceBudget /= unitCost;
					mParticleBudget = (resourceBudget < UINT_MAX) ? static_cast<physx::PxU32>(resourceBudget) : UINT_MAX;
				}
				else
				{
					mParticleBudget = UINT_MAX;
				}

				if (mParticleBudget > mMaxParticleCount)
				{
					mParticleBudget = mMaxParticleCount;
				}

				physx::PxU32 activeCount = mLastActiveCount + mInjectedCount;
				if (mParticleBudget > activeCount)
				{
					mParticleBudget = activeCount;
				}

				return static_cast<physx::PxF32>(mParticleBudget) * unitCost;
			}

			void BasicIosActor::injectNewParticles()
			{
				mInjectedBenefitSum = 0;
				mInjectedBenefitMin = +FLT_MAX;
				mInjectedBenefitMax = -FLT_MAX;

				physx::PxU32 maxInjectCount = (mMaxTotalParticleCount - mParticleCount);

				physx::PxU32 injectCount = 0;
				physx::PxU32 lastInjectCount = 0;
				do
				{
					lastInjectCount = injectCount;
					for (physx::PxU32 i = 0; i < mInjectorList.getSize(); i++)
					{
						BasicParticleInjector* inj = DYNAMIC_CAST(BasicParticleInjector*)(mInjectorList.getResource(i));
						if (inj->mInjectedParticles.size() == 0)
						{
							continue;
						}

						if (injectCount < maxInjectCount)
						{
							IosNewObject obj;
							if (inj->mInjectedParticles.popFront(obj))
							{
								PxU32 injectIndex = mParticleCount + injectCount;

								physx::PxF32 particleMass = mAsset->getParticleMass();
								mBufDesc.pmaPositionMass->get(injectIndex) = PxVec4(obj.initialPosition.x, obj.initialPosition.y, obj.initialPosition.z, particleMass);
								mBufDesc.pmaVelocityLife->get(injectIndex) = PxVec4(obj.initialVelocity.x, obj.initialVelocity.y, obj.initialVelocity.z, 1.0f);
								mBufDesc.pmaActorIdentifiers->get(injectIndex) = obj.iofxActorID;

								mBufDesc.pmaUserData->get(injectIndex) = obj.userData;

								mLifeSpan[injectIndex] = obj.lifetime;
								mInjector[injectIndex] = inj->mInjectorID;
								mBenefit[injectIndex] = obj.lodBenefit;

								mInjectedBenefitSum += obj.lodBenefit;
								mInjectedBenefitMin = PxMin(mInjectedBenefitMin, obj.lodBenefit);
								mInjectedBenefitMax = PxMax(mInjectedBenefitMax, obj.lodBenefit);

								++injectCount;
							}
						}
					}
				}
				while (injectCount > lastInjectCount);

				mInjectedCount = injectCount;

				//clear injectors FIFO
				for (physx::PxU32 i = 0; i < mInjectorList.getSize(); i++)
				{
					BasicParticleInjector* inj = DYNAMIC_CAST(BasicParticleInjector*)(mInjectorList.getResource(i));

					IosNewObject obj;
					while (inj->mInjectedParticles.popFront(obj))
					{
						;
					}
				}
			}

			////////////////////////////////////////////////////////////////////////////////

			BasicParticleInjector::BasicParticleInjector(NxResourceList& list, BasicIosActor& actor, physx::PxU32 injectorID)
				: mIosActor(&actor)
				, mIofxAssetResID(INVALID_RESOURCE_ID)
				, mIofxAsset(NULL)
				, mVolume(NULL)
				, mLastRandomID(0)
				, mVolumeID(NiIofxActorID::NO_VOLUME)
				, mInjectorID(injectorID)
				, mSimulatedParticlesCount(0)
			{
				mRand.setSeed(actor.mBasicIosScene->getApexScene().getSeed());

				list.add(*this);

				setLODWeights(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
			}

			BasicParticleInjector::~BasicParticleInjector()
			{
			}

			void BasicParticleInjector::setListIndex(NxResourceList& list, physx::PxU32 index)
			{
				m_listIndex = index;
				m_list = &list;

				InjectorParams& injParams = mIosActor->mBasicIosScene->getInjectorParams(mInjectorID);

				injParams.mLocalIndex = index;
			}

			/* Emitter calls this function to adjust their particle weights with respect to other emitters */
			void BasicParticleInjector::setLODWeights(physx::PxF32 maxDistance, physx::PxF32 distanceWeight, physx::PxF32 speedWeight, physx::PxF32 lifeWeight, physx::PxF32 separationWeight, physx::PxF32 bias)
			{
				PX_UNUSED(separationWeight);

				InjectorParams& injParams = mIosActor->mBasicIosScene->getInjectorParams(mInjectorID);

				//normalize weights
				PxF32 totalWeight = distanceWeight + speedWeight + lifeWeight;
				if (totalWeight > PX_EPS_F32)
				{
					distanceWeight /= totalWeight;
					speedWeight /= totalWeight;
					lifeWeight /= totalWeight;
				}

				injParams.mLODMaxDistance = maxDistance;
				injParams.mLODDistanceWeight = distanceWeight;
				injParams.mLODSpeedWeight = speedWeight;
				injParams.mLODLifeWeight = lifeWeight;
				injParams.mLODBias = bias;
			}

			physx::PxTaskID BasicParticleInjector::getCompletionTaskID() const
			{
				return mIosActor->mInjectTask->getTaskID();
			}

			void BasicParticleInjector::init(NxResID iofxAssetResID, physx::apex::NxIofxAsset* iofxAsset)
			{
				mIofxAssetResID = iofxAssetResID;
				mIofxAsset = iofxAsset;

				/* add this injector to the IOFX asset's context (so when the IOFX goes away our ::release() is called) */
				mIofxAsset->addDependentActor(this);

				mRandomActorClassIDs.clear();
				if (mIofxAsset->getMeshAssetCount() < 2)
				{
					mRandomActorClassIDs.pushBack(mIosActor->mIofxMgr->getActorClassID(mIofxAsset, 0));
					return;
				}

				/* Cache actorClassIDs for this asset */
				physx::Array<PxU16> temp;
				for (PxU32 i = 0 ; i < mIofxAsset->getMeshAssetCount() ; i++)
				{
					PxU32 w = mIofxAsset->getMeshAssetWeight(i);
					PxU16 acid = mIosActor->mIofxMgr->getActorClassID(mIofxAsset, (PxU16) i);
					for (PxU32 j = 0 ; j < w ; j++)
					{
						temp.pushBack(acid);
					}
				}

				mRandomActorClassIDs.reserve(temp.size());
				while (temp.size())
				{
					PxU32 index = (PxU32)mRand.getScaled(0, (PxF32)temp.size());
					mRandomActorClassIDs.pushBack(temp[ index ]);
					temp.replaceWithLast(index);
				}
			}


			void BasicParticleInjector::release()
			{
				if (mInRelease)
				{
					return;
				}
				mInRelease = true;
				mIosActor->releaseInjector(*this);
			}

			void BasicParticleInjector::destroy()
			{
				ApexActor::destroy();

				// Release reference counts to IOFX asset
				if (mIofxAssetResID != INVALID_RESOURCE_ID)
				{
					NiResourceProvider* nrp = NiGetApexSDK()->getInternalResourceProvider();
					nrp->releaseResource(mIofxAssetResID);
				}

				delete this;
			}

			void BasicParticleInjector::setPreferredRenderVolume(physx::apex::NxApexRenderVolume* volume)
			{
				mVolume = volume;
				mVolumeID = mVolume ? mIosActor->mIofxMgr->getVolumeID(mVolume) : NiIofxActorID::NO_VOLUME;
			}

			/* Emitter calls this virtual injector API to insert new particles.  It is safe for an emitter to
			* call this function at any time except for during the IOS::fetchResults().  Since
			* ParticleScene::fetchResults() is single threaded, it should be safe to call from
			* emitter::fetchResults() (destruction may want to do this because of contact reporting)
			*/
			void BasicParticleInjector::createObjects(physx::PxU32 count, const IosNewObject* createList)
			{
				PX_PROFILER_PERF_SCOPE("BasicIosCreateObjects");

				if (mRandomActorClassIDs.size() == 0)
				{
					return;
				}

				physx::PxVec3 eyePos = mIosActor->mBasicIosScene->getApexScene().getEyePosition();
				const InjectorParams& injParams = mIosActor->mBasicIosScene->getInjectorParams(mInjectorID);
				// Append new objects to our FIFO.  We do copies because we must perform buffering for the
				// emitters.  We have to hold these new objects until there is room in the NxFluid and the
				// injector's virtID range to emit them.
				for (physx::PxU32 i = 0 ; i < count ; i++)
				{
					IosNewObject obj = *createList++;

					obj.lodBenefit = calcParticleBenefit(injParams, eyePos, obj.initialPosition, obj.initialVelocity, 1.0f);
					obj.iofxActorID.set(mVolumeID, mRandomActorClassIDs[ mLastRandomID++ ]);
					mLastRandomID = mLastRandomID == mRandomActorClassIDs.size() ? 0 : mLastRandomID;
					//mInjectedParticleBenefit += obj.lodBenefit;
					mInjectedParticles.pushBack(obj);
				}
			}

#if defined(APEX_CUDA_SUPPORT)
			void BasicParticleInjector::createObjects(ApexMirroredArray<const IosNewObject>& createArray)
			{
				PX_UNUSED(createArray);

				// An emitter will call this API when it has filled a host or device buffer.  The injector
				// should trigger a copy to the location it would like to see the resulting data when the
				// IOS is finally ticked.

				PX_ALWAYS_ASSERT(); /* Not yet supported */
			}
#endif

			physx::PxF32 BasicParticleInjector::getBenefit()
			{
				return 0.0f;
			}

			physx::PxF32 BasicParticleInjector::setResource(physx::PxF32 suggested, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit)
			{
				PX_UNUSED(suggested);
				PX_UNUSED(maxRemaining);
				PX_UNUSED(relativeBenefit);

				return 0.0f;
			}


#ifdef APEX_TEST
			BasicIosActorTestData* BasicIosActor::createTestData()
			{
				mTestData = new BasicIosActorTestData();
				return mTestData;
			}
			void BasicIosActor::copyTestData() const
			{
				if (mTestData == NULL)
				{
					return;
				}
				mTestData->mParticleCountOld = mParticleCount;

				mTestData->mBenefitOld.resize(mParticleCount);
				for (physx::PxU32 i = 0; i < mParticleCount; i++)
				{
					mTestData->mBenefitOld[i] = mBenefit[i];
				}

				mTestData->mActiveParticleCount = mLastActiveCount;
				mTestData->mBenefitMin = PxMin(mLastBenefitMin, mInjectedBenefitMin);
				mTestData->mBenefitMax = PxMax(mLastBenefitMax, mInjectedBenefitMax) * 1.00001f;

				mTestData->mNOT_A_PARTICLE = NiIosBufferDesc::NOT_A_PARTICLE;
				mTestData->mNEW_PARTICLE_FLAG = NiIosBufferDesc::NEW_PARTICLE_FLAG;
			}

			void BasicIosActor::clearTestData()
			{
				mTestData->mInStateToInput.reset();
				mTestData->mBenefit.reset();
				mTestData->mBenefitOld.reset();
				mTestData->mHoleScanSum.reset();
				mTestData->mMoveIndices.reset();
			}
#endif
		}
	}
} // namespace physx::apex

