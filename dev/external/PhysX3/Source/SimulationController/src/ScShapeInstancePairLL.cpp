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
           
#include "ScShapeInstancePairLL.h"
//#include "ScMaterialCombiner.h"
#include "ScPhysics.h"
#include "PxsContext.h"
#include "PxsMaterialCombiner.h"
#include "GuTriangleMesh.h"
#include "ScStaticSim.h"

using namespace physx;

Sc::ShapeInstancePairLL::ShapeInstancePairLL(ShapeSim& s1, ShapeSim& s2, ActorPair& aPair, PxPairFlags pairFlags) :
	RbElementInteraction	(s1, s2, PX_INTERACTION_TYPE_OVERLAP, PX_INTERACTION_FLAG_RB_ELEMENT|PX_INTERACTION_FLAG_FILTERABLE|PX_INTERACTION_FLAG_SIP),
	mContactReportStamp		(PX_INVALID_U32),
	mFlags					(0),
	mActorPair				(aPair),
	mReportPairIndex		(INVALID_REPORT_PAIR_ID),
	mManager				(NULL),
	mReportStreamIndex		(0)
{
	// The PxPairFlags get stored in the SipFlag, make sure any changes get noticed
	PX_COMPILE_TIME_ASSERT(PxPairFlag::eSOLVE_CONTACT == (1<<0));
	PX_COMPILE_TIME_ASSERT(PxPairFlag::eMODIFY_CONTACTS == (1<<1));
	PX_COMPILE_TIME_ASSERT(PxPairFlag::eNOTIFY_TOUCH_FOUND == (1<<2));
	PX_COMPILE_TIME_ASSERT(PxPairFlag::eNOTIFY_TOUCH_PERSISTS == (1<<3));
	PX_COMPILE_TIME_ASSERT(PxPairFlag::eNOTIFY_TOUCH_LOST == (1<<4));
	PX_COMPILE_TIME_ASSERT(PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND == (1<<5));
	PX_COMPILE_TIME_ASSERT(PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS == (1<<6));
	PX_COMPILE_TIME_ASSERT(PxPairFlag::eNOTIFY_THRESHOLD_FORCE_LOST == (1<<7));
	PX_COMPILE_TIME_ASSERT(PxPairFlag::eNOTIFY_CONTACT_POINTS == (1<<8));
	PX_COMPILE_TIME_ASSERT(PxPairFlag::eDETECT_DISCRETE_CONTACT == (1<<9));
	PX_COMPILE_TIME_ASSERT(PxPairFlag::eDETECT_CCD_CONTACT == (1<<10));
	PX_COMPILE_TIME_ASSERT((PAIR_FLAGS_MASK & PxPairFlag::eRESOLVE_CONTACTS) == PxPairFlag::eRESOLVE_CONTACTS);
	PX_COMPILE_TIME_ASSERT((PxPairFlag::eRESOLVE_CONTACTS | PAIR_FLAGS_MASK) == PAIR_FLAGS_MASK);
	PX_COMPILE_TIME_ASSERT((PAIR_FLAGS_MASK & PxPairFlag::eCCD_LINEAR) == PxPairFlag::eCCD_LINEAR);
	PX_COMPILE_TIME_ASSERT((PxPairFlag::eCCD_LINEAR | PAIR_FLAGS_MASK) == PAIR_FLAGS_MASK);

	setPairFlags(pairFlags);

	// sizeof(ShapeInstancePairLL): 84 => 80 bytes

	PX_ASSERT(!mLLIslandHook.isManaged()); 
}


