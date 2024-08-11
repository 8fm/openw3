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
#include "FieldSamplerSceneWrapper.h"
#include "FieldSamplerManager.h"
#include "FieldBoundaryWrapper.h"
#include "FieldSamplerWrapper.h"


namespace physx
{
namespace apex
{
namespace fieldsampler
{

FieldSamplerSceneWrapper::FieldSamplerSceneWrapper(NxResourceList& list, FieldSamplerManager* manager, NiFieldSamplerScene* fieldSamplerScene)
	: mManager(manager)
	, mFieldSamplerScene(fieldSamplerScene)
	, mFieldBoundaryListChanged(false)
{
	mFieldSamplerScene->getFieldSamplerSceneDesc(mFieldSamplerSceneDesc);

	list.add(*this);
}

void FieldSamplerSceneWrapper::release()
{
	delete this;
}

FieldSamplerSceneWrapper::FieldBoundaryInfo* FieldSamplerSceneWrapper::addFieldBoundary(FieldBoundaryWrapper* fieldBoundaryWrapper)
{
	FieldBoundaryInfo* fieldBoundaryInfo = NULL;
	for (physx::PxU32 i = 0; i < mFieldBoundaryList.getSize(); ++i)
	{
		FieldBoundaryInfo* info = DYNAMIC_CAST(FieldBoundaryInfo*)(mFieldBoundaryList.getResource(i));
		if (info->getFieldBoundaryWrapper() == fieldBoundaryWrapper)
		{
			fieldBoundaryInfo = info;
			break;
		}
	}
	if (fieldBoundaryInfo == NULL)
	{
		fieldBoundaryInfo = createFieldBoundaryInfo(fieldBoundaryWrapper);
		mFieldBoundaryListChanged = true;
	}
	fieldBoundaryInfo->addRef();
	return fieldBoundaryInfo;
}

void FieldSamplerSceneWrapper::removeFieldBoundary(FieldBoundaryInfo* fieldBoundaryInfo)
{
	if (fieldBoundaryInfo->releaseRef())
	{
		mFieldBoundaryListChanged = true;
	}
}

void FieldSamplerSceneWrapper::update()
{
	for (physx::PxU32 i = 0; i < mFieldBoundaryList.getSize(); ++i)
	{
		FieldBoundaryInfo* info = DYNAMIC_CAST(FieldBoundaryInfo*)(mFieldBoundaryList.getResource(i));
		info->update();
	}
}

/******************************** CPU Version ********************************/
FieldSamplerSceneWrapperCPU::FieldSamplerSceneWrapperCPU(NxResourceList& list, FieldSamplerManager* manager, NiFieldSamplerScene* fieldSamplerScene)
	: FieldSamplerSceneWrapper(list, manager, fieldSamplerScene)
{
}

/******************************** GPU Version ********************************/
#if defined(APEX_CUDA_SUPPORT)

FieldSamplerSceneWrapperGPU::FieldSamplerSceneWrapperGPU(NxResourceList& list, FieldSamplerManager* manager, NiFieldSamplerScene* fieldSamplerScene)
	: FieldSamplerSceneWrapper(list, manager, fieldSamplerScene)
	, mConstMem(*fieldSamplerScene->getFieldSamplerCudaConstMem())
{
}

void FieldSamplerSceneWrapperGPU::postUpdate()
{
	PxCudaContextManager* ctx = DYNAMIC_CAST(FieldSamplerSceneGPU*)(mManager->getScene())->getCudaContext();
	physx::PxScopedCudaLock _lock_(*ctx);

	getConstMem().copyToDevice(0);
}


FieldSamplerSceneWrapperGPU::FieldBoundaryInfoGPU::FieldBoundaryInfoGPU(NxResourceList& list, FieldBoundaryWrapper* fieldBoundaryWrapper, ApexCudaConstMem& constMem)
	: FieldBoundaryInfo(list, fieldBoundaryWrapper)
	, mConstMemGroup(constMem)
{
	APEX_CUDA_CONST_MEM_GROUP_SCOPE(mConstMemGroup);
	mFieldShapeGroupParamsHandle.alloc(_storage_);
}

void FieldSamplerSceneWrapperGPU::FieldBoundaryInfoGPU::update()
{
	if (mFieldBoundaryWrapper->getFieldShapesChanged())
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mConstMemGroup);

		FieldShapeGroupParams* shapeGroupParams = mFieldShapeGroupParamsHandle.resolve(_storage_);
		PX_ASSERT(shapeGroupParams);

		const physx::Array<NiFieldShapeDesc>& shapes = mFieldBoundaryWrapper->getFieldShapes();
		physx::PxU32 shapeCount = shapes.size();
		shapeGroupParams->shapeArray.resize(_storage_, shapeCount);
		FieldShapeParams* elems = shapeGroupParams->shapeArray.getElems(_storage_);
		for (physx::PxU32 i = 0; i < shapeCount; ++i)
		{
			elems[i].type          = shapes[i].type;
			elems[i].dimensions    = shapes[i].dimensions;
			elems[i].worldToShape  = shapes[i].worldToShape;
			PX_ASSERT(shapes[i].weight >= 0.0f && shapes[i].weight <= 1.0f);
			elems[i].weight        = PxClamp(shapes[i].weight, 0.0f, 1.0f);
			elems[i].fade          = 0;
		}
	}
}

#endif // defined(APEX_CUDA_SUPPORT)

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
