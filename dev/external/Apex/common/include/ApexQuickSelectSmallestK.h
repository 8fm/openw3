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

#ifndef APEX_QUICK_SELECT_SMALLEST_K_H
#define APEX_QUICK_SELECT_SMALLEST_K_H

namespace physx
{
namespace apex
{
//A variant of quick sort to move the smallest k members of a sequence to its start.
//Does much less work than a full sort.

template<class Sortable, class Predicate>
PX_INLINE void ApexQuickSelectSmallestK(Sortable* start, Sortable* end, physx::PxU32 k, const Predicate& p = Predicate())
{
	Sortable* origStart = start;
	Sortable* i;
	Sortable* j;
	Sortable m;

	for (;;)
	{
		i = start;
		j = end;
		m = *(i + ((j - i) >> 1));

		while (i <= j)
		{
			while (p(*i, m))
			{
				i++;
			}
			while (p(m, *j))
			{
				j--;
			}
			if (i <= j)
			{
				if (i != j)
				{
					physx::swap(*i, *j);
				}
				i++;
				j--;
			}
		}



		if (start < j
		        && k + origStart - 1 < j)	//we now have found the (j - start+1) smallest.  we need to continue sorting these only if k < (j - start+1)
			//if we sort this we definitely won't need to sort the right hand side.
		{
			end = j;
		}
		else if (i < end
		         && k + origStart > i)	//only continue sorting these if left side is not larger than k.
			//we do this instead of recursing
		{
			start = i;
		}
		else
		{
			return;
		}
	}
}

} // namespace apex
} // namespace physx

#endif // APEX_QUICK_SELECT_SMALLEST_K_H
