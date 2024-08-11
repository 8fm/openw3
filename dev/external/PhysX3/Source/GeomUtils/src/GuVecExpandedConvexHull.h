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

#ifndef GU_VEC_EXPANDED_CONVEX_HULL_H
#define GU_VEC_EXPANDED_CONVEX_HULL_H

#include "PxPhysXCommonConfig.h"
#include "GuVecConvex.h"
#include "GuConvexMeshData.h"
#include "GuBigConvexData.h"
#include "GuConvexSupportTable.h"
#include "GuCubeIndex.h"
#include "GuVecConvexHull.h"
#include "PsFPU.h"

  
namespace physx
{
namespace Gu
{

	class ExpandedConvexHullV : public ConvexHullV
	{


		public:
		/**
		\brief Constructor
		*/
		PX_SUPPORT_INLINE ExpandedConvexHullV(): ConvexHullV()
		{
		}

		PX_SUPPORT_INLINE ExpandedConvexHullV(const Gu::ConvexHullData* _hullData, const Ps::aos::Vec3VArg _center, const Ps::aos::Vec3VArg scale, const Ps::aos::QuatVArg scaleRot):
													ConvexHullV(_hullData, _center, scale, scaleRot)
		{
		}


		PX_SUPPORT_INLINE ExpandedConvexHullV(const Gu::ConvexHullData* _hullData, const Ps::aos::Vec3VArg _center, const Ps::aos::FloatVArg _margin, const Ps::aos::FloatVArg _minMargin, const Ps::aos::Vec3VArg scale, const Ps::aos::QuatVArg scaleRot) : 
													ConvexHullV(_hullData, _center, _margin, _minMargin, scale, scaleRot)
		{
		}

	

		PX_FORCE_INLINE Ps::aos::Vec3V supportPoint(const PxI32 index)const
		{
			using namespace Ps::aos;
			return M33MulV3(vertex2Shape, V3LoadU(verts[index]));
		}

	
		//dir is in the shape space
		PX_SUPPORT_INLINE Ps::aos::Vec3V supportLocal(const Ps::aos::Vec3VArg dir)const
		{
			using namespace Ps::aos;
			//scale dir and put it in the vertex space
			const Vec3V n = V3Normalize(dir);
			const Vec3V _dir = M33TrnspsMulV3(vertex2Shape, dir);
			//const Vec3V maxPoint = supportVertex(_dir);
			const PxU32 maxIndex = supportVertexIndex(_dir);
			const Vec3V p=M33MulV3(vertex2Shape, V3LoadU(verts[maxIndex]));
			return V3ScaleAdd(n, margin, p);
		}
		

		PX_SUPPORT_INLINE Ps::aos::Vec3V supportRelative(const Ps::aos::Vec3VArg dir, const Ps::aos::PsMatTransformV& aTob)const
		{
			using namespace Ps::aos;
		
			//transform dir into the shape space
			const Vec3V _dir = aTob.rotateInv(dir);//relTra.rotateInv(dir);
			const Vec3V maxPoint =supportLocal(_dir);
			//translate maxPoint from shape space of a back to the b space
			return aTob.transform(maxPoint);//relTra.transform(maxPoint);
		}

		
		//dir is in the shape space
		PX_SUPPORT_INLINE Ps::aos::Vec3V supportLocal(const Ps::aos::Vec3VArg dir, Ps::aos::Vec3V& support, PxI32& index)const
		{
			using namespace Ps::aos;
			//scale dir and put it in the vertex space
			const Vec3V n = V3Normalize(dir);
			const Vec3V _dir = M33TrnspsMulV3(vertex2Shape, dir);
			//const Vec3V maxPoint = supportVertex(_dir);
			const PxU32 maxIndex = supportVertexIndex(_dir);
			index = maxIndex;

			//transfrom the vertex from vertex space to shape space
			const Vec3V pInShape=M33MulV3(vertex2Shape, V3LoadU(verts[maxIndex]));
			const Vec3V p=V3ScaleAdd(n, margin, pInShape);
			support = p;
			return p;
		}


		PX_SUPPORT_INLINE Ps::aos::Vec3V supportRelative(const Ps::aos::Vec3VArg dir, const Ps::aos::PsMatTransformV& aTob, Ps::aos::Vec3V& support, PxI32& index)const
		{
			using namespace Ps::aos;
		
			//transform dir into the shape space
			const Vec3V _dir = aTob.rotateInv(dir);//relTra.rotateInv(dir);

			const Vec3V maxPoint =supportLocal(_dir, support, index);

			//translate maxPoint from shape space of a back to the b space
			const Vec3V ret=aTob.transform(maxPoint);//relTra.transform(maxPoint);

			support = ret;
			return ret;
		}
	};


}

}

#endif	// 
