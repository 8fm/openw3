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


#include "ScBodySim.h"
#include "ScStaticSim.h"
#include "ScScene.h"
#include "ScRbElementInteraction.h"
#include "ScParticleBodyInteraction.h"
#include "ScShapeInstancePairLL.h"
#include "ScTriggerInteraction.h"
#include "ScObjectIDTracker.h"
#include "GuHeightFieldUtil.h"
#include "GuDebug.h"
#include "GuTriangleMesh.h"
#include "GuConvexMeshData.h"
#include "GuTriangleMeshData.h"
#include "GuHeightField.h"
#include "PxsContext.h"
#include "PxsAABBManager.h"
#include "PxsTransformCache.h"
#include "CmTransformUtils.h"

using namespace physx;

// PT: keep local functions in cpp, no need to pollute the header. Don't force conversions to bool if not necessary.
static PX_FORCE_INLINE Ps::IntBool hasTriggerFlags(PxShapeFlags flags)	{ return flags & PxShapeFlag::eTRIGGER_SHAPE ? 1 : 0;									}
static PX_FORCE_INLINE Ps::IntBool isBroadPhase(PxShapeFlags flags)		{ return flags & (PxShapeFlag::eTRIGGER_SHAPE|PxShapeFlag::eSIMULATION_SHAPE) ? 1 : 0;	}


Sc::ShapeSim::ShapeSim(RigidSim& owner, const ShapeCore& core, PxsRigidBody* atom, PxBounds3* outBounds)	:
	ElementSim			(owner, PX_ELEMENT_TYPE_SHAPE),
	mTransformCacheId	(PX_INVALID_U32),
	mCore				(core)
{
	// sizeof(ShapeSim) = 32 bytes
	Sc::Scene& scScene = getScene();

	PX_ASSERT(&owner);
	{
		CM_PROFILE_ZONE_WITH_SUBSYSTEM(scScene, SimAPI, simAddShapeToBroadPhase);
		if(isBroadPhase(core.getFlags()))
		{
			PX_ASSERT(PX_INVALID_BP_HANDLE==getAABBMgrId().mHandle);
			PxBounds3 bounds = computeWorldBounds(core, owner);
			scScene.addBroadPhaseVolume(bounds, owner.getBroadphaseGroupId(), atom ? atom->getAABBMgrId() : AABBMgrId(), *this);

			if(atom)
				atom->setAABBMgrId(getAABBMgrId());

			if(outBounds)
			{
				outBounds->minimum = bounds.minimum + PxVec3(core.getContactOffset());
				outBounds->maximum = bounds.maximum - PxVec3(core.getContactOffset());
			}
		}
	}

	mId = scScene.getShapeIDTracker().createID();
}

Sc::ShapeSim::~ShapeSim()
{
	PX_ASSERT((!hasAABBMgrHandle()) || (PX_INVALID_BP_HANDLE==getAABBMgrId().mHandle));

	Sc::Scene& scScene = getScene();
	scScene.getShapeIDTracker().releaseID(mId);
}

void Sc::ShapeSim::createTransformCache(PxsTransformCache& cache)
{
	if(mTransformCacheId == PX_INVALID_U32)
	{
		//Create transform cache entry
		PxU32 index = cache.createID();
		cache.setTransformCache(getAbsPose(), index);
		mTransformCacheId = index;
	}
	cache.incReferenceCount(mTransformCacheId);
}

void Sc::ShapeSim::destroyTransformCache(PxsTransformCache& cache)
{
	PX_ASSERT(mTransformCacheId != PX_INVALID_U32);
	if(cache.decReferenceCount(mTransformCacheId))
	{
		cache.releaseID(mTransformCacheId);
		mTransformCacheId = PX_INVALID_U32;
	}
}


