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

#ifndef OPC_OBBCOLLIDER_H
#define OPC_OBBCOLLIDER_H

#include "OPC_VolumeCollider.h"
#include "OPC_MeshInterface.h"
#include "PsVecMath.h"
#include "PxPhysXCommonConfig.h"
#include "Opcode.h"

namespace physx
{

namespace Cm
{
class Matrix34;
}

using namespace Ps::aos;

namespace Gu
{
	class OBBCollider : public VolumeCollider
	{
		public:
		PX_FORCE_INLINE						OBBCollider()	{}
		PX_FORCE_INLINE						~OBBCollider()	{}

		protected:
		// Precomputed data
							PxMat33			mAR;				//!< Absolute rotation matrix
							PxMat33			mRModelToBox;		//!< Rotation from model space to obb space
							PxMat33			mRBoxToModel;		//!< Rotation from obb space to model space
							PxVec3			mTModelToBox;		//!< Translation from model space to obb space
							PxVec3			mTBoxToModel;		//!< Translation from obb space to model space

							PxVec3			mBoxExtents;
							PxVec3			mB0;				//!< - mTModelToBox + mBoxExtents
							PxVec3			mB1;				//!< - mTModelToBox - mBoxExtents

							float			mBBx1;
							float			mBBy1;
							float			mBBz1;

							float			mBB_1;
							float			mBB_2;
							float			mBB_3;
							float			mBB_4;
							float			mBB_5;
							float			mBB_6;
							float			mBB_7;
							float			mBB_8;
							float			mBB_9;

#ifdef OPC_SUPPORT_VMX128
							Vec3V mBBxyz1;

							Vec3V mBB_123;
							Vec3V mBB_456;
							Vec3V mBB_789;
#endif

		// Leaf description
							PxVec3			mLeafVerts[3];		//!< Triangle vertices
		// Settings
		// AP: removed mFullBoxBoxTest since it wasn't used anywhere and we should use a different approach here anyway
		// (see rtree obb collider). Disabled to reduce code size.
		//					bool			mFullBoxBoxTest;	//!< Perform full BV-BV tests (true) or SAT-lite tests (false)

	public:

			// Overlap tests
#ifdef OPC_SUPPORT_VMX128
		PX_INLINE				Ps::IntBool		BoxBoxOverlap(const Vec3V extents, const Vec3V center) const;

		PX_INLINE				Ps::IntBool		BoxBoxOverlap(const Vec3V& extents, const Vec3V& center,
														  const Vec3V& TBoxToModel, const Vec3V& BB,
														  const Vec3V& rBoxToModel_0, const Vec3V& rBoxToModel_1, const Vec3V& rBoxToModel_2,
														  const Vec3V& ar_0, const Vec3V& ar_1, const Vec3V& ar_2,
														  const Vec3V& thisExtents,
														  const Vec3V& BB_123,	const Vec3V& BB_456, const Vec3V& BB_789) const;
		PX_FORCE_INLINE			Ps::IntBool		TriBoxOverlap(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2,
															  const Vec3V& tran, const Vec3V& extents) const;
		PX_FORCE_INLINE			Ps::IntBool		TriBoxOverlap(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2,
															  const Vec3V& rot_0, const Vec3V& rot_1, const Vec3V& rot2, 
															  const Vec3V& tran, const Vec3V& extents) const;
#endif
		PX_INLINE				Ps::IntBool		BoxBoxOverlap(const PxVec3& extents, const PxVec3& center) const;
		PX_FORCE_INLINE			Ps::IntBool		TriBoxOverlap(const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2) const;

		// AP: template - trick to shrink SPU code size.. todo: remove this code once we switch to rtrees and free up the space
		template<int nullWorldBM>
		PX_PHYSX_COMMON_API		Ps::IntBool		InitQuery(const Gu::Box& box, const Cm::Matrix34* worldb=NULL, const Cm::Matrix34* worldm=NULL);
							//Perform additional setup for obb-obb tests
		// changed to inline to solve linker error on Wii U (else, we would need to introduce an explicit far call, if that is even possible)
		// TODO: test if later compiler version can handle this
		PX_INLINE				void			InitTraversal();
	};

