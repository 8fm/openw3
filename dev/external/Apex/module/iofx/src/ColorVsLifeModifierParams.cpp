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

#include "ColorVsLifeModifierParams.h"
#include <string.h>
#include <stdlib.h>

using namespace NxParameterized;

namespace physx
{
namespace apex
{
namespace iofx
{

using namespace ColorVsLifeModifierParamsNS;

const char* const ColorVsLifeModifierParamsFactory::vptr =
    NxParameterized::getVptr<ColorVsLifeModifierParams, ColorVsLifeModifierParams::ClassAlignment>();

const physx::PxU32 NumParamDefs = 6;
static NxParameterized::DefinitionImpl* ParamDefTable; // now allocated in buildTree [NumParamDefs];


static const size_t ParamLookupChildrenTable[] =
{
	1, 2, 3, 4, 5,
};

#define TENUM(type) physx::##type
#define CHILDREN(index) &ParamLookupChildrenTable[index]
static const NxParameterized::ParamLookupNode ParamLookupTable[NumParamDefs] =
{
	{ TYPE_STRUCT, false, 0, CHILDREN(0), 2 },
	{ TYPE_ENUM, false, (size_t)(&((ParametersStruct*)0)->colorChannel), NULL, 0 }, // colorChannel
	{ TYPE_ARRAY, true, (size_t)(&((ParametersStruct*)0)->controlPoints), CHILDREN(2), 1 }, // controlPoints
	{ TYPE_STRUCT, false, 1 * sizeof(vec2_Type), CHILDREN(3), 2 }, // controlPoints[]
	{ TYPE_F32, false, (size_t)(&((vec2_Type*)0)->x), NULL, 0 }, // controlPoints[].x
	{ TYPE_F32, false, (size_t)(&((vec2_Type*)0)->y), NULL, 0 }, // controlPoints[].y
};


bool ColorVsLifeModifierParams::mBuiltFlag = false;
NxParameterized::MutexType ColorVsLifeModifierParams::mBuiltFlagMutex;

ColorVsLifeModifierParams::ColorVsLifeModifierParams(NxParameterized::Traits* traits, void* buf, PxI32* refCount) :
	NxParameters(traits, buf, refCount)
{
	//mParameterizedTraits->registerFactory(className(), &ColorVsLifeModifierParamsFactoryInst);

	if (!buf) //Do not init data if it is inplace-deserialized
	{
		initDynamicArrays();
		initStrings();
		initReferences();
		initDefaults();
	}
}

ColorVsLifeModifierParams::~ColorVsLifeModifierParams()
{
	freeStrings();
	freeReferences();
	freeDynamicArrays();
}

void ColorVsLifeModifierParams::destroy()
{
	// We cache these fields here to avoid overwrite in destructor
	bool doDeallocateSelf = mDoDeallocateSelf;
	NxParameterized::Traits* traits = mParameterizedTraits;
	physx::PxI32* refCount = mRefCount;
	void* buf = mBuffer;

	this->~ColorVsLifeModifierParams();

	NxParameters::destroy(this, traits, doDeallocateSelf, refCount, buf);
}

const NxParameterized::DefinitionImpl* ColorVsLifeModifierParams::getParameterDefinitionTree(void)
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

const NxParameterized::DefinitionImpl* ColorVsLifeModifierParams::getParameterDefinitionTree(void) const
{
	ColorVsLifeModifierParams* tmpParam = const_cast<ColorVsLifeModifierParams*>(this);

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

NxParameterized::ErrorType ColorVsLifeModifierParams::getParameterHandle(const char* long_name, Handle& handle) const
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

NxParameterized::ErrorType ColorVsLifeModifierParams::getParameterHandle(const char* long_name, Handle& handle)
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

void ColorVsLifeModifierParams::getVarPtr(const Handle& handle, void*& ptr, size_t& offset) const
{
	ptr = getVarPtrHelper(&ParamLookupTable[0], const_cast<ColorVsLifeModifierParams::ParametersStruct*>(&parameters()), handle, offset);
}


/* Dynamic Handle Indices */

void ColorVsLifeModifierParams::freeParameterDefinitionTable(NxParameterized::Traits* traits)
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

void ColorVsLifeModifierParams::buildTree(void)
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

	// Initialize DefinitionImpl node: nodeIndex=1, longName="colorChannel"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[1];
		ParamDef->init("colorChannel", TYPE_ENUM, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("defaultValue", "alpha", true);
		ParamDefTable[1].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("defaultValue", "alpha", true);
		HintTable[1].init("shortDescription", "Color channel", true);
		ParamDefTable[1].setHints((const NxParameterized::Hint**)HintPtrTable, 2);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */

		static const char* const EnumVals[] = { "red", "green", "blue", "alpha" };
		ParamDefTable[1].setEnumVals((const char**)EnumVals, 4);




	}

	// Initialize DefinitionImpl node: nodeIndex=2, longName="controlPoints"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[2];
		ParamDef->init("controlPoints", TYPE_ARRAY, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[3];
		static Hint* HintPtrTable[3] = { &HintTable[0], &HintTable[1], &HintTable[2], };
		HintTable[0].init("editorCurve", physx::PxU64(1), true);
		HintTable[1].init("xAxisLabel", "Life Time", true);
		HintTable[2].init("yAxisLabel", "Color", true);
		ParamDefTable[2].setHints((const NxParameterized::Hint**)HintPtrTable, 3);

#else

		static HintImpl HintTable[5];
		static Hint* HintPtrTable[5] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], &HintTable[4], };
		HintTable[0].init("editorCurve", physx::PxU64(1), true);
		HintTable[1].init("longDescription", "controlPoints is a sorted list of control points for a curve. Currently, the curve is a lame\nlirp'd curve. We could add support for other curvetypes in the future, either bezier curves,\nsplines, etc.\n", true);
		HintTable[2].init("shortDescription", "Control points for a curve", true);
		HintTable[3].init("xAxisLabel", "Life Time", true);
		HintTable[4].init("yAxisLabel", "Color", true);
		ParamDefTable[2].setHints((const NxParameterized::Hint**)HintPtrTable, 5);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */




		ParamDef->setArraySize(-1);
	}

	// Initialize DefinitionImpl node: nodeIndex=3, longName="controlPoints[]"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[3];
		ParamDef->init("controlPoints", TYPE_STRUCT, "vec2", true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[3];
		static Hint* HintPtrTable[3] = { &HintTable[0], &HintTable[1], &HintTable[2], };
		HintTable[0].init("editorCurve", physx::PxU64(1), true);
		HintTable[1].init("xAxisLabel", "Life Time", true);
		HintTable[2].init("yAxisLabel", "Color", true);
		ParamDefTable[3].setHints((const NxParameterized::Hint**)HintPtrTable, 3);

#else

		static HintImpl HintTable[5];
		static Hint* HintPtrTable[5] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], &HintTable[4], };
		HintTable[0].init("editorCurve", physx::PxU64(1), true);
		HintTable[1].init("longDescription", "controlPoints is a sorted list of control points for a curve. Currently, the curve is a lame\nlirp'd curve. We could add support for other curvetypes in the future, either bezier curves,\nsplines, etc.\n", true);
		HintTable[2].init("shortDescription", "Control points for a curve", true);
		HintTable[3].init("xAxisLabel", "Life Time", true);
		HintTable[4].init("yAxisLabel", "Color", true);
		ParamDefTable[3].setHints((const NxParameterized::Hint**)HintPtrTable, 5);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=4, longName="controlPoints[].x"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[4];
		ParamDef->init("x", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("max", physx::PxU64(1), true);
		HintTable[1].init("min", physx::PxU64(0), true);
		ParamDefTable[4].setHints((const NxParameterized::Hint**)HintPtrTable, 2);

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("max", physx::PxU64(1), true);
		HintTable[1].init("min", physx::PxU64(0), true);
		ParamDefTable[4].setHints((const NxParameterized::Hint**)HintPtrTable, 2);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=5, longName="controlPoints[].y"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[5];
		ParamDef->init("y", TYPE_F32, NULL, true);






	}

	// SetChildren for: nodeIndex=0, longName=""
	{
		static Definition* Children[2];
		Children[0] = PDEF_PTR(1);
		Children[1] = PDEF_PTR(2);

		ParamDefTable[0].setChildren(Children, 2);
	}

	// SetChildren for: nodeIndex=2, longName="controlPoints"
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(3);

		ParamDefTable[2].setChildren(Children, 1);
	}

	// SetChildren for: nodeIndex=3, longName="controlPoints[]"
	{
		static Definition* Children[2];
		Children[0] = PDEF_PTR(4);
		Children[1] = PDEF_PTR(5);

		ParamDefTable[3].setChildren(Children, 2);
	}

	mBuiltFlag = true;

}
void ColorVsLifeModifierParams::initStrings(void)
{
}

void ColorVsLifeModifierParams::initDynamicArrays(void)
{
	controlPoints.buf = NULL;
	controlPoints.isAllocated = true;
	controlPoints.elementSize = sizeof(vec2_Type);
	controlPoints.arraySizes[0] = 0;
}

void ColorVsLifeModifierParams::initDefaults(void)
{

	freeStrings();
	freeReferences();
	freeDynamicArrays();
	colorChannel = (const char*)"alpha";

	initDynamicArrays();
	initStrings();
	initReferences();
}

void ColorVsLifeModifierParams::initReferences(void)
{
}

void ColorVsLifeModifierParams::freeDynamicArrays(void)
{
	if (controlPoints.isAllocated && controlPoints.buf)
	{
		mParameterizedTraits->free(controlPoints.buf);
	}
}

void ColorVsLifeModifierParams::freeStrings(void)
{
}

void ColorVsLifeModifierParams::freeReferences(void)
{
}

} // namespace iofx
} // namespace apex
} // namespace physx