void Sc::ShapeInstancePairLL::visualize(Cm::RenderOutput& out)
{
	if (mManager)  // sleeping pairs have no contact points -> do not visualize
	{
		Scene& scene = getScene();
		PxReal scale = scene.getVisualizationScale();

		const PxReal flipNormal = (&mActorPair.getActorA() == &getShape0().getRbSim()) ? 1.0f : -1.0f;

		const void* contactData;
		PxU32 contactDataSize;
		const PxReal* impulses;
		PxU32 nbPoints = mManager->getContactPointData(contactData, contactDataSize, impulses);

		const PxReal param_contactForce = scene.getVisualizationParameter(PxVisualizationParameter::eCONTACT_FORCE);
		const PxReal param_contactNormal = scene.getVisualizationParameter(PxVisualizationParameter::eCONTACT_NORMAL);
		const PxReal param_contactError = scene.getVisualizationParameter(PxVisualizationParameter::eCONTACT_ERROR);
		const PxReal param_contactPoint = scene.getVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT);

		PxContactStreamIterator iter((PxU8*)contactData, contactDataSize);

		//for(PxU32 i=0; i < nbPoints; i++)
		PxU32 i = 0;
		while(iter.hasNextPatch())
		{
			iter.nextPatch();
			while(iter.hasNextContact())
			{
				iter.nextContact();

				PxReal length = 0;
				PxU32 color = 0;

				if ((param_contactForce != 0.0f) && impulses)
				{
					length = scale * param_contactForce * impulses[i];
					color = 0xff0000;
				}
				else if (param_contactNormal != 0.0f)
				{
					length = scale * param_contactNormal;
					color = 0x0000ff;
				}
				else if (param_contactError != 0.0f)
				{
					length = PxAbs(scale * param_contactError * iter.getSeparation());
					color = 0xffff00;
				}

				if (length != 0)
					out << Cm::RenderOutput::LINES << color << iter.getContactPoint() << iter.getContactPoint() + iter.getContactNormal() * length * flipNormal;

				if (param_contactPoint != 0)
				{
					PxReal s = scale * 0.1f;
					PxVec3 point = iter.getContactPoint();

					if(0) //temp debug to see identical contacts
						point.x += scale * 0.01f * (nbPoints - i + 1);

					out << Cm::RenderOutput::LINES << PxU32(PxDebugColor::eARGB_RED);
					out << point + PxVec3(-s,0,0) << point + PxVec3(s,0,0);
					out << point + PxVec3(0,-s,0) << point + PxVec3(0,s,0);
					out << point + PxVec3(0,0,-s) << point + PxVec3(0,0,s);

				}
			}
		}
	}
}

