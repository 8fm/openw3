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

#if NX_SDK_VERSION_MAJOR == 2
#include "ImpactExplosionEvent.h"
#endif

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
#include "NiModule.h"
#include "ModuleEmitter.h"
#include "ApexEmitterAsset.h"
#include "GroundEmitterAsset.h"
#include "ImpactEmitterAsset.h"
#include "NiApexScene.h"
#include "EmitterScene.h"
#include "PxMemoryBuffer.h"
#include "ApexEmitterActor.h"
#include "GroundEmitterActor.h"
#include "ImpactEmitterActor.h"
#include "ModulePerfScope.h"
using namespace emitter;
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

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
	gApexSdk = inSdk;
	APEX_INIT_FOUNDATION();
	initModuleProfiling(inSdk, "Emitter");
	ModuleEmitter* impl = PX_NEW(ModuleEmitter)(inSdk);
	*niRef  = (NiModule*) impl;
	return (NxModule*) impl;
#else
	if (errorCode != NULL)
	{
		*errorCode = APEX_CE_WRONG_VERSION;
	}

	PX_UNUSED(niRef);
	PX_UNUSED(inSdk);
	return NULL; // Emitter Module cannot use this PhysX version
#endif
}

#else /* !_USRDLL */

/* Statically linking entry function */
void instantiateModuleEmitter()
{
	NiApexSDK* sdk = NiGetApexSDK();
	initModuleProfiling(sdk, "Emitter");
	emitter::ModuleEmitter* impl = PX_NEW(emitter::ModuleEmitter)(sdk);
	sdk->registerExternalModule((NxModule*) impl, (NiModule*) impl);
}

#endif

