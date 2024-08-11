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

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2
#include "ModuleExplosion.h"
#include "ExplosionScene.h"
#include "ExplosionAsset.h"
#include "NiApexScene.h"
#include "PxMemoryBuffer.h"
#include "ModulePerfScope.h"
#include "NiModuleFieldSampler.h"
using namespace explosion;
#endif

#include "NxApex.h"
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
    physx::PxU32 APEXsdkVersion,
    physx::PxU32 PhysXsdkVersion,
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

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2
	gApexSdk = inSdk;
	APEX_INIT_FOUNDATION();
	initModuleProfiling(inSdk, "Explosion");
	ModuleExplosion* impl = PX_NEW(ModuleExplosion)(inSdk);
	*niRef  = (NiModule*) impl;
	return (NxModule*) impl;
#else // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
	if (errorCode != NULL)
	{
		*errorCode = APEX_CE_WRONG_VERSION;
	}

	PX_UNUSED(niRef);
	PX_UNUSED(inSdk);
	return NULL; // Force field Module can only compile against MIN_PHYSX_SDK_VERSION_REQUIRED and above
#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
}

#else
/* Statically linking entry function */
void instantiateModuleExplosion()
{
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2
	NiApexSDK* sdk = NiGetApexSDK();
	initModuleProfiling(sdk, "Explosion");
	explosion::ModuleExplosion* impl = PX_NEW(explosion::ModuleExplosion)(sdk);
	sdk->registerExternalModule((NxModule*) impl, (NiModule*) impl);
#endif
}
#endif // `defined(_USRDLL)

namespace explosion
{
/* === ModuleExplosion Implementation === */

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2

NxAuthObjTypeID ExplosionAsset::mAssetTypeID;  // Static class member of ExplosionAsset
#ifdef WITHOUT_APEX_AUTHORING

class ExplosionAssetDummyAuthoring : public NxApexAssetAuthoring, public UserAllocated
{
public:
	ExplosionAssetDummyAuthoring(ModuleExplosion* module, NxResourceList& list, NxParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	ExplosionAssetDummyAuthoring(ModuleExplosion* module, NxResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	ExplosionAssetDummyAuthoring(ModuleExplosion* module, NxResourceList& list)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
	}

	virtual void release()
	{
		destroy();
	}

	// internal
	void destroy()
	{
		delete this;
	}

	const char* getName(void) const
	{
		return NULL;
	}

	/**
	* \brief Returns the name of this APEX authorable object type
	*/
	virtual const char* getObjTypeName() const
	{
		return ExplosionAsset::getClassName();
	}

	/**
	 * \brief Prepares a fully authored Asset Authoring object for a specified platform
	*/
	virtual bool prepareForPlatform(physx::apex::NxPlatformTag)
	{
		PX_ASSERT(0);
		return false;
	}

	virtual void setToolString(const char* /*toolName*/, const char* /*toolVersion*/, PxU32 /*toolChangelist*/)
	{

	}

	/**
	* \brief Save asset's NxParameterized interface, may return NULL
	*/
	virtual NxParameterized::Interface* getNxParameterized() const
	{
		PX_ASSERT(0);
		return NULL;
	}

	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	}
};

typedef ApexAuthorableObject<ModuleExplosion, ExplosionAsset, ExplosionAssetDummyAuthoring> ExplosionAO;

#else
typedef ApexAuthorableObject<ModuleExplosion, ExplosionAsset, ExplosionAssetAuthoring> ExplosionAO;
#endif

ModuleExplosion::ModuleExplosion(NiApexSDK* inSdk)
{
	name = "Explosion";
	mSdk = inSdk;
	mApiProxy = this;
	mModuleParams = NULL;
	mFieldSamplerModule = NULL;

	/* Register asset type and create a namespace for its assets */
	const char* pName = ExplosionAssetParam::staticClassName();
	ExplosionAO* eAO = PX_NEW(ExplosionAO)(this, mAuthorableObjects, pName);
	ExplosionAsset::mAssetTypeID = eAO->getResID();

	/* Register the NxParameterized factories */
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
#	define PARAM_CLASS(clas) PARAM_CLASS_REGISTER_FACTORY(traits, clas)
#	include "ExplosionParamClasses.inc"

	registerLODParameter("Radius", NxRange<physx::PxU32>(1, 10));
}

ModuleExplosion::~ModuleExplosion()
{
	releaseModuleProfiling();
}

void ModuleExplosion::destroy()
{
	// release the NxParameterized factory
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
#		define PARAM_CLASS(clas) PARAM_CLASS_REMOVE_FACTORY(traits, clas)
#		include "ExplosionParamClasses.inc"
	}
}

