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

#ifndef APEX_LEGACY_MODULE
#define APEX_LEGACY_MODULE

#include "NxParameterizedTraits.h"

#include "NiModule.h"
#include "Module.h"

namespace physx
{
namespace apex
{

struct LegacyClassEntry
{
	physx::PxU32 version;
	physx::PxU32 nextVersion;
	NxParameterized::Factory* factory;
	void (*freeParameterDefinitionTable)(NxParameterized::Traits* t);
	NxParameterized::Conversion* (*createConv)(NxParameterized::Traits*);
	NxParameterized::Conversion* conv;
};

class ApexLegacyModule : public NxModule, public NiModule, public Module
{
public:
	virtual ~ApexLegacyModule() {}

	// base class methods
	void						init(NxParameterized::Interface&) {}

	NxParameterized::Interface* getDefaultModuleDesc()
	{
		return 0;
	}

	void release()
	{
		Module::release();
	}
	void destroy()
	{
		releaseLegacyObjects();
		Module::destroy();
		delete this;
	}

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

	void						setLODUnitCost(physx::PxF32 cost)
	{
		Module::setLODUnitCost(cost);
	}
	physx::PxF32				getLODUnitCost() const
	{
		return Module::getLODUnitCost();
	}
	void						setLODBenefitValue(physx::PxF32 value)
	{
		Module::setLODBenefitValue(value);
	}
	physx::PxF32				getLODBenefitValue() const
	{
		return Module::getLODBenefitValue();
	}
	void						setLODEnabled(bool enabled)
	{
		Module::setLODEnabled(enabled);
	}
	bool						getLODEnabled() const
	{
		return Module::getLODEnabled();
	}

	void						setIntValue(physx::PxU32 parameterIndex, physx::PxU32 value)
	{
		return Module::setIntValue(parameterIndex, value);
	}

	NiModuleScene* 				createNiModuleScene(NiApexScene&, NiApexRenderDebug*)
	{
		return NULL;
	}
	void						releaseNiModuleScene(NiModuleScene&) {}
	physx::PxU32				forceLoadAssets()
	{
		return 0;
	}
	NxAuthObjTypeID				getModuleID() const
	{
		return PX_MAX_U32;
	}
	NxApexRenderableIterator* 	createRenderableIterator(const NxApexScene&)
	{
		return NULL;
	}

protected:
	virtual void releaseLegacyObjects() = 0;

	void registerLegacyObjects(LegacyClassEntry* e)
	{
		NxParameterized::Traits* t = mSdk->getParameterizedTraits();
		if (!t)
		{
			return;
		}

		for (; e->factory; ++e)
		{
			t->registerFactory(*e->factory);

			e->conv = e->createConv(t);
			t->registerConversion(e->factory->getClassName(), e->version, e->nextVersion, *e->conv);
		}
	}

	void unregisterLegacyObjects(LegacyClassEntry* e)
	{
		NxParameterized::Traits* t = mSdk->getParameterizedTraits();
		if (!t)
		{
			return;
		}

		for (; e->factory; ++e)
		{
			t->removeConversion(
			    e->factory->getClassName(),
			    e->version,
			    e->nextVersion
			);
			e->conv->release();

			t->removeFactory(e->factory->getClassName(), e->factory->getVersion());

			e->freeParameterDefinitionTable(t);
		}
	}
};

} // namespace apex
} // namespace physx

#define DEFINE_CREATE_MODULE(Module) \
	NiApexSDK* gApexSdk = 0; \
	NxApexSDK* NxGetApexSDK() { return gApexSdk; } \
	NiApexSDK* NiGetApexSDK() { return gApexSdk; } \
	NXAPEX_API NxModule*  NX_CALL_CONV createModule( \
	        NiApexSDK* inSdk, \
	        NiModule** niRef, \
	        physx::PxU32 APEXsdkVersion, \
	        physx::PxU32 PhysXsdkVersion, \
	        NxApexCreateError* errorCode) \
	{ \
		if (APEXsdkVersion != NX_APEX_SDK_VERSION) \
		{ \
			if (errorCode) *errorCode = APEX_CE_WRONG_VERSION; \
			return NULL; \
		} \
		\
		if (PhysXsdkVersion != NX_PHYSICS_SDK_VERSION) \
		{ \
			if (errorCode) *errorCode = APEX_CE_WRONG_VERSION; \
			return NULL; \
		} \
		\
		gApexSdk = inSdk; \
		APEX_INIT_FOUNDATION(); \
		\
		Module *impl = PX_NEW(Module)(inSdk); \
		*niRef  = (NiModule *) impl; \
		return (NxModule *) impl; \
	}

#define DEFINE_INSTANTIATE_MODULE(Module) \
	void instantiate##Module() \
	{ \
		NiApexSDK *sdk = NiGetApexSDK(); \
		Module *impl = PX_NEW(Module)(sdk); \
		sdk->registerExternalModule((NxModule *) impl, (NiModule *) impl); \
	}

#endif // __APEX_LEGACY_MODULE__
