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

#ifndef GU_PCM_CONTACT_GEN_UTIL_H
#define GU_PCM_CONTACT_GEN_UTIL_H

#include "PsVecMath.h"
#include "CmPhysXCommon.h"
#include "GuShapeConvex.h"
#include "GuConvexSupportTable.h"

namespace physx
{

namespace Gu
{

	enum FeatureStatus
	{
		POLYDATA0,
		POLYDATA1,
		EDGE0,
		EDGE1
	};


	PX_FORCE_INLINE bool contains(Ps::aos::Vec3V* verts, const PxU32 numVerts, const Ps::aos::Vec3VArg p, const Ps::aos::Vec3VArg min, const Ps::aos::Vec3VArg max) 
	{ 
		using namespace Ps::aos;

		
		const BoolV bTrue = BTTTT();
		
		
		const BoolV tempCon = BOr(V3IsGrtr(min, p), V3IsGrtr(p, max));
		const BoolV con = BOr(BGetX(tempCon), BGetY(tempCon));
		
		if(BAllEq(con, bTrue))
			return false;  

		const FloatV tx = V3GetX(p);
		const FloatV ty = V3GetY(p); 

	/*	const FloatV tx = V3GetX(p);
		const FloatV ty = V3GetY(p); 
		const BoolV con0 = BOr(FIsGrtr(V3GetX(min), tx), FIsGrtr(tx, V3GetX(max)));
		const BoolV con1 = BOr(FIsGrtr(V3GetY(min), ty), FIsGrtr(ty, V3GetY(max)));
		if(BAllEq(BOr(con0, con1), bTrue))
			return false;
		*/
		
		const FloatV zero = FZero();
	//	const BoolV bFalse = BFFFF();
		PxU32 intersectionPoints = 0;
		
		for (PxU32 i = 0, j = numVerts - 1; i < numVerts; j = i++) 
		{
			
			const FloatV jx = V3GetX(verts[j]);
			const FloatV jy = V3GetY(verts[j]);
			const FloatV ix = V3GetX(verts[i]);
			const FloatV iy = V3GetY(verts[i]);

			//(verts[i].y > test.y) != (points[j].y > test.y) 
			const PxU32 yflag0 = FAllGrtr(jy, ty);
			const PxU32 yflag1 = FAllGrtr(iy, ty);

			//ML: the only case the ray will intersect this segment is when the p's y is in between two segments y
			if(yflag0 != yflag1)
			{
				//ML: choose ray, which start at p and every points in the ray will have the same y component
				//t1 = (yp - yj)/(yi - yj)
				//qx = xj + t1*(xi-xj)
				//t = qx - xp > 0 for the ray and segment intersection happen
				const FloatV jix = FSub(ix, jx);
				const FloatV jiy = FSub(iy, jy);
				//const FloatV jtx = FSub(tx, jy);
				const FloatV jty = FSub(ty, jy);
				const FloatV part1 = FMul(jty, jix);
				const FloatV part2 = FMul(jx,  jiy);
				//const FloatV part3 = FMul(V3Sub(tx, eps),  jiy);
				const FloatV part3 = FMul(tx,  jiy);

				const BoolV comp = FIsGrtr(jiy, zero);
				const FloatV tmp = FAdd(part1, part2);
				const FloatV comp1 = FSel(comp, tmp, part3);
				const FloatV comp2 = FSel(comp, part3, tmp);


				if(FAllGrtrOrEq(comp1, comp2))
				{
					if(intersectionPoints == 1)
					{
						return false;
					}
					intersectionPoints++;
				}

				//intersectPoints = VecI32V_Add(VecI32V_Sel(FIsGrtrOrEq(comp1, comp2), ione, izero), intersectPoints);

			}
		}
		return intersectionPoints> 0;
		//return BAllEq(VecI32V_IsEq(intersectPoints, ione), bTrue) != 0;
    } 
	

	PX_FORCE_INLINE Ps::aos::FloatV signed2DTriArea(const Ps::aos::Vec3VArg a, const Ps::aos::Vec3VArg b, const Ps::aos::Vec3VArg c)
	{
		using namespace Ps::aos;
		const FloatV cx = V3GetX(c);
		const FloatV cy = V3GetY(c);
		const FloatV t0 = FMul(FSub(V3GetX(a), cx), FSub(V3GetY(b), cy));
		const FloatV t1 = FMul(FSub(V3GetY(a), cy), FSub(V3GetX(b), cx));
		return FSub(t0, t1);
	}

#if defined(PX_X360) || defined(PX_PS3) || defined(__SPU__)

