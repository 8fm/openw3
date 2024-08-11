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

#include "PsIntrinsics.h"
#include "GuOverlapTests.h"

#include "CmScaling.h"
#include "CmMemFetch.h"
#include "GuHeightFieldUtil.h"
#include "PsUtilities.h"

#include "GuIntersectionBoxBox.h"
#include "GuIntersectionTriangleBox.h"
#include "GuDistancePointSegment.h"
#include "GuDistanceSegmentBox.h"
#include "GuDistanceSegmentSegment.h"

#include "PxSphereGeometry.h"
#include "PxBoxGeometry.h"
#include "PxCapsuleGeometry.h"
#include "PxPlaneGeometry.h"
#include "PxConvexMeshGeometry.h"

#include "GuCapsule.h"
#include "GuIceSupport.h"
#include "GuEdgeCache.h"
#include "GuBoxConversion.h"

#include "GuGeomUtilsInternal.h"
#include "GuConvexUtilsInternal.h"
#include "GuSPUHelpers.h"

#include "GuGJKFallBack.h"
#include "GuGJKWrapper.h"
#include "GuVecTriangle.h"
#include "GuVecSphere.h"
#include "GuVecCapsule.h"
#include "GuVecConvexHull.h"
#include "GuConvexMesh.h"

using namespace physx;
using namespace Cm;
using namespace Gu;

bool Gu::intersectHeightFieldSphere(const Gu::HeightFieldUtil& hfUtil, const Gu::Sphere& sphereInHfShape)
{
	const Gu::HeightField& hf = hfUtil.getHeightField();

	// sample the sphere center in the heightfield to find out
	// if we have penetration with more than the sphere radius
	if(hfUtil.isShapePointOnHeightField(sphereInHfShape.center.x, sphereInHfShape.center.z))
	{
		// The sphere origin projects within the bounds of the heightfield in the X-Z plane
		PxReal sampleHeight = hfUtil.getHeightAtShapePoint(sphereInHfShape.center.x, sphereInHfShape.center.z);
		PxReal deltaHeight = sphereInHfShape.center.y - sampleHeight;
		if(hf.isDeltaHeightInsideExtent(deltaHeight))
		{
			// The sphere origin is 'below' the heightfield surface
			PxU32 faceIndex = hfUtil.getFaceIndexAtShapePoint(sphereInHfShape.center.x, sphereInHfShape.center.z);
			if(faceIndex != 0xffffffff)
			{
				return true;
			}
			return false;
		}
	}

	const PxReal radiusSquared = sphereInHfShape.radius * sphereInHfShape.radius;

	const PxVec3 sphereInHF = hfUtil.shape2hfp(sphereInHfShape.center);

	const PxReal radiusOverRowScale = sphereInHfShape.radius * PxAbs(hfUtil.getOneOverRowScale());
	const PxReal radiusOverColumnScale = sphereInHfShape.radius * PxAbs(hfUtil.getOneOverColumnScale());

	const PxU32 minRow = hf.getMinRow(sphereInHF.x - radiusOverRowScale);
	const PxU32 maxRow = hf.getMaxRow(sphereInHF.x + radiusOverRowScale);
	const PxU32 minColumn = hf.getMinColumn(sphereInHF.z - radiusOverColumnScale);
	const PxU32 maxColumn = hf.getMaxColumn(sphereInHF.z + radiusOverColumnScale);

	for(PxU32 r = minRow; r < maxRow; r++) 
	{
		for(PxU32 c = minColumn; c < maxColumn; c++) 
		{

			// x--x--x
			// | x   |
			// x  x  x
			// |   x |
			// x--x--x
			PxVec3 pcp[11];
			PxU32 npcp = 0;
			npcp = hfUtil.findClosestPointsOnCell(r, c, sphereInHfShape.center, pcp, NULL, true, true, true);

			for(PxU32 pi = 0; pi < npcp; pi++)
			{
				PxVec3 d = sphereInHfShape.center - pcp[pi];
				
				PxReal ll = d.magnitudeSquared();
					
				if(ll > radiusSquared) 
					// Too far
					continue;

				return true;
			}
		}
	}
	return false;
}

