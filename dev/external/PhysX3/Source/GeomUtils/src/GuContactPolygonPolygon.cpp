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

#include "GuContactPolygonPolygon.h"
#include "PsMathUtils.h"
#include "GuShapeConvex.h"
#include "GuTriangleVertexPointers.h"
#include "GuContactBuffer.h"
#include "GuGeometryUnion.h"
#include "OPC_Common.h"

using namespace physx;
using namespace Gu;

// TODO:
// - discard 3rd component
// - use more restrict
// - edge planes?
// - inside circle?
// - revisit 'findRotationMatrix', return by ref
// - the EE case can still be skipped when all vertices are "inside the 2D convex polygon" but not *under* the plane.... difficult to detect though

#define CONTACT_REDUCTION

/*
void gVisualizeLocalLine(const PxVec3& a, const PxVec3& b, const Cm::Matrix34& m, PxsContactManager& manager)	//temp debug
{
	Cm::RenderOutput out = manager.getContext()->getRenderOutput();
	out << 0xffffff << m << Cm::RenderOutput::LINES << a << b;
}
*/

#ifdef CONTACT_REDUCTION
static PX_FORCE_INLINE PxReal dot2D(const PxVec3& v0, const PxVec3& v1)
{	
	return v0.x * v1.x + v0.y * v1.y;
}

static void ContactReductionAllIn(	ContactBuffer& contactBuffer, PxU32 nbExistingContacts, PxU32 numIn,
									const PxMat33& rotT,
									const PxVec3* PX_RESTRICT vertices, const PxU8* PX_RESTRICT indices)
{
	// Number of contacts created by current call
	const PxU32 nbNewContacts = contactBuffer.count - nbExistingContacts;

	if(nbNewContacts<=4)
		return;	// no reduction for less than 4 verts

	// We have 3 different numbers here:
	// - numVerts = number of vertices in the convex polygon we're dealing with
	// - numIn = number of those that were "inside" the other convex polygon (should be <= numVerts)
	// - contactBuffer.count = total number of contacts *from both polygons* (that's the catch here)
	// The fast path can only be chosen when the contact buffer contains all the verts from current polygon,
	// i.e. when contactBuffer.count == numIn == numVerts

	Gu::ContactPoint* PX_RESTRICT ctcs = contactBuffer.contacts + nbExistingContacts;

	if(numIn == nbNewContacts)
	{
		// Codepath 1: all vertices generated a contact

		PxReal deepestSeparation = ctcs[0].separation;
		PxU32 deepestIndex = 0;
		for(PxU32 i=1; i<nbNewContacts; ++i)
		{
			if(deepestSeparation > ctcs[i].separation)
			{
				deepestSeparation = ctcs[i].separation;
				deepestIndex = i;
			}
		}

		PxU32 index = 0;
		const PxU32 step = (numIn<<16)>>2;	// Fixed point math, don't use floats here please
		bool needsExtraPoint = true;
		for(PxU32 i=0;i<4;i++)
		{
			const PxU32 contactIndex = index>>16;
			ctcs[i] = ctcs[contactIndex];
			if(contactIndex==deepestIndex)
				needsExtraPoint = false;
			index += step;
		}

		if(needsExtraPoint)
		{
			ctcs[4] = ctcs[deepestIndex];
			contactBuffer.count = nbExistingContacts + 5;
		}
		else
		{
			contactBuffer.count = nbExistingContacts + 4;
		}

/*	PT: TODO: investigate why this one does not work
		PxU32 index = deepestIndex<<16;
		const PxU32 step = (numIn<<16)>>2;	// Fixed point math, don't use floats here please
		for(PxU32 i=0;i<4;i++)
		{
			PxU32 contactIndex = index>>16;
			if(contactIndex>=numIn)
				contactIndex -= numIn;
			ctcs[i] = ctcs[contactIndex];
			index += step;
		}
		contactBuffer.count = nbExistingContacts + 4;*/
	}
	else
	{
		// Codepath 2: all vertices are "in" but only some of them generated a contact

		// WARNING: this path doesn't work when the buffer contains vertices from both polys.

		// TODO: precompute those axes
		const PxU32 nbAxes = 8;
		PxVec3 dirs[nbAxes];
		float angle = 0.0f;
		const float angleStep = degToRad(180.0f/float(nbAxes));
		for(PxU32 i=0;i<nbAxes;i++)
		{
			dirs[i] = PxVec3(cosf(angle), sinf(angle), 0.0f);
			angle += angleStep;
		}

		float dpmin[nbAxes];
		float dpmax[nbAxes];
		for(PxU32 i=0;i<nbAxes;i++)
		{
			dpmin[i] = PX_MAX_F32;
			dpmax[i] = -PX_MAX_F32;
		}

		for(PxU32 i=0;i<nbNewContacts;i++)
		{
			const PxVec3& p = vertices[indices[i]];

			// Transform to 2D
			const PxVec3 p2d = rotT.transform(p);

			for(PxU32 j=0;j<nbAxes;j++)
			{
				const float dp = dot2D(dirs[j], p2d);
				dpmin[j] = physx::intrinsics::selectMin(dpmin[j], dp);
				dpmax[j] = physx::intrinsics::selectMax(dpmax[j], dp);
			}
		}

		PxU32 bestAxis = 0;
		float maxVariance = dpmax[0] - dpmin[0];
		for(PxU32 i=1;i<nbAxes;i++)
		{
			const float variance = dpmax[i] - dpmin[i];
			if(variance>maxVariance)
			{
				maxVariance = variance;
				bestAxis = i;
			}
		}

		const PxVec3 u = dirs[bestAxis];
		const PxVec3 v = PxVec3(-u.y, u.x, 0.0f);
		// PxVec3(1.0f, 0.0f, 0.0f)		=> PxVec3(0.0f, 1.0f, 0.0f)
		// PxVec3(0.0f, 1.0f, 0.0f)		=> PxVec3(-1.0f, 0.0f, 0.0f)
		// PxVec3(-1.0f, 1.0f, 0.0f)	=> PxVec3(-1.0f, -1.0f, 0.0f)
		// PxVec3(1.0f, 1.0f, 0.0f)		=> PxVec3(-1.0f, 1.0f, 0.0f)

		float dpminu = PX_MAX_F32;
		float dpmaxu = -PX_MAX_F32;
		float dpminv = PX_MAX_F32;
		float dpmaxv = -PX_MAX_F32;
		PxU32 indexMinU = 0;
		PxU32 indexMaxU = 0;
		PxU32 indexMinV = 0;
		PxU32 indexMaxV = 0;

		for(PxU32 i=0;i<nbNewContacts;i++)
		{
			const PxVec3& p = vertices[indices[i]];

			// Transform to 2D
			const PxVec3 p2d = rotT.transform(p);

			const float dpu = dot2D(u, p2d);
			const float dpv = dot2D(v, p2d);

			if(dpu<dpminu)
			{
				dpminu=dpu;
				indexMinU = i;
			}
			if(dpu>dpmaxu)
			{
				dpmaxu=dpu;
				indexMaxU = i;
			}

			if(dpv<dpminv)
			{
				dpminv=dpv;
				indexMinV = i;
			}
			if(dpv>dpmaxv)
			{
				dpmaxv=dpv;
				indexMaxV = i;
			}
		}

		if(indexMaxU == indexMinU)
			indexMaxU = 0xffffffff;
		if(indexMinV == indexMinU || indexMinV == indexMaxU)
			indexMinV = 0xffffffff;
		if(indexMaxV == indexMinU || indexMaxV == indexMaxU || indexMaxV == indexMinV)
			indexMaxV = 0xffffffff;
		
		PxU32 newCount = 0;
		for(PxU32 i=0;i<nbNewContacts;i++)
		{
			if(		i==indexMinU
				||	i==indexMaxU
				||	i==indexMinV
				||	i==indexMaxV)
			{
				ctcs[newCount++] = ctcs[i];
			}
		}
		contactBuffer.count = nbExistingContacts + newCount;
	}
}
#endif

