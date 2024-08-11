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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "NpPhysics.h"

// PX_SERIALIZATION
#include "CmCollection.h"
#include "CmIO.h"
#include "NpClothFabric.h"
#include "NpCloth.h"
#include "NpParticleSystem.h"
#include "NpParticleFluid.h"
#include "PxErrorCallback.h"
#include "NpRigidStatic.h"
#include "NpRigidDynamic.h"
#include "NpArticulation.h"
#include "NpArticulationLink.h"
#include "NpArticulationJoint.h"
#include "NpMaterial.h"
#include "GuHeightFieldData.h"
#include "GuHeightField.h"
#include "GuConvexMesh.h"
#include "GuTriangleMesh.h"
#include "PsIntrinsics.h"
#include "PxProfileZone.h"
#include "PxProfileZoneManager.h"
#include "PxTolerancesScale.h"
#include "PxvGlobals.h"		// dynamic registration of HFs & articulations in LL
#include "GuOverlapTests.h" // dynamic registration of HFs in Gu
#include "PxDeletionListener.h"

#if PX_SUPPORT_VISUAL_DEBUGGER
#include "PvdConnectionManager.h"
//#include "PvdConnectionDataProviderImpl.h"
#endif
//~PX_SERIALIZATION

#include "NpFactory.h"


#if PX_USE_CLOTH_API
#include "NpCloth.h"
#endif


#include <stdio.h> // sprintf

using namespace physx;
using namespace Cm;

bool		NpPhysics::apiReentryLock	= false;
NpPhysics*	NpPhysics::mInstance		= NULL;
PxU32		NpPhysics::mRefCount		= 0;

static CmEventNameProvider gProfileNameProvider;

NpPhysics::NpPhysics(const PxTolerancesScale& scale, bool trackOutstandingAllocations, PxProfileZoneManager* profileZoneManager) :
	mSceneArray(PX_DEBUG_EXP("physicsSceneArray"))
#if defined(PX_WINDOWS) && !defined(PX_WINMODERN)
	, mNbRegisteredGpuClients(0)
#endif
#if PX_SUPPORT_GPU_PHYSX
	, mPhysicsGpu(*this)
#endif
	, mSceneRunning(NULL)
	, mPhysics(scale)
	, mDeletionListenersExist(false)
{
#if PX_SUPPORT_VISUAL_DEBUGGER
	mVisualDebugger = PX_NEW(Pvd::VisualDebugger);
	PX_ASSERT(mVisualDebugger);
#endif

	PX_UNUSED(trackOutstandingAllocations);

	//mMasterMaterialTable.reserve(10);
		
	//16K event buffer size.
	PxFoundation* theFoundation( &getFoundation() );
	mProfileZone = &PxProfileZone::createProfileZone( theFoundation, "PhysXSDK", gProfileNameProvider );
	if (profileZoneManager)
	{
		profileZoneManager->addProfileZone( *mProfileZone );
	}
	mProfileZoneManager = profileZoneManager;
	
#if PX_SUPPORT_VISUAL_DEBUGGER
	mPVDFactoryManager = &PxVisualDebuggerConnectionManager::create( theFoundation->getAllocator(), theFoundation->getAllocatorCallback(), trackOutstandingAllocations );
	mPVDFactoryManager->addHandler( *mVisualDebugger );
	//mDataProvider = PX_NEW( physx::debugger::PvdConnectionDataProviderImpl )();
	//mPVDFactoryManager->setDataProvider( mDataProvider );
	if (profileZoneManager)
		mPVDFactoryManager->setProfileZoneManager( *profileZoneManager );

#endif

}


