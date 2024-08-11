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

#if NX_SDK_VERSION_MAJOR == 3
#include "ModuleParticleIos.h"
#include "ParticleIosScene.h"
#include "ParticleIosAsset.h"
#include "ParticleIosActor.h"
#include "NiApexScene.h"
#include "NiModuleIofx.h"
#include "NiModuleFieldSampler.h"
#include "PxMemoryBuffer.h"
#include "ModulePerfScope.h"
using namespace pxparticleios;
#if defined(PX_WINDOWS)
#include "PxPhysXGpu.h"
#endif
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

#if NX_SDK_VERSION_MAJOR == 3
	gApexSdk = inSdk;
	APEX_INIT_FOUNDATION();
	initModuleProfiling(inSdk, "ParticleIOS");
	ModuleParticleIos* impl = PX_NEW(ModuleParticleIos)(inSdk);
	*niRef  = (NiModule*) impl;
	return (NxModule*) impl;
#else
	if (errorCode != NULL)
	{
		*errorCode = APEX_CE_WRONG_VERSION;
	}

	PX_UNUSED(niRef);
	PX_UNUSED(inSdk);
	return NULL; // This module requires PhysX 3.0
#endif
}
#else
/* Statically linking entry function */
void instantiateModuleParticleIos()
{
#if NX_SDK_VERSION_MAJOR == 3
	NiApexSDK* sdk = NiGetApexSDK();
	initModuleProfiling(sdk, "ParticleIOS");
	pxparticleios::ModuleParticleIos* impl = PX_NEW(pxparticleios::ModuleParticleIos)(sdk);
	sdk->registerExternalModule((NxModule*) impl, (NiModule*) impl);
#endif
}
#endif

