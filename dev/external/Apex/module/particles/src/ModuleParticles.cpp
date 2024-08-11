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

#define SAFE_MODULE_RELEASE(x) if ( x ) { NiModule *m = mSdk->getNiModule(x); PX_ASSERT(m); m->setParent(NULL); x->release(); x = NULL; }

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

/* === ModuleParticles DLL Setup === */

#pragma warning(disable:4505)

#include "ModuleParticles.h"
#include "ModulePerfScope.h"
#include "ParticlesScene.h"
#include "NiApexScene.h"
#include "PxMaterial.h"
#include "NxModuleTurbulenceFS.h"
#include "PxMemoryBuffer.h"
#endif // #if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED 

#include "NiApexSDK.h"
#include "PsShare.h"
#include "NxApex.h"
#include "NxParamUtils.h"
#include "ApexEmitterAssetParameters.h"
#include "EmitterGeomSphereParams.h"
#include "IofxAssetParameters.h"
#include "SpriteIofxParameters.h"
#include "ViewDirectionSortingModifierParams.h"
#include "SimpleScaleModifierParams.h"
#include "InitialColorModifierParams.h"
#include "TurbulenceFSAssetParams.h"
#include "BasicIOSAssetParam.h"
#include "NxApexEmitterAsset.h"
#include "NxGroundEmitterAsset.h"
#include "NxImpactEmitterAsset.h"
#include "NxIofxAsset.h"
#include "NxBasicIosAsset.h"
#include "NxParticleIosAsset.h"
#include "NxTurbulenceFSAsset.h"
#include "EmitterGeomBoxParams.h"
#include "RandomScaleModifierParams.h"
#include "RandomRotationModifierParams.h"
#include "ColorVsLifeCompositeModifierParams.h"
#include "ScaleVsLife2DModifierParams.h"
#include "FloatMath.h"
#include "NxJetFSActor.h"
#include "NxAttractorFSActor.h"
#include "NxBasicFSAsset.h"
#include "JetFSAssetParams.h"
#include "NoiseFSAssetParams.h"
#include "VortexFSAssetParams.h"
#include "OrientScaleAlongScreenVelocityModifierParams.h"
#include "RandomRotationModifierParams.h"
#include "RotationRateModifierParams.h"
#include "RotationRateVsLifeModifierParams.h"
#include "EffectPackageActor.h"
#include "AttractorFSAssetParams.h"
#include "FluidParticleSystemParams.h"
#include "PxparticleiosParamClasses.h"
#include "MeshIofxParameters.h"
#include "SimpleScaleModifierParams.h"
#include "RotationModifierParams.h"
#include "ScaleVsLife3DModifierParams.h"
#include "ForceFieldAssetParams.h"
#include "NxHeatSourceAsset.h"
#include "NxSubstanceSourceAsset.h"
#include "NxForceFieldAsset.h"
#include "EffectPackageAssetParams.h"
#include "ViewDirectionSortingModifierParams.h"
#include "RandomSubtextureModifierParams.h"
#include "NxUserOpaqueMesh.h"
#include "HeatSourceAssetParams.h"
#include "HeatSourceGeomSphereParams.h"
#include "SubstanceSourceAssetParams.h"
#include "PxPhysics.h"
#include "EffectPackageData.h"

#define SAFE_DESTROY(x) if ( x ) { x->destroy(); x = NULL; }

#if defined(PX_X86)

#pragma comment(linker,"/include:_BasicFS_fieldSamplerGridKernel")
#pragma comment(linker,"/include:_BasicFS_fieldSamplerPointsKernel")

#pragma comment(linker,"/include:_BasicIOS_compactKernel")
#pragma comment(linker,"/include:_BasicIOS_histogram1Kernel")
#pragma comment(linker,"/include:_BasicIOS_histogram2Kernel")
#pragma comment(linker,"/include:_BasicIOS_mergeHistogramKernel")
#pragma comment(linker,"/include:_BasicIOS_reduce1Kernel")
#pragma comment(linker,"/include:_BasicIOS_reduce2Kernel")
#pragma comment(linker,"/include:_BasicIOS_scan1Kernel")
#pragma comment(linker,"/include:_BasicIOS_scan2Kernel")
#pragma comment(linker,"/include:_BasicIOS_scan3Kernel")
#pragma comment(linker,"/include:_BasicIOS_simulateApplyFieldKernel")
#pragma comment(linker,"/include:_BasicIOS_simulateKernel")
#pragma comment(linker,"/include:_BasicIOS_stateKernel")

#pragma comment(linker,"/include:_FieldSampler_applyParticlesKernel")
#pragma comment(linker,"/include:_FieldSampler_clearGridKernel")
#pragma comment(linker,"/include:_FieldSampler_clearKernel")
#pragma comment(linker,"/include:_FieldSampler_composeKernel")

#if NX_SDK_VERSION_MAJOR == 3
#pragma comment(linker,"/include:_ForceField_fieldSamplerGridKernel")
#pragma comment(linker,"/include:_ForceField_fieldSamplerPointsKernel")
#endif

#pragma comment(linker,"/include:_IOFX_actorRangeKernel")
#pragma comment(linker,"/include:_IOFX_bbox1Kernel")
#pragma comment(linker,"/include:_IOFX_bbox2Kernel")
#pragma comment(linker,"/include:_IOFX_meshModifiersKernel")
#pragma comment(linker,"/include:_IOFX_newRadixSortBlockKernel")
#pragma comment(linker,"/include:_IOFX_newRadixSortStep1Kernel")
#pragma comment(linker,"/include:_IOFX_newRadixSortStep2Kernel")
#pragma comment(linker,"/include:_IOFX_newRadixSortStep3Kernel")
#pragma comment(linker,"/include:_IOFX_radixSortStep1Kernel")
#pragma comment(linker,"/include:_IOFX_radixSortStep2Kernel")
#pragma comment(linker,"/include:_IOFX_radixSortStep3Kernel")
#pragma comment(linker,"/include:_IOFX_makeSortKeys")
#pragma comment(linker,"/include:_IOFX_remapKernel")
#pragma comment(linker,"/include:_IOFX_spriteModifiersKernel")
#pragma comment(linker,"/include:_IOFX_spriteTextureModifiersKernel")
#pragma comment(linker,"/include:_IOFX_volumeMigrationKernel")

#if NX_SDK_VERSION_MAJOR == 3
#pragma comment(linker,"/include:_ParticleIOS_compactKernel")
#pragma comment(linker,"/include:_ParticleIOS_histogram1Kernel")
#pragma comment(linker,"/include:_ParticleIOS_histogram2Kernel")
#pragma comment(linker,"/include:_ParticleIOS_mergeHistogramKernel")
#pragma comment(linker,"/include:_ParticleIOS_reduce1Kernel")
#pragma comment(linker,"/include:_ParticleIOS_reduce2Kernel")
#pragma comment(linker,"/include:_ParticleIOS_scan1Kernel")
#pragma comment(linker,"/include:_ParticleIOS_scan2Kernel")
#pragma comment(linker,"/include:_ParticleIOS_scan3Kernel")
#pragma comment(linker,"/include:_ParticleIOS_simulateApplyFieldKernel")
#pragma comment(linker,"/include:_ParticleIOS_simulateKernel")
#pragma comment(linker,"/include:_ParticleIOS_stateKernel")
#endif

#elif defined(PX_X64)

#pragma comment(linker,"/include:BasicFS_fieldSamplerGridKernel")
#pragma comment(linker,"/include:BasicFS_fieldSamplerPointsKernel")

#pragma comment(linker,"/include:BasicIOS_compactKernel")
#pragma comment(linker,"/include:BasicIOS_histogram1Kernel")
#pragma comment(linker,"/include:BasicIOS_histogram2Kernel")
#pragma comment(linker,"/include:BasicIOS_mergeHistogramKernel")
#pragma comment(linker,"/include:BasicIOS_reduce1Kernel")
#pragma comment(linker,"/include:BasicIOS_reduce2Kernel")
#pragma comment(linker,"/include:BasicIOS_scan1Kernel")
#pragma comment(linker,"/include:BasicIOS_scan2Kernel")
#pragma comment(linker,"/include:BasicIOS_scan3Kernel")
#pragma comment(linker,"/include:BasicIOS_simulateApplyFieldKernel")
#pragma comment(linker,"/include:BasicIOS_simulateKernel")
#pragma comment(linker,"/include:BasicIOS_stateKernel")

#pragma comment(linker,"/include:FieldSampler_applyParticlesKernel")
#pragma comment(linker,"/include:FieldSampler_clearGridKernel")
#pragma comment(linker,"/include:FieldSampler_clearKernel")
#pragma comment(linker,"/include:FieldSampler_composeKernel")


#pragma comment(linker,"/include:IOFX_actorRangeKernel")
#pragma comment(linker,"/include:IOFX_bbox1Kernel")
#pragma comment(linker,"/include:IOFX_bbox2Kernel")
#pragma comment(linker,"/include:IOFX_meshModifiersKernel")
#pragma comment(linker,"/include:IOFX_newRadixSortBlockKernel")
#pragma comment(linker,"/include:IOFX_newRadixSortStep1Kernel")
#pragma comment(linker,"/include:IOFX_newRadixSortStep2Kernel")
#pragma comment(linker,"/include:IOFX_newRadixSortStep3Kernel")
#pragma comment(linker,"/include:IOFX_radixSortStep1Kernel")
#pragma comment(linker,"/include:IOFX_radixSortStep2Kernel")
#pragma comment(linker,"/include:IOFX_radixSortStep3Kernel")
#pragma comment(linker,"/include:IOFX_makeSortKeys")
#pragma comment(linker,"/include:IOFX_remapKernel")
#pragma comment(linker,"/include:IOFX_spriteModifiersKernel")
#pragma comment(linker,"/include:IOFX_spriteTextureModifiersKernel")
#pragma comment(linker,"/include:IOFX_volumeMigrationKernel")

#if NX_SDK_VERSION_MAJOR == 3
#pragma comment(linker,"/include:ForceField_fieldSamplerGridKernel")
#pragma comment(linker,"/include:ForceField_fieldSamplerPointsKernel")
#pragma comment(linker,"/include:ParticleIOS_compactKernel")
#pragma comment(linker,"/include:ParticleIOS_histogram1Kernel")
#pragma comment(linker,"/include:ParticleIOS_histogram2Kernel")
#pragma comment(linker,"/include:ParticleIOS_mergeHistogramKernel")
#pragma comment(linker,"/include:ParticleIOS_reduce1Kernel")
#pragma comment(linker,"/include:ParticleIOS_reduce2Kernel")
#pragma comment(linker,"/include:ParticleIOS_scan1Kernel")
#pragma comment(linker,"/include:ParticleIOS_scan2Kernel")
#pragma comment(linker,"/include:ParticleIOS_scan3Kernel")
#pragma comment(linker,"/include:ParticleIOS_simulateApplyFieldKernel")
#pragma comment(linker,"/include:ParticleIOS_simulateKernel")
#pragma comment(linker,"/include:ParticleIOS_stateKernel")
#endif



#endif