	//normal is in vertex space, need to transform the vertex space
	PX_FORCE_INLINE PxI32 getPolygonIndex(const Gu::PolygonalData& polyData, SupportLocal* map, const Ps::aos::Vec3VArg normal)
	{
		using namespace Ps::aos;

		//normal is in shape space, need to transform the vertex space
		const Vec3V n = M33TrnspsMulV3(map->vertex2Shape, normal);
		const Vec3V nnormal = V3Neg(n);
		const Vec3V planeN =  V3LoadU(polyData.mPolygons[0].mPlane.n);
		FloatV minProj = V3Dot(n, planeN);
	
		const VecI32V vOne = VecI32V_One();  
		const VecI32V vZero = VecI32V_Zero();
		const FloatV zero = FZero();
		//get incident face
		VecI32V ind = vZero;
		VecI32V inc = vOne;
		for(PxU32 i=1; i< polyData.mNbPolygons; ++i, inc = VecI32V_Add(inc, vOne))
		{
			Vec3V planeN = V3LoadU(polyData.mPolygons[i].mPlane.n);
			const FloatV proj = V3Dot(n, planeN);
			const BoolV res = FIsGrtr(minProj, proj);
			minProj = FSel(res, proj, minProj);
			ind = VecI32V_Sel(res, inc, ind);
		}
		PxI32 closestFaceIndex;
		PxI32_From_VecI32V(ind, (PxI32*)&closestFaceIndex);

		const PxU32 numEdges = polyData.mNbEdges;
		const PxU8* const edgeToFace = polyData.mFacesByEdges;

		//Loop through edges
		//PxU32 closestEdge = 0xffffffff;
		FloatV maxDpSq = FMul(minProj, minProj);
		ind = VecI32V_MinusOne();
		inc = vZero;
		for(PxU32 i=0; i < numEdges; ++i, inc = VecI32V_Add(inc, vOne))
		{
			const PxU32 index = i*2;
			const PxU8 f0 = edgeToFace[index];
			const PxU8 f1 = edgeToFace[index+1];

			const Vec3V planeNormal0 = V3LoadU(polyData.mPolygons[f0].mPlane.n);
			const Vec3V planeNormal1 = V3LoadU(polyData.mPolygons[f1].mPlane.n);

			// unnormalized edge normal
			const Vec3V edgeNormal = V3Add(planeNormal0, planeNormal1);//polys[f0].mPlane.n + polys[f1].mPlane.n;
			const FloatV enMagSq = V3Dot(edgeNormal, edgeNormal);//edgeNormal.magnitudeSquared();
			//Test normal of current edge - squared test is valid if dp and maxDp both >= 0
			const FloatV dp = V3Dot(edgeNormal, nnormal);//edgeNormal.dot(normal);
			const FloatV sqDp = FMul(dp, dp);
			const BoolV con0 = FIsGrtrOrEq(dp, zero);
			const BoolV con1 = FIsGrtr(sqDp, FMul(maxDpSq, enMagSq));
			const BoolV con = BAnd(con0, con1);
			maxDpSq = FSel(con, FDiv(sqDp, enMagSq), maxDpSq);
			ind = VecI32V_Sel(con, inc, ind);
		}

		PxI32 closestEdge;
		PxI32_From_VecI32V(ind, (PxI32*)&closestEdge);
		
		if(closestEdge!=-1)
		{
			const PxU8* FBE = edgeToFace;

			const PxU32 index = closestEdge*2;
			const PxU32 f0 = FBE[index];
			const PxU32 f1 = FBE[index+1];

			const Vec3V planeNormal0 = V3LoadU(polyData.mPolygons[f0].mPlane.n);
			const Vec3V planeNormal1 = V3LoadU(polyData.mPolygons[f1].mPlane.n);

			const FloatV dp0 = V3Dot(planeNormal0, nnormal);
			const FloatV dp1 = V3Dot(planeNormal1, nnormal);
			if(FAllGrtr(dp0, dp1))
			{
				closestFaceIndex = f0;
			}
			else
			{
				closestFaceIndex = f1;
			}
		}
		//return polyData.mPolygons[closestFaceIndex];
		return closestFaceIndex;

	}

