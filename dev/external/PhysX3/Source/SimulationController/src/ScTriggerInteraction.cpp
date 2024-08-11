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


#include "ScTriggerInteraction.h"
#include "ScBodySim.h"

using namespace physx;


Sc::TriggerInteraction::~TriggerInteraction()
{
}


bool Sc::TriggerInteraction::isOneActorActive()
{
	if (getActor0().isActive())
	{
		PX_ASSERT(getTriggerShape()->getBodySim());

		PX_ASSERT(!getTriggerShape()->getBodySim()->isKinematic() || getTriggerShape()->getBodySim()->readInternalFlag(BodySim::BF_KINEMATIC_MOVED) || 
			getTriggerShape()->getBodySim()->readInternalFlag(BodySim::BF_KINEMATIC_SETTLING));
		return true;
	}
	if (getActor1().isActive())
	{
		PX_ASSERT(getOtherShape()->getBodySim());
		PX_ASSERT(!getOtherShape()->getBodySim()->isKinematic() || getOtherShape()->getBodySim()->readInternalFlag(BodySim::BF_KINEMATIC_MOVED) || 
			getOtherShape()->getBodySim()->readInternalFlag(BodySim::BF_KINEMATIC_SETTLING));
		return true;
	}

	return false;
}


//
// Some general information about triggers and sleeping
//
// The goal is to avoid running overlap tests if both objects are sleeping.
// This is an optimization for eNOTIFY_TOUCH_LOST events since the overlap state 
// can not change if both objects are sleeping. eNOTIFY_TOUCH_FOUND should be sent nonetheless.
// For this to work the following assumptions are made:
// - On creation or if the pose of an actor is set, the pair will always be checked.
// - If the scenario above does not apply, then a trigger pair can only be deactivated, if both actors are sleeping.
// - If an overlapping actor is activated/deactivated, the trigger interaction gets notified
//   (this should be enough, the interaction itself does not need to be transferring in sleep island generation
//    for this to work).
//
bool Sc::TriggerInteraction::onActivate()
{
	if (!(readIntFlag(PROCESS_THIS_FRAME)))
	{
		return isOneActorActive();
	}
	else
		return true;  // newly created trigger pairs should always test for overlap, no matter the sleep state
}


bool Sc::TriggerInteraction::onDeactivate()
{
	if (!readIntFlag(PROCESS_THIS_FRAME))
	{
		return !(isOneActorActive());
	}
	else
		return false;
}


void Sc::TriggerInteraction::initialize()
{
	RbElementInteraction::initialize();
	PX_ASSERT(getShape0().getFlags() & PxShapeFlag::eTRIGGER_SHAPE);
	mTriggerCache.state = Gu::TRIGGER_DISJOINT;
}


void Sc::TriggerInteraction::destroy()
{
	RbElementInteraction::destroy();
}