bool Gu::intersectHeightFieldCapsule(const Gu::HeightFieldUtil& hfUtil, const Gu::Capsule& capsuleInHfShape)
{
	const Gu::HeightField& hf = hfUtil.getHeightField();

	const PxReal radius = capsuleInHfShape.radius;
	const PxReal radiusOverRowScale = radius * PxAbs(hfUtil.getOneOverRowScale());
	const PxReal radiusOverColumnScale = radius * PxAbs(hfUtil.getOneOverColumnScale());

	PxVec3 verticesInHfShape[2];
	verticesInHfShape[0] = capsuleInHfShape.p0;
	verticesInHfShape[1] = capsuleInHfShape.p1;

	PxU32 absMinRow = 0xffffffff;
	PxU32 absMaxRow = 0;
	PxU32 absMinColumn = 0xffffffff;
	PxU32 absMaxColumn = 0;

	PxReal radiusSquared = radius * radius;

	// test both of capsule's corner vertices+radius for HF overlap
	for(PxU32 i = 0; i<2; i++)
	{
		const PxVec3& sphereInHfShape = verticesInHfShape[i];

		// we have to do this first to update the absMin / absMax correctly even if
		// we decide to continue from inside the deep penetration code.

		const PxVec3 sphereInHF = hfUtil.shape2hfp(sphereInHfShape);

		const PxU32 minRow = hf.getMinRow(sphereInHF.x - radiusOverRowScale);
		const PxU32 maxRow = hf.getMaxRow(sphereInHF.x + radiusOverRowScale);
		const PxU32 minColumn = hf.getMinColumn(sphereInHF.z - radiusOverColumnScale);
		const PxU32 maxColumn = hf.getMaxColumn(sphereInHF.z + radiusOverColumnScale);

		if(minRow < absMinRow)			absMinRow = minRow;
		if(minColumn < absMinColumn)	absMinColumn = minColumn;
		if(maxRow > absMaxRow)			absMaxRow = maxRow;
		if(maxColumn > absMaxColumn)	absMaxColumn = maxColumn;

		if(hfUtil.isShapePointOnHeightField(sphereInHfShape.x, sphereInHfShape.z))
		{
			// The sphere origin projects within the bounds of the heightfield in the X-Z plane
			PxReal sampleHeight = hfUtil.getHeightAtShapePoint(sphereInHfShape.x, sphereInHfShape.z);
			PxReal deltaHeight = sphereInHfShape.y - sampleHeight;
			if(hf.isDeltaHeightInsideExtent(deltaHeight))
			{
				// The sphere origin is 'below' the heightfield surface
				PxU32 faceIndex = hfUtil.getFaceIndexAtShapePoint(sphereInHfShape.x, sphereInHfShape.z);
				if(faceIndex != 0xffffffff)
				{
					return true;
				}
				continue;
			}
		}

		for(PxU32 r = minRow; r < maxRow; r++) 
		{
			for(PxU32 c = minColumn; c < maxColumn; c++) 
			{

				// x--x--x
				// | x   |
				// x  x  x
				// |   x |
				// x--x--x
				PxVec3 pcp[11];
				PxU32 npcp = 0;
				npcp = hfUtil.findClosestPointsOnCell(r, c, sphereInHfShape, pcp, NULL, true, true, true);

				for(PxU32 pi = 0; pi < npcp; pi++)
				{
					PxVec3 d = sphereInHfShape - pcp[pi];

					if(hf.isDeltaHeightOppositeExtent(d.y))
					{
						// We are 'above' the heightfield

						PxReal ll = d.magnitudeSquared();

						if(ll > radiusSquared) 
							// Too far above
							continue;

						return true;
					}
				}
			}
		}
	}

	// now test capsule's inflated segment for overlap with HF edges
	PxU32 row, column;
	for(row = absMinRow; row <= absMaxRow; row++)
	{
		for(column = absMinColumn; column <= absMaxColumn; column++)
		{
			const PxU32 vertexIndex = row * hf.getNbColumnsFast() + column;
			const PxU32 firstEdge = 3 * vertexIndex;
			// omg I am sorry about this code but I can't find a simpler way:
			//  last column will only test edge 2
			//  last row will only test edge 0
			//  and most importantly last row and column will not go inside the for
			const PxU32 minEi = (column == absMaxColumn) ? 2 : 0; 
			const PxU32 maxEi = (row    == absMaxRow   ) ? 1 : 3; 
			for(PxU32 ei = minEi; ei < maxEi; ei++)
			{
				const PxU32 edgeIndex = firstEdge + ei;

const PxU32 cell = vertexIndex;
PX_ASSERT(cell == edgeIndex / 3);
const PxU32 row_    = row;
PX_ASSERT(row_ == cell / hf.getNbColumnsFast());
const PxU32 column_ = column;
PX_ASSERT(column_ == cell % hf.getNbColumnsFast());

				const PxU32 faceIndex = hfUtil.getEdgeFaceIndex(edgeIndex, cell, row_, column_);
				if(faceIndex != 0xffffffff)
				{
					PxVec3 origin;
					PxVec3 direction;
					hfUtil.getEdge(edgeIndex, cell, row_, column_, origin, direction);

					PxReal s, t;
					const PxReal ll = Gu::distanceSegmentSegmentSquared(capsuleInHfShape, Gu::Segment(origin, origin + direction), &s, &t);
					if(ll < radiusSquared)
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

namespace physx
{
namespace Gu
{
	const bool gCompileBoxVertex         = true;
	const bool gCompileConvexVertex      = true;
	const bool gCompileEdgeEdge          = true;
	const bool gCompileHeightFieldVertex = true;

	const PxReal signs[24] = 
	{
		-1,-1,-1,
		-1,-1, 1,
		-1, 1,-1,
		-1, 1, 1,
		 1,-1,-1,
		 1,-1, 1,
		 1, 1,-1,
		 1, 1, 1,
	};

	const char edges[24] = 
	{
		0,1,
		1,3,
		3,2,
		2,0,
		4,5,
		5,7,
		7,6,
		6,4,
		0,4,
		1,5,
		2,6,
		3,7,
	};
	
	struct TriggerTraceSegmentCallback
	{
		bool intersection;

		PX_INLINE TriggerTraceSegmentCallback() : intersection(false)
		{
		}

		PX_INLINE bool underFaceHit(
			const Gu::HeightFieldUtil&, const PxVec3&,
			const PxVec3&, PxF32, PxF32, PxF32, PxU32)
		{
			return true;
		}

		PX_INLINE bool faceHit(const Gu::HeightFieldUtil&, const PxVec3&, PxU32)
		{
			intersection = true;
			return true;
		}
	};
} // namespace
}

bool Gu::intersectHeightFieldBox(const Gu::HeightFieldUtil& hfUtil, const Gu::Box& boxInHfShape)
{
	const Gu::HeightField& hf = hfUtil.getHeightField();

	PxU32 i;

	// Get box vertices
	PxVec3 boxVertices[8];
	for(i=0; i<8; i++) 
	{
		boxVertices[i] = PxVec3(boxInHfShape.extents.x*signs[3*i], boxInHfShape.extents.y*signs[3*i+1], boxInHfShape.extents.z*signs[3*i+2]);
	}

	// Transform box vertices to HeightFieldShape space
	PxVec3 boxVerticesInHfShape[8];
	for(i=0; i<8; i++) 
	{
		boxVerticesInHfShape[i] = boxInHfShape.transform(boxVertices[i]);
	}

	// Test box vertices.
	if(gCompileBoxVertex)
	{
		for(i=0; i<8; i++) 
		{
			const PxVec3& boxVertexInHfShape = boxVerticesInHfShape[i];
			if(hfUtil.isShapePointOnHeightField(boxVertexInHfShape.x, boxVertexInHfShape.z))
			{
				PxReal y = hfUtil.getHeightAtShapePoint(boxVertexInHfShape.x, boxVertexInHfShape.z);
				PxReal dy = boxVertexInHfShape.y - y;
				if(hf.isDeltaHeightInsideExtent(dy))
				{
					PxU32 faceIndex = hfUtil.getFaceIndexAtShapePoint(boxVertexInHfShape.x, boxVertexInHfShape.z);
					if(faceIndex != 0xffffffff)
					{
						return true;
					}
				}
			}
		}
	}

	// Test box edges.
	if(gCompileEdgeEdge)
	{
		for(i=0; i<12; i++) 
		{
			const PxVec3 v0 = boxVerticesInHfShape[PxU8(edges[2*i])];
			const PxVec3 v1 = boxVerticesInHfShape[PxU8(edges[2*i+1])];
			TriggerTraceSegmentCallback cb;
			hfUtil.traceSegment<TriggerTraceSegmentCallback, false, false>(v0, v1, &cb);
			if(cb.intersection)
				return true;
		}
	}

	// Test HeightField vertices.
	if(gCompileHeightFieldVertex)
	{
		Cm::Matrix34 _hfShape2BoxShape;
		{
			Cm::Matrix34 _boxPose;
			buildMatrixFromBox(_boxPose, boxInHfShape);
			_hfShape2BoxShape = _boxPose.getInverseRT();
		}

		PxReal minx(PX_MAX_REAL);
		PxReal minz(PX_MAX_REAL);
		PxReal maxx(-PX_MAX_REAL);
		PxReal maxz(-PX_MAX_REAL);

		for(i=0; i<8; i++) 
		{
			const PxVec3& boxVertexInHfShape = boxVerticesInHfShape[i];

/*			if(boxVertexInHfShape.x < minx) minx = boxVertexInHfShape.x;
			if(boxVertexInHfShape.z < minz) minz = boxVertexInHfShape.z;
			if(boxVertexInHfShape.x > maxx) maxx = boxVertexInHfShape.x;
			if(boxVertexInHfShape.z > maxz) maxz = boxVertexInHfShape.z;*/
			minx = physx::intrinsics::selectMin(boxVertexInHfShape.x, minx);
			minz = physx::intrinsics::selectMin(boxVertexInHfShape.z, minz);
			maxx = physx::intrinsics::selectMax(boxVertexInHfShape.x, maxx);
			maxz = physx::intrinsics::selectMax(boxVertexInHfShape.z, maxz);
		}

		const PxReal oneOverRowScale = hfUtil.getOneOverRowScale();
		const PxReal oneOverColumnScale = hfUtil.getOneOverColumnScale();
		const PxU32 minRow = hf.getMinRow(minx * oneOverRowScale);
		const PxU32 maxRow = hf.getMaxRow(maxx * oneOverRowScale);
		const PxU32 minColumn = hf.getMinColumn(minz * oneOverColumnScale);
		const PxU32 maxColumn = hf.getMaxColumn(maxz * oneOverColumnScale);

		for(PxU32 row = minRow; row <= maxRow; row++)
		{
			for(PxU32 column = minColumn; column <= maxColumn; column++)
			{
				PxU32 vertexIndex = row * hf.getNbColumnsFast() + column;
				if(hfUtil.isCollisionVertex(vertexIndex, row, column))
				{
					// check if hf vertex is inside the box
					const PxHeightFieldGeometry& geom = hfUtil.getHeightFieldGeometry();
					PxVec3 hfVertex(geom.rowScale * row, geom.heightScale * hf.getHeight(vertexIndex), geom.columnScale * column);
					const PxVec3 hfVertexInBoxShape = _hfShape2BoxShape.transform(hfVertex);
					if((PxAbs(hfVertexInBoxShape.x) - boxInHfShape.extents.x < 0)
						 && (PxAbs(hfVertexInBoxShape.y) - boxInHfShape.extents.y < 0)
						 && (PxAbs(hfVertexInBoxShape.z) - boxInHfShape.extents.z < 0))
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

static Cm::Matrix34 multiplyInverseRTLeft(const Cm::Matrix34& left, const Cm::Matrix34& right)
{
//	t = left.M % (right.t - left.t);
	PxVec3 t = left.rotateTranspose(right.base3 - left.base3);

//	M.setMultiplyTransposeLeft(left.M, right.M);
	PxMat33 left33(left.base0, left.base1, left.base2);
	PxMat33 right33(right.base0, right.base1, right.base2);
	PxMat33 multiplyTransposeLeft33 = (left33.getTranspose()) * right33;

	return Cm::Matrix34(multiplyTransposeLeft33, t);
}

bool Gu::intersectHeightFieldConvex(
	const Gu::HeightFieldUtil& hfUtil, const PxTransform& _hfAbsPose, const Gu::ConvexMesh& convexMesh,
	const PxTransform& _convexAbsPose, const PxMeshScale& convexMeshScaling)
{
	const Cm::Matrix34 hfAbsPose34(_hfAbsPose);
	const Cm::Matrix34 convexAbsPose34(_convexAbsPose);
	const Cm::Matrix34 vertexToShapeSkew34(convexMeshScaling.toMat33());
	const Cm::Matrix34 tmp34 = convexAbsPose34 * vertexToShapeSkew34;
	const Cm::Matrix34 convexShape2HfShapeSkew34 = multiplyInverseRTLeft(hfAbsPose34, tmp34);

	const Gu::ConvexHullData* hull = &convexMesh.getHull();

	// Allocate space for transformed vertices.
	PxVec3* convexVerticesInHfShape = (PxVec3*)PxAlloca(hull->mNbHullVertices*sizeof(PxVec3));

	// Transform vertices to height field shape
	const PxVec3* hullVerts = hull->getHullVertices();
	for(PxU32 i=0; i<hull->mNbHullVertices; i++)
		convexVerticesInHfShape[i] = convexShape2HfShapeSkew34.transform(hullVerts[i]);

	// Compute bounds of convex in hf space
	PxBounds3 convexBoundsInHfShape;
	computeBoundsAroundVertices(convexBoundsInHfShape, hull->mNbHullVertices, convexVerticesInHfShape);

	// Compute the height field extreme over the bounds area.
	const Gu::HeightField& hf = hfUtil.getHeightField();
	PxReal hfExtreme = (hf.getThicknessFast() <= 0) ? -PX_MAX_REAL : PX_MAX_REAL;
	const PxReal oneOverRowScale = hfUtil.getOneOverRowScale();
	const PxReal oneOverColumnScale = hfUtil.getOneOverColumnScale();
	const PxReal rowScale = (1.0f / hfUtil.getOneOverRowScale());
	const PxReal columnScale = (1.0f / hfUtil.getOneOverColumnScale());
	const PxReal heightScale = (1.0f / hfUtil.getOneOverHeightScale());

	// negative scale support
	PxU32 minRow;
	PxU32 maxRow;
	if(oneOverRowScale > 0.0f)
	{
		minRow = hf.getMinRow(convexBoundsInHfShape.minimum.x * oneOverRowScale);
		maxRow = hf.getMaxRow(convexBoundsInHfShape.maximum.x * oneOverRowScale);
	}
	else
	{
		minRow = hf.getMinRow(convexBoundsInHfShape.maximum.x * oneOverRowScale);
		maxRow = hf.getMaxRow(convexBoundsInHfShape.minimum.x * oneOverRowScale);
	}

	PxU32 minColumn;
	PxU32 maxColumn;
	if(oneOverColumnScale > 0.0f)
	{
		minColumn = hf.getMinColumn(convexBoundsInHfShape.minimum.z * oneOverColumnScale);
		maxColumn = hf.getMaxColumn(convexBoundsInHfShape.maximum.z * oneOverColumnScale);
	}
	else
	{
		minColumn = hf.getMinColumn(convexBoundsInHfShape.maximum.z * oneOverColumnScale);
		maxColumn = hf.getMaxColumn(convexBoundsInHfShape.minimum.z * oneOverColumnScale);
	}

	for(PxU32 row = minRow; row <= maxRow; row++)
	{
		for(PxU32 column = minColumn; column <= maxColumn; column++)
		{
			const PxReal h = hf.getHeight(row * hf.getNbColumnsFast() + column);
			hfExtreme = (hf.getThicknessFast() <= 0) ? PxMax(hfExtreme, h) : PxMin(hfExtreme, h);
		}
	}
	hfExtreme *= heightScale;


	// Return if convex is on the wrong side of the extreme.
	if(hf.getThicknessFast() <= 0)
	{
		if(convexBoundsInHfShape.minimum.y > hfExtreme)
			return false;
	}
	else
	{
		if(convexBoundsInHfShape.maximum.y < hfExtreme)
			return false;
	}


	// Test convex vertices
	if(gCompileConvexVertex)
	{
		for(PxU32 i=0; i<hull->mNbHullVertices; i++) 
		{
			const PxVec3& convexVertexInHfShape = convexVerticesInHfShape[i];
			bool insideExtreme = (hf.getThicknessFast() <= 0) ? (convexVertexInHfShape.y < hfExtreme) : (convexVertexInHfShape.y > hfExtreme);
			if(insideExtreme && hfUtil.isShapePointOnHeightField(convexVertexInHfShape.x, convexVertexInHfShape.z))
			{
				const PxReal y = hfUtil.getHeightAtShapePoint(convexVertexInHfShape.x, convexVertexInHfShape.z);
				const PxReal dy = convexVertexInHfShape.y - y;
				if(hf.isDeltaHeightInsideExtent(dy))
				{
					const PxU32 faceIndex = hfUtil.getFaceIndexAtShapePoint(convexVertexInHfShape.x, convexVertexInHfShape.z);
					if(faceIndex != 0xffffffff)
						return true;
				}
			}
		} 
	}

	// Test convex edges.
	if(gCompileEdgeEdge)
	{
		EdgeCache edgeCache;
		PxU32 numPolygons = hull->mNbPolygons;
		const Gu::HullPolygonData* polygons = hull->mPolygons;
		const PxU8* const vertexData = hull->getVertexData8();
		while(numPolygons--)
		{
			const Gu::HullPolygonData& polygon = *polygons++;

			const PxU8* verts = vertexData + polygon.mVRef8;

			PxU32 numEdges = polygon.mNbVerts;

			PxU32 a = numEdges - 1;
			PxU32 b = 0;
			while(numEdges--)
			{
				PxU8 vi0 =  verts[a];
				PxU8 vi1 =	verts[b];

				if(vi1 < vi0)
				{
					PxU8 tmp = vi0;
					vi0 = vi1;
					vi1 = tmp;
				}

				if(edgeCache.isInCache(vi0, vi1))	//avoid processing edges 2x if possible (this will typically have cache misses about 5% of the time leading to 5% redundant work.
					continue;

				const PxVec3& sv0 = convexVerticesInHfShape[vi0];
				const PxVec3& sv1 = convexVerticesInHfShape[vi1];
				a = b;
				b++;


				if(hf.getThicknessFast() <= 0) 
				{
					if((sv0.y > hfExtreme) && (sv1.y > hfExtreme))
						continue;
				}
				else
				{
					if((sv0.y < hfExtreme) && (sv1.y < hfExtreme))
						continue;
				}
				const PxVec3 v0 = sv0;
				const PxVec3 v1 = sv1;
				TriggerTraceSegmentCallback cb;
				hfUtil.traceSegment<TriggerTraceSegmentCallback, false, false>(v0, v1, &cb);
				if(cb.intersection)
					return true;
			}
		}
	}

	// Test HeightField vertices
	if(gCompileHeightFieldVertex)
	{
		const Cm::Matrix34 tmp34 = multiplyInverseRTLeft(convexAbsPose34, hfAbsPose34);
		const Cm::Matrix34 hfShape2ConvexShapeSkew34 = vertexToShapeSkew34 * tmp34;

		for(PxU32 row = minRow; row <= maxRow; row++)
		{
			for(PxU32 column = minColumn; column <= maxColumn; column++)
			{
				const PxU32 hfVertexIndex = row * hf.getNbColumnsFast() + column;
				if(hfUtil.isCollisionVertex(hfVertexIndex, row, column))
				{
					// Check if hf vertex is inside the convex
					const PxVec3 hfVertex(rowScale * row, heightScale * hf.getHeight(hfVertexIndex), columnScale * column);
					const PxVec3 hfVertexInConvexShape = hfShape2ConvexShapeSkew34.transform(hfVertex);

					bool inside = true;
					for(PxU32 poly = 0; poly < hull->mNbPolygons; poly++)
					{
						PxReal d = hull->mPolygons[poly].mPlane.distance(hfVertexInConvexShape);
						if(d >= 0)
						{
							inside = false;
							break;
						}
					}
					if(inside)
						return true;
				}
			}
		}
	}
	return false;
}

bool Gu::checkOverlapSphere_heightFieldGeom(const PxGeometry& geom, const PxTransform& pose, const Gu::Sphere& sphere)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eHEIGHTFIELD);
	const PxHeightFieldGeometry& hfGeom = static_cast<const PxHeightFieldGeometry&>(geom);

	const Cm::Matrix34 invAbsPose(pose.getInverse());
	const Gu::Sphere sphereInHfShape(invAbsPose.transform(sphere.center), sphere.radius);

#ifdef __SPU__
	GU_FETCH_HEIGHTFIELD_DATA(hfGeom);
#endif

	HeightFieldUtil hfUtil(hfGeom);
	return intersectHeightFieldSphere(hfUtil, sphereInHfShape);
}

bool Gu::checkOverlapOBB_heightFieldGeom(const PxGeometry& geom, const PxTransform& pose, const Gu::Box& box)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eHEIGHTFIELD);
	const PxHeightFieldGeometry& hfGeom = static_cast<const PxHeightFieldGeometry&>(geom);

	Gu::Box boxInHfShape;
	const Cm::Matrix34 invAbsPose(pose.getInverse());
	boxInHfShape.rot = PxMat33(invAbsPose.base0, invAbsPose.base1, invAbsPose.base2) * box.rot;
	boxInHfShape.center = invAbsPose.transform(box.center);
	boxInHfShape.extents = box.extents;

#ifdef __SPU__
	GU_FETCH_HEIGHTFIELD_DATA(hfGeom);
	//PX_ALIGN_PREFIX(16)  PxU8 heightFieldBuffer[sizeof(Gu::HeightField)+32] PX_ALIGN_SUFFIX(16);
	//Gu::HeightField* heightField = memFetchAsync<Gu::HeightField>(heightFieldBuffer, (uintptr_t)(hfGeom.heightField), sizeof(Gu::HeightField), 1);
	//memFetchWait(1);
	//
	//g_sampleCache.init((uintptr_t)(heightField->getData().samples), heightField->getData().tilesU);

	//const_cast<PxHeightFieldGeometry&>(hfGeom).heightField = heightField;
#endif

	HeightFieldUtil hfUtil(hfGeom);
	return intersectHeightFieldBox(hfUtil, boxInHfShape);
}

bool Gu::checkOverlapCapsule_heightFieldGeom(const PxGeometry& geom, const PxTransform& pose, const Gu::Capsule& worldCapsule)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eHEIGHTFIELD);
	const PxHeightFieldGeometry& hfGeom = static_cast<const PxHeightFieldGeometry&>(geom);

	const Cm::Matrix34 invAbsPose(pose.getInverse());
	const Gu::Capsule capsuleInHfShape(Segment(	invAbsPose.transform(worldCapsule.p0),
												invAbsPose.transform(worldCapsule.p1)),
										worldCapsule.radius);

#ifdef __SPU__
	GU_FETCH_HEIGHTFIELD_DATA(hfGeom);
#endif

	HeightFieldUtil hfUtil(hfGeom);
	return intersectHeightFieldCapsule(hfUtil, capsuleInHfShape);
}

bool GeomOverlapCallback_SphereHeightfield(GEOM_OVERLAP_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eSPHERE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eHEIGHTFIELD);
	PX_UNUSED(cache);

	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom0);
	const PxHeightFieldGeometry& hfGeom = static_cast<const PxHeightFieldGeometry&>(geom1);

#ifdef __SPU__
	GU_FETCH_HEIGHTFIELD_DATA(hfGeom);
#endif

	Gu::Sphere sphereInHf;
	sphereInHf.center = transform1.transformInv(transform0.p);
	sphereInHf.radius = sphereGeom.radius;

	Gu::HeightFieldUtil hfUtil(hfGeom);
	return Gu::intersectHeightFieldSphere(hfUtil, sphereInHf);
}

bool GeomOverlapCallback_PlaneHeightfield(GEOM_OVERLAP_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::ePLANE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eHEIGHTFIELD);
	PX_ASSERT(!"NOT SUPPORTED");
	PX_UNUSED(cache);
	PX_UNUSED(transform0);
	PX_UNUSED(transform1);
	PX_UNUSED(geom0);
	PX_UNUSED(geom1);
	return false;
}

bool GeomOverlapCallback_CapsuleHeightfield(GEOM_OVERLAP_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eCAPSULE);
	PX_ASSERT(geom1.getType()==PxGeometryType::eHEIGHTFIELD);
	PX_UNUSED(cache);

	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom0);
	const PxHeightFieldGeometry& hfGeom = static_cast<const PxHeightFieldGeometry&>(geom1);

	const PxTransform capsuleShapeToHfShape = transform1.transformInv(transform0);

	Gu::Capsule capsuleInHfShape;
	Gu::getCapsule(capsuleInHfShape, capsuleGeom, capsuleShapeToHfShape);

