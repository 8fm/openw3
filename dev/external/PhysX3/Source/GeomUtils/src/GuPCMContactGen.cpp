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

#include "GuGeometryUnion.h"
#include "GuPCMContactGen.h"
#include "GuPCMShapeConvex.h"
#include "CmRenderOutput.h"
#include "GuPCMContactGenUtil.h"
#include "PsVecMath.h"
#include "GuVecCapsule.h"
#include "GuVecBox.h"

#ifdef	PCM_LOW_LEVEL_DEBUG
#include "CmRenderOutput.h"
physx::Cm::RenderOutput* gRenderOutPut = NULL;
#endif


using namespace physx;
using namespace Gu;


	//Precompute the convex data
	//     7+------+6			0 = ---
	//     /|     /|			1 = +--
	//    / |    / |			2 = ++-
	//   / 4+---/--+5			3 = -+-
	// 3+------+2 /    y   z	4 = --+
	//  | /    | /     |  /		5 = +-+
	//  |/     |/      |/		6 = +++
	// 0+------+1      *---x	7 = -++

namespace physx
{

namespace Gu
{

	bool intersectRayAABBLocal2(const Ps::aos::Vec3VArg minimum, const Ps::aos::Vec3VArg maximum, 
							const Ps::aos::Vec3VArg ro, const Ps::aos::Vec3VArg rd, 
							Ps::aos::FloatV& tnear, Ps::aos::FloatV& tfar)
	{
		using namespace Ps::aos;
		const FloatV eps = FLoad(1e-5f);
		const Vec3V absD = V3Abs(rd);
		const BoolV bTrue = BTTTT();
		
		const BoolV bParallel = V3IsGrtrOrEq(Vec3V_From_FloatV_WUndefined(eps), absD);
		const BoolV bOutOfRange = BOr(V3IsGrtr(minimum, ro), V3IsGrtr(ro, maximum));
		const BoolV bParallelAndOutOfRange(BAnd(bParallel, bOutOfRange));
		const BoolV bMiss(BAnyTrue3(bParallelAndOutOfRange));
		if(BAllEq(bMiss, bTrue))
			return false;

		const Vec3V oneOverDir = V3Recip(rd);
		
		const Vec3V _tSlab0 = V3Mul(V3Sub(minimum, ro), oneOverDir);
		const Vec3V _tSlab1 = V3Mul(V3Sub(maximum, ro), oneOverDir);

	

		const Vec3V tSlab0 = V3Min(_tSlab0, _tSlab1);
		const Vec3V tSlab1 = V3Max(_tSlab0, _tSlab1);

		const FloatV x = V3GetX(tSlab0);
		const FloatV y = V3GetY(tSlab0);
		const BoolV con0 = BAllTrue3(V3IsGrtrOrEq(Vec3V_From_FloatV_WUndefined(x), tSlab0));
		const BoolV con1 = BAllTrue3(V3IsGrtrOrEq(Vec3V_From_FloatV_WUndefined(y), tSlab0));
		const FloatV tVal = FSel(con0, x, FSel(con1, y, V3GetZ(tSlab0)));
		const FloatV tVal2 = FMin(V3GetX(tSlab1), FMin(V3GetY(tSlab1), V3GetZ(tSlab1)));
		const BoolV bHitSlab(BAnd(FIsGrtrOrEq(tVal2, tVal), FIsGrtrOrEq(tVal2, eps)));
		
		tnear = tVal;
		tfar = tVal2;

		return BAllEq(bHitSlab, bTrue) != 0;
		
	}

	//a and d need to be in the local space of polyData
	static bool intersectSegmentPolyhedron(const Ps::aos::Vec3VArg a, const Ps::aos::Vec3VArg dir, const PolygonalData& polyData, Ps::aos::FloatV& tEnter, Ps::aos::FloatV& tExit)
	{
		using namespace Ps::aos;
		const FloatV zero = FZero();
		const FloatV one = FOne();
		const FloatV eps = FLoad(1e-7f);
		FloatV tFirst = zero;
		FloatV tLast= one;

		for(PxU32 k=0; k<polyData.mNbPolygons; ++k)
		{
			const Gu::HullPolygonData& data = polyData.mPolygons[k];
			const Vec3V n =V3LoadU(data.mPlane.n);

			FloatV d = FLoad(data.mPlane.d);
	
			const FloatV denominator = V3Dot(n, dir);
			const FloatV distToPlane = FAdd(V3Dot(n, a), d);
			//const FloatV numerator = FSub(FNeg(d), V3Dot(n, a));
	
			//if(FAllEq(denominator, zero))
			if(FAllGrtr(eps, FAbs(denominator)))
			{
				/*if(FAllGrtr(numerator, zero))
					return false;*/

				if(FAllGrtr(distToPlane, zero))
					return false;
			}
			else
			{
				FloatV tTemp = FNeg(FDiv(distToPlane, denominator));

				//ML: denominator < 0 means the ray is entering halfspace; denominator > 0 means the ray is exiting halfspace
				const BoolV con = FIsGrtr(zero, denominator);
				const BoolV con0= FIsGrtr(tTemp, tFirst); 
				const BoolV con1 = FIsGrtr(tLast, tTemp);
	
				tFirst = FSel(BAnd(con, con0), tTemp, tFirst);
				tLast = FSel(BAnd(BNot(con), con1), tTemp, tLast);
			}
			
			if(FAllGrtr(tFirst, tLast))
				return false;
		}

		//calculate the intersect p in the local space
		tEnter = tFirst;
		tExit = tLast;
	
		return true;

	}


  
	//static bool testEdgeNormalBruteForce(const Gu::PolygonalData& polyData0, const Gu::PolygonalData& polyData1, SupportLocal* map0, SupportLocal* map1, const PsMatTransformV& transform0To1, const PsMatTransformV& transform1To0, const Ps::aos::FloatVArg contactDist, Ps::aos::FloatV& minOverlap, Ps::aos::Vec3V& edgeNormalIn0)
	//{
	//	using namespace Ps::aos;
	//	FloatV overlap = FMax();
	//	FloatV min0, max0;
	//	FloatV min1, max1;
	//	const BoolV bTrue = BTTTT();

	//	//in the local space of polyData0
	//	for(PxU32 i=0; i<polyData0.mNbPolygons; ++i)
	//	{
	//		const Gu::HullPolygonData& polygon0 = polyData0.mPolygons[i];
	//		const PxU8* inds0 = polyData0.mPolygonVertexRefs + polygon0.mVRef8;

	//		for(PxU32 kStart = 0, kEnd =polygon0.mNbVerts-1; kStart<polygon0.mNbVerts; kEnd = kStart++)
	//		{
	//			//PxU32 indexK = (k+1)%polygon0.mNbVerts;

	//			const Vec3V p00 = Vec3V_From_PxVec3(polyData0.mVerts[inds0[kStart]]);
	//			const Vec3V p01 = Vec3V_From_PxVec3(polyData0.mVerts[inds0[kEnd]]);
	//			//change to shape space
	//			const Vec3V v0 = M33TrnspsMulV3(map0->vertex2Shape, V3Sub(p01, p00));
	//			//const Vec3V v0 = M33TrnspsMulV3(map0->shape2Vertex, V3Sub(p01, p00));

