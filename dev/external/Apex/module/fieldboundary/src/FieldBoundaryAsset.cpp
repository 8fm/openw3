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
#include "nxconvexmeshdesc.h"

#include "FieldBoundaryAsset.h"
#include "FieldBoundaryActor.h"
#include "ModuleFieldBoundary.h"
#include "ShapeBoxParams.h"
#include "ShapeCapsuleParams.h"
#include "ShapeSphereParams.h"
#include "ShapeConvexParams.h"
#include "FieldBoundaryAssetPreview.h"
#include "FieldBoundaryPreviewParameters.h"
#include "PsShare.h"

namespace physx
{
namespace apex
{
namespace fieldboundary
{

FieldBoundaryAsset::FieldBoundaryAsset(ModuleFieldBoundary* module, NxResourceList& list, const char* name) :
	mModule(module),
	mName(name),
	mDefaultActorParams(NULL),
	mDefaultPreviewParams(NULL)
{
	mParams = NULL;
	initAllMembers();
	list.add(*this);
}

FieldBoundaryAsset::FieldBoundaryAsset(ModuleFieldBoundary* module, NxResourceList& list,
                                       NxParameterized::Interface* params, const char* name) :
	mModule(module),
	mName(name),
	mParams((FieldBoundaryAssetParameters*)params),
	mDefaultActorParams(NULL),
	mDefaultPreviewParams(NULL)
{
	list.add(*this);
	loadAllMembersFromParams(mParams);
}

//set all of the data members to the default values
void FieldBoundaryAsset::initAllMembers(void)
{
	mDefScale = physx::PxVec3(1.0f, 1.0f, 1.0f);
	mFlag = NX_APEX_FFB_EXCLUDE;
	mShapes.clear();
	mConvex.clear();
}

void FieldBoundaryAsset::loadAllMembersFromParams(const FieldBoundaryAssetParameters* params)
{
	NxParameterized::Handle handle(*params), memberHandle(*params);
	physx::PxI32 numArrayEntries;
	NxParameterized::Interface* refPtr;
	ApexSimpleString tmpStr;
	FieldShapeDesc myFieldShapeDesc;
	ConvexMeshDesc myConvexMeshDesc;

	NxParameterized::ErrorType err = NxParameterized::ERROR_NONE;
	PX_UNUSED(err);

	// read the include / exclude field boundary enum flag.
	err = params->getParameterHandle("includeExcludeFlag", handle);
	PX_ASSERT(err == NxParameterized::ERROR_NONE);

	mFlag = (NxApexFieldBoundaryFlag) handle.parameterDefinition()->enumValIndex(params->includeExcludeFlag);

	// process the various shapes
	err = params->getParameterHandle("geometryType", handle);
	PX_ASSERT(err == NxParameterized::ERROR_NONE);
	params->getArraySize(handle, numArrayEntries, 0);
	for (physx::PxI32 i = 0; i < numArrayEntries; i++)
	{
		handle.getChildHandle(i, memberHandle);
		params->getParamRef(memberHandle, refPtr);
		PX_ASSERT(refPtr);
		if (!refPtr)
		{
			APEX_INTERNAL_ERROR("No force field boundary geometry specified!");
			return;
		}
		myFieldShapeDesc.setToDefault();
		tmpStr = refPtr->className();
		if (tmpStr == ShapeBoxParams::staticClassName())
		{
			myFieldShapeDesc.type		= NX_APEX_SHAPE_BOX;
			ffShapeBox* exp				= PX_NEW(ffShapeBox)(refPtr);
			myFieldShapeDesc.pose		= exp->mPose;
			myFieldShapeDesc.dimensions	= exp->mDimensions;
			mDefScale.x = mDefScale.y = mDefScale.z = exp->mScale;
			PX_DELETE(exp);
			mShapes.pushBack(myFieldShapeDesc);
		}
		else if (tmpStr == ShapeCapsuleParams::staticClassName())
		{
			myFieldShapeDesc.type	= NX_APEX_SHAPE_CAPSULE;
			ffShapeCapsule* exp		= PX_NEW(ffShapeCapsule)(refPtr);
			myFieldShapeDesc.pose	= exp->mPose;
			myFieldShapeDesc.height	= exp->mHeight;
			myFieldShapeDesc.radius	= exp->mRadius;
			mDefScale.x = mDefScale.y = mDefScale.z = exp->mScale;
			PX_DELETE(exp);
			mShapes.pushBack(myFieldShapeDesc);
		}
		else if (tmpStr == ShapeSphereParams::staticClassName())
		{
			myFieldShapeDesc.type	= NX_APEX_SHAPE_SPHERE;
			ffShapeSphere* exp		= PX_NEW(ffShapeSphere)(refPtr);
			myFieldShapeDesc.pose	= exp->mPose;
			myFieldShapeDesc.radius	= exp->mRadius;
			mDefScale.x = mDefScale.y = mDefScale.z = exp->mScale;
			PX_DELETE(exp);
			mShapes.pushBack(myFieldShapeDesc);
		}
		else if (tmpStr == ShapeConvexParams::staticClassName())
		{
			ffShapeConvex* exp		= PX_NEW(ffShapeConvex)(refPtr);

			myFieldShapeDesc.type	= NX_APEX_SHAPE_CONVEX;
			myFieldShapeDesc.pose	= exp->mPose;
			myFieldShapeDesc.mesh	= &myConvexMeshDesc;

			mDefScale.x = mDefScale.y = mDefScale.z = exp->mScale;

			myConvexMeshDesc.flags = NX_CF_COMPUTE_CONVEX;
			myConvexMeshDesc.numPoints = exp->mPoints.size();
			myConvexMeshDesc.pointStrideBytes = sizeof(physx::PxVec3);
			for (PxU32 j = 0; j < myConvexMeshDesc.numPoints; j++)
			{
				physx::PxVec3 p;
				p = exp->mPoints[j];
				myConvexMeshDesc.points.pushBack(p);
			}

			PX_DELETE(exp);

			mShapes.pushBack(myFieldShapeDesc);
			mConvex.pushBack(myConvexMeshDesc);
		}
	}
}

FieldBoundaryAsset::~FieldBoundaryAsset()
{
}

void FieldBoundaryAsset::destroy()
{
	if (mParams)
	{
		mParams->destroy();
		mParams = 0;
	}

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

	delete this;
}

PxU32 FieldBoundaryAsset::forceLoadAssets()
{
	//force fields do not load anyother assets so nothing to do here but return 0
	PxU32 loadedAssetCount = 0;
	return loadedAssetCount;
}

NxParameterized::Interface* FieldBoundaryAsset::getDefaultActorDesc()
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
		const char* className = FieldBoundaryActorParameters::staticClassName();
		NxParameterized::Interface* param = traits->createNxParameterized(className);
		mDefaultActorParams = static_cast<FieldBoundaryActorParameters*>(param);

