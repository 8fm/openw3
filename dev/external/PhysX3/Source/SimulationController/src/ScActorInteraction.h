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


#ifndef PX_COLLISION_ACTOR_INTERACTION
#define PX_COLLISION_ACTOR_INTERACTION

#include "ScInteraction.h"
#include "ScCoreInteraction.h"
#include "ScActorSim.h"

namespace physx
{
namespace Sc 
{

	class ActorSim;
	class Scene;

	class ActorInteraction : public Interaction, public CoreInteraction
	{
	public:
		PX_INLINE				ActorInteraction(ActorSim& actor0, ActorSim& actor1, InteractionType interactionType, PxU8 flags);
		virtual					~ActorInteraction();

		PX_INLINE	ActorSim&	getActorSim0() const;
		PX_INLINE	ActorSim&	getActorSim1() const;
		virtual		NPhaseCore*	getNPhaseCore() const;
		PX_INLINE	Scene&		getScene() const;
	private:
	};

} // namespace Sc

//////////////////////////////////////////////////////////////////////////
PX_INLINE Sc::ActorInteraction::ActorInteraction(ActorSim& actor0, ActorSim& actor1, InteractionType interactionType, PxU8 flags) :
	Interaction		(actor0, actor1, interactionType, flags),
	CoreInteraction	(false)
{
}

PX_INLINE Sc::ActorSim& Sc::ActorInteraction::getActorSim0() const
{
	return static_cast<ActorSim&>(getActor0());
}

PX_INLINE Sc::ActorSim& Sc::ActorInteraction::getActorSim1() const
{
	return static_cast<ActorSim&>(getActor1());
}

PX_INLINE Sc::Scene& Sc::ActorInteraction::getScene() const
{
	return getActorSim0().getScene();
}

}

#endif
