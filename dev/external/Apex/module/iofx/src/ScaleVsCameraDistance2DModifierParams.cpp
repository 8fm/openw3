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

#include "ScaleVsCameraDistance2DModifierParams.h"
#include <string.h>
#include <stdlib.h>

using namespace NxParameterized;

namespace physx
{
namespace apex
{
namespace iofx
{

using namespace ScaleVsCameraDistance2DModifierParamsNS;

const char* const ScaleVsCameraDistance2DModifierParamsFactory::vptr =
    NxParameterized::getVptr<ScaleVsCameraDistance2DModifierParams, ScaleVsCameraDistance2DModifierParams::ClassAlignment>();

const physx::PxU32 NumParamDefs = 5;
static NxParameterized::DefinitionImpl* ParamDefTable; // now allocated in buildTree [NumParamDefs];


static const size_t ParamLookupChildrenTable[] =
{
	1, 2, 3, 4,
};

#define TENUM(type) physx::##type
#define CHILDREN(index) &ParamLookupChildrenTable[index]
static const NxParameterized::ParamLookupNode ParamLookupTable[NumParamDefs] =
{
	{ TYPE_STRUCT, false, 0, CHILDREN(0), 1 },
	{ TYPE_ARRAY, true, (size_t)(&((ParametersStruct*)0)->controlPoints), CHILDREN(1), 1 }, // controlPoints
	{ TYPE_STRUCT, false, 1 * sizeof(scaleCameraDistanceStruct_Type), CHILDREN(2), 2 }, // controlPoints[]
	{ TYPE_F32, false, (size_t)(&((scaleCameraDistanceStruct_Type*)0)->cameraDistance), NULL, 0 }, // controlPoints[].cameraDistance
	{ TYPE_VEC2, false, (size_t)(&((scaleCameraDistanceStruct_Type*)0)->scale), NULL, 0 }, // controlPoints[].scale
};


bool ScaleVsCameraDistance2DModifierParams::mBuiltFlag = false;
NxParameterized::MutexType ScaleVsCameraDistance2DModifierParams::mBuiltFlagMutex;

ScaleVsCameraDistance2DModifierParams::ScaleVsCameraDistance2DModifierParams(NxParameterized::Traits* traits, void* buf, PxI32* refCount) :
	NxParameters(traits, buf, refCount)
{
	//mParameterizedTraits->registerFactory(className(), &ScaleVsCameraDistance2DModifierParamsFactoryInst);

	if (!buf) //Do not init data if it is inplace-deserialized
	{
		initDynamicArrays();
		initStrings();
		initReferences();
		initDefaults();
	}
}

ScaleVsCameraDistance2DModifierParams::~ScaleVsCameraDistance2DModifierParams()
{
	freeStrings();
	freeReferences();
	freeDynamicArrays();
}

void ScaleVsCameraDistance2DModifierParams::destroy()
{
	// We cache these fields here to avoid overwrite in destructor
	bool doDeallocateSelf = mDoDeallocateSelf;
	NxParameterized::Traits* traits = mParameterizedTraits;
	physx::PxI32* refCount = mRefCount;
	void* buf = mBuffer;

	this->~ScaleVsCameraDistance2DModifierParams();

	NxParameters::destroy(this, traits, doDeallocateSelf, refCount, buf);
}

const NxParameterized::DefinitionImpl* ScaleVsCameraDistance2DModifierParams::getParameterDefinitionTree(void)
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

const NxParameterized::DefinitionImpl* ScaleVsCameraDistance2DModifierParams::getParameterDefinitionTree(void) const
{
	ScaleVsCameraDistance2DModifierParams* tmpParam = const_cast<ScaleVsCameraDistance2DModifierParams*>(this);

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

NxParameterized::ErrorType ScaleVsCameraDistance2DModifierParams::getParameterHandle(const char* long_name, Handle& handle) const
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

NxParameterized::ErrorType ScaleVsCameraDistance2DModifierParams::getParameterHandle(const char* long_name, Handle& handle)
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

void ScaleVsCameraDistance2DModifierParams::getVarPtr(const Handle& handle, void*& ptr, size_t& offset) const
{
	ptr = getVarPtrHelper(&ParamLookupTable[0], const_cast<ScaleVsCameraDistance2DModifierParams::ParametersStruct*>(&parameters()), handle, offset);
}


/* Dynamic Handle Indices */

void ScaleVsCameraDistance2DModifierParams::freeParameterDefinitionTable(NxParameterized::Traits* traits)
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

void ScaleVsCameraDistance2DModifierParams::buildTree(void)
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

	// Initialize DefinitionImpl node: nodeIndex=1, longName="controlPoints"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[1];
		ParamDef->init("controlPoints", TYPE_ARRAY, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[5];
		static Hint* HintPtrTable[5] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], &HintTable[4], };
		HintTable[0].init("CURVE_X_SCALE", physx::PxU64(1000), true);
		HintTable[1].init("CURVE_Y_SCALE", physx::PxU64(1), true);
		HintTable[2].init("editorCurve", physx::PxU64(1), true);
		HintTable[3].init("xAxisLabel", "Camera Distance", true);
		HintTable[4].init("yAxisLabel", "Scale", true);
		ParamDefTable[1].setHints((const NxParameterized::Hint**)HintPtrTable, 5);

#else

