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

#include "NxApex.h"

#include "BasicIosAsset.h"
#include "BasicIosActor.h"
//#include "ApexSharedSerialization.h"
#include "ModuleBasicIos.h"
#include "PsShare.h"

#if defined(APEX_CUDA_SUPPORT)
#include "BasicIosActorGPU.h"
#endif

namespace physx
{
namespace apex
{
namespace basicios
{

void BasicIosAsset::processParams()
{
	NxParameterized::Handle handle(mParams);
	if (NxParameterized::ERROR_NONE != mParams->getParameterHandle("particleMass.type", handle))
	{
		PX_ALWAYS_ASSERT();
		return;
	}

	const char* type = 0;
	if (NxParameterized::ERROR_NONE != handle.getParamEnum(type))
	{
		PX_ALWAYS_ASSERT();
		return;
	}

	mMassDistribType = 0 == strcmp("uniform", type) ? UNIFORM : NORMAL;
}

BasicIosAsset::BasicIosAsset(ModuleBasicIos* module, NxResourceList& list, NxParameterized::Interface* params, const char* name) :
	mIofxAssetTracker(module->mSdk, NX_IOFX_AUTHORING_TYPE_NAME),
	mModule(module),
	mName(name),
	mParams((BasicIOSAssetParam*)params)
{
	list.add(*this);
	processParams();
}

BasicIosAsset::BasicIosAsset(ModuleBasicIos* module, NxResourceList& list, const char* name):
	mIofxAssetTracker(module->mSdk, NX_IOFX_AUTHORING_TYPE_NAME),
	mModule(module),
	mName(name),
	mParams(0)
{
	NxParameterized::Traits* traits = NiGetApexSDK()->getParameterizedTraits();
	mParams = (BasicIOSAssetParam*)traits->createNxParameterized(BasicIOSAssetParam::staticClassName());

	list.add(*this);

	processParams();
}

physx::PxF32 BasicIosAsset::getParticleMass() const
{
	physx::PxF32 m = 0.0f;
	switch (mMassDistribType)
	{
	case UNIFORM:
		m = mParams->particleMass.center + mParams->particleMass.spread * mSRand.getNext();
		break;
	case NORMAL:
		m = mNormRand.getScaled(mParams->particleMass.center, mParams->particleMass.spread);
		break;
	default:
		PX_ALWAYS_ASSERT();
	}

	return m <= 0 ? mParams->particleMass.center : m; // Clamp
}

void BasicIosAsset::release()
{
	mModule->mSdk->releaseAsset(*this);
}

void BasicIosAsset::destroy()
{
	if (mParams)
	{
		mParams->destroy();
		mParams = NULL;
	}

	delete this;
}

BasicIosAsset::~BasicIosAsset()
{
}

BasicIosActor* BasicIosAsset::getIosActorInScene(NxApexScene& scene, bool mesh) const
{
	BasicIosScene* iosScene = mModule->getBasicIosScene(scene);
	if (iosScene != 0)
	{
		for (physx::PxU32 i = 0 ; i < mIosActorList.getSize() ; i++)
		{
			BasicIosActor* iosActor = DYNAMIC_CAST(BasicIosActor*)(mIosActorList.getResource(i));
			if (iosActor->mBasicIosScene == iosScene && iosActor->mIsMesh == mesh)
			{
				return iosActor;
			}
		}
	}
	return NULL;
}

NxApexActor* BasicIosAsset::createIosActor(NxApexScene& scene, const char* iofxAssetName)
{
	BasicIosActor* iosActor = NULL;

	mIofxAssetTracker.addAssetName(iofxAssetName, false);
	physx::apex::NxIofxAsset* iofxAsset = static_cast<physx::apex::NxIofxAsset*>(mIofxAssetTracker.getAssetFromName(iofxAssetName));

	if (!iofxAsset)
	{
		APEX_DEBUG_WARNING("IOFX asset not found: %s", iofxAssetName);
	}
	else
	{
		iosActor = getIosActorInScene(scene, iofxAsset->getMeshAssetCount() > 0);
		if (iosActor == 0)
		{
			BasicIosScene* iosScene = mModule->getBasicIosScene(scene);
			if (iosScene != 0)
			{
				iosActor = iosScene->createIosActor(mIosActorList, *this, *iofxAsset);
				iosActor->mIsMesh = iofxAsset->getMeshAssetCount() > 0;
			}
		}
		PX_ASSERT(iosActor);
	}
	mIofxAssetTracker.removeAllAssetNames();
	return iosActor;
}

void BasicIosAsset::releaseIosActor(NxApexActor& actor)
{
	BasicIosActor* iosActor = DYNAMIC_CAST(BasicIosActor*)(&actor);
	iosActor->destroy();
}

PxU32 BasicIosAsset::forceLoadAssets()
{
	return 0;
}

bool BasicIosAsset::getSupportsDensity() const
{
	BasicIOSAssetParam* gridParams = (BasicIOSAssetParam*)(getAssetNxParameterized());
	return (gridParams->GridDensity.Enabled);
}

#ifndef WITHOUT_APEX_AUTHORING
/*******************   BasicIosAssetAuthoring *******************/
BasicIosAssetAuthoring::BasicIosAssetAuthoring(ModuleBasicIos* module, NxResourceList& list):
	BasicIosAsset(module, list, "Authoring")
{
}
BasicIosAssetAuthoring::BasicIosAssetAuthoring(ModuleBasicIos* module, NxResourceList& list, const char* name):
	BasicIosAsset(module, list, name)
{
}

BasicIosAssetAuthoring::BasicIosAssetAuthoring(ModuleBasicIos* module, NxResourceList& list, NxParameterized::Interface* params, const char* name) :
	BasicIosAsset(module, list, params, name)
{
}

void BasicIosAssetAuthoring::release()
{
	delete this;
}

void BasicIosAssetAuthoring::setCollisionGroupName(const char* collisionGroupName)
{
	NxParameterized::Handle h(*mParams, "collisionGroupName");
	h.setParamString(collisionGroupName);
}

void BasicIosAssetAuthoring::setCollisionGroupMaskName(const char* collisionGroupMaskName)
{
	NxParameterized::Handle h(*mParams, "collisionGroupMaskName");
	h.setParamString(collisionGroupMaskName);
}


#endif

}
}
} // namespace physx::apex
