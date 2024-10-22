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

#include "ColorVsVelocityCompositeModifierParams.h"
#include <string.h>
#include <stdlib.h>

using namespace NxParameterized;

namespace physx
{
namespace apex
{
namespace iofx
{

using namespace ColorVsVelocityCompositeModifierParamsNS;

const char* const ColorVsVelocityCompositeModifierParamsFactory::vptr =
    NxParameterized::getVptr<ColorVsVelocityCompositeModifierParams, ColorVsVelocityCompositeModifierParams::ClassAlignment>();

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
	{ TYPE_STRUCT, false, 0, CHILDREN(0), 3 },
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->velocity0), NULL, 0 }, // velocity0
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->velocity1), NULL, 0 }, // velocity1
	{ TYPE_ARRAY, true, (size_t)(&((ParametersStruct*)0)->controlPoints), CHILDREN(3), 1 }, // controlPoints
	{ TYPE_STRUCT, false, 1 * sizeof(colorVelocityStruct_Type), CHILDREN(4), 2 }, // controlPoints[]
	{ TYPE_F32, false, (size_t)(&((colorVelocityStruct_Type*)0)->velocity), NULL, 0 }, // controlPoints[].velocity
	{ TYPE_VEC4, false, (size_t)(&((colorVelocityStruct_Type*)0)->color), NULL, 0 }, // controlPoints[].color
};


bool ColorVsVelocityCompositeModifierParams::mBuiltFlag = false;
NxParameterized::MutexType ColorVsVelocityCompositeModifierParams::mBuiltFlagMutex;

ColorVsVelocityCompositeModifierParams::ColorVsVelocityCompositeModifierParams(NxParameterized::Traits* traits, void* buf, PxI32* refCount) :
	NxParameters(traits, buf, refCount)
{
	//mParameterizedTraits->registerFactory(className(), &ColorVsVelocityCompositeModifierParamsFactoryInst);

	if (!buf) //Do not init data if it is inplace-deserialized
	{
		initDynamicArrays();
		initStrings();
		initReferences();
		initDefaults();
	}
}

ColorVsVelocityCompositeModifierParams::~ColorVsVelocityCompositeModifierParams()
{
	freeStrings();
	freeReferences();
	freeDynamicArrays();
}

void ColorVsVelocityCompositeModifierParams::destroy()
{
	// We cache these fields here to avoid overwrite in destructor
	bool doDeallocateSelf = mDoDeallocateSelf;
	NxParameterized::Traits* traits = mParameterizedTraits;
	physx::PxI32* refCount = mRefCount;
	void* buf = mBuffer;

	this->~ColorVsVelocityCompositeModifierParams();

	NxParameters::destroy(this, traits, doDeallocateSelf, refCount, buf);
}

const NxParameterized::DefinitionImpl* ColorVsVelocityCompositeModifierParams::getParameterDefinitionTree(void)
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

