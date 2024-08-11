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

#ifndef GU_PROJECTION_H
#define GU_PROJECTION_H

#include "PxVec3.h"

namespace physx
{
namespace Gu
{

using namespace Ps::aos;

// A bunch of function for quickly projecting a set of points along a direction and 
// finding the minimum and maximum etc.

#ifndef PX_VMX

class Projection
{
public: 

	// PX_CONVEX_HULL_GAUSMAP_VERTEX_LIMIT is the (approximate) number of vertices for which it is faster to use brute force projection
	// rather than something smarter(eg hill climbing).

	static PX_FORCE_INLINE void pointsOnDir(const PxVec3& dir, const PxVec3* verts, PxU32 numVerts, PxReal& _min, PxReal& _max)
	{
		// PT: prevents aliasing
		PxReal minimum = PX_MAX_REAL;
		PxReal maximum = -PX_MAX_REAL;

		while(numVerts--)
		{
			const PxReal dp = (*verts++).dot(dir);
			minimum = physx::intrinsics::selectMin(minimum, dp);
			maximum = physx::intrinsics::selectMax(maximum, dp);
		}

		_min = minimum;
		_max = maximum;
	}

	//note: transform is the local=>world transform and dir is in world.
	// also adds the translation from transform
/*	static PX_FORCE_INLINE void pointsOnDir(const PxVec3& dir, const Cm::Matrix34& transform, 
		const PxVec3* PX_RESTRICT verts, PxU32 numVerts, PxReal& minimum, PxReal& maximum)
	{
		const PxVec3 localDir = transform.rotateTranspose(dir);

		pointsOnDir(localDir, verts, numVerts, minimum, maximum);

		const PxReal offset = transform.base3.dot(dir);
		minimum += offset;
		maximum += offset;

		PX_ASSERT(minimum <= maximum);
	}*/

/*	static PX_FORCE_INLINE void pointsOnDir(const PxVec3& dir, const PxVec3* verts, PxU32 numVerts, PxReal& _min, PxReal& _max, float offset)
	{
		// PT: prevents aliasing
		PxReal minimum = PX_MAX_REAL;
		PxReal maximum = -PX_MAX_REAL;

		while(numVerts--)
		{
			const PxReal dp = (*verts++).dot(dir);
			minimum = physx::intrinsics::selectMin(minimum, dp);
			maximum = physx::intrinsics::selectMax(maximum, dp);
		}

		_min = minimum + offset;
		_max = maximum + offset;
	}*/

	static PX_FORCE_INLINE PxU32 pointsOnDirMax(const PxVec3& dir, const PxVec3* verts, PxU32 numVerts)
	{
		PX_ASSERT(numVerts >= 1);

		PxReal bestValue = (*verts).dot(dir);
		PxU32 bestIndex = 0;

		for(PxU32 i = 1; i < numVerts; i++)
		{
			const PxReal dp = verts[i].dot(dir);
			if(dp > bestValue) 
			{
				bestValue = dp;
				bestIndex = i;
			}
		}

		return bestIndex;
	}

private:
	Projection(){}
};

#else

class Projection
{
public:

	// PX_CONVEX_HULL_GAUSMAP_VERTEX_LIMIT is the (approximate) number of vertices for which it is faster to use brute force projection
	// rather than something smarter(eg hill climbing).
	// Note: in terms of instructions we execute ~twice as many but it takes much less time due to
	// better pipelining. (at least up to ~100)

	//note: transform is the local=>world transform and dir is in world.
	// we need to do the rotation with simd to avoid LHSs
	// also adds the translation from transform
/*	static PX_FORCE_INLINE void pointsOnDir(const PxVec3& dir, const Cm::Matrix34& transform, 
		const PxVec3* PX_RESTRICT verts, PxU32 numVerts, PxReal& minimum, PxReal& maximum)
	{
		Cm::PxSimd::Vector4 dir4 = Cm::PxSimd::load(dir);
		Cm::PxSimd::Vector4 basis0 = Cm::PxSimd::load(transform.base0);
		Cm::PxSimd::Vector4 basis1 = Cm::PxSimd::load(transform.base1);
		Cm::PxSimd::Vector4 basis2 = Cm::PxSimd::load(transform.base2);
		Cm::PxSimd::Vector4 basis3 = Cm::PxSimd::load(transform.base3);

		Cm::PxSimd::Vector4 localDir4 = Cm::PxSimd::rotateInv(basis0, basis1, basis2, dir4);

		Cm::PxSimd::Vector4 min4, max4;

		pointsOnDir(localDir4, verts, numVerts, min4, max4);

		Cm::PxSimd::Vector4 offset4 = Cm::PxSimd::dot(basis3, dir4);
		min4 = Cm::PxSimd::add(min4, offset4);
		max4 = Cm::PxSimd::add(max4, offset4);

		Cm::PxSimd::store(minimum, min4);
		Cm::PxSimd::store(maximum, max4);

		PX_ASSERT(minimum <= maximum);
	}*/

