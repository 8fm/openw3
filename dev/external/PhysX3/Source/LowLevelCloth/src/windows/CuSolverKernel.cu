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

#include "CuSolverKernel.h"
#include "CuClothData.h"
#include "CuPhaseConfig.h" 

#include <new> // placement new

using namespace physx;

#if __CUDA_ARCH__ < 200
#define printf(x)
#define assert(x) /* not supported */
#else
extern "C"
{
	extern _CRTIMP __host__ __device__ int __cdecl printf(const char*, ...);
}
#endif 

// global symbol holding all cloth instances
static __constant__ cloth::CuKernelData gKernelData;

// shared memory copy (instead of relying on constant cache)
__shared__ cloth::CuClothData gClothData; 
__shared__ cloth::CuFrameData gFrameData; 
__shared__ cloth::CuIterationData gIterData;

static const uint32_t gCuClothDataSize = sizeof(cloth::CuClothData) / sizeof(float);
static const uint32_t gCuFrameDataSize = sizeof(cloth::CuFrameData) / sizeof(float);
static const uint32_t gCuIterationDataSize = sizeof(cloth::CuIterationData) / sizeof(float);
static const uint32_t gCuPhaseConfigSize = sizeof(cloth::CuPhaseConfig) / sizeof(float);

/*
Memory block for all temporary data in shared memory (in 'allocation' order).
The numbers indicate the allocation slot if used a stack allocator.
0) simulate*()::configs (numPhases*sizeof(CuPhaseConfig))
1) simulate*()::particles ({0,1,2}*4*numParticles floats)
2) CuCollision::mCapsuleIndices, mCapsuleMasks, mConvexMasks (numCapsules*4+numConvexes ints)
3) CuCollision::mPrevData (4*numSpheres+10*numCones floats)
4) CuCollision::collideConvexes() (4*numPlanes floats)
4) CuCollision::collideTriangles() (19*numTriangles floats)
4) CuCollision::mCurData::Spheres (4*numSpheres floats)
5) computeParticleBounds()::dst (192 floats written, 208 float read)
5) computeSphereBounds()::dst (192 floats written, 208 floats read)
5) CuCollision::mCurData::Cones (10*numCones floats)
6) CuCollision::mShapeGrid (2*6*sGridSize=96 floats)
4) CuSelfCollision::buildAcceleration()::buffer (34*16=544 ints)
*/ 
extern __shared__ float gSharedMemory[];
extern __shared__ uint32_t gSharedUnsigned[];

#if __CUDA_ARCH__ < 200

__device__ float* sharedBase(const float&) { return gSharedMemory; }
__device__ uint32_t* sharedBase(const uint32_t&) { return gSharedUnsigned; }

// pointer forced to point to shared memory (only works for sizeof(T) <= 4)
template <typename T>
class SharedPointer
{
	__device__ explicit SharedPointer(ptrdiff_t offset) : mOffset(offset) {}

public:
	typedef SharedPointer<T> Type;

	__device__ SharedPointer() {}
	__device__ SharedPointer(const SharedPointer& other) : mOffset(other.mOffset) {}
	__device__ SharedPointer(T* ptr) : mOffset(ptr-sharedBase(T())) {}

	template <typename S> // assuming pointee types of convertible pointers have same size
	__device__ SharedPointer(const SharedPointer<S>& other) : mOffset(other.mOffset) {}

	__device__ bool operator!=(const SharedPointer& other) const { return mOffset != other.mOffset; }
	__device__ bool operator<(const SharedPointer& other) const { return mOffset < other.mOffset; }

	__device__ SharedPointer operator+(ptrdiff_t i) const { return SharedPointer(mOffset + i); }
	__device__ SharedPointer& operator+=(ptrdiff_t i) { mOffset += i; return *this; }
	__device__ SharedPointer operator-(ptrdiff_t i) const { return SharedPointer(mOffset - i); }

	__device__ SharedPointer& operator++() { ++mOffset; return *this; }
	__device__ SharedPointer& operator--() { --mOffset; return *this; }

	__device__ SharedPointer operator++(int) { return SharedPointer(mOffset++); }

	__device__ T* operator->() const { return sharedBase(T()) + mOffset; }
	__device__ T& operator*() const { return sharedBase(T())[mOffset]; }
	__device__ T& operator[](int32_t i) const { return sharedBase(T())[mOffset+i]; }

	ptrdiff_t mOffset;

	PX_COMPILE_TIME_ASSERT(sizeof(T) == 4);
};

#else
template <typename T>
struct SharedPointer
{
	typedef T* Type;
};
#endif

// pointer with stride of 4
template <typename T>
struct GlobalPointer
{
	typedef GlobalPointer<T> Type;

	__device__ GlobalPointer() {}
	__device__ GlobalPointer(const GlobalPointer& other) : mPtr(other.mPtr) {}
	__device__ GlobalPointer(T* ptr) : mPtr(ptr) {}

	template <typename S> 
	__device__ GlobalPointer(const GlobalPointer<S>& other) 
		: mPtr(other.mPtr) 
	{}

