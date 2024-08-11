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

#ifndef GU_GJK_PENETRATION_H
#define GU_GJK_PENETRATION_H


#include "GuConvexSupportTable.h"
#include "GuGJKSimplex.h"
#include "GuVecShrunkConvexHullNoScale.h"
#include "GuVecConvexHullNoScale.h"
#include "GuGJKUtil.h"
#include "PsUtilities.h"

namespace physx
{
namespace Gu
{

	class ConvexV;


#ifndef	__SPU__


	//relative space, warm start
	template<class ConvexA, class ConvexB>
	PX_GJK_FORCE_INLINE PxGJKStatus gjkRelativePenetration(const ConvexA& a, const ConvexB& b,  const Ps::aos::PsMatTransformV& aToB, const Ps::aos::FloatVArg _contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict aIndices, PxU8* __restrict bIndices, PxU8& _size)
	{
		//PIX_PROFILE_ZONE(GJKPenetration);
		using namespace Ps::aos;

	
		const FloatV zero = FZero();
		const Vec3V zeroV = V3Zero();
		const BoolV bTrue = BTTTT();
		const BoolV bFalse = BFFFF();
		const FloatV marginA = a.getMargin();
		const FloatV marginB = b.getMargin();
		
		//This might change
		FloatV contactDist = _contactDist;
		//ML: eps2 is square value of an epsilon value for: 
		//(1)two (shrunk)shapes overlap. GJK will terminate based on sq(v) < eps2 and indicate that two shapes are overlap.
		//(2)two (shrunk + margin)shapes sepapate. GJK will terminate based sq(v)- v.dot(w) < epsRel*sq(v)
		//we calculate the eps2 based on 10% of the minimum margin of two shapes
		const FloatV minMargin = FMin(a.getMinMargin(), b.getMinMargin());
		const FloatV _eps2 = FMul(minMargin, FLoad(0.1f));
		const FloatV eps2 = FMul(_eps2, _eps2);
		const FloatV eps = FEps();
		
		const FloatV sumMargin0 = FAdd(marginA, marginB);

		Vec3V closA = zeroV;
		Vec3V closB = zeroV;
		FloatV sDist = FMax();
		FloatV minDist= sDist;
		Vec3V tempClosA = zeroV;
		Vec3V tempClosB = zeroV;

		BoolV bNotTerminated = bTrue;
		BoolV bCon = bTrue;
		Vec3V v;

		Vec3V Q[4];
		Vec3V A[4];
		Vec3V B[4];
		PxI32 aInd[4];
		PxI32 bInd[4];

		PxU32 size = 0;//_size;
		
		if(_size != 0)
		{
			Vec3V supportA = zeroV, supportB = zeroV, support = zeroV;
			Vec3V lastQ = zeroV, lastA = zeroV, lastB = zeroV;
			
			for(PxU32 i=0; i<_size; ++i)
			{
				aInd[i] = aIndices[i];
				bInd[i] = bIndices[i];
				supportA = aToB.transform(a.supportPoint(aIndices[i]));
				supportB = b.supportPoint(bIndices[i]);
				support = V3Sub(supportA, supportB);

				//Get rid of the duplicate point
				BoolV match = bFalse;
				for(PxU32 na = 0; na < size; ++na)
				{
					Vec3V dif = V3Sub(Q[na], support);
					match = BOr(match, FIsGrtr(eps, V3Dot(dif, dif)));	
				}

				if(BAllEq(match, bFalse))
				{
					lastA = supportA;
					lastB = supportB;
					lastQ = support;
					A[size] = supportA;
					B[size] = supportB;
					Q[size++] = support;
				}
			}

			v = GJKCPairDoSimplex(Q, A, B, aInd, bInd, lastQ, lastA, lastB, size, closA, closB);

			sDist = V3Dot(v, v);
			minDist = sDist;
			tempClosA = closA;
			tempClosB = closB;

			
			bNotTerminated = FIsGrtr(sDist, eps2);
		}
		else
		{
			const Vec3V _initialSearchDir = aToB.p;//V3Sub(a.getCenter(), b.getCenter());
			v = V3Sel(FIsGrtr(V3Dot(_initialSearchDir, _initialSearchDir), zero), _initialSearchDir, V3UnitX());
		}
		
		//FloatV maxMarginDif0 = zero, maxMarginDif1 = zero;

		while(BAllEq(bNotTerminated, bTrue))
		{
			/*const BoolV bA = a.supportRelativeIndex(V3Neg(v), aToB, aInd[size]);
			const BoolV bB = b.supportLocalIndex(v, bInd[size]);*/

			minDist = sDist;
			tempClosA = closA;
			tempClosB = closB;
			//const FloatV tmp = FMul(sDist, sqMargin);//FMulAdd(sDist, sqMargin, eps3);

			const Vec3V nv = V3Neg(v);

			const Vec3V supportA=a.supportRelative(nv, aToB, A[size], aInd[size]);
			const Vec3V supportB=b.supportLocal(v, B[size], bInd[size]);

			

			/*const Vec3V supportA = a.supportMarginRelative(A[size], aToB, aInd[size], bA);
			const Vec3V supportB = b.supportMarginLocal(B[size], bInd[size], bB);
		*/
			//calculate the support point
			const Vec3V support = V3Sub(supportA, supportB);
			Q[size++]=support;

			PX_ASSERT(size <= 4);

			//maxMarginDif0 = FMax(a.getMarginDif(), maxMarginDif0);
			//maxMarginDif1 = FMax(b.getMarginDif(), maxMarginDif1);
			//const FloatV maxMarginDif = FAdd(maxMarginDif0, maxMarginDif1);
			const FloatV maxMarginDif = FMax(a.getMarginDif(), b.getMarginDif());
			contactDist = FSel(FIsGrtr(contactDist, maxMarginDif), contactDist, maxMarginDif); 
			const FloatV sumMargin = FAdd(sumMargin0, contactDist);
			const FloatV sqMargin = FMul(sumMargin, sumMargin);
			const FloatV tmp = FMul(sDist, sqMargin);//FMulAdd(sDist, sqMargin, eps3);

			const FloatV vw = V3Dot(v, support);
			const FloatV sqVW = FMul(vw, vw);

			
			const BoolV bTmp1 = FIsGrtr(vw, zero);
			const BoolV bTmp2 = FIsGrtr(sqVW, tmp);
			BoolV con = BAnd(bTmp1, bTmp2);//this is the non intersect condition


			const FloatV tmp1 = FSub(sDist, vw);
			const FloatV tmp2 = FMul(eps2, sDist);
			const BoolV conGrtr = FIsGrtrOrEq(tmp2, tmp1);//this is the margin intersect condition

			const BoolV conOrconGrtr(BOr(con, conGrtr));

			if(BAllEq(conOrconGrtr, bTrue))
			{
				if(aIndices)
				{
					PX_ASSERT(bIndices);
					const PxU32 f = size - 1;
					_size = Ps::to8(f);
					for(PxU32 i=0; i<f; ++i)
					{
						aIndices[i] = Ps::to8(aInd[i]);
						bIndices[i] = Ps::to8(bInd[i]);
					}
				}
				//size--; if you want to get the correct size, this line need to be on
				if(BAllEq(con, bFalse)) //must be true otherwise we wouldn't be in here...
				{
					const FloatV dist = FSqrt(sDist);
					PX_ASSERT(FAllGrtr(dist, FEps()));
					const Vec3V n = V3ScaleInv(v, dist);//normalise
					contactA = V3NegScaleSub(n, marginA, closA);
					contactB = V3ScaleAdd(n, marginB, closB);
					penetrationDepth = FSub(dist, sumMargin0);
					normal = n; 
					return GJK_CONTACT;
					
				}
				else
				{ 
					return GJK_NON_INTERSECT;
				}
			}

			//calculate the closest point between two convex hull

			//v = GJKCPairDoSimplex(Q, A, B, size, closA, closB);
			v = GJKCPairDoSimplex(Q, A, B, aInd, bInd,  support, supportA, supportB, size, closA, closB);

			sDist = V3Dot(v, v);

			bCon = FIsGrtr(minDist, sDist);
			bNotTerminated = BAnd(FIsGrtr(sDist, eps2), bCon);
		}

		if(aIndices)
		{
			PX_ASSERT(bIndices);
			_size = Ps::to8(size);
			for(PxU32 i=0; i<size; ++i)
			{
				aIndices[i] = Ps::to8(aInd[i]);
				bIndices[i] = Ps::to8(bInd[i]);
			}
		}

		if(BAllEq(bCon, bFalse))
		{
			const FloatV sumMargin = FAdd(sumMargin0, contactDist);
			const FloatV sqMargin = FMul(sumMargin, sumMargin);
			
			//Reset back to older closest point
			closA = tempClosA;
			closB = tempClosB;
			sDist = minDist;
			v = V3Sub(closA, closB);
			const FloatV dist = FSqrt(sDist);
			PX_ASSERT(FAllGrtr(dist, FEps()));
			const Vec3V n = V3ScaleInv(v, dist);//normalise
			contactA = V3NegScaleSub(n, marginA, closA);
			contactB = V3ScaleAdd(n, marginB, closB);
			penetrationDepth = FSub(dist, sumMargin0);
			normal = n;
			if(FAllGrtrOrEq(sqMargin, sDist))
			{
				return GJK_CONTACT;    
			}
			return GJK_DEGENERATE; 
		}
		else 
		{		
			contactA = closA;
			contactB = closB;
			return EPA_CONTACT;     
		}
	}

