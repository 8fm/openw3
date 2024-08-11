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

#include "NxApex.h"

/* === ModuleBasicFS DLL Setup === */

#include "ModuleBasicFS.h"
#include "ModulePerfScope.h"
//#include "BasicFSAsset.h"
#include "JetFSAsset.h"
#include "AttractorFSAsset.h"
#include "VortexFSAsset.h"
#include "NoiseFSAsset.h"
#include "BasicFSScene.h"
#include "NiApexScene.h"
#include "PxMemoryBuffer.h"
//#include "BasicFSActor.h"
#include "JetFSActor.h"
#include "AttractorFSActor.h"
#include "VortexFSActor.h"
#include "NoiseFSActor.h"
#include "NiModuleFieldSampler.h"

using namespace basicfs;

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
	initModuleProfiling(inSdk, "BasicFS");
	ModuleBasicFS* impl = PX_NEW(ModuleBasicFS)(inSdk);
	*niRef  = (NiModule*) impl;
	return (NxModule*) impl;
#else // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
	if (errorCode != NULL)
	{
		*errorCode = APEX_CE_WRONG_VERSION;
	}

	PX_UNUSED(niRef);
	PX_UNUSED(inSdk);
	return NULL;
#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
}

#else
/* Statically linking entry function */
void instantiateModuleBasicFS()
{
	NiApexSDK* sdk = NiGetApexSDK();
	initModuleProfiling(sdk, "BasicFS");
	basicfs::ModuleBasicFS* impl = PX_NEW(basicfs::ModuleBasicFS)(sdk);
	sdk->registerExternalModule((NxModule*) impl, (NiModule*) impl);
}
#endif // `defined(_USRDLL)

