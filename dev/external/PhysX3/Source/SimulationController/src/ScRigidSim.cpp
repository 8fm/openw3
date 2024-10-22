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


#include "ScScene.h"
#include "ScRigidSim.h"
#include "ScShapeInstancePairLL.h"
#include "ScTriggerInteraction.h"
#include "ScObjectIDTracker.h"
#include "ScShapeIterator.h"

using namespace physx;

/*
	PT:

	The BP group ID comes from a Cm::IDPool, and RigidSim is the only class releasing the ID.

	The rigid tracker ID comes from a Cm::IDPool internal to an ObjectIDTracker, and RigidSim
	is the only class using it.

	Thus we should:
	- promote the BP group ID stuff to a "tracker" object
	- use the BP group ID as a rigid ID
*/

Sc::RigidSim::RigidSim(Scene& scene, RigidCore& core, IslandNodeInfo::Type type) :
	ActorSim(scene, core, type)
{
	mRigidId	= scene.getRigidIDTracker().createID();

#ifdef PX_CHECKED
	PX_CHECK_MSG(getBroadphaseGroupId() < PX_INVALID_BP_HANDLE, "The total of actors in the scene plus the number of adds cannot exceed 65535 between simulate()/fetchResult() calls.  The sdk will can now proceed with unexpected outcomes. \n");
#endif
}

Sc::RigidSim::~RigidSim()
{
	Sc::Scene& scScene = getScene();
	scScene.getRigidIDTracker().releaseID(mRigidId);
}

void Sc::RigidSim::updateCachedShapeState()
{
	for(Element*e = getElements_(); e!=0; e = e->mNextInActor)
	{
		if(e->getElementType() == PX_ELEMENT_TYPE_SHAPE)
			static_cast<Sc::ShapeSim*>(e)->onTransformChange();
	}

}

Sc::ShapeSim &Sc::RigidSim::getSimForShape(Sc::ShapeCore& core) const
{
	// DS: looks painful to traverse a linked list this way
	Sc::ShapeIterator iterator;
	iterator.init(*this);
	Sc::ShapeSim* sim;
	while((sim = iterator.getNext())!=NULL)
	{
		if(&sim->getCore() == &core)
			return *sim;
	}
	PX_ASSERT(0); // should never fail
	return *reinterpret_cast<Sc::ShapeSim*>(1);
}

PxActor* Sc::RigidSim::getPxActor()
{
	return getRigidCore().getPxActor();
}
