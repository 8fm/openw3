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

#include "NxApex.h"
#include "NxApexUtils.h"
#include "PsUserAllocated.h"
#include "ProfilerCallback.h"
#include "Module.h"

#ifdef PHYSX_PROFILE_SDK

#ifdef PX_WINDOWS
extern physx::PxProfileZone *gProfileZone;
#endif

#endif

namespace physx
{
namespace apex
{

Module::Module() :
	mSdk(0),
	mApiProxy(0),
	mLodEnabled(true),
	mLodUnitCost(1.0f),
	mLodBenefitValue(1.0f)
{
}

void Module::setIntValue(physx::PxU32 parameterIndex, physx::PxU32 value)
{
	NxApexParameter* lod = mParameters[parameterIndex];

	if (value >= lod->range.minimum && value <= lod->range.maximum)
	{
		lod->current = value;
	}
	else
	{
		APEX_INVALID_PARAMETER("value is out of range (%d not in [%d, %d])", value, lod->range.minimum, lod->range.maximum);
	}
}

physx::PxF32 Module::getCurrentValue(NxRange<physx::PxU32> range, physx::PxU32 parameterIndex, NxInterpolator* interp) const
{
	NxLerp defaultInterp;

	if (!interp)
	{
		interp = &defaultInterp;
	}

	NxApexParameter* lod = mParameters[parameterIndex];
	physx::PxF32 val = interp->interpolate((physx::PxF32)lod->current, (physx::PxF32)lod->range.minimum, (physx::PxF32)lod->range.maximum, (physx::PxF32) range.minimum, (physx::PxF32)range.maximum);

	return val;
}

physx::PxF32 Module::getCurrentValue(NxRange<physx::PxF32> range, physx::PxU32 parameterIndex, NxInterpolator* interp) const
{
	NxLerp localLerp;

	if (!interp)
	{
		interp = &localLerp;
	}

	NxApexParameter* lod = mParameters[parameterIndex];
	physx::PxF32 val = interp->interpolate((physx::PxF32)lod->current, (physx::PxF32)lod->range.minimum, (physx::PxF32)lod->range.maximum, range.minimum, range.maximum);

	return val;
}

const char* Module::getName() const
{
	return name.c_str();
}

void Module::registerLODParameter(const char* name, NxRange<physx::PxU32> range)
{
	PX_ASSERT(mSdk);
	PX_ASSERT(mApiProxy);

	physx::PxU32 index = 0;
	for (; index < allNames.size(); index++)
	{
		if (strcmp(allNames[index], name) == 0)
		{
			break;
		}
	}
	if (index >= allNames.size())
	{
		index = allNames.size();
		size_t len = strlen(name);
		char* temp = (char*)PX_ALLOC(len + 1, PX_DEBUG_EXP("Module::registerLODParameter"));

#if defined(WIN32)
		strncpy_s(temp, len + 1, name, len);
#else
		strcpy(temp, name);
#endif
		allNames.pushBack(temp);
	}
	PX_ASSERT(index < allNames.size());

	NxApexParameter* p = static_cast<NxApexParameter*>(PX_ALLOC(sizeof(NxApexParameter), PX_DEBUG_EXP("Module::registerLODParameter")));
	PX_PLACEMENT_NEW(p, NxApexParameter)(allNames[index], range);
	mParameters.pushBack(p);
}

void Module::release()
{
	NxGetApexSDK()->releaseModule(mApiProxy);
}

void Module::destroy()
{
	for (physx::PxU32  i = 0; i < mParameters.size(); i++)
	{
		PX_FREE(mParameters[i]);
	}

	for (physx::PxU32 i = 0; i < allNames.size(); i++)
	{
		PX_FREE(allNames[i]);
		allNames[i] = NULL;
	}
}

physx::PxU32 Module::getNbParameters() const
{
	return mParameters.size();
}

NxApexParameter** Module::getParameters()
{
	if (mParameters.size() > 0)
	{
		return &mParameters.front();
	}
	else
	{
		return NULL;
	}
}

void Module::setLODUnitCost(physx::PxF32 cost)
{
	mLodUnitCost = cost;
}

physx::PxF32 Module::getLODUnitCost() const
{
	return mLodUnitCost;
}

physx::PxF32 Module::getLODBenefitValue() const
{
	return mLodBenefitValue;
}

void Module::setLODBenefitValue(physx::PxF32 value)
{
	mLodBenefitValue = value;
}

bool Module::getLODEnabled() const
{
	return mLodEnabled;
}

void Module::setLODEnabled(bool enabled)
{
	mLodEnabled = enabled;
}

}
} // end namespace physx::apex
