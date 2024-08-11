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

          
#ifndef PX_COLLISION_SHAPEINSTANCEPAIR_LL
#define PX_COLLISION_SHAPEINSTANCEPAIR_LL

#include "ScRbElementInteraction.h"
#include "ScActorPair.h"
#include "ScScene.h"
#include "ScBodySim.h"
#include "PxsContactManager.h"
#include "PxsContext.h"

#define INVALID_REPORT_PAIR_ID	0xffffffff

namespace physx
{
namespace Sc
{
	/*
	Description: A shape instance pair represents a pair of objects which _may_ have contacts. Created by the broadphase
	and processed by the NPhaseCore.
	*/
	class ShapeInstancePairLL : public RbElementInteraction
	{
		friend class NPhaseCore;

	public:
		enum SipFlag
		{
			PAIR_FLAGS_MASK					= (0x7ff),	// Bits where the PxPairFlags get stored
			NEXT_FREE						= ((PAIR_FLAGS_MASK << 1) & ~PAIR_FLAGS_MASK),

			HAVE_CONTACTS_THIS_FRAME		= (NEXT_FREE << 0),

			CONTACTS_COLLECT_POINTS			= (NEXT_FREE << 1),		// The user wants to get the contact points (includes debug rendering)
			CONTACTS_RESPONSE_DISABLED		= (NEXT_FREE << 2),		// Collision response disabled

			CONTACT_FORCE_THRESHOLD_PAIRS	= (PxU32)PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND | (PxU32)PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS | (PxU32)PxPairFlag::eNOTIFY_THRESHOLD_FORCE_LOST,
			CONTACT_REPORT_EVENTS			= (PxU32)PxPairFlag::eNOTIFY_TOUCH_FOUND | (PxU32)PxPairFlag::eNOTIFY_TOUCH_PERSISTS | (PxU32)PxPairFlag::eNOTIFY_TOUCH_LOST |
												(PxU32)PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND | (PxU32)PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS | (PxU32)PxPairFlag::eNOTIFY_THRESHOLD_FORCE_LOST,

			FORCE_THRESHOLD_EXCEEDED_NOW	= (NEXT_FREE << 3),
			FORCE_THRESHOLD_EXCEEDED_BEFORE	= (NEXT_FREE << 4),
			FORCE_THRESHOLD_EXCEEDED_FLAGS	= FORCE_THRESHOLD_EXCEEDED_NOW | FORCE_THRESHOLD_EXCEEDED_BEFORE,

			IS_IN_PERSISTENT_EVENT_LIST		= (NEXT_FREE << 5), // The pair is in the list of persistent contact events
			WAS_IN_PERSISTENT_EVENT_LIST	= (NEXT_FREE << 6), // The pair is inactive but used to be in the list of persistent contact events
			IN_PERSISTENT_EVENT_LIST		= IS_IN_PERSISTENT_EVENT_LIST | WAS_IN_PERSISTENT_EVENT_LIST,
			IS_IN_FORCE_THRESHOLD_EVENT_LIST= (NEXT_FREE << 7), // The pair is in the list of force threshold contact events
			IS_IN_CONTACT_EVENT_LIST		= IS_IN_PERSISTENT_EVENT_LIST | IS_IN_FORCE_THRESHOLD_EVENT_LIST,

			SHAPE0_IS_KINEMATIC				= (NEXT_FREE << 8),	// Cached for performance reasons
			SHAPE1_IS_KINEMATIC				= (NEXT_FREE << 9),	// Cached for performance reasons

			LL_MANAGER_RECREATE_EVENT		= CONTACT_REPORT_EVENTS | CONTACTS_COLLECT_POINTS |
											  CONTACTS_RESPONSE_DISABLED | (PxU32)PxPairFlag::eMODIFY_CONTACTS,
			LL_MANAGER_HAS_TOUCH			= (NEXT_FREE << 10),

			FACE_INDEX_REPORT_PAIR			= (NEXT_FREE << 11),  // One shape is a mesh/heightfield shape and face indices are reported

			ACTIVE_MANAGER_NOT_ALLOWED		= (NEXT_FREE << 12),  // the active manager has not been allowed
		};
												ShapeInstancePairLL(ShapeSim& s1, ShapeSim& s2, ActorPair& aPair, PxPairFlags pairFlags);
		virtual									~ShapeInstancePairLL()	
		{ 
			PX_ASSERT(!mLLIslandHook.isManaged()); 
		}

		// Submits to contact stream
						void					processUserNotification(PxU32 contactEvent, PxU16 infoFlags, bool shapeDeleted);

						void					visualize(Cm::RenderOutput&);

						PxU32					getContactPointData(const void*& contactData, PxU32& contactDataSize, const PxReal*& impulses);

