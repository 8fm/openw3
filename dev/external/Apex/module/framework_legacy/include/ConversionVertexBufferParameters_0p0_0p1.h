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

#ifndef __CONVERSIONVERTEXBUFFERPARAMETERS_0P0_0P1H__
#define __CONVERSIONVERTEXBUFFERPARAMETERS_0P0_0P1H__

#include "ParamConversionTemplate.h"
#include "VertexBufferParameters_0p0.h"
#include "VertexBufferParameters_0p1.h"

#include "VertexFormatParameters.h"

#include <BufferF32x1.h>
#include <BufferU16x1.h>

#include <PsUtilities.h>

namespace physx
{
namespace apex
{
namespace legacy
{

typedef ParamConversionTemplate<VertexBufferParameters_0p0, VertexBufferParameters_0p1, 0, 1> ConversionVertexBufferParameters_0p0_0p1Parent;

class ConversionVertexBufferParameters_0p0_0p1: ConversionVertexBufferParameters_0p0_0p1Parent
{
public:
	static NxParameterized::Conversion* Create(NxParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionVertexBufferParameters_0p0_0p1));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionVertexBufferParameters_0p0_0p1)(t) : 0;
	}

protected:
	ConversionVertexBufferParameters_0p0_0p1(NxParameterized::Traits* t) : ConversionVertexBufferParameters_0p0_0p1Parent(t) {}

	const NxParameterized::PrefVer* getPreferredVersions() const
	{
		static NxParameterized::PrefVer prefVers[] =
		{
			//TODO:
			//	Add your preferred versions for included references here.
			//	Entry format is
			//		{ (const char *)longName, (PxU32)preferredVersion }

			{ "vertexFormat", 0 },
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



		// if one of these fails it means that they have been upgraded. Then the proper (_0p0.h) files need to be included
		// and the types also need the _0p0 suffix. Then it should work again.
		PX_COMPILE_TIME_ASSERT(VertexFormatParameters::ClassVersion == 0);
		PX_COMPILE_TIME_ASSERT(BufferF32x1::ClassVersion == 0);
		PX_COMPILE_TIME_ASSERT(BufferU16x1::ClassVersion == 0);


		VertexFormatParameters* format = static_cast<VertexFormatParameters*>(mNewData->vertexFormat);

		PxI32 boneWeightID = -1;
		PxI32 boneIndexID = -1;
		PxU32 numBoneWeights = 0;
		PxU32 numBoneIndices = 0;

		for (PxI32 formatID = 0; formatID < format->bufferFormats.arraySizes[0]; formatID++)
		{
			const PxU32 semantic = format->bufferFormats.buf[formatID].semantic;
			if (semantic == NxRenderVertexSemantic::BONE_INDEX)
			{
				boneIndexID = formatID;
				switch (format->bufferFormats.buf[formatID].format)
				{
				case NxRenderDataFormat::USHORT1:
					numBoneIndices = 1;
					break;
				case NxRenderDataFormat::USHORT2:
					numBoneIndices = 2;
					break;
				case NxRenderDataFormat::USHORT3:
					numBoneIndices = 3;
					break;
				case NxRenderDataFormat::USHORT4:
					numBoneIndices = 4;
					break;
				}
			}
			else if (semantic == NxRenderVertexSemantic::BONE_WEIGHT)
			{
				boneWeightID = formatID;
				switch (format->bufferFormats.buf[formatID].format)
				{
				case NxRenderDataFormat::FLOAT1:
					numBoneWeights = 1;
					break;
				case NxRenderDataFormat::FLOAT2:
					numBoneWeights = 2;
					break;
				case NxRenderDataFormat::FLOAT3:
					numBoneWeights = 3;
					break;
				case NxRenderDataFormat::FLOAT4:
					numBoneWeights = 4;
					break;
				}
			}
		}


		// sort bone weights
		if (numBoneIndices > 1 && numBoneWeights == numBoneIndices)
		{
			PxF32* boneWeightBuffer = static_cast<BufferF32x1*>(mNewData->buffers.buf[boneWeightID])->data.buf;
			PxU16* boneIndexBuffer = static_cast<BufferU16x1*>(mNewData->buffers.buf[boneIndexID])->data.buf;

			for (PxU32 vi = 0; vi < mNewData->vertexCount; vi++)
			{
				PxF32* verifyWeights = boneWeightBuffer + vi * numBoneWeights;
				PxU16* verifyIndices = boneIndexBuffer + vi * numBoneWeights;

				PxF32 sum = 0.0f;
				for (PxU32 j = 0; j < numBoneWeights; j++)
				{
					sum += verifyWeights[j];
				}

				if (PxAbs(1 - sum) > 0.001)
				{
					if (sum > 0.0f)
					{
						for (PxU32 j = 0; j < numBoneWeights; j++)
						{
							verifyWeights[j] /= sum;
						}
					}
				}

				// PH: bubble sort, don't kill me for this
				for (PxU32 j = 1; j < numBoneWeights; j++)
				{
					for (PxU32 k = 1; k < numBoneWeights; k++)
					{
						if (verifyWeights[k - 1] < verifyWeights[k])
						{
							physx::swap(verifyWeights[k - 1], verifyWeights[k]);
							physx::swap(verifyIndices[k - 1], verifyIndices[k]);
						}
					}
				}
			}
		}

		return true;
	}
};

}
}
} //end of physx::apex:: namespace

#endif