	__device__ bool operator<(const GlobalPointer& other) const { return mPtr < other.mPtr; }

	__device__ GlobalPointer operator+(int32_t i) const { return GlobalPointer(mPtr + 4*i); }
	__device__ GlobalPointer& operator+=(int32_t i) { mPtr += 4*i; return *this; }

	__device__ T& operator*() const { return *mPtr; }
	__device__ T& operator[](uint32_t i) const { return mPtr[4*i]; }

	T* mPtr;
};

template <template <typename> class PointerT>
struct ParticleData
{
	typedef typename PointerT<float>::Type PointerType;
	typedef typename PointerT<const float>::Type ConstPointerType;

	__device__ ParticleData() {}	// Empty constructor required for -G0 flag

	__device__ float& operator()(uint32_t index, uint32_t element)
	{
		return mPointers[element][index];
	}
	__device__ const float& operator()(uint32_t index, uint32_t element) const
	{
		return mPointers[element][index];
	}

	__device__ const PointerType& operator[](uint32_t element) 
	{
		return mPointers[element]; 
	}
	__device__ ConstPointerType operator[](uint32_t element) const 
	{
		return mPointers[element]; 
	}

	PointerType mPointers[4];
};

/***************** Profiling **********************/
struct ProfileDisabledZone 
{ 
	__device__ ProfileDisabledZone(cloth::CuProfileZoneIds::Enum) {} 
};


#if defined(__CUDA_ARCH__) && defined(PX_PROFILE) // profile zones enabled for profile build

#include "GPUProfile.h"

struct ProfileZone
{
	__device__ ProfileZone(cloth::CuProfileZoneIds::Enum id)
		: mEvent(0)
	{
		if (!gKernelData.mProfileBuffer || threadIdx.x & 0x1f)
			return;

		// +1: first entry reserved for counter
		uint32_t index = atomicAdd(reinterpret_cast<uint32_t*>(
			gKernelData.mProfileBuffer), 1) + 1; 

		if(index >= NUM_WARPS_PER_PROFILE_BUFFER)
			return;

		mEvent = reinterpret_cast<warpProfileEvent*>(
			gKernelData.mProfileBuffer) + index;

		fillKernelEvent(*mEvent, 
			gKernelData.mProfileBaseId + id, threadIdx.x );
	}

	__device__ ~ProfileZone() 
	{
		if(mEvent) 
			mEvent->endTime = clock();
	}

	warpProfileEvent* mEvent;
};

#else
typedef ProfileDisabledZone ProfileZone;
#endif

#if 1 // set to 1 to enable detailed profile zones
typedef ProfileZone ProfileDetailZone;
#else
typedef ProfileDisabledZone ProfileDetailZone;
#endif

namespace 
{
	// cut down version of thrust::uninitialized
	// avoids warning about non-empty c'tor
	template<typename T>
	struct uninitialized
	{
		__device__ inline T& get()
		{
			return *reinterpret_cast<T*>(data);
		}

		// maximum alignment required by device code is 16
		__align__(16) unsigned char data[sizeof(T)];
	};
} 

#if __CUDA_ARCH__ < 320
namespace 
{
	__device__ float __ldg(const float* __restrict ptr)
	{
		return *ptr;
	}
}
#endif
   
#define CU_SOLVER_KERNEL_CU               
#include "CuCollision.h"      
#include "CuSelfCollision.h"        

namespace     
{
	__device__ void loadIterData(const cloth::CuIterationData* __restrict iterData)
	{
		if(threadIdx.x < gCuIterationDataSize)
		{
			gIterData.mIntegrationTrafo[threadIdx.x] = 
				iterData->mIntegrationTrafo[threadIdx.x];
		}
	}

	template <typename CurrentT, typename PreviousT>
	__device__ void integrateParticles(CurrentT& current, PreviousT& previous)
	{
		if(gIterData.mIsTurning)
			integrateParticles<true >(current, previous);
		else
			integrateParticles<false>(current, previous);
	}

