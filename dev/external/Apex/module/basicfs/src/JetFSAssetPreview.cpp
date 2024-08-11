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
#include "JetFSAsset.h"
#include "JetFSAssetParams.h"
#include "NxJetFSPreview.h"
#include "JetFSAssetPreview.h"
#include "ModulePerfScope.h"
#include "PsShare.h"

namespace physx
{
namespace apex
{
namespace basicfs
{

using namespace APEX_JET;

void JetFSAssetPreview::drawJetFSPreview(void)
{
	PX_PROFILER_PERF_SCOPE("JetFSDrawPreview");
	if (mPreviewDetail & JET_DRAW_SHAPE)
	{
		drawShape();
	}

	toggleDrawPreview();
	setDrawGroupsPose();
}

#define ASSET_INFO_XPOS					(-0.9f)	// left position of the asset info
#define ASSET_INFO_YPOS					( 0.9f)	// top position of the asset info
#define DEBUG_TEXT_HEIGHT				(0.35f)	//in screen space -- would be nice to know this!



void JetFSAssetPreview::drawInfoLine(physx::PxU32 lineNum, const char* str)
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(lineNum);
	PX_UNUSED(str);
#else
	PxMat44 cameraMatrix = mPreviewScene->getCameraMatrix();
	mApexRenderDebug->setCurrentColor(mApexRenderDebug->getDebugColor(physx::DebugColors::Blue));
	PxVec3 textLocation = mPose.getPosition(); 
	textLocation += cameraMatrix.column1.getXYZ() * (ASSET_INFO_YPOS - (lineNum * DEBUG_TEXT_HEIGHT));
	cameraMatrix.setPosition(textLocation);
	mApexRenderDebug->debugOrientedText(cameraMatrix, str);
#endif
}

void JetFSAssetPreview::drawPreviewAssetInfo()
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
		mApexRenderDebug->addToCurrentState(physx::DebugRenderState::NoZbuffer);
		mApexRenderDebug->setCurrentTextScale(1.0f);

		// asset name
		APEX_SPRINTF_S(buf, sizeof(buf) - 1, "%s %s", mAsset->getObjTypeName(), mAsset->getName());
		drawInfoLine(lineNum++, buf);
		lineNum++;

