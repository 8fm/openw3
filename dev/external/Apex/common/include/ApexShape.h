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

#ifndef __APEX_SHAPE_H__
#define __APEX_SHAPE_H__

#include "NxApex.h"
#include "PsShare.h"
#include "PsUserAllocated.h"
#include "NxApexShape.h"

namespace physx
{
namespace apex
{

class ApexShape : public physx::UserAllocated
{
protected:
#if NX_SDK_VERSION_MAJOR == 2
	NxCollisionGroup	mCollisionGroup;
	NxGroupsMask		mCollisionGroup128;

	ApexShape()
	{
		mCollisionGroup = 0;
		mCollisionGroup128.bits0 = 0;
		mCollisionGroup128.bits1 = 0;
		mCollisionGroup128.bits2 = 0;
		mCollisionGroup128.bits3 = 0;
	}

	PX_INLINE void setCollisionGroup(NxCollisionGroup group)
	{
		mCollisionGroup = group;
	}
	PX_INLINE NxCollisionGroup getCollisionGroup() const
	{
		return mCollisionGroup;
	}

	PX_INLINE void setCollisionGroup128(const NxGroupsMask& group128)
	{
		mCollisionGroup128 = group128;
	}
	PX_INLINE const NxGroupsMask& getCollisionGroup128() const
	{
		return mCollisionGroup128;
	}
#endif
};

class ApexSphereShape : public NxApexSphereShape, public ApexShape
{
private:
	physx::PxF32		mRadius;
	physx::PxMat44		mTransform4x4;
	physx::PxMat44		mOldTransform4x4;
	physx::PxBounds3	mBounds;

	void calculateAABB();

public:

	ApexSphereShape();

	virtual ~ApexSphereShape() {};

	virtual void releaseApexShape()
	{
		delete this;
	}

	const ApexSphereShape* 	isSphereGeom() const
	{
		return this;
	}

	bool intersectAgainstAABB(physx::PxBounds3 bounds);

	PxBounds3 getAABB() const
	{
		return mBounds;
	}

	void setRadius(physx::PxF32 radius);

	physx::PxF32 getRadius() const
	{
		return mRadius;
	};

	void setPose(physx::PxMat44 pose);

	physx::PxMat44 getPose() const
	{
		return mTransform4x4;
	}

	physx::PxMat44 getPreviousPose() const
	{
		return mOldTransform4x4;
	}

	void visualize(NxApexRenderDebug* renderer) const;

#if NX_SDK_VERSION_MAJOR == 2
	void setCollisionGroup(NxCollisionGroup group)
	{
		ApexShape::setCollisionGroup(group);
	}
	NxCollisionGroup getCollisionGroup() const
	{
		return ApexShape::getCollisionGroup();
	}

	void setCollisionGroup128(const NxGroupsMask& group128)
	{
		ApexShape::setCollisionGroup128(group128);
	}
	const NxGroupsMask& getCollisionGroup128() const
	{
		return ApexShape::getCollisionGroup128();
	}
#endif
};

//the capsule is oriented along the y axis by default and its total height is height+2*radius
class ApexCapsuleShape : public NxApexCapsuleShape, public ApexShape
{
private:

	physx::PxF32 mRadius;
	physx::PxF32 mHeight;
	physx::PxMat44 mTransform4x4;
	physx::PxMat44 mOldTransform4x4;
	physx::PxBounds3 mBounds;
	void calculateAABB();

public:

	ApexCapsuleShape();

	virtual ~ApexCapsuleShape() {};

	virtual void releaseApexShape()
	{
		delete this;
	}

	const ApexCapsuleShape* 	isCapsuleGeom() const
	{
		return this;
	}

	bool intersectAgainstAABB(physx::PxBounds3 bounds);

	PxBounds3 getAABB() const
	{
		return mBounds;
	}

	void setDimensions(physx::PxF32 height, physx::PxF32 radius);

	void getDimensions(physx::PxF32& height, physx::PxF32& radius) const
	{
		radius = mRadius;
		height = mHeight;
	};

	void setPose(physx::PxMat44 pose);

	physx::PxMat44 getPose() const
	{
		return mTransform4x4;
	}

	physx::PxMat44 getPreviousPose() const
	{
		return mOldTransform4x4;
	}

