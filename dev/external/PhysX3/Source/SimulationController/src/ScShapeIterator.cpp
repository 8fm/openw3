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

           
#include "ScIterators.h"
#include "ScBodySim.h"
#include "ScShapeSim.h"
#include "ScConstraintSim.h"
#include "ScShapeInstancePairLL.h"
#include "ScShapeIterator.h"
#include "GuTriangleMesh.h"

using namespace physx;

///////////////////////////////////////////////////////////////////////////////

Sc::ShapeSim* Sc::ShapeIterator::getNext()	
{ 
	while(1)
	{
		if(!mCurrent)
			return NULL;
		Element* element = mCurrent;
		mCurrent = mCurrent->mNextInActor;
		if(element->getElementType() == Sc::PX_ELEMENT_TYPE_SHAPE)	// PT: this can also be a particle packet!
			return static_cast<Sc::ShapeSim*>(element);
	}
}

void Sc::ShapeIterator::init(const Sc::ActorSim& r)
{ 
	mCurrent = const_cast<Element*>(r.getElements_());
}	

///////////////////////////////////////////////////////////////////////////////

Sc::BodyCore* Sc::BodyIterator::getNext()
{ 
	while (!mRange.empty())
	{
		Sc::Actor* const current = mRange.front();
		mRange.popFront();

		if (current->isDynamicRigid())
			return &static_cast<Sc::BodySim*>(current)->getBodyCore();
	}
	return NULL;
}


///////////////////////////////////////////////////////////////////////////////

Sc::ContactIterator::Pair::Pair(const void*& contactData, PxU32 contactDataSize, const PxReal*& forces, PxU32 numContacts, ShapeSim& shape0, ShapeSim& shape1)
: mIndex(0)
, mNumContacts(numContacts)
, mIter(reinterpret_cast<const PxU8*>(contactData), contactDataSize)
, mForces(forces)
, mShape0(&shape0)
, mShape1(&shape1)
{	
	mCurrentContact.shape0 = shape0.getPxShape();
	mCurrentContact.shape1 = shape1.getPxShape();
	mCurrentContact.normalForceAvailable = (forces != NULL);
}

Sc::ContactIterator::Pair* Sc::ContactIterator::getNextPair()
{ 
	if(!mRange.empty())
	{
		ShapeInstancePairLL* llPair = static_cast<ShapeInstancePairLL*>(mRange.front());
		mRange.popFront();
		const void* contactData = NULL;
		PxU32 contactDataSize = 0;
		const PxReal* forces = NULL;
		PxU32 numContacts = llPair->getContactPointData(contactData, contactDataSize, forces);
		mCurrentPair = Pair(contactData, contactDataSize, forces, numContacts, llPair->getShape0(), llPair->getShape1());
		return &mCurrentPair;
	}
	else
		return NULL;
}

Sc::ContactIterator::Contact* Sc::ContactIterator::Pair::getNextContact()
{
	if(mIndex < mNumContacts)
	{
		if(!mIter.hasNextContact())
		{
			if(!mIter.hasNextPatch())
				return NULL;
			mIter.nextPatch();
		}
		PX_ASSERT(mIter.hasNextContact());
		mIter.nextContact();

		mCurrentContact.normal = mIter.getContactNormal();
		mCurrentContact.point = mIter.getContactPoint();
		mCurrentContact.separation = mIter.getSeparation();
		mCurrentContact.normalForce = mForces ? mForces[mIndex] : 0;
		mCurrentContact.faceIndex0 = mIter.getFaceIndex0();
		mCurrentContact.faceIndex1 = mIter.getFaceIndex1();

		mIndex++;
		return &mCurrentContact;
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
