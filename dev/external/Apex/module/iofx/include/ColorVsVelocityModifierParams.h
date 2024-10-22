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
// Created: 2013.10.01 14:57:22

#ifndef HEADER_ColorVsVelocityModifierParams_h
#define HEADER_ColorVsVelocityModifierParams_h

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
namespace iofx
{

#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())

namespace ColorVsVelocityModifierParamsNS
{

struct vec2_Type;

struct vec2_DynamicArray1D_Type
{
	vec2_Type* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct vec2_Type
{
	physx::PxF32 x;
	physx::PxF32 y;
};

struct ParametersStruct
{

	const char* colorChannel;
	physx::PxF32 velocity0;
	physx::PxF32 velocity1;
	vec2_DynamicArray1D_Type controlPoints;

};

static const physx::PxU32 checksum[] = { 0x4c79c470, 0xac91d48e, 0xa4578ba1, 0x9c1ae2f2, };

} // namespace ColorVsVelocityModifierParamsNS

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
class ColorVsVelocityModifierParams : public NxParameterized::NxParameters, public ColorVsVelocityModifierParamsNS::ParametersStruct
{
public:
	ColorVsVelocityModifierParams(NxParameterized::Traits* traits, void* buf = 0, PxI32* refCount = 0);

	virtual ~ColorVsVelocityModifierParams();

	virtual void destroy();

	static const char* staticClassName(void)
	{
		return("ColorVsVelocityModifierParams");
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
		bits = 8 * sizeof(ColorVsVelocityModifierParamsNS::checksum);
		return ColorVsVelocityModifierParamsNS::checksum;
	}

	static void freeParameterDefinitionTable(NxParameterized::Traits* traits);

	const physx::PxU32* checksum(physx::PxU32& bits) const
	{
		return staticChecksum(bits);
	}

	const ColorVsVelocityModifierParamsNS::ParametersStruct& parameters(void) const
	{
		ColorVsVelocityModifierParams* tmpThis = const_cast<ColorVsVelocityModifierParams*>(this);
		return *(static_cast<ColorVsVelocityModifierParamsNS::ParametersStruct*>(tmpThis));
	}

	ColorVsVelocityModifierParamsNS::ParametersStruct& parameters(void)
	{
		return *(static_cast<ColorVsVelocityModifierParamsNS::ParametersStruct*>(this));
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

class ColorVsVelocityModifierParamsFactory : public NxParameterized::Factory
{
	static const char* const vptr;

public:
	virtual NxParameterized::Interface* create(NxParameterized::Traits* paramTraits)
	{
		// placement new on this class using mParameterizedTraits

		void* newPtr = paramTraits->alloc(sizeof(ColorVsVelocityModifierParams), ColorVsVelocityModifierParams::ClassAlignment);
		if (!NxParameterized::IsAligned(newPtr, ColorVsVelocityModifierParams::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class ColorVsVelocityModifierParams");
			paramTraits->free(newPtr);
			return 0;
		}

		memset(newPtr, 0, sizeof(ColorVsVelocityModifierParams)); // always initialize memory allocated to zero for default values
		return NX_PARAM_PLACEMENT_NEW(newPtr, ColorVsVelocityModifierParams)(paramTraits);
	}

	virtual NxParameterized::Interface* finish(NxParameterized::Traits* paramTraits, void* bufObj, void* bufStart, physx::PxI32* refCount)
	{
		if (!NxParameterized::IsAligned(bufObj, ColorVsVelocityModifierParams::ClassAlignment)
		        || !NxParameterized::IsAligned(bufStart, ColorVsVelocityModifierParams::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class ColorVsVelocityModifierParams");
			return 0;
		}

		// Init NxParameters-part
		// We used to call empty constructor of ColorVsVelocityModifierParams here
		// but it may call default constructors of members and spoil the data
		NX_PARAM_PLACEMENT_NEW(bufObj, NxParameterized::NxParameters)(paramTraits, bufStart, refCount);

		// Init vtable (everything else is already initialized)
		*(const char**)bufObj = vptr;

		return (ColorVsVelocityModifierParams*)bufObj;
	}

	virtual const char* getClassName()
	{
		return (ColorVsVelocityModifierParams::staticClassName());
	}

	virtual physx::PxU32 getVersion()
	{
		return (ColorVsVelocityModifierParams::staticVersion());
	}

	virtual physx::PxU32 getAlignment()
	{
		return (ColorVsVelocityModifierParams::ClassAlignment);
	}

	virtual const physx::PxU32* getChecksum(physx::PxU32& bits)
	{
		return (ColorVsVelocityModifierParams::staticChecksum(bits));
	}
};
#endif // NX_PARAMETERIZED_ONLY_LAYOUTS

} // namespace iofx
} // namespace apex
} // namespace physx

#pragma warning(pop)

#endif
