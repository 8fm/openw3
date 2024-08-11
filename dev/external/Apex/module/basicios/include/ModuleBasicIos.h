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

#ifndef __MODULE_BASIC_IOS_H__
#define __MODULE_BASIC_IOS_H__

#include "NxApex.h"
#include "NxModuleBasicIos.h"
#include "NiApexSDK.h"
#include "Module.h"
#include "NiModule.h"
#include "NiResourceProvider.h"
#include "ApexSharedUtils.h"
#include "ApexSDKHelpers.h"
#include "ModulePerfScope.h"
#include "ApexAuthorableObject.h"
#include "BasicIosAsset.h"
#include "BasiciosParamClasses.h"

namespace physx
{
namespace apex
{

class NiModuleIofx;
class NiModuleFieldSampler;

namespace basicios
{

class BasicIosScene;

/**
\brief Module descriptor for BasicIOS module
*/
class NxModuleBasicIosDesc : public NxApexDesc
{
public:

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE NxModuleBasicIosDesc() : NxApexDesc()
	{
		init();
	}

	/**
	\brief sets members to default values.
	*/
	PX_INLINE void setToDefault()
	{
		NxApexDesc::setToDefault();
		init();
	}

	/**
	\brief checks if this is a valid descriptor.
	*/
	PX_INLINE bool isValid() const
	{
		bool retVal = NxApexDesc::isValid();
		return retVal;
	}

private:

	PX_INLINE void init()
	{
	}
};

class ModuleBasicIos : public NxModuleBasicIos, public NiModule, public Module
{
public:
	ModuleBasicIos(NiApexSDK* sdk);
	~ModuleBasicIos();

	void											init(const NxModuleBasicIosDesc& desc);

	// base class methods
	void											init(NxParameterized::Interface&);
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

	//NxBasicIosActor *								getApexActor( NxApexScene* scene, NxAuthObjTypeID type ) const;
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

	virtual const char*                             getBasicIosTypeName();

	BasicIosScene* 									getBasicIosScene(const NxApexScene& scene);
	const BasicIosScene* 							getBasicIosScene(const NxApexScene& scene) const;

	NiModuleIofx* 									getNiModuleIofx();
	NiModuleFieldSampler* 							getNiModuleFieldSampler();

#ifdef APEX_TEST
	NxBasicIosActor*								getActor(NxApexScene* apexScene) const;
#endif
protected:

	NxResourceList								mBasicIosSceneList;
	NxResourceList								mAuthorableObjects;

	friend class BasicIosScene;
private:

#	define PARAM_CLASS(clas) PARAM_CLASS_DECLARE_FACTORY(clas)
#	include "BasiciosParamClasses.inc"

	BasicIosModuleParameters*					mModuleParams;

	NiModuleIofx*                               mIofxModule;
	NiModuleFieldSampler*                       mFieldSamplerModule;
};

}
}
} // namespace physx::apex

#endif // __MODULE_BASIC_IOS_H__