	//			//const Vec3V p000 = M33TrnspsMulV3(map0->vertex2Shape, Vec3V_From_PxVec3(polyData0.mVerts[inds0[k]]));
	//			//const Vec3V p001 = M33TrnspsMulV3(map0->vertex2Shape, Vec3V_From_PxVec3(polyData0.mVerts[inds0[indexK]]));
	//			////change to shape space
	//			//const Vec3V dir =V3Sub(p001, p000);


	//			for(PxU32 j = 0; j < polyData1.mNbPolygons; ++j)
	//			{
	//				const Gu::HullPolygonData& polygon1 = polyData1.mPolygons[j];
	//				const PxU8* inds1 = polyData1.mPolygonVertexRefs + polygon1.mVRef8;

	//				for(PxU32 lStart = 0, lEnd =polygon1.mNbVerts-1; lStart<polygon1.mNbVerts; lEnd = lStart++)
	//				{
	//					//PxU32 indexL = (l+1)%polygon1.mNbVerts;
	//					
	//					const Vec3V p10 = Vec3V_From_PxVec3(polyData1.mVerts[inds1[lStart]]);
	//					const Vec3V p11 = Vec3V_From_PxVec3(polyData1.mVerts[inds1[lEnd]]);
	//					//change to shape space
	//					const Vec3V v1 = M33TrnspsMulV3(map1->vertex2Shape, V3Sub(p11, p10));
	//					//const Vec3V v1 = M33TrnspsMulV3(map1->shape2Vertex, V3Sub(p11, p10));

	//					//n0 is in polyData0's local space
	//					//const Vec3V n0 = V3Normalize(V3Cross(v0, transform1To0.rotate(v1)));
	//					const Vec3V n0 = V3Normalize(V3Cross(v0, transform1To0.rotate(v1)));
	//					/*const Vec3V worldN0 = V3Normalize(V3Cross(worldV0, worldV1));*/

	//					//n1 is in polyData1's local space
	//					const Vec3V n1 = transform0To1.rotate(n0);

	//					//get polyData0's projection

	//					const Vec3V support0A = map0->doSupport(V3Neg(n0));
	//					const Vec3V support0B = map0->doSupport(n0);

	//					min0 = V3Dot(n0, support0A);
	//					max0 = V3Dot(n0, support0B);
	//					

	//					//get polyData1's projection
	//					const Vec3V support1A = map1->doSupport(V3Neg(n1));
	//					const Vec3V support1B = map1->doSupport(n1);

	//					const Vec3V minPoint = transform1To0.transform(support1A);
	//					const Vec3V maxPoint = transform1To0.transform(support1B);
	//					

	//					min1 = V3Dot(n0, minPoint);
	//					max1 = V3Dot(n0, maxPoint);

	//					/*const FloatV worldMin1 = V3Dot(worldN0, map1->transform.transform(support1A));
	//					const FloatV worldMax1 = V3Dot(worldN0, map1->transform.transform(support1B));*/

	//					const BoolV con = BOr(FIsGrtr(min1, FAdd(max0, contactDist)), FIsGrtr(min0, FAdd(max1, contactDist)));
	//					if(BAllEq(con, bTrue))
	//						return false;

	//					//const FloatV tempOverlap = FMin(d0, d1);
	//					const FloatV tempOverlap = FSub(max0, min1);

	//					if(FAllGrtr(overlap, tempOverlap))
	//					{
	//						overlap = tempOverlap;
	//						edgeNormalIn0 = n0;
	//						//map0->transform.rotate(n0);
	//					}
	//				}

	//			}
	//		}

	//	}

	//	minOverlap = overlap;
	//
	//	return true;

	//}



	
	static bool testSATCapsulePoly(const Gu::CapsuleV& capsule, const Gu::PolygonalData& polyData, SupportLocal* map,  const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& seperatingAxis)
	{
		using namespace Ps::aos;
		FloatV _minOverlap = FMax();//minOverlap;
		FloatV min0, max0;
		FloatV min1, max1;
		Vec3V tempAxis = V3UnitY();
		const BoolV bTrue = BTTTT();


		//in the local space of polyData
		for(PxU32 i=0; i<polyData.mNbPolygons; ++i)
		{
			const Gu::HullPolygonData& polygon = polyData.mPolygons[i];

			const Vec3V minVert = V3LoadU(polyData.mVerts[polygon.mMinIndex]);
			const FloatV planeDist = FLoad(polygon.mPlane.d);
			const Vec3V vertexSpacePlaneNormal = V3LoadU(polygon.mPlane.n);

			//transform plane n to shape space
			const Vec3V shapeSpacePlaneNormal = M33TrnspsMulV3(map->shape2Vertex, vertexSpacePlaneNormal);

			const FloatV magnitude = FRecip(V3Length(shapeSpacePlaneNormal));
			//normalize shape space normal
			const Vec3V planeN = V3Scale(shapeSpacePlaneNormal, magnitude);
			//ML::use this to avoid LHS
			min0 = FMul(V3Dot(vertexSpacePlaneNormal, minVert), magnitude);
			max0 = FMul(FNeg(planeDist), magnitude);

		
			const FloatV tempMin = V3Dot(capsule.p0, planeN);
			const FloatV tempMax = V3Dot(capsule.p1, planeN);
			min1 = FMin(tempMin, tempMax);
			max1 = FMax(tempMin, tempMax);

			min1 = FSub(min1, capsule.radius);
			max1 = FAdd(max1, capsule.radius);


			const BoolV con = BOr(FIsGrtr(min1, FAdd(max0, contactDist)), FIsGrtr(min0, FAdd(max1, contactDist)));

			if(BAllEq(con, bTrue))
				return false;

			
			/*const FloatV d0 = FSub(max0, min1);
			const FloatV d1 = FSub(max1, min0);*/
			//const FloatV tempOverlap = FMin(d0, d1);
			const FloatV tempOverlap = FSub(max0, min1);

			if(FAllGrtr(_minOverlap, tempOverlap))
			{
				_minOverlap = tempOverlap;
				tempAxis = planeN;
			}
		}

		const Vec3V capsuleAxis = V3Sub(capsule.p1, capsule.p0);
	
		for(PxU32 i=0;i<polyData.mNbPolygons;i++)
		{
			const Gu::HullPolygonData& polygon = polyData.mPolygons[i];
			const PxU8* inds1 = polyData.mPolygonVertexRefs + polygon.mVRef8;

			for(PxU32 lStart = 0, lEnd =polygon.mNbVerts-1; lStart<polygon.mNbVerts; lEnd = lStart++)
			{

				//in the vertex space
				const Vec3V p10 = V3LoadU(polyData.mVerts[inds1[lStart]]);
				const Vec3V p11 = V3LoadU(polyData.mVerts[inds1[lEnd]]);
				const Vec3V vertexSpace = V3Sub(p11, p10);

				//transform v to shape space
				const Vec3V shapeSpaceV = M33TrnspsMulV3(map->shape2Vertex, vertexSpace);

				const Vec3V normal = V3Normalize(V3Cross(capsuleAxis, shapeSpaceV));

				map->doSupport(normal, min0, max0);

				const FloatV tempMin = V3Dot(capsule.p0, normal);
				const FloatV tempMax = V3Dot(capsule.p1, normal);
				min1 = FMin(tempMin, tempMax);
				max1 = FMax(tempMin, tempMax);

				min1 = FSub(min1, capsule.radius);
				max1 = FAdd(max1, capsule.radius);

				const BoolV con = BOr(FIsGrtr(min1, FAdd(max0, contactDist)), FIsGrtr(min0, FAdd(max1, contactDist)));

				if(BAllEq(con, bTrue))
					return false;

			
				const FloatV tempOverlap = FSub(max0, min1);

				if(FAllGrtr(_minOverlap, tempOverlap))
				{
					_minOverlap = tempOverlap;
					tempAxis = normal;
				}
			}
		}
		
		seperatingAxis = tempAxis;

		return true;

	}


