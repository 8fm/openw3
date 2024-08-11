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

//#include "GuVecSphere.h"
//#include "GuVecBox.h"
//#include "GuVecCapsule.h"
//#include "GuVecConvexHull.h"
//#include "GuVecCylinder.h"
//#include "GuVecCone.h"
////#include "GuVecSegment.h"
//#include "GuVecTriangle.h"

#include "GuGJKSimplex.h"

namespace physx
{
namespace Gu
{


	PX_NOALIAS Ps::aos::Vec3V closestPtPointTetrahedron(Ps::aos::Vec3V* PX_RESTRICT Q, Ps::aos::Vec3V* PX_RESTRICT A, Ps::aos::Vec3V* PX_RESTRICT B, PxU32& size, Ps::aos::Vec3V& closestA, Ps::aos::Vec3V& closestB)
	{
		using namespace Ps::aos;
		const FloatV zero = FZero();
		const Vec3V zeroV = V3Zero();
		const FloatV eps = FEps();
		PxU32 tempSize = size;
		FloatV tempT = zero;
		FloatV tempW = zero;
	
		FloatV bestSqDist = FMax();
		const Vec3V a = Q[0];
		const Vec3V b = Q[1];
		const Vec3V c = Q[2];  
		const Vec3V d = Q[3];
		const BoolV bTrue = BTTTT();
		const BoolV bFalse = BFFFF();

		//degenerated
		const Vec3V ad = V3Sub(d, a);
		const Vec3V bd = V3Sub(d, b);
		const Vec3V cd = V3Sub(d, c);
		const FloatV dad = V3Dot(ad, ad);
		const FloatV dbd = V3Dot(bd, bd);
		const FloatV dcd = V3Dot(cd, cd);
		const FloatV fMin = FMin(dad, FMin(dbd, dcd));
		if(FAllGrtr(eps, fMin))
		{
			size = 3;
			return closestPtPointTriangle(a, b, c, Q, A, B, size, closestA, closestB);
		}

		const BoolV bIsOutside4 = PointOutsideOfPlane4(a, b, c, d);

		if(BAllEq(bIsOutside4, bFalse))
		{
			//All inside
			return zeroV;
		}

		Vec3V _Q[] = {Q[0], Q[1], Q[2], Q[3]};
		Vec3V _A[] = {A[0], A[1], A[2], A[3]};
		Vec3V _B[] = {B[0], B[1], B[2], B[3]};

		PxU32 indices[3] = {0, 1, 2};

		Vec3V result = zeroV;
	
		if(BAllEq(BGetX(bIsOutside4), bTrue))
		{

			//PxU32 tempIndices[] = {0,1,2};
	
			PxU32 _size = 3;
			FloatV t, w;
			//result = closestPtPointTriangleBaryCentric(_Q[0], _Q[1], _Q[2], tempIndices, _size, t, w);
			result = closestPtPointTriangleBaryCentric(_Q[0], _Q[1], _Q[2], _size, t, w);

			const FloatV sqDist = V3Dot(result, result);
			//const BoolV con = FIsGrtr(bestSqDist, sqDist);
			bestSqDist = sqDist;
			
		/*	indices[0] = tempIndices[0];
			indices[1] = tempIndices[1];
			indices[2] = tempIndices[2];*/
			
			tempSize = _size;
			tempT = t;
			tempW = w;
		}

		if(BAllEq(BGetY(bIsOutside4), bTrue))
		{

			//PxU32 tempIndices[] = {0,2,3};

			PxU32 _size = 3;
			
			FloatV t, w;
			//const Vec3V q = closestPtPointTriangleBaryCentric(_Q[0], _Q[2], _Q[3], tempIndices, _size, t, w);
			const Vec3V q = closestPtPointTriangleBaryCentric(_Q[0], _Q[2], _Q[3],  _size, t, w);

			const FloatV sqDist = V3Dot(q, q);
			const BoolV con = FIsGrtr(bestSqDist, sqDist);
			if(BAllEq(con, bTrue))
			{
				result = q;
				bestSqDist = sqDist;
			/*	indices[0] = tempIndices[0];
				indices[1] = tempIndices[1];
				indices[2] = tempIndices[2];*/

				indices[0] = 0;
				indices[1] = 2;
				indices[2] = 3;

				tempSize = _size;
				tempT = t;
				tempW = w;
			}
		}

		if(BAllEq(BGetZ(bIsOutside4), bTrue))
		{
			
			//PxU32 tempIndices[] = {0,3,1};

			PxU32 _size = 3;
			FloatV t, w;
			//const Vec3V q = closestPtPointTriangleBaryCentric(_Q[0], _Q[3], _Q[1], tempIndices, _size, t, w);
			const Vec3V q = closestPtPointTriangleBaryCentric(_Q[0], _Q[3], _Q[1], _size, t, w);

			const FloatV sqDist = V3Dot(q, q);
			const BoolV con = FIsGrtr(bestSqDist, sqDist);
			if(BAllEq(con, bTrue))
			{
				result = q;
				bestSqDist = sqDist;

			/*	indices[0] = tempIndices[0];
				indices[1] = tempIndices[1];
				indices[2] = tempIndices[2];*/

				indices[0] = 0;
				indices[1] = 3;
				indices[2] = 1;


				tempSize = _size;
				tempT = t;
				tempW = w;
			}

		}

		if(BAllEq(BGetW(bIsOutside4), bTrue))
		{
	
			//PxU32 tempIndices[] = {1,3,2};

			PxU32 _size = 3;
			FloatV t, w;
			//const Vec3V q = closestPtPointTriangleBaryCentric(_Q[1], _Q[3], _Q[2], tempIndices, _size, t, w);
			const Vec3V q = closestPtPointTriangleBaryCentric(_Q[1], _Q[3], _Q[2], _size, t, w);

			const FloatV sqDist = V3Dot(q, q);
			const BoolV con = FIsGrtr(bestSqDist, sqDist);

			if(BAllEq(con, bTrue))
			{
				result = q;
				bestSqDist = sqDist;

				/*indices[0] = tempIndices[0];
				indices[1] = tempIndices[1];
				indices[2] = tempIndices[2];*/

				indices[0] = 1;
				indices[1] = 3;
				indices[2] = 2;

				tempSize = _size;
				tempT = t;
				tempW = w;
			}
		}

		A[0] = _A[indices[0]]; A[1] = _A[indices[1]]; A[2] = _A[indices[2]];
		B[0] = _B[indices[0]]; B[1] = _B[indices[1]]; B[2] = _B[indices[2]];
		Q[0] = _Q[indices[0]]; Q[1] = _Q[indices[1]]; Q[2] = _Q[indices[2]];

		const Vec3V a0 = V3Sub(_A[indices[1]], _A[indices[0]]);
		const Vec3V a1 = V3Sub(_A[indices[2]], _A[indices[0]]);

		const Vec3V b0 = V3Sub(_B[indices[1]], _B[indices[0]]);
		const Vec3V b1 = V3Sub(_B[indices[2]], _B[indices[0]]);

		closestA = V3Add( _A[indices[0]], V3ScaleAdd(a0, tempT, V3Scale(a1, tempW)));
		closestB = V3Add( _B[indices[0]], V3ScaleAdd(b0, tempT, V3Scale(b1, tempW)));

		size = tempSize;
		return result;
	}

