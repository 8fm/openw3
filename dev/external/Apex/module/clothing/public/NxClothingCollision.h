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

#ifndef NX_CLOTHING_COLLISION_H
#define NX_CLOTHING_COLLISION_H

/*!
\file
\brief classes NxClothingCollision, NxClothingPlane, NxClothingConvex, NxClothingSphere, NxClothingCapsule, NxClothingTriangleMesh
*/

#include "NxApexInterface.h"

namespace physx
{
	class PxPlane;

namespace apex
{
PX_PUSH_PACK_DEFAULT


/**
\brief Type of the NxClothingCollision
*/
struct NxClothingCollisionType
{
	/**
	\brief Enum.
	*/
	enum Enum
	{
		Plane,
		Convex,
		Sphere,
		Capsule,
		TriangleMesh
	};
};



class NxClothingPlane;
class NxClothingConvex;
class NxClothingSphere;
class NxClothingCapsule;
class NxClothingTriangleMesh;

/**
\brief Base class of all clothing collision types.
*/
class NxClothingCollision : public NxApexInterface
{
public:
	/**
	\brief Returns the Type of this collision object.
	*/
	virtual NxClothingCollisionType::Enum getType() const = 0;

	/**
	\brief Returns the pointer to this object if it is a plane, NULL otherwise.
	*/
	virtual NxClothingPlane* isPlane() = 0;

	/**
	\brief Returns the pointer to this object if it is a convex, NULL otherwise.
	*/
	virtual NxClothingConvex* isConvex() = 0;

	/**
	\brief Returns the pointer to this object if it is a sphere, NULL otherwise.
	*/
	virtual NxClothingSphere* isSphere() = 0;

	/**
	\brief Returns the pointer to this object if it is a capsule, NULL otherwise.
	*/
	virtual NxClothingCapsule* isCapsule() = 0;

	/**
	\brief Returns the pointer to this object if it is a triangle mesh, NULL otherwise.
	*/
	virtual NxClothingTriangleMesh* isTriangleMesh() = 0;
};



/**
\brief Plane collision of a clothing actor.
*/
class NxClothingPlane : public NxClothingCollision
{
public:
	/**
	\brief Sets the plane equation of the plane.
	*/
	virtual void setPlane(const PxPlane& plane) = 0;

	/**
	\brief Returns the plane equation of the plane.
	*/
	virtual const PxPlane& getPlane() = 0;

	/**
	\brief Returns the number of convexes that reference this plane.
	*/
	virtual PxU32 getRefCount() const = 0;
};



/**
\brief Convex collision of a clothing actor.

A convex is represented by a list of NxClothingPlanes.
*/
class NxClothingConvex : public NxClothingCollision
{
public:
	/**
	\brief Returns the number of planes that define this convex.
	*/
	virtual PxU32 getNumPlanes() = 0;

	/**
	\brief Returns pointer to the planes that define this convex.
	*/
	virtual NxClothingPlane** getPlanes() = 0;

	/**
	\brief Releases this convex and all its planes.

	\note Only use this if the planes are used exclusively by this convex. Planes referenced by other convexes will not be released.
	*/
	virtual void releaseWithPlanes() = 0;
};



/**
\brief Sphere collision of a clothing actor.
*/
class NxClothingSphere : public NxClothingCollision
{
public:
	/**
	\brief Sets the position of this sphere.
	*/
	virtual void setPosition(const PxVec3& position) = 0;

	/**
	\brief Returns the position of this sphere.
	*/
	virtual const PxVec3& getPosition() const = 0;

	/**
	\brief Sets the radius of this sphere.
	*/

	/**
	\brief Sets the Radius of this sphere.
	*/
	virtual void setRadius(PxF32 radius) = 0;

	/**
	\brief Returns the Radius of this sphere.
	*/
	virtual PxF32 getRadius() const = 0;

	/**
	\brief Returns the number of capsules that reference this plane.
	*/
	virtual PxU32 getRefCount() const = 0;
};



/**
\brief Capsule collision of a clothing actor.

A capsule is represented by two NxClothingSpheres.
*/
class NxClothingCapsule : public NxClothingCollision
{
public:
	/**
	\brief Returns the pointers to the two spheres that represent this capsule.
	*/
	virtual NxClothingSphere** getSpheres() = 0;

	/**
	\brief Releases this sphere and its two spheres.

	\note Only use releaseWithSpheres if the 2 spheres are used exclusively by this capsule. Spheres referenced by other convexes will not be released.
	*/
	virtual void releaseWithSpheres() = 0;
};



/**
\brief Triangle mesh collision of a clothing actor.
*/
class NxClothingTriangleMesh : public NxClothingCollision
{
public:
	/**
	\brief Adds a triangle to the mesh. Clockwise winding.
	*/
	virtual void addTriangle(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2) = 0;
	virtual const PxVec3* getTriangleBuffer() = 0;
	virtual PxU32 getNumTriangles() = 0;
	virtual bool removeTriangle(PxU32 index) = 0;

	/**
	\brief Clear all triangles to start with an empty mesh.
	Allows to reuse this instance and add new triangles.
	*/
	virtual void clearTriangles() = 0;

	/**
	\brief Sets the global pose of the mesh.
	*/
	virtual void setPose(PxMat44 pose) = 0;

	/**
	\brief Returns the global pose of the mesh.
	*/
	virtual const PxMat44& getPose() const = 0;
};



PX_POP_PACK
} // namespace apex
} // namespace physx


#endif // NX_CLOTHING_COLLISION_H