	static bool testFaceNormal(const Gu::PolygonalData& polyData0, const Gu::PolygonalData& polyData1, SupportLocal* map0, SupportLocal* map1, const Ps::aos::PsMatTransformV& transform0To1, const Ps::aos::PsMatTransformV& transform1To0, const Ps::aos::FloatVArg contactDist, 
		Ps::aos::FloatV& minOverlap, PxU32& feature, Ps::aos::Vec3V& faceNormal, const FeatureStatus faceStatus, FeatureStatus& status)
	{
		PX_UNUSED(polyData1);
		
		using namespace Ps::aos;
		FloatV _minOverlap = FMax();//minOverlap;
		PxU32  _feature = 0;
		Vec3V  _faceNormal = faceNormal;
		FloatV min0, max0;
		FloatV min1, max1;
		const BoolV bTrue = BTTTT();

		const Vec3V center1To0 = transform1To0.p;

		if(map0->isIdentityScale)
		{

			//in the local space of polyData0
			for(PxU32 i=0; i<polyData0.mNbPolygons; ++i)
			{
				const Gu::HullPolygonData& polygon = polyData0.mPolygons[i];

				const Vec3V minVert = V3LoadU(polyData0.mVerts[polygon.mMinIndex]);
				const FloatV planeDist = FLoad(polygon.mPlane.d);

				//vertex and shape space are the same
				const Vec3V shapeSpacePlaneNormal = V3LoadU(polygon.mPlane.n);

				//ML::use this to avoid LHS
				min0 = V3Dot(shapeSpacePlaneNormal, minVert);
				max0 = FNeg(planeDist);

				//normalize shape space normal

				//calculate polyData1 projection
				//rotate polygon's normal into the local space of polyData1
				const Vec3V normal = transform0To1.rotate(shapeSpacePlaneNormal);
				const FloatV translate = V3Dot(center1To0, shapeSpacePlaneNormal);

				map1->doSupport(normal, min1, max1);

				min1 = FAdd(translate, min1);
				max1 = FAdd(translate, max1);

				const BoolV con = BOr(FIsGrtr(min1, FAdd(max0, contactDist)), FIsGrtr(min0, FAdd(max1, contactDist)));

				if(BAllEq(con, bTrue))
					return false;

				
				/*const FloatV d0 = FSub(max0, min1);
				const FloatV d1 = FSub(max1, min0);*/
				//const FloatV tempOverlap = FMin(d0, d1);
				const FloatV tempOverlap = FSub(max0, min1);

				if(FAllGrtr(_minOverlap, tempOverlap))
				{
					_minOverlap = tempOverlap;
					_feature = i;
					_faceNormal = shapeSpacePlaneNormal;
				}
			}   

			if(FAllGrtr(minOverlap, _minOverlap))
			{
				faceNormal = _faceNormal; 
				minOverlap = _minOverlap;
				status = faceStatus;
			}
		}
		else
		{
			//in the local space of polyData0
			for(PxU32 i=0; i<polyData0.mNbPolygons; ++i)
			{
				const Gu::HullPolygonData& polygon = polyData0.mPolygons[i];

				const Vec3V minVert = V3LoadU(polyData0.mVerts[polygon.mMinIndex]);
				const FloatV planeDist = FLoad(polygon.mPlane.d);
				const Vec3V vertexSpacePlaneNormal = V3LoadU(polygon.mPlane.n);

				//transform plane n to shape space
				const Vec3V shapeSpacePlaneNormal = M33TrnspsMulV3(map0->shape2Vertex, vertexSpacePlaneNormal);

				const FloatV magnitude = FRecip(V3Length(shapeSpacePlaneNormal));
				//ML::use this to avoid LHS
				min0 = FMul(V3Dot(vertexSpacePlaneNormal, minVert), magnitude);
				max0 = FMul(FNeg(planeDist), magnitude);

				//normalize shape space normal
				const Vec3V planeN = V3Scale(shapeSpacePlaneNormal, magnitude);

				//calculate polyData1 projection
				//rotate polygon's normal into the local space of polyData1
				const Vec3V normal = transform0To1.rotate(planeN);

				const FloatV translate = V3Dot(center1To0, planeN);
				map1->doSupport(normal, min1, max1);

				min1 = FAdd(translate, min1);
				max1 = FAdd(translate, max1);

				const BoolV con = BOr(FIsGrtr(min1, FAdd(max0, contactDist)), FIsGrtr(min0, FAdd(max1, contactDist)));

				if(BAllEq(con, bTrue))
					return false;

				
				/*const FloatV d0 = FSub(max0, min1);
				const FloatV d1 = FSub(max1, min0);*/
				//const FloatV tempOverlap = FMin(d0, d1);
				const FloatV tempOverlap = FSub(max0, min1);

				if(FAllGrtr(_minOverlap, tempOverlap))
				{
					_minOverlap = tempOverlap;
					_feature = i;
					_faceNormal = planeN;
				}
			}   

			if(FAllGrtr(minOverlap, _minOverlap))
			{
				faceNormal = _faceNormal; 
				minOverlap = _minOverlap;
				status = faceStatus;
			}

		}

		feature = _feature;

		return true;

	}


