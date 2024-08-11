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

#ifndef __NI_FIELD_SAMPLER_H__
#define __NI_FIELD_SAMPLER_H__

#include "InplaceTypes.h"
#include "NiFieldBoundary.h"

#ifndef __CUDACC__
#include "NiApexSDK.h"
#endif

namespace physx
{
namespace apex
{


struct NiFieldSamplerType
{
	enum Enum
	{
		FORCE,
		ACCELERATION,
		VELOCITY_DRAG,
		VELOCITY_DIRECT,
	};
};
#ifndef __CUDACC__
template <> struct InplaceTypeTraits<NiFieldSamplerType::Enum>
{
	enum { hasReflect = false };
};
#endif

struct NiFieldSamplerGridSupportType
{
	enum Enum
	{
		NONE = 0,
		SINGLE_VELOCITY,
		VELOCITY_PER_CELL,
	};
};
#ifndef __CUDACC__
template <> struct InplaceTypeTraits<NiFieldSamplerGridSupportType::Enum>
{
	enum { hasReflect = false };
};
#endif


#ifndef __CUDACC__

struct NiFieldSamplerDesc
{
	NiFieldSamplerType::Enum			type;
	NiFieldSamplerGridSupportType::Enum	gridSupportType;

	NxGroupsMask64						samplerFilterData;
	NxGroupsMask64						boundaryFilterData;
	physx::PxF32						boundaryFadePercentage;

	physx::PxF32						dragCoeff; //only used then type is VELOCITY_DRAG

	void*								userData;

	NiFieldSamplerDesc()
	{
		type                     = NiFieldSamplerType::FORCE;
		gridSupportType          = NiFieldSamplerGridSupportType::NONE;

		samplerFilterData.bits0  = 0;
		samplerFilterData.bits1  = 0;

		boundaryFilterData.bits0 = 0;
		boundaryFilterData.bits1 = 0;
		boundaryFadePercentage   = 0.1;
		dragCoeff                = 0;
		userData                 = NULL;
	}
};


class NiFieldSampler
{
public:
	//returns true if shape/params was changed
	//required to return true on first call!
	virtual bool updateFieldSampler(NiFieldShapeDesc& shapeDesc, bool& isEnabled) = 0;

	struct ExecuteData
	{
		physx::PxU32            count;
		physx::PxU32            stride;
		physx::PxU32            massStride;
		const physx::PxF32*	    position;
		const physx::PxF32*		velocity;
		const physx::PxF32*		mass;
		physx::PxVec3*	        resultField;
	};

	virtual void executeFieldSampler(const ExecuteData& data)
	{
		PX_UNUSED(data);
		APEX_INVALID_OPERATION("not implemented");
	}

#if defined(APEX_CUDA_SUPPORT)
	struct CudaExecuteInfo
	{
		physx::PxU32		executeType;
		InplaceHandleBase	executeParamsHandle;
	};

	virtual void getFieldSamplerCudaExecuteInfo(CudaExecuteInfo& info) const
	{
		PX_UNUSED(info);
		APEX_INVALID_OPERATION("not implemented");
	}
#endif

	virtual physx::PxVec3 queryFieldSamplerVelocity() const
	{
		APEX_INVALID_OPERATION("not implemented");
		return physx::PxVec3(0.0f);
	}

protected:
	virtual ~NiFieldSampler() {}
};

#endif // __CUDACC__

}
} // end namespace physx::apex

#endif // #ifndef __NI_FIELD_SAMPLER_H__
