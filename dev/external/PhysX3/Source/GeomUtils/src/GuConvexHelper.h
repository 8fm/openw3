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

#ifndef GU_CONVEXHELPER_H
#define GU_CONVEXHELPER_H

#include "PsFPU.h"
#include "PxVec3.h"
#include "GuShapeConvex.h"

namespace physx
{
namespace Gu
{
	class GeometryUnion;

	PX_FORCE_INLINE void copyPlane(PxPlane* PX_RESTRICT dst, const Gu::HullPolygonData* PX_RESTRICT src)
	{
		// PT: "as usual" now, the direct copy creates LHS that are avoided by the IR macro...
#ifdef PX_PS3
		// AP: IR macro causes compiler warnings (dereferencing type-punned pointer will break strict-aliasing rules)
		dst->n = src->mPlane.n;
		dst->d = src->mPlane.d;
#else
		PX_IR(dst->n.x) = PX_IR(src->mPlane.n.x);
		PX_IR(dst->n.y) = PX_IR(src->mPlane.n.y);
		PX_IR(dst->n.z) = PX_IR(src->mPlane.n.z);
		PX_IR(dst->d) = PX_IR(src->mPlane.d);
#endif
	}

	PX_FORCE_INLINE bool testNormal(const PxVec3& axis, PxReal min0, PxReal max0,
							  const PolygonalData& polyData1,
							  const Cm::Matrix34& m1to0,
							  const Cm::FastVertex2ShapeScaling& scaling1,
							  PxReal& depth, PxReal contactDistance)
	{
		//The separating axis we want to test is a face normal of hull0
		PxReal min1, max1;
		(polyData1.mProjectHull)(polyData1, axis, m1to0, scaling1, min1, max1);

#ifdef _XBOX
		depth = physx::intrinsics::selectMin(max0 - min1, max1 - min0);
		const float cndt0 = physx::intrinsics::fsel(max0 + contactDistance - min1, 1.0f, 0.0f);
		const float cndt1 = physx::intrinsics::fsel(max1 + contactDistance - min0, 1.0f, 0.0f);
		return (cndt0*cndt1)!=0.0f;
#else
		if(max0+contactDistance<min1 || max1+contactDistance<min0)
			return false;

		const PxReal d0 = max0 - min1;
		const PxReal d1 = max1 - min0;
		depth = physx::intrinsics::selectMin(d0, d1);
		return true;
#endif
	}

	PX_FORCE_INLINE bool testSeparatingAxis(	const PolygonalData& polyData0, const PolygonalData& polyData1,
												const Cm::Matrix34& world0, const Cm::Matrix34& world1,
												const Cm::FastVertex2ShapeScaling& scaling0, const Cm::FastVertex2ShapeScaling& scaling1,
												const PxVec3& axis, PxReal& depth, PxReal contactDistance)
	{
		PxReal min0, max0;
		(polyData0.mProjectHull)(polyData0, axis, world0, scaling0, min0, max0);

		return testNormal(axis, min0, max0, polyData1, world1, scaling1, depth, contactDistance);
	}

	///////////////////////////////////////////////////////////////////////////

	PX_PHYSX_COMMON_API void getScaledConvex(	PxVec3*& scaledVertices, PxU8*& scaledIndices, PxVec3* dstVertices, PxU8* dstIndices,
												bool idtConvexScale, const PxVec3* srcVerts, const PxU8* srcIndices, PxU32 nbVerts, const Cm::FastVertex2ShapeScaling& convexScaling);

	// PT: calling this correctly isn't trivial so let's macroize it. At least we limit the damage since it immediately calls a real function.
	#define GET_SCALEX_CONVEX(scaledVertices, stackIndices, idtScaling, nbVerts, scaling, srcVerts, srcIndices)	\
	getScaledConvex(scaledVertices, stackIndices,																\
					idtScaling ? NULL : (PxVec3*)PxAlloca(nbVerts * sizeof(PxVec3)),							\
					idtScaling ? NULL : (PxU8*)PxAlloca(nbVerts * sizeof(PxU8)),								\
					idtScaling, srcVerts, srcIndices, nbVerts, scaling);

	PX_PHYSX_COMMON_API bool getConvexData(const Gu::GeometryUnion& shape, Cm::FastVertex2ShapeScaling& scaling, PxBounds3& bounds, PolygonalData& polyData);

	struct ConvexEdge
	{
		PxU8	vref0;
		PxU8	vref1;
		PxVec3	normal;	// warning: non-unit vector!
	};

	PX_PHYSX_COMMON_API PxU32 findUniqueConvexEdges(PxU32 maxNbEdges, ConvexEdge* PX_RESTRICT edges, PxU32 numPolygons, const Gu::HullPolygonData* PX_RESTRICT polygons, const PxU8* PX_RESTRICT vertexData);
}
}

#endif
