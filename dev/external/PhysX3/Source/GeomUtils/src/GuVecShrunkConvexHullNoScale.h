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

#ifndef GU_VEC_SHRUNK_CONVEX_HULL_NO_SCALE_H
#define GU_VEC_SHRUNK_CONVEX_HULL_NO_SCALE_H

#include "GuVecShrunkConvexHull.h"

namespace physx
{
namespace Gu
{


	/*
		ML:
		ShrinkedConvexHull is used in GJK code but not EPA code
	*/
	class ShrunkConvexHullNoScaleV : public ShrunkConvexHullV
	{


		public:
		/**
		\brief Constructor
		*/
		PX_SUPPORT_INLINE ShrunkConvexHullNoScaleV(): ShrunkConvexHullV()
		{
		}

		PX_SUPPORT_INLINE ShrunkConvexHullNoScaleV(const Gu::ConvexHullData* _hullData, const Ps::aos::Vec3VArg _center, const Ps::aos::Vec3VArg scale, const Ps::aos::QuatVArg scaleRot):
													ShrunkConvexHullV(_hullData, _center, scale, scaleRot)
		{
		}


		PX_SUPPORT_INLINE ShrunkConvexHullNoScaleV(const Gu::ConvexHullData* _hullData, const Ps::aos::Vec3VArg _center, const Ps::aos::FloatVArg _margin, const Ps::aos::FloatVArg _minMargin, const Ps::aos::Vec3VArg scale, const Ps::aos::QuatVArg scaleRot) : 
													ShrunkConvexHullV(_hullData, _center, _margin, _minMargin, scale, scaleRot)
		{
		}

	
		PX_FORCE_INLINE Ps::aos::Vec3V supportPoint(const PxI32 index)const
		{
			return planeShift(index, margin);
		}

		//get the support point in vertex space
		PX_SUPPORT_INLINE Ps::aos::Vec3V planeShift(const PxU32 index, const Ps::aos::FloatVArg margin)const
		{
			using namespace Ps::aos;

			//calculate the support point for the core(shrunk) shape
			const PxU8* __restrict polyInds = hullData->getFacesByVertices8();

			const Vec3V p = V3LoadU(verts[index]);

			const PxU32 ind = index*3;

			const PxPlane& data1 = hullData->mPolygons[polyInds[ind]].mPlane;
			const PxPlane& data2 = hullData->mPolygons[polyInds[ind+1]].mPlane;
			const PxPlane& data3 = hullData->mPolygons[polyInds[ind+2]].mPlane;

			//transform the normal from vertex space to shape space
			//This is only required if the scale is not uniform
			const Vec3V n1 = V3Normalize(V3LoadU(data1.n));
			const Vec3V n2 = V3Normalize(V3LoadU(data2.n));
			const Vec3V n3 = V3Normalize(V3LoadU(data3.n));

			/*const Vec3V n1 = V3Normalize(M33TrnspsMulV3(vertex2Shape, Vec3V_From_PxVec3(data1.n)));
			const Vec3V n2 = V3Normalize(M33TrnspsMulV3(vertex2Shape, Vec3V_From_PxVec3(data2.n)));
			const Vec3V n3 = V3Normalize(M33TrnspsMulV3(vertex2Shape, Vec3V_From_PxVec3(data3.n)));*/
			//This is only required if the scale is not 1
			const FloatV d1 = FSub(margin, V3Dot(p, n1));
			const FloatV d2 = FSub(margin, V3Dot(p, n2));
			const FloatV d3 = FSub(margin, V3Dot(p, n3));


			//This is unavoidable unless we pre-calc the core shape
			const Vec3V intersectPoints = intersectPlanes(n1, d1, n2, d2, n3, d3);
			const Vec3V v =V3Sub(p, intersectPoints); 
			marginDif = V3Length(v);
			return intersectPoints;
		}

		//This function is used in gjk
		//dir in the shape space
		PX_SUPPORT_INLINE Ps::aos::Vec3V supportLocal(const Ps::aos::Vec3VArg dir, Ps::aos::Vec3V& support, PxI32& index)const
		{
			using namespace Ps::aos;
			//get the extreme point index
			const PxU32 maxIndex = supportVertexIndex(dir);
			index = maxIndex;
			//p is in the shape space
			const Vec3V p = planeShift(maxIndex, margin);
			support = p;
			return p;
		}

		//this is used in the sat test for the full contact gen
		PX_SUPPORT_INLINE void supportLocal(const Ps::aos::Vec3VArg dir, Ps::aos::FloatV& min, Ps::aos::FloatV& max)const
		{
			supportVertexMinMax(dir, min, max);
		}

		PX_SUPPORT_INLINE Ps::aos::Vec3V supportRelative(const Ps::aos::Vec3VArg dir, const Ps::aos::PsMatTransformV& aTob, Ps::aos::Vec3V& support, PxI32& index)const
		{
			using namespace Ps::aos;

			//transform dir from b space to the shape space of a space
			const Vec3V _dir = aTob.rotateInv(dir);//relTra.rotateInv(dir);//M33MulV3(skewInvRot, dir);
			const Vec3V p = supportLocal(_dir, support, index);
			//transfrom from a to b space
			const Vec3V ret = aTob.transform(p);
			support = ret;
			return ret;
		}
	};

#ifdef PX_PS3
	PX_FORCE_INLINE ShrunkConvexHullNoScaleV* PX_SCONVEX_TO_NOSCALECONVEX(ShrunkConvexHullV* x)
	{
		return physx::PxUnionCast<ShrunkConvexHullNoScaleV*, ShrunkConvexHullV*>(x);
	}
#else
	#define PX_SCONVEX_TO_NOSCALECONVEX(x)			((ShrunkConvexHullV*)(x))
#endif


}

}

#endif	// 
