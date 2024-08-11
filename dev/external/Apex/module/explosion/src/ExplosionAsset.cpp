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
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2

#include "NxApex.h"
#include "NxFieldBoundaryAsset.h"
#include "ExplosionAssetPreview.h"
#include "ExplosionAsset.h"
#include "ExplosionActor.h"
#include "ModuleExplosion.h"
#include "ExplosionScene.h"

namespace physx
{
namespace apex
{
namespace explosion
{

ExplosionAsset::ExplosionAsset(ModuleExplosion* module, NxResourceList& list, const char* name) :
	mModule(module),
	mFieldBoundaryAssetTracker(module->mSdk, NX_FIELD_BOUNDARY_AUTHORING_TYPE_NAME),
	mName(name),
	mDefaultActorParams(NULL),
	mDefaultPreviewParams(NULL),
	mBoundariesNamesParamHandle(0)
{
	NxParameterized::Traits* traits = NiGetApexSDK()->getParameterizedTraits();
	mParams = (ExplosionAssetParam*)traits->createNxParameterized(ExplosionAssetParam::staticClassName());
	mBoundariesNamesParamHandle.reset();
	mBoundariesNamesParamHandle.setInterface(mParams);
	mParams->getParameterHandle("boundariesNames", mBoundariesNamesParamHandle);

	initializeAssetNameTable();

	list.add(*this);
}

ExplosionAsset::ExplosionAsset(ModuleExplosion* module, NxResourceList& list, NxParameterized::Interface* params, const char* name) :
	mModule(module),
	mFieldBoundaryAssetTracker(module->mSdk, NX_FIELD_BOUNDARY_AUTHORING_TYPE_NAME),
	mName(name),
	mParams((ExplosionAssetParam*)params),
	mDefaultActorParams(NULL),
	mDefaultPreviewParams(NULL),
	mBoundariesNamesParamHandle(*mParams)
{
	mParams->getParameterHandle("boundariesNames", mBoundariesNamesParamHandle);

	initializeAssetNameTable();

	list.add(*this);
}

ExplosionAsset::~ExplosionAsset()
{
}

void ExplosionAsset::destroy()
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

physx::PxU32 ExplosionAsset::forceLoadAssets()
{
	return mFieldBoundaryAssetTracker.forceLoadAssets();
}

void ExplosionAsset::initializeAssetNameTable()
{
	physx::Array<AssetNameIDMapping*>& nameIdList = mFieldBoundaryAssetTracker.getNameIdList();

	for (physx::PxU32 i = 0; i < getFieldBoundariesCount(); i++)
	{
		AssetNameIDMapping* map = PX_NEW(AssetNameIDMapping)(getFieldBoundariesName(i), false);
		nameIdList.pushBack(map);
	}
}

NxParameterized::Interface* ExplosionAsset::getDefaultActorDesc()
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

		const char* className = ExplosionActorParameters::staticClassName();
		NxParameterized::Interface* param = traits->createNxParameterized(className);
		NxParameterized::Handle h(param);
		mDefaultActorParams = static_cast<ExplosionActorParameters*>(param);

		PX_ASSERT(param);
		if (!param)
		{
			return NULL;
		}

		// initialize the explosion environment parameter
		error = param->getParameterHandle("envSetting", h);
		PX_ASSERT(error == NxParameterized::ERROR_NONE);

		error =  h.initParamRef(h.parameterDefinition()->refVariantVal(0), true);
		PX_ASSERT(error == NxParameterized::ERROR_NONE);
	}

