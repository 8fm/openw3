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

#ifndef APEX_CONTEXT_H
#define APEX_CONTEXT_H

#include "PsShare.h"
#include "NxApexContext.h"
#include "PsMutex.h"
#include "PsArray.h"
#include "PsUserAllocated.h"

namespace physx
{
namespace apex
{

class ApexActor;
class ApexRenderableIterator;

class ApexContext
{
public:
	ApexContext() : mIterator(NULL) {}
	virtual ~ApexContext();

	virtual physx::PxU32	addActor(ApexActor& actor, ApexActor* actorPtr = NULL);
	virtual void	callContextCreationCallbacks(ApexActor* actorPtr);
	virtual void	callContextDeletionCallbacks(ApexActor* actorPtr);
	virtual void	removeActorAtIndex(physx::PxU32 index);

	void			removeAllActors();
	NxApexRenderableIterator* createRenderableIterator();
	void			releaseRenderableIterator(NxApexRenderableIterator&);

protected:
	physx::Array<ApexActor*> mActorArray;
	physx::Array<ApexActor*> mActorArrayCallBacks;
	physx::ReadWriteLock	mActorListLock;
	ApexRenderableIterator* mIterator;

	friend class ApexRenderableIterator;
	friend class ApexActor;
};

class ApexRenderableIterator : public NxApexRenderableIterator, public physx::UserAllocated
{
public:
	NxApexRenderable* getFirst();
	NxApexRenderable* getNext();
	void			  reset();
	void			  release();

protected:
	void			  destroy();
	ApexRenderableIterator(ApexContext&);
	virtual ~ApexRenderableIterator() {}
	void			  removeCachedActor(ApexActor&);

	ApexContext*	  ctx;
	physx::PxU32             curActor;
	ApexActor*        mLockedActor;
	physx::Array<ApexActor*> mCachedActors;
	physx::Array<ApexActor*> mSkippedActors;

	friend class ApexContext;
};

}
} // end namespace physx::apex

#endif // APEX_CONTEXT_H