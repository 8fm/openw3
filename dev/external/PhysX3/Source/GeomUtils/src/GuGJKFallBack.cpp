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


#include "PsAllocator.h"
#include "GuVecBox.h"
#include "GuVecCapsule.h"
#include "GuVecConvexHull.h"
#include "GuGJKSimplex.h"
#include "GuGJKFallBack.h"
#include "GuGJKSimplexTesselation.h"


namespace physx
{
namespace Gu
{
	struct Simplex
	{
		Ps::aos::Vec3V mA[4];
		Ps::aos::Vec3V mB[4];
		Ps::aos::Vec3V mQ[4];
		PxU32 mSize;
	};

	struct SimplexStack
	{
		static const PxU32 MaxStackSize = 64;
		Simplex mSimplices[MaxStackSize];
		PxU32 stackSize;

		SimplexStack() : stackSize(0)
		{
		}

		void pushSimplex(Ps::aos::Vec3V* Q, Ps::aos::Vec3V* A, Ps::aos::Vec3V* B, PxU32 size)
		{
			PX_ASSERT(size < MaxStackSize);
			for(PxU32 a = 0; a < size; ++a)
			{
				mSimplices[stackSize].mA[a] = A[a];
				mSimplices[stackSize].mB[a] = B[a];
				mSimplices[stackSize].mQ[a] = Q[a];
				mSimplices[stackSize].mSize = size;
			}
			stackSize++;
		}

		PxU32 popSimplex(Ps::aos::Vec3V* Q, Ps::aos::Vec3V* A, Ps::aos::Vec3V* B)
		{
			--stackSize;
			for(PxU32 a = 0; a < mSimplices[stackSize].mSize; ++a)
			{
				Q[a] = mSimplices[stackSize].mQ[a];
				A[a] = mSimplices[stackSize].mA[a];
				B[a] = mSimplices[stackSize].mB[a];
			}
			return mSimplices[stackSize].mSize;
		}

		PxU32 getSize()
		{
			return stackSize;
		}

	};


