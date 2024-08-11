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


#ifndef PX_COLLISION_ACTORPAIR
#define PX_COLLISION_ACTORPAIR

#include "ScRigidSim.h"
#include "ScContactStream.h"
#include "ScNPhaseCore.h"

namespace physx
{
namespace Sc
{

	class ActorPair;

	class ActorPairContactReportData
	{
	public:
		ActorPairContactReportData() : 
			mStrmResetStamp						(0xffffffff),
			mActorAID							(0xffffffff),
			mActorBID							(0xffffffff),
			mPxActorA							(NULL),
			mPxActorB							(NULL),
			mActorAClientID						(0xff),
			mActorBClientID						(0xff),
			mActorAClientBehavior				(0),
			mActorBClientBehavior				(0)
			{}

		ContactStreamManager	mContactStreamManager;
		PxU32					mStrmResetStamp;
		PxU32					mActorAID;
		PxU32					mActorBID;
		PxActor*				mPxActorA;
		PxActor*				mPxActorB;
		PxClientID				mActorAClientID;
		PxClientID				mActorBClientID;
		PxU8					mActorAClientBehavior;
		PxU8					mActorBClientBehavior;
	};

	// Class shared by all SIPs or SIP markers for a pair of actors
	class ActorPair
	{
	public:

		enum InternalFlags
		{
			IS_IN_CONTACT_REPORT_ACTOR_PAIR_SET	= (1<<0),	// PT: whether the pair is already stored in the 'ContactReportActorPairSet' or not
			THIS_FRAME_FORCE_THRESHOLD_EXCEEDED	= (1<<1),	// PT: stored in ActorPair instead of ActorPairContactReportData to avoid L2s
			LAST_FRAME_FORCE_THRESHOLD_EXCEEDED	= (1<<2),	// PT: stored in ActorPair instead of ActorPairContactReportData to avoid L2s
		};

		PX_INLINE						ActorPair(RigidSim& actor0, RigidSim& actor1);
		PX_INLINE						~ActorPair();

		PX_INLINE ContactStreamManager&	getContactStreamManager()							{ return getContactReportData()->mContactStreamManager;							}
		PX_FORCE_INLINE	void			incRefCount()										{ ++mRefCount; PX_ASSERT(mRefCount>0);											}
		PX_FORCE_INLINE	PxU32			decRefCount()										{ PX_ASSERT(mRefCount>0); return --mRefCount;									}
		PX_FORCE_INLINE	PxU32			getRefCount()								const	{ return mRefCount;																}
		PX_FORCE_INLINE	RigidSim&		getActorA()									const	{ return mActorA;																}
		PX_FORCE_INLINE	RigidSim&		getActorB()									const	{ return mActorB;																}
		PX_INLINE		PxU32			getActorAID()								const	{ PX_ASSERT(mReportData); return getContactReportData()->mActorAID;				}
		PX_INLINE		PxU32			getActorBID()								const	{ PX_ASSERT(mReportData); return getContactReportData()->mActorBID;				}
		PX_INLINE		PxActor*		getPxActorA()								const	{ PX_ASSERT(mReportData); return getContactReportData()->mPxActorA;				}
		PX_INLINE		PxActor*		getPxActorB()								const	{ PX_ASSERT(mReportData); return getContactReportData()->mPxActorB;				}
		PX_INLINE		PxClientID		getActorAClientID()							const	{ PX_ASSERT(mReportData); return getContactReportData()->mActorAClientID;		}
		PX_INLINE		PxClientID		getActorBClientID()							const	{ PX_ASSERT(mReportData); return getContactReportData()->mActorBClientID;		}
		PX_INLINE		PxU8			getActorAClientBehavior()					const	{ PX_ASSERT(mReportData); return getContactReportData()->mActorAClientBehavior;	}
		PX_INLINE		PxU8			getActorBClientBehavior()					const	{ PX_ASSERT(mReportData); return getContactReportData()->mActorBClientBehavior;	}
		PX_INLINE		bool			streamResetStamp(PxU32 cmpStamp);
		PX_FORCE_INLINE	bool			streamCompareStamp(PxU32 cmpStamp);

		PX_FORCE_INLINE	PxU16			isInContactReportActorPairSet()				const	{ return mInternalFlags & IS_IN_CONTACT_REPORT_ACTOR_PAIR_SET;						}
		PX_FORCE_INLINE	void			setInContactReportActorPairSet()					{ mInternalFlags |= IS_IN_CONTACT_REPORT_ACTOR_PAIR_SET;							}
		PX_FORCE_INLINE	void			clearInContactReportActorPairSet()					{ mInternalFlags &= ~IS_IN_CONTACT_REPORT_ACTOR_PAIR_SET;							}

		PX_INLINE		void			incTouchCount();
		PX_INLINE		void			decTouchCount();
		PX_FORCE_INLINE	PxU32			getTouchCount()						const	{ return mTouchCount;	}

	private:
		ActorPair& operator=(const ActorPair&);

		PX_FORCE_INLINE	ActorPairContactReportData* getContactReportData() const;
						void			createContactReportData() const;
						void			releaseContactReportData() const;
						RigidSim&		mActorA;
						RigidSim&		mActorB;
						Scene&			mScene;  // Can't take the scene reference from the actors since they're already gone on scene release

						PxU16			mInternalFlags;

						PxU16			mTouchCount;
						PxU16			mRefCount;

		mutable	ActorPairContactReportData* mReportData;
	};

} // namespace Sc

Sc::ActorPair::ActorPair(RigidSim& actor0, RigidSim& actor1) : 
mActorA			(actor0), 
mActorB			(actor1),
mScene			(actor0.getScene()),
mInternalFlags	(0),
mTouchCount		(0), 
mRefCount		(0), 
mReportData		(NULL)
{ 
}


Sc::ActorPair::~ActorPair()
{
	if (mReportData != NULL) 
		releaseContactReportData();
}


PX_INLINE bool Sc::ActorPair::streamResetStamp(PxU32 cmpStamp) 
{ 
	bool ret = streamCompareStamp(cmpStamp); 
	getContactReportData()->mStrmResetStamp = cmpStamp; 
	return ret; 
}


PX_FORCE_INLINE bool Sc::ActorPair::streamCompareStamp(PxU32 cmpStamp)
{
	return (cmpStamp != getContactReportData()->mStrmResetStamp);
}


PX_INLINE void Sc::ActorPair::incTouchCount()
{
	++mTouchCount;
	PX_ASSERT(mTouchCount>0);
}


PX_INLINE void Sc::ActorPair::decTouchCount()
{
	PX_ASSERT(mTouchCount>0);
	--mTouchCount;
}


PX_FORCE_INLINE Sc::ActorPairContactReportData* Sc::ActorPair::getContactReportData() const 
{ 
	// Lazy create report data
	if(!mReportData)
		createContactReportData();

	return mReportData; 
}

}

#endif
