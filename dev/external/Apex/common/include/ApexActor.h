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

#ifndef APEX_ACTOR_H
#define APEX_ACTOR_H

#include "ApexContext.h"
#include "ApexRenderable.h"
#include "ApexInterface.h"
#include "NiResourceProvider.h"
#include "NxApexSDK.h"

#if NX_SDK_VERSION_MAJOR == 2
#include "NxActor.h"
#elif NX_SDK_VERSION_MAJOR == 3
#include "PxActor.h"
#include "PxShape.h"
#include "PxFiltering.h"
#include "PxRigidDynamic.h"
#include "foundation/PxTransform.h"
#include "PxRigidBodyExt.h"
#endif

/*#ifdef APEX_SHIPPING
#define UNIQUE_ACTOR_ID 0
#else*/
#define UNIQUE_ACTOR_ID 1
//#endif

#if NX_SDK_VERSION_MAJOR == 2
class NxActorDescBase;
#endif

// corresponds to 20 frames for a time step of 0.02, PhysX 3.3 took PX_SLEEP_INTERVAL away
#define APEX_DEFAULT_WAKE_UP_COUNTER 0.4f

namespace physx
{
namespace apex
{

class ApexContext;
class NiApexRenderDebug;
class NiApexScene;
class NxApexAsset;
class NxApexActor;

/**
Class that implements actor interface with its context(s)
*/
class ApexActor : public ApexRenderable, public ApexContext
{
public:
	ApexActor();
	~ApexActor();

	void				addSelfToContext(ApexContext& ctx, ApexActor* actorPtr = NULL);
	void				updateIndex(ApexContext& ctx, physx::PxU32 index);
	bool				findSelfInContext(ApexContext& ctx);

	// Each class that derives from ApexActor should implement the following functions
	// if it wants ActorCreationNotification and Deletion callbacks
	virtual NxApexAsset*	getNxApexAsset(void)
	{
		return NULL;
	}
	virtual void			ContextActorCreationNotification(NxAuthObjTypeID	authorableObjectType,
	        ApexActor*		actorPtr)
	{
		PX_UNUSED(authorableObjectType);
		PX_UNUSED(actorPtr);
		return;
	}
	virtual void			ContextActorDeletionNotification(NxAuthObjTypeID	authorableObjectType,
	        ApexActor*		actorPtr)
	{
		PX_UNUSED(authorableObjectType);
		PX_UNUSED(actorPtr);
		return;
	}

	// Each class that derives from ApexActor may optionally implement these functions
	virtual NxApexRenderable* getRenderable()
	{
		return NULL;
	}
	virtual NxApexActor*      getNxApexActor()
	{
		return NULL;
	}

	virtual void        release() = 0;
	void				destroy();

#if NX_SDK_VERSION_MAJOR == 2
	// NxScene pointer may be NULL
	virtual void		setPhysXScene(NxScene* s) = 0;
	virtual NxScene*	getPhysXScene() const = 0;
#elif NX_SDK_VERSION_MAJOR == 3
	virtual void		setPhysXScene(PxScene* s) = 0;
	virtual PxScene*	getPhysXScene() const = 0;
#endif

	enum ActorState
	{
		StateEnabled,
		StateDisabled,
		StateEnabling,
		StateDisabling,
	};

	void visualizeLodBenefit(NiApexRenderDebug& renderDebug, NiApexScene& apexScene, const PxVec3& centroid, PxF32 radius, PxF32 absoluteBenefit, ActorState state);

protected:
	bool				mInRelease;

	struct ContextTrack
	{
		physx::PxU32	index;
		ApexContext*	ctx;
	};
	physx::Array<ContextTrack> mContexts;

#if UNIQUE_ACTOR_ID
	static PxI32 mUniqueActorIdCounter;
	PxI32 mUniqueActorId;
#endif

	friend class ApexContext;
};


#if NX_SDK_VERSION_MAJOR == 2

/**
Class that manages creation templates.
*/
class ApexActorSource
{
public:

	// NxApexActorSource methods

	void setActorTemplate(const NxActorDescBase* desc)
	{
		actorTemplate.set(static_cast<const ActorTemplate*>(desc));
	}
	bool getActorTemplate(NxActorDescBase& dest) const
	{
		return actorTemplate.get(static_cast<ActorTemplate&>(dest));
	}
	void modifyActor(NxActor* actor) const
	{
		if (actorTemplate.isSet)
		{
			actorTemplate.data.apply(actor);
		}
	}

	void setShapeTemplate(const NxShapeDesc* desc)
	{
		shapeTemplate.set(static_cast<const ShapeTemplate*>(desc));
	}
	bool getShapeTemplate(NxShapeDesc& dest) const
	{
		return shapeTemplate.get(static_cast<ShapeTemplate&>(dest));
	}
	void modifyShape(NxShape* shape) const
	{
		if (shapeTemplate.isSet)
		{
			shapeTemplate.data.apply(shape);
		}
	}

