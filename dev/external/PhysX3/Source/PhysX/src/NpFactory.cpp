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


#include "NpFactory.h"
#include "NpPhysics.h"
#include "ScPhysics.h"

#include "NpScene.h"
#include "ScbScene.h"

#include "NpRigidStatic.h"
#include "ScbActor.h"

#include "NpRigidDynamic.h"
#include "ScbBody.h"

#include "NpShape.h"
#include "ScbShape.h"

#include "GuHeightField.h"
#include "GuHeightFieldUtil.h"

#include "NpArticulation.h"
#include "NpArticulationLink.h"
#include "NpArticulationJoint.h"
#include "ScbArticulation.h"

#include "NpAggregate.h"
#include "ScbAggregate.h"


#if PX_USE_PARTICLE_SYSTEM_API
#include "NpParticleSystem.h"
#include "NpParticleFluid.h"
#endif

#if PX_USE_CLOTH_API
#include "NpClothFabric.h"
#include "NpCloth.h"
#include "PxClothCollisionData.h"
#endif

#include "NpConnector.h"

using namespace physx;

NpFactory::NpFactory()
: GuMeshFactory()
, mConnectorArrayPool(PX_DEBUG_EXP("connectorArrayPool"))
#if PX_USE_CLOTH_API
	, mClothFabricArray(PX_DEBUG_EXP("clothFabricArray"))
#endif
#if PX_SUPPORT_VISUAL_DEBUGGER
	, mNpFactoryListener(NULL)
#endif
{
}

namespace
{
	template <typename T> void releaseAll(Ps::HashSet<T*>& container)
	{
		// a bit tricky: release will call the factory back to remove the object from
		// the tracking array, immediately evaluating the iterator. Reconstructing the
		// iterator per delete can be expensive. So, we use a temporary object.
		//
		// a coalesced hash would be efficient too, but we only ever iterate over it
		// here so it's not worth the 2x remove penalty over the normal hash.

		Ps::Array<T*, Ps::ReflectionAllocator<T*> > tmp;
		tmp.reserve(container.size());
		for(typename Ps::HashSet<T*>::Iterator iter = container.getIterator(); !iter.done(); ++iter)
			tmp.pushBack(*iter);

		PX_ASSERT(tmp.size() == container.size());
		for(PxU32 i=0;i<tmp.size();i++)
			tmp[i]->release();
	}
}


NpFactory::~NpFactory()
{
}


void NpFactory::release()
{
	releaseAll(mAggregateTracking);
	releaseAll(mConstraintTracking);
	releaseAll(mArticulationTracking);
	releaseAll(mActorTracking);
	while(mShapeTracking.size())
		static_cast<NpShape*>(mShapeTracking.getEntries()[0])->releaseInternal();

#if PX_USE_CLOTH_API
	while(mClothFabricArray.size())
	{
		mClothFabricArray[0]->release();
	}
#endif

	GuMeshFactory::release();  // deletes the class
}

void NpFactory::createInstance()
{
	PX_ASSERT(!mInstance);
	mInstance = PX_NEW(NpFactory)();
}


void NpFactory::destroyInstance()
{
	PX_ASSERT(mInstance);
	mInstance->release();
	mInstance = NULL;
}


NpFactory* NpFactory::mInstance = NULL;

///////////////////////////////////////////////////////////////////////////////

namespace
{
	template <typename T> void addToTracking(Ps::HashSet<T*>& set, T* element, Ps::Mutex& mutex, bool lock=true)
	{
		if(!element)
			return;

		if(lock)
			mutex.lock();

		set.insert(element);

		if(lock)
			mutex.unlock();
	}
}

/////////////////////////////////////////////////////////////////////////////// Actors

void NpFactory::addRigidStatic(PxRigidStatic* npActor, bool lock)
{
	addToTracking<PxActor>(mActorTracking, npActor, mTrackingMutex, lock);
}

void NpFactory::addRigidDynamic(PxRigidDynamic* npBody, bool lock)
{
	addToTracking<PxActor>(mActorTracking, npBody, mTrackingMutex, lock);
}

void NpFactory::addShape(PxShape* shape, bool lock)
{
	// this uses a coalesced hash set rather than a normal hash set so that we can iterate over it efficiently
	if(!shape)
		return;

	if(lock)
		mTrackingMutex.lock();

	mShapeTracking.insert(shape);

	if(lock)
		mTrackingMutex.unlock();
}


