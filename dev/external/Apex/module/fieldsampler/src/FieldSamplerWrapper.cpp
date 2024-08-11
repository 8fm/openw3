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

#include "NxApexDefs.h"
#include "MinPhysxSdkVersion.h"
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

#include "NxApex.h"
#include "FieldSamplerWrapper.h"
#include "FieldBoundaryWrapper.h"
#include "FieldSamplerManager.h"
#include "FieldSamplerSceneWrapper.h"


namespace physx
{
namespace apex
{
namespace fieldsampler
{


FieldSamplerWrapper::FieldSamplerWrapper(NxResourceList& list, FieldSamplerManager* manager, NiFieldSampler* fieldSampler, const NiFieldSamplerDesc& fieldSamplerDesc, FieldSamplerSceneWrapper* fieldSamplerSceneWrapper)
	: mManager(manager)
	, mFieldSampler(fieldSampler)
	, mFieldSamplerDesc(fieldSamplerDesc)
	, mFieldSamplerShapeChanged(false)
	, mSceneWrapper(fieldSamplerSceneWrapper)
	, mQueryRefCount(0)
	, mFieldBoundaryInfoArrayChanged(true)
	, mIsEnabled(false)
	, mIsEnabledLast(false)
{
	list.add(*this);

	//set default shape weight to 1
	mFieldSamplerShape.weight = 1;
}

void FieldSamplerWrapper::release()
{
	for (physx::PxU32 i = 0; i < mFieldBoundaryInfoArray.size(); ++i)
	{
		FieldSamplerSceneWrapper::FieldBoundaryInfo* fieldBoundaryInfo = mFieldBoundaryInfoArray[i];
		mSceneWrapper->removeFieldBoundary(fieldBoundaryInfo);
	}

	delete this;
}

bool FieldSamplerWrapper::addFieldBoundary(FieldBoundaryWrapper* fieldBoundaryWrapper)
{
	const NiFieldBoundaryDesc& fieldBoundaryDesc = fieldBoundaryWrapper->getNiFieldBoundaryDesc();

	if (mManager->getFieldBoundaryGroupsFiltering()(fieldBoundaryDesc.boundaryFilterData, mFieldSamplerDesc.boundaryFilterData))
	{
		FieldSamplerSceneWrapper::FieldBoundaryInfo* fieldBoundaryInfo =
		    mSceneWrapper->addFieldBoundary(fieldBoundaryWrapper);

		mFieldBoundaryInfoArray.pushBack(fieldBoundaryInfo);
		mFieldBoundaryInfoArrayChanged = true;
		return true;
	}
	return false;
}

bool FieldSamplerWrapper::removeFieldBoundary(FieldBoundaryWrapper* fieldBoundaryWrapper)
{
	for (physx::PxU32 i = 0; i < mFieldBoundaryInfoArray.size(); ++i)
	{
		FieldSamplerSceneWrapper::FieldBoundaryInfo* fieldBoundaryInfo = mFieldBoundaryInfoArray[i];
		if (fieldBoundaryInfo->getFieldBoundaryWrapper() == fieldBoundaryWrapper)
		{
			mSceneWrapper->removeFieldBoundary(fieldBoundaryInfo);

			mFieldBoundaryInfoArray.replaceWithLast(i);
			mFieldBoundaryInfoArrayChanged = true;
			return true;
		}
	}
	return false;
}

void FieldSamplerWrapper::update()
{
	mIsEnabledLast = mIsEnabled;
	mFieldSamplerShapeChanged = mFieldSampler->updateFieldSampler(mFieldSamplerShape, mIsEnabled);
	if (!mIsEnabledLast && mIsEnabled)
	{
		mFieldSamplerShapeChanged = true;
	}
	else if (!mIsEnabled)
	{
		mFieldSamplerShapeChanged = false;
	}
}

/******************************** CPU Version ********************************/

FieldSamplerWrapperCPU::FieldSamplerWrapperCPU(NxResourceList& list, FieldSamplerManager* manager, NiFieldSampler* fieldSampler, const NiFieldSamplerDesc& fieldSamplerDesc, FieldSamplerSceneWrapper* fieldSamplerSceneWrapper)
	: FieldSamplerWrapper(list, manager, fieldSampler, fieldSamplerDesc, fieldSamplerSceneWrapper)
{
}

/******************************** GPU Version ********************************/
#if defined(APEX_CUDA_SUPPORT)

FieldSamplerWrapperGPU::FieldSamplerWrapperGPU(NxResourceList& list, FieldSamplerManager* manager, NiFieldSampler* fieldSampler, const NiFieldSamplerDesc& fieldSamplerDesc, FieldSamplerSceneWrapper* fieldSamplerSceneWrapper)
	: FieldSamplerWrapper(list, manager, fieldSampler, fieldSamplerDesc, fieldSamplerSceneWrapper)
	, mConstMemGroup(DYNAMIC_CAST(FieldSamplerSceneWrapperGPU*)(fieldSamplerSceneWrapper)->getConstMem())
{
}

void FieldSamplerWrapperGPU::update()
{
	FieldSamplerWrapper::update();
	if (mFieldSamplerShapeChanged)
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mConstMemGroup);

		FieldSamplerParams* params = mFieldSamplerParamsHandle.allocOrResolve(_storage_);
		PX_ASSERT(params);

		NiFieldSampler::CudaExecuteInfo executeInfo;
		mFieldSampler->getFieldSamplerCudaExecuteInfo(executeInfo);

		params->executeType = executeInfo.executeType;
		params->executeParamsHandle = executeInfo.executeParamsHandle;

		params->type = mFieldSamplerDesc.type;
		params->gridSupportType = mFieldSamplerDesc.gridSupportType;
		params->dragCoeff = mFieldSamplerDesc.dragCoeff;

		params->includeShape.type           = mFieldSamplerShape.type;
		params->includeShape.dimensions     = mFieldSamplerShape.dimensions;
		params->includeShape.worldToShape   = mFieldSamplerShape.worldToShape;
		PX_ASSERT(mFieldSamplerShape.weight >= 0.0f && mFieldSamplerShape.weight <= 1.0f);
		params->includeShape.weight         = PxClamp(mFieldSamplerShape.weight, 0.0f, 1.0f);
		params->includeShape.fade           = PxClamp(mFieldSamplerDesc.boundaryFadePercentage, 0.0f, 1.0f);
	}

	if (mFieldBoundaryInfoArrayChanged)
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mConstMemGroup);

		FieldSamplerParams* params = mFieldSamplerParamsHandle.resolve(_storage_);
		if (params)
		{
			PxU32 shapeGroupCount = mFieldBoundaryInfoArray.size();
			if (params->excludeShapeGroupHandleArray.resize(_storage_, shapeGroupCount))
			{
				InplaceHandle<FieldShapeGroupParams>* elems = params->excludeShapeGroupHandleArray.getElems(_storage_);

				for (PxU32 shapeGroupIndex = 0; shapeGroupIndex < shapeGroupCount; ++shapeGroupIndex)
				{
					FieldSamplerSceneWrapperGPU::FieldBoundaryInfoGPU* fieldBoundaryInfo =
					    static_cast<FieldSamplerSceneWrapperGPU::FieldBoundaryInfoGPU*>(mFieldBoundaryInfoArray[shapeGroupIndex]);

					elems[shapeGroupIndex] = fieldBoundaryInfo->getShapeGroupParamsHandle();
				}
			}
		}

		mFieldBoundaryInfoArrayChanged = false;
	}
}

#endif

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
