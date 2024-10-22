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

#include "GuContactBuffer.h"
#include "PxcContactMethodImpl.h"
#include "PxcNpCache.h"

using namespace physx;

namespace physx
{
bool PxcContactSphereSphere(CONTACT_METHOD_ARGS)
{
	PX_UNUSED(npCache);

	// Get actual shape data
	const PxSphereGeometry& shapeSphere0 = shape0.get<const PxSphereGeometry>();
	const PxSphereGeometry& shapeSphere1 = shape1.get<const PxSphereGeometry>();

	// Separation
	PxVec3 delta = transform0.p - transform1.p;

	const PxReal distanceSq = delta.magnitudeSquared();
	const PxReal radiusSum = shapeSphere0.radius + shapeSphere1.radius;
	const PxReal inflatedSum = radiusSum + contactDistance;

	if(distanceSq < inflatedSum*inflatedSum)
	{
		// Have some kind of contact
		const PxReal magn = PxSqrt(distanceSq);
		if(magn<=0.00001f)
		{
			// PT: spheres are exactly overlapping => can't create normal => pick up random one
			delta = PxVec3(1.0f, 0.0f, 0.0f);
		}
		else
		{
			// Normalize delta
			delta *= 1.0f/magn;
		}

		// PT: why is this formula different from the original code?
		const PxVec3 contact = delta * ((shapeSphere0.radius + magn - shapeSphere1.radius)*-0.5f) + transform0.p;
		
		contactBuffer.contact(contact, delta, magn - radiusSum);
		return true;
	}
	return false;
}
}
