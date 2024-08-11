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

#include "NxParamUtils.h"
#include "AttractorFSAsset.h"
#include "AttractorFSAssetParams.h"
#include "NxAttractorFSPreview.h"
#include "AttractorFSAssetPreview.h"
#include "ModulePerfScope.h"
#include "PsShare.h"

namespace physx
{
namespace apex
{
namespace basicfs
{

using namespace APEX_ATTRACT;

void AttractorFSAssetPreview::drawAttractorFSPreview(void)
{
	PX_PROFILER_PERF_SCOPE("AttractorFSDrawPreview");

	if (mPreviewDetail & ATTRACT_DRAW_SHAPE)
	{
		drawPreviewShape();
	}
}

#define ASSET_INFO_XPOS					(-0.9f)	// left position of the asset info
#define ASSET_INFO_YPOS					( 0.9f)	// top position of the asset info
#define DEBUG_TEXT_HEIGHT				(0.35f)	//in screen space -- would be nice to know this!


void AttractorFSAssetPreview::drawPreviewShape()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mApexRenderDebug)
	{
		return;
	}

	if (mDrawGroupBox == 0)
	{
		mDrawGroupBox = mApexRenderDebug->beginDrawGroup(physx::PxMat44().createIdentity());
		mApexRenderDebug->setCurrentColor(mApexRenderDebug->getDebugColor(physx::DebugColors::DarkGreen));
		mApexRenderDebug->debugSphere(PxVec3(0.0f), mRadius);	// * scale);
		mApexRenderDebug->endDrawGroup();
	}

	setDrawGroupsPose();
#endif
}

void	AttractorFSAssetPreview::toggleDrawPreview()
{
	if (mPreviewDetail & ATTRACT_DRAW_SHAPE)
	{
		mApexRenderDebug->setDrawGroupVisible(mDrawGroupBox, true);
	}
	else
	{
		mApexRenderDebug->setDrawGroupVisible(mDrawGroupBox, false);
	}
}

void	AttractorFSAssetPreview::setDrawGroupsPose()
{
	mApexRenderDebug->setDrawGroupPose(mDrawGroupBox, mPose);
}


void AttractorFSAssetPreview::drawInfoLine(physx::PxU32 lineNum, const char* str)
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(lineNum);
	PX_UNUSED(str);
#else
	mApexRenderDebug->setCurrentColor(mApexRenderDebug->getDebugColor(physx::DebugColors::Green));
	PxMat44 cameraMatrix = mPreviewScene->getCameraMatrix();
	PxVec3 textLocation = mPose.getPosition();
	textLocation += cameraMatrix.column1.getXYZ() * (ASSET_INFO_YPOS - (lineNum * DEBUG_TEXT_HEIGHT));
	cameraMatrix.setPosition(textLocation);
	mApexRenderDebug->debugOrientedText(cameraMatrix, str);
#endif
}

void AttractorFSAssetPreview::drawPreviewAssetInfo()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mApexRenderDebug)
	{
		return;
	}

	char buf[128];
	buf[sizeof(buf) - 1] = 0;

	ApexSimpleString myString;
	ApexSimpleString floatStr;
	physx::PxU32 lineNum = 0;

	mApexRenderDebug->pushRenderState();
	//	mApexRenderDebug->addToCurrentState(physx::DebugRenderState::ScreenSpace);
	mApexRenderDebug->addToCurrentState(physx::DebugRenderState::NoZbuffer);
	mApexRenderDebug->setCurrentTextScale(1.0f);
	mApexRenderDebug->setCurrentColor(mApexRenderDebug->getDebugColor(physx::DebugColors::Yellow));

	// asset name
	APEX_SPRINTF_S(buf, sizeof(buf) - 1, "%s %s", mAsset->getObjTypeName(), mAsset->getName());
	drawInfoLine(lineNum++, buf);
	lineNum++;

	if(mPreviewScene->getShowFullInfo())
	{
		// TODO: cache strings
		AttractorFSAssetParams* assetParams = static_cast<AttractorFSAssetParams*>(mAsset->getAssetNxParameterized());
		PX_ASSERT(assetParams);

		// attractor info
		physx::PxF32 radius					= assetParams->radius;
		physx::PxF32 constFieldStrength		= assetParams->constFieldStrength;
		physx::PxF32 variableFieldStrength	= assetParams->variableFieldStrength;

		myString = "Attractor radius = ";
		ApexSimpleString::ftoa(radius, floatStr);
		myString += floatStr;
		drawInfoLine(lineNum++, myString.c_str());

		myString = "Constant field strength coefficient = ";
		ApexSimpleString::ftoa(constFieldStrength, floatStr);
		myString += floatStr;
		drawInfoLine(lineNum++, myString.c_str());

		myString = "Variable field strength coefficient = ";
		ApexSimpleString::ftoa(variableFieldStrength, floatStr);
		myString += floatStr;
		drawInfoLine(lineNum++, myString.c_str());

		// fieldSampler filter data info
		if (assetParams->fieldSamplerFilterDataName.buf)
		{
			APEX_SPRINTF_S(buf, sizeof(buf) - 1, "FieldSampler Filter Data = %s",
				assetParams->fieldSamplerFilterDataName.buf
				);
			drawInfoLine(lineNum++, buf);
		}

		// fieldBoundary filter data info
		if (assetParams->fieldBoundaryFilterDataName.buf)
		{
			myString = "FieldBoundary Filter Data = ";
			myString += ApexSimpleString(assetParams->fieldBoundaryFilterDataName.buf);
			drawInfoLine(lineNum++, myString.c_str());
		}

		// implicit info
		myString = "Fade Percentage = ";
		ApexSimpleString::ftoa(assetParams->boundaryFadePercentage, floatStr);
		myString += floatStr;
		drawInfoLine(lineNum++, myString.c_str());
	}

	mApexRenderDebug->popRenderState();