NxParameterized::Interface* ModuleExplosion::getDefaultModuleDesc()
{
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (!mModuleParams)
	{
		mModuleParams = DYNAMIC_CAST(ExplosionModuleParameters*)
		                (traits->createNxParameterized("ExplosionModuleParameters"));
		PX_ASSERT(mModuleParams);
	}
	else
	{
		mModuleParams->initDefaults();
	}

	return mModuleParams;
}

void ModuleExplosion::init(const NxModuleExplosionDesc& expDesc)
{
	PX_PROFILER_PERF_SCOPE("ExplosionModuleInit");  // profile this function
	mModuleValue = expDesc.moduleValue;
}

NxAuthObjTypeID ModuleExplosion::getExplosionAssetTypeID() const
{
	return ExplosionAsset::mAssetTypeID;
}
NxAuthObjTypeID ModuleExplosion::getModuleID() const
{
	return ExplosionAsset::mAssetTypeID;
}


/* == Explosion Scene methods == */
NiModuleScene* ModuleExplosion::createNiModuleScene(NiApexScene& scene, NiApexRenderDebug* renderDebug)
{
#if defined(APEX_CUDA_SUPPORT)
	if (scene.getTaskManager()->getGpuDispatcher())
	{
		return PX_NEW(ExplosionSceneGPU)(*this, scene, renderDebug, mExplosionScenes);
	}
	else
#endif
		return PX_NEW(ExplosionSceneCPU)(*this, scene, renderDebug, mExplosionScenes);
}

void ModuleExplosion::releaseNiModuleScene(NiModuleScene& scene)
{
	ExplosionScene* es = DYNAMIC_CAST(ExplosionScene*)(&scene);
	es->destroy();
}

physx::PxU32 ModuleExplosion::forceLoadAssets()
{
	physx::PxU32 loadedAssetCount = 0;
	for (physx::PxU32 i = 0; i < mAuthorableObjects.getSize(); i++)
	{
		NiApexAuthorableObject* ao = static_cast<NiApexAuthorableObject*>(mAuthorableObjects.getResource(i));
		loadedAssetCount += ao->forceLoadAssets();
	}
	return loadedAssetCount;
}

ExplosionScene* ModuleExplosion::getExplosionScene(const NxApexScene& apexScene)
{
	for (physx::PxU32 i = 0 ; i < mExplosionScenes.getSize() ; i++)
	{
		ExplosionScene* es = DYNAMIC_CAST(ExplosionScene*)(mExplosionScenes.getResource(i));
		if (es->mApexScene == &apexScene)
		{
			return es;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate ExplosionScene");
	return NULL;
}

NxApexRenderableIterator* ModuleExplosion::createRenderableIterator(const NxApexScene& apexScene)
{
	ExplosionScene* es = getExplosionScene(apexScene);
	if (es)
	{
		return es->createRenderableIterator();
	}

	return NULL;
}

NiModuleFieldSampler* ModuleExplosion::getNiModuleFieldSampler()
{
	NxApexCreateError err;
	if (!mFieldSamplerModule && mSdk->createModule("FieldSampler", &err))
	{
		NiModule* nim = mSdk->getNiModuleByName("FieldSampler");
		if (nim)
		{
			mFieldSamplerModule = DYNAMIC_CAST(NiModuleFieldSampler*)(nim);
		}
	}
	return mFieldSamplerModule;
}

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

}
}
} // end namespace physx::apex