#if PX_USE_PARTICLE_SYSTEM_API

namespace
{
	NpParticleSystem* createParticleSystem(PxU32 maxParticles, bool perParticleRestOffset)
	{
		return PX_NEW(NpParticleSystem)(maxParticles, perParticleRestOffset);
	}

	NpParticleFluid* createParticleFluid(PxU32 maxParticles, bool perParticleRestOffset)
	{
		return PX_NEW(NpParticleFluid)(maxParticles, perParticleRestOffset);
	}

	// pointers to function above, initialized during subsystem registration
	NpParticleSystem* (*sCreateParticleSystemFn)(PxU32 maxParticles, bool perParticleRestOffset) = 0;
	NpParticleFluid* (*sCreateParticleFluidFn)(PxU32 maxParticles, bool perParticleRestOffset) = 0;
}

void NpFactory::registerParticles()
{
	sCreateParticleSystemFn = &::createParticleSystem;
	sCreateParticleFluidFn = &::createParticleFluid;
}

void NpFactory::addParticleSystem(PxParticleSystem* ps, bool lock)
{
	addToTracking<PxActor>(mActorTracking, ps, mTrackingMutex, lock);
}

void NpFactory::addParticleFluid(PxParticleFluid* fluid, bool lock)
{
	addToTracking<PxActor>(mActorTracking, fluid, mTrackingMutex, lock);
}

PxParticleFluid* NpFactory::createParticleFluid(PxU32 maxParticles, bool perParticleRestOffset)
{
	if (!sCreateParticleFluidFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
			"Particle fluid creation failed. Use PxRegisterParticles to register particle module: returned NULL.");
		return NULL;
	}

	PxParticleFluid* fluid = sCreateParticleFluidFn(maxParticles, perParticleRestOffset);
	if (!fluid)
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
			"Particle fluid initialization failed: returned NULL.");
		return NULL;
	}

	addParticleFluid(fluid);
	return fluid;
}

PxParticleSystem* NpFactory::createParticleSystem(PxU32 maxParticles, bool perParticleRestOffset)
{
	if (!sCreateParticleSystemFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
			"Particle system creation failed. Use PxRegisterParticles to register particle module: returned NULL.");
		return NULL;
	}

	PxParticleSystem* ps = sCreateParticleSystemFn(maxParticles, perParticleRestOffset);
	if (!ps)
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
			"Particle system initialization failed: returned NULL.");
		return NULL;
	}

	addParticleSystem(ps);
	return ps;
}

#endif


#if PX_USE_CLOTH_API

namespace 
{
	NpCloth* createCloth(const PxTransform& globalPose, PxClothFabric& fabric, const PxClothParticle* particles, PxClothFlags flags)
	{
		return PX_NEW(NpCloth)(globalPose, static_cast<NpClothFabric&>(fabric), particles, flags);
	}

	NpClothFabric* createClothFabric(PxInputStream& stream)
	{
		if(NpClothFabric* fabric = PX_NEW(NpClothFabric)())
		{
			if(fabric->load(stream))
				return fabric;
			fabric->decRefCount();
		}
		return NULL;
	}

	NpClothFabric* createClothFabric(const PxClothFabricDesc& desc)
	{
		if(NpClothFabric* fabric = PX_NEW(NpClothFabric)())
		{
			if(fabric->load(desc))
				return fabric;
			fabric->decRefCount();
		}
		return NULL;
	}

	// pointers to functions above, initialized during subsystem registration
	NpCloth* (*sCreateClothFn)(const PxTransform&, PxClothFabric&, const PxClothParticle*, PxClothFlags) = 0;
	NpClothFabric* (*sCreateClothFabricFromStreamFn)(PxInputStream&) = 0;
	NpClothFabric* (*sCreateClothFabricFromDescFn)(const PxClothFabricDesc&) = 0;
}

void NpFactory::registerCloth()
{
	sCreateClothFn = &::createCloth;
	sCreateClothFabricFromStreamFn = &::createClothFabric;
	sCreateClothFabricFromDescFn = &::createClothFabric;

	Sc::Physics::getInstance().registerCloth();	
}

void NpFactory::addCloth(PxCloth* cloth, bool lock)
{
	addToTracking<PxActor>(mActorTracking, cloth, mTrackingMutex, lock);
}

