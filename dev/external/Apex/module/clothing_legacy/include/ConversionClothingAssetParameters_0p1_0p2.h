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

#ifndef __CONVERSIONCLOTHINGASSETPARAMETERS_0P1_0P2H__
#define __CONVERSIONCLOTHINGASSETPARAMETERS_0P1_0P2H__

#include "ParamConversionTemplate.h"
#include "ClothingAssetParameters_0p1.h"
#include "ClothingAssetParameters_0p2.h"

#include "ClothingPhysicalMeshParameters_0p2.h"
#include "ClothingCookedParam_0p1.h"
#include <NxParamArray.h>

namespace physx
{
namespace apex
{
namespace legacy
{

typedef ParamConversionTemplate<ClothingAssetParameters_0p1, ClothingAssetParameters_0p2, 1, 2> ConversionClothingAssetParameters_0p1_0p2Parent;

class ConversionClothingAssetParameters_0p1_0p2: ConversionClothingAssetParameters_0p1_0p2Parent
{
public:
	static NxParameterized::Conversion* Create(NxParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionClothingAssetParameters_0p1_0p2));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionClothingAssetParameters_0p1_0p2)(t) : 0;
	}

protected:
	ConversionClothingAssetParameters_0p1_0p2(NxParameterized::Traits* t) : ConversionClothingAssetParameters_0p1_0p2Parent(t) {}

	const NxParameterized::PrefVer* getPreferredVersions() const
	{
		static NxParameterized::PrefVer preferredVers[] =
		{
			{ "physicalMeshes[]", 2 },
			{ 0, 0}
		};

		return preferredVers;
	}

	bool convert()
	{
		//TODO:
		//	write custom conversion code here using mNewData and mLegacyData members
		//	note that members with same names were already copied in parent converter
		// and mNewData was already initialized with default values

		ClothingCookedParam_0p1* cookedScale1 = (ClothingCookedParam_0p1*)mNewData->getTraits()->createNxParameterized("ClothingCookedParam", 1);

		NxParamArray<PxU8> deformableCookedData(cookedScale1, "deformableCookedData", reinterpret_cast<NxParamDynamicArrayStruct*>(&cookedScale1->deformableCookedData));
		NxParamArray<ClothingCookedParam_0p1NS::CookedPhysicalSubmesh_Type> cookedPhysicalSubmeshes(cookedScale1, "cookedPhysicalSubmeshes", reinterpret_cast<NxParamDynamicArrayStruct*>(&cookedScale1->cookedPhysicalSubmeshes));


		for (PxI32 physicalMeshID = 0; physicalMeshID < mNewData->physicalMeshes.arraySizes[0]; physicalMeshID++)
		{
			NxParameterized::Interface* iface = mNewData->physicalMeshes.buf[physicalMeshID];
			if (iface->version() == 2)
			{
				ClothingPhysicalMeshParameters_0p2* physicalMesh = static_cast<ClothingPhysicalMeshParameters_0p2*>(iface);

				PxU32 offset = deformableCookedData.size();
				deformableCookedData.resize(offset + physicalMesh->deformableCookedData.arraySizes[0]);
				memcpy(deformableCookedData.begin() + offset, physicalMesh->deformableCookedData.buf, physicalMesh->deformableCookedData.arraySizes[0]);

				PX_ASSERT(cookedScale1->cookedDataVersion == 0 || physicalMesh->deformableCookedDataVersion == 0 || cookedScale1->cookedDataVersion == physicalMesh->deformableCookedDataVersion);
				cookedScale1->cookedDataVersion = physicalMesh->deformableCookedDataVersion;

				for (PxI32 submeshID = 0; submeshID < physicalMesh->submeshes.arraySizes[0]; submeshID++)
				{
					ClothingPhysicalMeshParameters_0p2NS::PhysicalSubmesh_Type& oldSubmesh = physicalMesh->submeshes.buf[submeshID];

					ClothingCookedParam_0p1NS::CookedPhysicalSubmesh_Type submesh;
					submesh.cookedDataLength = oldSubmesh.cookedDataLength;
					submesh.cookedDataOffset = oldSubmesh.cookedDataOffset + offset;
					submesh.deformableMeshPointer = NULL;
					submesh.physicalMeshId = physicalMeshID;
					submesh.submeshId = submeshID;

					cookedPhysicalSubmeshes.pushBack(submesh);
				}
			}
		}

		// copy convex cooked data
		if (cookedScale1 != NULL)
		{
			NxParameterized::Handle handle(*cookedScale1, "convexCookedData");
			PX_ASSERT(handle.isValid());
			handle.resizeArray(mLegacyData->convexesCookedData.arraySizes[0]);
			memcpy(cookedScale1->convexCookedData.buf, mLegacyData->convexesCookedData.buf, sizeof(PxU8) * mLegacyData->convexesCookedData.arraySizes[0]);
		}

		if (deformableCookedData.isEmpty())
		{
			cookedScale1->destroy();
			cookedScale1 = NULL;
		}

		if (cookedScale1 != NULL)
		{
			if (cookedScale1->cookedDataVersion == 0)
			{
				cookedScale1->cookedDataVersion = NX_SDK_VERSION_NUMBER;
			}

			PX_ASSERT(mNewData->cookedData.arraySizes[0] == 0);
			NxParameterized::Handle cookedData(*mNewData, "cookedData");
			PX_ASSERT(cookedData.isValid());
			cookedData.resizeArray(1);
			mNewData->cookedData.buf[0].scale = 1.0f;
			mNewData->cookedData.buf[0].cookedData = cookedScale1;
		}

		return true;
	}
};

}
}
} //end of physx::apex:: namespace

#endif
