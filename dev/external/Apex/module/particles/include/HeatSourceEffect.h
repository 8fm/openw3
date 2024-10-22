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
// Created: 2013.10.01 14:57:23

#ifndef HEADER_HeatSourceEffect_h
#define HEADER_HeatSourceEffect_h

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
namespace particles
{

#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())

namespace HeatSourceEffectNS
{

struct TranslateObject_Type;
struct OrientObject_Type;
struct EffectProperties_Type;

struct TranslateObject_Type
{
	physx::PxF32 TranslateX;
	physx::PxF32 TranslateY;
	physx::PxF32 TranslateZ;
};
struct OrientObject_Type
{
	physx::PxF32 RotateX;
	physx::PxF32 RotateY;
	physx::PxF32 RotateZ;
};
struct EffectProperties_Type
{
	NxParameterized::DummyStringStruct UserString;
	bool Enable;
	TranslateObject_Type Position;
	OrientObject_Type Orientation;
	physx::PxF32 InitialDelayTime;
	physx::PxF32 Duration;
	physx::PxU32 RepeatCount;
	physx::PxF32 RepeatDelay;
	physx::PxF32 RandomizeRepeatTime;
};

struct ParametersStruct
{

	EffectProperties_Type EffectProperties;
	NxParameterized::Interface* HeatSource;

};

static const physx::PxU32 checksum[] = { 0x8b986870, 0x47d5ef3d, 0xce64e48c, 0xef9776aa, };

} // namespace HeatSourceEffectNS

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
class HeatSourceEffect : public NxParameterized::NxParameters, public HeatSourceEffectNS::ParametersStruct
{
public:
	HeatSourceEffect(NxParameterized::Traits* traits, void* buf = 0, PxI32* refCount = 0);

	virtual ~HeatSourceEffect();

	virtual void destroy();

	static const char* staticClassName(void)
	{
		return("HeatSourceEffect");
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
		bits = 8 * sizeof(HeatSourceEffectNS::checksum);
		return HeatSourceEffectNS::checksum;
	}

	static void freeParameterDefinitionTable(NxParameterized::Traits* traits);

	const physx::PxU32* checksum(physx::PxU32& bits) const
	{
		return staticChecksum(bits);
	}

	const HeatSourceEffectNS::ParametersStruct& parameters(void) const
	{
		HeatSourceEffect* tmpThis = const_cast<HeatSourceEffect*>(this);
		return *(static_cast<HeatSourceEffectNS::ParametersStruct*>(tmpThis));
	}

	HeatSourceEffectNS::ParametersStruct& parameters(void)
	{
		return *(static_cast<HeatSourceEffectNS::ParametersStruct*>(this));
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

class HeatSourceEffectFactory : public NxParameterized::Factory
{
	static const char* const vptr;

public:
	virtual NxParameterized::Interface* create(NxParameterized::Traits* paramTraits)
	{
		// placement new on this class using mParameterizedTraits

		void* newPtr = paramTraits->alloc(sizeof(HeatSourceEffect), HeatSourceEffect::ClassAlignment);
		if (!NxParameterized::IsAligned(newPtr, HeatSourceEffect::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class HeatSourceEffect");
			paramTraits->free(newPtr);
			return 0;
		}

		memset(newPtr, 0, sizeof(HeatSourceEffect)); // always initialize memory allocated to zero for default values
		return NX_PARAM_PLACEMENT_NEW(newPtr, HeatSourceEffect)(paramTraits);
	}

	virtual NxParameterized::Interface* finish(NxParameterized::Traits* paramTraits, void* bufObj, void* bufStart, physx::PxI32* refCount)
	{
		if (!NxParameterized::IsAligned(bufObj, HeatSourceEffect::ClassAlignment)
		        || !NxParameterized::IsAligned(bufStart, HeatSourceEffect::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class HeatSourceEffect");
			return 0;
		}

		// Init NxParameters-part
		// We used to call empty constructor of HeatSourceEffect here
		// but it may call default constructors of members and spoil the data
		NX_PARAM_PLACEMENT_NEW(bufObj, NxParameterized::NxParameters)(paramTraits, bufStart, refCount);

		// Init vtable (everything else is already initialized)
		*(const char**)bufObj = vptr;

		return (HeatSourceEffect*)bufObj;
	}

	virtual const char* getClassName()
	{
		return (HeatSourceEffect::staticClassName());
	}

	virtual physx::PxU32 getVersion()
	{
		return (HeatSourceEffect::staticVersion());
	}

	virtual physx::PxU32 getAlignment()
	{
		return (HeatSourceEffect::ClassAlignment);
	}

	virtual const physx::PxU32* getChecksum(physx::PxU32& bits)
	{
		return (HeatSourceEffect::staticChecksum(bits));
	}
};
#endif // NX_PARAMETERIZED_ONLY_LAYOUTS

} // namespace particles
} // namespace apex
} // namespace physx

#pragma warning(pop)

#endif