		PX_ASSERT(param);
		if (!param)
		{
			return NULL;
		}
	}
	else
	{
		mDefaultActorParams->initDefaults();
	}

	return mDefaultActorParams;
}

NxParameterized::Interface* FieldBoundaryAsset::getDefaultAssetPreviewDesc()
{
	NxParameterized::Traits* traits = NiGetApexSDK()->getParameterizedTraits();
	PX_ASSERT(traits);
	if (!traits)
	{
		return NULL;
	}

	if (!mDefaultPreviewParams)
	{
		const char* const className = FieldBoundaryPreviewParameters::staticClassName();
		NxParameterized::Interface* param = traits->createNxParameterized(className);
		mDefaultPreviewParams = static_cast<FieldBoundaryPreviewParameters*>(param);
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

NxApexActor* FieldBoundaryAsset::createApexActor(const NxParameterized::Interface& parms, NxApexScene& apexScene)
{
	if (!isValidForActorCreation(parms, apexScene))
	{
		return NULL;
	}

	NxApexActor* ret = 0;

	const char* className = parms.className();
	if (strcmp(className, FieldBoundaryActorParameters::staticClassName()) == 0)
	{
		NxFieldBoundaryActorDesc desc;
		const FieldBoundaryActorParameters* pDesc = static_cast<const FieldBoundaryActorParameters*>(&parms);

		desc.scale			= pDesc->scale;
		desc.initialPose	= pDesc->initialPose;

		ret = createFieldBoundaryActor(desc, apexScene);
	}

	return ret;
}

NxApexAssetPreview* FieldBoundaryAsset::createApexAssetPreview(const NxParameterized::Interface& params, NxApexAssetPreviewScene* previewScene)
{
	PX_UNUSED(previewScene);
	if (strcmp(params.className(), FieldBoundaryPreviewParameters::staticClassName()) == 0)
	{
		const FieldBoundaryPreviewParameters* const pDesc = static_cast<const FieldBoundaryPreviewParameters * const>(&params);
		PX_ASSERT(mModule);
		PX_ASSERT(mModule->mSdk);
		PX_ASSERT(this);
		PX_ASSERT(pDesc);
		return PX_NEW(FieldBoundaryAssetPreview)(*mModule->mSdk, *this, *pDesc);
	}
	else
	{
		return NULL;
	}
}

void FieldBoundaryAsset::releaseFieldBoundaryPreview(NxFieldBoundaryPreview& nxp)
{
	FieldBoundaryAssetPreview* ap = DYNAMIC_CAST(FieldBoundaryAssetPreview*)(&nxp);
	ap->destroy();
}

NxFieldBoundaryActor* FieldBoundaryAsset::createFieldBoundaryActor(const NxFieldBoundaryActorDesc& desc, const NxApexScene& scene)
{
	NxFieldBoundaryActor* myFieldBoundaryActorPtr;

	if (!desc.isValid())
	{
		return NULL;
	}

	FieldBoundaryScene* fs = mModule->getFieldBoundaryScene(scene);
	myFieldBoundaryActorPtr = PX_NEW(FieldBoundaryActor)(desc, *this, mFieldBoundaryActors, *fs);
	return(myFieldBoundaryActorPtr);
}

NxFieldBoundaryActor* FieldBoundaryAsset::getActor(physx::PxU32 index) const
{
	if (index >= mFieldBoundaryActors.getSize())
	{
		return NULL;
	}

	FieldBoundaryActor* fba = DYNAMIC_CAST(FieldBoundaryActor*)(mFieldBoundaryActors.getResource(index));
	return static_cast<NxFieldBoundaryActor*>(fba);
}

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
