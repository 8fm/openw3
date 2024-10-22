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

#ifndef NX_APEX_SHAPE_H
#define NX_APEX_SHAPE_H

/*!
\file
\brief class NxApexShape
*/

#include "foundation/PxPreprocessor.h"
#include "NxApexUsingNamespace.h"

#include "NxApexDefs.h"

#if NX_SDK_VERSION_MAJOR < 3
#include "Nxp.h"
#endif

namespace physx
{
namespace apex
{

PX_PUSH_PACK_DEFAULT


class NxApexSphereShape;
class NxApexCapsuleShape;
class NxApexBoxShape;
class NxApexHalfSpaceShape;
class NxApexRenderDebug;

/**
\brief Describes the format of an NxApexShape.
*/
class NxApexShape
{
public:

	///intersect the collision shape against a given AABB
	virtual bool							intersectAgainstAABB(physx::PxBounds3) = 0;

	///AABB of collision shape
	virtual physx::PxBounds3				getAABB() const = 0;

	///If it is a box, cast to box class, return NULL otherwise
	virtual const NxApexBoxShape* 			isBoxGeom() const
	{
		return nullptr;
	}

	///If it is a sphere, cast to sphere class, return NULL otherwise
	virtual const NxApexSphereShape* 		isSphereGeom() const
	{
		return nullptr;
	}

	///If it is a capsule, cast to the capsule class, return NULL otherwise
	virtual const NxApexCapsuleShape* 		isCapsuleGeom() const
	{
		return nullptr;
	}

	///If it is a half space, cast to half space class, return NULL otherwise
	virtual const NxApexHalfSpaceShape* 	isHalfSpaceGeom() const
	{
		return nullptr;
	}

	///Release shape
	virtual void releaseApexShape() = 0;

	///Visualize shape
	virtual void visualize(NxApexRenderDebug* renderer) const = 0;

#if NX_SDK_VERSION_MAJOR < 3
	///set collision group
	virtual void setCollisionGroup(NxCollisionGroup) = 0;
	///set collision group
	virtual NxCollisionGroup getCollisionGroup() const = 0;
	///set collision group 128
	virtual void setCollisionGroup128(const NxGroupsMask&) = 0;
	///set collision group 128
	virtual const NxGroupsMask& getCollisionGroup128() const = 0;
#endif

	///get the pose of the shape
	virtual physx::PxMat44 getPose() const = 0;

	///set the pose of the shape - this pose will modify the scale, orientation and position of the shape
	virtual void setPose(physx::PxMat44 pose) = 0;

	///get the previous pose of the shape
	virtual physx::PxMat44 getPreviousPose() const = 0;
};

/// helper Sphere shape
class NxApexSphereShape : public NxApexShape
{
public:
	///get radius of the sphere
	virtual physx::PxF32 getRadius() const = 0;

	///set radius of the sphere
	virtual void setRadius(physx::PxF32 radius) = 0;

};

/// helper the capsule is oriented along the y axis by default and its total height is height+2*radius
class NxApexCapsuleShape : public NxApexShape
{
public:
	///get height and radius of the capsule
	virtual void getDimensions(physx::PxF32& height, physx::PxF32& radius) const = 0;

	///set height and radius of the capsule
	virtual void setDimensions(physx::PxF32 height, physx::PxF32 radius) = 0;

};

/// helper Box shape
class NxApexBoxShape : public NxApexShape
{
public:
	///set the length of the sides of the box. The default value is 1.0f for each of the sides.
	///the size of the sides can also be changed by using the setPose method
	virtual void setSize(physx::PxVec3 size) = 0;

	///get size of the box
	virtual physx::PxVec3 getSize() const = 0;

};

/// helper HalfSpace shape
class NxApexHalfSpaceShape : public NxApexShape
{
public:
	///set origin and normal of the halfspace
	virtual void setOriginAndNormal(physx::PxVec3 origin, physx::PxVec3 normal) = 0;

	///get normal of the halfspace
	virtual physx::PxVec3 getNormal() const = 0;

	///get origin of the halfspace
	virtual physx::PxVec3 getOrigin() const = 0;

	///get previous origin of the halfspace
	virtual physx::PxVec3 getPreviousOrigin() const = 0;
};


PX_POP_PACK

}
} // end namespace physx::apex

#endif // NX_APEX_SHAPE_H
