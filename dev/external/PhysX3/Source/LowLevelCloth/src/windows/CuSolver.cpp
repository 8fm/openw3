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

#include "CuSolver.h"
#include "CuCloth.h"
#include "ClothImpl.h"
#include "CuFabric.h"
#include "CuFactory.h"
#include "CuSolverKernel.h"
#include "CuContextLock.h"
#include "CuCheckSuccess.h"
#include "IterationState.h"
#include "PxCudaContextManager.h"
#include "PxProfileZone.h"
#include "PxGpuDispatcher.h"
#include "CudaKernelWrangler.h"
#include "PsUtilities.h"
#include "PsSort.h"
#include "nvToolsExt.h"

//#define ENABLE_CUDA_PRINTF PX_DEBUG // warning: not thread safe

#if ENABLE_CUDA_PRINTF
extern "C" cudaError_t cudaPrintfInit(CUmodule hmod, size_t bufferLen=1048576);
extern "C" void cudaPrintfEnd();
extern "C" cudaError_t cudaPrintfDisplay(CUmodule hmod, void *outputFP=NULL, bool showThreadID=false);
#endif

using namespace physx;

namespace
{
	const char* gKernelNames[] =
	{
		cloth::getKernelFunctionName(),
	};

	// Note: gCuProfileZoneNames has a corresponding enum list (CuProfileZoneIds) in CuSolverKernel.h.
	// Additions/deletions to gCuProfileZoneNames requires a similar action to CuProfileZoneIds.
	const char* gCuProfileZoneNames[] =
	{
		"cloth::CuSolverKernel::simulateKernel",
		"cloth::CuSolverKernel::integrateParticles",
		"cloth::CuSolverKernel::accelerateParticles",
		"cloth::CuSolverKernel::constrainTether",
		"cloth::CuSolverKernel::solveFabric",
		"cloth::CuSolverKernel::constrainMotion",
		"cloth::CuSolverKernel::constrainSeparation",
		"cloth::CuSolverKernel::collideParticles",
		"cloth::CuSolverKernel::selfCollideParticles",
		"cloth::CuSolverKernel::updateSleepState",
		"cloth::CuSolverKernel::solveConstraintSet",
		"cloth::CuCollision::buildAccleration",
		"cloth::CuCollision::collideCapsules",
		"cloth::CuCollision::collideVirtualCapsules",
		"cloth::CuCollision::collideContinuousCapsules",
		"cloth::CuCollision::collideConvexes",
		"cloth::CuCollision::collideTriangles",
		"cloth::CuSelfCollision::buildAccleration",
		"cloth::CuSelfCollision::collideParticles",
	};
}

namespace
{
	template <typename T>
	struct CuDeviceAllocator
	{
		CuDeviceAllocator(PxCudaContextManager* ctx) 
			: mManager(ctx->getMemoryManager())
		{}

		T* allocate(size_t n)
		{
			return reinterpret_cast<T*>(mManager->alloc(PxCudaBufferMemorySpace::T_GPU, n * sizeof(T)));
		}

		void deallocate(T* ptr)
		{
			mManager->free(PxCudaBufferMemorySpace::T_GPU, reinterpret_cast<PxCudaBufferPtr>(ptr));
		}

		PxCudaMemoryManager* mManager;
	};
}

