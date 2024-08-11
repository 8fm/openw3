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


#include "ScElement.h"
#include "ScElementInteraction.h"
#include "PsFoundation.h"
#include "PxsContext.h"
#include "PxvBroadPhase.h"
#include "PxsAABBManager.h"

using namespace physx;


Sc::ElementInteraction* Sc::Element::ElementInteractionIterator::getNext()
{
	for (; !mInteractions.empty(); mInteractions.popFront())
	{
		Interaction*const it = mInteractions.front();

		if (it->getInteractionFlags() & (PX_INTERACTION_FLAG_RB_ELEMENT | PX_INTERACTION_FLAG_ELEMENT_ACTOR))
		{
			PX_ASSERT(	(it->getType() == PX_INTERACTION_TYPE_MARKER) ||
						(it->getType() == PX_INTERACTION_TYPE_OVERLAP) ||
						(it->getType() == PX_INTERACTION_TYPE_TRIGGER) ||
						(it->getType() == PX_INTERACTION_TYPE_PARTICLE_BODY) );

			ElementInteraction* ei = static_cast<ElementInteraction*>(it);
			if ((&ei->getElement0() == mElement) || (&ei->getElement1() == mElement))
			{
				mInteractions.popFront();
				return ei;
			}
		}
	}

	return NULL;
}


Sc::ElementInteraction* Sc::Element::ElementInteractionReverseIterator::getNext()
{
	for (; !mInteractions.empty(); mInteractions.popBack())
	{
		Interaction*const it = mInteractions.back();

		if (it->getInteractionFlags() & (PX_INTERACTION_FLAG_RB_ELEMENT | PX_INTERACTION_FLAG_ELEMENT_ACTOR))
		{
			PX_ASSERT(	(it->getType() == PX_INTERACTION_TYPE_MARKER) ||
						(it->getType() == PX_INTERACTION_TYPE_OVERLAP) ||
						(it->getType() == PX_INTERACTION_TYPE_TRIGGER) ||
						(it->getType() == PX_INTERACTION_TYPE_PARTICLE_BODY) );

			ElementInteraction* ei = static_cast<ElementInteraction*>(it);
			if ((&ei->getElement0() == mElement) || (&ei->getElement1() == mElement))
			{
				mInteractions.popBack();
				return ei;
			}
		}
	}

	return NULL;
}


Sc::Element::~Element()
{
	PX_ASSERT(!hasAABBMgrHandle());
	mActor.onElementDetach(*this);
}

bool Sc::Element::createLowLevelVolume(const PxU32 group, const PxBounds3& bounds, const PxU32 compoundID, const AABBMgrId& aabbMgrId)
{
	PX_ASSERT(!hasAABBMgrHandle());

	PxsAABBManager* aabbMgr = getInteractionScene().getLowLevelContext()->getAABBManager();
	AABBMgrId newAABBMgrId = aabbMgr->createVolume(compoundID, aabbMgrId.mSingleOrCompoundId, group, this, bounds);
	mAABBMgrHandle = newAABBMgrId.mHandle;
	mAABBMgrSingleOrCompoundId = newAABBMgrId.mSingleOrCompoundId;

	if(newAABBMgrId.mHandle != PX_INVALID_BP_HANDLE)
		return true;

	//If the aabbmgr failed to add the new volume then report an error.
	Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "Unable to create broadphase entity because only 32768 shapes are supported");
	return false;
}

bool Sc::Element::destroyLowLevelVolume()
{
	if (hasAABBMgrHandle())
	{
		PxsAABBManager* aabbMgr = getInteractionScene().getLowLevelContext()->getAABBManager();
		bool removingLastShape = aabbMgr->releaseVolume(mAABBMgrHandle);
		mAABBMgrHandle=PX_INVALID_BP_HANDLE;
		mAABBMgrSingleOrCompoundId=PX_INVALID_BP_HANDLE;
		return removingLastShape;
	}

	return false;
}
