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

#ifndef __FIELD_BOUNDARY_DRAWER_H__
#define __FIELD_BOUNDARY_DRAWER_H__

#include "PsShare.h"
namespace physx
{
namespace apex
{

class NxApexRenderDebug;

namespace fieldboundary
{

class FieldBoundaryDrawer
{
public:
	static void drawFieldBoundarySphere(NxApexRenderDebug&, const physx::PxVec3& position, physx::PxF32 radius, physx::PxU32 stepCount);
	static void drawFieldBoundaryBox(NxApexRenderDebug&, const physx::PxVec3& bmin, const physx::PxVec3& bmax);
	static void drawFieldBoundaryCapsule(NxApexRenderDebug&, physx::PxF32 radius, physx::PxF32 height, physx::PxU32 subdivision, const physx::PxMat44& transform);
	static void drawFieldBoundaryConvex(NxApexRenderDebug&, const physx::PxVec3& position, physx::PxU32 numVertices, physx::PxU32 numTriangles, physx::PxU32 pointStrideBytes, physx::PxU32 triangleStrideBytes, const void* pointsBase, const void* trianglesBase);
};

} //namespace fieldboundary
} //namespace apex
} //namespace physx

#endif //__FIELD_BOUNDARY_DRAWER_H__