#ifdef __SPU__
	GU_FETCH_HEIGHTFIELD_DATA(hfGeom);
#endif

	const Gu::HeightFieldUtil hfUtil(hfGeom);
	return Gu::intersectHeightFieldCapsule(hfUtil, capsuleInHfShape);
}

bool GeomOverlapCallback_BoxHeightfield(GEOM_OVERLAP_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eBOX);
	PX_ASSERT(geom1.getType()==PxGeometryType::eHEIGHTFIELD);
	PX_UNUSED(cache);	

	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom0);
	const PxHeightFieldGeometry& hfGeom = static_cast<const PxHeightFieldGeometry&>(geom1);

#ifdef __SPU__
	GU_FETCH_HEIGHTFIELD_DATA(hfGeom);
#endif

	const PxTransform boxShape2HfShape = transform1.transformInv(transform0);

	Gu::Box box;
	buildFrom(box, boxShape2HfShape.p, boxGeom.halfExtents, boxShape2HfShape.q);

	Gu::HeightFieldUtil hfUtil(hfGeom);
	return Gu::intersectHeightFieldBox(hfUtil, box);
}

///////////////////////////////////////////////////////////////////////////////
bool GeomOverlapCallback_ConvexHeightfield(GEOM_OVERLAP_CALLBACK_PARAMS)
{
	PX_ASSERT(geom0.getType()==PxGeometryType::eCONVEXMESH);
	PX_ASSERT(geom1.getType()==PxGeometryType::eHEIGHTFIELD);
	PX_UNUSED(cache);	

	const PxConvexMeshGeometry& convexGeom = static_cast<const PxConvexMeshGeometry&>(geom0);
	const PxHeightFieldGeometry& hfGeom = static_cast<const PxHeightFieldGeometry&>(geom1);

	GU_FETCH_CONVEX_DATA(convexGeom);

#ifdef __SPU__
	GU_FETCH_HEIGHTFIELD_DATA(hfGeom);
#endif

	Gu::HeightFieldUtil hfUtil(hfGeom);
	return Gu::intersectHeightFieldConvex(hfUtil, transform1, *cm, transform0, convexGeom.scale);
}