namespace emitter
{
/* === ModuleEmitter Implementation === */

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

NxAuthObjTypeID ApexEmitterAsset::mAssetTypeID;
NxAuthObjTypeID GroundEmitterAsset::mAssetTypeID;
NxAuthObjTypeID ImpactEmitterAsset::mAssetTypeID;

NxAuthObjTypeID ModuleEmitter::getModuleID() const
{
	return ApexEmitterAsset::mAssetTypeID;
}
NxAuthObjTypeID ModuleEmitter::getEmitterAssetTypeID() const
{
	return ApexEmitterAsset::mAssetTypeID;
}

#ifdef WITHOUT_APEX_AUTHORING

class ApexEmitterAssetDummyAuthoring : public NxApexAssetAuthoring, public UserAllocated
{
public:
	ApexEmitterAssetDummyAuthoring(ModuleEmitter* module, NxResourceList& list, NxParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	ApexEmitterAssetDummyAuthoring(ModuleEmitter* module, NxResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	ApexEmitterAssetDummyAuthoring(ModuleEmitter* module, NxResourceList& list)
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
		return ApexEmitterAsset::getClassName();
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

class GroundEmitterAssetDummyAuthoring : public NxApexAssetAuthoring, public UserAllocated
{
public:
	GroundEmitterAssetDummyAuthoring(ModuleEmitter* module, NxResourceList& list, NxParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	GroundEmitterAssetDummyAuthoring(ModuleEmitter* module, NxResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	GroundEmitterAssetDummyAuthoring(ModuleEmitter* module, NxResourceList& list)
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
		return GroundEmitterAsset::getClassName();
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

class ImpactEmitterAssetDummyAuthoring : public NxApexAssetAuthoring, public UserAllocated
{
public:
	ImpactEmitterAssetDummyAuthoring(ModuleEmitter* module, NxResourceList& list, NxParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	ImpactEmitterAssetDummyAuthoring(ModuleEmitter* module, NxResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	ImpactEmitterAssetDummyAuthoring(ModuleEmitter* module, NxResourceList& list)
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
		return ImpactEmitterAsset::getClassName();
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



typedef ApexAuthorableObject<ModuleEmitter, ApexEmitterAsset, ApexEmitterAssetDummyAuthoring> ApexEmitterAO;
typedef ApexAuthorableObject<ModuleEmitter, GroundEmitterAsset, GroundEmitterAssetDummyAuthoring> GroundEmitterAO;
typedef ApexAuthorableObject<ModuleEmitter, ImpactEmitterAsset, ImpactEmitterAssetDummyAuthoring> ImpactEmitterAO;

#else
typedef ApexAuthorableObject<ModuleEmitter, ApexEmitterAsset, ApexEmitterAssetAuthoring> ApexEmitterAO;
typedef ApexAuthorableObject<ModuleEmitter, GroundEmitterAsset, GroundEmitterAssetAuthoring> GroundEmitterAO;
typedef ApexAuthorableObject<ModuleEmitter, ImpactEmitterAsset, ImpactEmitterAssetAuthoring> ImpactEmitterAO;
#endif

ModuleEmitter::ModuleEmitter(NiApexSDK* inSdk)
{
	name = "Emitter";
	mSdk = inSdk;
	mApiProxy = this;
	mModuleParams = NULL;

	/* Register asset type and create a namespace for its assets */
	const char* pName = ApexEmitterAssetParameters::staticClassName();
	ApexEmitterAO* eAO = PX_NEW(ApexEmitterAO)(this, mAuthorableObjects, pName);

	pName = GroundEmitterAssetParameters::staticClassName();
	GroundEmitterAO* geAO = PX_NEW(GroundEmitterAO)(this, mAuthorableObjects, pName);

	pName = ImpactEmitterAssetParameters::staticClassName();
	ImpactEmitterAO* ieAO = PX_NEW(ImpactEmitterAO)(this, mAuthorableObjects, pName);

	ApexEmitterAsset::mAssetTypeID = eAO->getResID();
	GroundEmitterAsset::mAssetTypeID = geAO->getResID();
	ImpactEmitterAsset::mAssetTypeID = ieAO->getResID();

	/* Register the NxParameterized factories */
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
#	define PARAM_CLASS(clas) PARAM_CLASS_REGISTER_FACTORY(traits, clas)
#	include "EmitterParamClasses.inc"

	// these need to be registered IN THIS ORDER
	registerLODParameter("Rate", NxRange<physx::PxU32>(1, 10));
	registerLODParameter("Density", NxRange<physx::PxU32>(1, 10));
	registerLODParameter("GroundDensity", NxRange<physx::PxU32>(1, 10));

}

ApexActor* ModuleEmitter::getApexActor(NxApexActor* nxactor, NxAuthObjTypeID type) const
{
	if (type == ApexEmitterAsset::mAssetTypeID)
	{
		return (ApexEmitterActor*) nxactor;
	}
	else if (type == GroundEmitterAsset::mAssetTypeID)
	{
		return (GroundEmitterActor*) nxactor;
	}
	else if (type == ImpactEmitterAsset::mAssetTypeID)
	{
		return (ImpactEmitterActor*) nxactor;
	}

	return NULL;
}

NxParameterized::Interface* ModuleEmitter::getDefaultModuleDesc()
{
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (!mModuleParams)
	{
		mModuleParams = DYNAMIC_CAST(EmitterModuleParameters*)
		                (traits->createNxParameterized("EmitterModuleParameters"));
		PX_ASSERT(mModuleParams);
	}
	else
	{
		mModuleParams->initDefaults();
	}

	return mModuleParams;
}

void ModuleEmitter::init(const NxModuleEmitterDesc& desc)
{
	PX_UNUSED(desc);
}

ModuleEmitter::~ModuleEmitter()
{
	releaseModuleProfiling();
}

void ModuleEmitter::destroy()
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
#		define PARAM_CLASS(clas) PARAM_CLASS_REMOVE_FACTORY(traits, clas)
#		include "EmitterParamClasses.inc"
	}
}

NiModuleScene* ModuleEmitter::createNiModuleScene(NiApexScene& scene, NiApexRenderDebug* debugRender)
{
	return PX_NEW(EmitterScene)(*this, scene, debugRender, mEmitterScenes);
}

void ModuleEmitter::releaseNiModuleScene(NiModuleScene& scene)
{
	EmitterScene* es = DYNAMIC_CAST(EmitterScene*)(&scene);
	es->destroy();
}

physx::PxU32 ModuleEmitter::forceLoadAssets()
{
	physx::PxU32 loadedAssetCount = 0;

	for (physx::PxU32 i = 0; i < mAuthorableObjects.getSize(); i++)
	{
		NiApexAuthorableObject* ao = static_cast<NiApexAuthorableObject*>(mAuthorableObjects.getResource(i));
		loadedAssetCount += ao->forceLoadAssets();
	}

	return loadedAssetCount;
}

EmitterScene* ModuleEmitter::getEmitterScene(const NxApexScene& apexScene)
{
	for (physx::PxU32 i = 0 ; i < mEmitterScenes.getSize() ; i++)
	{
		EmitterScene* es = DYNAMIC_CAST(EmitterScene*)(mEmitterScenes.getResource(i));
		if (es->mApexScene == &apexScene)
		{
			return es;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate EmitterScene");
	return NULL;
}

NxApexRenderableIterator* ModuleEmitter::createRenderableIterator(const NxApexScene& apexScene)
{
	EmitterScene* es = getEmitterScene(apexScene);
	if (es)
	{
		return es->createRenderableIterator();
	}

	return NULL;
}

#endif

}
}
} // namespace physx::apex
