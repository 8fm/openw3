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
#if NX_SDK_VERSION_MAJOR == 3

#include "NxApex.h"
#include "NiApexScene.h"
#include "NiApexSDK.h"

#include "NxParticleIosActor.h"
#include "ParticleIosActorCPU.h"
#include "ParticleIosAsset.h"
#include "NxIofxAsset.h"
#include "NxIofxActor.h"
#include "ModuleParticleIos.h"
#include "ParticleIosScene.h"
#include "NiApexRenderDebug.h"
#include "NiApexAuthorableObject.h"
#include "NiFieldSamplerQuery.h"
#include "foundation/PxMath.h"
#include "ApexMirroredArray.h"

#include "PxParticleSystem.h"
#include "PxParticleCreationData.h"
#include "PxParticleReadData.h"
#include "PxParticleDeviceExclusive.h"

namespace physx
{
namespace apex
{
namespace pxparticleios
{

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

ParticleIosActorCPU::ParticleIosActorCPU(
    NxResourceList& list,
    ParticleIosAsset& asset,
    ParticleIosScene& scene,
	NxIofxAsset& iofxAsset)
	: ParticleIosActor(list, asset, scene, iofxAsset, false)
	, mSimulateTask(*this)
{
	initStorageGroups(mSimulationStorage);

	mField.reserve(mMaxParticleCount);
	mLifeTime.setSize(mMaxParticleCount);
	mLifeSpan.setSize(mMaxTotalParticleCount);
	mInjector.setSize(mMaxTotalParticleCount);
	mBenefit.setSize(mMaxTotalParticleCount);

	mNewIndices.resize(mMaxParticleCount);
	mAddedParticleList.reserve(mMaxParticleCount);
	mRemovedParticleList.reserve(mMaxParticleCount);
	mInputIdToParticleIndex.setSize(mMaxParticleCount, ApexMirroredPlace::CPU);

	mIndexPool = PxParticleExt::createIndexPool(mMaxParticleCount);

	mUpdateIndexBuffer.reserve(mMaxParticleCount);
	mUpdateVelocityBuffer.reserve(mMaxParticleCount);
}
ParticleIosActorCPU::~ParticleIosActorCPU()
{
	if (mIndexPool)
	{
		mIndexPool->release();
		mIndexPool = NULL;
	}	

}

physx::PxTaskID ParticleIosActorCPU::submitTasks(physx::PxTaskManager* tm, physx::PxTaskID taskFinishBeforeID)
{
	ParticleIosActor::submitTasks(tm, taskFinishBeforeID);

	mInjectorsCounters.setSize(mInjectorList.getSize(), ApexMirroredPlace::CPU); 
	physx::PxTaskID	taskID	= tm->submitUnnamedTask(mSimulateTask);
	physx::PxTask*	task	= tm->getTaskFromID(taskID);

	if (taskFinishBeforeID != 0)
	{
		task->finishBefore(taskFinishBeforeID);
	}

	return taskID;
}

void ParticleIosActorCPU::setTaskDependencies()
{
	ParticleIosActor::setTaskDependencies(mSimulateTask, false);
}

namespace
{
class FieldAccessor
{
	const physx::PxVec4* mField;
public:
	explicit FieldAccessor(const physx::PxVec4* field)
	{
		mField = field;
	}