void NpFactory::addClothFabric(NpClothFabric* cf, bool lock)
{
	if(lock)
	{
		Ps::Mutex::ScopedLock lock(mTrackingMutex);
		if(!mClothFabricArray.size())
			mClothFabricArray.reserve(64);

		mClothFabricArray.pushBack(cf);
	}
	else
	{
		if(!mClothFabricArray.size())
			mClothFabricArray.reserve(64);

		mClothFabricArray.pushBack(cf);
	}
}

PxClothFabric* NpFactory::createClothFabric(PxInputStream& stream)
{
	if(!sCreateClothFabricFromStreamFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Cloth not registered: returned NULL.");
		return NULL;
	}

	NpClothFabric* result = (*sCreateClothFabricFromStreamFn)(stream);

	if(result)
		addClothFabric(result);

	return result;
}

PxClothFabric* NpFactory::createClothFabric(const PxClothFabricDesc& desc)
{
	if(!sCreateClothFabricFromDescFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Cloth not registered: returned NULL.");
		return NULL;
	}

	NpClothFabric* result = (*sCreateClothFabricFromDescFn)(desc);

	if(result)
		addClothFabric(result);

	return result;
}

bool NpFactory::removeClothFabric(PxClothFabric& cf)
{
	NpClothFabric* npClothFabric = &static_cast<NpClothFabric&>(cf);

	Ps::Mutex::ScopedLock lock(mTrackingMutex);

	// remove the cloth fabric from the array
	for(PxU32 i=0; i<mClothFabricArray.size(); i++)
	{
		if(mClothFabricArray[i]==npClothFabric)
		{
			mClothFabricArray.replaceWithLast(i);
#if PX_SUPPORT_VISUAL_DEBUGGER
			if(mNpFactoryListener)
				mNpFactoryListener->onNpFactoryBufferRelease(cf);
#endif
			return true;
		}
	}
	return false;
}

PxU32 NpFactory::getNbClothFabrics() const
{
	return mClothFabricArray.size();
}

PxU32 NpFactory::getClothFabrics(PxClothFabric** userBuffer, PxU32 bufferSize) const
{
	const PxU32 size = mClothFabricArray.size();

	const PxU32 writeCount = PxMin(size, bufferSize);
	for(PxU32 i=0; i<writeCount; i++)
		userBuffer[i] = mClothFabricArray[i];

	return writeCount;
}

PxCloth* NpFactory::createCloth(const PxTransform& globalPose, PxClothFabric& fabric, const PxClothParticle* particles, PxClothFlags flags)
{
	PX_CHECK_AND_RETURN_NULL(globalPose.isValid(),"globalPose is not valid.  createCloth returns NULL.");
	PX_CHECK_AND_RETURN_NULL((particles != NULL) && fabric.getNbParticles(), "No particles supplied. createCloth returns NULL.");

#ifdef PX_CHECKED
	PX_CHECK_AND_RETURN_NULL(NpCloth::checkParticles(fabric.getNbParticles(), particles), "PxPhysics::createCloth: particle values must be finite and inverse weight must not be negative");
#endif

	if(!sCreateClothFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Cloth not registered: returned NULL.");
		return NULL;
	}

	// create the internal cloth object
	NpCloth* npCloth = (*sCreateClothFn)(globalPose, fabric, particles, flags);
	if (npCloth)
	{
		addToTracking<PxActor>(mActorTracking, npCloth, mTrackingMutex);
		return npCloth;
	}
	else
	{
		Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
			"Cloth initialization failed: returned NULL.");
		return NULL;
	}
}
#endif

void NpFactory::onActorRelease(PxActor* a)
{
	Ps::Mutex::ScopedLock lock(mTrackingMutex);
	mActorTracking.erase(a);
}

void NpFactory::onShapeRelease(PxShape* a)
{
	Ps::Mutex::ScopedLock lock(mTrackingMutex);
	mShapeTracking.erase(a);
}


void NpFactory::addArticulation(PxArticulation* npArticulation, bool lock)
{
	addToTracking<PxArticulation>(mArticulationTracking, npArticulation, mTrackingMutex, lock);
}

namespace
{
	NpArticulation* createArticulation()
	{
		NpArticulation* npArticulation = PX_NEW(NpArticulation);
		if (!npArticulation)
			Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "Articulation initialization failed: returned NULL.");

