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

#include "GuGJKPenetrationWrapper.h"
#include "GuEPAPenetrationWrapper.h"
#include "GuVecBox.h"
#include "GuVecShrunkBox.h"
#include "GuGeometryUnion.h"

#include "GuConvexHelper.h"
#include "GuPCMShapeConvex.h"
#include "GuContactMethodImpl.h"
#include "GuContactBuffer.h"
#include "GuPCMContactGenUtil.h"


using namespace physx;
using namespace Gu;



namespace physx
{

namespace Gu
{
////p0 and p1 is in the local space of AABB
static bool intersectSegmentAABB(const Ps::aos::Vec3VArg p0, const Ps::aos::Vec3VArg d, const Ps::aos::Vec3VArg max, const Ps::aos::Vec3VArg min, Ps::aos::FloatV& tmin, Ps::aos::FloatV& tmax)
{
	using namespace Ps::aos;
	
	const BoolV bFalse = BFFFF();
	const Vec3V eps = V3Load(1e-6);
	const Vec3V absV = V3Abs(d);
	const Vec3V zero = V3Zero();
	const Vec3V fMax = Vec3V_From_FloatV(FMax());

	FloatV tminf = FZero();
	FloatV tmaxf = FOne();
	const BoolV isParallel = V3IsGrtr(eps, absV);
	const BoolV isOutsideOfRange = BOr(V3IsGrtr(p0, max), V3IsGrtr(min, p0));

	if(!BAllEq(BAnd(isParallel, isOutsideOfRange), bFalse))
	{
		return false;
	}

	const Vec3V odd = V3Recip(d);
	const Vec3V t1 = V3Sel(isParallel, zero, V3Mul(V3Sub(min, p0), odd));
	const Vec3V t2 = V3Sel(isParallel, fMax, V3Mul(V3Sub(max, p0), odd));

	const BoolV swap = V3IsGrtr(t1, t2);
	const Vec3V tt1 = V3Sel(swap, t2, t1);
	const Vec3V tt2 = V3Sel(swap, t1, t2);

	const FloatV ft1 = V3ExtractMax(tt1);
	const FloatV ft2 = V3ExtractMin(tt2);

	tminf = FSel(FIsGrtr(ft1, tminf), ft1, tminf);
	tmaxf = FSel(FIsGrtr(tmaxf, ft2), ft2, tmaxf);

	tmin = tminf;
	tmax = tmaxf;

	const BoolV con0 = FIsGrtr(tminf, tmaxf);
	const BoolV con1 = FIsGrtr(tminf, FOne());
	const BoolV isNotIntersect = BOr(con0, con1);//V3IsGrtr(tminf, tmaxf);
	return BAllEq(isNotIntersect, bFalse) == 1;
	
}

static void getIncidentPolygon(Ps::aos::Vec3V* pts, Ps::aos::Vec3V& faceNormal, const Ps::aos::Vec3VArg axis, const Ps::aos::PsMatTransformV& transf1To0, 
							   const Ps::aos::Vec3VArg extents)
{

	using namespace Ps::aos;

	const FloatV zero = FZero();

	FloatV ex = V3GetX(extents);
	FloatV ey = V3GetY(extents);
	FloatV ez = V3GetZ(extents);

	const Vec3V u0 = transf1To0.getCol0();
	const Vec3V u1 = transf1To0.getCol1();
	const Vec3V u2 = transf1To0.getCol2();

	//calcaulte the insident face for b
	const FloatV d0 = V3Dot(transf1To0.getCol0(), axis);
	const FloatV d1 = V3Dot(transf1To0.getCol1(), axis);
	const FloatV d2 = V3Dot(transf1To0.getCol2(), axis);

	const FloatV absd0 = FAbs(d0);
	const FloatV absd1 = FAbs(d1);
	const FloatV absd2 = FAbs(d2);


//	Vec3V pts[4];
	Vec3V r0, r1, r2;

	// 0 +------+ 1   
	//   |      |      
	//   |  *   |      
	//   |      |      
	// 3 +------+ 2   

	if(FAllGrtrOrEq(absd0, absd1) && FAllGrtrOrEq(absd0, absd2))
	{
		//the incident face is on u0
		faceNormal = V3Sel(FIsGrtr(d0, zero), V3Neg(u0), u0);
		ex = FSel(FIsGrtr(d0, zero), FNeg(ex), ex);
		r0 = V3Scale(u0, ex);
		r1 = V3Scale(u1, ey);
		r2 = V3Scale(u2, ez);

		const Vec3V temp0 = V3Add(transf1To0.p, r0);
		const Vec3V temp1 = V3Add(r1, r2);
		const Vec3V temp2 = V3Sub(r1, r2);
		

		pts[0] = V3Add(temp0, temp1);	// (-x/x,  y,  z)
		pts[1] = V3Add(temp0, temp2);	// (-x/x,  y, -z)
		pts[2] = V3Sub(temp0, temp1);	// (-x/x, -y, -z)
		pts[3] = V3Sub(temp0, temp2);	// (-x/x, -y,  z)

		//pts[0] = V3Add(transf1To0.p, V3Add(r0, V3Add(r1, r2)));	// (-x/x,  y,  z)
		//pts[1] = V3Add(transf1To0.p, V3Add(r0, V3Sub(r1, r2)));	// (-x/x,  y, -z)
		//pts[2] = V3Add(transf1To0.p, V3Sub(r0, V3Add(r1, r2)));	// (-x/x, -y, -z)
		//pts[3] = V3Add(transf1To0.p, V3Add(r0, V3Sub(r2, r1)));	// (-x/x, -y,  z)

	}
	else if(FAllGrtrOrEq(absd1, absd2))
	{
		//the incident face is on u1
		faceNormal = V3Sel(FIsGrtr(d1, zero), V3Neg(u1), u1);
		ey = FSel(FIsGrtr(d1, zero), FNeg(ey), ey);
		r0 = V3Scale(u0, ex);
		r1 = V3Scale(u1, ey);
		r2 = V3Scale(u2, ez);

		const Vec3V temp0 = V3Add(transf1To0.p, r1);
		const Vec3V temp1 = V3Add(r0, r2);
		const Vec3V temp2 = V3Sub(r0, r2);
		
		pts[0] = V3Add(temp0, temp1);	// (x,  -y/y,  z)
		pts[1] = V3Add(temp0, temp2);	// (x,  -y/y, -z)
		pts[2] = V3Sub(temp0, temp1);	// (-x, -y/y, -z)
		pts[3] = V3Sub(temp0, temp2);	// (-x, -y/y,  z)

		//pts[0] = V3Add(transf1To0.p, V3Add(r1, V3Add(r0, r2))); // (x,  -y/y,  z)
		//pts[1] = V3Add(transf1To0.p, V3Add(r1, V3Sub(r0, r2))); // (x,  -y/y, -z)
		//pts[2] = V3Add(transf1To0.p, V3Sub(r1, V3Add(r0, r2))); // (-x, -y/y, -z)
		//pts[3] = V3Add(transf1To0.p, V3Add(r1, V3Sub(r2, r0)));	// (-x, -y/y,  z)

	}
	else
	{
		//the incident face is on u2
		faceNormal = V3Sel(FIsGrtr(d2, zero), V3Neg(u2), u2);
		ez = FSel(FIsGrtr(d2, zero), FNeg(ez), ez);
		r0 = V3Scale(u0, ex);
		r1 = V3Scale(u1, ey);
		r2 = V3Scale(u2, ez);

		const Vec3V temp0 = V3Add(transf1To0.p, r2);
		const Vec3V temp1 = V3Add(r0, r1);
		const Vec3V temp2 = V3Sub(r0, r1);
		
		pts[0] = V3Add(temp0, temp1);	// ( x,   y,  z)
		pts[1] = V3Add(temp0, temp2);	// ( x,  -y,  z)
		pts[2] = V3Sub(temp0, temp1);	// (-x,  -y,  z)
		pts[3] = V3Sub(temp0, temp2);	// (-x,   y,  z)

		//pts[0] = V3Add(transf1To0.p, V3Add(r2, V3Add(r0, r1))); // (x,  y,  z)
		//pts[1] = V3Add(transf1To0.p, V3Add(r2, V3Sub(r0, r1))); // (x,  -y, z)
		//pts[2] = V3Add(transf1To0.p, V3Sub(r2, V3Add(r0, r1))); // (-x, -y, z)
		//pts[3] = V3Add(transf1To0.p, V3Add(r2, V3Sub(r1, r0)));	// (-x, y,  z)

	}
}


static void getPolygon(Ps::aos::Vec3V* pts, Ps::aos::Vec3V& faceNormal, const Ps::aos::Vec3VArg axis, const Ps::aos::Vec3VArg extents)
{

	using namespace Ps::aos;

	const FloatV zero = FZero();

	FloatV ex = V3GetX(extents);
	FloatV ey = V3GetY(extents);
	FloatV ez = V3GetZ(extents);

	const Vec3V u0 = V3UnitX();
	const Vec3V u1 = V3UnitY();
	const Vec3V u2 = V3UnitZ();

	//calcaulte the insident face for b
	const FloatV d0 = V3GetX(axis);
	const FloatV d1 = V3GetY(axis);
	const FloatV d2 = V3GetZ(axis);

	const FloatV absd0 = FAbs(d0);
	const FloatV absd1 = FAbs(d1);
	const FloatV absd2 = FAbs(d2);


//	Vec3V pts[4];
	Vec3V r0, r1, r2;

	// 0 +------+ 1   
	//   |      |      
	//   |  *   |      
	//   |      |      
	// 3 +------+ 2   

	if(FAllGrtrOrEq(absd0, absd1) && FAllGrtrOrEq(absd0, absd2))
	{
		//the incident face is on u0
		faceNormal = V3Sel(FIsGrtr(d0, zero), V3Neg(u0), u0);
		ex = FSel(FIsGrtr(d0, zero), FNeg(ex), ex);
		r0 = V3Scale(u0, ex);
		r1 = V3Scale(u1, ey);
		r2 = V3Scale(u2, ez);

		const Vec3V temp0 = r0;
		const Vec3V temp1 = V3Add(r1, r2);
		const Vec3V temp2 = V3Sub(r1, r2);
		

		pts[0] = V3Add(temp0, temp1);	// (-x/x,  y,  z)
		pts[1] = V3Add(temp0, temp2);	// (-x/x,  y, -z)
		pts[2] = V3Sub(temp0, temp1);	// (-x/x, -y, -z)
		pts[3] = V3Sub(temp0, temp2);	// (-x/x, -y,  z)

	}
	else if(FAllGrtrOrEq(absd1, absd2))
	{
		//the incident face is on u1
		faceNormal = V3Sel(FIsGrtr(d1, zero), V3Neg(u1), u1);
		ey = FSel(FIsGrtr(d1, zero), FNeg(ey), ey);
		r0 = V3Scale(u0, ex);
		r1 = V3Scale(u1, ey);
		r2 = V3Scale(u2, ez);

		const Vec3V temp0 = r1;
		const Vec3V temp1 = V3Add(r0, r2);
		const Vec3V temp2 = V3Sub(r0, r2);
		
		pts[0] = V3Add(temp0, temp1);	// (x,  -y/y,  z)
		pts[1] = V3Add(temp0, temp2);	// (x,  -y/y, -z)
		pts[2] = V3Sub(temp0, temp1);	// (-x, -y/y, -z)
		pts[3] = V3Sub(temp0, temp2);	// (-x, -y/y,  z)

	}
	else
	{
		//the incident face is on u2
		faceNormal = V3Sel(FIsGrtr(d2, zero), V3Neg(u2), u2);
		ez = FSel(FIsGrtr(d2, zero), FNeg(ez), ez);
		r0 = V3Scale(u0, ex);
		r1 = V3Scale(u1, ey);
		r2 = V3Scale(u2, ez);

		const Vec3V temp0 = r2;
		const Vec3V temp1 = V3Add(r0, r1);
		const Vec3V temp2 = V3Sub(r0, r1);
		
		pts[0] = V3Add(temp0, temp1);	// ( x,   y,  z)
		pts[1] = V3Add(temp0, temp2);	// ( x,  -y,  z)
		pts[2] = V3Sub(temp0, temp1);	// (-x,  -y,  z)
		pts[3] = V3Sub(temp0, temp2);	// (-x,   y,  z)
	}
}
//static void drawBox( PxcPersistentContactManifold& manifold, const Ps::aos::PsMatTransformV& transform, const Ps::aos::Vec3VArg extents, const PxU32 color = 0xfffffff0)
//{
//	using namespace Ps::aos;
//
//	Vec3V worldPts[8];
//	const Vec3V newExtents =extents;
//	Gu::BoxV box;
//	box.computeOBBPoints(worldPts, transform.p, newExtents, transform.getCol0(), transform.getCol1(), transform.getCol2());
//
//	//     7+------+6			0 = ---
//	//     /|     /|			1 = +--
//	//    / |    / |			2 = ++-
//	//   / 4+---/--+5			3 = -+-
//	// 3+------+2 /    y   z	4 = --+
//	//  | /    | /     |  /		5 = +-+
//	//  |/     |/      |/		6 = +++
//	// 0+------+1      *---x	7 = -++
//	manifold.drawLine(gThreadContext, worldPts[0], worldPts[1], color);
//	manifold.drawLine(gThreadContext, worldPts[1], worldPts[2], color);
//	manifold.drawLine(gThreadContext, worldPts[2], worldPts[3], color);
//	manifold.drawLine(gThreadContext, worldPts[3], worldPts[0], color);
//
//	manifold.drawLine(gThreadContext, worldPts[1], worldPts[5], color);
//	manifold.drawLine(gThreadContext, worldPts[6], worldPts[2], color);
//
//	manifold.drawLine(gThreadContext, worldPts[4], worldPts[5], color);
//	manifold.drawLine(gThreadContext, worldPts[5], worldPts[6], color);
//	manifold.drawLine(gThreadContext, worldPts[6], worldPts[7], color);
//	manifold.drawLine(gThreadContext, worldPts[7], worldPts[4], color);
//
//	manifold.drawLine(gThreadContext, worldPts[0], worldPts[4], color);
//	manifold.drawLine(gThreadContext, worldPts[7], worldPts[3], color);
//}


//static void addContactsAToB(const Ps::aos::Vec3VArg localNormal, const Ps::aos::Vec3VArg contactNormal, const Ps::aos::FloatVArg planeD, const Ps::aos::Vec3VArg point, const PsMatTransformV& transf0To1, PxcPersistentContactManifold& manifold, const Ps::aos::FloatVArg replaceBreakingThreshold)
//{
//	using namespace Ps::aos;
//	const FloatV t = FSub(planeD, V3Dot(contactNormal, point));
//	const Vec3V localPointA = V3ScaleAdd(contactNormal, t, point);
//	const Vec3V localPointB = transf0To1.transform(point);
//	const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(localNormal), FNeg(t));
//	manifold.addManifoldPoint(localPointA, localPointB, localNormalPen, replaceBreakingThreshold);
//}
//
//static void addContactsBToA(const Ps::aos::Vec3VArg localNormal, const Ps::aos::Vec3VArg contactNormal, const Ps::aos::FloatVArg planeD, const Ps::aos::Vec3VArg point, const PsMatTransformV& transf0To1, PxcPersistentContactManifold& manifold, const Ps::aos::FloatVArg replaceBreakingThreshold)
//{
//	using namespace Ps::aos;
//	const FloatV t = FSub(planeD, V3Dot(contactNormal, point));
//	const Vec3V localPointA = transf0To1.transform(point); 
//	const Vec3V localPointB = V3ScaleAdd(contactNormal, t, point);
//	const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(localNormal), FNeg(t));
//	manifold.addManifoldPoint(localPointA, localPointB, localNormalPen, replaceBreakingThreshold);  
//}

//bool contains(Ps::aos::Vec3V* verts, const PxU32 numVerts, const Ps::aos::Vec3VArg p, const Ps::aos::Vec3VArg min, const Ps::aos::Vec3VArg max);

template <bool aToB>
static void calculateContacts(const Ps::aos::Vec3VArg sabExtents, const Ps::aos::Vec3VArg nsabExtents, const Ps::aos::Vec3VArg extents0, const Ps::aos::PsMatTransformV& transform0, const Ps::aos::Vec3VArg extents1, const Ps::aos::PsMatTransformV& transform1, const Ps::aos::Vec3VArg contactNormal, const Ps::aos::FloatVArg planeD,
							  Gu::PersistentContactManifold& manifold, Gu::PersistentContact* manifoldContacts, PxU32& numContacts,const Ps::aos::FloatVArg replaceBreakingThreshold, const Ps::aos::FloatVArg contactDist, const PxU32 axis)
{
	PX_UNUSED(axis);
	PX_UNUSED(replaceBreakingThreshold);
	PX_UNUSED(manifold);

	using namespace Ps::aos;
	
	const PsMatTransformV& transf1To0 = transform0.transformInv(transform1);
	const PsMatTransformV& transf0To1 = transform1.transformInv(transform0);
	
	const Vec3V localNormal = aToB ? transf0To1.rotate(V3Neg(contactNormal)) : contactNormal;

	Vec3V iFaceNormalIn0;
	Vec3V pts[4];
	getIncidentPolygon(pts, iFaceNormalIn0, contactNormal, transf1To0, extents1);

	Vec3V rFaceNormal;
	Vec3V referencePts[4];
	getPolygon(referencePts, rFaceNormal, V3Neg(contactNormal), extents0);

	//this is the matrix transform all points to the 2d plane
	const Mat33V rot = findRotationMatrixFromZAxis(contactNormal);

	BoolV bTrue = BTTTT();
	//Vec3V eps = Vec3V_From_FloatV(FEps());
	Vec3V max = Vec3V_From_FloatV(FMax());
	Vec3V nmax = V3Neg(max); 

	Vec3V iPolygonMin= max; 
	Vec3V iPolygonMax = nmax;

	Vec3V incidentFaceIn2D[4];
	for(PxU32 i=0; i<4; ++i)
	{
		incidentFaceIn2D[i] = M33TrnspsMulV3(rot, pts[i]);
		iPolygonMin = V3Min(iPolygonMin, incidentFaceIn2D[i]);
		iPolygonMax = V3Max(iPolygonMax, incidentFaceIn2D[i]);
	}

	//transform reference polygon to 2d, calculate min and max
	Vec3V rPolygonMin= max;
	Vec3V rPolygonMax = nmax;
	Vec3V referenceFaceIn2D[4];
	for(PxU32 i=0; i<4; ++i)
	{
		referenceFaceIn2D[i] = M33TrnspsMulV3(rot, referencePts[i]);
		rPolygonMin = V3Min(rPolygonMin, referenceFaceIn2D[i]);
		rPolygonMax = V3Max(rPolygonMax, referenceFaceIn2D[i]);
	}

	const FloatV d = V3GetZ(referencePts[0]);
	const FloatV rd = FAdd(d, contactDist);
	for(PxU32 i=0; i<4; ++i)
	{
		if(FAllGrtr(rd, V3GetZ(incidentFaceIn2D[i])))
		{
			const BoolV con0 = V3IsGrtrOrEq(incidentFaceIn2D[i], rPolygonMin);
			const BoolV con1 = V3IsGrtrOrEq(rPolygonMax, incidentFaceIn2D[i]);
			const BoolV con = BAnd(con0, con1);
			if(BAllEq(BAnd(BGetX(con), BGetY(con)), bTrue))
			{
				const Vec3V point = pts[i];//transf1To0.transform(pts[i]);
				const FloatV t = V3Dot(rFaceNormal, V3Sub(referencePts[0], point));
				if(FAllGrtr(contactDist, FNeg(t)))
				{
					const Vec3V localPointA = V3ScaleAdd(rFaceNormal, t, point);  //transf0To1.transform(point); 
					const Vec3V localPointB = transf1To0.transformInv(pts[i]);//pts[i];
					const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(localNormal), FNeg(t));
					if(aToB)
					{
						manifoldContacts[numContacts].mLocalPointA = localPointA;
						manifoldContacts[numContacts].mLocalPointB = localPointB;
						manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;
					}
					else
					{
						manifoldContacts[numContacts].mLocalPointA = localPointB;
						manifoldContacts[numContacts].mLocalPointB = localPointA;
						manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;
					}
					/*if(aToB)
						manifold.addManifoldPoint(localPointA, localPointB, localNormalPen, replaceBreakingThreshold);
					else
						manifold.addManifoldPoint(localPointB, localPointA, localNormalPen, replaceBreakingThreshold);*/
				}
			}
		}
	}


