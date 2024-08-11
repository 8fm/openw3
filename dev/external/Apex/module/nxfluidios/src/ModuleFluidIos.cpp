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

#if NX_SDK_VERSION_MAJOR == 2
#include "ModuleFluidIos.h"
#include "FluidIosScene.h"
#include "FluidIosAsset.h"
#include "FluidIosActor.h"
#include "NiApexScene.h"
#include "NiModuleIofx.h"
#include "PxMemoryBuffer.h"
#include "ModulePerfScope.h"
using namespace nxfluidios;
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

#if NX_SDK_VERSION_MAJOR == 2
	gApexSdk = inSdk;
	APEX_INIT_FOUNDATION();
	initModuleProfiling(inSdk, "NxFluidIOS");
	ModuleFluidIos* impl = PX_NEW(ModuleFluidIos)(inSdk);
	*niRef  = (NiModule*) impl;
	return (NxModule*) impl;
#else
	if (errorCode != NULL)
	{
		*errorCode = APEX_CE_WRONG_VERSION;
	}

	PX_UNUSED(niRef);
	PX_UNUSED(inSdk);
	return NULL; // This module requires PhysX 2.x
#endif
}
#else
/* Statically linking entry function */
void instantiateModuleFluidIos()
{
#if NX_SDK_VERSION_MAJOR == 2
	NiApexSDK* sdk = NiGetApexSDK();
	initModuleProfiling(sdk, "NxFluidIOS");
	nxfluidios::ModuleFluidIos* impl = PX_NEW(nxfluidios::ModuleFluidIos)(sdk);
	sdk->registerExternalModule((NxModule*) impl, (NiModule*) impl);
#endif // NX_SDK_VERSION_MAJOR == 2
}
#endif