		return npArticulation;
	}

	NpArticulationLink* createArticulationLink(NpArticulation&root, NpArticulationLink* parent, const PxTransform& pose)
	{
		PX_CHECK_AND_RETURN_NULL(pose.isValid(),"Supplied PxArticulation pose is not valid. Articulation link creation method returns NULL.");
		PX_CHECK_AND_RETURN_NULL((!parent || (&parent->getRoot() == &root)), "specified parent link is not part of the destination articulation. Articulation link creation method returns NULL.");

		NpArticulationLink* npArticulationLink = PX_NEW(NpArticulationLink)(pose, root, parent);
		if (!npArticulationLink)
		{
			Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
				"Articulation link initialization failed: returned NULL.");
			return NULL;
		}

		if (parent)
		{
			PxTransform parentPose = parent->getCMassLocalPose().transformInv(pose);
			PxTransform childPose = PxTransform(PxIdentity);
			
			NpArticulationJoint* npArticulationJoint = PX_NEW(NpArticulationJoint)(*parent, parentPose, *npArticulationLink, childPose);
			if (!npArticulationJoint)
			{
				PX_DELETE(npArticulationLink);
	
				Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, 
				"Articulation link initialization failed due to joint creation failure: returned NULL.");
				return NULL;
			}

			npArticulationLink->setInboundJoint(*npArticulationJoint);
		}

		return npArticulationLink;
	}

	// pointers to functions above, initialized during subsystem registration
	static NpArticulation* (*sCreateArticulationFn)() = 0;
	static NpArticulationLink* (*sCreateArticulationLinkFn)(NpArticulation&, NpArticulationLink* parent, const PxTransform& pose) = 0;
}

void NpFactory::registerArticulations()
{
	sCreateArticulationFn = &::createArticulation;
	sCreateArticulationLinkFn = &::createArticulationLink;
}

PxArticulation* NpFactory::createArticulation()
{
	if(!sCreateArticulationFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Articulations not registered: returned NULL.");
		return NULL;
	}

	NpArticulation* npArticulation = (*sCreateArticulationFn)();
	if(npArticulation)
		addArticulation(npArticulation);

	return npArticulation;
}

void NpFactory::onArticulationRelease(PxArticulation* a)
{
	Ps::Mutex::ScopedLock lock(mTrackingMutex);
	mArticulationTracking.erase(a);
}


PxArticulationLink* NpFactory::createArticulationLink(NpArticulation& root, NpArticulationLink* parent, const PxTransform& pose)
{
	if(!sCreateArticulationLinkFn)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
			"Articulations not registered: returned NULL.");
		return NULL;
	}

	return (*sCreateArticulationLinkFn)(root, parent, pose);
}

/////////////////////////////////////////////////////////////////////////////// constraint

void NpFactory::addConstraint(PxConstraint* npConstraint, bool lock)
{
	addToTracking<PxConstraint>(mConstraintTracking, npConstraint, mTrackingMutex, lock);
}

PxConstraint* NpFactory::createConstraint(PxRigidActor* actor0, PxRigidActor* actor1, PxConstraintConnector& connector, const PxConstraintShaderTable& shaders, PxU32 dataSize)
{
#ifdef PX_PS3
	if(shaders.solverPrepSpuByteSize > PxConstraintShaderTable::eMAX_SOLVERPREPSPU_BYTESIZE || dataSize > PxConstraintShaderTable::eMAX_SOLVERPRPEP_DATASIZE)
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, 
			"constraint data (%d) and code (%d) sizes  and will not fit on SPU.", dataSize, shaders.solverPrepSpuByteSize);
		return NULL;
	}
#endif

	NpConstraint* npConstraint = PX_NEW(NpConstraint)(actor0, actor1, connector, shaders, dataSize);
	addConstraint(npConstraint);
	return npConstraint;
}

void NpFactory::onConstraintRelease(PxConstraint* c)
{
	Ps::Mutex::ScopedLock lock(mTrackingMutex);
	mConstraintTracking.erase(c);
}

/////////////////////////////////////////////////////////////////////////////// aggregate

// PX_AGGREGATE
void NpFactory::addAggregate(PxAggregate* npAggregate, bool lock)
{
	addToTracking<PxAggregate>(mAggregateTracking, npAggregate, mTrackingMutex, lock);
}