	for(PxU32 i=0; i<4; ++i)
	{
		if(contains(incidentFaceIn2D, 4, referenceFaceIn2D[i], iPolygonMin, iPolygonMax))
		{
			const FloatV t = V3Dot(iFaceNormalIn0, V3Sub(pts[0], referencePts[i]));
			if(FAllGrtr(contactDist, FNeg(t)))
			{
				const Vec3V projPoint = V3ScaleAdd(iFaceNormalIn0, t, referencePts[i]);

				const Vec3V v = V3Sub(projPoint, referencePts[i]);
				const FloatV t3 = V3Dot(v, contactNormal);

				const Vec3V localPointA = referencePts[i]; 
				const Vec3V localPointB = transf1To0.transformInv(projPoint);
				const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(localNormal), t3);
				if(aToB)
				{
					manifoldContacts[numContacts].mLocalPointA = localPointA;
					manifoldContacts[numContacts].mLocalPointB = localPointB;
					manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;
				}
				else
				{
					manifoldContacts[numContacts].mLocalPointA = localPointB;
					manifoldContacts[numContacts].mLocalPointB = localPointA;
					manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;
				}

				/*if(aToB)
					manifold.addManifoldPoint(localPointA, localPointB, localNormalPen, replaceBreakingThreshold);
				else
					manifold.addManifoldPoint(localPointB, localPointA, localNormalPen, replaceBreakingThreshold);*/
			}
		}


	}

