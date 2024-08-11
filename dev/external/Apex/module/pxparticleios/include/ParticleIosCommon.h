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

#ifndef __PARTICLE_IOS_COMMON_H__
#define __PARTICLE_IOS_COMMON_H__

#include "PxMat34Legacy.h"
#include "foundation/PxBounds3.h"
#include "foundation/PxVec3.h"
#include "InplaceTypes.h"

const unsigned int INVALID_PARTICLE_INDEX		= (unsigned int)-1;

namespace physx
{
namespace apex
{
namespace pxparticleios
{

struct Px3InjectorParams
{
	physx::PxF32 mLODMaxDistance;
	physx::PxF32 mLODDistanceWeight;
	physx::PxF32 mLODSpeedWeight;
	physx::PxF32 mLODLifeWeight;
	physx::PxF32 mLODBias;
	physx::PxU32 mLocalIndex;

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(mLODMaxDistance);
		r.reflect(mLODDistanceWeight);
		r.reflect(mLODSpeedWeight);
		r.reflect(mLODLifeWeight);
		r.reflect(mLODBias);
		r.reflect(mLocalIndex);
	}
#endif
};

typedef InplaceArray<Px3InjectorParams> InjectorParamsArray;

struct GridDensityParams
{
	bool Enabled;
	physx::PxF32 GridSize;
	physx::PxU32 GridMaxCellCount;
	PxU32 GridResolution;
	physx::PxVec3 DensityOrigin;
	GridDensityParams(): Enabled(false) {}
};

struct GridDensityFrustumParams
{
	PxReal nearDimX;
	PxReal farDimX;
	PxReal nearDimY;
	PxReal farDimY;
	PxReal dimZ; 
};

#ifdef __CUDACC__



#define SIM_FETCH(name, idx) tex1Dfetch(KERNEL_TEX_REF(name), idx)
#define SIM_FLOAT4 float4
#define SIM_INT_AS_FLOAT(x) *(const PxF32*)(&x)

PX_CUDA_CALLABLE PX_INLINE physx::PxF32 splitFloat4(physx::PxVec3& v3, const SIM_FLOAT4& f4)
{
	v3.x = f4.x;
	v3.y = f4.y;
	v3.z = f4.z;
	return f4.w;
}
PX_CUDA_CALLABLE PX_INLINE SIM_FLOAT4 combineFloat4(const physx::PxVec3& v3, float w)
{
	return make_float4(v3.x, v3.y, v3.z, w);
}

struct PxInternalParticleFlagGpu
{
	enum Enum
	{
		//reserved	(1<<0),
		//reserved	(1<<1),
		//reserved	(1<<2),
		//reserved	(1<<3),
		//reserved	(1<<4),
		//reserved	(1<<5),
		eCUDA_NOTIFY_CREATE					= (1 << 6),
		eCUDA_NOTIFY_SET_POSITION			= (1 << 7),
	};
};
struct PxParticleFlag
{
	enum Enum
	{
		eVALID								= (1 << 0),
		eCOLLISION_WITH_STATIC				= (1 << 1),
		eCOLLISION_WITH_DYNAMIC				= (1 << 2),
		eCOLLISION_WITH_DRAIN				= (1 << 3),
		eSPATIAL_DATA_STRUCTURE_OVERFLOW	= (1 << 4),
	};
};
struct PxParticleFlagGpu
{
	PxU16 api;	// PxParticleFlag
	PxU16 low;	// PxInternalParticleFlagGpu
};

#else

#define SIM_FETCH(name, idx) mem##name[idx]
#define SIM_FLOAT4 physx::PxVec4
#define SIM_INT_AS_FLOAT(x) *(const PxF32*)(&x)

PX_INLINE physx::PxF32 splitFloat4(physx::PxVec3& v3, const SIM_FLOAT4& f4)
{
	v3 = f4.getXYZ();
	return f4.w;
}
PX_INLINE SIM_FLOAT4 combineFloat4(const physx::PxVec3& v3, float w)
{
	return physx::PxVec4(v3.x, v3.y, v3.z, w);
}

#endif


PX_CUDA_CALLABLE PX_INLINE physx::PxF32 calcParticleBenefit(
	const Px3InjectorParams& inj, const physx::PxVec3& eyePos,
	const physx::PxVec3& pos, const physx::PxVec3& vel, physx::PxF32 life)
{
	physx::PxF32 benefit = inj.mLODBias;
	//distance term
	physx::PxF32 distance = (eyePos - pos).magnitude();
	if (physx::PxIsFinite(distance))
	{
		benefit += inj.mLODDistanceWeight * (1.0f - physx::PxMin(1.0f, distance / inj.mLODMaxDistance));
	}
	//velocity term, TODO: clamp velocity
	physx::PxF32 velMag = vel.magnitude();
	if (physx::PxIsFinite(velMag))
	{
		benefit += inj.mLODSpeedWeight * velMag;
	}
	//life term
	benefit += inj.mLODLifeWeight * life;

	return physx::PxClamp(benefit, 0.0f, 1.0f);
}



template <typename FieldAccessor>
PX_CUDA_CALLABLE PX_INLINE float simulateParticle(
    const Px3InjectorParams* injectorParamsList,
    float deltaTime,
    physx::PxVec3 eyePos,
    bool isNewParticle,
    unsigned int srcIdx,
    unsigned int dstIdx,
    SIM_FLOAT4* memPositionMass,
    SIM_FLOAT4* memVelocityLife,
	SIM_FLOAT4* memCollisionNormalFlags,
	physx::PxU32* memUserData,
    NiIofxActorID* memIofxActorIDs,
    float* memLifeSpan,
    float* memLifeTime,
	float* memDensity,
    unsigned int* memInjector,
    SIM_FLOAT4* memPxPosition,
    SIM_FLOAT4* memPxVelocity,
	SIM_FLOAT4* memPxCollision,
	float* memPxDensity,
    unsigned int* memPxFlags,
    FieldAccessor& fieldAccessor,
	unsigned int &injIndex,
	const GridDensityParams params
)
{
	PX_UNUSED(memCollisionNormalFlags);
	PX_UNUSED(memPxPosition);
	PX_UNUSED(memPxVelocity);
	PX_UNUSED(memPxCollision);
	PX_UNUSED(memPxDensity);
	PX_UNUSED(memDensity);
	PX_UNUSED(memPxFlags);
	PX_UNUSED(fieldAccessor);
	PX_UNUSED(params);

	//read
	physx::PxVec3 position;
	physx::PxVec3 velocity;
	physx::PxF32 mass = splitFloat4(position, SIM_FETCH(PositionMass, srcIdx));
	splitFloat4(velocity, SIM_FETCH(VelocityLife, srcIdx));
	physx::PxF32 lifeSpan = SIM_FETCH(LifeSpan, srcIdx);
	unsigned int injector = SIM_FETCH(Injector, srcIdx);
	NiIofxActorID iofxActorID = NiIofxActorID(SIM_FETCH(IofxActorIDs, srcIdx));
	
#ifdef __CUDACC__
	if (memPxFlags != NULL)
	{
		float density = 0.0f;
		if (isNewParticle)
		{
			memPxPosition[dstIdx]	= combineFloat4(position, 0);
			memPxVelocity[dstIdx]	= combineFloat4(velocity, 0);

			PxParticleFlagGpu flags;
			flags.api = PxParticleFlag::eVALID;
			flags.low = PxInternalParticleFlagGpu::eCUDA_NOTIFY_CREATE;
			memPxFlags[dstIdx] = *((unsigned int*)&flags);

			unsigned int collisionFlags = 0;
			memCollisionNormalFlags[dstIdx] = combineFloat4(physx::PxVec3(0.0f, 1.0f, 0.0f), SIM_INT_AS_FLOAT(collisionFlags));
		}
		else
		{
			const SIM_FLOAT4 pxPosition = SIM_FETCH(PxPosition, srcIdx);
			const SIM_FLOAT4 pxVelocity = SIM_FETCH(PxVelocity, srcIdx);

			if(memPxDensity) 
			{
				 density = SIM_FETCH(PxDensity, srcIdx);
				 memPxDensity[dstIdx] = density;
			}
			position = PxVec3(pxPosition.x, pxPosition.y, pxPosition.z);
			velocity = PxVec3(pxVelocity.x, pxVelocity.y, pxVelocity.z);

			PxParticleFlagGpu flags;
			*((unsigned int*)&flags) = SIM_FETCH(PxFlags, srcIdx);
			//flags.api = PxParticleFlag::eVALID;
			//flags.low = 0;

			/* Apply field sampler velocity */
			fieldAccessor(srcIdx, velocity);
			memPxVelocity[dstIdx] = combineFloat4(velocity, pxVelocity.w);

			const SIM_FLOAT4 pxCollision = SIM_FETCH(PxCollision, srcIdx);
			physx::PxVec3 collisionNormal(pxCollision.x, pxCollision.y, pxCollision.z);

			unsigned int collisionFlags = flags.low;
			memCollisionNormalFlags[dstIdx] = combineFloat4(collisionNormal, SIM_INT_AS_FLOAT(collisionFlags));

			if (dstIdx != srcIdx)
			{
				memPxPosition[dstIdx] = pxPosition;

				flags.low |= PxInternalParticleFlagGpu::eCUDA_NOTIFY_SET_POSITION;
				memPxFlags[dstIdx] = *((unsigned int*)&flags);
			}
		}

		if (memDensity != 0)
		{
			memDensity[dstIdx] = density;
		}
	}
#endif

	PxReal lifeTime = lifeSpan;
	if (!isNewParticle)
	{
		lifeTime = SIM_FETCH(LifeTime, srcIdx);
		lifeTime = physx::PxMax(lifeTime - deltaTime, 0.0f);
	}

	//write
	memLifeTime[dstIdx] = lifeTime;
	memPositionMass[dstIdx] = combineFloat4(position, mass);
	memVelocityLife[dstIdx] = combineFloat4(velocity, lifeTime / lifeSpan);

	if (dstIdx != srcIdx)
	{
		memIofxActorIDs[dstIdx] = iofxActorID;
		memLifeSpan[dstIdx]		= lifeSpan;
		memInjector[dstIdx]		= injector;

		memUserData[dstIdx] = SIM_FETCH(UserData,srcIdx);
	}

	const Px3InjectorParams& inj = injectorParamsList[injector];
	injIndex = inj.mLocalIndex;

	float benefit = -FLT_MAX;
	bool validActorID = isNewParticle || (iofxActorID.getVolumeID() != NiIofxActorID::NO_VOLUME);
	if (!validActorID)
	{
		injIndex = PX_MAX_U32;
	}

	if (lifeTime > 0.0f && inj.mLODBias < FLT_MAX && validActorID)
	{
		benefit = calcParticleBenefit(inj, eyePos, position, velocity, lifeTime / lifeSpan);
	}
	
	return benefit;
}

}
}
} // namespace physx::apex

#endif
