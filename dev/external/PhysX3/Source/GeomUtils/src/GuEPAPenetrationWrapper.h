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

#ifndef GU_EPA_PENETRATION_WRAPPER_H
#define GU_EPA_PENETRATION_WRAPPER_H

#include "PxPhysXCommonConfig.h"
#include "CmPhysXCommon.h"
#include "PsVecMath.h"
#include "PsVecTransform.h"
#include "GuGJKUtil.h"

/*
	This file is used to avoid the inner loop cross DLL calls
*/
namespace physx
{
namespace Gu
{
	
	class CapsuleV;
	class BoxV;
	class ConvexHullV;
	class ConvexHullNoScaleV;
	class TriangleV;
	/*class ExpandedBoxV;
	class ExpandedConvexHullV;*/

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	//													gjk/epa with contact dist
	//			
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	//---------------------------------------------------------------local space------------------------------------------------------------------------------------------------------------//
	/*	
		capsule vs others
	*/

	//b space, warm start
	PX_PHYSX_COMMON_API PxGJKStatus EPALocalPenetration(const CapsuleV& a, const BoxV& b, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size);

	PX_PHYSX_COMMON_API PxGJKStatus EPALocalPenetration(const CapsuleV& a, const ConvexHullV& b,  Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size);

	PX_PHYSX_COMMON_API PxGJKStatus EPALocalPenetration(const TriangleV& a, const BoxV& b, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size);

	PX_PHYSX_COMMON_API PxGJKStatus EPALocalPenetration(const TriangleV& a, const ConvexHullV& b,  Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size);
	

	//----------------------------------------------------------------relative space--------------------------------------------------------------------------------------------------------------//
	
	/*	
		box vs others warm start
	*/
	PX_PHYSX_COMMON_API PxGJKStatus EPARelativePenetration(const BoxV& a, const BoxV& b,  const Ps::aos::PsMatTransformV& aTob, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size);

	PX_PHYSX_COMMON_API PxGJKStatus EPARelativePenetration(const BoxV& a, const ConvexHullV& b,  const Ps::aos::PsMatTransformV& aTob, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size);

	/*PX_PHYSX_COMMON_API PxGJKStatus EPARelativePenetration(const ExpandedBoxV& a, const ExpandedConvexHullV& b,  const Ps::aos::PsMatTransformV& aTob, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndice, PxU8* __restrict bIndice, PxU8& _size);*/

	/*	
		convexhull vs others
	*/

	PX_PHYSX_COMMON_API PxGJKStatus EPARelativePenetration(const ConvexHullV& a, const ConvexHullV& b,  const Ps::aos::PsMatTransformV& aTob, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size);

	PX_PHYSX_COMMON_API PxGJKStatus EPARelativePenetration(const ConvexHullNoScaleV& a, const ConvexHullNoScaleV& b,  const Ps::aos::PsMatTransformV& aTob, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size);

	PX_PHYSX_COMMON_API PxGJKStatus EPARelativePenetration(const ConvexHullNoScaleV& a, const ConvexHullV& b,  const Ps::aos::PsMatTransformV& aTob, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size);

	PX_PHYSX_COMMON_API PxGJKStatus EPARelativePenetration(const ConvexHullV& a, const ConvexHullNoScaleV& b,  const Ps::aos::PsMatTransformV& aTob, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size);

	//PX_PHYSX_COMMON_API PxGJKStatus EPARelativePenetration(const ExpandedConvexHullV& a, const ExpandedConvexHullV& b, const Ps::aos::PsMatTransformV& aToB, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
	//	PxU8* __restrict  aIndice, PxU8* __restrict bIndice, PxU8& _size);

}
}

#endif  