	static PX_FORCE_INLINE void pointsOnDir(const Vec3V& dir4, const PxVec3* PX_RESTRICT verts, PxU32 numVerts, FloatV& min4, FloatV& max4)
	{
		//split to eliminate dependance within loop.
		FloatV min4_1 = FMax();
		FloatV max4_1 = FNegMax();
		FloatV min4_2 = min4_1;
		FloatV max4_2 = max4_1;
		FloatV min4_3 = min4_1;
		FloatV max4_3 = max4_1;
		FloatV min4_4 = min4_1;
		FloatV max4_4 = max4_1;

		const PxU32 numRolled = numVerts & 0x3;

		for(PxU32 i=0; i<numRolled; i++)
		{
			const FloatV dp = V3Dot(dir4, V3LoadU(*verts));

			min4_1 = FMin(min4_1, dp);
			max4_1 = FMax(max4_1, dp);

			verts++;
		}

		const PxU32 numUnRolled = numVerts & ~0x3;
		const PxVec3* PX_RESTRICT verts_end = verts + numUnRolled;

		while(verts<verts_end)
		{
			const Vec3V verts_0 = V3LoadU(verts[0]);
			const Vec3V verts_1 = V3LoadU(verts[1]);
			const Vec3V verts_2 = V3LoadU(verts[2]);
			const Vec3V verts_3 = V3LoadU(verts[3]);

			const FloatV dp_1 = V3Dot(dir4, verts_0);
			const FloatV dp_2 = V3Dot(dir4, verts_1);
			const FloatV dp_3 = V3Dot(dir4, verts_2);
			const FloatV dp_4 = V3Dot(dir4, verts_3);

			min4_1 = FMin(min4_1, dp_1);
			max4_1 = FMax(max4_1, dp_1);

			min4_2 = FMin(min4_2, dp_2);
			max4_2 = FMax(max4_2, dp_2);
			
			min4_3 = FMin(min4_3, dp_3);
			max4_3 = FMax(max4_3, dp_3);
			
			min4_4 = FMin(min4_4, dp_4);
			max4_4 = FMax(max4_4, dp_4);

			verts+=4;
		}

		min4_1 = FMin(min4_1, min4_2);
		min4_3 = FMin(min4_3, min4_4);
		min4 = FMin(min4_1, min4_3);

		max4_1 = FMax(max4_1, max4_2);
		max4_3 = FMax(max4_3, max4_4);
		max4 = FMax(max4_1, max4_3);
	}