NpPhysics::~NpPhysics()
{
	// Release all scenes in case the user didn't do it
	PxU32 nbScenes = mSceneArray.size();
	NpScene** scenes = mSceneArray.begin();
	for(PxU32 i=0;i<nbScenes;i++)
		PX_DELETE_AND_RESET(scenes[i]);
	mSceneArray.clear();

	//PxU32 matCount = mMasterMaterialTable.size();
	//while (mMasterMaterialTable.size() > 0)
	//{
	//	// It's done this way since the material destructor removes the material from the table and adjusts indices

	//	PX_ASSERT(mMasterMaterialTable[0]->getRefCount() == 1);
	//	mMasterMaterialTable[0]->decRefCount();
	//}
	//mMasterMaterialTable.clear();

	mMasterMaterialManager.releaseMaterials();

#if PX_SUPPORT_VISUAL_DEBUGGER
	PX_DELETE(mVisualDebugger);
#endif

	if ( mProfileZone ) mProfileZone->release(); mProfileZone = NULL;
#if PX_SUPPORT_VISUAL_DEBUGGER
	//if ( mDataProvider ) PX_DELETE( static_cast<physx::debugger::PvdConnectionDataProviderImpl*>( mDataProvider ) ); mDataProvider = NULL;
	mPVDFactoryManager->release();
#endif
	mProfileZoneManager = NULL;

	for(PxU32 i=0; i < mDeletionListenerArray.size(); i++)
	{
		PX_DELETE(mDeletionListenerArray[i]);
	}
	mDeletionListenerArray.clear();
	
	destroySceneLock();
}

NpPhysics* NpPhysics::createInstance(PxU32 version, PxFoundation& foundation, const PxTolerancesScale& scale, bool trackOutstandingAllocations, PxProfileZoneManager* profileZoneManager)
{
	PX_UNUSED(foundation);

	if (version!=PX_PHYSICS_VERSION) 
	{
		char buffer[256];
		sprintf(buffer, "Wrong version: PhysX version is 0x%08x, tried to create 0x%08x", PX_PHYSICS_VERSION, version);
		foundation.getErrorCallback().reportError(PxErrorCode::eINVALID_PARAMETER, buffer, __FILE__, __LINE__);
		return NULL;
	}

	if (!scale.isValid())
	{
		foundation.getErrorCallback().reportError(PxErrorCode::eINVALID_PARAMETER, "Scale invalid.\n", __FILE__, __LINE__);
		return NULL; 
	}

	if(0 == mRefCount)
	{
		PX_ASSERT(static_cast<Ps::Foundation*>(&foundation) == &Ps::Foundation::getInstance());

		Ps::Foundation::incRefCount();

		//SerialFactory::createInstance();
		mInstance = PX_NEW (NpPhysics)(scale,trackOutstandingAllocations, profileZoneManager);
		NpFactory::createInstance();
		
#if PX_SUPPORT_VISUAL_DEBUGGER
		NpFactory::getInstance().setNpFactoryListener( *mInstance->mVisualDebugger );
#endif

		NpFactory::getInstance().addFactoryListener(mInstance->mDeletionMeshListener);
	}
	++mRefCount;

	return mInstance;
}

PxU32 NpPhysics::releaseInstance()
{
	PX_ASSERT(mRefCount > 0);
	if (--mRefCount) 
		return mRefCount;

	NpFactory::destroyInstance();

	PX_ASSERT(mInstance);
	PX_DELETE_AND_RESET(mInstance);

	Ps::Foundation::decRefCount();

	return mRefCount;
}

void NpPhysics::release()
{
	NpPhysics::releaseInstance();
}

PxScene* NpPhysics::createScene(const PxSceneDesc& desc)
{
	PX_CHECK_AND_RETURN_NULL(desc.isValid(), "Physics::createScene: desc.isValid() is false!");

	NpScene* npScene = PX_NEW (NpScene)(desc);
	if(!npScene)
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "Unable to create scene.");
		return NULL;
	}
	if(!npScene->getTaskManager())
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "Unable to create scene. Task manager creation failed.");
		return NULL;
	}

	npScene->loadFromDesc(desc);

#if PX_SUPPORT_VISUAL_DEBUGGER
	if(mVisualDebugger->isConnected())
	{
		mVisualDebugger->setupSceneConnection(npScene->getScene());
		npScene->getScene().getSceneVisualDebugger().sendEntireScene();
	}
#endif

