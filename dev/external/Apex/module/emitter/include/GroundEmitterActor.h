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

#ifndef __GROUND_EMITTER_ACTOR_H__
#define __GROUND_EMITTER_ACTOR_H__

#include "NxGroundEmitterActor.h"
#include "GroundEmitterAsset.h"
#include "EmitterScene.h"
#include "ApexActor.h"
#include "ApexFIFO.h"
#include "ApexSharedUtils.h"
#include "PsUserAllocated.h"
#include "foundation/PxPlane.h"

#if NX_SDK_VERSION_MAJOR == 2
#include <NxSceneQuery.h>
class NxScene;
#elif NX_SDK_VERSION_MAJOR == 3
// SJB TODO3.0
#endif

namespace physx
{
namespace apex
{
namespace emitter
{

template<typename T>
struct ApexVec2
{
	T x;
	T y;
};

class InjectorData : public LODNode, public physx::UserAllocated
{
public:
	virtual ~InjectorData()
	{
	}
	virtual physx::PxF32 getBenefit();
	virtual physx::PxF32 setResource(physx::PxF32 suggested, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit);

	NiIosInjector* 				mInjector;  // For emitting
	physx::PxF32				mObjectRadius;

	ApexSimpleString			iofxAssetName;
	ApexSimpleString			iosAssetName;

	physx::Array<IosNewObject>	particles;

	class InjectTask : public physx::PxTask
	{
	public:
		InjectTask() {}
		void run();
		const char* getName() const
		{
			return "GroundEmitterActor::InjectTask";
		}
		GroundEmitterActor* mActor;
		InjectorData* mData;
	};
	void initTask(GroundEmitterActor& actor, InjectorData& data)
	{
		mTask.mActor = &actor;
		mTask.mData = &data;
	}

	InjectTask					mTask;
};


struct MaterialData
{
	physx::Array<physx::PxU32>  injectorIndices;
	physx::Array<physx::PxF32>  accumWeights;
	physx::Array<physx::PxF32>  maxSlopeAngles;

	physx::PxU32 chooseIOFX(physx::PxF32& maxSlopeAngle, QDSRand& rand)
	{
		PX_ASSERT(injectorIndices.size() == accumWeights.size());

		physx::PxF32 u = rand.getScaled(0, accumWeights.back());
		for (physx::PxU32 i = 0; i < accumWeights.size(); i++)
		{
			if (u <= accumWeights[i])
			{
				maxSlopeAngle = maxSlopeAngles[i];
				return injectorIndices[i];
			}
		}

		PX_ALWAYS_ASSERT();
		return (physx::PxU32) - 1;
	}
};

#if NX_SDK_VERSION_MAJOR == 2
class GroundEmitterActor;
class QueryReport : public NxSceneQueryReport
{
public:
	QueryReport(GroundEmitterActor& ge)
	{
		mGroundEmitter = &ge;
	}

	// callbacks
	virtual NxQueryReportResult onBooleanQuery(void* userData, bool result)
	{
		PX_UNUSED(userData);
		PX_UNUSED(result);
		PX_ALWAYS_ASSERT();
		return NX_SQR_CONTINUE;
	}

	virtual NxQueryReportResult onShapeQuery(void* userData, physx::PxU32 nbHits, NxShape** hits)
	{
		PX_UNUSED(userData);
		PX_UNUSED(nbHits);
		PX_UNUSED(hits);
		PX_ALWAYS_ASSERT();
		return NX_SQR_CONTINUE;
	}

	virtual NxQueryReportResult onSweepQuery(void* userData, physx::PxU32 nbHits, NxSweepQueryHit* hits)
	{
		PX_UNUSED(userData);
		PX_UNUSED(nbHits);
		PX_UNUSED(hits);
		PX_ALWAYS_ASSERT();
		return NX_SQR_CONTINUE;
	}

	virtual NxQueryReportResult onRaycastQuery(void* userData, physx::PxU32 nbHits, const NxRaycastHit* hits);

private:
	GroundEmitterActor* mGroundEmitter;
};
#endif

class GroundEmitterActor : public NxGroundEmitterActor, public EmitterActorBase, public NxApexResource, public ApexResource
{
private:
	GroundEmitterActor& operator=(const GroundEmitterActor&);

public:
	GroundEmitterActor(const NxGroundEmitterActorDesc&, GroundEmitterAsset&, NxResourceList&, EmitterScene&);
	~GroundEmitterActor();

	NxApexRenderable* 			getRenderable()
	{
		return NULL;
	}
	NxApexActor* 				getNxApexActor()
	{
		return this;
	}
	void						removeActorAtIndex(physx::PxU32 index);

