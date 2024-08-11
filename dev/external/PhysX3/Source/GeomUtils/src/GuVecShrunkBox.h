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

#ifndef GU_VEC_SHRUNK_BOX_H
#define GU_VEC_SHRUNK_BOX_H

/** \addtogroup geomutils
@{
*/
#include "PxPhysXCommonConfig.h"
#include "GuVecConvex.h"
#include "PsVecTransform.h"
#include "GuConvexSupportTable.h"



namespace physx
{

namespace Gu
{

	/**
	\brief Represents an oriented bounding box. 

	As a center point, extents(radii) and a rotation. i.e. the center of the box is at the center point, 
	the box is rotated around this point with the rotation and it is 2*extents in width, height and depth.
	*/

	/**
	Box geometry

	The rot member describes the world space orientation of the box.
	The center member gives the world space position of the box.
	The extents give the local space coordinates of the box corner in the positive octant.
	Dimensions of the box are: 2*extent.
	Transformation to world space is: worldPoint = rot * localPoint + center
	Transformation to local space is: localPoint = T(rot) * (worldPoint - center)
	Where T(M) denotes the transpose of M.
	*/

	class ShrunkBoxV : public BoxV
	{
	public:

		/**
		\brief Constructor
		*/
		PX_INLINE ShrunkBoxV() : BoxV()
		{
		}

		/**
		\brief Constructor

		\param _center Center of the OBB
		\param _extents Extents/radii of the obb.
		\param _rot rotation to apply to the obb.
		*/

		PX_FORCE_INLINE ShrunkBoxV(const Ps::aos::Vec3VArg origin, const Ps::aos::Vec3VArg extent) : 
																				BoxV(origin, extent)
		{
		}
		
		/**
		\brief Destructor
		*/
		PX_INLINE ~ShrunkBoxV()
		{
		}

		//! Assignment operator
		PX_INLINE const ShrunkBoxV& operator=(const ShrunkBoxV& other)
		{
			rot		= other.rot;
			center	= other.center;
			extents	= other.extents;
			margin =  other.margin;
			minMargin = other.minMargin;
			return *this;
		}


		PX_FORCE_INLINE Ps::aos::Vec3V supportPoint(const PxI32 index)const
		{
			using namespace Ps::aos;
			const Vec3V _extents = V3Sub(extents,  V3Splat(margin));
			const BoolV con = boxVertexTable[index];
			return V3Sel(con, _extents, V3Neg(_extents));
		}  

		//local space point
		PX_FORCE_INLINE Ps::aos::Vec3V supportLocal(const Ps::aos::Vec3VArg dir,  Ps::aos::Vec3V& support, PxI32& index)const
		{
			using namespace Ps::aos;
		
			const Vec3V zero = V3Zero();
			const Vec3V _extents = V3Sub(extents,  V3Splat(margin));
			const BoolV comp = V3IsGrtr(dir, zero);
			getIndex(comp, index);
			const Vec3V originalP = V3Sel(comp, extents, V3Neg(extents));
			const Vec3V p = V3Sel(comp, _extents, V3Neg(_extents));
			const Vec3V v =V3Sub(originalP, p); 
			marginDif = V3Length(v);
			support = p;
			return p;
		}


		PX_FORCE_INLINE Ps::aos::Vec3V supportRelative(const Ps::aos::Vec3VArg dir, const Ps::aos::PsMatTransformV& aTob, Ps::aos::Vec3V& support, PxI32& index)const
		{
			using namespace Ps::aos;
		
			const Vec3V zero = V3Zero();
			const Vec3V _extents = V3Sub(extents,  Vec3V_From_FloatV(margin));
			//transfer dir into the local space of the box
			//const Vec3V _dir = M33TrnspsMulV3(rot, dir);
			const Vec3V _dir =aTob.rotateInv(dir);//relTra.rotateInv(dir);
			const BoolV comp = V3IsGrtr(_dir, zero);
			getIndex(comp, index);
			const Vec3V p = V3Sel(comp, _extents, V3Neg(_extents));
			const Vec3V originalP = V3Sel(comp, extents, V3Neg(extents));
			const Vec3V v =V3Sub(originalP, p); 
			marginDif = FSub(V3Length(v), margin);
			//transfer p into the world space
			const Vec3V ret = aTob.transform(p);//relTra.transform(p);//V3Add(center, M33MulV3(rot, p));
			support = ret;
			return ret;
		}

	};
}	

}

/** @} */
#endif