#ifdef PX_PS3
	for(PxU32 i=0;i<CmPS3ConfigInternal::SCENE_PARAM_SPU_MAX;i++)
	{
		npScene->getScene().setSceneParamInt((PxPS3ConfigParam::Enum)i, g_iPhysXSPUCount);
	}

	const PxU32 numFrictionBlocks = desc.nbContactDataBlocks/4;
	const PxU32 numContactStreamBlocks = desc.nbContactDataBlocks/16;
	const PxU32 numConstraintBlocks = desc.nbContactDataBlocks - (numFrictionBlocks + numContactStreamBlocks);

	npScene->getScene().setSceneParamInt(PxPS3ConfigParam::eMEM_CONSTRAINT_BLOCKS, numConstraintBlocks);
	npScene->getScene().setSceneParamInt(PxPS3ConfigParam::eMEM_FRICTION_BLOCKS, numFrictionBlocks);
	npScene->getScene().setSceneParamInt(PxPS3ConfigParam::eMEM_CONTACT_STREAM_BLOCKS, numContactStreamBlocks);
#endif

	if (!sendMaterialTable(*npScene) || !npScene->getScene().isValid())
	{
		PX_DELETE(npScene);
		Ps::getFoundation().error(PxErrorCode::eOUT_OF_MEMORY, __FILE__, __LINE__, "Unable to create scene.");
		return NULL;
	}

	mSceneArray.pushBack(npScene);
	return npScene;
}


void NpPhysics::releaseSceneInternal(PxScene& scene)
{
	NpScene* pScene =  static_cast<NpScene*>(&scene);

	for(PxU32 i=0;i<mSceneArray.size();i++)
	{
		if(mSceneArray[i]==pScene)
		{
			mSceneArray.replaceWithLast(i);
			PX_DELETE_AND_RESET(pScene);
			return;
		}
	}
}


PxU32 NpPhysics::getNbScenes() const
{
	return mSceneArray.size();
}


PxU32 NpPhysics::getScenes(PxScene** userBuffer, PxU32 bufferSize, PxU32 startIndex) const
{
	const PxU32 size = mSceneArray.size();

	const PxU32 remainder = PxMax<PxI32>(size - startIndex, 0);
	const PxU32 writeCount = PxMin(remainder, bufferSize);
	for(PxU32 i=0; i<writeCount; i++)
		userBuffer[i] = mSceneArray[i+startIndex];

	return writeCount;
}


PxRigidStatic* NpPhysics::createRigidStatic(const PxTransform& globalPose)
{
	PX_CHECK_AND_RETURN_NULL(globalPose.isSane(), "PxPhysics::createRigidStatic: invalid transform");
	return NpFactory::getInstance().createRigidStatic(globalPose.getNormalized());
}

PxShape* NpPhysics::createShape(const PxGeometry& geometry, PxMaterial*const * materials, PxU16 materialCount, bool isExclusive, PxShapeFlags shapeFlags)
{
	PX_CHECK_AND_RETURN_NULL(materials, "createShape: material pointer is NULL");
	PX_CHECK_AND_RETURN_NULL(materialCount>0, "createShape: material count is zero");

#if defined(PX_CHECKED)
	const bool hasMeshTypeGeom = geometry.getType() == PxGeometryType::eTRIANGLEMESH || geometry.getType() == PxGeometryType::eHEIGHTFIELD;
	
	if(hasMeshTypeGeom && (shapeFlags & PxShapeFlag::eTRIGGER_SHAPE))
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, 
			"NpPhysics::createShape: triangle mesh and heightfield triggers are not supported!");
		return NULL;
	}

	if((shapeFlags & PxShapeFlag::eSIMULATION_SHAPE) && (shapeFlags & PxShapeFlag::eTRIGGER_SHAPE))
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, 
			"NpPhysics::createShape: shapes cannot simultaneously be trigger shapes and simulation shapes.");
		return NULL;
	}
#endif

	return NpFactory::getInstance().createShape(geometry, shapeFlags, materials, materialCount, isExclusive);
}