	/*Vec3V p0 = transform0.transform(pts[0]);
	Vec3V p1 = transform0.transform(pts[1]);
	Vec3V p2 = transform0.transform(pts[2]);
	Vec3V p3 = transform0.transform(pts[3]);

	manifold.drawLine(gThreadContext, p0, p1);
	manifold.drawLine(gThreadContext, p1, p2);
	manifold.drawLine(gThreadContext, p2, p3);
	manifold.drawLine(gThreadContext, p3, p0);

	p0 = transform0.transform(referencePts[0]);
	p1 = transform0.transform(referencePts[1]);
	p2 = transform0.transform(referencePts[2]);
	p3 = transform0.transform(referencePts[3]);

	manifold.drawLine(gThreadContext, p0, p1, 0x00ff0000);
	manifold.drawLine(gThreadContext, p1, p2, 0x00ff0000);
	manifold.drawLine(gThreadContext, p2, p3, 0x00ff0000);
	manifold.drawLine(gThreadContext, p3, p0, 0x00ff0000);

	FloatV ex = V3GetX(extents0);
	FloatV ey = V3GetY(extents0);
	FloatV ez = V3GetZ(extents0);*/


	//test four edges with the side plane of the reference face
	const static PxU32 indices[]={ 0,1, 1,2, 2,3, 3,0};

	//do slab test
	/*static PxU32 start = 0;
	static PxU32 end = 4;*/

	//drawBox(manifold, transform0, extents0);

	FloatV tmin;
	FloatV tmax;
	
	
	for(PxU32 i = 0; i<8; i+=2)
	{
		const PxU32 j = i+1;
		//one of the point is outside 
		//if((penetrated[indices[i]]|| penetrated[indices[j]]) && (!area[indices[i]] || !area[indices[j]]))
		{
			const Vec3V p0 = pts[indices[i]];
			const Vec3V p1 = pts[indices[j]];
			const Vec3V p0p1 = V3Sub(p1, p0);
			//drawLine(transform0.transform(p0), transform0.transform(p1));

			//if(intersectSegmentAABB(p0, p0p1, extents0, nextents, referencePlane, tmin, tmax))
			if(intersectSegmentAABB(p0, p0p1, sabExtents, nsabExtents, tmin, tmax))
			{
				//add to the manifold
				const Vec3V intersectPoint0 = V3ScaleAdd(p0p1, tmin, p0);

				const FloatV t = FSub(planeD, V3Dot(contactNormal, intersectPoint0));
				const Vec3V localPointA = V3ScaleAdd(contactNormal, t, intersectPoint0);
				const Vec3V localPointB = transf0To1.transform(intersectPoint0);
				const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(localNormal), FNeg(t));

				if(aToB)
				{
					manifoldContacts[numContacts].mLocalPointA = localPointA;
					manifoldContacts[numContacts].mLocalPointB = localPointB;
					manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;
				}
				else
				{
					manifoldContacts[numContacts].mLocalPointA = localPointB;
					manifoldContacts[numContacts].mLocalPointB = localPointA;
					manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;
				}
				/*if(aToB)
					manifold.addManifoldPoint(localPointA, localPointB, localNormalPen, replaceBreakingThreshold);
				else
					manifold.addManifoldPoint(localPointB, localPointA, localNormalPen, replaceBreakingThreshold);
	*/
				if(FAllEq(tmin, tmax)==0)  
				{
					const Vec3V intersectPoint1 = V3ScaleAdd(p0p1, tmax, p0);
					const FloatV t = FSub(planeD, V3Dot(contactNormal, intersectPoint1));
					const Vec3V localPointA = V3ScaleAdd(contactNormal, t, intersectPoint1);
					const Vec3V localPointB = transf0To1.transform(intersectPoint1);
					const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(localNormal), FNeg(t));

					if(aToB)
					{
						manifoldContacts[numContacts].mLocalPointA = localPointA;
						manifoldContacts[numContacts].mLocalPointB = localPointB;
						manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;
					}
					else
					{
						manifoldContacts[numContacts].mLocalPointA = localPointB;
						manifoldContacts[numContacts].mLocalPointB = localPointA;
						manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;
					}

					/*if(aToB)
						manifold.addManifoldPoint(localPointA, localPointB, localNormalPen, replaceBreakingThreshold);
					else
						manifold.addManifoldPoint(localPointB, localPointA, localNormalPen, replaceBreakingThreshold);*/
				}
			}
		}	
	}

}



/*
	box0 in itself local space
	box1 in itself local space
	normal in box1's local space
*/

