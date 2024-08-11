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

#ifndef __PARTICLE_IOS_ACTOR_H__
#define __PARTICLE_IOS_ACTOR_H__

#include "NxApex.h"

#include "NxParticleIosActor.h"
#include "NxIofxActor.h"
#include "ParticleIosAsset.h"
#include "NiInstancedObjectSimulation.h"
#include "ParticleIosScene.h"
#include "ApexActor.h"
#include "ApexContext.h"
#include "ApexFIFO.h"
#include "ParticleIosCommon.h"
#include "InplaceStorage.h"
#include "ApexMirroredArray.h"
#include "PxParticleExt.h"

namespace physx
{
namespace apex
{

class NiFieldSamplerQuery;
class NiFieldSamplerCallback;

namespace pxparticleios
{

/* Class for managing the interactions with each emitter */
class ParticleParticleInjector : public NiIosInjector, public NxApexResource, public ApexResource
{
public:
	void	setPreferredRenderVolume(NxApexRenderVolume* volume);
	PxF32	getLeastBenefitValue() const
	{
		return 0.0f;
	}
	bool	isBacklogged() const
	{
		return false;
	}

	void	createObjects(physx::PxU32 count, const IosNewObject* createList);
#if defined(APEX_CUDA_SUPPORT)
	void	createObjects(ApexMirroredArray<const IosNewObject>& createArray);
#endif

	void	setLODWeights(physx::PxF32 maxDistance, physx::PxF32 distanceWeight, physx::PxF32 speedWeight, physx::PxF32 lifeWeight, physx::PxF32 separationWeight, physx::PxF32 bias);

	physx::PxTaskID getCompletionTaskID() const;

	virtual physx::PxF32 getBenefit();
	virtual physx::PxF32 setResource(physx::PxF32 suggested, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit);

	void	release();
	void	destroy();

	// NxApexResource methods
	void	setListIndex(NxResourceList& list, physx::PxU32 index);
	
	physx::PxU32		getListIndex() const
	{
		return m_listIndex;
	}

	virtual void		setPhysXScene(PxScene*)	{}
	virtual PxScene*	getPhysXScene() const
	{
		return NULL;
	}

	void				assignSimParticlesCount(PxU32 input)
	{
		mSimulatedParticlesCount = input;
	}

	virtual PxU32		getSimParticlesCount() const
	{
		return mSimulatedParticlesCount;
	}

	virtual physx::PxU32 getActivePaticleCount() const;

protected:
	ParticleIosActor* 		mIosActor;
	NxResID					mIofxAssetResID;
	NxIofxAsset* 			mIofxAsset;
	NxApexRenderVolume* 	mVolume;
	physx::Array<PxU16>		mRandomActorClassIDs;
	PxU32					mLastRandomID;
	PxU16					mVolumeID;

	PxU32					mInjectorID;
	PxU32					mSimulatedParticlesCount;


	/* insertion buffer */
	ApexFIFO<IosNewObject>	mInjectedParticles;

	ParticleParticleInjector(NxResourceList& list, ParticleIosActor& actor, physx::PxU32 injectorID);
	~ParticleParticleInjector() {}

	void init(NxResID iofxAssetResID, NxIofxAsset* iofxAsset);

	friend class ParticleIosActor;
};


class ParticleIosActor : public NiInstancedObjectSimulation,
	public NxParticleIosActor,
	public NxApexResource,
	public ApexResource,
	public LODNode
{
public:
	ParticleIosActor(NxResourceList& list, ParticleIosAsset& asset, ParticleIosScene& scene, NxIofxAsset& iofxAsset, bool isDataOnDevice);
	~ParticleIosActor();

	// NxApexInterface API
	void								release();
	void								destroy();

	// NxApexActor API
	void								setPhysXScene(PxScene* s);
	PxScene*							getPhysXScene() const;
	virtual void						putInScene(PxScene* scene);

	NxApexAsset*						getOwner() const
	{
		return (NxApexAsset*) mAsset;
	}

	// ApexContext
	void								getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly);
	physx::PxF32						getActivePhysicalLod();
	void								forcePhysicalLod(physx::PxF32 lod);

	// NxApexResource methods
	void								setListIndex(NxResourceList& list, physx::PxU32 index)
	{
		m_listIndex = index;
		m_list = &list;
	}
	physx::PxU32						getListIndex() const
	{
		return m_listIndex;
	}

	// LODNode API
	PxF32								getBenefit();
	PxF32								setResource(physx::PxF32 suggested, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit);

	// NiIOS
	physx::PxF32						getObjectRadius() const
	{
		return getParticleRadius();
	}
	physx::PxF32						getObjectDensity() const
	{
		return 1.0f;
	} // mAsset->getRestDensity(); }

