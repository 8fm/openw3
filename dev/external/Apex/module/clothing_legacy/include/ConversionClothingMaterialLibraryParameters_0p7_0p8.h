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

#ifndef CONVERSIONCLOTHINGMATERIALLIBRARYPARAMETERS_0P7_0P8H_H
#define CONVERSIONCLOTHINGMATERIALLIBRARYPARAMETERS_0P7_0P8H_H

#include "ParamConversionTemplate.h"
#include "ClothingMaterialLibraryParameters_0p7.h"
#include "ClothingMaterialLibraryParameters_0p8.h"

namespace physx
{
namespace apex
{
namespace legacy
{

typedef ParamConversionTemplate<ClothingMaterialLibraryParameters_0p7, ClothingMaterialLibraryParameters_0p8, 7, 8> ConversionClothingMaterialLibraryParameters_0p7_0p8Parent;

class ConversionClothingMaterialLibraryParameters_0p7_0p8: ConversionClothingMaterialLibraryParameters_0p7_0p8Parent
{
public:
	static NxParameterized::Conversion* Create(NxParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionClothingMaterialLibraryParameters_0p7_0p8));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionClothingMaterialLibraryParameters_0p7_0p8)(t) : 0;
	}

protected:
	ConversionClothingMaterialLibraryParameters_0p7_0p8(NxParameterized::Traits* t) : ConversionClothingMaterialLibraryParameters_0p7_0p8Parent(t) {}

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

		for (PxI32 i = 0; i < mNewData->materials.arraySizes[0]; i++)
		{
			mNewData->materials.buf[i].massScale = 0.0f;

			if (mNewData->materials.buf[i].solverFrequency < 20)
			{
				if (mNewData->materials.buf[i].solverIterations > 0)
				{
					mNewData->materials.buf[i].solverFrequency = mNewData->materials.buf[i].solverIterations * 50.0f;
				}
				else
				{
					mNewData->materials.buf[i].solverFrequency = 20;
				}
			}
		}

		return true;
	}
};

}
}
} // namespace physx::apex

#endif