//const PxI32 PX_ALIGN(16, iInd0[2][4]) = {{0,1,2,0}, {3,4,5,0}};
//
//static PxU32 doBoxBoxGenerateContacts(const Gu::BoxV& box0, const Gu::BoxV& box1, const Ps::aos::PsMatTransformV& transform0, const Ps::aos::PsMatTransformV& transform1, const Ps::aos::FloatVArg replaceBreakingThreshold, const Ps::aos::FloatVArg contactDist, PxcPersistentContactManifold& manifold)
//{
//	using namespace Ps::aos;
//
//	FloatV min = FMax();
//	const FloatV ea0 = V3GetX(box0.extents);
//	const FloatV ea1 = V3GetY(box0.extents);
//	const FloatV ea2 = V3GetZ(box0.extents);
//
//	const FloatV eb0 = V3GetX(box1.extents);
//	const FloatV eb1 = V3GetY(box1.extents);
//	const FloatV eb2 = V3GetZ(box1.extents);
//
//	const BoolV bTrue = BTTTT();
//
//	//const PsMatTransformV aToB = transform1.transformInv(transform0);
//	const PsMatTransformV transform1To0 = transform0.transformInv(transform1);
//	//const PsMatTransformV transform0To1 = transform1.transformInv(transform0);
//	const Mat33V rot0To1 =M33Trnsps(transform1To0.rot);
//
//	//Get the MTD
//
////	const FloatV eps = FloatV_From_F32(0.999f);
//	const Vec3V uEps = Vec3V_From_F32(1e-6f);
//
//	const FloatV zero = FZero();
//
//	const FloatV tx = V3GetX(transform1To0.p);
//	const FloatV ty = V3GetY(transform1To0.p);
//	const FloatV tz = V3GetZ(transform1To0.p);
//	const Vec3V col0 = transform1To0.getCol0();
//	const Vec3V col1 = transform1To0.getCol1();
//	const Vec3V col2 = transform1To0.getCol2();
//
//	const Vec3V abs1To0Col0 = V3Add(V3Abs(col0), uEps);
//	const Vec3V abs1To0Col1 = V3Add(V3Abs(col1), uEps);
//	const Vec3V abs1To0Col2 = V3Add(V3Abs(col2), uEps);
//
//	const Vec3V abs0To1Col0 = V3Add(V3Abs(rot0To1.col0), uEps);
//	const Vec3V abs0To1Col1 = V3Add(V3Abs(rot0To1.col1), uEps);
//	const Vec3V abs0To1Col2 = V3Add(V3Abs(rot0To1.col2), uEps);
//
//
//	Vec3V sign0, sign1;
//	Vec3V overlap0, overlap1;
//	Vec3V contactDistV = Vec3V_From_FloatV(contactDist);
//
//	Vec3V ra, rb;
//	//ua0, ua1, ua2
//	{
//		
//		sign0 = transform1To0.p;
//
//		const Vec3V vtemp0 = V3Mul(abs1To0Col0, eb0);
//		const Vec3V vtemp1 = V3Mul(abs1To0Col1, eb1);
//		const Vec3V vtemp2 = V3Mul(abs1To0Col2, eb2);
//
//		rb = V3Add(vtemp0, V3Add(vtemp1, vtemp2));
//
//		const Vec3V radiusSum = V3Add(V3Add(rb, box0.extents), contactDistV);
//		overlap0 =  V3Sub(radiusSum, V3Abs(sign0));
//		if(!V3AllGrtr(overlap0, zero))
//			return false;
//	}
//
//	//ub0, ub1, ub2
//	{
//		sign1 = M33TrnspsMulV3(transform1To0.rot, transform1To0.p);
//
//		const Vec3V vtemp0 = V3Mul(abs0To1Col0, ea0);
//		const Vec3V vtemp1 = V3Mul(abs0To1Col1, ea1);
//		const Vec3V vtemp2 = V3Mul(abs0To1Col2, ea2);
//
//		ra = V3Add(vtemp0, V3Add(vtemp1, vtemp2));
//
//		const Vec3V radiusSum = V3Add(V3Add(ra, box1.extents), contactDistV);
//		overlap1 =  V3Sub(radiusSum, V3Abs(sign1));
//		if(!V3AllGrtr(overlap1, zero))
//			return false;
//	
//	}
//
//
//
//	//ua0 X ub0
//	{
//		//B into A's space, ua0Xub0[0,-b3, b2]
//		const FloatV absSign = FAbs(FSub(FMul(V3GetY(col0), tz), FMul(V3GetZ(col0), ty)));
//
//		//B into A's space, ua0Xub0[0,-b3, b2]
//		const FloatV vtemp0 = FMul(V3GetZ(abs1To0Col0), ea1);
//		const FloatV vtemp1 = FMul(V3GetY(abs1To0Col0), ea2);
//		ra = FAdd(vtemp0, vtemp1);
//
//		//A into B's space, ua0Xub0[0, a3, -a2]
//		const FloatV vtemp01 = FMul(V3GetZ(abs0To1Col0), eb1);
//		const FloatV vtemp02 = FMul(V3GetY(abs0To1Col0), eb2);
//		rb = FAdd(vtemp01, vtemp02);
//		
//		const FloatV radiusSum = FAdd(FAdd(ra, rb), contactDist);
//		if(FAllGrtr(absSign, radiusSum)) return false;
//	}
//
//	//ua0 X ub1
//	{
//		//B into A's space, ua0Xub0[0, -b3, b2]
//		const FloatV absSign = FAbs(FSub(FMul(V3GetY(col1), tz), FMul(V3GetZ(col1), ty)));
//
//		//B into A's space, ua0Xub0[0, -b3, b2]
//		const FloatV vtemp0 = FMul(V3GetZ(abs1To0Col1), ea1);
//		const FloatV vtemp1 = FMul(V3GetY(abs1To0Col1), ea2);
//		ra = FAdd(vtemp0, vtemp1);
//
//		//A into B's space, ua0Xub1[-a3, 0, a1]
//		const FloatV vtemp01 = FMul(V3GetZ(abs0To1Col0), eb0);
//		const FloatV vtemp02 = FMul(V3GetX(abs0To1Col0), eb2);
//		rb = FAdd(vtemp01, vtemp02);
//		
//		const FloatV radiusSum = FAdd(FAdd(ra, rb), contactDist);
//
//		if(FAllGrtr(absSign, radiusSum)) return false;
//
//	}
//
//	//ua0 X ub2
//	{
//		//B into A's space, ua0Xub0[0, -b3, b2]
//		const FloatV absSign = FAbs(FSub(FMul(V3GetY(col2), tz), FMul(V3GetZ(col2), ty)));
//
//
//		//B into A's space, ua0Xub0[0, -b3, b2]
//		const FloatV vtemp0 = FMul(V3GetZ(abs1To0Col2), ea1);
//		const FloatV vtemp1 = FMul(V3GetY(abs1To0Col2), ea2);
//		ra = FAdd(vtemp0, vtemp1);
//
//		//A into B's space, ua0Xub1[a2, -a1, 0]
//		const FloatV vtemp01 = FMul(V3GetY(abs0To1Col0), eb0);
//		const FloatV vtemp02 = FMul(V3GetX(abs0To1Col0), eb1);
//		rb = FAdd(vtemp01, vtemp02);
//		
//		const FloatV radiusSum = FAdd(FAdd(ra, rb), contactDist);
//		if(FAllGrtr(absSign, radiusSum)) return false;
//
//	}
//
//	//ua1 X ub0
//	{
//		//B into A's space, ua0Xub0[b3, 0, -b1]
//		const FloatV absSign = FAbs(FSub(FMul(V3GetZ(col0), tx), FMul(V3GetX(col0), tz)));
//
//		//B into A's space, ua0Xub0[b3, 0, -b1]
//		const FloatV vtemp0 = FMul(V3GetZ(abs1To0Col0), ea0);
//		const FloatV vtemp1 = FMul(V3GetX(abs1To0Col0), ea2);
//		ra = FAdd(vtemp0, vtemp1);
//
//		//A into B's space, ua0Xub1[0, a3, -a2]
//		const FloatV vtemp01 = FMul(V3GetZ(abs0To1Col1), eb1);
//		const FloatV vtemp02 = FMul(V3GetY(abs0To1Col1), eb2);
//		rb = FAdd(vtemp01, vtemp02);
//		
//		const FloatV radiusSum = FAdd(FAdd(ra, rb), contactDist);
//		if(FAllGrtr(absSign, radiusSum))return false;
//
//	}
//
//	//ua1 X ub1
//	{
//		//B into A's space, ua0Xub0[b3, 0, -b1]
//		const FloatV absSign = FAbs(FSub(FMul(V3GetZ(col1), tx), FMul(V3GetX(col1), tz)));
//
//		//B into A's space, ua0Xub0[b3, 0, -b1]
//		const FloatV vtemp0 = FMul(V3GetZ(abs1To0Col1), ea0);
//		const FloatV vtemp1 = FMul(V3GetX(abs1To0Col1), ea2);
//		ra = FAdd(vtemp0, vtemp1);
//
//		//A into B's space, ua0Xub1[-a3, 0, -a1]
//		const FloatV vtemp01 = FMul(V3GetZ(abs0To1Col1), eb0);
//		const FloatV vtemp02 = FMul(V3GetX(abs0To1Col1), eb2);
//		rb = FAdd(vtemp01, vtemp02);
//		
//		const FloatV radiusSum = FAdd(FAdd(ra, rb), contactDist);
//		if(FAllGrtr(absSign, radiusSum))return false;
//	}
//
//	//ua1 X ub2
//	{
//		//B into A's space, ua0Xub0[b3, 0, -b1]
//		const FloatV absSign=FAbs(FSub(FMul(V3GetZ(col2), tx), FMul(V3GetX(col2), tz)));
//
//		//B into A's space, ua0Xub0[b3, 0, -b1]
//		const FloatV vtemp0 = FMul(V3GetZ(abs1To0Col2), ea0);
//		const FloatV vtemp1 = FMul(V3GetX(abs1To0Col2), ea2);
//		ra = FAdd(vtemp0, vtemp1);
//
//		//A into B's space, ua0Xub1[a2, -a1, 0]
//		const FloatV vtemp01 = FMul(V3GetY(abs0To1Col1), eb0);
//		const FloatV vtemp02 = FMul(V3GetX(abs0To1Col1), eb1);
//		rb = FAdd(vtemp01, vtemp02);
//		
//		const FloatV radiusSum = FAdd(FAdd(ra, rb), contactDist);
//		if(FAllGrtr(absSign, radiusSum))return false;
//	}
//
//	//ua2 X ub0
//	{
//		//B into A's space, ua2Xub0[-b2, b1, 0]
//		const FloatV absSign = FAbs(FSub(FMul(V3GetX(col0), ty), FMul(V3GetY(col0), tx)));
//
//
//		//B into A's space, ua2Xub0[-b2, b1, 0]
//		const FloatV vtemp0 = FMul(V3GetY(abs1To0Col0), ea0);
//		const FloatV vtemp1 = FMul(V3GetX(abs1To0Col0), ea1);
//		ra = FAdd(vtemp0, vtemp1);
//
//		//A into B's space, ua0Xub1[0, a3, -a2]
//		const FloatV vtemp01 = FMul(V3GetZ(abs0To1Col2), eb1);
//		const FloatV vtemp02 = FMul(V3GetY(abs0To1Col2), eb2);
//		rb = FAdd(vtemp01, vtemp02);
//		
//		const FloatV radiusSum = FAdd(FAdd(ra, rb), contactDist);
//		if(FAllGrtr(absSign, radiusSum))return false;
//	}
//
//	//ua2 X ub1
//	{
//		//B into A's space, ua2Xub0[-b2, b1, 0]
//		const FloatV absSign = FAbs(FSub(FMul(V3GetX(col1), ty), FMul(V3GetY(col1), tx)));
//
//		//B into A's space, ua2Xub0[-b2, b1, 0]
//		const FloatV vtemp0 = FMul(V3GetY(abs1To0Col1), ea0);
//		const FloatV vtemp1 = FMul(V3GetX(abs1To0Col1), ea1);
//		ra = FAdd(vtemp0, vtemp1);
//
//		//A into B's space, ua0Xub1[-a3, 0, a1]
//		const FloatV vtemp01 = FMul(V3GetZ(abs0To1Col2), eb0);
//		const FloatV vtemp02 = FMul(V3GetX(abs0To1Col2), eb2);
//		rb = FAdd(vtemp01, vtemp02);
//		
//		const FloatV radiusSum = FAdd(FAdd(ra, rb), contactDist);
//		if(FAllGrtr(absSign, radiusSum))return false;
//	}
//
//	//ua2 X ub2
//	{
//		//B into A's space, ua2Xub0[-b2, b1, 0]
//		const FloatV absSign=FAbs(FSub(FMul(V3GetX(col2), ty), FMul(V3GetY(col2), tx)));
//
//		//B into A's space, ua2Xub0[-b2, b1, 0]
//		const FloatV vtemp0 = FMul(V3GetY(abs1To0Col2), ea0);
//		const FloatV vtemp1 = FMul(V3GetX(abs1To0Col2), ea1);
//		ra = FAdd(vtemp0, vtemp1);
//
//		//A into B's space, ua0Xub1[a2, -a1, 0]
//		const FloatV vtemp01 = FMul(V3GetY(abs0To1Col2), eb0);
//		const FloatV vtemp02 = FMul(V3GetX(abs0To1Col2), eb1);
//		rb = FAdd(vtemp01, vtemp02);
//		
//		const FloatV radiusSum = FAdd(FAdd(ra, rb), contactDist);
//		if(FAllGrtr(absSign, radiusSum))return false;
//	}
//
//	Vec3V mtd;
//
//
//	VecI32V vIndex0 = VecI32V_From_I32Array_Aligned(iInd0[0]);
//	VecI32V vIndex1 = VecI32V_From_I32Array_Aligned(iInd0[1]);
//
//	const BoolV bIsGrtr = V3IsGrtr(overlap0, overlap1);
//	const Vec3V vSmallest = V3Sel(bIsGrtr, overlap1, overlap0);
//	const VecI32V vIndex = VecI32V_Sel(bIsGrtr, vIndex1, vIndex0);
//	const BoolV xSmallest = BAnd(V3IsGrtr(V3GetY(vSmallest), V3GetX(vSmallest)), V3IsGrtr(V3GetZ(vSmallest), V3GetX(vSmallest)));
//	const BoolV ySmallest = V4IsGrtr(V3GetZ(vSmallest), V3GetY(vSmallest));
//
//	VecI32V smallestIndex = VecI32V_Sel(xSmallest, VecI32V_GetX(vIndex), VecI32V_Sel(ySmallest, VecI32V_GetY(vIndex), VecI32V_GetZ(vIndex)));
//
//	PxI32 feature;
//	PxI32_From_VecI32V(smallestIndex, &feature);
//	const BoolV bFalse = BFFFF();
//	const Vec3V xAxis = V3UnitX();
//	const Vec3V yAxis = V3UnitY();
//	const Vec3V zAxis = V3UnitZ();
//
//
//	switch(feature)
//	{
//	case 0: //ua0
//		{
//			const BoolV isgrtr = FIsGrtr(V3GetX(sign0), zero);
//			mtd = V3Sel(isgrtr, xAxis, V3Neg(xAxis));
//			
//			const Vec3V boxExtents = V3Merge(FAdd(ea0, contactDist), ea1, ea2);
//			const Vec3V extrudedExt = V3Merge(FMax(),ea1, ea2);
//			/*const Vec3V boxExtents = V3Add(box0.extents, contactDistV);
//			const Vec3V extrudedExt = V3Merge(FMax(),V3GetY(boxExtents), V3GetZ(boxExtents));*/
//			return calculateContacts(V3Sel(isgrtr, boxExtents, extrudedExt), V3Neg(V3Sel(isgrtr, extrudedExt, boxExtents)), transform0, box1.extents, transform1, mtd, 
//				ea0, manifold, bFalse, replaceBreakingThreshold, contactDist, BTFFF());
//		};
//	case 1: //ua1
//		{
//			const BoolV isgrtr = FIsGrtr(V3GetY(sign0), zero);
//			mtd = V3Sel(isgrtr, yAxis, V3Neg(yAxis));
//			
//			const Vec3V boxExtents = V3Merge(ea0, FAdd(ea1, contactDist),ea2);
//			const Vec3V extrudedExt = V3Merge(ea0, FMax(), ea2);
//		/*	const Vec3V boxExtents = V3Add(box0.extents, contactDistV);
//			const Vec3V extrudedExt = V3Merge(V3GetX(boxExtents), FMax(),V3GetZ(boxExtents));*/
//			//return calculateContacts(box0.extents, V3Merge(FNeg(ea0), FNeg(FMax()), FNeg(ea2)), transform0, box1.extents, transform1, mtd, V3GetY(box0.extents),manifold, false, replaceBreakingThreshold);
//			return calculateContacts(V3Sel(isgrtr, boxExtents, extrudedExt), V3Neg(V3Sel(isgrtr, extrudedExt, boxExtents)), transform0, box1.extents, transform1, mtd, 
//				ea1, manifold, bFalse, replaceBreakingThreshold, contactDist, BFTFF());
//		};
//	case 2: //ua2
//		{
//			const BoolV isgrtr = FIsGrtr(V3GetZ(sign0), zero);
//			mtd = V3Sel(isgrtr, zAxis, V3Neg(zAxis));
//
//			const Vec3V boxExtents = V3Merge(ea0, ea1, FAdd(ea2, contactDist));
//			const Vec3V extrudedExt = V3Merge(ea0, ea1, FMax());
//		/*	const Vec3V boxExtents = V3Add(box0.extents, contactDistV);
//			const Vec3V extrudedExt = V3Merge(V3GetX(boxExtents), V3GetY(boxExtents),FMax());*/
//			return calculateContacts(V3Sel(isgrtr, boxExtents, extrudedExt), V3Neg(V3Sel(isgrtr, extrudedExt, boxExtents)), transform0, box1.extents, transform1, mtd, 
//				ea2, manifold, bFalse, replaceBreakingThreshold, contactDist, BFFTF());
//		};
//	case 3: //ub0
//		{
//			const BoolV isgrtr = FIsGrtr(V3GetX(sign1), zero);
//			mtd = V3Sel(isgrtr, V3Neg(xAxis), xAxis);
//			
//			const Vec3V boxExtents = V3Merge(FAdd(eb0, contactDist), eb1, eb2);
//			const Vec3V extrudedExt = V3Merge(FMax(), eb1, eb2);
//		/*	const Vec3V boxExtents = V3Add(box1.extents, contactDistV);
//			const Vec3V extrudedExt = V3Merge(FMax(),V3GetY(boxExtents), V3GetZ(boxExtents));*/
//			return calculateContacts(V3Sel(isgrtr, extrudedExt, boxExtents), V3Neg(V3Sel(isgrtr,boxExtents, extrudedExt)), transform1, box0.extents, transform0, mtd, 
//				eb0, manifold, bTrue, replaceBreakingThreshold, contactDist, BTFFF());
//		};
//	case 4: //ub1
//		{
//			const BoolV isgrtr = FIsGrtr(V3GetY(sign1), zero);
//			mtd = V3Sel(isgrtr, V3Neg(yAxis), yAxis);
//		
//			const Vec3V boxExtents = V3Merge(eb0, FAdd(eb1, contactDist), eb2);
//			const Vec3V extrudedExt = V3Merge(eb0, FMax(), eb2);
//
//			/*const Vec3V boxExtents = V3Add(box1.extents, contactDistV);
//			const Vec3V extrudedExt = V3Merge(V3GetX(boxExtents), FMax(), V3GetZ(boxExtents));*/
//			return calculateContacts(V3Sel(isgrtr, extrudedExt, boxExtents), V3Neg(V3Sel(isgrtr, boxExtents, extrudedExt)), transform1, box0.extents, transform0, mtd, 
//				eb1,manifold, bTrue, replaceBreakingThreshold, contactDist, BFTFF());
//		};
//	case 5: //ub2;
//		{
//			const BoolV isgrtr = FIsGrtr(V3GetZ(sign1), zero);
//			mtd = V3Sel(isgrtr, V3Neg(zAxis), zAxis);
//			const Vec3V boxExtents = V3Merge(eb0, eb1, FAdd(eb2, contactDist));
//			const Vec3V extrudedExt = V3Merge(eb0, eb1, FMax());
//			/*const Vec3V boxExtents = V3Add(box1.extents, contactDistV);
//			const Vec3V extrudedExt = V3Merge(V3GetX(boxExtents), V3GetY(boxExtents), FMax());*/
//			return calculateContacts(V3Sel(isgrtr, extrudedExt, boxExtents), V3Neg(V3Sel(isgrtr, boxExtents, extrudedExt)), transform1, box0.extents, transform0, mtd, 
//				eb2, manifold, bTrue, replaceBreakingThreshold, contactDist, BFFTF());
//		};
//	default:
//		return 0;
//	}
//}

