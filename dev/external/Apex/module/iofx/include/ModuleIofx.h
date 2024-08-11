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

#ifndef __MODULE_IOFX_H__
#define __MODULE_IOFX_H__

#include "NxApex.h"
#include "NxModuleIofx.h"
#include "NiApexSDK.h"
#include "Module.h"
#include "NiModuleIofx.h"
#include "NiResourceProvider.h"
#include "ApexSharedUtils.h"
#include "ApexSDKHelpers.h"
#include "ModulePerfScope.h"
#include "ApexAuthorableObject.h"

#include "IofxParamClasses.h"

namespace physx
{
namespace apex
{

class NxApexRenderVolume;

namespace iofx
{
class IofxAsset;
class IofxScene;

class NxModuleIofxDesc : public NxApexDesc
{
public:

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE NxModuleIofxDesc() : NxApexDesc()
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

class ModuleIofx : public NxModuleIofx, public NiModuleIofx, public Module
{
public:
	ModuleIofx(NiApexSDK* sdk);
	~ModuleIofx();

	void							init(const NxModuleIofxDesc& ModuleIofxDesc);

	// base class methods
	void							init(NxParameterized::Interface&) {}
	NxParameterized::Interface* 	getDefaultModuleDesc();
	void							release()
	{
		Module::release();
	}
	void							destroy();
	const char*						getName() const
	{
		return Module::getName();
	}
	physx::PxU32					getNbParameters() const
	{
		return Module::getNbParameters();
	}
	NxApexParameter**				getParameters()
	{
		return Module::getParameters();
	}
	void							setLODUnitCost(physx::PxF32 cost)
	{
		Module::setLODUnitCost(cost);
	}
	physx::PxF32					getLODUnitCost() const
	{
		return Module::getLODUnitCost();
	}
	void							setLODBenefitValue(physx::PxF32 value)
	{
		Module::setLODBenefitValue(value);
	}
	physx::PxF32					getLODBenefitValue() const
	{
		return Module::getLODBenefitValue();
	}
	void							setLODEnabled(bool enabled)
	{
		Module::setLODEnabled(enabled);
	}
	bool							getLODEnabled() const
	{
		return Module::getLODEnabled();
	}

	NxApexRenderableIterator*		createRenderableIterator(const NxApexScene&);

	void							disableCudaInterop()
	{
		mInteropDisabled = true;
	}
	void							disableCudaModifiers()
	{
		mCudaDisabled = true;
	}
	void							disableDeferredRenderableAllocation()
	{
		mDeferredDisabled = true;
	}

	// NiModuleIofx methods
	NiIofxManager*					createActorManager(const NxApexScene& scene, const NxIofxAsset& asset, const NiIofxManagerDesc& desc);

	void setIntValue(physx::PxU32 parameterIndex, physx::PxU32 value)
	{
		return Module::setIntValue(parameterIndex, value);
	}
	physx::PxU32					forceLoadAssets();
	NxAuthObjTypeID					getModuleID() const;

	NiModuleScene* 					createNiModuleScene(NiApexScene&, NiApexRenderDebug*);
	void							releaseNiModuleScene(NiModuleScene&);

	IofxScene* 						getIofxScene(const NxApexScene& scene);
	const IofxScene* 				getIofxScene(const NxApexScene& scene) const;

	NxApexRenderVolume* 			createRenderVolume(const NxApexScene& apexScene, const PxBounds3& b, PxU32 priority, bool allIofx);
	void							releaseRenderVolume(NxApexRenderVolume& volume);

protected:

	NxResourceList								mAuthorableObjects;

#	define PARAM_CLASS(clas) PARAM_CLASS_DECLARE_FACTORY(clas)
#	include "IofxParamClasses.inc"

	IofxModuleParameters* 						mModuleParams;
	bool										mInteropDisabled;
	bool										mCudaDisabled;
	bool										mDeferredDisabled;

	NxResourceList								mIofxScenes;
	friend class IofxActor;
	friend class IofxScene;
	friend class IofxManager;
	friend class IofxManagerGPU;
};

}
}
} // namespace apex

#endif // __MODULE_PARTICLES_H__
