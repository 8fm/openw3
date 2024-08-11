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


#ifndef PX_PHYSICS_SQUTILITIES
#define PX_PHYSICS_SQUTILITIES

#include "CmPhysXCommon.h"
#include "NpActor.h"

namespace physx
{
	class PxRigidActor;
	class NpShape;
	class PxShape;

	namespace Scb
	{
		class Actor;
		class Shape;
	}

namespace Sq
{
	class SceneQueryManager;
	struct ActorShape;
	struct PrunerPayload;

struct PxActorShape2 : PxActorShape
{
	const Scb::Shape* scbShape;
	const Scb::Actor* scbActor;

	PxActorShape2() : PxActorShape() {}

	PxActorShape2(PxRigidActor* eaActor, PxShape* eaShape, Scb::Shape* sShape, Scb::Actor* sActor) : PxActorShape(eaActor, eaShape)
	{
		scbShape = sShape;
		scbActor = sActor;
	}
};

	PxTransform		getGlobalPose(const NpShape& shape, const PxRigidActor& actor);
	PxTransform 	getGlobalPose(const Scb::Shape& scbShape, const Scb::Actor& scbActor);
	PxBounds3		computeWorldAABB(const Scb::Shape& scbShape, const Scb::Actor& scbActor);

	ActorShape*   	populate(const ActorShape*);
	void			populate(const PrunerPayload&,PxActorShape2&);

	PxRigidActor*	getHitActor(const Scb::Actor* scbActor);
	PxShape*		getHitShape(const Scb::Shape* scbShape);

	PX_FORCE_INLINE PxBounds3 inflateBounds(const PxBounds3& bounds)
	{
		PxVec3 e = bounds.getExtents() * 0.01f;
		return PxBounds3(bounds.minimum - e, bounds.maximum + e);
	}

}  // namespace Sq

}

#endif
