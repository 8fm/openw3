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


#ifndef OPC_SPHERE_AABB_OVERLAP_H
#define OPC_SPHERE_AABB_OVERLAP_H

#include "Opcode.h"

namespace physx
{
namespace Gu
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Sphere-AABB overlap test, based on Jim Arvo's code.
 *	\param		center		[in] box center
 *	\param		extents		[in] box extents
 *	\return		Ps::IntTrue on overlap
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef OPC_SUPPORT_VMX128
PX_FORCE_INLINE Ps::IntBool SphereCollider::SphereAABBOverlap(const PxVec3& center, const PxVec3& extents) const
{ 
	// Stats
	mNbVolumeBVTests++;

	float d = 0.0f;

	//find the square of the distance
	//from the sphere to the box
#ifdef OLDIES
	for(udword i=0;i<3;i++)
	{
		float tmp = mCenter[i] - center[i];
		float s = tmp + extents[i];

		if(s<0.0f)	d += s*s;
		else
		{
			s = tmp - extents[i];
			if(s>0.0f)	d += s*s;
		}
	}
#endif

//#ifdef NEW_TEST

//	float tmp = mCenter.x - center.x;
//	float s = tmp + extents.x;

	float tmp,s;

	tmp = mCenter.x - center.x;
	s = tmp + extents.x;

	if(s<0.0f)
	{
		d += s*s;
		if(d>mRadius2)	return Ps::IntFalse;
	}
	else
	{
		s = tmp - extents.x;
		if(s>0.0f)
		{
			d += s*s;
			if(d>mRadius2)	return Ps::IntFalse;
		}
	}

	tmp = mCenter.y - center.y;
	s = tmp + extents.y;

	if(s<0.0f)
	{
		d += s*s;
		if(d>mRadius2)	return Ps::IntFalse;
	}
	else
	{
		s = tmp - extents.y;
		if(s>0.0f)
		{
			d += s*s;
			if(d>mRadius2)	return Ps::IntFalse;
		}
	}

	tmp = mCenter.z - center.z;
	s = tmp + extents.z;

	if(s<0.0f)
	{
		d += s*s;
		if(d>mRadius2)	return Ps::IntFalse;
	}
	else
	{
		s = tmp - extents.z;
		if(s>0.0f)
		{
			d += s*s;
			if(d>mRadius2)	return Ps::IntFalse;
		}
	}
//#endif

#ifdef OLDIES
//	Point Min = center - extents;
//	Point Max = center + extents;

	float d = 0.0f;

	//find the square of the distance
	//from the sphere to the box
	for(udword i=0;i<3;i++)
	{
float Min = center[i] - extents[i];

//		if(mCenter[i]<Min[i])
		if(mCenter[i]<Min)
		{
//			float s = mCenter[i] - Min[i];
			float s = mCenter[i] - Min;
			d += s*s;
		}
		else
		{
float Max = center[i] + extents[i];

//			if(mCenter[i]>Max[i])
			if(mCenter[i]>Max)
			{
				float s = mCenter[i] - Max;
				d += s*s;
			}
		}
	}
#endif
	return d <= mRadius2;
}
#else

PX_FORCE_INLINE Ps::IntBool SphereCollider::SphereAABBOverlap(const PxVec3& center, const PxVec3& extents) const
{
	Vec3V center4 = V3LoadU(center);
	Vec3V extents4 = V3LoadU(extents);

	return SphereAABBOverlap(center4, extents4);
}

PX_FORCE_INLINE Ps::IntBool SphereCollider::SphereAABBOverlap(const Vec3V center4, const Vec3V extents4) const
{
	// calculate the squared distance between the point and the AABB
	BoolV tttt = BTTTT();

	Vec3V sphereCen = V3LoadU(mCenter);
	FloatV sphereRadius2 = FLoad(mRadius2);

	Vec3V bbMin = V3Sub(center4, extents4);
	Vec3V bbMax = V3Add(center4, extents4);

	BoolV minMask = V3IsGrtr(bbMin, sphereCen);
	BoolV maxMask = V3IsGrtr(sphereCen, bbMax);

	Vec3V sqDist = V3Zero();

	Vec3V tmp = V3Sub(bbMin, sphereCen);
	Vec3V sqDistA = V3MulAdd(tmp, tmp, sqDist);
	sqDist = V3Sel(minMask, sqDistA, sqDist);

	Vec3V tmp2 = V3Sub(sphereCen, bbMax);
	Vec3V sqDistB = V3MulAdd(tmp2, tmp2, sqDist);
	sqDist = V3Sel(maxMask, sqDistB, sqDist);

	// horizontal add(3 component)
	FloatV sqDistRes = V3SumElems(sqDist);

	//compare to radius squared
	BoolV resMask = FIsGrtrOrEq(sphereRadius2, sqDistRes);
	return BAllEq(resMask, tttt);
}

#endif

} // namespace Gu

}

#endif