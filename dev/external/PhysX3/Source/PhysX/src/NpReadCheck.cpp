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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#include "NpReadCheck.h"

#include "NpScene.h"

using namespace physx;

NpReadCheck::NpReadCheck(const NpScene* scene, const char* functionName)
	: mScene(scene), mName(functionName), mErrorCount(0)
{
	if (mScene)
	{
		if (!mScene->startRead())
		{
			if (mScene->getScene().getFlags() & PxSceneFlag::eREQUIRE_RW_LOCK)
			{
				Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
					"An API read call (%s) was made from thread %d but PxScene::lockRead() was not called first, note that "
					"when PxSceneFlag::eREQUIRE_RW_LOCK is enabled all API reads and writes must be "
					"wrapped in the appropriate locks.", mName, PxU32(Ps::Thread::getId()));
			}
			else
			{
				Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, 
					"Overlapping API read and write call detected during %s from thread %d! Note that read operations to "
					"the SDK must not be overlapped with write calls, else the resulting behavior is undefined.", mName, PxU32(Ps::Thread::getId()));
			}
		}

		mErrorCount = mScene->getReadWriteErrorCount();
	}
}


NpReadCheck::~NpReadCheck()
{
	if (mScene)
	{
		// check if an error occurred during our read, if so 
		// then output our name in order to help debugging
		if (mScene->getReadWriteErrorCount() != mErrorCount && !(mScene->getScene().getFlags() & PxSceneFlag::eREQUIRE_RW_LOCK))
		{
			Ps::getFoundation().error(PxErrorCode::eDEBUG_INFO, __FILE__, __LINE__, 
				"Leaving %s on thread %d, an API overlapping write on another thread was detected.", mName, PxU32(Ps::Thread::getId()));
		}

		mScene->stopRead();
	}
}
