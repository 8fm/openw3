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

#include "GuIntersectionBoxBox.h"
#include "GuIntersectionBoxBoxSIMD.h"

using namespace physx;

bool Gu::intersectOBBOBB(const PxVec3& e0, const PxVec3& c0, const PxMat33& r0, const PxVec3& e1, const PxVec3& c1, const PxMat33& r1, bool full_test)
{
	// Translation, in parent frame
	const PxVec3 v = c1 - c0;
	// Translation, in A's frame
	const PxVec3 T(v.dot(r0[0]), v.dot(r0[1]), v.dot(r0[2]));

	// B's basis with respect to A's local frame
	PxReal R[3][3];
	PxReal FR[3][3];
	PxReal ra, rb, t;

	// Calculate rotation matrix
	for(PxU32 i=0;i<3;i++)
	{
		for(PxU32 k=0;k<3;k++)
		{
			R[i][k] = r0[i].dot(r1[k]);
			FR[i][k] = 1e-6f + PxAbs(R[i][k]);	// Precompute fabs matrix
		}
	}

	// A's basis vectors
	for(PxU32 i=0;i<3;i++)
	{
		ra = e0[i];

		rb = e1[0]*FR[i][0] + e1[1]*FR[i][1] + e1[2]*FR[i][2];

		t = PxAbs(T[i]);

		if(t > ra + rb)		return false;
	}

	// B's basis vectors
	for(PxU32 k=0;k<3;k++)
	{
		ra = e0[0]*FR[0][k] + e0[1]*FR[1][k] + e0[2]*FR[2][k];

		rb = e1[k];

		t = PxAbs(T[0]*R[0][k] + T[1]*R[1][k] + T[2]*R[2][k]);

		if( t > ra + rb )	return false;
	}

	if(full_test)
	{
		//9 cross products

		//L = A0 x B0
		ra	= e0[1]*FR[2][0] + e0[2]*FR[1][0];
		rb	= e1[1]*FR[0][2] + e1[2]*FR[0][1];
		t	= PxAbs(T[2]*R[1][0] - T[1]*R[2][0]);
		if(t > ra + rb)	return false;

		//L = A0 x B1
		ra	= e0[1]*FR[2][1] + e0[2]*FR[1][1];
		rb	= e1[0]*FR[0][2] + e1[2]*FR[0][0];
		t	= PxAbs(T[2]*R[1][1] - T[1]*R[2][1]);
		if(t > ra + rb)	return false;

		//L = A0 x B2
		ra	= e0[1]*FR[2][2] + e0[2]*FR[1][2];
		rb	= e1[0]*FR[0][1] + e1[1]*FR[0][0];
		t	= PxAbs(T[2]*R[1][2] - T[1]*R[2][2]);
		if(t > ra + rb)	return false;

		//L = A1 x B0
		ra	= e0[0]*FR[2][0] + e0[2]*FR[0][0];
		rb	= e1[1]*FR[1][2] + e1[2]*FR[1][1];
		t	= PxAbs(T[0]*R[2][0] - T[2]*R[0][0]);
		if(t > ra + rb)	return false;

		//L = A1 x B1
		ra	= e0[0]*FR[2][1] + e0[2]*FR[0][1];
		rb	= e1[0]*FR[1][2] + e1[2]*FR[1][0];
		t	= PxAbs(T[0]*R[2][1] - T[2]*R[0][1]);
		if(t > ra + rb)	return false;

		//L = A1 x B2
		ra	= e0[0]*FR[2][2] + e0[2]*FR[0][2];
		rb	= e1[0]*FR[1][1] + e1[1]*FR[1][0];
		t	= PxAbs(T[0]*R[2][2] - T[2]*R[0][2]);
		if(t > ra + rb)	return false;

		//L = A2 x B0
		ra	= e0[0]*FR[1][0] + e0[1]*FR[0][0];
		rb	= e1[1]*FR[2][2] + e1[2]*FR[2][1];
		t	= PxAbs(T[1]*R[0][0] - T[0]*R[1][0]);
		if(t > ra + rb)	return false;

		//L = A2 x B1
		ra	= e0[0]*FR[1][1] + e0[1]*FR[0][1];
		rb	= e1[0] *FR[2][2] + e1[2]*FR[2][0];
		t	= PxAbs(T[1]*R[0][1] - T[0]*R[1][1]);
		if(t > ra + rb)	return false;

		//L = A2 x B2
		ra	= e0[0]*FR[1][2] + e0[1]*FR[0][2];
		rb	= e1[0]*FR[2][1] + e1[1]*FR[2][0];
		t	= PxAbs(T[1]*R[0][2] - T[0]*R[1][2]);
		if(t > ra + rb)	return false;
	}
	return true;
}

