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


#include "PxsBroadPhaseCommon.h"
#include "PxvBroadPhase.h"
#include "CmBitMap.h"

using namespace physx;

#ifdef PX_CHECKED
bool PxcBroadPhaseUpdateData::isValid(const PxcBroadPhaseUpdateData& updateData, const PxvBroadPhase& bp)
{
	return (updateData.isValid() && bp.isValid(updateData));
}
#endif

#ifdef PX_CHECKED
bool PxcBroadPhaseUpdateData::isValid() const 
{
	const PxcBpHandle* created=getCreatedHandles();
	const PxcBpHandle* updated=getUpdatedHandles();
	const PxcBpHandle* removed=getRemovedHandles();
	const PxU32 createdSize=getNumCreatedHandles();
	const PxU32 updatedSize=getNumUpdatedHandles();
	const PxU32 removedSize=getNumRemovedHandles();
	const IntegerAABB* bounds=getAABBs();
	const PxcBpHandle* groups=getGroups();
	const PxU32 boxesCapacity=getCapacity();

	if(NULL==created && createdSize>0)
	{
		return false;
	}
	if(NULL==updated && updatedSize>0)
	{
		return false;
	}
	if(NULL==removed && removedSize>0)
	{
		return false;
	}

	PxcBPValType minVal=0;
	PxcBPValType maxVal=0xffffffff;

	for(PxU32 i=0;i<createdSize;i++)
	{
		if(created[i]>=boxesCapacity)
			return false;

		//Created array in ascending order of id.
		if(i>0 && (created[i] < created[i-1]))
		{
			return false;
		}

		for(PxU32 j=0;j<3;j++)
		{
			//Max must be greater than min.
			if(bounds[created[i]].getMin(j)>=bounds[created[i]].getMax(j))
				return false;

			//Bounds have an upper limit.
			if(bounds[created[i]].getMax(j)>=maxVal)
				return false;

			//Bounds have a lower limit.
			if(bounds[created[i]].getMin(j)<=minVal)
				return false;

			//Max must be odd.
			if(1 != (bounds[created[i]].getMax(j) & 1))
				return false;

			//Min must be even.
			if(0 != (bounds[created[i]].getMin(j) & 1))
				return false;
		}

		//Group ids must be less than PX_INVALID_BP_HANDLE.
		if(groups[created[i]]>=PX_INVALID_BP_HANDLE)
			return false;
	}

	for(PxU32 i=0;i<updatedSize;i++)
	{
		if(updated[i]>=boxesCapacity)
			return false;

		//Updated array in ascending order of id
		if(i>0 && (updated[i] < updated[i-1]))
		{
			return false;
		}

		for(PxU32 j=0;j<3;j++)
		{
			//Max must be greater than min.
			if(bounds[updated[i]].getMin(j)>=bounds[updated[i]].getMax(j))
				return false;

			//Bounds have an upper limit.
			if(bounds[updated[i]].getMax(j)>=maxVal)
				return false;

			//Bounds have a lower limit.
			if(bounds[updated[i]].getMin(j)<=minVal)
				return false;

			//Max must be odd.
			if(1 != (bounds[updated[i]].getMax(j) & 1))
				return false;

			//Min must be even.
			if(0 != (bounds[updated[i]].getMin(j) & 1))
				return false;
		}

		//Group ids must be less than PX_INVALID_BP_HANDLE.
		if(groups[updated[i]]>=PX_INVALID_BP_HANDLE)
			return false;
	}

	for(PxU32 i=0;i<removedSize;i++)
	{
		if(removed[i]>=boxesCapacity)
			return false;

		//Removed array in ascending order of id
		if(i>0 && (removed[i] < removed[i-1]))
		{
			return false;
		}
	}

	//An index cannot appear twice in a single list or be duplicated in multiple lists.
	Cm::BitMap bm;
	bm.resize(boxesCapacity);
	for(PxU32 i=0;i<createdSize;i++)
	{
		if(bm.test(created[i]))
			return false;

		bm.set(created[i]);
	}
	for(PxU32 i=0;i<updatedSize;i++)
	{
		if(bm.test(updated[i]))
			return false;

		bm.set(updated[i]);
	}
	for(PxU32 i=0;i<removedSize;i++)
	{
		if(bm.test(removed[i]))
			return false;

		bm.set(removed[i]);
	}

	return true;
}
#endif