#endif
}

AttractorFSAssetPreview::~AttractorFSAssetPreview(void)
{
	if (mApexRenderDebug)
	{
		mApexRenderDebug->reset(mDrawGroupBox);
		mApexRenderDebug->reset();
		mApexRenderDebug->release();
		mApexRenderDebug = NULL;
	}
}

void AttractorFSAssetPreview::setPose(const physx::PxMat44& pose)
{
	mPose = pose;
	setDrawGroupsPose();
}

const physx::PxMat44 AttractorFSAssetPreview::getPose() const
{
	return mPose;
}

void AttractorFSAssetPreview::setRadius(PxF32 radius)
{
	mRadius = radius;
}

const PxF32 AttractorFSAssetPreview::getRadius() const
{
	return mRadius;
}

// from NxApexRenderDataProvider
void AttractorFSAssetPreview::lockRenderResources(void)
{
	ApexRenderable::renderDataLock();
}

void AttractorFSAssetPreview::unlockRenderResources(void)
{
	ApexRenderable::renderDataUnLock();
}

void AttractorFSAssetPreview::updateRenderResources(bool /*rewriteBuffers*/, void* /*userRenderData*/)
{
	if (mApexRenderDebug)
	{
		mApexRenderDebug->updateRenderResources();
	}
}

void AttractorFSAssetPreview::dispatchRenderResources(NxUserRenderer& renderer)
{
	if (mApexRenderDebug)
	{
		if (mPreviewDetail & ATTRACT_DRAW_ASSET_INFO)
		{
			drawPreviewAssetInfo();
		}
		mApexRenderDebug->dispatchRenderResources(renderer);
	}
}

physx::PxBounds3 AttractorFSAssetPreview::getBounds(void) const
{
	if (mApexRenderDebug)
	{
		return(mApexRenderDebug->getBounds());
	}
	else
	{
		physx::PxBounds3 b;
		b.setEmpty();
		return b;
	}
}

void AttractorFSAssetPreview::destroy(void)
{
	delete this;
}

PxF32 AttractorFSAssetPreview::getAttractorRadius(NxParameterized::Interface* assetParams)
{
	PxF32 radius = 0.0f;
	const char* name = "radius";

	NxParameterized::Handle handle(*assetParams, name);
	bool handleIsValid = handle.isValid();
	PX_ASSERT(handleIsValid);
	PX_UNUSED(handleIsValid);
	//APEX_DEBUG_WARNING("Test.");
	NxParameterized::ErrorType errorGetRadius = handle.getParamF32(radius);
	PX_ASSERT(errorGetRadius == NxParameterized::ERROR_NONE);
	PX_UNUSED(errorGetRadius);

	return radius;

	// the other way to do it ...
	//AttractorFSAssetParams* attractorAssetParams = static_cast<AttractorFSAssetParams*>(assetParams);
	//PX_ASSERT(assetParams);

	//return attractorAssetParams->radius;
}

void AttractorFSAssetPreview::release(void)
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	mAsset->releaseAttractorFSPreview(*this);
}

AttractorFSAssetPreview::AttractorFSAssetPreview(const NxAttractorFSPreviewDesc& PreviewDesc, NxApexSDK* myApexSDK, AttractorFSAsset* mAttractorFSAsset, NxApexAssetPreviewScene* previewScene) :
	mPose(PreviewDesc.mPose),
	mApexSDK(myApexSDK),
	mAsset(mAttractorFSAsset),
	mPreviewScene(previewScene),
	mPreviewDetail(PreviewDesc.mPreviewDetail),
	mDrawGroupBox(0)
{
	NxParameterized::Interface* assetParams = mAttractorFSAsset->getAssetNxParameterized(); //FIXME: const
	PX_ASSERT(assetParams);

	if (assetParams)
	{
		mRadius = getAttractorRadius(assetParams);
	}

	mApexRenderDebug = mApexSDK->createApexRenderDebug();
	drawAttractorFSPreview();
};


void AttractorFSAssetPreview::setDetailLevel(physx::PxU32 detail)
{
	if(detail != mPreviewDetail)
	{
		mPreviewDetail = detail;
		toggleDrawPreview();
	}
}

}
}
} // namespace physx::apex

#endif
