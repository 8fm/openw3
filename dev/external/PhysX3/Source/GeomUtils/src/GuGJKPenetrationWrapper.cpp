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


#include "GuGJKPenetrationWrapper.h"
#include "GuVecSphere.h"
#include "GuVecCapsule.h"
#include "GuVecBox.h"
#include "GuVecShrunkBox.h"
#include "GuVecConvexHull.h"
#include "GuVecShrunkConvexHull.h"
#include "GuVecTriangle.h"
#include "GuGJKRaycast.h"
#include "GuGJKPenetration.h"

namespace physx
{
namespace Gu
{

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	//													gjk/epa with contact dist
	//			
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	//capsule vs other  

	//b space,  warm start
	PxGJKStatus GJKLocalPenetration(const CapsuleV& a, const BoxV& b, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size, const bool takeCoreShape)
	{
		return 	gjkLocalPenetration(a, b, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size, takeCoreShape);
	}  

	PxGJKStatus GJKLocalPenetration(const CapsuleV& a, const ConvexHullV& b, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size, const bool takeCoreShape)
	{
		return 	gjkLocalPenetration(a, b, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size, takeCoreShape);
	}

	PxGJKStatus GJKLocalPenetration(const CapsuleV& a, const ShrunkConvexHullV& b, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size, const bool takeCoreShape)
	{
		return 	gjkLocalPenetration(a, b, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size, takeCoreShape);
	}

	PxGJKStatus GJKLocalPenetration(const CapsuleV& a, const ConvexHullNoScaleV& b, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size, const bool takeCoreShape)
	{
		return 	gjkLocalPenetration(a, b, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size, takeCoreShape);
	}

	PxGJKStatus GJKLocalPenetration(const CapsuleV& a, const ShrunkConvexHullNoScaleV& b, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size, const bool takeCoreShape)
	{
		return 	gjkLocalPenetration(a, b, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size, takeCoreShape);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	PxGJKStatus GJKLocalPenetration(const TriangleV& a, const BoxV& b, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size, const bool takeCoreShape)
	{
		return 	gjkLocalPenetration(a, b, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size, takeCoreShape);
	}

	PxGJKStatus GJKLocalPenetration(const TriangleV& a, const TriangleV& b, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size, const bool takeCoreShape)
	{
		return 	gjkLocalPenetration(a, b, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size, takeCoreShape);
	}

	PxGJKStatus GJKLocalPenetration(const TriangleV& a, const ConvexHullV& b, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size, const bool takeCoreShape)
	{
		return 	gjkLocalPenetration(a, b, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size, takeCoreShape);
	}

	PxGJKStatus GJKLocalPenetration(const TriangleV& a, const ConvexHullNoScaleV& b, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices, PxU8& _size, const bool takeCoreShape)
	{
		return 	gjkLocalPenetration(a, b, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size, takeCoreShape);
	}

	//box vs other

	//relative space, warm start
	PxGJKStatus GJKRelativePenetration(const BoxV& a, const BoxV& b, const Ps::aos::PsMatTransformV& aTob,  const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices,PxU8& _size)
	{
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size);
	}


	PxGJKStatus GJKRelativePenetration(const BoxV& a, const ConvexHullV& b, const Ps::aos::PsMatTransformV& aTob,  const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices,PxU8& _size)
	{
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size);
	}