void Sc::ShapeInstancePairLL::processUserNotification(PxU32 contactEvent, PxU16 infoFlags, bool shapeDeleted)
{
	PX_ASSERT(thisFrameHaveContacts());

	if(mManager)
		Ps::prefetchLine(mManager);

	Scene& scene = getScene();
	NPhaseCore* npcore = scene.getNPhaseCore();

	PX_ASSERT(getPairFlags() & contactEvent);
	
	// make sure shape A and shape B are the same way round as the actors (in compounds they may be swapped)
	// TODO: make "unswapped" a SIP flag and set it in updateState()
	const bool unswapped = &mActorPair.getActorA() == &getShape0().getRbSim();
	const Sc::ShapeSim& shapeA = unswapped ? getShape0() : getShape1();
	const Sc::ShapeSim& shapeB = unswapped ? getShape1() : getShape0();

	if(!mActorPair.isInContactReportActorPairSet())
	{
		mActorPair.setInContactReportActorPairSet();
		npcore->addToContactReportActorPairSet(&mActorPair);
		mActorPair.incRefCount();
	}

	// Prepare user notification
	PxU32 timeStamp = scene.getTimeStamp();
	PxU32 shapePairTimeStamp = scene.getReportShapePairTimeStamp();
	
	ContactShapePair* stream = NULL;
	ContactStreamManager& cs = mActorPair.getContactStreamManager();
	if(mActorPair.streamResetStamp(timeStamp))
	{
		PX_ASSERT(mContactReportStamp != shapePairTimeStamp);  // actor pair and shape pair timestamps must both be out of sync in this case

		if (cs.maxPairCount != 0)
		{			
			stream = npcore->reserveContactShapePairs(cs.maxPairCount,cs.bufferIndex);  // use value from previous report
		}
		else
		{
			// TODO: Use some kind of heuristic
			PxU32 maxCount = 2;
			stream = npcore->reserveContactShapePairs(maxCount,cs.bufferIndex);
			cs.maxPairCount = Ps::to16(maxCount);
		}

		cs.reset();
	}
	else
	{
		if(cs.currentPairCount != 0)
			stream = npcore->getContactReportShapePairs(cs.bufferIndex);
	}

	if(!stream)
	{
		cs.flags |= ContactStreamManagerFlag::eINVALID_STREAM;
		return;
	}

	ContactShapePair* cp;
	if (mContactReportStamp != shapePairTimeStamp)
	{
		// this shape pair is not in the contact notification stream yet

		if (cs.currentPairCount < cs.maxPairCount)
			cp = stream + cs.currentPairCount;
		else
		{
			PxU32 newSize = cs.currentPairCount + (cs.currentPairCount >> 1) + 1;
			stream = npcore->resizeContactShapePairs(newSize, cs);
			if(stream)
				cp = stream + cs.currentPairCount;
			else
			{
				cs.flags |= ContactStreamManagerFlag::eINCOMPLETE_STREAM;
				return;
			}
		}

		PX_ASSERT(0==((uintptr_t)stream & 0x0f));  // check 16Byte alignment
		
		mReportStreamIndex = cs.currentPairCount;
		cp->shapes[0] = shapeA.getPxShape();
		cp->shapes[1] = shapeB.getPxShape();
		cp->contactStream = NULL;
		cp->contactCount = 0;
		cp->constraintStreamSize = 0;
		cp->requiredBufferSize = 0;
		cp->flags = infoFlags;
		PX_ASSERT(contactEvent <= 0xffff);
		cp->events = (PxU16)contactEvent;
		cp->shapeID[0] = shapeA.getID();
		cp->shapeID[1] = shapeB.getID();

		cs.currentPairCount++;

		mContactReportStamp = shapePairTimeStamp;
	}
	else
	{
		// this shape pair is in the contact notification stream already but there is a second event (can happen with force threshold reports, for example).
		
		PX_ASSERT(mReportStreamIndex < cs.currentPairCount);
		cp = &stream[mReportStreamIndex];
		cp->events |= contactEvent;
		cp->flags |= infoFlags;
	}

	const PxU32 contactFlags = mFlags;

	if(shapeDeleted)
		cs.flags |= ContactStreamManagerFlag::eDELETED_SHAPES;

	if ((contactFlags & CONTACTS_COLLECT_POINTS) && mManager && (!cp->contactStream) && !(contactEvent & (PxU32)(PxPairFlag::eNOTIFY_TOUCH_LOST | PxPairFlag::eNOTIFY_THRESHOLD_FORCE_LOST)))
	{
		const void* contactData;
		PxU32 contactDataSize;
		const PxReal* impulses;
		PxU32 nbPoints = mManager->getContactPointData(contactData, contactDataSize, impulses);

		if (contactDataSize)
		{
			infoFlags = cp->flags;
			infoFlags |= unswapped ? 0 : PxContactPairFlag::eINTERNAL_CONTACTS_ARE_FLIPPED;
			infoFlags |= readIntFlag(FACE_INDEX_REPORT_PAIR) ? PxContactPairFlag::eINTERNAL_HAS_FACE_INDICES : 0;

			PX_ASSERT(0==((uintptr_t)contactData & 0x0f));  // check 16Byte alignment
			PX_ASSERT(0==((uintptr_t)impulses & 0x0f));

			PxU32 alignedContactDataSize = (contactDataSize + 0xf) & 0xfffffff0;

			PX_ASSERT((impulses == NULL) || (((const PxU8*)contactData + alignedContactDataSize) == (const PxU8*)(impulses)));	// make sure contacts and impulses use consecutive memory space.
																												// Note: There are no impulses if collision response has been disabled.
			PxU32 impulseSize = impulses ? sizeof(PxReal) : 0;
			if (impulseSize)
				infoFlags |= PxContactPairFlag::eINTERNAL_HAS_IMPULSES;
			cp->contactStream = reinterpret_cast<const PxU8*>(contactData);
			cp->contactCount = Ps::to16(nbPoints);
			cp->constraintStreamSize = (PxU16)contactDataSize;
			cp->requiredBufferSize = alignedContactDataSize + impulseSize;

			cp->flags = infoFlags;
		}
	}
}

