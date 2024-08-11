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

#ifndef __FIELD_SAMPLER_QUERY_H__
#define __FIELD_SAMPLER_QUERY_H__

#include "NxApex.h"
#include "ApexSDKHelpers.h"
#include "NiFieldSamplerQuery.h"
#include "NiFieldSampler.h"

#if defined(APEX_CUDA_SUPPORT)
#include "ApexCudaWrapper.h"
#endif

#include "FieldSamplerCommon.h"

namespace physx
{
namespace apex
{
namespace fieldsampler
{

class FieldSamplerManager;
class FieldSamplerSceneWrapper;
class FieldSamplerWrapper;

class FieldSamplerQuery : public NiFieldSamplerQuery, public NxApexResource, public ApexResource
{
protected:

	class SceneInfo : public NxApexResource, public ApexResource
	{
	protected:
		FieldSamplerQuery* mQuery;
		FieldSamplerSceneWrapper* mSceneWrapper;
		physx::Array<FieldSamplerWrapper*> mFieldSamplerArray;
		bool mFieldSamplerArrayChanged;

		physx::PxU32 mEnabledFieldSamplerCount;

		SceneInfo(NxResourceList& list, FieldSamplerQuery* query, FieldSamplerSceneWrapper* sceneWrapper)
			: mQuery(query), mSceneWrapper(sceneWrapper), mFieldSamplerArrayChanged(false), mEnabledFieldSamplerCount(0)
		{
			list.add(*this);
		}

	public:
		FieldSamplerSceneWrapper* getSceneWrapper() const
		{
			return mSceneWrapper;
		}

		void addFieldSampler(FieldSamplerWrapper* fieldSamplerWrapper)
		{
			mFieldSamplerArray.pushBack(fieldSamplerWrapper);
			mFieldSamplerArrayChanged = true;
		}
		bool removeFieldSampler(FieldSamplerWrapper* fieldSamplerWrapper)
		{
			if (mFieldSamplerArray.findAndReplaceWithLast(fieldSamplerWrapper))
			{
				mFieldSamplerArrayChanged = true;
				return true;
			}
			return false;
		}
		void clearAllFieldSamplers()
		{
			mFieldSamplerArray.clear();
			mFieldSamplerArrayChanged = true;
		}
		physx::PxU32 getEnabledFieldSamplerCount() const
		{
			return mEnabledFieldSamplerCount;
		}
		const physx::Array<FieldSamplerWrapper*>& getFieldSamplerArray() const
		{
			return mFieldSamplerArray;
		}

		virtual bool update();

		// NxApexResource methods
		void						release()
		{
			delete this;
		}
		void						setListIndex(NxResourceList& list, physx::PxU32 index)
		{
			m_listIndex = index;
			m_list = &list;
		}
		physx::PxU32				getListIndex() const
		{
			return m_listIndex;
		}
	};

	SceneInfo* findSceneInfo(FieldSamplerSceneWrapper* sceneWrapper) const;

	virtual SceneInfo* createSceneInfo(FieldSamplerSceneWrapper* sceneWrapper) = 0;

	FieldSamplerQuery(const NiFieldSamplerQueryDesc& desc, NxResourceList& list, FieldSamplerManager* manager);

public:
	virtual void update();
	
	PX_INLINE void setOnStartCallback(NiFieldSamplerCallback* callback)
	{
		if (mOnStartCallback) 
		{
			PX_DELETE(mOnStartCallback);
		}
		mOnStartCallback = callback;
	}
	PX_INLINE void setOnFinishCallback(NiFieldSamplerCallback* callback)
	{
		if (mOnFinishCallback) 
		{
			PX_DELETE(mOnFinishCallback);
		}
		mOnFinishCallback = callback;
	}

	virtual void		submitTasks() {}
	virtual void		setTaskDependencies() {}
	virtual void		fetchResults() {}

	void	release();
	void	destroy();

	// NxApexResource methods
	void						setListIndex(NxResourceList& list, physx::PxU32 index)
	{
		m_listIndex = index;
		m_list = &list;
	}
	physx::PxU32				getListIndex() const
	{
		return m_listIndex;
	}

#if NX_SDK_VERSION_MAJOR == 2
	void		setPhysXScene(NxScene*)	{}
	NxScene*	getPhysXScene() const
	{
		return NULL;
	}
#elif NX_SDK_VERSION_MAJOR == 3
	void		setPhysXScene(PxScene*)	{}
	PxScene*	getPhysXScene() const
	{
		return NULL;
	}
#endif

