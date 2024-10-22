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

#ifndef __APEX_CONSTRAINED_DISTRIBUTOR_H__
#define __APEX_CONSTRAINED_DISTRIBUTOR_H__

#include "NxApex.h"
#include "PsUserAllocated.h"

namespace physx
{
namespace apex
{

template <typename T = PxU32>
class ApexConstrainedDistributor
{
public:
	ApexConstrainedDistributor()
	{
	}

	PX_INLINE void resize(PxU32 size)
	{
		mConstraintDataArray.resize(size);
	}
	PX_INLINE void setBenefit(PxU32 index, PxF32 benefit)
	{
		PX_ASSERT(index < mConstraintDataArray.size());
		mConstraintDataArray[index].benefit = benefit;
	}
	PX_INLINE void setTargetValue(PxU32 index, T targetValue)
	{
		PX_ASSERT(index < mConstraintDataArray.size());
		mConstraintDataArray[index].targetValue = targetValue;
	}
	PX_INLINE T getResultValue(PxU32 index) const
	{
		PX_ASSERT(index < mConstraintDataArray.size());
		return mConstraintDataArray[index].resultValue;
	}

	void solve(T totalValueLimit)
	{
		PxU32 size = mConstraintDataArray.size();
		if (size == 0)
		{
			return;
		}
		if (size == 1)
		{
			ConstraintData& data = mConstraintDataArray.front();
			data.resultValue = PxMin(data.targetValue, totalValueLimit);
			return;
		}

		PxF32 totalBenefit = 0;
		T totalValue = 0;
		for (PxU32 i = 0; i < size; i++)
		{
			ConstraintData& data = mConstraintDataArray[i];

			totalBenefit += data.benefit;
			totalValue += data.targetValue;

			data.resultValue = data.targetValue;
		}
		if (totalValue <= totalValueLimit)
		{
			//resultValue was setted in prev. for-scope
			return;
		}

		mConstraintSortPairArray.resize(size);
		for (PxU32 i = 0; i < size; i++)
		{
			ConstraintData& data = mConstraintDataArray[i];

			data.weight = (totalValueLimit * data.benefit / totalBenefit);
			if (data.weight > 0)
			{
				mConstraintSortPairArray[i].key = (data.targetValue / data.weight);
			}
			else
			{
				mConstraintSortPairArray[i].key = FLT_MAX;
				data.resultValue = 0; //reset resultValue
			}
			mConstraintSortPairArray[i].index = i;
		}

		shdfnd::sort(mConstraintSortPairArray.begin(), size, ConstraintSortPredicate());

		for (PxU32 k = 0; k < size; k++)
		{
			PxF32 firstKey = mConstraintSortPairArray[k].key;
			if (firstKey == FLT_MAX)
			{
				break;
			}
			ConstraintData& firstData = mConstraintDataArray[mConstraintSortPairArray[k].index];

			//special case when k == i
			PxF32 sumWeight = firstData.weight;
			T sum = firstData.targetValue;
			for (PxU32 i = k + 1; i < size; i++)
			{
				const ConstraintData& data = mConstraintDataArray[mConstraintSortPairArray[i].index];

				sumWeight += data.weight;
				const T value = static_cast<T>(firstKey * data.weight);
				PX_ASSERT(value <= data.targetValue);
				sum += value;
			}

			if (sum > totalValueLimit)
			{
				for (PxU32 i = k; i < size; i++)
				{
					ConstraintData& data = mConstraintDataArray[mConstraintSortPairArray[i].index];

					const T value = static_cast<T>(totalValueLimit * data.weight / sumWeight);
					PX_ASSERT(value <= data.targetValue);
					data.resultValue = value;
				}
				break;
			}
			//allready here: firstData.resultData = firstData.targetValue
			totalValueLimit -= firstData.targetValue;
		}
	}

private:
	struct ConstraintData
	{
		PxF32	benefit;     //input benefit
		T		targetValue; //input constraint on value
		PxF32	weight;      //temp
		T		resultValue; //output
	};
	struct ConstraintSortPair
	{
		PxF32 key;
		PxU32 index;
	};
	class ConstraintSortPredicate
	{
	public:
		PX_INLINE bool operator()(const ConstraintSortPair& a, const ConstraintSortPair& b) const
		{
			return a.key < b.key;
		}
	};

	physx::Array<ConstraintData>		mConstraintDataArray;
	physx::Array<ConstraintSortPair>	mConstraintSortPairArray;
};

}
} // end namespace physx::apex

#endif