cloth::CuSolver::CuSolver(CuFactory& factory, PxProfileZone* profiler)
: CuContextLock(factory),
  mFactory(factory),
  mFrameDt(0.0f),
  mSharedMemorySize(0), 
  mSharedMemoryLimit(0),
  mStartSimulationTask(&CuSolver::beginFrame, "cloth::CuSolver::StartSimulationTask"),
  mKernelSimulationTask(&CuSolver::executeKernel, "cloth::CuSolver::KernelSimulationTask"),
  mEndSimulationTask(&CuSolver::endFrame, "cloth::CuSolver::EndSimulationTask"),
  mClothData(mFactory.mContextManager),
  mClothDataHostCopy(CuHostAllocator(mFactory.mContextManager, cudaHostAllocWriteCombined)),
  mClothDataDirty(false),
  mFrameData(getMappedAllocator<CuFrameData>(mFactory.mContextManager)),
  mIterationData(getMappedAllocator<CuIterationData>(mFactory.mContextManager)),
  mIterationDataBegin(0),
  mKernelModule(0),
  mKernelFunction(0),
  mKernelSharedMemorySize(0),
  mStream(0),
  mClothIndex(CuDeviceAllocator<uint32_t>(mFactory.mContextManager).allocate(1)),
  mInterCollisionDistance(0.0f), 
  mInterCollisionStiffness(1.0f), 
  mInterCollisionIterations(1),
  mInterCollisionScratchMem(NULL),
  mInterCollisionScratchMemSize(0),
  mProfiler(profiler), 
  mSimulateEventId(mProfiler ? mProfiler->getEventIdForName("cloth::CuSolver::simulate") : -1),
  mSimulateNvtxRangeId(0),
  mProfileBuffer(0),
  mKernelWrangler(getDispatcher(), gKernelNames, sizeof(gKernelNames) / sizeof(char*)),
  mProfileBaseId(getDispatcher().registerKernelNames(gCuProfileZoneNames, CuProfileZoneIds::NUMZONES)),
  mCudaError(mKernelWrangler.hadError())
{
	PX_ASSERT(CuProfileZoneIds::NUMZONES == PX_ARRAY_SIZE(gCuProfileZoneNames));

	if (mCudaError)
	{
		CuContextLock::release();
		return;
	}

	mStartSimulationTask.mSolver = this;
	mKernelSimulationTask.mSolver = this;
	mEndSimulationTask.mSolver = this;

	if(mFactory.mContextManager->getUsingConcurrentStreams())
		checkSuccess( cuStreamCreate(&mStream, 0) );

	mKernelModule = mKernelWrangler.getCuModule(CuProfileZoneIds::SIMULATE);
	mKernelFunction = mKernelWrangler.getCuFunction(CuProfileZoneIds::SIMULATE);

	// get amount of statically allocated shared memory 
	checkSuccess( cuFuncGetAttribute(&mKernelSharedMemorySize,
		CU_FUNC_ATTRIBUTE_SHARED_SIZE_BYTES, mKernelFunction) );

	// extract CuKernelData device pointer
	size_t size = 0;
	CUdeviceptr ptr = 0;
	checkSuccess( cuModuleGetGlobal(&ptr, &size, mKernelModule, getKernelDataName()) );
	mKernelData = CuDevicePointer<CuKernelData>(reinterpret_cast<CuKernelData*>(ptr));

	// initialize cloth index
	checkSuccess( cuMemsetD32(mClothIndex.dev(), 0, 1) );

	CuContextLock::release();
}

cloth::CuSolver::~CuSolver()
{
	PX_ASSERT(mCloths.empty());

	CuContextLock::acquire();

	CuKernelData kernelData = {}; 
	*mKernelData = kernelData;

	CuDeviceAllocator<uint32_t>(mFactory.mContextManager).deallocate(mClothIndex.get());

	if (mStream)
		checkSuccess( cuStreamDestroy(mStream) );

	if (mInterCollisionScratchMem)
		PX_FREE(mInterCollisionScratchMem);
}

void cloth::CuSolver::updateKernelData()
{
	CuKernelData kernelData;

	kernelData.mClothIndex = mClothIndex.get();
	kernelData.mClothData = mClothData.begin().get();
	kernelData.mFrameData = getDevicePointer(mFrameData);

	kernelData.mProfileBuffer = mProfileBuffer;
	kernelData.mProfileBaseId = mProfileBaseId;

	*mKernelData = kernelData;
}

PxGpuDispatcher& cloth::CuSolver::getDispatcher() const
{
	return *mFactory.mContextManager->getGpuDispatcher();
}

namespace
{
	struct ClothSimCostGreater
	{
		bool operator()(const cloth::CuCloth* left, const cloth::CuCloth* right) const
		{
			return left->mNumParticles *  left->mSolverFrequency
				> right->mNumParticles * right->mSolverFrequency;
		}
	};
}

void cloth::CuSolver::addCloth( Cloth* cloth )
{
	CuCloth& cuCloth = static_cast<CuClothImpl&>(*cloth).mCloth;

	PX_ASSERT(mCloths.find(&cuCloth) == mCloths.end());

	mCloths.pushBack(&cuCloth);
	// trigger update of mClothData array
	cuCloth.notifyChanged();

	// sort cloth instances by size
	shdfnd::sort(mCloths.begin(), mCloths.size(), ClothSimCostGreater());

	CuContextLock contextLock(mFactory);

	// resize containers and update kernel data
	mClothDataHostCopy.resize(mCloths.size());
	mClothData.resize(mCloths.size());
	mFrameData.resize(mCloths.size());
	updateKernelData();
}

