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
// Created: 2013.10.01 14:57:23

#include "ForceFieldEffect.h"
#include <string.h>
#include <stdlib.h>

using namespace NxParameterized;

namespace physx
{
namespace apex
{
namespace particles
{

using namespace ForceFieldEffectNS;

const char* const ForceFieldEffectFactory::vptr =
    NxParameterized::getVptr<ForceFieldEffect, ForceFieldEffect::ClassAlignment>();

const physx::PxU32 NumParamDefs = 18;
static NxParameterized::DefinitionImpl* ParamDefTable; // now allocated in buildTree [NumParamDefs];


static const size_t ParamLookupChildrenTable[] =
{
	1, 17, 2, 3, 4, 8, 12, 13, 14, 15, 16, 5, 6, 7, 9, 10, 11,
};

#define TENUM(type) physx::##type
#define CHILDREN(index) &ParamLookupChildrenTable[index]
static const NxParameterized::ParamLookupNode ParamLookupTable[NumParamDefs] =
{
	{ TYPE_STRUCT, false, 0, CHILDREN(0), 2 },
	{ TYPE_STRUCT, false, (size_t)(&((ParametersStruct*)0)->EffectProperties), CHILDREN(2), 9 }, // EffectProperties
	{ TYPE_STRING, false, (size_t)(&((EffectProperties_Type*)0)->UserString), NULL, 0 }, // EffectProperties.UserString
	{ TYPE_BOOL, false, (size_t)(&((EffectProperties_Type*)0)->Enable), NULL, 0 }, // EffectProperties.Enable
	{ TYPE_STRUCT, false, (size_t)(&((EffectProperties_Type*)0)->Position), CHILDREN(11), 3 }, // EffectProperties.Position
	{ TYPE_F32, false, (size_t)(&((TranslateObject_Type*)0)->TranslateX), NULL, 0 }, // EffectProperties.Position.TranslateX
	{ TYPE_F32, false, (size_t)(&((TranslateObject_Type*)0)->TranslateY), NULL, 0 }, // EffectProperties.Position.TranslateY
	{ TYPE_F32, false, (size_t)(&((TranslateObject_Type*)0)->TranslateZ), NULL, 0 }, // EffectProperties.Position.TranslateZ
	{ TYPE_STRUCT, false, (size_t)(&((EffectProperties_Type*)0)->Orientation), CHILDREN(14), 3 }, // EffectProperties.Orientation
	{ TYPE_F32, false, (size_t)(&((OrientObject_Type*)0)->RotateX), NULL, 0 }, // EffectProperties.Orientation.RotateX
	{ TYPE_F32, false, (size_t)(&((OrientObject_Type*)0)->RotateY), NULL, 0 }, // EffectProperties.Orientation.RotateY
	{ TYPE_F32, false, (size_t)(&((OrientObject_Type*)0)->RotateZ), NULL, 0 }, // EffectProperties.Orientation.RotateZ
	{ TYPE_F32, false, (size_t)(&((EffectProperties_Type*)0)->InitialDelayTime), NULL, 0 }, // EffectProperties.InitialDelayTime
	{ TYPE_F32, false, (size_t)(&((EffectProperties_Type*)0)->Duration), NULL, 0 }, // EffectProperties.Duration
	{ TYPE_U32, false, (size_t)(&((EffectProperties_Type*)0)->RepeatCount), NULL, 0 }, // EffectProperties.RepeatCount
	{ TYPE_F32, false, (size_t)(&((EffectProperties_Type*)0)->RepeatDelay), NULL, 0 }, // EffectProperties.RepeatDelay
	{ TYPE_F32, false, (size_t)(&((EffectProperties_Type*)0)->RandomizeRepeatTime), NULL, 0 }, // EffectProperties.RandomizeRepeatTime
	{ TYPE_REF, false, (size_t)(&((ParametersStruct*)0)->ForceField), NULL, 0 }, // ForceField
};


bool ForceFieldEffect::mBuiltFlag = false;
NxParameterized::MutexType ForceFieldEffect::mBuiltFlagMutex;

ForceFieldEffect::ForceFieldEffect(NxParameterized::Traits* traits, void* buf, PxI32* refCount) :
	NxParameters(traits, buf, refCount)
{
	//mParameterizedTraits->registerFactory(className(), &ForceFieldEffectFactoryInst);

	if (!buf) //Do not init data if it is inplace-deserialized
	{
		initDynamicArrays();
		initStrings();
		initReferences();
		initDefaults();
	}
}

ForceFieldEffect::~ForceFieldEffect()
{
	freeStrings();
	freeReferences();
	freeDynamicArrays();
}

void ForceFieldEffect::destroy()
{
	// We cache these fields here to avoid overwrite in destructor
	bool doDeallocateSelf = mDoDeallocateSelf;
	NxParameterized::Traits* traits = mParameterizedTraits;
	physx::PxI32* refCount = mRefCount;
	void* buf = mBuffer;

	this->~ForceFieldEffect();

	NxParameters::destroy(this, traits, doDeallocateSelf, refCount, buf);
}

const NxParameterized::DefinitionImpl* ForceFieldEffect::getParameterDefinitionTree(void)
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

const NxParameterized::DefinitionImpl* ForceFieldEffect::getParameterDefinitionTree(void) const
{
	ForceFieldEffect* tmpParam = const_cast<ForceFieldEffect*>(this);

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

NxParameterized::ErrorType ForceFieldEffect::getParameterHandle(const char* long_name, Handle& handle) const
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

NxParameterized::ErrorType ForceFieldEffect::getParameterHandle(const char* long_name, Handle& handle)
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

void ForceFieldEffect::getVarPtr(const Handle& handle, void*& ptr, size_t& offset) const
{
	ptr = getVarPtrHelper(&ParamLookupTable[0], const_cast<ForceFieldEffect::ParametersStruct*>(&parameters()), handle, offset);
}


/* Dynamic Handle Indices */

void ForceFieldEffect::freeParameterDefinitionTable(NxParameterized::Traits* traits)
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

void ForceFieldEffect::buildTree(void)
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

	// Initialize DefinitionImpl node: nodeIndex=1, longName="EffectProperties"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[1];
		ParamDef->init("EffectProperties", TYPE_STRUCT, "EffectProperties", true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		ParamDefTable[1].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		ParamDefTable[1].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=2, longName="EffectProperties.UserString"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[2];
		ParamDef->init("UserString", TYPE_STRING, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=3, longName="EffectProperties.Enable"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[3];
		ParamDef->init("Enable", TYPE_BOOL, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=4, longName="EffectProperties.Position"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[4];
		ParamDef->init("Position", TYPE_STRUCT, "TranslateObject", true);






	}

	// Initialize DefinitionImpl node: nodeIndex=5, longName="EffectProperties.Position.TranslateX"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[5];
		ParamDef->init("TranslateX", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[3];
		static Hint* HintPtrTable[3] = { &HintTable[0], &HintTable[1], &HintTable[2], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		HintTable[1].init("max", physx::PxF64(500.000000000), true);
		HintTable[2].init("min", physx::PxF64(-500.000000000), true);
		ParamDefTable[5].setHints((const NxParameterized::Hint**)HintPtrTable, 3);

#else

		static HintImpl HintTable[4];
		static Hint* HintPtrTable[4] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		HintTable[1].init("max", physx::PxF64(500.000000000), true);
		HintTable[2].init("min", physx::PxF64(-500.000000000), true);
		HintTable[3].init("shortDescription", "Nudge up to +/- one meter in each direction", true);
		ParamDefTable[5].setHints((const NxParameterized::Hint**)HintPtrTable, 4);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=6, longName="EffectProperties.Position.TranslateY"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[6];
		ParamDef->init("TranslateY", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[3];
		static Hint* HintPtrTable[3] = { &HintTable[0], &HintTable[1], &HintTable[2], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		HintTable[1].init("max", physx::PxF64(500.000000000), true);
		HintTable[2].init("min", physx::PxF64(-500.000000000), true);
		ParamDefTable[6].setHints((const NxParameterized::Hint**)HintPtrTable, 3);

#else

		static HintImpl HintTable[4];
		static Hint* HintPtrTable[4] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		HintTable[1].init("max", physx::PxF64(500.000000000), true);
		HintTable[2].init("min", physx::PxF64(-500.000000000), true);
		HintTable[3].init("shortDescription", "Nudge up to +/- one meter in each direction", true);
		ParamDefTable[6].setHints((const NxParameterized::Hint**)HintPtrTable, 4);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=7, longName="EffectProperties.Position.TranslateZ"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[7];
		ParamDef->init("TranslateZ", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[3];
		static Hint* HintPtrTable[3] = { &HintTable[0], &HintTable[1], &HintTable[2], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		HintTable[1].init("max", physx::PxF64(500.000000000), true);
		HintTable[2].init("min", physx::PxF64(-500.000000000), true);
		ParamDefTable[7].setHints((const NxParameterized::Hint**)HintPtrTable, 3);

#else

		static HintImpl HintTable[4];
		static Hint* HintPtrTable[4] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		HintTable[1].init("max", physx::PxF64(500.000000000), true);
		HintTable[2].init("min", physx::PxF64(-500.000000000), true);
		HintTable[3].init("shortDescription", "Nudge up to +/- one meter in each direction", true);
		ParamDefTable[7].setHints((const NxParameterized::Hint**)HintPtrTable, 4);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=8, longName="EffectProperties.Orientation"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[8];
		ParamDef->init("Orientation", TYPE_STRUCT, "OrientObject", true);






	}

	// Initialize DefinitionImpl node: nodeIndex=9, longName="EffectProperties.Orientation.RotateX"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[9];
		ParamDef->init("RotateX", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[3];
		static Hint* HintPtrTable[3] = { &HintTable[0], &HintTable[1], &HintTable[2], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		HintTable[1].init("max", physx::PxF64(360.000000000), true);
		HintTable[2].init("min", physx::PxF64(0.000000000), true);
		ParamDefTable[9].setHints((const NxParameterized::Hint**)HintPtrTable, 3);

#else

		static HintImpl HintTable[4];
		static Hint* HintPtrTable[4] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		HintTable[1].init("max", physx::PxF64(360.000000000), true);
		HintTable[2].init("min", physx::PxF64(0.000000000), true);
		HintTable[3].init("shortDescription", "Rotate on the X axis (0-360 degrees)", true);
		ParamDefTable[9].setHints((const NxParameterized::Hint**)HintPtrTable, 4);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=10, longName="EffectProperties.Orientation.RotateY"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[10];
		ParamDef->init("RotateY", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[3];
		static Hint* HintPtrTable[3] = { &HintTable[0], &HintTable[1], &HintTable[2], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		HintTable[1].init("max", physx::PxF64(360.000000000), true);
		HintTable[2].init("min", physx::PxF64(0.000000000), true);
		ParamDefTable[10].setHints((const NxParameterized::Hint**)HintPtrTable, 3);

#else

		static HintImpl HintTable[4];
		static Hint* HintPtrTable[4] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		HintTable[1].init("max", physx::PxF64(360.000000000), true);
		HintTable[2].init("min", physx::PxF64(0.000000000), true);
		HintTable[3].init("shortDescription", "Rotate on the Y axis (0-360 degrees)", true);
		ParamDefTable[10].setHints((const NxParameterized::Hint**)HintPtrTable, 4);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=11, longName="EffectProperties.Orientation.RotateZ"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[11];
		ParamDef->init("RotateZ", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[3];
		static Hint* HintPtrTable[3] = { &HintTable[0], &HintTable[1], &HintTable[2], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		HintTable[1].init("max", physx::PxF64(360.000000000), true);
		HintTable[2].init("min", physx::PxF64(0.000000000), true);
		ParamDefTable[11].setHints((const NxParameterized::Hint**)HintPtrTable, 3);

#else

		static HintImpl HintTable[4];
		static Hint* HintPtrTable[4] = { &HintTable[0], &HintTable[1], &HintTable[2], &HintTable[3], };
		HintTable[0].init("READONLY", physx::PxU64(0), true);
		HintTable[1].init("max", physx::PxF64(360.000000000), true);
		HintTable[2].init("min", physx::PxF64(0.000000000), true);
		HintTable[3].init("shortDescription", "Rotate on the Z axis (0-360 degrees)", true);
		ParamDefTable[11].setHints((const NxParameterized::Hint**)HintPtrTable, 4);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=12, longName="EffectProperties.InitialDelayTime"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[12];
		ParamDef->init("InitialDelayTime", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("min", physx::PxF64(0.000000000), true);
		ParamDefTable[12].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("min", physx::PxF64(0.000000000), true);
		HintTable[1].init("shortDescription", "Initial time to delay before spawning this effect", true);
		ParamDefTable[12].setHints((const NxParameterized::Hint**)HintPtrTable, 2);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=13, longName="EffectProperties.Duration"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[13];
		ParamDef->init("Duration", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("min", physx::PxF64(0.000000000), true);
		ParamDefTable[13].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("min", physx::PxF64(0.000000000), true);
		HintTable[1].init("shortDescription", "The duration of the effect in secontds", true);
		ParamDefTable[13].setHints((const NxParameterized::Hint**)HintPtrTable, 2);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=14, longName="EffectProperties.RepeatCount"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[14];
		ParamDef->init("RepeatCount", TYPE_U32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("min", physx::PxU64(1), true);
		ParamDefTable[14].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("min", physx::PxU64(1), true);
		HintTable[1].init("shortDescription", "The number of times to repeat the effect; 9999 means repeate forever", true);
		ParamDefTable[14].setHints((const NxParameterized::Hint**)HintPtrTable, 2);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=15, longName="EffectProperties.RepeatDelay"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[15];
		ParamDef->init("RepeatDelay", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("min", physx::PxF64(0.000000000), true);
		ParamDefTable[15].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("min", physx::PxF64(0.000000000), true);
		HintTable[1].init("shortDescription", "The time to delay activation each time the effect repeats in seconds; this can be different than the initial delay time", true);
		ParamDefTable[15].setHints((const NxParameterized::Hint**)HintPtrTable, 2);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=16, longName="EffectProperties.RandomizeRepeatTime"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[16];
		ParamDef->init("RandomizeRepeatTime", TYPE_F32, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("max", physx::PxF64(1.000000000), true);
		HintTable[1].init("min", physx::PxF64(0.000000000), true);
		ParamDefTable[16].setHints((const NxParameterized::Hint**)HintPtrTable, 2);

#else

		static HintImpl HintTable[3];
		static Hint* HintPtrTable[3] = { &HintTable[0], &HintTable[1], &HintTable[2], };
		HintTable[0].init("max", physx::PxF64(1.000000000), true);
		HintTable[1].init("min", physx::PxF64(0.000000000), true);
		HintTable[2].init("shortDescription", "Amount of randomization to the repeat cycle; a value of zero (default) means no random and a value of one means completely random.", true);
		ParamDefTable[16].setHints((const NxParameterized::Hint**)HintPtrTable, 3);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=17, longName="ForceField"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[17];
		ParamDef->init("ForceField", TYPE_REF, NULL, true);



		static const char* const RefVariantVals[] = { "ForceFieldAsset" };
		ParamDefTable[17].setRefVariantVals((const char**)RefVariantVals, 1);



	}

	// SetChildren for: nodeIndex=0, longName=""
	{
		static Definition* Children[2];
		Children[0] = PDEF_PTR(1);
		Children[1] = PDEF_PTR(17);

		ParamDefTable[0].setChildren(Children, 2);
	}

	// SetChildren for: nodeIndex=1, longName="EffectProperties"
	{
		static Definition* Children[9];
		Children[0] = PDEF_PTR(2);
		Children[1] = PDEF_PTR(3);
		Children[2] = PDEF_PTR(4);
		Children[3] = PDEF_PTR(8);
		Children[4] = PDEF_PTR(12);
		Children[5] = PDEF_PTR(13);
		Children[6] = PDEF_PTR(14);
		Children[7] = PDEF_PTR(15);
		Children[8] = PDEF_PTR(16);

		ParamDefTable[1].setChildren(Children, 9);
	}

	// SetChildren for: nodeIndex=4, longName="EffectProperties.Position"
	{
		static Definition* Children[3];
		Children[0] = PDEF_PTR(5);
		Children[1] = PDEF_PTR(6);
		Children[2] = PDEF_PTR(7);

		ParamDefTable[4].setChildren(Children, 3);
	}

	// SetChildren for: nodeIndex=8, longName="EffectProperties.Orientation"
	{
		static Definition* Children[3];
		Children[0] = PDEF_PTR(9);
		Children[1] = PDEF_PTR(10);
		Children[2] = PDEF_PTR(11);

		ParamDefTable[8].setChildren(Children, 3);
	}

	mBuiltFlag = true;

}
void ForceFieldEffect::initStrings(void)
{
	EffectProperties.UserString.isAllocated = true;
	EffectProperties.UserString.buf = NULL;
}

void ForceFieldEffect::initDynamicArrays(void)
{
}

void ForceFieldEffect::initDefaults(void)
{

	freeStrings();
	freeReferences();
	freeDynamicArrays();
	EffectProperties.Enable = bool(1);
	EffectProperties.Position.TranslateX = physx::PxF32(0.000000000);
	EffectProperties.Position.TranslateY = physx::PxF32(0.000000000);
	EffectProperties.Position.TranslateZ = physx::PxF32(0.000000000);
	EffectProperties.Orientation.RotateX = physx::PxF32(0.000000000);
	EffectProperties.Orientation.RotateY = physx::PxF32(0.000000000);
	EffectProperties.Orientation.RotateZ = physx::PxF32(0.000000000);
	EffectProperties.InitialDelayTime = physx::PxF32(0.000000000);
	EffectProperties.Duration = physx::PxF32(0.000000000);
	EffectProperties.RepeatCount = physx::PxU32(1);
	EffectProperties.RepeatDelay = physx::PxF32(0.000000000);
	EffectProperties.RandomizeRepeatTime = physx::PxF32(0.000000000);

	initDynamicArrays();
	initStrings();
	initReferences();
}

void ForceFieldEffect::initReferences(void)
{
	ForceField = NULL;

}

void ForceFieldEffect::freeDynamicArrays(void)
{
}

void ForceFieldEffect::freeStrings(void)
{

	if (EffectProperties.UserString.isAllocated && EffectProperties.UserString.buf)
	{
		mParameterizedTraits->strfree((char*)EffectProperties.UserString.buf);
	}
}

void ForceFieldEffect::freeReferences(void)
{
	if (ForceField)
	{
		ForceField->destroy();
	}

}

} // namespace particles
} // namespace apex
} // namespace physx