	void						getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly);
	physx::PxF32				getActivePhysicalLod();
	void						forcePhysicalLod(physx::PxF32 lod);

	/* NxApexResource, ApexResource */
	void						release();
	physx::PxU32				getListIndex() const
	{
		return m_listIndex;
	}
	void						setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	/* LODNode */
	physx::PxF32                getBenefit();
	physx::PxF32                setResource(physx::PxF32 suggested, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit);


	/* EmitterActorBase */
	void                        destroy();
	NxApexAsset*		        getOwner() const;
	void						visualize(NiApexRenderDebug& renderDebug);

#if NX_SDK_VERSION_MAJOR == 2
	void						setPhysXScene(NxScene*);
	NxScene*					getPhysXScene() const
	{
		return mNxScene;
	}
	NxScene* 					mNxScene;
#elif NX_SDK_VERSION_MAJOR == 3
	void						setPhysXScene(PxScene*);
	PxScene*					getPhysXScene() const
	{
		return mNxScene;
	}
	PxScene* 					mNxScene;
#endif

	void						submitTasks();
	void						setTaskDependencies();
	void						fetchResults();

	NxGroundEmitterAsset*       getEmitterAsset() const;

	const physx::PxMat44		getPose() const;
	void						setPose(const physx::PxMat44& pos);

	const physx::PxVec3& 		getPosition() const
	{
		return mLocalPlayerPosition;
	}
	void				        setPosition(const physx::PxVec3& pos);
	const physx::PxMat34Legacy& getRotation() const
	{
		return mPose;
	}
	void						setRotation(const physx::PxMat34Legacy& rotation);
	const NxRange<physx::PxVec3> &getVelocityRange() const
	{
		return mAsset->getVelocityRange();
	}
	const NxRange<physx::PxF32> &getLifetimeRange() const
	{
		return mAsset->getLifetimeRange();
	}
	physx::PxU32				getRaycastCollisionGroups() const
	{
		return mRaycastCollisionGroups;
	}
	void						setRaycastCollisionGroups(physx::PxU32 g)
	{
		mRaycastCollisionGroups = g;
	}
#if NX_SDK_VERSION_MAJOR == 2
	const NxGroupsMask*			getRaycastCollisionGroupsMask() const
	{
		return mShouldUseGroupsMask ? &mRaycastCollisionGroupsMask : NULL;
	}
	void						setRaycastCollisionGroupsMask(NxGroupsMask*);
#elif NX_SDK_VERSION_MAJOR == 3
	const physx::PxFilterData*			getRaycastCollisionGroupsMask() const
	{
		return mShouldUseGroupsMask ? &mRaycastCollisionGroupsMask : NULL;
	}
	void						setRaycastCollisionGroupsMask(physx::PxFilterData*);
#endif

	void						setMaterialLookupCallback(NxMaterialLookupCallback* mlc)
	{
		mMaterialCallback = mlc;
	}
	NxMaterialLookupCallback* 	getMaterialLookupCallback() const
	{
		return mMaterialCallback;
	}
#if NX_SDK_VERSION_MAJOR == 2
	void						setAttachActor(NxActor* a)
	{
		mAttachActor = a;
	}
	const NxActor* 				getAttachActor() const
	{
		return mAttachActor;
	}
	NxActor* 					mAttachActor;
#elif NX_SDK_VERSION_MAJOR == 3
	void						setAttachActor(PxActor* a)
	{
		mAttachActor = a;
	}
	const PxActor* 				getAttachActor() const
	{
		return mAttachActor;
	}
	PxActor* 					mAttachActor;
#endif
	void						setAttachRelativePosition(const physx::PxVec3& pos)
	{
		mAttachRelativePosition = pos;
	}

	const physx::PxVec3& 		getAttachRelativePosition() const
	{
		return mAttachRelativePosition;
	}

	bool						addMeshForGroundMaterial(const NxMaterialFactoryMappingDesc&, const NxEmitterLodParamDesc&);
	void						setDensityRange(const NxRange<physx::PxF32>& d)
	{
		mDensityRange = d;
	}
	void                        setMaxRaycastsPerFrame(physx::PxU32 m)
	{
		mMaxNumRaycastsPerFrame = m;
	}
	void                        setRaycastHeight(physx::PxF32 h)
	{
		mRaycastHeight = h;
	}
	void						setSpawnHeight(physx::PxF32 h)
	{
		mSpawnHeight = h;
	}
	void                        setRadius(physx::PxF32);
	void						setPreferredRenderVolume(physx::apex::NxApexRenderVolume*);