namespace physx
{
namespace apex
{


void instantiateModuleBasicIos();
void instantiateModuleEmitter();
void instantiateModuleIofx();
void instantiateModuleFieldSampler();
void instantiateModuleFieldBoundary();
void instantiateModuleExplosion();
void instantiateModuleFluidIos();
void instantiateModuleParticleIos();
void instantiateModuleForceField();
void instantiateModuleBasicFS();

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

static PxTransform getPose(physx::PxF32 x, physx::PxF32 y, physx::PxF32 z, physx::PxF32 rotX, physx::PxF32 rotY, physx::PxF32 rotZ)
{
	PxTransform ret;
	ret.p = PxVec3(x, y, z);
	fm_eulerToQuat(rotX * FM_DEG_TO_RAD, rotY * FM_DEG_TO_RAD, rotZ * FM_DEG_TO_RAD, &ret.q.x);
	return ret;
}

static const char* getAuthoringTypeName(const char* className)
{
	const char* ret = NULL;

	if (strcmp(className, "BasicIOSAssetParam") == 0)
	{
		ret = NX_BASIC_IOS_AUTHORING_TYPE_NAME;
	}
	else if (strcmp(className, "IofxAssetParameters") == 0)
	{
		ret = NX_IOFX_AUTHORING_TYPE_NAME;
	}
	else if (strcmp(className, "ParticleIosAssetParam") == 0)
	{
		ret = NX_PARTICLE_IOS_AUTHORING_TYPE_NAME;
	}


	PX_ASSERT(ret);
	return ret;
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


	/* Setup common module global variables */
	gApexSdk = inSdk;
	APEX_INIT_FOUNDATION();
	::particles::initModuleProfiling(inSdk, "Particles");
	particles::ModuleParticles* impl = PX_NEW(particles::ModuleParticles)(inSdk);
	*niRef  = (NiModule*) impl;
	return (NxModule*) impl;
#else // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
	if (errorCode != NULL)
	{
		*errorCode = APEX_CE_WRONG_VERSION;
	}

	PX_UNUSED(PhysXsdkVersion);
	PX_UNUSED(APEXsdkVersion);
	PX_UNUSED(niRef);
	PX_UNUSED(inSdk);
	return NULL; // Force field Module can only compile against MIN_PHYSX_SDK_VERSION_REQUIRED and above
#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
}

#else
/* Statically linking entry function */
void instantiateModuleParticles()
{
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
	ParticlesModule::initModuleProfiling(sdk, "Particles");
	NiApexSDK* sdk = NiGetApexSDK();
	particles::ModuleParticles* impl = PX_NEW(particles::ModuleParticles)(sdk);
	sdk->registerExternalModule((NxModule*) impl, (NiModule*) impl);
#endif
}
#endif // `defined(_USRDLL)

namespace particles
{

/* === ModuleParticles Implementation === */
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED


#ifdef WITHOUT_APEX_AUTHORING

class ParticlesAssetDummyAuthoring : public NxApexAssetAuthoring, public UserAllocated
{
public:
	ParticlesAssetDummyAuthoring(ModuleParticles* module, NxResourceList& list, NxParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	ParticlesAssetDummyAuthoring(ModuleParticles* module, NxResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	ParticlesAssetDummyAuthoring(ModuleParticles* module, NxResourceList& list)
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
	virtual NxParameterized::Interface* getNxParameterized()
	{
		PX_ASSERT(0);
		return NULL;
	}

	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	}

	virtual void setToolString(const char* toolName, const char* toolVersion, PxU32 toolChangelist)
	{
		PX_ALWAYS_ASSERT();
		PX_UNUSED(toolName);
		PX_UNUSED(toolVersion);
		PX_UNUSED(toolChangelist);
	}
};

typedef ApexAuthorableObject<ModuleParticles, ParticlesAsset, ParticlesAssetDummyAuthoring> ParticlesAO;

#else
typedef ApexAuthorableObject<ModuleParticles, ParticlesAsset, ParticlesAssetAuthoring> ParticlesAO;
#endif


NxAuthObjTypeID EffectPackageAsset::mAssetTypeID;  // Static class member of ParticlesAsset

#ifdef WITHOUT_APEX_AUTHORING

class EffectPackageAssetDummyAuthoring : public NxApexAssetAuthoring, public UserAllocated
{
public:
	EffectPackageAssetDummyAuthoring(ModuleParticles* module, NxResourceList& list, NxParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	EffectPackageAssetDummyAuthoring(ModuleParticles* module, NxResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	EffectPackageAssetDummyAuthoring(ModuleParticles* module, NxResourceList& list)
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
		return EffectPackageAsset::getClassName();
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
	virtual NxParameterized::Interface* getNxParameterized()
	{
		PX_ASSERT(0);
		return NULL;
	}

	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	}

	virtual void setToolString(const char* toolName, const char* toolVersion, PxU32 toolChangelist)
	{
		PX_ALWAYS_ASSERT();
		PX_UNUSED(toolName);
		PX_UNUSED(toolVersion);
		PX_UNUSED(toolChangelist);
	}
};

typedef ApexAuthorableObject<ModuleParticles, EffectPackageAsset, EffectPackageAssetDummyAuthoring> EffectPackageAO;

#else
typedef ApexAuthorableObject<ModuleParticles, EffectPackageAsset, EffectPackageAssetAuthoring> EffectPackageAO;
#endif


//********************************************************************

#define MODULE_PARENT(x) if ( x ) { NiModule *m = mSdk->getNiModule(x); PX_ASSERT(m); m->setParent(this); m->setCreateOk(false); }

ModuleParticles::ModuleParticles(NiApexSDK* inSdk)
{
	mSdk = inSdk;

	instantiateModuleBasicIos();			// Instantiate the BasicIOS module statically
	mModuleBasicIos = mSdk->createModule("BasicIOS");
	PX_ASSERT(mModuleBasicIos);
	MODULE_PARENT(mModuleBasicIos);

	instantiateModuleEmitter();				// Instantiate the Emitter module statically
	mModuleEmitter = mSdk->createModule("Emitter");
	PX_ASSERT(mModuleEmitter);
	MODULE_PARENT(mModuleEmitter);

	instantiateModuleIofx();				// Instantiate the IOFX module statically
	mModuleIofx = mSdk->createModule("IOFX");
	PX_ASSERT(mModuleIofx);
	MODULE_PARENT(mModuleIofx);

	instantiateModuleFieldSampler();		// Instantiate the field sampler module statically
	mModuleFieldSampler = mSdk->createModule("FieldSampler");
	PX_ASSERT(mModuleFieldSampler);
	MODULE_PARENT(mModuleFieldSampler);

	instantiateModuleBasicFS();				// Instantiate the BasicFS module statically
	mModuleBasicFS = mSdk->createModule("BasicFS");
	PX_ASSERT(mModuleBasicFS);
	MODULE_PARENT(mModuleBasicFS);

#	if NX_SDK_VERSION_MAJOR == 2
	instantiateModuleFieldBoundary();		// PhysX 2.8 only : Instantiate the FieldBoundary module
	mModuleFieldBoundary = mSdk->createModule("FieldBoundary");
	PX_ASSERT(mModuleFieldBoundary);
	MODULE_PARENT(mModuleFieldBoundary);

	instantiateModuleExplosion();			// PhysX 2.8 only : Instantiate the explosion module
	mModuleExplosion = mSdk->createModule("Explosion");
	PX_ASSERT(mModuleExplosion);
	MODULE_PARENT(mModuleExplosion);

	instantiateModuleFluidIos();			// PhysX 2.8 only : Instantiate the fluidIOS module
	mModuleFluidIos = mSdk->createModule("NxFluidIOS");
	PX_ASSERT(mModuleFluidIos);
	MODULE_PARENT(mModuleFluidIos);


#	elif NX_SDK_VERSION_MAJOR == 3
	instantiateModuleParticleIos();			// PhysX 3.x only : Instantiate the ParticleIOS module
	mModuleParticleIos = mSdk->createModule("ParticleIOS");
	PX_ASSERT(mModuleParticleIos);
	MODULE_PARENT(mModuleParticleIos);

	instantiateModuleForceField();			// PhysX 3.x only : Instantiate the ForceField module
	mModuleForceField = mSdk->createModule("ForceField");
	PX_ASSERT(mModuleForceField);
	MODULE_PARENT(mModuleForceField);

#	endif


	name = "Particles";
	mApiProxy = this;
	mModuleParams = NULL;
	mTurbulenceModule = NULL;
	mGraphicsMaterialsDatabase = NULL;
	mEnableScreenCulling = false;
	mZnegative = false;
	mUseEmitterPool = false;

	mEffectPackageIOSDatabaseParams = NULL;
	mEffectPackageIOFXDatabaseParams = NULL;
	mEffectPackageEmitterDatabaseParams = NULL;
	mEffectPackageDatabaseParams = NULL;
	mEffectPackageFieldSamplerDatabaseParams = NULL;

	{
		/* Register asset type and create a namespace for it's assets */
		const char* pName = EffectPackageAssetParams::staticClassName();
		EffectPackageAO* eAO = PX_NEW(EffectPackageAO)(this, mEffectPackageAuthorableObjects, pName);
		EffectPackageAsset::mAssetTypeID = eAO->getResID();
	}

	/* Register the NxParameterized factories */
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();


	traits->registerFactory(mParticlesDebugRenderParamsFactory);
	traits->registerFactory(mParticlesModuleParametersFactory);

	traits->registerFactory(mEffectPackageDataFactory);
	traits->registerFactory(mAttractorFieldSamplerDataFactory);
	traits->registerFactory(mJetFieldSamplerDataFactory);
	traits->registerFactory(mNoiseFieldSamplerDataFactory);
	traits->registerFactory(mVortexFieldSamplerDataFactory);
	traits->registerFactory(mTurbulenceFieldSamplerDataFactory);
	traits->registerFactory(mHeatSourceDataFactory);
	traits->registerFactory(mSubstanceSourceDataFactory);
	traits->registerFactory(mForceFieldDataFactory);
	traits->registerFactory(mEmitterDataFactory);
	traits->registerFactory(mGraphicsEffectDataFactory);
	traits->registerFactory(mParticleSimulationDataFactory);

	traits->registerFactory(mEffectPackageAssetParamsFactory);
	traits->registerFactory(mEffectPackageActorParamsFactory);
	traits->registerFactory(mEmitterEffectFactory);
	traits->registerFactory(mHeatSourceEffectFactory);
	traits->registerFactory(mSubstanceSourceEffectFactory);
	traits->registerFactory(mForceFieldEffectFactory);
	traits->registerFactory(mJetFieldSamplerEffectFactory);
	traits->registerFactory(mNoiseFieldSamplerEffectFactory);
	traits->registerFactory(mVortexFieldSamplerEffectFactory);
	traits->registerFactory(mAttractorFieldSamplerEffectFactory);
	traits->registerFactory(mTurbulenceFieldSamplerEffectFactory);
	traits->registerFactory(mEffectPackageIOSDatabaseParamsFactory);
	traits->registerFactory(mEffectPackageIOFXDatabaseParamsFactory);
	traits->registerFactory(mEffectPackageEmitterDatabaseParamsFactory);
	traits->registerFactory(mEffectPackageDatabaseParamsFactory);
	traits->registerFactory(mEffectPackageFieldSamplerDatabaseParamsFactory);
	traits->registerFactory(mEffectPackageGraphicsMaterialsParamsFactory);
	traits->registerFactory(mGraphicsMaterialDataFactory);
	traits->registerFactory(mVolumeRenderMaterialDataFactory);

	registerLODParameter("Radius", NxRange<physx::PxU32>(1, 10));

	{
		PxU32 count = mSdk->getNbModules();
		NxModule** modules = mSdk->getModules();
		for (PxU32 i = 0; i < count; i++)
		{
			NxModule* m = modules[i];
			const char* name = m->getName();
			if (strcmp(name, "TurbulenceFS") == 0)
			{
				mTurbulenceModule = static_cast< NxModuleTurbulenceFS*>(m);
				break;
			}
		}
	}
}

ModuleParticles::~ModuleParticles()
{
}

void ModuleParticles::destroy()
{
	// release the NxParameterized factory
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (mModuleParams)
	{
		mModuleParams->destroy();
		mModuleParams = NULL;
	}

	SAFE_DESTROY(mEffectPackageIOSDatabaseParams);
	SAFE_DESTROY(mEffectPackageIOFXDatabaseParams);
	SAFE_DESTROY(mEffectPackageEmitterDatabaseParams);
	SAFE_DESTROY(mEffectPackageDatabaseParams);
	SAFE_DESTROY(mEffectPackageFieldSamplerDatabaseParams);
	SAFE_DESTROY(mGraphicsMaterialsDatabase);

	Module::destroy();

	if (traits)
	{
		traits->removeFactory(ParticlesDebugRenderParams::staticClassName());
		traits->removeFactory(ParticlesModuleParameters::staticClassName());
		traits->removeFactory(EffectPackageGraphicsMaterialsParams::staticClassName());
		traits->removeFactory(GraphicsMaterialData::staticClassName());
		traits->removeFactory(VolumeRenderMaterialData::staticClassName());

		traits->removeFactory(EffectPackageData::staticClassName());
		traits->removeFactory(AttractorFieldSamplerData::staticClassName());
		traits->removeFactory(JetFieldSamplerData::staticClassName());
		traits->removeFactory(NoiseFieldSamplerData::staticClassName());
		traits->removeFactory(VortexFieldSamplerData::staticClassName());
		traits->removeFactory(TurbulenceFieldSamplerData::staticClassName());
		traits->removeFactory(HeatSourceData::staticClassName());
		traits->removeFactory(SubstanceSourceData::staticClassName());
		traits->removeFactory(ForceFieldData::staticClassName());
		traits->removeFactory(EmitterData::staticClassName());
		traits->removeFactory(GraphicsEffectData::staticClassName());
		traits->removeFactory(ParticleSimulationData::staticClassName());

		traits->removeFactory(EffectPackageAssetParams::staticClassName());
		traits->removeFactory(EffectPackageActorParams::staticClassName());
		traits->removeFactory(EmitterEffect::staticClassName());
		traits->removeFactory(HeatSourceEffect::staticClassName());
		traits->removeFactory(SubstanceSourceEffect::staticClassName());
		traits->removeFactory(ForceFieldEffect::staticClassName());
		traits->removeFactory(JetFieldSamplerEffect::staticClassName());
		traits->removeFactory(NoiseFieldSamplerEffect::staticClassName());
		traits->removeFactory(VortexFieldSamplerEffect::staticClassName());
		traits->removeFactory(AttractorFieldSamplerEffect::staticClassName());
		traits->removeFactory(TurbulenceFieldSamplerEffect::staticClassName());

		traits->removeFactory(EffectPackageIOSDatabaseParams::staticClassName());
		traits->removeFactory(EffectPackageIOFXDatabaseParams::staticClassName());
		traits->removeFactory(EffectPackageEmitterDatabaseParams::staticClassName());
		traits->removeFactory(EffectPackageDatabaseParams::staticClassName());
		traits->removeFactory(EffectPackageFieldSamplerDatabaseParams::staticClassName());

		ParticlesDebugRenderParams::freeParameterDefinitionTable(traits);
		ParticlesModuleParameters::freeParameterDefinitionTable(traits);
		EffectPackageGraphicsMaterialsParams::freeParameterDefinitionTable(traits);
		GraphicsMaterialData::freeParameterDefinitionTable(traits);
		VolumeRenderMaterialData::freeParameterDefinitionTable(traits);

		EffectPackageData::freeParameterDefinitionTable(traits);
		AttractorFieldSamplerData::freeParameterDefinitionTable(traits);
		JetFieldSamplerData::freeParameterDefinitionTable(traits);
		NoiseFieldSamplerData::freeParameterDefinitionTable(traits);
		VortexFieldSamplerData::freeParameterDefinitionTable(traits);
		TurbulenceFieldSamplerData::freeParameterDefinitionTable(traits);
		HeatSourceData::freeParameterDefinitionTable(traits);
		SubstanceSourceData::freeParameterDefinitionTable(traits);
		ForceFieldData::freeParameterDefinitionTable(traits);
		EmitterData::freeParameterDefinitionTable(traits);
		GraphicsEffectData::freeParameterDefinitionTable(traits);
		ParticleSimulationData::freeParameterDefinitionTable(traits);

		EffectPackageAssetParams::freeParameterDefinitionTable(traits);
		EffectPackageActorParams::freeParameterDefinitionTable(traits);
		EmitterEffect::freeParameterDefinitionTable(traits);
		HeatSourceEffect::freeParameterDefinitionTable(traits);
		SubstanceSourceEffect::freeParameterDefinitionTable(traits);
		ForceFieldEffect::freeParameterDefinitionTable(traits);
		JetFieldSamplerEffect::freeParameterDefinitionTable(traits);
		NoiseFieldSamplerEffect::freeParameterDefinitionTable(traits);
		VortexFieldSamplerEffect::freeParameterDefinitionTable(traits);
		AttractorFieldSamplerEffect::freeParameterDefinitionTable(traits);
		TurbulenceFieldSamplerEffect::freeParameterDefinitionTable(traits);

		EffectPackageIOSDatabaseParams::freeParameterDefinitionTable(traits);
		EffectPackageIOFXDatabaseParams::freeParameterDefinitionTable(traits);
		EffectPackageEmitterDatabaseParams::freeParameterDefinitionTable(traits);
		EffectPackageDatabaseParams::freeParameterDefinitionTable(traits);
		EffectPackageFieldSamplerDatabaseParams::freeParameterDefinitionTable(traits);
	}


	SAFE_MODULE_RELEASE(mModuleBasicIos);
	SAFE_MODULE_RELEASE(mModuleEmitter);
	SAFE_MODULE_RELEASE(mModuleIofx);
	SAFE_MODULE_RELEASE(mModuleFieldSampler);
	SAFE_MODULE_RELEASE(mModuleBasicFS);
#	if NX_SDK_VERSION_MAJOR == 2
	SAFE_MODULE_RELEASE(mModuleFieldBoundary);
	SAFE_MODULE_RELEASE(mModuleExplosion);			// PhysX 2.8 only : Instantiate the explosion module
	SAFE_MODULE_RELEASE(mModuleFluidIos);			// PhysX 2.8 only : Instantiate the fluidIOS module
#	elif NX_SDK_VERSION_MAJOR == 3
	SAFE_MODULE_RELEASE(mModuleParticleIos);			// PhysX 3.x only : Instantiate the ParticleIOS module
	SAFE_MODULE_RELEASE(mModuleForceField);			// PhysX 3.x only : Instantiate the ForceField module
#	endif

	// clear before deletion, so that we don't call back into releaseNiModuleScene during "delete this"
	mParticlesScenes.clear();

	delete this;


}

NxParameterized::Interface* ModuleParticles::getDefaultModuleDesc()
{
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (!mModuleParams)
	{
		mModuleParams = DYNAMIC_CAST(ParticlesModuleParameters*)
		                (traits->createNxParameterized("ParticlesModuleParameters"));
		PX_ASSERT(mModuleParams);
	}
	else
	{
		mModuleParams->initDefaults();
	}

	return mModuleParams;
}

void ModuleParticles::init(const NxModuleParticlesDesc& expDesc)
{
	mModuleValue = expDesc.moduleValue;
}

NxAuthObjTypeID ModuleParticles::getParticlesAssetTypeID() const
{
// TODO	return ParticlesAsset::mAssetTypeID;
	return 0;
}
NxAuthObjTypeID ModuleParticles::getModuleID() const
{
//	return ParticlesAsset::mAssetTypeID;
// TODO
	return 0;
}


/* == Particles Scene methods == */
NiModuleScene* ModuleParticles::createNiModuleScene(NiApexScene& scene, NiApexRenderDebug* renderDebug)
{
	NiModuleScene* ret = PX_NEW(ParticlesScene)(*this, scene, renderDebug, mParticlesScenes);
	mScenes.pushBack(ret);

	return ret;
}

void ModuleParticles::releaseNiModuleScene(NiModuleScene& scene)
{
	for (PxU32 i = 0; i < mScenes.size(); ++i)
	{
		if (mScenes[i] == &scene)
		{
			mScenes.remove(i);
			break;
		}
	}
	ParticlesScene* es = DYNAMIC_CAST(ParticlesScene*)(&scene);
	es->destroy();

}

physx::PxU32 ModuleParticles::forceLoadAssets()
{
	physx::PxU32 loadedAssetCount = 0;

	for (physx::PxU32 i = 0; i < mAuthorableObjects.getSize(); i++)
	{
		NiApexAuthorableObject* ao = static_cast<NiApexAuthorableObject*>(mAuthorableObjects.getResource(i));
		loadedAssetCount += ao->forceLoadAssets();
	}

	for (physx::PxU32 i = 0; i < mEffectPackageAuthorableObjects.getSize(); i++)
	{
		NiApexAuthorableObject* ao = static_cast<NiApexAuthorableObject*>(mEffectPackageAuthorableObjects.getResource(i));
		loadedAssetCount += ao->forceLoadAssets();
	}


	return loadedAssetCount;
}

ParticlesScene* ModuleParticles::getParticlesScene(const NxApexScene& apexScene)
{
	for (physx::PxU32 i = 0 ; i < mParticlesScenes.getSize() ; i++)
	{
		ParticlesScene* es = DYNAMIC_CAST(ParticlesScene*)(mParticlesScenes.getResource(i));
		if (es->mApexScene == &apexScene)
		{
			return es;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate ParticlesScene");
	return NULL;
}

NxApexRenderableIterator* ModuleParticles::createRenderableIterator(const NxApexScene& apexScene)
{
	ParticlesScene* es = getParticlesScene(apexScene);
	if (es)
	{
		return es->createRenderableIterator();
	}

	return NULL;
}

const NxParameterized::Interface* ModuleParticles::locateVolumeRenderMaterialData(const char* name) const
{
	const NxParameterized::Interface* ret = NULL;

	if (mGraphicsMaterialsDatabase)
	{
		EffectPackageGraphicsMaterialsParams* d = static_cast< EffectPackageGraphicsMaterialsParams*>(mGraphicsMaterialsDatabase);
		for (PxI32 i = 0; i < d->GraphicsMaterials.arraySizes[0]; i++)
		{
			NxParameterized::Interface *ei = d->GraphicsMaterials.buf[i];
			if ( ei && strcmp(ei->className(),"VolumeRenderMaterialData") == 0 )
			{
				VolumeRenderMaterialData* e = static_cast< VolumeRenderMaterialData*>(ei);
				if (e)
				{
					if (e->Name && physx::string::stricmp(e->Name, name) == 0)
					{
						ret = e;
						break;
					}
				}
			}
		}
	}
	return ret;
}



const NxParameterized::Interface* ModuleParticles::locateGraphicsMaterialData(const char* name) const
{
	const NxParameterized::Interface* ret = NULL;

	if (mGraphicsMaterialsDatabase)
	{
		EffectPackageGraphicsMaterialsParams* d = static_cast< EffectPackageGraphicsMaterialsParams*>(mGraphicsMaterialsDatabase);
		for (PxI32 i = 0; i < d->GraphicsMaterials.arraySizes[0]; i++)
		{
			NxParameterized::Interface *ei = d->GraphicsMaterials.buf[i];
			if ( ei && strcmp(ei->className(),"GraphicsMaterialData") == 0 || strcmp(ei->className(),"VolumeRenderMaterialData") == 0 )
			{
				GraphicsMaterialData* e = static_cast< GraphicsMaterialData*>(ei);
				if (e)
				{
					if (e->Name && physx::string::stricmp(e->Name, name) == 0)
					{
						ret = e;
						break;
					}
				}
			}
		}
	}
	return ret;
}

static bool isUnique(Array< const char* > &nameList, const char* baseName)
{
	bool ret = true;
	for (physx::PxU32 i = 0; i < nameList.size(); i++)
	{
		if (physx::string::stricmp(baseName, nameList[i]) == 0)
		{
			ret = false;
			break;
		}
	}
	return ret;
}

static const char* getUniqueName(Array< const char* > &nameList, const char* baseName)
{
	const char* ret = baseName;

	if (baseName == NULL || strlen(baseName) == 0)
	{
		baseName = "default";
	}
	if (!isUnique(nameList, baseName))
	{
		static char uniqueName[512];
		strncpy(uniqueName, baseName, 512);
		for (physx::PxU32 i = 1; i < 1000; i++)
		{
			sprintf_s(uniqueName, 512, "%s%d", baseName, i);
			if (isUnique(nameList, uniqueName))
			{
				ret = uniqueName;
				break;
			}
		}
	}
	return ret;
}


bool ModuleParticles::setEffectPackageGraphicsMaterialsDatabase(const NxParameterized::Interface* dataBase)
{
	bool ret = false;
	if (dataBase)
	{
		NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
		if (strcmp(dataBase->className(), EffectPackageGraphicsMaterialsParams::staticClassName()) == 0 && dataBase != mGraphicsMaterialsDatabase)
		{
			if (mGraphicsMaterialsDatabase)
			{
				mGraphicsMaterialsDatabase->destroy();
				mGraphicsMaterialsDatabase = NULL;
			}
			dataBase->clone(mGraphicsMaterialsDatabase);
			ret = true;
		}
		else
		{
			// add it to the end of the existing array..
			if (strcmp(dataBase->className(), GraphicsMaterialData::staticClassName()) == 0 || strcmp(dataBase->className(), VolumeRenderMaterialData::staticClassName()) == 0 )
			{
				bool revised = false;
				const char* itemName = dataBase->name();
				EffectPackageGraphicsMaterialsParams* d = static_cast< EffectPackageGraphicsMaterialsParams*>(mGraphicsMaterialsDatabase);
				for (PxU32 i = 0; i < (PxU32)d->GraphicsMaterials.arraySizes[0]; i++)
				{
					NxParameterized::Interface* ei = d->GraphicsMaterials.buf[i];
					if (!ei)
					{
						continue;
					}
					const char *materialName=NULL;
					if ( strcmp(ei->className(),"GraphicsMaterialData") == 0 )
					{
						GraphicsMaterialData* ed = static_cast< GraphicsMaterialData*>(ei);
						materialName = ed->Name.buf;
					}
					else if ( strcmp(ei->className(),"VolumeRenderMaterialData") == 0 )
					{
						VolumeRenderMaterialData* ed = static_cast< VolumeRenderMaterialData*>(ei);
						materialName = ed->Name.buf;
					}
					else
					{
						PX_ALWAYS_ASSERT();
					}
					if ( materialName && physx::string::stricmp(materialName, itemName) == 0)
					{
						ei->copy(*dataBase);
						revised = true;
						ret = true;
						break;
					}
				}
				if (!revised)
				{
					PxU32 arraySize = d->GraphicsMaterials.arraySizes[0];
					NxParameterized::Handle handle(mGraphicsMaterialsDatabase);
					NxParameterized::ErrorType err = handle.getParameter("GraphicsMaterials");
					PX_ASSERT(err == NxParameterized::ERROR_NONE);
					err = handle.resizeArray(arraySize + 1);
					PX_ASSERT(err == NxParameterized::ERROR_NONE);
					if (err == NxParameterized::ERROR_NONE)
					{
						NxParameterized::Interface* ei = NULL;
						NxParameterized::ErrorType err = dataBase->clone(ei);
						if ( err != NxParameterized::ERROR_NONE )
						{
							PX_ALWAYS_ASSERT();
						}
						else
						{
							NxParameterized::Handle item(ei);
							err = item.getParameter("Name");
							PX_ASSERT(err == NxParameterized::ERROR_NONE);
							item.setParamString(itemName);
							ei->setName(itemName);
							d->GraphicsMaterials.buf[arraySize] = ei;
						}
						ret = true;
					}
				}
			}
		}

		if (mGraphicsMaterialsDatabase)
		{
			EffectPackageGraphicsMaterialsParams* ds = static_cast< EffectPackageGraphicsMaterialsParams*>(mGraphicsMaterialsDatabase);
			if (ds->GraphicsMaterials.arraySizes[0] == 0)
			{
				NxParameterized::Handle handle(mGraphicsMaterialsDatabase);
				NxParameterized::ErrorType err = handle.getParameter("GraphicsMaterials");
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				err = handle.resizeArray(1);
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				if (err == NxParameterized::ERROR_NONE)
				{
					NxParameterized::Interface* ei = traits->createNxParameterized(GraphicsMaterialData::staticClassName());
					ds->GraphicsMaterials.buf[0] = ei;
				}
			}

			Array< const char* > nameList;
			for (PxI32 i = 0; i < ds->GraphicsMaterials.arraySizes[0]; i++)
			{
				NxParameterized::Interface *ei = ds->GraphicsMaterials.buf[i];
				const char *materialName=NULL;
				if ( strcmp(ei->className(),"GraphicsMaterialData") == 0 )
				{
					GraphicsMaterialData* e = static_cast< GraphicsMaterialData*>(ei);
					if (e)
					{
						materialName = e->Name;
					}
				}
				else if ( strcmp(ei->className(),"VolumeRenderMaterialData") == 0 )
				{
					VolumeRenderMaterialData* e = static_cast< VolumeRenderMaterialData*>(ei);
					if (e)
					{
						materialName = e->Name.buf;
					}
				}
				else
				{
					PX_ALWAYS_ASSERT();
				}
				if ( materialName )
				{
					for (PxU32 j = 0; j < nameList.size(); j++)
					{
						if (physx::string::stricmp(nameList[j], materialName) == 0)
						{
							NxParameterized::Handle handle(ei);
							NxParameterized::ErrorType err = handle.getParameter("Name");
							PX_ASSERT(err == NxParameterized::ERROR_NONE);
							if (err == NxParameterized::ERROR_NONE)
							{
								materialName = getUniqueName(nameList, materialName);
								handle.setParamString(materialName);
								ret = true;
								break;
							}
						}
					}
					nameList.pushBack(materialName);
				}
			}
		}
	}
	return ret;
}


const NxParameterized::Interface* ModuleParticles::getEffectPackageGraphicsMaterialsDatabase(void) const
{
	return mGraphicsMaterialsDatabase;
}


NxParameterized::Interface* ModuleParticles::locateResource(const char* resourceName, const char* nameSpace)
{
	NxParameterized::Interface* ret = NULL;

	if (mEffectPackageDatabaseParams == NULL)
	{
		return NULL;
	}

	if (strcmp(nameSpace, NX_PARTICLES_EFFECT_PACKAGE_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageDatabaseParams* d = static_cast< EffectPackageDatabaseParams*>(mEffectPackageDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->EffectPackages.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->EffectPackages.buf[i];
			if (!ei)
			{
				continue;
			}
			EffectPackageData* ed = static_cast< EffectPackageData*>(ei);
			if (physx::string::stricmp(ed->Name, resourceName) == 0)
			{
				ret = ed->EffectPackage;
				break;
			}
		}
	}
	else if (strcmp(nameSpace, APEX_MATERIALS_NAME_SPACE) == 0 || strcmp(nameSpace,"GraphicsMaterialData") == 0 )
	{
		EffectPackageGraphicsMaterialsParams* d = static_cast< EffectPackageGraphicsMaterialsParams*>(mGraphicsMaterialsDatabase);
		for (PxU32 i = 0; i < (PxU32)d->GraphicsMaterials.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->GraphicsMaterials.buf[i];
			if (!ei)
			{
				continue;
			}
			if ( strcmp(ei->className(),"GraphicsMaterialData") == 0 )
			{
				GraphicsMaterialData* ed = static_cast< GraphicsMaterialData*>(ei);
				if ( strcmp(ed->Name,resourceName) == 0 )
				{
					ret = ei;
					break;
				}
			}

		}
	}
	else if (strcmp(nameSpace, APEX_VOLUME_RENDER_MATERIALS_NAME_SPACE) == 0 || strcmp(nameSpace,"VolumeRenderMaterialData") == 0 )
	{
		EffectPackageGraphicsMaterialsParams* d = static_cast< EffectPackageGraphicsMaterialsParams*>(mGraphicsMaterialsDatabase);
		for (PxU32 i = 0; i < (PxU32)d->GraphicsMaterials.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->GraphicsMaterials.buf[i];
			if (!ei)
			{
				continue;
			}
			if ( strcmp(ei->className(),"VolumeRenderMaterialData") == 0 )
			{
				VolumeRenderMaterialData* ed = static_cast< VolumeRenderMaterialData*>(ei);
				if ( strcmp(ed->Name,resourceName) == 0 )
				{
					ret = ei;
					break;
				}
			}
		}
	}
	else if (strcmp(nameSpace, NX_IOFX_AUTHORING_TYPE_NAME) == 0 || strcmp(nameSpace, "GraphicsEffectData") == 0 )
	{
		EffectPackageIOFXDatabaseParams* d = static_cast< EffectPackageIOFXDatabaseParams*>(mEffectPackageIOFXDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->GraphicsEffects.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->GraphicsEffects.buf[i];
			if (!ei)
			{
				continue;
			}
			GraphicsEffectData* ed = static_cast< GraphicsEffectData*>(ei);
			if (physx::string::stricmp(resourceName, ed->Name) == 0)
			{
				if (strcmp(nameSpace, NX_IOFX_AUTHORING_TYPE_NAME) == 0 )
				{
					ret = ed->IOFX;
				}
				else
				{
					ret = ed;
				}
				break;
			}
		}
	}
	else if (strcmp(nameSpace, "ParticleSimulationData") == 0)
	{
		EffectPackageIOSDatabaseParams* d = static_cast< EffectPackageIOSDatabaseParams*>(mEffectPackageIOSDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->ParticleSimulations.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->ParticleSimulations.buf[i];
			if (!ei)
			{
				continue;
			}
			ParticleSimulationData* ed = static_cast< ParticleSimulationData*>(ei);
			if (physx::string::stricmp(ed->Name, resourceName) == 0)
			{
				ret = ei;
				break;
			}
		}
	}
	else if (strcmp(nameSpace, NX_BASIC_IOS_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageIOSDatabaseParams* d = static_cast< EffectPackageIOSDatabaseParams*>(mEffectPackageIOSDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->ParticleSimulations.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->ParticleSimulations.buf[i];
			if (!ei)
			{
				continue;
			}
			ParticleSimulationData* ed = static_cast< ParticleSimulationData*>(ei);
			if (strcmp(ed->IOS->className(), apex::basicios::BasicIOSAssetParam::staticClassName()) == 0)
			{
				if (physx::string::stricmp(ed->Name, resourceName) == 0)
				{
					ret = ed->IOS;
					break;
				}
			}
		}
	}
	else if (strcmp(nameSpace, NX_PARTICLE_IOS_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageIOSDatabaseParams* d = static_cast< EffectPackageIOSDatabaseParams*>(mEffectPackageIOSDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->ParticleSimulations.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->ParticleSimulations.buf[i];
			if (!ei)
			{
				continue;
			}
			ParticleSimulationData* ed = static_cast< ParticleSimulationData*>(ei);
			if (strcmp(ed->IOS->className(), apex::pxparticleios::ParticleIosAssetParam::staticClassName()) == 0)
			{
				if (physx::string::stricmp(ed->Name, resourceName) == 0)
				{
					ret = ed->IOS;
					break;
				}
			}
		}
	}
	else if (strcmp(nameSpace, NX_APEX_EMITTER_AUTHORING_TYPE_NAME) == 0 || strcmp(nameSpace,"EmitterData") == 0 )
	{
		EffectPackageEmitterDatabaseParams* d = static_cast< EffectPackageEmitterDatabaseParams*>(mEffectPackageEmitterDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->Emitters.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->Emitters.buf[i];
			if (!ei)
			{
				continue;
			}
			EmitterData* ed = static_cast< EmitterData*>(ei);
			if (physx::string::stricmp(resourceName, ed->Name) == 0)
			{
				if ( strcmp(nameSpace, NX_APEX_EMITTER_AUTHORING_TYPE_NAME) == 0 )
				{
					ret = ed->Emitter;
				}
				else
				{
					ret = ed;
				}
				break;
			}
		}
	}
	else if (strcmp(nameSpace, NX_HEAT_SOURCE_AUTHORING_TYPE_NAME) == 0 || strcmp(nameSpace, "HeatSourceData") == 0 )
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), HeatSourceData::staticClassName()) != 0)
			{
				continue;
			}
			HeatSourceData* ed = static_cast< HeatSourceData*>(ei);
			if (physx::string::stricmp(ed->Name, resourceName) == 0)
			{
				if (strcmp(nameSpace, NX_HEAT_SOURCE_AUTHORING_TYPE_NAME) == 0 )
				{
					ret = ed->HeatSource;
				}
				else
				{
					ret = ed;
				}
				break;
			}
		}
	}
	else if (strcmp(nameSpace, NX_SUBSTANCE_SOURCE_AUTHORING_TYPE_NAME) == 0 || strcmp(nameSpace, "SubstanceSourceData") == 0 )
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), SubstanceSourceData::staticClassName()) != 0)
			{
				continue;
			}
			SubstanceSourceData* ed = static_cast< SubstanceSourceData*>(ei);
			if (physx::string::stricmp(ed->Name, resourceName) == 0)
			{
				if (strcmp(nameSpace, NX_SUBSTANCE_SOURCE_AUTHORING_TYPE_NAME) == 0 )
				{
					ret = ed->SubstanceSource;
				}
				else
				{
					ret = ed;
				}
				break;
			}
		}
	}
	else if (strcmp(nameSpace, NX_JET_FS_AUTHORING_TYPE_NAME) == 0 || strcmp(nameSpace, "JetFieldSamplerData") == 0 )
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), JetFieldSamplerData::staticClassName()) != 0)
			{
				continue;
			}
			JetFieldSamplerData* ed = static_cast< JetFieldSamplerData*>(ei);
			if (physx::string::stricmp(ed->Name, resourceName) == 0)
			{
				if (strcmp(nameSpace, NX_JET_FS_AUTHORING_TYPE_NAME) == 0 )
				{
					ret = ed->JetFieldSampler;
				}
				else
				{
					ret = ed;
				}
				break;
			}
		}
	}
	else if (strcmp(nameSpace, NX_VORTEX_FS_AUTHORING_TYPE_NAME) == 0 || strcmp(nameSpace, "VortexFieldSamplerData") == 0 )
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), VortexFieldSamplerData::staticClassName()) != 0)
			{
				continue;
			}
			VortexFieldSamplerData* ed = static_cast< VortexFieldSamplerData*>(ei);
			if (physx::string::stricmp(ed->Name, resourceName) == 0)
			{
				if (strcmp(nameSpace, NX_VORTEX_FS_AUTHORING_TYPE_NAME) == 0 )
				{
					ret = ed->VortexFieldSampler;
				}
				else
				{
					ret = ed;
				}
				break;
			}
		}
	}
	else if (strcmp(nameSpace, NX_NOISE_FS_AUTHORING_TYPE_NAME) == 0 || strcmp(nameSpace, "NoiseFieldSamplerData") == 0 )
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), NoiseFieldSamplerData::staticClassName()) != 0)
			{
				continue;
			}
			NoiseFieldSamplerData* ed = static_cast< NoiseFieldSamplerData*>(ei);
			if (physx::string::stricmp(ed->Name, resourceName) == 0)
			{
				if (strcmp(nameSpace, NX_NOISE_FS_AUTHORING_TYPE_NAME) == 0 )
				{
					ret = ed->NoiseFieldSampler;
				}
				else
				{
					ret = ed;
				}
				break;
			}
		}
	}
	else if (strcmp(nameSpace, NX_ATTRACTOR_FS_AUTHORING_TYPE_NAME) == 0 || strcmp(nameSpace, "AttractorFieldSamplerData") == 0 )
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), AttractorFieldSamplerData::staticClassName()) != 0)
			{
				continue;
			}
			AttractorFieldSamplerData* ed = static_cast< AttractorFieldSamplerData*>(ei);
			if (physx::string::stricmp(ed->Name, resourceName) == 0)
			{
				if (strcmp(nameSpace, NX_ATTRACTOR_FS_AUTHORING_TYPE_NAME) == 0 )
				{
					ret = ed->AttractorFieldSampler;
				}
				else
				{
					ret = ed;
				}
				break;
			}
		}
	}
	else if (strcmp(nameSpace, NX_TURBULENCE_FS_AUTHORING_TYPE_NAME) == 0 || strcmp(nameSpace, "TurbulenceFieldSamplerData") == 0)
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), TurbulenceFieldSamplerData::staticClassName()) != 0)
			{
				continue;
			}
			TurbulenceFieldSamplerData* ed = static_cast< TurbulenceFieldSamplerData*>(ei);
			if (physx::string::stricmp(ed->Name, resourceName) == 0)
			{
				if (strcmp(nameSpace, NX_TURBULENCE_FS_AUTHORING_TYPE_NAME) == 0 )
				{
					ret = ed->TurbulenceFieldSampler;
				}
				else
				{
					ret = ed;
				}
				break;
			}
		}
	}
	else if (strcmp(nameSpace, NX_FORCEFIELD_AUTHORING_TYPE_NAME) == 0 || strcmp(nameSpace, "ForceFieldData") == 0)
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), ForceFieldData::staticClassName()) != 0)
			{
				continue;
			}
			ForceFieldData* ed = static_cast< ForceFieldData*>(ei);
			if (physx::string::stricmp(ed->Name, resourceName) == 0)
			{
				if (strcmp(nameSpace, NX_FORCEFIELD_AUTHORING_TYPE_NAME) == 0 )
				{
					ret = ed->ForceField;
				}
				else
				{
					ret = ed;
				}
				break;
			}
		}
	}
	else
	{
// just for debugging		PX_ALWAYS_ASSERT();
	}

	return ret;
}




