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

#ifndef OPC_AABBCOLLIDER_H
#define OPC_AABBCOLLIDER_H

#include "OPC_VolumeCollider.h"
#include "OPC_ModelData.h"
#include "PsVecMath.h"

namespace physx
{

using namespace Ps::aos;

namespace Gu
{
	class AABBCollider : public VolumeCollider
	{
		public:
		PX_FORCE_INLINE						AABBCollider()	{}
		PX_FORCE_INLINE						~AABBCollider()	{}

							CollisionAABB	mBox;			//!< Query box in (center, extents) form
							PxVec3			mMin;			//!< Query box minimum point
							PxVec3			mMax;			//!< Query box maximum point
		// Leaf description
							PxVec3			mLeafVerts[3];	//!< Triangle vertices
			// Overlap tests
		PX_INLINE			Ps::IntBool		AABBAABBOverlap(const PxVec3& b, const PxVec3& Pb);
		PX_INLINE			Ps::IntBool		AABBAABBOverlap(const Vec3V& extents, const Vec3V& center);
#ifdef OPC_SUPPORT_VMX128
		PX_INLINE			Ps::IntBool		TriBoxOverlap(const Vec3V& leafVerts0, const Vec3V& leafVerts1, const Vec3V& leafVerts2,
														  const Vec3V& center, const Vec3V& extents);
#endif
		PX_INLINE			Ps::IntBool		TriBoxOverlap(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2);
	};

	class HybridAABBCollider : public AABBCollider
	{
		public:
		PX_PHYSX_COMMON_API void Collide(
			const CollisionAABB& box,
			const RTreeMidphaseData& model, bool primTests,
			VolumeColliderTrigCallback* resultsCallback);
	};

} // namespace Gu

}

#include "OPC_AABBColliderOverlap.h"

#endif // OPC_AABBCOLLIDER_H
