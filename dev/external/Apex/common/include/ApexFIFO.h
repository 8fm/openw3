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

#ifndef __APEX_FIFO_H__
#define __APEX_FIFO_H__

#include "NxApex.h"
#include "PsUserAllocated.h"

namespace physx
{
namespace apex
{

template <typename T>
struct FIFOEntry
{
	T data;
	physx::PxU32 next;
	bool isValidEntry;
};

template<typename T>
class ApexFIFO : public physx::UserAllocated
{
public:
	ApexFIFO() : first((physx::PxU32) - 1), last((physx::PxU32) - 1), count(0) {}

	bool popFront(T& frontElement)
	{
		if (first == (physx::PxU32)-1)
		{
			return false;
		}

		PX_ASSERT(first < list.size());
		frontElement = list[first].data;

		if (first == last)
		{
			list.clear();
			first = (physx::PxU32) - 1;
			last = (physx::PxU32) - 1;
		}
		else
		{
			list[first].isValidEntry = false;

			if (list[last].next == (physx::PxU32)-1)
			{
				list[last].next = first;
			}
			first = list[first].next;
		}

		count--;
		return true;
	}


	void pushBack(const T& newElement)
	{
		if (list.size() == 0 || list[last].next == (physx::PxU32)-1)
		{
			FIFOEntry<T> newEntry;
			newEntry.data = newElement;
			newEntry.next = (physx::PxU32) - 1;
			newEntry.isValidEntry = true;
			list.pushBack(newEntry);

			if (first == (physx::PxU32) - 1)
			{
				PX_ASSERT(last == (physx::PxU32) - 1);
				first = list.size() - 1;
			}
			else
			{
				PX_ASSERT(last != (physx::PxU32) - 1);
				list[last].next = list.size() - 1;
			}

			last = list.size() - 1;
		}
		else
		{
			physx::PxU32 freeIndex = list[last].next;
			PX_ASSERT(freeIndex < list.size());

			FIFOEntry<T>& freeEntry = list[freeIndex];
			freeEntry.data = newElement;
			freeEntry.isValidEntry = true;

			if (freeEntry.next == first)
			{
				freeEntry.next = (physx::PxU32) - 1;
			}

			last = freeIndex;
		}
		count++;
	}

	physx::PxU32 size()
	{
		return count;
	}

private:
	physx::PxU32 first;
	physx::PxU32 last;
	physx::PxU32 count;
	physx::Array<FIFOEntry<T> > list;
};

}
} // end namespace physx::apex

#endif