const char** ModuleParticles::getResourceNames(const char* nameSpace, physx::PxU32& nameCount, const char** &variants)
{
	const char** ret = NULL;

	variants = NULL;
	mTempNames.clear();
	mTempVariantNames.clear();

	if (strcmp(nameSpace, NX_IOFX_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageIOFXDatabaseParams* d = static_cast< EffectPackageIOFXDatabaseParams*>(mEffectPackageIOFXDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->GraphicsEffects.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->GraphicsEffects.buf[i];
			if (!ei)
			{
				continue;
			}
			GraphicsEffectData* ed = static_cast< GraphicsEffectData*>(ei);
			mTempNames.pushBack(ed->Name);
			mTempVariantNames.pushBack(NX_IOFX_AUTHORING_TYPE_NAME);
		}
	}
	else if (strcmp(nameSpace, APEX_MATERIALS_NAME_SPACE) == 0)
	{
		EffectPackageGraphicsMaterialsParams* d = static_cast< EffectPackageGraphicsMaterialsParams*>(mGraphicsMaterialsDatabase);
		for (PxU32 i = 0; i < (PxU32)d->GraphicsMaterials.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->GraphicsMaterials.buf[i];
			if (!ei)
			{
				continue;
			}
			if ( strcmp(ei->className(),"GraphicsMaterialData") == 0 )
			{
				GraphicsMaterialData* ed = static_cast< GraphicsMaterialData*>(ei);
				mTempNames.pushBack(ed->Name);
				mTempVariantNames.pushBack(APEX_MATERIALS_NAME_SPACE);
			}

		}
	}
	else if (strcmp(nameSpace, APEX_VOLUME_RENDER_MATERIALS_NAME_SPACE) == 0)
	{
		EffectPackageGraphicsMaterialsParams* d = static_cast< EffectPackageGraphicsMaterialsParams*>(mGraphicsMaterialsDatabase);
		for (PxU32 i = 0; i < (PxU32)d->GraphicsMaterials.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->GraphicsMaterials.buf[i];
			if (!ei)
			{
				continue;
			}
			if ( strcmp(ei->className(),"VolumeRenderMaterialData") == 0 )
			{
				VolumeRenderMaterialData* ed = static_cast< VolumeRenderMaterialData*>(ei);
				mTempNames.pushBack(ed->Name);
				mTempVariantNames.pushBack(APEX_VOLUME_RENDER_MATERIALS_NAME_SPACE);
			}
		}
	}
	else if (strcmp(nameSpace, NX_BASIC_IOS_AUTHORING_TYPE_NAME) == 0 ||  strcmp(nameSpace, NX_PARTICLE_IOS_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageIOSDatabaseParams* d = static_cast< EffectPackageIOSDatabaseParams*>(mEffectPackageIOSDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->ParticleSimulations.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->ParticleSimulations.buf[i];
			if (!ei)
			{
				continue;
			}
			ParticleSimulationData* ed = static_cast< ParticleSimulationData*>(ei);
			mTempNames.pushBack(ed->Name);
			if (strcmp(ed->IOS->className(), apex::basicios::BasicIOSAssetParam::staticClassName()) == 0)
			{
				mTempVariantNames.pushBack(NX_BASIC_IOS_AUTHORING_TYPE_NAME);
			}
			else
			{
				mTempVariantNames.pushBack(NX_PARTICLE_IOS_AUTHORING_TYPE_NAME);
			}
		}
	}
	else if (strcmp(nameSpace, NX_APEX_EMITTER_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageEmitterDatabaseParams* d = static_cast< EffectPackageEmitterDatabaseParams*>(mEffectPackageEmitterDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->Emitters.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->Emitters.buf[i];
			if (!ei)
			{
				continue;
			}
			EmitterData* ed = static_cast< EmitterData*>(ei);
			mTempNames.pushBack(ed->Name);
			mTempVariantNames.pushBack(NX_APEX_EMITTER_AUTHORING_TYPE_NAME);
		}
	}
	else if (strcmp(nameSpace, NX_HEAT_SOURCE_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), HeatSourceData::staticClassName()) != 0)
			{
				continue;
			}
			HeatSourceData* ed = static_cast< HeatSourceData*>(ei);
			mTempNames.pushBack(ed->Name);
			mTempVariantNames.pushBack(NX_HEAT_SOURCE_AUTHORING_TYPE_NAME);
		}
	}
	else if (strcmp(nameSpace, NX_SUBSTANCE_SOURCE_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), SubstanceSourceData::staticClassName()) != 0)
			{
				continue;
			}
			SubstanceSourceData* ed = static_cast< SubstanceSourceData*>(ei);
			mTempNames.pushBack(ed->Name);
			mTempVariantNames.pushBack(NX_SUBSTANCE_SOURCE_AUTHORING_TYPE_NAME);
		}
	}
	else if (strcmp(nameSpace, NX_JET_FS_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), JetFieldSamplerData::staticClassName()) != 0)
			{
				continue;
			}
			JetFieldSamplerData* ed = static_cast< JetFieldSamplerData*>(ei);
			mTempNames.pushBack(ed->Name);
			mTempVariantNames.pushBack(NX_JET_FS_AUTHORING_TYPE_NAME);
		}
	}
	else if (strcmp(nameSpace, NX_NOISE_FS_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), NoiseFieldSamplerData::staticClassName()) != 0)
			{
				continue;
			}
			NoiseFieldSamplerData* ed = static_cast< NoiseFieldSamplerData*>(ei);
			mTempNames.pushBack(ed->Name);
			mTempVariantNames.pushBack(NX_NOISE_FS_AUTHORING_TYPE_NAME);
		}
	}
	else if (strcmp(nameSpace, NX_VORTEX_FS_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), VortexFieldSamplerData::staticClassName()) != 0)
			{
				continue;
			}
			VortexFieldSamplerData* ed = static_cast< VortexFieldSamplerData*>(ei);
			mTempNames.pushBack(ed->Name);
			mTempVariantNames.pushBack(NX_VORTEX_FS_AUTHORING_TYPE_NAME);
		}
	}
	else if (strcmp(nameSpace, NX_ATTRACTOR_FS_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), AttractorFieldSamplerData::staticClassName()) != 0)
			{
				continue;
			}
			AttractorFieldSamplerData* ed = static_cast< AttractorFieldSamplerData*>(ei);
			mTempNames.pushBack(ed->Name);
			mTempVariantNames.pushBack(NX_ATTRACTOR_FS_AUTHORING_TYPE_NAME);
		}
	}
	else if (strcmp(nameSpace, NX_TURBULENCE_FS_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), TurbulenceFieldSamplerData::staticClassName()) != 0)
			{
				continue;
			}
			TurbulenceFieldSamplerData* ed = static_cast< TurbulenceFieldSamplerData*>(ei);
			mTempNames.pushBack(ed->Name);
			mTempVariantNames.pushBack(NX_TURBULENCE_FS_AUTHORING_TYPE_NAME);
		}
	}
	else if (strcmp(nameSpace, NX_FORCEFIELD_AUTHORING_TYPE_NAME) == 0)
	{
		EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
		for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
		{
			NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
			if (!ei)
			{
				continue;
			}
			if (strcmp(ei->className(), ForceFieldData::staticClassName()) != 0)
			{
				continue;
			}
			ForceFieldData* ed = static_cast< ForceFieldData*>(ei);
			mTempNames.pushBack(ed->Name);
			mTempVariantNames.pushBack(NX_FORCEFIELD_AUTHORING_TYPE_NAME);
		}
	}
	else
	{
		PX_ALWAYS_ASSERT();
	}

	nameCount = mTempNames.size();
	if (nameCount)
	{
		ret = &mTempNames[0];
		variants = &mTempVariantNames[0];
	}

	return ret;
}


