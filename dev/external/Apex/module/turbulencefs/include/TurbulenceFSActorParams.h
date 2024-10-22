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
// Created: 2013.10.01 14:57:25

#ifndef HEADER_TurbulenceFSActorParams_h
#define HEADER_TurbulenceFSActorParams_h

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
namespace turbulencefs
{

#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())

namespace TurbulenceFSActorParamsNS
{



struct ParametersStruct
{

	physx::PxMat34Legacy initialPose;
	physx::PxVec3 gridSizeWorld;
	NxParameterized::DummyStringStruct collisionFilterDataName;
	NxParameterized::DummyStringStruct fieldBoundaryFilterDataName;
	NxParameterized::DummyStringStruct fieldSamplerFilterDataName;
	bool isAutoSwitchingBC;
	physx::PxF32 outletPressureBC;
	physx::PxF32 dragCoeffForRigidBody;
	physx::PxF32 fluidViscosity;

};

static const physx::PxU32 checksum[] = { 0x5451ef73, 0xfaa48f03, 0xdb0c2660, 0xe720efea, };

} // namespace TurbulenceFSActorParamsNS

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
class TurbulenceFSActorParams : public NxParameterized::NxParameters, public TurbulenceFSActorParamsNS::ParametersStruct
{
public:
	TurbulenceFSActorParams(NxParameterized::Traits* traits, void* buf = 0, PxI32* refCount = 0);

	virtual ~TurbulenceFSActorParams();

	virtual void destroy();

	static const char* staticClassName(void)
	{
		return("TurbulenceFSActorParams");
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
		bits = 8 * sizeof(TurbulenceFSActorParamsNS::checksum);
		return TurbulenceFSActorParamsNS::checksum;
	}

	static void freeParameterDefinitionTable(NxParameterized::Traits* traits);

	const physx::PxU32* checksum(physx::PxU32& bits) const
	{
		return staticChecksum(bits);
	}

	const TurbulenceFSActorParamsNS::ParametersStruct& parameters(void) const
	{
		TurbulenceFSActorParams* tmpThis = const_cast<TurbulenceFSActorParams*>(this);
		return *(static_cast<TurbulenceFSActorParamsNS::ParametersStruct*>(tmpThis));
	}

	TurbulenceFSActorParamsNS::ParametersStruct& parameters(void)
	{
		return *(static_cast<TurbulenceFSActorParamsNS::ParametersStruct*>(this));
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

class TurbulenceFSActorParamsFactory : public NxParameterized::Factory
{
	static const char* const vptr;

public:
	virtual NxParameterized::Interface* create(NxParameterized::Traits* paramTraits)
	{
		// placement new on this class using mParameterizedTraits

		void* newPtr = paramTraits->alloc(sizeof(TurbulenceFSActorParams), TurbulenceFSActorParams::ClassAlignment);
		if (!NxParameterized::IsAligned(newPtr, TurbulenceFSActorParams::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class TurbulenceFSActorParams");
			paramTraits->free(newPtr);
			return 0;
		}

		memset(newPtr, 0, sizeof(TurbulenceFSActorParams)); // always initialize memory allocated to zero for default values
		return NX_PARAM_PLACEMENT_NEW(newPtr, TurbulenceFSActorParams)(paramTraits);
	}

	virtual NxParameterized::Interface* finish(NxParameterized::Traits* paramTraits, void* bufObj, void* bufStart, physx::PxI32* refCount)
	{
		if (!NxParameterized::IsAligned(bufObj, TurbulenceFSActorParams::ClassAlignment)
		        || !NxParameterized::IsAligned(bufStart, TurbulenceFSActorParams::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class TurbulenceFSActorParams");
			return 0;
		}

		// Init NxParameters-part
		// We used to call empty constructor of TurbulenceFSActorParams here
		// but it may call default constructors of members and spoil the data
		NX_PARAM_PLACEMENT_NEW(bufObj, NxParameterized::NxParameters)(paramTraits, bufStart, refCount);

		// Init vtable (everything else is already initialized)
		*(const char**)bufObj = vptr;

		return (TurbulenceFSActorParams*)bufObj;
	}

	virtual const char* getClassName()
	{
		return (TurbulenceFSActorParams::staticClassName());
	}

	virtual physx::PxU32 getVersion()
	{
		return (TurbulenceFSActorParams::staticVersion());
	}

	virtual physx::PxU32 getAlignment()
	{
		return (TurbulenceFSActorParams::ClassAlignment);
	}

	virtual const physx::PxU32* getChecksum(physx::PxU32& bits)
	{
		return (TurbulenceFSActorParams::staticChecksum(bits));
	}
};
#endif // NX_PARAMETERIZED_ONLY_LAYOUTS

} // namespace turbulencefs
} // namespace apex
} // namespace physx

#pragma warning(pop)

#endif
