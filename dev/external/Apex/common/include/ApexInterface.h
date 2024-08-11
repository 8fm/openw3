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

#ifndef APEX_INTERFACE_H
#define APEX_INTERFACE_H

#include "PsShare.h"
#include "PsUserAllocated.h"

namespace physx
{
	namespace debugger
	{
		namespace comm
		{
			class PvdDataStream;
		}
	}

namespace apex
{

/**
 *  Class defines semi-public interface to ApexResource objects
 *	Resource - gets added to a list, will be deleted when the list is deleted
 */
class NxApexResource
{
public:
	virtual void    release() = 0;
	virtual void	setListIndex(class NxResourceList& list, physx::PxU32 index) = 0;
	virtual physx::PxU32	getListIndex() const = 0;
	virtual void	initPvdInstances(physx::debugger::comm::PvdDataStream& /*pvdStream*/) {};
};

/**
Class that implements resource ID and bank
*/
class ApexResource : public physx::UserAllocated
{
public:
	ApexResource() : m_listIndex(0xFFFFFFFF), m_list(NULL) {}
	void removeSelf();
	virtual ~ApexResource();

	physx::PxU32			m_listIndex;
	class NxResourceList*	m_list;
};


/**
Initialized Template class.
*/
template <class DescType>class InitTemplate
{
	//gotta make a derived class cause of protected ctor
public:
	bool isSet;
	DescType data;


	void set(const DescType* desc)
	{
		if (desc)
		{
			isSet = true;
			//memcpy(this,desc, sizeof(DescType));
			data = *desc;
		}
		else
		{
			isSet = false;
		}
	}


	bool get(DescType& dest) const
	{
		if (isSet)
		{
			//memcpy(&dest,this, sizeof(DescType));
			dest = data;
			return true;
		}
		else
		{
			return false;
		}

	}
};

} // namespace apex
} // namespace physx

#endif // APEX_INTERFACE_H