void Sc::ShapeSim::createLowLevelVolume(const PxU32 group, const PxBounds3& bounds, const PxU32 compoundID, AABBMgrId aabbMgrId)
{
	//Add the volume to the aabb manager.

	if(!Element::createLowLevelVolume(group, bounds, compoundID, aabbMgrId))
		return;

	const PxsShapeCore& shapeCore = getCore().getCore();
	const Gu::GeometryUnion &geometry = shapeCore.geometry;
	const PxBounds3* localSpaceAABB = NULL;
	switch (geometry.getType())
	{
	case PxGeometryType::eCONVEXMESH:
		localSpaceAABB = &geometry.get<const PxConvexMeshGeometryLL>().hullData->mAABB;
		break;
	case PxGeometryType::eTRIANGLEMESH:
		localSpaceAABB = &geometry.get<const PxTriangleMeshGeometryLL>().meshData->mAABB;
		break;
	case PxGeometryType::eHEIGHTFIELD:
		localSpaceAABB = &geometry.get<const PxHeightFieldGeometryLL>().heightFieldData->mAABB;											
		break;
	default:
		break;
	}

	Sc::Actor& actor = getScActor();
	PxsContext* llContext = getInteractionScene().getLowLevelContext();
	if(actor.isDynamicRigid())
	{
		PxcAABBDataDynamic aabbData;
		aabbData.mShapeCore = &shapeCore;
		aabbData.mLocalSpaceAABB = localSpaceAABB;
		aabbData.mRigidCore = &static_cast<BodySim&>(actor).getBodyCore().getCore();
		aabbData.mBodyAtom = &static_cast<BodySim*>(&actor)->getLowLevelBody();
		llContext->getAABBManager()->setDynamicAABBData(getAABBMgrId().mHandle, aabbData);
	}
	else
	{
		PxcAABBDataStatic aabbData;
		aabbData.mShapeCore = &shapeCore;
		aabbData.mRigidCore = &static_cast<StaticSim&>(actor).getStaticCore().getCore();
		llContext->getAABBManager()->setStaticAABBData(getAABBMgrId().mHandle, aabbData);
	}

	llContext->markShape(getAABBMgrId().mSingleOrCompoundId);
}

bool Sc::ShapeSim::destroyLowLevelVolume()
{
	//Need to test that shape has entry in bp.  The shape might not have a bp
	//entry because it has its simulation flag set to false.
	const PxcBpHandle singleOrCompoundId = getAABBMgrId().mSingleOrCompoundId;
	if(PX_INVALID_BP_HANDLE!=singleOrCompoundId)
	{
		getInteractionScene().getLowLevelContext()->unMarkShape(singleOrCompoundId);
	}
	bool removingLastShape=Element::destroyLowLevelVolume();
	if(removingLastShape)
	{
		BodySim* b = getBodySim();
		if(b)
			b->getLowLevelBody().resetAABBMgrId();
	}
	return removingLastShape;
}

Sc::Scene& Sc::ShapeSim::getScene() const
{
	return getInteractionScene().getOwnerScene();
}

PX_FORCE_INLINE void Sc::ShapeSim::internalAddToBroadPhase()
{
	getScene().addBroadPhaseVolume(computeWorldBounds(getCore(), getRbSim()), *this);
}

PX_FORCE_INLINE void Sc::ShapeSim::internalRemoveFromBroadPhase()
{
	getScene().removeBroadPhaseVolume(true, *this);
}

void Sc::ShapeSim::removeFromBroadPhase(bool wakeOnLostTouch)
{
	if(hasAABBMgrHandle())
	{
		getScene().removeBroadPhaseVolume(wakeOnLostTouch, *this);
		PX_ASSERT(PX_INVALID_BP_HANDLE==getAABBMgrId().mHandle);
	}
}

void Sc::ShapeSim::reinsertBroadPhase()
{
	internalRemoveFromBroadPhase();
	internalAddToBroadPhase();
}

