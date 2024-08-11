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
// Created: 2013.09.23 14:34:47

#ifndef HEADER_ClothingGraphicalLodParameters_0p2_h
#define HEADER_ClothingGraphicalLodParameters_0p2_h

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

namespace ClothingGraphicalLodParameters_0p2NS
{

struct SkinClothMapB_Type;
struct SkinClothMapC_Type;
struct TetraLink_Type;

struct STRING_DynamicArray1D_Type
{
	NxParameterized::DummyStringStruct* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct U32_DynamicArray1D_Type
{
	physx::PxU32* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct SkinClothMapB_DynamicArray1D_Type
{
	SkinClothMapB_Type* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct SkinClothMapC_DynamicArray1D_Type
{
	SkinClothMapC_Type* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct TetraLink_DynamicArray1D_Type
{
	TetraLink_Type* buf;
	bool isAllocated;
	physx::PxI32 elementSize;
	physx::PxI32 arraySizes[1];
};

struct SkinClothMapC_Type
{
	physx::PxVec3 vertexBary;
	physx::PxU32 faceIndex0;
	physx::PxVec3 normalBary;
	physx::PxU32 vertexIndexPlusOffset;
};
struct SkinClothMapB_Type
{
	physx::PxVec3 vtxTetraBary;
	physx::PxU32 vertexIndexPlusOffset;
	physx::PxVec3 nrmTetraBary;
	physx::PxU32 faceIndex0;
	physx::PxU32 tetraIndex;
	physx::PxU32 submeshIndex;
};
struct TetraLink_Type
{
	physx::PxVec3 vertexBary;
	physx::PxU32 tetraIndex0;
	physx::PxVec3 normalBary;
	physx::PxU32 _dummyForAlignment;
};

struct ParametersStruct
{

	STRING_DynamicArray1D_Type platforms;
	physx::PxU32 lod;
	physx::PxU32 physicalMeshId;
	NxParameterized::Interface* renderMeshAsset;
	void* renderMeshAssetPointer;
	U32_DynamicArray1D_Type immediateClothMap;
	SkinClothMapB_DynamicArray1D_Type skinClothMapB;
	SkinClothMapC_DynamicArray1D_Type skinClothMapC;
	physx::PxF32 skinClothMapThickness;
	physx::PxF32 skinClothMapOffset;
	TetraLink_DynamicArray1D_Type tetraMap;

};

static const physx::PxU32 checksum[] = { 0x54c73376, 0x1977a7e7, 0x92780a14, 0x8c4d2b89, };

} // namespace ClothingGraphicalLodParameters_0p2NS

#ifndef NX_PARAMETERIZED_ONLY_LAYOUTS
class ClothingGraphicalLodParameters_0p2 : public NxParameterized::NxParameters, public ClothingGraphicalLodParameters_0p2NS::ParametersStruct
{
public:
	ClothingGraphicalLodParameters_0p2(NxParameterized::Traits* traits, void* buf = 0, PxI32* refCount = 0);

	virtual ~ClothingGraphicalLodParameters_0p2();

	virtual void destroy();

	static const char* staticClassName(void)
	{
		return("ClothingGraphicalLodParameters");
	}

	const char* className(void) const
	{
		return(staticClassName());
	}

	static const physx::PxU32 ClassVersion = ((physx::PxU32)0 << 16) + (physx::PxU32)2;

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
		bits = 8 * sizeof(ClothingGraphicalLodParameters_0p2NS::checksum);
		return ClothingGraphicalLodParameters_0p2NS::checksum;
	}

	static void freeParameterDefinitionTable(NxParameterized::Traits* traits);

	const physx::PxU32* checksum(physx::PxU32& bits) const
	{
		return staticChecksum(bits);
	}

	const ClothingGraphicalLodParameters_0p2NS::ParametersStruct& parameters(void) const
	{
		ClothingGraphicalLodParameters_0p2* tmpThis = const_cast<ClothingGraphicalLodParameters_0p2*>(this);
		return *(static_cast<ClothingGraphicalLodParameters_0p2NS::ParametersStruct*>(tmpThis));
	}

	ClothingGraphicalLodParameters_0p2NS::ParametersStruct& parameters(void)
	{
		return *(static_cast<ClothingGraphicalLodParameters_0p2NS::ParametersStruct*>(this));
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

class ClothingGraphicalLodParameters_0p2Factory : public NxParameterized::Factory
{
	static const char* const vptr;

public:
	virtual NxParameterized::Interface* create(NxParameterized::Traits* paramTraits)
	{
		// placement new on this class using mParameterizedTraits

		void* newPtr = paramTraits->alloc(sizeof(ClothingGraphicalLodParameters_0p2), ClothingGraphicalLodParameters_0p2::ClassAlignment);
		if (!NxParameterized::IsAligned(newPtr, ClothingGraphicalLodParameters_0p2::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class ClothingGraphicalLodParameters_0p2");
			paramTraits->free(newPtr);
			return 0;
		}

		memset(newPtr, 0, sizeof(ClothingGraphicalLodParameters_0p2)); // always initialize memory allocated to zero for default values
		return NX_PARAM_PLACEMENT_NEW(newPtr, ClothingGraphicalLodParameters_0p2)(paramTraits);
	}

	virtual NxParameterized::Interface* finish(NxParameterized::Traits* paramTraits, void* bufObj, void* bufStart, physx::PxI32* refCount)
	{
		if (!NxParameterized::IsAligned(bufObj, ClothingGraphicalLodParameters_0p2::ClassAlignment)
		        || !NxParameterized::IsAligned(bufStart, ClothingGraphicalLodParameters_0p2::ClassAlignment))
		{
			NX_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class ClothingGraphicalLodParameters_0p2");
			return 0;
		}

		// Init NxParameters-part
		// We used to call empty constructor of ClothingGraphicalLodParameters_0p2 here
		// but it may call default constructors of members and spoil the data
		NX_PARAM_PLACEMENT_NEW(bufObj, NxParameterized::NxParameters)(paramTraits, bufStart, refCount);

		// Init vtable (everything else is already initialized)
		*(const char**)bufObj = vptr;

		return (ClothingGraphicalLodParameters_0p2*)bufObj;
	}

	virtual const char* getClassName()
	{
		return (ClothingGraphicalLodParameters_0p2::staticClassName());
	}

	virtual physx::PxU32 getVersion()
	{
		return (ClothingGraphicalLodParameters_0p2::staticVersion());
	}

	virtual physx::PxU32 getAlignment()
	{
		return (ClothingGraphicalLodParameters_0p2::ClassAlignment);
	}

	virtual const physx::PxU32* getChecksum(physx::PxU32& bits)
	{
		return (ClothingGraphicalLodParameters_0p2::staticChecksum(bits));
	}
};
#endif // NX_PARAMETERIZED_ONLY_LAYOUTS

} // namespace apex
} // namespace physx

#pragma warning(pop)

#endif