	PX_INLINE void operator()(unsigned int srcIdx, physx::PxVec3& velocityDelta)
	{
		if (mField != NULL)
		{
			velocityDelta += mField[srcIdx].getXYZ();
		}
	}
};
}

void ParticleIosActorCPU::simulateParticles()
{
	PxF32 deltaTime = mParticleIosScene->getApexScene().getPhysXSimulateTime();
	const PxVec3& eyePos = mParticleIosScene->getApexScene().getEyePosition();
	const bool useGridDensityDistance = false;
	const PxF32 gridDensityDistance = 1.f;

	SCOPED_PHYSX_LOCK_WRITE(mParticleIosScene->getApexScene());

	mTotalElapsedTime += deltaTime;

	for (PxU32 i = 0; i < mParticleCount; ++i)
	{
		mNewIndices[i] = NiIosBufferDesc::NOT_A_PARTICLE;
	}
	PxU32 maxStateID = mParticleCount;

	PxU32 totalCount = mParticleCount + mInjectedCount;
	PxU32 activeCount = mLastActiveCount + mInjectedCount;
	PxU32 targetCount = mParticleBudget;

	for(PxU32 i = 0; i < mInjectorList.getSize(); ++i)
	{
		mInjectorsCounters[i] = 0; 
	}


	if (targetCount)
	{
		PxU32 boundCount = 0;
		if (activeCount > targetCount)
		{
			boundCount = activeCount - targetCount;
		}

		PxF32 benefitMin = PxMin(mLastBenefitMin, mInjectedBenefitMin);
		PxF32 benefitMax = PxMax(mLastBenefitMax, mInjectedBenefitMax);
		PX_ASSERT(benefitMin <= benefitMax);
		benefitMax *= 1.00001f;

		/*
			boundBin - the highest benefit bin that should be culled
			boundCount - before computeHistogram it's the total culled particles.
					   - after computeHistogram it's the count of culled particles in boundBin
			boundIndex - count of culled particles in boundBin (0..boundCount-1)
		 */
		PxI32 boundBin = (PxI32)computeHistogram(totalCount, benefitMin, benefitMax, boundCount);
		physx::PxF32	factor = HISTOGRAM_BIN_COUNT / (benefitMax - benefitMin);
		for (PxU32 i = 0, boundIndex = 0; i < totalCount; ++i)
		{
			PxF32 benefit = mBenefit[i];
			if (benefit != -FLT_MAX)
			{
				PX_ASSERT(benefit >= benefitMin && benefit < benefitMax);

				PxI32 bin = PxI32((benefit - benefitMin) * factor);
				if (bin < boundBin)
				{
					mBenefit[i] = -FLT_MAX;
					continue;
				}
				if (bin == boundBin && boundIndex < boundCount)
				{
					mBenefit[i] = -FLT_MAX;
					++boundIndex;
				}
			}
		}
	}

	if (totalCount)
	{
		mRemovedParticleList.clear();
		for (physx::PxU32 i = 0 ; i < totalCount; ++i)
		{
			if (mBenefit[i] == -FLT_MAX && i < mParticleCount)
			{
				mRemovedParticleList.pushBack(mInputIdToParticleIndex[i]);
				mInputIdToParticleIndex[i]	= INVALID_PARTICLE_INDEX;
			}
		}
		if (mRemovedParticleList.size())
		{
			((PxParticleBase*)mParticleActor)->releaseParticles(mRemovedParticleList.size(),
					PxStrideIterator<const PxU32>(mRemovedParticleList.begin()));

			PxStrideIterator<const PxU32> indexData(&mRemovedParticleList[0]);
			mIndexPool->freeIndices(static_cast<PxU32>(mRemovedParticleList.size()), indexData);
			mRemovedParticleList.clear();
		}
	}

	FieldAccessor fieldAccessor(mFieldSamplerQuery ? mField.getPtr() : 0);

	mLastActiveCount = 0;
	mLastBenefitSum  = 0.0f;
	mLastBenefitMin  = +FLT_MAX;
	mLastBenefitMax  = -FLT_MAX;

	if (targetCount > 0)
	{
		const Px3InjectorParams* injectorParamsList = DYNAMIC_CAST(ParticleIosSceneCPU*)(mParticleIosScene)->mInjectorParamsArray.begin();

		mAddedParticleList.clear();
		mUpdateIndexBuffer.clear();
		mUpdateVelocityBuffer.clear();
		PxParticleReadData* readData = ((PxParticleBase*)mParticleActor)->lockParticleReadData();
		for (PxU32 dstIdx = 0, srcHole = targetCount; dstIdx < targetCount; ++dstIdx)
		{
			PxU32 srcIdx	= dstIdx;
			PxU32 pxIdx;
			
			//do we have a hole in dstIdx region?
			if (mBenefit[dstIdx] == -FLT_MAX)
			{
				//skip holes in srcIdx region
				while (mBenefit[srcHole] == -FLT_MAX)
				{
					++srcHole;
				}
				PX_ASSERT(srcHole < totalCount);
				srcIdx = srcHole++;

				if (srcIdx < mParticleCount)
				{
					pxIdx		= mInputIdToParticleIndex[srcIdx];
					mInputIdToParticleIndex[dstIdx]	= pxIdx;
				}
			}
			//do we have a new particle?
			bool isNewParticle = (srcIdx >= mParticleCount);

			if (isNewParticle)
			{
				PxStrideIterator<PxU32> indexBuffer(&pxIdx);
				if (1 != mIndexPool->allocateIndices(1, indexBuffer) )
				{
					PX_ASSERT(0);
					continue;
				}
				
				NewParticleData data;
				data.destIndex	= pxIdx;
				data.position	= mBufDesc.pmaPositionMass->get(srcIdx).getXYZ();
				data.velocity	= mBufDesc.pmaVelocityLife->get(srcIdx).getXYZ();
				mAddedParticleList.pushBack(data);

				mBufDesc.pmaInStateToInput->get(maxStateID) = dstIdx | NiIosBufferDesc::NEW_PARTICLE_FLAG;
				++maxStateID;

				mInputIdToParticleIndex[dstIdx]	= pxIdx;
				if(mBufDesc.pmaDensity)
				{
					if(useGridDensityDistance)
					{
						const PxF32 dist = (data.position-mDensityOrigin).magnitude();
						const PxF32 density = (gridDensityDistance - dist)/gridDensityDistance;
						mBufDesc.pmaDensity->get(dstIdx) = PxMin(PxMax(density,0.f),1.f);
					}
					else
					{
						mBufDesc.pmaDensity->get(dstIdx) = 0.0f;
					}
				}
				
				//Note: Field sampler query is only made for existing particles. No query has been done
				//      for new particles so there is no need to update their velocities.
			}
			else
			{
				pxIdx		= mInputIdToParticleIndex[srcIdx];

				/* Take position and velocity from PhysX fluid simulation */
				PxVec3 pos = readData->positionBuffer[pxIdx];
				PxVec3 vel = readData->velocityBuffer[pxIdx];
				PxParticleFluidReadData* fluidReadData = static_cast<PxParticleFluidReadData*>(readData);
				if(fluidReadData->densityBuffer.ptr())
				{
					mBufDesc.pmaDensity->get(dstIdx) = fluidReadData->densityBuffer[pxIdx];
				}
				else if(mBufDesc.pmaDensity)
				{
					if(useGridDensityDistance)
					{
						const PxF32 dist = (pos-mDensityOrigin).magnitude();
						const PxF32 density = (gridDensityDistance - dist)/gridDensityDistance;
						mBufDesc.pmaDensity->get(dstIdx) = PxMin(PxMax(density,0.f),1.f);
					}
				}

				/* Apply field sampler velocity */
				fieldAccessor(srcIdx, vel);
				mUpdateIndexBuffer.pushBack(pxIdx);
				mUpdateVelocityBuffer.pushBack(vel);

				PxVec4& pm = mBufDesc.pmaPositionMass->get(dstIdx);
				PxVec4& vl = mBufDesc.pmaVelocityLife->get(dstIdx);
				pm	= PxVec4(pos, pm.w);
				vl	= PxVec4(vel, vl.w);

				if (readData->collisionNormalBuffer.ptr())
				{
					const PxVec3&	normal	= readData->collisionNormalBuffer[pxIdx];
					PxU32	flags			= readData->flagsBuffer[pxIdx];
					PxVec4& nf				= mBufDesc.pmaCollisionNormalFlags->get(dstIdx);
					
					nf	= PxVec4(normal, *(const PxF32*)(&flags));
				}

				mNewIndices[srcIdx] = dstIdx;
			}

			unsigned int injIndex;
			float benefit = simulateParticle(
			                    injectorParamsList,
			                    deltaTime, eyePos,
			                    isNewParticle, srcIdx, dstIdx,
			                    mBufDesc.pmaPositionMass->getPtr(), mBufDesc.pmaVelocityLife->getPtr(), 
								mBufDesc.pmaCollisionNormalFlags->getPtr(), mBufDesc.pmaUserData->getPtr(), mBufDesc.pmaActorIdentifiers->getPtr(),
			                    mLifeSpan.getPtr(), mLifeTime.getPtr(), NULL, mInjector.getPtr(),
			                    NULL, NULL, NULL, NULL, NULL,	// g_pxPosition, g_pxVelocity, g_pxCollision, g_pxDensity, g_pxFlags
			                    fieldAccessor, injIndex,
								mGridDensityParams
			                );

			if (injIndex < mInjectorsCounters.getSize())
			{
				++mInjectorsCounters[injIndex]; 
			}

			mBenefit[dstIdx] = benefit;
			if (benefit != -FLT_MAX)
			{
				mLastBenefitSum += benefit;
				mLastBenefitMin = PxMin(mLastBenefitMin, benefit);
				mLastBenefitMax = PxMax(mLastBenefitMax, benefit);
				++mLastActiveCount;
			}
		}

		if (readData)
		{
			readData->unlock();
		}

		if (mUpdateIndexBuffer.size())
		{
			((PxParticleBase*)mParticleActor)->setVelocities(mUpdateIndexBuffer.size(), PxStrideIterator<const PxU32>(&mUpdateIndexBuffer[0]), PxStrideIterator<const PxVec3>(&mUpdateVelocityBuffer[0]));
		}

		if (mAddedParticleList.size())
		{
			PxParticleCreationData createData;
			createData.numParticles = mAddedParticleList.size();
			createData.positionBuffer = PxStrideIterator<const PxVec3>(&mAddedParticleList[0].position, sizeof(NewParticleData));
			createData.velocityBuffer = PxStrideIterator<const PxVec3>(&mAddedParticleList[0].velocity, sizeof(NewParticleData));
			createData.indexBuffer = PxStrideIterator<const PxU32>(&mAddedParticleList[0].destIndex, sizeof(NewParticleData));
			bool ok = ((PxParticleBase*)mParticleActor)->createParticles(createData);
			PX_ASSERT(ok);
			PX_UNUSED(ok);
		}
	}

	//update stateToInput
	for (PxU32 i = 0; i < mParticleCount; ++i)
	{
		PxU32 srcIdx = mBufDesc.pmaOutStateToInput->get(i);
		PX_ASSERT(srcIdx < mParticleCount);
		mBufDesc.pmaInStateToInput->get(i) = mNewIndices[srcIdx];
	}
	mParticleCount = targetCount;


	/* Oh! Manager of the IOFX! do your thing */
	mIofxMgr->updateEffectsData(deltaTime, mParticleCount, mParticleCount, maxStateID);
}

physx::PxU32 ParticleIosActorCPU::computeHistogram(physx::PxU32 dataCount, physx::PxF32 dataMin, physx::PxF32 dataMax, physx::PxU32& bound)
{
	const PxF32* dataArray = mBenefit.getPtr();

	PxU32 histogram[HISTOGRAM_BIN_COUNT];

	//clear Histogram
	for (PxU32 i = 0; i < HISTOGRAM_BIN_COUNT; ++i)
	{
		histogram[i] = 0;
	}

	physx::PxF32	factor = HISTOGRAM_BIN_COUNT / (dataMax - dataMin);
	//accum Histogram
	for (PxU32 i = 0; i < dataCount; ++i)
	{
		PxF32 data = dataArray[i];
		if (data >= dataMin && data < dataMax)
		{
			PxI32 bin = PxI32((data - dataMin) * factor);
			++histogram[bin];
		}
	}
	//compute CDF from Histogram
	PxU32 countSum = 0;
	for (PxU32 i = 0; i < HISTOGRAM_BIN_COUNT; ++i)
	{
		PxU32 count = histogram[i];
		countSum += count;
		histogram[i] = countSum;
	}

	PX_ASSERT(countSum == mLastActiveCount + mInjectedCount);

	//binary search in CDF
	PxU32 beg = 0;
	PxU32 end = HISTOGRAM_BIN_COUNT;
	while (beg < end)
	{
		PxU32 mid = beg + ((end - beg) >> 1);
		if (bound > histogram[mid])
		{
			beg = mid + 1;
		}
		else
		{
			end = mid;
		}
	}

	PX_ASSERT(histogram[beg] >= bound);
	if (beg > 0)
	{
		bound -= histogram[beg - 1];
	}

	return beg;
}

}
}
} // end namespace physx::apex

#endif // NX_SDK_VERSION_MAJOR == 3