// based on void Sc::ShapeInstancePairLL::processUserNotification()	
PxU32 Sc::ShapeInstancePairLL::getContactPointData(const void*& contacts, PxU32& contactDataSize, const PxReal*& impulses)
{
	// Process LL generated contacts
	if (mManager == NULL)
		return 0;

	const PxU32 nbConstraints = mManager->getContactCount();
	if (nbConstraints == 0)
		return 0;

	PX_ASSERT(thisFrameHaveContacts() || thisFrameHaveCCDContacts());
	return mManager->getContactPointData(contacts, contactDataSize, impulses);
}

// Note that LL will not send end touch events for managers that are destroyed while having contact
void Sc::ShapeInstancePairLL::managerNewTouch()
{
	if (readIntFlag(LL_MANAGER_HAS_TOUCH))
		return; // Do not count the touch twice (for instance when recreating a manager with touch)
	// We have contact this frame
	raiseFlag(SipFlag(HAVE_CONTACTS_THIS_FRAME|LL_MANAGER_HAS_TOUCH));	// PT: touch flag member only once to avoid LHS
	mActorPair.incTouchCount();

	BodySim* body0 = getShape0().getBodySim();
	BodySim* body1 = getShape1().getBodySim();
	if(body0)
		body0->incrementBodyConstraintCounter();
	if(body1)
		body1->incrementBodyConstraintCounter();

	PxsIslandManager& islandManager = getScene().getInteractionScene().getLLIslandManager();
	PX_ASSERT(mLLIslandHook.isManaged());
	islandManager.setEdgeConnected(mLLIslandHook);

	if(!isReportPair())
		return;
	else
	{
		PX_ASSERT(thisFrameHaveContacts());
		PX_ASSERT(!readIntFlag(IS_IN_PERSISTENT_EVENT_LIST));
		PX_ASSERT(!readIntFlag(IS_IN_FORCE_THRESHOLD_EVENT_LIST));

		PxU32 pairFlags = getPairFlags();
		if (pairFlags & PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			PxU16 infoFlag = 0;
			if (mActorPair.getTouchCount() == 1)  // this code assumes that the actor pair touch count does get incremented beforehand
			{
				infoFlag = PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH;
			}

			processUserNotification(PxPairFlag::eNOTIFY_TOUCH_FOUND, infoFlag, false);
		}

		if (pairFlags & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
		{
			getScene().getNPhaseCore()->addToPersistentContactEventPairs(this);  // to make sure that from now on, the pairs are tested for persistent contact events
		}
		else if (pairFlags & ShapeInstancePairLL::CONTACT_FORCE_THRESHOLD_PAIRS)
		{
			// new touch -> need to start checking for force threshold events
			// Note: this code assumes that it runs before the pairs get tested for force threshold exceeded
			getScene().getNPhaseCore()->addToForceThresholdContactEventPairs(this);
		}
	}
}

bool Sc::ShapeInstancePairLL::managerLostTouch()
{
	if (!readIntFlag(LL_MANAGER_HAS_TOUCH))
		return false;
	clearFlag(LL_MANAGER_HAS_TOUCH);

	PxsIslandManager& islandManager = getScene().getInteractionScene().getLLIslandManager();
	PX_ASSERT(mLLIslandHook.isManaged());
	islandManager.setEdgeUnconnected(mLLIslandHook);

	// We do not have LL contacts this frame and also we lost LL contact this frame

	if (!isReportPair())
		clearFlag(HAVE_CONTACTS_THIS_FRAME);
	else
	{
		PX_ASSERT(thisFrameHaveContacts());

		sendLostTouchReport(false);

		if (readIntFlag(IS_IN_CONTACT_EVENT_LIST))
		{
			// don't need to worry about persistent/force-threshold contact events until next new touch

			if (readIntFlag(IS_IN_FORCE_THRESHOLD_EVENT_LIST))
			{
				getScene().getNPhaseCore()->removeFromForceThresholdContactEventPairs(this);
			}
			else
			{
				PX_ASSERT(readIntFlag(IS_IN_PERSISTENT_EVENT_LIST));
				getScene().getNPhaseCore()->removeFromPersistentContactEventPairs(this);
			}

			clearFlag(FORCE_THRESHOLD_EXCEEDED_FLAGS);
		}

		clearFlag(HAVE_CONTACTS_THIS_FRAME);
	}

	mActorPair.decTouchCount();

	BodySim* body0 = getShape0().getBodySim();
	BodySim* body1 = getShape1().getBodySim();
	if (body0)
		body0->decrementBodyConstraintCounter();
	if (body1)
		body1->decrementBodyConstraintCounter();

	if (!body0 || !body1)
	{
		if (body0)
		{
			body0->internalWakeUp();
		}
		if (body1)
		{
			body1->internalWakeUp();
		}

		return false;
	}
	return true;
}

void Sc::ShapeInstancePairLL::updateState()
{
	const PxU32 oldContactState = getManagerContactState();

	// Copy dirty flags before they get cleared below
	const PxU16 dirtyFlags = getDirtyFlags();
	CoreInteraction::updateState();
	Scene& scene = getScene();
	InteractionScene& iscene = scene.getInteractionScene();

	// Update notification status
	if (dirtyFlags & (CIF_DIRTY_NOTIFICATION | CIF_DIRTY_VISUALIZATION))
	{
		PxU32 notifyFlags = getPairFlags();

		// Check if collision response is disabled
		Sc::BodySim* bs0 = getShape0().getBodySim();
		Sc::BodySim* bs1 = getShape1().getBodySim();
		bool enabled = (bs0 && !bs0->isKinematic()) || (bs1 && !bs1->isKinematic());  // If the pair has no dynamic body then disable response
		enabled = enabled && ((notifyFlags & PxPairFlag::eCONTACT_DEFAULT) == PxPairFlag::eCONTACT_DEFAULT);
		setFlag(CONTACTS_RESPONSE_DISABLED, !enabled);

		// Check if contact points needed
		setFlag(CONTACTS_COLLECT_POINTS, ((notifyFlags & PxPairFlag::eNOTIFY_CONTACT_POINTS) ||
										  (notifyFlags & PxPairFlag::eMODIFY_CONTACTS) || 
										  scene.getVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT) ||
										  scene.getVisualizationParameter(PxVisualizationParameter::eCONTACT_NORMAL) ||
										  scene.getVisualizationParameter(PxVisualizationParameter::eCONTACT_ERROR) ||
										  scene.getVisualizationParameter(PxVisualizationParameter::eCONTACT_FORCE)) );
	}

	if (readDirtyFlag(dirtyFlags, CIF_DIRTY_BODY_KINEMATIC))
	{
		// Check kinematic shapes
		Sc::BodySim* bs0 = getShape0().getBodySim();
		Sc::BodySim* bs1 = getShape1().getBodySim();
		setFlag(SHAPE0_IS_KINEMATIC, (bs0 && (bs0->isKinematic()!=0)));
		setFlag(SHAPE1_IS_KINEMATIC, (bs1 && (bs1->isKinematic()!=0)));
	}

	// Check if the sip has contact reports requested
	if (isReportPair())
	{
		PX_ASSERT((mReportPairIndex == INVALID_REPORT_PAIR_ID) || (!readIntFlag(WAS_IN_PERSISTENT_EVENT_LIST)));

		if (mReportPairIndex == INVALID_REPORT_PAIR_ID && scene.getInteractionScene().isActiveInteraction(this))
		{
			PX_ASSERT(!readIntFlag(WAS_IN_PERSISTENT_EVENT_LIST));  // sanity check: an active pair should never have this flag set

			// the code below is to cover the case where the pair flags for a pair changed
			if (thisFrameHaveContacts() && (getPairFlags() & ShapeInstancePairLL::CONTACT_FORCE_THRESHOLD_PAIRS))
				scene.getNPhaseCore()->addToForceThresholdContactEventPairs(this);
		}

		if ((getPairFlags() & PxPairFlag::eNOTIFY_CONTACT_POINTS) &&
			((getShape0().getGeometryType() == PxGeometryType::eTRIANGLEMESH) || (getShape0().getGeometryType() == PxGeometryType::eHEIGHTFIELD) ||
			(getShape1().getGeometryType() == PxGeometryType::eTRIANGLEMESH) || (getShape1().getGeometryType() == PxGeometryType::eHEIGHTFIELD)))
			raiseFlag(FACE_INDEX_REPORT_PAIR);
		else
			clearFlag(FACE_INDEX_REPORT_PAIR);
	}
	PX_ASSERT(isReportPair() || (!readIntFlag(WAS_IN_PERSISTENT_EVENT_LIST)));  // pairs without contact reports should never have this flag set.

	PxU32 newContactState = getManagerContactState();
	bool recreateManager = (oldContactState != newContactState);

	// No use in updating manager properties if the manager is going to be re-created or does not exist yet
	if ((!recreateManager) && (mManager != 0))
	{
		ShapeSim& shapeSim0 = getShape0();
		ShapeSim& shapeSim1 = getShape1();

		// Update dominance
		if (readDirtyFlag(dirtyFlags, CIF_DIRTY_DOMINANCE))
		{
			Sc::BodySim* bs0 = shapeSim0.getBodySim();
			Sc::BodySim* bs1 = shapeSim1.getBodySim();

			// Static actors are in dominance group zero and must remain there
			const PxDominanceGroup dom0 = bs0 ? bs0->getActorCore().getDominanceGroup() : 0;
			const PxDominanceGroup dom1 = bs1 ? bs1->getActorCore().getDominanceGroup() : 0;

			const PxDominanceGroupPair cdom = getScene().getDominanceGroupPairFast(dom0, dom1);
			mManager->setDominance0(cdom.dominance0);
			mManager->setDominance1(cdom.dominance1);
		}

		// Update skin width
		if (readDirtyFlag(dirtyFlags, CIF_DIRTY_REST_OFFSET))
		{
			mManager->setRestDistance(shapeSim0.getRestOffset() + shapeSim1.getRestOffset());
		}

		// Activate, create managers as needed
		updateManager();
	}
	else if (iscene.isActiveInteraction(this))  // only re-create the manager if the pair is active
	{
		// A) This is a newly created pair
		//
		// B) The contact notification or processing state has changed.
		//    All existing managers need to be deleted and recreated with the correct flag set
		//    These flags can only be set at creation in LL
		resetManager();
	}

	if (mManager)	//TODO: this looks misplaced here!
		setSweptProperties();
}