		if(mPreviewScene->getShowFullInfo())
		{
			// TODO: cache strings
			JetFSAssetParams& assetParams = *static_cast<JetFSAssetParams*>(mAsset->getAssetNxParameterized());

			APEX_SPRINTF_S(buf, sizeof(buf) - 1, "fieldStrength = %f",
				assetParams.fieldStrength
				);
			drawInfoLine(lineNum++, buf);


			APEX_SPRINTF_S(buf, sizeof(buf) - 1, "fieldStrengthDeviationPercentage = %f",
				assetParams.fieldStrengthDeviationPercentage
				);
			drawInfoLine(lineNum++, buf);


			APEX_SPRINTF_S(buf, sizeof(buf) - 1, "fieldDirectionDeviationAngle = %f",
				assetParams.fieldDirectionDeviationAngle
				);
			drawInfoLine(lineNum++, buf);

			// fieldBoundary filter data info
			if (assetParams.fieldBoundaryFilterDataName.buf)
			{
				myString = "FieldBoundary Filter Data = ";
				myString += ApexSimpleString(assetParams.fieldBoundaryFilterDataName.buf);
				drawInfoLine(lineNum++, myString.c_str());
			}

			// implicit info
			myString = "Fade Percentage = ";
			ApexSimpleString::ftoa(assetParams.boundaryFadePercentage, floatStr);
			myString += floatStr;
			drawInfoLine(lineNum++, myString.c_str());
		}
		mApexRenderDebug->popRenderState();
#endif
}

void JetFSAssetPreview::drawShape()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mApexRenderDebug)
	{
		return;
	}

	JetFSAssetParams& assetParams = *static_cast<JetFSAssetParams*>(mAsset->getAssetNxParameterized());
	//asset preview init
	if (mDrawGroupShape == 0)
	{
		mDrawGroupShape = mApexRenderDebug->beginDrawGroup(physx::PxMat44().createIdentity());

		//debug visualization
		mApexRenderDebug->setCurrentColor(mApexRenderDebug->getDebugColor(physx::DebugColors::DarkBlue));

		

		PxMat44 dirToWorld = PxMat44(PxVec4(mDirToWorld.column0, 0), PxVec4(mDirToWorld.column1, 0), PxVec4(mDirToWorld.column2, 0),  PxVec4(0, 0, 0, 1));
		//dirToWorld.setPosition(mPose.getPosition());
		mApexRenderDebug->debugOrientedCapsule(assetParams.farRadius, assetParams.farRadius * assetParams.directionalStretch, 2, dirToWorld);
		mApexRenderDebug->debugOrientedCapsule(assetParams.gridShapeRadius, assetParams.gridShapeHeight, 2, dirToWorld);
		mApexRenderDebug->endDrawGroup();
	}

	if(mDrawGroupTorus == 0)
	{
		//draw torus
		//mApexRenderDebug->setPose(mPose);???

		mDrawGroupTorus = mApexRenderDebug->beginDrawGroup(physx::PxMat44().createIdentity());
		mApexRenderDebug->setCurrentColor(mApexRenderDebug->getDebugColor(physx::DebugColors::DarkBlue));
		const PxU32 NUM_PHI_SLICES = 16;
		const PxU32 NUM_THETA_SLICES = 16;

		const PxF32 torusRadius = assetParams.farRadius / 2;

		PxF32 cosPhiLast = 1;
		PxF32 sinPhiLast = 0;
		for (PxU32 i = 1; i <= NUM_PHI_SLICES; ++i)
		{
			PxF32 phi = (i * PxTwoPi / NUM_PHI_SLICES);
			PxF32 cosPhi = PxCos(phi);
			PxF32 sinPhi = PxSin(phi);

			mApexRenderDebug->debugLine(
				mDirToWorld * PxVec3(cosPhiLast * assetParams.pivotRadius, 0, sinPhiLast * assetParams.pivotRadius),
				mDirToWorld * PxVec3(cosPhi * assetParams.pivotRadius, 0, sinPhi * assetParams.pivotRadius));

			mApexRenderDebug->debugLine(
				mDirToWorld * PxVec3(cosPhiLast * assetParams.nearRadius, 0, sinPhiLast * assetParams.nearRadius),
				mDirToWorld * PxVec3(cosPhi * assetParams.nearRadius, 0, sinPhi * assetParams.nearRadius));

			PxF32 cosThetaLast = 1;
			PxF32 sinThetaLast = 0;
			for (PxU32 j = 1; j <= NUM_THETA_SLICES; ++j)
			{
				PxF32 theta = (j * PxTwoPi / NUM_THETA_SLICES);
				PxF32 cosTheta = PxCos(theta);
				PxF32 sinTheta = PxSin(theta);

				PxF32 d = torusRadius * (1 + cosTheta);
				PxF32 h = torusRadius * sinTheta * assetParams.directionalStretch;

				PxF32 dLast = torusRadius * (1 + cosThetaLast);
				PxF32 hLast = torusRadius * sinThetaLast * assetParams.directionalStretch;

				mApexRenderDebug->debugLine(
					mDirToWorld * PxVec3(cosPhi * dLast, hLast, sinPhi * dLast),
					mDirToWorld * PxVec3(cosPhi * d, h, sinPhi * d));

				mApexRenderDebug->debugLine(
					mDirToWorld * PxVec3(cosPhiLast * d, h, sinPhiLast * d),
					mDirToWorld * PxVec3(cosPhi * d, h, sinPhi * d));

				mApexRenderDebug->debugLine(
					mDirToWorld * PxVec3(cosPhiLast * dLast, hLast, sinPhiLast * dLast),
					mDirToWorld * PxVec3(cosPhi * dLast, hLast, sinPhi * dLast));

				cosThetaLast = cosTheta;
				sinThetaLast = sinTheta;
			}

			cosPhiLast = cosPhi;
			sinPhiLast = sinPhi;
		}
		mApexRenderDebug->endDrawGroup();
	}