	PxGJKStatus GJKRelativePenetration(const ShrunkBoxV& a, const ShrunkBoxV& b, const Ps::aos::PsMatTransformV& aTob,  const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices,PxU8& _size)
	{
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size);
	}

	PxGJKStatus GJKRelativePenetration(const ShrunkBoxV& a, const ShrunkConvexHullV& b, const Ps::aos::PsMatTransformV& aTob,  const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices,PxU8& _size)
	{
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size);
	}

	PxGJKStatus GJKRelativePenetration(const ShrunkBoxV& a, const ShrunkConvexHullNoScaleV& b, const Ps::aos::PsMatTransformV& aTob,  const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices,PxU8& _size)
	{
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size);
	}


	/*
		convexHull vs others
	*/

	//relative space, warm start

	PxGJKStatus GJKRelativePenetration(const ConvexHullV& a, const ConvexHullV& b, const Ps::aos::PsMatTransformV& aTob,  const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices,PxU8& _size)
	{
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size);
	}

	PxGJKStatus GJKRelativePenetration(const ShrunkConvexHullV& a, const ShrunkConvexHullV& b, const Ps::aos::PsMatTransformV& aTob,  const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices,PxU8& _size)
	{
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size);
	}

	PxGJKStatus GJKRelativePenetration(const ShrunkConvexHullV& a, const ShrunkConvexHullNoScaleV& b, const Ps::aos::PsMatTransformV& aTob,  const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices,PxU8& _size)
	{
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size);
	}

	PxGJKStatus GJKRelativePenetration(const ShrunkConvexHullNoScaleV& a, const ShrunkConvexHullV& b, const Ps::aos::PsMatTransformV& aTob,  const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices,PxU8& _size)
	{
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size);
	}

	PxGJKStatus GJKRelativePenetration(const ShrunkConvexHullNoScaleV& a, const ShrunkConvexHullNoScaleV& b, const Ps::aos::PsMatTransformV& aTob,  const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth,
		PxU8* __restrict  aIndices, PxU8* __restrict bIndices,PxU8& _size)
	{
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, aIndices, bIndices, _size);
	}

	

	//triangle vs others, no warmstart
	PxGJKStatus GJKRelativePenetration(const TriangleV& a, const BoxV& b, const Ps::aos::PsMatTransformV& aToB, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth)
	{
		PxU8 size = 0;
		return gjkRelativePenetration(a, b, aToB, contactDist, contactA,contactB, normal, penetrationDepth, NULL, NULL, size);
	}

	PxGJKStatus GJKRelativePenetration(const TriangleV& a, const CapsuleV& b, const Ps::aos::PsMatTransformV& aToB, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth)
	{
		PxU8 size = 0;
		return gjkRelativePenetration(a, b, aToB, contactDist, contactA,contactB, normal, penetrationDepth, NULL, NULL, size);
	}

	PxGJKStatus GJKRelativePenetration(const TriangleV& a, const ConvexHullV& b, const Ps::aos::PsMatTransformV& aToB, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth)
	{
		PxU8 size = 0;
		return gjkRelativePenetration(a, b, aToB, contactDist, contactA,contactB, normal, penetrationDepth, NULL, NULL, size);
	}
	
	//capsule vs other

	PxGJKStatus GJKRelativePenetration(const CapsuleV& a, const CapsuleV& b, const Ps::aos::PsMatTransformV& aToB, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth)
	{
		PxU8 size = 0;
		return 	gjkRelativePenetration(a, b, aToB, contactDist, contactA,contactB, normal, penetrationDepth, NULL, NULL, size);
	}

	PxGJKStatus GJKRelativePenetration(const CapsuleV& a, const BoxV& b, const Ps::aos::PsMatTransformV& aToB, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth)
	{
		PxU8 size = 0;
		return 	gjkRelativePenetration(a, b, aToB, contactDist, contactA,contactB, normal, penetrationDepth, NULL, NULL, size);
	}

	PxGJKStatus GJKRelativePenetration(const CapsuleV& a, const ConvexHullV& b, const Ps::aos::PsMatTransformV& aToB, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth)
	{
		PxU8 size = 0;
		return gjkRelativePenetration(a, b, aToB, contactDist, contactA,contactB, normal, penetrationDepth, NULL, NULL, size);
	}

	//box vs other
	PxGJKStatus GJKRelativePenetration(const BoxV& a, const BoxV& b, const Ps::aos::PsMatTransformV& aTob, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth)
	{
		PxU8 size = 0;
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, NULL, NULL, size);
	}

	PxGJKStatus GJKRelativePenetration(const BoxV& a, const ConvexHullV& b, const Ps::aos::PsMatTransformV& aTob,  const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth)
	{
		PxU8 size = 0;
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, NULL, NULL, size);
	}


	/*
		convexHull vs others
	*/
	PxGJKStatus GJKRelativePenetration(const ConvexHullV& a, const ConvexHullV& b, const Ps::aos::PsMatTransformV& aTob, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth)
	{
		PxU8 size = 0;
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, NULL, NULL, size);
	}  

	PxGJKStatus GJKRelativePenetration(const ShrunkConvexHullV& a, const ShrunkConvexHullV& b, const Ps::aos::PsMatTransformV& aTob, const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth)
	{
		PxU8 size = 0;
		return 	gjkRelativePenetration(a, b, aTob, contactDist, contactA,contactB, normal, penetrationDepth, NULL, NULL, size);
	}  

}
}