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

#ifndef __VORTEX_FSPREVIEW_H__
#define __VORTEX_FSPREVIEW_H__

#include "ApexPreview.h"

#include "NiAPexSDK.h"
#include "NxVortexFSPreview.h"
#include "NxApexRenderDebug.h"

namespace physx
{
namespace apex
{
namespace basicfs
{

class VortexFSAsset;

/**
\brief Descriptor for a VortexFS Preview Asset
*/
class NxVortexFSPreviewDesc
{
public:
	NxVortexFSPreviewDesc() :
		mPose(physx::PxMat44()),
		mPreviewDetail()
	{
		mPose = PxMat44::createIdentity();
	};

	/**
	\brief The pose that translates from preview coordinates to world coordinates.
	*/
	physx::PxMat44			mPose;
	/**
	\brief Radius of the attractor.
	*/
	physx::PxF32			mRadius;
	/**
	\brief The detail options of the preview drawing
	*/
	physx::PxU32			mPreviewDetail;
};

/*
	APEX asset preview asset.
	Preview.
*/
class VortexFSAssetPreview : public NxVortexFSPreview, public ApexResource, public ApexPreview
{
public:
	VortexFSAssetPreview(const NxVortexFSPreviewDesc& PreviewDesc, NxApexSDK* myApexSDK, VortexFSAsset* myAsset, NxApexAssetPreviewScene* previewScene);
	void					drawVortexFSPreview(void);
	void					destroy();

	PxF32					getVortexRadius(NxParameterized::Interface* assetParams);

	void					setPose(const physx::PxMat44& pose);	// Sets the preview instance's pose.  This may include scaling.
	const physx::PxMat44	getPose() const;

	void					setRadius(PxF32 radius);
	const PxF32				getRadius() const;

	void					setDetailLevel(physx::PxU32 detail);

	// from NxApexRenderDataProvider
	void lockRenderResources(void);
	void unlockRenderResources(void);
	void updateRenderResources(bool rewriteBuffers = false, void* userRenderData = 0);

	// from NxApexRenderable.h
	void dispatchRenderResources(NxUserRenderer& renderer);
	PxBounds3 getBounds(void) const;

	// from NxApexInterface.h
	void	release(void);

private:
	~VortexFSAssetPreview();

	physx::PxMat44					mPose;						// the pose for the preview rendering
	NxApexSDK*						mApexSDK;					// pointer to the APEX SDK
	VortexFSAsset*				mAsset;						// our parent VortexFS Asset
	NxApexRenderDebug*				mApexRenderDebug;			// Pointer to the RenderLines class to draw the
	NxApexAssetPreviewScene*		mPreviewScene;
																// preview stuff
	physx::PxF32					mRadius;					// the radius of the attractor
	physx::PxU32					mPreviewDetail;				// the detail options of the preview drawing

	physx::PxU32					mDrawGroupBox;

	void drawPreviewShape();
	void drawPreviewAssetInfo();
	void toggleDrawPreview();
	void setDrawGroupsPose();

	void drawInfoLine(physx::PxU32 lineNum, const char* str);
};

}
}
} // namespace physx::apex

#endif // __TURBULENCE_ASSET_PREVIEW_H__
