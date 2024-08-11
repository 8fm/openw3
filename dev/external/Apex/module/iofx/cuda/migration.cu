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
#include "include/migration.h"


__device__
bool contains( const physx::PxBounds3& b, const physx::PxVec3& v )
{
	return !(v.x < b.minimum.x || v.x > b.maximum.x ||
			 v.y < b.minimum.y || v.y > b.maximum.y ||
			 v.z < b.minimum.z || v.z > b.maximum.z);
}

/* Input Space */
BOUND_KERNEL_BEG( VOLUME_MIGRATION_WARPS_PER_BLOCK, volumeMigrationKernel,
	InplaceHandle<VolumeParamsArray> volumeParamsArrayHandle,
	InplaceHandle<ActorIDBitmapArray> actorIDBitmapArrayHandle,
	physx::PxU32 numActorClasses, physx::PxU32 numVolumes,
	NiIofxActorID* actorID, physx::PxU32 maxInputID,
	const float4* positionMass, 
	physx::PxU32* actorStart, physx::PxU32* actorEnd, physx::PxU32* actorVisibleEnd
)
	for (unsigned int input = BlockSize*blockIdx.x + threadIdx.x; input < maxInputID; input += BlockSize*gridDim.x)
	{
		NiIofxActorID id = actorID[ input ];
		const float4 pos4 = positionMass[ input ];
		const physx::PxVec3 pos = physx::PxVec3(pos4.x, pos4.y, pos4.z);

		physx::PxU32 bit = id.getActorClassID();
		if (bit == NiIofxActorID::INV_ACTOR || bit >= numActorClasses)
		{
			id.set( NiIofxActorID::NO_VOLUME, NiIofxActorID::INV_ACTOR );
		}
		else
		{
			physx::PxU32 curPri = 0;
			physx::PxU32 curVID = NiIofxActorID::NO_VOLUME;
			
			const VolumeParamsArray& volumeParamsArray = *volumeParamsArrayHandle.resolve( KERNEL_CONST_MEM(volumeConstMem) );
			//const physx::PxU32 numVolumes = volumeParamsArray.getSize();
			const VolumeParams* volumeParams = volumeParamsArray.getElems( KERNEL_CONST_MEM(volumeConstMem) );

			const ActorIDBitmapArray& actorIDBitmapArray = *actorIDBitmapArrayHandle.resolve( KERNEL_CONST_MEM(volumeConstMem) );
			const physx::PxU32* iofxActorBitmap = actorIDBitmapArray.getElems( KERNEL_CONST_MEM(volumeConstMem) );

			for (physx::PxU32 i = 0 ; i < numVolumes ; i++)
			{
				const physx::PxBounds3& b = volumeParams[i].bounds;
				const physx::PxU32 pri = volumeParams[i].priority;

				// This volume owns this particle if:
				//  1. The volume bounds contain the particle
				//  2. The volume affects the particle's IOFX Asset
				//  3. This volume has the highest priority or was the previous owner
				if ( contains( b, pos ) &&
				     (iofxActorBitmap[ bit >> 5 ] & (1u << (bit & 31))) &&
				     (curVID == NiIofxActorID::NO_VOLUME || pri > curPri || (pri == curPri && id.getVolumeID() == i)) )
				{
					curVID = i;
					curPri = pri;
				}

				bit += numActorClasses;
			}

			id.setVolumeID( curVID );
		}
		actorID[ input ] = id;
	}

	// Clear actorID start/stop table
	const physx::PxU32 numActorIDValues = ((numActorClasses >> NiIofxActorID::ASSETS_PER_MATERIAL_BITS) * numVolumes) + 2;
	for (physx::PxU32 idx = BlockSize*blockIdx.x + threadIdx.x; idx < numActorIDValues; idx += BlockSize*gridDim.x)
	{
		actorStart[ idx ] = 0;
		actorEnd[ idx ] = 0;
		actorVisibleEnd[ idx ] = 0;
	}
		
BOUND_KERNEL_END()