	void setBodyTemplate(const NxBodyDesc* desc)
	{
		bodyTemplate.set(static_cast<const BodyTemplate*>(desc));
	}
	bool getBodyTemplate(NxBodyDesc& dest)	const
	{
		return bodyTemplate.get(static_cast<BodyTemplate&>(dest));
	}
	void modifyBody(NxActor* actor) const
	{
		if (bodyTemplate.isSet)
		{
			bodyTemplate.data.apply(actor);
		}
	}


protected:

	class ShapeTemplate : public NxShapeDesc
	{
		//gotta make a special derived class cause of non-void ctor
	public:
		ShapeTemplate() : NxShapeDesc(NX_SHAPE_PLANE) {}	//type is trash, we have to supply something.
		ShapeTemplate& operator=(const ShapeTemplate& src)
		{
			//copy over the variables that Shape templates transfer: (memcopy is a bad idea because it will clobber types, etc.)

			shapeFlags = src.shapeFlags;
			group = src.group;
			materialIndex = src.materialIndex;
			density = src.density;	//do we want this!?
			skinWidth = src.skinWidth;
			userData = src.userData;
			name = src.name;
			groupsMask = src.groupsMask;
#if (NX_SDK_VERSION_NUMBER >= 280) && (NX_SDK_VERSION_NUMBER < 290)
			nonInteractingCompartmentTypes = src.nonInteractingCompartmentTypes;
#endif
			return *this;
		}

		void apply(NxShape* shape) const
		{
#define SETFLAG(_FLAG) shape->setFlag(_FLAG, (shapeFlags & _FLAG) != 0);
			SETFLAG(NX_TRIGGER_ON_ENTER);
			SETFLAG(NX_TRIGGER_ON_LEAVE);
			SETFLAG(NX_TRIGGER_ON_STAY);
			SETFLAG(NX_SF_VISUALIZATION);
			SETFLAG(NX_SF_DISABLE_COLLISION);
			SETFLAG(NX_SF_FEATURE_INDICES);
			SETFLAG(NX_SF_DISABLE_RAYCASTING);
			SETFLAG(NX_SF_POINT_CONTACT_FORCE);
			SETFLAG(NX_SF_FLUID_DRAIN);
			SETFLAG(NX_SF_FLUID_DISABLE_COLLISION);
			SETFLAG(NX_SF_FLUID_TWOWAY);
			SETFLAG(NX_SF_DISABLE_RESPONSE);
			SETFLAG(NX_SF_DYNAMIC_DYNAMIC_CCD);
			SETFLAG(NX_SF_DISABLE_SCENE_QUERIES);
			SETFLAG(NX_SF_CLOTH_DRAIN);
			SETFLAG(NX_SF_CLOTH_DISABLE_COLLISION);
			SETFLAG(NX_SF_CLOTH_TWOWAY);
			SETFLAG(NX_SF_SOFTBODY_DRAIN);
			SETFLAG(NX_SF_SOFTBODY_DISABLE_COLLISION);
			SETFLAG(NX_SF_SOFTBODY_TWOWAY);
#undef SETFLAG

			shape->setGroup(group);
			shape->setMaterial(materialIndex);
			shape->setSkinWidth(skinWidth);
			shape->userData = userData;
			shape->setName(name);
			shape->setGroupsMask(groupsMask);
#if (NX_SDK_VERSION_NUMBER >= 280) && (NX_SDK_VERSION_NUMBER < 290)
			//shape->setNonInteractingCompartmentTypes(nonInteractingCompartmentTypes); // this crashes the mirror manager in the clothing tool, nvbug 552007
#endif
		}

	};

	class ActorTemplate : public NxActorDescBase
	{
	public:
		ActorTemplate()
		{
			setToDefault();	//this is not done by default for NxActorDescBase!
		}

		ActorTemplate& operator=(const ActorTemplate& src)
		{
			//copy over the variables that Actor templates transfer: (memcopy is a bad idea because it will clobber types, etc.)

			density = src.density;
			flags = src.flags;
			group = src.group;
			dominanceGroup = src.dominanceGroup;
#if (NX_SDK_VERSION_NUMBER >= 280)
			contactReportFlags = src.contactReportFlags;
			forceFieldMaterial = src.forceFieldMaterial;
#endif
			userData = src.userData;
			name = src.name;
			compartment = src.compartment;

			return *this;
		}

