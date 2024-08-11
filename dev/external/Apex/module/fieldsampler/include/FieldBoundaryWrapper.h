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

#ifndef __FIELD_BOUNDARY_WRAPPER_H__
#define __FIELD_BOUNDARY_WRAPPER_H__

#include "NxApex.h"
#include "ApexSDKHelpers.h"
#include "ApexActor.h"
#include "NiFieldBoundary.h"

#if defined(APEX_CUDA_SUPPORT)
#include "ApexCudaWrapper.h"
#endif

#include "FieldSamplerCommon.h"


namespace physx
{
namespace apex
{
namespace fieldsampler
{

class FieldSamplerManager;


class FieldBoundaryWrapper : public NxApexResource, public ApexResource
{
public:
	// NxApexResource methods
	void			release();
	void			setListIndex(NxResourceList& list, physx::PxU32 index)
	{
		m_listIndex = index;
		m_list = &list;
	}
	physx::PxU32	getListIndex() const
	{
		return m_listIndex;
	}

	FieldBoundaryWrapper(NxResourceList& list, FieldSamplerManager* manager, NiFieldBoundary* fieldBoundary, const NiFieldBoundaryDesc& fieldBoundaryDesc);

	NiFieldBoundary* getNiFieldBoundary() const
	{
		return mFieldBoundary;
	}
	PX_INLINE const NiFieldBoundaryDesc& getNiFieldBoundaryDesc() const
	{
		return mFieldBoundaryDesc;
	}

	void update();

	const physx::Array<NiFieldShapeDesc>&	getFieldShapes() const
	{
		return mFieldShapes;
	}
	bool									getFieldShapesChanged() const
	{
		return mFieldShapesChanged;
	}

protected:
	FieldSamplerManager*			mManager;

	NiFieldBoundary*				mFieldBoundary;
	NiFieldBoundaryDesc				mFieldBoundaryDesc;

	physx::Array<NiFieldShapeDesc>	mFieldShapes;
	bool							mFieldShapesChanged;
};

}
}
} // end namespace physx::apex

#endif