						bool					managerLostTouch();
						void					managerNewTouch();
		PX_FORCE_INLINE	void					sendLostTouchReport(bool shapeDeleted);
		PX_FORCE_INLINE	void					resetManagerCachedState()	const	{ if (mManager)	mManager->resetCachedState();		}
		PX_FORCE_INLINE	ActorPair*				getActorPair()				const	{ return &mActorPair;								}
		PX_INLINE		Ps::IntBool				isReportPair()				const	{ return getPairFlags() & CONTACT_REPORT_EVENTS;	}
		PX_INLINE		Ps::IntBool				thisFrameHaveContacts()		const	{ return readIntFlag(HAVE_CONTACTS_THIS_FRAME);		}
		PX_INLINE		Ps::IntBool				thisFrameHaveCCDContacts()	const	{ PX_ASSERT(mManager); return mManager->getHadCCDContact(); }
		PX_INLINE		void					swapAndClearForceThresholdExceeded();

		PX_FORCE_INLINE void					raiseFlag(SipFlag flag)				{ mFlags |= flag; }
		PX_FORCE_INLINE	Ps::IntBool				readIntFlag(SipFlag flag)	const	{ return mFlags & flag; }
		PX_FORCE_INLINE	PxU32					getPairFlags() const;

		PX_FORCE_INLINE	void					removeFromReportPairList();

	private:
						PxU32					mContactReportStamp;
						PxU32					mFlags;
						ActorPair&				mActorPair;
						PxU32					mReportPairIndex;			// Owned by NPhaseCore for its report pair list

						PxsContactManager*		mManager;

						PxsIslandManagerEdgeHook mLLIslandHook;

						PxU16					mReportStreamIndex;  // position of this pair in the contact report stream

		// Internal functions:

						void					createManager();
		PX_INLINE		void					resetManager();
		PX_INLINE		bool					updateManager();
		PX_INLINE		void					destroyManager();
		PX_FORCE_INLINE	bool					activeManagerAllowed();
		PX_FORCE_INLINE	PxU32					getManagerContactState()		const	{ return mFlags & LL_MANAGER_RECREATE_EVENT; }

		PX_FORCE_INLINE void					clearFlag(SipFlag flag)					{ mFlags &= ~flag;				}
		PX_INLINE		void					setFlag(SipFlag flag, bool value)
												{
													if (value)
														raiseFlag(flag);
													else
														clearFlag(flag);
												}

		PX_FORCE_INLINE void					setSweptProperties();

		PX_FORCE_INLINE	void					setPairFlags(PxPairFlags flags);
		PX_FORCE_INLINE	bool					reportPairFlagsChanged(PxPairFlags flags) const;

		// Sc::Interaction
		virtual			void					initialize();
		virtual			void					destroy();
		virtual			bool					onActivate();
		virtual			bool					onDeactivate();
		//~Sc::Interaction

		// CoreInteraction
		virtual			void					updateState();
		//~CoreInteraction

