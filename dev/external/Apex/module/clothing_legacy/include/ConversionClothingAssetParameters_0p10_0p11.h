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

#ifndef CONVERSIONCLOTHINGASSETPARAMETERS_0P10_0P11H_H
#define CONVERSIONCLOTHINGASSETPARAMETERS_0P10_0P11H_H

#include "ParamConversionTemplate.h"
#include "ClothingAssetParameters_0p10.h"
#include "ClothingAssetParameters_0p11.h"

#include "ApexSharedUtils.h"

namespace physx
{
namespace apex
{
namespace legacy
{

typedef ParamConversionTemplate<ClothingAssetParameters_0p10, ClothingAssetParameters_0p11, 10, 11> ConversionClothingAssetParameters_0p10_0p11Parent;

class ConversionClothingAssetParameters_0p10_0p11: ConversionClothingAssetParameters_0p10_0p11Parent
{
public:
	static NxParameterized::Conversion* Create(NxParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionClothingAssetParameters_0p10_0p11));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionClothingAssetParameters_0p10_0p11)(t) : 0;
	}

protected:
	ConversionClothingAssetParameters_0p10_0p11(NxParameterized::Traits* t) : ConversionClothingAssetParameters_0p10_0p11Parent(t) {}

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
		//TODO:
		//	Write custom conversion code here using mNewData and mLegacyData members.
		//
		//	Note that
		//		- mNewData has already been initialized with default values
		//		- same-named/same-typed members have already been copied
		//			from mLegacyData to mNewData
		//		- included references were moved to mNewData
		//			(and updated to preferred versions according to getPreferredVersions)
		//
		//	For more info see the versioning wiki.

		// extract planes from convex collision boneActors and store in bonePlanes
		Array<ClothingAssetParameters_0p11NS::BonePlane_Type> bonePlanes;
		Array<PxU32> convexes;
		for (PxI32 i = 0; i < mLegacyData->boneActors.arraySizes[0]; ++i)
		{
			ClothingAssetParameters_0p10NS::ActorEntry_Type& boneActor = mLegacyData->boneActors.buf[i];
			if (boneActor.convexVerticesCount > 0)
			{
				// extract planes from points
				ConvexHull convexHull;
				convexHull.init();

				convexHull.buildFromPoints(&mLegacyData->boneVertices.buf[boneActor.convexVerticesStart], boneActor.convexVerticesCount, sizeof(PxVec3));
				PxU32 planeCount = convexHull.getPlaneCount();
				if (planeCount + bonePlanes.size() > 32)
				{
					APEX_DEBUG_WARNING("The asset contains more than 32 planes for collision convexes, some of them will not be simulated with 3.x cloth.");
					break;
				}
				else
				{
					PxU32 convex = 0; // each bit references a plane
					for (PxU32 i = 0; i < planeCount; ++i)
					{
						PxPlane plane = convexHull.getPlane(i);
						convex |= 1 << bonePlanes.size();

						ClothingAssetParameters_0p11NS::BonePlane_Type newEntry;
						memset(&newEntry, 0, sizeof(ClothingAssetParameters_0p11NS::BonePlane_Type));
						newEntry.boneIndex = boneActor.boneIndex;
						newEntry.n = plane.n;
						newEntry.d = plane.d;

						bonePlanes.pushBack(newEntry);
					}

					convexes.pushBack(convex);
				}
			}
		}

		NxParameterized::Handle bonePlanesHandle(*mNewData, "bonePlanes");
		mNewData->resizeArray(bonePlanesHandle, bonePlanes.size());
		for (PxU32 i = 0; i < bonePlanes.size(); ++i)
		{
			mNewData->bonePlanes.buf[i] = bonePlanes[i];
		}

		NxParameterized::Handle convexesHandle(*mNewData, "collisionConvexes");
		mNewData->resizeArray(convexesHandle, convexes.size());
		for (PxU32 i = 0; i < convexes.size(); ++i)
		{
			mNewData->collisionConvexes.buf[i] = convexes[i];
		}
		return true;
	}
};

}
}
} // namespace physx::apex

#endif
