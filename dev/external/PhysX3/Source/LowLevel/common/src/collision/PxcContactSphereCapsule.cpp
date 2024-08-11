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

#include "GuDistancePointSegment.h"
#include "PxcContactMethodImpl.h"
#include "PxcNpCache.h"
#include "GuContactBuffer.h"
#include "GuGeomUtilsInternal.h"

using namespace physx;
using namespace Gu;

namespace physx
{
bool PxcContactSphereCapsule(CONTACT_METHOD_ARGS)
{
	PX_UNUSED(npCache);

	// Get actual shape data
	const PxSphereGeometry& shapeSphere = shape0.get<const PxSphereGeometry>();
	const PxCapsuleGeometry& shapeCapsule = shape1.get<const PxCapsuleGeometry>();

	//Sphere in world space
	const PxVec3& sphere = transform0.p;
	
	//Capsule in world space
	Gu::Segment segment;
	getCapsuleSegment(transform1, shapeCapsule, segment);
	
	const PxReal radiusSum = shapeSphere.radius + shapeCapsule.radius;
	const PxReal inflatedSum = radiusSum + contactDistance;

	// Collision detection
	PxReal u;
	const PxReal squareDist = Gu::distancePointSegmentSquared(segment, sphere, &u);

	if(squareDist < inflatedSum*inflatedSum)
	{
		PxVec3 normal = sphere - segment.getPointAt(u);
		
		//We do a *manual* normalization to check for singularity condition
		const PxReal lenSq = normal.magnitudeSquared();
		if(lenSq==0.0f) 
		{
			// PT: zero normal => pick up random one
			normal = PxVec3(1.0f, 0.0f, 0.0f);
		}
		else
		{
			normal *= PxRecipSqrt(lenSq);
		}
		const PxVec3 point = sphere - normal * shapeSphere.radius;

		contactBuffer.contact(point, normal, PxSqrt(squareDist) - radiusSum);
		return true;
	}
	return false;
}
}