void cloth::CuSolver::removeCloth( Cloth* cloth )
{
	CuCloth& cuCloth = static_cast<CuClothImpl&>(*cloth).mCloth;

	ClothVector::Iterator begin = mCloths.begin(), end = mCloths.end();
	ClothVector::Iterator it = mCloths.find(&cuCloth);

	if(it == end)
		return; // not found

	uint32_t index = uint32_t(it - begin);

	mCloths.remove(index);
	mClothDataHostCopy.remove(index);
	mClothData.resize(mCloths.size());
	mClothDataDirty = true;
}

PxBaseTask& cloth::CuSolver::simulate(
	float dt, PxBaseTask& continuation)
{
	mFrameDt = dt;

	if(mCloths.empty() || mCudaError)
	{
		continuation.addReference();
		return continuation;
	}

	PxGpuDispatcher& disp = getDispatcher();
	mEndSimulationTask.setContinuation(&continuation);
	disp.addPostLaunchDependent(mEndSimulationTask);
	mKernelSimulationTask.setContinuation(&disp.getPostLaunchTask());
	disp.getPostLaunchTask().removeReference();
	disp.addPreLaunchDependent(mKernelSimulationTask);
	mStartSimulationTask.setContinuation(&disp.getPreLaunchTask());
	disp.getPreLaunchTask().removeReference();

	mEndSimulationTask.removeReference();
	mKernelSimulationTask.removeReference();

	return mStartSimulationTask;
}

void cloth::CuSolver::beginFrame()
{
	CuContextLock contextLock(mFactory);

	if(mProfiler)
	{
		mProfiler->startEvent(mSimulateEventId, uintptr_t(this), 
			uint32_t(uintptr_t(this) & 0xffffffff));
	}
#ifdef PX_NVTX
	mSimulateNvtxRangeId = nvtxRangeStart("cloth::CuSolver::simulate");
#endif

	CuIterationData* iterationDataBegin = 
		mIterationData.empty() ? 0 : &mIterationData.front();

	mFrameData.resize(0);
	mIterationData.resize(0);

	// update cloth data
	ClothVector::Iterator cIt, cEnd = mCloths.end();
	CuPinnedVector<CuClothData>::Type::Iterator dIt = mClothDataHostCopy.begin();
	for(cIt = mCloths.begin(); cIt != cEnd; ++cIt, ++dIt)
		mClothDataDirty |= (*cIt)->updateClothData(*dIt);

	if(mClothDataDirty)
	{
		/* find optimal number of cloths per SM */

		// at least 192 threads per block (e.g. CuCollision::buildAcceleration)
		uint32_t numSMs = mFactory.mContextManager->getMultiprocessorCount();
		uint32_t maxClothsPerSM = PxMin(mFactory.mMaxThreadsPerBlock / 192, 
			(mCloths.size() + numSMs - 1) / numSMs);

		// tuning parameters: relative performance per numSharedPositions
		float weights[3] = { 0.4f, 0.8f, 1.0f };

		// try all possible number of cloths per SM and estimate performance
		float maxWeightSum = 0.0f;
		uint32_t numClothsPerSM = 0;
		for(uint32_t i = 1; i <= maxClothsPerSM; ++i)
		{
			uint32_t sharedMemoryLimit = (mFactory.mContextManager->
				getSharedMemPerBlock() / i) - mKernelSharedMemorySize;

			float weightSum = 0.0f;
			for(cIt = mCloths.begin(); cIt != cEnd; ++cIt)
			{
				uint32_t sharedMemorySize = (*cIt)->mSharedMemorySize;
				uint32_t positionsSize = (*cIt)->mNumParticles * sizeof(PxVec4);

				if(sharedMemorySize > sharedMemoryLimit)
					break;

				uint32_t numSharedPositions = PxMin(2u, 
					(sharedMemoryLimit - sharedMemorySize) / positionsSize);

				weightSum += weights[numSharedPositions] * positionsSize;
			}
			// tuning parameter: inverse performance for running i cloths per SM
			weightSum *= 2.0f + i;

			if(cIt == cEnd && weightSum > maxWeightSum)
			{
				maxWeightSum = weightSum;
				numClothsPerSM = i;
			}
		}
		PX_ASSERT(numClothsPerSM);

		// update block size
		uint32_t numThreadsPerBlock = mFactory.mMaxThreadsPerBlock / numClothsPerSM & ~31;
		if (mFactory.mNumThreadsPerBlock != numThreadsPerBlock)
		{
			checkSuccess( cuFuncSetBlockShape(mKernelFunction, 
				mFactory.mNumThreadsPerBlock = numThreadsPerBlock, 1, 1 ) );
		}

		// remember num cloths per SM in terms of max shared memory per block
		mSharedMemoryLimit = (mFactory.mContextManager->
			getSharedMemPerBlock() / numClothsPerSM) - mKernelSharedMemorySize;
	}

	uint32_t maxSharedMemorySize = 0;
	for(cIt = mCloths.begin(); cIt != cEnd; ++cIt)
	{
		CuCloth& cloth = **cIt;

		uint32_t sharedMemorySize = cloth.mSharedMemorySize;
		uint32_t positionsSize = cloth.mNumParticles * sizeof(PxVec4);

		uint32_t numSharedPositions = PxMin(2u, 
			(mSharedMemoryLimit - sharedMemorySize) / positionsSize);

		maxSharedMemorySize = PxMax(maxSharedMemorySize, 
			sharedMemorySize + numSharedPositions * positionsSize);

		IterationStateFactory factory(cloth, mFrameDt);
		IterationState<Simd4f> state = factory.create<Simd4f>(cloth);

		mFrameData.pushBack(CuFrameData(cloth, numSharedPositions,
			state, mIterationDataBegin + mIterationData.size()));

		while(state.mRemainingIterations)
		{
			mIterationData.pushBack(CuIterationData(state));
			state.update();
		}
	}
	mSharedMemorySize = maxSharedMemorySize;

	// add dummy element because we read past the end
	mIterationData.pushBack(CuIterationData());

	if(&mIterationData.front() != iterationDataBegin)
	{
		// mIterationData grew, update pointers
		iterationDataBegin = getDevicePointer(mIterationData);

		ptrdiff_t diff = (char*)iterationDataBegin - (char*)mIterationDataBegin;
		CuPinnedVector<CuFrameData>::Type::Iterator fIt = mFrameData.begin(), fEnd;
		for(fEnd = mFrameData.end(); fIt != fEnd; ++fIt)
			reinterpret_cast<const char*&>(fIt->mIterationData) += diff;

		mIterationDataBegin = iterationDataBegin;
	}
}