	void visualize(NxApexRenderDebug* renderer) const;

#if NX_SDK_VERSION_MAJOR == 2
	void setCollisionGroup(NxCollisionGroup group)
	{
		ApexShape::setCollisionGroup(group);
	}
	NxCollisionGroup getCollisionGroup() const
	{
		return ApexShape::getCollisionGroup();
	}

	void setCollisionGroup128(const NxGroupsMask& group128)
	{
		ApexShape::setCollisionGroup128(group128);
	}
	const NxGroupsMask& getCollisionGroup128() const
	{
		return ApexShape::getCollisionGroup128();
	}
#endif
};

class ApexBoxShape : public NxApexBoxShape, public ApexShape
{
private:

	physx::PxVec3 mSize;
	physx::PxMat44 mTransform4x4;
	physx::PxMat44 mOldTransform4x4;
	physx::PxBounds3 mBounds;
	void calculateAABB();

public:

	ApexBoxShape();

	virtual ~ApexBoxShape() {};

	virtual void releaseApexShape()
	{
		delete this;
	}

	const ApexBoxShape* 	isBoxGeom() const
	{
		return this;
	}

	bool intersectAgainstAABB(physx::PxBounds3 bounds);

	PxBounds3 getAABB() const
	{
		return mBounds;
	}

	void setSize(physx::PxVec3 size);

	void setPose(physx::PxMat44 pose);

	physx::PxMat44 getPose() const
	{
		return mTransform4x4;
	}

	physx::PxMat44 getPreviousPose() const
	{
		return mOldTransform4x4;
	}

	physx::PxVec3 getSize() const
	{
		return mSize;
	}

	void visualize(NxApexRenderDebug* renderer) const;

#if NX_SDK_VERSION_MAJOR == 2
	void setCollisionGroup(NxCollisionGroup group)
	{
		ApexShape::setCollisionGroup(group);
	}
	NxCollisionGroup getCollisionGroup() const
	{
		return ApexShape::getCollisionGroup();
	}

	void setCollisionGroup128(const NxGroupsMask& group128)
	{
		ApexShape::setCollisionGroup128(group128);
	}
	const NxGroupsMask& getCollisionGroup128() const
	{
		return ApexShape::getCollisionGroup128();
	}
#endif
};

class ApexHalfSpaceShape : public NxApexHalfSpaceShape, public ApexShape
{
private:
	physx::PxVec3 mOrigin;
	physx::PxVec3 mPreviousOrigin;
	physx::PxVec3 mNormal;
	physx::PxVec3 mPreviousNormal;
	bool isPointInside(physx::PxVec3 pos);

public:
	ApexHalfSpaceShape();

	virtual ~ApexHalfSpaceShape() {};

	virtual void releaseApexShape()
	{
		delete this;
	}

	const ApexHalfSpaceShape* 	isHalfSpaceGeom() const
	{
		return this;
	}

	bool intersectAgainstAABB(physx::PxBounds3 bounds);

	PxBounds3 getAABB() const
	{
		return PxBounds3(PxVec3(0), PxVec3(PX_MAX_F32));
	}

	void setOriginAndNormal(physx::PxVec3 origin, physx::PxVec3 normal);

	physx::PxVec3 getNormal() const
	{
		return mNormal;
	};

	physx::PxVec3 getPreviousNormal() const
	{
		return mPreviousNormal;
	};

	physx::PxVec3 getOrigin() const
	{
		return mOrigin;
	};

	physx::PxVec3 getPreviousOrigin() const
	{
		return mPreviousOrigin;
	};

	physx::PxMat44 getPose() const;

	physx::PxMat44 getPreviousPose() const;

	void visualize(NxApexRenderDebug* renderer) const;

#if NX_SDK_VERSION_MAJOR == 2
	void setCollisionGroup(NxCollisionGroup group)
	{
		ApexShape::setCollisionGroup(group);
	}
	NxCollisionGroup getCollisionGroup() const
	{
		return ApexShape::getCollisionGroup();
	}

	void setCollisionGroup128(const NxGroupsMask& group128)
	{
		ApexShape::setCollisionGroup128(group128);
	}
	const NxGroupsMask& getCollisionGroup128() const
	{
		return ApexShape::getCollisionGroup128();
	}
#endif

	virtual void setPose(physx::PxMat44 pose)
	{
		PX_UNUSED(pose);
		//dummy
	}
};


}
} // end namespace physx::apex

#endif
