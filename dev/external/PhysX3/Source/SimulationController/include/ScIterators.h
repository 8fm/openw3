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
      

#ifndef PX_PHYSICS_SCP_ITERATOR
#define PX_PHYSICS_SCP_ITERATOR

#include "PxVec3.h"
#include "PxContact.h"
#include "CmRange.h"

namespace physx
{

class PxShape;

namespace Sc
{
	class ShapeSim;
	class BodyCore;
	class Interaction;
	class Actor;

	class BodyIterator
	{
		public:
			BodyIterator() {}
			explicit BodyIterator(const Cm::Range<Actor*const>& range) : mRange(range) {}
			BodyCore*		getNext();
		private:
			Cm::Range<Actor* const> mRange;
	};

	class ContactIterator
	{
		public:
			struct Contact
			{
				Contact() 
					: normal(0.0f)
					, point(0.0f)
					, separation(0.0f)
					, normalForce(0.0f)
				{}

				PxVec3 normal;
				PxVec3 point;
				PxShape* shape0;
				PxShape* shape1;
				PxReal separation;
				PxReal normalForce;
				PxU32 faceIndex0;  // these are the external indices
				PxU32 faceIndex1;
				bool normalForceAvailable;
			};

			class Pair
			{
			public:
				Pair() : mIter(NULL, 0) {};
				Pair(const void*& contactData, const PxU32 contactDataSize, const PxReal*& forces, PxU32 numContacts, ShapeSim& shape0, ShapeSim& shape1);
				Contact* getNextContact();

			private:
				PxU32						mIndex;
				PxU32						mNumContacts;
				PxContactStreamIterator		mIter;
				const PxReal*				mForces;
				Contact						mCurrentContact;
				ShapeSim*					mShape0;
				ShapeSim*					mShape1;
			};

			ContactIterator() {}
			explicit ContactIterator(const Cm::Range<Interaction*const>& range): mRange(range) {}
			Pair* getNextPair();

		private:
			Cm::Range<Interaction* const>	mRange;
			Pair							mCurrentPair;
	};

}  // namespace Sc

}

#endif
