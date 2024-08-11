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

#ifndef __MODULE_FIELDBOUNDARY_H__
#define __MODULE_FIELDBOUNDARY_H__

#include "NxApex.h"
#include "NxModuleFieldBoundary.h"
#include "ShapeBoxParams.h"
#include "ShapeCapsuleParams.h"
#include "ShapeSphereParams.h"
#include "ShapeConvexParams.h"

#include "NiApexSDK.h"
#include "NiModule.h"
#include "Module.h"

#include "ApexInterface.h"
#include "ApexSDKHelpers.h"
#include "ApexAuthorableObject.h"

#include "FieldboundaryParamClasses.h"

namespace physx
{
namespace apex
{

class NiApexScene;
class NiModuleFieldSampler;

namespace fieldboundary
{

class FieldBoundaryAsset;
//class FieldBoundaryAssetAuthoring;
class FieldBoundaryScene;


class NxModuleFieldBoundaryDesc : public NxApexDesc
{
public:

	/**
	\brief Constructor sets to default.
	*/
	PX_INLINE NxModuleFieldBoundaryDesc()
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
	\brief Returns true iff an object can be created using this descriptor.
	*/
	PX_INLINE bool	isValid() const
	{
		return NxApexDesc::isValid();
	}

	/**
	\brief Module configurable parameter.
	*/
	physx::PxU32 moduleValue;
};

class ModuleFieldBoundary : public NxModuleFieldBoundary, public NiModule, public Module
{
public:

	ModuleFieldBoundary(NiApexSDK* sdk);
	~ModuleFieldBoundary();
	PxU32 forceLoadAssets();

	void						init(const NxModuleFieldBoundaryDesc& moduleDesc);

	// base class methods
	void						init(NxParameterized::Interface&) {}
	NxParameterized::Interface* getDefaultModuleDesc();
	void release()
	{
		Module::release();
	}
	void destroy();
	const char* getName() const
	{
		return Module::getName();
	}
	physx::PxU32 getNbParameters() const
	{
		return Module::getNbParameters();
	}
	NxApexParameter** getParameters()
	{
		return Module::getParameters();
	}
	void setLODUnitCost(physx::PxF32 cost)
	{
		Module::setLODUnitCost(cost);
	}
	physx::PxF32 getLODUnitCost() const
	{
		return Module::getLODUnitCost();
	}
	void setLODBenefitValue(physx::PxF32 value)
	{
		Module::setLODBenefitValue(value);
	}
	physx::PxF32 getLODBenefitValue() const
	{
		return Module::getLODBenefitValue();
	}
	void setLODEnabled(bool enabled)
	{
		Module::setLODEnabled(enabled);
	}
	bool getLODEnabled() const
	{
		return Module::getLODEnabled();
	}
	void setIntValue(physx::PxU32 parameterIndex, physx::PxU32 value)
	{
		return Module::setIntValue(parameterIndex, value);
	}

	NiModuleScene* 				createNiModuleScene(NiApexScene&, NiApexRenderDebug*);
	void						releaseNiModuleScene(NiModuleScene&);
	NxAuthObjTypeID				getModuleID() const;
	NxApexRenderableIterator* 	createRenderableIterator(const NxApexScene&);

	NxAuthObjTypeID             getFieldBoundaryAssetTypeID() const;

	physx::PxU32				getModuleValue() const
	{
		return mModuleValue;
	}

	NiModuleFieldSampler* 		getNiModuleFieldSampler();

#	define PARAM_CLASS(clas) PARAM_CLASS_DECLARE_FACTORY(clas)
#	include "FieldboundaryParamClasses.inc"

	FieldBoundaryModuleParameters* 			mModuleParams;

protected:
	static NxResID				mFieldBoundaryAssetCounter;
	FieldBoundaryScene* 		getFieldBoundaryScene(const NxApexScene& apexScene);

	NxResourceList				mAuthorableObjects;
	NxResourceList				mFieldBoundaryScenes;

	physx::PxU32				mModuleValue;

	NiModuleFieldSampler* 		mFieldSamplerModule;

	friend class FieldBoundaryAsset;
	friend class FieldBoundaryScene;
};

}
}
} // end namespace

#endif // __MODULE_FIELDBOUNDARY_H__