PxU32 NpPhysics::getNbShapes()	const
{
	return NpFactory::getInstance().getNbShapes();
}

PxU32 NpPhysics::getShapes(PxShape** userBuffer, PxU32 bufferSize, PxU32 startIndex)	const
{
	return NpFactory::getInstance().getShapes(userBuffer, bufferSize, startIndex);
}





PxRigidDynamic* NpPhysics::createRigidDynamic(const PxTransform& globalPose)
{
	PX_CHECK_AND_RETURN_NULL(globalPose.isSane(), "PxPhysics::createRigidDynamic: invalid transform");
	return NpFactory::getInstance().createRigidDynamic(globalPose.getNormalized());
}


PxConstraint* NpPhysics::createConstraint(PxRigidActor* actor0, PxRigidActor* actor1, PxConstraintConnector& connector, const PxConstraintShaderTable& shaders, PxU32 dataSize)
{
	return NpFactory::getInstance().createConstraint(actor0, actor1, connector, shaders, dataSize);
}


PxArticulation* NpPhysics::createArticulation()
{
	return NpFactory::getInstance().createArticulation();
}


// PX_AGGREGATE


PxAggregate* NpPhysics::createAggregate(PxU32 maxSize, bool selfCollisionEnabled)
{
	return NpFactory::getInstance().createAggregate(maxSize, selfCollisionEnabled);
}
//~PX_AGGREGATE


#if PX_USE_PARTICLE_SYSTEM_API
PxParticleSystem* NpPhysics::createParticleSystem(PxU32 maxParticles, bool perParticleRestOffset)
{
	return NpFactory::getInstance().createParticleSystem(maxParticles, perParticleRestOffset);
}


PxParticleFluid* NpPhysics::createParticleFluid(PxU32 maxParticles, bool perParticleRestOffset)
{
	return NpFactory::getInstance().createParticleFluid(maxParticles, perParticleRestOffset);
}
#endif

#if PX_USE_CLOTH_API
PxCloth* NpPhysics::createCloth(const PxTransform& globalPose, PxClothFabric& fabric, const PxClothParticle* particles, PxClothFlags flags)
{
	PX_CHECK_AND_RETURN_NULL(globalPose.isSane(), "PxPhysics::createCloth: invalid transform");
	return NpFactory::getInstance().createCloth(globalPose.getNormalized(), fabric, particles, flags);
}
#endif




///////////////////////////////////////////////////////////////////////////////

NpMaterial* NpPhysics::addMaterial(NpMaterial* m)
{
	if(m)
	{
		Ps::Mutex::ScopedLock lock(mMaterialMutex);

		//the handle is set inside the setMaterial method
		if(mMasterMaterialManager.setMaterial(m))
		{
			// Let all scenes know of the new material
			for(PxU32 i=0; i < getNbScenes(); i++)
			{
				NpScene* s = getScene(i);
				s->addMaterial(*m);
			}

			return m;
		}
		else
		{
#ifdef PX_CHECKED
#ifdef PX_PS3
			Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "Cannot create material: There is a limit of 127 user created materials on PS3.");
#endif
#endif
			m->release();
			return NULL;
		}
	}
	return NULL;
}

PxMaterial* NpPhysics::createMaterial(PxReal staticFriction, PxReal dynamicFriction, PxReal restitution)
{
	PxMaterial* m = NpFactory::getInstance().createMaterial(staticFriction, dynamicFriction, restitution);
	return addMaterial(static_cast<NpMaterial*>(m));
}

PxU32 NpPhysics::getNbMaterials() const
{
	Ps::Mutex::ScopedLock lock(const_cast<Ps::Mutex&>(mMaterialMutex));
	NpMaterialManagerIterator iter(mMasterMaterialManager);
	return iter.getNumMaterial();
}