	// integrate particle positions and store transposed
	template <bool IsTurning, typename CurrentT, typename PreviousT>
	__device__ void integrateParticles(CurrentT& current, PreviousT& previous)
	{
		ProfileDetailZone zone(cloth::CuProfileZoneIds::INTEGRATE);

		const float* __restrict trafo = gIterData.mIntegrationTrafo;

		for(uint32_t i = threadIdx.x; i < gClothData.mNumParticles; i += blockDim.x)
		{
			float nextX = current(i, 0), curX = nextX;
			float nextY = current(i, 1), curY = nextY;
			float nextZ = current(i, 2), curZ = nextZ;			
			float nextW = current(i, 3);

			if(nextW == 0.0f)
				nextW = previous(i, 3);

			if(nextW > 0.0f)
			{
				float prevX = previous(i, 0);
				float prevY = previous(i, 1);
				float prevZ = previous(i, 2);

				if(IsTurning)
				{
					nextX = nextX + trafo[3] +
						curX * trafo[15] + prevX * trafo[ 6] + 
						curY * trafo[16] + prevY * trafo[ 7] + 
						curZ * trafo[17] + prevZ * trafo[ 8];
																								
					nextY = nextY + trafo[4] +
						curX * trafo[18] + prevX * trafo[ 9] +
						curY * trafo[19] + prevY * trafo[10] +
						curZ * trafo[20] + prevZ * trafo[11];

					nextZ = nextZ + trafo[5] +
						curX * trafo[21] + prevX * trafo[12] +
						curY * trafo[22] + prevY * trafo[13] +
						curZ * trafo[23] + prevZ * trafo[14];
				} else {
					nextX += (curX - prevX) * trafo[ 6] + trafo[3];
					nextY += (curY - prevY) * trafo[ 9] + trafo[4];
					nextZ += (curZ - prevZ) * trafo[12] + trafo[5];
				}

				curX  += trafo[0];
				curY  += trafo[1];
				curZ  += trafo[2];
			}

			current(i, 0) = nextX;
			current(i, 1) = nextY;
			current(i, 2) = nextZ;
			current(i, 3) = nextW;

			previous(i, 0) = curX;
			previous(i, 1) = curY;
			previous(i, 2) = curZ;
		}
	}

	template <typename CurrentT>
	__device__ void accelerateParticles(CurrentT& current)
	{
		// might be better to move this into integrate particles
		const float* accelerations = gFrameData.mParticleAccelerations;

		if(!accelerations)
			return;

		ProfileDetailZone zone(cloth::CuProfileZoneIds::ACCELERATE);

		__syncthreads(); // looping with 4 instead of 1 thread per particle

		float sqrIterDt = ~threadIdx.x & 0x3 ? gFrameData.mIterDt * gFrameData.mIterDt : 0.0f;
		typename CurrentT::PointerType sharedCurPos = current[threadIdx.x % 4];

		for(uint32_t i=threadIdx.x; i < gClothData.mNumParticles*4; i += blockDim.x)
		{
			if(current(i/4, 3) > 0.0f)
				sharedCurPos[i/4] += accelerations[i] * sqrIterDt;
		}

		__syncthreads();
	}

	template <typename CurrentT>
	__device__ void constrainTether(CurrentT& current)
	{
		if(0.0f == gFrameData.mTetherConstraintStiffness || !gClothData.mNumTethers)
			return;

		ProfileDetailZone zone(cloth::CuProfileZoneIds::TETHER);

		uint32_t numParticles = gClothData.mNumParticles;
		uint32_t numTethers = gClothData.mNumTethers;
		assert(0 == numTethers % numParticles);

		float stiffness = numParticles * 
			gFrameData.mTetherConstraintStiffness / numTethers;
		float scale = gClothData.mTetherConstraintScale;

		const uint32_t* __restrict tIt = reinterpret_cast<
			const uint32_t*>(gClothData.mTethers);

		for(uint32_t i=threadIdx.x; i<gClothData.mNumParticles; i+=blockDim.x)
		{
			float posX = current(i, 0);
			float posY = current(i, 1);
			float posZ = current(i, 2);

			float offsetX = 0.0f;
			float offsetY = 0.0f;
			float offsetZ = 0.0f;

			for(uint32_t j=i; j<numTethers; j+=gClothData.mNumParticles)
			{
				uint32_t tether = tIt[j];

				uint32_t anchor = tether & 0xffff;
				float deltaX = current(anchor, 0) - posX;
				float deltaY = current(anchor, 1) - posY;
				float deltaZ = current(anchor, 2) - posZ;

				float sqrLength = FLT_EPSILON + deltaX*deltaX 
					+ deltaY*deltaY + deltaZ*deltaZ;

				float radius = (tether >> 16) * scale;
				float slack = 1.0f - radius * rsqrtf(sqrLength);

				if (slack > 0.0f)
				{
					offsetX += deltaX * slack;
					offsetY += deltaY * slack;
					offsetZ += deltaZ * slack;
				}
			}

			current(i, 0) = posX + offsetX * stiffness;
			current(i, 1) = posY + offsetY * stiffness;
			current(i, 2) = posZ + offsetZ * stiffness;
		}

	}

