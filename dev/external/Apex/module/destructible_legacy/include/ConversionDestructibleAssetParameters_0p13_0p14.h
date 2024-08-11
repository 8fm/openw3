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

#ifndef CONVERSIONDESTRUCTIBLEASSETPARAMETERS_0P13_0P14H_H
#define CONVERSIONDESTRUCTIBLEASSETPARAMETERS_0P13_0P14H_H

#include "ParamConversionTemplate.h"
#include "DestructibleAssetParameters_0p13.h"
#include "DestructibleAssetParameters_0p14.h"

#include <algorithm> // for find_if

namespace physx
{
namespace apex
{
namespace legacy
{

typedef ParamConversionTemplate<DestructibleAssetParameters_0p13, DestructibleAssetParameters_0p14, 13, 14> ConversionDestructibleAssetParameters_0p13_0p14Parent;

class ConversionDestructibleAssetParameters_0p13_0p14: ConversionDestructibleAssetParameters_0p13_0p14Parent
{
public:
	static NxParameterized::Conversion* Create(NxParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionDestructibleAssetParameters_0p13_0p14));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionDestructibleAssetParameters_0p13_0p14)(t) : 0;
	}

protected:
	ConversionDestructibleAssetParameters_0p13_0p14(NxParameterized::Traits* t) : ConversionDestructibleAssetParameters_0p13_0p14Parent(t) {}

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
		typedef DestructibleAssetParameters_0p14NS::BehaviorGroup_Type BehaviorGroup;

		const PxI8 DEFAULT_GROUP = -1;
		mNewData->RTFractureBehaviorGroup = DEFAULT_GROUP;
		mNewData->defaultBehaviorGroup.damageThreshold = mLegacyData->destructibleParameters.damageThreshold;
		mNewData->defaultBehaviorGroup.damageToRadius = mLegacyData->destructibleParameters.damageToRadius;
		mNewData->defaultBehaviorGroup.materialStrength = mLegacyData->destructibleParameters.materialStrength;
		
		PX_ASSERT(mLegacyData->chunks.arraySizes[0] == mNewData->chunks.arraySizes[0]);
		for (PxI32 chunkIndex = 0; chunkIndex < mLegacyData->chunks.arraySizes[0]; ++chunkIndex)
		{
			DestructibleAssetParameters_0p14NS::Chunk_Type& newChunk = mNewData->chunks.buf[chunkIndex];
			newChunk.behaviorGroupIndex = DEFAULT_GROUP;
		}

		return true;
	}
};

}
}
} // namespace physx::apex

#endif