		static HintImpl HintTable[6];
		static Hint* HintPtrTable[6] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], &HintTable[4], &HintTable[5], };
		HintTable[0].init("CURVE_X_SCALE", physx::PxU64(1000), true);
		HintTable[1].init("CURVE_Y_SCALE", physx::PxU64(1), true);
		HintTable[2].init("editorCurve", physx::PxU64(1), true);
		HintTable[3].init("shortDescription", "Control points for a composite 2D scale vs camera distance curve", true);
		HintTable[4].init("xAxisLabel", "Camera Distance", true);
		HintTable[5].init("yAxisLabel", "Scale", true);
		ParamDefTable[1].setHints((const NxParameterized::Hint**)HintPtrTable, 6);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */




		ParamDef->setArraySize(-1);
	}

	// Initialize DefinitionImpl node: nodeIndex=2, longName="controlPoints[]"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[2];
		ParamDef->init("controlPoints", TYPE_STRUCT, "scaleCameraDistanceStruct", true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[5];
		static Hint* HintPtrTable[5] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], &HintTable[4], };
		HintTable[0].init("CURVE_X_SCALE", physx::PxU64(1000), true);
		HintTable[1].init("CURVE_Y_SCALE", physx::PxU64(1), true);
		HintTable[2].init("editorCurve", physx::PxU64(1), true);
		HintTable[3].init("xAxisLabel", "Camera Distance", true);
		HintTable[4].init("yAxisLabel", "Scale", true);
		ParamDefTable[2].setHints((const NxParameterized::Hint**)HintPtrTable, 5);

#else

		static HintImpl HintTable[6];
		static Hint* HintPtrTable[6] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], &HintTable[4], &HintTable[5], };
		HintTable[0].init("CURVE_X_SCALE", physx::PxU64(1000), true);
		HintTable[1].init("CURVE_Y_SCALE", physx::PxU64(1), true);
		HintTable[2].init("editorCurve", physx::PxU64(1), true);
		HintTable[3].init("shortDescription", "Control points for a composite 2D scale vs camera distance curve", true);
		HintTable[4].init("xAxisLabel", "Camera Distance", true);
		HintTable[5].init("yAxisLabel", "Scale", true);
		ParamDefTable[2].setHints((const NxParameterized::Hint**)HintPtrTable, 6);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=3, longName="controlPoints[].cameraDistance"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[3];
		ParamDef->init("cameraDistance", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("min", physx::PxU64(0), true);
		ParamDefTable[3].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("min", physx::PxU64(0), true);
		ParamDefTable[3].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=4, longName="controlPoints[].scale"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[4];
		ParamDef->init("scale", TYPE_VEC2, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("min", physx::PxU64(0), true);
		ParamDefTable[4].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("min", physx::PxU64(0), true);
		ParamDefTable[4].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// SetChildren for: nodeIndex=0, longName=""
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(1);

		ParamDefTable[0].setChildren(Children, 1);
	}

	// SetChildren for: nodeIndex=1, longName="controlPoints"
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(2);

		ParamDefTable[1].setChildren(Children, 1);
	}

	// SetChildren for: nodeIndex=2, longName="controlPoints[]"
	{
		static Definition* Children[2];
		Children[0] = PDEF_PTR(3);
		Children[1] = PDEF_PTR(4);

		ParamDefTable[2].setChildren(Children, 2);
	}

	mBuiltFlag = true;

}
void ScaleVsCameraDistance2DModifierParams::initStrings(void)
{
}

void ScaleVsCameraDistance2DModifierParams::initDynamicArrays(void)
{
	controlPoints.buf = NULL;
	controlPoints.isAllocated = true;
	controlPoints.elementSize = sizeof(scaleCameraDistanceStruct_Type);
	controlPoints.arraySizes[0] = 0;
}

void ScaleVsCameraDistance2DModifierParams::initDefaults(void)
{

	freeStrings();
	freeReferences();
	freeDynamicArrays();

	initDynamicArrays();
	initStrings();
	initReferences();
}

void ScaleVsCameraDistance2DModifierParams::initReferences(void)
{
}

void ScaleVsCameraDistance2DModifierParams::freeDynamicArrays(void)
{
	if (controlPoints.isAllocated && controlPoints.buf)
	{
		mParameterizedTraits->free(controlPoints.buf);
	}
}

void ScaleVsCameraDistance2DModifierParams::freeStrings(void)
{
}

void ScaleVsCameraDistance2DModifierParams::freeReferences(void)
{
}

} // namespace iofx
} // namespace apex
} // namespace physx
