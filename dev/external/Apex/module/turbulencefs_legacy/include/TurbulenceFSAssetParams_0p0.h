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

#ifndef HEADER_TurbulenceFSAssetParams_0p0_h
#define HEADER_TurbulenceFSAssetParams_0p0_h

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

namespace TurbulenceFSAssetParams_0p0NS
{

struct rangeStructF32_Type;
struct rangeStructU32_Type;

struct rangeStructF32_Type
{
	physx::PxF32 min;
	physx::PxF32 max;
};
struct rangeStructU32_Type
{
	physx::PxU32 min;
	physx::PxU32 max;
};

struct ParametersStruct
{

	rangeStructF32_Type gridXRange;
	rangeStructF32_Type gridYRange;
	rangeStructF32_Type gridZRange;
	physx::PxVec3 gridSizeWorld;
	rangeStructF32_Type updatesPerFrameRange;
	physx::PxF32 fluidVelocityMultiplier;
	physx::PxF32 fluidVelocityClamp;
	physx::PxF32 boundaryFadePercentage;
	physx::PxF32 boundarySizePercentage;
	NxParameterized::DummyStringStruct collisionFilterDataName;
	NxParameterized::DummyStringStruct fieldBoundaryFilterDataName;
	NxParameterized::DummyStringStruct fieldSamplerFilterDataName;
	physx::PxU32 maxCollidingObjects;
	physx::PxU32 maxHeatSources;

};

static const physx::PxU32 checksum[] = { 0x60827838, 0x801870ba, 0xa303c521, 0x0b9ee96e, };

} // namespace TurbulenceFSAssetParams_0p0NS

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
class TurbulenceFSAssetParams_0p0 : public NxParameterized::NxParameters, public TurbulenceFSAssetParams_0p0NS::ParametersStruct
{
public:
	TurbulenceFSAssetParams_0p0(NxParameterized::Traits* traits, void* buf = 0, PxI32* refCount = 0);

	virtual ~TurbulenceFSAssetParams_0p0();

	virtual void destroy();

	static const char* staticClassName(void)
	{
		return("TurbulenceFSAssetParams");
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
		bits = 8 * sizeof(TurbulenceFSAssetParams_0p0NS::checksum);
		return TurbulenceFSAssetParams_0p0NS::checksum;
	}

	static void freeParameterDefinitionTable(NxParameterized::Traits* traits);

	const physx::PxU32* checksum(physx::PxU32& bits) const
	{
		return staticChecksum(bits);
	}

	const TurbulenceFSAssetParams_0p0NS::ParametersStruct& parameters(void) const
	{
		TurbulenceFSAssetParams_0p0* tmpThis = const_cast<TurbulenceFSAssetParams_0p0*>(this);
		return *(static_cast<TurbulenceFSAssetParams_0p0NS::ParametersStruct*>(tmpThis));
	}

	TurbulenceFSAssetParams_0p0NS::ParametersStruct& parameters(void)
	{
		return *(static_cast<TurbulenceFSAssetParams_0p0NS::ParametersStruct*>(this));
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

class TurbulenceFSAssetParams_0p0Factory : public NxParameterized::Factory
{
	static const char* const vptr;

public:
	virtual NxParameterized::Interface* create(NxParameterized::Traits* paramTraits)
	{
		// placement new on this class using mParameterizedTraits

		void* newPtr = paramTraits->alloc(sizeof(TurbulenceFSAssetParams_0p0), TurbulenceFSAssetParams_0p0::ClassAlignment);
		if (!NxParameterized::IsAligned(newPtr, TurbulenceFSAssetParams_0p0::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class TurbulenceFSAssetParams_0p0");
			paramTraits->free(newPtr);
			return 0;
		}

		memset(newPtr, 0, sizeof(TurbulenceFSAssetParams_0p0)); // always initialize memory allocated to zero for default values
		return NX_PARAM_PLACEMENT_NEW(newPtr, TurbulenceFSAssetParams_0p0)(paramTraits);
	}

	virtual NxParameterized::Interface* finish(NxParameterized::Traits* paramTraits, void* bufObj, void* bufStart, physx::PxI32* refCount)
	{
		if (!NxParameterized::IsAligned(bufObj, TurbulenceFSAssetParams_0p0::ClassAlignment)
		        || !NxParameterized::IsAligned(bufStart, TurbulenceFSAssetParams_0p0::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class TurbulenceFSAssetParams_0p0");
			return 0;
		}

		// Init NxParameters-part
		// We used to call empty constructor of TurbulenceFSAssetParams_0p0 here
		// but it may call default constructors of members and spoil the data
		NX_PARAM_PLACEMENT_NEW(bufObj, NxParameterized::NxParameters)(paramTraits, bufStart, refCount);

		// Init vtable (everything else is already initialized)
		*(const char**)bufObj = vptr;

		return (TurbulenceFSAssetParams_0p0*)bufObj;
	}

	virtual const char* getClassName()
	{
		return (TurbulenceFSAssetParams_0p0::staticClassName());
	}

	virtual physx::PxU32 getVersion()
	{
		return (TurbulenceFSAssetParams_0p0::staticVersion());
	}

	virtual physx::PxU32 getAlignment()
	{
		return (TurbulenceFSAssetParams_0p0::ClassAlignment);
	}

	virtual const physx::PxU32* getChecksum(physx::PxU32& bits)
	{
		return (TurbulenceFSAssetParams_0p0::staticChecksum(bits));
	}
};
#endif // NX_PARAMETERIZED_ONLY_LAYOUTS

} // namespace apex
} // namespace physx

#pragma warning(pop)

#endif