	PX_INLINE void OBBCollider::InitTraversal()
	{
		// Now we can precompute box-box data

		// Precompute absolute box-to-model rotation matrix
		for(PxU32 i=0;i<3;i++)
		{
			// Epsilon value prevents floating-point inaccuracies (strategy borrowed from RAPID)
			mAR[i][0] = 1e-6f + PxAbs(mRBoxToModel[i][0]);
			mAR[i][1] = 1e-6f + PxAbs(mRBoxToModel[i][1]);
			mAR[i][2] = 1e-6f + PxAbs(mRBoxToModel[i][2]);
		}

		// Precompute box-box data - Courtesy of Erwin de Vries
		mBBx1 = mBoxExtents.x*mAR[0][0] + mBoxExtents.y*mAR[1][0] + mBoxExtents.z*mAR[2][0];
		mBBy1 = mBoxExtents.x*mAR[0][1] + mBoxExtents.y*mAR[1][1] + mBoxExtents.z*mAR[2][1];
		mBBz1 = mBoxExtents.x*mAR[0][2] + mBoxExtents.y*mAR[1][2] + mBoxExtents.z*mAR[2][2];

		mBB_1 = mBoxExtents.y*mAR[2][0] + mBoxExtents.z*mAR[1][0];
		mBB_2 = mBoxExtents.x*mAR[2][0] + mBoxExtents.z*mAR[0][0];
		mBB_3 = mBoxExtents.x*mAR[1][0] + mBoxExtents.y*mAR[0][0];
		mBB_4 = mBoxExtents.y*mAR[2][1] + mBoxExtents.z*mAR[1][1];
		mBB_5 = mBoxExtents.x*mAR[2][1] + mBoxExtents.z*mAR[0][1];
		mBB_6 = mBoxExtents.x*mAR[1][1] + mBoxExtents.y*mAR[0][1];
		mBB_7 = mBoxExtents.y*mAR[2][2] + mBoxExtents.z*mAR[1][2];
		mBB_8 = mBoxExtents.x*mAR[2][2] + mBoxExtents.z*mAR[0][2];
		mBB_9 = mBoxExtents.x*mAR[1][2] + mBoxExtents.y*mAR[0][2];

		// Precompute bounds for box-in-box test
		mB0 = mBoxExtents - mTModelToBox;
		mB1 = - mBoxExtents - mTModelToBox;

	#ifdef OPC_SUPPORT_VMX128

		//mBBxyz1.x = mBBx1; mBBxyz1.y = mBBy1; mBBxyz1.z = mBBz1;
		PxVec3 xyz(mBBx1, mBBy1, mBBz1);
		mBBxyz1 = V3LoadU(xyz);

		//mBB_123.x = mBB_1; mBB_123.y = mBB_2; mBB_123.z = mBB_3;
		//mBB_456.x = mBB_4; mBB_456.y = mBB_5; mBB_456.z = mBB_6;
		//mBB_789.x = mBB_7; mBB_789.y = mBB_8; mBB_789.z = mBB_9;
		PxVec3 v123(mBB_1, mBB_2, mBB_3);
		PxVec3 v456(mBB_4, mBB_5, mBB_6);
		PxVec3 v789(mBB_7, mBB_8, mBB_9);
		mBB_123 = V3LoadU(v123);
		mBB_456 = V3LoadU(v456);
		mBB_789 = V3LoadU(v789);

	#endif
	}


	class HybridOBBCollider : public OBBCollider
	{
	public:
		// AP: CompactVersion in template - a trick to shrink SPU code size.. controls other template expansion later on
		// if set to true it also ignores worldb, worldm params (as if they were NULL), so beware - the functionality is NOT the same
		template <int DoPrimTests, int ReportVerts, int CompactVersion>
		PX_PHYSX_COMMON_API void Collide(
			const Gu::Box& box, const RTreeMidphaseData& model, VolumeColliderTrigCallback* callback,
			const Cm::Matrix34* worldb=NULL, const Cm::Matrix34* worldm=NULL); // was reportVerts = true
	};

} // namespace Gu

}

#endif // OPC_OBBCOLLIDER_H
