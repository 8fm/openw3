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

#include "RTdef.h"
#if RT_COMPILE
#ifndef RT_ACTOR_H
#define RT_ACTOR_H

#include <PxFoundation.h>

#include "ActorBase.h"

namespace physx
{
namespace apex
{
namespace destructible
{
	class DestructibleActor;
	struct DamageEvent;
	struct FractureEvent;
}
}
using namespace apex::destructible;
namespace fracture
{

class Compound;
class FracturePattern;

class Actor : public base::Actor
{
	friend class SimScene;
	friend class Renderable;
protected:
	Actor(base::SimScene* scene, DestructibleActor* actor);
public:
	virtual ~Actor();

	Compound* createCompound();

	Compound* createCompoundFromChunk(const apex::destructible::DestructibleActor& destructibleActor, PxU32 partIndex);

	bool patternFracture(const PxVec3& hitLocation, const PxVec3& dir, float scale = 1.f, float vel = 0.f, float radius = 0.f);
	bool patternFracture(const DamageEvent& damageEvent);
	bool patternFracture(const FractureEvent& fractureEvent, bool fractureOnLoad = true);

	bool rayCast(const PxVec3& orig, const PxVec3& dir, float &dist) const;

	DestructibleActor* getDestructibleActor() {return mActor;}

protected:
	void attachBasedOnFlags(base::Compound* c);

	FracturePattern* mDefaultFracturePattern;
	DestructibleActor* mActor;
	bool mRenderResourcesDirty;

	PxF32 mMinRadius;
	PxF32 mRadiusMultiplier;
	PxF32 mImpulseScale;
	bool mSheetFracture;

	struct AttachmentFlags
	{
		union
		{
			struct
			{
				PxU32 posX : 1;
				PxU32 negX : 1;
				PxU32 posY : 1;
				PxU32 negY : 1;
				PxU32 posZ : 1;
				PxU32 negZ : 1;
			};
			PxU32 v;
		};
	}mAttachmentFlags;

	struct MyDamageEvent
	{
		PxVec3	position;
		PxVec3	direction;
		PxF32	damage;
		PxF32	radius;
	};
	MyDamageEvent mDamageEvent;

};

}
}

#endif
#endif