void Sc::ShapeInstancePairLL::initialize()
{
	//Add a fresh edge to the island manager.
	PxsIslandManager& islandManager = getScene().getInteractionScene().getLLIslandManager();
	Sc::BodySim* bs0 = getShape0().getBodySim();
	Sc::BodySim* bs1 = getShape1().getBodySim();
	PxsIslandManagerNodeHook islandManagerHookA(PxsIslandManagerNodeHook::INVALID);
	PxsIslandManagerNodeHook islandManagerHookB(PxsIslandManagerNodeHook::INVALID);
	const PxU32 actorTypeA = bs0 ? bs0->getActorType() : PxActorType::eRIGID_STATIC;
	const PxU32 actorTypeB = bs1 ? bs1->getActorType() : PxActorType::eRIGID_STATIC;
	if(PxActorType::eRIGID_DYNAMIC == actorTypeA || PxActorType::eARTICULATION_LINK==actorTypeA)
	{
		islandManagerHookA = bs0->getLLIslandManagerNodeHook();
		PX_ASSERT(islandManagerHookA.isManaged());
	}
	if(PxActorType::eRIGID_DYNAMIC == actorTypeB || PxActorType::eARTICULATION_LINK==actorTypeB)
	{
		islandManagerHookB = bs1->getLLIslandManagerNodeHook();
		PX_ASSERT(islandManagerHookB.isManaged());
	}
	PX_ASSERT(!mLLIslandHook.isManaged());
	islandManager.addEdge(PxsIslandManager::EDGE_TYPE_CONTACT_MANAGER,islandManagerHookA,islandManagerHookB,mLLIslandHook);
	PX_ASSERT(mLLIslandHook.isManaged());

	RbElementInteraction::initialize();
	PX_ASSERT((&getShape0()) && (&getShape1()));

	mActorPair.incRefCount();
}