	// NxParticleIosActor
	physx::PxF32						getParticleRadius() const
	{
		return mAsset->getParticleRadius();
	}
	//physx::PxF32						getRestDensity() const			{ return mAsset->getRestDensity();}
	physx::PxU32						getParticleCount() const
	{
		return mParticleCount;
	}

	PX_INLINE void						setOnStartFSCallback(NiFieldSamplerCallback* callback)
	{
		if (mFieldSamplerQuery)
		{
			mFieldSamplerQuery->setOnStartCallback(callback);
		}
	}
	PX_INLINE void						setOnFinishIOFXCallback(NiIofxManagerCallback* callback)
	{
		if (mIofxMgr)
		{
			mIofxMgr->setOnFinishCallback(callback);
		}
	}

	const physx::PxVec3* 				getRecentPositions(physx::PxU32& count, physx::PxU32& stride) const;

	void								visualize();
	virtual physx::PxTaskID				submitTasks(physx::PxTaskManager* tm, physx::PxTaskID taskFinishBeforeID) = 0;
	virtual void						setTaskDependencies() = 0;
	virtual void						fetchResults();

	NiIosInjector*						allocateInjector(const char* iofxAssetName);
	void								releaseInjector(NiIosInjector&);

	virtual void								setDensityOrigin(const PxVec3& v) 
	{ 
		mDensityOrigin = v; 
	}

protected:
	virtual void						removeFromScene();

	void								injectNewParticles();
	bool								isParticleDescValid( const ParticleIosAssetParam* desc) const;
	void								initStorageGroups(InplaceStorage& storage);

	void								setTaskDependencies(physx::PxTask& iosTask, bool isDataOnDevice);

	ParticleIosAsset* 					mAsset;
	ParticleIosScene* 					mParticleIosScene;
	bool								mIsParticleSystem;	// true:SimpleParticleSystemParams , false:FluidParticleSystemParams
	PxActor*							mParticleActor;

	NiIofxManager* 						mIofxMgr;
	NiIosBufferDesc						mBufDesc;

	NxResourceList						mInjectorList;

	physx::Array<NxIofxAsset*>			mIofxAssets;
	physx::Array<PxU32>					mIofxAssetRefs;

	physx::PxVec3						mUp;
	physx::PxF32						mGravity;
	physx::PxF32						mTotalElapsedTime;			//AM: People, methinks this will backfire eventually due to floating point precision loss!

	physx::PxU32						mMaxParticleCount;
	physx::PxU32						mMaxTotalParticleCount;

	physx::PxU32						mParticleCount;
	physx::PxU32						mParticleBudget;

	physx::PxU32						mInjectedCount;
	physx::PxF32						mInjectedBenefitSum;
	physx::PxF32						mInjectedBenefitMin;
	physx::PxF32						mInjectedBenefitMax;

	physx::PxU32						mLastActiveCount;
	physx::PxF32						mLastBenefitSum;
	physx::PxF32						mLastBenefitMin;
	physx::PxF32						mLastBenefitMax;

	ApexMirroredArray<physx::PxF32>		mLifeSpan;
	ApexMirroredArray<physx::PxF32>		mLifeTime;
	ApexMirroredArray<physx::PxU32>		mInjector;
	ApexMirroredArray<physx::PxF32>		mBenefit;
	ApexMirroredArray<physx::PxU32>		mInjectorsCounters;
	ApexMirroredArray<physx::PxU32>		mInputIdToParticleIndex;

	ApexMirroredArray<physx::PxF32>		mGridDensityGrid;
	ApexMirroredArray<physx::PxF32>		mGridDensityGridLowPass;

	// Only for use by the IOS Asset, the actor is unaware of this
	bool								mIsMesh;
	
	NiFieldSamplerQuery*				mFieldSamplerQuery;
	ApexMirroredArray<physx::PxVec4>	mField;

	InplaceStorageGroup					mSimulationStorageGroup;

	class InjectTask : public physx::PxTask
	{
	public:
		InjectTask(ParticleIosActor& actor) : mActor(actor) {}

		const char* getName() const
		{
			return "ParticleIosActor::InjectTask";
		}
		void run()
		{
			mActor.injectNewParticles();
		}

	protected:
		ParticleIosActor& mActor;

	private:
		InjectTask& operator=(const InjectTask&);
	};
	InjectTask							mInjectTask;

	// Particle Density Origin
	PxVec3 mDensityOrigin;
	PxMat44 mDensityDebugMatInv;

	GridDensityParams		mGridDensityParams;

	NiFieldSamplerCallback*				mOnStartCallback;
	NiIofxManagerCallback*				mOnFinishCallback;

	friend class ParticleIosAsset;
	friend class ParticleParticleInjector;
};

}
}
} // namespace physx::apex

#endif // __PARTICLE_IOS_ACTOR_H__
