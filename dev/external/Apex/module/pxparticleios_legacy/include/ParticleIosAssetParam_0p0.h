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

// This file was generated by NxParameterized/scripts/GenParameterized.pl
// Created: 2013.10.01 14:57:26

#ifndef HEADER_ParticleIosAssetParam_0p0_h
#define HEADER_ParticleIosAssetParam_0p0_h

#include "NxParametersTypes.h"

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
#include "NxParameterized.h"
#include "NxParameters.h"
#include "NxParameterizedTraits.h"
#include "NxTraitsInternal.h"
#endif

namespace physx
{
namespace apex
{

#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())

namespace ParticleIosAssetParam_0p0NS
{



struct ParametersStruct
{

	NxParameterized::Interface* particleType;
	physx::PxU32 maxParticleCount;
	physx::PxF32 particleRadius;
	physx::PxF32 maxInjectedParticleCount;
	physx::PxF32 maxMotionDistance;
	physx::PxF32 contactOffset;
	physx::PxF32 restOffset;
	physx::PxF32 gridSize;
	physx::PxF32 damping;
	physx::PxVec3 externalAcceleration;
	physx::PxVec3 projectionPlaneNormal;
	physx::PxF32 projectionPlaneDistance;
	physx::PxF32 particleMass;
	physx::PxF32 restitution;
	physx::PxF32 dynamicFriction;
	physx::PxF32 staticFriction;
	NxParameterized::DummyStringStruct simulationFilterData;
	bool CollisionTwoway;
	bool CollisionWithDynamicActors;
	bool Enable;
	bool ProjectToPlane;
	bool PerParticleRestOffset;
	bool PerParticleCollisionCacheHint;
	bool useGPU;
	bool PositionBuffer;
	bool VelocityBuffer;
	bool RestOffsetBuffer;
	bool FlagsBuffer;
	bool CollisionNormalBuffer;
	bool DensityBuffer;

};

static const physx::PxU32 checksum[] = { 0x6ade1528, 0x48d5e8e8, 0x974dda3a, 0xd10d8d0a, };

} // namespace ParticleIosAssetParam_0p0NS

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
class ParticleIosAssetParam_0p0 : public NxParameterized::NxParameters, public ParticleIosAssetParam_0p0NS::ParametersStruct
{
public:
	ParticleIosAssetParam_0p0(NxParameterized::Traits* traits, void* buf = 0, PxI32* refCount = 0);

	virtual ~ParticleIosAssetParam_0p0();

	virtual void destroy();

	static const char* staticClassName(void)
	{
		return("ParticleIosAssetParam");
	}

	const char* className(void) const
	{
		return(staticClassName());
	}

	static const physx::PxU32 ClassVersion = ((physx::PxU32)0 << 16) + (physx::PxU32)0;

	static physx::PxU32 staticVersion(void)
	{
		return ClassVersion;
	}

	physx::PxU32 version(void) const
	{
		return(staticVersion());
	}

	static const physx::PxU32 ClassAlignment = 8;

	static const physx::PxU32* staticChecksum(physx::PxU32& bits)
	{
		bits = 8 * sizeof(ParticleIosAssetParam_0p0NS::checksum);
		return ParticleIosAssetParam_0p0NS::checksum;
	}

	static void freeParameterDefinitionTable(NxParameterized::Traits* traits);

	const physx::PxU32* checksum(physx::PxU32& bits) const
	{
		return staticChecksum(bits);
	}

	const ParticleIosAssetParam_0p0NS::ParametersStruct& parameters(void) const
	{
		ParticleIosAssetParam_0p0* tmpThis = const_cast<ParticleIosAssetParam_0p0*>(this);
		return *(static_cast<ParticleIosAssetParam_0p0NS::ParametersStruct*>(tmpThis));
	}

	ParticleIosAssetParam_0p0NS::ParametersStruct& parameters(void)
	{
		return *(static_cast<ParticleIosAssetParam_0p0NS::ParametersStruct*>(this));
	}

	virtual NxParameterized::ErrorType getParameterHandle(const char* long_name, NxParameterized::Handle& handle) const;
	virtual NxParameterized::ErrorType getParameterHandle(const char* long_name, NxParameterized::Handle& handle);

	void initDefaults(void);

protected:

	virtual const NxParameterized::DefinitionImpl* getParameterDefinitionTree(void);
	virtual const NxParameterized::DefinitionImpl* getParameterDefinitionTree(void) const;


	virtual void getVarPtr(const NxParameterized::Handle& handle, void*& ptr, size_t& offset) const;

private:

	void buildTree(void);
	void initDynamicArrays(void);
	void initStrings(void);
	void initReferences(void);
	void freeDynamicArrays(void);
	void freeStrings(void);
	void freeReferences(void);

	static bool mBuiltFlag;
	static NxParameterized::MutexType mBuiltFlagMutex;
};

class ParticleIosAssetParam_0p0Factory : public NxParameterized::Factory
{
	static const char* const vptr;

public:
	virtual NxParameterized::Interface* create(NxParameterized::Traits* paramTraits)
	{
		// placement new on this class using mParameterizedTraits

		void* newPtr = paramTraits->alloc(sizeof(ParticleIosAssetParam_0p0), ParticleIosAssetParam_0p0::ClassAlignment);
		if (!NxParameterized::IsAligned(newPtr, ParticleIosAssetParam_0p0::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class ParticleIosAssetParam_0p0");
			paramTraits->free(newPtr);
			return 0;
		}

		memset(newPtr, 0, sizeof(ParticleIosAssetParam_0p0)); // always initialize memory allocated to zero for default values
		return NX_PARAM_PLACEMENT_NEW(newPtr, ParticleIosAssetParam_0p0)(paramTraits);
	}

	virtual NxParameterized::Interface* finish(NxParameterized::Traits* paramTraits, void* bufObj, void* bufStart, physx::PxI32* refCount)
	{
		if (!NxParameterized::IsAligned(bufObj, ParticleIosAssetParam_0p0::ClassAlignment)
		        || !NxParameterized::IsAligned(bufStart, ParticleIosAssetParam_0p0::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class ParticleIosAssetParam_0p0");
			return 0;
		}

		// Init NxParameters-part
		// We used to call empty constructor of ParticleIosAssetParam_0p0 here
		// but it may call default constructors of members and spoil the data
		NX_PARAM_PLACEMENT_NEW(bufObj, NxParameterized::NxParameters)(paramTraits, bufStart, refCount);

		// Init vtable (everything else is already initialized)
		*(const char**)bufObj = vptr;

		return (ParticleIosAssetParam_0p0*)bufObj;
	}

	virtual const char* getClassName()
	{
		return (ParticleIosAssetParam_0p0::staticClassName());
	}

	virtual physx::PxU32 getVersion()
	{
		return (ParticleIosAssetParam_0p0::staticVersion());
	}

	virtual physx::PxU32 getAlignment()
	{
		return (ParticleIosAssetParam_0p0::ClassAlignment);
	}

	virtual const physx::PxU32* getChecksum(physx::PxU32& bits)
	{
		return (ParticleIosAssetParam_0p0::staticChecksum(bits));
	}
};
#endif // NX_PARAMETERIZED_ONLY_LAYOUTS

} // namespace apex
} // namespace physx

#pragma warning(pop)

#endif