const NxParameterized::DefinitionImpl* ColorVsVelocityCompositeModifierParams::getParameterDefinitionTree(void) const
{
	ColorVsVelocityCompositeModifierParams* tmpParam = const_cast<ColorVsVelocityCompositeModifierParams*>(this);

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

NxParameterized::ErrorType ColorVsVelocityCompositeModifierParams::getParameterHandle(const char* long_name, Handle& handle) const
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

NxParameterized::ErrorType ColorVsVelocityCompositeModifierParams::getParameterHandle(const char* long_name, Handle& handle)
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

void ColorVsVelocityCompositeModifierParams::getVarPtr(const Handle& handle, void*& ptr, size_t& offset) const
{
	ptr = getVarPtrHelper(&ParamLookupTable[0], const_cast<ColorVsVelocityCompositeModifierParams::ParametersStruct*>(&parameters()), handle, offset);
}


/* Dynamic Handle Indices */

void ColorVsVelocityCompositeModifierParams::freeParameterDefinitionTable(NxParameterized::Traits* traits)
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

void ColorVsVelocityCompositeModifierParams::buildTree(void)
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

	// Initialize DefinitionImpl node: nodeIndex=1, longName="velocity0"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[1];
		ParamDef->init("velocity0", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("gameScale", "true", true);
		ParamDefTable[1].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("gameScale", "true", true);
		ParamDefTable[1].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=2, longName="velocity1"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[2];
		ParamDef->init("velocity1", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("gameScale", "true", true);
		ParamDefTable[2].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("gameScale", "true", true);
		ParamDefTable[2].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=3, longName="controlPoints"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[3];
		ParamDef->init("controlPoints", TYPE_ARRAY, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[6];
		static Hint* HintPtrTable[6] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], &HintTable[4], &HintTable[5], };
		HintTable[0].init("CURVE_X_SCALE", physx::PxU64(1), true);
		HintTable[1].init("CURVE_Y_SCALE", physx::PxU64(2), true);
		HintTable[2].init("ColorWheel", "true", true);
		HintTable[3].init("editorCurve", physx::PxU64(1), true);
		HintTable[4].init("xAxisLabel", "Velocity", true);
		HintTable[5].init("yAxisLabel", "Alpha + Color", true);
		ParamDefTable[3].setHints((const NxParameterized::Hint**)HintPtrTable, 6);

#else

		static HintImpl HintTable[7];
		static Hint* HintPtrTable[7] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], &HintTable[4], &HintTable[5], &HintTable[6], };
		HintTable[0].init("CURVE_X_SCALE", physx::PxU64(1), true);
		HintTable[1].init("CURVE_Y_SCALE", physx::PxU64(2), true);
		HintTable[2].init("ColorWheel", "true", true);
		HintTable[3].init("editorCurve", physx::PxU64(1), true);
		HintTable[4].init("shortDescription", "Control points for a composite color vs velocity curve", true);
		HintTable[5].init("xAxisLabel", "Velocity", true);
		HintTable[6].init("yAxisLabel", "Alpha + Color", true);
		ParamDefTable[3].setHints((const NxParameterized::Hint**)HintPtrTable, 7);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */




		ParamDef->setArraySize(-1);
	}

	// Initialize DefinitionImpl node: nodeIndex=4, longName="controlPoints[]"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[4];
		ParamDef->init("controlPoints", TYPE_STRUCT, "colorVelocityStruct", true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[6];
		static Hint* HintPtrTable[6] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], &HintTable[4], &HintTable[5], };
		HintTable[0].init("CURVE_X_SCALE", physx::PxU64(1), true);
		HintTable[1].init("CURVE_Y_SCALE", physx::PxU64(2), true);
		HintTable[2].init("ColorWheel", "true", true);
		HintTable[3].init("editorCurve", physx::PxU64(1), true);
		HintTable[4].init("xAxisLabel", "Velocity", true);
		HintTable[5].init("yAxisLabel", "Alpha + Color", true);
		ParamDefTable[4].setHints((const NxParameterized::Hint**)HintPtrTable, 6);

#else

		static HintImpl HintTable[7];
		static Hint* HintPtrTable[7] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], &HintTable[4], &HintTable[5], &HintTable[6], };
		HintTable[0].init("CURVE_X_SCALE", physx::PxU64(1), true);
		HintTable[1].init("CURVE_Y_SCALE", physx::PxU64(2), true);
		HintTable[2].init("ColorWheel", "true", true);
		HintTable[3].init("editorCurve", physx::PxU64(1), true);
		HintTable[4].init("shortDescription", "Control points for a composite color vs velocity curve", true);
		HintTable[5].init("xAxisLabel", "Velocity", true);
		HintTable[6].init("yAxisLabel", "Alpha + Color", true);
		ParamDefTable[4].setHints((const NxParameterized::Hint**)HintPtrTable, 7);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=5, longName="controlPoints[].velocity"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[5];
		ParamDef->init("velocity", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("max", physx::PxU64(1), true);
		HintTable[1].init("min", physx::PxU64(0), true);
		ParamDefTable[5].setHints((const NxParameterized::Hint**)HintPtrTable, 2);

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("max", physx::PxU64(1), true);
		HintTable[1].init("min", physx::PxU64(0), true);
		ParamDefTable[5].setHints((const NxParameterized::Hint**)HintPtrTable, 2);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=6, longName="controlPoints[].color"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[6];
		ParamDef->init("color", TYPE_VEC4, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[8];
		static Hint* HintPtrTable[8] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], &HintTable[4], &HintTable[5], &HintTable[6], &HintTable[7], };
		HintTable[0].init("COLOR", physx::PxU64(1), true);
		HintTable[1].init("max_w", physx::PxU64(8), true);
		HintTable[2].init("max_x", physx::PxU64(8), true);
		HintTable[3].init("max_y", physx::PxU64(8), true);
		HintTable[4].init("min_w", physx::PxU64(0), true);
		HintTable[5].init("min_x", physx::PxU64(0), true);
		HintTable[6].init("min_y", physx::PxU64(0), true);
		HintTable[7].init("min_z", physx::PxU64(0), true);
		ParamDefTable[6].setHints((const NxParameterized::Hint**)HintPtrTable, 8);

