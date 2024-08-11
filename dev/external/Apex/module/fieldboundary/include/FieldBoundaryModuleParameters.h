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
// Created: 2013.10.01 14:57:21

#ifndef HEADER_FieldBoundaryModuleParameters_h
#define HEADER_FieldBoundaryModuleParameters_h

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
namespace fieldboundary
{

#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())

namespace FieldBoundaryModuleParametersNS
{



struct ParametersStruct
{

	physx::PxU32 unused;

};

static const physx::PxU32 checksum[] = { 0x6d3b9932, 0x95578fe1, 0xd23c3b13, 0x3aa23b4b, };

} // namespace FieldBoundaryModuleParametersNS

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
class FieldBoundaryModuleParameters : public NxParameterized::NxParameters, public FieldBoundaryModuleParametersNS::ParametersStruct
{
public:
	FieldBoundaryModuleParameters(NxParameterized::Traits* traits, void* buf = 0, PxI32* refCount = 0);

	virtual ~FieldBoundaryModuleParameters();

	virtual void destroy();

	static const char* staticClassName(void)
	{
		return("FieldBoundaryModuleParameters");
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
		bits = 8 * sizeof(FieldBoundaryModuleParametersNS::checksum);
		return FieldBoundaryModuleParametersNS::checksum;
	}

	static void freeParameterDefinitionTable(NxParameterized::Traits* traits);

	const physx::PxU32* checksum(physx::PxU32& bits) const
	{
		return staticChecksum(bits);
	}

	const FieldBoundaryModuleParametersNS::ParametersStruct& parameters(void) const
	{
		FieldBoundaryModuleParameters* tmpThis = const_cast<FieldBoundaryModuleParameters*>(this);
		return *(static_cast<FieldBoundaryModuleParametersNS::ParametersStruct*>(tmpThis));
	}

	FieldBoundaryModuleParametersNS::ParametersStruct& parameters(void)
	{
		return *(static_cast<FieldBoundaryModuleParametersNS::ParametersStruct*>(this));
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

class FieldBoundaryModuleParametersFactory : public NxParameterized::Factory
{
	static const char* const vptr;

public:
	virtual NxParameterized::Interface* create(NxParameterized::Traits* paramTraits)
	{
		// placement new on this class using mParameterizedTraits

		void* newPtr = paramTraits->alloc(sizeof(FieldBoundaryModuleParameters), FieldBoundaryModuleParameters::ClassAlignment);
		if (!NxParameterized::IsAligned(newPtr, FieldBoundaryModuleParameters::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class FieldBoundaryModuleParameters");
			paramTraits->free(newPtr);
			return 0;
		}

		memset(newPtr, 0, sizeof(FieldBoundaryModuleParameters)); // always initialize memory allocated to zero for default values
		return NX_PARAM_PLACEMENT_NEW(newPtr, FieldBoundaryModuleParameters)(paramTraits);
	}

	virtual NxParameterized::Interface* finish(NxParameterized::Traits* paramTraits, void* bufObj, void* bufStart, physx::PxI32* refCount)
	{
		if (!NxParameterized::IsAligned(bufObj, FieldBoundaryModuleParameters::ClassAlignment)
		        || !NxParameterized::IsAligned(bufStart, FieldBoundaryModuleParameters::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class FieldBoundaryModuleParameters");
			return 0;
		}

		// Init NxParameters-part
		// We used to call empty constructor of FieldBoundaryModuleParameters here
		// but it may call default constructors of members and spoil the data
		NX_PARAM_PLACEMENT_NEW(bufObj, NxParameterized::NxParameters)(paramTraits, bufStart, refCount);

		// Init vtable (everything else is already initialized)
		*(const char**)bufObj = vptr;

		return (FieldBoundaryModuleParameters*)bufObj;
	}

	virtual const char* getClassName()
	{
		return (FieldBoundaryModuleParameters::staticClassName());
	}

	virtual physx::PxU32 getVersion()
	{
		return (FieldBoundaryModuleParameters::staticVersion());
	}

	virtual physx::PxU32 getAlignment()
	{
		return (FieldBoundaryModuleParameters::ClassAlignment);
	}

	virtual const physx::PxU32* getChecksum(physx::PxU32& bits)
	{
		return (FieldBoundaryModuleParameters::staticChecksum(bits));
	}
};
#endif // NX_PARAMETERIZED_ONLY_LAYOUTS

} // namespace fieldboundary
} // namespace apex
} // namespace physx

#pragma warning(pop)

#endif
