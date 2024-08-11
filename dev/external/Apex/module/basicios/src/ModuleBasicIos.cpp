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

#include "NxApex.h"
#include "NiApexSDK.h"
#include "ModuleBasicIos.h"
#include "ModulePerfScope.h"
#include "PsShare.h"
#include "BasicIosScene.h"
#include "BasicIosAsset.h"
#include "BasicIosActor.h"

#include "NiApexScene.h"
#include "NiModuleIofx.h"
#include "NiModuleFieldSampler.h"
#include "PxMemoryBuffer.h"

using namespace basicios;

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

	/* Setup common module global variables */
	gApexSdk = inSdk;
	APEX_INIT_FOUNDATION();
	initModuleProfiling(inSdk, "BasicIOS");

	ModuleBasicIos* impl = PX_NEW(ModuleBasicIos)(inSdk);
	*niRef  = (NiModule*) impl;
	return (NxModule*) impl;
}
#else
/* Statically linking entry function */
void instantiateModuleBasicIos()
{
	NiApexSDK* sdk = NiGetApexSDK();
	initModuleProfiling(sdk, "BasicIOS");
	basicios::ModuleBasicIos* impl = PX_NEW(basicios::ModuleBasicIos)(sdk);
	sdk->registerExternalModule((NxModule*) impl, (NiModule*) impl);
}
#endif


