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

#include "NxApex.h"
#include "ApexContext.h"
#include "ApexActor.h"

namespace physx
{
namespace apex
{

//*** Apex Context ***
ApexContext::~ApexContext()
{
	if (mIterator)
	{
		mIterator->release();
	}
	removeAllActors();
	mActorArray.clear();
	mActorArrayCallBacks.clear();
}

physx::PxU32 ApexContext::addActor(ApexActor& actor, ApexActor* actorPtr)
{
	mActorListLock.lockWriter();
	physx::PxU32 index = mActorArray.size();
	mActorArray.pushBack(&actor);
	if (actorPtr != NULL)
	{
		mActorArrayCallBacks.pushBack(actorPtr);
	}
	callContextCreationCallbacks(&actor);
	mActorListLock.unlockWriter();
	return index;
}

void ApexContext::callContextCreationCallbacks(ApexActor* actorPtr)
{
	NxApexAsset*	assetPtr;
	NxAuthObjTypeID	ObjectTypeID;
	physx::PxU32	numCallBackActors;

	numCallBackActors = mActorArrayCallBacks.size();
	for (physx::PxU32 i = 0; i < numCallBackActors; i++)
	{
		assetPtr = actorPtr->getNxApexAsset();
		if (assetPtr != NULL)
		{
			// get the resIds
			ObjectTypeID	= assetPtr->getObjTypeID();
			// call the call back function
			mActorArrayCallBacks[i]->ContextActorCreationNotification(ObjectTypeID,
			        actorPtr);
		}
	}
}

void ApexContext::callContextDeletionCallbacks(ApexActor* actorPtr)
{
	NxApexAsset*	assetPtr;
	NxAuthObjTypeID	ObjectTypeID;
	PxU32			numCallBackActors;

	numCallBackActors = mActorArrayCallBacks.size();
	for (physx::PxU32 i = 0; i < numCallBackActors; i++)
	{
		assetPtr = actorPtr->getNxApexAsset();
		if (assetPtr != NULL)
		{
			// get the resIds
			ObjectTypeID	= assetPtr->getObjTypeID();
			// call the call back function
			mActorArrayCallBacks[i]->ContextActorDeletionNotification(ObjectTypeID,
			        actorPtr);
		}
	}

}

void ApexContext::removeActorAtIndex(physx::PxU32 index)
{
	ApexActor*	actorPtr;

	// call the callbacks so they know this actor is going to be deleted!
	callContextDeletionCallbacks(mActorArray[index]);

	mActorListLock.lockWriter();

	// remove the actor from the call back array if it is in it
	actorPtr = mActorArray[index];

	for (physx::PxU32 i = 0; i < mActorArrayCallBacks.size(); i++)
	{
		if (actorPtr == mActorArrayCallBacks[i])			// is this the actor to be removed?
		{
			mActorArrayCallBacks.replaceWithLast(i);	// yes, remove it
		}
	}

	if (mIterator)
	{
		NxApexRenderable* renderable = mActorArray[ index ]->getRenderable();
		if (renderable)
		{
			mIterator->removeCachedActor(*(mActorArray[ index ]));
		}
	}
	mActorArray.replaceWithLast(index);
	if (index < mActorArray.size())
	{
		mActorArray[index]->updateIndex(*this, index);
	}
	mActorListLock.unlockWriter();
}

void ApexContext::removeAllActors()
{
	while (mActorArray.size())
	{
		mActorArray.back()->release();
	}
	mActorArrayCallBacks.clear();
}

NxApexRenderableIterator* ApexContext::createRenderableIterator()
{
	if (mIterator)
	{
		PX_ALWAYS_ASSERT(); // Only one per context at a time, please
		return NULL;
	}
	else
	{
		mIterator = PX_NEW(ApexRenderableIterator)(*this);
		return mIterator;
	}
}
void ApexContext::releaseRenderableIterator(NxApexRenderableIterator& iter)
{
	if (mIterator == DYNAMIC_CAST(ApexRenderableIterator*)(&iter))
	{
		mIterator->destroy();
		mIterator = NULL;
	}
	else
	{
		PX_ASSERT(mIterator == DYNAMIC_CAST(ApexRenderableIterator*)(&iter));
	}
}

ApexRenderableIterator::ApexRenderableIterator(ApexContext& _ctx) :
	ctx(&_ctx),
	curActor(0),
	mLockedActor(NULL)
{
	// Make copy of list of renderable actors currently in the context.
	// If an actor is later removed, we mark it as NULL in our cached
	// array.  If an actor is added, we do _NOT_ add it to our list since
	// it would be quite dangerous to call dispatchRenderResources() on an
	// actor that has never had updateRenderResources() called to it.

	mCachedActors.reserve(ctx->mActorArray.size());
	ctx->mActorListLock.lockWriter();
	for (physx::PxU32 i = 0 ; i < ctx->mActorArray.size() ; i++)
	{
		NxApexRenderable* renderable = ctx->mActorArray[ i ]->getRenderable();
		if (renderable)
		{
			mCachedActors.pushBack(ctx->mActorArray[ i ]);
		}
	}
	ctx->mActorListLock.unlockWriter();
}

void ApexRenderableIterator::removeCachedActor(ApexActor& actor)
{
	// This function is called with a locked context, so we can modify our
	// internal lists at will.

	for (physx::PxU32 i = 0 ; i < mCachedActors.size() ; i++)
	{
		if (&actor == mCachedActors[ i ])
		{
			mCachedActors[ i ] = NULL;
			break;
		}
	}
	for (physx::PxU32 i = 0 ; i < mSkippedActors.size() ; i++)
	{
		if (&actor == mSkippedActors[ i ])
		{
			mSkippedActors.replaceWithLast(i);
			break;
		}
	}
	if (&actor == mLockedActor)
	{
		mLockedActor->renderDataUnLock();
		mLockedActor = NULL;
	}
}

NxApexRenderable* ApexRenderableIterator::getFirst()
{
	curActor = 0;
	mSkippedActors.reserve(mCachedActors.size());
	return getNext();
}

NxApexRenderable*  ApexRenderableIterator::getNext()
{
	if (mLockedActor)
	{
		mLockedActor->renderDataUnLock();
		mLockedActor = NULL;
	}

	physx::ScopedReadLock scopedContextLock( ctx->mActorListLock );
	while (curActor < mCachedActors.size())
	{
		ApexActor* actor = mCachedActors[ curActor++ ];
		if (actor)
		{
			if (actor->renderDataTryLock())
			{
				mLockedActor = actor;
				return actor->getRenderable();
			}
			else
			{
				mSkippedActors.pushBack(actor);
			}
		}
	}
	if (mSkippedActors.size())
	{
		ApexActor* actor = mSkippedActors.back();
		mSkippedActors.popBack();
		actor->renderDataLock();
		mLockedActor = actor;
		return actor->getRenderable();
	}
	return NULL;
}

void ApexRenderableIterator::release()
{
	ctx->releaseRenderableIterator(*this);
}

void ApexRenderableIterator::reset()
{
	if (mLockedActor)
	{
		mLockedActor->renderDataUnLock();
		mLockedActor = NULL;
	}

	curActor = 0;

	mCachedActors.reserve(ctx->mActorArray.size());
	mCachedActors.reset();
	ctx->mActorListLock.lockWriter();
	for (physx::PxU32 i = 0 ; i < ctx->mActorArray.size() ; i++)
	{
		NxApexRenderable* renderable = ctx->mActorArray[ i ]->getRenderable();
		if (renderable)
		{
			mCachedActors.pushBack(ctx->mActorArray[ i ]);
		}
	}
	ctx->mActorListLock.unlockWriter();
}

void ApexRenderableIterator::destroy()
{
	if (mLockedActor)
	{
		mLockedActor->renderDataUnLock();
		mLockedActor = NULL;
	}
	delete this;
}

}
} // end namespace physx::apex