PxAggregate* NpFactory::createAggregate(PxU32 maxActors, bool selfCollisions)
{
	PX_CHECK_AND_RETURN_NULL(maxActors<=128, "maxActors limited to 128. createAggregate method returns NULL.");

	NpAggregate* npAggregate = PX_NEW(NpAggregate)(maxActors, selfCollisions);
	addAggregate(npAggregate);
	return npAggregate;
}

void NpFactory::onAggregateRelease(PxAggregate* a)
{
	Ps::Mutex::ScopedLock lock(mTrackingMutex);
	mAggregateTracking.erase(a);
}
//~PX_AGGREGATE


///////////////////////////////////////////////////////////////////////////////

PxMaterial* NpFactory::createMaterial(PxReal staticFriction, PxReal dynamicFriction, PxReal restitution)
{
	PX_CHECK_AND_RETURN_NULL(dynamicFriction >= 0.0f, "createMaterial: dynamicFriction must be >= 0.");
	PX_CHECK_AND_RETURN_NULL(staticFriction >= 0.0f, "createMaterial: staticFriction must be >= 0.");
	PX_CHECK_AND_RETURN_NULL(restitution >= 0.0f || restitution <= 1.0f, "createMaterial: restitution must be between 0 and 1.");
	
	Sc::MaterialData data;
	data.staticFriction = staticFriction;
	data.dynamicFriction = dynamicFriction;
	data.restitution = restitution;

	return PX_NEW(NpMaterial)(data);
}

///////////////////////////////////////////////////////////////////////////////

NpConnectorArray* NpFactory::acquireConnectorArray()
{
	Ps::MutexT<>::ScopedLock l(mConnectorArrayPoolLock);
	return mConnectorArrayPool.construct();
}

void NpFactory::releaseConnectorArray(NpConnectorArray* array)
{
	Ps::MutexT<>::ScopedLock l(mConnectorArrayPoolLock);
	mConnectorArrayPool.destroy(array);
}

///////////////////////////////////////////////////////////////////////////////

NpShape* NpFactory::createShape(const PxGeometry& geometry,
								PxShapeFlags shapeFlags,
								PxMaterial*const* materials,
								PxU16 materialCount,
								bool isExclusive)
{	
	switch(geometry.getType())
	{
		case PxGeometryType::eBOX:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxBoxGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		case PxGeometryType::eSPHERE:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxSphereGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		case PxGeometryType::eCAPSULE:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxCapsuleGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		case PxGeometryType::eCONVEXMESH:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxConvexMeshGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		case PxGeometryType::ePLANE:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxPlaneGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		case PxGeometryType::eHEIGHTFIELD:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxHeightFieldGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		case PxGeometryType::eTRIANGLEMESH:
			PX_CHECK_AND_RETURN_NULL(static_cast<const PxTriangleMeshGeometry&>(geometry).isValid(), "Supplied PxGeometry is not valid. Shape creation method returns NULL.");
			break;
		default:
			PX_ASSERT(0);
	}

	//
	// Check for invalid material table setups
	//

#ifdef PX_CHECKED
	if (!NpShape::checkMaterialSetup(geometry, "Shape creation", materials, materialCount))
		return NULL;
#endif

	Ps::InlineArray<PxU16, 4> materialIndices("NpFactory::TmpMaterialIndexBuffer");
	materialIndices.resize(materialCount);
	if (materialCount == 1)
	{
		materialIndices[0] = Ps::to16((static_cast<NpMaterial*>(materials[0]))->getHandle());
	}
	else
	{
		NpMaterial::getMaterialIndices(materials, materialIndices.begin(), materialCount);
	}

	NpShape* npShape = PX_NEW (NpShape)(geometry, shapeFlags, materialIndices.begin(), materialCount, isExclusive);
	if(!npShape)
		return NULL;

	for (PxU32 i=0; i < materialCount; i++)
		static_cast<NpMaterial*>(npShape->getMaterial(i))->incRefCount();

	addShape(npShape);

	return npShape;
}


PxU32 NpFactory::getNbShapes() const
{
	return mShapeTracking.size();
}

