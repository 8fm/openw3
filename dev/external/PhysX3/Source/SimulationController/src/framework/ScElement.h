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


#ifndef PX_FRAMEWORK_PXELEMENT
#define PX_FRAMEWORK_PXELEMENT

#include "PxvConfig.h"
#include "ScActor.h"
#include "PsUserAllocated.h"
#include "PxsAABBManagerId.h"

namespace physx
{
namespace Sc
{
	
	class ElementInteraction;
	enum ElementType
	{
		PX_ELEMENT_TYPE_SHAPE = 0,
		PX_ELEMENT_TYPE_PARTICLE_PACKET,
        PX_ELEMENT_TYPE_CLOTH,
		PX_ELEMENT_TYPE_COUNT,
	};

	enum ElementFlags
	{
		PX_ELEMENT_FLAG_USER		= (1<<0),
		PX_ELEMENT_FLAG_MAX			= (1<<1)
	};

	PX_COMPILE_TIME_ASSERT((PX_ELEMENT_FLAG_MAX>>2) <= 1);		//2 bits reserved for flags on win32 and win64 (8 bits on other platforms)
	PX_COMPILE_TIME_ASSERT(PX_ELEMENT_TYPE_COUNT <= 4);			//2 bits reserved for type on win32 and win64 (8 bits on other platforms)

	/*
	A Element is a part of a Actor. It contributes to the activation framework by adding its 
	interactions to the actor. */
	class Element : public Ps::UserAllocated
	{
		friend class ElementInteraction;
		friend class InteractionScene;

	public:
		class ElementInteractionIterator
		{
			public:
				PX_FORCE_INLINE			ElementInteractionIterator(const Element& e, Cm::Range<Interaction*const> interactions) : mInteractions(interactions), mElement(&e) {}
				ElementInteraction*		getNext();

			private:
				Cm::Range<Interaction*const>	mInteractions;
				const Element*				mElement;
		};

		class ElementInteractionReverseIterator
		{
			public:
				PX_FORCE_INLINE			ElementInteractionReverseIterator(const Element& e, Cm::Range<Interaction*const> interactions) : mInteractions(interactions), mElement(&e) {}
				ElementInteraction*		getNext();

			private:
				Cm::Range<Interaction*const>	mInteractions;
				const Element*				mElement;
		};

	public:
												PX_FORCE_INLINE Element(Actor& actor, ElementType type)
													: mNextInActor				(NULL)
													, mActor					(actor)
#if PX_USE_16_BIT_HANDLES
													, mAABBMgrHandle			(PX_INVALID_BP_HANDLE)
													, mAABBMgrSingleOrCompoundId(PX_INVALID_BP_HANDLE)
													, mType						(Ps::to8(type))
													, mElemFlags				(0)
#else
													, mAABBMgrHandle			(PX_INVALID_BP_HANDLE)
													, mType						(Ps::to8(type))
													, mAABBMgrSingleOrCompoundId(PX_INVALID_BP_HANDLE)
													, mElemFlags				(0)
#endif
													{
														PX_ASSERT((type & 0x03) == type);	// we use 2 bits to store
														actor.onElementAttach(*this);
													}

		virtual									~Element();

		// Get an iterator to the interactions connected to the element
		PX_INLINE		ElementInteractionIterator getElemInteractions()	const	{ return ElementInteractionIterator(*this, mActor.getActorInteractions()); }
		PX_INLINE		ElementInteractionReverseIterator getElemInteractionsReverse()	const	{ return ElementInteractionReverseIterator(*this, mActor.getActorInteractions()); }
		PX_INLINE		PxU32					getElemInteractionCount()	const	{ return mActor.getActorInteractionCount(); }

		PX_FORCE_INLINE	Actor&					getScActor()				const	{ return mActor;	}

		PX_FORCE_INLINE	InteractionScene&		getInteractionScene()		const;
		PX_INLINE		bool					isActive()					const;

		PX_FORCE_INLINE	ElementType				getElementType()			const	{ return ElementType(mType);	}

		PX_FORCE_INLINE bool					hasAABBMgrHandle()			const	{ return mAABBMgrHandle != PX_INVALID_BP_HANDLE; }
		PX_FORCE_INLINE PxcBpHandle				getAABBMgrHandle()			const	{ return mAABBMgrHandle; }
		PX_FORCE_INLINE	AABBMgrId				getAABBMgrId()				const	{ return AABBMgrId(mAABBMgrHandle, mAABBMgrSingleOrCompoundId);	}

						bool					createLowLevelVolume(const PxU32 group, const PxBounds3& bounds, const PxU32 compoundID=PX_INVALID_U32, const AABBMgrId& aabbMgrId = AABBMgrId());
						bool					destroyLowLevelVolume();

						Element*				mNextInActor;
	private:
						Element&				operator=(const Element&);
						Actor&					mActor;

#if PX_USE_16_BIT_HANDLES
						PxcBpHandle				mAABBMgrHandle;
						PxcBpHandle				mAABBMgrSingleOrCompoundId;
						PxU8					mType;
	protected:
						PxU8					mElemFlags;
						PxU8					mPad[2];
#else
						PxcBpHandle				mAABBMgrHandle				: 30;
						PxcBpHandle				mType						:  2;
						PxcBpHandle				mAABBMgrSingleOrCompoundId	: 30;
	protected:
						PxcBpHandle				mElemFlags					:  2;

#endif
	};

} // namespace Sc

//////////////////////////////////////////////////////////////////////////

PX_FORCE_INLINE Sc::InteractionScene& Sc::Element::getInteractionScene() const
{
	return mActor.getInteractionScene();
}

PX_INLINE bool Sc::Element::isActive() const
{
	return mActor.isActive();
}


}

#endif
