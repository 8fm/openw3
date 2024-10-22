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
#include "IceContainer.h"

/*
mGrowthFactor > 0.0			normal container, resize enabled, owns memory
mGrowthFactor < 0.0			local container, resize enabled, doesn't own memory
mGrowthFactor = INVALID_ID	static container, resize disabled, doesn't own memory
*/

using namespace physx;
using namespace Gu;

#define INITIAL_GROWTH_FACTOR	2.0f

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor. No entries allocated there.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Container::Container() : mMaxNbEntries(0), mCurNbEntries(0), mEntries(NULL), mGrowthFactor(INITIAL_GROWTH_FACTOR)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor. Also allocates a given number of entries.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Container::Container(PxU32 size, float growth_factor) : mMaxNbEntries(0), mCurNbEntries(0), mEntries(NULL)
{
	SetGrowthFactor(growth_factor);
	SetSize(size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Copy constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Container::Container(const Container& object) : mMaxNbEntries(0), mCurNbEntries(0), mEntries(NULL), mGrowthFactor(INITIAL_GROWTH_FACTOR)
{
	*this = object;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Destructor.	Frees everything and leaves.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Container::~Container()
{
	Empty();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Checks whether the container already contains a given value.
 *	\param		entry			[in] the value to look for in the container
 *	\param		location		[out] a possible pointer to store the entry location
 *	\see		Add(PxU32 entry)
 *	\see		Add(float entry)
 *	\see		Empty()
 *	\return		true if the value has been found in the container, else false.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Container::Contains(PxU32 entry, PxU32* location) const
{
	// Look for the entry
	for(PxU32 i=0;i<mCurNbEntries;i++)
	{
		if(mEntries[i]==entry)
		{
			if(location)	*location = i;
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Deletes an entry. If the container contains such an entry, it's removed.
 *	\param		entry		[in] the value to delete.
 *	\return		true if the value has been found in the container, else false.
 *	\warning	This method is arbitrary slow (O(n)) and should be used carefully. Insertion order is not preserved.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Container::Delete(PxU32 entry)
{
	// Look for the entry
	for(PxU32 i=0;i<mCurNbEntries;i++)
	{
		if(mEntries[i]==entry)
		{
			// Entry has been found at index i. The strategy is to copy the last current entry at index i, and decrement the current number of entries.
			DeleteIndex(i);
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Deletes an entry, preserving the insertion order. If the container contains such an entry, it's removed.
 *	\param		entry		[in] the value to delete.
 *	\return		true if the value has been found in the container, else false.
 *	\warning	This method is arbitrary slow (O(n)) and should be used carefully.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Container::DeleteKeepingOrder(PxU32 entry)
{
	// Look for the entry
	for(PxU32 i=0;i<mCurNbEntries;i++)
	{
		if(mEntries[i]==entry)
		{
			// Entry has been found at index i.
			// Shift entries to preserve order. You really should use a linked list instead.
			mCurNbEntries--;
			for(PxU32 j=i;j<mCurNbEntries;j++)
			{
				mEntries[j] = mEntries[j+1];
			}
			return true;
		}
	}
	return false;
}

//! Operator for "Container A = Container B"
void Container::operator=(const Container& object)
{
	SetSize(object.GetNbEntries());
	PxMemCopy(mEntries, object.GetEntries(), mMaxNbEntries*sizeof(PxU32));
	mCurNbEntries = mMaxNbEntries;
}
