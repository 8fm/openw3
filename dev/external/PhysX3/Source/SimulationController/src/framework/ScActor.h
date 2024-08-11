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


#ifndef PX_FRAMEWORK_PXACTOR
#define PX_FRAMEWORK_PXACTOR

#include "CmPhysXCommon.h"
#include "CmUtils.h"
#include "ScInteractionType.h"
#include "PsUserAllocated.h"
#include "CmRange.h"
#include "PxActor.h"

namespace physx
{

#define SC_NOT_IN_SCENE_INDEX		0xffffffff  // the actor is not in the scene yet
#define SC_NOT_IN_ACTIVE_LIST_INDEX	0xfffffffe  // the actor is in the scene but not in the active list

namespace Sc
{
	struct IslandNodeInfo
	{
		enum Flag
		{
			eACTIVE						= (1 << 0),	// This is the sleep state of the actor. For rigid bodies, this state should be in sync with the eINSLEEPINGISLAND flag in
													// the sleep island generation system. The state gets adjusted either by the user explicitly, or by the simulation (as a
													// consequence of the results of sleep island generation or special cases like lost touch, refiltering etc.)

			eNEXT_FREE					= (1 << 1),
		};

		enum Type
		{
			eONE_WAY_DOMINATOR	= (eNEXT_FREE << 0),  // Objects which can push others but don't get pushed (ex. kinematics)
			eTWO_WAY			= (eNEXT_FREE << 1),  // Objects which push others and can get pushed
			eNON_PARTICIPANT	= (eNEXT_FREE << 2),  // Objects which do not take part in the island generation
		};
	};

	class InteractionScene;
	class Interaction;
	class Element;

	// The actor represents a simulated item that can be
	// connected to other actors through interactions. An actor
	// is created inactive but can be set either active or inactive. 

	class Actor : public Ps::UserAllocated
	{
		friend class InteractionScene;
		friend class Interaction;
	public:

		// Get the scene the actor resides in
		PX_FORCE_INLINE	InteractionScene&	getInteractionScene()		const	{ return mInteractionScene; }

		// Get whether the actor is active or not
		PX_FORCE_INLINE	bool				isActive()					const	{ return (mIslandNodeInfo & IslandNodeInfo::eACTIVE); }

		PX_FORCE_INLINE	PxU8				getIslandNodeInfo()			const	{ return mIslandNodeInfo;		}
		PX_FORCE_INLINE	void				raiseIslandNodeFlags(PxU8 flags)	{ mIslandNodeInfo |= flags;		}
		PX_FORCE_INLINE	void				clearIslandNodeFlags(PxU8 flags)	{ mIslandNodeInfo &= ~flags;	}
		PX_FORCE_INLINE	PxU8				getIslandNodeType()			const	{ return mIslandNodeInfo & (IslandNodeInfo::eONE_WAY_DOMINATOR | IslandNodeInfo::eTWO_WAY | IslandNodeInfo::eNON_PARTICIPANT); }
						void				setIslandNodeType(IslandNodeInfo::Type type);

		// Get the number of interactions connected to the actor
		PX_FORCE_INLINE	PxU32				getActorInteractionCount()	const	{ return mInteractions.size(); }

		// Prepares the actor for less than n interactions
						void				setInteractionCountHint(PxU32 n)	{ mInteractions.reserve(n, *this); }

		// Get an iterator to the interactions connected to the actor
		PX_INLINE Cm::Range<Interaction*const>	getActorInteractions()		const	{ return Cm::Range<Interaction*const>(mInteractions.begin(), mInteractions.end()); }

		// Get first element in the actor (linked list)
		PX_FORCE_INLINE	Element*			getElements_()						{ return mFirstElement;		}
		PX_FORCE_INLINE	const Element*		getElements_()				const	{ return mFirstElement;		}

		// Get the type ID of the actor
		PX_FORCE_INLINE	PxActorType::Enum	getActorType()				const	{ return PxActorType::Enum(mActorType);	}

		// Returns true if the actor is a dynamic rigid body (including articulation links)
		PX_FORCE_INLINE	bool				isDynamicRigid()			const	{ return mActorType == PxActorType::eRIGID_DYNAMIC || mActorType == PxActorType::eARTICULATION_LINK; }

		// Set active state - should not be called in the midst of a simulation step
						void				setActive(bool active, bool asPartOfCreation=false);

		// Get the number of counted interactions
		PX_FORCE_INLINE PxU32				getNumCountedInteractions()	const	{ return mNumCountedInteractions;	}

						void				onElementAttach(Element& element);
						void				onElementDetach(Element& element);

		PX_FORCE_INLINE	bool				notInScene()				const	{ return mSceneArrayIndex == SC_NOT_IN_SCENE_INDEX; }

	protected:
											Actor(InteractionScene& scene, PxU8 actorType, IslandNodeInfo::Type nodeType);
		virtual								~Actor();

		// Called by the framework when an actor is activated. 
		virtual			void				onActivate()	{}
		// Called by the framework when an actor is deactivated. 
		virtual			void				onDeactivate()	{}

						void				activateInteractions();
						void				deactivateInteractions();
	private:
		//These are called from interaction creation/destruction
						void				registerInteraction(Interaction* interaction);
						void				unregisterInteraction(Interaction* interaction);

						void				reallocInteractions(Sc::Interaction**& mem, PxU32& capacity, PxU32 size, PxU32 requiredMinCapacity);

						Actor&				operator=(const Actor&);

		// dsequeira: interaction arrays are a major cause of small allocations, so we don't want to delegate them to the heap allocator
		// it's not clear this inline array is really needed, we should take it out and see whether the cache perf is worse

		static const PxU32 INLINE_INTERACTION_CAPACITY = 4;
						Interaction*		mInlineInteractionMem[INLINE_INTERACTION_CAPACITY];

		Cm::OwnedArray<Sc::Interaction*, Sc::Actor, PxU32, &Sc::Actor::reallocInteractions>
											mInteractions;

						Element*			mFirstElement;

						InteractionScene&	mInteractionScene;
						PxU32				mSceneArrayIndex;				// Used by InteractionScene 

		// The interactions are sorted such that the ones which participate in island generation are separated from the ones which never do.
						PxU32				mNumTransferringInteractions;	// PT: probably doesn't need 32bits here
						PxU16				mNumCountedInteractions;		// PT: stored on PxU16 to save space
						PxU8				mActorType;						// PT: stored on a byte to save space
						PxU8				mIslandNodeInfo;
	};

} // namespace Sc

}

//////////////////////////////////////////////////////////////////////////

#endif // PX_FRAMEWORK_PXACTOR
