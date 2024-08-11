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
#if NX_SDK_VERSION_MAJOR == 3

#include "ParticleIosAsset.h"
#include "ParticleIosActor.h"
#include "ModuleParticleIos.h"
#include "PsShare.h"
#include "FluidParticleSystemParams.h"

#if defined(APEX_CUDA_SUPPORT)
#include "ParticleIosActorGPU.h"
#endif

namespace physx
{
namespace apex
{
namespace pxparticleios
{

ParticleIosAsset::ParticleIosAsset(ModuleParticleIos* module, NxResourceList& list, NxParameterized::Interface* params, const char* name) :
	mIofxAssetTracker(module->mSdk, NX_IOFX_AUTHORING_TYPE_NAME),
	mModule(module),
	mName(name),
	mParams((ParticleIosAssetParam*)params)
{
	list.add(*this);
}

ParticleIosAsset::ParticleIosAsset(ModuleParticleIos* module, NxResourceList& list, const char* name):
	mIofxAssetTracker(module->mSdk, NX_IOFX_AUTHORING_TYPE_NAME),
	mModule(module),
	mName(name),
	mParams(0)
{
	NxParameterized::Traits* traits = NiGetApexSDK()->getParameterizedTraits();
	mParams = (ParticleIosAssetParam*)traits->createNxParameterized(ParticleIosAssetParam::staticClassName());

	list.add(*this);
}

void ParticleIosAsset::release()
{
	mModule->mSdk->releaseAsset(*this);
}

void ParticleIosAsset::destroy()
{
	if (mParams)
	{
		mParams->destroy();
		mParams = NULL;
	}

	delete this;
}

ParticleIosAsset::~ParticleIosAsset()
{
}

ParticleIosActor* ParticleIosAsset::getIosActorInScene(NxApexScene& scene, bool mesh) const
{
	ParticleIosScene* iosScene = mModule->getParticleIosScene(scene);
	if (iosScene != 0)
	{
		for (physx::PxU32 i = 0 ; i < mIosActorList.getSize() ; i++)
		{
			ParticleIosActor* iosActor = DYNAMIC_CAST(ParticleIosActor*)(mIosActorList.getResource(i));
			if (iosActor->mParticleIosScene == iosScene && iosActor->mIsMesh == mesh)
			{
				return iosActor;
			}
		}
	}
	return NULL;
}

NxApexActor* ParticleIosAsset::createIosActor(NxApexScene& scene, const char* iofxAssetName)
{
	mIofxAssetTracker.addAssetName(iofxAssetName, false);
	NxIofxAsset* iofxAsset = static_cast<NxIofxAsset*>(mIofxAssetTracker.getAssetFromName(iofxAssetName));

	ParticleIosActor* iosActor = getIosActorInScene(scene, iofxAsset->getMeshAssetCount() > 0);
	if (iosActor == 0)
	{
		ParticleIosScene* iosScene = mModule->getParticleIosScene(scene);
		if (iosScene != 0)
		{
			iosActor = iosScene->createIosActor(mIosActorList, *this, *iofxAsset);
			iosActor->mIsMesh = iofxAsset->getMeshAssetCount() > 0;
		}
	}
	PX_ASSERT(iosActor);
	return iosActor;
}

void ParticleIosAsset::releaseIosActor(NxApexActor& actor)
{
	ParticleIosActor* iosActor = DYNAMIC_CAST(ParticleIosActor*)(&actor);
	iosActor->destroy();
}

PxU32 ParticleIosAsset::forceLoadAssets()
{
	return 0;
}


#ifndef WITHOUT_APEX_AUTHORING
/*******************   ParticleIosAssetAuthoring *******************/
ParticleIosAssetAuthoring::ParticleIosAssetAuthoring(ModuleParticleIos* module, NxResourceList& list):
	ParticleIosAsset(module, list, "Authoring")
{
}
ParticleIosAssetAuthoring::ParticleIosAssetAuthoring(ModuleParticleIos* module, NxResourceList& list, const char* name):
	ParticleIosAsset(module, list, name)
{
}

ParticleIosAssetAuthoring::ParticleIosAssetAuthoring(ModuleParticleIos* module, NxResourceList& list, NxParameterized::Interface* params, const char* name) :
	ParticleIosAsset(module, list, params, name)
{
}

void ParticleIosAssetAuthoring::release()
{
	delete this;
}

void ParticleIosAssetAuthoring::setCollisionGroupName(const char* collisionGroupName)
{
	NxParameterized::Handle h(*mParams, "collisionGroupName");
	h.setParamString(collisionGroupName);
}

void ParticleIosAssetAuthoring::setCollisionGroupMaskName(const char* collisionGroupMaskName)
{
	NxParameterized::Handle h(*mParams, "collisionGroupMaskName");
	h.setParamString(collisionGroupMaskName);
}
#endif // !WITHOUT_APEX_AUTHORING

}
}
} // namespace physx::apex

#endif // NX_SDK_VERSION_MAJOR == 3