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


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PxMemory.h"
#include "SqPruningPool.h"

using namespace physx;
using namespace Sq;
using namespace Cm;

#ifdef __SPU__
#undef PX_NEW
#undef PX_DELETE_AND_RESET

#define PX_NEW(a) (a*)((a*)NULL)
#define PX_DELETE_AND_RESET(a)
#endif

PruningPool::PruningPool()
:	mNbObjects(0)
,	mMaxNbObjects(0)
,	mWorldBoxes(NULL)
,	mObjects(NULL)
,	mHandleToIndex(0)
,	mIndexToHandle(0)
,	mFirstFreshHandle(0)
,	mHandleFreeList(INVALID_PRUNERHANDLE)
{}


#if !__SPU__
PruningPool::~PruningPool()
{
	PX_FREE_AND_RESET(mWorldBoxes);
	PX_FREE_AND_RESET(mObjects);
	PX_FREE_AND_RESET(mHandleToIndex);
	PX_FREE_AND_RESET(mIndexToHandle);
}
#endif




void PruningPool::resize(PxU32 newCapacity)
{
#ifndef __SPU__
	PxBounds3*		newBoxes			= (PxBounds3*)		PX_ALLOC(sizeof(PxBounds3)*newCapacity, PX_DEBUG_EXP("PxBounds3"));
	PrunerPayload*	newData				= (PrunerPayload*)	PX_ALLOC(sizeof(PrunerPayload)*newCapacity, PX_DEBUG_EXP("PrunerPayload*"));
	PxU32*			newIndexToHandle	= (PxU32*)			PX_ALLOC(sizeof(PxU32)*newCapacity, PX_DEBUG_EXP("Pruner Index Mapping"));
	PxU32*			newHandleToIndex	= (PxU32*)			PX_ALLOC(sizeof(PxU32)*newCapacity, PX_DEBUG_EXP("Pruner Index Mapping"));

	if( (NULL==newBoxes) || (NULL==newData) || (NULL==newIndexToHandle) || (NULL==newHandleToIndex) )
	{
		PX_FREE_AND_RESET(newBoxes);
		PX_FREE_AND_RESET(newData);
		PX_FREE_AND_RESET(newIndexToHandle);
		PX_FREE_AND_RESET(newHandleToIndex);
		return;
	}

	if(mWorldBoxes)		PxMemCopy(newBoxes, mWorldBoxes, mNbObjects*sizeof(PxBounds3));
	if(mObjects)		PxMemCopy(newData, mObjects, mNbObjects*sizeof(PrunerPayload));
	if(mIndexToHandle)	PxMemCopy(newIndexToHandle, mIndexToHandle, mNbObjects*sizeof(PxU32));
	if(mHandleToIndex)	PxMemCopy(newHandleToIndex, mHandleToIndex, mMaxNbObjects*sizeof(PxU32));

	mMaxNbObjects = newCapacity;

	PX_FREE_AND_RESET(mWorldBoxes);
	PX_FREE_AND_RESET(mObjects);
	PX_FREE_AND_RESET(mHandleToIndex);
	PX_FREE_AND_RESET(mIndexToHandle);

	mWorldBoxes		= newBoxes;
	mObjects		= newData;
	mHandleToIndex	= newHandleToIndex;
	mIndexToHandle	= newIndexToHandle;
#endif
}

void PruningPool::preallocate(PxU32 newCapacity)
{
	if(newCapacity>mMaxNbObjects)
		resize(newCapacity);
}

PrunerHandle PruningPool::addObject(const PxBounds3& worldAABB, const PrunerPayload& payload)
{
	if(mNbObjects==mMaxNbObjects)
		resize(PxMax<PxU32>(mMaxNbObjects*2, 64));

	if(mNbObjects==mMaxNbObjects)
		// i.e. allocation failed
		return INVALID_PRUNERHANDLE;

	const PxU32 index = mNbObjects++;
	mWorldBoxes[index] = worldAABB;
	mObjects[index] = payload;
	PxU32 handle;
	if(mHandleFreeList!=INVALID_PRUNERHANDLE)
	{
		handle = mHandleFreeList;
		mHandleFreeList = mHandleToIndex[handle];
	}
	else
		handle = mFirstFreshHandle++;

	mIndexToHandle[index] = handle;
	mHandleToIndex[handle] = index;
	return handle;
}

PxU32 PruningPool::removeObject(PrunerHandle h)
{
	PX_ASSERT(mNbObjects);

	PxU32 index = mHandleToIndex[h];

	const PxU32 lastIndex = --mNbObjects;
	if(lastIndex!=index)
	{
		PxU32 lastIndexHandle = mIndexToHandle[lastIndex];

		mWorldBoxes[index]				= mWorldBoxes[lastIndex];
		mObjects[index]					= mObjects[lastIndex];
		mIndexToHandle[index]			= lastIndexHandle;
		mHandleToIndex[lastIndexHandle] = index;
	}

	mHandleToIndex[h] = mHandleFreeList;
	mHandleFreeList = h;

	return lastIndex;
}


void PruningPool::shiftOrigin(const PxVec3& shift)
{
	for(PxU32 i=0; i < mNbObjects; i++)
	{
		mWorldBoxes[i].minimum -= shift;
		mWorldBoxes[i].maximum -= shift;
	}
}
