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

#ifndef __IOS_OBJECT_DATA_H__
#define __IOS_OBJECT_DATA_H__

#include "PsShare.h"
#include "PsUserAllocated.h"
#include "PsSync.h"
#include "foundation/PxVec3.h"
#include "foundation/PxVec4.h"
#include "NiIofxManager.h"
#include "PxMat33Legacy.h"
#include "ApexMirroredArray.h"
#include "ApexMath.h"

#include "IofxOutputData.h"
#include "IofxRenderData.h"
#include "ModifierData.h"

namespace physx
{
class PxCudaContextManager;
}

namespace physx
{
namespace apex
{

class NxUserRenderResourceManager;
class NxUserRenderSpriteBuffer;
class NxUserRenderInstanceBuffer;

namespace iofx
{

class IosObjectBaseData : public NiIofxManagerDesc, public NiIosBufferDesc, public physx::UserAllocated
{
public:
	IosObjectBaseData(PxU32 instance, IofxOutputData* iofxOutputData)
		: instanceID(instance)
		, outputData(iofxOutputData)
		, outputSemantics(0)
		, outputDWords(0)
		, renderData(NULL)
		, renderDataUpdate(false)
	{
		outputSync.set();
	}
	virtual ~IosObjectBaseData()
	{
		PX_DELETE(outputData);
	}

	void updateSemantics(PxU32 semantics)
	{
		if (outputSemantics != semantics)
		{
			outputSemantics = semantics;
			outputDWords = outputData->updateSemantics(outputSemantics, maxObjectCount);
		}
	}

	void allocOutputs(physx::PxCudaContextManager* ctx = 0)
	{
		const size_t capacity = maxObjectCount * outputDWords * 4;
		const size_t size = numParticles * outputDWords * 4;

		outputData->alloc(numParticles, capacity, size, ctx);
	}

	void prepareRenderDataUpdate()
	{
		outputSync.reset();
		renderDataUpdate = true;
	}
	void executeRenderDataUpdate()
	{
		if (renderDataUpdate)
		{
			renderData->update(this);

			outputSync.set();
			renderDataUpdate = false;
		}
	}
	void waitForRenderDataUpdate()
	{
		outputSync.wait();
	}

	PX_INLINE ModifierCommonParams getCommonParams() const
	{
		ModifierCommonParams common;

		common.inputHasCollision = iosSupportsCollision;
		common.inputHasDensity = iosSupportsDensity;
		common.upVector = upVector;
		common.eyePosition = eyePosition;
		common.eyeDirection = eyeDirection;
		common.eyeAxisX = eyeAxisX;
		common.eyeAxisY = eyeAxisY;
		common.zNear = zNear;
		common.deltaTime = deltaTime;

		return common;
	}

	const PxU32		instanceID;

	PxVec3			upVector;
	PxF32			radius;
	PxF32			gravity;
	PxF32			restDensity; //!< resting density of simulation

	PxF32			elapsedTime;			//!< total simulation time
	PxF32			deltaTime;				//!< NiApexScene::getElapsedTime()
	PxVec3			eyePosition;			//!< NxApexScene::getEyePosition()
	PxVec3			eyeDirection;			//!< NxApexScene::getEyeDirection()
	PxVec3			eyeAxisX;
	PxVec3			eyeAxisY;
	PxF32			zNear;

	PxU32           maxStateID;				//!< From IOS each frame
	PxU32			maxInputID;				//!< From IOS each frame
	PxU32           numParticles;			//!< From IOS each frame

	bool			writeBufferCalled;
	PxU32           outputSemantics;
	PxU32			outputDWords;

	IofxOutputData* outputData;
	IofxSharedRenderData* renderData;

	bool			renderDataUpdate;
	physx::Sync		outputSync;

private:
	IosObjectBaseData& operator=(const IosObjectBaseData&);
};

class IosObjectCpuData : public IosObjectBaseData
{
public:
	IosObjectCpuData(PxU32 instance, IofxOutputData* iofxOutputData)
		: IosObjectBaseData(instance, iofxOutputData)
		, outputToState(NULL)
		, sortingKeys(NULL)
	{
	}

	IofxSlice**				inPubState;
	IofxSlice**				outPubState;
	PxU32					numPubSlices;

	IofxSlice**				inPrivState;
	IofxSlice**				outPrivState;
	PxU32					numPrivSlices;

	PxU32*					outputToState;

	PxU32*					sortingKeys;
};

#if defined(APEX_CUDA_SUPPORT)
class IosObjectGpuData : public IosObjectBaseData
{
public:
	IosObjectGpuData(PxU32 instance, IofxOutputData* iofxOutputData)
		: IosObjectBaseData(instance, iofxOutputData)
	{
	}
};
#endif

}
}
} // namespace apex

#endif /* __IOS_OBJECT_DATA_H__ */
