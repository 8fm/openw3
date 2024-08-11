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
// Created: 2013.10.01 14:57:20

#include "ExplosionDebugRenderParams.h"
#include <string.h>
#include <stdlib.h>

using namespace NxParameterized;

namespace physx
{
namespace apex
{
namespace explosion
{

using namespace ExplosionDebugRenderParamsNS;

const char* const ExplosionDebugRenderParamsFactory::vptr =
    NxParameterized::getVptr<ExplosionDebugRenderParams, ExplosionDebugRenderParams::ClassAlignment>();

const physx::PxU32 NumParamDefs = 8;
static NxParameterized::DefinitionImpl* ParamDefTable; // now allocated in buildTree [NumParamDefs];


static const size_t ParamLookupChildrenTable[] =
{
	1, 2, 3, 4, 5, 6, 7,
};

#define TENUM(type) physx::##type
#define CHILDREN(index) &ParamLookupChildrenTable[index]
static const NxParameterized::ParamLookupNode ParamLookupTable[NumParamDefs] =
{
	{ TYPE_STRUCT, false, 0, CHILDREN(0), 7 },
	{ TYPE_BOOL, false, (size_t)(&((ParametersStruct*)0)->VISUALIZE_EXPLOSION_ACTOR), NULL, 0 }, // VISUALIZE_EXPLOSION_ACTOR
	{ TYPE_BOOL, false, (size_t)(&((ParametersStruct*)0)->VISUALIZE_EXPLOSION_FORCE_FIELDS), NULL, 0 }, // VISUALIZE_EXPLOSION_FORCE_FIELDS
	{ TYPE_BOOL, false, (size_t)(&((ParametersStruct*)0)->VISUALIZE_EXPLOSION_FORCES), NULL, 0 }, // VISUALIZE_EXPLOSION_FORCES
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->FORCE_FIELDS_FORCES_DEBUG_RENDERING_SCALING), NULL, 0 }, // FORCE_FIELDS_FORCES_DEBUG_RENDERING_SCALING
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->FORCE_FIELDS_FORCES_DEBUG_RENDERING_SPACING), NULL, 0 }, // FORCE_FIELDS_FORCES_DEBUG_RENDERING_SPACING
	{ TYPE_BOOL, false, (size_t)(&((ParametersStruct*)0)->VISUALIZE_EXPLOSION_ACTOR_NAME), NULL, 0 }, // VISUALIZE_EXPLOSION_ACTOR_NAME
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->THRESHOLD_DISTANCE_EXPLOSION_ACTOR_NAME), NULL, 0 }, // THRESHOLD_DISTANCE_EXPLOSION_ACTOR_NAME
};


bool ExplosionDebugRenderParams::mBuiltFlag = false;
NxParameterized::MutexType ExplosionDebugRenderParams::mBuiltFlagMutex;

ExplosionDebugRenderParams::ExplosionDebugRenderParams(NxParameterized::Traits* traits, void* buf, PxI32* refCount) :
	NxParameters(traits, buf, refCount)
{
	//mParameterizedTraits->registerFactory(className(), &ExplosionDebugRenderParamsFactoryInst);

	if (!buf) //Do not init data if it is inplace-deserialized
	{
		initDynamicArrays();
		initStrings();
		initReferences();
		initDefaults();
	}
}

ExplosionDebugRenderParams::~ExplosionDebugRenderParams()
{
	freeStrings();
	freeReferences();
	freeDynamicArrays();
}

void ExplosionDebugRenderParams::destroy()
{
	// We cache these fields here to avoid overwrite in destructor
	bool doDeallocateSelf = mDoDeallocateSelf;
	NxParameterized::Traits* traits = mParameterizedTraits;
	physx::PxI32* refCount = mRefCount;
	void* buf = mBuffer;

	this->~ExplosionDebugRenderParams();

	NxParameters::destroy(this, traits, doDeallocateSelf, refCount, buf);
}

const NxParameterized::DefinitionImpl* ExplosionDebugRenderParams::getParameterDefinitionTree(void)
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