// PT: please leave that function in the same translation unit as the calling code
/*static*/ PxMat33 Gu::findRotationMatrixFromZ(const PxVec3& to)
{
	PxMat33 result;

	const PxVec3 v = Ps::cross001(to);
	const PxReal e = to.z;
	const PxReal f = PxAbs(e);

	if(f <= 0.9999f)
	{
		// PT: please keep the normal case first for PS3 branch prediction

		// Normal case, to and from are not parallel or anti-parallel
		const PxReal h = 1.0f/(1.0f + e); /* optimization by Gottfried Chen */
		const PxReal hvx = h * v.x;
		const PxReal hvz = h * v.z;
		const PxReal hvxy = hvx * v.y;
		const PxReal hvxz = hvx * v.z;
		const PxReal hvyz = hvz * v.y;

		result(0,0) = e + hvx*v.x;
		result(0,1) = hvxy - v.z;
		result(0,2) = hvxz + v.y;

		result(1,0) = hvxy + v.z;
		result(1,1) = e + h*v.y*v.y;
		result(1,2) = hvyz - v.x;

		result(2,0) = hvxz - v.y;
		result(2,1) = hvyz + v.x;
		result(2,2) = e + hvz*v.z;
	}
	else
	{
		//Vectors almost parallel
		// PT: TODO: simplify code below
		PxVec3 from(0.0f, 0.0f, 1.0f);
		PxVec3 absFrom(0.0f, 0.0f, 1.0f);

		if(absFrom.x < absFrom.y)
		{
			if(absFrom.x < absFrom.z)
				absFrom = PxVec3(1.0f, 0.0f, 0.0f);
			else
				absFrom = PxVec3(0.0f, 0.0f, 1.0f);
		}
		else
		{
			if(absFrom.y < absFrom.z)
				absFrom = PxVec3(0.0f, 1.0f, 0.0f);
			else
				absFrom = PxVec3(0.0f, 0.0f, 1.0f);
		}

		PxVec3 u, v;
		u.x = absFrom.x - from.x; u.y = absFrom.y - from.y; u.z = absFrom.z - from.z;
		v.x = absFrom.x - to.x; v.y = absFrom.y - to.y; v.z = absFrom.z - to.z;

		const PxReal c1 = 2.0f / u.dot(u);
		const PxReal c2 = 2.0f / v.dot(v);
		const PxReal c3 = c1 * c2 * u.dot(v);

		for(int i = 0; i < 3; i++)
		{
			for(int j = 0; j < 3; j++)
			{
				result(i,j) = - c1*u[i]*u[j] - c2*v[i]*v[j] + c3*v[i]*u[j];
			}
			result(i,i) += 1.0f;
		}
	}
	return result;
}

// PT: using this specialized version avoids doing an explicit transpose, which reduces LHS
PX_FORCE_INLINE Cm::Matrix34 transformTranspose(const PxMat33& a, const Cm::Matrix34& b)
{
	return Cm::Matrix34(a.transformTranspose(b.base0), a.transformTranspose(b.base1), a.transformTranspose(b.base2), a.transformTranspose(b.base3));
}

// Helper function to transform x/y coordinate of point. 
PX_FORCE_INLINE void transform2D(float& x, float& y, const PxVec3& src, const Cm::Matrix34& mat)
{
	x = src.x * mat.base0.x + src.y * mat.base1.x + src.z * mat.base2.x + mat.base3.x;
	y = src.x * mat.base0.y + src.y * mat.base1.y + src.z * mat.base2.y + mat.base3.y;
}

