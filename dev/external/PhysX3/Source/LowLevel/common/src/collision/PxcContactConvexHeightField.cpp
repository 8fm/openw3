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

        
#include "PxcContactTraceLineCallback.h"
#include "GuConvexMeshData.h"
#include "GuEdgeCache.h"
#include "GuConvexHelper.h"
#include "PxcContactMethodImpl.h"
#include "PxcNpCache.h"

#include "GuContactBuffer.h"

using namespace physx;
using namespace Gu;

#define DISTANCE_BASED_TEST

#ifdef __SPU__
extern unsigned char HeightFieldBuffer[sizeof(Gu::HeightField)+16];
#include "CmMemFetch.h"
#endif

// ptchernev TODO: make sure these are ok before shipping
static const bool gCompileConvexVertex		= true;
static const bool gCompileEdgeEdge          = true;
static const bool gCompileHeightFieldVertex = true;

/** 
\param point vertex tested for penetration (in local hull space)
\param safeNormal if none of the faces are good this becomes the normal
\param hull
\param normal the direction to translate vertex to depenetrate
\param distance the distance along normal to translate vertex to depenetrate
*/
static bool PxcDepenetrateConvex(	const PxVec3& point,
									const PxVec3& safeNormal,
									const Gu::ConvexHullData& hull,
									float contactDistance,
									PxVec3& normal,
									PxReal& distance,
									const Cm::FastVertex2ShapeScaling& scaling,
									bool isConvexScaleIdentity)
{
	PxVec3 faceNormal(PxReal(0));
	PxReal distance1 = -PX_MAX_REAL; // cant be more
	PxReal distance2 = -PX_MAX_REAL; // cant be more
	PxI32 poly1 = -1;
	PxI32 poly2 = -2;

//	const Cm::FastVertex2ShapeScaling& scaling = context.mVertex2ShapeSkew[0];

	for (PxU32 poly = 0; poly < hull.mNbPolygons; poly++)
	{
		PxPlane shapeSpacePlane;
		if(isConvexScaleIdentity)
		{
			copyPlane(&shapeSpacePlane, &hull.mPolygons[poly]);
		}
		else
		{
			const PxPlane& vertSpacePlane = hull.mPolygons[poly].mPlane;
			scaling.transformPlaneToShapeSpace(vertSpacePlane.n, vertSpacePlane.d, shapeSpacePlane.n, shapeSpacePlane.d);//transform plane into shape space
		}

#ifdef DISTANCE_BASED_TEST
		// PT: I'm not really sure about contactDistance here
		const PxReal d = shapeSpacePlane.distance(point) - contactDistance;
#else
		const PxReal d = shapeSpacePlane.distance(point);
#endif

		if (d >= 0)
		{
			// no penetration at all
			return false;
		}

		//const PxVec3& n = plane.normal;
		const PxReal proj = shapeSpacePlane.n.dot(safeNormal);
		if (proj > 0)
		{
			if (d > distance1) // less penetration
			{
				distance1 = d;
				faceNormal = shapeSpacePlane.n;
				poly1 = poly;
			}

			// distance2 / d = 1 / proj 
			const PxReal tmp = d / proj;
			if (tmp > distance2)
			{
				distance2 = tmp;
				poly2 = poly;
			}
		}
	}

	if (poly1 == poly2)
	{
		PX_ASSERT(faceNormal.magnitudeSquared() != 0.0f);
		normal = faceNormal;
		distance = -distance1;
	}
	else
	{
		normal = safeNormal;
		distance = -distance2;
	}

	return true;
}

//Box-Heightfield and Convex-Heightfield do not support positive values for contactDistance,
//and if in this case we would emit contacts normally, we'd cause things to jitter.
//as a workaround we add contactDistance to the distance values that we emit in contacts.
//this has the effect that the biasing will work exactly as if we had specified a legacy skinWidth of (contactDistance - restDistance)

#include "GuContactMethodImpl.h"

