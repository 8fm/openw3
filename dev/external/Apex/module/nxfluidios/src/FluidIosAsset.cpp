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

#include "NxApex.h"
#include "PsShare.h"
#include "NxIofxAsset.h"
#include "FluidIosAsset.h"
#include "FluidIosActor.h"
#include "ModuleFluidIos.h"
#include "NxParamArray.h"

namespace physx
{
namespace apex
{
namespace nxfluidios
{

using namespace NxFluidIosParametersNS;

FluidIosAsset::FluidIosAsset(ModuleFluidIos* module, NxResourceList& list, const char* name) :
	mIofxAssetTracker(module->mSdk, NX_IOFX_AUTHORING_TYPE_NAME),
	mModule(module),
	mName(name)
{
	NxParameterized::Traits* traits = NiGetApexSDK()->getParameterizedTraits();
	mParams = (NxFluidIosParameters*)traits->createNxParameterized(NxFluidIosParameters::staticClassName());

	list.add(*this);
}

FluidIosAsset::FluidIosAsset(ModuleFluidIos* module,
                             NxResourceList& list,
                             NxParameterized::Interface* params,
                             const char* name) :
	mIofxAssetTracker(module->mSdk, NX_IOFX_AUTHORING_TYPE_NAME),
	mModule(module),
	mName(name),
	mParams((NxFluidIosParameters*)params)
{
	list.add(*this);
}

void FluidIosAsset::release()
{
	mModule->mSdk->releaseAsset(*this);
}

void FluidIosAsset::destroy()
{
	if (mParams)
	{
		mParams->destroy();
		mParams = NULL;
	}

	delete this;
}

FluidIosAsset::~FluidIosAsset()
{
}


/* Find an IOS actor in this scene that is the correct sprite/mesh type */
FluidIosActor* FluidIosAsset::getIosActorInScene(NxApexScene& scene, bool mesh) const
{
	FluidIosScene* ps =	mModule->getParticleScene(scene);
	if (ps)
	{
		for (physx::PxU32 i = 0 ; i < mIosActorList.getSize() ; i++)
		{
			FluidIosActor* ios = DYNAMIC_CAST(FluidIosActor*)(mIosActorList.getResource(i));
			if (ios->mParticleScene == ps && ios->mIsMesh == mesh)
			{
				return ios;
			}
		}
	}
	return NULL;
}

NxApexActor* FluidIosAsset::createIosActor(NxApexScene& scene, const char* iofxAssetName)
{
	mIofxAssetTracker.addAssetName(iofxAssetName, false);
	NxIofxAsset* iofxAsset = static_cast<NxIofxAsset*>(mIofxAssetTracker.getAssetFromName(iofxAssetName));
	if (!iofxAsset)
	{
		return NULL;
	}

	FluidIosActor* ios = getIosActorInScene(scene, iofxAsset->getMeshAssetCount() > 0);
	if (!ios)
	{
		FluidIosScene* ps =	mModule->getParticleScene(scene);
		if (ps)
		{
			ios = PX_NEW(FluidIosActor)(mIosActorList, *this, *ps, iofxAsset);
			ios->mIsMesh = iofxAsset->getMeshAssetCount() > 0;
		}
	}
	PX_ASSERT(ios);
	return ios;
}

void FluidIosAsset::releaseIosActor(NxApexActor& actor)
{
	FluidIosActor* ios = DYNAMIC_CAST(FluidIosActor*)(&actor);
	ios->destroy();
}

bool FluidIosAsset::getSupportsDensity() const
{
#if APEX_PARTICLES_USE_OLD_SERIALIZATION
	return (mFluidTemplate.data.simulationMethod != NX_F_NO_PARTICLE_INTERACTION);
#else
	NxParameterized::Handle h(*mParams, "simulationMethod");
	PX_ASSERT(h.isValid());
	const char* tmpStrPtr;
	mParams->getParamEnum(h, tmpStrPtr);

	return (strcmp(tmpStrPtr, "noInteraction") != 0);
#endif
}


bool FluidIosAsset::getFluidTemplate(NxFluidDescBase& dest) const
{
#if APEX_PARTICLES_USE_OLD_SERIALIZATION

	return mFluidTemplate.get(static_cast<FluidTemplate&>(dest));

#else

	NxParameterized::Handle h(*mParams);
	const char* tmpStrPtr;
	ApexSimpleString tmpStr;

	dest.attractionForDynamicShapes			= mParams->attractionForDynamicShapes;
	dest.attractionForStaticShapes			= mParams->attractionForStaticShapes;
	dest.damping							= mParams->damping;
	dest.dynamicFrictionForDynamicShapes	= mParams->dynamicFrictionForDynamicShapes;
	dest.dynamicFrictionForStaticShapes		= mParams->dynamicFrictionForStaticShapes;
	NxFromPxVec3(dest.externalAcceleration, mParams->externalAcceleration);
	dest.fadeInTime							= mParams->fadeInTime;
	dest.kernelRadiusMultiplier				= mParams->kernelRadiusMultiplier;
	dest.maxParticles						= mParams->maxParticleCount;
	dest.motionLimitMultiplier				= mParams->motionLimitMultiplier;
	dest.packetSizeMultiplier				= mParams->packetSizeMultiplier;
	dest.numReserveParticles				= mParams->reserveParticleCount;
	dest.restDensity						= mParams->restDensity;
	dest.restitutionForDynamicShapes		= mParams->restitutionForDynamicShapes;
	dest.restitutionForStaticShapes			= mParams->restitutionForStaticShapes;
	dest.staticFrictionForDynamicShapes		= mParams->staticFrictionForDynamicShapes;
	dest.staticFrictionForStaticShapes		= mParams->staticFrictionForStaticShapes;
	dest.stiffness							= mParams->stiffness;
	dest.surfaceTension						= mParams->surfaceTension;
	dest.viscosity							= mParams->viscosity;
	dest.collisionDistanceMultiplier		= mParams->collisionDistanceMultiplier;

	mParams->getParameterHandle("simulationMethod", h);
	PX_ASSERT(h.isValid());
	mParams->getParamEnum(h, tmpStrPtr);
	tmpStr = tmpStrPtr;

	if (tmpStr == "SPH")
	{
		dest.simulationMethod = NX_F_SPH;
	}
	else if (tmpStr == "noInteraction")
	{
		dest.simulationMethod = NX_F_NO_PARTICLE_INTERACTION;
	}
	else if (tmpStr == "mixedMode")
	{
		dest.simulationMethod = NX_F_MIXED_MODE;
	}

	dest.collisionMethod = 0;
	dest.collisionMethod |= mParams->staticCollision ? NX_F_STATIC : 0;
	dest.collisionMethod |= mParams->dynamicCollision ? NX_F_DYNAMIC : 0;

	dest.flags = 0;
	dest.flags |= mParams->visualization ? NX_FF_VISUALIZATION : 0;
	dest.flags |= mParams->disableGravity ? NX_FF_DISABLE_GRAVITY : 0;
	dest.flags |= mParams->twoWayCollision ? NX_FF_COLLISION_TWOWAY : 0;
	dest.flags |= mParams->simulationEnabled ? NX_FF_ENABLED : 0;
	dest.flags |= mParams->useGPU ? NX_FF_HARDWARE : 0;
	dest.flags |= mParams->priorityMode ? NX_FF_PRIORITY_MODE : 0;
	dest.flags |= mParams->projectedToPlane ? NX_FF_PROJECT_TO_PLANE : 0;

	if (mParams->collisionGroupName && mParams->collisionGroupName[0])
	{
		// If authored, resolve the collision group name into the actual ID
		NiResourceProvider* nrp = mModule->mSdk->getInternalResourceProvider();
		NxResID cgns = mModule->mSdk->getCollisionGroupNameSpace();
		NxResID cgresid = nrp->createResource(cgns, mParams->collisionGroupName);
		dest.collisionGroup = (NxCollisionGroup)(size_t) nrp->getResource(cgresid);

		NxResID cgmns = mModule->mSdk->getCollisionGroup128NameSpace();
		NxResID cgmresid = nrp->createResource(cgmns, mParams->collisionGroupName);
		void* tmpCGM = nrp->getResource(cgmresid);
		if (tmpCGM)
		{
			dest.groupsMask = *(static_cast<NxGroupsMask*>(tmpCGM));
		}
		//nrp->releaseResource( cgresid );
	}
	if (mParams->forcefieldMaterialName && mParams->forcefieldMaterialName[0])
	{
		// If authored, resolve the collision group name into the actual ID
		NiResourceProvider* nrp = mModule->mSdk->getInternalResourceProvider();
		NxResID ffns = mModule->mSdk->getPhysicalMaterialNameSpace();
		NxResID ffresid = nrp->createResource(ffns, mParams->forcefieldMaterialName);
		dest.forceFieldMaterial = (NxForceFieldMaterial)(size_t) nrp->getResource(ffresid);
		//nrp->releaseResource( ffresid );
	}

	PX_ASSERT(dest.isValid());
	if (!dest.isValid())
	{
		APEX_DEBUG_WARNING("Invalid fluid desc");
	}

	return true;
#endif
}


#ifndef WITHOUT_APEX_AUTHORING
/*******************   FluidIosAssetAuthoring *******************/

void FluidIosAssetAuthoring::release()
{
	delete this;
}

void FluidIosAsset::internalSetFluidTemplate(const NxFluidDescBase* desc)
{

#if APEX_PARTICLES_USE_OLD_SERIALIZATION
	PX_COMPILE_TIME_ASSERT(sizeof(NxFluidDescBase) == sizeof(FluidTemplate));
	mFluidTemplate.set(static_cast<const FluidTemplate*>(desc));
#else

	NxParameterized::Handle h(*mParams);

	PX_ASSERT(desc->isValid());
	if (!desc->isValid())
	{
		APEX_DEBUG_WARNING("Invalid fluid desc");
	}

	mParams->attractionForDynamicShapes		= desc->attractionForDynamicShapes;
	mParams->attractionForStaticShapes		= desc->attractionForStaticShapes;
	mParams->damping							= desc->damping;
	mParams->dynamicFrictionForDynamicShapes	= desc->dynamicFrictionForDynamicShapes;
	mParams->dynamicFrictionForStaticShapes	= desc->dynamicFrictionForStaticShapes;
	PxFromNxVec3(mParams->externalAcceleration, desc->externalAcceleration);
	mParams->fadeInTime						= desc->fadeInTime;
	mParams->kernelRadiusMultiplier			= desc->kernelRadiusMultiplier;
	mParams->maxParticleCount				= desc->maxParticles; //?
	mParams->motionLimitMultiplier			= desc->motionLimitMultiplier;
	mParams->packetSizeMultiplier			= desc->packetSizeMultiplier;
	mParams->reserveParticleCount			= desc->numReserveParticles;
	mParams->restDensity						= desc->restDensity;
	mParams->restitutionForDynamicShapes		= desc->restitutionForDynamicShapes;
	mParams->restitutionForStaticShapes		= desc->restitutionForStaticShapes;
	mParams->staticFrictionForDynamicShapes	= desc->staticFrictionForDynamicShapes;
	mParams->staticFrictionForStaticShapes	= desc->staticFrictionForStaticShapes;
	mParams->stiffness						= desc->stiffness;
	mParams->surfaceTension					= desc->surfaceTension;
	mParams->viscosity						= desc->viscosity;
	mParams->collisionDistanceMultiplier	= desc->collisionDistanceMultiplier;

	mParams->getParameterHandle("simulationMethod", h);
	PX_ASSERT(h.isValid());
	switch (desc->simulationMethod)
	{
	case NX_F_SPH:
		mParams->setParamEnum(h, "SPH");
		break;
	case NX_F_NO_PARTICLE_INTERACTION:
		mParams->setParamEnum(h, "noInteraction");
		break;
	case NX_F_MIXED_MODE:
		mParams->setParamEnum(h, "mixedMode");
		break;
	}


	mParams->staticCollision = (desc->collisionMethod & NX_F_STATIC) ? true : false;
	mParams->dynamicCollision = (desc->collisionMethod & NX_F_DYNAMIC) ? true : false;

	mParams->visualization = (desc->flags & NX_FF_VISUALIZATION) ? true : false;
	mParams->disableGravity = (desc->flags & NX_FF_DISABLE_GRAVITY) ? true : false;
	mParams->twoWayCollision = (desc->flags & NX_FF_COLLISION_TWOWAY) ? true : false;
	mParams->simulationEnabled = (desc->flags & NX_FF_ENABLED) ? true : false;
	mParams->useGPU = (desc->flags & NX_FF_HARDWARE) ? true : false;
	mParams->priorityMode = (desc->flags & NX_FF_PRIORITY_MODE) ? true : false;
	mParams->projectedToPlane = (desc->flags & NX_FF_PROJECT_TO_PLANE) ? true : false;

#endif

}

void FluidIosAssetAuthoring::setFluidTemplate(const NxFluidDescBase* desc)
{
	internalSetFluidTemplate(desc);
}
#endif

/*******************   FluidTemplate *******************/


FluidTemplate& FluidTemplate::operator=(const FluidTemplate& src)
{
	//copy over the variables that Fluid templates transfer: (memcopy is a bad idea because it will clobber types, etc.)
	restDensity						= src.restDensity;				// TODO could be defined by a particle mass
	kernelRadiusMultiplier			= src.kernelRadiusMultiplier;	// TODO it's bad to set the restParticlesPerMeter inside, but kernelRadiusMultiplier from outside?
	motionLimitMultiplier			= src.motionLimitMultiplier;
	packetSizeMultiplier			= src.packetSizeMultiplier;
	stiffness						= src.stiffness;
	viscosity						= src.viscosity;
	damping							= src.damping;
	fadeInTime						= src.fadeInTime;
	externalAcceleration			= src.externalAcceleration;
	collisionResponseCoefficient	= src.collisionResponseCoefficient;
	simulationMethod				= src.simulationMethod;
	collisionMethod					= src.collisionMethod;
	collisionGroup					= src.collisionGroup;
	groupsMask						= src.groupsMask;
	flags							= src.flags;
	compartment						= src.compartment;


#if NX_SDK_VERSION_NUMBER < 280
#if NX_SDK_VERSION_NUMBER == 275
	//PX_COMPILE_TIME_ASSERT(sizeof(FluidTemplate) == TODO);
	staticCollisionStiction			= src.staticCollisionStiction;
	dynamicCollisionStiction	    = src.dynamicCollisionStiction;
#else
	//PX_COMPILE_TIME_ASSERT(sizeof(FluidTemplate) == TODO);
#endif
	staticCollisionRestitution		= src.staticCollisionRestitution;
	staticCollisionAdhesion			= src.staticCollisionAdhesion;
	staticCollisionAttraction		= src.staticCollisionAttraction;
	dynamicCollisionRestitution		= src.dynamicCollisionRestitution;
	dynamicCollisionAdhesion		= src.dynamicCollisionAdhesion;
	dynamicCollisionAttraction		= src.dynamicCollisionAttraction;
#else
	//PX_COMPILE_TIME_ASSERT(sizeof(FluidTemplate) == TODO);
	surfaceTension					= src.surfaceTension;
	projectionPlane					= src.projectionPlane;
	restitutionForStaticShapes		= src.restitutionForStaticShapes;
	dynamicFrictionForStaticShapes	= src.dynamicFrictionForStaticShapes;
	staticFrictionForStaticShapes	= src.staticFrictionForStaticShapes;
	attractionForStaticShapes		= src.attractionForStaticShapes;
	restitutionForDynamicShapes		= src.restitutionForDynamicShapes;
	dynamicFrictionForDynamicShapes	= src.dynamicFrictionForDynamicShapes;
	staticFrictionForDynamicShapes	= src.staticFrictionForDynamicShapes;
	attractionForDynamicShapes		= src.attractionForDynamicShapes;
	forceFieldMaterial				= src.forceFieldMaterial;
	numReserveParticles				= src.numReserveParticles;
#endif

	return *this;
}

}
}
} // namespace physx::apex

#endif // NX_SDK_VERSION_MAJOR == 2