void Sc::ShapeSim::onFilterDataChange()
{
	setElementInteractionsDirty(CoreInteraction::CIF_DIRTY_FILTER_STATE, PX_INTERACTION_FLAG_FILTERABLE);
}

void Sc::ShapeSim::onResetFiltering()
{
	if(hasAABBMgrHandle())
		internalRemoveFromBroadPhase();

	if(isBroadPhase(mCore.getFlags()))
	{
		internalAddToBroadPhase();
		PX_ASSERT(getAABBMgrId().mHandle!=PX_INVALID_BP_HANDLE);
		if(getBodySim())
			getBodySim()->getLowLevelBody().setAABBMgrId(getAABBMgrId());
	}
}

void Sc::ShapeSim::onMaterialChange()
{
	setElementInteractionsDirty(CoreInteraction::CIF_DIRTY_MATERIAL, PX_INTERACTION_FLAG_RB_ELEMENT);
}

void Sc::ShapeSim::onRestOffsetChange()
{
	setElementInteractionsDirty(CoreInteraction::CIF_DIRTY_REST_OFFSET, PX_INTERACTION_FLAG_RB_ELEMENT);
}

void Sc::ShapeSim::onFlagChange(PxShapeFlags oldFlags)
{
	PxShapeFlags newFlags = mCore.getFlags();

	if (hasTriggerFlags(newFlags) != hasTriggerFlags(oldFlags))
	{
		setElementInteractionsDirty(CoreInteraction::CIF_DIRTY_FILTER_STATE, PX_INTERACTION_FLAG_FILTERABLE);
	}

	// Change of collision shape flag requires removal/add to broadphase
	const Ps::IntBool oldBp = isBroadPhase(oldFlags);
	const Ps::IntBool newBp = isBroadPhase(newFlags);

	if (!oldBp && newBp)
	{
		PX_ASSERT(!hasAABBMgrHandle());
		internalAddToBroadPhase();
		if(getBodySim())
			getBodySim()->getLowLevelBody().setAABBMgrId(getAABBMgrId());
	}
	else if (oldBp && !newBp)
	{
		internalRemoveFromBroadPhase();
	}
}

void Sc::ShapeSim::getFilterInfo(PxFilterObjectAttributes& filterAttr, PxFilterData& filterData) const
{
	filterAttr = 0;
	const PxShapeFlags flags = mCore.getFlags();

	if (hasTriggerFlags(flags))
		filterAttr |= PxFilterObjectFlag::eTRIGGER;

	BodySim* b = getBodySim();
	if (b)
	{
		if (!b->isArticulationLink())
		{
			if (b->isKinematic())
				filterAttr |= PxFilterObjectFlag::eKINEMATIC;

			setFilterObjectAttributeType(filterAttr, PxFilterObjectType::eRIGID_DYNAMIC);
		}
		else
			setFilterObjectAttributeType(filterAttr, PxFilterObjectType::eARTICULATION);
	}
	else
	{
		setFilterObjectAttributeType(filterAttr, PxFilterObjectType::eRIGID_STATIC);
	}

	filterData = mCore.getSimulationFilterData();
}

PxTransform Sc::ShapeSim::getAbsPose() const
{
	const PxTransform& shape2Actor = getCore().getCore().transform;
	if(getActorSim().getActorType()==PxActorType::eRIGID_STATIC)
	{
		PxsRigidCore& core = static_cast<StaticSim&>(getScActor()).getStaticCore().getCore();
		return core.body2World.transform(shape2Actor);
	}
	else
	{
		PxsBodyCore& core = static_cast<BodySim&>(getScActor()).getBodyCore().getCore();
		return core.body2World.transform(core.body2Actor.getInverse()).transform(shape2Actor);
	}
		       
}

Sc::RigidSim& Sc::ShapeSim::getRbSim() const	
{ 
	return static_cast<RigidSim&>(getScActor());
}

Sc::BodySim* Sc::ShapeSim::getBodySim() const	
{ 
	Actor& a = getScActor();
	return a.isDynamicRigid() ? static_cast<BodySim*>(&a) : 0;
}

