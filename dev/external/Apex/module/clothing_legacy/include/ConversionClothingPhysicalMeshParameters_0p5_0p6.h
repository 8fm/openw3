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

#ifndef __CONVERSIONCLOTHINGPHYSICALMESHPARAMETERS_0P5_0P6H__
#define __CONVERSIONCLOTHINGPHYSICALMESHPARAMETERS_0P5_0P6H__

#include "ParamConversionTemplate.h"
#include "ClothingPhysicalMeshParameters_0p5.h"
#include "ClothingPhysicalMeshParameters_0p6.h"

namespace physx
{
namespace apex
{
namespace legacy
{

typedef ParamConversionTemplate<ClothingPhysicalMeshParameters_0p5, ClothingPhysicalMeshParameters_0p6, 5, 6> ConversionClothingPhysicalMeshParameters_0p5_0p6Parent;

class ConversionClothingPhysicalMeshParameters_0p5_0p6: ConversionClothingPhysicalMeshParameters_0p5_0p6Parent
{
public:
	static NxParameterized::Conversion* Create(NxParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionClothingPhysicalMeshParameters_0p5_0p6));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionClothingPhysicalMeshParameters_0p5_0p6)(t) : 0;
	}

protected:
	ConversionClothingPhysicalMeshParameters_0p5_0p6(NxParameterized::Traits* t) : ConversionClothingPhysicalMeshParameters_0p5_0p6Parent(t) {}

	const NxParameterized::PrefVer* getPreferredVersions() const
	{
		static NxParameterized::PrefVer prefVers[] =
		{
			//TODO:
			//	Add your preferred versions for included references here.
			//	Entry format is
			//		{ (const char *)longName, (PxU32)preferredVersion }

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
		//	For more info see the versioning wiki

		const PxI32 numVertices = mLegacyData->physicalMesh.vertices.arraySizes[0];
		const PxU32 numBonesPerVertex = mLegacyData->physicalMesh.numBonesPerVertex;

		const PxF32* boneWeights = mNewData->physicalMesh.boneWeights.buf;

		PxU32 numVerticesPerCacheBlock = 8;// 128 / sizeof boneWeights per vertex (4 PxF32), this is the biggest per vertex data
		PxU32 allocNumVertices = ((PxU32)ceil((PxF32)numVertices / numVerticesPerCacheBlock)) * numVerticesPerCacheBlock; // allocate more to have a multiple of numVerticesPerCachBlock

		NxParameterized::Handle optimizationDataHandle(*mNewData, "physicalMesh.optimizationData");
		PX_ASSERT(optimizationDataHandle.isValid());
		optimizationDataHandle.resizeArray((allocNumVertices + 1) / 2);
		PxU8* optimizationData = mNewData->physicalMesh.optimizationData.buf;
		memset(optimizationData, 0, (allocNumVertices + 1) / 2);

		const ClothingPhysicalMeshParameters_0p6NS::ConstrainCoefficient_Type* constrainCoeffs = mNewData->physicalMesh.constrainCoefficients.buf;

		for (PxI32 i = 0; i < numVertices; ++i)
		{
			PxU8 numBones = 0;
			while (numBones < numBonesPerVertex)
			{
				if (boneWeights[numBonesPerVertex * i + numBones] == 0.0f)
				{
					break;
				}

				++numBones;
			}


			PxU8& data = optimizationData[i / 2];
			PX_ASSERT(numBones < 8);
			PxU8 bitShift = 0;
			if (i % 2 == 0)
			{
				data = 0;
			}
			else
			{
				bitShift = 4;
			}
			data |= numBones << bitShift;

			// store for each vertex if collisionSphereDistance is < 0
			if (constrainCoeffs[i].collisionSphereDistance < 0.0f)
			{
				data |= 8 << bitShift;
				mNewData->physicalMesh.hasNegativeBackstop = true;
			}
			else
			{
				data &= ~(8 << bitShift);
			}
		}

		return true;
	}
};

}
}
} //end of physx::apex:: namespace

#endif
