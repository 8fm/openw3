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

#ifndef PAIR_FILTER_H
#define PAIR_FILTER_H

#include "PxSimpleTypes.h"
#include "PsUserAllocated.h"
#include "PsHashMap.h"

#ifndef NULL
#define NULL 0
#endif

namespace physx
{

using namespace shdfnd;

class userPair;

#ifdef PX_X64
typedef PxU64 PxSizeT;
#else
typedef PxU32 PxSizeT;
#endif

// this is a helper class which can convert a pointer address into a unique index
typedef HashMap< PxSizeT, PxU32 > AddressToIndexMap;

class AddressToIndex : public UserAllocated
{
public:
	AddressToIndex(void)
	{
		mFreeList = NULL;
		mFreePool = NULL;
		mFreePoolCount = 16;
		mBaseFreePool = NULL;
	}

	~AddressToIndex(void)
	{
		PX_FREE(mBaseFreePool);
	}

	bool findIndex(const void *ptr,PxU32 &index) // returns index, only if it already exists
	{
		bool ret = false;
#ifdef PX_X64
		PxSizeT i = (PxSizeT)ptr;
		const AddressToIndexMap::Entry *found = mAddressToIndex.find(i);
		if ( found != NULL )
		{
			index = (*found).second;
			ret = true;
		}
#else
		ret = true;
		index = (PxU32)ptr;
#endif
		return ret;
	}

	PxU32	getIndex(const void *ptr)
	{
		PxU32 ret;
#ifdef PX_X64
		PxSizeT i = (PxSizeT)ptr;
		const AddressToIndexMap::Entry *found = mAddressToIndex.find(i);
		if ( found == NULL )
		{
			if ( mFreeList )
			{
				ret = mFreeList->mIndex;
				mFreeList = mFreeList->mNext;
			}
			else
			{
				PX_ASSERT( mNextIndex != PX_MAX_U32 );
				ret = mNextIndex++;
			}
			mAddressToIndex[i] = ret;
		}
		else
		{
			ret = (*found).second;
		}
#else
		ret = (PxU32)ptr;
#endif
		return ret;
	}

	bool	freeIndex(const void *ptr)
	{
		bool ret = false;
#ifdef PX_X64
		PxSizeT i = (PxSizeT)ptr;
		const AddressToIndexMap::Entry *found = mAddressToIndex.find(i);
		if ( found != NULL )
		{
			PxU32 index = (*found).second;
			if ( !mFreePool )
			{
				growFreePool();
			}
			IndexFree *nextFree = mFreeList;
			mFreeList = mFreePool;
			mFreePool = mFreePool->mNext;
			mFreeList->mNext = nextFree;
			mFreeList->mIndex = index;
			mAddressToIndex.erase(i);
			ret = true;
		}
#else
		PX_UNUSED(ptr);
		ret = true;
#endif
		return ret;
	}

	void growFreePool(void)
	{
		mFreePoolCount = mFreePoolCount*2;
		IndexFree *newPool = (IndexFree *)PX_ALLOC(sizeof(IndexFree)*mFreePoolCount,"PairFilter:IndexFree");

		// Initialize the linked list...
		IndexFree *scan = newPool;
		for (PxU32 i=0; i<(mFreePoolCount-1); i++)
		{
			scan->mNext = scan+1;
			scan++;
		}
		scan->mNext = NULL;
		if ( mBaseFreePool ) // if this isn't the very first time..
		{
			IndexFree	*dest = newPool;
			scan = mFreeList;
			while ( scan )
			{
				dest->mIndex = scan->mIndex;
				if ( scan->mNext == NULL )
				{
					dest->mNext = NULL;
				}
				scan = scan->mNext;
				dest++;
			}
			mFreePool = dest; // head of the new free pool
			mFreeList = newPool;
			PX_FREE(mBaseFreePool);
		}
		mBaseFreePool = newPool;
	}

private:
	class IndexFree
	{
	public:
		IndexFree	*mNext;
		PxU32		mIndex;
	};
	PxU32				mNextIndex;
	PxU32				mFreePoolCount;
	AddressToIndexMap	mAddressToIndex;
	IndexFree			*mFreeList;
	IndexFree			*mFreePool;
	IndexFree			*mBaseFreePool;
};



class PairFilter : public UserAllocated
{
public:

	typedef HashMap< PxU64, PxU64 > PairMap;

	PairFilter(void)
	{

	}

	~PairFilter(void)
	{

	}

	void	purge(void)
	{
		mPairMap.clear();
	}

	PX_INLINE PxU64 getIndex(PxU32 id0,PxU32 id1) const
	{
		PxU64 index;
		PxU32 *t = (PxU32 *)&index;
		t[0] = id0;
		t[1] = id1;
		return index;
	}

	void	addPair(PxU32 id0, PxU32 id1)
	{
		PxU64 index = getIndex(id0,id1);
		mPairMap[index] = index;
	}

	bool	removePair(PxU32 id0, PxU32 id1)
	{
		bool ret = false;
		PxU64 index = getIndex(id0,id1);
		const PairMap::Entry *found = mPairMap.find(index);
		if ( found )
		{
			ret = true;
			mPairMap.erase(index);
		}
		return ret;
	}

	bool	findPair(PxU32 id0,PxU32 id1)	const
	{
		PxU64 index = getIndex(id0,id1);
		const PairMap::Entry *found = mPairMap.find(index);
		return found ? true : false;
	}


private:
	PairMap		mPairMap;
};



};// end of namesapace

#endif