	PX_FORCE_INLINE const Gu::HullPolygonData& getGJKPolygonIndex(const Gu::PolygonalData& polyData,  SupportLocal* map, const Ps::aos::Vec3VArg normal)
	{
		using namespace Ps::aos;

		//normal is in shape space, need to transform the vertex space
		const Vec3V n = M33TrnspsMulV3(map->vertex2Shape, normal);
		Vec3V planeN =  V3LoadU(polyData.mPolygons[0].mPlane.n);
		FloatV minProj = V3Dot(n, planeN);
		//incidentIndex = 0;

		const VecI32V vOne = VecI32V_One();
		//get incident face
		VecI32V ind = VecI32V_Zero();
		VecI32V inc = vOne;
		for(PxU32 i=1; i< polyData.mNbPolygons; ++i, inc = VecI32V_Add(inc, vOne))
		{
			Vec3V planeN = V3LoadU(polyData.mPolygons[i].mPlane.n);
			const FloatV proj = V3Dot(n, planeN);
			const BoolV res = FIsGrtr(minProj, proj);
			minProj = FSel(res, proj, minProj);
			ind = VecI32V_Sel(res, inc, ind);
		}
		PxI32 closestFaceIndex;
		PxI32_From_VecI32V(ind, (PxI32*)&closestFaceIndex);

		return polyData.mPolygons[closestFaceIndex];
	}

#else
	PX_FORCE_INLINE PxI32 getPolygonIndex(const Gu::PolygonalData& polyData, SupportLocal* map, const Ps::aos::Vec3VArg normal)
	{
		using namespace Ps::aos;

		//normal is in shape space, need to transform the vertex space
		const Vec3V n = M33TrnspsMulV3(map->vertex2Shape, normal);
		const Vec3V nnormal = V3Neg(n);
		const Vec3V planeN =  V3LoadU(polyData.mPolygons[0].mPlane.n);
		FloatV minProj = V3Dot(n, planeN);

		const FloatV zero = FZero();
		const BoolV bTrue = BTTTT();
		PxI32 closestFaceIndex = 0;

		for(PxU32 i=1; i< polyData.mNbPolygons; ++i)
		{
			Vec3V planeN = V3LoadU(polyData.mPolygons[i].mPlane.n);
			const FloatV proj = V3Dot(n, planeN);
			if(FAllGrtr(minProj, proj))
			{
				minProj = proj;
				closestFaceIndex = i;
			}
		}

		const PxU32 numEdges = polyData.mNbEdges;
		const PxU8* const edgeToFace = polyData.mFacesByEdges;

		//Loop through edges
		PxU32 closestEdge = 0xffffffff;
		FloatV maxDpSq = FMul(minProj, minProj);

		for(PxU32 i=0; i < numEdges; ++i)//, inc = VecI32V_Add(inc, vOne))
		{
			const PxU32 index = i*2;
			const PxU8 f0 = edgeToFace[index];
			const PxU8 f1 = edgeToFace[index+1];

			const Vec3V planeNormal0 = V3LoadU(polyData.mPolygons[f0].mPlane.n);
			const Vec3V planeNormal1 = V3LoadU(polyData.mPolygons[f1].mPlane.n);

			// unnormalized edge normal
			const Vec3V edgeNormal = V3Add(planeNormal0, planeNormal1);//polys[f0].mPlane.n + polys[f1].mPlane.n;
			const FloatV enMagSq = V3Dot(edgeNormal, edgeNormal);//edgeNormal.magnitudeSquared();
			//Test normal of current edge - squared test is valid if dp and maxDp both >= 0
			const FloatV dp = V3Dot(edgeNormal, nnormal);//edgeNormal.dot(normal);
			const FloatV sqDp = FMul(dp, dp);
			/*if(FAllGrtrOrEq(dp, zero) && FAllGrtr(sqDp, FMul(maxDpSq, enMagSq)))
			{
				maxDpSq = FDiv(sqDp, enMagSq);
				closestEdge = i;
			}*/
			const BoolV con0 = FIsGrtrOrEq(dp, zero);
			const BoolV con1 = FIsGrtr(sqDp, FMul(maxDpSq, enMagSq));
			const BoolV con = BAnd(con0, con1);
			if(BAllEq(con, bTrue))
			{
				maxDpSq = FDiv(sqDp, enMagSq);
				closestEdge = i;
			}
		}

		if(closestEdge!=-1)
		{
			const PxU8* FBE = edgeToFace;

			const PxU32 index = closestEdge*2;
			const PxU32 f0 = FBE[index];
			const PxU32 f1 = FBE[index+1];

			const Vec3V planeNormal0 = V3LoadU(polyData.mPolygons[f0].mPlane.n);
			const Vec3V planeNormal1 = V3LoadU(polyData.mPolygons[f1].mPlane.n);

			const FloatV dp0 = V3Dot(planeNormal0, nnormal);
			const FloatV dp1 = V3Dot(planeNormal1, nnormal);
			if(FAllGrtr(dp0, dp1))
			{
				closestFaceIndex = f0;
			}
			else
			{
				closestFaceIndex = f1;
			}
		}

		return closestFaceIndex;

	}

	PX_FORCE_INLINE const Gu::HullPolygonData& getGJKPolygonIndex(const Gu::PolygonalData& polyData,  SupportLocal* map, const Ps::aos::Vec3VArg normal)
	{
		using namespace Ps::aos;

		//normal is in shape space, need to transform the vertex space
		const Vec3V n = M33TrnspsMulV3(map->vertex2Shape, normal);
		Vec3V planeN =  V3LoadU(polyData.mPolygons[0].mPlane.n);
		FloatV minProj = V3Dot(n, planeN);

		PxU32 closestFaceIndex = 0;

		for(PxU32 i=1; i< polyData.mNbPolygons; ++i)
		{
			Vec3V planeN = V3LoadU(polyData.mPolygons[i].mPlane.n);
			const FloatV proj = V3Dot(n, planeN);
			if(FAllGrtr(minProj, proj))
			{
				minProj = proj;
				closestFaceIndex = i;
			}
		}

		return polyData.mPolygons[closestFaceIndex];
	}
#endif	
}
}

#endif