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


#include "Opcode.h"

// Following code from Magic-Software (http://www.magic-software.com/)
// A bit modified for Opcode

// New version in Opcode 1.4

namespace physx
{

using namespace Ps::aos;

namespace Gu
{

#ifndef OPC_SUPPORT_VMX128
PX_FORCE_INLINE Ps::IntBool LSSCollider::LSSAABBOverlap(const PxVec3& center, const PxVec3& extents) const
{
	// Stats
	mNbVolumeBVTests++;

	const float dcx = mSCen.x - center.x;
	const float ex = extents.x + mRadius;
	if(PxAbs(dcx)>ex + mFDir.x)	return Ps::IntFalse;

	const float dcy = mSCen.y - center.y;
	const float ey = extents.y + mRadius;
	if(PxAbs(dcy)>ey + mFDir.y)	return Ps::IntFalse;

	const float dcz = mSCen.z - center.z;
	const float ez = extents.z + mRadius;
	if(PxAbs(dcz)>ez + mFDir.z)	return Ps::IntFalse;

	if(PxAbs(mSDir.y * dcz - mSDir.z * dcy) > ey*mFDir.z + ez*mFDir.y)	return Ps::IntFalse;
	if(PxAbs(mSDir.z * dcx - mSDir.x * dcz) > ex*mFDir.z + ez*mFDir.x)	return Ps::IntFalse;
	if(PxAbs(mSDir.x * dcy - mSDir.y * dcx) > ex*mFDir.y + ey*mFDir.x)	return Ps::IntFalse;

	return Ps::IntTrue;
}
#else
PX_FORCE_INLINE Ps::IntBool LSSCollider::LSSAABBOverlap(const PxVec3& center, const PxVec3& extents) const
{
	Vec3V center4 = V3LoadU(center);
	Vec3V extents4 = V3LoadU(extents);

	return LSSAABBOverlap(center4, extents4);
}

PX_FORCE_INLINE Ps::IntBool LSSCollider::LSSAABBOverlap(const Vec3V center, const Vec3V extents) const
{
	BoolV ffff = BFFFF();

	Vec3V sCen = V3LoadU(mSCen);
	Vec3V radius = V3Load(mRadius);
	Vec3V fDir = V3LoadU(mFDir);
	Vec3V sDir = V3LoadU(mSDir);//sDir appears to equal sCen can we remove.

	Vec3V dc = V3Sub(sCen, center);
	Vec3V e = V3Add(extents, radius);
	Vec3V absDc = V3Abs(dc);
	Vec3V ePlusFDir = V3Add(e, fDir);


	BoolV mask = V3IsGrtr(absDc, ePlusFDir);
	if(!BAllEq(mask, ffff))//could avoid this with any true variant of greater.
		return Ps::IntFalse;

	/*
	if(fabsf(mSDir.x * dcy - mSDir.y * dcx) > ex*mFDir.y + ey*mFDir.x)	return Ps::IntFalse;
	if(fabsf(mSDir.y * dcz - mSDir.z * dcy) > ey*mFDir.z + ez*mFDir.y)	return Ps::IntFalse;
	if(fabsf(mSDir.z * dcx - mSDir.x * dcz) > ex*mFDir.z + ez*mFDir.x)	return Ps::IntFalse;
	*/

	Vec3V dcYZX = V3PermYZX(dc);
	Vec3V sDirYZX = V3PermYZX(sDir);
	Vec3V eXYX = V3PermXYX(e);
	Vec3V fDirYZZ = V3PermYZZ(fDir);
	Vec3V eYZZ = V3PermYZZ(e);
	Vec3V fDirXYX = V3PermXYX(fDir);

	Vec3V a = V3Sub(V3Mul(sDir, dcYZX), V3Mul(sDirYZX, dc));
	a = V3Abs(a);

	Vec3V b = V3MulAdd(eXYX, fDirYZZ, V3Mul(eYZZ, fDirXYX));

	mask = V3IsGrtr(a, b);
	if(!BAllEq(mask, ffff))//could avoid this with any true variant of greater.
		return Ps::IntFalse;

	return Ps::IntTrue;
}
#endif //OPC_SUPPORT_VMX128

} // namespace Gu

}