		PX_INLINE		Scene&					getScene()				const	{ return getShape0().getScene();									}
	};

} // namespace Sc


PX_FORCE_INLINE void Sc::ShapeInstancePairLL::sendLostTouchReport(bool shapeDeleted)
{
	PX_ASSERT(thisFrameHaveContacts());
	PX_ASSERT(isReportPair());

	PxU32 thresholdForceLost = readIntFlag(ShapeInstancePairLL::FORCE_THRESHOLD_EXCEEDED_NOW) ? PxPairFlag::eNOTIFY_THRESHOLD_FORCE_LOST : 0;  // make sure to only send report if force is still above threshold
	PxU32 triggeredFlags = getPairFlags() & ((PxU32)PxPairFlag::eNOTIFY_TOUCH_LOST | thresholdForceLost);
	if (triggeredFlags)
	{
		PxU16 infoFlag = 0;
		if (mActorPair.getTouchCount() == 1)  // this code assumes that the actor pair touch count does get decremented afterwards
		{
			infoFlag |= PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH;
		}

		processUserNotification(triggeredFlags, infoFlag, shapeDeleted);
	}
}


PX_FORCE_INLINE void Sc::ShapeInstancePairLL::setPairFlags(PxPairFlags flags)
{
	PX_ASSERT((PxU32)flags < (PxPairFlag::eCCD_LINEAR << 1));  // to find out if a new PxPairFlag has been added in which case PAIR_FLAGS_MASK needs to get adjusted

	PxU32 newFlags = mFlags;
	PxU32 fl = (PxU32)flags & PAIR_FLAGS_MASK;
	newFlags &= (~PAIR_FLAGS_MASK);  // clear old flags
	newFlags |= fl;

	mFlags = newFlags;
}


// PT: using PxU32 to remove LHS. Please do not undo this.
//PX_FORCE_INLINE PxPairFlags Sc::ShapeInstancePairLL::getReportPairFlags() const
PX_FORCE_INLINE PxU32 Sc::ShapeInstancePairLL::getPairFlags() const
{
	return (mFlags & PAIR_FLAGS_MASK);
}


PX_FORCE_INLINE bool Sc::ShapeInstancePairLL::reportPairFlagsChanged(PxPairFlags flags) const
{
	PX_COMPILE_TIME_ASSERT((PAIR_FLAGS_MASK & 1) == 1);  // the code below assumes that the pair flags are stored first within mFlags
	PxU32 newFlags = flags;
	newFlags = (newFlags & PAIR_FLAGS_MASK);
	PxU32 oldFlags = mFlags & PAIR_FLAGS_MASK;

	return (newFlags != oldFlags);
}


PX_INLINE void Sc::ShapeInstancePairLL::swapAndClearForceThresholdExceeded()
{
	PxU32 flags = mFlags;

	PX_COMPILE_TIME_ASSERT(FORCE_THRESHOLD_EXCEEDED_NOW == (FORCE_THRESHOLD_EXCEEDED_BEFORE >> 1));

	PxU32 nowToBefore = (flags & FORCE_THRESHOLD_EXCEEDED_NOW) << 1;
	flags &= ~(FORCE_THRESHOLD_EXCEEDED_NOW | FORCE_THRESHOLD_EXCEEDED_BEFORE);
	flags |= nowToBefore;

	mFlags = flags;
}

PX_FORCE_INLINE	void Sc::ShapeInstancePairLL::removeFromReportPairList()
{
	// this method should only get called if the pair is in the list for
	// persistent or force based contact reports
	PX_ASSERT(mReportPairIndex != INVALID_REPORT_PAIR_ID);
	PX_ASSERT(readIntFlag(IS_IN_CONTACT_EVENT_LIST));

	Scene& scene = getScene();

	if (readIntFlag(IS_IN_FORCE_THRESHOLD_EVENT_LIST))
		scene.getNPhaseCore()->removeFromForceThresholdContactEventPairs(this);
	else 
	{
		PX_ASSERT(readIntFlag(IS_IN_PERSISTENT_EVENT_LIST));
		scene.getNPhaseCore()->removeFromPersistentContactEventPairs(this);
	}
}

PX_INLINE void Sc::ShapeInstancePairLL::resetManager()
{
	destroyManager();

	if (activeManagerAllowed())
		createManager();
}

PX_INLINE bool Sc::ShapeInstancePairLL::updateManager()
{
	if (activeManagerAllowed())
	{
		if (mManager == 0)
			createManager();

		return (mManager != NULL);  // creation might fail (pool reached limit, mem allocation failed etc.)
	}
	else
		return false;
}

PX_INLINE void Sc::ShapeInstancePairLL::destroyManager()
{
	if (mManager != 0)
	{
		PX_ASSERT(mLLIslandHook.isManaged());
		InteractionScene& intScene = getScene().getInteractionScene();
		intScene.getLLIslandManager().clearEdgeRigidCM(mLLIslandHook);
		intScene.getLowLevelContext()->destroyContactManager(mManager);
		mManager = 0;

		PxsTransformCache& cache = getScene().getInteractionScene().getLowLevelContext()->getTransformCache();
		getShape0().destroyTransformCache(cache);
		getShape1().destroyTransformCache(cache);

	}
}

PX_FORCE_INLINE bool Sc::ShapeInstancePairLL::activeManagerAllowed() 
{
	PX_ASSERT(getShape0().getActorSim().isDynamicRigid() || getShape1().getActorSim().isDynamicRigid());
	
	const ActorSim& actorSim0 = getShape0().getActorSim();
	const ActorSim& actorSim1 = getShape1().getActorSim();

	const bool isRigidDyamic0 = actorSim0.isDynamicRigid();
	const bool isRigidDyamic1 = actorSim1.isDynamicRigid();

	if(!isRigidDyamic0 || !isRigidDyamic1)
	{
		if(isRigidDyamic0 && !actorSim0.isActive())
		{
			//Static 1 vs sleeping dynamic 0.
			raiseFlag(ACTIVE_MANAGER_NOT_ALLOWED);
			return false;
		}
		else if(isRigidDyamic1 && !actorSim1.isActive())
		{
			//Static 0 vs sleeping dynamic 1
			raiseFlag(ACTIVE_MANAGER_NOT_ALLOWED);
			return false;
		}
	}

	//Dynamic vs dynamic or awake dynamic vs static
	clearFlag(ACTIVE_MANAGER_NOT_ALLOWED);
	return true;
}

PX_FORCE_INLINE void Sc::ShapeInstancePairLL::setSweptProperties()
{
	PX_ASSERT(mManager);

	//we may want to only write these if they have changed, the set code is a bit painful for the integration flags because of bit unpacking + packing.
	mManager->setCCD((getPairFlags() & PxPairFlag::eDETECT_CCD_CONTACT) != 0);
}

}

#endif
