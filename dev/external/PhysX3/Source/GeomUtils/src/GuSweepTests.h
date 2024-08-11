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

#ifndef GU_SWEEP_TESTS_H
#define GU_SWEEP_TESTS_H

#include "CmPhysXCommon.h"
#include "PxQueryReport.h"

namespace physx
{

class PxGeometry;
class PxBoxGeometry;
class PxCapsuleGeometry;
class PxSphereGeometry;
class PxPlaneGeometry;
class PxConvexMeshGeometry;
class PxTriangleMeshGeometry;
class PxHeightFieldGeometry;
class PxMeshScale;
class PxTriangle;
struct PxSweepHit;

namespace Gu
{
	class Capsule;
	class Box;

#define GU_CAPSULE_SWEEP_FUNC_PARAMS	const PxGeometry& geom,	const PxTransform& pose, \
		const Gu::Capsule& lss, const PxVec3& unitDir, const PxReal distance, \
		PxSweepHit& sweepHit, PxHitFlags hintFlags, const PxReal inflation 

#define GU_BOX_SWEEP_FUNC_PARAMS	const PxGeometry& geom,	const PxTransform& pose,   \
		const Gu::Box& box, const PxVec3& unitDir, const PxReal distance,    \
		PxSweepHit& sweepHit, PxHitFlags hintFlags, const PxReal inflation

	PX_PHYSX_COMMON_API bool sweepCapsule_SphereGeom		(GU_CAPSULE_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepCapsule_PlaneGeom			(GU_CAPSULE_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepCapsule_CapsuleGeom		(GU_CAPSULE_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepCapsule_BoxGeom			(GU_CAPSULE_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepCapsule_ConvexGeom		(GU_CAPSULE_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepCapsule_MeshGeom			(GU_CAPSULE_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepCapsule_HeightFieldGeom	(GU_CAPSULE_SWEEP_FUNC_PARAMS);

	PX_PHYSX_COMMON_API bool sweepBox_SphereGeom		(GU_BOX_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepBox_PlaneGeom			(GU_BOX_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepBox_CapsuleGeom		(GU_BOX_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepBox_BoxGeom			(GU_BOX_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepBox_ConvexGeom		(GU_BOX_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepBox_MeshGeom			(GU_BOX_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API bool sweepBox_HeightFieldGeom	(GU_BOX_SWEEP_FUNC_PARAMS);


	typedef bool (*SweepCapsuleFunc) (GU_CAPSULE_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API const SweepCapsuleFunc* GetSweepCapsuleMap();
	extern const SweepCapsuleFunc gSweepCapsuleMap[7];

	typedef bool (*SweepBoxFunc) (GU_BOX_SWEEP_FUNC_PARAMS);
	PX_PHYSX_COMMON_API const SweepBoxFunc* GetSweepBoxMap();
	extern const SweepBoxFunc gSweepBoxMap[7];

	typedef bool (*SweepConvexFunc) (const PxGeometry&, const PxTransform&, const PxConvexMeshGeometry&, const PxTransform&, const PxVec3& unitDir, const PxReal distance, PxSweepHit&, PxHitFlags hintFlags, const PxReal inflation);
	PX_PHYSX_COMMON_API const SweepConvexFunc* GetSweepConvexMap();
	extern const SweepConvexFunc gSweepConvexMap[7];

	// For sweep vs. triangle list: PxGeometryQuery::sweep()
	bool SweepCapsuleTriangles(PxU32 nb_tris, const PxTriangle* triangles,
								const PxCapsuleGeometry& capsuleGeom, const PxTransform& capsulePose,
								const PxVec3& unitDir, const PxReal distance, const PxU32* cachedIndex, PxVec3& hit,
								PxVec3& normal, PxReal& d, PxU32& index, const PxReal inflation, PxHitFlags hintFlags);

	// For sweep vs. triangle list: PxGeometryQuery::sweep()
	bool SweepBoxTriangles(PxU32 nb_tris, const PxTriangle* triangles,
							const PxBoxGeometry& boxGeom, const PxTransform& boxPose, const PxVec3& unitDir, const PxReal distance, 
							PxVec3& _hit, PxVec3& _normal, float& _d, PxU32& _index, const PxU32* cachedIndex, const PxReal inflation, PxHitFlags hintFlags);


}  // namespace Gu

}

#endif