namespace physx
{

bool PxcContactConvexHeightFieldUnified(CONTACT_METHOD_ARGS)
{
	return Gu::contactConvexHeightfield(shape0, shape1, transform0, transform1, contactDistance, (Gu::Cache&)npCache, contactBuffer);
}

bool PxcContactConvexHeightField(CONTACT_METHOD_ARGS)
{
	PX_UNUSED(npCache);

#ifndef DISTANCE_BASED_TEST
	PX_WARN_ONCE(contactDistance > 0.0f, "PxcContactConvexHeightField: Convex-Heightfield does not support distance based contact generation! Ignoring contactOffset > 0!");
#endif

	// Get actual shape data
	const PxConvexMeshGeometryLL& shapeConvex = shape0.get<const PxConvexMeshGeometryLL>();
	const PxHeightFieldGeometryLL& hfGeom = shape1.get<const PxHeightFieldGeometryLL>();

	Cm::Matrix34 convexShapeAbsPose(transform0);
	Cm::Matrix34 hfShapeAbsPose(transform1);

	PX_ASSERT(contactBuffer.count==0);

#ifdef __SPU__
	const Gu::HeightField& hf = *Cm::memFetchAsync<const Gu::HeightField>(HeightFieldBuffer, Cm::MemFetchPtr(static_cast<Gu::HeightField*>(hfGeom.heightField)), sizeof(Gu::HeightField), 1);
	Cm::memFetchWait(1);
#if HF_TILED_MEMORY_LAYOUT
	g_sampleCache.init((uintptr_t)(hf.getData().samples), hf.getData().tilesU);
#endif
#else
	const Gu::HeightField& hf = *static_cast<Gu::HeightField*>(hfGeom.heightField);
#endif
	const Gu::HeightFieldUtil hfUtil(hfGeom, hf);

	const bool isConvexScaleIdentity = shapeConvex.scale.isIdentity();
	Cm::FastVertex2ShapeScaling convexScaling;	// PT: TODO: remove default ctor
	if(!isConvexScaleIdentity)
		convexScaling.init(shapeConvex.scale);

	PxMat33 left( hfShapeAbsPose.base0, hfShapeAbsPose.base1, hfShapeAbsPose.base2);
	PxMat33 right( convexShapeAbsPose.base0, convexShapeAbsPose.base1, convexShapeAbsPose.base2);

	Cm::Matrix34 convexShape2HfShape(left.getInverse()* right, left.getInverse()*(convexShapeAbsPose.base3 - hfShapeAbsPose.base3));

	//boxShape2HfShape.multiplyInverseRTLeft(hfShapeAbsPose, boxShapeAbsPose);


	Cm::Matrix34 convexVertex2World( right * convexScaling.getVertex2ShapeSkew(),convexShapeAbsPose.base3 );

	//Cm::Matrix34 convexVertex2World = Cm::Matrix34(context.mVertex2ShapeSkew[0].getVertex2WorldSkew(convexShapeAbsPose));

	//PxMat34Legacy hfShapeInvAbsPose = hfShapeAbsPose.getInverseRT();

	//Cm::Matrix34 convexShape2HfShape;
	//convexShape2HfShape.multiplyInverseRTLeft(hfShapeAbsPose, convexShapeAbsPose);//	= hfShapeInvAbsPose * convexShapeAbsPose;

	// Allocate space for transformed vertices.
	const Gu::ConvexHullData* PX_RESTRICT hull = shapeConvex.hullData;
	PxVec3* PX_RESTRICT convexVerticesInHfShape = (PxVec3* PX_RESTRICT)PxAlloca(hull->mNbHullVertices*sizeof(PxVec3));

	// Transform vertices to height field shape
	PxMat33 convexShape2HfShape_rot(convexShape2HfShape[0],convexShape2HfShape[1],convexShape2HfShape[2]);
	Cm::Matrix34 convexVertices2HfShape(convexShape2HfShape_rot*convexScaling.getVertex2ShapeSkew(), convexShape2HfShape[3]);
	//Cm::Matrix34 convexVertices2HfShape( context.mVertex2ShapeSkew[0].getVertex2WorldSkew(convexShape2HfShape));
	const PxVec3* const PX_RESTRICT hullVerts = hull->getHullVertices();
	for(PxU32 i = 0; i<hull->mNbHullVertices; i++)
	{
		convexVerticesInHfShape[i] = convexVertices2HfShape.transform(hullVerts[i]);
	}

	PxVec3 convexBoundsInHfShapeMin( PX_MAX_REAL,  PX_MAX_REAL,  PX_MAX_REAL);
	PxVec3 convexBoundsInHfShapeMax(-PX_MAX_REAL, -PX_MAX_REAL, -PX_MAX_REAL);

	// Compute bounds of convex in hf space
	for(PxU32 i = 0; i<hull->mNbHullVertices; i++)
	{
		const PxVec3& v = convexVerticesInHfShape[i];

		convexBoundsInHfShapeMin.x = PxMin(convexBoundsInHfShapeMin.x, v.x);
		convexBoundsInHfShapeMin.y = PxMin(convexBoundsInHfShapeMin.y, v.y);
		convexBoundsInHfShapeMin.z = PxMin(convexBoundsInHfShapeMin.z, v.z);

		convexBoundsInHfShapeMax.x = PxMax(convexBoundsInHfShapeMax.x, v.x);
		convexBoundsInHfShapeMax.y = PxMax(convexBoundsInHfShapeMax.y, v.y);
		convexBoundsInHfShapeMax.z = PxMax(convexBoundsInHfShapeMax.z, v.z);
	}

	const bool thicknessNegOrNull = (hf.getThicknessFast() <= 0.0f);	// PT: don't do this each time! FCMPs are slow.

	// Compute the height field extreme over the bounds area.
//	PxReal hfExtreme = thicknessNegOrNull ? -PX_MAX_REAL : PX_MAX_REAL;
	//PxReal oneOverRowScale = 1 / hfShape.getRowScale();
//	const PxReal oneOverRowScale = 1 / hfGeom.rowScale;
	//PxReal oneOverColumnScale = 1 / hfShape.getColumnScale();
//	const PxReal oneOverColumnScale = 1 / hfGeom.columnScale;
	const PxReal oneOverRowScale = hfUtil.getOneOverRowScale();
	const PxReal oneOverColumnScale = hfUtil.getOneOverColumnScale();

	PxU32 minRow;
	PxU32 maxRow;
	PxU32 minColumn;
	PxU32 maxColumn;

	// negative scale support
	if (oneOverRowScale > 0.0f)
	{
		minRow = hf.getMinRow(convexBoundsInHfShapeMin.x * oneOverRowScale);
		maxRow = hf.getMaxRow(convexBoundsInHfShapeMax.x * oneOverRowScale);
	}
	else
	{
		minRow = hf.getMinRow(convexBoundsInHfShapeMax.x * oneOverRowScale);
		maxRow = hf.getMaxRow(convexBoundsInHfShapeMin.x * oneOverRowScale);
	}

	if (oneOverColumnScale > 0.0f)
	{
		minColumn = hf.getMinColumn(convexBoundsInHfShapeMin.z * oneOverColumnScale);
		maxColumn = hf.getMaxColumn(convexBoundsInHfShapeMax.z * oneOverColumnScale);
	}
	else
	{
		minColumn = hf.getMinColumn(convexBoundsInHfShapeMax.z * oneOverColumnScale);
		maxColumn = hf.getMaxColumn(convexBoundsInHfShapeMin.z * oneOverColumnScale);
	}

	PxReal hfExtreme = hf.computeExtreme(minRow, maxRow, minColumn, maxColumn);

	//hfExtreme *= hfShape.getHeightScale();
	hfExtreme *= hfGeom.heightScale;


	// Return if convex is on the wrong side of the extreme.
	if (thicknessNegOrNull)
	{
		if (convexBoundsInHfShapeMin.y > hfExtreme) return false;
	}
	else
	{
		if (convexBoundsInHfShapeMax.y < hfExtreme) return false;
	}

	// Test convex vertices
	if (gCompileConvexVertex)
	{
		for(PxU32 i=0; i<hull->mNbHullVertices; i++)
		{
			const PxVec3& convexVertexInHfShape = convexVerticesInHfShape[i];

//////// SAME CODE AS IN BOX-HF
			const bool insideExtreme = thicknessNegOrNull ? (convexVertexInHfShape.y < hfExtreme) : (convexVertexInHfShape.y > hfExtreme);

			//if (insideExtreme && hfShape.isShapePointOnHeightField(convexVertexInHfShape.x, convexVertexInHfShape.z))
			if (insideExtreme && hfUtil.isShapePointOnHeightField(convexVertexInHfShape.x, convexVertexInHfShape.z))
			{
//				const PxReal y = hfShape.getHeightAtShapePoint(convexVertexInHfShape.x, convexVertexInHfShape.z);
//				const PxReal y = hfUtil.getHeightAtShapePoint(convexVertexInHfShape.x, convexVertexInHfShape.z);

// PT: compute this once, reuse results (3 times!)
// PT: TODO: also reuse this in EE tests
				PxReal fracX, fracZ;
				const PxU32 vertexIndex = hfUtil.getHeightField().computeCellCoordinates(convexVertexInHfShape.x * oneOverRowScale, convexVertexInHfShape.z * oneOverColumnScale, fracX, fracZ);

				const PxReal y = hfUtil.getHeightAtShapePoint2(vertexIndex, fracX, fracZ);

				const PxReal dy = convexVertexInHfShape.y - y;
#ifdef DISTANCE_BASED_TEST
//				if (hfShape.isDeltaHeightInsideExtent(dy - contactDistance*2.0f))
				if (hf.isDeltaHeightInsideExtent(dy, contactDistance/**2.0f*/))
#else
				if (hf.isDeltaHeightInsideExtent(dy))
#endif
				{
					const PxU32 faceIndex = hfUtil.getFaceIndexAtShapePointNoTest2(vertexIndex, fracX, fracZ);
					if (faceIndex != 0xffffffff)
					{
						PxVec3 n;
						n = hfUtil.getNormalAtShapePoint2(vertexIndex, fracX, fracZ);
						n = n.getNormalized();

						contactBuffer
#ifdef DISTANCE_BASED_TEST
							.contact(convexVertex2World.transform(hullVerts[i]), hfShapeAbsPose.rotate(n), n.y*dy/* - contactDistance*/, PXC_CONTACT_NO_FACE_INDEX, faceIndex);
#else
							.contact(convexVertex2World.transform(hullVerts[i]), hfShapeAbsPose.rotate(n), n.y*dy + contactDistance, PXC_CONTACT_NO_FACE_INDEX, faceIndex);//add contactDistance to compensate for fact that we don't support dist based contacts! See comment at start of funct.
#endif
//							.contact(convexVertex2World.transform(hullVerts[i]), hfShapeAbsPose.rotate(n), dy + contactDistance, PXC_CONTACT_NO_FACE_INDEX, feature);//add contactDistance to compensate for fact that we don't support dist based contacts! See comment at start of funct.
					}
				}
			}
////////~SAME CODE AS IN BOX-HF
		} 
	}

	// Test convex edges
	if (gCompileEdgeEdge)
	{
		if(1)
		{
			PxU32 numPolygons = hull->mNbPolygons;
			const Gu::HullPolygonData* polygons = hull->mPolygons;
			const PxU8* vertexData = hull->getVertexData8();

			ConvexEdge edges[512];
			PxU32 nbEdges = findUniqueConvexEdges(512, edges, numPolygons, polygons, vertexData);

			for(PxU32 i=0;i<nbEdges;i++)
			{
				const PxVec3 convexNormalInHfShape = convexVertices2HfShape.rotate(edges[i].normal);
				if(convexNormalInHfShape.y>0.0f)
					continue;

				const PxU8 vi0 = edges[i].vref0;
				const PxU8 vi1 = edges[i].vref1;

				const PxVec3& sv0 = convexVerticesInHfShape[vi0];
				const PxVec3& sv1 = convexVerticesInHfShape[vi1];

				if (thicknessNegOrNull) 
				{
					if ((sv0.y > hfExtreme) && (sv1.y > hfExtreme)) continue;
				}
				else
				{
					if ((sv0.y < hfExtreme) && (sv1.y < hfExtreme)) continue;
				}
				PxcContactTraceSegmentCallback cb(sv1 - sv0, contactBuffer, hfShapeAbsPose, contactDistance);
				hfUtil.traceSegment<PxcContactTraceSegmentCallback, true, true>(sv0, sv1, &cb);
			}
		}
		else
		{
			Gu::EdgeCache edgeCache;
			PxU32 numPolygons = hull->mNbPolygons;
			const Gu::HullPolygonData* polygons = hull->mPolygons;
			const PxU8* vertexData = hull->getVertexData8();
			while (numPolygons--)
			{
				const Gu::HullPolygonData& polygon = *polygons++;
	//			const PxU8* vRefBase = hull->mVertexData8 + polygon.mVRef8;
				const PxU8* vRefBase = vertexData + polygon.mVRef8;

				PxU32 numEdges = polygon.mNbVerts;

				PxU32 a = numEdges - 1;
				PxU32 b = 0;
				while(numEdges--)
				{
					PxU8 vi0 =  vRefBase[a];
					PxU8 vi1 =	vRefBase[b];


					if(vi1 < vi0)
					{
						PxU8 tmp = vi0;
						vi0 = vi1;
						vi1 = tmp;
					}

					a = b;
					b++;

					if (edgeCache.isInCache(vi0, vi1))	//avoid processing edges 2x if possible (this will typically have cache misses about 5% of the time leading to 5% redundant work.
						continue;

					const PxVec3& sv0 = convexVerticesInHfShape[vi0];
					const PxVec3& sv1 = convexVerticesInHfShape[vi1];

					if (thicknessNegOrNull) 
					{
						if ((sv0.y > hfExtreme) && (sv1.y > hfExtreme)) continue;
					}
					else
					{
						if ((sv0.y < hfExtreme) && (sv1.y < hfExtreme)) continue;
					}
					PxcContactTraceSegmentCallback cb(sv1 - sv0,
						contactBuffer,
						hfShapeAbsPose, contactDistance/*, context.mRenderOutput*/);
					//hfShape.traceSegment<PxcContactTraceSegmentCallback>(sv0, sv1, &cb);
					hfUtil.traceSegment<PxcContactTraceSegmentCallback, true, true>(sv0, sv1, &cb);
				}
			}
		}
	}

	// Test height field vertices
	if (gCompileHeightFieldVertex)
	{

		//Cm::Matrix34 hfShape2ConvexShape = convexShape2HfShape.getInverseRT();

		// Iterate over HeightField vertices inside the projected box bounds.
		for(PxU32 row = minRow; row <= maxRow; row++)
		{
			for(PxU32 column = minColumn; column <= maxColumn; column++)
			{
				const PxU32 vertexIndex = row * hf.getNbColumnsFast() + column;

				//if (!hfShape.isCollisionVertex(vertexIndex)) continue;
				if (!hfUtil.isCollisionVertex(vertexIndex, row, column)) continue;

				//PxVec3 hfVertex;
				//hfVertex.set(hfShape.getRowScale() * row, hfShape.getHeightScale() * hfShape.getHeight(vertexIndex), hfShape.getColumnScale() * column);
				const PxVec3 hfVertex(hfGeom.rowScale * row, hfGeom.heightScale * hf.getHeight(vertexIndex), hfGeom.columnScale * column);

				//PxVec3 hfVertexNormal = thicknessNegOrNull ? hfShape.getVertexNormal(vertexIndex) : -hfShape.getVertexNormal(vertexIndex);
//				PxVec3 hfVertexNormal = thicknessNegOrNull ? hfUtil.getVertexNormal(vertexIndex) : -hfUtil.getVertexNormal(vertexIndex);
				const PxVec3 nrm = hfUtil.getVertexNormal(vertexIndex, row, column);
				PxVec3 hfVertexNormal = thicknessNegOrNull ? nrm : -nrm;
				hfVertexNormal = hfVertexNormal.getNormalized();
//				const PxVec3 hfVertexNormalInConvexShape = convexShape2HfShape.transformTranspose(hfVertexNormal);
				const PxVec3 hfVertexNormalInConvexShape = convexShape2HfShape.rotateTranspose(hfVertexNormal);
//PxMat33 rot(convexShape2HfShape[0],convexShape2HfShape[1],convexShape2HfShape[2]);
//PxVec3 hfVertexInConvexShape = rot.getInverse() * hfVertex;
//PxVec3 hfVertexInConvexShape2 = convexShape2HfShape.rotateTranspose(hfVertex);
PxVec3 hfVertexInConvexShape = convexShape2HfShape.transformTranspose(hfVertex);
				//PxVec3 hfVertexInConvexShape = convexShape2HfShape % hfVertex;
				PxReal depth;
				PxVec3 normal;
				//Gu::Point iceNormal;
				//Gu::Point icePoint      = IceMaths::Point(hfVertexInConvexShape.x, hfVertexInConvexShape.y, hfVertexInConvexShape.z);
				//Gu::Point iceSafeNormal = IceMaths::Point(hfVertexNormalInConvexShape.x, hfVertexNormalInConvexShape.y, hfVertexNormalInConvexShape.z);
				if (!PxcDepenetrateConvex(hfVertexInConvexShape, -hfVertexNormalInConvexShape, *hull, contactDistance, normal, depth, 
					convexScaling,
					isConvexScaleIdentity))
				{
					continue;
				}
				//PxVec3 normal = PxVec3(iceNormal.x, iceNormal.y, iceNormal.z);
				
//PxVec3 normalInHfShape = rot * (-normal);
PxVec3 normalInHfShape = convexShape2HfShape.rotate(-normal);
				//PxVec3 normalInHfShape = convexShape2HfShape.M * (-normal);
				//hfShape.clipShapeNormalToVertexVoronoi(normalInHfShape, vertexIndex);
				hfUtil.clipShapeNormalToVertexVoronoi(normalInHfShape, vertexIndex, row, column);
				if (normalInHfShape.dot(hfVertexNormal) < PX_EPS_REAL) 
				{
					// hmm, I dont think this can happen
					continue;
				}
				normalInHfShape = normalInHfShape.getNormalized();
				/*
				if (row != minRow && row != maxRow && column != minColumn && column != maxColumn && normalInHfShape.dot(hfVertexNormal) < 0.9)
				{
				int brejk = 1;
				PxVec3 n2 = convexShape2HfShape.M * (-normal);
				hfShape.clipShapeNormalToVertexVoronoi(n2, vertexIndex);
				}
				*/
				PxU32 faceIndex = hfUtil.getVertexFaceIndex(vertexIndex, row, column);
				contactBuffer
					.contact(hfShapeAbsPose.transform(hfVertex), hfShapeAbsPose.rotate(normalInHfShape),
#ifdef DISTANCE_BASED_TEST
					-depth,
#else
					-depth + contactDistance,	//add contactDistance to compensate for fact that we don't support dist based contacts! See comment at start of funct.
#endif
					PXC_CONTACT_NO_FACE_INDEX, faceIndex);
			}
		}
	}

	return contactBuffer.count>0;
}
}