void Sc::ShapeInstancePairLL::destroy()
{
	destroyManager();

	getScene().getInteractionScene().getLLIslandManager().removeEdge(PxsIslandManager::EDGE_TYPE_CONTACT_MANAGER, mLLIslandHook);

	// This will remove the interaction from the actors list, which will prevent
	// update calls to this actor because of Body::wakeUp below.
	RbElementInteraction::destroy();

	if (mReportPairIndex != INVALID_REPORT_PAIR_ID)
	{
		removeFromReportPairList();
	}

	if (thisFrameHaveContacts())
	{
		// The SIP is removed explicitly because we still have contact
		BodySim* body0 = getShape0().getBodySim();
		BodySim* body1 = getShape1().getBodySim();
		if (body0)
			body0->decrementBodyConstraintCounter();
		if (body1)
			body1->decrementBodyConstraintCounter();
		mActorPair.decTouchCount();
	}

	mActorPair.decRefCount();
}

bool Sc::ShapeInstancePairLL::onActivate()
{
	RbElementInteraction::onActivate();

	if (isReportPair())
	{
		PX_ASSERT(mReportPairIndex == INVALID_REPORT_PAIR_ID);

		if (readIntFlag(WAS_IN_PERSISTENT_EVENT_LIST))
		{
			getScene().getNPhaseCore()->addToPersistentContactEventPairs(this);
			mFlags &= ~WAS_IN_PERSISTENT_EVENT_LIST;
		}
	}

	if (updateManager())
	{
		setSweptProperties();
		return true;
	}
	else
		return false;
}

