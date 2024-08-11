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
// Copyright (c) 2008-2011 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef GRBBODYFLAGS_H
#define GRBBODYFLAGS_H

//-----------------------------------------------------------------------------
// This enum must be identical to NxBodyFlag up to and including bit 11
// Grb uses bit 15 to indicate whether the actor is static. 
// Bit 7 indicates whether the actor is kinematic as in PhysX. 
// Of bits 7 and 15, only one must be set. 
// If 7 and 15 are not set then the actor is dynamic.
// The reason the extra bit is not necessary in PhysX is that static actors
// don't have bodies in PhysX.
//-----------------------------------------------------------------------------
enum GrbBodyFlag
	{
	/**
	\brief Set if gravity should not be applied on this body

	@see NxBodyDesc.flags NxScene.setGravity()
	*/
	GRB_BF_DISABLE_GRAVITY	= (1<<0),
	
	/**	
	\brief Enable/disable freezing for this body/actor. 

	\note This is an EXPERIMENTAL feature which doesn't always work on in all situations, e.g. 
	for actors which have joints connected to them.
	
	To freeze an actor is a way to simulate that it is static. The actor is however still simulated
	as if it was dynamic, it's position is just restored after the simulation has finished. A much
	more stable way to make an actor temporarily static is to raise the GRB_BF_KINEMATIC flag.
	*/
	GRB_BF_FROZEN_POS_X		= (1<<1),
	GRB_BF_FROZEN_POS_Y		= (1<<2),
	GRB_BF_FROZEN_POS_Z		= (1<<3),
	GRB_BF_FROZEN_ROT_X		= (1<<4),
	GRB_BF_FROZEN_ROT_Y		= (1<<5),
	GRB_BF_FROZEN_ROT_Z		= (1<<6),
	GRB_BF_FROZEN_POS		= GRB_BF_FROZEN_POS_X|GRB_BF_FROZEN_POS_Y|GRB_BF_FROZEN_POS_Z,
	GRB_BF_FROZEN_ROT		= GRB_BF_FROZEN_ROT_X|GRB_BF_FROZEN_ROT_Y|GRB_BF_FROZEN_ROT_Z,
	GRB_BF_FROZEN			= GRB_BF_FROZEN_POS|GRB_BF_FROZEN_ROT,


	/**
	\brief Enables kinematic mode for the actor.
	
	Kinematic actors are special dynamic actors that are not 
	influenced by forces (such as gravity), and have no momentum. They are considered to have infinite
	mass and can be moved around the world using the moveGlobal*() methods. They will push 
	regular dynamic actors out of the way. Kinematics will not collide with static or other kinematic objects.
	
	Kinematic actors are great for moving platforms or characters, where direct motion control is desired.

	You can not connect Reduced joints to kinematic actors. Lagrange joints work ok if the platform
	is moving with a relatively low, uniform velocity.

	@see NxActor NxActor.raiseActorFlag()
	*/
	GRB_BF_KINEMATIC			= (1<<7),		//!< Enable kinematic mode for the body.

	/**
	\brief Enable debug renderer for this body

	@see NxScene.getDebugRenderable() NxDebugRenderable NxParameter
	*/
	GRB_BF_VISUALIZATION		= (1<<8),

	GRB_BF_DUMMY_0				= (1<<9), // deprecated flag placeholder

	/**
	\brief Filter velocities used keep body awake. The filter reduces rapid oscillations and transient spikes.
	@see NxActor.isSleeping()
	*/
	GRB_BF_FILTER_SLEEP_VEL		= (1<<10),

	/**
	\brief Enables energy-based sleeping algorithm.
	@see NxActor.isSleeping() NxBodyDesc.sleepEnergyThreshold 
	*/
	GRB_BF_ENERGY_SLEEP_TEST	= (1<<11),

	//GRB only
	GRB_BF_STATIC	            = (1<<15),

	// These two from RrbBodyFunc 'combination of Nx actor and body flags'
	GRB_BF_DISABLE_COLLISION	= (1<<16),
    GRB_BF_INVALID				= (1<<17),

	GRB_BF_INTEROP_ACTOR		= (1<<18),

	//GRB only
	GRB_BF_SLEEPING	            = (1<<19),
	GRB_BF_WAKING	            = (1<<20),
	GRB_BF_FALLING_ASLEEP       = (1<<21),

	GRB_BF_MOVING_KINEMATIC		= (1<<22),
	};
//-----------------------------------------------------------------------------
#endif