	return mDefaultActorParams;
}


NxParameterized::Interface* ExplosionAsset::getDefaultAssetPreviewDesc()
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
		const char* className = ExplosionAssetPreviewParameters::staticClassName();
		NxParameterized::Interface* param = traits->createNxParameterized(className);
		mDefaultPreviewParams = static_cast<ExplosionAssetPreviewParameters*>(param);

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

NxApexActor* ExplosionAsset::createApexActor(const NxParameterized::Interface& parms, NxApexScene& apexScene)
{
	if (!isValidForActorCreation(parms, apexScene))
	{
		return NULL;
	}

	NxApexActor* ret = 0;

	if (strcmp(parms.className(), ExplosionActorParameters::staticClassName()) == 0)
	{
		NxExplosionActorDesc desc;

		const ExplosionActorParameters* pDesc = static_cast<const ExplosionActorParameters*>(&parms);
		desc.initialPose		= pDesc->initialPose;
		desc.scale				= pDesc->scale;

#if NX_SDK_VERSION_MAJOR == 2
		const ExplosionEnvParameters* pEnvDesc = static_cast<const ExplosionEnvParameters*>(pDesc->envSetting);
		NxParamToExplEnvSettings(desc.envSetting, pEnvDesc);
#endif

		ExplosionScene* es = mModule->getExplosionScene(apexScene);
		ret = es->createExplosionActor(desc, *this, mExplosionActors);
	}

	return ret;
}

NxApexAssetPreview* ExplosionAsset::createApexAssetPreview(const NxParameterized::Interface& parms, NxApexAssetPreviewScene* previewScene)
{
	PX_UNUSED(previewScene);

	NxApexAssetPreview* ret = 0;

	const char* className = parms.className();
	if (strcmp(className, ExplosionAssetPreviewParameters::staticClassName()) == 0)
	{
		NxExplosionPreviewDesc desc;
		const ExplosionAssetPreviewParameters* pDesc = static_cast<const ExplosionAssetPreviewParameters*>(&parms);

		desc.mPose	= pDesc->pose;
		desc.mIconScale = pDesc->iconScale;
		desc.mPreviewDetail = 0;
		if (pDesc->drawIcon)
		{
			desc.mPreviewDetail |= APEX_EXPLOSION::EXPLOSION_DRAW_ICON;
		}
		if (pDesc->drawBoundaries)
		{
			desc.mPreviewDetail |= APEX_EXPLOSION::EXPLOSION_DRAW_BOUNDARIES;
		}
		if (pDesc->drawBold)
		{
			desc.mPreviewDetail |= APEX_EXPLOSION::EXPLOSION_DRAW_WITH_CYLINDERS;
		}

		ret = createExplosionPreview(desc);
	}

	return ret;
}

void ExplosionAsset::releaseExplosionActor(NxExplosionActor& nxactor)
{
	ExplosionActor* actor = DYNAMIC_CAST(ExplosionActor*)(&nxactor);
	actor->destroy();

#if 0 // This may still be a good idea, but the asset should just be released entirely

	/* On last actor release, release our reference to the FieldBoundary asset.  This may result
	* in a callback to the game engine to release the field boundary asset entirely.
	*/
	if (mExplosionActors.getSize() == 0)
	{
		NiResourceProvider* nrp = mModule->mSdk->getInternalResourceProvider();
		physx::PxU32 boundaryCount = mBoundariesNames.size();
		for (physx::PxU32 i = 0; i < boundaryCount; i++)
		{
			if (mBoundariesResIDs[i] != INVALID_RESOURCE_ID)
			{
				nrp->releaseResource(mBoundariesResIDs[i]);
				mBoundariesResIDs[i] = INVALID_RESOURCE_ID;
				mBoundariesAssets[i] = NULL;
			}
		}
		bBoundariesLoaded = false;
	}
#endif
}

NxExplosionPreview* ExplosionAsset::createExplosionPreview(const NxExplosionPreviewDesc& desc)
{
	return(createExplosionPreviewImpl(desc, this));
}

NxExplosionPreview* ExplosionAsset::createExplosionPreviewImpl(const NxExplosionPreviewDesc& desc, ExplosionAsset* ExplosionAsset)
{
	return(PX_NEW(ExplosionAssetPreview)(desc, mModule->mSdk, ExplosionAsset));
}

void ExplosionAsset::releaseExplosionPreview(NxExplosionPreview& nxpreview)
{
	ExplosionAssetPreview* preview = DYNAMIC_CAST(ExplosionAssetPreview*)(&nxpreview);
	preview->destroy();
}

physx::PxU32	ExplosionAsset::getFieldBoundariesCount() const
{
	int size = 0;
	mParams->getArraySize(mBoundariesNamesParamHandle, size);

	return size;
}

const char*	ExplosionAsset::getFieldBoundariesName(physx::PxU32 boundIndex)
{
	NxParameterized::Handle elemHndl(*mParams);
	NxParameterized::Interface* refPtr = 0;

	if (mBoundariesNamesParamHandle.getChildHandle(boundIndex, elemHndl) == NxParameterized::ERROR_NONE)
	{
		if (elemHndl.getParamRef(refPtr) == NxParameterized::ERROR_NONE)
		{
			return refPtr->name();
		}
	}
	return NULL;
}

#ifndef WITHOUT_APEX_AUTHORING
void ExplosionAssetAuthoring::addFieldBoundaryName(const char* n)
{
	NxParameterized::ErrorType err;
	NxParameterized::Handle lastElemHndl(*mParams);
	int size;
	err = mParams->getArraySize(mBoundariesNamesParamHandle, size);

	if (err != NxParameterized::ERROR_NONE)
	{
		return;
	}

	err = mParams->resizeArray(mBoundariesNamesParamHandle, size + 1);

	if (err != NxParameterized::ERROR_NONE)
	{
		return;
	}

	err = mBoundariesNamesParamHandle.getChildHandle(size, lastElemHndl);

	if (err != NxParameterized::ERROR_NONE)
	{
		return;
	}

	mParams->setParamString(lastElemHndl, n);
}
#endif

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
