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

#ifndef __EMITTER_ACTOR_H__
#define __EMITTER_ACTOR_H__

#include "NxApex.h"

#include "NxApexEmitterActor.h"
#include "ApexActor.h"
#include "ApexInterface.h"
#include "EmitterScene.h"
#include "NxEmitterLodParamDesc.h"

#include "ApexRand.h"

namespace physx
{
namespace apex
{

class NxApexEmitterAsset;

namespace emitter
{
class ApexEmitterAsset;
class EmitterGeom;

class ApexEmitterActor : public NxApexEmitterActor, public EmitterActorBase, public NxApexResource, public ApexResource
{
public:
	/* NxApexEmitterActor methods */
	ApexEmitterActor(const NxApexEmitterActorDesc&, ApexEmitterAsset&, NxResourceList&, EmitterScene&);
	~ApexEmitterActor();

	NxApexEmitterAsset*             getEmitterAsset() const;
	physx::PxMat44					getGlobalPose() const
	{
		return PxMat44(mPose);
	}
	void							setCurrentPose(const physx::PxMat44& pose)
	{
		mPose = pose;
	}
	void							setCurrentPosition(const physx::PxVec3& pos)
	{
		mPose.t = pos;
	}
	NxApexRenderable* 				getRenderable()
	{
		return NULL;
	}
	NxApexActor* 					getNxApexActor()
	{
		return this;
	}
	void							removeActorAtIndex(physx::PxU32 index);

	void							getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly);
	physx::PxF32					getActivePhysicalLod();
	void							forcePhysicalLod(physx::PxF32 lod);

	NxEmitterExplicitGeom* 			isExplicitGeom();

	const NxEmitterLodParamDesc&	getLodParamDesc() const
	{
		return mLodParams;
	}
	void							setLodParamDesc(const NxEmitterLodParamDesc& d);

	/* LODNode */
	physx::PxF32					getBenefit();
	physx::PxF32					setResource(physx::PxF32 suggested, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit);

	/* NxApexResource, ApexResource */
	void				            release();
	physx::PxU32				    getListIndex() const
	{
		return m_listIndex;
	}
	void				            setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	/* EmitterActorBase */
	void                            destroy();
	NxApexAsset*		            getOwner() const;
	void							visualize(NiApexRenderDebug& renderDebug);
#if NX_SDK_VERSION_MAJOR == 2
	void							setPhysXScene(NxScene* s)
	{
		mNxScene = s;
	}
	NxScene*						getPhysXScene() const
	{
		return mNxScene;
	}
	NxScene*						mNxScene;
#elif NX_SDK_VERSION_MAJOR == 3
	void							setPhysXScene(PxScene* s)
	{
		mNxScene = s;
	}
	PxScene*						getPhysXScene() const
	{
		return mNxScene;
	}
	PxScene*						mNxScene;
#endif
	void							submitTasks();
	void							setTaskDependencies();
	void							fetchResults();

	void							setDensityRange(const NxRange<physx::PxF32>& r)
	{
		mDensityRange = r;
	}
	void							setRateRange(const NxRange<physx::PxF32>& r)
	{
		mRateRange = r;
	}
	void							setVelocityRange(const NxRange<physx::PxVec3>& r)
	{
		mVelocityRange = r;
	}
	void							setLifetimeRange(const NxRange<physx::PxF32>& r)
	{
		mLifetimeRange = r;
	}

	void							getRateRange(NxRange<physx::PxF32>& r) const
	{
		r = mRateRange;
	}

	void							setOverlapTestCollisionGroups(physx::PxU32 g)
	{
		mOverlapTestCollisionGroups = g;
	}
#if NX_SDK_VERSION_MAJOR == 2
	void							setOverlapTestCollisionGroupsMask(NxGroupsMask*);
#endif

	physx::PxU32					getOverlapTestCollisionGroups() const
	{
		return mOverlapTestCollisionGroups;
	}
#if NX_SDK_VERSION_MAJOR == 2
	const NxGroupsMask*				getOverlapTestCollisionGroupsMask() const
	{
		return mShouldUseGroupsMask ? &mOverlapTestCollisionGroupsMask : NULL;
	}
#endif

	virtual void				setDensityGridPosition(const physx::PxVec3 &pos);

	virtual void setApexEmitterValidateCallback(NxApexEmitterValidateCallback *callback) 
	{
		mEmitterValidateCallback = callback;
	}

	void							startEmit(bool persistent);
	void							stopEmit();
	bool							isEmitting()
	{
		return mDoEmit;
	}