#else

		static HintImpl HintTable[9];
		static Hint* HintPtrTable[9] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], &HintTable[4], &HintTable[5], &HintTable[6], &HintTable[7], &HintTable[8], };
		HintTable[0].init("COLOR", physx::PxU64(1), true);
		HintTable[1].init("max_w", physx::PxU64(8), true);
		HintTable[2].init("max_x", physx::PxU64(8), true);
		HintTable[3].init("max_y", physx::PxU64(8), true);
		HintTable[4].init("min_w", physx::PxU64(0), true);
		HintTable[5].init("min_x", physx::PxU64(0), true);
		HintTable[6].init("min_y", physx::PxU64(0), true);
		HintTable[7].init("min_z", physx::PxU64(0), true);
		HintTable[8].init("shortDescription", "Color is formatted x=R, y=G, z=B, w=A", true);
		ParamDefTable[6].setHints((const NxParameterized::Hint**)HintPtrTable, 9);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// SetChildren for: nodeIndex=0, longName=""
	{
		static Definition* Children[3];
		Children[0] = PDEF_PTR(1);
		Children[1] = PDEF_PTR(2);
		Children[2] = PDEF_PTR(3);

		ParamDefTable[0].setChildren(Children, 3);
	}

	// SetChildren for: nodeIndex=3, longName="controlPoints"
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(4);

		ParamDefTable[3].setChildren(Children, 1);
	}

	// SetChildren for: nodeIndex=4, longName="controlPoints[]"
	{
		static Definition* Children[2];
		Children[0] = PDEF_PTR(5);
		Children[1] = PDEF_PTR(6);

		ParamDefTable[4].setChildren(Children, 2);
	}

	mBuiltFlag = true;

}
void ColorVsVelocityCompositeModifierParams::initStrings(void)
{
}

void ColorVsVelocityCompositeModifierParams::initDynamicArrays(void)
{
	controlPoints.buf = NULL;
	controlPoints.isAllocated = true;
	controlPoints.elementSize = sizeof(colorVelocityStruct_Type);
	controlPoints.arraySizes[0] = 0;
}

void ColorVsVelocityCompositeModifierParams::initDefaults(void)
{

	freeStrings();
	freeReferences();
	freeDynamicArrays();
	velocity0 = physx::PxF32(0);
	velocity1 = physx::PxF32(1);

	initDynamicArrays();
	initStrings();
	initReferences();
}

void ColorVsVelocityCompositeModifierParams::initReferences(void)
{
}

void ColorVsVelocityCompositeModifierParams::freeDynamicArrays(void)
{
	if (controlPoints.isAllocated && controlPoints.buf)
	{
		mParameterizedTraits->free(controlPoints.buf);
	}
}

void ColorVsVelocityCompositeModifierParams::freeStrings(void)
{
}

void ColorVsVelocityCompositeModifierParams::freeReferences(void)
{
}

} // namespace iofx
} // namespace apex
} // namespace physx
