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

#include "NxApex.h"
#include "FieldSamplerQuery.h"
#include "FieldSamplerManager.h"
#include "FieldSamplerWrapper.h"
#include "FieldSamplerSceneWrapper.h"
#include "FieldBoundaryWrapper.h"

#include "NiApexScene.h"

#if defined(APEX_CUDA_SUPPORT)
#include "PxGpuTask.h"

#if APEX_CUDA_CHECK_ENABLED
#define CUDA_CHECK(msg) \
	{ \
		physx::PxTaskManager* tm = mManager->getApexScene().getTaskManager(); \
		physx::PxCudaContextManager* ctx = tm->getGpuDispatcher()->getCudaContextManager(); \
		physx::PxScopedCudaLock s(*ctx); \
		CUresult ret = cuCtxSynchronize(); \
		if( CUDA_SUCCESS != ret ) { \
			APEX_INTERNAL_ERROR("Cuda Error %d, %s", ret, msg); \
			PX_ASSERT(!ret); \
		} \
	}
#else
#define CUDA_CHECK(msg)
#endif

#endif

#include "FieldSamplerCommon.h"


namespace physx
{
namespace apex
{
namespace fieldsampler
{


FieldSamplerQuery::FieldSamplerQuery(const NiFieldSamplerQueryDesc& desc, NxResourceList& list, FieldSamplerManager* manager)
	: mManager(manager)
	, mQueryDesc(desc)
	, mAccumVelocity(manager->getApexScene(), NV_ALLOC_INFO("mAccumVelocity", PARTICLES))
	, mOnStartCallback(NULL)
	, mOnFinishCallback(NULL)
{
	list.add(*this);
}

void FieldSamplerQuery::release()
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	destroy();
}

void FieldSamplerQuery::destroy()
{
	delete this;
}


FieldSamplerQuery::SceneInfo* FieldSamplerQuery::findSceneInfo(FieldSamplerSceneWrapper* sceneWrapper) const
{
	for (physx::PxU32 i = 0; i < mSceneList.getSize(); ++i)
	{
		SceneInfo* sceneInfo = DYNAMIC_CAST(SceneInfo*)(mSceneList.getResource(i));
		if (sceneInfo->getSceneWrapper() == sceneWrapper)
		{
			return sceneInfo;
		}
	}
	return NULL;
}


bool FieldSamplerQuery::addFieldSampler(FieldSamplerWrapper* fieldSamplerWrapper)
{
	const NiFieldSamplerDesc& fieldSamplerDesc = fieldSamplerWrapper->getNiFieldSamplerDesc();

	bool result = mManager->getFieldSamplerGroupsFiltering()(
	                  mQueryDesc.samplerFilterData, fieldSamplerDesc.samplerFilterData
	              );
	if (result)
	{
		FieldSamplerSceneWrapper* sceneWrapper = fieldSamplerWrapper->getFieldSamplerSceneWrapper();
		SceneInfo* sceneInfo = findSceneInfo(sceneWrapper);
		if (sceneInfo == NULL)
		{
			sceneInfo = createSceneInfo(sceneWrapper);
		}
		sceneInfo->addFieldSampler(fieldSamplerWrapper);
	}
	return result;
}

bool FieldSamplerQuery::removeFieldSampler(FieldSamplerWrapper* fieldSamplerWrapper)
{
	FieldSamplerSceneWrapper* sceneWrapper = fieldSamplerWrapper->getFieldSamplerSceneWrapper();
	SceneInfo* sceneInfo = findSceneInfo(sceneWrapper);
	return (sceneInfo != NULL) ? sceneInfo->removeFieldSampler(fieldSamplerWrapper) : false;
}

void FieldSamplerQuery::clearAllFieldSamplers()
{
	for (physx::PxU32 i = 0; i < mSceneList.getSize(); ++i)
	{
		SceneInfo* sceneInfo = DYNAMIC_CAST(SceneInfo*)(mSceneList.getResource(i));
		sceneInfo->clearAllFieldSamplers();
	}
}

void FieldSamplerQuery::setTaskDependencies(PxTask* task)
{
	for (physx::PxU32 i = 0; i < mSceneList.getSize(); ++i)
	{
		SceneInfo* sceneInfo = DYNAMIC_CAST(SceneInfo*)(mSceneList.getResource(i));
		NiFieldSamplerScene* niFieldSamplerScene = sceneInfo->getSceneWrapper()->getNiFieldSamplerScene();
		const physx::PxTask* fieldSamplerReadyTask = niFieldSamplerScene->getFieldSamplerReadyTask();
		if (fieldSamplerReadyTask != 0)
		{
			task->startAfter(fieldSamplerReadyTask->getTaskID());
		}
	}
}

void FieldSamplerQuery::update()
{
	mPrimarySceneList.clear();
	mSecondarySceneList.clear();

	for (physx::PxU32 i = 0; i < mSceneList.getSize(); ++i)
	{
		SceneInfo* sceneInfo = DYNAMIC_CAST(SceneInfo*)(mSceneList.getResource(i));
		sceneInfo->update();

		if (sceneInfo->getEnabledFieldSamplerCount() > 0 && (sceneInfo->getSceneWrapper()->getNiFieldSamplerScene() != mQueryDesc.ownerFieldSamplerScene))
		{
			((sceneInfo->getSceneWrapper()->getNiFieldSamplerSceneDesc().isPrimary) ? mPrimarySceneList : mSecondarySceneList).pushBack(sceneInfo);
		}
	}
}

bool FieldSamplerQuery::SceneInfo::update()
{
	mEnabledFieldSamplerCount = 0;
	for (physx::PxU32 i = 0; i < mFieldSamplerArray.size(); ++i)
	{
		if (mFieldSamplerArray[i]->isEnabled())
		{
			++mEnabledFieldSamplerCount;
		}
		if (mFieldSamplerArray[i]->isEnabledChanged())
		{
			mFieldSamplerArrayChanged = true;
		}
	}

	if (mFieldSamplerArrayChanged)
	{
		mFieldSamplerArrayChanged = false;
		return true;
	}
	return false;
}
/******************************** CPU Version ********************************/
class TaskExecute : public physx::PxTask, public physx::UserAllocated
{
public:
	TaskExecute(FieldSamplerQueryCPU* query) : mQuery(query) {}

