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

#ifndef __FLUID_IOS_ACTOR_H__
#define __FLUID_IOS_ACTOR_H__

#include "NxApex.h"

#include "NxFluidIosActor.h"
#include "FluidIosAsset.h"
#include "NiInstancedObjectSimulation.h"
#include "FluidIosScene.h"
#include "ApexActor.h"
#include "ApexContext.h"
#include "LeastBenefit.h"
#include "NiIofxManager.h"
#include "ApexConstrainedDistributor.h"

namespace physx
{
namespace apex
{

namespace iofx
{
class NxApexRenderVolume;
class NxIofxAsset;
}

namespace nxfluidios
{
class FluidParticleInjector;

struct ParticleData
{
	NiIofxActorID			iofxActorID;
	physx::PxF32			lifespan;
	physx::PxF32			lodBenefit;
	bool                    iosDeleted;
	FluidParticleInjector*  injector;
};

/************************************************************************/
/* Template for a Graph axis that zooms in and out by itself            */
/************************************************************************/
template<class T>	// float or integer
class PersistentGraphAxis		//TODO: move somewhere else!
{
	T						lastValue;
public:
	PersistentGraphAxis()
	{
		lastValue = 0;
	}
	T getGraphAxis(T currValue, bool canZoomIn)
	{
		if ((currValue > lastValue)	//zoom out
		        ||
		        ((currValue * 3 < lastValue) && canZoomIn)//only using 1/3rd of the axis for the chart --> zoom in
		   )
		{
			lastValue = currValue;
		}

		return lastValue;
	}
};


class FluidIosActor : public NiInstancedObjectSimulation,
	public NxFluidIosActor,
	public NxApexResource,
	public ApexResource,
	public LODNode
{
public:
	FluidIosActor(NxResourceList&, FluidIosAsset&, FluidIosScene&, NxIofxAsset* iofxAsset);
	~FluidIosActor();

	void						submitTasks();
	void						setTaskDependencies();

	// NxApexInterface API
	void						release();
	void						destroy();

	// NxApexActor API
	void						setPhysXScene(NxScene* s);
	NxScene*					getPhysXScene() const;
	NxApexAsset*				getOwner() const
	{
		return (NxApexAsset*) mAsset;
	}

	void						getPhysicalLodRange(physx::PxF32& min, physx::PxF32& max, bool& intOnly);
	physx::PxF32				getActivePhysicalLod();
	void						forcePhysicalLod(physx::PxF32 lod);

	// LODNode API
	physx::PxF32				getBenefit();
	physx::PxF32				setResource(physx::PxF32 suggested, physx::PxF32 maxRemaining, physx::PxF32 relativeBenefit);

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

	// ParticleSystemBase methods
	void						fetchResults();
	void						visualize();

	// NiIOS
	physx::PxF32				getObjectRadius() const
	{
		return mAsset->getParticleRadius();
	}
	physx::PxF32				getObjectDensity() const
	{
		return mRestDensity;
	}
	NiIosInjector*				allocateInjector(const char* iofxAssetName);
	void                        releaseInjector(NiIosInjector&);

	NxCollisionGroup            getCollisionGroup() const
	{
		return mCollisionGroup;
	}
	physx::PxU32				getMaxParticleCount() const
	{
		return mMaxParticleCount;
	}
	physx::PxU32				getParticleCount() const
	{
		return mParticleCount;
	}
	const physx::PxVec3* 		getRecentPositions(physx::PxU32& count, physx::PxU32& stride) const;
	physx::PxF32                getLeastBenefitValue() const
	{
		return mLeastBenefit;
	}

	/* task entry point functions */
	void						prepareBenefit();
	void						cullAndReplace();
	void						postUpdateEffects();

	/* Internal utility functions */
	void						putInScene(NxScene* scene);
	void						removeFromScene();

	PxF32						calcParticleBenefit(const FluidParticleInjector& inj, const PxVec3& eyePos, const PxVec3& pos, const PxVec3& vel, PxF32 life) const;
	PxU32						fluidSubStepCount();

	void						distributeBudgetAmongInjectors();

	FluidIosAsset* 				mAsset;
	FluidIosScene* 				mParticleScene;
	NxFluid* 					mFluid;
	NxCompartment* 				mCompartment;
	NiIofxManager*              mIofxMgr;
	NiIosBufferDesc				mBufDesc;
	NxResourceList				mInjectors;

	NxCollisionGroup            mCollisionGroup;
	physx::PxU32				mMaxParticleCount;
	physx::PxU32				mMaxInsertionCount;
	physx::PxF32				mRestDensity;

	physx::PxF32				mParticleMass;

	// buffer for addParticles calls
	struct IosNewParticle
	{
		IosNewObject	object;
		ParticleData    data;
		PxU32           newStateID;
	};
	physx::Array<IosNewParticle> mAddBuffer;

	// buffer to update particles (particle removal)
	struct IosDelParticle
	{
		PxU32	nxid;
		PxU32	flags;
	};
	physx::Array<IosDelParticle> mDelBuffer;

	physx::Array<NxIofxAsset*>	mIofxAssets;
	physx::Array<PxU32>			mIofxAssetRefs;

	// NxFluid particle data written by PhysX simulation is a series of arrays
	// Most data is written into buffers provided by the IOFX Manager, but these
	// Two fields are needed only by the IOS.
	PxU32						mParticleCount;
	physx::Array<PxU32>			mInputToNx;
	physx::Array<PxF32>			mLifetime;
	PxU32						mSubmittedParticleCount;

	physx::Array<PxU32>         mNxToState;			// Maps NXID to stateID, persistent
	physx::Array<ParticleData>	mParticleData;		// IOS particle data, indexed by NxID
	PxU32                       mMaxStateID;
	PxU32                       mMaxInputID;
	physx::Array<PxU32>         mDeletedIDs;
	physx::Array<PxU32>         mHomelessParticles;
	PxU32                       mNumDeletedIDs;
	physx::Array<PxU32>         mReuseStateIDs;

	// Used during LOD
	//LeastBenefitList<PxU32>		mForceDeleteList;
	PxF32						mLeastBenefit;
	physx::Array<PxU32>         mForceDeleteArray;

	// Output of nxFluid->addParticles()
	PxU32						mCreatedParticleCount;
	physx::Array<PxU32>			mCreatedIDs;

	ApexConstrainedDistributor<PxU32>	mBudgetDistributor;

	class TaskPrepareBenefit : public physx::PxTask
	{
	public:
		TaskPrepareBenefit(FluidIosActor& owner) : mOwner(owner) {}
		const char* getName() const
		{
			return "FluidIosActor::PrepareBenefit";
		}
		void run();

	protected:
		FluidIosActor& mOwner;

	private:
		TaskPrepareBenefit& operator=(const TaskPrepareBenefit&);
	};
	TaskPrepareBenefit			mPrepareBenefitTask;

	class TaskUpdate : public physx::PxTask
	{
	public:
		TaskUpdate(FluidIosActor& owner) : mOwner(owner) {}
		const char* getName() const
		{
			return "FluidIosActor::CullAndReplace";
		}
		void run();

	protected:
		FluidIosActor& mOwner;

	private:
		TaskUpdate& operator=(const TaskUpdate&);
	};
	TaskUpdate					mUpdateTask;

	class TaskPostUpdate : public physx::PxTask
	{
	public:
		TaskPostUpdate(FluidIosActor& owner) : mOwner(owner) {}
		const char* getName() const
		{
			return "FluidIosActor::PostUpdate";
		}
		void run();

	protected:
		FluidIosActor& mOwner;

	private:
		TaskPostUpdate& operator=(const TaskPostUpdate&);
	};
	TaskPostUpdate				mPostUpdateTask;

	bool						mNxFluidBroken;

	// Helper class used to empty injectors into NxFluid
	struct BusyInjector
	{
		BusyInjector(FluidParticleInjector* i);

		void                    markDeleted(PxU32 i);
		IosNewObject&			read();
		bool					empty();

		FluidParticleInjector* inj;
		PxU32                  readid;
		PxU32                  remain;
	};
	physx::Array<BusyInjector>  mBusyInj;

	//struct InjectedBenefit
	//{
	//	PxU32		injid;
	//	PxU32		index;
	//};
	//LeastBenefitList<InjectedBenefit> mInjectorCullList;


#if !defined(WITHOUT_DEBUG_VISUALIZE)
	physx::PxF32						mLodRelativeBenefit;
	physx::PxI32						mDebugRenderGroupID;
	PersistentGraphAxis<physx::PxF32>	mHistogramYAxis;
	PersistentGraphAxis<physx::PxF32>	mHistogramXAxis;
	PersistentGraphAxis<physx::PxF32>	mSortedPtsYAxis;
	PersistentGraphAxis<physx::PxU32>	mSortedPtsXAxis;
	struct DeleteVisInfo
	{
		physx::PxVec3 position;
		physx::PxI32  deletionTime;
	};
	physx::Array<DeleteVisInfo>	mVisualizeDeletedPositions;
	physx::PxU32				mTotalElapsedMS;
	physx::PxU32                mLastVisualizeMS;
#endif

	// Only for use by the IOS Asset, the actor is unaware of this
	bool mIsMesh;
};

}
}
} // namespace physx::apex

#endif // __FLUID_IOS_ACTOR_H__
