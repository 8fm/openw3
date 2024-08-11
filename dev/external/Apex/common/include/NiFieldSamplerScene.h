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

#ifndef __NI_FIELD_SAMPLER_SCENE_H__
#define __NI_FIELD_SAMPLER_SCENE_H__

#include "NxApexDefs.h"
#include "PxTask.h"

#include "NiModule.h"

#if defined(APEX_CUDA_SUPPORT)
#include "ApexCudaWrapper.h"
#endif


namespace physx
{
namespace apex
{


struct NiFieldSamplerSceneDesc
{
	bool	isPrimary;

	NiFieldSamplerSceneDesc()
	{
		isPrimary = false;
	}
};

namespace fieldsampler
{
struct NiFieldSamplerKernelLaunchData;
}

class NiFieldSamplerScene : public NiModuleScene
{
public:
	virtual void getFieldSamplerSceneDesc(NiFieldSamplerSceneDesc& desc) const = 0;

	virtual const physx::PxTask* getFieldSamplerReadyTask() const
	{
		return 0;
	}

#if defined(APEX_CUDA_SUPPORT)
	virtual ApexCudaConstMem*	getFieldSamplerCudaConstMem()
	{
		APEX_INVALID_OPERATION("not implemented");
		return 0;
	}

	virtual bool				launchFieldSamplerCudaKernel(const fieldsampler::NiFieldSamplerKernelLaunchData&)
	{
		APEX_INVALID_OPERATION("not implemented");
		return false;
	}
#endif

	virtual NxApexSceneStats* getStats()
	{
		return 0;
	}

};

#define FSST_PHYSX_MONITOR_LOAD		"FieldSamplerScene::PhysXMonitorLoad"
#define FSST_PHYSX_MONITOR_FETCH	"FieldSamplerScene::PhysXMonitorFetch"
#define FSST_PHYSX_MONITOR_UPDATE	"FieldSamplerPhysXMonitor::Update"
}

} // end namespace physx::apex

#endif // __NI_FIELD_SAMPLER_SCENE_H__