bool ModuleParticles::setEffectPackageIOSDatabase(const NxParameterized::Interface* dataBase)
{
	bool ret = false;

	if (dataBase)
	{
		NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
		const char* className = dataBase->className();
		if (strcmp(className, apex::basicios::BasicIOSAssetParam::staticClassName()) == 0 ||  strcmp(className, apex::pxparticleios::ParticleIosAssetParam::staticClassName()) == 0)
		{
			bool revised = false;
			const char* itemName = dataBase->name() ? dataBase->name()  : "defaultBasicIOS";
			EffectPackageIOSDatabaseParams* d = static_cast< EffectPackageIOSDatabaseParams*>(mEffectPackageIOSDatabaseParams);
			for (PxU32 i = 0; i < (PxU32)d->ParticleSimulations.arraySizes[0]; i++)
			{
				NxParameterized::Interface* ei = d->ParticleSimulations.buf[i];
				if (!ei)
				{
					continue;
				}
				ParticleSimulationData* ed = static_cast< ParticleSimulationData*>(ei);
				if (physx::string::stricmp(ed->Name.buf, itemName) == 0)
				{
					ed->IOS->copy(*dataBase);
					initParticleSimulationData(ed);
					revised = true;
					ret = true;
					break;
				}
			}
			if (!revised)
			{
				PxU32 arraySize = d->ParticleSimulations.arraySizes[0];
				NxParameterized::Handle handle(mEffectPackageIOSDatabaseParams);
				NxParameterized::ErrorType err = handle.getParameter("ParticleSimulations");
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				err = handle.resizeArray(arraySize + 1);
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				if (err == NxParameterized::ERROR_NONE)
				{
					NxParameterized::Interface* ei = traits->createNxParameterized(ParticleSimulationData::staticClassName());
					if (ei)
					{
						ParticleSimulationData* ed = static_cast< ParticleSimulationData*>(ei);
						NxParameterized::Handle item(ei);
						err = item.getParameter("Name");
						PX_ASSERT(err == NxParameterized::ERROR_NONE);
						item.setParamString(itemName);
						ei->setName(itemName);
						d->ParticleSimulations.buf[arraySize] = ei;
						ed->IOS = NULL;
						NxParameterized::ErrorType err = dataBase->clone(ed->IOS);
						if ( err != NxParameterized::ERROR_NONE )
						{
							APEX_DEBUG_WARNING("Failed to clone asset.");
						}
						else
						{
							initParticleSimulationData(ed);
						}
					}
					ret = true;
				}
			}
		}
		else if (strcmp(dataBase->className(), EffectPackageIOSDatabaseParams::staticClassName()) == 0)
		{
			if (mEffectPackageIOSDatabaseParams && mEffectPackageIOSDatabaseParams != dataBase)
			{
				mEffectPackageIOSDatabaseParams->destroy();
				mEffectPackageIOSDatabaseParams = NULL;
			}
			if (mEffectPackageIOSDatabaseParams == NULL)
			{
				dataBase->clone(mEffectPackageIOSDatabaseParams);
			}

			EffectPackageIOSDatabaseParams* d = static_cast< EffectPackageIOSDatabaseParams*>(mEffectPackageIOSDatabaseParams);

			{
				if (d->ParticleSimulations.arraySizes[0] == 0)
				{
					NxParameterized::Handle handle(mEffectPackageIOSDatabaseParams);
					NxParameterized::ErrorType err = handle.getParameter("ParticleSimulations");
					PX_ASSERT(err == NxParameterized::ERROR_NONE);
#if NX_SDK_VERSION_MAJOR == 3
					err = handle.resizeArray(3);
#else
					err = handle.resizeArray(1);
#endif
					PX_ASSERT(err == NxParameterized::ERROR_NONE);
					if (err == NxParameterized::ERROR_NONE)
					{
						// create the default BasicIOS
						{
							NxParameterized::Interface* ei = traits->createNxParameterized(ParticleSimulationData::staticClassName());
							d->ParticleSimulations.buf[0] = ei;
							ParticleSimulationData* psd = static_cast< ParticleSimulationData*>(ei);
							if (psd)
							{
								NxParameterized::setParamString(*ei, "Name", "defaultBasicIOS");
								psd->IOS = traits->createNxParameterized(apex::basicios::BasicIOSAssetParam::staticClassName());
							}
						}
#if NX_SDK_VERSION_MAJOR == 3
						// create the default SimpleParticleIOS
						{
							NxParameterized::Interface* ei = traits->createNxParameterized(ParticleSimulationData::staticClassName());
							d->ParticleSimulations.buf[1] = ei;
							ParticleSimulationData* psd = static_cast< ParticleSimulationData*>(ei);
							if (psd)
							{
								NxParameterized::setParamString(*ei, "Name", "defaultSimpleParticleIOS");
								psd->IOS = traits->createNxParameterized(apex::pxparticleios::ParticleIosAssetParam::staticClassName());
								apex::pxparticleios::ParticleIosAssetParam* pia = static_cast< apex::pxparticleios::ParticleIosAssetParam*>(psd->IOS);
								if (pia)
								{
									pia->particleType = traits->createNxParameterized(apex::pxparticleios::SimpleParticleSystemParams::staticClassName());
								}
							}
						}
						// create the default FluidParticleIOS
						{
							NxParameterized::Interface* ei = traits->createNxParameterized(ParticleSimulationData::staticClassName());
							d->ParticleSimulations.buf[2] = ei;
							ParticleSimulationData* psd = static_cast< ParticleSimulationData*>(ei);

							if (psd)
							{
								NxParameterized::setParamString(*ei, "Name", "defaultFluidParticleIOS");
								psd->IOS = traits->createNxParameterized(apex::pxparticleios::ParticleIosAssetParam::staticClassName());
								apex::pxparticleios::ParticleIosAssetParam* pia = static_cast< apex::pxparticleios::ParticleIosAssetParam*>(psd->IOS);

								if (pia)
								{
									pia->particleType = traits->createNxParameterized(apex::pxparticleios::FluidParticleSystemParams::staticClassName());
									apex::pxparticleios::FluidParticleSystemParams* fp = static_cast< apex::pxparticleios::FluidParticleSystemParams*>(pia->particleType);
									if (fp)
									{
										fp->restParticleDistance = 0.02f;
										fp->stiffness = 20;
										fp->viscosity = 6;
									}
									pia->maxParticleCount = 16834;
									pia->particleRadius = 0.05f;
									pia->maxInjectedParticleCount = 0.1f;
									pia->maxMotionDistance = 0.2;
									pia->contactOffset = 0.008f;
									pia->restOffset = 0.004f;
									pia->gridSize = 1.5f;
									pia->damping = 0;
									pia->externalAcceleration = PxVec3(0, 0, 0);
									pia->projectionPlaneNormal = PxVec3(0, 1, 0);
									pia->projectionPlaneDistance = 0;
									pia->particleMass = 0.01f;
									pia->restitution = 0.5f;
									pia->dynamicFriction = 0.05f;
									pia->staticFriction = 0.5f;
									pia->CollisionTwoway = false;
									pia->CollisionWithDynamicActors = true;
									pia->Enable = true;
									pia->ProjectToPlane = false;
									pia->PerParticleRestOffset = false;
									pia->PerParticleCollisionCacheHint = false;
								}
							}
						}
#endif
						ret = true;
					}
				}

				Array< const char* > nameList;
				for (PxI32 i = 0; i < d->ParticleSimulations.arraySizes[0]; i++)
				{
					ParticleSimulationData* e = static_cast< ParticleSimulationData*>(d->ParticleSimulations.buf[i]);
					if (e)
					{
						for (PxU32 j = 0; j < nameList.size(); j++)
						{
							if (physx::string::stricmp(nameList[j], e->Name) == 0)
							{
								NxParameterized::Handle handle(e);
								NxParameterized::ErrorType err = handle.getParameter("Name");
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								if (err == NxParameterized::ERROR_NONE)
								{
									handle.setParamString(getUniqueName(nameList, e->Name));
									ret = true;
									break;
								}
							}
						}
						nameList.pushBack(e->Name);
					}
				}


				for (PxU32 i = 0; i < (PxU32)d->ParticleSimulations.arraySizes[0]; i++)
				{
					ParticleSimulationData* ed = static_cast< ParticleSimulationData* >(d->ParticleSimulations.buf[i]);
					PX_ASSERT(ed);
					if (ed && initParticleSimulationData(ed))
					{
						ret = true;
					}
				}
			}
		}
		else
		{
			PX_ALWAYS_ASSERT(); // not a valid nxparameterized interface
		}
		if (fixupNamedReferences())
		{
			ret = true;
		}
	}
	return ret;
}

static void initColor(apex::iofx::ColorVsLifeCompositeModifierParamsNS::colorLifeStruct_Type&  c, PxF32 lifeRemaining, PxF32 red, PxF32 green, PxF32 blue, PxF32 alpha)
{
	c.lifeRemaining = lifeRemaining;
	c.color.x = red;
	c.color.y = green;
	c.color.z = blue;
	c.color.w = alpha;
}

static void initScale(apex::iofx::ScaleVsLife2DModifierParamsNS::scaleLifeStruct_Type& s, physx::PxF32 lifetime, physx::PxF32 scalex, physx::PxF32 scaley)
{
	s.lifeRemaining = lifetime;
	s.scale.x = scalex;
	s.scale.y = scaley;
}


