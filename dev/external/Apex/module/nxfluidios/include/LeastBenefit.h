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

#ifndef __LEAST_BENEFIT_H__
#define __LEAST_BENEFIT_H__

#include "PsShare.h"
#include "PsArray.h"

namespace physx
{
namespace apex
{
namespace nxfluidios
{

/* To use:

   list.reset( numToDelete );
   foreach item:  list.insert( index, benefit);

   After this loop, list.mostBenefit() returns the most
   benefit of the items on the delete list.

   You can then use list.popBack() to retrieve the IDs
   of the least benefit items.

   while( list.mCount )
   {
       release( id = list.popBack() );
   }

*/

template<typename ElemType> class LeastBenefitList
{
public:
	LeastBenefitList() : mCount(0) {};

	void reset(PxU32 size)
	{
		mCount = 0;
		mHead = mTail = INV;
		mElements.resize(size);
	}

	void insert(ElemType id, PxF32 benefit)
	{
		// trivial case #1 - empty list
		if (mCount == 0)
		{
			mHead = mTail = 0;
			data& d = mElements[ mCount++ ];
			d.benefit = benefit;
			d.id = id;
			d.next = INV;
			d.prev = INV;
			return;
		}

		// trivial case #2 - append at mTail
		if (benefit >= mElements[ mTail ].benefit)
		{
			if (mCount < mElements.size())
			{
				data& d = mElements[ mCount ];
				d.benefit = benefit;
				d.id = id;
				d.next = INV;
				d.prev = mTail;

				mElements[ mTail ].next = mCount;
				mTail = mCount;
				mCount++;
			}
			return;
		}

		// trivial case #3 - insert before mHead
		if (benefit < mElements[ mHead ].benefit)
		{
			PxU32 me;

			if (mElements.size() == 1)
			{
				// Special path for single element list
				// Just update element 0 (mHead, mTail)
				data& d = mElements[ 0 ];
				d.id = id;
				d.benefit = benefit;
				return;
			}
			else if (mCount == mElements.size())
			{
				// reuse mTail entry
				me = mTail;
				mTail = mElements[ mTail ].prev;
				mElements[ mTail ].next = INV;
			}
			else
			{
				me = mCount++;
			}

			// brand myself
			data& d = mElements[ me ];
			d.id = id;
			d.benefit = benefit;

			d.prev = INV;
			d.next = mHead;
			mElements[ mHead ].prev = me;
			mHead = me;
			return;
		}

		// insertion sort
		PxU32 cur = mElements[ mHead ].next;
		while (cur != INV)
		{
			if (benefit < mElements[ cur ].benefit)
			{
				PxU32 me;

				if (mCount == mElements.size())
				{
					// inserting before mTail == replace mTail
					if (cur == mTail)
					{
						mElements[ mTail ].id = id;
						mElements[ mTail ].benefit = benefit;
						return;
					}
					else
					{
						// prune mTail element, reuse it
						me = mTail;
						mTail = mElements[ mTail ].prev;
						mElements[ mTail ].next = INV;
					}
				}
				else
				{
					me = mCount++;
				}

				// brand myself
				data& d = mElements[ me ];
				d.id = id;
				d.benefit = benefit;

				// insert before cur
				d.next = cur;
				d.prev = mElements[ cur ].prev;
				mElements[ d.prev ].next = me;
				mElements[ cur ].prev = me;
				return;
			}

			cur = mElements[ cur ].next;
		}
	}


	// These two functions should only be called after
	// all insertions are complete
	PxF32 mostBenefit() const
	{
		if (mCount)
		{
			return mElements[ mTail ].benefit;
		}
		else
		{
			return 0.0f;
		}
	}


	ElemType	popBack()
	{
		if (mCount)
		{
			PX_ASSERT(mTail != INV);
			ElemType ret = mElements[ mTail ].id;
			mCount--;
			mTail = mElements[ mTail ].prev;
			return ret;
		}
		else
		{
			PX_ALWAYS_ASSERT();
			return ElemType();
		}
	}

	bool	validate() const
	{
#if _DEBUG
		if (mCount == 0)
		{
			return true;
		}
		PX_ASSERT(mHead < mElements.size());
		PX_ASSERT(mTail < mElements.size());
		PX_ASSERT(mCount <= mElements.size());
		PxU32 cur = mHead;
		PxF32 oldbenefit = 0.0f;
		for (PxU32 i = 0 ; i < mCount ; i++)
		{
			PX_ASSERT(mElements[ cur ].benefit >= oldbenefit);
			oldbenefit = mElements[ cur ].benefit;
			cur = mElements[ cur ].next;
		}
		PX_ASSERT(cur == INV);
		return true;
#else
		// Throw a fit if this is run in release mode
		return false;
#endif
	}

	struct data
	{
		ElemType id;
		PxF32 benefit;
		PxU32 next;
		PxU32 prev;
	};
	physx::Array<data>	mElements;
	PxU32               mHead, mTail;
	PxU32               mCount;

	static const PxU32 INV = 0xffffffffu;
};

}
}
} // namespace physx::apex

#endif // __LEAST_BENEFIT_H__
