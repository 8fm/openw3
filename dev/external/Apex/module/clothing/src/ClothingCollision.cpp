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

#include "ClothingCollision.h"
#include "ClothingActor.h"

using namespace physx;
using namespace apex;
using namespace clothing;


ClothingCollision::ClothingCollision(NxResourceList& list, ClothingActor& owner) :
mOwner(owner),
mInRelease(false),
mId(-1)
{
	list.add(*this);
}


void ClothingCollision::release()
{
	if (mInRelease)
		return;
	mInRelease = true;
	mOwner.releaseCollision( *this );
}


void ClothingCollision::destroy()
{
	delete this;
}


void ClothingPlane::setPlane(const PxPlane& plane)
{
	mPlane = plane;
	mOwner.notifyCollisionChange();
}


void ClothingSphere::setPosition(const PxVec3& position)
{
	mPosition = position;
	mOwner.notifyCollisionChange();
}



void ClothingSphere::setRadius(PxF32 radius)
{
	mRadius = radius;
	mOwner.notifyCollisionChange();
}



void ClothingTriangleMesh::addTriangle(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2)
{
	mTriangles.pushBack(v0);
	mTriangles.pushBack(v1);
	mTriangles.pushBack(v2);

	mOwner.notifyCollisionChange();
}

const PxVec3* ClothingTriangleMesh::getTriangleBuffer()
{
	if( mTriangles.empty() ) return 0;
	return &mTriangles[ 0 ];
}

PxU32 ClothingTriangleMesh::getNumTriangles()
{
	return mTriangles.size() / 3;
}

bool ClothingTriangleMesh::removeTriangle(PxU32 index)
{
	if( getNumTriangles() <= index ) return false;
	mTriangles.removeRange( index * 3, 3 );
	mOwner.notifyCollisionChange();
	return true;
}

void ClothingTriangleMesh::clearTriangles()
{
	mTriangles.clear();
	mOwner.clearCollision(*this);
}


void ClothingTriangleMesh::setPose(PxMat44 pose)
{
	mPose = pose;

	mOwner.notifyCollisionChange();
}
