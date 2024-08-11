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

#include "PxcContactMethodImpl.h"
#include "PxcNpCache.h"
#include "GuContactBuffer.h"
#include "GuGeomUtilsInternal.h"

using namespace physx;
using namespace Gu;

namespace physx
{
bool PxcContactPlaneCapsule(CONTACT_METHOD_ARGS)
{
	PX_UNUSED(npCache);
	PX_UNUSED(shape0);

	// Get actual shape data
	//const PxPlaneGeometry& shapePlane = shape.get<const PxPlaneGeometry>();
	const PxCapsuleGeometry& shapeCapsule = shape1.get<const PxCapsuleGeometry>();

	const PxTransform capsuleToPlane = transform0.transformInv(transform1);

	//Capsule in plane space
	Gu::Segment segment;
	getCapsuleSegment(capsuleToPlane, shapeCapsule, segment);
	
	const PxVec3 negPlaneNormal = transform0.q.getBasisVector0();
	
	bool contact = false;

	const PxReal separation0 = segment.p0.x - shapeCapsule.radius;
	const PxReal separation1 = segment.p1.x - shapeCapsule.radius;
	if(separation0 <= contactDistance)
	{
		const PxVec3 temp(segment.p0.x - shapeCapsule.radius, segment.p0.y, segment.p0.z);
		const PxVec3 point = transform0.transform(temp);
		contactBuffer.contact(point, -negPlaneNormal, separation0);
		contact = true;
	}

	if(separation1 <= contactDistance)
	{
		const PxVec3 temp(segment.p1.x - shapeCapsule.radius, segment.p1.y, segment.p1.z);
		const PxVec3 point = transform0.transform(temp);
		contactBuffer.contact(point, -negPlaneNormal, separation1);
		contact = true;
	}
	return contact;
}
}