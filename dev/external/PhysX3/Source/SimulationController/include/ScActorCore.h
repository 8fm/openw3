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


#ifndef PX_COLLISION_ACTOR_CORE
#define PX_COLLISION_ACTOR_CORE

#include "PsUserAllocated.h"
#include "CmPhysXCommon.h"
#include "PxMetaData.h"
#include "PxActor.h"


namespace physx
{

class PxActor;

namespace Sc
{

	class Scene;
	class ActorSim;

	class ActorCore : public Ps::UserAllocated
	{
	public:
// PX_SERIALIZATION
													ActorCore(const PxEMPTY&) :	mSim(NULL), mActorFlags(PxEmpty)	{}
		static			void						getBinaryMetaData(PxOutputStream& stream);
//~PX_SERIALIZATION
													ActorCore(PxActorType::Enum actorType, PxU16 actorFlags, 
															  PxClientID owner, PxU8 behavior, PxDominanceGroup dominanceGroup);
		/*virtual*/									~ActorCore();

		PX_FORCE_INLINE	ActorSim*					getSim()						const	{ return mSim;							}
		PX_FORCE_INLINE	void						setSim(ActorSim* sim)
													{
														PX_ASSERT((sim==NULL) ^ (mSim==NULL));
														mSim = sim;
													}

		PX_FORCE_INLINE	PxActorFlags				getActorFlags()					const	{ return mActorFlags;					}
						void						setActorFlags(PxActorFlags af); // BodyCore has a customized implementation

		PX_FORCE_INLINE	PxDominanceGroup			getDominanceGroup()				const	{ return mDominanceGroup;				}
						void						setDominanceGroup(PxDominanceGroup g);

		PX_FORCE_INLINE	void						setOwnerClient(PxClientID inId)			{ mOwnerClient = inId;					}
		PX_FORCE_INLINE	PxClientID					getOwnerClient()				const	{ return mOwnerClient;					}

		PX_FORCE_INLINE	PxActorClientBehaviorFlags	getClientBehaviorFlags()		const	{ return mClientBehaviorFlags;			}
		PX_FORCE_INLINE	void						setClientBehaviorFlags(PxActorClientBehaviorFlags b)
													{
														PX_ASSERT(PxU32(b)<256);	// PT: because we store it in a PxU8
														mClientBehaviorFlags = b;
													}

		PX_FORCE_INLINE	PxActorType::Enum			getActorCoreType()				const 	{ return PxActorType::Enum(mActorType);	}

						void						reinsertShapes();
// PX_AGGREGATE
						PxU32						mCompoundID;			// PT: TODO: this one makes us waste 12 bytes in bodycore. Use virtuals to store it somewhere else.
//~PX_AGGREGATE
	private:
						ActorSim*					mSim;

						PxActorFlags				mActorFlags;			// PxActor's flags (PxU16)
						PxU8						mActorType;				// store as 8 bits to save space.
						PxActorClientBehaviorFlags	mClientBehaviorFlags;	// similarly

						PxDominanceGroup			mDominanceGroup;		// PxU8
						PxClientID					mOwnerClient;			// PxU8
	};

#ifndef PX_X64
	PX_COMPILE_TIME_ASSERT(sizeof(Sc::ActorCore)==16);
#endif

} // namespace Sc

}

//////////////////////////////////////////////////////////////////////////

#endif