	//ML: if we are using gjk local which means one of the object will be sphere/capsule, in that case, if we define takeCoreShape is true, we just need to return the closest point as the sphere center or a point in the capsule segment. This will increase the stability
	//for the manifold recycling code
	template<class ConvexA, class ConvexB>
	PX_GJK_FORCE_INLINE PxGJKStatus gjkLocalPenetration(const ConvexA& a, const ConvexB& b, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict aIndices, PxU8* __restrict bIndices, PxU8& _size, bool takeCoreShape)
	{
		//PIX_PROFILE_ZONE(GJKPenetration);
		using namespace Ps::aos;

	
		const FloatV zero = FZero();

		const FloatV marginA = a.getMargin();
		const FloatV marginB = b.getMargin();
		
	//	const FloatV minMargin = FMin(marginA, marginB);
		const FloatV minMargin = FMin(a.getMinMargin(), b.getMinMargin());

		//ML: eps2 is square value of an epsilon value for: 
		//(1)two (shrunk)shapes overlap. GJK will terminate based on sq(v) < eps2 and indicate that two shapes are overlap.
		//(2)two (shrunk + margin)shapes sepapate. GJK will terminate based sq(v)- v.dot(w) < epsRel*sq(v)
		//we calculate the eps2 based on 10% of the minimum margin of two shapes
		const FloatV _eps2 = FMul(minMargin, FLoad(0.1f));
		const FloatV eps2 = FMul(_eps2, _eps2);
		const FloatV eps = FEps();
		const Vec3V zeroV = V3Zero();
		const BoolV bTrue = BTTTT();
		const BoolV bFalse = BFFFF();
		//PxU32 size=0;
	
		const FloatV sumMargin0 = FAdd(marginA, marginB);
		const FloatV sumMargin = FAdd(sumMargin0, contactDist);
		const FloatV sqMargin = FMul(sumMargin, sumMargin);

		Vec3V closA = zeroV;
		Vec3V closB = zeroV;
		FloatV sDist = FMax();
		FloatV minDist= sDist;
		Vec3V tempClosA = zeroV;
		Vec3V tempClosB = zeroV;

		BoolV bNotTerminated = bTrue;
		BoolV bCon = bTrue;
		Vec3V v;

		Vec3V Q[4];
		Vec3V A[4];
		Vec3V B[4];
		PxI32 aInd[4];
		PxI32 bInd[4];
		Vec3V supportA = zeroV, supportB = zeroV, support=zeroV;

		PxU32 size = 0;//_size;
		
		if(_size != 0)
		{
			Vec3V lastQ = zeroV, lastA = zeroV, lastB = zeroV;
			for(PxU32 i=0; i<_size; ++i)
			{
				aInd[i] = aIndices[i];
				bInd[i] = bIndices[i];
				supportA = a.supportPoint(aIndices[i]);
				supportB = b.supportPoint(bIndices[i]);
				support = V3Sub(supportA, supportB);
				//Get rid of the duplicate point
				BoolV match = bFalse;
				for(PxU32 na = 0; na < size; ++na)
				{
					const Vec3V dif = V3Sub(Q[na], support);
					match = BOr(match, FIsGrtr(eps, V3Dot(dif, dif)));	
				}

				if(BAllEq(match, bFalse))
				{
					lastA = supportA;
					lastB = supportB;
					lastQ = support;
					A[size] = supportA;
					B[size] = supportB;
					Q[size++] = support;
				}
			}

			v = GJKCPairDoSimplex(Q, A, B, aInd, bInd, lastQ, lastA, lastB, size, closA, closB);

			sDist = V3Dot(v, v);
			minDist = sDist;
			
			bNotTerminated = FIsGrtr(sDist, eps2);
		}
		else
		{
			const Vec3V _initialSearchDir = V3Sub(a.getCenter(), b.getCenter());
			v = V3Sel(FIsGrtr(V3Dot(_initialSearchDir, _initialSearchDir), zero), _initialSearchDir, V3UnitX());
		}
		
		while(BAllEq(bNotTerminated, bTrue))
		{
			
			////ML: don't change the order, otherwise, we will get LHS penalty in Xbox
			//const BoolV bB = b.supportLocalIndex(v, bInd[size]);
			////const Vec3V nv = V3Neg(v);
			//const BoolV bA = a.supportLocalIndex(V3Neg(v), aInd[size]);

			minDist = sDist;
			tempClosA = closA;
			tempClosB = closB;
			const FloatV tmp = FMul(sDist, sqMargin);//FMulAdd(sDist, sqMargin, eps3);
				
			
			/*supportA=a.supportMarginLocal(A[size], aInd[size], bA);
			supportB=b.supportMarginLocal(B[size], bInd[size], bB);*/

			supportA=a.supportLocal(V3Neg(v), A[size], aInd[size]);
			supportB=b.supportLocal(v, B[size], bInd[size]);
			
		
			//calculate the support point
			support = V3Sub(supportA, supportB);
			Q[size++]=support;

			PX_ASSERT(size <= 4);

			const FloatV vw = V3Dot(v, support);
			const FloatV sqVW = FMul(vw, vw);

			
			const BoolV bTmp1 = FIsGrtr(vw, zero);
			const BoolV bTmp2 = FIsGrtr(sqVW, tmp);
			BoolV con = BAnd(bTmp1, bTmp2);//this is the non intersect condition


			const FloatV tmp1 = FSub(sDist, vw);
			const FloatV tmp2 = FMul(eps2, sDist);
			const BoolV conGrtr = FIsGrtrOrEq(tmp2, tmp1);//this is the margin intersect condition

			const BoolV conOrconGrtr(BOr(con, conGrtr));

			if(BAllEq(conOrconGrtr, bTrue))
			{
				if(aIndices)
				{
					PX_ASSERT(bIndices);
					const PxU32 f = size - 1;
					_size = Ps::to8(f);
					for(PxU32 i=0; i<f; ++i)
					{
						aIndices[i] = Ps::to8(aInd[i]);
						bIndices[i] = Ps::to8(bInd[i]);
					}
				}
				//size--; if you want to get the correct size, this line need to be on
				if(BAllEq(con, bFalse)) //must be true otherwise we wouldn't be in here...
				{
					const FloatV dist = FSqrt(sDist);
					PX_ASSERT(FAllGrtr(dist, FEps()));
					const Vec3V n = V3ScaleInv(v, dist);//normalise
					normal = n; 
					if(takeCoreShape)
					{
						contactA = closA;
						contactB = closB;
						penetrationDepth =dist;
					}
					else
					{
						contactA = V3NegScaleSub(n, marginA, closA);
						contactB = V3ScaleAdd(n, marginB, closB);
						penetrationDepth = FSub(dist, sumMargin0);
					}
					
					return GJK_CONTACT;
					
				}
				else
				{
					return GJK_NON_INTERSECT;
				}
			}

			//calculate the closest point between two convex hull

			//v = GJKCPairDoSimplex(Q, A, B, size, closA, closB);
			v = GJKCPairDoSimplex(Q, A, B, aInd, bInd, support, supportA, supportB,  size, closA, closB);

			sDist = V3Dot(v, v);

			bCon = FIsGrtr(minDist, sDist);
			bNotTerminated = BAnd(FIsGrtr(sDist, eps2), bCon);
		}
		
		if(aIndices)
		{
			PX_ASSERT(bIndices);
			_size = Ps::to8(size);
			for(PxU32 i=0; i<size; ++i)
			{
				aIndices[i] = Ps::to8(aInd[i]);
				bIndices[i] = Ps::to8(bInd[i]);
			}
		}

		if(BAllEq(bCon, bFalse))
		{
			//Reset back to older closest point
			closA = tempClosA;
			closB = tempClosB;
			sDist = minDist;
			v = V3Sub(closA, closB);
			const FloatV dist = FSqrt(sDist);
			PX_ASSERT(FAllGrtr(dist, FEps()));
			const Vec3V n = V3ScaleInv(v, dist);//normalise

			if(takeCoreShape)
			{
				contactA = closA;
				contactB = closB;
				penetrationDepth =dist;
			}
			else
			{
				contactA = V3NegScaleSub(n, marginA, closA);
				contactB = V3ScaleAdd(n, marginB, closB);
				penetrationDepth = FSub(dist, sumMargin0);
			}
			normal = n;
			if(FAllGrtrOrEq(sqMargin, sDist))
			{
				return GJK_CONTACT;  
			}
			return GJK_DEGENERATE;  
		}
		else 
		{

			contactA = closA;
			contactB = closB;

			return EPA_CONTACT;
			
		}
	}
 

#else