	const NxRange<physx::PxF32> &getDensityRange() const
	{
		return mDensityRange;
	}
	physx::PxF32				getRadius() const
	{
		return mRadius;
	}
	physx::PxU32				getMaxRaycastsPerFrame() const
	{
		return mMaxNumRaycastsPerFrame;
	}
	physx::PxF32				getRaycastHeight() const
	{
		return mRaycastHeight;
	}
	physx::PxF32				getSpawnHeight() const
	{
		return mSpawnHeight;
	}

	void							setSeed(PxU32 seed)
	{
		mRand.setSeed(seed);
	}

protected:
	/* internal methods */
	void                        submitRaycasts();
	void                        injectParticles(InjectorData& data);
	void                        refreshCircle(bool edgeOnly);
	void						tick();
#if NX_SDK_VERSION_MAJOR == 3
	bool						onRaycastQuery(physx::PxU32 nbHits, const PxRaycastHit* hits);
#endif

	GroundEmitterAsset* 		mAsset;
	EmitterScene* 				mScene;

	/* Actor modifiable versions of asset data */
	NxRange<physx::PxF32> 		mDensityRange;
	physx::PxF32				mRadius;
	physx::PxU32				mMaxNumRaycastsPerFrame;
	physx::PxF32				mRaycastHeight;
	physx::PxF32				mSpawnHeight;
	physx::PxVec3				mLocalUpDirection;
	physx::PxMat34Legacy		mPose;
	physx::PxMat34Legacy		mInversePose;
	physx::PxU32				mRaycastCollisionGroups;
#if NX_SDK_VERSION_MAJOR == 2
	NxGroupsMask				mRaycastCollisionGroupsMask;
#elif NX_SDK_VERSION_MAJOR == 3
	physx::PxFilterData			mRaycastCollisionGroupsMask;
#endif

	physx::PxVec3				mAttachRelativePosition;

	NxMaterialLookupCallback* 	mMaterialCallback;
	physx::Array<NxMaterialLookupCallback::MaterialRequest> mMaterialRequestArray;

	/* Actor local data */
	NxBank<MaterialData, physx::PxU16> mPerMaterialData;
	physx::Array<InjectorData*>	mInjectorList;
	physx::Array<NiInstancedObjectSimulation*> mIosList;
	physx::PxF32				mGridCellSize;
	physx::PxU32				mTotalElapsedTimeMs;

	struct RaycastVisInfo
	{
		physx::PxVec3 rayStart;
		physx::PxU32  timeSubmittedMs;	// running time in milliseconds (should last for 50 days or so)
	};
	physx::Array<RaycastVisInfo>	mVisualizeRaycastsList;


	// use FIFO so it does the earliest (that are probably the closest to the player first)
	ApexFIFO<physx::PxVec3>		mToRaycast;
	physx::Array<physx::PxU32>  mCellLastRefreshSteps;
	physx::PxU32                mSimulationSteps;
	ApexVec2<physx::PxI32>		mNextGridCell;

	/* runtime state members */
#if NX_SDK_VERSION_MAJOR == 2
	QueryReport                 mQueryReport;
	NxSceneQuery*               mQueryObject;
#elif NX_SDK_VERSION_MAJOR == 3
#endif

	physx::PxVec3				mLocalPlayerPosition;
	physx::PxVec3				mWorldPlayerPosition;
	physx::PxVec3				mOldLocalPlayerPosition; // used to determine when to clear per cell step counts
	physx::PxF32                mStepsize;
	physx::PxPlane				mHorizonPlane;

	physx::PxF32				mCurrentDensity;
	physx::PxF32                mRadius2;
	physx::PxF32                mCircleArea;
	bool                        mShouldUseGroupsMask;
	physx::PxF32                mMaxStepSize;
	bool                        mRefreshFullCircle;

	physx::Mutex				mInjectorDataLock;

	physx::QDSRand				mRand;

	class GroundEmitterTickTask : public physx::PxTask
	{
	public:
		GroundEmitterTickTask(GroundEmitterActor& actor) : mActor(actor) {}

		const char* getName() const
		{
			return "GroundEmitterActor::TickTask";
		}
		void run()
		{
			mActor.tick();
		}

	protected:
		GroundEmitterActor& mActor;

	private:
		GroundEmitterTickTask& operator=(const GroundEmitterTickTask&);
	};
	GroundEmitterTickTask 		mTickTask;

	friend class InjectorData::InjectTask;
	friend class QueryReport;
};

}
}
} // end namespace physx::apex

#endif
