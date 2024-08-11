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

#ifndef __APEX_GROUPS_FILTERING_H__
#define __APEX_GROUPS_FILTERING_H__

#include "NxApexDefs.h"

namespace physx
{
namespace apex
{

template <typename G>
class ApexGroupsFiltering
{
	typedef void (*FilterOp)(const G& mask0, const G& mask1, G& result);

	static void filterOp_AND(const G& mask0, const G& mask1, G& result)
	{
		result = (mask0 & mask1);
	}
	static void filterOp_OR(const G& mask0, const G& mask1, G& result)
	{
		result = (mask0 | mask1);
	}
	static void filterOp_XOR(const G& mask0, const G& mask1, G& result)
	{
		result = (mask0 ^ mask1);
	}
	static void filterOp_NAND(const G& mask0, const G& mask1, G& result)
	{
		result = ~(mask0 & mask1);
	}
	static void filterOp_NOR(const G& mask0, const G& mask1, G& result)
	{
		result = ~(mask0 | mask1);
	}
	static void filterOp_NXOR(const G& mask0, const G& mask1, G& result)
	{
		result = ~(mask0 ^ mask1);
	}
	static void filterOp_SWAP_AND(const G& mask0, const G& mask1, G& result)
	{
		result = SWAP_AND(mask0, mask1);
	}

	NxGroupsFilterOp::Enum	mFilterOp0, mFilterOp1, mFilterOp2;
	bool					mFilterBool;
	G						mFilterConstant0;
	G						mFilterConstant1;

public:
	ApexGroupsFiltering()
	{
		mFilterOp0 = mFilterOp1 = mFilterOp2 = NxGroupsFilterOp::AND;
		mFilterBool = false;
		setZero(mFilterConstant0);
		setZero(mFilterConstant1);
	}

	bool	setFilterOps(NxGroupsFilterOp::Enum op0, NxGroupsFilterOp::Enum op1, NxGroupsFilterOp::Enum op2)
	{
		if (mFilterOp0 != op0 || mFilterOp1 != op1 || mFilterOp2 != op2)
		{
			mFilterOp0 = op0;
			mFilterOp1 = op1;
			mFilterOp2 = op2;
			return true;
		}
		return false;
	}
	void	getFilterOps(NxGroupsFilterOp::Enum& op0, NxGroupsFilterOp::Enum& op1, NxGroupsFilterOp::Enum& op2) const
	{
		op0 = mFilterOp0;
		op1 = mFilterOp1;
		op2 = mFilterOp2;
	}

	bool	setFilterBool(bool flag)
	{
		if (mFilterBool != flag)
		{
			mFilterBool = flag;
			return true;
		}
		return false;
	}
	bool	getFilterBool() const
	{
		return mFilterBool;
	}

	bool	setFilterConstant0(const G& mask)
	{
		if (mFilterConstant0 != mask)
		{
			mFilterConstant0 = mask;
			return true;
		}
		return false;
	}
	G		getFilterConstant0() const
	{
		return mFilterConstant0;
	}
	bool	setFilterConstant1(const G& mask)
	{
		if (mFilterConstant1 != mask)
		{
			mFilterConstant1 = mask;
			return true;
		}
		return false;
	}
	G		getFilterConstant1() const
	{
		return mFilterConstant1;
	}

	bool	operator()(const G& mask0, const G& mask1) const
	{
		static const FilterOp sFilterOpList[] =
		{
			&filterOp_AND,
			&filterOp_OR,
			&filterOp_XOR,
			&filterOp_NAND,
			&filterOp_NOR,
			&filterOp_NXOR,
			&filterOp_SWAP_AND,
		};

		if (hasBits(mask0) & hasBits(mask1))
		{
			G result0, result1, result;
			sFilterOpList[mFilterOp0](mask0, mFilterConstant0, result0);
			sFilterOpList[mFilterOp1](mask1, mFilterConstant1, result1);
			sFilterOpList[mFilterOp2](result0, result1, result);
			return (hasBits(result) == mFilterBool);
		}
		return true;
	}
};


}
} // end namespace physx::apex

#endif