// Helper function to transform x/y coordinate of point. Use transposed matrix
PX_FORCE_INLINE void transform2DT(float& x, float& y, const PxVec3& src, const PxMat33& mat)
{
	x = mat.column0.dot(src);
	y = mat.column1.dot(src);
}

// Helper function to transform z coordinate of point.
PX_FORCE_INLINE PxReal transformZ(const PxVec3& src, const Cm::Matrix34& mat)
{
	return src.x * mat.base0.z + src.y * mat.base1.z + src.z * mat.base2.z + mat.base3.z;
}

static void transformVertices(	float& minX, float& minY,
								float& maxX, float& maxY,
								float* PX_RESTRICT verts2D,
								PxU32 nb, const PxVec3* PX_RESTRICT vertices, const PxU8* PX_RESTRICT indices, const PxMat33& RotT)
{
	// PT: using local variables is important to reduce LHS.
	float lminX = FLT_MAX;
	float lminY = FLT_MAX;
	float lmaxX = -FLT_MAX;
	float lmaxY = -FLT_MAX;

	// PT: project points, compute min & max at the same time
	for(PxU32 i=0; i<nb; i++)
	{
		float x,y;
		transform2DT(x, y, vertices[indices[i]], RotT);
		lminX = physx::intrinsics::selectMin(lminX, x);
		lminY = physx::intrinsics::selectMin(lminY, y);
		lmaxX = physx::intrinsics::selectMax(lmaxX, x);
		lmaxY = physx::intrinsics::selectMax(lmaxY, y);
		verts2D[i*2+0] = x;
		verts2D[i*2+1] = y;
	}

	// DE702
		// Compute center of polygon
		const float cx = (lminX + lmaxX)*0.5f;
		const float cy = (lminY + lmaxY)*0.5f;
		// We'll scale the polygon by epsilon
		const float epsilon = 1.e-6f;
		// Adjust bounds to take care of scaling
		lminX -= epsilon;
		lminY -= epsilon;
		lmaxX += epsilon;
		lmaxY += epsilon;
	//~DE702

	// PT: relocate polygon to positive quadrant
	for(PxU32 i=0; i<nb; i++)
	{
		const float x = verts2D[i*2+0];
		const float y = verts2D[i*2+1];

		// PT: original code suffering from DE702 (relocation)
//		verts2D[i*2+0] = x - lminX;
//		verts2D[i*2+1] = y - lminY;

		// PT: theoretically proper DE702 fix (relocation + scaling)
		const float dx = x - cx;
		const float dy = y - cy;
//		const float coeff = epsilon * physx::intrinsics::recipSqrt(dx*dx+dy*dy);
//		verts2D[i*2+0] = x - lminX + dx * coeff;
//		verts2D[i*2+1] = y - lminY + dy * coeff;

		// PT: approximate but faster DE702 fix. We multiply by epsilon so this is good enough.
		verts2D[i*2+0] = x - lminX + physx::intrinsics::fsel(dx, epsilon, -epsilon);
		verts2D[i*2+1] = y - lminY + physx::intrinsics::fsel(dy, epsilon, -epsilon);
	}
	lmaxX -= lminX;
	lmaxY -= lminY;

	minX = lminX;
	minY = lminY;
	maxX = lmaxX;
	maxY = lmaxY;
}

//! Dedicated triangle version
PX_FORCE_INLINE bool pointInTriangle2D(	float px, float pz,
										float p0x, float p0z,
										float e10x, float e10z,
										float e20x, float e20z)
{ 
	const float a = e10x*e10x + e10z*e10z;
	const float b = e10x*e20x + e10z*e20z;
	const float c = e20x*e20x + e20z*e20z;
	const float ac_bb = (a*c)-(b*b);

	const float vpx = px - p0x;
	const float vpz = pz - p0z;

	const float d = vpx*e10x + vpz*e10z;
	const float e = vpx*e20x + vpz*e20z;

	const float x = (d*c) - (e*b);
	const float y = (e*a) - (d*b);
	const float z = x + y - ac_bb;

	// Same as: if(x>0.0f && y>0.0f && z<0.0f)	return TRUE;
	//			else							return FALSE;
//		return (( IR(z) & ~(IR(x)|IR(y)) ) & SIGN_BITMASK) != 0;
	if(x>0.0f && y>0.0f && z<0.0f)	return true;
	else							return false;
}


	enum OutCode
	{
		OUT_XP	= (1<<0),
		OUT_XN	= (1<<1),
		OUT_YP	= (1<<2),
		OUT_YN	= (1<<3),
	};