bool Sc::ShapeInstancePairLL::onDeactivate()
{
	destroyManager();

	if (mReportPairIndex != INVALID_REPORT_PAIR_ID)
	{
		PX_COMPILE_TIME_ASSERT(IS_IN_PERSISTENT_EVENT_LIST == (WAS_IN_PERSISTENT_EVENT_LIST >> 1));
		PX_ASSERT(!(readIntFlag(WAS_IN_PERSISTENT_EVENT_LIST)));

		PxU32 wasInPersList = (mFlags & IS_IN_PERSISTENT_EVENT_LIST) << 1;
		mFlags |= wasInPersList;

		removeFromReportPairList();
	}

	RbElementInteraction::onDeactivate();

	return true;
}

void Sc::ShapeInstancePairLL::createManager()
{
	Sc::Scene& scene = getScene();

	PxsMaterialManager* materialManager = scene.getMaterialManager();

	ShapeSim& shapeSim0 = getShape0();
	ShapeSim& shapeSim1 = getShape1();

	PxActorType::Enum type0 = shapeSim0.getActorSim().getActorType(), type1 = shapeSim1.getActorSim().getActorType();

	const int disableResponse = readIntFlag(CONTACTS_RESPONSE_DISABLED) ? 1 : 0;
	const int disableCCDResponse = (getPairFlags() & PxPairFlag::eCCD_LINEAR) != PxPairFlag::eCCD_LINEAR;
	const int reportContactInfo = (getPairFlags() & PxPairFlag::eNOTIFY_CONTACT_POINTS) || readIntFlag(CONTACTS_COLLECT_POINTS);
	const int hasForceThreshold = !disableResponse && (getPairFlags() & CONTACT_FORCE_THRESHOLD_PAIRS);

	// Check if contact generation callback has been ordered on the pair
	int contactChangeable = 0;
	if (getPairFlags() & PxPairFlag::eMODIFY_CONTACTS)
		contactChangeable = 1;

	// Static actors are in dominance group zero and must remain there

	Sc::BodySim* bs0 = shapeSim0.getBodySim();
	Sc::BodySim* bs1 = shapeSim1.getBodySim();
	const PxDominanceGroup dom0 = bs0 ? bs0->getActorCore().getDominanceGroup() : 0;
	const PxDominanceGroup dom1 = bs1 ? bs1->getActorCore().getDominanceGroup() : 0;

	const PxDominanceGroupPair cdom = scene.getDominanceGroupPairFast(dom0, dom1);

	PxsTransformCache& cache = scene.getInteractionScene().getLowLevelContext()->getTransformCache();

	shapeSim0.createTransformCache(cache);
	shapeSim1.createTransformCache(cache);

	PxvManagerDescRigidRigid managerDesc;
	managerDesc.restDistance			= shapeSim0.getRestOffset() + shapeSim1.getRestOffset();
	managerDesc.rigidBody0				= bs0 ? &bs0->getLowLevelBody() : NULL;
	managerDesc.rigidBody1				= bs1 ? &bs1->getLowLevelBody() : NULL;
	managerDesc.reportContactInfo		= reportContactInfo;
	managerDesc.hasForceThreshold		= hasForceThreshold;
	managerDesc.contactChangeable		= contactChangeable;
	managerDesc.disableResponse			= disableResponse;
	managerDesc.disableCCDResponse		= disableCCDResponse;
	managerDesc.dominance0				= cdom.dominance0;
	managerDesc.dominance1				= cdom.dominance1;
	managerDesc.shapeCore0				= &shapeSim0.getCore().getCore();
	managerDesc.shapeCore1				= &shapeSim1.getCore().getCore();

	managerDesc.hasArticulations		= PxU32(type0 == PxActorType::eARTICULATION_LINK) | PxU32(type1 == PxActorType::eARTICULATION_LINK)<<1;
	managerDesc.hasDynamics				= PxU32(type0 != PxActorType::eRIGID_STATIC)      | PxU32(type1 != PxActorType::eRIGID_STATIC)<<1;

	managerDesc.rigidCore0				= &shapeSim0.getPxsRigidCore();
	managerDesc.rigidCore1				= &shapeSim1.getPxsRigidCore();

	managerDesc.transformCache0			= shapeSim0.getTransformCacheID();
	managerDesc.transformCache1			= shapeSim1.getTransformCacheID();

	managerDesc.userData				= this;
	mManager = scene.getInteractionScene().getLowLevelContext()->createContactManager(managerDesc, materialManager);

	PX_ASSERT(mLLIslandHook.isManaged());
	if (mManager)
		scene.getInteractionScene().getLLIslandManager().setEdgeRigidCM(mLLIslandHook, mManager);
}
