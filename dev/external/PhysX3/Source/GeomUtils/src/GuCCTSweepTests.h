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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef GU_CCT_SWEEP_TESTS_H
#define GU_CCT_SWEEP_TESTS_H

#include "GuSweepTests.h"

namespace physx
{


#define USE_SAT_VERSION_CCT			// PT: the SAT-based sweep for this function, almost exclusively used by the CCT
#define USE_SAT_VERSION				// PT: uses SAT-based sweep test instead of feature-based.

#define PRECOMPUTE_FAT_BOX			// PT: TODO: clean this up
#define PRECOMPUTE_FAT_BOX_MORE


namespace Gu
{


	PX_PHYSX_COMMON_API bool sweepCCTCapsule_BoxGeom		(GU_CAPSULE_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepCCTBox_SphereGeom			(GU_BOX_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepCCTBox_CapsuleGeom		(GU_BOX_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepCCTBox_BoxGeom			(GU_BOX_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepCCTBox_MeshGeom			(GU_BOX_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepCCTBox_HeightFieldGeom	(GU_BOX_SWEEP_FUNC_PARAMS);


	PX_PHYSX_COMMON_API const SweepCapsuleFunc* GetSweepCCTCapsuleMap();
	extern const SweepCapsuleFunc gSweepCCTCapsuleMap[7];

	PX_PHYSX_COMMON_API const SweepBoxFunc* GetSweepCCTBoxMap();
	extern const SweepBoxFunc gSweepCCTBoxMap[7];


	// For CCT sweep vs. triangle list: PxGeometryQuery::sweep()
	PX_PHYSX_COMMON_API bool SweepCCTBoxTriangles(PxU32 nb_tris, const PxTriangle* triangles,
							const PxBoxGeometry& boxGeom, const PxTransform& boxPose, const PxVec3& unitDir, const PxReal distance, 
							PxVec3& _hit, PxVec3& _normal, float& _d, PxU32& _index, const PxU32* cachedIndex, const PxReal inflation, PxHitFlags hintFlags);


}  // namespace Gu

}

#endif