static
//PX_FORCE_INLINE
bool PointInConvexPolygon2D_OutCodes(const float* PX_RESTRICT pgon2D, PxU32 numVerts, const PxReal tx, const PxReal ty, const PxReal maxX, const PxReal maxY, PxU8& outCodes)
{
	// PT: 6144 LHS + 44599 FCMPS with this - 1048566 instr - 132.000 ticks
//	if(tx<0.0f || ty<0.0f)	return false;
//	if(tx>maxX || ty>maxY)	return false;

	PxU32 out = 0;
	if(tx<0.0f)	out |= OUT_XN;
	if(ty<0.0f)	out |= OUT_YN;
	if(tx>maxX)	out |= OUT_XP;
	if(ty>maxY)	out |= OUT_YP;
	outCodes = (PxU8)out;
	if(out)
		return false;

	// PT: 18203 LHS + 12288 FCMPs with this - 1092936 instr - 136.000 ticks
//	if(((int&)tx)&0x80000000)	return false;
//	if(((int&)ty)&0x80000000)	return false;
//	if(((int&)tx)>((int&)maxX))	return false;
//	if(((int&)ty)>((int&)maxY))	return false;

/*	// PT: 6144 LHS + 22072 FCMPs with this - 1111136 instr - 128.000 ticks
	const float cndt0 = physx::intrinsics::fsel(tx, 1.0f, 0.0f);
	const float cndt1 = physx::intrinsics::fsel(ty, 1.0f, 0.0f);
	const float cndt2 = physx::intrinsics::fsel(maxX-tx, 1.0f, 0.0f);
	const float cndt3 = physx::intrinsics::fsel(maxY-ty, 1.0f, 0.0f);
	const float cndt = cndt0 * cndt1 * cndt2 * cndt3;
	if(cndt==0.0f)
		return false;
*/

	// PT: 130.000 ticks for that one
//	const float cndt = physx::intrinsics::fsel(maxY-ty, physx::intrinsics::fsel(maxX-tx, physx::intrinsics::fsel(ty, physx::intrinsics::fsel(tx, 1.0f, 0.0f), 0.0f), 0.0f), 0.0f);
//	if(cndt==0.0f)
//		return false;

	if(numVerts==3)
		return pointInTriangle2D(	tx, ty,
									pgon2D[0], pgon2D[1],
									pgon2D[2] - pgon2D[0],
									pgon2D[3] - pgon2D[1],
									pgon2D[4] - pgon2D[0],
									pgon2D[5] - pgon2D[1]);

#define X 0
#define Y 1

	const PxReal* PX_RESTRICT vtx0 = pgon2D + (numVerts-1)*2;
	const PxReal* PX_RESTRICT vtx1 = pgon2D;

	const int* PX_RESTRICT ivtx0 = reinterpret_cast<const int* PX_RESTRICT>(vtx0);
	const int* PX_RESTRICT ivtx1 = reinterpret_cast<const int* PX_RESTRICT>(vtx1);
	//const int itx = (int&)tx;
	//const int ity = (int&)ty;
	const int ity = PX_SIR(ty);

	// get test bit for above/below X axis
	int yflag0 = ivtx0[Y] >= ity;

	int InsideFlag = 0;

	while(numVerts--)
	{
		const int yflag1 = ivtx1[Y] >= ity;
		if(yflag0 != yflag1)
		{
			const PxReal* PX_RESTRICT vtx0 = reinterpret_cast<const PxReal* PX_RESTRICT>(ivtx0);
			const PxReal* PX_RESTRICT vtx1 = reinterpret_cast<const PxReal* PX_RESTRICT>(ivtx1);
			if( ((vtx1[Y]-ty) * (vtx0[X]-vtx1[X]) > (vtx1[X]-tx) * (vtx0[Y]-vtx1[Y])) == yflag1 )
			{
				if(InsideFlag == 1) return false;

				InsideFlag++;
			}
		}
		yflag0 = yflag1;
		ivtx0 = ivtx1;
		ivtx1 += 2;
	}
#undef X
#undef Y

	return InsideFlag & 1;
}

// disabling as function not referenced (supress warning)
#if 0	

// Helper to check if point is inside polygon
static
//PX_FORCE_INLINE
bool PointInConvexPolygon2D(const float* PX_RESTRICT pgon2D, PxU32 numVerts, const PxReal tx, const PxReal ty, const PxReal maxX, const PxReal maxY)
{
	// PT: 6144 LHS + 44599 FCMPS with this - 1048566 instr - 132.000 ticks
//	if(tx<0.0f || ty<0.0f)	return false;
//	if(tx>maxX || ty>maxY)	return false;

	// PT: 18203 LHS + 12288 FCMPs with this - 1092936 instr - 136.000 ticks
//	if(((int&)tx)&0x80000000)	return false;
//	if(((int&)ty)&0x80000000)	return false;
//	if(((int&)tx)>((int&)maxX))	return false;
//	if(((int&)ty)>((int&)maxY))	return false;

	// PT: 6144 LHS + 22072 FCMPs with this - 1111136 instr - 128.000 ticks
	const float cndt0 = physx::intrinsics::fsel(tx, 1.0f, 0.0f);
	const float cndt1 = physx::intrinsics::fsel(ty, 1.0f, 0.0f);
	const float cndt2 = physx::intrinsics::fsel(maxX-tx, 1.0f, 0.0f);
	const float cndt3 = physx::intrinsics::fsel(maxY-ty, 1.0f, 0.0f);
	const float cndt = cndt0 * cndt1 * cndt2 * cndt3;
	if(cndt==0.0f)
		return false;

	// PT: 130.000 ticks for that one
//	const float cndt = physx::intrinsics::fsel(maxY-ty, physx::intrinsics::fsel(maxX-tx, physx::intrinsics::fsel(ty, physx::intrinsics::fsel(tx, 1.0f, 0.0f), 0.0f), 0.0f), 0.0f);
//	if(cndt==0.0f)
//		return false;

	if(numVerts==3)
		return pointInTriangle2D(	tx, ty,
									pgon2D[0], pgon2D[1],
									pgon2D[2] - pgon2D[0],
									pgon2D[3] - pgon2D[1],
									pgon2D[4] - pgon2D[0],
									pgon2D[5] - pgon2D[1]);

#define X 0
#define Y 1

	const PxReal* PX_RESTRICT vtx0 = pgon2D + (numVerts-1)*2;
	const PxReal* PX_RESTRICT vtx1 = pgon2D;

	const int* PX_RESTRICT ivtx0 = reinterpret_cast<const int* PX_RESTRICT>(vtx0);
	const int* PX_RESTRICT ivtx1 = reinterpret_cast<const int* PX_RESTRICT>(vtx1);
	//const int itx = (int&)tx;
	//const int ity = (int&)ty;
	const int ity = PX_SIR(ty);

	// get test bit for above/below X axis
	int yflag0 = ivtx0[Y] >= ity;

	int InsideFlag = 0;

	while(numVerts--)
	{
		const int yflag1 = ivtx1[Y] >= ity;
		if(yflag0 != yflag1)
		{
			const PxReal* PX_RESTRICT vtx0 = reinterpret_cast<const PxReal* PX_RESTRICT>(ivtx0);
			const PxReal* PX_RESTRICT vtx1 = reinterpret_cast<const PxReal* PX_RESTRICT>(ivtx1);
			if( ((vtx1[Y]-ty) * (vtx0[X]-vtx1[X]) > (vtx1[X]-tx) * (vtx0[Y]-vtx1[Y])) == yflag1 )
			{
				if(InsideFlag == 1) return false;

				InsideFlag++;
			}
		}
		yflag0 = yflag1;
		ivtx0 = ivtx1;
		ivtx1 += 2;
	}
#undef X
#undef Y

	return InsideFlag & 1;
}

