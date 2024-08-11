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

#ifndef __IOFX_MANAGER_H__
#define __IOFX_MANAGER_H__

#include "PsArray.h"
#include "PsHashMap.h"
#include "ApexInterface.h"
#include "NiApexScene.h"
#include "NiIofxManager.h"
#include "ApexActor.h"

#include "ModifierData.h"

#include "PxTask.h"
#include "ApexMirroredArray.h"

namespace physx
{
namespace apex
{
namespace iofx
{

class IofxScene;
class IofxManager;
class IosObjectBaseData;
class IofxAsset;
class IofxActor;
class ApexRenderVolume;
class IofxSharedRenderData;

class TaskUpdateEffects : public physx::PxTask
{
public:
	TaskUpdateEffects(IofxManager& owner) : mOwner(owner) {}
	const char* getName() const
	{
		return "IofxManager::UpdateEffects";
	}
	void run();
protected:
	IofxManager& mOwner;

private:
	TaskUpdateEffects& operator=(const TaskUpdateEffects&);
};

class IofxAsset;

class IofxAssetSceneInst : public physx::UserAllocated
{
public:
	IofxAssetSceneInst(IofxAsset* asset, PxU32 assetID, PxU32 semantics);
	virtual ~IofxAssetSceneInst();

	PX_INLINE IofxAsset* getAsset() const
	{
		return _asset;
	}

	PX_INLINE PxU32 getAssetID() const
	{
		return _assetID;
	}
	PX_INLINE PxU32 getSemantics() const
	{
		return _semantics;
	}

	PX_INLINE void addActorClassId(PxU16 actorClassId)
	{
		_actorClassIdList.pushBack(actorClassId);
	}
	PX_INLINE const Array<PxU16>& getActorClassIdList() const
	{
		return _actorClassIdList;
	}

protected:
	IofxAsset*		_asset;
	PxU32			_assetID;
	PxU32			_semantics;

	Array<PxU16>	_actorClassIdList;
};

class CudaPipeline
{
public:
	virtual ~CudaPipeline() {}
	virtual void release() = 0;
	virtual void fetchResults() = 0;
	virtual void submitTasks() = 0;

	virtual PxTaskID launchGpuTasks() = 0;
	virtual void launchPrep() = 0;

	virtual IofxAssetSceneInst* createAssetSceneInst(IofxAsset* asset, PxU32 assetID, PxU32 semantics) = 0;
};


class IofxManager : public NiIofxManager, public NxApexResource, public ApexResource, public ApexContext
{
public:
	IofxManager(IofxScene& scene, const NiIofxManagerDesc& desc, bool isMesh);
	~IofxManager();

	void destroy();

	/* Over-ride this ApexContext method to capture IofxActor deletion events */
	void removeActorAtIndex(physx::PxU32 index);

	void createSimulationBuffers(NiIosBufferDesc& outDesc);
	void setSimulationParameters(PxF32 radius, const PxVec3& up, PxF32 gravity, PxF32 restDensity);
	void updateEffectsData(PxF32 deltaTime, PxU32 numObjects, PxU32 maxInputID, PxU32 maxStateID);
	void submitTasks();
	void fetchResults();
	void release();
	void outputHostToDevice();
	PxTaskID getUpdateEffectsTaskID(PxTaskID);
	void cpuModifiers();
	PxBounds3 getBounds() const;
	void swapStates();

	PxU16	getActorClassID(NxIofxAsset* asset, PxU16 meshID);
	void	releaseAssetID(NxIofxAsset* asset);
	PxU16	getVolumeID(NxApexRenderVolume* vol);
	PX_INLINE PxU32	getSimulatedParticlesCount() const
	{
		return mSimulatedParticlesCount;
	}

	PX_INLINE void setOnStartCallback(NiIofxManagerCallback* callback)
	{
		if (mOnStartCallback) 
		{
			PX_DELETE(mOnStartCallback);
		}
		mOnStartCallback = callback;
	}
	PX_INLINE void setOnFinishCallback(NiIofxManagerCallback* callback)
	{
		if (mOnFinishCallback) 
		{
			PX_DELETE(mOnFinishCallback);
		}
		mOnFinishCallback = callback;
	}

	PX_INLINE bool isMesh()
	{
		return mIsMesh;
	}

	physx::PxU32	    getListIndex() const
	{
		return m_listIndex;
	}
	void	            setListIndex(NxResourceList& list, physx::PxU32 index)
	{
		m_listIndex = index;
		m_list = &list;
	}

	PxF32	getObjectRadius() const;

	IofxAssetSceneInst* 	createAssetSceneInst(NxIofxAsset*);
	void					releaseAssetSceneInst(IofxAssetSceneInst*);