const NxParameterized::DefinitionImpl* ExplosionDebugRenderParams::getParameterDefinitionTree(void) const
{
	ExplosionDebugRenderParams* tmpParam = const_cast<ExplosionDebugRenderParams*>(this);

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

NxParameterized::ErrorType ExplosionDebugRenderParams::getParameterHandle(const char* long_name, Handle& handle) const
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

NxParameterized::ErrorType ExplosionDebugRenderParams::getParameterHandle(const char* long_name, Handle& handle)
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

void ExplosionDebugRenderParams::getVarPtr(const Handle& handle, void*& ptr, size_t& offset) const
{
	ptr = getVarPtrHelper(&ParamLookupTable[0], const_cast<ExplosionDebugRenderParams::ParametersStruct*>(&parameters()), handle, offset);
}


/* Dynamic Handle Indices */

void ExplosionDebugRenderParams::freeParameterDefinitionTable(NxParameterized::Traits* traits)
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

void ExplosionDebugRenderParams::buildTree(void)
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

	// Initialize DefinitionImpl node: nodeIndex=1, longName="VISUALIZE_EXPLOSION_ACTOR"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[1];
		ParamDef->init("VISUALIZE_EXPLOSION_ACTOR", TYPE_BOOL, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=2, longName="VISUALIZE_EXPLOSION_FORCE_FIELDS"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[2];
		ParamDef->init("VISUALIZE_EXPLOSION_FORCE_FIELDS", TYPE_BOOL, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=3, longName="VISUALIZE_EXPLOSION_FORCES"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[3];
		ParamDef->init("VISUALIZE_EXPLOSION_FORCES", TYPE_BOOL, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=4, longName="FORCE_FIELDS_FORCES_DEBUG_RENDERING_SCALING"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[4];
		ParamDef->init("FORCE_FIELDS_FORCES_DEBUG_RENDERING_SCALING", TYPE_F32, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=5, longName="FORCE_FIELDS_FORCES_DEBUG_RENDERING_SPACING"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[5];
		ParamDef->init("FORCE_FIELDS_FORCES_DEBUG_RENDERING_SPACING", TYPE_F32, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=6, longName="VISUALIZE_EXPLOSION_ACTOR_NAME"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[6];
		ParamDef->init("VISUALIZE_EXPLOSION_ACTOR_NAME", TYPE_BOOL, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=7, longName="THRESHOLD_DISTANCE_EXPLOSION_ACTOR_NAME"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[7];
		ParamDef->init("THRESHOLD_DISTANCE_EXPLOSION_ACTOR_NAME", TYPE_F32, NULL, true);






	}

	// SetChildren for: nodeIndex=0, longName=""
	{
		static Definition* Children[7];
		Children[0] = PDEF_PTR(1);
		Children[1] = PDEF_PTR(2);
		Children[2] = PDEF_PTR(3);
		Children[3] = PDEF_PTR(4);
		Children[4] = PDEF_PTR(5);
		Children[5] = PDEF_PTR(6);
		Children[6] = PDEF_PTR(7);

		ParamDefTable[0].setChildren(Children, 7);
	}

	mBuiltFlag = true;

}
void ExplosionDebugRenderParams::initStrings(void)
{
}

void ExplosionDebugRenderParams::initDynamicArrays(void)
{
}

void ExplosionDebugRenderParams::initDefaults(void)
{

	freeStrings();
	freeReferences();
	freeDynamicArrays();
	VISUALIZE_EXPLOSION_ACTOR = bool(true);
	VISUALIZE_EXPLOSION_FORCE_FIELDS = bool(false);
	VISUALIZE_EXPLOSION_FORCES = bool(false);
	FORCE_FIELDS_FORCES_DEBUG_RENDERING_SCALING = physx::PxF32(1.0f);
	FORCE_FIELDS_FORCES_DEBUG_RENDERING_SPACING = physx::PxF32(1.0f);
	VISUALIZE_EXPLOSION_ACTOR_NAME = bool(true);
	THRESHOLD_DISTANCE_EXPLOSION_ACTOR_NAME = physx::PxF32(3.402823466e+038);

	initDynamicArrays();
	initStrings();
	initReferences();
}

void ExplosionDebugRenderParams::initReferences(void)
{
}

void ExplosionDebugRenderParams::freeDynamicArrays(void)
{
}

void ExplosionDebugRenderParams::freeStrings(void)
{
}

void ExplosionDebugRenderParams::freeReferences(void)
{
}

} // namespace explosion
} // namespace apex
} // namespace physx
