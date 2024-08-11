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

#include "NxExplosionPreview.h"
#include "ExplosionAssetPreview.h"
#include "ModulePerfScope.h"
#include "PsShare.h"

namespace physx
{
namespace apex
{
namespace explosion
{

using namespace APEX_EXPLOSION;

#ifndef WITHOUT_DEBUG_VISUALIZE

static const ExplosionAssetPreview::point2 iconFrame[] =
{
	{ -0.50f, -0.50f}, { +0.50f, -0.50f}, { +0.50f, +0.50f}, { -0.50f, 0.50f}, { -0.50f, -0.50f}
};

static const ExplosionAssetPreview::point2 iconBombCircle[] =
{
	{ +0.35000f, +0.00000f}, { +0.33807f, +0.09059f}, { +0.30311f, +0.17500f}, { +0.24749f, +0.24749f}, { +0.17500f, +0.30311f},
	{ +0.09059f, +0.33807f}, { +0.00000f, +0.35000f}, { -0.09059f, +0.33807f}, { -0.17500f, +0.30311f}, { -0.24749f, +0.24749f},
	{ -0.30311f, +0.17500f}, { -0.33807f, +0.09059f}, { -0.35000f, +0.00000f}, { -0.33807f, -0.09059f}, { -0.30311f, -0.17500f},
	{ -0.24749f, -0.24749f}, { -0.17500f, -0.30311f}, { -0.09059f, -0.33807f}, { +0.00000f, -0.35000f}, { +0.09059f, -0.33807f},
	{ +0.17500f, -0.30311f}, { +0.24749f, -0.24749f}, { +0.30311f, -0.17500f}, { +0.33807f, -0.09059f}, { +0.35000f, +0.00000f}
};

static const ExplosionAssetPreview::point2 iconFuse[] =
{
	{ +0.00000f, +0.35000f}, { +0.00000f, +0.36500f}, { +0.00500f, +0.38000f}, { +0.02500f, +0.41000f}, { +0.05000f, +0.41500f}
};
static const ExplosionAssetPreview::point2 iconSpark1[] =
{
	{ +0.0500f, +0.4300f}, { +0.0500f, +0.4550f}
};
static const ExplosionAssetPreview::point2 iconSpark2[] =
{
	{ +0.0650f, +0.4150f}, { +0.0900f, +0.4150f}
};
static const ExplosionAssetPreview::point2 iconSpark3[] =
{
	{ +0.0500f, +0.4000f}, { +0.0500f, +0.3700f}
};
static const ExplosionAssetPreview::point2 iconSpark4[] =
{
	{ +0.0350f, +0.4300f}, { +0.0200f, +0.4400f}
};
static const ExplosionAssetPreview::point2 iconSpark5[] =
{
	{ +0.0600f, +0.4300f}, { +0.0750f, +0.4450f}
};
static const ExplosionAssetPreview::point2 iconSpark6[] =
{
	{ +0.0600f, +0.4000f}, { +0.0750f, +0.3850f}
};
static const ExplosionAssetPreview::point2 iconSpark7[] =
{
	{ +0.0350f, +0.4000f}, { +0.0200f, +0.3850f}
};

#endif

void ExplosionAssetPreview::drawMultilinePoint2(const point2* pts, physx::PxU32 numPoints, physx::PxU32 color)
{
	physx::PxU32 i;
	physx::PxVec3 p1, p2;

	PX_ASSERT(numPoints > 1);
	mApexRenderDebug->pushRenderState();

	for (i = 0; i < numPoints - 1; i++)
	{
		p1.x = -pts->x;
		p1.y = 0.0f;
		p1.z = pts->y;
		pts++;
		p2.x = -pts->x;
		p2.y = 0.0f;
		p2.z = pts->y;

		mApexRenderDebug->setCurrentColor(color);

		if (mDrawWithCylinder)
		{
			//draw with cylinders - makes it look BOLD.
			mApexRenderDebug->addToCurrentState(physx::DebugRenderState::SolidShaded);
			mApexRenderDebug->debugCylinder(p1, p2, .01f);
			mApexRenderDebug->removeFromCurrentState(physx::DebugRenderState::SolidShaded);
		}
		else
		{
			mApexRenderDebug->debugLine(p1, p2);
		}
	}
	mApexRenderDebug->popRenderState();
}

void ExplosionAssetPreview::drawIcon(void)
{
	PX_PROFILER_PERF_SCOPE("ExplosionDrawIcon");
#ifndef WITHOUT_DEBUG_VISUALIZE
	drawMultilinePoint2(iconFrame,		PX_ARRAY_SIZE(iconFrame),		mApexRenderDebug->getDebugColor(physx::DebugColors::White));
	drawMultilinePoint2(iconBombCircle,	PX_ARRAY_SIZE(iconBombCircle),	mApexRenderDebug->getDebugColor(physx::DebugColors::Black));
	drawMultilinePoint2(iconFuse,		PX_ARRAY_SIZE(iconFuse),		mApexRenderDebug->getDebugColor(physx::DebugColors::Black));
	drawMultilinePoint2(iconSpark1,		PX_ARRAY_SIZE(iconSpark1),		mApexRenderDebug->getDebugColor(physx::DebugColors::Red));
	drawMultilinePoint2(iconSpark2,		PX_ARRAY_SIZE(iconSpark2),		mApexRenderDebug->getDebugColor(physx::DebugColors::Red));
	drawMultilinePoint2(iconSpark3,		PX_ARRAY_SIZE(iconSpark3),		mApexRenderDebug->getDebugColor(physx::DebugColors::Red));
	drawMultilinePoint2(iconSpark4,		PX_ARRAY_SIZE(iconSpark4),		mApexRenderDebug->getDebugColor(physx::DebugColors::Red));
	drawMultilinePoint2(iconSpark5,		PX_ARRAY_SIZE(iconSpark5),		mApexRenderDebug->getDebugColor(physx::DebugColors::Red));
	drawMultilinePoint2(iconSpark6,		PX_ARRAY_SIZE(iconSpark6),		mApexRenderDebug->getDebugColor(physx::DebugColors::Red));
	drawMultilinePoint2(iconSpark7,		PX_ARRAY_SIZE(iconSpark7),		mApexRenderDebug->getDebugColor(physx::DebugColors::Red));
#endif
}

void ExplosionAssetPreview::drawExplosionPreviewIcon(void)
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mApexRenderDebug)
	{
		return;
	}

