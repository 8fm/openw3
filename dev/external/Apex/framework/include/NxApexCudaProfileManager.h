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

#ifndef NX_APEX_CUDA_PROFILE_MANAGER_H
#define NX_APEX_CUDA_PROFILE_MANAGER_H

/*!
\file
\brief classes NxApexCudaProfileManager
*/

#include <NxApexDefs.h>
#include <foundation/PxSimpleTypes.h>

namespace physx
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief Interface for options of ApexCudaProfileManager
 */
class NxApexCudaProfileManager
{
public:
	/**
	 * Normalized time unit for profile data
	 */
	enum TimeFormat
	{
		MILLISECOND = 1,
		MICROSECOND = 1000,
		NANOSECOND = 1000000
	};

	 /**
	\brief Set path for writing results
	*/
	virtual void setPath(const char* path) = 0;
	 /**
	\brief Set kernel for profile
	*/
	virtual void setKernel(const char* functionName, const char* moduleName) = 0;
	/**
	\brief Set normailized time unit
	*/
	virtual void setTimeFormat(TimeFormat tf) = 0;
	/**
	\brief Set state (on/off) for profile manager
	*/
	virtual void enable(bool state) = 0;
	/**
	\brief Get state (on/off) of profile manager
	*/
	virtual bool isEnabled() const = 0;
};

PX_POP_PACK

}
}

#endif // NX_APEX_CUDA_PROFILE_MANAGER_H

