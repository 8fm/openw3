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

#ifndef __NI_FIELD_SAMPLER_MANAGER_H__
#define __NI_FIELD_SAMPLER_MANAGER_H__

#include "NxApex.h"
#include "ApexSDKHelpers.h"

namespace physx
{
namespace apex
{

struct NiFieldSamplerQueryDesc;
class NiFieldSamplerQuery;

class NiFieldSamplerScene;

struct NiFieldSamplerDesc;
class NiFieldSampler;

struct NiFieldBoundaryDesc;
class NiFieldBoundary;

class NiFieldSamplerManager
{
public:
	virtual NiFieldSamplerQuery*	createFieldSamplerQuery(const NiFieldSamplerQueryDesc&) = 0;

	virtual void					registerFieldSampler(NiFieldSampler* , const NiFieldSamplerDesc& , NiFieldSamplerScene*) = 0;
	virtual void					unregisterFieldSampler(NiFieldSampler*) = 0;

	virtual void					registerFieldBoundary(NiFieldBoundary* , const NiFieldBoundaryDesc&) = 0;
	virtual void					unregisterFieldBoundary(NiFieldBoundary*) = 0;

#if NX_SDK_VERSION_MAJOR == 3
	virtual void					registerUnhandledParticleSystem(PxActor*) = 0;
	virtual void					unregisterUnhandledParticleSystem(PxActor*) = 0;
	virtual bool					isUnhandledParticleSystem(PxActor*) = 0;
#endif
};

}
} // end namespace physx::apex

#endif // #ifndef __NI_FIELD_SAMPLER_MANAGER_H__