	template <typename CurrentT>
	__device__ void solveFabric(CurrentT& current)
	{
		ProfileDetailZone zone(cloth::CuProfileZoneIds::FABRIC);

		const cloth::CuPhaseConfig* cIt = (cloth::CuPhaseConfig*)gSharedMemory;
		const cloth::CuPhaseConfig* cEnd = cIt + gClothData.mNumPhases;

		for(; cIt != cEnd; ++cIt)
		{
			__syncthreads();

			ProfileDetailZone zone(cloth::CuProfileZoneIds::CONSTRAINT_SET);

			uint32_t numConstraints = cIt->mNumConstaints;
			if(threadIdx.x >= numConstraints)
				continue;

			const float* restvalues = cIt->mRestvalues;
			const float* rIt = restvalues + threadIdx.x;
			const float* rEnd = restvalues + numConstraints;
			const uint16_t* iIt = cIt->mIndices + threadIdx.x * 2;

			float stiffness = cIt->mStiffness;
			float stiffnessMultiplier = cIt->mStiffnessMultiplier;
			float compressionLimit = cIt->mCompressionLimit;
			float stretchLimit = cIt->mStretchLimit;

			uint32_t vpi = iIt[0];
			uint32_t vpj = iIt[1];
			float rij = __ldg(rIt);

			do
			{
				rIt += blockDim.x;
				iIt += blockDim.x * 2;

				uint32_t vpiPrefetch, vpjPrefetch;
				float rijPrefetch;
				if(rIt < rEnd)
				{
					vpiPrefetch = iIt[0];
					vpjPrefetch = iIt[1];
					rijPrefetch = __ldg(rIt);
				}

				float vxi = current(vpi, 0);
				float vyi = current(vpi, 1);
				float vzi = current(vpi, 2);
				float vwi = current(vpi, 3);

				float vxj = current(vpj, 0);
				float vyj = current(vpj, 1);
				float vzj = current(vpj, 2);
				float vwj = current(vpj, 3);

				float hxij = vxj - vxi;
				float hyij = vyj - vyi;
				float hzij = vzj - vzi;

				float e2ij = FLT_EPSILON + hxij*hxij + hyij*hyij + hzij*hzij;
				float negErij = rij > FLT_EPSILON ? -1.0f + rij * rsqrtf(e2ij) : 0.0f;

				negErij = negErij + stiffnessMultiplier * 
					max(compressionLimit, min(-negErij, stretchLimit));

				float negExij = __fdividef(negErij * stiffness, FLT_EPSILON + vwi + vwj);

				float vmi = -vwi * negExij;
				current(vpi, 0) = vxi + vmi * hxij;
				current(vpi, 1) = vyi + vmi * hyij;
				current(vpi, 2) = vzi + vmi * hzij;
				
				float vmj = +vwj * negExij;
				current(vpj, 0) = vxj + vmj * hxij;
				current(vpj, 1) = vyj + vmj * hyij;
				current(vpj, 2) = vzj + vmj * hzij;

				vpi = vpiPrefetch;
				vpj = vpjPrefetch;
				rij = rijPrefetch;

			} while(rIt < rEnd);
		}

		__syncthreads();
	}

#if __CUDA_ARCH__ >= 200 && !defined(_WIN64)
	// ptx version for 32bit shared memory (about 9% faster)
	__device__ void solveFabric(ParticleData<SharedPointer>& current)
	{
		ProfileDetailZone zone(cloth::CuProfileZoneIds::FABRIC);

		const ParticleData<SharedPointer>::PointerType* pointers = current.mPointers;

		asm volatile (
			"{\n\t"
			"	.reg .u32 cIt, cEnd;\n\t"
			"	.reg .pred p;\n\t"
			"	.reg .u32 posPtr, posX, posY, posZ, posW;\n\t"
			"	.reg .u32 tid4, ntid4, numConstraints4;\n\t"
			"	.reg .u32 restvalues, indices;\n\t"
			"	.reg .f32 stiffness, multiplier, compressionLimit, stretchLimit;\n\t"
			"	.reg .u32 rEnd, iIt, rIt;\n\t"
			"	.reg .u32 vpi, vpj;\n\t"
			"	.reg .u32 axj, ayj, azj, awj;\n\t"
			"	.reg .u32 axi, ayi, azi, awi;\n\t"
			"	.reg .f32 vxj, vyj, vzj, vwj;\n\t"
			"	.reg .f32 vxi, vyi, vzi, vwi;\n\t"
			"	.reg .f32 hxij, hyij, hzij;\n\t"
			"	.reg .f32 e2ij;\n\t"
			"	.reg .f32 rsqrtE2ij;\n\t"
			"	.reg .f32 rij;\n\t"
			"	.reg .f32 negErij;\n\t"
			"	.reg .f32 satErij;\n\t"
			"	.reg .f32 negExij;\n\t"
			"	.reg .f32 vwij;\n\t"
			"	.reg .f32 negvwi;\n\t"
			"	.reg .f32 vmi, vmj;\n\t"
			"	ld.shared.u32 cEnd, [gClothData+12];\n\t"
			"	mov.u32 cIt, gSharedMemory;\n\t"
			"   mad.lo.u32 cEnd, cEnd, 28, cIt;\n\t"
			"	setp.eq.u32 p, cIt, cEnd;\n\t"
			"@p bra configEnd;\n\t"
			"	cvta.to.shared.u32 posPtr, %0;\n\t"
			"	ld.shared.u32 posX, [posPtr   ];\n\t"
			"	ld.shared.u32 posY, [posPtr+ 4];\n\t"
			"	ld.shared.u32 posZ, [posPtr+ 8];\n\t"
			"	ld.shared.u32 posW, [posPtr+12];\n\t"
			"	cvta.to.shared.u32 posX, posX;\n\t"
			"	cvta.to.shared.u32 posY, posY;\n\t"
			"	cvta.to.shared.u32 posZ, posZ;\n\t"
			"	cvta.to.shared.u32 posW, posW;\n\t"
			"	mov.u32 tid4, %tid.x;\n\t"
			"	mov.u32 ntid4, %ntid.x;\n\t"
			"	shl.b32 tid4, tid4, 2;\n\t"
			"	shl.b32 ntid4, ntid4, 2;\n\t"
			"configBegin:\n\t"
			"	bar.sync 0;\n\t"
			"	ld.shared.u32 numConstraints4, [cIt+16];\n\t"
			"	shl.b32 numConstraints4, numConstraints4, 2;\n\t"
			"	setp.ge.u32 p, tid4, numConstraints4;\n\t"
			"@p bra constraintEnd;\n\t"
			"	ld.shared.u32 restvalues, [cIt+20];\n\t"
			"	ld.shared.u32 indices, [cIt+24];\n\t"
			"	ld.shared.f32 stiffness, [cIt+0];\n\t"
			"	ld.shared.f32 multiplier, [cIt+4];\n\t"
			"	ld.shared.f32 compressionLimit, [cIt+8];\n\t"
			"	ld.shared.f32 stretchLimit, [cIt+12];\n\t"
			"	add.u32 iIt, indices, tid4;\n\t"
			"	add.u32 rIt, restvalues, tid4;\n\t"
			"	add.u32 rEnd, restvalues, numConstraints4;\n\t"
			"constraintBegin:\n\t"
			"	ld.global.u32 vpi, [iIt];\n\t"
			"	and.b32 vpj, vpi, 0xffff0000;\n\t"
			"	and.b32 vpi, vpi, 0x0000ffff;\n\t"
			"	shr.b32 vpj, vpj, 14;\n\t"
			"	shl.b32 vpi, vpi, 2;\n\t"
			"	add.u32 axj, posX, vpj;\n\t"
			"	add.u32 ayj, posY, vpj;\n\t"
			"	add.u32 azj, posZ, vpj;\n\t"
			"	add.u32 awj, posW, vpj;\n\t"
			"	add.u32 axi, posX, vpi;\n\t"
			"	add.u32 ayi, posY, vpi;\n\t"
			"	add.u32 azi, posZ, vpi;\n\t"
			"	add.u32 awi, posW, vpi;\n\t"
			"	ld.shared.f32 vxj, [axj];\n\t"
			"	ld.shared.f32 vyj, [ayj];\n\t"
			"	ld.shared.f32 vzj, [azj];\n\t"
			"	ld.shared.f32 vwj, [awj];\n\t"
			"	ld.shared.f32 vxi, [axi];\n\t"
			"	ld.shared.f32 vyi, [ayi];\n\t"
			"	ld.shared.f32 vzi, [azi];\n\t"
			"	ld.shared.f32 vwi, [awi];\n\t"
			"	sub.ftz.f32 hxij, vxj, vxi;\n\t"
			"	sub.ftz.f32 hyij, vyj, vyi;\n\t"
			"	sub.ftz.f32 hzij, vzj, vzi;\n\t"
			"	fma.rn.ftz.f32 e2ij, hxij, hxij, 0f34000000;\n\t"
			"	fma.rn.ftz.f32 e2ij, hyij, hyij, e2ij;\n\t"
			"	fma.rn.ftz.f32 e2ij, hzij, hzij, e2ij;\n\t"
			"	rsqrt.approx.ftz.f32 rsqrtE2ij, e2ij;\n\t"
			"	ld.global.f32 rij, [rIt];\n\t"
			"	fma.rn.ftz.f32 negErij, rij, rsqrtE2ij, 0fBF800000;\n\t"
			"	setp.le.ftz.f32 p, rij, 0f34000000;\n\t"
			"@p mov.f32 negErij, 0f00000000;\n\t"
			"	neg.f32 satErij, negErij;\n\t"
			"	min.ftz.f32 satErij, satErij, stretchLimit;\n\t"
			"	max.ftz.f32 satErij, satErij, compressionLimit;\n\t"
			"	fma.rn.ftz.f32 negErij, satErij, multiplier, negErij;\n\t"
			"	mul.ftz.f32 negExij, negErij, stiffness;\n\t"
			"	add.ftz.f32 vwij, vwj, 0f34000000;\n\t"
			"	add.ftz.f32 vwij, vwij, vwi;\n\t"
			"	div.approx.ftz.f32 negExij, negExij, vwij;\n\t"
			"	neg.f32 negvwi, vwi;\n\t"
			"	mul.ftz.f32 vmi, negExij, negvwi;\n\t"
			"	mul.ftz.f32 vmj, negExij, vwj;\n\t"
			"	fma.rn.ftz.f32 vxi, vmi, hxij, vxi;\n\t"
			"	fma.rn.ftz.f32 vyi, vmi, hyij, vyi;\n\t"
			"	fma.rn.ftz.f32 vzi, vmi, hzij, vzi;\n\t"
			"	fma.rn.ftz.f32 vxj, vmj, hxij, vxj;\n\t"
			"	fma.rn.ftz.f32 vyj, vmj, hyij, vyj;\n\t"
			"	fma.rn.ftz.f32 vzj, vmj, hzij, vzj;\n\t"
			"	st.shared.f32 [axj], vxj;\n\t"
			"	st.shared.f32 [ayj], vyj;\n\t"
			"	st.shared.f32 [azj], vzj;\n\t"
			"	st.shared.f32 [axi], vxi;\n\t"
			"	st.shared.f32 [ayi], vyi;\n\t"
			"	st.shared.f32 [azi], vzi;\n\t"
			"	add.u32 rIt, rIt, ntid4;\n\t"
			"	add.u32 iIt, iIt, ntid4;\n\t"
			"	setp.lt.u32 p, rIt, rEnd;\n\t"
			"@p bra constraintBegin;\n\t"
			"constraintEnd:\n\t"
			"	add.u32 cIt, cIt, 28;\n\t"
			"	setp.ne.u32 p, cIt, cEnd;\n\t"
			"@p bra configBegin;\n\t"
			"configEnd:\n\t"
			"	bar.sync 0;\n\t"
			"}"
			: : "r"(pointers) : "memory"
		);
	}
#endif

