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


#include "ScActorSim.h"
#include "ScActorCore.h"
#include "ScScene.h"
#include "ScRbElementInteraction.h"
#include "ScConstraintInteraction.h"

using namespace physx;

Sc::ActorSim::ActorSim(Scene& scene, ActorCore& core, IslandNodeInfo::Type type) :
	Actor(scene.getInteractionScene(), Ps::to8(core.getActorCoreType()), type),
	mCore(core)
{
	core.setSim(this);
}


Sc::ActorSim::~ActorSim()
{
}


Sc::Scene& Sc::ActorSim::getScene() const
{
	return getInteractionScene().getOwnerScene();
}

void Sc::ActorSim::postDominanceGroupChange()
{
	//force all related interactions to refresh, so they fetch new dominance values.
//	setActorsInteractionsDirty(	CoreInteraction::CIF_DIRTY_DOMINANCE, NULL, PX_INTERACTION_FLAG_RB_ELEMENT);
//	setActorsInteractionsDirty(	CoreInteraction::CIF_DIRTY_DOMINANCE, NULL, PX_INTERACTION_FLAG_CONSTRAINT);
	setActorsInteractionsDirty(	CoreInteraction::CIF_DIRTY_DOMINANCE, NULL, PX_INTERACTION_FLAG_RB_ELEMENT|PX_INTERACTION_FLAG_CONSTRAINT);
	//!!! MS: Something is fishy here. Dirty interactions end up in calling ::updateState()
	//        but this method does nothing dominance related (see ConstraintInteraction::updateState())
	//        I think the reason is that the shader constraints don't support dominance yet.
}

void Sc::ActorSim::setActorsInteractionsDirty(CoreInteraction::DirtyFlag flag, const Actor* other, PxU8 interactionFlag)
{
	Cm::Range<Interaction*const> interactions = getActorInteractions();
	for (; !interactions.empty(); interactions.popFront())
	{
		Interaction*const interaction = interactions.front();
		if ((!other || other == &interaction->getActor0() || other == &interaction->getActor1()) &&
			(interaction->getInteractionFlags() & interactionFlag))
		{
			CoreInteraction* ci = CoreInteraction::isCoreInteraction(interaction);
			if (ci != NULL)
				ci->setDirty(flag);
		}
	}
}