void cloth::CuSolver::executeKernel()
{
	CuContextLock contextLock(mFactory);

#ifdef PX_PROFILE
	// Note: The profile buffer is valid only within the cuda launch context
	void* profileBuffer = getDispatcher().getCurrentProfileBuffer();
	if(mProfileBuffer != profileBuffer && mProfileBaseId+1)
	{
		mProfileBuffer = profileBuffer;
		updateKernelData();
	}
#endif

#if ENABLE_CUDA_PRINTF
	if(cudaError result = cudaPrintfInit(mKernelModule))
	{
		shdfnd::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, 
			__FILE__, __LINE__, "cudaPrintfInit() returned %u.", result);
	}
#endif

	if(mClothDataDirty)
	{
		PX_ASSERT(mClothDataHostCopy.size() == mClothData.size());
		size_t numBytes = mClothData.size() * sizeof(CuClothData);
		checkSuccess( cuMemcpyHtoDAsync(mClothData.begin().dev(), 
			mClothDataHostCopy.begin(), numBytes, mStream) );
		mClothDataDirty = false;
	}

#if 0
	static int frame = 0;
	if(++frame == 100)
		record(*this);
#endif

	// launch kernel
	CUresult result = cuLaunchKernel(mKernelFunction, int(mCloths.size()), 1, 1, 
		mFactory.mNumThreadsPerBlock, 1, 1, mSharedMemorySize, mStream, 0, 0);

#if ENABLE_CUDA_PRINTF
	cudaPrintfDisplay(mKernelModule);
	cudaPrintfEnd();
#endif

#ifdef PX_DEBUG
	// in debug builds check kernel result
	checkSuccess( result );
	checkSuccess( cuStreamSynchronize(mStream) );
#endif

	// mark the solver as being in an error state
	// all cloth instances will be migrated to software
	if (result != CUDA_SUCCESS)
		mCudaError = true;
}