#endif // 0

#ifdef DO_NOT_REMOVE_PLEASE
// Helper to check if point is inside polygon
static bool PointInConvexPolygon2D(const PxVec3* PX_RESTRICT pgon2D, PxU32 numVerts, const PxReal tx, const PxReal ty)
{
	if(numVerts==3)
		return pointInTriangle2D(tx, ty, pgon2D[0].x, pgon2D[0].y, pgon2D[1].x - pgon2D[0].x, pgon2D[1].y - pgon2D[0].y, pgon2D[2].x - pgon2D[0].x, pgon2D[2].y - pgon2D[0].y);

#define X 0
#define Y 1

	const PxReal* PX_RESTRICT vtx0 = &pgon2D[numVerts-1].x;
	// get test bit for above/below X axis
#ifdef _XBOX_	// PT: not working yet
	float yflag0 = physx::intrinsics::fsel(vtx0[Y] - ty, 1.0f, 0.0f);
//	int yflag0 = (int)physx::intrinsics::fsel(vtx0[Y] - ty, 0.0f, 1.0f);
#else
	int yflag0 = vtx0[Y] >= ty;
#endif
	const PxReal* PX_RESTRICT vtx1 = &pgon2D[0].x;

	int InsideFlag = 0;

	while(numVerts--)
	{
#ifdef _XBOX_	// PT: not working yet
		const float yflag1 = physx::intrinsics::fsel(vtx1[Y] - ty, 1.0f, 0.0f);
//		const int yflag1 = (int)physx::intrinsics::fsel(vtx1[Y] - ty, 0.0f, 1.0f);
#else
		const int yflag1 = vtx1[Y] >= ty;
#endif

		/* Check if endpoints straddle (are on opposite sides) of X axis
		* (i.e. the Y's differ); if so, +X ray could intersect this edge.
		* The old test also checked whether the endpoints are both to the
		* right or to the left of the test point. However, given the faster
		* intersection point computation used below, this test was found to
		* be a break-even proposition for most polygons and a loser for
		* triangles (where 50% or more of the edges which survive this test
		* will cross quadrants and so have to have the X intersection computed
		* anyway). I credit Joseph Samosky with inspiring me to try dropping
		* the "both left or both right" part of my code.
		*/
//#ifndef _XBOX	// PT: not working yet
		if(yflag0 != yflag1)
//#endif
		{
			/* Check intersection of pgon segment with +X ray.
			* Note if >= point's X; if so, the ray hits it.
			* The division operation is avoided for the ">=" test by checking
			* the sign of the first vertex wrto the test point; idea inspired
			* by Joseph Samosky's and Mark Haigh-Hutchinson's different
			* polygon inclusion tests.
			*/
#ifdef _XBOX_	// PT: not working yet
			const float cndt = physx::intrinsics::fsel( ((vtx1[X]-tx) * (vtx0[Y]-vtx1[Y])) - ((vtx1[Y]-ty) * (vtx0[X]-vtx1[X])), 0.0f, 1.0f);
			if( cndt == yflag1 )
#else
//			if( ((vtx1[Y]-ty) * (vtx0[X]-vtx1[X]) >= (vtx1[X]-tx) * (vtx0[Y]-vtx1[Y])) == yflag1 )
			// PT: this fixes the perfectly-aligned-quad-on-quad bug. I have to ask Eric about this though.
			if( ((vtx1[Y]-ty) * (vtx0[X]-vtx1[X]) > (vtx1[X]-tx) * (vtx0[Y]-vtx1[Y])) == yflag1 )
#endif
			{
				if(InsideFlag == 1) return false;

				InsideFlag++;
			}
		}
		yflag0 = yflag1;
		vtx0 = vtx1;
		vtx1 += 3;
	}
#undef X
#undef Y

	return InsideFlag & 1;
}
#endif

