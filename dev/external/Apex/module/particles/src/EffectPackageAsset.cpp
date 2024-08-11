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

#include "EffectPackageAsset.h"
#include "EffectPackageActorParams.h"
#include "EffectPackageActor.h"
#include "ModuleParticles.h"
#include "EmitterEffect.h"

#pragma warning(disable:4100)

namespace physx
{

namespace apex
{

namespace particles
{

EffectPackageAsset::EffectPackageAsset(ModuleParticles*, NxResourceList&, const char* name)
{
	PX_ALWAYS_ASSERT();
}

EffectPackageAsset::EffectPackageAsset(ModuleParticles* moduleParticles, NxResourceList& resourceList, NxParameterized::Interface* params, const char* name)
{
	mDefaultActorParams = NULL;
	mName = name;
	mModule = moduleParticles;
	mParams = static_cast< EffectPackageAssetParams*>(params);
	initializeAssetNameTable();
	resourceList.add(*this);
}

EffectPackageAsset::~EffectPackageAsset()
{
}

physx::PxU32	EffectPackageAsset::forceLoadAssets()
{
	return 0;
}

NxParameterized::Interface* EffectPackageAsset::getDefaultActorDesc()
{
	NxParameterized::Traits* traits = NiGetApexSDK()->getParameterizedTraits();
	PX_ASSERT(traits);
	if (!traits)
	{
		return NULL;
	}
	// create if not yet created
	if (!mDefaultActorParams)
	{
		const char* className = EffectPackageActorParams::staticClassName();
		NxParameterized::Interface* param = traits->createNxParameterized(className);
		NxParameterized::Handle h(param);
		mDefaultActorParams = static_cast<EffectPackageActorParams*>(param);
		PX_ASSERT(param);
		if (!param)
		{
			return NULL;
		}
	}
	return mDefaultActorParams;

}

void	EffectPackageAsset::release()
{
	mModule->mSdk->releaseAsset(*this);
}


NxParameterized::Interface* EffectPackageAsset::getDefaultAssetPreviewDesc()
{
	PX_ALWAYS_ASSERT();
	return NULL;
}

NxApexActor* EffectPackageAsset::createApexActor(const NxParameterized::Interface& parms, NxApexScene& apexScene)
{
	NxApexActor* ret = NULL;

	ParticlesScene* ds = mModule->getParticlesScene(apexScene);
	if (ds)
	{
		const EffectPackageAssetParams* assetParams = mParams;
		const EffectPackageActorParams* actorParams = static_cast<const EffectPackageActorParams*>(&parms);
		EffectPackageActor* ea = PX_NEW(EffectPackageActor)(this, assetParams, actorParams,
		                         *NiGetApexSDK(),
		                         apexScene,
		                         *ds,
		                         mModule->getModuleTurbulenceFS());

		ret = static_cast< NxApexActor*>(ea);
	}
	return ret;
}

void	EffectPackageAsset::destroy()
{
	if (mDefaultActorParams)
	{
		mDefaultActorParams->destroy();
		mDefaultActorParams = 0;
	}

	if (mParams)
	{
		mParams->destroy();
		mParams = NULL;
	}

	/* Actors are automatically cleaned up on deletion by NxResourceList dtor */
	delete this;

}

void EffectPackageAsset::initializeAssetNameTable()
{
}

PxF32 EffectPackageAsset::getDuration() const
{
	PxF32 ret = 0;

	for (PxI32 i = 0; i < mParams->Effects.arraySizes[0]; i++)
	{
		NxParameterized::Interface* iface = mParams->Effects.buf[i];
		if (iface && strcmp(iface->className(), EmitterEffect::staticClassName()) == 0)
		{
			EmitterEffect* ee = static_cast< EmitterEffect*>(iface);
			PxF32 v = 0;
			if (ee->EffectProperties.Duration != 0 && ee->EffectProperties.RepeatCount != 9999)
			{
				v = ee->EffectProperties.InitialDelayTime + ee->EffectProperties.RepeatCount * ee->EffectProperties.Duration + ee->EffectProperties.RepeatCount * ee->EffectProperties.RepeatDelay;
			}
			if (v == 0)	// any infinite lifespan sub-effect means the entire effect package has an infinite life
			{
				ret = 0;
				break;
			}
			else if (v > ret)
			{
				ret = v;
			}
		}
	}

	return ret;
}

bool EffectPackageAsset::useUniqueRenderVolume() const
{
	return mParams ? mParams->LODSettings.UniqueRenderVolume : false;
}

} // end of particles namespace
} // end of apex namespace
} // end of physx namespace
