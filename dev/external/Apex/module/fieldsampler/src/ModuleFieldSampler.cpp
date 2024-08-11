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
#include "ModuleFieldSampler.h"
#include "FieldSamplerScene.h"
#include "FieldSamplerManager.h"
#include "NiApexScene.h"
#include "PxMemoryBuffer.h"
#include "ModulePerfScope.h"
using namespace fieldsampler;
#endif

#include "NiApexSDK.h"
#include "PsShare.h"

namespace physx
{
namespace apex
{

#if defined(_USRDLL)

/* Modules don't have to link against the framework, they keep their own */
NiApexSDK* gApexSdk = 0;
NxApexSDK* NxGetApexSDK()
{
	return gApexSdk;
}
NiApexSDK* NiGetApexSDK()
{
	return gApexSdk;
}

NXAPEX_API NxModule*  NX_CALL_CONV createModule(
    NiApexSDK* inSdk,
    NiModule** niRef,
    PxU32 APEXsdkVersion,
    PxU32 PhysXsdkVersion,
    NxApexCreateError* errorCode)
{
	if (APEXsdkVersion != NX_APEX_SDK_VERSION)
	{
		if (errorCode)
		{
			*errorCode = APEX_CE_WRONG_VERSION;
		}
		return NULL;
	}

	if (PhysXsdkVersion != NX_PHYSICS_SDK_VERSION)
	{
		if (errorCode)
		{
			*errorCode = APEX_CE_WRONG_VERSION;
		}
		return NULL;
	}

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
	gApexSdk = inSdk;
	APEX_INIT_FOUNDATION();
	initModuleProfiling(inSdk, "FieldSampler");
	ModuleFieldSampler* impl = PX_NEW(ModuleFieldSampler)(inSdk);
	*niRef  = (NiModule*) impl;
	return (NxModule*) impl;
#else // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
	if (errorCode != NULL)
	{
		*errorCode = APEX_CE_WRONG_VERSION;
	}

	PX_UNUSED(niRef);
	PX_UNUSED(inSdk);
	return NULL; // FieldSampler Module can only compile against 283
#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
}

#else
/* Statically linking entry function */
void instantiateModuleFieldSampler()
{
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
	NiApexSDK* sdk = NiGetApexSDK();
	initModuleProfiling(sdk, "FieldSampler");
	fieldsampler::ModuleFieldSampler* impl = PX_NEW(fieldsampler::ModuleFieldSampler)(sdk);
	sdk->registerExternalModule((NxModule*) impl, (NiModule*) impl);
#endif
}
#endif // `defined(_USRDLL)

namespace fieldsampler
{
/* === ModuleFieldSampler Implementation === */
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

ModuleFieldSampler::ModuleFieldSampler(NiApexSDK* sdk)
{
	name = "FieldSampler";
	mSdk = sdk;
	mApiProxy = this;
	mModuleParams = NULL;

	/* Register the NxParameterized factories */
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
#	define PARAM_CLASS(clas) PARAM_CLASS_REGISTER_FACTORY(traits, clas)
#	include "FieldsamplerParamClasses.inc"
}

ModuleFieldSampler::~ModuleFieldSampler()
{
	releaseModuleProfiling();
}

void ModuleFieldSampler::destroy()
{
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (mModuleParams)
	{
		mModuleParams->destroy();
		mModuleParams = NULL;
	}

	Module::destroy();
	delete this;

	if (traits)
	{
		/* Remove the NxParameterized factories */

#		define PARAM_CLASS(clas) PARAM_CLASS_REMOVE_FACTORY(traits, clas)
#		include "FieldsamplerParamClasses.inc"
	}
}

void ModuleFieldSampler::init(NxParameterized::Interface&)
{
}

NxParameterized::Interface* ModuleFieldSampler::getDefaultModuleDesc()
{
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (!mModuleParams)
	{
		mModuleParams = DYNAMIC_CAST(FieldSamplerModuleParameters*)
		                (traits->createNxParameterized("FieldSamplerModuleParameters"));
		PX_ASSERT(mModuleParams);
	}
	else
	{
		mModuleParams->initDefaults();
	}

	return mModuleParams;
}

NiFieldSamplerManager* ModuleFieldSampler::getNiFieldSamplerManager(const NxApexScene& apexScene)
{
	FieldSamplerScene* scene = ModuleFieldSampler::getFieldSamplerScene(apexScene);
	return scene->getManager();
}


NxAuthObjTypeID ModuleFieldSampler::getModuleID() const
{
	return 0;
}


/* == Example Scene methods == */
NiModuleScene* ModuleFieldSampler::createNiModuleScene(NiApexScene& scene, NiApexRenderDebug* debugRender)
{
#if defined(APEX_CUDA_SUPPORT)
	if (scene.getTaskManager()->getGpuDispatcher() && scene.isUsingCuda())
	{
		return PX_NEW(FieldSamplerSceneGPU)(*this, scene, debugRender, mFieldSamplerScenes);
	}
	else
#endif
		return PX_NEW(FieldSamplerSceneCPU)(*this, scene, debugRender, mFieldSamplerScenes);
}

void ModuleFieldSampler::releaseNiModuleScene(NiModuleScene& scene)
{
	FieldSamplerScene* es = DYNAMIC_CAST(FieldSamplerScene*)(&scene);
	es->destroy();
}

fieldsampler::FieldSamplerScene* ModuleFieldSampler::getFieldSamplerScene(const NxApexScene& apexScene) const
{
	for (PxU32 i = 0 ; i < mFieldSamplerScenes.getSize() ; i++)
	{
		FieldSamplerScene* es = DYNAMIC_CAST(FieldSamplerScene*)(mFieldSamplerScenes.getResource(i));
		if (es->mApexScene == &apexScene)
		{
			return es;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate FieldSamplerScene");
	return NULL;
}

NxApexRenderableIterator* ModuleFieldSampler::createRenderableIterator(const NxApexScene& apexScene)
{
	FieldSamplerScene* es = getFieldSamplerScene(apexScene);
	if (es)
	{
		return es->createRenderableIterator();
	}

	return NULL;
}

bool ModuleFieldSampler::setFieldBoundaryGroupsFilteringParams(const NxApexScene& apexScene ,
        const NxGroupsFilteringParams64& params)
{
	FieldSamplerScene* scene = getFieldSamplerScene(apexScene);
	if (scene != NULL)
	{
		DYNAMIC_CAST(FieldSamplerManager*)(scene->getManager())->setFieldBoundaryGroupsFilteringParams(params);
		return true;
	}
	return false;
}

bool ModuleFieldSampler::getFieldBoundaryGroupsFilteringParams(const NxApexScene& apexScene ,
        NxGroupsFilteringParams64& params) const
{
	FieldSamplerScene* scene = getFieldSamplerScene(apexScene);
	if (scene != NULL)
	{
		DYNAMIC_CAST(FieldSamplerManager*)(scene->getManager())->getFieldBoundaryGroupsFilteringParams(params);
		return true;
	}
	return false;
}

bool ModuleFieldSampler::setFieldSamplerGroupsFilteringParams(const NxApexScene& apexScene ,
        const NxGroupsFilteringParams64& params)
{
	FieldSamplerScene* scene = getFieldSamplerScene(apexScene);
	if (scene != NULL)
	{
		DYNAMIC_CAST(FieldSamplerManager*)(scene->getManager())->setFieldSamplerGroupsFilteringParams(params);
		return true;
	}
	return false;
}

bool ModuleFieldSampler::getFieldSamplerGroupsFilteringParams(const NxApexScene& apexScene ,
        NxGroupsFilteringParams64& params) const
{
	FieldSamplerScene* scene = getFieldSamplerScene(apexScene);
	if (scene != NULL)
	{
		DYNAMIC_CAST(FieldSamplerManager*)(scene->getManager())->getFieldSamplerGroupsFilteringParams(params);
		return true;
	}
	return false;
}

#if NX_SDK_VERSION_MAJOR == 3
void ModuleFieldSampler::enablePhysXMonitor(const NxApexScene& apexScene, bool enable)
{
	getFieldSamplerScene(apexScene)->enablePhysXMonitor(enable);	
}

void ModuleFieldSampler::addPhysXMonitorFilterData(const NxApexScene& apexScene, physx::PxFilterData filterData)
{
	getFieldSamplerScene(apexScene)->addPhysXFilterData(filterData);
}

void ModuleFieldSampler::removePhysXMonitorFilterData(const NxApexScene& apexScene, physx::PxFilterData filterData)
{
	getFieldSamplerScene(apexScene)->removePhysXFilterData(filterData);
}
#endif

#ifdef APEX_TEST

bool ModuleFieldSampler::setPhysXMonitorParticlesData(const NxApexScene& apexScene, PxU32 numParticles, PxVec4** positions, PxVec4** velocities)
{
	return getFieldSamplerScene(apexScene)->setPhysXMonitorParticlesData(numParticles, positions, velocities);
}

void ModuleFieldSampler::getPhysXMonitorParticlesData(const NxApexScene& apexScene, PxVec4** velocities)
{
	getFieldSamplerScene(apexScene)->getPhysXMonitorParticlesData(velocities);
}
#endif

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

}
}
} // end namespace physx::apex
