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

#include "NxApex.h"
#include "EmitterGeom.h"

namespace physx
{
namespace apex
{
namespace emitter
{

/* Return percentage of new volume not covered by old volume */
physx::PxF32 EmitterGeom::computeNewlyCoveredVolume(
    const physx::PxMat34Legacy& oldPose,
    const physx::PxMat34Legacy& newPose,
    QDSRand& rand) const
{
	// estimate by sampling
	physx::PxU32 numSamples = 100;
	physx::PxU32 numOutsideOldVolume = 0;
	for (physx::PxU32 i = 0; i < numSamples; i++)
	{
		if (!isInEmitter(randomPosInFullVolume(newPose, rand), oldPose))
		{
			numOutsideOldVolume++;
		}
	}

	return (physx::PxF32) numOutsideOldVolume / numSamples;
}


// TODO make better, this is very slow when emitter moves slowly
// SJB: I'd go one further, this seems mildly retarted
physx::PxVec3 EmitterGeom::randomPosInNewlyCoveredVolume(const physx::PxMat34Legacy& pose, const physx::PxMat34Legacy& oldPose, QDSRand& rand) const
{
	physx::PxVec3 pos;
	do
	{
		pos = randomPosInFullVolume(pose, rand);
	}
	while (isInEmitter(pos, oldPose));
	return pos;
}

}
}
} // namespace physx::apex