// Helper function to detect contact between two edges
PX_FORCE_INLINE bool EdgeEdgeContactSpecial(const PxVec3& v1, const PxPlane& plane, 
	const PxVec3& p1, const PxVec3& p2, const PxVec3& dir, const PxVec3& p3, const PxVec3& p4,
	PxReal& dist, PxVec3& ip, int i, int j, float coeff)
{
	const PxReal d3 = plane.distance(p3);
	PxReal temp = d3 * plane.distance(p4);
	if(temp > 0.0f)
		return false;

	// if colliding edge (p3,p4) and plane are parallel return no collision
	const PxVec3 v2 = (p4-p3);
	temp = plane.n.dot(v2);
	if(temp == 0.0f) // ### epsilon would be better
		return false;

	// compute intersection point of plane and colliding edge (p3,p4)
	ip = p3-v2*(d3/temp);

	// compute distance of intersection from line (ip, -dir) to line (p1,p2)
	dist =	(v1[i]*(ip[j]-p1[j])-v1[j]*(ip[i]-p1[i])) * coeff;
	if(dist < 0.0f)
		return false;

	// compute intersection point on edge (p1,p2) line
	ip -= dist*dir;

	// check if intersection point (ip) is between edge (p1,p2) vertices
	temp = (p1.x-ip.x)*(p2.x-ip.x)+(p1.y-ip.y)*(p2.y-ip.y)+(p1.z-ip.z)*(p2.z-ip.z);
	if(temp<0.0f)	
		return true;	// collision found

	return false; //no collision
}

	//! Absolute integer representation of a floating-point value
	#define AIR(x)					(IR(x)&0x7fffffff)

#ifdef SLOW
PX_FORCE_INLINE void closestAxis2(const PxVec3& v, PxU32& j, PxU32& k)
{
	// find largest 2D plane projection
#ifdef _XBOX
/*	const float delta = absX - absY;
	float max = __fself(delta, absX, absY);
	float m = __fself(delta, 0.0f, 1.0f);

	const float delta2 = max - absZ;
	max = __fself(delta2, max, absZ);
	m = __fself(delta2, m, 2.0f);*/


	const PxU32 absX = AIR(v.x);
	const PxF32 absY = AIR(v.y);
	const PxF32 absZ = AIR(v.z);

	if(absY > absX && absY > absZ)
	{
		//y biggest
		j = 2;
		k = 0;
	}
	else if(absZ > absX)
	{
		//z biggest
		j = 0;
		k = 1;
	}
	else
	{
		//x biggest
		j = 1;
		k = 2;
	}

#else
	const PxF32 absX = physx::intrinsics::abs(v.x);
	const PxF32 absY = physx::intrinsics::abs(v.y);
	const PxF32 absZ = physx::intrinsics::abs(v.z);

	if( absY > absX && absY > absZ)
	{
		//y biggest
		j = 2;
		k = 0;
	}
	else if(absZ > absX)
	{
		//z biggest
		j = 0;
		k = 1;
	}
	else
	{
		//x biggest
		j = 1;
		k = 2;
	}
#endif
}

	PX_FORCE_INLINE void closestAxis2(const PxVec3& v, PxU32& j, PxU32& k)
	{
		// find largest 2D plane projection
		const PxF32 absPx = physx::intrinsics::abs(v.x);
		const PxF32 absPy = physx::intrinsics::abs(v.y);
		const PxF32 absPz = physx::intrinsics::abs(v.z);
#ifdef _XBOX
		const float delta = absPx - absPy;

		float max = __fself(delta, absPx, absPy);
//		float m = __fself(delta, 0.0f, 1.0f);
		float m = __fself(delta, 1.0f, 2.0f);

		const float delta2 = max - absPz;
//		max = __fself(delta2, max, absPz);
//		m = __fself(delta2, m, 2.0f);
		m = __fself(delta2, m, 0.0f);

		j = PxU32(m);
		k=j+1;
		if(k>2)
			k=0;

//		j = Ps::getNextIndex3(i);
//		k = Ps::getNextIndex3(j);

//		return i;
#else
		PxU32 m = 0;	//x biggest axis
		j = 1;
		k = 2;
		if( absPy > absPx && absPy > absPz)
		{
			//y biggest
			j = 2;
			k = 0;
			m = 1;
		}
		else if(absPz > absPx)
		{
			//z biggest
			j = 0;
			k = 1;
			m = 2;
		}
//		return m;
#endif
	}
#endif


