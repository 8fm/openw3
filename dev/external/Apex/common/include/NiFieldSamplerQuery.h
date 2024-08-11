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

#ifndef __NI_FIELD_SAMPLER_QUERY_H__
#define __NI_FIELD_SAMPLER_QUERY_H__

#include "NxApexDefs.h"
#include "ApexMirroredArray.h"

#include "PxTask.h"
#include "ApexActor.h"
#include "PxMat34Legacy.h"

namespace physx
{
namespace apex
{


class NiFieldSamplerScene;

struct NiFieldSamplerQueryDesc
{
	PxU32					maxCount;

	NxGroupsMask64			samplerFilterData;

	NiFieldSamplerScene*	ownerFieldSamplerScene;


	NiFieldSamplerQueryDesc()
	{
		maxCount                = 0;

		samplerFilterData.bits0 = 0;
		samplerFilterData.bits1 = 0;

		ownerFieldSamplerScene  = 0;
	}
};

struct NiFieldSamplerQueryData
{
	PxF32						timeStep;
	PxU32						count;
	bool						isDataOnDevice;

	PxU32						strideBytes; //Stride for velocity and position
	PxF32*						pmaInPosition;
	PxF32*						pmaInVelocity;
	PxVec4*						pmaOutField;

	PxU32						massStrideBytes; //if massStride set to 0 supposed single mass for all objects
	PxF32*						pmaInMass;
};


#if defined(APEX_CUDA_SUPPORT)
struct NiFieldSamplerQueryGridData
{
	PxU32 numX, numY, numZ;
	PxU32 gapX, gapY, gapZ;
	PxU32 strideX, strideY, offset;

	PxMat34Legacy	gridToWorld;

	PxF32			mass;

	PxF32			timeStep;

	PxF32			cellRadius;

	PxVec4*			resultVelocity; //x, y, z = velocity vector, w = weight

	CUstream		stream;
};
#endif

class NiFieldSamplerCallback
{
public:
	virtual void operator()(void*) = 0;
};

class NiFieldSamplerQuery : public ApexActor
{
public:
	virtual PxTaskID submitFieldSamplerQuery(const NiFieldSamplerQueryData& data, PxTaskID taskID) = 0;

	//! NiFieldSamplerCallback will be called before FieldSampler computations
	virtual void setOnStartCallback(NiFieldSamplerCallback*) = 0;
	//! NiFieldSamplerCallback will be called after FieldSampler computations
	virtual void setOnFinishCallback(NiFieldSamplerCallback*) = 0;

#if defined(APEX_CUDA_SUPPORT)
	virtual physx::PxVec3 executeFieldSamplerQueryOnGrid(const NiFieldSamplerQueryGridData&)
	{
		APEX_INVALID_OPERATION("not implemented");
		return physx::PxVec3(0.0f);
	}
#endif

protected:
	virtual ~NiFieldSamplerQuery() {}
};

}
} // end namespace physx::apex

#endif // #ifndef __NI_FIELD_SAMPLER_QUERY_H__
