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

#include "GuVecBox.h"
#include "GuVecCapsule.h"

using namespace physx;
using namespace Gu;

PX_FORCE_INLINE void computeBasis2(const Ps::aos::Vec3VArg dir, Ps::aos::Vec3V& right, Ps::aos::Vec3V& up)
{
	//compute the closest axis
	using namespace Ps::aos;
	const FloatV zero = FZero();
	const FloatV x = V3GetX(dir);
	const FloatV y = V3GetY(dir);
	const FloatV z = V3GetZ(dir);

	const Vec3V v0 = V3Merge(FNeg(y), x, zero);// x is the longest axis
	const Vec3V v1 = V3Merge(zero, FNeg(z), y); //y is the longest axis
	const Vec3V v2 = V3Merge(z, zero, FNeg(x)); // z is the longest axis

	const BoolV con0 = BAllTrue3(V3IsGrtrOrEq(dir, Vec3V_From_FloatV_WUndefined(FAbs(x))));
	const BoolV con1 = BAllTrue3(V3IsGrtrOrEq(dir, Vec3V_From_FloatV_WUndefined(FAbs(y))));
	right = V3Normalize(V3Sel(con0, v0, V3Sel(con1, v1, v2)));
	up = V3Cross(dir, right);
}

void BoxV::create(const Gu::CapsuleV& capsule)
{
	using namespace Ps::aos;
	const FloatV half = FLoad(0.5f);
	const FloatV cRadius = capsule.radius;
	// Box center = center of the two LSS's endpoints
	center = capsule.getCenter();

	const Vec3V dir = capsule.computeDirection(); //capsule.p1 - capsule.p0;
	const FloatV d = V3Length(dir);
	rot.col0 = V3Scale(dir, FRecip(d));

	//Box extent
	extents = V3Splat(cRadius);
	V3SetX(extents, FMulAdd(d, half, cRadius));

	// Box orientation
	computeBasis2(rot.col0, rot.col1, rot.col2);
}




PxU32 BoxV::isInside(const Gu::BoxV& box) const
{
	using namespace Ps::aos;

	// Make a 4x4 from the box & inverse it
	const Mat34V M0(box.rot.col0, box.rot.col1, box.rot.col2, box.center);
	const Mat33V rot(M0.col0, M0.col1, M0.col2);
	const Vec3V nCol3 = V3Neg(M0.col3);
	const Mat33V tRot = M33Trnsps(rot);
	const Vec3V col3 = M33TrnspsMulV3(rot, nCol3);
	const Mat34V M0Inv =  Mat34V(tRot.col0, tRot.col1, tRot.col2, col3);
	Gu::BoxV _lin0;
	rotate(M0Inv, _lin0);

	// This should cancel out box0's rotation, i.e. it's now an AABB.
	// => Center(0,0,0), Rot(identity)

	// The two boxes are in the same space so now we can compare them.

	// Create the AABB of (box1 in space of box0)
	//const PxMat33& mtx = _1in0.rot;
	const Mat33V mtx = _lin0.rot;

	const Mat33V mtxAbs = M33Abs(mtx);
	const Vec3V eAbs = V3Abs(extents);

	const Vec3V f = V3Sub(M33TrnspsMulV3(mtxAbs, eAbs), box.extents);

	const BoolV con0 = BAnyTrue3(V3IsGrtr(f, _lin0.center));
	const BoolV con1 = BAnyTrue3(V3IsGrtr(_lin0.center, V3Neg(f)));
	const BoolV con = BAnd(con0, con1);
	return BAllEq(con, BFFFF());
}

//Ps::aos::Vec3V BoxV::supportVertsLocal(const Ps::aos::Vec3VArg dir) const
//{
//	using namespace Ps::aos;
//	return V3Sel(V3IsGrtr(dir,  V3Zero()), extents, V3Neg(extents));
//}
//
//void BoxV::populateVerts(const PxU8* inds, PxU32 numInds, const PxVec3* originalVerts, Ps::aos::Vec3V* verts)const
//{
//	using namespace Ps::aos;
//
//	for(PxU32 i=0; i<numInds; ++i)
//	{
//		verts[i] = Vec3V_From_PxVec3(originalVerts[inds[i]]);
//	}
//}  