	static bool testEdgeNormal(const Gu::HullPolygonData& polygon0, const Gu::PolygonalData& polyData0, const Gu::HullPolygonData& polygon1, const Gu::PolygonalData& polyData1, SupportLocal* map0, SupportLocal* map1, const Ps::aos::PsMatTransformV& transform0To1, const Ps::aos::PsMatTransformV& transform1To0, const Ps::aos::FloatVArg contactDist,
		Ps::aos::FloatV& minOverlap, Ps::aos::Vec3V& edgeNormalIn0, const FeatureStatus edgeStatus, FeatureStatus& status)
	{
		using namespace Ps::aos;
		FloatV overlap = minOverlap;
		FloatV min0, max0;
		FloatV min1, max1;
		const BoolV bTrue = BTTTT();
		const FloatV eps = FEps();
		//in the local space of polyData0
	
		const PxU8* inds0 = polyData0.mPolygonVertexRefs + polygon0.mVRef8;
		const PxU8* inds1 = polyData1.mPolygonVertexRefs + polygon1.mVRef8;

		const Vec3V center1To0 = transform1To0.p;


		for(PxU32 kStart = 0, kEnd =polygon0.mNbVerts-1; kStart<polygon0.mNbVerts; kEnd = kStart++)
		{
			const Vec3V p00 = V3LoadU(polyData0.mVerts[inds0[kStart]]);
			const Vec3V p01 = V3LoadU(polyData0.mVerts[inds0[kEnd]]);
			//change to shape space
			const Vec3V v0 = M33TrnspsMulV3(map0->vertex2Shape, V3Sub(p01, p00));

			for(PxU32 lStart = 0, lEnd =polygon1.mNbVerts-1; lStart<polygon1.mNbVerts; lEnd = lStart++)
			{
				//PxU32 indexL = (l+1)%polygon1.mNbVerts;
				//change to shape space
				const Vec3V p10 = V3LoadU(polyData1.mVerts[inds1[lStart]]);
				const Vec3V p11 = V3LoadU(polyData1.mVerts[inds1[lEnd]]);
				const Vec3V v1 =  M33TrnspsMulV3(map1->vertex2Shape, V3Sub(p11, p10));

				//const Vec3V worldP10 = map1->transform.transform(p10);
				//const Vec3V worldP11 = map1->transform.transform(p11);
				//const Vec3V worldV1 = V3Sub(worldP11, worldP10);

				const Vec3V dir = V3Cross(v0, transform1To0.rotate(v1));
				const FloatV lenSq = V3Dot(dir, dir);
				if(FAllGrtr(eps, lenSq))
					continue;

				//
				//const Vec3V n0 = V3Sel(FIsGrtr(len, eps), V3Div(dir, len), dir);

				////n0 is in polyData0's local space
				const Vec3V n0 = V3Scale(dir, FRsqrt(lenSq));
				/*const Vec3V worldN0 = V3Normalize(V3Cross(worldV0, worldV1));*/

				//n1 is in polyData1's local space
				const Vec3V n1 = transform0To1.rotate(n0);

				//get polyData0's projection

				/*min0 = V3Dot(n0, map0->doSupportFast(V3Neg(n0)));
				max0 = V3Dot(n0, map0->doSupportFast(n0));*/

				map0->doSupport(n0, min0, max0);

				////get polyData1's projection
				//const Vec3V minPoint = transform1To0.transform(map1->doSupportFast(V3Neg(n1)));
				//const Vec3V maxPoint = transform1To0.transform(map1->doSupportFast(n1));

				//min1 = V3Dot(n0, minPoint);
				//max1 = V3Dot(n0, maxPoint);

				const FloatV translate = V3Dot(center1To0, n0);

				map1->doSupport(n1, min1, max1);

				min1 = FAdd(translate, min1);
				max1 = FAdd(translate, max1);

				/*const FloatV worldMin1 = V3Dot(worldN0, map1->transform.transform(support1A));
				const FloatV worldMax1 = V3Dot(worldN0, map1->transform.transform(support1B));*/

				const BoolV con = BOr(FIsGrtr(min1, FAdd(max0, contactDist)), FIsGrtr(min0, FAdd(max1, contactDist)));
				if(BAllEq(con, bTrue))
					return false;

				//const FloatV tempOverlap = FMin(d0, d1);
				const FloatV tempOverlap = FSub(max0, min1);

				if(FAllGrtr(overlap, tempOverlap))
				{
					overlap = tempOverlap;
					edgeNormalIn0 = n0;
					status = edgeStatus;
				}
			}
		}

		minOverlap = overlap;
		
		return true;

	}

	void generatedCapsuleBoxFaceContacts(const Gu::CapsuleV& capsule,  Gu::PolygonalData& polyData,const Gu::HullPolygonData& referencePolygon, Gu::SupportLocal* map, const Ps::aos::PsMatTransformV& aToB, Gu::PersistentContact* manifoldContacts, PxU32& numContacts, 
		const Ps::aos::FloatVArg contactDist, const Ps::aos::Vec3VArg normal)
	{

		using namespace Ps::aos;

		const BoolV bTrue = BTTTT();
		const FloatV radius = FAdd(capsule.radius, contactDist);

		//calculate the intersect point of ray to plane
		const Vec3V planeNormal = V3Normalize(M33TrnspsMulV3(map->shape2Vertex, V3LoadU(referencePolygon.mPlane.n)));
		const PxU8* inds = polyData.mPolygonVertexRefs + referencePolygon.mVRef8;
		const Vec3V a = M33MulV3(map->vertex2Shape,V3LoadU(polyData.mVerts[inds[0]]));
		/*const FloatV denom0 = V3Dot(planeNormal, V3Sub(a, capsule.p0)); 
		const FloatV denom1 = V3Dot(planeNormal, V3Sub(a, capsule.p1)); */

		const FloatV denom0 = V3Dot(planeNormal, V3Sub(capsule.p0, a)); 
		const FloatV denom1 = V3Dot(planeNormal, V3Sub(capsule.p1, a)); 
		const FloatV numer= FRecip(V3Dot(planeNormal, normal));
		const FloatV t0 = FMul(denom0, numer);
		const FloatV t1 = FMul(denom1, numer);

		const BoolV con0 = FIsGrtrOrEq(radius, t0);
		const BoolV con1 = FIsGrtrOrEq(radius, t1);
		if(BAllEq(BOr(con0, con1), bTrue))
		{

			const Mat33V rot = findRotationMatrixFromZAxis(planeNormal);
			Vec3V* points0In0 = (Vec3V*)PxAllocaAligned(sizeof(Vec3V)*referencePolygon.mNbVerts, 16);
			map->populateVerts(inds, referencePolygon.mNbVerts, polyData.mVerts, points0In0);
			//Gu::PersistentContactManifold::drawPolygon(*gRenderOutPut, map->transform, points0In0, referencePolygon.mNbVerts, 0xffff0000);


			Vec3V rPolygonMin = V3Splat(FMax());
			Vec3V rPolygonMax = V3Neg(rPolygonMin);
			for(PxU32 i=0; i<referencePolygon.mNbVerts; ++i)
			{
				points0In0[i] = M33MulV3(rot, points0In0[i]);
				rPolygonMin = V3Min(rPolygonMin, points0In0[i]);
				rPolygonMax = V3Max(rPolygonMax, points0In0[i]);
			}

			if(BAllEq(con0, bTrue))
			{
				//add contact
				const Vec3V proj = V3NegScaleSub(normal, t0, capsule.p0);//V3ScaleAdd(t0, normal, capsule.p0);
				//transform proj0 to 2D
				const Vec3V point = M33MulV3(rot, proj);

				if(contains(points0In0, referencePolygon.mNbVerts, point, rPolygonMin, rPolygonMax))
				{
					//const FloatV pen = t0;
					manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(capsule.p0);
					/*const FloatV pen = FSub(t0, capsule.radius);
					manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(V3NegScaleSub(normal, capsule.radius, capsule.p0));*/
					manifoldContacts[numContacts].mLocalPointB = proj;
					manifoldContacts[numContacts++].mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal), t0);
				}

			}