	template <typename CurrentT>
	__device__ void constrainMotion(CurrentT& current, float alpha)
	{
		if(!gFrameData.mStartMotionConstraints)
			return;

		ProfileDetailZone zone(cloth::CuProfileZoneIds::MOTION);

		// negative because of fused multiply-add optimization
		float negativeScale = -gClothData.mMotionConstraintScale;
		float negativeBias = -gClothData.mMotionConstraintBias;

		for(uint32_t i=threadIdx.x; i<gClothData.mNumParticles; i+=blockDim.x)
		{
			const float* startIt = gFrameData.mStartMotionConstraints + 4*i; // ! bank conflicts
			const float* targetIt = gFrameData.mTargetMotionConstraints + 4*i;

			float sphereX = startIt[0] + (targetIt[0] - startIt[0]) * alpha;
			float sphereY = startIt[1] + (targetIt[1] - startIt[1]) * alpha;
			float sphereZ = startIt[2] + (targetIt[2] - startIt[2]) * alpha;
			float sphereW = startIt[3] + (targetIt[3] - startIt[3]) * alpha;

			float dx = sphereX - current(i, 0);
			float dy = sphereY - current(i, 1);
			float dz = sphereZ - current(i, 2);

			float sqrLength = FLT_EPSILON + dx*dx + dy*dy + dz*dz;
			float negativeRadius = min(0.0f, sphereW * negativeScale + negativeBias);

			float slack = max(negativeRadius * rsqrtf(sqrLength) + 1.0f,
				0.0f) * gFrameData.mMotionConstraintStiffness;

			current(i, 0) += slack * dx;
			current(i, 1) += slack * dy;
			current(i, 2) += slack * dz;

			// set invMass to zero if radius is zero
			if(negativeRadius >= 0.0f)
				current(i, 3) = 0.0f;

		}
	}