	PX_NOALIAS Ps::aos::Vec3V closestPtPointTetrahedron(Ps::aos::Vec3V* PX_RESTRICT Q, Ps::aos::Vec3V* PX_RESTRICT A, Ps::aos::Vec3V* PX_RESTRICT B, PxI32* PX_RESTRICT aInd,  PxI32* PX_RESTRICT bInd, 
		const Ps::aos::Vec3VArg Q4, const Ps::aos::Vec3VArg A4, const Ps::aos::Vec3VArg B4, PxU32& size, Ps::aos::Vec3V& closestA, Ps::aos::Vec3V& closestB)
	{
		using namespace Ps::aos;
		const FloatV zero = FZero();
		const FloatV eps = FEps();
		const Vec3V zeroV = V3Zero();
		PxU32 tempSize = size;
		FloatV tempT = zero;
		FloatV tempW = zero;
	
		
		FloatV bestSqDist = FMax();
		const Vec3V a = Q[0];
		const Vec3V b = Q[1];
		const Vec3V c = Q[2];
		const Vec3V d = Q4;//Q[3];
		const BoolV bTrue = BTTTT();
		const BoolV bFalse = BFFFF();

		//degenerated
		const Vec3V ad = V3Sub(d, a);
		const Vec3V bd = V3Sub(d, b);
		const Vec3V cd = V3Sub(d, c);
		const FloatV dad = V3Dot(ad, ad);
		const FloatV dbd = V3Dot(bd, bd);
		const FloatV dcd = V3Dot(cd, cd);
		const FloatV fMin = FMin(dad, FMin(dbd, dcd));
		if(FAllGrtr(eps, fMin))
		{
			size = 3;
			return closestPtPointTriangle(a, b, c, Q, A, B, size, closestA, closestB);
		}

		BoolV bIsOutside4 = PointOutsideOfPlane4(a, b, c, d);

		if(BAllEq(bIsOutside4, bFalse))
		{
			//All inside
			return zeroV;
		}

		//bIsOutside4 = BTTTT();

		Vec3V _Q[] = {Q[0], Q[1], Q[2], Q4};
		Vec3V _A[] = {A[0], A[1], A[2], A4};
		Vec3V _B[] = {B[0], B[1], B[2], B4};
		PxI32 _aInd[] = {aInd[0], aInd[1], aInd[2], aInd[3]};
		PxI32 _bInd[] = {bInd[0], bInd[1], bInd[2], bInd[3]};

		PxU32 indices[3] = {0, 1, 2};

		Vec3V result = zeroV;
		FloatV t, w;

		if(BAllEq(BGetX(bIsOutside4), bTrue))
		{
			PxU32 _size = 3;
			result = closestPtPointTriangleBaryCentric(_Q[0], _Q[1], _Q[2], _size, t, w);
			bestSqDist = V3Dot(result, result);
			tempSize = _size;
			tempT = t;
			tempW = w;
		}

		if(BAllEq(BGetY(bIsOutside4), bTrue))
		{

			PxU32 _size = 3;
			const Vec3V q = closestPtPointTriangleBaryCentric(_Q[0], _Q[2], _Q[3], _size, t, w);

			const FloatV sqDist = V3Dot(q, q);
			const BoolV con = FIsGrtr(bestSqDist, sqDist);
			if(BAllEq(con, bTrue))
			{
				result = q;
				bestSqDist = sqDist;
				indices[0] = 0;
				indices[1] = 2;
				indices[2] = 3;

				tempSize = _size;
				tempT = t;
				tempW = w;
			}
		}

		if(BAllEq(BGetZ(bIsOutside4), bTrue))
		{

			PxU32 _size = 3;
			const Vec3V q = closestPtPointTriangleBaryCentric(_Q[0], _Q[3], _Q[1], _size, t, w);

			const FloatV sqDist = V3Dot(q, q);
			const BoolV con = FIsGrtr(bestSqDist, sqDist);
			if(BAllEq(con, bTrue))
			{
				result = q;
				bestSqDist = sqDist;
				indices[0] = 0;
				indices[1] = 3;
				indices[2] = 1;

				tempSize = _size;
				tempT = t;
				tempW = w;
			}

		}

		if(BAllEq(BGetW(bIsOutside4), bTrue))
		{
			PxU32 _size = 3;
			FloatV t, w;
			const Vec3V q = closestPtPointTriangleBaryCentric(_Q[1], _Q[3], _Q[2], _size, t, w);

			const FloatV sqDist = V3Dot(q, q);
			const BoolV con = FIsGrtr(bestSqDist, sqDist);

			if(BAllEq(con, bTrue))
			{
				result = q;
				bestSqDist = sqDist;

				indices[0] = 1;
				indices[1] = 3;
				indices[2] = 2;

				tempSize = _size;
				tempT = t;
				tempW = w;
			}
		}

		A[0] = _A[indices[0]]; A[1] = _A[indices[1]]; A[2] = _A[indices[2]];
		B[0] = _B[indices[0]]; B[1] = _B[indices[1]]; B[2] = _B[indices[2]];
		Q[0] = _Q[indices[0]]; Q[1] = _Q[indices[1]]; Q[2] = _Q[indices[2]];
		aInd[0] = _aInd[indices[0]]; aInd[1] = _aInd[indices[1]]; aInd[2] = _aInd[indices[2]];
		bInd[0] = _bInd[indices[0]]; bInd[1] = _bInd[indices[1]]; bInd[2] = _bInd[indices[2]];

		const Vec3V a0 = V3Sub(_A[indices[1]], _A[indices[0]]);
		const Vec3V a1 = V3Sub(_A[indices[2]], _A[indices[0]]);

		const Vec3V b0 = V3Sub(_B[indices[1]], _B[indices[0]]);
		const Vec3V b1 = V3Sub(_B[indices[2]], _B[indices[0]]);

		closestA = V3Add( _A[indices[0]], V3ScaleAdd(a0, tempT, V3Scale(a1, tempW)));
		closestB = V3Add( _B[indices[0]], V3ScaleAdd(b0, tempT, V3Scale(b1, tempW)));

		size = tempSize;
		return result;
	}
}

}
