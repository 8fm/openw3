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

#ifndef GU_SWEEP_SHARED_TESTS_H
#define GU_SWEEP_SHARED_TESTS_H

#include "CmPhysXCommon.h"
#include "PxQueryReport.h"
#include "GuBoxConversion.h"
#include "GuGeomUtilsInternal.h"
#include "GuTriangleMesh.h"

namespace physx
{
#if __SPU__
#define GU_SWEEP_SHARED inline
#else
#define GU_SWEEP_SHARED PX_FORCE_INLINE
#endif

#define LOCAL_EPSILON 0.00001f	// PT: this value makes the 'basicAngleTest' pass. Fails because of a ray almost parallel to a triangle

#define LAZY_NORMALIZE

#define PREFETCH_TRI
#define HF_SWEEP_REPORT_BUFFER_SIZE 64

#define USE_NEW_SWEEP_TEST
#define NEW_SWEEP_CAPSULE_MESH	// PT: test to extrude the mesh on the fly
//#define CHECK_SWEEP_CAPSULE_TRIANGLES	// PT: please keep around for a while



GU_SWEEP_SHARED void computeWorldToBoxMatrix(physx::Cm::Matrix34& worldToBox, const physx::Gu::Box& box)
{
	physx::Cm::Matrix34 boxToWorld;
	physx::buildMatrixFromBox(boxToWorld, box);
	worldToBox = boxToWorld.getInverseRT();
}

GU_SWEEP_SHARED void computeSweptBox(const PxVec3& extents, const PxVec3& center, const PxMat33& rot, const PxVec3& unitDir, const PxReal distance, Gu::Box& box)
{
	PxVec3 R1, R2;
	Ps::computeBasis(unitDir, R1, R2);

	PxReal dd[3];
	dd[0] = PxAbs(rot.column0.dot(unitDir));
	dd[1] = PxAbs(rot.column1.dot(unitDir));
	dd[2] = PxAbs(rot.column2.dot(unitDir));
	PxReal dmax = dd[0];
	PxU32 ax0=1;
	PxU32 ax1=2;
	if(dd[1]>dmax)
	{
		dmax=dd[1];
		ax0=0;
		ax1=2;
	}
	if(dd[2]>dmax)
	{
		dmax=dd[2];
		ax0=0;
		ax1=1;
	}
	if(dd[ax1]<dd[ax0])
	{
		PxU32 swap = ax0;
		ax0 = ax1;
		ax1 = swap;
	}

	R1 = rot[ax0];
	R1 -= (R1.dot(unitDir))*unitDir;	// Project to plane whose normal is dir
	R1.normalize();
	R2 = unitDir.cross(R1);

	box.setAxes(unitDir, R1, R2);

	PxReal Offset[3];
	Offset[0] = distance;
	Offset[1] = distance*(unitDir.dot(R1));
	Offset[2] = distance*(unitDir.dot(R2));

	for(PxU32 r=0; r<3; r++)
	{
		const PxVec3& R = box.rot[r];
		box.extents[r] = Offset[r]*0.5f + PxAbs(rot.column0.dot(R))*extents.x + PxAbs(rot.column1.dot(R))*extents.y + PxAbs(rot.column2.dot(R))*extents.z;
	}

	box.center = center + unitDir*distance*0.5f;
}

#if __SPU__ // AP: this is to reduce code size on SPU
inline void buildFrom1(Gu::Box& dst, const PxVec3& center, const PxVec3& extents, const PxQuat& q)
{
	dst.center	= center;
	dst.extents	= extents;
	dst.rot		= PxMat33(q);
}
#else
#define buildFrom1 buildFrom
#endif

// AP: uninlining increases SPU size
PX_FORCE_INLINE PxU32 getTriangleIndex(PxU32 i, PxU32 cachedIndex)
{
	PxU32 triangleIndex;
	if(i==0)				triangleIndex = cachedIndex;
	else if(i==cachedIndex)	triangleIndex = 0;
	else					triangleIndex = i;
	return triangleIndex;
}

// PT: returning a float is the fastest on Xbox!
#if __SPU__ // AP: disable inlining on SPU
inline bool CullTriangle(const PxTriangle& CurrentTri, const PxVec3& dir, PxReal radius, PxReal t, const PxReal dpc0)
#else
#ifdef _XBOX
PX_FORCE_INLINE float CullTriangle(const PxTriangle& CurrentTri, const PxVec3& dir, PxReal radius, PxReal t, const PxReal dpc0)
#else
PX_FORCE_INLINE bool CullTriangle(const PxTriangle& CurrentTri, const PxVec3& dir, PxReal radius, PxReal t, const PxReal dpc0)
#endif
#endif // !__SPU__
{
	const PxReal dp0 = CurrentTri.verts[0].dot(dir);
	const PxReal dp1 = CurrentTri.verts[1].dot(dir);
	const PxReal dp2 = CurrentTri.verts[2].dot(dir);

#ifdef _XBOX
	// PT: we have 3 ways to write that on Xbox:
	// - with the original code: suffers from a lot of FCMPs
	// - with the cndt stuff below, cast to an int: it's faster, but suffers from a very bad LHS from the float-to-int
	// - with the cndt stuff not cast to an int: we get only one FCMP instead of many, the LHS is gone, that's the fastest solution. Even though it looks awkward.
	// AP: new correct implementation
	PxReal dp = dp0;
	dp = physx::intrinsics::selectMin(dp, dp1);
	dp = physx::intrinsics::selectMin(dp, dp2);

	using physx::intrinsics::fsel;

	//if(dp>dpc0 + t + radius) return false;
	const float cndt0 = fsel(dp - (dpc0 + t + radius + 0.01f), 0.0f, 1.0f);

	//if(dp0<dpc0 && dp1<dpc0 && dp2<dpc0) return false; <=>
	//if(dp0>=dpc0 || dp1>=dpc0 || dp2>=dpc0) return true;
	const float cndt1 = fsel(dp0-dpc0, 1.0f, 0.0f) + fsel(dp1-dpc0, 1.0f, 0.0f) + fsel(dp2-dpc0, 1.0f, 0.0f);

	return cndt0*cndt1;
	//PxReal resx = cndt0*cndt1;
#else
	PxReal dp = dp0;
	dp = physx::intrinsics::selectMin(dp, dp1);
	dp = physx::intrinsics::selectMin(dp, dp2);

	if(dp>dpc0 + t + radius + 0.01f)
	{
		//PX_ASSERT(resx == 0.0f);
		return false;
	}

	// ExperimentalCulling
	if(dp0<dpc0 && dp1<dpc0 && dp2<dpc0)
	{
		//PX_ASSERT(resx == 0.0f);
		return false;
	}

	//PX_ASSERT(resx != 0.0f);
	return true;
#endif
}

PX_FORCE_INLINE	void getScaledTriangle(const PxTriangleMeshGeometry& triGeom, const Cm::Matrix34& vertex2worldSkew, PxTriangle& triangle, PxTriangleID triangleIndex)
{
	Gu::TriangleMesh* tm = static_cast<Gu::TriangleMesh*>(triGeom.triangleMesh);
	tm->computeWorldTriangle(triangle, triangleIndex, vertex2worldSkew);
}

#ifdef PREFETCH_TRI
static PX_FORCE_INLINE void prefetchTriangle(const PxTriangleMeshGeometry& triGeom, PxTriangleID triangleIndex)
{
	const Gu::TriangleMesh& tm = *static_cast<Gu::TriangleMesh*>(triGeom.triangleMesh);
	const PxVec3* PX_RESTRICT vertices = tm.getVerticesFast();
	if(tm.mMesh.has16BitIndices())
	{
		const Gu::TriangleT<PxU16>& T = ((const Gu::TriangleT<PxU16>*)tm.getTrianglesFast())[triangleIndex];
		Ps::prefetchLine(vertices + T.v[0]);
		Ps::prefetchLine(vertices + T.v[1]);
		Ps::prefetchLine(vertices + T.v[2]);
	}
	else
	{
		const Gu::TriangleT<PxU32>& T = ((const Gu::TriangleT<PxU32>*)tm.getTrianglesFast())[triangleIndex];
		Ps::prefetchLine(vertices + T.v[0]);
		Ps::prefetchLine(vertices + T.v[1]);
		Ps::prefetchLine(vertices + T.v[2]);
	}
}
#endif

}


#endif