bool ModuleParticles::setEffectPackageIOFXDatabase(const NxParameterized::Interface* dataBase)
{
	bool ret = false;

	if (dataBase)
	{
		NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
		const char* className = dataBase->className();
		if (strcmp(className, apex::iofx::IofxAssetParameters::staticClassName()) == 0)
		{
			bool revised = false;
			const char* itemName = dataBase->name();
			EffectPackageIOFXDatabaseParams* d = static_cast< EffectPackageIOFXDatabaseParams*>(mEffectPackageIOFXDatabaseParams);
			for (PxU32 i = 0; i < (PxU32)d->GraphicsEffects.arraySizes[0]; i++)
			{
				NxParameterized::Interface* ei = d->GraphicsEffects.buf[i];
				if (!ei)
				{
					continue;
				}
				GraphicsEffectData* ed = static_cast< GraphicsEffectData*>(ei);
				if (physx::string::stricmp(ed->Name.buf, itemName) == 0)
				{
					ed->IOFX->copy(*dataBase);
					revised = true;
					ret = true;
					break;
				}
			}
			if (!revised)
			{
				PxU32 arraySize = d->GraphicsEffects.arraySizes[0];
				NxParameterized::Handle handle(mEffectPackageIOFXDatabaseParams);
				NxParameterized::ErrorType err = handle.getParameter("GraphicsEffects");
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				err = handle.resizeArray(arraySize + 1);
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				if (err == NxParameterized::ERROR_NONE)
				{
					NxParameterized::Interface* ei = traits->createNxParameterized(GraphicsEffectData::staticClassName());
					if (ei)
					{
						GraphicsEffectData* ed = static_cast< GraphicsEffectData*>(ei);
						NxParameterized::Handle item(ei);
						err = item.getParameter("Name");
						PX_ASSERT(err == NxParameterized::ERROR_NONE);
						item.setParamString(itemName);
						ei->setName(itemName);
						d->GraphicsEffects.buf[arraySize] = ei;
						ed->IOFX = NULL;
						dataBase->clone(ed->IOFX);
					}
					ret = true;
				}
			}
		}
		else if (strcmp(dataBase->className(), EffectPackageIOFXDatabaseParams::staticClassName()) == 0)
		{
			if (mEffectPackageIOFXDatabaseParams && mEffectPackageIOFXDatabaseParams != dataBase)
			{
				mEffectPackageIOFXDatabaseParams->destroy();
				mEffectPackageIOFXDatabaseParams = NULL;
			}
			if (mEffectPackageIOFXDatabaseParams == NULL)
			{
				dataBase->clone(mEffectPackageIOFXDatabaseParams);
			}

			EffectPackageIOFXDatabaseParams* d = static_cast< EffectPackageIOFXDatabaseParams*>(mEffectPackageIOFXDatabaseParams);

			{
				if (d->GraphicsEffects.arraySizes[0] == 0)
				{
					NxParameterized::Handle handle(mEffectPackageIOFXDatabaseParams);
					NxParameterized::ErrorType err = handle.getParameter("GraphicsEffects");
					PX_ASSERT(err == NxParameterized::ERROR_NONE);
					err = handle.resizeArray(2);
					PX_ASSERT(err == NxParameterized::ERROR_NONE);
					if (err == NxParameterized::ERROR_NONE)
					{
						{
							NxParameterized::Interface* ei = traits->createNxParameterized(GraphicsEffectData::staticClassName());
							d->GraphicsEffects.buf[0] = ei;
							if (ei)
							{
								GraphicsEffectData* ged = static_cast< GraphicsEffectData*>(ei);
								if (ged)
								{
									NxParameterized::setParamString(*ei, "Name", "defaultSpriteIOFX");
									ged->IOFX = traits->createNxParameterized(apex::iofx::IofxAssetParameters::staticClassName());
									apex::iofx::IofxAssetParameters* iofx = static_cast< apex::iofx::IofxAssetParameters*>(ged->IOFX);
									if (iofx)
									{
										iofx->iofxType = traits->createNxParameterized(apex::iofx::SpriteIofxParameters::staticClassName());
										iofx->renderOutput.useFloat4Color = true;
									}
								}
							}
						}
						{
							NxParameterized::Interface* ei = traits->createNxParameterized(GraphicsEffectData::staticClassName());
							d->GraphicsEffects.buf[1] = ei;
							if (ei)
							{
								GraphicsEffectData* ged = static_cast< GraphicsEffectData*>(ei);
								if (ged)
								{
									NxParameterized::setParamString(*ei, "Name", "defaultMeshIOFX");
									ged->IOFX = traits->createNxParameterized(apex::iofx::IofxAssetParameters::staticClassName());
									apex::iofx::IofxAssetParameters* iofx = static_cast< apex::iofx::IofxAssetParameters*>(ged->IOFX);
									if (iofx)
									{
										iofx->iofxType = traits->createNxParameterized(apex::iofx::MeshIofxParameters::staticClassName());
										iofx->renderOutput.useFloat4Color = true;
									}
								}
							}
						}

						ret = true;
					}
				}


				Array< const char* > nameList;
				for (PxI32 i = 0; i < d->GraphicsEffects.arraySizes[0]; i++)
				{
					GraphicsEffectData* e = static_cast< GraphicsEffectData*>(d->GraphicsEffects.buf[i]);
					if (e)
					{
						for (PxU32 j = 0; j < nameList.size(); j++)
						{
							if (physx::string::stricmp(nameList[j], e->Name) == 0)
							{
								NxParameterized::Handle handle(e);
								NxParameterized::ErrorType err = handle.getParameter("Name");
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								if (err == NxParameterized::ERROR_NONE)
								{
									handle.setParamString(getUniqueName(nameList, e->Name));
									ret = true;
									break;
								}
							}
						}
						nameList.pushBack(e->Name);
					}
				}


				for (PxU32 i = 0; i < (PxU32)d->GraphicsEffects.arraySizes[0]; i++)
				{
					GraphicsEffectData* ed = static_cast< GraphicsEffectData*>(d->GraphicsEffects.buf[i]);
					// first, see if it's assigned to a sprite IOFX without valid default values...
					{
						apex::iofx::IofxAssetParameters* ae = static_cast< apex::iofx::IofxAssetParameters*>(ed->IOFX);
						if (ae)
						{
							if (strcmp(ae->iofxType->className(), apex::iofx::SpriteIofxParameters::staticClassName()) == 0)
							{
								apex::iofx::SpriteIofxParameters* iofxP = static_cast< apex::iofx::SpriteIofxParameters*>(ae->iofxType);
								if (iofxP->spawnModifierList.arraySizes[0] == 0 && iofxP->continuousModifierList.arraySizes[0] == 0)
								{
									ed->IOFX->destroy();
									ed->IOFX = NULL;
									ret = true;
								}
							}
						}
					}

					if (ed->IOFX == NULL)
					{
						ed->IOFX = traits->createNxParameterized(apex::iofx::IofxAssetParameters::staticClassName());
						apex::iofx::IofxAssetParameters* ae = static_cast< apex::iofx::IofxAssetParameters*>(ed->IOFX);
						if (ae)
						{
							ae->renderOutput.useFloat4Color = true;
							NxParameterized::Interface* iofx = traits->createNxParameterized(apex::iofx::SpriteIofxParameters::staticClassName());
							ae->iofxType = iofx;
							apex::iofx::SpriteIofxParameters* iofxP = static_cast< apex::iofx::SpriteIofxParameters*>(iofx);
							if (iofxP)
							{
								NxParameterized::Handle handle(iofx);
								NxParameterized::ErrorType err = handle.getParameter("spriteMaterialName");
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								if (err == NxParameterized::ERROR_NONE)
								{
									err = handle.initParamRef(APEX_MATERIALS_NAME_SPACE,true);
									PX_ASSERT(err == NxParameterized::ERROR_NONE);
									if (err == NxParameterized::ERROR_NONE)
									{
										iofxP->spriteMaterialName->setName("defaultGraphicsMaterial");
									}
								}
							}


							{
								NxParameterized::Handle handle(iofx);
								NxParameterized::ErrorType err = handle.getParameter("spawnModifierList");
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								err = handle.resizeArray(4);
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								if (err == NxParameterized::ERROR_NONE)
								{
									iofxP->spawnModifierList.buf[0] = traits->createNxParameterized(apex::iofx::InitialColorModifierParams::staticClassName());
									iofxP->spawnModifierList.buf[1] = traits->createNxParameterized(apex::iofx::RandomRotationModifierParams::staticClassName());
									iofxP->spawnModifierList.buf[2] = traits->createNxParameterized(apex::iofx::RandomSubtextureModifierParams::staticClassName());
									iofxP->spawnModifierList.buf[3] = traits->createNxParameterized(apex::iofx::RandomScaleModifierParams::staticClassName());
								}
							}

							{
								NxParameterized::Handle handle(iofx);
								NxParameterized::ErrorType err = handle.getParameter("continuousModifierList");
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								err = handle.resizeArray(1);
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								if (err == NxParameterized::ERROR_NONE)
								{
									iofxP->continuousModifierList.buf[0] = traits->createNxParameterized(apex::iofx::ViewDirectionSortingModifierParams::staticClassName());
								}
							}
						}
						ret = true;
					}
					if (ed->IOFX)
					{
						apex::iofx::IofxAssetParameters* ae = static_cast< apex::iofx::IofxAssetParameters*>(ed->IOFX);
						if (ae->iofxType)
						{
							ae->renderOutput.useFloat4Color = true;
							const char* c = ae->iofxType->className();
							if (c && strcmp(c, "MeshIofxParameters") == 0)
							{
								apex::iofx::MeshIofxParameters* m = static_cast< apex::iofx::MeshIofxParameters*>(ae->iofxType);
								if (m->renderMeshList.arraySizes[0] == 0)
								{
									ret = true;
									NxParameterized::Handle handle(m);
									NxParameterized::ErrorType err = handle.getParameter("renderMeshList");
									PX_ASSERT(err == NxParameterized::ERROR_NONE);
									err = handle.resizeArray(1);
									PX_ASSERT(err == NxParameterized::ERROR_NONE);
									if (err == NxParameterized::ERROR_NONE)
									{
										apex::iofx::MeshIofxParametersNS::meshProperties_Type& t = m->renderMeshList.buf[0];
										t.weight = 1;
										NxParameterized::Handle elementHandle(m);
										err = handle.getChildHandle(0, elementHandle);
										PX_ASSERT(err == NxParameterized::ERROR_NONE);
										NxParameterized::Handle item(*elementHandle.getInterface());
										NxParameterized::ErrorType err = elementHandle.getChildHandle(elementHandle.getInterface(), "meshAssetName", item);
										PX_ASSERT(err == NxParameterized:: ERROR_NONE);
										if (err == NxParameterized::ERROR_NONE)
										{
											err = item.initParamRef(APEX_OPAQUE_MESH_NAME_SPACE,true);
											PX_ASSERT(err == NxParameterized::ERROR_NONE);
											if (err == NxParameterized::ERROR_NONE)
											{
												t.meshAssetName->setName("SampleMesh.apx");
											}
										}
									}
									// initialize the spawn modifier list
									{
										NxParameterized::Handle handle(m);
										NxParameterized::ErrorType err = handle.getParameter("spawnModifierList");
										PX_ASSERT(err == NxParameterized::ERROR_NONE);
										err = handle.resizeArray(2);
										PX_ASSERT(err == NxParameterized::ERROR_NONE);
										if (err == NxParameterized::ERROR_NONE)
										{
											m->spawnModifierList.buf[0] = traits->createNxParameterized(apex::iofx::SimpleScaleModifierParams::staticClassName());
											apex::iofx::SimpleScaleModifierParams* ss = static_cast< apex::iofx::SimpleScaleModifierParams*>(m->spawnModifierList.buf[0]);
											if (ss)
											{
												ss->scaleFactor = PxVec3(1, 1, 1);
												m->spawnModifierList.buf[1] = traits->createNxParameterized(apex::iofx::RotationModifierParams::staticClassName());
												NxParameterized::setParamEnum(*m->spawnModifierList.buf[1], "rollType", "SPHERICAL");
											}
										}
									}
									// initialize the continuous modifier list
									{
										NxParameterized::Handle handle(m);
										NxParameterized::ErrorType err = handle.getParameter("continuousModifierList");
										PX_ASSERT(err == NxParameterized::ERROR_NONE);
										err = handle.resizeArray(2);
										PX_ASSERT(err == NxParameterized::ERROR_NONE);
										if (err == NxParameterized::ERROR_NONE)
										{
											// init RotationModifierParams
											{
												m->continuousModifierList.buf[0] = traits->createNxParameterized(apex::iofx::RotationModifierParams::staticClassName());
												if (m->continuousModifierList.buf[0])
												{
													NxParameterized::setParamEnum(*m->continuousModifierList.buf[0], "rollType", "SPHERICAL");
													apex::iofx::RotationModifierParams* rmp = static_cast< apex::iofx::RotationModifierParams*>(m->continuousModifierList.buf[0]);
													rmp->maxRotationRatePerSec = 0;
													rmp->maxSettleRatePerSec = 1;
													rmp->collisionRotationMultiplier = 1;
													rmp->inAirRotationMultiplier = 1;
												}
											}
											{
												m->continuousModifierList.buf[1] = traits->createNxParameterized(apex::iofx::ScaleVsLife3DModifierParams::staticClassName());
												if (m->continuousModifierList.buf[1])
												{
													NxParameterized::Handle h(m->continuousModifierList.buf[1]);
													err = h.getParameter("controlPoints");
													PX_ASSERT(err == NxParameterized::ERROR_NONE);
													err = h.resizeArray(3);
													apex::iofx::ScaleVsLife3DModifierParams& svl = *(static_cast< apex::iofx::ScaleVsLife3DModifierParams*>(m->continuousModifierList.buf[1]));
													svl.controlPoints.buf[0].lifeRemaining = 0;
													svl.controlPoints.buf[0].scale = PxVec3(0, 0, 0);

													svl.controlPoints.buf[1].lifeRemaining = 0.5f;
													svl.controlPoints.buf[1].scale = PxVec3(1, 1, 1);

													svl.controlPoints.buf[2].lifeRemaining = 1;
													svl.controlPoints.buf[2].scale = PxVec3(0, 0, 0);

												}
											}
										}

									}
								}
							}
						}
					}
				}
			}
		}
		else
		{
			PX_ALWAYS_ASSERT();
		}
	}


	return ret;

}