namespace pxparticleios
{
#if NX_SDK_VERSION_MAJOR == 3

/* =================== ModuleParticleIos =================== */


NxAuthObjTypeID ParticleIosAsset::mAssetTypeID;

#ifdef WITHOUT_APEX_AUTHORING

class ParticleIosAssetDummyAuthoring : public NxApexAssetAuthoring, public UserAllocated
{
public:
	ParticleIosAssetDummyAuthoring(ModuleParticleIos* module, NxResourceList& list, NxParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	ParticleIosAssetDummyAuthoring(ModuleParticleIos* module, NxResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	ParticleIosAssetDummyAuthoring(ModuleParticleIos* module, NxResourceList& list)
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
	* \brief Save asset configuration to a stream
	*/
	virtual physx::PxFileBuf& serialize(physx::PxFileBuf& stream) const
	{
		PX_ASSERT(0);
		return stream;
	}

	/**
	* \brief Load asset configuration from a stream
	*/
	virtual physx::PxFileBuf& deserialize(physx::PxFileBuf& stream)
	{
		PX_ASSERT(0);
		return stream;
	}

	/**
	* \brief Returns the name of this APEX authorable object type
	*/
	virtual const char* getObjTypeName() const
	{
		return ParticleIosAsset::getClassName();
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

typedef ApexAuthorableObject<ModuleParticleIos, ParticleIosAsset, ParticleIosAssetDummyAuthoring> ParticleIosAO;

#else
typedef ApexAuthorableObject<ModuleParticleIos, ParticleIosAsset, ParticleIosAssetAuthoring> ParticleIosAO;
#endif

ModuleParticleIos::ModuleParticleIos(NiApexSDK* sdk)
{
	mSdk = sdk;
	mApiProxy = this;
	name = "ParticleIOS";
	mModuleParams = NULL;
	mIofxModule = NULL;
	mFieldSamplerModule = NULL;

	/* Register this module's authorable object types and create their namespaces */
	const char* pName = ParticleIosAssetParam::staticClassName();
	ParticleIosAO* eAO = PX_NEW(ParticleIosAO)(this, mAuthorableObjects,  pName);
	ParticleIosAsset::mAssetTypeID = eAO->getResID();

	/* Register the NxParameterized factories */
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
#	define PARAM_CLASS(clas) PARAM_CLASS_REGISTER_FACTORY(traits, clas)
#	include "PxparticleiosParamClasses.inc"

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

	// LOD Hack: remove once a proper benefit calculation is in place
	mLodEnabled = false;
}

NxAuthObjTypeID ModuleParticleIos::getModuleID() const
{
	return ParticleIosAsset::mAssetTypeID;
}

ModuleParticleIos::~ModuleParticleIos()
{
	releaseModuleProfiling();
}

void ModuleParticleIos::destroy()
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
#		include "PxparticleiosParamClasses.inc"
	}
}


void ModuleParticleIos::init(const NxModuleParticleIosDesc&)
{
}

void ModuleParticleIos::init(NxParameterized::Interface&)
{
	NxModuleParticleIosDesc desc;
	init(desc);
}

NxParameterized::Interface* ModuleParticleIos::getDefaultModuleDesc()
{
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (!mModuleParams)
	{
		mModuleParams = DYNAMIC_CAST(ParticleIosModuleParameters*)
		                (traits->createNxParameterized("ParticleIosModuleParameters"));
		PX_ASSERT(mModuleParams);
	}
	else
	{
		mModuleParams->initDefaults();
	}

	return mModuleParams;
}

physx::PxU32 ModuleParticleIos::forceLoadAssets()
{
	return 0;
}

ParticleIosScene* ModuleParticleIos::getParticleIosScene(const NxApexScene& apexScene)
{
	const NiApexScene* niScene = DYNAMIC_CAST(const NiApexScene*)(&apexScene);
	for (physx::PxU32 i = 0 ; i < mParticleIosSceneList.getSize() ; i++)
	{
		ParticleIosScene* ps = DYNAMIC_CAST(ParticleIosScene*)(mParticleIosSceneList.getResource(i));
		if (ps->mApexScene == niScene)
		{
			return ps;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate ParticleIosScene");
	return NULL;
}

const ParticleIosScene* ModuleParticleIos::getParticleIosScene(const NxApexScene& apexScene) const
{
	const NiApexScene* niScene = DYNAMIC_CAST(const NiApexScene*)(&apexScene);
	for (physx::PxU32 i = 0 ; i < mParticleIosSceneList.getSize() ; i++)
	{
		ParticleIosScene* ps = DYNAMIC_CAST(ParticleIosScene*)(mParticleIosSceneList.getResource(i));
		if (ps->mApexScene == niScene)
		{
			return ps;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate ParticleIosScene");
	return NULL;
}

NxApexRenderableIterator* ModuleParticleIos::createRenderableIterator(const NxApexScene& apexScene)
{
	ParticleIosScene* ps = getParticleIosScene(apexScene);
	if (ps)
	{
		return ps->createRenderableIterator();
	}

	return NULL;
}

NiModuleScene* ModuleParticleIos::createNiModuleScene(NiApexScene& scene, NiApexRenderDebug* renderDebug)
{
#if defined(APEX_CUDA_SUPPORT)
	if (scene.getTaskManager()->getGpuDispatcher() && scene.isUsingCuda())
	{
		return PX_NEW(ParticleIosSceneGPU)(*this, scene, renderDebug, mParticleIosSceneList);
	}
	else
#endif
	{
		return PX_NEW(ParticleIosSceneCPU)(*this, scene, renderDebug, mParticleIosSceneList);
	}
}

void ModuleParticleIos::releaseNiModuleScene(NiModuleScene& scene)
{
	ParticleIosScene* ps = DYNAMIC_CAST(ParticleIosScene*)(&scene);
	ps->destroy();
}

const char* ModuleParticleIos::getParticleIosTypeName()
{
	return ParticleIosAsset::getClassName();
}


ApexActor* ModuleParticleIos::getApexActor(NxApexActor* nxactor, NxAuthObjTypeID type) const
{
	if (type == ParticleIosAsset::mAssetTypeID)
	{
		return (ParticleIosActor*) nxactor;
	}

	return NULL;
}

NiModuleIofx* ModuleParticleIos::getNiModuleIofx()
{
	if (!mIofxModule)
	{
		NiModule* nim = mSdk->getNiModuleByName("IOFX");
		if (nim)
		{
			mIofxModule = DYNAMIC_CAST(NiModuleIofx*)(nim);
		}
	}
	return mIofxModule;
}

NiModuleFieldSampler* ModuleParticleIos::getNiModuleFieldSampler()
{
	if (!mFieldSamplerModule )
	{
		NiModule* nim = mSdk->getNiModuleByName("FieldSampler");
		if (nim)
		{
			mFieldSamplerModule = DYNAMIC_CAST(NiModuleFieldSampler*)(nim);
		}
	}
	return mFieldSamplerModule;
}

#endif

}
}
} // namespace physx::apex