	template <typename T>
	__device__ void constrainSeparation(T& current, float alpha)
	{
		if(!gFrameData.mStartSeparationConstraints)
			return;
		
		ProfileDetailZone zone(cloth::CuProfileZoneIds::SEPARATION);

		for(uint32_t i=threadIdx.x; i<gClothData.mNumParticles; i+=blockDim.x)
		{
			const float* startIt = gFrameData.mStartSeparationConstraints + 4*i;
			const float* targetIt = gFrameData.mTargetSeparationConstraints + 4*i;

			float sphereX = startIt[0] + (targetIt[0] - startIt[0]) * alpha;
			float sphereY = startIt[1] + (targetIt[1] - startIt[1]) * alpha;
			float sphereZ = startIt[2] + (targetIt[2] - startIt[2]) * alpha;
			float sphereW = startIt[3] + (targetIt[3] - startIt[3]) * alpha;

			float dx = sphereX - current(i, 0);
			float dy = sphereY - current(i, 1);
			float dz = sphereZ - current(i, 2);

			float sqrLength = FLT_EPSILON + dx*dx + dy*dy + dz*dz;

			float slack = min(0.0f, 1.0f - sphereW * rsqrtf(sqrLength));

			current(i, 0) += slack * dx;
			current(i, 1) += slack * dy;
			current(i, 2) += slack * dz;
		}
	}
 
	template <typename CurrentT, typename PreviousT>
	__device__ void updateSleepState(const CurrentT& current, const PreviousT& previous)
	{
		ProfileDetailZone zone(cloth::CuProfileZoneIds::SLEEP);

		if(!threadIdx.x)
			gFrameData.mSleepTestCounter += max(1, uint32_t(gFrameData.mIterDt*1000));

		__syncthreads();  
 
		if (gFrameData.mSleepTestCounter < gClothData.mSleepTestInterval)
			return;

		float maxDelta = 0.0f;
		for(uint32_t i = threadIdx.x; i < gClothData.mNumParticles; i += blockDim.x)
		{
			maxDelta = max(fabsf(current(i,0) - previous(i,0)), maxDelta);
			maxDelta = max(fabsf(current(i,1) - previous(i,1)), maxDelta);
			maxDelta = max(fabsf(current(i,2) - previous(i,2)), maxDelta);
		}

		if(!threadIdx.x)
		{
			++gFrameData.mSleepPassCounter;
			gFrameData.mSleepTestCounter -= gClothData.mSleepTestInterval;
		}

		__syncthreads();

		if(maxDelta > gClothData.mSleepThreshold*gFrameData.mIterDt)
			gFrameData.mSleepPassCounter = 0;
	}

