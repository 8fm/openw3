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

#include "TurbulenceFSAssetParams_0p0.h"
#include <string.h>
#include <stdlib.h>

using namespace NxParameterized;

namespace physx
{
namespace apex
{

using namespace TurbulenceFSAssetParams_0p0NS;

const char* const TurbulenceFSAssetParams_0p0Factory::vptr =
    NxParameterized::getVptr<TurbulenceFSAssetParams_0p0, TurbulenceFSAssetParams_0p0::ClassAlignment>();

const physx::PxU32 NumParamDefs = 23;
static NxParameterized::DefinitionImpl* ParamDefTable; // now allocated in buildTree [NumParamDefs];


static const size_t ParamLookupChildrenTable[] =
{
	1, 4, 7, 10, 11, 14, 15, 16, 17, 18, 19, 20, 21, 22, 2, 3, 5, 6, 8, 9, 12, 13,
};

#define TENUM(type) physx::##type
#define CHILDREN(index) &ParamLookupChildrenTable[index]
static const NxParameterized::ParamLookupNode ParamLookupTable[NumParamDefs] =
{
	{ TYPE_STRUCT, false, 0, CHILDREN(0), 14 },
	{ TYPE_STRUCT, false, (size_t)(&((ParametersStruct*)0)->gridXRange), CHILDREN(14), 2 }, // gridXRange
	{ TYPE_F32, false, (size_t)(&((rangeStructF32_Type*)0)->min), NULL, 0 }, // gridXRange.min
	{ TYPE_F32, false, (size_t)(&((rangeStructF32_Type*)0)->max), NULL, 0 }, // gridXRange.max
	{ TYPE_STRUCT, false, (size_t)(&((ParametersStruct*)0)->gridYRange), CHILDREN(16), 2 }, // gridYRange
	{ TYPE_F32, false, (size_t)(&((rangeStructF32_Type*)0)->min), NULL, 0 }, // gridYRange.min
	{ TYPE_F32, false, (size_t)(&((rangeStructF32_Type*)0)->max), NULL, 0 }, // gridYRange.max
	{ TYPE_STRUCT, false, (size_t)(&((ParametersStruct*)0)->gridZRange), CHILDREN(18), 2 }, // gridZRange
	{ TYPE_F32, false, (size_t)(&((rangeStructF32_Type*)0)->min), NULL, 0 }, // gridZRange.min
	{ TYPE_F32, false, (size_t)(&((rangeStructF32_Type*)0)->max), NULL, 0 }, // gridZRange.max
	{ TYPE_VEC3, false, (size_t)(&((ParametersStruct*)0)->gridSizeWorld), NULL, 0 }, // gridSizeWorld
	{ TYPE_STRUCT, false, (size_t)(&((ParametersStruct*)0)->updatesPerFrameRange), CHILDREN(20), 2 }, // updatesPerFrameRange
	{ TYPE_F32, false, (size_t)(&((rangeStructF32_Type*)0)->min), NULL, 0 }, // updatesPerFrameRange.min
	{ TYPE_F32, false, (size_t)(&((rangeStructF32_Type*)0)->max), NULL, 0 }, // updatesPerFrameRange.max
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->fluidVelocityMultiplier), NULL, 0 }, // fluidVelocityMultiplier
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->fluidVelocityClamp), NULL, 0 }, // fluidVelocityClamp
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->boundaryFadePercentage), NULL, 0 }, // boundaryFadePercentage
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->boundarySizePercentage), NULL, 0 }, // boundarySizePercentage
	{ TYPE_STRING, false, (size_t)(&((ParametersStruct*)0)->collisionFilterDataName), NULL, 0 }, // collisionFilterDataName
	{ TYPE_STRING, false, (size_t)(&((ParametersStruct*)0)->fieldBoundaryFilterDataName), NULL, 0 }, // fieldBoundaryFilterDataName
	{ TYPE_STRING, false, (size_t)(&((ParametersStruct*)0)->fieldSamplerFilterDataName), NULL, 0 }, // fieldSamplerFilterDataName
	{ TYPE_U32, false, (size_t)(&((ParametersStruct*)0)->maxCollidingObjects), NULL, 0 }, // maxCollidingObjects
	{ TYPE_U32, false, (size_t)(&((ParametersStruct*)0)->maxHeatSources), NULL, 0 }, // maxHeatSources
};


bool TurbulenceFSAssetParams_0p0::mBuiltFlag = false;
NxParameterized::MutexType TurbulenceFSAssetParams_0p0::mBuiltFlagMutex;

TurbulenceFSAssetParams_0p0::TurbulenceFSAssetParams_0p0(NxParameterized::Traits* traits, void* buf, PxI32* refCount) :
	NxParameters(traits, buf, refCount)
{
	//mParameterizedTraits->registerFactory(className(), &TurbulenceFSAssetParams_0p0FactoryInst);

	if (!buf) //Do not init data if it is inplace-deserialized
	{
		initDynamicArrays();
		initStrings();
		initReferences();
		initDefaults();
	}
}