static PxU32 doBoxBoxGenerateContacts(const Gu::BoxV& box0, const Gu::BoxV& box1, const Ps::aos::PsMatTransformV& transform0, const Ps::aos::PsMatTransformV& transform1, const Ps::aos::FloatVArg replaceBreakingThreshold, const Ps::aos::FloatVArg contactDist, Gu::PersistentContactManifold& manifold, Gu::PersistentContact* manifoldContacts, PxU32& numContacts)
{
	using namespace Ps::aos;

	const FloatV ea0 = V3GetX(box0.extents);
	const FloatV ea1 = V3GetY(box0.extents);
	const FloatV ea2 = V3GetZ(box0.extents);

	const FloatV eb0 = V3GetX(box1.extents);
	const FloatV eb1 = V3GetY(box1.extents);
	const FloatV eb2 = V3GetZ(box1.extents);

	//const PsMatTransformV aToB = transform1.transformInv(transform0);
	const PsMatTransformV transform1To0 = transform0.transformInv(transform1);
	//const PsMatTransformV transform0To1 = transform1.transformInv(transform0);
	const Mat33V rot0To1 =M33Trnsps(transform1To0.rot);

	//Get the MTD
	const Vec3V xAxis = V3UnitX();
	const Vec3V yAxis = V3UnitY();
	const Vec3V zAxis = V3UnitZ();
//	const FloatV eps = FloatV_From_F32(0.999f);
	const Vec3V uEps = V3Load(1e-6f);

	const FloatV zero = FZero();

	const FloatV tx = V3GetX(transform1To0.p);
	const FloatV ty = V3GetY(transform1To0.p);
	const FloatV tz = V3GetZ(transform1To0.p);
	const Vec3V col0 = transform1To0.getCol0();
	const Vec3V col1 = transform1To0.getCol1();
	const Vec3V col2 = transform1To0.getCol2();

	const Vec3V abs1To0Col0 = V3Add(V3Abs(col0), uEps);
	const Vec3V abs1To0Col1 = V3Add(V3Abs(col1), uEps);
	const Vec3V abs1To0Col2 = V3Add(V3Abs(col2), uEps);

	const Vec3V abs0To1Col0 = V3Add(V3Abs(rot0To1.col0), uEps);
	const Vec3V abs0To1Col1 = V3Add(V3Abs(rot0To1.col1), uEps);
	const Vec3V abs0To1Col2 = V3Add(V3Abs(rot0To1.col2), uEps);

	
	FloatV sign[6];
	FloatV overlap[6];

	FloatV ra, rb, radiusSum;

	{
		
		sign[0] = tx;

		const Vec3V vtemp3 = V3Mul(abs0To1Col0, box1.extents);
		rb = FAdd(V3GetX(vtemp3), FAdd(V3GetY(vtemp3), V3GetZ(vtemp3)));

		radiusSum = FAdd(ea0, rb);
		overlap[0] =  FAdd(FSub(radiusSum, FAbs(sign[0])), contactDist);
		if(FAllGrtr(zero, overlap[0]))
			return false;
	}

	//ua1
	{
		sign[1] = ty;

		const Vec3V vtemp3 = V3Mul(abs0To1Col1, box1.extents);
		rb = FAdd(V3GetX(vtemp3), FAdd(V3GetY(vtemp3), V3GetZ(vtemp3)));

		radiusSum = FAdd(ea1, rb);
		overlap[1] =  FAdd(FSub(radiusSum, FAbs(sign[1])), contactDist);
		if(FAllGrtr(zero, overlap[1]))
			return false;

	}


	//ua2
	//t = V3Dot(normal, ua2);
	
	//if(FAllGrtr(t, eps))
	{
		sign[2] = tz;
		ra = ea2;

		const Vec3V vtemp3 = V3Mul(abs0To1Col2, box1.extents);
		rb = FAdd(V3GetX(vtemp3), FAdd(V3GetY(vtemp3), V3GetZ(vtemp3)));

		radiusSum = FAdd(ea2, rb);
		overlap[2] =  FAdd(FSub(radiusSum, FAbs(sign[2])), contactDist);
		if(FAllGrtr(zero, overlap[2]))
			return false;
		
	}
   
	//ub0 
	{
		sign[3] = V3Dot(transform1To0.p, col0);

		const Vec3V vtemp3 = V3Mul(abs1To0Col0, box0.extents);
		ra =  FAdd(V3GetX(vtemp3), FAdd(V3GetY(vtemp3), V3GetZ(vtemp3)));

		radiusSum = FAdd(ra, eb0);
		overlap[3] =  FAdd(FSub(radiusSum, FAbs(sign[3])), contactDist);
		if(FAllGrtr(zero, overlap[3]))
			return false;
	
	}

	//ub1
	//t = V3Dot(normal, ub1);
	//if(FAllGrtr(t, eps))
	{
		sign[4] = V3Dot(transform1To0.p, col1);

		const Vec3V vtemp3 = V3Mul(abs1To0Col1, box0.extents);
		ra =  FAdd(V3GetX(vtemp3), FAdd(V3GetY(vtemp3), V3GetZ(vtemp3)));

		radiusSum = FAdd(ra, eb1);
		overlap[4] =  FAdd(FSub(radiusSum, FAbs(sign[4])), contactDist);
		if(FAllGrtr(zero, overlap[4]))
			return false;
	}

	//ub2
	//if(FAllGrtr(t, eps))
	{
		sign[5] = V3Dot(transform1To0.p, col2);

		const Vec3V vtemp3 = V3Mul(abs1To0Col2, box0.extents);
		ra =  FAdd(V3GetX(vtemp3), FAdd(V3GetY(vtemp3), V3GetZ(vtemp3)));
		
		radiusSum = FAdd(ra, eb2);
		overlap[5] =  FAdd(FSub(radiusSum, FAbs(sign[5])), contactDist);
		if(FAllGrtr(zero, overlap[5]))
			return false;
	}


	//ua0 X ub0
	{
		//B into A's space, ua0Xub0[0,-b3, b2]
		const FloatV absSign = FAbs(FSub(FMul(V3GetY(col0), tz), FMul(V3GetZ(col0), ty)));

		//B into A's space, ua0Xub0[0,-b3, b2]
		const FloatV vtemp0 = FMul(V3GetZ(abs1To0Col0), ea1);
		const FloatV vtemp1 = FMul(V3GetY(abs1To0Col0), ea2);
		ra = FAdd(vtemp0, vtemp1);

		//A into B's space, ua0Xub0[0, a3, -a2]
		const FloatV vtemp01 = FMul(V3GetZ(abs0To1Col0), eb1);
		const FloatV vtemp02 = FMul(V3GetY(abs0To1Col0), eb2);
		rb = FAdd(vtemp01, vtemp02);
		
		radiusSum = FAdd(FAdd(ra, rb), contactDist);
		if(FAllGrtr(absSign, radiusSum)) return false;
	}

	//ua0 X ub1
	{
		//B into A's space, ua0Xub0[0, -b3, b2]
		const FloatV absSign = FAbs(FSub(FMul(V3GetY(col1), tz), FMul(V3GetZ(col1), ty)));

		//B into A's space, ua0Xub0[0, -b3, b2]
		const FloatV vtemp0 = FMul(V3GetZ(abs1To0Col1), ea1);
		const FloatV vtemp1 = FMul(V3GetY(abs1To0Col1), ea2);
		ra = FAdd(vtemp0, vtemp1);

		//A into B's space, ua0Xub1[-a3, 0, a1]
		const FloatV vtemp01 = FMul(V3GetZ(abs0To1Col0), eb0);
		const FloatV vtemp02 = FMul(V3GetX(abs0To1Col0), eb2);
		rb = FAdd(vtemp01, vtemp02);
		
		radiusSum = FAdd(FAdd(ra, rb), contactDist);

		if(FAllGrtr(absSign, radiusSum)) return false;

	}

	//ua0 X ub2
	{
		//B into A's space, ua0Xub0[0, -b3, b2]
		const FloatV absSign = FAbs(FSub(FMul(V3GetY(col2), tz), FMul(V3GetZ(col2), ty)));


		//B into A's space, ua0Xub0[0, -b3, b2]
		const FloatV vtemp0 = FMul(V3GetZ(abs1To0Col2), ea1);
		const FloatV vtemp1 = FMul(V3GetY(abs1To0Col2), ea2);
		ra = FAdd(vtemp0, vtemp1);

		//A into B's space, ua0Xub1[a2, -a1, 0]
		const FloatV vtemp01 = FMul(V3GetY(abs0To1Col0), eb0);
		const FloatV vtemp02 = FMul(V3GetX(abs0To1Col0), eb1);
		rb = FAdd(vtemp01, vtemp02);
		
		radiusSum = FAdd(FAdd(ra, rb), contactDist);
		if(FAllGrtr(absSign, radiusSum)) return false;

	}

	//ua1 X ub0
	{
		//B into A's space, ua0Xub0[b3, 0, -b1]
		const FloatV absSign = FAbs(FSub(FMul(V3GetZ(col0), tx), FMul(V3GetX(col0), tz)));

		//B into A's space, ua0Xub0[b3, 0, -b1]
		const FloatV vtemp0 = FMul(V3GetZ(abs1To0Col0), ea0);
		const FloatV vtemp1 = FMul(V3GetX(abs1To0Col0), ea2);
		ra = FAdd(vtemp0, vtemp1);

		//A into B's space, ua0Xub1[0, a3, -a2]
		const FloatV vtemp01 = FMul(V3GetZ(abs0To1Col1), eb1);
		const FloatV vtemp02 = FMul(V3GetY(abs0To1Col1), eb2);
		rb = FAdd(vtemp01, vtemp02);
		
		radiusSum = FAdd(FAdd(ra, rb), contactDist);
		if(FAllGrtr(absSign, radiusSum))return false;

	}

	//ua1 X ub1
	{
		//B into A's space, ua0Xub0[b3, 0, -b1]
		const FloatV absSign = FAbs(FSub(FMul(V3GetZ(col1), tx), FMul(V3GetX(col1), tz)));

		//B into A's space, ua0Xub0[b3, 0, -b1]
		const FloatV vtemp0 = FMul(V3GetZ(abs1To0Col1), ea0);
		const FloatV vtemp1 = FMul(V3GetX(abs1To0Col1), ea2);
		ra = FAdd(vtemp0, vtemp1);

		//A into B's space, ua0Xub1[-a3, 0, -a1]
		const FloatV vtemp01 = FMul(V3GetZ(abs0To1Col1), eb0);
		const FloatV vtemp02 = FMul(V3GetX(abs0To1Col1), eb2);
		rb = FAdd(vtemp01, vtemp02);
		
		radiusSum = FAdd(FAdd(ra, rb), contactDist);
		if(FAllGrtr(absSign, radiusSum))return false;
	}

	//ua1 X ub2
	{
		//B into A's space, ua0Xub0[b3, 0, -b1]
		const FloatV absSign=FAbs(FSub(FMul(V3GetZ(col2), tx), FMul(V3GetX(col2), tz)));

		//B into A's space, ua0Xub0[b3, 0, -b1]
		const FloatV vtemp0 = FMul(V3GetZ(abs1To0Col2), ea0);
		const FloatV vtemp1 = FMul(V3GetX(abs1To0Col2), ea2);
		ra = FAdd(vtemp0, vtemp1);

		//A into B's space, ua0Xub1[a2, -a1, 0]
		const FloatV vtemp01 = FMul(V3GetY(abs0To1Col1), eb0);
		const FloatV vtemp02 = FMul(V3GetX(abs0To1Col1), eb1);
		rb = FAdd(vtemp01, vtemp02);
		
		radiusSum = FAdd(FAdd(ra, rb), contactDist);
		if(FAllGrtr(absSign, radiusSum))return false;
	}

	//ua2 X ub0
	{
		//B into A's space, ua2Xub0[-b2, b1, 0]
		const FloatV absSign = FAbs(FSub(FMul(V3GetX(col0), ty), FMul(V3GetY(col0), tx)));


		//B into A's space, ua2Xub0[-b2, b1, 0]
		const FloatV vtemp0 = FMul(V3GetY(abs1To0Col0), ea0);
		const FloatV vtemp1 = FMul(V3GetX(abs1To0Col0), ea1);
		ra = FAdd(vtemp0, vtemp1);

		//A into B's space, ua0Xub1[0, a3, -a2]
		const FloatV vtemp01 = FMul(V3GetZ(abs0To1Col2), eb1);
		const FloatV vtemp02 = FMul(V3GetY(abs0To1Col2), eb2);
		rb = FAdd(vtemp01, vtemp02);
		
		radiusSum = FAdd(FAdd(ra, rb), contactDist);
		if(FAllGrtr(absSign, radiusSum))return false;
	}

	//ua2 X ub1
	{
		//B into A's space, ua2Xub0[-b2, b1, 0]
		const FloatV absSign = FAbs(FSub(FMul(V3GetX(col1), ty), FMul(V3GetY(col1), tx)));

		//B into A's space, ua2Xub0[-b2, b1, 0]
		const FloatV vtemp0 = FMul(V3GetY(abs1To0Col1), ea0);
		const FloatV vtemp1 = FMul(V3GetX(abs1To0Col1), ea1);
		ra = FAdd(vtemp0, vtemp1);

		//A into B's space, ua0Xub1[-a3, 0, a1]
		const FloatV vtemp01 = FMul(V3GetZ(abs0To1Col2), eb0);
		const FloatV vtemp02 = FMul(V3GetX(abs0To1Col2), eb2);
		rb = FAdd(vtemp01, vtemp02);
		
		radiusSum = FAdd(FAdd(ra, rb), contactDist);
		if(FAllGrtr(absSign, radiusSum))return false;
	}

	//ua2 X ub2
	{
		//B into A's space, ua2Xub0[-b2, b1, 0]
		const FloatV absSign=FAbs(FSub(FMul(V3GetX(col2), ty), FMul(V3GetY(col2), tx)));

		//B into A's space, ua2Xub0[-b2, b1, 0]
		const FloatV vtemp0 = FMul(V3GetY(abs1To0Col2), ea0);
		const FloatV vtemp1 = FMul(V3GetX(abs1To0Col2), ea1);
		ra = FAdd(vtemp0, vtemp1);

		//A into B's space, ua0Xub1[a2, -a1, 0]
		const FloatV vtemp01 = FMul(V3GetY(abs0To1Col2), eb0);
		const FloatV vtemp02 = FMul(V3GetX(abs0To1Col2), eb1);
		rb = FAdd(vtemp01, vtemp02);
		
		radiusSum = FAdd(FAdd(ra, rb), contactDist);
		if(FAllGrtr(absSign, radiusSum))return false;
	}

	Vec3V mtd;

	PxU32 feature = 0;
	FloatV minOverlap = overlap[0];

	for(PxU32 i=1; i<6; ++i)
	{
		if(FAllGrtr(minOverlap, overlap[i]))
		{
			minOverlap = overlap[i];
			feature = i;
		}
	}

	//const BoolV bTrue = BTTTT();
	//const BoolV bFalse = BFFFF();


	switch(feature)
	{
	case 0: //ua0
		{
			const BoolV isgrtr = FIsGrtr(sign[0], zero);
			mtd = V3Sel(isgrtr, xAxis, V3Neg(xAxis));

			const Vec3V boxExtents = V3Merge(FAdd(ea0, contactDist), ea1, ea2);
			const Vec3V extrudedExt = V3Merge(FMax(),ea1, ea2);
			const Vec3V sabExtents = V3Sel(isgrtr, boxExtents, extrudedExt);
			const Vec3V nsabExtents = V3Neg(V3Sel(isgrtr, extrudedExt, boxExtents));
			/*const Vec3V boxExtents = V3Add(box0.extents, contactDist);
			const Vec3V extrudedExt = V3Merge(FMax(),V3GetY(boxExtents), V3GetZ(boxExtents));
			*/
			calculateContacts<true>(sabExtents, nsabExtents, box0.extents, transform0, box1.extents, transform1, mtd, 
				ea0, manifold, manifoldContacts, numContacts, replaceBreakingThreshold, contactDist, 0);

			break;
		};
	case 1: //ua1
		{
			const BoolV isgrtr = FIsGrtr(sign[1], zero);
			mtd = V3Sel(isgrtr, yAxis, V3Neg(yAxis));
			
			const Vec3V boxExtents = V3Merge(ea0, FAdd(ea1, contactDist),ea2);
			const Vec3V extrudedExt = V3Merge(ea0, FMax(), ea2);
			const Vec3V sabExtents = V3Sel(isgrtr, boxExtents, extrudedExt);
			const Vec3V nsabExtents= V3Neg(V3Sel(isgrtr, extrudedExt, boxExtents));


			/*const Vec3V boxExtents = V3Add(box0.extents, contactDist);
			const Vec3V extrudedExt = V3Merge(V3GetX(boxExtents), FMax(),V3GetZ(boxExtents));
	*/
			calculateContacts<true>(sabExtents, nsabExtents, box0.extents, transform0, box1.extents, transform1, mtd, 
				ea1, manifold, manifoldContacts, numContacts, replaceBreakingThreshold, contactDist, 1);
			break;
		};
	case 2: //ua2
		{
			const BoolV isgrtr = FIsGrtr(sign[2], zero);
			mtd = V3Sel(isgrtr, zAxis, V3Neg(zAxis));

			const Vec3V boxExtents = V3Merge(ea0, ea1, FAdd(ea2, contactDist));
			const Vec3V extrudedExt = V3Merge(ea0, ea1, FMax());
			const Vec3V sabExtents = V3Sel(isgrtr, boxExtents, extrudedExt);
			const Vec3V nsabExtents= V3Neg(V3Sel(isgrtr, extrudedExt, boxExtents));
			/*const Vec3V boxExtents = V3Add(box0.extents, contactDist);
			const Vec3V extrudedExt = V3Merge(V3GetX(boxExtents), V3GetY(boxExtents),FMax());*/
			
			calculateContacts<true>(sabExtents, nsabExtents, box0.extents, transform0, box1.extents, transform1, mtd, 
				ea2, manifold, manifoldContacts, numContacts, replaceBreakingThreshold, contactDist, 2);
			break;
		};
	case 3: //ub0
		{
			const BoolV isgrtr = FIsGrtr(sign[3], zero);
			mtd = V3Sel(isgrtr, V3Neg(xAxis), xAxis);
			
			const Vec3V boxExtents = V3Merge(FAdd(eb0, contactDist), eb1, eb2);
			const Vec3V extrudedExt = V3Merge(FMax(), eb1, eb2);
			const Vec3V sabExtents = V3Sel(isgrtr, extrudedExt, boxExtents);
			const Vec3V nsabExtents= V3Neg(V3Sel(isgrtr,boxExtents, extrudedExt));
		/*	const Vec3V boxExtents = V3Add(box1.extents, contactDist);
			const Vec3V extrudedExt = V3Merge(FMax(),V3GetY(boxExtents), V3GetZ(boxExtents));*/

			calculateContacts<false>(sabExtents, nsabExtents, box1.extents, transform1, box0.extents, transform0, mtd, 
				eb0, manifold, manifoldContacts, numContacts, replaceBreakingThreshold, contactDist, 0);
			break;
		};
	case 4: //ub1
		{
			const BoolV isgrtr = FIsGrtr(sign[4], zero);
			mtd = V3Sel(isgrtr, V3Neg(yAxis), yAxis);
		
			const Vec3V boxExtents = V3Merge(eb0, FAdd(eb1, contactDist), eb2);
			const Vec3V extrudedExt = V3Merge(eb0, FMax(), eb2);
			const Vec3V sabExtents = V3Sel(isgrtr, extrudedExt, boxExtents);
			const Vec3V nsabExtents= V3Neg(V3Sel(isgrtr, boxExtents, extrudedExt));



			/*const Vec3V boxExtents = V3Add(box1.extents, contactDist);
			const Vec3V extrudedExt = V3Merge(V3GetX(boxExtents), FMax(), V3GetZ(boxExtents));
		*/
			calculateContacts<false>(sabExtents, nsabExtents,  box1.extents, transform1, box0.extents, transform0, mtd, 
				eb1,manifold, manifoldContacts, numContacts, replaceBreakingThreshold, contactDist, 1);
			break;
		};
	case 5: //ub2;
		{
			const BoolV isgrtr = FIsGrtr(sign[5], zero);
			mtd = V3Sel(isgrtr, V3Neg(zAxis), zAxis);
			const Vec3V boxExtents = V3Merge(eb0, eb1, FAdd(eb2, contactDist));
			const Vec3V extrudedExt = V3Merge(eb0, eb1, FMax());
			const Vec3V sabExtents = V3Sel(isgrtr, extrudedExt, boxExtents);
			const Vec3V nsabExtents= V3Neg(V3Sel(isgrtr,boxExtents, extrudedExt));

		/*	const Vec3V boxExtents = V3Add(box1.extents, contactDist);
			const Vec3V extrudedExt = V3Merge(V3GetX(boxExtents), V3GetY(boxExtents), FMax());*/
			
			calculateContacts<false>(sabExtents, nsabExtents,  box1.extents, transform1, box0.extents, transform0, mtd, 
				eb2, manifold, manifoldContacts, numContacts,replaceBreakingThreshold, contactDist, 2);
			break;
		};
	default:
		return false;
	}

	return true;
}  