	Ps::aos::Vec3V doSupportMappingPair(const ConvexV&, const ConvexV&, SupportMap* map1, SupportMap* map2, /*const Ps::aos::PsMatTransformV& aToB, SupportLocal aSupportLocal, SupportLocal bSupportLocal,*/ const Ps::aos::Vec3V v, Ps::aos::Vec3V* Q, 
		Ps::aos::Vec3V* A, Ps::aos::Vec3V* B, PxU32 size, SimplexStack& stack)
	{
		using namespace Ps::aos;
		const FloatV fEps = FLoad(1e-2f);
		const Vec3V eps = Vec3V_From_FloatV(fEps);

		//const Vec3V negV = aToB.rotateInv(V3Neg(v));//relTra.rotateInv(dir);

		//const Vec3V supportA=aToB.transform(aSupportLocal(a, negV));
		//const Vec3V supportB=bSupportLocal(b, v);

		const Vec3V negV = V3Neg(v);
		Vec3V supportA = map1->doSupport(negV);
		Vec3V supportB = map2->doSupport(v);
		Vec3V support = V3Sub(supportA, supportB);


		PxU32 aDegen = FAllGrtr(fEps, V3ExtractMin(V3Abs(negV)));
		PxU32 bDegen = FAllGrtr(fEps, V3ExtractMin(V3Abs(v)));

		//Check for degenerates!!!!

		Vec3V degenA = V3Zero(), degenB = V3Zero();

		if(aDegen)
		{
			/*const BoolV isDegen = V3IsGrtr(eps, V3Abs(negV));
			const Vec3V degenerateDir = V3Sel(isDegen, V3Neg(negV), negV);
			degenA = aToB.transform(aSupportLocal(a, degenerateDir));*/

			const BoolV isDegen = V3IsGrtr(eps, V3Abs(negV));
			const Vec3V degenerateDir = V3Sel(isDegen, v, negV);
			degenA = map1->doSupport(degenerateDir);

			//degenA = aToB.transform(a.supportLocalDegenerate(negV));
		}

		if(bDegen)
		{
			/*const BoolV isDegen = V3IsGrtr(eps, V3Abs(v));
			const Vec3V degenerateDir = V3Sel(isDegen, V3Neg(v), v);
			degenB = bSupportLocal(b, degenerateDir);*/

			const BoolV isDegen = V3IsGrtr(eps, V3Abs(v));
			const Vec3V degenerateDir = V3Sel(isDegen, negV, v);
			degenB = map2->doSupport(degenerateDir);//bSupportLocal(b, degenerateDir);
			
		}

		if(aDegen && bDegen)
		{
			A[size] = degenA;
			B[size] = degenB;
			Q[size] = V3Sub(degenA, degenB);
			stack.pushSimplex(Q, A, B, size+1);
		}
		if(aDegen)
		{
			A[size] = degenA;
			B[size] = supportB;
			Q[size] = V3Sub(degenA, supportB);
			stack.pushSimplex(Q, A, B, size+1);
		}
		if(bDegen)
		{
			A[size] = supportA;
			B[size] = degenB;
			Q[size] = V3Sub(supportA, degenB);
			stack.pushSimplex(Q, A, B, size+1);
		}

		A[size] = supportA;
		B[size] = supportB;
		Q[size] = support;
		
		//calculate the support point
		return support;
	}

//	PxGJKStatus gjkRelativeFallbackWithStack(const ConvexV& a, const ConvexV& b, SupportMap* map1, SupportMap* map2, const Ps::aos::Vec3VArg initialDir, Ps::aos::Vec3V& closestA, Ps::aos::Vec3V& closestB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& sqDist)
//	{
//		using namespace Ps::aos;
//		Vec3V Q[4];
//		Vec3V A[4];
//		Vec3V B[4];
//
//		const Vec3V zeroV = V3Zero();
//		const FloatV zero = FZero();
//		const BoolV bTrue = BTTTT();
//	//	const BoolV bFalse = BFFFF();
//		PxU32 size=0;
//
//		//const Vec3V _initialSearchDir = V3Sub(a.getCenter(), b.getCenter());
//		Vec3V v = V3Sel(FIsGrtr(V3Dot(initialDir, initialDir), zero), initialDir, V3UnitX());
//
//		const FloatV minMargin = FMin(a.getMinMargin(), b.getMinMargin());
//		const FloatV eps2 = FMul(minMargin, FloatV_From_F32(0.01f));
//		const FloatV epsRel = FMul(eps2, eps2);
//
//		Vec3V closA(zeroV), closB(zeroV);
//		FloatV sDist = FMax();
//		FloatV minDist = sDist;
//		Vec3V closAA = zeroV;   
//		Vec3V closBB = zeroV;
//
//		
//		BoolV bNotTerminated = bTrue;
//		BoolV bCon = bTrue;
//		PxU32 degenerate = 2;
//	//	PxU32 invalidExit = 0;
//
//		SimplexStack stack;
//		PxU32 stackSize = 0;
//		
//		do
//		{
//			do
//			{
//				const BoolV bClosest = FIsGrtr(minDist, sDist);
//				minDist = FSel(bClosest, sDist, minDist);
//				closAA = V3Sel(bClosest, closA, closAA);
//				closBB = V3Sel(bClosest, closB, closBB);
//
//				//detect alternative choices here (say if we have multiple support points with similar/same projection)
//				//push these options onto stack but keep processing current path
//				
//
//				Vec3V support, supportA, supportB;
//				if(degenerate > 1)
//				{
//					support = doSupportMappingPair(a, b, map1, map2, v, Q, A, B, size, stack);
//					supportA = A[size];
//					supportB = B[size];
//					++size;
//				}
//				else
//				{
//					/*supportA=aSupportRelative(a, V3Neg(v), aToB);
//					supportB=bSupportLocal(b, v);*/
//					supportA = map1->doSupport(V3Neg(v));
//					supportB = map2->doSupport(v);
//					support = V3Sub(supportA, supportB);
//					PX_ASSERT(size < 4);
//					A[size]=supportA;
//					B[size]=supportB;
//					Q[size++]=support;
//
//				}
//
//			
//				
//				//calculate the support point
//				
//				const FloatV signDist = V3Dot(v, support);
//				const FloatV tmp0 = FSub(sDist, signDist);
//
//		
//				if(FAllGrtr(FMul(epsRel, sDist), tmp0))
//				{
//					const Vec3V n = V3Normalize(V3Sub(closB, closA));
//					closestA = closA;
//					closestB = closB;
//					sqDist = sDist;
//					normal = n;
//					return GJK_NON_INTERSECT;
//				}
//
//				//calculate the closest point between two convex hull
//				v = GJKCPairDoSimplexTesselation(Q, A, B, support, supportA, supportB, size, closA, closB);
//				sDist = V3Dot(v, v);
//				bCon = FIsGrtr(minDist, sDist);
//
//				bNotTerminated = BAnd(FIsGrtr(sDist, eps2), bCon);
//				degenerate++;
//			}while(BAllEq(bNotTerminated, bTrue));
//			//Now, we call support degenerate...
//
//			//exit if our results are acceptable. If not, continue in loop to 
//			//see if any alternative paths through GJK would have found the correct result
//			stackSize = stack.getSize();
//			if(!FAllGrtr(sDist, eps2))
//				stackSize = 0;
//			if(stackSize)
//			{
//				size = stack.popSimplex(Q, A, B);
//				--stackSize;
//				//calculate the closest point between two convex hull
//				v = GJKCPairDoSimplexTesselation(Q, A, B, Q[size-1], A[size-1], B[size-1], size, closA, closB);
//				sDist = V3Dot(v, v);
//				bCon = FIsGrtr(minDist, sDist);
//				bNotTerminated = FIsGrtr(sDist, eps2);
//				degenerate = 0;
//			}
//
//		}
//		while(BAllEq(bNotTerminated, bTrue));
//
//		closA = V3Sel(bCon, closA, closAA);
//		closB = V3Sel(bCon, closB, closBB);
//		closestA = closA;
//		closestB = closB;
//		normal = V3Normalize(V3Sub(closB, closA));
//		sqDist = FSel(bCon, sDist, minDist);
//#ifdef __SPU__
//		//PX_PRINTF("fallBackSqDist = %f\n", PxF32_From_FloatV(sqDist));
//#endif
//		return BAllEq(bCon, bTrue) == 1 ? GJK_CONTACT : GJK_DEGENERATE;
//	}  

