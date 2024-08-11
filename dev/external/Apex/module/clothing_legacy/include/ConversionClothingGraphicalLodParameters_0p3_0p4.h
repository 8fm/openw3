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

#ifndef CONVERSIONCLOTHINGGRAPHICALLODPARAMETERS_0P3_0P4H_H
#define CONVERSIONCLOTHINGGRAPHICALLODPARAMETERS_0P3_0P4H_H

#include "ParamConversionTemplate.h"
#include "ClothingGraphicalLodParameters_0p3.h"
#include "ClothingGraphicalLodParameters_0p4.h"

namespace physx
{
namespace apex
{
namespace legacy
{

typedef ParamConversionTemplate<ClothingGraphicalLodParameters_0p3, ClothingGraphicalLodParameters_0p4, 3, 4> ConversionClothingGraphicalLodParameters_0p3_0p4Parent;

class ConversionClothingGraphicalLodParameters_0p3_0p4: ConversionClothingGraphicalLodParameters_0p3_0p4Parent
{
public:
	static NxParameterized::Conversion* Create(NxParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionClothingGraphicalLodParameters_0p3_0p4));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionClothingGraphicalLodParameters_0p3_0p4)(t) : 0;
	}

protected:
	ConversionClothingGraphicalLodParameters_0p3_0p4(NxParameterized::Traits* t) : ConversionClothingGraphicalLodParameters_0p3_0p4Parent(t) {}

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

		PxF32 meshThickness = mLegacyData->skinClothMapThickness;
		NxParameterized::Handle skinClothMap(*mNewData, "skinClothMap");
		skinClothMap.resizeArray(mLegacyData->skinClothMapC.arraySizes[0]);
		for (PxI32 i = 0; i < mLegacyData->skinClothMapC.arraySizes[0]; ++i)
		{
			const ClothingGraphicalLodParameters_0p3NS::SkinClothMapC_Type& mapC = mLegacyData->skinClothMapC.buf[i];
			ClothingGraphicalLodParameters_0p4NS::SkinClothMapD_Type& mapD = mNewData->skinClothMap.buf[i];

			mapD.vertexBary = mapC.vertexBary;
			mapD.vertexBary.z *= meshThickness;
			mapD.normalBary = mapC.normalBary;
			mapD.normalBary.z *= meshThickness;
			mapD.tangentBary = PxVec3(PX_MAX_F32); // mark tangents as invalid
			mapD.vertexIndexPlusOffset = mapC.vertexIndexPlusOffset;
			//PX_ASSERT((PxU32)i == mapC.vertexIndexPlusOffset);

			// Temporarily store the face index. The ClothingAsset update will look up the actual vertex indices.
			mapD.vertexIndex0 = mapC.faceIndex0;
		}

		return true;
	}
};

}
}
} // namespace physx::apex

#endif
