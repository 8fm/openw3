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

#ifndef __MODULE_PARTICLES_H__
#define __MODULE_PARTICLES_H__

#include "NxApex.h"
#include "NxModuleParticles.h"
#include "NiApexSDK.h"
#include "NiModule.h"
#include "Module.h"

#include "ApexInterface.h"
#include "ApexSDKHelpers.h"
#include "ParticlesDebugRenderParams.h"
#include "ParticlesModuleParameters.h"
#include "EffectPackageGraphicsMaterialsParams.h"
#include "EffectPackageAssetParams.h"
#include "EffectPackageActorParams.h"
#include "GraphicsMaterialData.h"
#include "VolumeRenderMaterialData.h"
#include "EmitterEffect.h"
#include "HeatSourceEffect.h"
#include "SubstanceSourceEffect.h"
#include "ForceFieldEffect.h"
#include "JetFieldSamplerEffect.h"
#include "NoiseFieldSamplerEffect.h"
#include "VortexFieldSamplerEffect.h"
#include "AttractorFieldSamplerEffect.h"
#include "TurbulenceFieldSamplerEffect.h"

#include "EffectPackageData.h"
#include "AttractorFieldSamplerData.h"
#include "JetFieldSamplerData.h"
#include "NoiseFieldSamplerData.h"
#include "VortexFieldSamplerData.h"
#include "TurbulenceFieldSamplerData.h"
#include "HeatSourceData.h"
#include "SubstanceSourceData.h"
#include "ForceFieldData.h"
#include "EmitterData.h"
#include "GraphicsEffectData.h"
#include "ParticleSimulationData.h"

#include "EffectPackageIOSDatabaseParams.h"
#include "EffectPackageIOFXDatabaseParams.h"
#include "EffectPackageEmitterDatabaseParams.h"
#include "EffectPackageDatabaseParams.h"
#include "EffectPackageFieldSamplerDatabaseParams.h"

namespace physx
{
namespace apex
{

class NiApexScene;
class NxModuleTurbulenceFS;
class NxApexEmitterActor;
class NxApexEmitterAsset;

namespace particles
{
class ParticlesAsset;
class ParticlesAssetAuthoring;
class ParticlesScene;

typedef Array< NiModuleScene* > ModuleSceneVector;

class NxModuleParticlesDesc : public NxApexDesc
{
public:

	/**
	\brief Constructor sets to default.
	*/
	PX_INLINE NxModuleParticlesDesc()
	{
		setToDefault();
	}
	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void	setToDefault()
	{
		NxApexDesc::setToDefault();
		moduleValue = 0;
	}

	/**
	Returns true if an object can be created using this descriptor.
	*/
	PX_INLINE bool	isValid() const
	{
		return NxApexDesc::isValid();
	}

	/**
	Module configurable parameter.
	*/
	physx::PxU32 moduleValue;
};


class ModuleParticles : public NxModuleParticles, public NiModule, public Module
{
public:
	ModuleParticles(NiApexSDK* sdk);
	~ModuleParticles();

	void						init(const NxModuleParticlesDesc& desc);

	// base class methods
	void						init(NxParameterized::Interface&) {}
	NxParameterized::Interface* getDefaultModuleDesc();
	void release()
	{
		Module::release();
	}
	void destroy();
	const char*					getName() const
	{
		return Module::getName();
	}
	physx::PxU32				getNbParameters() const
	{
		return Module::getNbParameters();
	}
	NxApexParameter**			getParameters()
	{
		return Module::getParameters();
	}

	/**
	\brief Get the cost of one LOD aspect unit.
	*/
	virtual physx::PxF32 getLODUnitCost() const
	{
		return 0;
	}

	/**
	\brief Set the cost of one LOD aspect unit.
	*/
	virtual void setLODUnitCost(physx::PxF32)
	{

	}

	/**
	\brief Get the resource value of one unit of benefit.
	*/
	virtual physx::PxF32 getLODBenefitValue() const
	{
		return 0;
	}

	/**
	\brief Set the resource value of one unit of benefit.
	*/
	virtual void setLODBenefitValue(physx::PxF32)
	{

	}

	/**
	\brief Get enabled/disabled state of automatic LOD system.
	*/
	virtual bool getLODEnabled() const
	{
		return false;
	}

	/**
	\brief Set enabled/disabled state of automatic LOD system.
	*/
	virtual void setLODEnabled(bool)
	{

	}


	void						setIntValue(physx::PxU32 parameterIndex, physx::PxU32 value)
	{
		return Module::setIntValue(parameterIndex, value);
	}