	typedef HashMap<IofxAsset*, IofxAssetSceneInst*> AssetHashMap_t;
	AssetHashMap_t			mAssetHashMap;

	PxU32								mNextAssetID;
	struct ActorClassData
	{
		NxApexAsset*		renderAsset; // NULL for empty rows
		IofxAssetSceneInst* iofxAssetSceneInst;
	};
	physx::Array<ActorClassData>		mActorClassTable;

	struct VolumeData
	{
		ApexRenderVolume* 		 vol;			// NULL for empty rows
		PxBounds3				 mBounds;
		PxU32                    mPri;
		PxU32                    mFlags;
		physx::Array<IofxActor*> mActors; // Indexed by actorClassID
	};
	physx::Array<VolumeData>			mVolumeTable;
	physx::Array<PxU32>					mCountPerActor;
	physx::Array<PxU32>					mStartPerActor;
	physx::Array<PxU32>					mBuildPerActor;
	physx::Array<PxU32>					mOutputToState;
	physx::PxTaskID				mPostUpdateTaskID;

	physx::Array<PxU32>					mSortingKeys;

	physx::Array<PxU32>					mVolumeActorClassBitmap;

	IofxScene*                          mIofxScene;
	physx::Array<IosObjectBaseData*>	mObjData;
	NiIosBufferDesc						mSimBuffers;
	ApexSimpleString					mIosAssetName;

	// reference pointers for IOFX actors, so they know which buffer
	// in in which mode.
	IosObjectBaseData* 					mWorkingIosData;
	IosObjectBaseData* 					mResultIosData;
	IosObjectBaseData*					mStagingIosData;

	IosObjectBaseData*					mRenderIosData;

	bool								mIsInteropEnabled;

	enum ResultReadyState
	{
		RESULT_WAIT_FOR_NEW = 0,
		RESULT_READY,
		RESULT_WAIT_FOR_INTEROP,
		RESULT_INTEROP_READY,
	}									mResultReadyState;

	volatile physx::PxU32				mTargetSemantics;
	physx::ReadWriteLock				mTargetSemanticsLock;

	IofxSharedRenderData*				mSharedRenderData;
	physx::Array<IofxSharedRenderData*> mInteropRenderData;

	// Simulation storage, for CPU/GPU IOS
	ApexMirroredArray<PxVec4>			positionMass;
	ApexMirroredArray<PxVec4>			velocityLife;
	ApexMirroredArray<PxVec4>			collisionNormalFlags;
	ApexMirroredArray<PxF32>			density;
	ApexMirroredArray<NiIofxActorID>	actorIdentifiers;
	ApexMirroredArray<PxU32>			inStateToInput;
	ApexMirroredArray<PxU32>			outStateToInput;

	ApexMirroredArray<PxU32>			userData;

	NiIofxManagerCallback*				mOnStartCallback;
	NiIofxManagerCallback*				mOnFinishCallback;

	// Assets that were added on this frame (prior to simulate)
	physx::Array<const IofxAsset*>		addedAssets;

	// Max size of public/private states over active (simulated) assets
	PxU32								pubStateSize, privStateSize;

	// State data (CPU only)

	typedef ApexMirroredArray<IofxSlice> SliceArray;

	struct State
	{
		physx::Array<SliceArray*> slices; // Slices
		physx::Array<IofxSlice*> a, b; // Pointers to slices' halves
	};

	State								pubState;
	State								privState;

	PxU32								mInStateOffset;
	PxU32								mOutStateOffset;
	bool								mStateSwap;

	PxF32                               mTotalElapsedTime;
	bool                                mIsMesh;
	bool                                mDistanceSortingEnabled;
	bool                                mCudaIos;
	bool                                mCudaModifiers;

	void								prepareRenderResources();
	void								postPrepareRenderResources();

	void								fillMapUnmapArraysForInterop(physx::Array<CUgraphicsResource> &, physx::Array<CUgraphicsResource> &);
	void								mapBufferResults(bool, bool);

	CudaPipeline*                       mCudaPipeline;
	TaskUpdateEffects					mSimulateTask;

	PxBounds3							mBounds;

	physx::PxGpuCopyDescQueue     mCopyQueue;

	PxU32								mSimulatedParticlesCount;
	
#ifdef APEX_TEST
	IofxManagerTestData*				mTestData;

	virtual IofxManagerTestData*		createTestData();
	virtual void						copyTestData() const;
	virtual void						clearTestData();
#endif
};

#define DEFERRED_IOFX_ACTOR ((IofxActor*)(1))

}
}
} // end namespace physx::apex

#endif // __NI_IOFX_MANAGER_H__
