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

#ifndef __APEX_PHYSX_OBJECT_DESC_H__
#define __APEX_PHYSX_OBJECT_DESC_H__

#include "NxApex.h"
#include "NiApexPhysXObjectDesc.h"

namespace physx
{
namespace apex
{

class ApexPhysXObjectDesc : public NiApexPhysXObjectDesc
{
public:
	ApexPhysXObjectDesc() : mNext(0), mPrev(0)
	{
		mFlags = 0;
		userData = NULL;
		mPhysXObject = NULL;
	}

	// Need a copy constructor because we contain an array, and we are in arrays
	ApexPhysXObjectDesc(const ApexPhysXObjectDesc& desc) : NiApexPhysXObjectDesc(desc)
	{
		*this = desc;
	}

	ApexPhysXObjectDesc& operator = (const ApexPhysXObjectDesc& desc)
	{
		mFlags = desc.mFlags;
		userData = desc.userData;
		mApexActors = desc.mApexActors;
		mPhysXObject = desc.mPhysXObject;
		mNext = desc.mNext;
		mPrev = desc.mPrev;
		return *this;
	}

	static physx::PxU16 makeHash(size_t hashable);

	physx::PxU32		mNext, mPrev;

	friend class ApexSDK;
	virtual ~ApexPhysXObjectDesc(void)
	{

	}
};

}
} // end namespace physx::apex

#endif // __APEX_PHYSX_OBJECT_DESC_H__