bool ModuleParticles::setEffectPackageEmitterDatabase(const NxParameterized::Interface* dataBase)
{
	bool ret = false;

	if (dataBase)
	{
		NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
		const char* className = dataBase->className();
		if (strcmp(className, apex::emitter::ApexEmitterAssetParameters::staticClassName()) == 0)
		{
			bool revised = false;
			const char* itemName = dataBase->name();
			EffectPackageEmitterDatabaseParams* d = static_cast< EffectPackageEmitterDatabaseParams*>(mEffectPackageEmitterDatabaseParams);
			for (PxU32 i = 0; i < (PxU32)d->Emitters.arraySizes[0]; i++)
			{
				NxParameterized::Interface* ei = d->Emitters.buf[i];
				if (!ei)
				{
					continue;
				}
				EmitterData* ed = static_cast< EmitterData*>(ei);
				if (physx::string::stricmp(ed->Name.buf, itemName) == 0)
				{
					ed->Emitter->copy(*dataBase);
					revised = true;
					ret = true;
					break;
				}
			}
			if (!revised)
			{
				PxU32 arraySize = d->Emitters.arraySizes[0];
				NxParameterized::Handle handle(mEffectPackageEmitterDatabaseParams);
				NxParameterized::ErrorType err = handle.getParameter("Emitters");
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				err = handle.resizeArray(arraySize + 1);
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				if (err == NxParameterized::ERROR_NONE)
				{
					NxParameterized::Interface* ei = traits->createNxParameterized(EmitterData::staticClassName());
					if (ei)
					{
						EmitterData* ed = static_cast< EmitterData*>(ei);
						NxParameterized::Handle item(ei);
						err = item.getParameter("Name");
						PX_ASSERT(err == NxParameterized::ERROR_NONE);
						item.setParamString(itemName);
						ei->setName(itemName);
						d->Emitters.buf[arraySize] = ei;
						ed->Emitter = NULL;
						dataBase->clone(ed->Emitter);
					}
					ret = true;
				}
			}
		}
		else if (strcmp(dataBase->className(), EffectPackageEmitterDatabaseParams::staticClassName()) == 0)
		{
			if (mEffectPackageEmitterDatabaseParams && mEffectPackageEmitterDatabaseParams != dataBase)
			{
				mEffectPackageEmitterDatabaseParams->destroy();
				mEffectPackageEmitterDatabaseParams = NULL;
			}
			if (mEffectPackageEmitterDatabaseParams == NULL)
			{
				dataBase->clone(mEffectPackageEmitterDatabaseParams);
			}

			EffectPackageEmitterDatabaseParams* d = static_cast< EffectPackageEmitterDatabaseParams*>(mEffectPackageEmitterDatabaseParams);

			{
				if (d->Emitters.arraySizes[0] == 0)
				{
					NxParameterized::Handle handle(mEffectPackageEmitterDatabaseParams);
					NxParameterized::ErrorType err = handle.getParameter("Emitters");
					PX_ASSERT(err == NxParameterized::ERROR_NONE);
					err = handle.resizeArray(1);
					PX_ASSERT(err == NxParameterized::ERROR_NONE);
					if (err == NxParameterized::ERROR_NONE)
					{
						NxParameterized::Interface* ei = traits->createNxParameterized(EmitterData::staticClassName());
						d->Emitters.buf[0] = ei;
						ret = true;
					}
				}


				Array< const char* > nameList;
				for (PxI32 i = 0; i < d->Emitters.arraySizes[0]; i++)
				{
					EmitterData* e = static_cast< EmitterData*>(d->Emitters.buf[i]);
					if (e)
					{
						for (PxU32 j = 0; j < nameList.size(); j++)
						{
							if (physx::string::stricmp(nameList[j], e->Name) == 0)
							{
								NxParameterized::Handle handle(e);
								NxParameterized::ErrorType err = handle.getParameter("Name");
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								if (err == NxParameterized::ERROR_NONE)
								{
									handle.setParamString(getUniqueName(nameList, e->Name));
									ret = true;
									break;
								}
							}
						}
						nameList.pushBack(e->Name);
					}
				}

				for (PxU32 i = 0; i < (PxU32)d->Emitters.arraySizes[0]; i++)
				{
					EmitterData* ed = static_cast< EmitterData*>(d->Emitters.buf[i]);
					if (ed->Emitter == NULL)
					{
						ed->Emitter = traits->createNxParameterized(apex::emitter::ApexEmitterAssetParameters::staticClassName());
						apex::emitter::ApexEmitterAssetParameters* ae = static_cast< apex::emitter::ApexEmitterAssetParameters*>(ed->Emitter);
						if (ae)
						{
							ae->densityRange.min = 0;
							ae->densityRange.max = 0;
							ae->rateRange.min = 1750;
							ae->rateRange.max = 2000;
							ae->lifetimeRange.min = 2;
							ae->lifetimeRange.max = 4;
							ae->velocityRange.min.x = -1;
							ae->velocityRange.min.y = 0;
							ae->velocityRange.min.z = -1;
							ae->velocityRange.max.x = 1;
							ae->velocityRange.max.y = 8;
							ae->velocityRange.max.z = 1;

							ae->maxSamples = 10000;
							ae->geometryType = traits->createNxParameterized(apex::emitter::EmitterGeomSphereParams::staticClassName());
							apex::emitter::EmitterGeomSphereParams* sphere = static_cast< apex::emitter::EmitterGeomSphereParams*>(ae->geometryType);
							if (sphere)
							{
								sphere->radius = 1;
							}
							ae->emitterDuration = 1200;
						}
						ret = true;
					}
					if (ed->Emitter)
					{
						apex::emitter::ApexEmitterAssetParameters* ae = static_cast< apex::emitter::ApexEmitterAssetParameters*>(ed->Emitter);
						NxParameterized::Handle handle(ed->Emitter);
						NxParameterized::ErrorType err = ed->Emitter->getParameterHandle("iofxAssetName", handle);
						PX_ASSERT(err == NxParameterized::ERROR_NONE);
						if (err == NxParameterized::ERROR_NONE)
						{
							NxParameterized::Interface* val = NULL;
							err = handle.getParamRef(val);
							if (val == NULL)
							{
								err = handle.initParamRef(NX_IOFX_AUTHORING_TYPE_NAME,true);
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								if (err == NxParameterized::ERROR_NONE)
								{
									char scratch[512];
									sprintf_s(scratch, 512, "defaultSpriteIOFX");
									ae->iofxAssetName->setName(scratch);
								}
							}
						}
					}
					if (ed->Emitter)
					{
						apex::emitter::ApexEmitterAssetParameters* ae = static_cast< apex::emitter::ApexEmitterAssetParameters*>(ed->Emitter);
						NxParameterized::Handle handle(ed->Emitter);
						NxParameterized::ErrorType err = ed->Emitter->getParameterHandle("iosAssetName", handle);
						PX_ASSERT(err == NxParameterized::ERROR_NONE);
						if (err == NxParameterized::ERROR_NONE)
						{
							NxParameterized::Interface* val = NULL;
							err = handle.getParamRef(val);
							if (val == NULL)
							{
								err = handle.initParamRef(NX_BASIC_IOS_AUTHORING_TYPE_NAME,true);
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								if (err == NxParameterized::ERROR_NONE)
								{
									char scratch[512];
									sprintf_s(scratch, 512, "defaultBasicIOS");
									ae->iosAssetName->setName(scratch);
								}
							}
						}
					}
				}
			}
		}
		else
		{
			PX_ALWAYS_ASSERT(); // invalid NxParameterized::Interface passed in
		}
	}


	return ret;
}

enum FieldSamplerType
{
	FST_NONE,
	FST_FORCE_FIELD,
	FST_HEAT_SOURCE,
	FST_SUBSTANCE_SOURCE,
	FST_JET,
	FST_ATTRACTOR,
	FST_TURBULENCE,
	FST_NOISE,
	FST_VORTEX,
	FST_LAST
};

static FieldSamplerType getFieldSamplerType(const NxParameterized::Interface* iface)
{
	FieldSamplerType ret = FST_NONE;

	if (iface)
	{
		const char* className = iface->className();
		if (strcmp(className, apex::forcefield::ForceFieldAssetParams::staticClassName()) == 0)
		{
			ret = FST_FORCE_FIELD;
		}
		else if (strcmp(className, turbulencefs::HeatSourceAssetParams::staticClassName()) == 0)
		{
			ret = FST_HEAT_SOURCE;
		}
		else if (strcmp(className, turbulencefs::SubstanceSourceAssetParams::staticClassName()) == 0)
		{
			ret = FST_SUBSTANCE_SOURCE;
		}
		else if (strcmp(className, apex::basicfs::JetFSAssetParams::staticClassName()) == 0)
		{
			ret = FST_JET;
		}
		else if (strcmp(className, apex::basicfs::NoiseFSAssetParams::staticClassName()) == 0)
		{
			ret = FST_NOISE;
		}
		else if (strcmp(className, apex::basicfs::VortexFSAssetParams::staticClassName()) == 0)
		{
			ret = FST_VORTEX;
		}
		else if (strcmp(className, apex::basicfs::AttractorFSAssetParams::staticClassName()) == 0)
		{
			ret = FST_ATTRACTOR;
		}
		else if (strcmp(className, turbulencefs::TurbulenceFSAssetParams::staticClassName()) == 0)
		{
			ret = FST_TURBULENCE;
		}
		else if (strcmp(className, particles::ForceFieldData::staticClassName()) == 0)
		{
			ret = FST_FORCE_FIELD;
		}
		else if (strcmp(className, particles::HeatSourceData::staticClassName()) == 0)
		{
			ret = FST_HEAT_SOURCE;
		}
		else if (strcmp(className, particles::SubstanceSourceData::staticClassName()) == 0)
		{
			ret = FST_SUBSTANCE_SOURCE;
		}
		else if (strcmp(className, particles::AttractorFieldSamplerData::staticClassName()) == 0)
		{
			ret = FST_ATTRACTOR;
		}
		else if (strcmp(className, particles::JetFieldSamplerData::staticClassName()) == 0)
		{
			ret = FST_JET;
		}
		else if (strcmp(className, particles::NoiseFieldSamplerData::staticClassName()) == 0)
		{
			ret = FST_NOISE;
		}
		else if (strcmp(className, particles::VortexFieldSamplerData::staticClassName()) == 0)
		{
			ret = FST_VORTEX;
		}
		else if (strcmp(className, particles::TurbulenceFieldSamplerData::staticClassName()) == 0)
		{
			ret = FST_TURBULENCE;
		}
	}

	return ret;
}

static NxParameterized::Interface* getFieldSamplerData(FieldSamplerType fst, NxParameterized::Traits* traits, const NxParameterized::Interface* dataBase)
{
	NxParameterized::Interface* ret = NULL;

	const char* itemName = dataBase->name();
	NxParameterized::ErrorType err;

	switch (fst)
	{
	case FST_FORCE_FIELD:
	{
		NxParameterized::Interface* ei = traits->createNxParameterized(ForceFieldData::staticClassName());
		ForceFieldData* ed = static_cast< ForceFieldData*>(ei);
		if (ed)
		{
			NxParameterized::Handle item(ei);
			err = item.getParameter("Name");
			PX_ASSERT(err == NxParameterized::ERROR_NONE);
			item.setParamString(itemName);
			ei->setName(itemName);
			ret = ei;
			ed->ForceField = NULL;
			dataBase->clone(ed->ForceField);
		}
	}
	break;
	case FST_HEAT_SOURCE:
	{
		NxParameterized::Interface* ei = traits->createNxParameterized(HeatSourceData::staticClassName());
		HeatSourceData* ed = static_cast< HeatSourceData*>(ei);
		if (ed)
		{
			NxParameterized::Handle item(ei);
			err = item.getParameter("Name");
			PX_ASSERT(err == NxParameterized::ERROR_NONE);
			item.setParamString(itemName);
			ei->setName(itemName);
			ret = ei;
			ed->HeatSource = NULL;
			dataBase->clone(ed->HeatSource);
		}
	}
	break;
	case FST_SUBSTANCE_SOURCE:
		{
			NxParameterized::Interface* ei = traits->createNxParameterized(SubstanceSourceData::staticClassName());
			SubstanceSourceData* ed = static_cast< SubstanceSourceData*>(ei);
			if (ed)
			{
				NxParameterized::Handle item(ei);
				err = item.getParameter("Name");
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				item.setParamString(itemName);
				ei->setName(itemName);
				ret = ei;
				ed->SubstanceSource = NULL;
				dataBase->clone(ed->SubstanceSource);
			}
		}
		break;
	case FST_ATTRACTOR:
	{
		NxParameterized::Interface* ei = traits->createNxParameterized(AttractorFieldSamplerData::staticClassName());
		AttractorFieldSamplerData* ed = static_cast< AttractorFieldSamplerData*>(ei);
		if (ed)
		{
			NxParameterized::Handle item(ei);
			err = item.getParameter("Name");
			PX_ASSERT(err == NxParameterized::ERROR_NONE);
			item.setParamString(itemName);
			ei->setName(itemName);
			ret = ei;
			ed->AttractorFieldSampler = NULL;
			dataBase->clone(ed->AttractorFieldSampler);
		}
	}
	break;
	case FST_VORTEX:
	{
		NxParameterized::Interface* ei = traits->createNxParameterized(VortexFieldSamplerData::staticClassName());
		VortexFieldSamplerData* ed = static_cast< VortexFieldSamplerData*>(ei);
		if (ed)
		{
			NxParameterized::Handle item(ei);
			err = item.getParameter("Name");
			PX_ASSERT(err == NxParameterized::ERROR_NONE);
			item.setParamString(itemName);
			ei->setName(itemName);
			ret = ei;
			ed->VortexFieldSampler = NULL;
			dataBase->clone(ed->VortexFieldSampler);
		}
	}
	break;
	case FST_NOISE:
	{
		NxParameterized::Interface* ei = traits->createNxParameterized(NoiseFieldSamplerData::staticClassName());
		NoiseFieldSamplerData* ed = static_cast< NoiseFieldSamplerData*>(ei);
		if (ed)
		{
			NxParameterized::Handle item(ei);
			err = item.getParameter("Name");
			PX_ASSERT(err == NxParameterized::ERROR_NONE);
			item.setParamString(itemName);
			ei->setName(itemName);
			ret = ei;
			ed->NoiseFieldSampler = NULL;
			dataBase->clone(ed->NoiseFieldSampler);
		}
	}
	break;
	case FST_JET:
	{
		NxParameterized::Interface* ei = traits->createNxParameterized(JetFieldSamplerData::staticClassName());
		JetFieldSamplerData* ed = static_cast< JetFieldSamplerData*>(ei);
		if (ed)
		{
			NxParameterized::Handle item(ei);
			err = item.getParameter("Name");
			PX_ASSERT(err == NxParameterized::ERROR_NONE);
			item.setParamString(itemName);
			ei->setName(itemName);
			ret = ei;
			ed->JetFieldSampler = NULL;
			dataBase->clone(ed->JetFieldSampler);
		}
	}
	break;
	case FST_TURBULENCE:
	{
		NxParameterized::Interface* ei = traits->createNxParameterized(TurbulenceFieldSamplerData::staticClassName());
		TurbulenceFieldSamplerData* ed = static_cast< TurbulenceFieldSamplerData*>(ei);
		if (ed)
		{
			NxParameterized::Handle item(ei);
			err = item.getParameter("Name");
			PX_ASSERT(err == NxParameterized::ERROR_NONE);
			item.setParamString(itemName);
			ei->setName(itemName);
			ret = ei;
			ed->TurbulenceFieldSampler = NULL;
			dataBase->clone(ed->TurbulenceFieldSampler);
		}
	}
	break;

	default:
		PX_ALWAYS_ASSERT(); // not yet implemented!
		break;
	}

	return ret;
}

bool ModuleParticles::fixFieldSamplerCollisionFilterNames(NxParameterized::Interface *iface)
{
	bool ret = false;

	FieldSamplerType fst = getFieldSamplerType(iface);
	NxParameterized::Interface *asset = NULL;

	const char *filterName=NULL;

	switch (fst)
	{
		case FST_FORCE_FIELD:
			{
				ForceFieldData* ed = static_cast< ForceFieldData*>(iface);
				asset = ed->ForceField;
				filterName = "ForceFieldFS=all";
			}
			break;
		case FST_HEAT_SOURCE:
			{
			}
			break;
		case FST_SUBSTANCE_SOURCE:
			{
			}
			break;
		case FST_ATTRACTOR:
			{
				AttractorFieldSamplerData* ed = static_cast< AttractorFieldSamplerData*>(iface);
				asset = ed->AttractorFieldSampler;
				filterName = "AttractorFS=all";
			}
			break;
		case FST_VORTEX:
			{
				VortexFieldSamplerData* ed = static_cast< VortexFieldSamplerData*>(iface);
				asset = ed->VortexFieldSampler;
				filterName = "VortexFS=all";
			}
			break;
		case FST_NOISE:
			{
				NoiseFieldSamplerData* ed = static_cast< NoiseFieldSamplerData*>(iface);
				asset = ed->NoiseFieldSampler;
				filterName = "NoiseFS=all";
			}
			break;
		case FST_JET:
			{
				JetFieldSamplerData* ed = static_cast< JetFieldSamplerData*>(iface);
				asset = ed->JetFieldSampler;
				filterName = "JetFS=all";
			}
			break;
		case FST_TURBULENCE:
			{
				TurbulenceFieldSamplerData* ed = static_cast< TurbulenceFieldSamplerData*>(iface);
				asset = ed->TurbulenceFieldSampler;
				filterName = "TurbulenceFS=all";
			}
			break;
	}
	if ( asset && filterName )
	{
		const char *fieldBoundaryFilterDataName=NULL;
		const char *fieldSamplerFilterDataName=NULL;
		bool ok = NxParameterized::getParamString(*asset,"fieldBoundaryFilterDataName",fieldBoundaryFilterDataName);
		PX_ASSERT( ok );
		ok = NxParameterized::getParamString(*asset,"fieldSamplerFilterDataName",fieldSamplerFilterDataName);
		PX_ASSERT( ok );

		if ( fieldBoundaryFilterDataName == NULL || fieldBoundaryFilterDataName[0] == 0 || strcmp(fieldBoundaryFilterDataName,"defaultFieldBoundaryFilterDataName") == 0 )
		{
			NxParameterized::setParamString(*asset,"fieldBoundaryFilterDataName",filterName);
			ret = true;
		}
		if ( fieldSamplerFilterDataName == NULL || fieldSamplerFilterDataName[0] == 0 || strcmp(fieldSamplerFilterDataName,"defaultFieldSamplerFilterDataName") == 0 )
		{
			NxParameterized::setParamString(*asset,"fieldSamplerFilterDataName",filterName);
			ret = true;
		}
		if ( fst == FST_TURBULENCE )
		{
			const char *collisionFilterDataName=NULL;
			bool ok = NxParameterized::getParamString(*asset,"collisionFilterDataName",collisionFilterDataName);
			PX_UNUSED( ok );
			PX_ASSERT( ok );
			if ( collisionFilterDataName == NULL || collisionFilterDataName[0] == 0 || strcmp(collisionFilterDataName,"defaultTurbulenceCollisionFilterDataName") == 0 )
			{
				NxParameterized::setParamString(*asset,"collisionFilterDataName",filterName);
				ret = true;
			}
		}
	}
	return ret;
}