	physx::PxMat34Legacy scaledPose;	// scaling pose for the preview rendering items that need to be scaled
	physx::PxMat33Legacy scaleit;

	//asset preview init
	bool isPreviewInit = false;
	static physx::PxU32 currentDetail = 0;
	if (mDrawGroupIconScaled == 0)
	{
		mDrawWithCylinder = false;
		mDrawGroupIconScaled = mApexRenderDebug->beginDrawGroup(physx::PxMat44().createIdentity());
		drawIcon();
		mApexRenderDebug->endDrawGroup();
		isPreviewInit = true;
	}

	//asset preview set pose
	{
		scaledPose = mPose;
		scaleit = physx::PxMat33Legacy(physx::PxVec3(mIconScale,	0.0f,		0.0f),
		                               physx::PxVec3(0.0f,			mIconScale,	0.0f),
		                               physx::PxVec3(0.0f,			0.0f,		mIconScale));
		scaledPose.t.y = 2.0f;
		scaledPose.M = scaledPose.M * scaleit;
		mApexRenderDebug->setDrawGroupPose(mDrawGroupIconScaled, scaledPose);
	}

	//asset preview set visibility
	if (currentDetail != mPreviewDetail || isPreviewInit)
	{
		currentDetail = mPreviewDetail;
		if (mPreviewDetail & EXPLOSION_DRAW_ICON)
		{
			mApexRenderDebug->setDrawGroupVisible(mDrawGroupIconScaled, true);
		}
		else
		{
			mApexRenderDebug->setDrawGroupVisible(mDrawGroupIconScaled, false);
		}
	}
#endif
}
void ExplosionAssetPreview::drawExplosionBoundaries(void)
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mApexRenderDebug)
	{
		return;
	}

	physx::PxMat34Legacy scaledPose;	// scaling pose for the preview rendering items that need to be scaled
	physx::PxMat33Legacy scaleit;

	const ExplosionAssetParam& myExplosion = mAsset->getExplosionParameters();

	//asset preview init
	bool isPreviewInit = false;
	static physx::PxU32 currentDetail = 0;
	physx::PxU32 drawGroupUniform = mDrawGroupFixed;
	physx::PxU32 drawGroupNear = mDrawGroupFixed + 1;
	physx::PxU32 drawGroupFar = mDrawGroupFixed + 2;

	if (mDrawGroupFixed == 0)
	{
		drawGroupUniform = mApexRenderDebug->beginDrawGroup(physx::PxMat44().createIdentity());
		mApexRenderDebug->setCurrentColor(mApexRenderDebug->getDebugColor(physx::DebugColors::Red));
		mApexRenderDebug->debugDetailedSphere(physx::PxVec3(0.0f, 0.0f, 0.0f),
		                                      myExplosion.nonUniformParams.distanceTarget * myExplosion.nonUniformParams.distanceScale,
		                                      16);
		mApexRenderDebug->endDrawGroup();

		drawGroupNear = mApexRenderDebug->beginDrawGroup(physx::PxMat44().createIdentity());
		mApexRenderDebug->setCurrentColor(mApexRenderDebug->getDebugColor(physx::DebugColors::Yellow));
		mApexRenderDebug->debugDetailedSphere(physx::PxVec3(0.0f, 0.0f, 0.0f),
		                                      myExplosion.nearDistance,
		                                      16);
		mApexRenderDebug->endDrawGroup();

		drawGroupFar = mApexRenderDebug->beginDrawGroup(physx::PxMat44().createIdentity());
		mApexRenderDebug->setCurrentColor(mApexRenderDebug->getDebugColor(physx::DebugColors::Orange));
		mApexRenderDebug->debugDetailedSphere(physx::PxVec3(0.0f, 0.0f, 0.0f),
		                                      myExplosion.farDistance,
		                                      16);
		mApexRenderDebug->endDrawGroup();

		mDrawGroupFixed = drawGroupUniform;	//represents 3 draw groups
		isPreviewInit = true;
	}

	//asset preview set pose
	{
		physx::PxMat44 groupPose = mPose;
		mApexRenderDebug->setDrawGroupPose(drawGroupUniform, groupPose);
		mApexRenderDebug->setDrawGroupPose(drawGroupNear, groupPose);
		mApexRenderDebug->setDrawGroupPose(drawGroupFar, groupPose);
	}

	//asset preview set visibility
	if (currentDetail != mPreviewDetail || isPreviewInit)
	{
		currentDetail = mPreviewDetail;
		if (mPreviewDetail & EXPLOSION_DRAW_BOUNDARIES)
		{
			mApexRenderDebug->setDrawGroupVisible(drawGroupUniform, myExplosion.useUniformField);
			mApexRenderDebug->setDrawGroupVisible(drawGroupNear, myExplosion.useNearDistance);
			mApexRenderDebug->setDrawGroupVisible(drawGroupFar, myExplosion.useFarDistance);
		}
		else
		{
			mApexRenderDebug->setDrawGroupVisible(drawGroupUniform, false);
			mApexRenderDebug->setDrawGroupVisible(drawGroupNear, false);
			mApexRenderDebug->setDrawGroupVisible(drawGroupFar, false);
		}
	}
