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
#include "MinPhysxSdkVersion.h"
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

#include "NxApex.h"
#include "ForceFieldAssetPreview.h"
#include "ForceFieldAsset.h"
#include "ForceFieldActor.h"
#include "ModuleForceField.h"
#include "ForceFieldScene.h"
#include "ApexResourceHelper.h"
#include "NxApexAssetPreviewScene.h"


namespace physx
{
namespace apex
{
namespace forcefield
{

ForceFieldAsset::ForceFieldAsset(ModuleForceField* module, NxResourceList& list, const char* name) :
	mModule(module),
	mName(name),
	mDefaultActorParams(NULL),
	mDefaultPreviewParams(NULL)
{
	NxParameterized::Traits* traits = NiGetApexSDK()->getParameterizedTraits();
	mParams = (ForceFieldAssetParams*)traits->createNxParameterized(ForceFieldAssetParams::staticClassName());

	initializeAssetNameTable();

	list.add(*this);
}

ForceFieldAsset::ForceFieldAsset(ModuleForceField* module, NxResourceList& list, NxParameterized::Interface* params, const char* name) :
	mModule(module),
	mName(name),
	mParams((ForceFieldAssetParams*)params),
	mDefaultActorParams(NULL),
	mDefaultPreviewParams(NULL)
{
	initializeAssetNameTable();

	list.add(*this);
}

ForceFieldAsset::~ForceFieldAsset()
{
}

void ForceFieldAsset::destroy()
{
	if (mDefaultActorParams)
	{
		mDefaultActorParams->destroy();
		mDefaultActorParams = 0;
	}

	if (mDefaultPreviewParams)
	{
		mDefaultPreviewParams->destroy();
		mDefaultPreviewParams = 0;
	}
	if (mParams)
	{
		mParams->destroy();
		mParams = NULL;
	}
	/* Assets that were forceloaded or loaded by actors will be automatically
	 * released by the ApexAssetTracker member destructors.
	 */

	/* Actors are automatically cleaned up on deletion by NxResourceList dtor */
	delete this;
}

physx::PxU32 ForceFieldAsset::forceLoadAssets()
{
	// Is there anything to be done here?
	return NULL;
}

void ForceFieldAsset::initializeAssetNameTable()
{
	// Is there anything to be done here?
}

NxParameterized::Interface* ForceFieldAsset::getDefaultActorDesc()
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
		NxParameterized::ErrorType error = NxParameterized::ERROR_NONE;
		PX_UNUSED(error);

		const char* className = ForceFieldActorParams::staticClassName();
		NxParameterized::Interface* param = traits->createNxParameterized(className);
		NxParameterized::Handle h(param);
		mDefaultActorParams = static_cast<ForceFieldActorParams*>(param);

		PX_ASSERT(param);
		if (!param)
		{
			return NULL;
		}
	}

	return mDefaultActorParams;
}


NxParameterized::Interface* ForceFieldAsset::getDefaultAssetPreviewDesc()
{
	NxParameterized::Traits* traits = NiGetApexSDK()->getParameterizedTraits();
	PX_ASSERT(traits);
	if (!traits)
	{
		return NULL;
	}

	// create if not yet created
	if (!mDefaultPreviewParams)
	{
		const char* className = ForceFieldAssetPreviewParams::staticClassName();
		NxParameterized::Interface* param = traits->createNxParameterized(className);
		mDefaultPreviewParams = static_cast<ForceFieldAssetPreviewParams*>(param);

		PX_ASSERT(param);
		if (!param)
		{
			return NULL;
		}
	}
	else
	{
		mDefaultPreviewParams->initDefaults();
	}

	return mDefaultPreviewParams;
}

NxApexActor* ForceFieldAsset::createApexActor(const NxParameterized::Interface& parms, NxApexScene& apexScene)
{
	if (!isValidForActorCreation(parms, apexScene))
	{
		return NULL;
	}

	NxApexActor* ret = 0;

	if (strcmp(parms.className(), ForceFieldActorParams::staticClassName()) == 0)
	{
		NxForceFieldActorDesc desc;

		const ForceFieldActorParams* pDesc = static_cast<const ForceFieldActorParams*>(&parms);
		desc.initialPose		= pDesc->initialPose;
		desc.scale				= pDesc->scale;
		desc.samplerFilterData  = ApexResourceHelper::resolveCollisionGroup64(pDesc->fieldSamplerFilterDataName ? pDesc->fieldSamplerFilterDataName : mParams->fieldSamplerFilterDataName);
		desc.boundaryFilterData = ApexResourceHelper::resolveCollisionGroup64(pDesc->fieldBoundaryFilterDataName ? pDesc->fieldBoundaryFilterDataName : mParams->fieldBoundaryFilterDataName);

		ForceFieldScene* es = mModule->getForceFieldScene(apexScene);
		ret = es->createForceFieldActor(desc, *this, mForceFieldActors);
	}

	return ret;
}

NxApexAssetPreview* ForceFieldAsset::createApexAssetPreview(const NxParameterized::Interface& parms, NxApexAssetPreviewScene* previewScene)
{
	NxApexAssetPreview* ret = 0;

	const char* className = parms.className();
	if (strcmp(className, ForceFieldAssetPreviewParams::staticClassName()) == 0)
	{
		NxForceFieldPreviewDesc desc;
		const ForceFieldAssetPreviewParams* pDesc = static_cast<const ForceFieldAssetPreviewParams*>(&parms);

		desc.mPose	= pDesc->pose;
		desc.mIconScale = pDesc->iconScale;
		desc.mPreviewDetail = 0;
		if (pDesc->drawIcon)
		{
			desc.mPreviewDetail |= APEX_FORCEFIELD::FORCEFIELD_DRAW_ICON;
		}
		if (pDesc->drawBoundaries)
		{
			desc.mPreviewDetail |= APEX_FORCEFIELD::FORCEFIELD_DRAW_BOUNDARIES;
		}
		if (pDesc->drawBold)
		{
			desc.mPreviewDetail |= APEX_FORCEFIELD::FORCEFIELD_DRAW_WITH_CYLINDERS;
		}

		ret = createForceFieldPreview(desc, previewScene);
	}

	return ret;
}

void ForceFieldAsset::releaseForceFieldActor(NxForceFieldActor& nxactor)
{
	ForceFieldActor* actor = DYNAMIC_CAST(ForceFieldActor*)(&nxactor);
	actor->destroy();
}

NxForceFieldPreview* ForceFieldAsset::createForceFieldPreview(const NxForceFieldPreviewDesc& desc, NxApexAssetPreviewScene* previewScene)
{
	return(createForceFieldPreviewImpl(desc, this, previewScene));
}

NxForceFieldPreview* ForceFieldAsset::createForceFieldPreviewImpl(const NxForceFieldPreviewDesc& desc, ForceFieldAsset* ForceFieldAsset, NxApexAssetPreviewScene* previewScene)
{
	return(PX_NEW(ForceFieldAssetPreview)(desc, mModule->mSdk, ForceFieldAsset, previewScene));
}

void ForceFieldAsset::releaseForceFieldPreview(NxForceFieldPreview& nxpreview)
{
	ForceFieldAssetPreview* preview = DYNAMIC_CAST(ForceFieldAssetPreview*)(&nxpreview);
	preview->destroy();
}

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
