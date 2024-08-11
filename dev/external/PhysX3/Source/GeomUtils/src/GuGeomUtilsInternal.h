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

#ifndef GU_GEOM_UTILS_INTERNAL_H
#define GU_GEOM_UTILS_INTERNAL_H

#include "CmPhysXCommon.h"
#include "PsFPU.h"
#include "GuSphere.h"
#include "PxTriangle.h"
#include "PxBounds3.h"
#include "PsVecMath.h"
#include "PxTransform.h"
#include "GuCapsule.h"
#include "GuGeometryUnion.h"
#include "PsVecTransform.h"

namespace physx
{

class PxCapsuleGeometry;
class PxTriangleMeshGeometry;

namespace Cm
{
	class Matrix34;
}

namespace Gu
{
	class Plane;
	class PlaneV;
	class Capsule;
	class CapsuleV;
	class Box;
	class BoxV;
	class Segment;
	class SegmentV;      

	PX_PHYSX_COMMON_API const PxF32*	getBoxVertexNormals();
	PX_PHYSX_COMMON_API const PxU8*		getBoxTriangles();
	PX_PHYSX_COMMON_API const PxVec3*	getBoxLocalEdgeNormals();
	PX_PHYSX_COMMON_API const PxU8*		getBoxEdges();

	PX_PHYSX_COMMON_API void			computeBoxPoints(const PxBounds3& bounds, PxVec3* PX_RESTRICT pts);
	PX_PHYSX_COMMON_API void			computeBoundsAroundVertices(PxBounds3& bounds, PxU32 nbVerts, const PxVec3* PX_RESTRICT verts);

	PX_PHYSX_COMMON_API void			computeBoxWorldEdgeNormal(const Box& box, PxU32 edge_index, PxVec3& world_normal);
	PX_PHYSX_COMMON_API void			computeBoxAroundCapsule(const Capsule& capsule, Box& box);  //TODO: Refactor this one out in the future
	PX_PHYSX_COMMON_API void			computeBoxAroundCapsule(const CapsuleV& capsule, BoxV& box);  //TODO: Refactor this one out in the future
	PX_PHYSX_COMMON_API void			computeBoxAroundCapsule(const PxCapsuleGeometry& capsuleGeom, const PxTransform& capsulePose, Box& box);

	PX_PHYSX_COMMON_API PxPlane			getPlane(const PxTransform& pose);
	PX_PHYSX_COMMON_API Gu::PlaneV      getPlaneV(const PxTransform& pose);
	PX_PHYSX_COMMON_API PxTransform		getCapsuleTransform(const Gu::Capsule& capsule, PxReal& halfHeight);

	PX_INLINE void computeBasis(const Ps::aos::Vec3VArg p0, const Ps::aos::Vec3VArg p1, Ps::aos::Vec3V& dir, Ps::aos::Vec3V& right, Ps::aos::Vec3V& up)
	{
		//Need to test
		using namespace Ps::aos;
		// Compute the new direction vector
		const FloatV eps = FLoad(0.9999f);
		const Vec3V v = V3Sub(p1, p0);
		dir = V3Normalize(v);
		const BoolV con = FIsGrtr(FAbs(V3GetY(dir)), eps);
		const Vec3V w = V3Normalize(V3Cross(V3UnitY(), dir));
		right = V3Sel(con, V3UnitX(), w);
		up = V3Cross(dir, right);
	}

	// TODO: This should move to a capsule shape specific class if we ever introduce that in the LL
	PX_FORCE_INLINE void getCapsuleSegment(const PxTransform& transform, const PxCapsuleGeometry& capsuleGeom, Gu::Segment& segment)
	{
		const PxVec3 tmp = transform.q.getBasisVector0() * capsuleGeom.halfHeight;
		segment.p0 = transform.p + tmp;
		segment.p1 = transform.p - tmp;
	}

	PX_FORCE_INLINE	void getCapsule(Gu::Capsule& capsule, const PxCapsuleGeometry& capsuleGeom, const PxTransform& pose)
	{
		getCapsuleSegment(pose, capsuleGeom, capsule);
		capsule.radius = capsuleGeom.radius;
	}

	PX_FORCE_INLINE void getCapsuleSegment(const Ps::aos::PsTransformV& transform, const PxCapsuleGeometry& shape, Ps::aos::Vec3V& p0, Ps::aos::Vec3V& p1)
	{
		using namespace Ps::aos;
		const FloatV halfHeight = FLoad(shape.halfHeight);
		const Vec3V basisVector = QuatGetBasisVector0(transform.q);
		const Vec3V tmp = V3Scale(basisVector, halfHeight);
		p0 = V3Add(transform.p, tmp);
		p1 = V3Sub(transform.p, tmp);
	}
}  // namespace Gu

}

#endif
