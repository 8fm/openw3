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


#include "ScArticulationJointSim.h"
#include "ScArticulationJointCore.h"
#include "ScBodySim.h"
#include "ScScene.h"
#include "PxsRigidBody.h"
#include "PxsArticulation.h"
#include "ScArticulationSim.h"

using namespace physx;

Sc::ArticulationJointSim::ArticulationJointSim(ArticulationJointCore& joint, ActorSim& parent, ActorSim& child) :
	ActorInteraction	(parent, child, PX_INTERACTION_TYPE_ARTICULATION, 0),
	mCore				(joint)
{
	initialize();

	BodySim& childBody = static_cast<BodySim&>(child),
		   & parentBody = static_cast<BodySim&>(parent);

	parentBody.getArticulation()->addBody(childBody, &parentBody, this);

	mCore.setSim(this);
}


Sc::ArticulationJointSim::~ArticulationJointSim()
{
	if(mIslandHook.isManaged())
		getScene().getInteractionScene().getLLIslandManager().removeEdge(PxsIslandManager::EDGE_TYPE_ARTIC, mIslandHook);

	BodySim& child = getChild();
	child.getArticulation()->removeBody(child);

	mCore.setSim(NULL);
}




Sc::BodySim& Sc::ArticulationJointSim::getParent() const
{
	return static_cast<BodySim&>(getActorSim0());
}


Sc::BodySim& Sc::ArticulationJointSim::getChild() const
{
	return static_cast<BodySim&>(getActorSim1());
}



bool Sc::ArticulationJointSim::onActivate() 
{
	if(!mIslandHook.isManaged())
	{
		PxsIslandManager& islandManager = getParent().getInteractionScene().getLLIslandManager();
		islandManager.addEdge(PxsIslandManager::EDGE_TYPE_ARTIC, getParent().getLLIslandManagerNodeHook(), getChild().getLLIslandManagerNodeHook(), mIslandHook);
		islandManager.setEdgeArticulationJoint(mIslandHook);
		islandManager.setEdgeConnected(mIslandHook);
	}

	if(!(getParent().isActive() && getChild().isActive()))
		return false;

	return true; 
}

bool Sc::ArticulationJointSim::onDeactivate() 
{ 
	return true; 
}


void Sc::ArticulationJointSim::destroy()
{
	setClean();
	ActorInteraction::destroy();
	delete this;
}