PxsRigidCore& Sc::ShapeSim::getPxsRigidCore() const
{
	Actor& a = getScActor();
	return a.isDynamicRigid() ? static_cast<BodySim&>(a).getBodyCore().getCore()
							  : static_cast<StaticSim&>(a).getStaticCore().getCore();
}

bool Sc::ShapeSim::actorIsDynamic() const
{
	return getScActor().isDynamicRigid();
}

PxBounds3 Sc::ShapeSim::computeWorldBounds(const ShapeCore& core, const RigidSim& actor) const
{
	const PxTransform& shape2Actor = core.getShape2Actor();
	PX_ALIGN(16, PxTransform) globalPose;
	
	if(actor.isDynamicRigid())
	{
		const PxsBodyCore& bodyCore = static_cast<const BodySim&>(actor).getBodyCore().getCore();
		Cm::getDynamicGlobalPoseAligned(bodyCore.body2World, shape2Actor, bodyCore.body2Actor, globalPose);
	}
	else
		Cm::getStaticGlobalPoseAligned(static_cast<const StaticSim&>(actor).getStaticCore().getCore().body2World, shape2Actor, globalPose);

	PxBounds3 bounds;
	core.getGeometryUnion().computeBounds(bounds, globalPose, core.getContactOffset(), NULL);
	return bounds;
}

void Sc::ShapeSim::onTransformChange()
{
	InteractionScene& scene = getInteractionScene();
	
	const AABBMgrId aabbMgrId=getAABBMgrId();
	if(PX_INVALID_BP_HANDLE!=aabbMgrId.mSingleOrCompoundId)
		scene.getLowLevelContext()->markShape(aabbMgrId.mSingleOrCompoundId);

	Element::ElementInteractionIterator iter = getElemInteractions();
	ElementInteraction* i = iter.getNext();
	while(i)
	{
		if(i->getType()==PX_INTERACTION_TYPE_OVERLAP)
			(static_cast<Sc::ShapeInstancePairLL *>(i))->resetManagerCachedState();
		else if (i->getType()==PX_INTERACTION_TYPE_TRIGGER)
			(static_cast<Sc::TriggerInteraction*>(i))->forceProcessingThisFrame(scene);  // trigger pairs need to be checked next frame
#if PX_USE_PARTICLE_SYSTEM_API
		else if (i->getType()==PX_INTERACTION_TYPE_PARTICLE_BODY)
			(static_cast<Sc::ParticleElementRbElementInteraction *>(i))->onRbShapeChange();
#endif

		i = iter.getNext();
	}
	
	getInteractionScene().getLowLevelContext()->onShapeChange(mCore.getCore(), getPxsRigidCore(), actorIsDynamic());
}

void Sc::ShapeSim::onGeometryChange()
{
	const AABBMgrId aabbMgrId=getAABBMgrId();
	if(PX_INVALID_BP_HANDLE!=aabbMgrId.mSingleOrCompoundId)
		getInteractionScene().getLowLevelContext()->markShape(aabbMgrId.mSingleOrCompoundId);

	Element::ElementInteractionIterator iter = getElemInteractions();
	ElementInteraction* i = iter.getNext();
	while(i)
	{
#if PX_USE_PARTICLE_SYSTEM_API
		if (i->getType()==PX_INTERACTION_TYPE_PARTICLE_BODY)
			(static_cast<Sc::ParticleElementRbElementInteraction *>(i))->onRbShapeChange();
		else
#endif
		if (i->getType()==PX_INTERACTION_TYPE_OVERLAP)
			(static_cast<Sc::ShapeInstancePairLL *>(i))->resetManagerCachedState();

		i = iter.getNext();
	}

	getInteractionScene().getLowLevelContext()->onShapeChange(mCore.getCore(), getPxsRigidCore(), actorIsDynamic());
}