//bool Gu::intersectOBBOBB(const Ps::aos::Vec3VArg e0, const Ps::aos::Vec3VArg c0, const Ps::aos::Mat33V& r0, const Ps::aos::Vec3VArg e1, const Ps::aos::Vec3VArg c1, const Ps::aos::Mat33V& r1, bool full_test)
bool Gu::intersectOBBOBB(const Ps::aos::Vec3VArg, const Ps::aos::Vec3VArg, const Ps::aos::Mat33V&, const Ps::aos::Vec3VArg, const Ps::aos::Vec3VArg, const Ps::aos::Mat33V&, bool)
{
	using namespace Ps::aos;

	// Translation, in parent frame
	//const Vec3 v = V3Sub(c1, c0);
	//const Vec3 t = M33MulV3(r0, v);
	//const Vec3 absT = V3Abs(t);
	//const Mat33 rotMatFBTA = M33TrnspsMulV3(r0, r1);

	//const Vec3 eps = FEps();

	//const Vec3 c0 = V3Add(V3Abs(rotMatFBTA.col0), eps);
	//const Vec3 c1 = V3Add(V3Abs(rotMatFBTA.col1), eps);
	//const Vec3 c2 = V3Add(V3Abs(rotMatFBTA.col2), eps);

	//const Mat33 absRotMat =Mat33(c0, c1, c2); 
	//const Mat33 trnAbsRotMat = M33Trnsps(absRotMat);

	////Test axes L = r0[0], L = r0[1], L = r0[2]
	//const Vec3 ra0 = e0;
	//const Vec3 rb0 = M33MulV3(absRotMat, e1);
	//const Vec3V rab0 = V3Add(ra0, rb0);
	//const BoolV con0 = BAnyTrue3(V3IsGrtr(absT, rab0)); 
	//

	////Test axes L = r1[0], L = r1[1], L = r1[2]
	//const Vec3 ra1 = M33TrnspsMulV3(absRotMat, e0);
	//const Vec3 rb1 = e1;
	//const Vec3V rab1 = V3Add(ra1, rb1);
	//const Vec3V tt = M33TrnspsMulV3(rotMat, t);
	//const Vec3V absTT = V3Abs(tt);
	//const BoolV con1 = BAnyTrue3(V3IsGrtr(absTT, rab1));
	//

	//if(full_test)
	//{
	//	//9 cross products

	//	//L = A0 x B0
	//	ra	= e0[1]*FR[2][0] + e0[2]*FR[1][0];
	//	rb	= e1[1]*FR[0][2] + e1[2]*FR[0][1];
	//	t	= PxAbs(T[2]*R[1][0] - T[1]*R[2][0]);
	//	if(t > ra + rb)	return false;

	//	//L = A0 x B1
	//	ra	= e0[1]*FR[2][1] + e0[2]*FR[1][1];
	//	rb	= e1[0]*FR[0][2] + e1[2]*FR[0][0];
	//	t	= PxAbs(T[2]*R[1][1] - T[1]*R[2][1]);
	//	if(t > ra + rb)	return false;

	//	//L = A0 x B2
	//	ra	= e0[1]*FR[2][2] + e0[2]*FR[1][2];
	//	rb	= e1[0]*FR[0][1] + e1[1]*FR[0][0];
	//	t	= PxAbs(T[2]*R[1][2] - T[1]*R[2][2]);
	//	if(t > ra + rb)	return false;

	//	//L = A1 x B0
	//	ra	= e0[0]*FR[2][0] + e0[2]*FR[0][0];
	//	rb	= e1[1]*FR[1][2] + e1[2]*FR[1][1];
	//	t	= PxAbs(T[0]*R[2][0] - T[2]*R[0][0]);
	//	if(t > ra + rb)	return false;

	//	//L = A1 x B1
	//	ra	= e0[0]*FR[2][1] + e0[2]*FR[0][1];
	//	rb	= e1[0]*FR[1][2] + e1[2]*FR[1][0];
	//	t	= PxAbs(T[0]*R[2][1] - T[2]*R[0][1]);
	//	if(t > ra + rb)	return false;

	//	//L = A1 x B2
	//	ra	= e0[0]*FR[2][2] + e0[2]*FR[0][2];
	//	rb	= e1[0]*FR[1][1] + e1[1]*FR[1][0];
	//	t	= PxAbs(T[0]*R[2][2] - T[2]*R[0][2]);
	//	if(t > ra + rb)	return false;

	//	//L = A2 x B0
	//	ra	= e0[0]*FR[1][0] + e0[1]*FR[0][0];
	//	rb	= e1[1]*FR[2][2] + e1[2]*FR[2][1];
	//	t	= PxAbs(T[1]*R[0][0] - T[0]*R[1][0]);
	//	if(t > ra + rb)	return false;

	//	//L = A2 x B1
	//	ra	= e0[0]*FR[1][1] + e0[1]*FR[0][1];
	//	rb	= e1[0] *FR[2][2] + e1[2]*FR[2][0];
	//	t	= PxAbs(T[1]*R[0][1] - T[0]*R[1][1]);
	//	if(t > ra + rb)	return false;

	//	//L = A2 x B2
	//	ra	= e0[0]*FR[1][2] + e0[1]*FR[0][2];
	//	rb	= e1[0]*FR[2][1] + e1[1]*FR[2][0];
	//	t	= PxAbs(T[1]*R[0][2] - T[0]*R[1][2]);
	//	if(t > ra + rb)	return false;
	//}
	return true;

}