//This one can also handle 2 vertex 'polygons' (useful for capsule surface segments) and can shift the results before contact generation.
bool Gu::contactPolygonPolygonExt(	PxU32 numVerts0, const PxVec3* vertices0, const PxU8* indices0,	//polygon 0
									const Cm::Matrix34& world0, const PxPlane& localPlane0,	//xform of polygon 0, plane of polygon
									const PxMat33& rotT0,
									//
									PxU32 numVerts1, const PxVec3* PX_RESTRICT vertices1, const PxU8* PX_RESTRICT indices1,	//polygon 1
									const Cm::Matrix34& world1, const PxPlane& localPlane1,	//xform of polygon 1, plane of polygon
									const PxMat33& rotT1,
								//
									const PxVec3& worldSepAxis,	//world normal of separating plane - this is the world space normal of polygon0!!
									const Cm::Matrix34& transform0to1, const Cm::Matrix34& transform1to0,	//transforms between polygons
									PxU32 polyIndex0, PxU32 polyIndex1,	//feature indices for contact callback
									ContactBuffer& contactBuffer,
									bool flipNormal, const PxVec3& posShift, PxReal sepShift)	// shape order, result shift
{
//	ReportTicks reportTicks;

#ifdef _XBOX
	const int ild0 = IR(localPlane0.d) ^ 0x80000000;
	const int ild1 = IR(localPlane1.d) ^ 0x80000000;
#endif

	const PxVec3 n = flipNormal ? -worldSepAxis : worldSepAxis;

	PX_ASSERT(indices0 != NULL && indices1 != NULL);

	// - optimize "from to" computation
	// - do the raycast case && EE tests in same space as 2D case...
	// - project all edges at the same time ?
	PxU32 NumIn = 0;
	bool status = false;

	void* PX_RESTRICT stackMemory;
	{
		const PxU32 maxNumVert = PxMax(numVerts0, numVerts1);
		stackMemory = PxAlloca(maxNumVert * sizeof(PxVec3));
	}

	const PxU32 size0 = numVerts0 * sizeof(bool);
	bool* PX_RESTRICT flags0 = (bool*)PxAlloca(size0);
	PxU8* PX_RESTRICT outCodes0 = (PxU8*)PxAlloca(size0);
//	Ps::memZero(flags0, size0);
//	Ps::memZero(outCodes0, size0);

	const PxU32 size1 = numVerts1 * sizeof(bool);
	bool* PX_RESTRICT flags1 = (bool*)PxAlloca(size1);
	PxU8* PX_RESTRICT outCodes1 = (PxU8*)PxAlloca(size1);
//	Ps::memZero(flags1, size1);
//	Ps::memZero(outCodes1, size1);

#ifdef CONTACT_REDUCTION
	// We want to do contact reduction on newly created contacts, not on all the already existing ones...
	PxU32 nbExistingContacts = contactBuffer.count;
	PxU32 nbCurrentContacts=0;
	PxU8 indices[ContactBuffer::MAX_CONTACTS];
#endif

	if(1)
	{
		//polygon 1
		float* PX_RESTRICT verts2D = NULL;
		float minX=0, minY=0;
		float maxX=0, maxY=0;

		const PxVec3 localDir = -world1.rotateTranspose(worldSepAxis);		//contactNormal in hull1 space
																		//that's redundant, its equal to -localPlane1.d
		const Cm::Matrix34 t0to2D = transformTranspose(rotT1, transform0to1);	//transform from hull0 to RotT

		PxReal dn = localDir.dot(localPlane1.n);					//if the contactNormal == +-(normal of poly0) is NOT orthogonal to poly1 ...this is just to protect the division below.

		// PT: TODO: if "numVerts1>2" we may skip more
		if (numVerts1 > 2											//no need to test whether we're 'inside' ignore capsule segments and points
//		if(!(-1E-7 < dn && dn < 1E-7))
		&& dn >= 1E-7)	// PT: it should never be negative so this unique test is enough
		{
			dn = 1.0f / dn;
#ifdef _XBOX
			const float ld1 = FR(ild1);
#else
			const float ld1 = -localPlane1.d;						// PT: unavoidable "int-to-float" LHS here, so we only want to read it once!
#endif

			// Lazy-transform vertices
			if(!verts2D)
			{
				verts2D = reinterpret_cast<float* PX_RESTRICT>(stackMemory);
				//Project points
				transformVertices(
					minX, minY,
					maxX, maxY,
					verts2D, numVerts1, vertices1, indices1, rotT1);
			}

			for(PxU32 i=0; i < numVerts0; i++)							//for all vertices of poly0
			{
				const PxVec3& p = vertices0[indices0[i]];
				const float p0_z = transformZ(p, t0to2D);							//transform ith vertex of poly0 to RotT

				const PxVec3 pIn1 = transform0to1.transform(p);		//transform vertex to hull1 space, in which we have the poly1 vertices.

				const PxReal dd = (p0_z - ld1) * dn;			//(p0_z + localPlane1.d) is the depth of the vertex behind the triangle measured along the triangle's normal.
																	//we convert this to being measured along the 'contact normal' using the division.

//				if(dd < 0.0f)										//if the penetrating vertex will have a penetration along the contact normal: 
//				PX_ASSERT(dd <= 0.0f);		// PT: dn is always positive, so dd is always negative
				{
					float px, py;
					transform2DT(px, py, pIn1 - dd*localDir, rotT1);	//project vertex into poly1 plane along CONTACT NORMAL - not the polygon's normal. 

					const bool res = PointInConvexPolygon2D_OutCodes(verts2D, numVerts1, px-minX, py-minY, maxX, maxY, outCodes0[i]);
					flags0[i] = res;
					if(res)
					{
						NumIn++;

						if(p0_z < ld1)
						{
							status = true;	// PT: keep this first to avoid an LHS when leaving the function

							Gu::ContactPoint* PX_RESTRICT ctc = contactBuffer.contact();
							if(ctc)
							{
#ifdef CONTACT_REDUCTION
								indices[nbCurrentContacts++] = indices0[i];
#endif
								ctc->normal				= n;
								ctc->point				= world0.transform(p) + (flipNormal ? posShift : PxVec3(0.0f));
								ctc->separation			= dd + sepShift;
								ctc->internalFaceIndex0	= polyIndex0;
								ctc->internalFaceIndex1	= polyIndex1;
							}
						}
					}
				}
			}
		}
		else
		{
			PxMemZero(flags0, size0);
			PxMemZero(outCodes0, size0);
		}

		if(NumIn == numVerts0)
		{
			//All vertices0 are inside polygon 1
#ifdef CONTACT_REDUCTION
			ContactReductionAllIn(contactBuffer, nbExistingContacts, NumIn, rotT0, vertices0, indices);
#endif
			return status;
		}

#ifdef CONTACT_REDUCTION
		ContactReductionAllIn(contactBuffer, nbExistingContacts, NumIn, rotT0, vertices0, indices);
#endif

#ifdef CONTACT_REDUCTION
		nbExistingContacts = contactBuffer.count;
		nbCurrentContacts = 0;
#endif
		NumIn = 0;
		verts2D = NULL;

		//Polygon 0
		const Cm::Matrix34 t1to2D = transformTranspose(rotT0, transform1to0);

		if (numVerts0 > 2)											//no need to test whether we're 'inside' ignore capsule segments and points
		{
#ifdef _XBOX
		const float ld0 = FR(ild0);
#else
		const float ld0 = -localPlane0.d;						// PT: unavoidable "int-to-float" LHS here, so we only want to read it once!
#endif

		// Lazy-transform vertices
		if(!verts2D)
		{
			verts2D = reinterpret_cast<float* PX_RESTRICT>(stackMemory);
			//Project vertices
			transformVertices(
				minX, minY,
				maxX, maxY,
				verts2D, numVerts0, vertices0, indices0, rotT0);
		}

		for(PxU32 i=0; i < numVerts1; i++)
		{
			const PxVec3& p = vertices1[indices1[i]];

			float px, py;
			transform2D(px, py, p, t1to2D);

			const bool res = PointInConvexPolygon2D_OutCodes(verts2D, numVerts0, px-minX, py-minY, maxX, maxY, outCodes1[i]);
			flags1[i] = res;
			if(res)
			{
				NumIn++;

				const float pz = transformZ(p, t1to2D);
				if(pz < ld0)
				{
					status = true;	// PT: keep this first to avoid an LHS when leaving the function

					// PT: in theory, with this contact point we should use "worldSepAxis" as a contact normal.
					// However we want to output the same normal for all contact points not to break friction
					// patches!!! In theory again, it should be exactly the same since the contact point at
					// time of impact is supposed to be the same on both bodies. In practice however, and with
					// a depth-based engine, this is not the case. So the contact point here is not exactly
					// right, but preserving the friction patch seems more important.

					Gu::ContactPoint* PX_RESTRICT ctc = contactBuffer.contact();
					if(ctc)
					{
#ifdef CONTACT_REDUCTION
						indices[nbCurrentContacts++] = indices1[i];
#endif
						ctc->normal				= n;
						ctc->point				= world1.transform(p) + (flipNormal ? PxVec3(0.0f) : posShift);
						ctc->separation			= (pz - ld0) + sepShift;
						ctc->internalFaceIndex0	= polyIndex0;
						ctc->internalFaceIndex1	= polyIndex1;
					}
				}
			}
		}

		if(NumIn == numVerts1)
		{
			//all vertices 1 are inside polygon 0
#ifdef CONTACT_REDUCTION
			ContactReductionAllIn(contactBuffer, nbExistingContacts, NumIn, rotT1, vertices1, indices);
#endif
			return status;
		}
#ifdef CONTACT_REDUCTION
		ContactReductionAllIn(contactBuffer, nbExistingContacts, NumIn, rotT1, vertices1, indices);
#endif
		}
		else
		{
			PxMemZero(flags1, size1);
			PxMemZero(outCodes1, size1);
		}
	}

	//Edge/edge case
	//Calculation done in space 0
	PxVec3* PX_RESTRICT verts1in0 = (PxVec3* PX_RESTRICT)stackMemory;
	for(PxU32 i=0; i<numVerts1; i++)
	{
		verts1in0[i] = transform1to0.transform(vertices1[indices1[i]]);
	}

	if (numVerts0 >= 2 && numVerts1 >= 2)//useless if one of them is degenerate.
	for(PxU32 j=0; j<numVerts1; j++)
	{
		PxU32 j1 = j+1;
		if(j1 >= numVerts1) j1 = 0;

//		if(!(flags1[j] ^ flags1[j1]))
//			continue;
		if(flags1[j] && flags1[j1])
			continue;
		if(outCodes1[j]&outCodes1[j1])
			continue;

		const PxVec3& p0 = verts1in0[j];
		const PxVec3& p1 = verts1in0[j1];

//		gVisualizeLocalLine(vertices1[indices1[j]], vertices1[indices1[j1]], world1, callback.getManager());

		const PxVec3 v1 = p1-p0;
		const PxVec3 planeNormal = v1.cross(localPlane0.n);
		const PxPlane plane(planeNormal, -(planeNormal.dot(p0)));

		// find largest 2D plane projection
		PxU32 _i, _j;
		Ps::closestAxis(planeNormal, _i, _j);
//		closestAxis2(planeNormal, _i, _j);

		const PxReal coeff = 1.0f / (v1[_i]*localPlane0.n[_j]-v1[_j]*localPlane0.n[_i]);

		for(PxU32 i=0; i<numVerts0; i++)
		{
			PxU32 i1 = i+1;
			if(i1 >= numVerts0) i1 = 0;

//			if(!(flags0[i] ^ flags0[i1]))
//				continue;
			if(flags0[i] && flags0[i1])
				continue;
			if(outCodes0[i]&outCodes0[i1])
				continue;

			const PxVec3& p0b = vertices0[indices0[i]];
			const PxVec3& p1b = vertices0[indices0[i1]];

//			gVisualizeLocalLine(p0b, p1b, world0, callback.getManager());

			PxReal dist;
			PxVec3 p;

			if(EdgeEdgeContactSpecial(v1, plane, p0, p1, localPlane0.n, p0b, p1b, dist, p, _i, _j, coeff))
			{
				status = true;	// PT: keep this first to avoid an LHS when leaving the function
/*				p = world0.transform(p);

				//contacts are generated on the edges of polygon 1
				//we only have to shift the position of polygon 1 if flipNormal is false, because
				//in this case convex 0 gets passed as polygon 1, and it is convex 0 that was shifted.
				if (!flipNormal)
					p += posShift;

				contactBuffer.contact(p, n, -dist + sepShift, polyIndex0, polyIndex1, convexID);*/

				Gu::ContactPoint* PX_RESTRICT ctc = contactBuffer.contact();
				if(ctc)
				{
					ctc->normal				= n;
					ctc->point				= world0.transform(p) + (flipNormal ? PxVec3(0.0f) : posShift);
					ctc->separation			= -dist + sepShift;
					ctc->internalFaceIndex0	= polyIndex0;
					ctc->internalFaceIndex1	= polyIndex1;
				}
			}
		}
	}
	return status;
}
