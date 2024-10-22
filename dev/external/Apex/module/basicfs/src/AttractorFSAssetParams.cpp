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
// Created: 2013.10.01 14:57:14

#include "AttractorFSAssetParams.h"
#include <string.h>
#include <stdlib.h>

using namespace NxParameterized;

namespace physx
{
namespace apex
{
namespace basicfs
{

using namespace AttractorFSAssetParamsNS;

const char* const AttractorFSAssetParamsFactory::vptr =
    NxParameterized::getVptr<AttractorFSAssetParams, AttractorFSAssetParams::ClassAlignment>();

const physx::PxU32 NumParamDefs = 7;
static NxParameterized::DefinitionImpl* ParamDefTable; // now allocated in buildTree [NumParamDefs];


static const size_t ParamLookupChildrenTable[] =
{
	1, 2, 3, 4, 5, 6,
};

#define TENUM(type) physx::##type
#define CHILDREN(index) &ParamLookupChildrenTable[index]
static const NxParameterized::ParamLookupNode ParamLookupTable[NumParamDefs] =
{
	{ TYPE_STRUCT, false, 0, CHILDREN(0), 6 },
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->boundaryFadePercentage), NULL, 0 }, // boundaryFadePercentage
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->radius), NULL, 0 }, // radius
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->constFieldStrength), NULL, 0 }, // constFieldStrength
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->variableFieldStrength), NULL, 0 }, // variableFieldStrength
	{ TYPE_STRING, false, (size_t)(&((ParametersStruct*)0)->fieldBoundaryFilterDataName), NULL, 0 }, // fieldBoundaryFilterDataName
	{ TYPE_STRING, false, (size_t)(&((ParametersStruct*)0)->fieldSamplerFilterDataName), NULL, 0 }, // fieldSamplerFilterDataName
};


bool AttractorFSAssetParams::mBuiltFlag = false;
NxParameterized::MutexType AttractorFSAssetParams::mBuiltFlagMutex;

AttractorFSAssetParams::AttractorFSAssetParams(NxParameterized::Traits* traits, void* buf, PxI32* refCount) :
	NxParameters(traits, buf, refCount)
{
	//mParameterizedTraits->registerFactory(className(), &AttractorFSAssetParamsFactoryInst);

	if (!buf) //Do not init data if it is inplace-deserialized
	{
		initDynamicArrays();
		initStrings();
		initReferences();
		initDefaults();
	}
}

AttractorFSAssetParams::~AttractorFSAssetParams()
{
	freeStrings();
	freeReferences();
	freeDynamicArrays();
}

void AttractorFSAssetParams::destroy()
{
	// We cache these fields here to avoid overwrite in destructor
	bool doDeallocateSelf = mDoDeallocateSelf;
	NxParameterized::Traits* traits = mParameterizedTraits;
	physx::PxI32* refCount = mRefCount;
	void* buf = mBuffer;

	this->~AttractorFSAssetParams();

	NxParameters::destroy(this, traits, doDeallocateSelf, refCount, buf);
}

const NxParameterized::DefinitionImpl* AttractorFSAssetParams::getParameterDefinitionTree(void)
{
	if (!mBuiltFlag) // Double-checked lock
	{
		NxParameterized::MutexType::ScopedLock lock(mBuiltFlagMutex);
		if (!mBuiltFlag)
		{
			buildTree();
		}
	}

	return(&ParamDefTable[0]);
}

const NxParameterized::DefinitionImpl* AttractorFSAssetParams::getParameterDefinitionTree(void) const
{
	AttractorFSAssetParams* tmpParam = const_cast<AttractorFSAssetParams*>(this);

	if (!mBuiltFlag) // Double-checked lock
	{
		NxParameterized::MutexType::ScopedLock lock(mBuiltFlagMutex);
		if (!mBuiltFlag)
		{
			tmpParam->buildTree();
		}
	}

	return(&ParamDefTable[0]);
}