	void							emitAssetParticles(bool enable)
	{
		mEmitAssetParticles = enable;
	}
	bool							getEmitAssetParticles() const
	{
		return mEmitAssetParticles;
	}

#if NX_SDK_VERSION_MAJOR == 2
	void							setAttachActor(NxActor* a)
	{
		mAttachActor = a;
	}
	const NxActor* 					getAttachActor() const
	{
		return mAttachActor;
	}
	NxActor* 						mAttachActor;
#elif NX_SDK_VERSION_MAJOR == 3
	void							setAttachActor(PxActor* a)
	{
		mAttachActor = a;
	}
	const PxActor* 					getAttachActor() const
	{
		return mAttachActor;
	}
	PxActor* 						mAttachActor;
#endif
	void							setAttachRelativePose(const physx::PxMat44& p)
	{
		mAttachRelativePose = p;
	}

	const physx::PxMat44			getAttachRelativePose() const
	{
		return PxMat44(mAttachRelativePose);
	}
	physx::PxF32					getObjectRadius() const
	{
		return mObjectRadius;
	}
	void							setPreferredRenderVolume(physx::apex::NxApexRenderVolume*);

	static physx::PxVec3			random(const NxRange<physx::PxVec3>& range, QDSRand& rand);
	static physx::PxF32				random(const NxRange<physx::PxF32>& range, QDSRand& rand);

	void							setSeed(PxU32 seed)
	{
		mRand.setSeed(seed);
	}

	virtual PxU32					getSimParticlesCount() const
	{
		return	mInjector->getSimParticlesCount();
	}

	virtual PxU32					getActiveParticleCount() const;

protected:
	physx::PxU32					computeNbEmittedFromRate(physx::PxF32 dt);
	bool							overlapsWithCollision(const physx::PxBounds3& bounds);
	bool							overlapsWithCollision(const physx::PxVec3& pos);
	void							emitObjects(physx::PxU32 toEmitNum, bool useFullVolume);
	void							emitObjects(const physx::Array<physx::PxVec3>& positions, physx::PxBounds3& bounds);
	void							tick();

	NxApexEmitterValidateCallback	*mEmitterValidateCallback;
	NiIosInjector* 					mInjector;
	NiInstancedObjectSimulation		*mIOS;
	ApexEmitterAsset* 		    	mAsset;
	EmitterScene* 					mScene;
	physx::PxBounds3				mOverlapAABB;
	physx::PxBounds3				mLastNonEmptyOverlapAABB;

#if NX_SDK_VERSION_MAJOR == 2
	NxGroupsMask						mOverlapTestCollisionGroupsMask;
#endif

	/* runtime state */
	physx::PxMat34Legacy				mPose;
	physx::Array<physx::PxMat34Legacy>	mPoses;
	bool								mFirstStartEmitCall;
	bool								mDoEmit;
	bool								mEmitAssetParticles;
	bool                            	mPersist;
	physx::PxF32						mRemainder;
	physx::PxF32						mEmitterVolume;
	bool								mIsOldPoseInitialized;
	physx::PxMat34Legacy				mOldPose;
	NxRange<physx::PxF32>				mDensityRange;
	NxRange<physx::PxF32>				mRateRange;
	NxRange<physx::PxVec3>				mVelocityRange;
	NxRange<physx::PxF32>				mLifetimeRange;
	physx::PxF32						mObjectRadius;
	physx::PxF32						mEmitDuration;
	physx::PxF32						mDescEmitDuration;
	physx::PxU32						mOverlapTestCollisionGroups;
	bool								mShouldUseGroupsMask;
	physx::Array<IosNewObject>			mNewObjectArray;
	physx::Array<physx::PxVec3>			mNewPositions;
	physx::Array<physx::PxVec3>			mNewVelocities;
	physx::Array<physx::PxU32>			mNewUserData;

	EmitterGeom*						mExplicitGeom;
	NxEmitterLodParamDesc				mLodParams;
	physx::PxMat34Legacy				mAttachRelativePose;

	physx::QDSRand						mRand;

	class ApexEmitterTickTask : public physx::PxTask
	{
	private:
		ApexEmitterTickTask& operator=(const ApexEmitterTickTask&);

	public:
		ApexEmitterTickTask(ApexEmitterActor& actor) : mActor(actor) {}

		const char* getName() const
		{
			return "ApexEmitterActor::TickTask";
		}
		void run()
		{
			mActor.tick();
		}

	protected:
		ApexEmitterActor& mActor;
	};
	ApexEmitterTickTask 				mTickTask;

	friend class EmitterScene;
};

}
}
} // end namespace physx::apex

#endif
