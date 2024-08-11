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

#include "NxApex.h"
#include "NxApexDefs.h"
#include "ApexAssetPreviewScene.h"
#include "ApexSceneTasks.h"
#include "ApexSDK.h"
#include "ApexActor.h"
#include "FrameworkPerfScope.h"
#include "ApexRenderDebug.h"
#include "NiModule.h"
#include "PVDBinding.h"
#include "PsTime.h"
#include "NxApexReadWriteLock.h"
#include "PsShare.h"
#include "PsSync.h"
#include "PxTask.h"
#include "PxTaskManager.h"
#include "PxGpuDispatcher.h"
#include "PxCudaContextManager.h"


namespace physx
{
namespace apex
{

	ApexAssetPreviewScene::ApexAssetPreviewScene(ApexSDK* sdk) : mApexSDK(sdk)
	, mShowFullInfo(false)
	{
		mCameraMatrix = mCameraMatrix.createIdentity();
	}

	void ApexAssetPreviewScene::setCameraMatrix(const physx::PxMat44& cameraMatrix)
	{
		mCameraMatrix = cameraMatrix;
	}

	physx::PxMat44 ApexAssetPreviewScene::getCameraMatrix() const
	{
		return mCameraMatrix;
	}

	void ApexAssetPreviewScene::setShowFullInfo(bool showFullInfo)
	{
		mShowFullInfo = showFullInfo;
	}

	bool ApexAssetPreviewScene::getShowFullInfo() const
	{
		return mShowFullInfo;
	}

	void ApexAssetPreviewScene::release()
	{
		mApexSDK->releaseAssetPreviewScene(this);
	}

	void ApexAssetPreviewScene::destroy()
	{
		PX_DELETE(this);
	}
}
} // end namespace physx::apex
