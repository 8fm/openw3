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

#ifndef HEADER_ForceFieldEffect_h
#define HEADER_ForceFieldEffect_h

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

namespace ForceFieldEffectNS
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
	NxParameterized::Interface* ForceField;

};

static const physx::PxU32 checksum[] = { 0x01814487, 0x33af1a7c, 0x51e2b782, 0x36868507, };

} // namespace ForceFieldEffectNS

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
class ForceFieldEffect : public NxParameterized::NxParameters, public ForceFieldEffectNS::ParametersStruct
{
public:
	ForceFieldEffect(NxParameterized::Traits* traits, void* buf = 0, PxI32* refCount = 0);

	virtual ~ForceFieldEffect();

	virtual void destroy();

	static const char* staticClassName(void)
	{
		return("ForceFieldEffect");
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
		bits = 8 * sizeof(ForceFieldEffectNS::checksum);
		return ForceFieldEffectNS::checksum;
	}

	static void freeParameterDefinitionTable(NxParameterized::Traits* traits);

	const physx::PxU32* checksum(physx::PxU32& bits) const
	{
		return staticChecksum(bits);
	}

	const ForceFieldEffectNS::ParametersStruct& parameters(void) const
	{
		ForceFieldEffect* tmpThis = const_cast<ForceFieldEffect*>(this);
		return *(static_cast<ForceFieldEffectNS::ParametersStruct*>(tmpThis));
	}

	ForceFieldEffectNS::ParametersStruct& parameters(void)
	{
		return *(static_cast<ForceFieldEffectNS::ParametersStruct*>(this));
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

class ForceFieldEffectFactory : public NxParameterized::Factory
{
	static const char* const vptr;

public:
	virtual NxParameterized::Interface* create(NxParameterized::Traits* paramTraits)
	{
		// placement new on this class using mParameterizedTraits

		void* newPtr = paramTraits->alloc(sizeof(ForceFieldEffect), ForceFieldEffect::ClassAlignment);
		if (!NxParameterized::IsAligned(newPtr, ForceFieldEffect::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class ForceFieldEffect");
			paramTraits->free(newPtr);
			return 0;
		}

		memset(newPtr, 0, sizeof(ForceFieldEffect)); // always initialize memory allocated to zero for default values
		return NX_PARAM_PLACEMENT_NEW(newPtr, ForceFieldEffect)(paramTraits);
	}

	virtual NxParameterized::Interface* finish(NxParameterized::Traits* paramTraits, void* bufObj, void* bufStart, physx::PxI32* refCount)
	{
		if (!NxParameterized::IsAligned(bufObj, ForceFieldEffect::ClassAlignment)
		        || !NxParameterized::IsAligned(bufStart, ForceFieldEffect::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class ForceFieldEffect");
			return 0;
		}

		// Init NxParameters-part
		// We used to call empty constructor of ForceFieldEffect here
		// but it may call default constructors of members and spoil the data
		NX_PARAM_PLACEMENT_NEW(bufObj, NxParameterized::NxParameters)(paramTraits, bufStart, refCount);

		// Init vtable (everything else is already initialized)
		*(const char**)bufObj = vptr;

		return (ForceFieldEffect*)bufObj;
	}

	virtual const char* getClassName()
	{
		return (ForceFieldEffect::staticClassName());
	}

	virtual physx::PxU32 getVersion()
	{
		return (ForceFieldEffect::staticVersion());
	}

	virtual physx::PxU32 getAlignment()
	{
		return (ForceFieldEffect::ClassAlignment);
	}

	virtual const physx::PxU32* getChecksum(physx::PxU32& bits)
	{
		return (ForceFieldEffect::staticChecksum(bits));
	}
};
#endif // NX_PARAMETERIZED_ONLY_LAYOUTS

} // namespace particles
} // namespace apex
} // namespace physx

#pragma warning(pop)

#endif
