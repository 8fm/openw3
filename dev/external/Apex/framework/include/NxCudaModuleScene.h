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

#ifndef NX_CUDA_MODULE_SCENE_H
#define NX_CUDA_MODULE_SCENE_H

#include <NxApexDefs.h>

namespace physx
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief A scene encapsulation for CUDA scenes
*/
class NxCudaModuleScene
{
public:
	/**
	\brief Get the CuModule from the given function name
	*/
	virtual void* getCuModule(const char* functionName) = 0;

	/**
	\brief Get the CuFunction from the given function name
	*/
	virtual void* getCuFunction(const char* functionName) = 0;
};

PX_POP_PACK

}
} // end namespace physx::apex

#endif // NX_CUDA_MODULE_SCENE_H