namespace nxfluidios
{
#if NX_SDK_VERSION_MAJOR == 2
/* =================== ModuleFluidIos =================== */


NxAuthObjTypeID FluidIosAsset::mAssetTypeID;
#ifdef WITHOUT_APEX_AUTHORING

class FluidIosAssetDummyAuthoring : public NxApexAssetAuthoring, public UserAllocated
{
public:
	FluidIosAssetDummyAuthoring(ModuleFluidIos* module, NxResourceList& list, NxParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	FluidIosAssetDummyAuthoring(ModuleFluidIos* module, NxResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	FluidIosAssetDummyAuthoring(ModuleFluidIos* module, NxResourceList& list)
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

	const char* getName(void) const
	{
		return NULL;
	}

	/**
	* \brief Returns the name of this APEX authorable object type
	*/
	virtual const char* getObjTypeName() const
	{
		return FluidIosAsset::getClassName();
	}

	/**
	 * \brief Prepares a fully authored Asset Authoring object for a specified platform
	 */
	virtual bool prepareForPlatform(physx::apex::NxPlatformTag)
	{
		PX_ASSERT(0);
		return false;
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

typedef ApexAuthorableObject<ModuleFluidIos, FluidIosAsset, FluidIosAssetDummyAuthoring> FluidIosAO;

#else
typedef ApexAuthorableObject<ModuleFluidIos, FluidIosAsset, FluidIosAssetAuthoring> FluidIosAO;
#endif

ModuleFluidIos::ModuleFluidIos(NiApexSDK* sdk)
{
	mSdk = sdk;
	mApiProxy = this;
	name = "NxFluidIOS";
	mModuleParams = NULL;
	mIofxModule = NULL;

	/* Register this module's authorable object types and create their namespaces */
	const char* pName = NxFluidIosParameters::staticClassName();
	FluidIosAO* AO = PX_NEW(FluidIosAO)(this, mAuthorableObjects, pName);
	FluidIosAsset::mAssetTypeID = AO->getResID();

	/* Register the NxParameterized factories */
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
#	define PARAM_CLASS(clas) PARAM_CLASS_REGISTER_FACTORY(traits, clas)
#	include "NxfluidiosParamClasses.inc"

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

NxAuthObjTypeID ModuleFluidIos::getModuleID() const
{
	return FluidIosAsset::mAssetTypeID;
}

ModuleFluidIos::~ModuleFluidIos()
{
	releaseModuleProfiling();
}

void ModuleFluidIos::destroy()
{
	/* Remove the NxParameterized factories */
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
#		include "NxfluidiosParamClasses.inc"
	}
}

NxParameterized::Interface* ModuleFluidIos::getDefaultModuleDesc()
{
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (!mModuleParams)
	{
		mModuleParams = DYNAMIC_CAST(FluidIosModuleParameters*)
		                (traits->createNxParameterized("FluidIosModuleParameters"));
		PX_ASSERT(mModuleParams);
	}
	else
	{
		mModuleParams->initDefaults();
	}

	return mModuleParams;
}

NiModuleIofx* ModuleFluidIos::getNiModuleIofx()
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

void ModuleFluidIos::init(const NxModuleFluidIosDesc&)
{
}


physx::PxU32 ModuleFluidIos::forceLoadAssets()
{
	return 0;
}

FluidIosScene* ModuleFluidIos::getParticleScene(const NxApexScene& apexScene)
{
	const NiApexScene* niScene = DYNAMIC_CAST(const NiApexScene*)(&apexScene);
	for (physx::PxU32 i = 0 ; i < mParticleSceneList.getSize() ; i++)
	{
		FluidIosScene* ps = DYNAMIC_CAST(FluidIosScene*)(mParticleSceneList.getResource(i));
		if (ps->mApexScene == niScene)
		{
			return ps;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate FluidIosScene");
	return NULL;
}

const FluidIosScene* ModuleFluidIos::getParticleScene(const NxApexScene& apexScene) const
{
	const NiApexScene* niScene = DYNAMIC_CAST(const NiApexScene*)(&apexScene);
	for (physx::PxU32 i = 0 ; i < mParticleSceneList.getSize() ; i++)
	{
		FluidIosScene* ps = DYNAMIC_CAST(FluidIosScene*)(mParticleSceneList.getResource(i));
		if (ps->mApexScene == niScene)
		{
			return ps;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate FluidIosScene");
	return NULL;
}

NxApexRenderableIterator* ModuleFluidIos::createRenderableIterator(const NxApexScene& apexScene)
{
	FluidIosScene* ps = getParticleScene(apexScene);
	if (ps)
	{
		return ps->createRenderableIterator();
	}

	return NULL;
}

NiModuleScene* ModuleFluidIos::createNiModuleScene(NiApexScene& scene, NiApexRenderDebug* renderDebug)
{
	return PX_NEW(FluidIosScene)(*this, scene, renderDebug, mParticleSceneList);
}

void ModuleFluidIos::releaseNiModuleScene(NiModuleScene& scene)
{
	FluidIosScene* ps = DYNAMIC_CAST(FluidIosScene*)(&scene);
	ps->destroy();
}

const char* ModuleFluidIos::getFluidIosTypeName()
{
	return FluidIosAsset::getClassName();
}

void ModuleFluidIos::setCompartment(const NxApexScene& scene, NxCompartment& comp)
{
	FluidIosScene* ps = getParticleScene(scene);
	if (ps)
	{
		ps->setCompartment(comp);
	}
}

const NxCompartment* ModuleFluidIos::getCompartment(const NxApexScene& scene) const
{
	const FluidIosScene* ps = getParticleScene(scene);
	if (ps)
	{
		return ps->getCompartment();
	}

	return NULL;
}

void ModuleFluidIos::setSPHCompartment(const NxApexScene& scene, NxCompartment& comp)
{
	FluidIosScene* ps = getParticleScene(scene);
	if (ps)
	{
		ps->setSPHCompartment(comp);
	}
}

const NxCompartment* ModuleFluidIos::getSPHCompartment(const NxApexScene& scene) const
{
	const FluidIosScene* ps = getParticleScene(scene);
	if (ps)
	{
		return ps->getSPHCompartment();
	}

	return NULL;
}

ApexActor* ModuleFluidIos::getApexActor(NxApexActor* nxactor, NxAuthObjTypeID type) const
{
	if (type == FluidIosAsset::mAssetTypeID)
	{
		return (FluidIosActor*) nxactor;
	}

	return NULL;
}

#endif //NX_SDK_VERSION_MAJOR == 2

}
}
} // namespace physx::apex
