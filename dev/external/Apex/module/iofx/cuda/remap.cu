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


#include "common.cuh"

#include "include/common.h"
using namespace physx::apex;
using namespace physx::apex::iofx;
#include "include/remap.h"

__device__
PX_INLINE unsigned int floatFlip(float f)
{
    unsigned int i = __float_as_int(f);
	unsigned int mask = -int(i >> 31) | 0x80000000;
	return i ^ mask;
}

__device__
PX_INLINE physx::PxU32 getActorIndex(physx::PxU32 inputID, const NiIofxActorID* actorID, physx::PxU32 numActorClasses, physx::PxU32 numActorIDs)
{
	NiIofxActorID id;
	id.value = tex1Dfetch(KERNEL_TEX_REF(RemapActorIDs), inputID);

	return (id.getVolumeID() == NiIofxActorID::NO_VOLUME) ? (numActorIDs << NiIofxActorID::ASSETS_PER_MATERIAL_BITS)
		: numActorClasses * id.getVolumeID() + id.getActorClassID();
}

/* State Space */
BOUND_KERNEL_BEG( REMAP_WARPS_PER_BLOCK, makeSortKeys,
	const physx::PxU32* inStateToInput, physx::PxU32 maxInputID,
	const NiIofxActorID* actorID, physx::PxU32 numActorClasses, physx::PxU32 numActorIDs,
	const float4* positionMass, bool outputDensityKeys,
	physx::PxVec3 eyePos, physx::PxVec3 eyeDir, physx::PxF32 zNear,
	physx::PxU32* sortKey, physx::PxU32* sortValue
)
	const physx::PxU32 maxStateID = _threadCount;
	for (physx::PxU32 stateID = BlockSize*blockIdx.x + threadIdx.x; stateID < maxStateID; stateID += BlockSize*gridDim.x)
	{
		physx::PxU32 key = outputDensityKeys ? 0xFFFFFFFFu : ((numActorIDs + 1) << NiIofxActorID::ASSETS_PER_MATERIAL_BITS);
		physx::PxU32 value = stateID;

		physx::PxU32 inputID = inStateToInput[ stateID ];
		inputID &= ~NiIosBufferDesc::NEW_PARTICLE_FLAG;
		if (inputID < maxInputID) //this will check also that (inputID != NiIosBufferDesc::NOT_A_PARTICLE)
		{
			if (outputDensityKeys)
			{
				const float4 pos4 = tex1Dfetch(KERNEL_TEX_REF(RemapPositions), inputID);
				const physx::PxVec3 pos = physx::PxVec3(pos4.x, pos4.y, pos4.z);
				const float dist = zNear + (eyePos - pos).dot(eyeDir);
				key = floatFlip( dist );

				//store distance sign in the highest bit of value
				value |= (key & STATE_ID_DIST_SIGN);
			}
			else
			{
				key = getActorIndex(inputID, actorID, numActorClasses, numActorIDs);
			}
		}
		sortKey[ stateID ] = key;
		sortValue[ stateID ] = value;
	}
BOUND_KERNEL_END()


/* Sorted State Space */
BOUND_KERNEL_BEG( REMAP_WARPS_PER_BLOCK, remapKernel,
	const physx::PxU32* inStateToInput, physx::PxU32 maxInputID,
	const NiIofxActorID* actorID, physx::PxU32 numActorClasses, physx::PxU32 numActorIDs,
	const physx::PxU32* inSortedValue, physx::PxU32* outSortKey
)
	const physx::PxU32 maxStateID = _threadCount;
	for (physx::PxU32 stateID = BlockSize*blockIdx.x + threadIdx.x; stateID < maxStateID; stateID += BlockSize*gridDim.x)
	{
		physx::PxU32 actorIndex = ((numActorIDs + 1) << NiIofxActorID::ASSETS_PER_MATERIAL_BITS);

		const physx::PxU32 sortedStateID = (inSortedValue[ stateID ] & STATE_ID_MASK);
		// sortedStateID should be < maxStateID
		physx::PxU32 inputID = tex1Dfetch(KERNEL_TEX_REF(RemapInStateToInput), sortedStateID);
		inputID &= ~NiIosBufferDesc::NEW_PARTICLE_FLAG;
		if (inputID < maxInputID) //this will check also that (inputID != NiIosBufferDesc::NOT_A_PARTICLE)
		{
			actorIndex = getActorIndex(inputID, actorID, numActorClasses, numActorIDs);
		}

		outSortKey[ stateID ] = actorIndex;
	}
BOUND_KERNEL_END()