PxU32 NpPhysics::getMaterials(PxMaterial** userBuffer, PxU32 bufferSize, PxU32 startIndex) const
{
	Ps::Mutex::ScopedLock lock(const_cast<Ps::Mutex&>(mMaterialMutex));
	NpMaterialManagerIterator iter(mMasterMaterialManager);
	PxU32 writeCount =0;
	PxU32 index = 0;
	while(iter.hasNextMaterial())
	{
		if(index++ < startIndex)
			continue;
		if(writeCount == bufferSize)
			break;
		NpMaterial* mat = iter.getNextMaterial();
		userBuffer[writeCount++] = mat;
	}
	return writeCount;
}

void NpPhysics::removeMaterialFromTable(NpMaterial& m)
{
	Ps::Mutex::ScopedLock lock(mMaterialMutex);

	// Let all scenes know of the deleted material
	for(PxU32 i=0; i < getNbScenes(); i++)
	{
		NpScene* s = getScene(i);
		s->removeMaterial(m);
	}

	mMasterMaterialManager.removeMaterial(&m);
}

void NpPhysics::updateMaterial(NpMaterial& m)
{
	Ps::Mutex::ScopedLock lock(mMaterialMutex);

	// Let all scenes know of the updated material
	for(PxU32 i=0; i < getNbScenes(); i++)
	{
		NpScene* s = getScene(i);
		s->updateMaterial(m);
	}
	mMasterMaterialManager.updateMaterial(&m);
}