PxU32 NpFactory::getShapes(PxShape** userBuffer, PxU32 bufferSize, PxU32 startIndex)	const
{
	if(mShapeTracking.size()<startIndex)
		return 0;
	PxU32 count = PxMax(bufferSize, mShapeTracking.size()-startIndex);
	PxShape*const *shapes = mShapeTracking.getEntries();
	for(PxU32 i=0;i<count;i++)
		userBuffer[i] = shapes[startIndex+i];
	return count;
}


PxRigidStatic* NpFactory::createRigidStatic(const PxTransform& pose)
{
	PX_CHECK_AND_RETURN_NULL(pose.isValid(), "pose is not valid. createRigidStatic returns NULL.");

	NpRigidStatic* npActor = PX_NEW(NpRigidStatic)(pose);
	addRigidStatic(npActor);
	return npActor;
}

PxRigidDynamic* NpFactory::createRigidDynamic(const PxTransform& pose)
{
	PX_CHECK_AND_RETURN_NULL(pose.isValid(), "pose is not valid. createRigidDynamic returns NULL.");

	NpRigidDynamic* npBody = PX_NEW(NpRigidDynamic)(pose);
	addRigidDynamic(npBody);
	return npBody;
}

///////////////////////////////////////////////////////////////////////////////

// PT: this function is here to minimize the amount of locks when deserializing a collection
void NpFactory::addCollection(PxU32 nb, PxBase*const* objects)
{
	PX_ASSERT(objects);

	// PT: we take the lock only once, here
	Ps::Mutex::ScopedLock lock(mTrackingMutex);

	for(PxU32 i=0;i<nb;i++)
	{
		PxBase* s = objects[i];
		const PxType serialType = s->getConcreteType();
//////////////////////////
		if(serialType==PxConcreteType::eHEIGHTFIELD)
		{
			Gu::HeightField* np = static_cast<Gu::HeightField*>(s);
			np->setMeshFactory(this);
			addHeightField(np, false);
		}
		else if(serialType==PxConcreteType::eCONVEX_MESH)
		{
			Gu::ConvexMesh* np = static_cast<Gu::ConvexMesh*>(s);
			np->setMeshFactory(this);
			addConvexMesh(np, false);
		}
		else if(serialType==PxConcreteType::eTRIANGLE_MESH)
		{
			Gu::TriangleMesh* np = static_cast<Gu::TriangleMesh*>(s);
			np->setMeshFactory(this);
			addTriangleMesh(np, false);
		}
//////////////////////////
#if PX_USE_CLOTH_API
		else if (serialType==PxConcreteType::eCLOTH_FABRIC)
		{
			NpClothFabric* np = static_cast<NpClothFabric*>(s);
			// PT: TODO: investigate why cloth don't need a "setMeshFactory" call here
			addClothFabric(np, false);
		}
#endif
		else if(serialType==PxConcreteType::eRIGID_DYNAMIC)
		{
			NpRigidDynamic* np = static_cast<NpRigidDynamic*>(s);
			addRigidDynamic(np, false);
		}
		else if(serialType==PxConcreteType::eRIGID_STATIC)
		{
			NpRigidStatic* np = static_cast<NpRigidStatic*>(s);
			addRigidStatic(np, false);
		}
		else if(serialType==PxConcreteType::eSHAPE)
		{
			NpShape* np = static_cast<NpShape*>(s);
			addShape(np, false);
		}
		else if(serialType==PxConcreteType::eMATERIAL)
		{
		}
		else if(serialType==PxConcreteType::eCONSTRAINT)
		{
			NpConstraint* np = static_cast<NpConstraint*>(s);
			addConstraint(np, false);
		}
#if PX_USE_CLOTH_API
		else if (serialType==PxConcreteType::eCLOTH)
		{
			NpCloth* np = static_cast<NpCloth*>(s);
			addCloth(np, false);
		}
#endif
#if PX_USE_PARTICLE_SYSTEM_API
		else if(serialType==PxConcreteType::ePARTICLE_SYSTEM)
		{
			NpParticleSystem* np = static_cast<NpParticleSystem*>(s);
			addParticleSystem(np, false);
		}
		else if(serialType==PxConcreteType::ePARTICLE_FLUID)
		{
			NpParticleFluid* np = static_cast<NpParticleFluid*>(s);
			addParticleFluid(np, false);
		}
#endif
		else if(serialType==PxConcreteType::eAGGREGATE)
		{
			NpAggregate* np = static_cast<NpAggregate*>(s);
			addAggregate(np, false);

			// PT: TODO: double-check this.... is it correct?
			const PxU32 nb = np->getCurrentSizeFast();
			for(PxU32 j=0;j<nb;j++)
			{
				PxBase* actor = np->getActorFast(j);
				const PxType serialType = actor->getConcreteType();

				if(serialType==PxConcreteType::eRIGID_STATIC)
					addRigidStatic(static_cast<NpRigidStatic*>(actor), false);
				else if(serialType==PxConcreteType::eRIGID_DYNAMIC)
					addRigidDynamic(static_cast<NpRigidDynamic*>(actor), false);
				else if(serialType==PxConcreteType::ePARTICLE_SYSTEM)
				{}
				else if(serialType==PxConcreteType::ePARTICLE_FLUID)
				{}
				else if(serialType==PxConcreteType::eARTICULATION_LINK)
				{}
				else PX_ASSERT(0);
			}
		}
		else if(serialType==PxConcreteType::eARTICULATION)
		{
			NpArticulation* np = static_cast<NpArticulation*>(s);
			addArticulation(np, false);
		}
		else if(serialType==PxConcreteType::eARTICULATION_LINK)
		{
//			NpArticulationLink* np = static_cast<NpArticulationLink*>(s);
		}
		else if(serialType==PxConcreteType::eARTICULATION_JOINT)
		{
//			NpArticulationJoint* np = static_cast<NpArticulationJoint*>(s);
		}
		else
		{
//			assert(0);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

#if PX_SUPPORT_GPU_PHYSX
void NpFactory::notifyReleaseTriangleMesh(const PxTriangleMesh& tm)
{
	NpPhysics::getInstance().getNpPhysicsGpu().releaseTriangleMeshMirror(tm);
}

void NpFactory::notifyReleaseHeightField(const PxHeightField& hf)
{
	NpPhysics::getInstance().getNpPhysicsGpu().releaseHeightFieldMirror(hf);
}

void NpFactory::notifyReleaseConvexMesh(const PxConvexMesh& cm)
{
	NpPhysics::getInstance().getNpPhysicsGpu().releaseConvexMeshMirror(cm);
}
#endif

#if PX_SUPPORT_VISUAL_DEBUGGER
void NpFactory::setNpFactoryListener( NpFactoryListener& inListener)
{
	mNpFactoryListener = &inListener;
	addFactoryListener(inListener);
}
#endif
///////////////////////////////////////////////////////////////////////////////

// these calls are issued from the Scb layer when buffered deletes are issued. 
// TODO: we should really push these down as a virtual interface that is part of Scb's reqs
// to eliminate this link-time dep.


static void NpDestroyRigidActor(Scb::RigidStatic& scb)
{
	const size_t offset = size_t(&(reinterpret_cast<NpRigidStatic*>(0)->getScbActorFast()));
	NpRigidStatic* np = reinterpret_cast<NpRigidStatic*>(reinterpret_cast<char*>(&scb)-offset);
	void* ud = np->userData;
	Cm::deletePxBase(np);
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
}

static void NpDestroyRigidDynamic(Scb::Body& scb)
{
	const size_t offset = size_t(&(reinterpret_cast<NpRigidDynamic*>(0)->getScbActorFast()));
	NpRigidDynamic* np = reinterpret_cast<NpRigidDynamic*>(reinterpret_cast<char*>(&scb)-offset);
	void* ud = np->userData;
	Cm::deletePxBase(np);
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
}

#if PX_USE_PARTICLE_SYSTEM_API
static void NpDestroyParticleSystem(Scb::ParticleSystem& scb)
{
	const size_t offset = size_t(&(reinterpret_cast<NpParticleSystem*>(0)->getScbParticleSystem()));
	NpParticleSystem* np = reinterpret_cast<NpParticleSystem*>(reinterpret_cast<char*>(&scb)-offset);
	void* ud = np->userData;
	Cm::deletePxBase(np);
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
}
#endif

static void NpDestroyArticulationLink(Scb::Body& scb)
{
	const size_t offset = size_t(&(reinterpret_cast<NpArticulationLink*>(0)->getScbActorFast()));
	NpArticulationLink* np = reinterpret_cast<NpArticulationLink*>(reinterpret_cast<char*>(&scb)-offset);
	void* ud = np->userData;
	Cm::deletePxBase(np);
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
}

static void NpDestroyArticulationJoint(Scb::ArticulationJoint& scb)
{
	const size_t offset = size_t(&(reinterpret_cast<NpArticulationJoint*>(0)->getScbArticulationJoint()));
	NpArticulationJoint* np = reinterpret_cast<NpArticulationJoint*>(reinterpret_cast<char*>(&scb)-offset);
	Cm::deletePxBase(np);
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, NULL);
}

static void NpDestroyArticulation(Scb::Articulation& scb)
{
	const size_t offset = size_t(&(reinterpret_cast<NpArticulation*>(0)->getArticulation()));
	NpArticulation* np = reinterpret_cast<NpArticulation*>(reinterpret_cast<char*>(&scb)-offset);
	void* ud = np->userData;
	Cm::deletePxBase(np);
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
}

static void NpDestroyAggregate(Scb::Aggregate& scb)
{
	const size_t offset = size_t(&(reinterpret_cast<NpAggregate*>(0)->getScbAggregate()));
	NpAggregate* np = reinterpret_cast<NpAggregate*>(reinterpret_cast<char*>(&scb)-offset);
	Cm::deletePxBase(np);
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, NULL);
}

static void NpDestroyShape(Scb::Shape& scb)
{
	const size_t offset = size_t(&(reinterpret_cast<NpShape*>(0)->getScbShape()));
	NpShape* np = reinterpret_cast<NpShape*>(reinterpret_cast<char*>(&scb)-offset);
	void* ud = np->userData;
	Cm::deletePxBase(np);
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
}

static void NpDestroyConstraint(Scb::Constraint& scb)
{
	const size_t offset = size_t(&(reinterpret_cast<NpConstraint*>(0)->getScbConstraint()));
	NpConstraint* np = reinterpret_cast<NpConstraint*>(reinterpret_cast<char*>(&scb)-offset);
	Cm::deletePxBase(np);
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, NULL);
}