	virtual bool setEffectPackageGraphicsMaterialsDatabase(const NxParameterized::Interface* dataBase);

	virtual const NxParameterized::Interface* getEffectPackageGraphicsMaterialsDatabase() const;

	virtual bool setEffectPackageIOSDatabase(const NxParameterized::Interface* dataBase);
	virtual bool setEffectPackageIOFXDatabase(const NxParameterized::Interface* dataBase);
	virtual bool setEffectPackageEmitterDatabase(const NxParameterized::Interface* dataBase);
	virtual bool setEffectPackageDatabase(const NxParameterized::Interface* dataBase);
	virtual bool setEffectPackageFieldSamplerDatabase(const NxParameterized::Interface* dataBase);

	virtual const NxParameterized::Interface* getEffectPackageIOSDatabase(void) const
	{
		return mEffectPackageIOSDatabaseParams;
	};
	virtual const NxParameterized::Interface* getEffectPackageIOFXDatabase(void) const
	{
		return mEffectPackageIOFXDatabaseParams;
	};
	virtual const NxParameterized::Interface* getEffectPackageEmitterDatabase(void) const
	{
		return mEffectPackageEmitterDatabaseParams;
	};
	virtual const NxParameterized::Interface* getEffectPackageDatabase(void) const
	{
		return mEffectPackageDatabaseParams;
	};
	virtual const NxParameterized::Interface* getEffectPackageFieldSamplerDatabase(void) const
	{
		return mEffectPackageFieldSamplerDatabaseParams;
	};

	bool initParticleSimulationData(ParticleSimulationData* ed);

	virtual NxParameterized::Interface* locateResource(const char* resourceName,		// the name of the resource
	        const char* nameSpace);

	virtual const char** getResourceNames(const char* nameSpace, physx::PxU32& nameCount, const char** &variants);

	virtual const NxParameterized::Interface* locateGraphicsMaterialData(const char* name) const;
	virtual const NxParameterized::Interface* locateVolumeRenderMaterialData(const char* name) const;


	NiModuleScene* 				createNiModuleScene(NiApexScene&, NiApexRenderDebug*);
	void						releaseNiModuleScene(NiModuleScene&);
	physx::PxU32				forceLoadAssets();
	NxAuthObjTypeID				getModuleID() const;
	NxApexRenderableIterator* 	createRenderableIterator(const NxApexScene&);

	NxAuthObjTypeID             getParticlesAssetTypeID() const;

	physx::PxU32				getModuleValue() const
	{
		return mModuleValue;
	}

	NxModuleTurbulenceFS* getModuleTurbulenceFS(void)
	{
		return mTurbulenceModule;
	}

	ParticlesScene* 			getParticlesScene(const NxApexScene& apexScene);

	virtual void setEnableScreenCulling(bool state, bool znegative)
	{
		mEnableScreenCulling = state;
		mZnegative = znegative;
	}

	bool getEnableScreenCulling(void) const
	{
		return mEnableScreenCulling;
	}

	bool getZnegative(void) const
	{
		return mZnegative;
	}

	virtual void resetEmitterPool(void);

	virtual void setUseEmitterPool(bool state)
	{
		mUseEmitterPool = state;
	}

	virtual bool getUseEmitterPool(void) const
	{
		return mUseEmitterPool;
	}

	virtual void notifyReleaseSDK(void);

	virtual void notifyChildGone(NiModule* imodule);

protected:
	bool						mUseEmitterPool;
	bool						mEnableScreenCulling;
	bool						mZnegative;

	NxResourceList				mAuthorableObjects;
	NxResourceList				mEffectPackageAuthorableObjects;

	NxResourceList				mParticlesScenes;

	physx::PxU32				mModuleValue;

	friend class ParticlesAsset;

	/**
	\brief Used by the ParticleEffectTool to initialize the default database values for the editor
	*/
	virtual void initializeDefaultDatabases(void);

	virtual physx::apex::NxModule* getModule(const char* moduleName);

private:

	bool fixFieldSamplerCollisionFilterNames(NxParameterized::Interface *fs);

	bool fixupNamedReferences(void);

	ParticlesDebugRenderParamsFactory							mParticlesDebugRenderParamsFactory;
	ParticlesModuleParametersFactory							mParticlesModuleParametersFactory;
	EffectPackageGraphicsMaterialsParamsFactory					mEffectPackageGraphicsMaterialsParamsFactory;