bool NpPhysics::sendMaterialTable(NpScene& scene)
{
	Ps::Mutex::ScopedLock lock(mMaterialMutex);

	NpMaterialManagerIterator iter(mMasterMaterialManager);
	while(iter.hasNextMaterial())
	{
		NpMaterial* mat = iter.getNextMaterial();
		if(!scene.addMaterial(*mat))
			return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
PxTriangleMesh* NpPhysics::createTriangleMesh(PxInputStream& stream)
{
	return NpFactory::getInstance().createTriangleMesh(stream);
}

PxU32 NpPhysics::getNbTriangleMeshes() const
{
	return NpFactory::getInstance().getNbTriangleMeshes();
}

PxU32 NpPhysics::getTriangleMeshes(PxTriangleMesh** userBuffer, PxU32 bufferSize, PxU32 startIndex) const
{
	return NpFactory::getInstance().getTriangleMeshes(userBuffer, bufferSize, startIndex);
}

///////////////////////////////////////////////////////////////////////////////
PxHeightField* NpPhysics::createHeightField(const PxHeightFieldDesc& desc)
{
	return NpFactory::getInstance().createHeightField(desc);
}

PxHeightField* NpPhysics::createHeightField(PxInputStream& stream)
{
	return NpFactory::getInstance().createHeightField(stream);
}

PxU32 NpPhysics::getNbHeightFields() const
{
	return NpFactory::getInstance().getNbHeightFields();
}

PxU32 NpPhysics::getHeightFields(PxHeightField** userBuffer, PxU32 bufferSize, PxU32 startIndex) const
{
	return NpFactory::getInstance().getHeightFields(userBuffer, bufferSize, startIndex);
}

///////////////////////////////////////////////////////////////////////////////
PxConvexMesh* NpPhysics::createConvexMesh(PxInputStream& stream)
{
	return NpFactory::getInstance().createConvexMesh(stream);
}


PxU32 NpPhysics::getNbConvexMeshes() const
{
	return NpFactory::getInstance().getNbConvexMeshes();
}

PxU32 NpPhysics::getConvexMeshes(PxConvexMesh** userBuffer, PxU32 bufferSize, PxU32 startIndex) const
{
	return NpFactory::getInstance().getConvexMeshes(userBuffer, bufferSize, startIndex);
}

///////////////////////////////////////////////////////////////////////////////


#if PX_USE_CLOTH_API
PxClothFabric* NpPhysics::createClothFabric(PxInputStream& stream)
{
	return NpFactory::getInstance().createClothFabric(stream);
}

PxClothFabric* NpPhysics::createClothFabric(const PxClothFabricDesc& desc)
{
	return NpFactory::getInstance().createClothFabric(desc);
}

PxU32 NpPhysics::getNbClothFabrics() const
{
	return NpFactory::getInstance().getNbClothFabrics();
}


PxU32 NpPhysics::getClothFabrics(PxClothFabric** userBuffer, PxU32 bufferSize) const
{
	return NpFactory::getInstance().getClothFabrics(userBuffer, bufferSize);
}
#endif

#if defined(PX_WINDOWS) && !defined(PX_WINMODERN)
void NpPhysics::registerPhysXIndicatorGpuClient()
{
	Ps::Mutex::ScopedLock lock(mPhysXIndicatorMutex);

	++mNbRegisteredGpuClients;

	mPhysXIndicator.setIsGpu(mNbRegisteredGpuClients>0);
}

void NpPhysics::unregisterPhysXIndicatorGpuClient()
{
	Ps::Mutex::ScopedLock lock(mPhysXIndicatorMutex);

	if (mNbRegisteredGpuClients)
		--mNbRegisteredGpuClients;

	mPhysXIndicator.setIsGpu(mNbRegisteredGpuClients>0);
}
#endif

///////////////////////////////////////////////////////////////////////////////
void NpPhysics::registerDeletionListener(PxDeletionListener& observer, const PxDeletionEventFlags& deletionEvents, bool restrictedObjectSet)
{
	Ps::Mutex::ScopedLock lock(mDeletionListenerMutex);

	PxU32 idx = NpDelListenerEntry::find(mDeletionListenerArray, &observer);
	if(idx == mDeletionListenerArray.size())
	{
		NpDelListenerEntry* e = PX_NEW(NpDelListenerEntry)(&observer, deletionEvents);
		if (e)
		{
			e->restrictedObjectSet = restrictedObjectSet;
			mDeletionListenerArray.pushBack(e);
			mDeletionListenersExist = true;
		}
	}
	else
		PX_ASSERT(mDeletionListenersExist);
}

void NpPhysics::unregisterDeletionListener(PxDeletionListener& observer)
{
	Ps::Mutex::ScopedLock lock(mDeletionListenerMutex);

	PxU32 idx = NpDelListenerEntry::find(mDeletionListenerArray, &observer);
	if(idx < mDeletionListenerArray.size())
	{
		PX_DELETE(mDeletionListenerArray[idx]);
		mDeletionListenerArray.replaceWithLast(idx);
	}
	mDeletionListenersExist = mDeletionListenerArray.size()>0;
}


void NpPhysics::registerDeletionListenerObjects(PxDeletionListener& observer, const PxBase* const* observables, PxU32 observableCount)
{
	PxU32 idx = NpDelListenerEntry::find(mDeletionListenerArray, &observer);
	if(idx < mDeletionListenerArray.size())
	{
		NpDelListenerEntry& entry = *mDeletionListenerArray[idx];
		PX_CHECK_AND_RETURN(entry.restrictedObjectSet, "PxPhysics::registerDeletionListenerObjects: deletion listener is not configured to receive events from specific objects.");

		for(PxU32 i=0; i < observableCount; i++)
			entry.registeredObjects.insert(observables[i]);
	}
	else
		PX_CHECK_AND_RETURN(false, "PxPhysics::registerDeletionListenerObjects: deletion listener has to be registered in PxPhysics first.");
}


void NpPhysics::unregisterDeletionListenerObjects(PxDeletionListener& observer, const PxBase* const* observables, PxU32 observableCount)
{
	PxU32 idx = NpDelListenerEntry::find(mDeletionListenerArray, &observer);
	if(idx < mDeletionListenerArray.size())
	{
		NpDelListenerEntry& entry = *mDeletionListenerArray[idx];
		if (entry.restrictedObjectSet)
		{
			for(PxU32 i=0; i < observableCount; i++)
				entry.registeredObjects.erase(observables[i]);
		}
		else
			PX_CHECK_AND_RETURN(false, "PxPhysics::unregisterDeletionListenerObjects: deletion listener is not configured to receive events from specific objects.");
	}
	else
		PX_CHECK_AND_RETURN(false, "PxPhysics::unregisterDeletionListenerObjects: deletion listener has to be registered in PxPhysics first.");
}


void NpPhysics::notifyDeletionListeners(const PxBase* base, void* userData, PxDeletionEventFlag::Enum deletionEvent)
{
	// we don't protect the check for whether there are any listeners, because we don't want to take a hit in the 
	// common case where there are no listeners. Note the API comments here, that users should not register or 
	// unregister deletion listeners while deletions are occurring

	if(mDeletionListenersExist)
	{
		Ps::Mutex::ScopedLock lock(mDeletionListenerMutex);
		for(PxU32 i=0;i<mDeletionListenerArray.size();i++)
		{
			const NpDelListenerEntry& entry = *mDeletionListenerArray[i];
			
			if (entry.flags & deletionEvent)
			{
				if (entry.restrictedObjectSet)
				{
					if (entry.registeredObjects.contains(base))
						entry.listener->onRelease(base, userData, deletionEvent);
				}
				else
					entry.listener->onRelease(base, userData, deletionEvent);
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
const PxTolerancesScale& NpPhysics::getTolerancesScale() const
{
	return mPhysics.getTolerancesScale();
}

physx::PxProfileZone& NpPhysics::getProfileZone()
{
	return *mProfileZone;
}

physx::PxProfileZoneManager* NpPhysics::getProfileZoneManager()
{
	return mProfileZoneManager;
}

PxVisualDebugger* NpPhysics::getVisualDebugger()
{
#if PX_SUPPORT_VISUAL_DEBUGGER
	return mVisualDebugger;
#else
	return NULL;
#endif
}

physx::debugger::comm::PvdConnectionManager* NpPhysics::getPvdConnectionManager()
{
#if PX_SUPPORT_VISUAL_DEBUGGER
	return mPVDFactoryManager;
#else
	return NULL;
#endif
}

PxFoundation& NpPhysics::getFoundation()
{
	return Ps::Foundation::getInstance();
}


PxPhysics& PxGetPhysics()
{
	return NpPhysics::getInstance();
}


PxPhysics* PxCreateBasePhysics(PxU32 version, PxFoundation& foundation, const PxTolerancesScale& scale, bool trackOutstandingAllocations, PxProfileZoneManager* profileZoneManager)
{
	return NpPhysics::createInstance(version, foundation, scale, trackOutstandingAllocations, profileZoneManager);
}

void PxRegisterArticulations(PxPhysics& physics)
{
	PX_UNUSED(&physics);	// for the moment
	PxvRegisterArticulations();
	NpFactory::registerArticulations();	
}

void PxRegisterUnifiedHeightFields(PxPhysics& physics)
{
	PX_UNUSED(&physics);	// for the moment
	PxvRegisterHeightFields();
	Gu::registerHeightFields();	
}

void PxRegisterHeightFields(PxPhysics& physics)
{
	PX_UNUSED(&physics);	// for the moment
	PxvRegisterLegacyHeightFields();
	Gu::registerHeightFields();	
}

void PxRegisterCloth(PxPhysics& physics)
{
	PX_UNUSED(&physics);	// for the moment

#if PX_USE_CLOTH_API
	//PxvRegisterCloth();
	NpFactory::registerCloth();	

	PxScene* s;
	for(PxU32 i = 0;  i < physics.getNbScenes(); i++)
	{
		physics.getScenes(&s, 1, i);
		NpScene* ns = static_cast<NpScene*>(s);
		ns->getScene().getScScene().createClothSolver();
	}
#endif
}

void PxRegisterParticles(PxPhysics& physics)
{
	PX_UNUSED(&physics);	// for the moment

#if PX_USE_PARTICLE_SYSTEM_API
	PxvRegisterParticles();
	NpFactory::registerParticles();
#endif
}

static const PxU32 gGlobalConvexVersionPC		      = 0x00030001;
static const PxU32 gGlobalTriangleVersionPC		      = 0x00020000;
static const PxU32 gGlobalConvexVersionXENON	      = 0x00030001;
static const PxU32 gGlobalTriangleVersionXENON	      = 0x00020000;
static const PxU32 gGlobalConvexVersionPlaystation3	  = 0x00030001;
static const PxU32 gGlobalTriangleVersionPlaystation3 = 0x00020000;

PxU32 PxGetValue(PxCookingValue::Enum val)
{
	switch(val)
	{
		case PxCookingValue::eCONVEX_VERSION_PC:			return gGlobalConvexVersionPC;
		case PxCookingValue::eMESH_VERSION_PC:				return gGlobalTriangleVersionPC;
		case PxCookingValue::eCONVEX_VERSION_XENON:			return gGlobalConvexVersionXENON;
		case PxCookingValue::eMESH_VERSION_XENON:			return gGlobalTriangleVersionXENON;
		case PxCookingValue::eCONVEX_VERSION_PLAYSTATION3:	return gGlobalConvexVersionPlaystation3;
		case PxCookingValue::eMESH_VERSION_PLAYSTATION3:	return gGlobalTriangleVersionPlaystation3;
	}
	return 0;
}

bool NpPhysics::lockScene()
{
	//The mutex on ps3 (lw and non-lw) has the restriction that lock and unlock must be called from the same thread.
	//This is an unreasonable restriction when trying to force scenes to run sequentially. 
	//The solution adopted here is to use a binary semaphore on ps3 and mutexes on other platforms.
	//Another solution to this problem might have been to add a x-platform semaphore class to foundation but this
	//seems like a lot of unnecessary work for a single use case.  If we have more uses cases for semaphores then
	//it would make sense to tidy up this code and implement the semaphore across all platforms.
#ifdef PX_PS3
	return lockScenePS3();
#else
	if(!mSceneRunning)
	{
		mSceneRunning = PX_ALLOC(sizeof(Ps::Mutex), PX_DEBUG_EXP("Ps::Mutex"));
		new (mSceneRunning) Ps::Mutex;
	}
	Ps::Mutex* mutex = (Ps::Mutex*)mSceneRunning;
	mutex->lock();

	return true;
#endif
}

bool NpPhysics::unlockScene()
{
	//The mutex on ps3 (lw and non-lw) has the restriction that lock and unlock must be called from the same thread.
	//This is an unreasonable restriction when trying to force scenes to run sequentially. 
	//The solution adopted here is to use a binary semaphore on ps3 and mutexes on other platforms.
	//Another solution to this problem might have been to add a x-platform semaphore class to foundation but this
	//seems like a lot of unnecessary work for a single use case.  If we have more uses cases for semaphores then
	//it would make sense to tidy up this code and implement the semaphore across all platforms.
	PX_ASSERT(mSceneRunning);
#ifdef PX_PS3
	return unlockScenePS3();
#else
	Ps::Mutex* mutex = (Ps::Mutex*)mSceneRunning;
	mutex->unlock();

	return true;
#endif
}

void NpPhysics::destroySceneLock()
{
	//The mutex on ps3 (lw and non-lw) has the restriction that lock and unlock must be called from the same thread.
	//This is an unreasonable restriction when trying to force scenes to run sequentially. 
	//The solution adopted here is to use a binary semaphore on ps3 and mutexes on other platforms.
	//Another solution to this problem might have been to add a x-platform semaphore class to foundation but this
	//seems like a lot of unnecessary work for a single use case.  If we have more uses cases for semaphores then
	//it would make sense to tidy up this code and implement the semaphore across all platforms.
#ifdef PX_PS3
	destroySceneLockPS3();
#else
	if(mSceneRunning)
	{
		Ps::Mutex* mutex = (Ps::Mutex*)mSceneRunning;
		mutex->~Mutex();
		PX_FREE_AND_RESET(mSceneRunning);
	}
#endif
}

void PxAddCollectionToPhysics(const PxCollection& collection)
{
	NpFactory& factory = NpFactory::getInstance();
	const Cm::Collection& c = static_cast<const Cm::Collection&>(collection);
    factory.addCollection(c.internalGetNbObjects(), c.internalGetObjects());
}
