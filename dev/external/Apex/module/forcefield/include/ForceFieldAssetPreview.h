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

#ifndef __FORCEFIELD_ASSET_PREVIEW_H__
#define __FORCEFIELD_ASSET_PREVIEW_H__

#include "ApexPreview.h"

#include "NiAPexSDK.h"
#include "NxForceFieldPreview.h"
#include "NxApexRenderDebug.h"
#include "ForceFieldAsset.h"

namespace physx
{
namespace apex
{

namespace forcefield
{

/*
	APEX asset preview explosion asset.
	Preview.
*/
class ForceFieldAssetPreview : public NxForceFieldPreview, public ApexResource, public ApexPreview
{
public:
	ForceFieldAssetPreview(const NxForceFieldPreviewDesc& PreviewDesc, NxApexSDK* myApexSDK, ForceFieldAsset* myForceFieldAsset, NxApexAssetPreviewScene* previewScene);
	void					drawForceFieldPreview(void);
	void					drawForceFieldPreviewUnscaled(void);
	void					drawForceFieldPreviewScaled(void);
	void					drawForceFieldPreviewIcon(void);
	void					drawForceFieldBoundaries(void);
	void					drawForceFieldWithCylinder();
	void					destroy();

	void					setPose(const physx::PxMat44& pose);	// Sets the preview instance's pose.  This may include scaling.
	const physx::PxMat44	getPose() const;

	// from NxApexRenderDataProvider
	void					lockRenderResources(void);
	void					unlockRenderResources(void);
	void					updateRenderResources(bool rewriteBuffers = false, void* userRenderData = 0);

	// from NxApexRenderable.h
	void					dispatchRenderResources(NxUserRenderer& renderer);
	physx::PxBounds3		getBounds(void) const;

	// from NxApexInterface.h
	void					release(void);

	void					setDetailLevel(physx::PxU32 detail);

	typedef struct
	{
		physx::PxF32 x, y;
	} point2;

private:

	~ForceFieldAssetPreview();
	physx::PxMat44					mPose;						// the pose for the preview rendering
	NxApexSDK*						mApexSDK;					// pointer to the APEX SDK
	ForceFieldAsset*				mAsset;						// our parent ForceField Asset
	NxApexRenderDebug*				mApexRenderDebug;			// Pointer to the RenderLines class to draw the
	// preview stuff
	physx::PxU32					mDrawGroupIconScaled;		// the ApexDebugRenderer draw group for the Icon
	physx::PxU32					mDrawGroupCylinder;
	physx::PxU32					mPreviewDetail;				// the detail options of the preview drawing
	physx::PxF32					mIconScale;					// the scale for the icon
	NxApexAssetPreviewScene*		mPreviewScene;

	bool							mDrawWithCylinder;

	void							drawIcon(void);
	void							drawMultilinePoint2(const point2* pts, physx::PxU32 numPoints, physx::PxU32 color);
	void							setIconScale(physx::PxF32 scale);

	void							setPose(physx::PxMat44 pose);

	void							toggleDrawPreview();
	void							setDrawGroupsPoseScaled();
};

}
}
} // end namespace physx::apex

#endif // __FORCEFIELD_ASSET_PREVIEW_H__