namespace basicios
{
/* =================== ModuleBasicIos =================== */


NxAuthObjTypeID BasicIosAsset::mAssetTypeID;

#ifdef WITHOUT_APEX_AUTHORING

class BasicIosAssetDummyAuthoring : public NxApexAssetAuthoring, public UserAllocated
{
public:
	BasicIosAssetDummyAuthoring(ModuleBasicIos* module, NxResourceList& list, NxParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	BasicIosAssetDummyAuthoring(ModuleBasicIos* module, NxResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	BasicIosAssetDummyAuthoring(ModuleBasicIos* module, NxResourceList& list)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
	}

	virtual void setToolString(const char* /*toolName*/, const char* /*toolVersion*/, PxU32 /*toolChangelist*/)
	{

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

	/**
	* \brief Returns the name of this APEX authorable object type
	*/
	virtual const char* getObjTypeName() const
	{
		return BasicIosAsset::getClassName();
	}

	/**
	 * \brief Prepares a fully authored Asset Authoring object for a specified platform
	*/
	virtual bool prepareForPlatform(physx::apex::NxPlatformTag)
	{
		PX_ASSERT(0);
		return false;
	}

	const char* getName(void) const
	{
		return NULL;
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

typedef ApexAuthorableObject<ModuleBasicIos, BasicIosAsset, BasicIosAssetDummyAuthoring> BasicIOSAO;

#else
typedef ApexAuthorableObject<ModuleBasicIos, BasicIosAsset, BasicIosAssetAuthoring> BasicIOSAO;
#endif

ModuleBasicIos::ModuleBasicIos(NiApexSDK* sdk)
{
	mSdk = sdk;
	mApiProxy = this;
	name = "BasicIOS";
	mModuleParams = NULL;
	mIofxModule = NULL;
	mFieldSamplerModule = NULL;

	/* Register this module's authorable object types and create their namespaces */
	const char* pName = BasicIOSAssetParam::staticClassName();
	BasicIOSAO* eAO = PX_NEW(BasicIOSAO)(this, mAuthorableObjects,  pName);
	BasicIosAsset::mAssetTypeID = eAO->getResID();

	/* Register the NxParameterized factories */
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
#	define PARAM_CLASS(clas) PARAM_CLASS_REGISTER_FACTORY(traits, clas)
#	include "BasiciosParamClasses.inc"

	// Set per-platform unit cost.  One unit is one cloth vertex times one solver iteration
#if defined( PX_WINDOWS )
	mLodUnitCost = 0.0001f;
#elif defined( PX_X360 )
	mLodUnitCost = 0.001f;
#elif defined( PX_PS3 )
	mLodUnitCost = 0.001f;
#elif defined( PX_ANDROID )
	mLodUnitCost = 0.001f;
#else
	// Using default value set in Module class
#endif
}

NxAuthObjTypeID ModuleBasicIos::getModuleID() const
{
	return BasicIosAsset::mAssetTypeID;
}

ModuleBasicIos::~ModuleBasicIos()
{
	releaseModuleProfiling();
}

void ModuleBasicIos::destroy()
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
#		include "BasiciosParamClasses.inc"
	}
}

void ModuleBasicIos::init(const NxModuleBasicIosDesc&)
{
}

void ModuleBasicIos::init(NxParameterized::Interface&)
{
	NxModuleBasicIosDesc desc;
	init(desc);
}

NxParameterized::Interface* ModuleBasicIos::getDefaultModuleDesc()
{
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (!mModuleParams)
	{
		mModuleParams = DYNAMIC_CAST(BasicIosModuleParameters*)
		                (traits->createNxParameterized("BasicIosModuleParameters"));
		PX_ASSERT(mModuleParams);
	}
	else
	{
		mModuleParams->initDefaults();
	}

	return mModuleParams;
}

physx::PxU32 ModuleBasicIos::forceLoadAssets()
{
	return 0;
}

BasicIosScene* ModuleBasicIos::getBasicIosScene(const NxApexScene& apexScene)
{
	const NiApexScene* niScene = DYNAMIC_CAST(const NiApexScene*)(&apexScene);
	for (physx::PxU32 i = 0 ; i < mBasicIosSceneList.getSize() ; i++)
	{
		BasicIosScene* ps = DYNAMIC_CAST(BasicIosScene*)(mBasicIosSceneList.getResource(i));
		if (ps->mApexScene == niScene)
		{
			return ps;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate BasicIosScene");
	return NULL;
}

const BasicIosScene* ModuleBasicIos::getBasicIosScene(const NxApexScene& apexScene) const
{
	const NiApexScene* niScene = DYNAMIC_CAST(const NiApexScene*)(&apexScene);
	for (physx::PxU32 i = 0 ; i < mBasicIosSceneList.getSize() ; i++)
	{
		BasicIosScene* ps = DYNAMIC_CAST(BasicIosScene*)(mBasicIosSceneList.getResource(i));
		if (ps->mApexScene == niScene)
		{
			return ps;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate BasicIosScene");
	return NULL;
}

NxApexRenderableIterator* ModuleBasicIos::createRenderableIterator(const NxApexScene& apexScene)
{
	BasicIosScene* ps = getBasicIosScene(apexScene);
	if (ps)
	{
		return ps->createRenderableIterator();
	}

	return NULL;
}

NiModuleScene* ModuleBasicIos::createNiModuleScene(NiApexScene& scene, NiApexRenderDebug* renderDebug)
{
#if defined(APEX_CUDA_SUPPORT)
	if (scene.getTaskManager()->getGpuDispatcher() && scene.isUsingCuda())
	{
		return PX_NEW(BasicIosSceneGPU)(*this, scene, renderDebug, mBasicIosSceneList);
	}
	else
#endif
	{
		return PX_NEW(BasicIosSceneCPU)(*this, scene, renderDebug, mBasicIosSceneList);
	}
}

void ModuleBasicIos::releaseNiModuleScene(NiModuleScene& scene)
{
	BasicIosScene* ps = DYNAMIC_CAST(BasicIosScene*)(&scene);
	ps->destroy();
}

const char* ModuleBasicIos::getBasicIosTypeName()
{
	return BasicIosAsset::getClassName();
}


ApexActor* ModuleBasicIos::getApexActor(NxApexActor* nxactor, NxAuthObjTypeID type) const
{
	if (type == BasicIosAsset::mAssetTypeID)
	{
		return static_cast<BasicIosActor*>(nxactor);
	}

	return NULL;
}

NiModuleIofx* ModuleBasicIos::getNiModuleIofx()
{
	if (!mIofxModule )
	{
		NiModule* nim = mSdk->getNiModuleByName("IOFX");
		if (nim)
		{
			mIofxModule = DYNAMIC_CAST(NiModuleIofx*)(nim);
		}
	}
	return mIofxModule;
}

NiModuleFieldSampler* ModuleBasicIos::getNiModuleFieldSampler()
{
	if (!mFieldSamplerModule)
	{
		NiModule* nim = mSdk->getNiModuleByName("FieldSampler");
		if (nim)
		{
			mFieldSamplerModule = DYNAMIC_CAST(NiModuleFieldSampler*)(nim);
		}
	}
	return mFieldSamplerModule;
}
#ifdef APEX_TEST
NxBasicIosActor* ModuleBasicIos::getActor(NxApexScene* apexScene) const
{
	const BasicIosScene* ps = getBasicIosScene(*apexScene);
	return ps->getApexActor(0);
}
#endif
}
}
} // namespace physx::apex