		void apply(NxActor* actor) const
		{
			PX_ASSERT(actor != NULL);

#define SETFLAG(_FLAG) if (flags & _FLAG) actor->raiseActorFlag(_FLAG); else actor->clearActorFlag(_FLAG);
			SETFLAG(NX_AF_DISABLE_RESPONSE);
			SETFLAG(NX_AF_CONTACT_MODIFICATION);
			SETFLAG(NX_AF_FORCE_CONE_FRICTION);
			SETFLAG(NX_AF_USER_ACTOR_PAIR_FILTERING);
			SETFLAG(NX_AF_DISABLE_COLLISION);
#undef SETFLAG

			actor->setGroup(group);
			actor->setDominanceGroup(dominanceGroup);
#if (NX_SDK_VERSION_NUMBER >= 280)
			actor->setContactReportFlags(contactReportFlags);
			actor->setForceFieldMaterial(forceFieldMaterial);
#endif
			actor->userData = userData;
			actor->setName(name);
		}
	};

	class BodyTemplate : public NxBodyDesc
	{
	public:
		BodyTemplate& operator=(const BodyTemplate& src)
		{
			//copy over the variables that Body templates transfer: (memcopy is a bad idea because it will clobber types, etc.)

			wakeUpCounter = src.wakeUpCounter;
			linearDamping = src.linearDamping;
			angularDamping = src.angularDamping;
			maxAngularVelocity = src.maxAngularVelocity;
			CCDMotionThreshold = src.CCDMotionThreshold;
			flags = src.flags;
			sleepLinearVelocity = src.sleepLinearVelocity;
			sleepAngularVelocity = src.sleepAngularVelocity;
			solverIterationCount = src.solverIterationCount;
			sleepEnergyThreshold = src.sleepEnergyThreshold;
			sleepDamping = src.sleepDamping;
#if (NX_SDK_VERSION_NUMBER >= 280)
			contactReportThreshold = src.contactReportThreshold;
#endif

			return *this;
		}

		void apply(NxActor* actor) const
		{
			if (actor->isDynamic())
			{
#define SETFLAG(_FLAG) if (flags & _FLAG) actor->raiseBodyFlag(_FLAG); else actor->clearBodyFlag(_FLAG);
				SETFLAG(NX_BF_DISABLE_GRAVITY);
				SETFLAG(NX_BF_FILTER_SLEEP_VEL);
				SETFLAG(NX_BF_ENERGY_SLEEP_TEST);
#undef SETFLAG

				actor->wakeUp(wakeUpCounter);
				actor->setLinearDamping(linearDamping);
				actor->setAngularDamping(angularDamping);
				actor->setMaxAngularVelocity(maxAngularVelocity);
				actor->setCCDMotionThreshold(CCDMotionThreshold);
				actor->setSleepLinearVelocity(sleepLinearVelocity);
				actor->setSleepAngularVelocity(sleepAngularVelocity);
				actor->setSolverIterationCount(solverIterationCount);
				actor->setSleepEnergyThreshold(sleepEnergyThreshold);
				// actor->setSleepDamping(sleepDamping); // interface does not exist
#if (NX_SDK_VERSION_NUMBER >= 280)
				//actor->setContactReportThreshold(contactReportThreshold); // this crashes the mirror manager in the clothing tool, nvbug 552022
#endif
			}
		}
	};


	InitTemplate<ActorTemplate> actorTemplate;
	InitTemplate<ShapeTemplate> shapeTemplate;
	InitTemplate<BodyTemplate> bodyTemplate;
};
#elif NX_SDK_VERSION_MAJOR == 3

// template for PhysX3.0 actor, body and shape.
class PhysX3DescTemplate
{
public:
	PhysX3DescTemplate()
	{
		SetToDefault();
	}
	void apply(PxActor* actor) const
	{
		actor->setActorFlags((PxActorFlags)actorFlags);
		actor->setDominanceGroup(dominanceGroup);
		actor->setOwnerClient(ownerClient);
		PX_ASSERT(clientBehaviorBits < PX_MAX_U8);
		actor->setClientBehaviorFlags(PxActorClientBehaviorFlags((PxU8)clientBehaviorBits));
		//actor->contactReportFlags;	// must be set via call NxApexPhysX3Interface::setContactReportFlags
		actor->userData	= userData;
		if (name)
		{
			actor->setName(name);
		}

		// body
		PxRigidBody*	rb	= actor->isRigidBody();
		if (rb)
		{
			// density, user should call updateMassAndInertia when shapes are created.
		}

		PxRigidDynamic*	rd	= actor->isRigidDynamic();
		if (rd)
		{
			rd->setRigidDynamicFlags(PxRigidDynamicFlags(bodyFlags));
			rd->setWakeCounter(wakeUpCounter);
			rd->setLinearDamping(linearDamping);
			rd->setAngularDamping(angularDamping);
			rd->setMaxAngularVelocity(maxAngularVelocity);
			// sleepLinearVelocity	attribute for deformable/cloth, see below.
			rd->setSolverIterationCounts(solverIterationCount);
			rd->setContactReportThreshold(contactReportThreshold);
			rd->setSleepThreshold(sleepThreshold);
		}
	}
	void apply(PxShape* shape) const
	{
		shape->setFlags((PxShapeFlags)shapeFlags);
		shape->setMaterials(materials.begin(), static_cast<physx::PxU16>(materials.size()));
		shape->userData	= shapeUserData;
		if (shapeName)
		{
			shape->setName(shapeName);
		}
		shape->setSimulationFilterData(simulationFilterData);
		shape->setQueryFilterData(queryFilterData);
		shape->setContactOffset(contactOffset);
		shape->setRestOffset(restOffset);
	}

