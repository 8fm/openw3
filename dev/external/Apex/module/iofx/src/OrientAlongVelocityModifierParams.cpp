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

#include "OrientAlongVelocityModifierParams.h"
#include <string.h>
#include <stdlib.h>

using namespace NxParameterized;

namespace physx
{
namespace apex
{
namespace iofx
{

using namespace OrientAlongVelocityModifierParamsNS;

const char* const OrientAlongVelocityModifierParamsFactory::vptr =
    NxParameterized::getVptr<OrientAlongVelocityModifierParams, OrientAlongVelocityModifierParams::ClassAlignment>();

const physx::PxU32 NumParamDefs = 2;
static NxParameterized::DefinitionImpl* ParamDefTable; // now allocated in buildTree [NumParamDefs];


static const size_t ParamLookupChildrenTable[] =
{
	1,
};

#define TENUM(type) physx::##type
#define CHILDREN(index) &ParamLookupChildrenTable[index]
static const NxParameterized::ParamLookupNode ParamLookupTable[NumParamDefs] =
{
	{ TYPE_STRUCT, false, 0, CHILDREN(0), 1 },
	{ TYPE_VEC3, false, (size_t)(&((ParametersStruct*)0)->modelForward), NULL, 0 }, // modelForward
};


bool OrientAlongVelocityModifierParams::mBuiltFlag = false;
NxParameterized::MutexType OrientAlongVelocityModifierParams::mBuiltFlagMutex;

OrientAlongVelocityModifierParams::OrientAlongVelocityModifierParams(NxParameterized::Traits* traits, void* buf, PxI32* refCount) :
	NxParameters(traits, buf, refCount)
{
	//mParameterizedTraits->registerFactory(className(), &OrientAlongVelocityModifierParamsFactoryInst);

	if (!buf) //Do not init data if it is inplace-deserialized
	{
		initDynamicArrays();
		initStrings();
		initReferences();
		initDefaults();
	}
}

OrientAlongVelocityModifierParams::~OrientAlongVelocityModifierParams()
{
	freeStrings();
	freeReferences();
	freeDynamicArrays();
}

void OrientAlongVelocityModifierParams::destroy()
{
	// We cache these fields here to avoid overwrite in destructor
	bool doDeallocateSelf = mDoDeallocateSelf;
	NxParameterized::Traits* traits = mParameterizedTraits;
	physx::PxI32* refCount = mRefCount;
	void* buf = mBuffer;

	this->~OrientAlongVelocityModifierParams();

	NxParameters::destroy(this, traits, doDeallocateSelf, refCount, buf);
}

const NxParameterized::DefinitionImpl* OrientAlongVelocityModifierParams::getParameterDefinitionTree(void)
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

const NxParameterized::DefinitionImpl* OrientAlongVelocityModifierParams::getParameterDefinitionTree(void) const
{
	OrientAlongVelocityModifierParams* tmpParam = const_cast<OrientAlongVelocityModifierParams*>(this);

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

NxParameterized::ErrorType OrientAlongVelocityModifierParams::getParameterHandle(const char* long_name, Handle& handle) const
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

NxParameterized::ErrorType OrientAlongVelocityModifierParams::getParameterHandle(const char* long_name, Handle& handle)
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

void OrientAlongVelocityModifierParams::getVarPtr(const Handle& handle, void*& ptr, size_t& offset) const
{
	ptr = getVarPtrHelper(&ParamLookupTable[0], const_cast<OrientAlongVelocityModifierParams::ParametersStruct*>(&parameters()), handle, offset);
}


/* Dynamic Handle Indices */

void OrientAlongVelocityModifierParams::freeParameterDefinitionTable(NxParameterized::Traits* traits)
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

void OrientAlongVelocityModifierParams::buildTree(void)
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

	// Initialize DefinitionImpl node: nodeIndex=1, longName="modelForward"
	{
		NxParameterized::DefinitionImpl* ParamDef = &ParamDefTable[1];
		ParamDef->init("modelForward", TYPE_VEC3, NULL, true);

#ifdef NX_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[1];
		static Hint* HintPtrTable[1] = { &HintTable[0], };
		HintTable[0].init("shortDescription", "Model forward", true);
		ParamDefTable[1].setHints((const NxParameterized::Hint**)HintPtrTable, 1);

#endif /* NX_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// SetChildren for: nodeIndex=0, longName=""
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(1);

		ParamDefTable[0].setChildren(Children, 1);
	}

	mBuiltFlag = true;

}
void OrientAlongVelocityModifierParams::initStrings(void)
{
}

void OrientAlongVelocityModifierParams::initDynamicArrays(void)
{
}

void OrientAlongVelocityModifierParams::initDefaults(void)
{

	freeStrings();
	freeReferences();
	freeDynamicArrays();
	modelForward = physx::PxVec3(init(0, 1, 0));

	initDynamicArrays();
	initStrings();
	initReferences();
}

void OrientAlongVelocityModifierParams::initReferences(void)
{
}

void OrientAlongVelocityModifierParams::freeDynamicArrays(void)
{
}

void OrientAlongVelocityModifierParams::freeStrings(void)
{
}

void OrientAlongVelocityModifierParams::freeReferences(void)
{
}

} // namespace iofx
} // namespace apex
} // namespace physx