#endif		
}

JetFSAssetPreview::~JetFSAssetPreview(void)
{
	if (mApexRenderDebug)
	{
		mApexRenderDebug->reset(mDrawGroupShape);
		mApexRenderDebug->reset(mDrawGroupTorus);
		mApexRenderDebug->reset();
		mApexRenderDebug->release();
		mApexRenderDebug = NULL;
	}
}

void JetFSAssetPreview::setPose(const physx::PxMat44& pose)
{
	mPose = pose;
	setDrawGroupsPose();
}

const physx::PxMat44 JetFSAssetPreview::getPose() const
{
	return	mPose;
}

void	JetFSAssetPreview::toggleDrawPreview()
{
	if (mPreviewDetail & JET_DRAW_SHAPE)
	{
		mApexRenderDebug->setDrawGroupVisible(mDrawGroupShape, true);
		mApexRenderDebug->setDrawGroupVisible(mDrawGroupTorus, true);
	}
	else
	{
		mApexRenderDebug->setDrawGroupVisible(mDrawGroupShape, false);
		mApexRenderDebug->setDrawGroupVisible(mDrawGroupTorus, false);
	}
}

void	JetFSAssetPreview::setDrawGroupsPose()
{
	mApexRenderDebug->setDrawGroupPose(mDrawGroupShape, mPose);
	mApexRenderDebug->setDrawGroupPose(mDrawGroupTorus, mPose);
}


// from NxApexRenderDataProvider
void JetFSAssetPreview::lockRenderResources(void)
{
	ApexRenderable::renderDataLock();
}

void JetFSAssetPreview::unlockRenderResources(void)
{
	ApexRenderable::renderDataUnLock();
}

void JetFSAssetPreview::updateRenderResources(bool /*rewriteBuffers*/, void* /*userRenderData*/)
{
	if (mApexRenderDebug)
	{
		mApexRenderDebug->updateRenderResources();
	}
}

// from NxApexRenderable.h
void JetFSAssetPreview::dispatchRenderResources(NxUserRenderer& renderer)
{
	if (mApexRenderDebug)
	{
		if (mPreviewDetail & JET_DRAW_ASSET_INFO)
		{
			drawPreviewAssetInfo();
		}
		mApexRenderDebug->dispatchRenderResources(renderer);
	}
}

physx::PxBounds3 JetFSAssetPreview::getBounds(void) const
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

void JetFSAssetPreview::destroy(void)
{
	delete this;
}

void JetFSAssetPreview::release(void)
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	mAsset->releaseJetFSPreview(*this);
}

JetFSAssetPreview::JetFSAssetPreview(const NxJetFSPreviewDesc& PreviewDesc, NxApexSDK* myApexSDK, JetFSAsset* myJetFSAsset, NxApexAssetPreviewScene* previewScene) :
	mPose(PreviewDesc.mPose),
	mApexSDK(myApexSDK),
	mAsset(myJetFSAsset),
	mPreviewScene(previewScene),
	mPreviewDetail(PreviewDesc.mPreviewDetail),
	mDrawGroupShape(0),
	mDrawGroupTorus(0)
{
	PxMat33 poseRot = PxMat33(mPose.column0.getXYZ(), mPose.column1.getXYZ(), mPose.column2.getXYZ());
	physx::PxVec3 vecN = poseRot.transform(mAsset->mParams->fieldDirection.getNormalized());
	vecN.normalize();
	physx::PxVec3 vecP, vecQ;
	BuildPlaneBasis(vecN, vecP, vecQ);

	mDirToWorld.column0 = vecP;
	mDirToWorld.column1 = vecN;
	mDirToWorld.column2 = vecQ;

	mApexRenderDebug = mApexSDK->createApexRenderDebug();
	drawJetFSPreview();
};


void JetFSAssetPreview::setDetailLevel(physx::PxU32 detail)
{
	mPreviewDetail = detail;
	setDrawGroupsPose();
}

}
}
} // namespace physx::apex

#endif
