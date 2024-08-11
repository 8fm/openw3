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

#include "FieldSamplerManager.h"

#include "FieldSamplerQuery.h"
#include "FieldSamplerSceneWrapper.h"
#include "FieldSamplerWrapper.h"
#include "FieldBoundaryWrapper.h"

#include "NiApexScene.h"

namespace physx
{
namespace apex
{
namespace fieldsampler
{

FieldSamplerManager::FieldSamplerManager(FieldSamplerScene* scene)
	: mScene(scene)
	, mFieldSamplerGroupsFilteringChanged(true)
{
}


PX_INLINE void FieldSamplerManager::addFieldSamplerToQuery(FieldSamplerWrapper* fieldSamplerWrapper, FieldSamplerQuery* query)
{
	if (query->addFieldSampler(fieldSamplerWrapper))
	{
		fieldSamplerWrapper->mQueryRefCount += 1;
	}
}

void FieldSamplerManager::addAllFieldSamplersToQuery(FieldSamplerQuery* query) const
{
	for (PxU32 i = 0; i < mFieldSamplerWrapperList.getSize(); ++i)
	{
		addFieldSamplerToQuery(static_cast<FieldSamplerWrapper*>(mFieldSamplerWrapperList.getResource(i)), query);
	}
}

void FieldSamplerManager::submitTasks()
{
	if (mFieldSamplerGroupsFilteringChanged)
	{
		mFieldSamplerGroupsFilteringChanged = false;

		//clear queryRefCounts
		for (physx::PxU32 i = 0; i < mFieldSamplerWrapperList.getSize(); ++i)
		{
			FieldSamplerWrapper* fieldSamplerWrapper = static_cast<FieldSamplerWrapper*>(mFieldSamplerWrapperList.getResource(i));
			fieldSamplerWrapper->mQueryRefCount = 0;
		}

		//rebuild all connection based on changed collision checking
		for (physx::PxU32 i = 0; i < mFieldSamplerQueryList.getSize(); ++i)
		{
			FieldSamplerQuery* query = DYNAMIC_CAST(FieldSamplerQuery*)(mFieldSamplerQueryList.getResource(i));
			query->clearAllFieldSamplers();
			addAllFieldSamplersToQuery(query);
		}
	}

	for (physx::PxU32 i = 0; i < mFieldSamplerQueryList.getSize(); ++i)
	{
		FieldSamplerQuery* query = DYNAMIC_CAST(FieldSamplerQuery*)(mFieldSamplerQueryList.getResource(i));
		query->submitTasks();
	}
}

void FieldSamplerManager::setTaskDependencies()
{
	for (physx::PxU32 i = 0; i < mFieldSamplerQueryList.getSize(); ++i)
	{
		FieldSamplerQuery* query = DYNAMIC_CAST(FieldSamplerQuery*)(mFieldSamplerQueryList.getResource(i));
		query->setTaskDependencies();
	}

	//update
	for (physx::PxU32 i = 0; i < mFieldBoundaryWrapperList.getSize(); ++i)
	{
		FieldBoundaryWrapper* wrapper = DYNAMIC_CAST(FieldBoundaryWrapper*)(mFieldBoundaryWrapperList.getResource(i));
		wrapper->update();
	}
	for (physx::PxU32 i = 0; i < mFieldSamplerWrapperList.getSize(); ++i)
	{
		FieldSamplerWrapper* wrapper = DYNAMIC_CAST(FieldSamplerWrapper*)(mFieldSamplerWrapperList.getResource(i));
		wrapper->update();
	}
	for (physx::PxU32 i = 0; i < mFieldSamplerSceneWrapperList.getSize(); ++i)
	{
		FieldSamplerSceneWrapper* wrapper = DYNAMIC_CAST(FieldSamplerSceneWrapper*)(mFieldSamplerSceneWrapperList.getResource(i));
		wrapper->update();
	}
	for (physx::PxU32 i = 0; i < mFieldSamplerQueryList.getSize(); ++i)
	{
		FieldSamplerQuery* query = DYNAMIC_CAST(FieldSamplerQuery*)(mFieldSamplerQueryList.getResource(i));
		query->update();
	}

	//postUpdate
	for (physx::PxU32 i = 0; i < mFieldSamplerSceneWrapperList.getSize(); ++i)
	{
		FieldSamplerSceneWrapper* wrapper = DYNAMIC_CAST(FieldSamplerSceneWrapper*)(mFieldSamplerSceneWrapperList.getResource(i));
		wrapper->postUpdate();
	}
}

void FieldSamplerManager::fetchResults()
{
	for (physx::PxU32 i = 0; i < mFieldSamplerQueryList.getSize(); ++i)
	{
		FieldSamplerQuery* query = DYNAMIC_CAST(FieldSamplerQuery*)(mFieldSamplerQueryList.getResource(i));
		query->fetchResults();
	}
}


NiFieldSamplerQuery* FieldSamplerManager::createFieldSamplerQuery(const NiFieldSamplerQueryDesc& desc)
{
	FieldSamplerQuery* query = allocateFieldSamplerQuery(desc);
	if (query)
	{
		addAllFieldSamplersToQuery(query);
	}
	return query;
}

void FieldSamplerManager::registerFieldSampler(NiFieldSampler* fieldSampler, const NiFieldSamplerDesc& fieldSamplerDesc, NiFieldSamplerScene* fieldSamplerScene)
{
	FieldSamplerSceneWrapper* fieldSamplerSceneWrapper = NULL;
	//find FieldSamplerSceneWrapper
	for (physx::PxU32 i = 0; i < mFieldSamplerSceneWrapperList.getSize(); ++i)
	{
		FieldSamplerSceneWrapper* wrapper = DYNAMIC_CAST(FieldSamplerSceneWrapper*)(mFieldSamplerSceneWrapperList.getResource(i));
		if (wrapper->getNiFieldSamplerScene() == fieldSamplerScene)
		{
			fieldSamplerSceneWrapper = wrapper;
			break;
		}
	}
	if (fieldSamplerSceneWrapper == NULL)
	{
		fieldSamplerSceneWrapper = allocateFieldSamplerSceneWrapper(fieldSamplerScene);
	}
	PX_ASSERT(fieldSamplerSceneWrapper != NULL);

	FieldSamplerWrapper* fieldSamplerWrapper = allocateFieldSamplerWrapper(fieldSampler, fieldSamplerDesc, fieldSamplerSceneWrapper);
	PX_ASSERT(fieldSamplerWrapper != NULL);

	// add all mFieldBoundaryWrapperList
	for (physx::PxU32 i = 0; i < mFieldBoundaryWrapperList.getSize(); ++i)
	{
		FieldBoundaryWrapper* wrapper = DYNAMIC_CAST(FieldBoundaryWrapper*)(mFieldBoundaryWrapperList.getResource(i));
		fieldSamplerWrapper->addFieldBoundary(wrapper);
	}

	for (physx::PxU32 i = 0; i < mFieldSamplerQueryList.getSize(); ++i)
	{
		FieldSamplerQuery* query = DYNAMIC_CAST(FieldSamplerQuery*)(mFieldSamplerQueryList.getResource(i));
		addFieldSamplerToQuery(fieldSamplerWrapper, query);
	}
}

void FieldSamplerManager::unregisterFieldSampler(NiFieldSampler* fieldSampler)
{
	FieldSamplerWrapper* fieldSamplerWrapper = NULL;
	//find FieldSamplerWrapper
	for (physx::PxU32 i = 0; i < mFieldSamplerWrapperList.getSize(); ++i)
	{
		FieldSamplerWrapper* wrapper = static_cast<FieldSamplerWrapper*>(mFieldSamplerWrapperList.getResource(i));
		if (wrapper->getNiFieldSampler() == fieldSampler)
		{
			fieldSamplerWrapper = wrapper;
			break;
		}
	}
	if (fieldSamplerWrapper != NULL)
	{
		for (physx::PxU32 i = 0; i < mFieldSamplerQueryList.getSize(); ++i)
		{
			FieldSamplerQuery* query = DYNAMIC_CAST(FieldSamplerQuery*)(mFieldSamplerQueryList.getResource(i));
			query->removeFieldSampler(fieldSamplerWrapper);
		}
		fieldSamplerWrapper->release();
	}
}

void FieldSamplerManager::registerFieldBoundary(NiFieldBoundary* fieldBoundary, const NiFieldBoundaryDesc& fieldBoundaryDesc)
{
	FieldBoundaryWrapper* fieldBoundaryWrapper = PX_NEW(FieldBoundaryWrapper)(mFieldBoundaryWrapperList, this, fieldBoundary, fieldBoundaryDesc);
	if (fieldBoundaryWrapper)
	{
		for (PxU32 i = 0; i < mFieldSamplerWrapperList.getSize(); ++i)
		{
			static_cast<FieldSamplerWrapper*>(mFieldSamplerWrapperList.getResource(i))->addFieldBoundary(fieldBoundaryWrapper);
		}
	}
}
void FieldSamplerManager::unregisterFieldBoundary(NiFieldBoundary* fieldBoundary)
{
	FieldBoundaryWrapper* fieldBoundaryWrapper = 0;
	for (physx::PxU32 i = 0; i < mFieldBoundaryWrapperList.getSize(); ++i)
	{
		FieldBoundaryWrapper* wrapper = static_cast<FieldBoundaryWrapper*>(mFieldBoundaryWrapperList.getResource(i));
		if (wrapper->getNiFieldBoundary() == fieldBoundary)
		{
			fieldBoundaryWrapper = wrapper;
			break;
		}
	}
	if (fieldBoundaryWrapper != 0)
	{
		for (PxU32 i = 0; i < mFieldSamplerWrapperList.getSize(); ++i)
		{
			static_cast<FieldSamplerWrapper*>(mFieldSamplerWrapperList.getResource(i))->removeFieldBoundary(fieldBoundaryWrapper);
		}
		fieldBoundaryWrapper->release();
	}
}

void FieldSamplerManager::setFieldBoundaryGroupsFilteringParams(const NxGroupsFilteringParams64& params)
{
	mFieldBoundaryGroupsFiltering.setFilterOps(params.op0, params.op1, params.op2);
	mFieldBoundaryGroupsFiltering.setFilterBool(params.flag);
	mFieldBoundaryGroupsFiltering.setFilterConstant0(params.const0);
	mFieldBoundaryGroupsFiltering.setFilterConstant1(params.const1);
}
void FieldSamplerManager::getFieldBoundaryGroupsFilteringParams(NxGroupsFilteringParams64& params) const
{
	mFieldBoundaryGroupsFiltering.getFilterOps(params.op0, params.op1, params.op2);
	params.flag = mFieldBoundaryGroupsFiltering.getFilterBool();
	params.const0 = mFieldBoundaryGroupsFiltering.getFilterConstant0();
	params.const1 = mFieldBoundaryGroupsFiltering.getFilterConstant1();
}

void FieldSamplerManager::setFieldSamplerGroupsFilteringParams(const NxGroupsFilteringParams64& params)
{
	bool bChanged = false;
	bChanged |= mFieldSamplerGroupsFiltering.setFilterOps(params.op0, params.op1, params.op2);
	bChanged |= mFieldSamplerGroupsFiltering.setFilterBool(params.flag);
	bChanged |= mFieldSamplerGroupsFiltering.setFilterConstant0(params.const0);
	bChanged |= mFieldSamplerGroupsFiltering.setFilterConstant1(params.const1);

	mFieldSamplerGroupsFilteringChanged |= bChanged;
}
void FieldSamplerManager::getFieldSamplerGroupsFilteringParams(NxGroupsFilteringParams64& params) const
{
	mFieldSamplerGroupsFiltering.getFilterOps(params.op0, params.op1, params.op2);
	params.flag = mFieldSamplerGroupsFiltering.getFilterBool();
	params.const0 = mFieldSamplerGroupsFiltering.getFilterConstant0();
	params.const1 = mFieldSamplerGroupsFiltering.getFilterConstant1();
}

#if NX_SDK_VERSION_MAJOR == 3

void FieldSamplerManager::registerUnhandledParticleSystem(physx::PxActor* actor)
{
	if (!isUnhandledParticleSystem(actor))
	{
		mUnhandledParticleSystems.pushBack(actor);
	}
}

void FieldSamplerManager::unregisterUnhandledParticleSystem(physx::PxActor* actor)
{
	mUnhandledParticleSystems.findAndReplaceWithLast(actor);
}

bool FieldSamplerManager::isUnhandledParticleSystem(physx::PxActor* actor)
{
	for (physx::PxU32 i = 0; i < mUnhandledParticleSystems.size(); i++)
	if (mUnhandledParticleSystems[i] == actor)
	{
		return true;
	}
	return false;
}

#endif

/******************************** CPU Version ********************************/

FieldSamplerQuery* FieldSamplerManagerCPU::allocateFieldSamplerQuery(const NiFieldSamplerQueryDesc& desc)
{
	return PX_NEW(FieldSamplerQueryCPU)(desc, mFieldSamplerQueryList, this);
}
FieldSamplerSceneWrapper* FieldSamplerManagerCPU::allocateFieldSamplerSceneWrapper(NiFieldSamplerScene* fieldSamplerScene)
{
	return PX_NEW(FieldSamplerSceneWrapperCPU)(mFieldSamplerSceneWrapperList, this, fieldSamplerScene);
}
FieldSamplerWrapper* FieldSamplerManagerCPU::allocateFieldSamplerWrapper(NiFieldSampler* fieldSampler, const NiFieldSamplerDesc& fieldSamplerDesc, FieldSamplerSceneWrapper* fieldSamplerSceneWrapper)
{
	return PX_NEW(FieldSamplerWrapperCPU)(mFieldSamplerWrapperList, this, fieldSampler, fieldSamplerDesc, fieldSamplerSceneWrapper);
}

/******************************** GPU Version ********************************/

#if defined(APEX_CUDA_SUPPORT)

FieldSamplerQuery* FieldSamplerManagerGPU::allocateFieldSamplerQuery(const NiFieldSamplerQueryDesc& desc)
{
	return PX_NEW(FieldSamplerQueryGPU)(desc, mFieldSamplerQueryList, this);
}
FieldSamplerSceneWrapper* FieldSamplerManagerGPU::allocateFieldSamplerSceneWrapper(NiFieldSamplerScene* fieldSamplerScene)
{
	return PX_NEW(FieldSamplerSceneWrapperGPU)(mFieldSamplerSceneWrapperList, this, fieldSamplerScene);
}
FieldSamplerWrapper* FieldSamplerManagerGPU::allocateFieldSamplerWrapper(NiFieldSampler* fieldSampler, const NiFieldSamplerDesc& fieldSamplerDesc, FieldSamplerSceneWrapper* fieldSamplerSceneWrapper)
{
	return PX_NEW(FieldSamplerWrapperGPU)(mFieldSamplerWrapperList, this, fieldSampler, fieldSamplerDesc, fieldSamplerSceneWrapper);
}

#endif // defined(APEX_CUDA_SUPPORT)

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