	static PX_FORCE_INLINE void pointsOnDir(const PxVec3& dir, const PxVec3* PX_RESTRICT verts, PxU32 numVerts,
								PxReal& minimum, PxReal& maximum)
	{
		Vec3V dir4 = V3LoadU(dir);
		FloatV min4, max4;

		pointsOnDir(dir4, verts, numVerts, min4, max4);

		minimum=FStore(min4);
		maximum=FStore(max4);

		PX_ASSERT(minimum <= maximum);
	}

/*	static PX_FORCE_INLINE void pointsOnDir(const PxVec3& dir, const PxVec3* PX_RESTRICT verts, PxU32 numVerts,
								PxReal& minimum, PxReal& maximum, PxReal offset)
	{
		Cm::PxSimd::Vector4 dir4 = Cm::PxSimd::load(dir);
Cm::PxSimd::Vector4 offset4 = Cm::PxSimd::load(offset);
		Cm::PxSimd::Vector4 min4, max4;

		pointsOnDir(dir4, verts, numVerts, min4, max4);
min4 = Cm::PxSimd::add(min4, offset4);
max4 = Cm::PxSimd::add(max4, offset4);
		Cm::PxSimd::store(minimum, min4);
		Cm::PxSimd::store(maximum, max4);

		PX_ASSERT(minimum <= maximum);
	}*/

/*	static PX_FORCE_INLINE void pointsOnDirA(const Cm::PxSimd::Vector4& dir4, const PxVec3* PX_RESTRICT verts, PxU32 numVerts,
								Cm::PxSimd::Vector4& min4, Cm::PxSimd::Vector4& max4)
	{
		//check alignment
		PX_ASSERT( ((PxU32)verts & 0xf) == 0);

		//split to eliminate dependance within loop.
		Cm::PxSimd::Vector4 min4_1 = Cm::PxSimd::floatMax();
		Cm::PxSimd::Vector4 max4_1 = Cm::PxSimd::floatMin();
		Cm::PxSimd::Vector4 min4_2 = Cm::PxSimd::floatMax();
		Cm::PxSimd::Vector4 max4_2 = Cm::PxSimd::floatMin();
		Cm::PxSimd::Vector4 min4_3 = Cm::PxSimd::floatMax();
		Cm::PxSimd::Vector4 max4_3 = Cm::PxSimd::floatMin();
		Cm::PxSimd::Vector4 min4_4 = Cm::PxSimd::floatMax();
		Cm::PxSimd::Vector4 max4_4 = Cm::PxSimd::floatMin();

		const Cm::PxSimd::Vector4* PX_RESTRICT ptr = (Cm::PxSimd::Vector4* PX_RESTRICT)verts;
		const Cm::PxSimd::Vector4* PX_RESTRICT endPtr = (Cm::PxSimd::Vector4* PX_RESTRICT)(verts + (numVerts & ~0x3));
		
		while(ptr<endPtr)
		{
			Cm::PxSimd::Vector4 d1 = ptr[0];
			Cm::PxSimd::Vector4 d2 = ptr[1];
			Cm::PxSimd::Vector4 d3 = ptr[2];

			Cm::PxSimd::Vector4 v1, v2, v3, v4;
			
			Cm::PxSimd::unpack3to4(d1, d2, d3, v1, v2, v3, v4);

			Cm::PxSimd::Vector4 dp_1 = Cm::PxSimd::dot(dir4, v1);
			min4_1 = Cm::PxSimd::minimum(min4_1, dp_1);
			max4_1 = Cm::PxSimd::maximum(max4_1, dp_1);

			Cm::PxSimd::Vector4 dp_2 = Cm::PxSimd::dot(dir4, v2);
			min4_2 = Cm::PxSimd::minimum(min4_2, dp_2);
			max4_2 = Cm::PxSimd::maximum(max4_2, dp_2);

			Cm::PxSimd::Vector4 dp_3 = Cm::PxSimd::dot(dir4, v3);
			min4_3 = Cm::PxSimd::minimum(min4_3, dp_3);
			max4_3 = Cm::PxSimd::maximum(max4_3, dp_3);

			Cm::PxSimd::Vector4 dp_4 = Cm::PxSimd::dot(dir4, v4);
			min4_4 = Cm::PxSimd::minimum(min4_4, dp_4);
			max4_4 = Cm::PxSimd::maximum(max4_4, dp_4);

			ptr+=3;
		}

		const PxVec3* PX_RESTRICT vertsPtr = verts + (numVerts & ~0x3);
		const PxVec3* PX_RESTRICT vertsPtrEnd = verts + numVerts;

		while(vertsPtr < vertsPtrEnd)
		{
			Cm::PxSimd::Vector4 dp = Cm::PxSimd::dot(dir4, Cm::PxSimd::load(*vertsPtr));

			min4_1 = Cm::PxSimd::minimum(min4_1, dp);
			max4_1 = Cm::PxSimd::maximum(max4_1, dp);

			vertsPtr++;
		}

		Cm::PxSimd::Vector4 tmp1 = Cm::PxSimd::minimum(min4_1, min4_2);
		Cm::PxSimd::Vector4 tmp2 = Cm::PxSimd::minimum(min4_3, min4_4);
		min4 = Cm::PxSimd::minimum(tmp1, tmp2);

		Cm::PxSimd::Vector4 tmp3 = Cm::PxSimd::maximum(max4_1, max4_2);
		Cm::PxSimd::Vector4 tmp4 = Cm::PxSimd::maximum(max4_3, max4_4);
		max4 = Cm::PxSimd::maximum(tmp3, tmp4);
	}*/

	static PX_FORCE_INLINE PxU32 pointsOnDirMax(const PxVec3& dir, const PxVec3* PX_RESTRICT verts, PxU32 numVerts)
	{
		PX_ASSERT(numVerts >= 1);

		/*
		PxReal bestValue = (*verts).dot(dir);
		PxU32 bestIndex = 0;
		*/

		Vec3V dir4 = V3LoadU(dir);
		FloatV bestValue4 = V3Dot(dir4,V3LoadU(*verts));
		VecI32V bestIndex4 = VecI32V_Zero();
		PX_ALIGN(16, PxI32 bestIndex[4]);
		verts++;

		/*for (PxU32 i = 1; i < numVerts; i++)
		{
			PxReal dp = verts[i].dot(dir);
			if(dp > bestValue) 
			{
				bestValue = dp;
				bestIndex = i;
			}
		}*/
		const VecI32V iOne = I4Load(1);
		VecI32V i4 = iOne;
		for (PxU32 i = 1; i < numVerts; i++)
		{
			FloatV dp = V3Dot(dir4,V3LoadU(*verts));
			BoolV mask = FIsGrtr(dp,bestValue4);
			bestValue4 = FSel(mask, dp, bestValue4);
			bestIndex4 = V4I32Sel(mask, i4, bestIndex4);
			verts++;
			i4 = VecI32V_Add(i4, iOne);
		}

		/*maxProjection = bestValue;
		return bestIndex;*/

		I4StoreA(bestIndex4, bestIndex);

		return (PxU32)bestIndex[0];
	}

private:
	Projection(){}
};

#endif

}
}

#endif