	// factory definitions in support of effect packages
	EffectPackageAssetParamsFactory								mEffectPackageAssetParamsFactory;
	EffectPackageActorParamsFactory								mEffectPackageActorParamsFactory;
	EmitterEffectFactory										mEmitterEffectFactory;
	HeatSourceEffectFactory										mHeatSourceEffectFactory;
	SubstanceSourceEffectFactory								mSubstanceSourceEffectFactory;
	ForceFieldEffectFactory										mForceFieldEffectFactory;
	JetFieldSamplerEffectFactory								mJetFieldSamplerEffectFactory;
	NoiseFieldSamplerEffectFactory								mNoiseFieldSamplerEffectFactory;
	VortexFieldSamplerEffectFactory								mVortexFieldSamplerEffectFactory;
	AttractorFieldSamplerEffectFactory							mAttractorFieldSamplerEffectFactory;
	TurbulenceFieldSamplerEffectFactory							mTurbulenceFieldSamplerEffectFactory;
	EffectPackageIOSDatabaseParamsFactory						mEffectPackageIOSDatabaseParamsFactory;
	EffectPackageIOFXDatabaseParamsFactory						mEffectPackageIOFXDatabaseParamsFactory;
	EffectPackageEmitterDatabaseParamsFactory					mEffectPackageEmitterDatabaseParamsFactory;
	EffectPackageDatabaseParamsFactory							mEffectPackageDatabaseParamsFactory;
	EffectPackageFieldSamplerDatabaseParamsFactory				mEffectPackageFieldSamplerDatabaseParamsFactory;

	EffectPackageDataFactory									mEffectPackageDataFactory;
	AttractorFieldSamplerDataFactory							mAttractorFieldSamplerDataFactory;
	NoiseFieldSamplerDataFactory								mNoiseFieldSamplerDataFactory;
	VortexFieldSamplerDataFactory								mVortexFieldSamplerDataFactory;
	JetFieldSamplerDataFactory									mJetFieldSamplerDataFactory;
	TurbulenceFieldSamplerDataFactory							mTurbulenceFieldSamplerDataFactory;
	HeatSourceDataFactory										mHeatSourceDataFactory;
	SubstanceSourceDataFactory									mSubstanceSourceDataFactory;
	ForceFieldDataFactory										mForceFieldDataFactory;
	EmitterDataFactory											mEmitterDataFactory;
	GraphicsEffectDataFactory									mGraphicsEffectDataFactory;
	ParticleSimulationDataFactory								mParticleSimulationDataFactory;
	GraphicsMaterialDataFactory									mGraphicsMaterialDataFactory;
	VolumeRenderMaterialDataFactory								mVolumeRenderMaterialDataFactory;

	NxParameterized::Interface*									mEffectPackageIOSDatabaseParams;
	NxParameterized::Interface*									mEffectPackageIOFXDatabaseParams;
	NxParameterized::Interface*									mEffectPackageEmitterDatabaseParams;
	NxParameterized::Interface*									mEffectPackageDatabaseParams;
	NxParameterized::Interface*									mEffectPackageFieldSamplerDatabaseParams;

	ParticlesModuleParameters*									mModuleParams;
	NxParameterized::Interface*									mGraphicsMaterialsDatabase;
	NxModuleTurbulenceFS*							mTurbulenceModule;
	ModuleSceneVector											mScenes;
	Array< const char*>										mTempNames;
	Array< const char*>										mTempVariantNames;

	physx::apex::NxModule* 	mModuleBasicIos;			// Instantiate the BasicIOS module statically
	physx::apex::NxModule* 	mModuleEmitter;				// Instantiate the Emitter module statically
	physx::apex::NxModule* 	mModuleIofx;				// Instantiate the IOFX module statically
	physx::apex::NxModule* 	mModuleFieldSampler;		// Instantiate the field sampler module statically
	physx::apex::NxModule* 	mModuleBasicFS;				// Instantiate the BasicFS module statically
#	if NX_SDK_VERSION_MAJOR == 2
	physx::apex::NxModule* 	mModuleFieldBoundary;		// PhysX 2.8 only : Instantiate the FieldBoundary module
	physx::apex::NxModule* 	mModuleExplosion;			// PhysX 2.8 only : Instantiate the explosion module
	physx::apex::NxModule* 	mModuleFluidIos;			// PhysX 2.8 only : Instantiate the fluidIOS module
#	elif NX_SDK_VERSION_MAJOR == 3
	physx::apex::NxModule* 	mModuleParticleIos;			// PhysX 3.x only : Instantiate the ParticleIOS module
	physx::apex::NxModule* 	mModuleForceField;			// PhysX 3.x only : Instantiate the ForceField module
#	endif

};

}
}
} // end namespace physx::apex

#endif // __MODULE_PARTICLES_H__