#endif
}

void ExplosionAssetPreview::drawExplosionWithCylinder()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mApexRenderDebug)
	{
		return;
	}

	physx::PxMat34Legacy scaledPose;	// scaling pose for the preview rendering items that need to be scaled
	physx::PxMat33Legacy scaleit;

	//asset preview init
	bool isPreviewInit = false;
	static physx::PxU32 currentDetail = 0;
	if (mDrawGroupCylinder == 0)
	{
		mDrawWithCylinder = true;
		scaledPose = mPose;
		mDrawGroupCylinder = mApexRenderDebug->beginDrawGroup(physx::PxMat44().createIdentity());
		drawIcon();
		mApexRenderDebug->endDrawGroup();
		isPreviewInit = true;
	}

	//asset preview set pose
	{
		scaledPose = mPose;
		scaleit = physx::PxMat33Legacy(physx::PxVec3(mIconScale,	0.0f,		0.0f),
		                               physx::PxVec3(0.0f,			mIconScale,	0.0f),
		                               physx::PxVec3(0.0f,			0.0f,		mIconScale));
		scaledPose.t.y = 2.0f;
		scaledPose.M = scaledPose.M * scaleit;
		mApexRenderDebug->setDrawGroupPose(mDrawGroupCylinder, scaledPose);
	}

	//asset preview set visibility
	if (currentDetail != mPreviewDetail || isPreviewInit)
	{
		currentDetail = mPreviewDetail;
		if (mPreviewDetail & EXPLOSION_DRAW_WITH_CYLINDERS)
		{
			mApexRenderDebug->setDrawGroupVisible(mDrawGroupIconScaled, false);
			mApexRenderDebug->setDrawGroupVisible(mDrawGroupCylinder, true);
		}
		else
		{
			mApexRenderDebug->setDrawGroupVisible(mDrawGroupCylinder, false);
		}
	}