	bool isValid()
	{
		if (density < 0)
		{
			return false;
		}
		if (wakeUpCounter < 0.0f) //must be nonnegative
		{
			return false;
		}
		if (linearDamping < 0.0f) //must be nonnegative
		{
			return false;
		}
		if (angularDamping < 0.0f) //must be nonnegative
		{
			return false;
		}
		if (solverIterationCount < 1) //must be positive
		{
			return false;
		}
		if (solverIterationCount > 255)
		{
			return false;
		}
		if (contactReportThreshold < 0.0f) //must be nonnegative
		{
			return false;
		}
		if (materials.size() == 0)
		{
			return false;
		}

		return true;
	}

	void SetToDefault()
	{
		// actor
		dominanceGroup			= 0;
		actorFlags				= PxActorFlag::eSEND_SLEEP_NOTIFIES;
		ownerClient				= 0;
		clientBehaviorBits		= 0;
		contactReportFlags		= 0;
		userData				= 0;
		name					= 0;

		// body
		density					= 1.0f;

		bodyFlags				= 0;
		wakeUpCounter			= APEX_DEFAULT_WAKE_UP_COUNTER;
		linearDamping			= 0.0f;
		angularDamping			= 0.05f;
		maxAngularVelocity		= 7.0f;		// eRIGID_DYNAMIC
		sleepLinearVelocity		= 0.0f;
		solverIterationCount	= 4;
		contactReportThreshold	= PX_MAX_F32;
		sleepThreshold			= 0.005f;

		// shape
		shapeFlags				= PxShapeFlag::eSIMULATION_SHAPE
		                          | PxShapeFlag::eSCENE_QUERY_SHAPE
		                          | PxShapeFlag::eVISUALIZATION;
		//materials
		shapeName				= 0;
		simulationFilterData	= PxFilterData(0, 0, 0, 0);
		queryFilterData			= PxFilterData(0, 0, 0, 0);
		contactOffset			= -1.0f;
		restOffset				= PX_MAX_F32;
	}
public:
	// actor
	PxDominanceGroup	dominanceGroup;
	PxU16 				actorFlags;
	PxClientID			ownerClient;
	PxU32				clientBehaviorBits;
	PxU16				contactReportFlags;
	void* 				userData;
	const char* 		name;

	// body
	PxReal				density;

	PxU16				bodyFlags;
	PxReal				wakeUpCounter;
	PxReal				linearDamping;
	PxReal				angularDamping;
	PxReal				maxAngularVelocity;
	PxReal				sleepLinearVelocity;
	PxU32				solverIterationCount;
	PxReal				contactReportThreshold;
	PxReal				sleepThreshold;

	// shape
	PxU8				shapeFlags;
	Array<PxMaterial*>	materials;
	void*				shapeUserData;
	const char*			shapeName;
	PxFilterData		simulationFilterData;
	PxFilterData		queryFilterData;
	PxReal				contactOffset;
	PxReal				restOffset;
};	// PhysX3DescTemplate

class ApexActorSource
{
public:

	// NxApexActorSource methods

	void setPhysX3Template(const PhysX3DescTemplate* desc)
	{
		physX3Template.set(desc);
	}
	bool getPhysX3Template(PhysX3DescTemplate& dest) const
	{
		return physX3Template.get(dest);
	}
	void modifyActor(PxRigidActor* actor) const
	{
		if (physX3Template.isSet)
		{
			physX3Template.data.apply(actor);
		}
	}
	void modifyShape(PxShape* shape) const
	{
		if (physX3Template.isSet)
		{
			physX3Template.data.apply(shape);
		}
	}



protected:

	InitTemplate<PhysX3DescTemplate> physX3Template;
};

#endif // NX_SDK_VERSION_MAJOR == 2

}
} // end namespace physx::apex

#endif // __APEX_ACTOR_H__