bool pcmContactBoxBox(GU_CONTACT_METHOD_ARGS)
{

	using namespace Ps::aos;
	// Get actual shape data
	Gu::PersistentContactManifold& manifold = cache.getManifold();
	/*Ps::prefetchLine(&manifold);
	Ps::prefetchLine(&manifold, 128);*/  
	Ps::prefetchLine(&manifold, 256);
	const PxBoxGeometry& shapeBox0 = shape0.get<const PxBoxGeometry>();
	const PxBoxGeometry& shapeBox1 = shape1.get<const PxBoxGeometry>();

	PX_ASSERT(transform1.q.isSane());
	PX_ASSERT(transform0.q.isSane());
	const Vec3V zeroV = V3Zero();

	const QuatV q0 = QuatVLoadA(&transform0.q.x);
	const Vec3V p0 = V3LoadA(&transform0.p.x);

	const QuatV q1 = QuatVLoadA(&transform1.q.x);
	const Vec3V p1 = V3LoadA(&transform1.p.x);

	const FloatV contactDist = FLoad(contactDistance);
	const Vec3V boxExtents0 = V3LoadU(shapeBox0.halfExtents);
	const Vec3V boxExtents1 = V3LoadU(shapeBox1.halfExtents);
	
	//Transfer A into the local space of B
	const PsTransformV transf0(p0, q0);
	const PsTransformV transf1(p1, q1);
	const PsTransformV curRTrans(transf1.transformInv(transf0));
	//const PsTransformV curRTrans(transform1.transformInv(transform0));
	const PsMatTransformV aToB(curRTrans);

	const FloatV boxMargin0 = Gu::CalculatePCMBoxMargin(boxExtents0);
	const FloatV boxMargin1 = Gu::CalculatePCMBoxMargin(boxExtents1);
	const FloatV minMargin = FMin(boxMargin0, boxMargin1);

	const PxU32 initialContacts = manifold.mNumContacts;
	//const FloatV distanceBreakingThreshold = zero;//FMul(minMargin, FloatV_From_F32(0.25f));
	const FloatV projectBreakingThreshold = FMul(minMargin, FLoad(0.8f));
	const FloatV replaceBreakingThreshold = FMul(minMargin, FLoad(0.05f));
	
	//manifold.refreshContactPoints(curRTrans, projectBreakingThreshold, contactDist);
	manifold.refreshContactPoints(aToB, projectBreakingThreshold, contactDist);

	const PxU32 newContacts = manifold.mNumContacts;

	const bool bLostContacts = (newContacts != initialContacts);
	
	PxGJKStatus status = manifold.mNumContacts > 0 ? GJK_UNDEFINED : GJK_NON_INTERSECT;

	Vec3V closestA(zeroV), closestB(zeroV), normal(zeroV); // these will be in the local space of B
	FloatV penDep = FZero(); 
	//manifold.numContacts = 0;
  
	if(bLostContacts || manifold.invalidate(curRTrans, minMargin))	
	{
		
		Gu::ShrunkBoxV box0(zeroV, boxExtents0);

		Gu::ShrunkBoxV box1(zeroV, boxExtents1);
		
		manifold.setRelativeTransform(curRTrans);

		//manifold.numContacts = 0.f;
		const PsMatTransformV transfV0(transf0);
		const PsMatTransformV transfV1(transf1);

		Gu::PersistentContact* manifoldContacts = PX_CP_TO_PCP(contactBuffer.contacts);
		PxU32 numContacts = 0;
	
		if(doBoxBoxGenerateContacts((Gu::BoxV&)box0, (Gu::BoxV&)box1, transfV0, transfV1, replaceBreakingThreshold, contactDist, manifold, manifoldContacts, numContacts)) 
		{
			//overlap
			if(numContacts == 0)
			{
				manifold.mNumWarmStartPoints = 0;
				status = Gu::GJKRelativePenetration(box0, box1, aToB, contactDist, closestA, closestB, normal, penDep,
				manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints);

				if(status == EPA_CONTACT)
				{
					status= Gu::EPARelativePenetration((Gu::BoxV&)box0, (Gu::BoxV&)box1, aToB,  closestA, closestB, normal, penDep,
						manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints);
					if(status == EPA_CONTACT)
					{
						const Vec3V v1 = V3Sub(closestB, closestA);  
						const FloatV dist = V3Length(v1);
						normal = V3ScaleInv(v1, dist);
						penDep = FNeg(dist);
						status = GJK_CONTACT;
					}
				}  

			}
			else  
			{
				//remove duplidate contacts
				manifold.removeDuplidateManifoldContacts(manifoldContacts, numContacts, replaceBreakingThreshold);

				//reduce contacts
				manifold.addBatchManifoldContacts(manifoldContacts, numContacts);

				const Vec3V normal =  manifold.getWorldNormal(transf1);
				//add the manifold contacts;
				manifold.addManifoldContactsToContactBuffer(contactBuffer, normal, transf1);
				
				//manifold.drawManifold(gThreadContext, transf0, transf1, true);
			
				return manifold.mNumContacts > 0;
			}
		}
	}
	

	if(status)
	{   

		//const PsTransformV transf1(p1, q1);
		PxU32 numContacts = manifold.getNumContacts();
		if(status == GJK_CONTACT)
		{
			const Vec3V localPointA = aToB.transformInv(closestA);//curRTrans.transformInv(closestA);
			const Vec3V localPointB = closestB;
			const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(normal), penDep);
			numContacts += manifold.addManifoldPoint(localPointA, localPointB, localNormalPen, replaceBreakingThreshold);

			//transform the normal back to world space
			normal = transf1.rotate(normal);

		}
		else
		{
			normal =  manifold.getWorldNormal(transf1);
	
		}

		
		
	//	manifold.drawManifold(gThreadContext, transf0, transf1, true);
		
		PX_ASSERT(numContacts <= 4 );

		manifold.addManifoldContactsToContactBuffer(contactBuffer, normal, transf1);

		return numContacts > 0;
	}
	
	return false;
}