	PxGJKStatus gjkPenetration(const ConvexV& a, const ConvexV& b, GJKSupportMapPair* pair, const Ps::aos::Vec3VArg initialDir,  const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size, const bool useDegeneratedCase, const bool takeCoreShape);
	
	template<typename ConvexA, typename ConvexB>
	PxGJKStatus gjkRelativePenetration(const ConvexA& a, const ConvexB& b, const Ps::aos::PsMatTransformV& aToB, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth)
	{
		GJKSupportMapPairRelativeImpl<ConvexA, ConvexB> supportMap(a, b, aToB);
		return gjkPenetration(a, b,  &supportMap, aToB.p, contactDist, contactA, contactB, normal, penetrationDepth, NULL, NULL, 0, false, false);
	}


	template<class ConvexA, class ConvexB>
	PxGJKStatus gjkRelativePenetration(const ConvexA& a, const ConvexB& b, const Ps::aos::PsMatTransformV& aToB, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size)
	{
		GJKSupportMapPairRelativeImpl<ConvexA, ConvexB> supportMap(a, b, aToB);
		return gjkPenetration(a, b, &supportMap, aToB.p, contactDist, contactA, contactB, normal, penetrationDepth, aIndices, bIndices, _size, false, false);
	}


	template<class ConvexA, class ConvexB>
	PxGJKStatus gjkLocalPenetration(const ConvexA& a, const ConvexB& b, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size, const bool takeCoreShape)
	{
		using namespace Ps::aos;

		GJKSupportMapPairLocalImpl<ConvexA, ConvexB> supportMap(a, b);
		const Vec3V initialDir = V3Sub(a.getCenter(), b.getCenter());
		return gjkPenetration(a, b, &supportMap, initialDir, contactDist, contactA, contactB, normal, penetrationDepth, aIndices, bIndices, _size, true, takeCoreShape);
	}
	

#endif

}

}

#endif