			if(BAllEq(con1, bTrue))
			{
				const Vec3V proj = V3NegScaleSub(normal, t1, capsule.p1);//V3ScaleAdd(t1, normal, capsule.p1);
				//transform proj0 to 2D
				const Vec3V point = M33MulV3(rot, proj);

				if(contains(points0In0, referencePolygon.mNbVerts, point, rPolygonMin, rPolygonMax))
				{
					
					//const FloatV pen = t1;
					manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(capsule.p1);
					/*const FloatV pen = FSub(t1, capsule.radius);
					manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(V3NegScaleSub(normal, capsule.radius, capsule.p1));*/
					manifoldContacts[numContacts].mLocalPointB = proj;
					manifoldContacts[numContacts++].mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal),t1);
				}
			}
		}

	}

	////capsule in the local space of box
	//void generatedCapsuleBoxFaceContacts(const Gu::CapsuleV& capsule,  const Gu::BoxV& box, const Ps::aos::PsMatTransformV& aToB, Gu::PersistentContact* manifoldContacts, PxU32& numContacts, 
	//	const Ps::aos::FloatVArg contactDist, const Ps::aos::Vec3VArg normal)
	//{

	//	using namespace Ps::aos;
	//	const Vec3V inflatedRadius = FAdd(capsule.radius, contactDist);
	//	const Vec3V dir = V3Neg(normal);
	//	const Vec3V minimum = V3Neg(box.extents);
	//	FloatV tNear, tFar;

	//	if(intersectRayAABBLocal2(minimum, box.extents, capsule.p0, dir, tNear, tFar) && FAllGrtr(inflatedRadius, tNear))
	//	{
	//		
	//		manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(capsule.p0);
	//		manifoldContacts[numContacts].mLocalPointB = V3ScaleAdd(dir, tNear, capsule.p0);
	//		manifoldContacts[numContacts++].mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal),tNear);
	//		
	//	}
	//
	//	if(intersectRayAABBLocal2(minimum, box.extents, capsule.p1, dir, tNear, tFar) && FAllGrtr(inflatedRadius, tNear))
	//	{
	//		manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(capsule.p1);
	//		manifoldContacts[numContacts].mLocalPointB = V3ScaleAdd(dir, tNear, capsule.p1);
	//		manifoldContacts[numContacts++].mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal),tNear);
	//	}
	//}

	static void generatedFaceContacts(const Gu::CapsuleV& capsule,  Gu::PolygonalData& polyData, Gu::SupportLocal* map, const Ps::aos::PsMatTransformV& aToB, Gu::PersistentContact* manifoldContacts, PxU32& numContacts, 
		const Ps::aos::FloatVArg contactDist, const Ps::aos::Vec3VArg normal)
	{
		using namespace Ps::aos;

		FloatV tEnter, tExit;
		const FloatV inflatedRadius = FAdd(capsule.radius, contactDist);
		const Vec3V dir = V3Neg(normal);
		const Vec3V vertexSpaceDir = M33MulV3(map->shape2Vertex, dir);
		//const Vec3V vertexSpaceDir = M33TrnspsMulV3(map->vertex2Shape, dir);
		const Vec3V p0 = M33MulV3(map->shape2Vertex, capsule.p0);
		if(intersectSegmentPolyhedron(p0, vertexSpaceDir, polyData, tEnter, tExit) && FAllGrtrOrEq(inflatedRadius, tEnter))
		{
			manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(capsule.p0);
			manifoldContacts[numContacts].mLocalPointB = V3ScaleAdd(dir, tEnter, capsule.p0);
			manifoldContacts[numContacts++].mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal), tEnter);
		}

		const Vec3V p1 = M33MulV3(map->shape2Vertex, capsule.p1);
		if(intersectSegmentPolyhedron(p1, vertexSpaceDir, polyData, tEnter, tExit) && FAllGrtrOrEq(inflatedRadius, tEnter))
		{
			manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(capsule.p1);
			manifoldContacts[numContacts].mLocalPointB = V3ScaleAdd(dir, tEnter, capsule.p1);
			manifoldContacts[numContacts++].mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal), tEnter);
		}
		
	}


	static void generateEE(const Ps::aos::Vec3VArg p, const Ps::aos::Vec3VArg q, const Ps::aos::Vec3VArg normal, const Ps::aos::Vec3VArg a, const Ps::aos::Vec3VArg b,
		const Ps::aos::PsMatTransformV& aToB,  Gu::PersistentContact* manifoldContacts, PxU32& numContacts, const Ps::aos::FloatVArg inflatedRadius)
	{
		using namespace Ps::aos;
		const FloatV zero = FZero();
		const Vec3V ab = V3Sub(b, a);
		const Vec3V n = V3Cross(ab, normal);
		const FloatV d = V3Dot(n, a);
		const FloatV np = V3Dot(n, p);
		const FloatV nq = V3Dot(n,q);
		const FloatV signP = FSub(np, d);
		const FloatV signQ = FSub(nq, d);
		const FloatV temp = FMul(signP, signQ);
		if(FAllGrtr(temp, zero)) return;//If both points in the same side as the plane, no intersect points
		
		// if colliding edge (p3,p4) and plane are parallel return no collision
		const Vec3V pq = V3Sub(q, p);
		const FloatV npq= V3Dot(n, pq); 
		if(FAllEq(npq, zero))	return;


		//calculate the intersect point in the segment pq
		const FloatV segTValue = FDiv(FSub(d, np), npq);
		const Vec3V localPointA = V3ScaleAdd(pq, segTValue, p);

		//calculate a normal perpendicular to ray localPointA + normal, 2D segment segment intersection
		const Vec3V perNormal = V3Cross(normal, n);
		const Vec3V ap = V3Sub(localPointA, a);
		const FloatV nom = V3Dot(perNormal, ap);
		const FloatV denom = V3Dot(perNormal, ab);

		const FloatV tValue = FDiv(nom, denom);

		if(FAllGrtr(tValue, FOne()) || FAllGrtr(zero, tValue))
			return;

		//const Vec3V localPointB = V3ScaleAdd(ab, tValue, a); v = V3Sub(localPointA, localPointB); v =  V3NegScaleSub(ab, tValue, tap)
		const Vec3V v = V3NegScaleSub(ab, tValue, ap);
		const FloatV signedDist = V3Dot(v, normal);
		if(FAllGrtrOrEq(inflatedRadius, signedDist))
		{
			const Vec3V localPointB = V3Sub(localPointA, v);
			manifoldContacts[numContacts].mLocalPointA = aToB.transformInv(localPointA);
			manifoldContacts[numContacts].mLocalPointB = localPointB;
			manifoldContacts[numContacts++].mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(normal), signedDist);
		}
	}


	void generatedContactsEEContacts(const Gu::CapsuleV& capsule, Gu::PolygonalData& polyData, const Gu::HullPolygonData& referencePolygon, Gu::SupportLocal* map,  const Ps::aos::PsMatTransformV& aToB, Gu::PersistentContact* manifoldContacts, 
		PxU32& numContacts, const Ps::aos::FloatVArg contactDist, const Ps::aos::Vec3VArg contactNormal)
	{

		using namespace Ps::aos;

		const PxU8* inds = polyData.mPolygonVertexRefs + referencePolygon.mVRef8;

		Vec3V* points0In0 = (Vec3V*)PxAllocaAligned(sizeof(Vec3V)*referencePolygon.mNbVerts, 16);


		//Transform all the verts from vertex space to shape space
		map->populateVerts(inds, referencePolygon.mNbVerts, polyData.mVerts, points0In0);

		/*static bool draw = false;
		if(draw)
		{
			Gu::PersistentContactManifold::drawPolygon(*gRenderOutPut, map->transform, points0In0, referencePolygon.mNbVerts, 0xffff0000);
		}*/

		const FloatV inflatedRadius = FAdd(capsule.radius, contactDist);

		for (PxU32 rStart = 0, rEnd = referencePolygon.mNbVerts - 1; rStart < referencePolygon.mNbVerts; rEnd = rStart++) 
		{
			generateEE(capsule.p0, capsule.p1, contactNormal,points0In0[rStart], points0In0[rEnd], aToB,  manifoldContacts, numContacts, inflatedRadius);
		}
	}

  
	void generatedContacts(Gu::PolygonalData& polyData0, Gu::PolygonalData& polyData1,const Gu::HullPolygonData& referencePolygon, const Gu::HullPolygonData& incidentPolygon,  Gu::SupportLocal* map0, Gu::SupportLocal* map1, const Ps::aos::PsMatTransformV& transform0To1, Gu::PersistentContact* manifoldContacts, PxU32& numContacts, const Ps::aos::FloatVArg contactDist)
	{
		using namespace Ps::aos;

		
		const FloatV zero = FZero();
		const BoolV bTrue = BTTTT();


		const PxU8* inds0 = polyData0.mPolygonVertexRefs + referencePolygon.mVRef8;

		//transform the plane normal to shape space
		const Vec3V contactNormal = V3Normalize(M33TrnspsMulV3(map0->shape2Vertex, V3LoadU(referencePolygon.mPlane.n)));
		//const FloatV rPlaneDist = V3Dot(contactNormal, Vec3V_From_PxVec3(polyData0.mVerts[inds0[0]]));
		//const FloatV referenceD = FloatV_From_F32(referencePolygon.mPlane.d);
		
		//this is the matrix transform all points to the 2d plane
		const Mat33V rot = findRotationMatrixFromZAxis(contactNormal);

		const PxU8* inds1 = polyData1.mPolygonVertexRefs + incidentPolygon.mVRef8;


		Vec3V* points0In0 = (Vec3V*)PxAllocaAligned(sizeof(Vec3V)*referencePolygon.mNbVerts, 16);
		Vec3V* points1In0 = (Vec3V*)PxAllocaAligned(sizeof(Vec3V)*incidentPolygon.mNbVerts, 16);
		bool* points1In0Penetration = (bool*)PxAlloca(sizeof(bool)*incidentPolygon.mNbVerts);
		FloatV* points1In0TValue = (FloatV*)PxAllocaAligned(sizeof(FloatV)*incidentPolygon.mNbVerts, 16);

		//Transform all the verts from vertex space to shape space
		map0->populateVerts(inds0, referencePolygon.mNbVerts, polyData0.mVerts, points0In0);
		map1->populateVerts(inds1, incidentPolygon.mNbVerts, polyData1.mVerts, points1In0);

	
		//This is used to calculate the project point when the 2D reference face points is inside the 2D incident face point
		const Vec3V sPoint = points1In0[0]; 
		
		//const PxU32 referenceColor = 0xff0000ff;
		//const PxU32 incidentColor = 0xff00ffff;
		//{
		//	//draw out reference face
		//	Gu::PersistentContactManifold::drawPolygon(*gRenderOutPut, map0->transform, points0In0, referencePolygon.mNbVerts, referenceColor);

		//	//draw out incident face
		//	Gu::PersistentContactManifold::drawPolygon(*gRenderOutPut, map1->transform, points1In0, incidentPolygon.mNbVerts, incidentColor);
		//}  

		//the first point in the reference plane
		//const Vec3V referencePoint = points0In0[0];

		PX_ASSERT(incidentPolygon.mNbVerts <= 64);
 
		Vec3V eps = Vec3V_From_FloatV(FEps());
		Vec3V max = Vec3V_From_FloatV(FMax());
		Vec3V nmax = V3Neg(max); 

		//transform reference polygon to 2d, calculate min and max
		Vec3V rPolygonMin= max;
		Vec3V rPolygonMax = nmax;
		for(PxU32 i=0; i<referencePolygon.mNbVerts; ++i)
		{
			//points0In0[i].vertext = M33TrnspsMulV3(rot, Vec3V_From_PxVec3(polyData0.mVerts[inds0[i]]));
			points0In0[i] = M33MulV3(rot, points0In0[i]);
			rPolygonMin = V3Min(rPolygonMin, points0In0[i]);
			rPolygonMax = V3Max(rPolygonMax, points0In0[i]);
		}
		
		rPolygonMin = V3Sub(rPolygonMin, eps);
		rPolygonMax = V3Add(rPolygonMax, eps);

		const FloatV d = V3GetZ(points0In0[0]);
		const FloatV rd = FAdd(d, contactDist);

		Vec3V iPolygonMin= max; 
		Vec3V iPolygonMax = nmax;

	/*	Vec4V normals[64];
		PX_ASSERT(referencePolygon.mNbVerts <=64);
		createNormal(polyData0, referencePolygon,inds0, normals);*/

		PxU32 inside = 0;
		for(PxU32 i=0; i<incidentPolygon.mNbVerts; ++i)
		{
			//const Vec3V vert1 = Vec3V_From_PxVec3(polyData1.mVerts[inds1[i]]);
			const Vec3V vert1 =points1In0[i]; //this still in polyData1's local space
			const Vec3V a = transform0To1.transformInv(vert1);
			points1In0[i] = M33MulV3(rot, a);
			const FloatV z = V3GetZ(points1In0[i]);
			//points1In0[i].t = FSub(z, d);
			points1In0TValue[i] = FSub(z, d);
			points1In0[i] = V3SetZ(points1In0[i], d);
			iPolygonMin = V3Min(iPolygonMin, points1In0[i]);
			iPolygonMax = V3Max(iPolygonMax, points1In0[i]);
			if(FAllGrtr(rd, z))
			{
				points1In0Penetration[i] = true;

				if(contains(points0In0, referencePolygon.mNbVerts, points1In0[i], rPolygonMin, rPolygonMax))
				{
					inside++;

					/*const FloatV t = V3Dot(contactNormal, V3Sub(a, referencePoint));
					const Vec3V projectPoint = V3NegScaleSub(contactNormal, t, a);

					const FloatV dif = V3Sub(t, points1In0TValue[i]);
					PX_ASSERT(FAllGrtr(FloatV_From_F32(0.0001f), FAbs(dif)) == 1);*/

					const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(contactNormal), points1In0TValue[i]);
					//manifold.addManifoldPoint(vert1, M33MulV3(rot, points1In0[i]), localNormalPen, replaceBreakingThreshold);
					manifoldContacts[numContacts].mLocalPointA = vert1;
					manifoldContacts[numContacts].mLocalPointB = M33TrnspsMulV3(rot, points1In0[i]);
					manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;
					//manifold.drawPoint(gThreadContext, transform0.transform(M33MulV3(rot, points1In0[i].vertext)), 0.5, 0x00ff0000);
				}
			}
			else
			{
				points1In0Penetration[i] = false;
			}
			
		}


		if(inside == incidentPolygon.mNbVerts)
		{
			//manifold.addBatchManifoldContacts(manifoldContacts, numContacts);
			return;
		}

		inside = 0;
		iPolygonMin = V3Sub(iPolygonMin, eps);
		iPolygonMax = V3Add(iPolygonMax, eps);

		const Vec3V incidentNormal = V3Normalize(M33TrnspsMulV3(map1->shape2Vertex, V3LoadU(incidentPolygon.mPlane.n)));
		
		//const FloatV iPlaneDist = FNeg(V3Dot(incidentNormal, M33MulV3(map1->vertex2Shape, V3LoadU(polyData1.mVerts[inds1[0]]))));
		//const FloatV iPlaneInflatedDist = FSub(iPlaneDist, contactDist);
		//const FloatV iPlaneD = FNeg(iPlaneDist);

		const Vec3V contactNormalIn1 = transform0To1.rotate(contactNormal);

		for(PxU32 i=0; i<referencePolygon.mNbVerts; ++i)
		{
			if(contains(points1In0, incidentPolygon.mNbVerts, points0In0[i], iPolygonMin, iPolygonMax))
			{
				//const Vec3V vert0=Vec3V_From_PxVec3(polyData0.mVerts[inds0[i]]);
				const Vec3V vert0 = M33TrnspsMulV3(rot, points0In0[i]);
				const Vec3V a = transform0To1.transform(vert0);

				const FloatV nom = V3Dot(incidentNormal, V3Sub(sPoint, a)); 
				const FloatV denom = V3Dot(incidentNormal, contactNormalIn1);
				PX_ASSERT(FAllEq(denom, zero)==0);
				const FloatV t = FDiv(nom, denom);

				if(FAllGrtr(t, contactDist))
					continue;


				const Vec3V projPoint = V3ScaleAdd(contactNormalIn1, t, a);
			

				const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(contactNormal), t);
		
				manifoldContacts[numContacts].mLocalPointA = projPoint;
				manifoldContacts[numContacts].mLocalPointB = vert0;
				manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;

				//manifold.drawPoint(gThreadContext, transform0.transform(M33MulV3(rot, points0In0[i].vertext)), 0.5, 0x00ff0000);

			}
		}

		if(inside == referencePolygon.mNbVerts)
			return;


		//(2) segment intesection
		for (PxU32 iStart = 0, iEnd = incidentPolygon.mNbVerts - 1; iStart < incidentPolygon.mNbVerts; iEnd = iStart++)
		{
			if((!points1In0Penetration[iStart] && !points1In0Penetration[iEnd] ) )//|| (points1In0[i].status == POINT_OUTSIDE && points1In0[incidentIndex].status == POINT_OUTSIDE))
				continue;

	
			const Vec3V ipA = points1In0[iStart];
			const Vec3V ipB = points1In0[iEnd];

			Vec3V ipAOri = V3SetZ(points1In0[iStart], FAdd(points1In0TValue[iStart], d));
			Vec3V ipBOri = V3SetZ(points1In0[iEnd], FAdd(points1In0TValue[iEnd], d));

			const Vec3V iMin = V3Min(ipA, ipB);
			const Vec3V iMax = V3Max(ipA, ipB);
			//const Vec3V iV = V3Sub(ipB, ipA);

			for (PxU32 rStart = 0, rEnd = referencePolygon.mNbVerts - 1; rStart < referencePolygon.mNbVerts; rEnd = rStart++) 
			{
	
				const Vec3V rpA = points0In0[rStart];
				const Vec3V rpB = points0In0[rEnd];

				const Vec3V rMin = V3Min(rpA, rpB);
				const Vec3V rMax = V3Max(rpA, rpB);
				//const BoolV con =BOr(FIsGrtr(iMin, FAdd(rMax, contactDist)), FIsGrtr(rMin, FAdd(iMax, contactDist)));
				const BoolV tempCon =BOr(V3IsGrtr(iMin, rMax), V3IsGrtr(rMin, iMax));
				const BoolV con = BOr(BGetX(tempCon), BGetY(tempCon));
		
				if(BAllEq(con, bTrue))
					continue;
			
					
				FloatV a1 = signed2DTriArea(rpA, rpB, ipA);
				FloatV a2 = signed2DTriArea(rpA, rpB, ipB);


				if(FAllGrtr(zero, FMul(a1, a2)))
				{
					FloatV a3 = signed2DTriArea(ipA, ipB, rpA);
					FloatV a4 = signed2DTriArea(ipA, ipB, rpB);

					if(FAllGrtr(zero, FMul(a3, a4)))
					{
					
						//these two segment intersect

						const FloatV t = FDiv(a1, FSub(a2, a1));

						const Vec3V pBB = V3NegScaleSub(V3Sub(ipBOri, ipAOri), t, ipAOri); 
						const Vec3V pAA = V3SetZ(pBB, d);
						const Vec3V pA = M33TrnspsMulV3(rot, pAA);
						const Vec3V pB = transform0To1.transform(M33TrnspsMulV3(rot, pBB));
						const FloatV pen = FSub(V3GetZ(pBB), V3GetZ(pAA));
						if(FAllGrtr(pen, contactDist))
							continue;


						const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(contactNormal), pen);
					
						//manifold.addManifoldPoint(pB, pA, localNormalPen, replaceBreakingThreshold);

						manifoldContacts[numContacts].mLocalPointA = pB;
						manifoldContacts[numContacts].mLocalPointB = pA;
						manifoldContacts[numContacts++].mLocalNormalPen = localNormalPen;

						//manifold.drawLine(gThreadContext, map0->transform.transform(M33MulV3(rot, ipA)), map0->transform.transform(M33MulV3(rot, ipB)), 0x00ff0000);
						//manifold.drawLine(gThreadContext, map0->transform.transform(M33MulV3(rot, rpA)), map0->transform.transform(M33MulV3(rot, rpB)), 0x0000ff00);
			
						/*manifold.drawPoint(gThreadContext, map0->transform.transform(pA), 0.2f, 0x0000ffff);
						manifold.drawPoint(gThreadContext, map1->transform.transform(pB), 0.2f, 0x00ffffff);*/
					}
				}
			}

		}
	}


	bool generateCapsuleBoxFullContactManifold(const Gu::CapsuleV& capsule, const Gu::BoxV& box, Gu::PolygonalData& polyData, Gu::SupportLocal* map, const Ps::aos::PsMatTransformV& aToB, Gu::PersistentContact* manifoldContacts, PxU32& numContacts,
		const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& normal, const bool doOverlapTest)
	{
		using namespace Ps::aos;
		PX_UNUSED(box);
		
		if(doOverlapTest)
		{	
			Ps::aos::Vec3V seperatingAxis;
			if(!testSATCapsulePoly(capsule, polyData, map, contactDist, seperatingAxis))
				return false;

			const Gu::HullPolygonData& referencePolygon = polyData.mPolygons[getPolygonIndex(polyData, map, V3Neg(seperatingAxis))];
			generatedCapsuleBoxFaceContacts(capsule, polyData, referencePolygon, map, aToB, manifoldContacts, numContacts, contactDist, seperatingAxis);
			//generatedFaceContacts(capsule, polyData, map, aToB, manifoldContacts, numContacts, contactDist, seperatingAxis);
			//generatedCapsuleBoxFaceContacts(capsule, box, aToB, manifoldContacts, numContacts, contactDist, seperatingAxis);
			if(numContacts < 2)
			{
				//const Gu::HullPolygonData& referencePolygon = polyData.mPolygons[getPolygonIndex(polyData, map, V3Neg(seperatingAxis))];
				generatedContactsEEContacts(capsule, polyData, referencePolygon, map, aToB, manifoldContacts, numContacts, contactDist, seperatingAxis);
			}

			normal = seperatingAxis;
		}
		else
		{
			const Gu::HullPolygonData& referencePolygon = polyData.mPolygons[getPolygonIndex(polyData, map, V3Neg(normal))];
			generatedCapsuleBoxFaceContacts(capsule, polyData, referencePolygon, map, aToB, manifoldContacts, numContacts, contactDist, normal);
			//generatedCapsuleBoxFaceContacts(capsule, box, aToB, manifoldContacts, numContacts, contactDist, normal);
			if(numContacts < 2)
			{
				//const Gu::HullPolygonData& referencePolygon = polyData.mPolygons[getPolygonIndex(polyData, map, V3Neg(normal))];
				generatedContactsEEContacts(capsule, polyData, referencePolygon, map, aToB, manifoldContacts, numContacts, contactDist, normal);
			}
		}
		
		return true;
	}



	//capsule is in the local space of polyData
	bool generateFullContactManifold(const Gu::CapsuleV& capsule, Gu::PolygonalData& polyData, Gu::SupportLocal* map, const Ps::aos::PsMatTransformV& aToB, Gu::PersistentContact* manifoldContacts, PxU32& numContacts,
		const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& normal, const bool doOverlapTest)
	{
		using namespace Ps::aos;
		
		if(doOverlapTest)
		{	
			Ps::aos::Vec3V seperatingAxis;
			if(!testSATCapsulePoly(capsule, polyData, map, contactDist, seperatingAxis))
				return false;

			generatedFaceContacts(capsule, polyData, map, aToB, manifoldContacts, numContacts, contactDist, seperatingAxis);
			if(numContacts < 2)
			{
				const Gu::HullPolygonData& referencePolygon = polyData.mPolygons[getPolygonIndex(polyData, map, V3Neg(seperatingAxis))];
				generatedContactsEEContacts(capsule, polyData, referencePolygon, map, aToB, manifoldContacts, numContacts, contactDist, seperatingAxis);
			}

			normal = seperatingAxis;
		}
		else
		{
			generatedFaceContacts(capsule, polyData, map, aToB, manifoldContacts, numContacts, contactDist, normal);
			if(numContacts < 2)
			{
				const Gu::HullPolygonData& referencePolygon = polyData.mPolygons[getPolygonIndex(polyData, map, V3Neg(normal))];
				generatedContactsEEContacts(capsule, polyData,referencePolygon, map, aToB,  manifoldContacts, numContacts, contactDist, normal);
			}
		}
		
		return true;
	}

	bool generateFullContactManifold(Gu::PolygonalData& polyData0, Gu::PolygonalData& polyData1,  SupportLocal* map0, SupportLocal* map1,/* Gu::PersistentContactManifold& manifold,*/ Gu::PersistentContact* manifoldContacts, PxU32& numContacts,
		const Ps::aos::FloatVArg contactDist, const Ps::aos::Vec3VArg normal, const bool doOverlapTest)
	{
	
		using namespace Ps::aos;

		const PsMatTransformV transform1To0V = map0->transform.transformInv(map1->transform);
		const PsMatTransformV transform0To1V = map1->transform.transformInv(map0->transform);

		PxU32 origContactCount = numContacts;

		if(doOverlapTest)
		{
			//if gjk fail, SAT based yes/no test
			FeatureStatus status = POLYDATA0;
			FloatV minOverlap = FMax();
			Vec3V minNormal = V3Zero();
	
			PxU32 feature0;
			//in the local space of polyData0, minNormal is in polyData0 space
			if(!testFaceNormal(polyData0, polyData1, map0, map1, transform0To1V, transform1To0V, contactDist, minOverlap, feature0, minNormal, POLYDATA0, status))
				return false;
			
			PxU32 feature1;
			//in the local space of polyData1, if minOverlap is overwrite inside this function, minNormal will be in polyData1 space
			if(!testFaceNormal(polyData1, polyData0, map1, map0, transform1To0V, transform0To1V, contactDist, minOverlap, feature1, minNormal, POLYDATA1, status))
				return false;

		
			bool doEdgeTest = false;
			
EdgeTest:
			if(doEdgeTest)
			{
				const Gu::HullPolygonData& polygon0 = polyData0.mPolygons[feature0];
				const Gu::HullPolygonData& polygon1 = polyData1.mPolygons[feature1];
				if(!testEdgeNormal(polygon0, polyData0, polygon1, polyData1, map0, map1, transform0To1V, transform1To0V, contactDist, minOverlap, minNormal, EDGE0, status))
					return false;

				if(status != EDGE0)
					return true;
			}


			//we should be able to get rid of this
			/*if(!testEdgeNormal1(polygon1, polyData1, polygon0, polyData0, map1, map0, transform1To0V, transform0To1V, contactDist, minOverlap, minNormal, EDGE1, status))
				return false;*/
  

			//FloatV minOverlap2 = FMax();
			//Vec3V edgeNormalIn0 = V3Zero();
			////test for edges
			////const Gu::HullPolygonData& polygon0 = polyData0.mPolygons[feature];
			//if(!testEdgeNormalBruteForce(polyData0, polyData1, map0, map1, transform0To1V, transform1To0V, contactDist, minOverlap2, edgeNormalIn0))
			//	return false;

			
			if(status == POLYDATA0)
			{
				//minNormal is in the local space of polydata0
				const Gu::HullPolygonData& incidentPolygon = polyData0.mPolygons[feature0];
				const Vec3V n = transform0To1V.rotate(minNormal);
				const Gu::HullPolygonData& referencePolygon = polyData1.mPolygons[getPolygonIndex(polyData1, map1, n)];
				

				generatedContacts(polyData1, polyData0, referencePolygon,  incidentPolygon, map1, map0, transform1To0V, manifoldContacts, numContacts, contactDist);
			}
			else if(status == POLYDATA1)
			{
				//minNormal is in the local space of polydata1
				const Gu::HullPolygonData& referencePolygon = polyData1.mPolygons[feature1];
				const Gu::HullPolygonData& incidentPolygon =  polyData0.mPolygons[getPolygonIndex(polyData0, map0, transform1To0V.rotate(minNormal))];
				
				//reference face is polyData1
				generatedContacts(polyData1, polyData0, referencePolygon,  incidentPolygon, map1, map0, transform1To0V, manifoldContacts, numContacts, contactDist);

			}
			else //if(status == EDGE0)
			{
				//minNormal is in the local space of polydata0
			
				const Gu::HullPolygonData& incidentPolygon = polyData0.mPolygons[getPolygonIndex(polyData0, map0, V3Neg(minNormal))];
				const Gu::HullPolygonData& referencePolygon = polyData1.mPolygons[getPolygonIndex(polyData1, map1, transform0To1V.rotate(minNormal))];
				generatedContacts(polyData1, polyData0, referencePolygon,  incidentPolygon, map1, map0, transform1To0V, manifoldContacts, numContacts, contactDist);
			}

			if(numContacts == origContactCount && !doEdgeTest)
			{
				doEdgeTest = true;
				goto EdgeTest;
			}

		}
		else
		{

			//use gjk normal to get the faceIndex(status == GJK_CONTACT)
			const PxI32 faceIndex1 = getPolygonIndex(polyData1, map1, V3Neg(normal));
			const PxI32 faceIndex0 = getPolygonIndex(polyData0, map0, transform0To1V.rotateInv(normal));
			const Gu::HullPolygonData& referencePolygon = polyData1.mPolygons[faceIndex1];
			const Gu::HullPolygonData& incidentPolygon = polyData0.mPolygons[faceIndex0];
			generatedContacts(polyData1, polyData0, referencePolygon,  incidentPolygon, map1, map0, transform1To0V, manifoldContacts, numContacts, contactDist);

		}
	
		return true;
	}

}//Gu
}//physx