//bool pcmContactBoxBox(GU_CONTACT_METHOD_ARGS)
//{
//
//	using namespace Ps::aos;
//	// Get actual shape data
//	PersistentContactManifold& manifold = cache.getManifold();
//	/*Ps::prefetchLine(&manifold);
//	Ps::prefetchLine(&manifold, 128);*/  
//	Ps::prefetchLine(&manifold, 256);
//	const PxBoxGeometry& shapeBox0 = shape0.get<const PxBoxGeometry>();
//	const PxBoxGeometry& shapeBox1 = shape1.get<const PxBoxGeometry>();
//
//	PX_ASSERT(transform1.q.isSane());
//	PX_ASSERT(transform0.q.isSane());
//	const Vec3V zeroV = V3Zero();
//
//	const QuatV q0 = QuatV_From_F32Array(&transform0.q.x);
//	const Vec3V p0 = Vec3V_From_Vec4V(Vec4V_From_F32Array(&transform0.p.x));
//
//	const QuatV q1 = QuatV_From_F32Array(&transform1.q.x);
//	const Vec3V p1 = Vec3V_From_Vec4V(Vec4V_From_F32Array(&transform1.p.x));
//	const FloatV contactDist = FloatV_From_F32(contactDistance);
//	const Vec3V boxExtents0 = Vec3V_From_PxVec3(shapeBox0.halfExtents);
//	const Vec3V boxExtents1 = Vec3V_From_PxVec3(shapeBox1.halfExtents);
//	
//	//Transfer A into the local space of B
//	const PsTransformV transf0(p0, q0);
//	const PsTransformV transf1(p1, q1);
//	const PsTransformV curRTrans(transf1.transformInv(transf0));
//	//const PsTransformV curRTrans(transform1.transformInv(transform0));
//	const PsMatTransformV aToB(curRTrans);
//
//	const FloatV boxMargin0 = Gu::CalculatePCMBoxMargin(boxExtents0);
//	const FloatV boxMargin1 = Gu::CalculatePCMBoxMargin(boxExtents1);
//	const FloatV minMargin = FMin(boxMargin0, boxMargin1);
//
//	const PxU32 initialContacts = manifold.numContacts;
//	//const FloatV distanceBreakingThreshold = zero;//FMul(minMargin, FloatV_From_F32(0.25f));
//	const FloatV projectBreakingThreshold = FMul(minMargin, FloatV_From_F32(0.8f));
//	const FloatV replaceBreakingThreshold = FMul(minMargin, FloatV_From_F32(0.05f));
//	
//	//manifold.refreshContactPoints(curRTrans, projectBreakingThreshold, contactDist);
//	manifold.refreshContactPoints(aToB, projectBreakingThreshold, contactDist);
//
//	const PxU32 newContacts = manifold.numContacts;
//
//	const bool bLostContacts = (newContacts != initialContacts);
//	
//	PxGJKStatus status = manifold.numContacts > 0 ? GJK_UNDEFINED : GJK_NON_INTERSECT;
//
//	Vec3V closestA, closestB(zeroV), normal(zeroV); // these will be in the local space of B
//	FloatV penDep = FZero(); 
//	//manifold.numContacts = 0;
//  
//	//ML::we change the invalidate threshold, the recycle code won't work with the incremental threshold anymore
//	if(1)//bLostContacts || manifold.invalidate(curRTrans, minMargin))	
//	{
//		
//		Gu::ShrinkedBoxV box0(zeroV, boxExtents0);
//		Gu::ShrinkedBoxV box1(zeroV, boxExtents1);
//	
//		status = Gu::GJKRelativePenetration(box0, box1, aToB, contactDist, closestA, closestB, normal, penDep, 
//			manifold.aIndice, manifold.bIndice, manifold.numWarmStartPoints);
//
//		if(status == EPA_CONTACT)
//		{
//			status= Gu::EPARelativePenetration((Gu::BoxV&)box0, (Gu::BoxV&)box1, aToB,  closestA, closestB, normal, penDep,
//				manifold.aIndice, manifold.bIndice, manifold.numWarmStartPoints);
//
//			if(status == EPA_CONTACT)
//			{
//				const Vec3V v1 = V3Sub(closestB, closestA);  
//				const FloatV dist = V3Length(v1);
//				normal = V3Scale(v1, FRecip(dist));
//				penDep = FNeg(dist);
//				status = GJK_CONTACT;
//			}
//
//		}
//		
//		//manifold.SetRelativeTransform(curRTrans);
//		manifold.setRelativeTransform(curRTrans);
//
//		if(status == GJK_DEGENERATE || status == EPA_DEGENERATE || status == EPA_FAIL)
//		{
//			//manifold.numContacts = 0.f;
//			const PsMatTransformV transfV0(transf0);
//			const PsMatTransformV transfV1(transf1);
//
//			Gu::PersistentContact* manifoldContacts = PX_CP_TO_PCP(contactBuffer.contacts);
//			PxU32 numContacts = 0;
//		
//			if(doBoxBoxGenerateContacts((Gu::BoxV&)box0, (Gu::BoxV&)box1, transfV0, transfV1, replaceBreakingThreshold, contactDist, manifold, manifoldContacts, numContacts)) 
//			{
//				//overlap
//				if(manifold.numContacts == 0)
//				{
//					manifold.numWarmStartPoints = 0;
//					status = Gu::GJKRelativePenetration(box0, box1, aToB, contactDist, closestA, closestB, normal, penDep,
//					manifold.aIndice, manifold.bIndice, manifold.numWarmStartPoints);
//
//					if(status == EPA_CONTACT)
//					{
//						status= Gu::EPARelativePenetration((Gu::BoxV)box0, (Gu::BoxV)box1, aToB,  closestA, closestB, normal, penDep,
//							manifold.aIndice, manifold.bIndice, manifold.numWarmStartPoints);
//						if(status == EPA_CONTACT)
//						{
//							const Vec3V v1 = V3Sub(closestB, closestA);  
//							const FloatV dist = V3Length(v1);
//							normal = V3Scale(v1, FRecip(dist));
//							penDep = FNeg(dist);
//							status = GJK_CONTACT;
//						}
//					}
//
//				}
//				else
//				{
//					normal =  manifold.getWorldNormal(transf1);
//					//add the manifold contacts;
//					manifold.addManifoldContactsToContactBuffer(contactBuffer, normal, transf1);
//					//manifold.drawManifold(gThreadContext, transf0, transf1, true);
//				
//					return manifold.numContacts > 0;
//				}
//			}
//		}
//	}  
//
//	if(status)
//	{   
//
//		//const PsTransformV transf1(p1, q1);
//		PxU32 numContacts = manifold.getNumContacts();
//		if(status == GJK_CONTACT)
//		{
//			const Vec3V localPointA = aToB.transformInv(closestA);//curRTrans.transformInv(closestA);
//			const Vec3V localPointB = closestB;
//			const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(normal), penDep);
//			numContacts += manifold.addManifoldPoint(localPointA, localPointB, localNormalPen, replaceBreakingThreshold);
//
//			//transform the normal back to world space
//			normal = transf1.rotate(normal);
//
//		}
//		else
//		{
//			normal =  manifold.getWorldNormal(transf1);
//	
//		}
//		
//	//	manifold.drawManifold(gThreadContext, transf0, transf1, true);
//		
//		PX_ASSERT(numContacts <= 4 );
//
//		manifold.addManifoldContactsToContactBuffer(contactBuffer, normal, transf1);
//
//		return numContacts > 0;
//	}
//	
//	return false;
//}


}
}
