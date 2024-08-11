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
#include "NxModuleFluidIos.h"
#include "NiApexSDK.h"
#include "Module.h"
#include "NiModule.h"
#include "NiResourceProvider.h"
#include "ApexSharedUtils.h"
#include "ApexSDKHelpers.h"
#include "ModulePerfScope.h"
#include "ApexAuthorableObject.h"
#include "FluidIosAsset.h"
#include "NxfluidiosParamClasses.h"

class NxCompartment;

namespace physx
{
namespace apex
{

class NiModuleIofx;

namespace nxfluidios
{
class FluidIosScene;

class ModuleFluidIos : public NxModuleFluidIos, public NiModule, public Module
{
public:
	ModuleFluidIos(NiApexSDK* sdk);
	~ModuleFluidIos();

	void											init(const NxModuleFluidIosDesc& moduleParticlesDesc);

	// base class methods
	void											init(NxParameterized::Interface&) {}
	NxParameterized::Interface* 					getDefaultModuleDesc();
	void											release()
	{
		Module::release();
	}
	void											destroy();
	const char*										getName() const
	{
		return Module::getName();
	}
	physx::PxU32									getNbParameters() const
	{
		return Module::getNbParameters();
	}
	NxApexParameter**								getParameters()
	{
		return Module::getParameters();
	}
	void											setLODUnitCost(physx::PxF32 cost)
	{
		Module::setLODUnitCost(cost);
	}
	physx::PxF32									getLODUnitCost() const
	{
		return Module::getLODUnitCost();
	}
	void											setLODBenefitValue(physx::PxF32 value)
	{
		Module::setLODBenefitValue(value);
	}
	physx::PxF32									getLODBenefitValue() const
	{
		return Module::getLODBenefitValue();
	}
	void											setLODEnabled(bool enabled)
	{
		Module::setLODEnabled(enabled);
	}
	bool											getLODEnabled() const
	{
		return Module::getLODEnabled();
	}

	ApexActor* 										getApexActor(NxApexActor* nxactor, NxAuthObjTypeID type) const;


	void setIntValue(physx::PxU32 parameterIndex, physx::PxU32 value)
	{
		return Module::setIntValue(parameterIndex, value);
	}
	NiModuleScene* 									createNiModuleScene(NiApexScene&, NiApexRenderDebug*);
	void											releaseNiModuleScene(NiModuleScene&);
	physx::PxU32									forceLoadAssets();
	NxAuthObjTypeID									getModuleID() const;
	NxApexRenderableIterator* 						createRenderableIterator(const NxApexScene&);

	virtual const char*                             getFluidIosTypeName();

	void											setCompartment(const NxApexScene&, NxCompartment& comp);
	const NxCompartment* 							getCompartment(const NxApexScene&) const;

	void                                            setSPHCompartment(const NxApexScene&, NxCompartment& comp);
	const NxCompartment*                            getSPHCompartment(const NxApexScene&) const;

	FluidIosScene* 									getParticleScene(const NxApexScene& scene);
	const FluidIosScene* 							getParticleScene(const NxApexScene& scene) const;

	NiModuleIofx* 									getNiModuleIofx();

protected:
	NiModuleIofx*                               mIofxModule;

#	define PARAM_CLASS(clas) PARAM_CLASS_DECLARE_FACTORY(clas)
#	include "NxfluidiosParamClasses.inc"

	NxResourceList								mParticleSceneList;
	NxResourceList								mAuthorableObjects;

	FluidIosModuleParameters* 					mModuleParams;

	friend class FluidIosScene;
};

}
}
} // namespace physx::apex

#endif // __MODULE_PARTICLES_H__