void cloth::CuSolver::endFrame()
{
	CuPinnedVector<CuFrameData>::Type::ConstIterator fIt = mFrameData.begin();
	ClothVector::Iterator cIt, cEnd = mCloths.end();
	for(cIt = mCloths.begin(); cIt != cEnd; ++cIt, ++fIt)
	{
		CuCloth& cloth = **cIt;

		cloth.mHostParticlesDirty = false;
		cloth.mDeviceParticlesDirty = false;

		cloth.mMotionConstraints.pop();
		cloth.mMotionConstraints.mHostCopy.resize(0);

		cloth.mSeparationConstraints.pop();
		cloth.mSeparationConstraints.mHostCopy.resize(0);

		if (!cloth.mTargetCollisionSpheres.empty())
		{
			shdfnd::swap(cloth.mStartCollisionSpheres,
				cloth.mTargetCollisionSpheres);
			cloth.mTargetCollisionSpheres.resize(0);
		}

		if (!cloth.mTargetCollisionPlanes.empty())
		{
			shdfnd::swap(cloth.mStartCollisionPlanes,
				cloth.mTargetCollisionPlanes);
			cloth.mTargetCollisionPlanes.resize(0);
		}

		if (!cloth.mTargetCollisionTriangles.empty())
		{
			shdfnd::swap(cloth.mStartCollisionTriangles,
				cloth.mTargetCollisionTriangles);
			cloth.mTargetCollisionTriangles.resize(0);
		}

		for(uint32_t i=0; i<3; ++i)
		{
			float upper = fIt->mParticleBounds[i*2+0];
			float negativeLower = fIt->mParticleBounds[i*2+1];
			cloth.mParticleBoundsCenter[i] = (upper - negativeLower) * 0.5f;
			cloth.mParticleBoundsHalfExtent[i] = (upper + negativeLower) * 0.5f;
		}

		cloth.mSleepPassCounter = fIt->mSleepPassCounter;
		cloth.mSleepTestCounter = fIt->mSleepTestCounter;
	}

	interCollision();
#ifdef PX_NVTX
	nvtxRangeEnd(mSimulateNvtxRangeId);
#endif
	if(mProfiler)
	{
		mProfiler->stopEvent(mSimulateEventId, uintptr_t(this), 
			uint32_t(uintptr_t(this) & 0xffffffff));
	}
}

void cloth::CuSolver::interCollision()
{
	if (!mInterCollisionIterations || mInterCollisionDistance == 0.0f)
		return;

	typedef SwInterCollision<Simd4f> SwInterCollision;

	float elasticity = 1.0f;

	// rebuild cloth instance array
	mInterCollisionInstances.resize(0);
	for(uint32_t i=0, n = mCloths.size(); i < n; ++i)
	{
		CuCloth& cloth = *mCloths[i];
		uint32_t numIterations = mFrameData[i].mNumIterations;

		mInterCollisionInstances.pushBack(
			SwInterCollisionData(cloth.getHostParticles(true), cloth.getHostParticles(true) + cloth.mNumParticles,
				cloth.mSelfCollisionIndices.empty()?cloth.mNumParticles:uint32_t(cloth.mSelfCollisionIndices.size()),
				cloth.mSelfCollisionIndices.empty()?NULL:&cloth.mSelfCollisionIndicesHost[0],
				cloth.mTargetMotion,
				cloth.mParticleBoundsCenter,
				cloth.mParticleBoundsHalfExtent,
				elasticity / numIterations,
				cloth.mUserData));
	}

	uint32_t requiredTempMemorySize = uint32_t(SwInterCollision::estimateTemporaryMemory(
		&mInterCollisionInstances[0], mInterCollisionInstances.size()));

	// realloc temp memory if necessary
	if (mInterCollisionScratchMemSize < requiredTempMemorySize)
	{
		if (mInterCollisionScratchMem)
			PX_FREE(mInterCollisionScratchMem);

		mInterCollisionScratchMem = PX_ALLOC(requiredTempMemorySize, 
			"cloth::SwSolver::mInterCollisionScratchMem");
		mInterCollisionScratchMemSize = requiredTempMemorySize;
	}

	SwKernelAllocator allocator(mInterCollisionScratchMem, mInterCollisionScratchMemSize);

	// run inter-collision
	SwInterCollision(mInterCollisionInstances.begin(), mInterCollisionInstances.size(), 
		mInterCollisionDistance, mInterCollisionStiffness, mInterCollisionIterations, 
		mInterCollisionFilter, allocator, mProfiler)();
}

cloth::CuSolver::ClothSolverTask::ClothSolverTask(FunctionPtr functionPtr, const char* name)
: mSolver(0), mFunctionPtr(functionPtr), mName(name)
{}

void cloth::CuSolver::ClothSolverTask::run()
{
	(mSolver->*mFunctionPtr)();
}

const char* cloth::CuSolver::ClothSolverTask::getName() const
{
	return mName;
}