TurbulenceFSAssetParams_0p0::~TurbulenceFSAssetParams_0p0()
{
	freeStrings();
	freeReferences();
	freeDynamicArrays();
}

void TurbulenceFSAssetParams_0p0::destroy()
{
	// We cache these fields here to avoid overwrite in destructor
	bool doDeallocateSelf = mDoDeallocateSelf;
	NxParameterized::Traits* traits = mParameterizedTraits;
	physx::PxI32* refCount = mRefCount;
	void* buf = mBuffer;

	this->~TurbulenceFSAssetParams_0p0();

	NxParameters::destroy(this, traits, doDeallocateSelf, refCount, buf);
}

const NxParameterized::DefinitionImpl* TurbulenceFSAssetParams_0p0::getParameterDefinitionTree(void)
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

const NxParameterized::DefinitionImpl* TurbulenceFSAssetParams_0p0::getParameterDefinitionTree(void) const
{
	TurbulenceFSAssetParams_0p0* tmpParam = const_cast<TurbulenceFSAssetParams_0p0*>(this);

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

NxParameterized::ErrorType TurbulenceFSAssetParams_0p0::getParameterHandle(const char* long_name, Handle& handle) const
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

NxParameterized::ErrorType TurbulenceFSAssetParams_0p0::getParameterHandle(const char* long_name, Handle& handle)
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

void TurbulenceFSAssetParams_0p0::getVarPtr(const Handle& handle, void*& ptr, size_t& offset) const
{
	ptr = getVarPtrHelper(&ParamLookupTable[0], const_cast<TurbulenceFSAssetParams_0p0::ParametersStruct*>(&parameters()), handle, offset);
}


/* Dynamic Handle Indices */

void TurbulenceFSAssetParams_0p0::freeParameterDefinitionTable(NxParameterized::Traits* traits)
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

void TurbulenceFSAssetParams_0p0::buildTree(void)
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

	// Initialize DefinitionImpl node: nodeIndex=1, longName="gridXRange"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[1];
		ParamDef->init("gridXRange", TYPE_STRUCT, "rangeStructF32", true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "number of grid cells in X dimension.", true);
		ParamDefTable[1].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=2, longName="gridXRange.min"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[2];
		ParamDef->init("min", TYPE_F32, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=3, longName="gridXRange.max"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[3];
		ParamDef->init("max", TYPE_F32, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=4, longName="gridYRange"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[4];
		ParamDef->init("gridYRange", TYPE_STRUCT, "rangeStructF32", true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "number of grid cells in Y dimension.", true);
		ParamDefTable[4].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=5, longName="gridYRange.min"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[5];
		ParamDef->init("min", TYPE_F32, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=6, longName="gridYRange.max"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[6];
		ParamDef->init("max", TYPE_F32, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=7, longName="gridZRange"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[7];
		ParamDef->init("gridZRange", TYPE_STRUCT, "rangeStructF32", true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "number of grid cells in Z dimension.", true);
		ParamDefTable[7].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=8, longName="gridZRange.min"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[8];
		ParamDef->init("min", TYPE_F32, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=9, longName="gridZRange.max"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[9];
		ParamDef->init("max", TYPE_F32, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=10, longName="gridSizeWorld"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[10];
		ParamDef->init("gridSizeWorld", TYPE_VEC3, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "size of the grid in world space.", true);
		ParamDefTable[10].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=11, longName="updatesPerFrameRange"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[11];
		ParamDef->init("updatesPerFrameRange", TYPE_STRUCT, "rangeStructF32", true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "the number of updates per frame.", true);
		ParamDefTable[11].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=12, longName="updatesPerFrameRange.min"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[12];
		ParamDef->init("min", TYPE_F32, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=13, longName="updatesPerFrameRange.max"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[13];
		ParamDef->init("max", TYPE_F32, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=14, longName="fluidVelocityMultiplier"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[14];
		ParamDef->init("fluidVelocityMultiplier", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "a multiplier for the velocity of the grid set by the user.", true);
		ParamDefTable[14].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=15, longName="fluidVelocityClamp"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[15];
		ParamDef->init("fluidVelocityClamp", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "a clamp for the velocity of the grid set by the user.", true);
		ParamDefTable[15].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=16, longName="boundaryFadePercentage"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[16];
		ParamDef->init("boundaryFadePercentage", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "Percentage of distance from boundary to center where fade out starts.", true);
		ParamDefTable[16].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=17, longName="boundarySizePercentage"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[17];
		ParamDef->init("boundarySizePercentage", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "Boundary size as the percentage of grid size.", true);
		ParamDefTable[17].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=18, longName="collisionFilterDataName"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[18];
		ParamDef->init("collisionFilterDataName", TYPE_STRING, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "The filter data (group/groupsMask) name for TurbulenceFS vs PhysX interaction.", true);
		ParamDefTable[18].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=19, longName="fieldBoundaryFilterDataName"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[19];
		ParamDef->init("fieldBoundaryFilterDataName", TYPE_STRING, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "The filter data name for TurbulenceFS vs Field Boundaries interaction.", true);
		ParamDefTable[19].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=20, longName="fieldSamplerFilterDataName"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[20];
		ParamDef->init("fieldSamplerFilterDataName", TYPE_STRING, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "The filter data name for TurbulenceFS vs other Field Samplers interaction.", true);
		ParamDefTable[20].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=21, longName="maxCollidingObjects"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[21];
		ParamDef->init("maxCollidingObjects", TYPE_U32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "The maximum number of colliding objects.", true);
		ParamDefTable[21].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=22, longName="maxHeatSources"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[22];
		ParamDef->init("maxHeatSources", TYPE_U32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "The maximum number of heat source.", true);
		ParamDefTable[22].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// SetChildren for: nodeIndex=0, longName=""
	{
		static Definition* Children[14];
		Children[0] = PDEF_PTR(1);
		Children[1] = PDEF_PTR(4);
		Children[2] = PDEF_PTR(7);
		Children[3] = PDEF_PTR(10);
		Children[4] = PDEF_PTR(11);
		Children[5] = PDEF_PTR(14);
		Children[6] = PDEF_PTR(15);
		Children[7] = PDEF_PTR(16);
		Children[8] = PDEF_PTR(17);
		Children[9] = PDEF_PTR(18);
		Children[10] = PDEF_PTR(19);
		Children[11] = PDEF_PTR(20);
		Children[12] = PDEF_PTR(21);
		Children[13] = PDEF_PTR(22);

		ParamDefTable[0].setChildren(Children, 14);
	}

	// SetChildren for: nodeIndex=1, longName="gridXRange"
	{
		static Definition* Children[2];
		Children[0] = PDEF_PTR(2);
		Children[1] = PDEF_PTR(3);

		ParamDefTable[1].setChildren(Children, 2);
	}

	// SetChildren for: nodeIndex=4, longName="gridYRange"
	{
		static Definition* Children[2];
		Children[0] = PDEF_PTR(5);
		Children[1] = PDEF_PTR(6);

		ParamDefTable[4].setChildren(Children, 2);
	}

	// SetChildren for: nodeIndex=7, longName="gridZRange"
	{
		static Definition* Children[2];
		Children[0] = PDEF_PTR(8);
		Children[1] = PDEF_PTR(9);

		ParamDefTable[7].setChildren(Children, 2);
	}

	// SetChildren for: nodeIndex=11, longName="updatesPerFrameRange"
	{
		static Definition* Children[2];
		Children[0] = PDEF_PTR(12);
		Children[1] = PDEF_PTR(13);

		ParamDefTable[11].setChildren(Children, 2);
	}

	mBuiltFlag = true;

}
void TurbulenceFSAssetParams_0p0::initStrings(void)
{
	collisionFilterDataName.isAllocated = true;
	collisionFilterDataName.buf = NULL;
	fieldBoundaryFilterDataName.isAllocated = true;
	fieldBoundaryFilterDataName.buf = NULL;
	fieldSamplerFilterDataName.isAllocated = true;
	fieldSamplerFilterDataName.buf = NULL;
}

void TurbulenceFSAssetParams_0p0::initDynamicArrays(void)
{
}

void TurbulenceFSAssetParams_0p0::initDefaults(void)
{

	freeStrings();
	freeReferences();
	freeDynamicArrays();

	gridXRange.min = 32.0f;
	gridXRange.max = 64.0f;


	gridYRange.min = 32.0f;
	gridYRange.max = 64.0f;


	gridZRange.min = 32.0f;
	gridZRange.max = 64.0f;

	gridSizeWorld = physx::PxVec3(init(12, 12, 12));

	updatesPerFrameRange.min = 0.0f;
	updatesPerFrameRange.max = 1.0f;

	fluidVelocityMultiplier = physx::PxF32(1);
	fluidVelocityClamp = physx::PxF32(1000000);
	boundaryFadePercentage = physx::PxF32(0.1);
	boundarySizePercentage = physx::PxF32(1);
	maxCollidingObjects = physx::PxU32(32);
	maxHeatSources = physx::PxU32(8);

	initDynamicArrays();
	initStrings();
	initReferences();
}

void TurbulenceFSAssetParams_0p0::initReferences(void)
{
}

void TurbulenceFSAssetParams_0p0::freeDynamicArrays(void)
{
}

void TurbulenceFSAssetParams_0p0::freeStrings(void)
{

	if (collisionFilterDataName.isAllocated && collisionFilterDataName.buf)
	{
		mParameterizedTraits->strfree((char*)collisionFilterDataName.buf);
	}

	if (fieldBoundaryFilterDataName.isAllocated && fieldBoundaryFilterDataName.buf)
	{
		mParameterizedTraits->strfree((char*)fieldBoundaryFilterDataName.buf);
	}

	if (fieldSamplerFilterDataName.isAllocated && fieldSamplerFilterDataName.buf)
	{
		mParameterizedTraits->strfree((char*)fieldSamplerFilterDataName.buf);
	}
}

void TurbulenceFSAssetParams_0p0::freeReferences(void)
{
}

} // namespace apex
} // namespace physx