#endif
}

void ExplosionAssetPreview::drawExplosionPreview(void)
{
	PX_PROFILER_PERF_SCOPE("ExplosionDrawExplosionPreview");

	drawExplosionPreviewIcon();
	drawExplosionBoundaries();
	drawExplosionWithCylinder();
}

ExplosionAssetPreview::~ExplosionAssetPreview(void)
{
	if (mApexRenderDebug)
	{
		mApexRenderDebug->reset(mDrawGroupIconScaled);
		mApexRenderDebug->reset(mDrawGroupFixed);
		mApexRenderDebug->reset(mDrawGroupFixed + 1);
		mApexRenderDebug->reset(mDrawGroupFixed + 2);
		mApexRenderDebug->reset(mDrawGroupCylinder);
		mApexRenderDebug->release();
		mApexRenderDebug = NULL;
	}
}

void ExplosionAssetPreview::setPose(const physx::PxMat44& pose)
{
	mPose = pose;
	if (mApexRenderDebug)
	{
		mApexRenderDebug->setPose(pose);
	}
}

const physx::PxMat44 ExplosionAssetPreview::getPose() const
{
	return(mPose);
}

// from NxApexRenderDataProvider
void ExplosionAssetPreview::lockRenderResources(void)
{
	ApexRenderable::renderDataLock();
}

void ExplosionAssetPreview::unlockRenderResources(void)
{
	ApexRenderable::renderDataUnLock();
}

void ExplosionAssetPreview::updateRenderResources(bool /*rewriteBuffers*/, void* /*userRenderData*/)
{
	if (mApexRenderDebug)
	{
		mApexRenderDebug->updateRenderResources();
	}
}

// from NxApexRenderable.h
void ExplosionAssetPreview::dispatchRenderResources(NxUserRenderer& renderer)
{
	if (mApexRenderDebug)
	{
		mApexRenderDebug->dispatchRenderResources(renderer);
	}
}

physx::PxBounds3 ExplosionAssetPreview::getBounds(void) const
{
	if (mApexRenderDebug)
	{
		return(mApexRenderDebug->getBounds());
	}
	physx::PxBounds3 b;
	b.setEmpty();
	return b;
}

void ExplosionAssetPreview::destroy(void)
{
	delete this;
}

void ExplosionAssetPreview::release(void)
{
	if (mInRelease)
	{
		return;
	}
	mAsset->releaseExplosionPreview(*this);
}

ExplosionAssetPreview::ExplosionAssetPreview(const NxExplosionPreviewDesc& PreviewDesc, NxApexSDK* myApexSDK, ExplosionAsset* myExplosionAsset) :
	mPose(PreviewDesc.mPose),
	mApexSDK(myApexSDK),
	mAsset(myExplosionAsset),
	mDrawGroupFixed(0),
	mDrawGroupIconScaled(0),
	mDrawGroupCylinder(0),
	mPreviewDetail(PreviewDesc.mPreviewDetail),
	mIconScale(1.0f)
{
	mPose.getInverse(mInversePose);
	mApexRenderDebug = mApexSDK->createApexRenderDebug();
	drawExplosionPreview();
};

}
}
} // end namespace physx::apex

#endif
