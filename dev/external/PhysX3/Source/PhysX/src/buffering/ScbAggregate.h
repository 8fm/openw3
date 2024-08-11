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


#ifndef PX_PHYSICS_SCB_AGGREGATE
#define PX_PHYSICS_SCB_AGGREGATE

#include "CmPhysXCommon.h"
#include "PxAggregate.h"
#include "ScbActor.h"
#include "ScbAggregate.h"
#include "ScbBase.h"

// PX_SERIALIZATION
#include "PxSerialFramework.h"
//~PX_SERIALIZATION

namespace physx
{
namespace Scb
{

class Actor;

struct AggregateBuffer
{
	AggregateBuffer() : addBufferIdx(0xffffffff), addCount(0), removeBufferIdx(0xffffffff), removeCount(0) {}

	PxU32			addBufferIdx;
	PxU32			addCount;
	PxU32			removeBufferIdx;
	PxU32			removeCount;
};


class Aggregate : public Base
{
private:
	enum BufferFlag
	{
		BF_ADD_ACTOR			= (1 << 0),
		BF_REMOVE_ACTOR			= (1 << 1)
	};

public:

// PX_SERIALIZATION
										Aggregate(const PxEMPTY&) : Base(PxEmpty) {}
	static		void					getBinaryMetaData(PxOutputStream& stream);
//~PX_SERIALIZATION
	PX_INLINE							Aggregate(PxAggregate* px, PxU32 maxActors, bool selfCollision);
	PX_INLINE							~Aggregate();

				void					addActor(Scb::Actor&);
				void					removeActor(Scb::Actor& actor, bool reinsert);


	//---------------------------------------------------------------------------------
	// Data synchronization
	//---------------------------------------------------------------------------------
	PX_INLINE void						syncState(Scb::Scene& scene);


	//---------------------------------------------------------------------------------
	// Miscellaneous
	//---------------------------------------------------------------------------------
	PX_FORCE_INLINE	PxU32				getMaxActorCount() const { return mMaxNbActors; }
	PX_FORCE_INLINE	bool				getSelfCollide() const { return mSelfCollide; }
	PX_FORCE_INLINE PxU32				getCompoundID() const { return mCompoundID; }
	PX_FORCE_INLINE void				setCompoundID(PxU32 cid) { mCompoundID = cid; }

	PX_FORCE_INLINE bool				isBufferingSpecial(ControlState::Enum state) const;

					PxAggregate*		mPxAggregate;	// Back pointer
private:
					PxU32				mCompoundID;
					PxU32				mMaxNbActors;
					bool				mSelfCollide;

	PX_FORCE_INLINE	const Scb::AggregateBuffer*	getBufferedData()	const	{ return (const Scb::AggregateBuffer*)getStream();	}
	PX_FORCE_INLINE	Scb::AggregateBuffer*		getBufferedData()			{ return (Scb::AggregateBuffer*)getStream();		}
};


PX_INLINE Aggregate::Aggregate(PxAggregate* px, PxU32 maxActors, bool selfCollision) :
	mPxAggregate	(px),
	mCompoundID		(PX_INVALID_U32),
	mMaxNbActors	(maxActors),
	mSelfCollide	(selfCollision)
{
	setScbType(ScbType::AGGREGATE);
}


PX_INLINE Aggregate::~Aggregate()
{
}

PX_FORCE_INLINE bool Aggregate::isBufferingSpecial(ControlState::Enum state) const
{
	// A special version of the buffer check is needed for aggregates because it is not fine for adding/removing 
	// an actor to be not double buffered if the aggregate is pending for insertion.
	// For example: Adding an actor can not be processed if the aggregate is pending for insertion because the compoundID
	//              is not yet available (an there is no Sc::Aggregate object to store the actors)

	Scb::Scene* scbScene = getScbSceneForAPI();
	return state == ControlState::eREMOVE_PENDING || // pending remove not possible if not buffered
		(scbScene && scbScene->isPhysicsBuffering());
}


//--------------------------------------------------------------
//
// Data synchronization
//
//--------------------------------------------------------------

PX_INLINE void Aggregate::syncState(Scb::Scene& scene)
{
	PxU32 flags = getBufferFlags();
	if (flags)
	{
		const Scb::AggregateBuffer* PX_RESTRICT bufferedData = getBufferedData();

		if (flags & BF_ADD_ACTOR)
		{
			Scb::Actor* const* actorBuffer = scene.getActorBuffer(bufferedData->addBufferIdx);

			PX_ASSERT(mCompoundID != PX_INVALID_U32);
			for(PxU32 i=0; i < bufferedData->addCount; i++)
			{
				actorBuffer[i]->getActorCore().mCompoundID = mCompoundID;
			}
		}

		if (flags & BF_REMOVE_ACTOR)
		{
			Scb::Actor* const* actorBuffer = scene.getActorBuffer(bufferedData->removeBufferIdx);

			for(PxU32 i=0; i < bufferedData->removeCount; i++)
			{
				const ControlState::Enum state = actorBuffer[i]->getControlState();

				Sc::ActorCore& ac = actorBuffer[i]->getActorCore();
				ac.mCompoundID = PX_INVALID_U32;
				if (state == ControlState::eINSERT_PENDING || state == ControlState::eIN_SCENE)
					ac.reinsertShapes();
			}
		}
	}

	postSyncState();
}

}  // namespace Scb

}

#endif