#if PX_USE_CLOTH_API
static void NpDestroyCloth(Scb::Cloth& scb)
{
	const size_t offset = size_t(&(reinterpret_cast<NpCloth*>(0)->getScbCloth()));
	NpCloth* np = reinterpret_cast<NpCloth*>(reinterpret_cast<char*>(&scb)-offset);
	void* ud = np->userData;
	Cm::deletePxBase(np);
	NpPhysics::getInstance().notifyDeletionListenersMemRelease(np, ud);
}
#endif

namespace physx
{
	void NpDestroy(Scb::Base& base)
	{
		switch(base.getScbType())
		{
			case ScbType::SHAPE_EXCLUSIVE:
			case ScbType::SHAPE_SHARED:					{ NpDestroyShape(static_cast<Scb::Shape&>(base));							}break;
			case ScbType::BODY:							{ NpDestroyRigidDynamic(static_cast<Scb::Body&>(base));						}break;
			case ScbType::BODY_FROM_ARTICULATION_LINK:	{ NpDestroyArticulationLink(static_cast<Scb::Body&>(base));					}break;
			case ScbType::RIGID_STATIC:					{ NpDestroyRigidActor(static_cast<Scb::RigidStatic&>(base));				}break;
			case ScbType::CONSTRAINT:					{ NpDestroyConstraint(static_cast<Scb::Constraint&>(base));					}break;
#if PX_USE_PARTICLE_SYSTEM_API
			case ScbType::PARTICLE_SYSTEM:				{ NpDestroyParticleSystem(static_cast<Scb::ParticleSystem&>(base));			}break;
#endif
			case ScbType::ARTICULATION:					{ NpDestroyArticulation(static_cast<Scb::Articulation&>(base));				}break;
			case ScbType::ARTICULATION_JOINT:			{ NpDestroyArticulationJoint(static_cast<Scb::ArticulationJoint&>(base));	}break;
			case ScbType::AGGREGATE:					{ NpDestroyAggregate(static_cast<Scb::Aggregate&>(base));					}break;
#if PX_USE_CLOTH_API
			case ScbType::CLOTH:						{ NpDestroyCloth(static_cast<Scb::Cloth&>(base));							}break;
#endif
			default:
				PX_ASSERT(!"NpDestroy: missing type!");
				break;
		}
	}
}

