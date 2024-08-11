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

#ifndef __APEX_EMITTER_ASSET_PREVIEW_H__
#define __APEX_EMITTER_ASSET_PREVIEW_H__

#include "ApexPreview.h"

#include "NiApexSDK.h"
#include "NxApexEmitterPreview.h"
#include "NxApexRenderDebug.h"
#include "ApexEmitterAsset.h"

namespace physx
{
namespace apex
{
namespace emitter
{

class ApexEmitterAssetPreview : public NxApexEmitterPreview, public ApexResource, public ApexPreview
{
public:
	ApexEmitterAssetPreview(const NxApexEmitterPreviewDesc& pdesc, const ApexEmitterAsset& asset, NxApexAssetPreviewScene* previewScene, NxApexSDK* myApexSDK);

	bool					isValid() const;

	void					drawEmitterPreview(void);
	void					drawPreviewAssetInfo(void);
	void					drawInfoLine(physx::PxU32 lineNum, const char* str);
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

private:
	~ApexEmitterAssetPreview();

	void					toggleDrawPreview();
	void					setDrawGroupsPose();


	NxAuthObjTypeID					mModuleID;					// the module ID of Emitter.
	NxApexSDK*						mApexSDK;					// pointer to the APEX SDK
	NxApexRenderDebug*				mApexRenderDebug;			// Pointer to the RenderLines class to draw the
	physx::PxMat44					mPose;						// the pose for the preview rendering
	physx::PxF32					mScale;
	const ApexEmitterAsset*         mAsset;
	physx::PxI32                    mGroupID;
	NxApexAssetPreviewScene*		mPreviewScene;

	void							setScale(physx::PxF32 scale);
};

}
}
} // end namespace physx::apex

#endif // __APEX_EMITTER_ASSET_PREVIEW_H__