	PxGJKStatus gjkRelativeFallback(const ConvexV& a, const ConvexV& b, SupportMap* map1, SupportMap* map2, const Ps::aos::Vec3VArg initialDir, Ps::aos::Vec3V& closestA, Ps::aos::Vec3V& closestB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& sqDist)
	{
		using namespace Ps::aos;
		Vec3V Q[4];
		Vec3V A[4];
		Vec3V B[4];

		const Vec3V zeroV = V3Zero();
		const FloatV zero = FZero();
		const BoolV bTrue = BTTTT();
		PxU32 size=0;

		Vec3V v = V3Sel(FIsGrtr(V3Dot(initialDir, initialDir), zero), initialDir, V3UnitX());

		const FloatV minMargin = FMin(a.getMinMargin(), b.getMinMargin());
		const FloatV eps2 = FMul(minMargin, FLoad(0.01f));
		//const FloatV epsRel = FMul(eps2, eps2);
		const FloatV epsRel = FLoad(0.000225f);//1.5%
		//const FloatV epsRel = FloatV_From_F32(0.0001f);
		//const FloatV epsRel = FloatV_From_F32(0.2f);

		Vec3V closA(zeroV), closB(zeroV);
		FloatV sDist = FMax();
		FloatV minDist = sDist;
		Vec3V closAA = zeroV;   
		Vec3V closBB = zeroV;

		
		BoolV bNotTerminated = bTrue;
		BoolV bCon = bTrue;
	
		do
		{
			minDist = sDist; 
			closAA = closA;
			closBB = closB;

			const Vec3V supportA = map1->doSupport(V3Neg(v));
			const Vec3V supportB = map2->doSupport(v);
			const Vec3V support = V3Sub(supportA, supportB);
			PX_ASSERT(size < 4);
			A[size]=supportA;
			B[size]=supportB;
			Q[size++]=support;

			
			//calculate the support point
			
			const FloatV signDist = V3Dot(v, support);
			const FloatV tmp0 = FSub(sDist, signDist);

	
			if(FAllGrtr(FMul(epsRel, sDist), tmp0))
			{
				const Vec3V n = V3Normalize(V3Sub(closB, closA));
				closestA = closA;
				closestB = closB;
				sqDist = sDist;
				normal = n;
				return GJK_NON_INTERSECT;
			}

			//calculate the closest point between two convex hull
			v = GJKCPairDoSimplexTesselation(Q, A, B, support, supportA, supportB, size, closA, closB);
			sDist = V3Dot(v, v);
			bCon = FIsGrtr(minDist, sDist);

			bNotTerminated = BAnd(FIsGrtr(sDist, eps2), bCon);

		}while(BAllEq(bNotTerminated, bTrue));

		closA = V3Sel(bCon, closA, closAA);
		closB = V3Sel(bCon, closB, closBB);
		closestA = closA;
		closestB = closB;
		normal = V3Normalize(V3Sub(closB, closA));
		sqDist = FSel(bCon, sDist, minDist);
		
		//PxGJKStatus status =  BAllEq(bCon, bTrue) == 1 ? GJK_CONTACT : GJK_DEGENERATE;
		//PX_PRINTF("GJK Tesselation status = %i, fallBackSqDist = %f\n", status, PxF32_From_FloatV(sqDist));
		//PX_PRINTF("--------------------------------------------------------------------------------------\n");

		return BAllEq(bCon, bTrue) == 1 ? GJK_CONTACT : GJK_DEGENERATE;
	}
}


}  