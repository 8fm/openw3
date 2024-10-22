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
// Created: 2013.10.01 14:57:16

#include "BasicIosDebugRenderParams.h"
#include <string.h>
#include <stdlib.h>

using namespace NxParameterized;

namespace physx
{
namespace apex
{
namespace basicios
{

using namespace BasicIosDebugRenderParamsNS;

const char* const BasicIosDebugRenderParamsFactory::vptr =
    NxParameterized::getVptr<BasicIosDebugRenderParams, BasicIosDebugRenderParams::ClassAlignment>();

const physx::PxU32 NumParamDefs = 4;
static NxParameterized::DefinitionImpl* ParamDefTable; // now allocated in buildTree [NumParamDefs];


static const size_t ParamLookupChildrenTable[] =
{
	1, 2, 3,
};

#define TENUM(type) physx::##type
#define CHILDREN(index) &ParamLookupChildrenTable[index]
static const NxParameterized::ParamLookupNode ParamLookupTable[NumParamDefs] =
{
	{ TYPE_STRUCT, false, 0, CHILDREN(0), 3 },
	{ TYPE_BOOL, false, (size_t)(&((ParametersStruct*)0)->VISUALIZE_BASIC_IOS_ACTOR), NULL, 0 }, // VISUALIZE_BASIC_IOS_ACTOR
	{ TYPE_BOOL, false, (size_t)(&((ParametersStruct*)0)->VISUALIZE_BASIC_IOS_COLLIDE_SHAPES), NULL, 0 }, // VISUALIZE_BASIC_IOS_COLLIDE_SHAPES
	{ TYPE_BOOL, false, (size_t)(&((ParametersStruct*)0)->VISUALIZE_BASIC_IOS_GRID_DENSITY), NULL, 0 }, // VISUALIZE_BASIC_IOS_GRID_DENSITY
};


bool BasicIosDebugRenderParams::mBuiltFlag = false;
NxParameterized::MutexType BasicIosDebugRenderParams::mBuiltFlagMutex;

BasicIosDebugRenderParams::BasicIosDebugRenderParams(NxParameterized::Traits* traits, void* buf, PxI32* refCount) :
	NxParameters(traits, buf, refCount)
{
	//mParameterizedTraits->registerFactory(className(), &BasicIosDebugRenderParamsFactoryInst);

	if (!buf) //Do not init data if it is inplace-deserialized
	{
		initDynamicArrays();
		initStrings();
		initReferences();
		initDefaults();
	}
}

BasicIosDebugRenderParams::~BasicIosDebugRenderParams()
{
	freeStrings();
	freeReferences();
	freeDynamicArrays();
}

void BasicIosDebugRenderParams::destroy()
{
	// We cache these fields here to avoid overwrite in destructor
	bool doDeallocateSelf = mDoDeallocateSelf;
	NxParameterized::Traits* traits = mParameterizedTraits;
	physx::PxI32* refCount = mRefCount;
	void* buf = mBuffer;

	this->~BasicIosDebugRenderParams();

	NxParameters::destroy(this, traits, doDeallocateSelf, refCount, buf);
}

const NxParameterized::DefinitionImpl* BasicIosDebugRenderParams::getParameterDefinitionTree(void)
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

const NxParameterized::DefinitionImpl* BasicIosDebugRenderParams::getParameterDefinitionTree(void) const
{
	BasicIosDebugRenderParams* tmpParam = const_cast<BasicIosDebugRenderParams*>(this);

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

NxParameterized::ErrorType BasicIosDebugRenderParams::getParameterHandle(const char* long_name, Handle& handle) const
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

NxParameterized::ErrorType BasicIosDebugRenderParams::getParameterHandle(const char* long_name, Handle& handle)
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

void BasicIosDebugRenderParams::getVarPtr(const Handle& handle, void*& ptr, size_t& offset) const
{
	ptr = getVarPtrHelper(&ParamLookupTable[0], const_cast<BasicIosDebugRenderParams::ParametersStruct*>(&parameters()), handle, offset);
}


/* Dynamic Handle Indices */

void BasicIosDebugRenderParams::freeParameterDefinitionTable(NxParameterized::Traits* traits)
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

void BasicIosDebugRenderParams::buildTree(void)
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

	// Initialize DefinitionImpl node: nodeIndex=1, longName="VISUALIZE_BASIC_IOS_ACTOR"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[1];
		ParamDef->init("VISUALIZE_BASIC_IOS_ACTOR", TYPE_BOOL, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=2, longName="VISUALIZE_BASIC_IOS_COLLIDE_SHAPES"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[2];
		ParamDef->init("VISUALIZE_BASIC_IOS_COLLIDE_SHAPES", TYPE_BOOL, NULL, true);






	}

	// Initialize DefinitionImpl node: nodeIndex=3, longName="VISUALIZE_BASIC_IOS_GRID_DENSITY"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[3];
		ParamDef->init("VISUALIZE_BASIC_IOS_GRID_DENSITY", TYPE_BOOL, NULL, true);






	}

	// SetChildren for: nodeIndex=0, longName=""
	{
		static Definition* Children[3];
		Children[0] = PDEF_PTR(1);
		Children[1] = PDEF_PTR(2);
		Children[2] = PDEF_PTR(3);

		ParamDefTable[0].setChildren(Children, 3);
	}

	mBuiltFlag = true;

}
void BasicIosDebugRenderParams::initStrings(void)
{
}

void BasicIosDebugRenderParams::initDynamicArrays(void)
{
}

void BasicIosDebugRenderParams::initDefaults(void)
{

	freeStrings();
	freeReferences();
	freeDynamicArrays();
	VISUALIZE_BASIC_IOS_ACTOR = bool(true);
	VISUALIZE_BASIC_IOS_COLLIDE_SHAPES = bool(false);
	VISUALIZE_BASIC_IOS_GRID_DENSITY = bool(false);

	initDynamicArrays();
	initStrings();
	initReferences();
}

void BasicIosDebugRenderParams::initReferences(void)
{
}

void BasicIosDebugRenderParams::freeDynamicArrays(void)
{
}

void BasicIosDebugRenderParams::freeStrings(void)
{
}

void BasicIosDebugRenderParams::freeReferences(void)
{
}

} // namespace basicios
} // namespace apex
} // namespace physx