bool ModuleParticles::setEffectPackageFieldSamplerDatabase(const NxParameterized::Interface* dataBase)
{
	bool ret = false;

	if (dataBase)
	{
		NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
		FieldSamplerType fst = getFieldSamplerType(dataBase);
		if (fst != FST_NONE)
		{
			bool revised = false;
			const char* itemName = dataBase->name();
			EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);
			for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
			{
				NxParameterized::Interface* ei = d->FieldSamplers.buf[i];
				if (!ei)
				{
					continue;
				}
				const char* dataName;
				NxParameterized::getParamString(*ei, "Name", dataName);
				if (physx::string::stricmp(dataName, itemName) == 0)
				{
					ei->destroy();
					d->FieldSamplers.buf[i] = getFieldSamplerData(fst, traits, dataBase);
					fixFieldSamplerCollisionFilterNames( d->FieldSamplers.buf[i] );
					revised = true;
					ret = true;
					break;
				}
			}
			if (!revised)
			{
				PxU32 arraySize = d->FieldSamplers.arraySizes[0];
				NxParameterized::Handle handle(mEffectPackageFieldSamplerDatabaseParams);
				NxParameterized::ErrorType err = handle.getParameter("FieldSamplers");
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				err = handle.resizeArray(arraySize + 1);
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				if (err == NxParameterized::ERROR_NONE)
				{
					d->FieldSamplers.buf[arraySize] = getFieldSamplerData(fst, traits, dataBase);
					fixFieldSamplerCollisionFilterNames( d->FieldSamplers.buf[arraySize] );
				}
			}
		}
		else if (strcmp(dataBase->className(), EffectPackageFieldSamplerDatabaseParams::staticClassName()) == 0)
		{
			if (mEffectPackageFieldSamplerDatabaseParams && mEffectPackageFieldSamplerDatabaseParams != dataBase)
			{
				mEffectPackageFieldSamplerDatabaseParams->destroy();
				mEffectPackageFieldSamplerDatabaseParams = NULL;
			}

			if (mEffectPackageFieldSamplerDatabaseParams == NULL)
			{
				dataBase->clone(mEffectPackageFieldSamplerDatabaseParams);
			}

			EffectPackageFieldSamplerDatabaseParams* d = static_cast< EffectPackageFieldSamplerDatabaseParams*>(mEffectPackageFieldSamplerDatabaseParams);

			{
				if (d->FieldSamplers.arraySizes[0] == 0)
				{
					NxParameterized::Handle handle(mEffectPackageFieldSamplerDatabaseParams);
					NxParameterized::ErrorType err = handle.getParameter("FieldSamplers");
					PX_ASSERT(err == NxParameterized::ERROR_NONE);
					err = handle.resizeArray(1);
					PX_ASSERT(err == NxParameterized::ERROR_NONE);
					if (err == NxParameterized::ERROR_NONE)
					{
						NxParameterized::Interface* ei = traits->createNxParameterized(ForceFieldData::staticClassName());
						d->FieldSamplers.buf[0] = ei;
						ret = true;
					}
				}


				Array< const char* > nameList;
				for (PxI32 i = 0; i < d->FieldSamplers.arraySizes[0]; i++)
				{
					ForceFieldData* e = static_cast< ForceFieldData*>(d->FieldSamplers.buf[i]);
					if (e)
					{
						for (PxU32 j = 0; j < nameList.size(); j++)
						{
							if (physx::string::stricmp(nameList[j], e->Name) == 0)
							{
								NxParameterized::Handle handle(e);
								NxParameterized::ErrorType err = handle.getParameter("Name");
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								if (err == NxParameterized::ERROR_NONE)
								{
									handle.setParamString(getUniqueName(nameList, e->Name));
									ret = true;
									break;
								}
							}
						}
						nameList.pushBack(e->Name);
					}
				}


				for (PxU32 i = 0; i < (PxU32)d->FieldSamplers.arraySizes[0]; i++)
				{
					NxParameterized::Interface* iface = d->FieldSamplers.buf[i];
					FieldSamplerType fst = getFieldSamplerType(iface);
					switch (fst)
					{
					case FST_FORCE_FIELD:
					{
						ForceFieldData* ed = static_cast< ForceFieldData*>(d->FieldSamplers.buf[i]);
						if (ed->ForceField == NULL)
						{
							ed->ForceField = traits->createNxParameterized(apex::forcefield::ForceFieldAssetParams::staticClassName());
							ret = true;
						}
						apex::forcefield::ForceFieldAssetParams* fap = static_cast< apex::forcefield::ForceFieldAssetParams*>(ed->ForceField);
						if (fap && fap->strength == 0 && fap->lifetime == 0)    // initialize the forcefield to reasonable values; if lifetime is zero, then the force field is not considered initalized.
						{
							ret = true;
							fap->defScale = 1;
							fap->strength = 20.0f;
							fap->lifetime = 1000;

							fap->includeShapeParameters.dimensions = PxVec3(5, 5, 5);

							fap->falloffParameters.multiplier = 0.5f;
							fap->falloffParameters.start = 100;
							fap->falloffParameters.end = 0;

							fap->noiseParameters.strength = 0.01f;
							fap->noiseParameters.spaceScale = 8;
							fap->noiseParameters.timeScale = 1;
							fap->noiseParameters.octaves = 3;
						}
					}
					break;
					case FST_HEAT_SOURCE:
					{
						HeatSourceData* ed = static_cast< HeatSourceData*>(d->FieldSamplers.buf[i]);
						if (ed->HeatSource == NULL)
						{
							ed->HeatSource = traits->createNxParameterized(turbulencefs::HeatSourceAssetParams::staticClassName());
							ret = true;
						}
						turbulencefs::HeatSourceAssetParams* ap = static_cast< turbulencefs::HeatSourceAssetParams*>(ed->HeatSource);
						if (ap && ap->geometryType == NULL)
						{
							ap->averageTemperature = 4;
							ap->stdTemperature = 2;
							ap->geometryType = traits->createNxParameterized(turbulencefs::HeatSourceGeomSphereParams::staticClassName());
							ret = true;
						}
					}
					break;
					case FST_SUBSTANCE_SOURCE:
						{
							SubstanceSourceData* ed = static_cast< SubstanceSourceData*>(d->FieldSamplers.buf[i]);
							if (ed->SubstanceSource == NULL)
							{
								ed->SubstanceSource = traits->createNxParameterized(turbulencefs::SubstanceSourceAssetParams::staticClassName());
								ret = true;
							}
							turbulencefs::SubstanceSourceAssetParams* ap = static_cast< turbulencefs::SubstanceSourceAssetParams*>(ed->SubstanceSource);
							if (ap && ap->geometryType == NULL)
							{
								ap->averageDensity = 32;
								ap->stdDensity = 16;
								ap->geometryType = traits->createNxParameterized(turbulencefs::HeatSourceGeomSphereParams::staticClassName());
								ret = true;
							}
						}
						break;
					case FST_ATTRACTOR:
					{
						AttractorFieldSamplerData* ed = static_cast< AttractorFieldSamplerData*>(d->FieldSamplers.buf[i]);
						if (ed->AttractorFieldSampler == NULL)
						{
							ed->AttractorFieldSampler = traits->createNxParameterized(apex::basicfs::AttractorFSAssetParams::staticClassName());
							apex::basicfs::AttractorFSAssetParams* a = static_cast< apex::basicfs::AttractorFSAssetParams*>(ed->AttractorFieldSampler);
							if (a)
							{
								a->boundaryFadePercentage = 0.3f;
								a->radius = 2;
								a->constFieldStrength = 20;
								a->variableFieldStrength = 1;
							}
							ret = true;
						}
					}
					break;
					case FST_VORTEX:
					{
						VortexFieldSamplerData* ed = static_cast< VortexFieldSamplerData*>(d->FieldSamplers.buf[i]);
						if (ed->VortexFieldSampler == NULL)
						{
							ed->VortexFieldSampler = traits->createNxParameterized(apex::basicfs::VortexFSAssetParams::staticClassName());
							apex::basicfs::VortexFSAssetParams* j = static_cast< apex::basicfs::VortexFSAssetParams*>(ed->VortexFieldSampler);
							if (j)
							{
								j->boundaryFadePercentage = 0.1f;
								j->height = 1;
								j->bottomRadius = 1;
								j->topRadius = 1;
								j->rotationalStrength = 4;
								j->radialStrength = 4;
								j->liftStrength = 1;
							}
							ret = true;
						}
					}
					break;
					case FST_NOISE:
					{
						NoiseFieldSamplerData* ed = static_cast< NoiseFieldSamplerData*>(d->FieldSamplers.buf[i]);
						if (ed->NoiseFieldSampler == NULL)
						{
							ed->NoiseFieldSampler = traits->createNxParameterized(apex::basicfs::NoiseFSAssetParams::staticClassName());
							ret = true;
						}
					}
					break;
					case FST_JET:
					{
						JetFieldSamplerData* ed = static_cast< JetFieldSamplerData*>(d->FieldSamplers.buf[i]);
						if (ed->JetFieldSampler == NULL)
						{
							ed->JetFieldSampler = traits->createNxParameterized(apex::basicfs::JetFSAssetParams::staticClassName());
							apex::basicfs::JetFSAssetParams* j = static_cast< apex::basicfs::JetFSAssetParams*>(ed->JetFieldSampler);
							if (j)
							{
								j->defaultScale = 1;
								j->boundaryFadePercentage = 0.3f;
								j->fieldDirection = PxVec3(1, 0, 0);
								j->fieldDirectionDeviationAngle = 0;
								j->fieldDirectionOscillationPeriod = 0;
								j->fieldStrength = 10;
								j->fieldStrengthDeviationPercentage = 0;
								j->fieldStrengthOscillationPeriod = 0;
								j->gridShapeRadius = 2;
								j->gridShapeHeight = 2;
								j->gridBoundaryFadePercentage = 0.01f;
								j->nearRadius = 0.1f;
								j->pivotRadius = 0.2f;
								j->farRadius = 4;
								j->directionalStretch = 1;
								j->averageStartDistance = 1;
								j->averageEndDistance = 5;
								j->noisePercentage = 0.1f;
								j->noiseTimeScale = 1;
								j->noiseOctaves = 1;
							}
							ret = true;
						}
					}
					break;
					case FST_TURBULENCE:
					{
						TurbulenceFieldSamplerData* ed = static_cast< TurbulenceFieldSamplerData*>(d->FieldSamplers.buf[i]);
						if (ed->TurbulenceFieldSampler == NULL)
						{
							ed->TurbulenceFieldSampler = traits->createNxParameterized(turbulencefs::TurbulenceFSAssetParams::staticClassName());
							turbulencefs::TurbulenceFSAssetParams* fs = static_cast< turbulencefs::TurbulenceFSAssetParams*>(ed->TurbulenceFieldSampler);
							if (fs)
							{
								fs->gridSizeWorld = PxVec3(15, 15, 15);
								fs->updatesPerFrameRange.min = 0;
								fs->updatesPerFrameRange.max = 1;
								fs->angularVelocityMultiplier = 1;
								fs->angularVelocityClamp = 100000;
								fs->linearVelocityMultiplier = 1;
								fs->linearVelocityClamp = 100000;
								fs->boundaryFadePercentage = 0.3f;
								fs->boundarySizePercentage = 1;
								fs->maxCollidingObjects = 32;
								fs->maxHeatSources = 8;
								fs->dragCoeff = 0;
								fs->externalVelocity = PxVec3(0, 0, 0);
								fs->fieldVelocityMultiplier = 1;
								fs->fieldVelocityWeight = 0.75;
								fs->useHeat = true;
								fs->heatParams.temperatureBasedForceMultiplier = 0.02f;
								fs->heatParams.ambientTemperature = 0;
								fs->heatParams.heatForceDirection = PxVec3(0, 1, 0);
								fs->isEnabledOptimizedLOD = false;
								fs->customLOD = 1;
								fs->lodWeights.maxDistance = 100;
								fs->lodWeights.distanceWeight = 1;
								fs->lodWeights.bias = 0;
								fs->lodWeights.benefitBias = 0;
								fs->lodWeights.benefitWeight = 1;
								fs->noiseParams.noiseStrength = 0;
								fs->noiseParams.noiseSpacePeriod = PxVec3(0.9f, 0.9f, 0.9f);
								fs->noiseParams.noiseTimePeriod = 10;
								fs->noiseParams.noiseOctaves = 1;
								fs->dragCoeffForRigidBody = 0;
								fs->fluidViscosity = 0;
							}

							ret = true;
						}
					}
					break;
					default:
						PX_ALWAYS_ASSERT(); // not yet implemented
						break;
					}
					if ( fixFieldSamplerCollisionFilterNames( iface ) )
					{
						ret = true;
					}
				}
			}
		}
		else
		{
			PX_ALWAYS_ASSERT(); // invalid NxParameterized::Interface passed in
		}
	}


	return ret;
}


static void setNamedReference(NxParameterized::Interface* parentInterface, const char* parentName, const char* authoringTypeName)
{
	NxParameterized::Handle handle(parentInterface);
	NxParameterized::ErrorType err = parentInterface->getParameterHandle(parentName, handle);
	PX_ASSERT(err == NxParameterized::ERROR_NONE);
	if (err == NxParameterized::ERROR_NONE)
	{
		err = handle.initParamRef(authoringTypeName,true);
		PX_ASSERT(err == NxParameterized::ERROR_NONE);
	}
}