	template <typename CurrentT, typename PreviousT>
	__device__ void simulateCloth(CurrentT& current, PreviousT& previous)
	{
		// apply exponent to phase configs
		assert(blockDim.x >= gClothData.mNumPhases);
		if(threadIdx.x < gClothData.mNumPhases)
		{
			float exponent = gFrameData.mStiffnessExponent;
			float* ptr = gSharedMemory + threadIdx.x * gCuPhaseConfigSize;
			ptr[0] = 1.0f - exp2f(ptr[0] * exponent);
			ptr[1] = 1.0f - exp2f(ptr[1] * exponent);
		}

		uint32_t numIterations = gFrameData.mNumIterations;
		float invNumIterations = __fdividef(1.0f, numIterations);

		const cloth::CuIterationData* iterData = gFrameData.mIterationData;
		const cloth::CuIterationData* iterEnd = iterData + numIterations;

		loadIterData(iterData);

		__syncthreads();

		for(float alpha = invNumIterations; iterData != iterEnd; alpha += invNumIterations)
		{
			integrateParticles(current, previous);
			accelerateParticles(current);
			constrainMotion(current, alpha);
			constrainTether(current);
			solveFabric(current);
			loadIterData(++iterData);
			constrainSeparation(current, alpha);
			gCollideParticles.get()(current, previous, alpha);
#if __CUDA_ARCH__ < 200 // see DE8360
			if(gFrameData.mRestPositions)
#endif
			gSelfCollideParticles.get()(current);
			updateSleepState(current, previous);
		}

		__syncthreads();
	}

	template <typename CurrentData, typename PreviousData>
	struct ParticleDataPair
	{
		CurrentData mCurrent;
		PreviousData mPrevious;
	};

	typedef ParticleData<SharedPointer> SharedData;
	typedef ParticleData<GlobalPointer> GlobalData;

	__device__ void simulateShared()
	{
		__shared__ uninitialized<ParticleDataPair<SharedData, SharedData>> particles;
		
		uint32_t configDataSize = gClothData.mNumPhases * gCuPhaseConfigSize;
		uint32_t particlesDataSize = 4 * gClothData.mNumParticles;

		SharedPointer<float>::Type sharedCurPos = gSharedMemory +
				configDataSize + threadIdx.x % 4 * gClothData.mNumParticles;
		SharedPointer<float>::Type sharedPrevPos = sharedCurPos + particlesDataSize;

		if(threadIdx.x < 4)
		{
			particles.get().mCurrent.mPointers[threadIdx.x] = sharedCurPos;
			particles.get().mPrevious.mPointers[threadIdx.x] = sharedPrevPos;
		}

		float* globalCurPos = gClothData.mParticles;
		float* globalPrevPos = gClothData.mParticles + particlesDataSize;

		// copy particles from device memory to shared memory and transpose
		for(uint32_t i = threadIdx.x; i < particlesDataSize; i += blockDim.x)
		{
			sharedCurPos[i/4] = globalCurPos[i];
			sharedPrevPos[i/4] = globalPrevPos[i];
		}

		simulateCloth(particles.get().mCurrent, particles.get().mPrevious);

		// copy particles from shared memory to device memory and transpose
		for(uint32_t i = threadIdx.x; i < particlesDataSize; i += blockDim.x)
		{
			globalCurPos[i] = sharedCurPos[i/4];
			globalPrevPos[i] = sharedPrevPos[i/4];
		}

		__syncthreads();
	}

	__device__ void simulateStreamed()
	{
		__shared__ uninitialized<ParticleDataPair<SharedData, GlobalData>> particles;

		uint32_t configDataSize = gClothData.mNumPhases * gCuPhaseConfigSize;
		uint32_t particlesDataSize = 4 * gClothData.mNumParticles;

		float* globalCurPos = gClothData.mParticles;
		SharedPointer<float>::Type sharedCurPos = gSharedMemory +
				configDataSize + threadIdx.x % 4 * gClothData.mNumParticles;

		if(threadIdx.x < 4)
		{
			particles.get().mCurrent.mPointers[threadIdx.x] = sharedCurPos;
			particles.get().mPrevious.mPointers[threadIdx.x] = globalCurPos 
				+ particlesDataSize + threadIdx.x;
		}

		// copy particles from device memory to shared memory and transpose
		for(uint32_t i = threadIdx.x; i < particlesDataSize; i += blockDim.x)
			sharedCurPos[i/4] = globalCurPos[i];

		simulateCloth(particles.get().mCurrent, particles.get().mPrevious);

		// copy particles from shared memory to device memory and transpose
		for(uint32_t i = threadIdx.x; i < particlesDataSize; i += blockDim.x)
			globalCurPos[i] = sharedCurPos[i/4];

		__syncthreads();
	}