	const char* getName() const
	{
		return "FieldSamplerQueryCPU::TaskExecute";
	}
	void run()
	{
		mQuery->execute();
	}

protected:
	FieldSamplerQueryCPU* mQuery;
};

FieldSamplerQueryCPU::FieldSamplerQueryCPU(const NiFieldSamplerQueryDesc& desc, NxResourceList& list, FieldSamplerManager* manager)
	: FieldSamplerQuery(desc, list, manager)
{
	//PX_ASSERT( mQueryDesc.isInputOnDevice == false );
	//PX_ASSERT( mQueryDesc.isOutputOnDevice == false );

	mTaskExecute = PX_NEW(TaskExecute)(this);

	mExecuteCount = 256;
	mResultField.resize(mExecuteCount);
	mWeights.resize(mExecuteCount);

	mAccumVelocity.setSize(desc.maxCount);
}

FieldSamplerQueryCPU::~FieldSamplerQueryCPU()
{
	delete mTaskExecute;
}

PxTaskID FieldSamplerQueryCPU::submitFieldSamplerQuery(const NiFieldSamplerQueryData& data, PxTaskID taskID)
{
	PX_ASSERT(data.isDataOnDevice == false);

	mQueryData = data;

	physx::PxTaskManager* tm = mManager->getApexScene().getTaskManager();
	tm->submitUnnamedTask(*mTaskExecute);

	FieldSamplerQuery::setTaskDependencies(mTaskExecute);

	mTaskExecute->finishBefore(taskID);
	return mTaskExecute->getTaskID();
}

void FieldSamplerQueryCPU::execute()
{
	if (mQueryData.count == 0)
	{
		return;
	}

	if (mOnStartCallback)
	{
		(*mOnStartCallback)(NULL);
	}

	for (physx::PxU32 executeOffset = 0; executeOffset < mQueryData.count; executeOffset += mExecuteCount)
	{
		const PxU32 stride = mQueryData.strideBytes / 4;
		const PxU32 offset = executeOffset * stride;
		const PxU32 massStride = mQueryData.massStrideBytes / 4;
		const PxU32 massOffset = executeOffset * massStride;

		NiFieldSampler::ExecuteData executeData;
		executeData.count        = physx::PxMin(mExecuteCount, mQueryData.count - executeOffset);
		executeData.position	 = mQueryData.pmaInPosition  + offset;
		executeData.velocity     = mQueryData.pmaInVelocity + offset;
		executeData.mass		 = mQueryData.pmaInMass  + massOffset;
		executeData.resultField  = mResultField.begin();
		executeData.stride		 = mQueryData.strideBytes;
		executeData.massStride	 = mQueryData.massStrideBytes;

		PxVec4* accumField = (PxVec4*)(mQueryData.pmaOutField + executeOffset);
		PxVec4* accumVelocity = mAccumVelocity.getPtr() + executeOffset;
		//clear accum
		for (physx::PxU32 i = 0; i < executeData.count; ++i)
		{
			accumField[i] = physx::PxVec4(0.0f);
			accumVelocity[i] = physx::PxVec4(0.0f);
		}
		for (physx::PxU32 sceneIdx = 0; sceneIdx < mPrimarySceneList.size(); ++sceneIdx)
		{
			executeScene(mPrimarySceneList[sceneIdx], executeData, accumField, accumVelocity, stride, massStride);
		}

		//setup weights for secondary scenes
		for (physx::PxU32 i = 0; i < executeData.count; ++i)
		{
			accumField[i].w = accumVelocity[i].w;
			accumVelocity[i].w = 0.0f;
		}
		for (physx::PxU32 sceneIdx = 0; sceneIdx < mSecondarySceneList.size(); ++sceneIdx)
		{
			executeScene(mSecondarySceneList[sceneIdx], executeData, accumField, accumVelocity, stride, massStride);
		}

		//compose accum field
		for (physx::PxU32 i = 0; i < executeData.count; ++i)
		{
			PxF32 blend = accumField[i].w;
			PxF32 velW = accumVelocity[i].w;
			PxF32 weight = blend + velW * (1 - blend);
			if (weight >= VELOCITY_WEIGHT_THRESHOLD)
			{
				PxVec3 result = accumField[i].getXYZ();
				const PxVec3& velocity = *(physx::PxVec3*)(executeData.velocity + i * stride);
				result += (accumVelocity[i].getXYZ() - weight * velocity);
				accumField[i] = PxVec4(result, 0);
			}
		}
	}

	if (mOnFinishCallback)
	{
		(*mOnFinishCallback)(NULL);
	}
}

void FieldSamplerQueryCPU::executeScene(const SceneInfo* sceneInfo, const NiFieldSampler::ExecuteData& executeData, PxVec4* accumField, PxVec4* accumVelocity, PxU32 stride, PxU32 massStride)
{
	FieldSamplerExecuteArgs execArgs;
	execArgs.elapsedTime = mQueryData.timeStep;
	execArgs.totalElapsedMS = mManager->getApexScene().getTotalElapsedMS();

	const physx::Array<FieldSamplerWrapper*>& fieldSamplerArray = sceneInfo->getFieldSamplerArray();
	for (physx::PxU32 fieldSamplerIdx = 0; fieldSamplerIdx < fieldSamplerArray.size(); ++fieldSamplerIdx)
	{
		const FieldSamplerWrapperCPU* fieldSampler = DYNAMIC_CAST(FieldSamplerWrapperCPU*)(fieldSamplerArray[fieldSamplerIdx]);
		if (fieldSampler->isEnabled())
		{
			const NiFieldSamplerDesc& desc = fieldSampler->getNiFieldSamplerDesc();
			const NiFieldShapeDesc& shapeDesc = fieldSampler->getNiFieldSamplerShape();
			PX_ASSERT(shapeDesc.weight >= 0.0f && shapeDesc.weight <= 1.0f);

			for (physx::PxU32 i = 0; i < executeData.count; ++i)
			{
				mWeights[i] = 0;
			}

			physx::PxU32 boundaryCount = fieldSampler->getFieldBoundaryCount();
			for (physx::PxU32 boundaryIndex = 0; boundaryIndex < boundaryCount; ++boundaryIndex)
			{
				FieldBoundaryWrapper* fieldBoundaryWrapper = fieldSampler->getFieldBoundaryWrapper(boundaryIndex);

				const physx::Array<NiFieldShapeDesc>& fieldShapes = fieldBoundaryWrapper->getFieldShapes();
				for (PxU32 shapeIndex = 0; shapeIndex < fieldShapes.size(); ++shapeIndex)
				{
					const NiFieldShapeDesc& boundaryShapeDesc = fieldShapes[shapeIndex];
					PX_ASSERT(boundaryShapeDesc.weight >= 0.0f && boundaryShapeDesc.weight <= 1.0f);

					for (physx::PxU32 i = 0; i < executeData.count; ++i)
					{
						physx::PxVec3* pos = (physx::PxVec3*)(executeData.position + i * stride);
						const PxF32 excludeWeight = evalFade(evalDistInShape(boundaryShapeDesc, *pos), 0.0f) * boundaryShapeDesc.weight;
						mWeights[i] = physx::PxMax(mWeights[i], excludeWeight);
					}
				}
			}

			for (physx::PxU32 i = 0; i < executeData.count; ++i)
			{
				physx::PxVec3* pos = (physx::PxVec3*)(executeData.position + i * stride);
				const PxF32 includeWeight = evalFade(evalDistInShape(shapeDesc, *pos), desc.boundaryFadePercentage) * shapeDesc.weight;
				const PxF32 excludeWeight = mWeights[i];
				mWeights[i] = includeWeight * (1.0f - excludeWeight);
			}

			//execute field
			fieldSampler->getNiFieldSampler()->executeFieldSampler(executeData);

			//accum field
			switch (desc.type)
			{
			case NiFieldSamplerType::FORCE:
				for (physx::PxU32 i = 0; i < executeData.count; ++i)
				{
					execArgs.position = *(physx::PxVec3*)(executeData.position + i * stride);
					execArgs.velocity = *(physx::PxVec3*)(executeData.velocity + i * stride);
					execArgs.mass     = *(executeData.mass + massStride * i);

					accumFORCE(execArgs, executeData.resultField[i], mWeights[i], accumField[i], accumVelocity[i]);
				}
				break;
			case NiFieldSamplerType::ACCELERATION:
				for (physx::PxU32 i = 0; i < executeData.count; ++i)
				{
					execArgs.position = *(physx::PxVec3*)(executeData.position + i * stride);
					execArgs.velocity = *(physx::PxVec3*)(executeData.velocity + i * stride);
					execArgs.mass     = *(executeData.mass + massStride * i);

					accumACCELERATION(execArgs, executeData.resultField[i], mWeights[i], accumField[i], accumVelocity[i]);
				}
				break;
			case NiFieldSamplerType::VELOCITY_DRAG:
				for (physx::PxU32 i = 0; i < executeData.count; ++i)
				{
					execArgs.position = *(physx::PxVec3*)(executeData.position + i * stride);
					execArgs.velocity = *(physx::PxVec3*)(executeData.velocity + i * stride);
					execArgs.mass     = *(executeData.mass + massStride * i);

					accumVELOCITY_DRAG(execArgs, desc.dragCoeff, executeData.resultField[i], mWeights[i], accumField[i], accumVelocity[i]);
				}
				break;
			case NiFieldSamplerType::VELOCITY_DIRECT:
				for (physx::PxU32 i = 0; i < executeData.count; ++i)
				{
					execArgs.position = *(physx::PxVec3*)(executeData.position + i * stride);
					execArgs.velocity = *(physx::PxVec3*)(executeData.velocity + i * stride);
					execArgs.mass     = *(executeData.mass + massStride * i);

					accumVELOCITY_DIRECT(execArgs, executeData.resultField[i], mWeights[i], accumField[i], accumVelocity[i]);
				}
				break;
			};
		}
	}

}


/******************************** GPU Version ********************************/
#if defined(APEX_CUDA_SUPPORT)


class FieldSamplerQueryLaunchTask : public PxGpuTask, public physx::UserAllocated
{
public:
	FieldSamplerQueryLaunchTask(FieldSamplerQueryGPU& query) : mQuery(&query) {}
	const char* getName() const
	{
		return "FieldSamplerQueryLaunchTask";
	}
	void         run()
	{
		PX_ALWAYS_ASSERT();
	}
	bool         launchInstance(CUstream stream, int kernelIndex)
	{
		return mQuery->launch(stream, kernelIndex);
	}
	physx::PxGpuTaskHint::Enum getTaskHint() const
	{
		return physx::PxGpuTaskHint::Kernel;
	}

protected:
	FieldSamplerQueryGPU* mQuery;
};

FieldSamplerQueryGPU::FieldSamplerQueryGPU(const NiFieldSamplerQueryDesc& desc, NxResourceList& list, FieldSamplerManager* manager)
	: FieldSamplerQuery(desc, list, manager)
{
	//PX_ASSERT( mQueryDesc.isInputOnDevice == true );
	//PX_ASSERT( mQueryDesc.isOutputOnDevice == true );

	mTaskLaunch     = PX_NEW(FieldSamplerQueryLaunchTask)(*this);

	if (desc.maxCount)
	{
		mAccumVelocity.reserve(desc.maxCount, ApexMirroredPlace::GPU);
	}
}

FieldSamplerQueryGPU::~FieldSamplerQueryGPU()
{
	PX_DELETE(mTaskLaunch);
}

PxTaskID FieldSamplerQueryGPU::submitFieldSamplerQuery(const NiFieldSamplerQueryData& data, PxTaskID taskID)
{
	PX_ASSERT(data.isDataOnDevice == true);

	mQueryData = data;

	physx::PxTaskManager* tm = mManager->getApexScene().getTaskManager();
	tm->submitUnnamedTask(*mTaskLaunch, physx::PxTaskType::TT_GPU);

	FieldSamplerQuery::setTaskDependencies(mTaskLaunch);

	mTaskLaunch->finishBefore(taskID);
	return mTaskLaunch->getTaskID();
}

bool FieldSamplerQueryGPU::launch(CUstream stream, int kernelIndex)
{
	if (mQueryData.count == 0)
	{
		return false;
	}

	FieldSamplerPointsKernelArgs args;
	args.elapsedTime    = mQueryData.timeStep;
	args.totalElapsedMS = mManager->getApexScene().getTotalElapsedMS();
	args.positionMass   = (float4*)mQueryData.pmaInPosition;
	args.velocity       = (float4*)mQueryData.pmaInVelocity;

	args.accumField     = (float4*)mQueryData.pmaOutField;
	args.accumVelocity  = (float4*)mAccumVelocity.getGpuPtr();

	NiFieldSamplerKernelLaunchData launchData;
	launchData.stream       = stream;
	launchData.threadCount  = mQueryData.count;
	launchData.kernelType   = FieldSamplerKernelType::POINTS;
	launchData.kernelArgs   = &args;

	if (kernelIndex == 0)
	{
		if (mOnStartCallback)
		{
			(*mOnStartCallback)(stream);
		}
		CUDA_OBJ(clearKernel)(stream, mQueryData.count,
		                      args.accumField,
		                      args.accumVelocity);

		CUDA_CHECK("FieldSamplerQueryGPU::clearKernel");
		return true;
	}
	const PxU32 bothSceneCount = mPrimarySceneList.size() + mSecondarySceneList.size();
	if (kernelIndex <= (int) bothSceneCount)
	{
		SceneInfo* sceneInfo = (kernelIndex <= (int) mPrimarySceneList.size()) ? mPrimarySceneList[kernelIndex - 1] : mSecondarySceneList[kernelIndex - 1 - mPrimarySceneList.size()];
		SceneInfoGPU* sceneInfoGPU = DYNAMIC_CAST(SceneInfoGPU*)(sceneInfo);

		launchData.kernelMode = FieldSamplerKernelMode::DEFAULT;
		if (kernelIndex == (int) mPrimarySceneList.size())
		{
			launchData.kernelMode = FieldSamplerKernelMode::FINISH_PRIMARY;
		}
		if ((kernelIndex == (int) bothSceneCount))
		{
			launchData.kernelMode = FieldSamplerKernelMode::FINISH_SECONDARY;
		}

		FieldSamplerSceneWrapperGPU* sceneWrapper = DYNAMIC_CAST(FieldSamplerSceneWrapperGPU*)(sceneInfo->getSceneWrapper());

		launchData.queryParamsHandle = sceneInfoGPU->getQueryParamsHandle();
		launchData.paramsHandleArrayHandle = sceneInfoGPU->getParamsHandle();
		launchData.fieldSamplerArray = &sceneInfo->getFieldSamplerArray();
		launchData.activeFieldSamplerCount = sceneInfo->getEnabledFieldSamplerCount();

		sceneWrapper->getNiFieldSamplerScene()->launchFieldSamplerCudaKernel(launchData);
		CUDA_CHECK("FieldSamplerQueryGPU::launchFieldSamplerCudaKernel_POINTS");
		return true;
	}

	CUDA_OBJ(composeKernel)(stream, mQueryData.count,
	                        args.accumField, args.accumVelocity, args.velocity, args.elapsedTime);

	CUDA_CHECK("FieldSamplerQueryGPU::composeKernel");

	if (mOnFinishCallback)
	{
		(*mOnFinishCallback)(stream);
	}
	return false;
}

FieldSamplerQueryGPU::SceneInfoGPU::SceneInfoGPU(NxResourceList& list, FieldSamplerQuery* query, FieldSamplerSceneWrapper* sceneWrapper)
	: SceneInfo(list, query, sceneWrapper)
	, mConstMemGroup(DYNAMIC_CAST(FieldSamplerSceneWrapperGPU*)(sceneWrapper)->getConstMem())
{
	APEX_CUDA_CONST_MEM_GROUP_SCOPE(mConstMemGroup);

	mQueryParamsHandle.alloc(_storage_);
}

bool FieldSamplerQueryGPU::SceneInfoGPU::update()
{
	if (FieldSamplerQuery::SceneInfo::update())
	{
		APEX_CUDA_CONST_MEM_GROUP_SCOPE(mConstMemGroup);

		FieldSamplerParamsHandleArray* paramsHandleArray = mParamsHandleArrayHandle.allocOrResolve(_storage_);
		PX_ASSERT(paramsHandleArray);
		if (paramsHandleArray->resize(_storage_, mEnabledFieldSamplerCount))
		{
			InplaceHandle<FieldSamplerParams>* elems = paramsHandleArray->getElems(_storage_);
			for (physx::PxU32 i = 0, enabledIdx = 0; i < mFieldSamplerArray.size(); ++i)
			{
				FieldSamplerWrapperGPU* fieldSamplerWrapper = DYNAMIC_CAST(FieldSamplerWrapperGPU*)(mFieldSamplerArray[i]);
				if (fieldSamplerWrapper->isEnabled())
				{
					PX_ASSERT(enabledIdx < mEnabledFieldSamplerCount);
					elems[enabledIdx] = fieldSamplerWrapper->getParamsHandle();
					++enabledIdx;
				}
			}
		}
		return true;
	}
	return false;
}

physx::PxVec3 FieldSamplerQueryGPU::executeFieldSamplerQueryOnGrid(const NiFieldSamplerQueryGridData& data)
{
	FieldSamplerGridKernelArgs args;

	args.numX           = data.numX;
	args.numY           = data.numY;
	args.numZ           = data.numZ;
	args.strideX        = data.strideX;
	args.strideY        = data.strideY;
	args.offset         = data.offset;

	args.gridToWorld    = data.gridToWorld;

	args.mass           = data.mass;
	args.elapsedTime    = data.timeStep;
	args.cellRadius		= data.cellRadius;
	args.totalElapsedMS = mManager->getApexScene().getTotalElapsedMS();

	args.accumVelocity  = (float4*)data.resultVelocity + data.offset;

	NiFieldSamplerKernelLaunchData launchData;
	launchData.stream       = data.stream;
	launchData.threadCount  = data.numX * data.numY * data.strideY;
	launchData.kernelType   = FieldSamplerKernelType::GRID;
	launchData.kernelArgs   = &args;

	CUDA_OBJ(clearGridKernel)(data.stream, launchData.threadCount,
	                          args.numX, args.numY, args.numZ,
	                          args.strideX, args.strideY,
	                          args.accumVelocity);

	CUDA_CHECK("FieldSamplerQueryGPU::clearGridKernel");

	physx::PxVec3 velocity(0.0f);

	for (physx::PxU32 i = 0; i < mSecondarySceneList.size(); ++i)
	{
		SceneInfoGPU* sceneInfo = DYNAMIC_CAST(SceneInfoGPU*)(mSecondarySceneList[i]);
		FieldSamplerSceneWrapperGPU* sceneWrapper = DYNAMIC_CAST(FieldSamplerSceneWrapperGPU*)(sceneInfo->getSceneWrapper());

		launchData.activeFieldSamplerCount = 0;

		const physx::Array<FieldSamplerWrapper*>& fieldSamplerArray = sceneInfo->getFieldSamplerArray();
		for (physx::PxU32 fieldSamplerIdx = 0; fieldSamplerIdx < fieldSamplerArray.size(); ++fieldSamplerIdx)
		{
			const FieldSamplerWrapperGPU* wrapper = static_cast<const FieldSamplerWrapperGPU* >( fieldSamplerArray[fieldSamplerIdx] );
			if (wrapper->isEnabled())
			{
				switch (wrapper->getNiFieldSamplerDesc().gridSupportType)
				{
					case NiFieldSamplerGridSupportType::SINGLE_VELOCITY:
					{
						const NiFieldSampler* fieldSampler = wrapper->getNiFieldSampler();
						velocity += fieldSampler->queryFieldSamplerVelocity();
					}
					break;
					case NiFieldSamplerGridSupportType::VELOCITY_PER_CELL:
					{
						launchData.activeFieldSamplerCount += 1;
					}
					break;
				}
			}
		}

		if (launchData.activeFieldSamplerCount > 0)
		{
			launchData.queryParamsHandle = sceneInfo->getQueryParamsHandle();
			launchData.paramsHandleArrayHandle = sceneInfo->getParamsHandle();
			launchData.fieldSamplerArray = &sceneInfo->getFieldSamplerArray();
			launchData.kernelMode = FieldSamplerKernelMode::DEFAULT;

			sceneWrapper->getNiFieldSamplerScene()->launchFieldSamplerCudaKernel(launchData);
			CUDA_CHECK("FieldSamplerQueryGPU::launchFieldSamplerCudaKernel_GRID");
		}
	}
	return velocity;
}


#endif

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
