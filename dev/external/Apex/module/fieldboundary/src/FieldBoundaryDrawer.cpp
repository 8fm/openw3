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

#include "NxApexDefs.h"
#include "MinPhysxSdkVersion.h"
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2

#include "FieldBoundaryDrawer.h"
#include "NiApexRenderDebug.h"

namespace physx
{
namespace apex
{
namespace fieldboundary
{

void FieldBoundaryDrawer::drawFieldBoundarySphere(NxApexRenderDebug& rd, const physx::PxVec3& position, physx::PxF32 radius, physx::PxU32 stepCount)
{
	PX_ASSERT(&rd != NULL);
	rd.debugDetailedSphere(position, radius, stepCount);
}

void FieldBoundaryDrawer::drawFieldBoundaryBox(NxApexRenderDebug& rd, const physx::PxVec3& bmin, const physx::PxVec3& bmax)
{
	PX_ASSERT(&rd != NULL);
	rd.debugBound(bmin, bmax);
}

void FieldBoundaryDrawer::drawFieldBoundaryCapsule(NxApexRenderDebug& rd, physx::PxF32 radius, physx::PxF32 height, physx::PxU32 subdivision, const physx::PxMat44& transform)
{
	PX_ASSERT(&rd != NULL);
	rd.debugOrientedCapsule(radius, height, subdivision, transform);
}

void FieldBoundaryDrawer::drawFieldBoundaryConvex(NxApexRenderDebug& rd, const physx::PxVec3& position, physx::PxU32 numVertices, physx::PxU32 numTriangles, physx::PxU32 pointStrideBytes, physx::PxU32 triangleStrideBytes, const void* pointsBase, const void* trianglesBase)
{
	PX_UNUSED(pointStrideBytes);
	PX_UNUSED(numVertices);
	PX_ASSERT(&rd != NULL);
	PX_ASSERT(numTriangles != 0u);
	PX_ASSERT((triangleStrideBytes == (3 * sizeof(physx::PxU32))) || (triangleStrideBytes == (3 * sizeof(physx::PxU16))));
	PX_ASSERT(numVertices != 0);
	PX_ASSERT(pointStrideBytes == sizeof(physx::PxVec3));
	for (physx::PxU32 k = 0; k < numTriangles; k++)
	{
		physx::PxU32 triIndices[3];
		physx::PxVec3 triPoints[3];
		if (triangleStrideBytes == (3 * sizeof(physx::PxU32)))
		{
			triIndices[0] = *((physx::PxU32*)trianglesBase + 3 * k + 0);
			triIndices[1] = *((physx::PxU32*)trianglesBase + 3 * k + 1);
			triIndices[2] = *((physx::PxU32*)trianglesBase + 3 * k + 2);
		}
		else
		{
			triIndices[0] = *((physx::PxU16*)trianglesBase + 3 * k + 0);
			triIndices[1] = *((physx::PxU16*)trianglesBase + 3 * k + 1);
			triIndices[2] = *((physx::PxU16*)trianglesBase + 3 * k + 2);
		}
		PX_ASSERT(triIndices[0] < numVertices);
		PX_ASSERT(triIndices[1] < numVertices);
		PX_ASSERT(triIndices[2] < numVertices);
		triPoints[0] = *((physx::PxVec3*)pointsBase + triIndices[0]);
		triPoints[1] = *((physx::PxVec3*)pointsBase + triIndices[1]);
		triPoints[2] = *((physx::PxVec3*)pointsBase + triIndices[2]);
		triPoints[0] += position;
		triPoints[1] += position;
		triPoints[2] += position;
		rd.debugPolygon(3, triPoints);
	}
}

} //namespace fieldboundary
} //namespace apex
} //namespace physx

#endif