	bool addFieldSampler(FieldSamplerWrapper*);
	bool removeFieldSampler(FieldSamplerWrapper*);
	void clearAllFieldSamplers();

	const NiFieldSamplerQueryDesc& getQueryDesc() const
	{
		return mQueryDesc;
	}

protected:
	void		setTaskDependencies(PxTask*);

	FieldSamplerManager* 				mManager;

	NiFieldSamplerQueryDesc				mQueryDesc;
	NiFieldSamplerQueryData				mQueryData;

	NxResourceList						mSceneList;

	physx::Array<SceneInfo*>			mPrimarySceneList;
	physx::Array<SceneInfo*>			mSecondarySceneList;

	ApexMirroredArray<physx::PxVec4>	mAccumVelocity;

	NiFieldSamplerCallback*				mOnStartCallback;
	NiFieldSamplerCallback*				mOnFinishCallback;

	friend class FieldSamplerManager;
};


class FieldSamplerQueryCPU : public FieldSamplerQuery
{
public:
	FieldSamplerQueryCPU(const NiFieldSamplerQueryDesc& desc, NxResourceList& list, FieldSamplerManager* manager);
	~FieldSamplerQueryCPU();

	// NiFieldSamplerQuery methods
	PxTaskID submitFieldSamplerQuery(const NiFieldSamplerQueryData& data, PxTaskID taskID);

protected:
	void		execute();
	void		executeScene(const SceneInfo* sceneInfo, const NiFieldSampler::ExecuteData& executeData, PxVec4* accumField, PxVec4* accumVelocity, PxU32 stride, PxU32 massStride);

	physx::PxU32				mExecuteCount;
	physx::Array<physx::PxVec3>	mResultField;
	physx::Array<physx::PxF32>	mWeights;

	PxTask*				mTaskExecute;

	friend class TaskExecute;

	class SceneInfoCPU : public SceneInfo
	{
	public:
		SceneInfoCPU(NxResourceList& list, FieldSamplerQuery* query, FieldSamplerSceneWrapper* sceneWrapper)
			: SceneInfo(list, query, sceneWrapper)
		{
		}
	};

	virtual SceneInfo* createSceneInfo(FieldSamplerSceneWrapper* sceneWrapper)
	{
		return PX_NEW(SceneInfoCPU)(mSceneList, this, sceneWrapper);
	}

};

#if defined(APEX_CUDA_SUPPORT)

class FieldSamplerQueryGPU : public FieldSamplerQuery
{
public:
	FieldSamplerQueryGPU(const NiFieldSamplerQueryDesc& desc, NxResourceList& list, FieldSamplerManager* manager);
	~FieldSamplerQueryGPU();

	// NiFieldSamplerQuery methods
	PxTaskID submitFieldSamplerQuery(const NiFieldSamplerQueryData& data, PxTaskID taskID);

	physx::PxVec3 executeFieldSamplerQueryOnGrid(const NiFieldSamplerQueryGridData&);

protected:
	bool		launch(CUstream stream, int kernelIndex);

	PxTask*	mTaskLaunch;

	friend class FieldSamplerQueryLaunchTask;

	class SceneInfoGPU : public SceneInfo
	{
	public:
		SceneInfoGPU(NxResourceList& list, FieldSamplerQuery* query, FieldSamplerSceneWrapper* sceneWrapper);

		virtual bool update();

		PX_INLINE InplaceHandle<physx::apex::fieldsampler::FieldSamplerParamsHandleArray> getParamsHandle() const
		{
			PX_ASSERT(mParamsHandleArrayHandle.isNull() == false);
			return mParamsHandleArrayHandle;
		}
		PX_INLINE InplaceHandle<physx::apex::fieldsampler::FieldSamplerQueryParams> getQueryParamsHandle() const
		{
			PX_ASSERT(mQueryParamsHandle.isNull() == false);
			return mQueryParamsHandle;
		}

	private:
		ApexCudaConstMemGroup                           mConstMemGroup;
		InplaceHandle<physx::apex::fieldsampler::FieldSamplerParamsHandleArray>    mParamsHandleArrayHandle;
		InplaceHandle<physx::apex::fieldsampler::FieldSamplerQueryParams>          mQueryParamsHandle;
	};

	virtual SceneInfo* createSceneInfo(FieldSamplerSceneWrapper* sceneWrapper)
	{
		return PX_NEW(SceneInfoGPU)(mSceneList, this, sceneWrapper);
	}
};

#endif

}
}
} // end namespace physx::apex

#endif