NxParameterized::ErrorType AttractorFSAssetParams::getParameterHandle(const char* long_name, Handle& handle) const
{
	ErrorType Ret = NxParameters::getParameterHandle(long_name, handle);
	if (Ret != ERROR_NONE)
	{
		return(Ret);
	}

	size_t offset;
	void* ptr;

	getVarPtr(handle, ptr, offset);

	if (ptr == NULL)
	{
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	return(ERROR_NONE);
}

NxParameterized::ErrorType AttractorFSAssetParams::getParameterHandle(const char* long_name, Handle& handle)
{
	ErrorType Ret = NxParameters::getParameterHandle(long_name, handle);
	if (Ret != ERROR_NONE)
	{
		return(Ret);
	}

	size_t offset;
	void* ptr;

	getVarPtr(handle, ptr, offset);

	if (ptr == NULL)
	{
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	return(ERROR_NONE);
}

void AttractorFSAssetParams::getVarPtr(const Handle& handle, void*& ptr, size_t& offset) const
{
	ptr = getVarPtrHelper(&ParamLookupTable[0], const_cast<AttractorFSAssetParams::ParametersStruct*>(&parameters()), handle, offset);
}


/* Dynamic Handle Indices */

void AttractorFSAssetParams::freeParameterDefinitionTable(NxParameterized::Traits* traits)
{
	if (!traits)
	{
		return;
	}

	if (!mBuiltFlag) // Double-checked lock
	{
		return;
	}

	NxParameterized::MutexType::ScopedLock lock(mBuiltFlagMutex);

	if (!mBuiltFlag)
	{
		return;
	}

	for (physx::PxU32 i = 0; i < NumParamDefs; ++i)
	{
		ParamDefTable[i].~DefinitionImpl();
	}

	traits->free(ParamDefTable);

	mBuiltFlag = false;
}

#define PDEF_PTR(index) (&ParamDefTable[index])

void AttractorFSAssetParams::buildTree(void)
{

	physx::PxU32 allocSize = sizeof(NxParameterized::DefinitionImpl) * NumParamDefs;
	ParamDefTable = (NxParameterized::DefinitionImpl*)(mParameterizedTraits->alloc(allocSize));
	memset(ParamDefTable, 0, allocSize);

	for (physx::PxU32 i = 0; i < NumParamDefs; ++i)
	{
		NX_PARAM_PLACEMENT_NEW(ParamDefTable + i, NxParameterized::DefinitionImpl)(*mParameterizedTraits);
	}

	// Initialize DefinitionImpl node: nodeIndex=0, longName=""
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[0];
		ParamDef->init("", TYPE_STRUCT, "STRUCT", true);






	}

	// Initialize DefinitionImpl node: nodeIndex=1, longName="boundaryFadePercentage"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[1];
		ParamDef->init("boundaryFadePercentage", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "Percentage of distance from boundary to center where fade out starts.", true);
		ParamDefTable[1].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=2, longName="radius"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[2];
		ParamDef->init("radius", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "Radius of the sphere where attracting force is acting.", true);
		ParamDefTable[2].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=3, longName="constFieldStrength"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[3];
		ParamDef->init("constFieldStrength", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "Coefficient that sets strength of the constant part of attracting force.", true);
		ParamDefTable[3].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=4, longName="variableFieldStrength"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[4];
		ParamDef->init("variableFieldStrength", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "Variable part of the force is proportional to this coefficient and inverse proportional to distance from the senter of attractor.", true);
		HintTable[1].init("shortDescription", "Coefficient that sets strength of the variable part of attracting force.", true);
		ParamDefTable[4].setHints((const NxParameterized::Hint**)HintPtrTable, 2);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=5, longName="fieldBoundaryFilterDataName"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[5];
		ParamDef->init("fieldBoundaryFilterDataName", TYPE_STRING, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "The filter data name for AttractorFS vs Field Boundaries interaction.", true);
		ParamDefTable[5].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=6, longName="fieldSamplerFilterDataName"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[6];
		ParamDef->init("fieldSamplerFilterDataName", TYPE_STRING, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "The filter data name for AttractorFS vs other Field Samplers interaction.", true);
		ParamDefTable[6].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// SetChildren for: nodeIndex=0, longName=""
	{
		static Definition* Children[6];
		Children[0] = PDEF_PTR(1);
		Children[1] = PDEF_PTR(2);
		Children[2] = PDEF_PTR(3);
		Children[3] = PDEF_PTR(4);
		Children[4] = PDEF_PTR(5);
		Children[5] = PDEF_PTR(6);

		ParamDefTable[0].setChildren(Children, 6);
	}

	mBuiltFlag = true;

}
void AttractorFSAssetParams::initStrings(void)
{
	fieldBoundaryFilterDataName.isAllocated = true;
	fieldBoundaryFilterDataName.buf = NULL;
	fieldSamplerFilterDataName.isAllocated = true;
	fieldSamplerFilterDataName.buf = NULL;
}

void AttractorFSAssetParams::initDynamicArrays(void)
{
}

void AttractorFSAssetParams::initDefaults(void)
{

	freeStrings();
	freeReferences();
	freeDynamicArrays();
	boundaryFadePercentage = physx::PxF32(0.1);
	radius = physx::PxF32(0);
	constFieldStrength = physx::PxF32(0);
	variableFieldStrength = physx::PxF32(0);

	initDynamicArrays();
	initStrings();
	initReferences();
}

void AttractorFSAssetParams::initReferences(void)
{
}

void AttractorFSAssetParams::freeDynamicArrays(void)
{
}

void AttractorFSAssetParams::freeStrings(void)
{

	if (fieldBoundaryFilterDataName.isAllocated && fieldBoundaryFilterDataName.buf)
	{
		mParameterizedTraits->strfree((char*)fieldBoundaryFilterDataName.buf);
	}

	if (fieldSamplerFilterDataName.isAllocated && fieldSamplerFilterDataName.buf)
	{
		mParameterizedTraits->strfree((char*)fieldSamplerFilterDataName.buf);
	}
}

void AttractorFSAssetParams::freeReferences(void)
{
}

} // namespace basicfs
} // namespace apex
} // namespace physx
