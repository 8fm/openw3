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

#ifndef CONVERSIONDESTRUCTIBLEACTORPARAM_0P16_0P17H_H
#define CONVERSIONDESTRUCTIBLEACTORPARAM_0P16_0P17H_H

#include "ParamConversionTemplate.h"
#include "DestructibleActorParam_0p16.h"
#include "DestructibleActorParam_0p17.h"

namespace physx
{
namespace apex
{
namespace legacy
{

typedef ParamConversionTemplate<DestructibleActorParam_0p16, DestructibleActorParam_0p17, 16, 17> ConversionDestructibleActorParam_0p16_0p17Parent;

class ConversionDestructibleActorParam_0p16_0p17: ConversionDestructibleActorParam_0p16_0p17Parent
{
public:
	static NxParameterized::Conversion* Create(NxParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionDestructibleActorParam_0p16_0p17));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionDestructibleActorParam_0p16_0p17)(t) : 0;
	}

protected:
	ConversionDestructibleActorParam_0p16_0p17(NxParameterized::Traits* t) : ConversionDestructibleActorParam_0p16_0p17Parent(t) {}

	const NxParameterized::PrefVer* getPreferredVersions() const
	{
		static NxParameterized::PrefVer prefVers[] =
		{
			//TODO:
			//	Add your preferred versions for included references here.
			//	Entry format is
			//		{ (const char*)longName, (PxU32)preferredVersion }

			{ 0, 0 } // Terminator (do not remove!)
		};

		return prefVers;
	}

	bool convert()
	{
		// Convert default behavior group's damage spread function parameters
		mNewData->defaultBehaviorGroup.damageSpread.minimumRadius = mLegacyData->defaultBehaviorGroup.minimumDamageRadius;
		mNewData->defaultBehaviorGroup.damageSpread.radiusMultiplier = mLegacyData->defaultBehaviorGroup.damageRadiusMultiplier;
		mNewData->defaultBehaviorGroup.damageSpread.falloffExponent = mLegacyData->defaultBehaviorGroup.damageFalloffExponent;

		// Convert user-defined behavior group's damage spread function parameters
		const physx::PxI32 behaviorGroupCount = mLegacyData->behaviorGroups.arraySizes[0];
		PX_ASSERT(mNewData->behaviorGroups.arraySizes[0] == behaviorGroupCount);
		for (physx::PxI32 i = 0; i < physx::PxMin(behaviorGroupCount, mNewData->behaviorGroups.arraySizes[0]); ++i)
		{
			mNewData->behaviorGroups.buf[i].damageSpread.minimumRadius = mLegacyData->behaviorGroups.buf[i].minimumDamageRadius;
			mNewData->behaviorGroups.buf[i].damageSpread.radiusMultiplier = mLegacyData->behaviorGroups.buf[i].damageRadiusMultiplier;
			mNewData->behaviorGroups.buf[i].damageSpread.falloffExponent = mLegacyData->behaviorGroups.buf[i].damageFalloffExponent;
		}

		return true;
	}
};

}
}
} // namespace physx::apex

#endif