	__device__ void simulateGlobal()
	{
		__shared__ uninitialized<ParticleDataPair<GlobalData, GlobalData>> particles;

		if(threadIdx.x < 8)
		{
			particles.get().mCurrent.mPointers[threadIdx.x] = gClothData.mParticles
				+ (threadIdx.x&4)*(gClothData.mNumParticles-1) + threadIdx.x;
		}

		simulateCloth(particles.get().mCurrent, particles.get().mPrevious);
	}

} // anonymous namespace

#if __CUDA_ARCH__ >= 300
__global__ void __launch_bounds__(1024, 1) simulateCloths()
#elif __CUDA_ARCH__ >= 200
__global__ void __launch_bounds__(512, 1) simulateCloths()
#else
__global__ void __launch_bounds__(192, 1) simulateCloths()
#endif
{
	ProfileZone zone(cloth::CuProfileZoneIds::SIMULATE);

	// check that http://nvbugs/1038473 is fixed
	assert(gSharedMemory > (float*)&gFrameData);
	assert(gSharedMemory > (float*)&gClothData);

	// fetch cloth index from queue
	__shared__ uint32_t clothIdx;
	if(!threadIdx.x)
		clothIdx = atomicInc(gKernelData.mClothIndex, gridDim.x-1);
	__syncthreads();
	assert(clothIdx < gridDim.x);

	// copy cloth data to shared memory
	const uint32_t* clothData = reinterpret_cast<const uint32_t*>(gKernelData.mClothData + clothIdx);
	if(threadIdx.x < gCuClothDataSize)
		reinterpret_cast<uint32_t*>(&gClothData)[threadIdx.x] = clothData[threadIdx.x];

	// copy frame data to shared memory
	uint32_t* frameData = reinterpret_cast<uint32_t*>(gKernelData.mFrameData + clothIdx);
	if(threadIdx.x < gCuFrameDataSize)
		reinterpret_cast<uint32_t*>(&gFrameData)[threadIdx.x] = frameData[threadIdx.x];

	__syncthreads();

	if(gFrameData.mSleepPassCounter >= gClothData.mSleepAfterCount)
		return; // cloth is sleeping, exit

	// copy phase configs to shared memory
	uint32_t configDataSize = gClothData.mNumPhases * gCuPhaseConfigSize;
	for(uint32_t i = threadIdx.x; i < configDataSize; i += blockDim.x)
		gSharedUnsigned[i] = reinterpret_cast<const uint32_t*>(gClothData.mPhaseConfigs)[i];

	SharedPointer<uint32_t>::Type scratchPtr = gSharedUnsigned + configDataSize 
		+ 4*gFrameData.mNumSharedPositions*gClothData.mNumParticles;

	// initialize with placement new
	new (gCollideParticles.data) CuCollision(scratchPtr);
	new (gSelfCollideParticles.data) CuSelfCollision();

	// copy particles and constraints to device
	if(gFrameData.mDeviceParticlesDirty)
	{
		for(uint32_t i = threadIdx.x; i < gClothData.mNumParticles*8; i += blockDim.x)
			gClothData.mParticles[i] = gClothData.mParticlesHostCopy[i];
	}
	if(gFrameData.mHostMotionConstraints)
	{
		for(uint32_t i = threadIdx.x; i < gClothData.mNumParticles*4; i += blockDim.x)
			gFrameData.mTargetMotionConstraints[i] = gFrameData.mHostMotionConstraints[i];
	}
	if(gFrameData.mHostSeparationConstraints)
	{
		for(uint32_t i = threadIdx.x; i < gClothData.mNumParticles*4; i += blockDim.x)
			gFrameData.mTargetSeparationConstraints[i] = gFrameData.mHostSeparationConstraints[i];
	}
	if(gFrameData.mHostParticleAccelerations)
	{
		for(uint32_t i = threadIdx.x; i < gClothData.mNumParticles*4; i += blockDim.x)
			gFrameData.mParticleAccelerations[i] = gFrameData.mHostParticleAccelerations[i];
	}

	// necessary to ensure phase configs are fully loaded before setup in simulateCloth()
	__syncthreads();

	switch(gFrameData.mNumSharedPositions) 
	{
	case 0: simulateGlobal(); break; 
	case 1: simulateStreamed(); break;  
	case 2: simulateShared(); break;  
	}
     
	// write back frame data 
	if(threadIdx.x < gCuFrameDataSize)
		frameData[threadIdx.x] = reinterpret_cast<const uint32_t*>(&gFrameData)[threadIdx.x];

	// copy particles to host
	for(uint32_t i = threadIdx.x; i < gClothData.mNumParticles*8; i += blockDim.x)
		gClothData.mParticlesHostCopy[i] = gClothData.mParticles[i];
}

const char* cloth::getKernelDataName()
{
	return "gKernelData";
}

const char* cloth::getKernelFunctionName()
{
	return "_Z14simulateClothsv";
}