bool ModuleParticles::setEffectPackageDatabase(const NxParameterized::Interface* dataBase)
{
	bool ret = false;

	if (dataBase)
	{
		NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
		const char* className = dataBase->className();
		if (strcmp(className, EffectPackageAssetParams::staticClassName()) == 0)
		{
			bool revised = false;
			const char* itemName = dataBase->name();
			EffectPackageDatabaseParams* d = static_cast< EffectPackageDatabaseParams*>(mEffectPackageDatabaseParams);
			for (PxU32 i = 0; i < (PxU32)d->EffectPackages.arraySizes[0]; i++)
			{
				NxParameterized::Interface* ei = d->EffectPackages.buf[i];
				EffectPackageData* ed = static_cast< EffectPackageData*>(ei);
				if (physx::string::stricmp(ed->Name.buf, itemName) == 0)
				{
					ed->EffectPackage->copy(*dataBase);
					revised = true;
					ret = true;
					break;
				}
			}
			if (!revised)
			{
				PxU32 arraySize = d->EffectPackages.arraySizes[0];
				NxParameterized::Handle handle(mEffectPackageDatabaseParams);
				NxParameterized::ErrorType err = handle.getParameter("EffectPackages");
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				err = handle.resizeArray(arraySize + 1);
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				if (err == NxParameterized::ERROR_NONE)
				{
					NxParameterized::Interface* ei = traits->createNxParameterized(EffectPackageData::staticClassName());
					EffectPackageData* ed = static_cast< EffectPackageData*>(ei);
					if (ed)
					{
						NxParameterized::Handle item(ei);
						err = item.getParameter("Name");
						PX_ASSERT(err == NxParameterized::ERROR_NONE);
						item.setParamString(itemName);
						ei->setName(itemName);
						d->EffectPackages.buf[arraySize] = ei;
						dataBase->clone(ed->EffectPackage);
					}
					ret = true;
				}
			}
		}
		else if (strcmp(className, EffectPackageDatabaseParams::staticClassName()) == 0)
		{
			if (mEffectPackageDatabaseParams && mEffectPackageDatabaseParams != dataBase)
			{
				mEffectPackageDatabaseParams->destroy();
				mEffectPackageDatabaseParams = NULL;
			}
			if (mEffectPackageDatabaseParams == NULL)
			{
				dataBase->clone(mEffectPackageDatabaseParams);
			}
			EffectPackageDatabaseParams* d = static_cast< EffectPackageDatabaseParams*>(mEffectPackageDatabaseParams);

			{
				if (d->EffectPackages.arraySizes[0] == 0)
				{
					NxParameterized::Handle handle(mEffectPackageDatabaseParams);
					NxParameterized::ErrorType err = handle.getParameter("EffectPackages");
					PX_ASSERT(err == NxParameterized::ERROR_NONE);
					err = handle.resizeArray(1);
					PX_ASSERT(err == NxParameterized::ERROR_NONE);
					if (err == NxParameterized::ERROR_NONE)
					{
						NxParameterized::Interface* ei = traits->createNxParameterized(EffectPackageData::staticClassName());
						EffectPackageData* ed = static_cast< EffectPackageData*>(ei);
						d->EffectPackages.buf[0] = ei;
						if (ed)
						{
							ed->EffectPackage = traits->createNxParameterized(EffectPackageAssetParams::staticClassName());
							EffectPackageAssetParams* p = static_cast< EffectPackageAssetParams*>(ed->EffectPackage);
							if (p->Effects.arraySizes[0] == 0 && p)
							{
								NxParameterized::Handle handle(p);
								NxParameterized::ErrorType err = handle.getParameter("Effects");
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								err = handle.resizeArray(1);
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								if (err == NxParameterized::ERROR_NONE)
								{
									NxParameterized::Interface* ei = traits->createNxParameterized(EmitterEffect::staticClassName());
									p->Effects.buf[0] = ei;
									ret = true;
								}
							}
						}
						ret = true;
					}
				}
				Array< const char* > nameList;
				for (PxI32 i = 0; i < d->EffectPackages.arraySizes[0]; i++)
				{
					EffectPackageData* e = static_cast< EffectPackageData*>(d->EffectPackages.buf[i]);
					if (e)
					{
						for (PxU32 j = 0; j < nameList.size(); j++)
						{
							if (physx::string::stricmp(nameList[j], e->Name) == 0)
							{
								NxParameterized::Handle handle(e);
								NxParameterized::ErrorType err = handle.getParameter("Name");
								PX_ASSERT(err == NxParameterized::ERROR_NONE);
								if (err == NxParameterized::ERROR_NONE)
								{
									handle.setParamString(getUniqueName(nameList, e->Name));
									ret = true;
									break;
								}
							}
						}
						nameList.pushBack(e->Name);
					}
				}

				for (PxU32 i = 0; i < (PxU32)d->EffectPackages.arraySizes[0]; i++)
				{
					NxParameterized::Interface* ei = d->EffectPackages.buf[i];
					EffectPackageData* d = static_cast< EffectPackageData*>(ei);

					EffectPackageAssetParams* p = static_cast< EffectPackageAssetParams*>(d->EffectPackage);
					if (p == NULL)
					{
						NxParameterized::Interface* ep = traits->createNxParameterized(EffectPackageAssetParams::staticClassName());
						p = static_cast< EffectPackageAssetParams*>(ep);
						d->EffectPackage = ep;
					}


					for (PxU32 i = 0; i < (PxU32)p->Effects.arraySizes[0]; i++)
					{
						NxParameterized::Interface* ei = p->Effects.buf[i];
						if (!ei)
						{
							continue;
						}
						if (strcmp(ei->className(), EmitterEffect::staticClassName()) == 0)
						{
							EmitterEffect* e = static_cast< EmitterEffect*>(ei);
							if (e->Emitter == NULL)
							{
								setNamedReference(ei, "Emitter", NX_APEX_EMITTER_AUTHORING_TYPE_NAME);
								PX_ASSERT(e->Emitter);
								e->Emitter->setName("defaultEmitter");
								ret = true;
							}
						}
						else if (strcmp(ei->className(), HeatSourceEffect::staticClassName()) == 0)
						{
							HeatSourceEffect* e = static_cast< HeatSourceEffect*>(ei);
							if (e->HeatSource == NULL)
							{
								setNamedReference(ei, "HeatSource", NX_HEAT_SOURCE_AUTHORING_TYPE_NAME);
								PX_ASSERT(e->HeatSource);
								e->HeatSource->setName("defaultHeatSource");
								ret = true;
							}
						}
						else if (strcmp(ei->className(), SubstanceSourceEffect::staticClassName()) == 0)
						{
							SubstanceSourceEffect* e = static_cast< SubstanceSourceEffect*>(ei);
							if (e->SubstanceSource == NULL)
							{
								setNamedReference(ei, "SubstanceSource", NX_SUBSTANCE_SOURCE_AUTHORING_TYPE_NAME);
								PX_ASSERT(e->SubstanceSource);
								e->SubstanceSource->setName("defaultSubstanceSource");
								ret = true;
							}
						}
						else if (strcmp(ei->className(), JetFieldSamplerEffect::staticClassName()) == 0)
						{
							JetFieldSamplerEffect* e = static_cast< JetFieldSamplerEffect*>(ei);
							if (e->JetFieldSampler == NULL)
							{
								setNamedReference(ei, "JetFieldSampler", NX_JET_FS_AUTHORING_TYPE_NAME);
								PX_ASSERT(e->JetFieldSampler);
								e->JetFieldSampler->setName("defaultJetFieldSampler");
								ret = true;
							}
						}
						else if (strcmp(ei->className(), NoiseFieldSamplerEffect::staticClassName()) == 0)
						{
							NoiseFieldSamplerEffect* e = static_cast< NoiseFieldSamplerEffect*>(ei);
							if (e->NoiseFieldSampler == NULL)
							{
								setNamedReference(ei, "NoiseFieldSampler", NX_NOISE_FS_AUTHORING_TYPE_NAME);
								PX_ASSERT(e->NoiseFieldSampler);
								e->NoiseFieldSampler->setName("defaultNoiseFieldSampler");
								ret = true;
							}
						}
						else if (strcmp(ei->className(), VortexFieldSamplerEffect::staticClassName()) == 0)
						{
							VortexFieldSamplerEffect* e = static_cast< VortexFieldSamplerEffect*>(ei);
							if (e->VortexFieldSampler == NULL)
							{
								setNamedReference(ei, "VortexFieldSampler", NX_VORTEX_FS_AUTHORING_TYPE_NAME);
								PX_ASSERT(e->VortexFieldSampler);
								e->VortexFieldSampler->setName("defaultVortexFieldSampler");
								ret = true;
							}
						}
						else if (strcmp(ei->className(), AttractorFieldSamplerEffect::staticClassName()) == 0)
						{
							AttractorFieldSamplerEffect* e = static_cast< AttractorFieldSamplerEffect*>(ei);
							if (e->AttractorFieldSampler == NULL)
							{
								setNamedReference(ei, "AttractorFieldSampler", NX_ATTRACTOR_FS_AUTHORING_TYPE_NAME);
								PX_ASSERT(e->AttractorFieldSampler);
								e->AttractorFieldSampler->setName("defaultAttractorFieldSampler");
								ret = true;
							}
						}
						else if (strcmp(ei->className(), TurbulenceFieldSamplerEffect::staticClassName()) == 0)
						{
							TurbulenceFieldSamplerEffect* e = static_cast< TurbulenceFieldSamplerEffect*>(ei);
							if (e->TurbulenceFieldSampler == NULL)
							{
								setNamedReference(ei, "TurbulenceFieldSampler", NX_TURBULENCE_FS_AUTHORING_TYPE_NAME);
								PX_ASSERT(e->TurbulenceFieldSampler);
								e->TurbulenceFieldSampler->setName("defaultTurbulenceFieldSampler");
								ret = true;
							}
						}
						else if (strcmp(ei->className(), ForceFieldEffect::staticClassName()) == 0)
						{
							ForceFieldEffect* e = static_cast< ForceFieldEffect*>(ei);
							if (e->ForceField == NULL)
							{
								setNamedReference(ei, "ForceField", NX_FORCEFIELD_AUTHORING_TYPE_NAME);
								PX_ASSERT(e->ForceField);
								e->ForceField->setName("defaultForceField");
								ret = true;
							}
						}

					}
				}
			}
		}
		else
		{
			PX_ALWAYS_ASSERT(); // invalid NxParameterized::Interface passed in
		}
	}


	return ret;
}


bool ModuleParticles::fixupNamedReferences(void)
{
	bool ret = false;

	if (mEffectPackageEmitterDatabaseParams)
	{
		EffectPackageEmitterDatabaseParams* d = static_cast< EffectPackageEmitterDatabaseParams*>(mEffectPackageEmitterDatabaseParams);

		for (PxI32 i = 0; i < d->Emitters.arraySizes[0]; i++)
		{
			EmitterData* ed = static_cast< EmitterData*>(d->Emitters.buf[i]);

			if (ed->Emitter)
			{
				NxParameterized::Handle handle(ed->Emitter);
				NxParameterized::ErrorType err = ed->Emitter->getParameterHandle("iosAssetName", handle);
				PX_ASSERT(err == NxParameterized::ERROR_NONE);
				if (err == NxParameterized::ERROR_NONE)
				{
					NxParameterized::Interface* val = NULL;
					err = handle.getParamRef(val);
					// ok..need to see if it has changed!
					if (val)
					{
						const char* refName = val->name();
						EffectPackageIOSDatabaseParams* d = static_cast< EffectPackageIOSDatabaseParams*>(mEffectPackageIOSDatabaseParams);
						for (PxU32 i = 0; i < (PxU32)d->ParticleSimulations.arraySizes[0]; i++)
						{
							NxParameterized::Interface* ei = d->ParticleSimulations.buf[i];
							apex::emitter::ApexEmitterAssetParameters* ae = static_cast< apex::emitter::ApexEmitterAssetParameters*>(ed->Emitter);
							ParticleSimulationData* ed = static_cast< ParticleSimulationData*>(ei);
							if (physx::string::stricmp(ed->Name.buf, refName) == 0)
							{
								NxParameterized::Interface* ios = ed->IOS;
								if ( ios )
								{
									const char* paramRef = getAuthoringTypeName(ios->className());
									err = handle.initParamRef(paramRef,true);
									PX_ASSERT(err == NxParameterized::ERROR_NONE);
									if (err == NxParameterized::ERROR_NONE)
									{
										ae->iosAssetName->setName(ed->Name.buf);
									}
								}
								break;
							}
						}
					}
				}
			}
		}
	}
	return ret;
}

bool ModuleParticles::initParticleSimulationData(ParticleSimulationData* ed)
{
	bool ret = false;

	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (ed->IOS == NULL)
	{
		ed->IOS = traits->createNxParameterized(apex::basicios::BasicIOSAssetParam::staticClassName());
		apex::basicios::BasicIOSAssetParam* ios = static_cast< apex::basicios::BasicIOSAssetParam*>(ed->IOS);
		if (ios)
		{
			ios->restDensity = 1;
			ios->particleRadius = 0.1f;
			ios->maxParticleCount = 16384;
			ios->maxInjectedParticleCount = 0.05;
			ios->sceneGravityScale = 1;
			ios->externalAcceleration.x = 0;
			ios->externalAcceleration.y = 0;
			ios->externalAcceleration.z = 0;
			ios->particleMass.center = 1;
			ios->particleMass.spread = 0;
			ios->staticCollision = true;
			ios->restitutionForStaticShapes = 0.2f;
			ios->dynamicCollision = true;
			ios->restitutionForDynamicShapes = 0.2f;
			ios->collisionDistanceMultiplier = 1;
			ios->collisionThreshold = 0.001f;
			ios->collisionWithConvex = true;
			ios->collisionWithTriangleMesh = false;
		}
		ret = true;
	}

	if (ed->IOS && strcmp(ed->IOS->className(), "ParticleIosAssetParam") == 0)
	{
		apex::pxparticleios::ParticleIosAssetParam* p = static_cast< apex::pxparticleios::ParticleIosAssetParam*>(ed->IOS);
		if (p->particleType == NULL)
		{
			p->particleType = traits->createNxParameterized(apex::pxparticleios::FluidParticleSystemParams::staticClassName());
			ret = true;
		}
	}

	if ( ed->IOS )
	{
		if ( strcmp(ed->IOS->className(), "ParticleIosAssetParam") == 0 )
		{
			const char *filterName = "ParticlesSimple=all";
			apex::pxparticleios::ParticleIosAssetParam* p = static_cast< apex::pxparticleios::ParticleIosAssetParam*>(ed->IOS);
			if (p->particleType && strcmp(p->particleType->className(), apex::pxparticleios::FluidParticleSystemParams::staticClassName()) == 0 )
			{
				filterName = "ParticlesFluid=all";
			}

			{
				const char *value=NULL;
				NxParameterized::getParamString(*ed->IOS,"simulationFilterData",value);
				if ( value == NULL || value[0] == 0 )
				{
					NxParameterized::setParamString(*ed->IOS,"simulationFilterData",filterName);
					ret = true;
				}
			}
			{
				const char *value=NULL;
				NxParameterized::getParamString(*ed->IOS,"fieldSamplerFilterData",value);
				if ( value == NULL || value[0] == 0 )
				{
					NxParameterized::setParamString(*ed->IOS,"fieldSamplerFilterData",filterName);
					ret = true;
				}
			}
		}
		else
		{
			const char *filterName = "ParticlesBasicIOS=all";
			{
				const char *value=NULL;
				NxParameterized::getParamString(*ed->IOS,"collisionFilterDataName",value);
				if ( value == NULL || value[0] == 0 )
				{
					NxParameterized::setParamString(*ed->IOS,"collisionFilterDataName",filterName);
					ret = true;
				}
			}
			{
				const char *value=NULL;
				NxParameterized::getParamString(*ed->IOS,"fieldSamplerFilterDataName",value);
				if ( value == NULL || value[0] == 0 )
				{
					NxParameterized::setParamString(*ed->IOS,"fieldSamplerFilterDataName",filterName);
					ret = true;
				}
			}

		}
	}
	return ret;
}

void ModuleParticles::resetEmitterPool(void)
{
	for (PxU32 i = 0; i < mScenes.size(); i++)
	{
		ParticlesScene* ds = static_cast< ParticlesScene*>(mScenes[i]);
		ds->resetEmitterPool();
	}
}

#define MODULE_CHECK(x) if ( x == module ) { x = NULL; }

/**
Notification from ApexSDK when a module has been released
*/
void ModuleParticles::notifyChildGone(NiModule* imodule)
{
	NxModule* module = mSdk->getNxModule(imodule);
	PX_ASSERT(module);
	MODULE_CHECK(mModuleBasicIos);
	MODULE_CHECK(mModuleEmitter);
	MODULE_CHECK(mModuleIofx);
	MODULE_CHECK(mModuleFieldSampler);
	MODULE_CHECK(mModuleBasicFS);
#	if NX_SDK_VERSION_MAJOR == 2
	MODULE_CHECK(mModuleFieldBoundary);
	MODULE_CHECK(mModuleExplosion);
	MODULE_CHECK(mModuleFluidIos);
#	elif NX_SDK_VERSION_MAJOR == 3
	MODULE_CHECK(mModuleParticleIos);			// PhysX 3.x only : Instantiate the ParticleIOS module
	MODULE_CHECK(mModuleForceField);			// PhysX 3.x only : Instantiate the ForceField module
#	endif
};

// This is a notification that the ApexSDK is being released.  During the shutdown process
// the APEX SDK will automatically release all currently registered modules; therefore we are no longer
// responsible for releasing these modules ourselves.

#define SAFE_MODULE_NULL(x) if ( x ) { NiModule *m = mSdk->getNiModule(x); PX_ASSERT(m); m->setParent(NULL); x = NULL; }

void ModuleParticles::notifyReleaseSDK(void)
{
	SAFE_MODULE_NULL(mModuleBasicIos);
	SAFE_MODULE_NULL(mModuleEmitter);
	SAFE_MODULE_NULL(mModuleIofx);
	SAFE_MODULE_NULL(mModuleFieldSampler);
	SAFE_MODULE_NULL(mModuleBasicFS);
#	if NX_SDK_VERSION_MAJOR == 2
	SAFE_MODULE_NULL(mModuleFieldBoundary);
	SAFE_MODULE_NULL(mModuleExplosion);
	SAFE_MODULE_NULL(mModuleFluidIos);
#	elif NX_SDK_VERSION_MAJOR == 3
	SAFE_MODULE_NULL(mModuleParticleIos);			// PhysX 3.x only : Instantiate the ParticleIOS module
	SAFE_MODULE_NULL(mModuleForceField);			// PhysX 3.x only : Instantiate the ForceField module
#	endif
}

void ModuleParticles::initializeDefaultDatabases(void)
{
	/* Register the NxParameterized factories */
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (mTurbulenceModule == NULL)
	{
		PxU32 count = mSdk->getNbModules();
		NxModule** modules = mSdk->getModules();
		for (PxU32 i = 0; i < count; i++)
		{
			NxModule* m = modules[i];
			const char* name = m->getName();
			if (strcmp(name, "TurbulenceFS") == 0)
			{
				mTurbulenceModule = static_cast< NxModuleTurbulenceFS*>(m);
				break;
			}
		}
	}

	// Initialize the effect package databases
	if (mEffectPackageIOSDatabaseParams == NULL)
	{
		NxParameterized::Interface* iface = traits->createNxParameterized(EffectPackageIOSDatabaseParams::staticClassName());
		if (iface)
		{
			setEffectPackageIOSDatabase(iface);
			iface->destroy();
		}
	}
	if (mEffectPackageIOFXDatabaseParams == NULL)
	{
		NxParameterized::Interface* iface = traits->createNxParameterized(EffectPackageIOFXDatabaseParams::staticClassName());
		if (iface)
		{
			setEffectPackageIOFXDatabase(iface);
			iface->destroy();
		}
	}
	if (mEffectPackageEmitterDatabaseParams == NULL)
	{
		NxParameterized::Interface* iface = traits->createNxParameterized(EffectPackageEmitterDatabaseParams::staticClassName());
		if (iface)
		{
			setEffectPackageEmitterDatabase(iface);
			iface->destroy();
		}
	}
#if NX_SDK_VERSION_MAJOR == 3
	if (mEffectPackageFieldSamplerDatabaseParams == NULL)
	{
		NxParameterized::Interface* iface = traits->createNxParameterized(EffectPackageFieldSamplerDatabaseParams::staticClassName());
		setEffectPackageFieldSamplerDatabase(iface);
		iface->destroy();
	}
#endif
	if (mEffectPackageDatabaseParams == NULL)
	{
		NxParameterized::Interface* iface = traits->createNxParameterized(EffectPackageDatabaseParams::staticClassName());
		if (iface)
		{
			setEffectPackageDatabase(iface);
			iface->destroy();
		}
	}
	if (mGraphicsMaterialsDatabase == NULL)
	{
		NxParameterized::Interface* iface = traits->createNxParameterized(EffectPackageGraphicsMaterialsParams::staticClassName());
		if (iface)
		{
			setEffectPackageGraphicsMaterialsDatabase(iface);
			iface->destroy();
		}
	}

}

#define MODULE_NAME_CHECK(x) if ( x ) { if ( strcmp(x->getName(),moduleName) == 0) ret = x; }

physx::apex::NxModule* ModuleParticles::getModule(const char* moduleName)
{
	physx::apex::NxModule* ret = NULL;
	MODULE_NAME_CHECK(mModuleBasicIos);
	MODULE_NAME_CHECK(mModuleEmitter);
	MODULE_NAME_CHECK(mModuleIofx);
	MODULE_NAME_CHECK(mModuleFieldSampler);
	MODULE_NAME_CHECK(mModuleBasicFS);
#	if NX_SDK_VERSION_MAJOR == 2
	MODULE_NAME_CHECK(mModuleFieldBoundary);
	MODULE_NAME_CHECK(mModuleExplosion);
	MODULE_NAME_CHECK(mModuleFluidIos);
#	elif NX_SDK_VERSION_MAJOR == 3
	MODULE_NAME_CHECK(mModuleParticleIos);
	MODULE_NAME_CHECK(mModuleForceField);
#	endif
	return ret;
}

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

}
}
} // end namespace physx::apex

