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

#ifndef __NI_APEX_PHYSX_OBJECT_DESC_H__
#define __NI_APEX_PHYSX_OBJECT_DESC_H__

#include "NxApexPhysXObjectDesc.h"

namespace physx
{
namespace apex
{

/**
 * Module/Asset interface to actor info structure.  This allows the asset to
 * set the various flags without knowing their implementation.
 */
class NiApexPhysXObjectDesc : public NxApexPhysXObjectDesc
{
public:
	void setIgnoreTransform(bool b)
	{
		if (b)
		{
			mFlags |= TRANSFORM;
		}
		else
		{
			mFlags &= ~(physx::PxU32)TRANSFORM;
		}
	};
	void setIgnoreRaycasts(bool b)
	{
		if (b)
		{
			mFlags |= RAYCASTS;
		}
		else
		{
			mFlags &= ~(physx::PxU32)RAYCASTS;
		}
	};
	void setIgnoreContacts(bool b)
	{
		if (b)
		{
			mFlags |= CONTACTS;
		}
		else
		{
			mFlags &= ~(physx::PxU32)CONTACTS;
		}
	};
	void setUserDefinedFlag(physx::PxU32 index, bool b)
	{
		if (b)
		{
			mFlags |= (1 << index);
		}
		else
		{
			mFlags &= ~(1 << index);
		}
	}

	/**
	\brief Implementation of pure virtual functions in NxApexPhysXObjectDesc, used for external (read-only)
	access to the NxApexActor list
	*/
	physx::PxU32				getApexActorCount() const
	{
		return mApexActors.size();
	}
	const NxApexActor*	getApexActor(physx::PxU32 i) const
	{
		return mApexActors[i];
	}

	/**
	\brief Array of pointers to APEX actors assiciated with this PhysX object

	Pointers may be NULL in cases where the APEX actor has been deleted
	but PhysX actor cleanup has been deferred
	*/
	physx::Array<const NxApexActor*>	mApexActors;

	/**
	\brief the PhysX object which uses this descriptor
	*/
	const void* mPhysXObject;
protected:
	virtual ~NiApexPhysXObjectDesc(void) {}
};

}
} // end namespace physx::apex

#endif // __NI_APEX_PHYSX_OBJECT_DESC_H__
