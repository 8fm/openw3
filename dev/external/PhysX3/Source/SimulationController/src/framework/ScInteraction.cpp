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


#include "Px.h"

#include "ScInteraction.h"
#include "ScInteractionScene.h"

using namespace physx;

Sc::Interaction::Interaction(Actor& actor0, Actor& actor1, InteractionType type, PxU8 flags) :
	mActor0				(actor0),
	mActor1				(actor1), 
	mSceneId			(PX_INVALID_INTERACTION_SCENE_ID), 
	mActorId0			(PX_INVALID_INTERACTION_ACTOR_ID),
	mActorId1			(PX_INVALID_INTERACTION_ACTOR_ID), 
	mInteractionType	(Ps::to8(type)),
	mInteractionFlags	(flags)
{
	PX_ASSERT(&actor0.getInteractionScene() == &actor1.getInteractionScene() && "Cannot create an interaction between actors belonging to different scenes.");
	PX_ASSERT((PxU32)type<256);	// PT: type is now stored on a byte

	// sizeof(Sc::Interaction): 32 => 24 bytes
}