namespace basicfs
{
/* === ModuleBasicFS Implementation === */
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

#ifdef WITHOUT_APEX_AUTHORING

class BasicFSAssetDummyAuthoring : public NxApexAssetAuthoring, public UserAllocated
{
public:
	BasicFSAssetDummyAuthoring(ModuleBasicFS* module, NxResourceList& list, NxParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	BasicFSAssetDummyAuthoring(ModuleBasicFS* module, NxResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	BasicFSAssetDummyAuthoring(ModuleBasicFS* module, NxResourceList& list)
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
		return JetFSAsset::getClassName(); // Fix
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

typedef ApexAuthorableObject<ModuleBasicFS, JetFSAsset, JetFSAssetAuthoring> JetFSAO;
typedef ApexAuthorableObject<ModuleBasicFS, AttractorFSAsset, AttractorFSAssetAuthoring> AttractorFSAO;
typedef ApexAuthorableObject<ModuleBasicFS, VortexFSAsset, VortexFSAssetAuthoring> VortexFSAO;
typedef ApexAuthorableObject<ModuleBasicFS, NoiseFSAsset, NoiseFSAssetAuthoring> NoiseFSAO;

#else
typedef ApexAuthorableObject<ModuleBasicFS, JetFSAsset, JetFSAssetAuthoring> JetFSAO;
typedef ApexAuthorableObject<ModuleBasicFS, AttractorFSAsset, AttractorFSAssetAuthoring> AttractorFSAO;
typedef ApexAuthorableObject<ModuleBasicFS, VortexFSAsset, VortexFSAssetAuthoring> VortexFSAO;
typedef ApexAuthorableObject<ModuleBasicFS, NoiseFSAsset, NoiseFSAssetAuthoring> NoiseFSAO;
#endif

ModuleBasicFS::ModuleBasicFS(NiApexSDK* sdk)
{
	name = "BasicFS";
	mSdk = sdk;
	mApiProxy = this;
	mModuleParams = NULL;
	mFieldSamplerModule = NULL;

	/* Register asset type and create a namespace for its assets */
	const char* pName = JetFSAssetParams::staticClassName();					
	JetFSAO* eAO = PX_NEW(JetFSAO)(this, mAuthorableObjects, pName);
	JetFSAsset::mAssetTypeID = eAO->getResID();

	const char* pName2 = AttractorFSAssetParams::staticClassName();					
	AttractorFSAO* eAO2 = PX_NEW(AttractorFSAO)(this, mAuthorableObjects, pName2);
	AttractorFSAsset::mAssetTypeID = eAO2->getResID();

	const char* pName3 = NoiseFSAssetParams::staticClassName();					
	NoiseFSAO* eAO3 = PX_NEW(NoiseFSAO)(this, mAuthorableObjects, pName3);
	NoiseFSAsset::mAssetTypeID = eAO3->getResID();

	const char* pName4 = VortexFSAssetParams::staticClassName();					
	VortexFSAO* eAO4 = PX_NEW(VortexFSAO)(this, mAuthorableObjects, pName4);
	VortexFSAsset::mAssetTypeID = eAO4->getResID();

	/* Register the NxParameterized factories */
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
#	define PARAM_CLASS(clas) PARAM_CLASS_REGISTER_FACTORY(traits, clas)
#	include "BasicfsParamClasses.inc"
}

ModuleBasicFS::~ModuleBasicFS()
{
	releaseModuleProfiling();
}

void ModuleBasicFS::destroy()
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
#		include "BasicfsParamClasses.inc"
	}
}


void ModuleBasicFS::init(NxParameterized::Interface&)
{
}

NxParameterized::Interface* ModuleBasicFS::getDefaultModuleDesc()
{
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (!mModuleParams)
	{
		mModuleParams = DYNAMIC_CAST(BasicFSModuleParameters*)
		                (traits->createNxParameterized("BasicFSModuleParameters"));
		PX_ASSERT(mModuleParams);
	}
	else
	{
		mModuleParams->initDefaults();
	}

	return mModuleParams;
}

NxAuthObjTypeID ModuleBasicFS::getJetFSAssetTypeID() const
{
	return JetFSAsset::mAssetTypeID;
}

NxAuthObjTypeID ModuleBasicFS::getAttractorFSAssetTypeID() const
{
	return AttractorFSAsset::mAssetTypeID;
}

NxAuthObjTypeID ModuleBasicFS::getVortexFSAssetTypeID() const
{
	return VortexFSAsset::mAssetTypeID;
}

NxAuthObjTypeID ModuleBasicFS::getNoiseFSAssetTypeID() const
{
	return NoiseFSAsset::mAssetTypeID;
}

NxAuthObjTypeID ModuleBasicFS::getModuleID() const
{
	return JetFSAsset::mAssetTypeID; // What should return?
}

ApexActor* ModuleBasicFS::getApexActor(NxApexActor* nxactor, NxAuthObjTypeID type) const
{
	if (type == JetFSAsset::mAssetTypeID)
	{
		return static_cast<JetFSActor*>(nxactor);
	}
	else if (type == AttractorFSAsset::mAssetTypeID)
	{
		return static_cast<AttractorFSActor*>(nxactor);
	}
	else if (type == VortexFSAsset::mAssetTypeID)
	{
		return static_cast<VortexFSActor*>(nxactor);
	}
	else if (type == NoiseFSAsset::mAssetTypeID)
	{
		return static_cast<NoiseFSActor*>(nxactor);
	}

	return NULL;
}

/* == Example Scene methods == */
NiModuleScene* ModuleBasicFS::createNiModuleScene(NiApexScene& scene, NiApexRenderDebug* debugRender)
{
#if defined(APEX_CUDA_SUPPORT)
	if (scene.getTaskManager()->getGpuDispatcher())
	{
		return PX_NEW(BasicFSSceneGPU)(*this, scene, debugRender, mBasicFSScenes);
	}
	else
#endif
		return PX_NEW(BasicFSSceneCPU)(*this, scene, debugRender, mBasicFSScenes);
}

void ModuleBasicFS::releaseNiModuleScene(NiModuleScene& scene)
{
	BasicFSScene* es = DYNAMIC_CAST(BasicFSScene*)(&scene);
	es->destroy();
}

BasicFSScene* ModuleBasicFS::getBasicFSScene(const NxApexScene& apexScene)
{
	for (PxU32 i = 0 ; i < mBasicFSScenes.getSize() ; i++)
	{
		BasicFSScene* es = DYNAMIC_CAST(BasicFSScene*)(mBasicFSScenes.getResource(i));
		if (es->mApexScene == &apexScene)
		{
			return es;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate BasicFSScene");
	return NULL;
}

NxApexRenderableIterator* ModuleBasicFS::createRenderableIterator(const NxApexScene& apexScene)
{
	BasicFSScene* es = getBasicFSScene(apexScene);
	if (es)
	{
		return es->createRenderableIterator();
	}

	return NULL;
}

NiModuleFieldSampler* ModuleBasicFS::getNiModuleFieldSampler()
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

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
