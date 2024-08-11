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


#include "CmPtrTable.h"
#include "PxMemory.h"
#include "PxAssert.h"
#include "PsAllocator.h"
#include "PsAlloca.h"
#include "PsUtilities.h"
#include "PsIntrinsics.h"
#include "CmUtils.h"

using namespace physx;
using namespace Cm;

void PtrTable::clear()
{
	if(mCount > 1 && mOwnsMemory)
	{
		PX_ASSERT(mList);
		PX_DELETE_POD(mList);
	}
	mCount = 0;
	mList = NULL;
}

void PtrTable::setPtrs(void** ptrs, PxU32 count)
{
	if (mCount>1 && PxU32(mCount)!=count && mOwnsMemory)
		PX_DELETE_POD(mList); 

	if(count > 1)
	{
		mBufferUsed = false;
		if(PxU32(mCount) != count)
		{
			mList = (void**)PX_ALLOC(sizeof(void*)*count, PX_DEBUG_EXP("void*"));
			mOwnsMemory = true;
		}
		PxMemCopy(mList, ptrs, count*sizeof(void*));
	}
	else
	{
		mBufferUsed = true;
		mSingle = count ? ptrs[0] : 0;
	}
	mCount = Ps::to16(count);
}

void PtrTable::addPtr(void* ptr)
{
	if(mCount)
	{
		void*const* currentPtrs = getPtrs();
		const PxU32 newCount = mCount+1;
		PX_ALLOCA(ptrs, void*, newCount);
		PxMemCopy(ptrs, currentPtrs, mCount*sizeof(void*));
		ptrs[mCount] = ptr;
		setPtrs(ptrs, newCount);
	}
	else
	{
		setPtrs(&ptr, 1);
	}
}

PxU32 PtrTable::find(const void* ptr) const
{
	const PxU32 nbPtrs = mCount;
	void*const * PX_RESTRICT ptrs = mCount == 1 ? &mSingle : mList;

	for(PxU32 i=0; i<nbPtrs; i++)
	{
		if(ptrs[i] == ptr)
			return i;
	}
		
	PX_ASSERT(0);
	return 0xffffffff;
}

void PtrTable::remove(PxU32 index)
{
	PX_ASSERT(index<mCount);
	const PxU32 nbPtrs = mCount;
	void** PX_RESTRICT ptrs = mCount == 1 ? &mSingle : mList;

	if(nbPtrs>2)
	{	
		ptrs[index] = ptrs[--mCount];		// PT: we were using the list and we'll keep using the list
	}
	else if(nbPtrs==2)
	{
		PX_ASSERT(index==0 || index==1);	// PT: we were using the list and we'll now use the single
		void* lastPtr = ptrs[1-index];
		setPtrs(&lastPtr, 1);
	}
	else
	{
		PX_ASSERT(nbPtrs==1);			// PT: we were using the single and we'll now be empty
		clear();
	}		
}



bool PtrTable::findAndRemovePtr(void* ptr)
{
	PxU32 index = find(ptr);
	if(index == 0xffffffff)
		return false;
	remove(index);
	return true;
}

void PtrTable::exportExtraData(PxSerializationContext& stream)
{
	if(mCount>1)
	{
		stream.alignData(PX_SERIAL_ALIGN);
		stream.writeData(mList, sizeof(void*)*mCount);
	}
}

void PtrTable::importExtraData(PxDeserializationContext& context)
{
	if(mCount>1)
		mList = context.readExtraData<void*, PX_SERIAL_ALIGN>(mCount);
}
