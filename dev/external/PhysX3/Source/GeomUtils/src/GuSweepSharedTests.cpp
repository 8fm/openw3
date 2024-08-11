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

#include "GuSweepTests.h"
#include "GuHeightFieldUtil.h"
#include "GuEntityReport.h"
#include "CmScaling.h"
#include "GuConvexMesh.h"
#include "GuDistanceSegmentBox.h"
#include "GuDistanceSegmentSegment.h"
#include "GuDistancePointTriangle.h"
#include "GuIntersectionRayPlane.h"
#include "GuIntersectionRayCapsule.h"
#include "GuIntersectionRaySphere.h"
#include "GuIntersectionTriangleBox.h"
#include "GuIceSupport.h"
#include "GuConvexUtilsInternal.h"
#include "GuSPUHelpers.h"
#include "GuVecBox.h"
#include "GuVecSphere.h"
#include "GuVecCapsule.h"
#include "GuVecTriangle.h"
#include "GuVecConvexHull.h"
#include "GuGJKWrapper.h"
#include "GuSweepSharedTests.h"

//#define LOCAL_EPSILON 0.000001f
//#define LOCAL_EPSILON 0.00001f	// PT: this value makes the 'basicAngleTest' pass. Fails because of a ray almost parallel to a triangle

//#define NEW_SWEEP_CAPSULE_MESH	// PT: test to extrude the mesh on the fly
//#define CHECK_SWEEP_CAPSULE_TRIANGLES	// PT: please keep around for a while

using namespace physx;
using namespace Gu;
using namespace Cm;

//static const PxReal gGJKEpsilon = 0.005f;				//TODO: try simply using gEpsilon here.  Make sure that doesn't break behavior.
static const PxReal gEpsilon = .01f;





#ifndef USE_NEW_SWEEP_TEST
class SE_Vector2
{
public:
	//! Constructor
	PX_FORCE_INLINE				SE_Vector2()										{}
	PX_FORCE_INLINE				SE_Vector2(PxReal x_, PxReal y_) : x(x_), y(y_)		{}
	PX_FORCE_INLINE				SE_Vector2(const SE_Vector2& v) : x(v.x), y(v.y)	{}
	//! Destructor
	PX_FORCE_INLINE				~SE_Vector2()										{}

	PX_FORCE_INLINE	SE_Vector2	operator + (const SE_Vector2& inOther)		const	{ return SE_Vector2(x + inOther.x, y + inOther.y);		}
	PX_FORCE_INLINE	SE_Vector2	operator - (const SE_Vector2& inOther)		const	{ return SE_Vector2(x - inOther.x, y - inOther.y);		}
	PX_FORCE_INLINE	SE_Vector2	operator * (PxReal inConstant)				const	{ return SE_Vector2(x * inConstant, y * inConstant);	}
	PX_FORCE_INLINE	PxReal		Dot(const SE_Vector2& inOther)				const	{ return x * inOther.x + y * inOther.y;					}
	PX_FORCE_INLINE	PxReal		GetLengthSquared()							const	{ return x*x + y*y;										}

	PxReal		x, y;
};

class SE_Vector3
{
public:
	//! Constructor
	PX_FORCE_INLINE				SE_Vector3(PxReal inX = 0.0f, PxReal inY = 0.0f, PxReal inZ = 0.0f) : x(inX), y(inY), z(inZ)	{}
	PX_FORCE_INLINE				SE_Vector3(const SE_Vector2& inOther) : x(inOther.x), y(inOther.y), z(0.0f)					{}

	PX_FORCE_INLINE	SE_Vector3	operator + (const SE_Vector3 &inOther)	const	{ return SE_Vector3(x + inOther.x, y + inOther.y, z + inOther.z);		}
	PX_FORCE_INLINE	SE_Vector3	operator - (const SE_Vector3 &inOther)	const	{ return SE_Vector3(x - inOther.x, y - inOther.y, z - inOther.z);		}

	PX_FORCE_INLINE	SE_Vector3	operator * (PxReal inConstant)			const	{ return SE_Vector3(x * inConstant, y * inConstant, z * inConstant);	}

	PX_FORCE_INLINE	SE_Vector3	operator / (PxReal inConstant)			const	{ return SE_Vector3(x / inConstant, y / inConstant, z / inConstant);	}

	PX_FORCE_INLINE	PxReal		Dot(const SE_Vector3 &inOther)			const	{ return x * inOther.x + y * inOther.y + z * inOther.z;					}

	PX_FORCE_INLINE	PxReal		GetLengthSquared()						const	{ return x*x + y*y + z*z;												}

	PX_FORCE_INLINE	PxReal		GetLength()								const	{ return PxSqrt(GetLengthSquared());								}

	PX_FORCE_INLINE	SE_Vector3	Cross(const SE_Vector3 &inOther)		const
	{
		return SE_Vector3(y * inOther.z - z * inOther.y,
			z * inOther.x - x * inOther.z,
			x * inOther.y - y * inOther.x);
	}

	PX_FORCE_INLINE	SE_Vector3	GetPerpendicular() const
	{
		if (PxAbs(x) > PxAbs(y))
		{
			const PxReal len = PxRecipSqrt(x*x + z*z);		
			return SE_Vector3(z * len, 0.0f, -x * len);
		}
		else
		{
			const PxReal len = PxRecipSqrt(y*y + z*z);		
			return SE_Vector3(0.0f, z * len, -y * len);
		}
	}

	// Data
	PxReal			x, y, z;
};


// Simple plane
class SE_Plane
{
public:
	// Constructor
	PX_FORCE_INLINE SE_Plane() :
	  mConstant(0.0f)
	  {
	  }

	  // Get signed distance to inPoint
	  PX_FORCE_INLINE	PxReal GetSignedDistance(const SE_Vector3 &inPoint) const
	  {
		  return inPoint.Dot(mNormal) + mConstant;
	  }

	  // Get two vectors that together with mNormal form a basis for the plane
	  PX_FORCE_INLINE	void GetBasisVectors(SE_Vector3 &outU, SE_Vector3 &outV) const
	  { 
		  outU = mNormal.GetPerpendicular();
		  outV = mNormal.Cross(outU); 
	  } 

	  // Convert a point from plane space to world space (2D -> 3D)
	  PX_FORCE_INLINE	SE_Vector3 ConvertPlaneToWorld(const SE_Vector3 &inU, const SE_Vector3 &inV, const SE_Vector2 &inPoint) const
	  {
		  return SE_Vector3(inU * inPoint.x + inV * inPoint.y - mNormal * mConstant);
	  }

	  // Plane equation: mNormal.Dot(point) + mConstant == 0
	  SE_Vector3	mNormal;
	  PxReal		mConstant;
};


// Convert a point from world space to plane space (3D -> 2D)
static PX_FORCE_INLINE SE_Vector2 ConvertWorldToPlane(const SE_Vector3& inU, const SE_Vector3& inV, const SE_Vector3& inPoint)
{
	return SE_Vector2(inU.Dot(inPoint), inV.Dot(inPoint));
}
#endif

// Adapted from Gamasutra (Gomez article)
// Return true if r1 and r2 are real
static PX_FORCE_INLINE bool quadraticFormula(const PxReal a, const PxReal b, const PxReal c, PxReal& r1, PxReal& r2)
{
	const PxReal q = b*b - 4*a*c; 
	if(q>=0.0f)
	{
		PX_ASSERT(a!=0.0f);
		const PxReal sq = PxSqrt(q);
		const PxReal d = 1.0f / (2.0f*a);
		r1 = (-b + sq) * d;
		r2 = (-b - sq) * d;
		return true;//real roots
	}
	else
	{
		return false;//complex roots
	}
}

static bool sphereSphereSweep(	const PxReal ra, //radius of sphere A
								const PxVec3& A0, //previous position of sphere A
								const PxVec3& A1, //current position of sphere A
								const PxReal rb, //radius of sphere B
								const PxVec3& B0, //previous position of sphere B
								const PxVec3& B1, //current position of sphere B
								PxReal& u0, //normalized time of first collision
								PxReal& u1 //normalized time of second collision
								)
{
	const PxVec3 va = A1 - A0;
	const PxVec3 vb = B1 - B0;
	const PxVec3 AB = B0 - A0;
	const PxVec3 vab = vb - va;	// relative velocity (in normalized time)
	const PxReal rab = ra + rb;

	const PxReal a = vab.dot(vab);		//u*u coefficient
	const PxReal b = 2.0f*(vab.dot(AB));	//u coefficient

	const PxReal c = (AB.dot(AB)) - rab*rab;	//constant term

	//check if they're currently overlapping
	if(c<=0.0f || a==0.0f)
	{
		u0 = 0.0f;
		u1 = 0.0f;
		return true;
	}

	//check if they hit each other during the frame
	if(quadraticFormula(a, b, c, u0, u1))
	{
		if(u0>u1)
			TSwap(u0, u1);

		// u0<u1
//		if(u0<0.0f || u1>1.0f)	return false;
		if(u1<0.0f || u0>1.0f)	return false;

		return true;
	}
	return false;
}

static bool sweptSphereIntersect(const PxVec3& center0, PxReal radius0, const PxVec3& center1, PxReal radius1, const PxVec3& motion, PxReal& d, PxVec3& nrm)
{
	const PxVec3 movedCenter = center1 + motion;

	PxReal tmp;
	if(!sphereSphereSweep(radius0, center0, center0, radius1, center1, movedCenter, d, tmp))
		return false;

	// Compute normal
	// PT: if spheres initially overlap, the convention is that returned normal = -sweep direction
	if(d==0.0f)
		nrm = -motion;
	else
		nrm = (center1 + d * motion) - center0;
	nrm.normalize();
	return true;
}

static bool sweepSphereCapsule(const Gu::Sphere& sphere, const Gu::Capsule& lss, const PxVec3& dir, PxReal length, PxReal& d, PxVec3& ip, PxVec3& nrm, PxHitFlags hintFlags)
{
	const PxReal radiusSum = lss.radius + sphere.radius;

	if(!(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP))
	{
		// PT: test if shapes initially overlap
		if(Gu::distancePointSegmentSquared(lss.p0, lss.p1, sphere.center)<radiusSum*radiusSum)
		{
			d	= 0.0f;
			nrm	= -dir;
			ip	= sphere.center;	// PT: this is arbitrary
			return true;
		}
	}

	if(lss.p0 == lss.p1)
	{
		// Sphere vs. sphere
		if(sweptSphereIntersect(sphere.center, sphere.radius, lss.p0, lss.radius, -dir*length, d, nrm))
		{
			d*=length;
//				if(hintFlags & PxHitFlag::ePOSITION)	// PT: TODO
				ip = sphere.center + nrm * sphere.radius;
			return true;
		}
		return false;
	}

	// Create inflated capsule
	Gu::Capsule Inflated;
	Inflated.p0		= lss.p0;
	Inflated.p1		= lss.p1;
	Inflated.radius	= radiusSum;

	// Raycast against it
	PxReal s[2];
	PxU32 n = Gu::intersectRayCapsule(sphere.center, dir, Inflated, s);
	if(n)
	{
		PxReal t;
		if (n == 1)
			t = s[0];
		else
			t = (s[0] < s[1]) ? s[0]:s[1];

		if(t>=0.0f && t<=length)
		{
			d = t;

// PT: TODO:
//			const Ps::IntBool needsImpactPoint = hintFlags & PxHitFlag::ePOSITION;
//			if(needsImpactPoint || hintFlags & PxHitFlag::eNORMAL)
			{
				// Move capsule against sphere
				const PxVec3 tdir = t*dir;
				Inflated.p0 -= tdir;
				Inflated.p1 -= tdir;

				// Compute closest point between moved capsule & sphere
				Gu::distancePointSegmentSquared(Inflated, sphere.center, &t);
				Inflated.computePoint(ip, t);

				// Normal
				nrm = (ip - sphere.center);
				nrm.normalize();

//					if(needsImpactPoint)	// PT: TODO
					ip -= nrm * lss.radius;
			}
			return true;
		}
	}
	return false;
}

static void edgeEdgeDist(PxVec3& x, PxVec3& y,				// closest points
						 const PxVec3& p, const PxVec3& a,	// seg 1 origin, vector
						 const PxVec3& q, const PxVec3& b)	// seg 2 origin, vector
{
	const PxVec3 T = q - p;
	const PxReal ADotA = a.dot(a);
	const PxReal BDotB = b.dot(b);
	const PxReal ADotB = a.dot(b);
	const PxReal ADotT = a.dot(T);
	const PxReal BDotT = b.dot(T);

	// t parameterizes ray (p, a)
	// u parameterizes ray (q, b)

	// Compute t for the closest point on ray (p, a) to ray (q, b)
	const PxReal Denom = ADotA*BDotB - ADotB*ADotB;

	PxReal t;
	if(Denom!=0.0f)	
	{
		t = (ADotT*BDotB - BDotT*ADotB) / Denom;

		// Clamp result so t is on the segment (p, a)
				if(t<0.0f)	t = 0.0f;
		else	if(t>1.0f)	t = 1.0f;
	}
	else
	{
		t = 0.0f;
	}

	// find u for point on ray (q, b) closest to point at t
	PxReal u;
	if(BDotB!=0.0f)
	{
		u = (t*ADotB - BDotT) / BDotB;

		// if u is on segment (q, b), t and u correspond to closest points, otherwise, clamp u, recompute and clamp t
		if(u<0.0f)
		{
			u = 0.0f;
			if(ADotA!=0.0f)
			{
				t = ADotT / ADotA;

						if(t<0.0f)	t = 0.0f;
				else	if(t>1.0f)	t = 1.0f;
			}
			else
			{
				t = 0.0f;
			}
		}
		else if(u > 1.0f)
		{
			u = 1.0f;
			if(ADotA!=0.0f)
			{
				t = (ADotB + ADotT) / ADotA;

						if(t<0.0f)	t = 0.0f;
				else	if(t>1.0f)	t = 1.0f;
			}
			else
			{
				t = 0.0f;
			}
		}
	}
	else
	{
		u = 0.0f;

		if(ADotA!=0.0f)
		{
			t = ADotT / ADotA;

					if(t<0.0f)	t = 0.0f;
			else	if(t>1.0f)	t = 1.0f;
		}
		else
		{
			t = 0.0f;
		}
	}

	x = p + a * t;
	y = q + b * u;
}

static bool rayQuad(const PxVec3& orig, const PxVec3& dir, const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2, PxReal& t, PxReal& u, PxReal& v, bool cull)
{
	// Find vectors for two edges sharing vert0
	const PxVec3 edge1 = vert1 - vert0;
	const PxVec3 edge2 = vert2 - vert0;

	// Begin calculating determinant - also used to calculate U parameter
	const PxVec3 pvec = dir.cross(edge2);

	// If determinant is near zero, ray lies in plane of triangle
	const PxReal det = edge1.dot(pvec);

	if(cull)
	{
		if(det<LOCAL_EPSILON)						return false;

		// Calculate distance from vert0 to ray origin
		const PxVec3 tvec = orig - vert0;

		// Calculate U parameter and test bounds
		u = tvec.dot(pvec);
		if(u<0.0f || u>det)							return false;

		// Prepare to test V parameter
		const PxVec3 qvec = tvec.cross(edge1);

		// Calculate V parameter and test bounds
		v = dir.dot(qvec);
		if(v<0.0f || v>det)							return false;

		// Calculate t, scale parameters, ray intersects triangle
		t = edge2.dot(qvec);
		const PxReal oneOverDet = 1.0f / det;
		t *= oneOverDet;
		u *= oneOverDet;
		v *= oneOverDet;
	}
	else
	{
		// the non-culling branch
		if(det>-LOCAL_EPSILON && det<LOCAL_EPSILON)	return false;
		const PxReal oneOverDet = 1.0f / det;

		// Calculate distance from vert0 to ray origin
		const PxVec3 tvec = orig - vert0;

		// Calculate U parameter and test bounds
		u = (tvec.dot(pvec)) * oneOverDet;
		if(u<0.0f || u>1.0f)						return false;

		// prepare to test V parameter
		const PxVec3 qvec = tvec.cross(edge1);

		// Calculate V parameter and test bounds
		v = (dir.dot(qvec)) * oneOverDet;
		if(v<0.0f || v>1.0f)						return false;

		// Calculate t, ray intersects triangle
		t = (edge2.dot(qvec)) * oneOverDet;
	}
	return true;
}

static PxU32 computeSweepConvexPlane(const PxConvexMeshGeometry& convexGeom, Gu::ConvexHullData* hullData, const PxU32& nbPolys, const PxTransform& pose, const PxVec3& impact_, const PxVec3& unitDir)
{
	PX_ASSERT(nbPolys);

	const PxVec3 impact = impact_ - unitDir * gEpsilon;

	const PxVec3 localPoint = pose.transformInv(impact);
	const PxVec3 localDir = pose.rotateInv(unitDir);

	const Cm::FastVertex2ShapeScaling scaling(convexGeom.scale);

	PxU32 minIndex = 0;
	PxReal minD = PX_MAX_REAL;
	for(PxU32 j=0; j<nbPolys; j++)
	{
		const PxPlane& pl = hullData->mPolygons[j].mPlane;

		PxPlane plane;
		scaling.transformPlaneToShapeSpace(pl.n, pl.d, plane.n, plane.d);

		PxReal d = plane.distance(localPoint);
		if(d<0.0f)
			continue;

		const PxReal tweak = plane.n.dot(localDir) * gEpsilon;
		d += tweak;

		if(d<minD)
		{
			minIndex = j;
			minD = d;
		}
	}

	return minIndex;
}

static bool sweepCapsuleCapsule(const Gu::Capsule& lss0, const Gu::Capsule& lss1, const PxVec3& dir, PxReal length, PxReal& min_dist, PxVec3& ip, PxVec3& normal, PxU32 inHintFlags, PxU16& outHintFlags)
{
	const PxReal radiusSum = lss0.radius + lss1.radius;
	const PxVec3 center = lss1.computeCenter();

	if(!(inHintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP))
	{
		// PT: test if shapes initially overlap

		// PT: using the same codepath for spheres and capsules was a bad idea. The segment-segment distance
		// function doesn't work for degenerate capsules so we need to test all combinations here anyway.
		bool initialOverlapStatus;
		if(lss0.p0==lss0.p1)
			initialOverlapStatus = Gu::distancePointSegmentSquared(lss1, lss0.p0)<radiusSum*radiusSum;
		else if(lss1.p0==lss1.p1)
			initialOverlapStatus = Gu::distancePointSegmentSquared(lss0, lss1.p0)<radiusSum*radiusSum;
		else
			initialOverlapStatus = Gu::distanceSegmentSegmentSquared(lss0, lss1)<radiusSum*radiusSum;
			
		if(initialOverlapStatus)
		{
			min_dist	= 0.0f;
			normal		= -dir;
			ip			= center;	// PT: this is arbitrary
			outHintFlags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
			return true;
		}
	}

	// 1. Extrude lss0 by lss1's length
	// 2. Inflate extruded shape by lss1's radius
	// 3. Raycast against resulting shape

	// Extrusion dir = capsule segment
	const PxVec3 D = (lss1.p1 - lss1.p0)*0.5f;

	const PxVec3 p0 = lss0.p0 - D;
	const PxVec3 p1 = lss0.p1 - D;
	const PxVec3 p0b = lss0.p0 + D;
	const PxVec3 p1b = lss0.p1 + D;

	PxTriangle T(p0b, p1b, p1);
	PxVec3 Normal;
	T.normal(Normal);

	PxReal MinDist = length;
	bool Status = false;

	PxVec3 pa,pb,pc;
	if((Normal.dot(dir)) >= 0)  // Same direction
	{
		Normal *= radiusSum;
		pc = p0 - Normal;
		pa = p1 - Normal;
		pb = p1b - Normal;
	}
	else
	{
		Normal *= radiusSum;
		pb = p0 + Normal;
		pa = p1 + Normal;
		pc = p1b + Normal;
	}
	PxReal t, u, v;
	if(rayQuad(center, dir, pa, pb, pc, t, u, v, true) && t>=0.0f && t<MinDist)
	{
		MinDist = t;
		Status = true;
	}

	// PT: optimization: if we hit one of the quad we can't possibly get a better hit, so let's skip all
	// the remaining tests!
	if(!Status)
	{
		Gu::Capsule Caps[4];
		Caps[0] = Gu::Capsule(Gu::Segment(p0, p1), radiusSum);
		Caps[1] = Gu::Capsule(Gu::Segment(p1, p1b), radiusSum);
		Caps[2] = Gu::Capsule(Gu::Segment(p1b, p0b), radiusSum);
		Caps[3] = Gu::Capsule(Gu::Segment(p0, p0b), radiusSum);

		// ### a lot of ray-sphere tests could be factored out of the ray-capsule tests...
		for(PxU32 i=0;i<4;i++)
		{
			PxReal s[2];
			PxU32 n = Gu::intersectRayCapsule(center, dir, Caps[i], s);
			if(n)
			{
				PxReal t;
				if (n == 1)
					t = s[0];
				else
					t = (s[0] < s[1]) ? s[0]:s[1];

				if(t>=0.0f && t<= MinDist)
				{
					MinDist = t;
					Status = true;
				}
			}
		}
	}

	if(Status)
	{
		outHintFlags = PxHitFlag::eDISTANCE;
		if(inHintFlags & (PxU32)(PxHitFlag::ePOSITION|PxHitFlag::eNORMAL))
		{
			if(1)
			{
				const PxVec3 p00 = lss0.p0 - MinDist * dir;
				const PxVec3 p01 = lss0.p1 - MinDist * dir;
//				const PxVec3 p10 = lss1.p0;// - MinDist * dir;
//				const PxVec3 p11 = lss1.p1;// - MinDist * dir;

				const PxVec3 edge0 = p01 - p00;
				const PxVec3 edge1 = lss1.p1 - lss1.p0;

				PxVec3 x, y;
				edgeEdgeDist(x, y, p00, edge0, lss1.p0, edge1);

				if(inHintFlags & PxHitFlag::eNORMAL)
				{
					normal = (x - y);
					const float epsilon = 0.001f;
					if(normal.normalize()<epsilon)
					{
						// PT: happens when radiuses are zero
						normal = edge1.cross(edge0);
						if(normal.normalize()<epsilon)
						{
							// PT: happens when edges are parallel
							edgeEdgeDist(x, y, lss0.p0, lss0.p1 - lss0.p0, lss1.p0, edge1);
							normal = (x - y);
							normal.normalize();
						}
					}

					outHintFlags |= PxHitFlag::eNORMAL;
				}

				if(inHintFlags & PxHitFlag::ePOSITION)
				{
					ip = (lss1.radius*x + lss0.radius*y)/(lss0.radius+lss1.radius);
					outHintFlags |= PxHitFlag::ePOSITION;
				}
			}
			else
			{
				// Old CCT code
				PxVec3 x, y;
				edgeEdgeDist(x, y, lss0.p0, lss0.p1-lss0.p0, lss1.p0, lss1.p1-lss1.p0);

				if(inHintFlags & PxHitFlag::eNORMAL)
				{
					normal = (x - y);
					normal.normalize();
					outHintFlags |= PxHitFlag::eNORMAL;
				}

				if(inHintFlags & PxHitFlag::ePOSITION)
				{
					ip = (x+y)*0.5f;
					outHintFlags |= PxHitFlag::ePOSITION;
				}
			}
		}
		min_dist = MinDist;
	}
	return Status;
}

//
//                     point
//                      o
//                   __/|
//                __/ / |
//             __/   /  |(B)
//          __/  (A)/   |
//       __/       /    |                dir
//  p0 o/---------o---------------o--    -->
//                t (t<=fT)       t (t>fT)
//                return (A)^2    return (B)^2
//
//     |<-------------->|
//             fT
//
//
static PX_FORCE_INLINE PxReal squareDistance(const PxVec3& p0, const PxVec3& dir, PxReal t, const PxVec3& point)
{
	PxVec3 Diff = point - p0;
/*	const PxReal fT = (Diff.dot(dir));
	if(fT>0.0f)
	{
		if(fT>=t)
			Diff -= dir*t;	// Take travel distance of point p0 into account (shortens the distance)
		else
			Diff -= fT*dir;
	}*/

	PxReal fT = (Diff.dot(dir));
	fT = physx::intrinsics::selectMax(fT, 0.0f);
	fT = physx::intrinsics::selectMin(fT, t);
	Diff -= fT*dir;

	return Diff.magnitudeSquared();
}

#if __SPU__ // disabled inlining on SPU to save space
static bool coarseCulling(const PxVec3& center, const PxVec3& dir, PxReal t, PxReal radius, const PxTriangle& tri)
#else
static PX_FORCE_INLINE bool coarseCulling(const PxVec3& center, const PxVec3& dir, PxReal t, PxReal radius, const PxTriangle& tri)
#endif
{
	// ### could be precomputed
	const PxVec3 TriCenter = (tri.verts[0] + tri.verts[1] + tri.verts[2]) * (1.0f/3.0f);

	// PT: distance between the triangle center and the swept path (an LSS)
	// Same as: Gu::distancePointSegmentSquared(center, center+dir*t, TriCenter);
	PxReal d = PxSqrt(squareDistance(center, dir, t, TriCenter)) - radius - 0.0001f;

	if (d < 0.0f)	// The triangle center lies inside the swept sphere
		return true;

	d*=d;

	// ### distances could be precomputed
/*	if(d <= (TriCenter-tri.verts[0]).magnitudeSquared())
	return true;
	if(d <= (TriCenter-tri.verts[1]).magnitudeSquared())
		return true;
	if(d <= (TriCenter-tri.verts[2]).magnitudeSquared())
		return true;
	return false;*/
	const PxReal d0 = (TriCenter-tri.verts[0]).magnitudeSquared();
	const PxReal d1 = (TriCenter-tri.verts[1]).magnitudeSquared();
	const PxReal d2 = (TriCenter-tri.verts[2]).magnitudeSquared();
	PxReal triRadius = physx::intrinsics::selectMax(d0, d1);
	triRadius = physx::intrinsics::selectMax(triRadius, d2);
	if(d <= triRadius)
		return true;
	return false;
}



static PX_FORCE_INLINE void computeTriData(const PxTriangle& tri, PxVec3& normal, PxReal& magnitude, PxReal& area)
{
	tri.denormalizedNormal(normal);

	magnitude = normal.magnitude();

	area = magnitude * 0.5f;
}

//#ifdef USE_NEW_SWEEP_TEST
// PT: special version computing (u,v) even when the ray misses the tri
static PX_FORCE_INLINE PxU32 rayTriSpecial(const PxVec3& orig, const PxVec3& dir, const PxVec3& vert0, const PxVec3& vert1, const PxVec3& vert2, PxReal& t, PxReal& u, PxReal& v)
{
	// Find vectors for two edges sharing vert0
	const PxVec3 edge1 = vert1 - vert0;
	const PxVec3 edge2 = vert2 - vert0;

	// Begin calculating determinant - also used to calculate U parameter
	const PxVec3 pvec = dir.cross(edge2);

	// If determinant is near zero, ray lies in plane of triangle
	const PxReal det = edge1.dot(pvec);

	// the non-culling branch
	if(det>-LOCAL_EPSILON && det<LOCAL_EPSILON)
		return 0;
	const PxReal OneOverDet = 1.0f / det;

	// Calculate distance from vert0 to ray origin
	const PxVec3 tvec = orig - vert0;

	// Calculate U parameter
	u = (tvec.dot(pvec)) * OneOverDet;

	// prepare to test V parameter
	const PxVec3 qvec = tvec.cross(edge1);

	// Calculate V parameter
	v = (dir.dot(qvec)) * OneOverDet;

	if(u<0.0f || u>1.0f)
		return 1;
	if(v<0.0f || u+v>1.0f)
		return 1;

	// Calculate t, ray intersects triangle
	t = (edge2.dot(qvec)) * OneOverDet;

	return 2;
}

// Returns true if sphere can be tested against triangle vertex, false if edge test should be performed
//
// Uses a conservative approach to work for "sliver triangles" (long & thin) as well.
#if __SPU__ // disable inlining on SPU to save space
static bool edgeOrVertexTest(const PxVec3& planeIntersectPoint, const PxTriangle& tri, PxU32 vertIntersectCandidate, PxU32 vert0, PxU32 vert1, PxU32& secondEdgeVert)
#else
static PX_FORCE_INLINE bool edgeOrVertexTest(const PxVec3& planeIntersectPoint, const PxTriangle& tri, PxU32 vertIntersectCandidate, PxU32 vert0, PxU32 vert1, PxU32& secondEdgeVert)
#endif
{
	const PxVec3 edge0 = tri.verts[vertIntersectCandidate] - tri.verts[vert0];
	const PxReal edge0LengthSqr = edge0.dot(edge0);

	PxVec3 diff = planeIntersectPoint - tri.verts[vert0];

	if (edge0.dot(diff) < edge0LengthSqr)  // If the squared edge length is used for comparison, the edge vector does not need to be normalized
	{
		secondEdgeVert = vert0;
		return false;
	}

	const PxVec3 edge1 = tri.verts[vertIntersectCandidate] - tri.verts[vert1];
	const PxReal edge1LengthSqr = edge1.dot(edge1);

	diff = planeIntersectPoint - tri.verts[vert1];

	if (edge1.dot(diff) < edge1LengthSqr)
	{
		secondEdgeVert = vert1;
		return false;
	}

	return true;
}

static bool sweepTriSphere(const PxTriangle& tri, const PxVec3& normal, const PxVec3& center, PxReal radius, const PxVec3& dir, PxReal& min_dist)
{
	// Ok, this new version is now faster than the original code. Needs more testing though.

	#define INTERSECT_POINT (tri.verts[1]*u) + (tri.verts[2]*v) + (tri.verts[0] * (1.0f-u-v))

	PxReal u,v;
	if(1)
	{
		PxVec3 R = normal * radius;
		if(dir.dot(R) >= 0.0f)
			R = -R;

		// The first point of the sphere to hit the triangle plane is the point of the sphere nearest to
		// the triangle plane. Hence, we use center - (normal*radius) below.

		// PT: casting against the extruded triangle in direction R is the same as casting from a ray moved by -R
		PxReal t;
//		int r = rayTriSpecial(center, dir, tri.mVerts[0]+R, tri.mVerts[1]+R, tri.mVerts[2]+R, t, u, v);
		int r = rayTriSpecial(center-R, dir, tri.verts[0], tri.verts[1], tri.verts[2], t, u, v);
		if(!r)	return false;
		if(r==2)
		{
			if(t<0.0f)	return false;
			min_dist = t;
			return true;
		}
	}

	//
	// Let's do some art!
	//
	// The triangle gets divided into the following areas (based on the barycentric coordinates (u,v)):
	//
	//               \   A0    /
	//                 \      /
	//                   \   /
	//                     \/ 0
	//            A02      *      A01
	//   u /              /   \          \ v
	//    *              /      \         *
	//                  /         \						.
	//               2 /            \ 1
	//          ------*--------------*-------
	//               /                 \				.
	//        A2    /        A12         \   A1
	//
	//
	// Based on the area where the computed triangle plane intersection point lies in, a different sweep test will be applied.
	//
	// A) A01, A02, A12  : Test sphere against the corresponding edge
	// B) A0, A1, A2     : Test sphere against the corresponding vertex
	//
	// Unfortunately, B) does not work for long, thin triangles. Hence there is some extra code which does a conservative check and
	// switches to edge tests if necessary.
	//

	bool TestSphere;
	PxU32 e0,e1;
	if(u<0.0f)
	{
		if(v<0.0f)
		{
			// 0 or 0-1 or 0-2
			e0 = 0;
			PxVec3 intersectPoint = INTERSECT_POINT;
			TestSphere = edgeOrVertexTest(intersectPoint, tri, 0, 1, 2, e1);
		}
		else if(u+v>1.0f)
		{
			// 2 or 2-0 or 2-1
			e0 = 2;
			PxVec3 intersectPoint = INTERSECT_POINT;
			TestSphere = edgeOrVertexTest(intersectPoint, tri, 2, 0, 1, e1);
		}
		else
		{
			// 0-2
			TestSphere = false;
			e0 = 0;
			e1 = 2;
		}
	}
	else
	{
		if(v<0.0f)
		{
			if(u+v>1.0f)
			{
				// 1 or 1-0 or 1-2
				e0 = 1;
				PxVec3 intersectPoint = INTERSECT_POINT;
				TestSphere = edgeOrVertexTest(intersectPoint, tri, 1, 0, 2, e1);
			}
			else
			{
				// 0-1
				TestSphere = false;
				e0 = 0;
				e1 = 1;
			}
		}
		else
		{
			PX_ASSERT(u+v>=1.0f);	// Else hit triangle
			// 1-2
			TestSphere = false;
			e0 = 1;
			e1 = 2;
		}
	}

	if(TestSphere)
	{
		PxReal t;
//		if(Gu::intersectRaySphere(center, dir, min_dist*2.0f, tri.verts[e0], radius, t))
		if(Gu::intersectRaySphere(center, dir, PX_MAX_F32, tri.verts[e0], radius, t))
		{
			min_dist = t;
			return true;
		}
	}
	else
	{
		const Gu::Capsule capsule(Gu::Segment(tri.verts[e0], tri.verts[e1]), radius);

		PxReal s[2];
		PxU32 n = Gu::intersectRayCapsule(center, dir, capsule, s);
		if(n)
		{
			PxReal t;
			if (n == 1)	t = s[0];
			else t = (s[0] < s[1]) ? s[0]:s[1];

			if(t>=0.0f/* && t<MinDist*/)
			{
				min_dist = t;
				return true;
			}
		}
	}
	return false;
}
//#endif

#ifndef USE_NEW_SWEEP_TEST
// Test intersection between a plane inPlane and a swept sphere with radius inRadius moving from inBegin to inBegin + inDelta
// If there is an intersection the function returns true and the intersection range is from 
// inBegin + outT1 * inDelta to inBegin + outT2 * inDelta
// PT: this function is only used once so we'd better inline it
static PX_FORCE_INLINE bool planeSweptSphereIntersect(const SE_Plane& inPlane, const SE_Vector3& inBegin, const SE_Vector3& inDelta, PxReal inRadius, PxReal& outT1, PxReal& outT2)
{
	// If the center of the sphere moves like: center = inBegin + t * inDelta for t e [0, 1]
	// then the sphere intersects the plane if: -R <= distance plane to center <= R
	const PxReal n_dot_d = inPlane.mNormal.Dot(inDelta);
	const PxReal dist_to_b = inPlane.GetSignedDistance(inBegin);
	if (n_dot_d == 0.0f)
	{
		// The sphere is moving nearly parallel to the plane, check if the distance
		// is smaller than the radius
		if (PxAbs(dist_to_b) > inRadius)
			return false;

		// Intersection on the entire range
		outT1 = 0.0f;
		outT2 = 1.0f;
	}
	else
	{
		// Determine interval of intersection
		const PxReal over = 1.0f / n_dot_d;
		outT1 = (inRadius - dist_to_b) * over;
		outT2 = (-inRadius - dist_to_b) * over;

		// Order the results
		if (outT1 > outT2)
		{
			PxReal tmp = outT1;
			outT1 = outT2;
			outT2 = tmp;
		}

		// Early out if no hit possible
		if (outT1 > 1.0f || outT2 < 0.0f)
			return false;

		// Clamp it to the range [0, 1], the range of the swept sphere
		if (outT1 < 0.0f) outT1 = 0.0f;
		if (outT2 > 1.0f) outT2 = 1.0f;
	}
	return true;
}

// Check if a polygon contains inPoint, returns true when it does
// PT: this function is only used once so we'd better inline it
static PX_FORCE_INLINE bool polygonContains(const SE_Vector2* inVertices, PxU32 inNumVertices, const SE_Vector2& inPoint)
{
	// Loop through edges
	for (const SE_Vector2 *v1 = inVertices, *v2 = inVertices + inNumVertices - 1; v1 < inVertices + inNumVertices; v2 = v1, ++v1)
	{
		// If the point is outside this edge, the point is outside the polygon
		SE_Vector2 v1_v2 = *v2 - *v1;
		SE_Vector2 v1_point = inPoint - *v1;
		if (v1_v2.x * v1_point.y - v1_point.x * v1_v2.y > 0.0f)
			return false;
	}
	return true;
}

// Check if circle at inCenter with radius^2 = inRadiusSq intersects with a polygon.
// Function returns true when it does and the intersection point is in outPoint
// PT: this function is only used once so we'd better inline it
static PX_FORCE_INLINE bool polygonCircleIntersect(const SE_Vector2* inVertices, PxU32 inNumVertices, const SE_Vector2& inCenter, PxReal inRadiusSq, SE_Vector2& outPoint)
{
	// Check if the center is inside the polygon
	if(polygonContains(inVertices, inNumVertices, inCenter))
	{
		outPoint = inCenter;
		return true;
	}

	// Loop through edges
	bool collision = false;
	for(const SE_Vector2 *v1 = inVertices, *v2 = inVertices + inNumVertices - 1; v1 < inVertices + inNumVertices; v2 = v1, ++v1)
	{
		// Get fraction where the closest point to this edge occurs
		SE_Vector2 v1_v2 = *v2 - *v1;
		SE_Vector2 v1_center = inCenter - *v1;
		const PxReal fraction = v1_center.Dot(v1_v2);
		if (fraction < 0.0f)
		{
			// Closest point is v1
			const PxReal dist_sq = v1_center.GetLengthSquared();
			if (dist_sq <= inRadiusSq)
			{
				collision = true;
				outPoint = *v1;
				inRadiusSq = dist_sq;
			}
		}
		else 
		{
			const PxReal v1_v2_len_sq = v1_v2.GetLengthSquared();
			if (fraction <= v1_v2_len_sq)
			{
				// Closest point is on line segment
				const SE_Vector2 point = *v1 + v1_v2 * (fraction / v1_v2_len_sq);
				const PxReal dist_sq = (point - inCenter).GetLengthSquared();
				if (dist_sq <= inRadiusSq)
				{
					collision = true;
					outPoint = point;
					inRadiusSq = dist_sq;
				}
			}
		}
	}
	return collision;
}

// Solve the equation inA * x^2 + inB * x + inC == 0 for the lowest x in [0, inUpperBound].
// Returns true if there is such a solution and returns the solution in outX
// SDS: Fixed implementation for xbox, since assumptions on NAN/INF compares failed.
static PX_FORCE_INLINE bool findLowestRootInInterval(PxReal inA, PxReal inB, PxReal inC, PxReal inUpperBound, PxReal& outX)
{
	// Check if a solution exists
	const PxReal determinant = inB * inB - 4.0f * inA * inC;
	if(determinant < 0.0f)
		return false;

	PxReal x;
	const bool nullA = (inA == 0.0f);
	const bool nullDet = (determinant == 0.0f);
//	if(determinant != 0.0f && inA != 0.0f)
	if(!nullDet && !nullA)
	{
		// The standard way of doing this is by computing: x = (-b +/- Sqrt(b^2 - 4 a c)) / 2 a 
		// is not numerically stable when a is close to zero. 
		// Solve the equation according to "Numerical Recipies in C" paragraph 5.6
		const PxReal q = -0.5f * (inB + (inB < 0.0f? -1.0f : 1.0f) * PxSqrt(determinant));

		// Order the results
		x = PxMin(q / inA, inC / q);
	}
//	else if(inA == 0.0f)
	else if(nullA)
	{
		if(inB!=0.0f)
			x = -inC / inB;
		else 
			return false; //in case inC == 0, outX is undefined.
	}
//	else if(determinant == 0.0f)
	else //if(nullDet)
	{
		PX_ASSERT(nullDet);
		x = -inB / (2.0f * inA);
	}

	// Check if x1 is a solution
	if(x >= 0.0f && x <= inUpperBound)
	{
		outX = x;
		return true;
	}

	return false;
}

// Checks intersection between a polygon an moving circle at inBegin + t * inDelta with radius^2 = inA * t^2 + inB * t + inC, t in [0, 1]
// Returns true when it does and returns the intersection position in outPoint and the intersection fraction (value for t) in outFraction
static bool sweptCircleEdgeVertexIntersect(const SE_Vector2* inVertices, int inNumVertices, const SE_Vector2& inBegin, const SE_Vector2& inDelta, PxReal inA, PxReal inB, PxReal inC, SE_Vector2* outPoint, PxReal* outFraction)
{
	// Loop through edges
	PxReal upper_bound = 1.0f;
	bool collision = false;
	for (const SE_Vector2 *v1 = inVertices, *v2 = inVertices + inNumVertices - 1; v1 < inVertices + inNumVertices; v2 = v1, ++v1)
	{
		PxReal t;

		// Check if circle hits the vertex
		const SE_Vector2 bv1 = *v1 - inBegin;
		const PxReal a1 = inA - inDelta.GetLengthSquared();
		const PxReal b1 = inB + 2.0f * inDelta.Dot(bv1);
		const PxReal c1 = inC - bv1.GetLengthSquared();
		if(findLowestRootInInterval(a1, b1, c1, upper_bound, t))
//		if(findLowestRootInInterval(a1, b1, c1, upper_bound, t) && t<=1.0f)
//		if(findLowestRootInInterval2(a1, b1, c1, upper_bound, t))
		{
			// We have a collision
			collision = true;
			upper_bound = t;
			if(outPoint)	*outPoint = *v1;
		}

		// Check if circle hits the edge
		const SE_Vector2 v1v2 = *v2 - *v1;
		const PxReal v1v2_dot_delta = v1v2.Dot(inDelta);
		const PxReal v1v2_dot_bv1 = v1v2.Dot(bv1);
		const PxReal v1v2_len_sq = v1v2.GetLengthSquared();
		const PxReal a2 = v1v2_len_sq * a1 + v1v2_dot_delta * v1v2_dot_delta;
		const PxReal b2 = v1v2_len_sq * b1 - 2.0f * v1v2_dot_bv1 * v1v2_dot_delta;
		const PxReal c2 = v1v2_len_sq * c1 + v1v2_dot_bv1 * v1v2_dot_bv1;
		if(findLowestRootInInterval(a2, b2, c2, upper_bound, t))
//		if(findLowestRootInInterval(a2, b2, c2, upper_bound, t) && t<=1.0f)
//		if(findLowestRootInInterval2(a2, b2, c2, upper_bound, t))
		{
			// Check if the intersection point is on the edge
			const PxReal f = t * v1v2_dot_delta - v1v2_dot_bv1;
			if (f >= 0.0f && f <= v1v2_len_sq)
			{
				// We have a collision
				collision = true;
				upper_bound = t;
				if(outPoint)	*outPoint = *v1 + v1v2 * (f / v1v2_len_sq);
			}
		}
	}

	// Check if we had a collision
	if (!collision)
		return false;
	if(outFraction)
	{
		PX_ASSERT(upper_bound>=0.0f && upper_bound<=1.0f);
		*outFraction = upper_bound;
	}
	return true;
}

// Test between a polygon and a swept sphere with radius inRadius moving from inBegin to inBegin + inDelta
// If there is an intersection the intersection position is returned in outPoint and the center of the
// sphere is at inBegin + outFraction * inDelta when it collides
static bool polygonSweptSphereIntersect(const SE_Plane& inPlane, const SE_Vector3& u, const SE_Vector3& v, const SE_Vector2* inVertices, PxU32 inNumVertices, const SE_Vector3& inBegin, const SE_Vector3& inDelta, PxReal inRadius, SE_Vector3* outPoint, PxReal* outFraction)
{
	// Determine the range over which the sphere intersects the plane
	PxReal t1, t2;
	if(!planeSweptSphereIntersect(inPlane, inBegin, inDelta, inRadius, t1, t2))
		return false;

	// The radius of the circle is defined as: radius^2 = (sphere radius)^2 - (distance plane to center)^2
	// this can be written as: radius^2 = a * t^2 + b * t + c
	const PxReal n_dot_d = inPlane.mNormal.Dot(inDelta);
	const PxReal dist_to_b = inPlane.GetSignedDistance(inBegin);
	const PxReal a = -n_dot_d * n_dot_d;
	const PxReal b = -2.0f * n_dot_d * dist_to_b;
	const PxReal c = inRadius * inRadius - dist_to_b * dist_to_b;

	// Get begin and delta in plane space
	const SE_Vector2 begin = ConvertWorldToPlane(u, v, inBegin);
	const SE_Vector2 delta = ConvertWorldToPlane(u, v, inDelta);

	// Test if sphere intersects at t1
	SE_Vector2 p(0.0f, 0.0f);
	if(polygonCircleIntersect(inVertices, inNumVertices, begin + delta * t1, a * t1 * t1 + b * t1 + c, p))
	{
		if(outFraction)	*outFraction = t1;
		if(outPoint)	*outPoint = inPlane.ConvertPlaneToWorld(u, v, p);
		return true;
	}

	// Test if sphere intersects with one of the edges or vertices
	if(sweptCircleEdgeVertexIntersect(inVertices, inNumVertices, begin, delta, a, b, c, &p, outFraction))
	{
		if(outPoint)	*outPoint = inPlane.ConvertPlaneToWorld(u, v, p);
		return true;
	}
	return false;
}

static bool LSSTriangleOverlap(const PxTriangle& triangle, const PxVec3& p0, const PxVec3& dir, PxReal radius, PxVec3* hit_point, PxReal* t)
{
	const PxPlane plane(triangle.verts[0], triangle.verts[1], triangle.verts[2]);

	SE_Plane SEP;
	SEP.mConstant = plane.d;
	SEP.mNormal.x = plane.n.x;
	SEP.mNormal.y = plane.n.y;
	SEP.mNormal.z = plane.n.z;

	// Get basis
	SE_Vector3 u, v;
	SEP.GetBasisVectors(u, v);

	SE_Vector2 Proj[3];
	for(PxU32 i=0; i<3; i++)
	{
		//Remove strict alias warning on gcc
		const PxVec3& vert = triangle.verts[i];
		Proj[i] = ConvertWorldToPlane(u, v, (const SE_Vector3&)vert);
	}
	return polygonSweptSphereIntersect(SEP, u, v, Proj, 3, (const SE_Vector3&)p0, (const SE_Vector3&)dir, radius, (SE_Vector3*)hit_point, t);
}
#endif



static PX_FORCE_INLINE PxU32 getInitIndex(const PxU32* PX_RESTRICT cachedIndex, PxU32 nbTris)
{
	PxU32 initIndex = 0;
	if(cachedIndex)
	{
		PX_ASSERT(*cachedIndex < nbTris);
		PX_UNUSED(nbTris);
		initIndex = *cachedIndex;
	}
	return initIndex;
}

static void computeImpactData(PxVec3& hit, PxVec3& normal, const PxVec3& center, const PxVec3& dir, float t, const PxTriangle& tri)
{
	const PxVec3 newSphereCenter = center + dir*t;

#ifdef USE_NEW_SWEEP_TEST
	// We need the impact point, not computed by the new code
	PxReal u, v;
	hit = closestPtPointTriangle(newSphereCenter, tri.verts[0], tri.verts[1], tri.verts[2], u, v);
	PX_UNUSED(u);
	PX_UNUSED(v);
#endif

	// This is responsible for the cap-vs-box stuck while jumping. However it's needed to slide on box corners!
	// PT: this one is also dubious since the sphere/capsule center can be far away from the hit point when the radius is big!
	normal = newSphereCenter - hit;
	const PxReal m = normal.normalize();
	if(m<1e-3f)
		tri.normal(normal);
}

static PX_FORCE_INLINE bool rejectTriangle(const PxVec3& center, const PxVec3& unitDir, PxReal curT, PxReal radius, const PxTriangle& currentTri, const PxReal dpc0)
{
	if(!coarseCulling(center, unitDir, curT, radius, currentTri))
		return true;
#ifdef _XBOX
	if(CullTriangle(currentTri, unitDir, radius, curT, dpc0)==0.0f)
		return true;
#else
	if(!CullTriangle(currentTri, unitDir, radius, curT, dpc0))
		return true;
#endif
	return false;
}

static PX_FORCE_INLINE bool sweepSphereAgainstTri(	const PxVec3& center, const PxReal radius,							// Sphere data
													PxU32 i, const PxTriangle& currentTri, const PxVec3& triUnitNormal,	// Triangle data
													const PxVec3& unitDir, PxReal distance,								// Ray data
													const PxVec3& tweakUnitNormal,										// Tweak data
													PxReal& curT, PxReal& currentTweak, PxU32& index					// Results
													)
{
	// Sweep against current triangle
	PxReal currentDistance;

#ifdef USE_NEW_SWEEP_TEST
	currentDistance = 10000.0f;
	if(!sweepTriSphere(currentTri, triUnitNormal, center, radius, unitDir, currentDistance))
		return false;
#else
	PxVec3 tmp;
	if(!LSSTriangleOverlap(currentTri, center, D, radius, &tmp, &currentDistance))
		return false;

	currentDistance *= distance;
#endif
	if(currentDistance > distance)
		return false;

	const PxReal tweak = tweakUnitNormal.dot(unitDir) * gEpsilon;
	currentDistance += tweak;

	// PT: using ">" or ">=" is enough to block the CCT or not in the DE5967 visual test. Change to ">=" is a repro is needed.
	if(currentDistance > curT)
		return false;
#ifndef USE_NEW_SWEEP_TEST
	hit = tmp;
#endif
	currentTweak = tweak;
	curT = currentDistance;
	index = i;
	return true;
}

bool sweepSphereTriangles(	PxU32 nbTris, const PxTriangle* PX_RESTRICT triangles,		// Triangle data
							const PxVec3& center, const PxReal radius,					// Sphere data
							const PxVec3& unitDir, PxReal distance,						// Ray data
							const PxU32* PX_RESTRICT cachedIndex,						// Cache data
							PxVec3& _hit, PxVec3& _normal, PxReal& _t, PxU32& _index,	// Results
							bool isDoubleSided)											// Query modifiers
{
	if(!nbTris)
		return false;

	PxU32 index = PX_INVALID_U32;
	const PxU32 initIndex = getInitIndex(cachedIndex, nbTris);

	PxReal curT = distance;
	const PxReal dpc0 = center.dot(unitDir);

	PxReal currentTweak = 0.0f;

	for(PxU32 ii=0; ii<nbTris; ii++)	// We need i for returned triangle index
	{
		const PxU32 i = getTriangleIndex(ii, initIndex);

		const PxTriangle& currentTri = triangles[i];

		if(rejectTriangle(center, unitDir, curT, radius, currentTri, dpc0))
			continue;

		// FIXES BUG BUT TODO BETTER
		PxVec3 triNormal;
		PxReal magnitude;
		PxReal area;
		computeTriData(currentTri, triNormal, magnitude, area);
		if(area==0.0f) continue;

		// Backface culling
		const bool culled = !isDoubleSided && (triNormal.dot(unitDir) > 0.0f);
		if(culled) continue;

		triNormal /= magnitude;

		sweepSphereAgainstTri(center, radius, i, currentTri, triNormal, unitDir, distance, triNormal, curT, currentTweak, index);
	}
	if(index==PX_INVALID_U32)
		return false;	// We didn't touch any triangle

	// Put back the real dist
	const PxReal t = curT - currentTweak;

	// Compute impact data only once, using best triangle
	PxVec3 hit, normal;
	computeImpactData(hit, normal, center, unitDir, t, triangles[index]);

	_hit	= hit;
	_normal	= normal;
	_t		= t;
	_index	= index;

	return true;
}


#if __SPU__ // disable inline on SPU
void computeSweptBox(const PxVec3& extents, const PxVec3& center, const PxMat33& rot, const PxVec3& unitDir, const PxReal distance, Gu::Box& box)
{
	PxVec3 R1, R2;
	Ps::computeBasis(unitDir, R1, R2);

	PxReal dd[3];
	dd[0] = PxAbs(rot.column0.dot(unitDir));
	dd[1] = PxAbs(rot.column1.dot(unitDir));
	dd[2] = PxAbs(rot.column2.dot(unitDir));
	PxReal dmax = dd[0];
	PxU32 ax0=1;
	PxU32 ax1=2;
	if(dd[1]>dmax)
	{
		dmax=dd[1];
		ax0=0;
		ax1=2;
	}
	if(dd[2]>dmax)
	{
		dmax=dd[2];
		ax0=0;
		ax1=1;
	}
	if(dd[ax1]<dd[ax0])
	{
		PxU32 swap = ax0;
		ax0 = ax1;
		ax1 = swap;
	}

	R1 = rot[ax0];
	R1 -= (R1.dot(unitDir))*unitDir;	// Project to plane whose normal is dir
	R1.normalize();
	R2 = unitDir.cross(R1);

	box.setAxes(unitDir, R1, R2);

	PxReal Offset[3];
	Offset[0] = distance;
	Offset[1] = distance*(unitDir.dot(R1));
	Offset[2] = distance*(unitDir.dot(R2));

	for(PxU32 r=0; r<3; r++)
	{
		const PxVec3& R = box.rot[r];
		box.extents[r] = Offset[r]*0.5f + PxAbs(rot.column0.dot(R))*extents.x + PxAbs(rot.column1.dot(R))*extents.y + PxAbs(rot.column2.dot(R))*extents.z;
	}

	box.center = center + unitDir*distance*0.5f;
}
#endif

#if __SPU__
// AP: function version to reduce SPU code size
void sweepCapsuleTrianglesOutputTri2(
	const PxVec3& p0, const PxVec3& p1, const PxVec3& p2, const PxVec3& d, PxTriangle* extrudedTris,
	PxU32& nbExtrudedTris, PxVec3* extrudedTrisNormals
	)
{
	PxTriangle& t = extrudedTris[nbExtrudedTris];
	t.verts[0] = p0;
	t.verts[1] = p1;
	t.verts[2] = p2;
	PxVec3 nrm;
	t.denormalizedNormal(nrm);
	if(nrm.dot(d)>0.0f)
	{
		PxVec3 tmp = t.verts[1];
		t.verts[1] = t.verts[2];
		t.verts[2] = tmp;
		nrm = -nrm;
	}
	extrudedTrisNormals[nbExtrudedTris] = nrm;
	nbExtrudedTris++;
}
#define _OUTPUT_TRI2(p0, p1, p2, d) sweepCapsuleTrianglesOutputTri2(p0, p1, p2, d, extrudedTris, nbExtrudedTris, extrudedTrisNormals);
#else
#define _OUTPUT_TRI2(p0, p1, p2, d){			\
PxTriangle& t = extrudedTris[nbExtrudedTris];	\
t.verts[0] = p0;								\
t.verts[1] = p1;								\
t.verts[2] = p2;								\
PxVec3 nrm;										\
t.denormalizedNormal(nrm);						\
if(nrm.dot(d)>0.0f) {							\
PxVec3 tmp = t.verts[1];						\
t.verts[1] = t.verts[2];						\
t.verts[2] = tmp;								\
nrm = -nrm;										\
}												\
extrudedTrisNormals[nbExtrudedTris] = nrm;		\
nbExtrudedTris++; }
#endif

static bool sweepCapsuleTriangles(	PxU32 nbTris, const PxTriangle* PX_RESTRICT triangles,	// Triangle data
									const Gu::Capsule& capsule,								// Capsule data
									const PxVec3& unitDir, const PxReal distance,			// Ray data
									const PxU32* PX_RESTRICT cachedIndex,					// Cache data
									PxF32& t, PxVec3& normal, PxVec3& hit, PxU32& hitIndex,	// Results
									PxHitFlags hintFlags, bool isDoubleSided,				// Query modifiers
									const Gu::Box* cullBox=NULL)							// Cull data
{
	if(!nbTris)
		return false;

	const PxVec3 capsuleCenter = capsule.computeCenter();

	if(!(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP))
	{
		// PT: test if shapes initially overlap
		const PxVec3 segmentExtent = capsule.p1 - capsule.p0;
		const PxReal r2 = capsule.radius*capsule.radius;
		for(PxU32 i=0;i<nbTris;i++)
		{
			// PT: add culling here for now, but could be made more efficiently...
			{
				const PxTriangle& currentSrcTri = triangles[i];	// PT: src tri, i.e. non-extruded

				// Create triangle normal
				PxVec3 denormalizedNormal;
				currentSrcTri.denormalizedNormal(denormalizedNormal);

				// Backface culling
				const bool culled = !isDoubleSided && (denormalizedNormal.dot(unitDir) > 0.0f);
				if(culled)
					continue;
			}

			const PxVec3& p0 = triangles[i].verts[0];
			const PxVec3& p1 = triangles[i].verts[1];
			const PxVec3& p2 = triangles[i].verts[2];

#if __SPU__ // this is to reduce the code size on SPU
			Vec3V dummy1, dummy2;
			FloatV result = Gu::distanceSegmentTriangleSquared(
				V3LoadU(capsule.p0), V3LoadU(capsule.p1),
				V3LoadU(p0), V3LoadU(p1), V3LoadU(p2),
				dummy1, dummy2);
			PxReal dist2 = FStore(result);
#else
			PxReal dist2 = Gu::distanceSegmentTriangleSquared(capsule.p0, segmentExtent, p0, p1 - p0, p2 - p0);
#endif
			if (dist2<=r2)
			{
				hitIndex	= i;
				t			= 0.0f;
				normal		= -unitDir;
				hit			= capsuleCenter;	// PT: this is arbitrary
				return true;
			}
		}
	}

	// The nice thing with this approach is that we "just" fallback to already existing code

	// PT: we can fallback to sphere sweep:
	// - if the capsule is degenerate (i.e. it's a sphere)
	// - if the sweep direction is the same as the capsule axis, in which case we can just sweep the top or bottom sphere

	const PxVec3 extrusionDir = (capsule.p0 - capsule.p1)*0.5f;	// Extrusion dir = capsule segment
	const PxReal halfHeight = extrusionDir.magnitude();
	bool mustExtrude = halfHeight!=0.0f;
	if(mustExtrude)
	{
		const PxVec3 capsuleAxis = extrusionDir/halfHeight;
		const PxReal colinearity = PxAbs(capsuleAxis.dot(unitDir));
		mustExtrude = (colinearity < (1.0f - LOCAL_EPSILON));
	}

	if(!mustExtrude)
	{
		const PxVec3 sphereCenter = capsuleCenter + unitDir * halfHeight;
		return sweepSphereTriangles(nbTris, triangles, sphereCenter, capsule.radius, unitDir, distance, cachedIndex, hit, normal, t, hitIndex, isDoubleSided);
	}

#ifdef NEW_SWEEP_CAPSULE_MESH
	// PT: extrude mesh on the fly. This is a modified copy of sweepSphereTriangles, unfortunately
	PxTriangle extrudedTris[7];
	PxVec3 extrudedTrisNormals[7];	// Not normalized

	hitIndex = PX_INVALID_U32;
	const PxU32 initIndex = getInitIndex(cachedIndex, nbTris);

	const PxReal radius = capsule.radius;
	PxReal curT = distance;
	const PxReal dpc0 = capsuleCenter.dot(unitDir);

	// PT: we will copy the best triangle here. Using indices alone doesn't work
	// since we extrude on-the-fly (and we don't want to re-extrude later)
	PxTriangle bestTri;
	PxReal currentTweak = 0.0f;

	for(PxU32 ii=0; ii<nbTris; ii++)	// We need i for returned triangle index
	{
		const PxU32 i = getTriangleIndex(ii, initIndex);

		const PxTriangle& currentSrcTri = triangles[i];	// PT: src tri, i.e. non-extruded

///////////// PT: this part comes from "ExtrudeMesh"
		// Create triangle normal
		PxVec3 denormalizedNormal;
		currentSrcTri.denormalizedNormal(denormalizedNormal);

		// Backface culling
		// bool DoCulling = (CurrentFlags & Gu::TriangleCollisionFlag::eDOUBLE_SIDED)==0;
		// bool Culled = (DoCulling && (DenormalizedNormal|dir) > 0.0f);
		const bool culled = !isDoubleSided && (denormalizedNormal.dot(unitDir) > 0.0f);
		if(culled)
			continue;

		if(cullBox)
		{
			PxVec3 tmp[3];
			tmp[0] = cullBox->rotateInv(currentSrcTri.verts[0] - cullBox->center);
			tmp[1] = cullBox->rotateInv(currentSrcTri.verts[1] - cullBox->center);
			tmp[2] = cullBox->rotateInv(currentSrcTri.verts[2] - cullBox->center);
			const PxVec3 center(0.0f);
			if(!Gu::intersectTriangleBox(center, cullBox->extents, tmp[0], tmp[1], tmp[2]))
				continue;
		}

		// Extrude mesh on the fly
		PxU32 nbExtrudedTris=0;

		PxVec3 p0 = currentSrcTri.verts[0];
		PxVec3 p1 = currentSrcTri.verts[1];
		PxVec3 p2 = currentSrcTri.verts[2];

		PxVec3 p0b = p0 + extrusionDir;
		PxVec3 p1b = p1 + extrusionDir;
		PxVec3 p2b = p2 + extrusionDir;

		p0 -= extrusionDir;
		p1 -= extrusionDir;
		p2 -= extrusionDir;

#define _OUTPUT_TRI(p0, p1, p2){														\
extrudedTris[nbExtrudedTris].verts[0] = p0;												\
extrudedTris[nbExtrudedTris].verts[1] = p1;												\
extrudedTris[nbExtrudedTris].verts[2] = p2;												\
extrudedTris[nbExtrudedTris].denormalizedNormal(extrudedTrisNormals[nbExtrudedTris]);	\
nbExtrudedTris++;}

		if(denormalizedNormal.dot(extrusionDir) >= 0.0f)	_OUTPUT_TRI(p0b, p1b, p2b)
		else												_OUTPUT_TRI(p0, p1, p2)

		// ### it's probably useless to extrude all the shared edges !!!!!
		//if(CurrentFlags & Gu::TriangleCollisionFlag::eACTIVE_EDGE12)
		{
			_OUTPUT_TRI2(p1, p1b, p2b, unitDir)
			_OUTPUT_TRI2(p1, p2b, p2, unitDir)
		}
		//if(CurrentFlags & Gu::TriangleCollisionFlag::eACTIVE_EDGE20)
		{
			_OUTPUT_TRI2(p0, p2, p2b, unitDir)
			_OUTPUT_TRI2(p0, p2b, p0b, unitDir)
		}
		//if(CurrentFlags & Gu::TriangleCollisionFlag::eACTIVE_EDGE01)
		{
			_OUTPUT_TRI2(p0b, p1b, p1, unitDir)
			_OUTPUT_TRI2(p0b, p1, p0, unitDir)
		}
/////////////

		// PT: TODO: this one is new, to fix the tweak issue. However this wasn't
		// here before so the perf hit should be analyzed.
		denormalizedNormal.normalize();

		for(PxU32 j=0;j<nbExtrudedTris;j++)
		{
			const PxTriangle& currentTri = extrudedTris[j];

			PxVec3& triNormal = extrudedTrisNormals[j];
			// Backface culling
			const bool culled = (triNormal.dot(unitDir)) > 0.0f;
			if(culled)
				continue;

			// PT: beware, culling is only ok on the sphere I think
			if(rejectTriangle(capsuleCenter, unitDir, curT, radius, currentTri, dpc0))
				continue;

			// FIXES BUG BUT TODO BETTER
//			PxVec3 triNormal;
//			PxReal magnitude;
//			PxReal area;
//			computeTriData(currentTri, triNormal, magnitude, area);
//			if(area==0.0f)
//				continue;

//			PxVec3 triNormal;
//			currentTri.denormalizedNormal(triNormal);
			PxReal magnitude = triNormal.magnitude();
			if(magnitude==0.0f)
				continue;

			triNormal /= magnitude;

			if(sweepSphereAgainstTri(capsuleCenter, radius, i, currentTri, triNormal, unitDir, distance, denormalizedNormal, curT, currentTweak, hitIndex))
			{
	#ifdef USE_NEW_SWEEP_TEST
				bestTri = currentTri;
	#endif
			}
		}
	}

	if(hitIndex==PX_INVALID_U32)
		return false;	// We didn't touch any triangle

	// Put back the real dist
	t = curT - currentTweak;

	// Compute impact data only once, using best triangle
	computeImpactData(hit, normal, capsuleCenter, unitDir, t, bestTri);

#else	// NEW_SWEEP_CAPSULE_MESH
	{
		PX_ALLOCA(Extruded, PxTriangle, nbTris*7);
		PX_ALLOCA(Ids, PxU32, nbTris*7);

		PxU32 NbExtruded = ExtrudeMesh(nbTris, triangles, extrusionDir, Extruded, Ids, unitDir * distance, NULL);

		if(!sweepSphereTriangles(NbExtruded, Extruded, capsuleCenter, capsule.radius, unitDir, distance, NULL, hit, normal, t, hitIndex, false))
			return false;

		if(hitIndex!=PX_INVALID_U32)
			hitIndex = Ids[hitIndex];
	}
#endif

	// PT: revisit this
	if(hitIndex!=PX_INVALID_U32)
	{
//		hitIndex = Ids[hitIndex];

		// PT: deadline in a few hours. No time. Should be cleaned later or re-thought.
		// PT: we need to recompute a hit here because the hit between the *capsule* and the source mesh can be very
		// different from the hit between the *sphere* and the extruded mesh.

		// Touched tri
		const PxVec3& p0 = triangles[hitIndex].verts[0];
		const PxVec3& p1 = triangles[hitIndex].verts[1];
		const PxVec3& p2 = triangles[hitIndex].verts[2];

#if __SPU__ // this is to reduce the code size on SPU
		Vec3V pointOnSeg, pointOnTri;
		Gu::distanceSegmentTriangleSquared(
			V3LoadU(capsule.p0 + unitDir*t), V3LoadU(capsule.p1 + unitDir*t),
			V3LoadU(p0), V3LoadU(p1), V3LoadU(p2),
			pointOnSeg, pointOnTri);
		V3StoreU(pointOnTri, hit);
#else
		// Move capsule
		const Gu::Segment Moved(
			PxVec3(capsule.p0 + (unitDir * t)),
			PxVec3(capsule.p1 + (unitDir * t)));

		PxReal Alpha, Beta, Gamma;
		const PxReal s = Gu::distanceSegmentTriangleSquared(Moved, p0, p1-p0, p2-p0, &Alpha, &Beta, &Gamma);
		PX_UNUSED(s);

		// AP: un-inlining doesn't help with SPU sweeps
		hit = Ps::computeBarycentricPoint(p0, p1, p2, Beta, Gamma);
#endif
	}
	return true;
}


//#define PRECOMPUTE_FAT_BOX	// PT: TODO: clean this up
//#define PRECOMPUTE_FAT_BOX_MORE
//
//#ifdef PRECOMPUTE_FAT_BOX

/*
static PX_FORCE_INLINE void computeFatEdges(const PxVec3* PX_RESTRICT boxVertices, PxVec3* PX_RESTRICT fatEdges)
{
	const PxU8* PX_RESTRICT Edges = Gu::getBoxEdges();
	// Loop through box edges
	for(PxU32 i=0;i<12;i++)	// 12 edges
	{
		fatEdges[i*2  ] = boxVertices[*Edges++];
		fatEdges[i*2+1] = boxVertices[*Edges++];
		Ps::makeFatEdge(fatEdges[i*2], fatEdges[i*2+1], gFatBoxEdgeCoeff);
	}
}
*/


#ifdef TOSEE	// PT: TODO: vectorize this properly

static PX_FORCE_INLINE int intersectRayAABB3(const PxVec3& minimum, const PxVec3& maximum,
									  const PxTriangle& tri, const PxVec3& rd, const PxVec3& oneOverDir,
									float& tnear, int& hit,
									bool fbx, bool fby, bool fbz)
{
/*	if(fbx)
		if(ro.x<minimum.x || ro.x>maximum.x)
		{
//			return -1;
		}
	if(fby)
		if(ro.y<minimum.y || ro.y>maximum.y)
		{
//			return -1;
		}
	if(fbz)
		if(ro.z<minimum.z || ro.z>maximum.z)
		{
//			return -1;
		}
*/

	const PxReal t1x_candidate0 = (minimum.x - tri.verts[0].x) * oneOverDir.x;
	const PxReal t2x_candidate0 = (maximum.x - tri.verts[0].x) * oneOverDir.x;
	const PxReal t1x_candidate1 = (minimum.x - tri.verts[1].x) * oneOverDir.x;
	const PxReal t2x_candidate1 = (maximum.x - tri.verts[1].x) * oneOverDir.x;
	const PxReal t1x_candidate2 = (minimum.x - tri.verts[2].x) * oneOverDir.x;
	const PxReal t2x_candidate2 = (maximum.x - tri.verts[2].x) * oneOverDir.x;

	const PxReal t1y_candidate0 = (minimum.y - tri.verts[0].y) * oneOverDir.y;
	const PxReal t2y_candidate0 = (maximum.y - tri.verts[0].y) * oneOverDir.y;
	const PxReal t1y_candidate1 = (minimum.y - tri.verts[1].y) * oneOverDir.y;
	const PxReal t2y_candidate1 = (maximum.y - tri.verts[1].y) * oneOverDir.y;
	const PxReal t1y_candidate2 = (minimum.y - tri.verts[2].y) * oneOverDir.y;
	const PxReal t2y_candidate2 = (maximum.y - tri.verts[2].y) * oneOverDir.y;

	const PxReal t1z_candidate0 = (minimum.z - tri.verts[0].z) * oneOverDir.z;
	const PxReal t2z_candidate0 = (maximum.z - tri.verts[0].z) * oneOverDir.z;
	const PxReal t1z_candidate1 = (minimum.z - tri.verts[1].z) * oneOverDir.z;
	const PxReal t2z_candidate1 = (maximum.z - tri.verts[1].z) * oneOverDir.z;
	const PxReal t1z_candidate2 = (minimum.z - tri.verts[2].z) * oneOverDir.z;
	const PxReal t2z_candidate2 = (maximum.z - tri.verts[2].z) * oneOverDir.z;

	const float deltax0 = t1x_candidate0 - t2x_candidate0;
	const float deltax1 = t1x_candidate1 - t2x_candidate1;
	const float deltax2 = t1x_candidate2 - t2x_candidate2;

	const float deltay0 = t1y_candidate0 - t2y_candidate0;
	const float deltay1 = t1y_candidate1 - t2y_candidate1;
	const float deltay2 = t1y_candidate2 - t2y_candidate2;

	const float deltaz0 = t1z_candidate0 - t2z_candidate0;
	const float deltaz1 = t1z_candidate1 - t2z_candidate1;
	const float deltaz2 = t1z_candidate2 - t2z_candidate2;

	const float t1x0 = physx::intrinsics::fsel(deltax0, t2x_candidate0, t1x_candidate0);
	const float t1x1 = physx::intrinsics::fsel(deltax1, t2x_candidate1, t1x_candidate1);
	const float t1x2 = physx::intrinsics::fsel(deltax2, t2x_candidate2, t1x_candidate2);

	const float t1y0 = physx::intrinsics::fsel(deltay0, t2y_candidate0, t1y_candidate0);
	const float t1y1 = physx::intrinsics::fsel(deltay1, t2y_candidate1, t1y_candidate1);
	const float t1y2 = physx::intrinsics::fsel(deltay2, t2y_candidate2, t1y_candidate2);

	const float t1z0 = physx::intrinsics::fsel(deltaz0, t2z_candidate0, t1z_candidate0);
	const float t1z1 = physx::intrinsics::fsel(deltaz1, t2z_candidate1, t1z_candidate1);
	const float t1z2 = physx::intrinsics::fsel(deltaz2, t2z_candidate2, t1z_candidate2);

	const float t2x0 = physx::intrinsics::fsel(deltax0, t1x_candidate0, t2x_candidate0);
	const float t2x1 = physx::intrinsics::fsel(deltax1, t1x_candidate1, t2x_candidate1);
	const float t2x2 = physx::intrinsics::fsel(deltax2, t1x_candidate2, t2x_candidate2);

	const float t2y0 = physx::intrinsics::fsel(deltay0, t1y_candidate0, t2y_candidate0);
	const float t2y1 = physx::intrinsics::fsel(deltay1, t1y_candidate1, t2y_candidate1);
	const float t2y2 = physx::intrinsics::fsel(deltay2, t1y_candidate2, t2y_candidate2);

	const float t2z0 = physx::intrinsics::fsel(deltaz0, t1z_candidate0, t2z_candidate0);
	const float t2z1 = physx::intrinsics::fsel(deltaz1, t1z_candidate1, t2z_candidate1);
	const float t2z2 = physx::intrinsics::fsel(deltaz2, t1z_candidate2, t2z_candidate2);

	const float bxf0 = physx::intrinsics::fsel(deltax0, 3.0f, 0.0f);
	const float bxf1 = physx::intrinsics::fsel(deltax1, 3.0f, 0.0f);
	const float bxf2 = physx::intrinsics::fsel(deltax2, 3.0f, 0.0f);

	const float byf0 = physx::intrinsics::fsel(deltay0, 4.0f, 1.0f);
	const float byf1 = physx::intrinsics::fsel(deltay1, 4.0f, 1.0f);
	const float byf2 = physx::intrinsics::fsel(deltay2, 4.0f, 1.0f);

	const float bzf0 = physx::intrinsics::fsel(deltaz0, 5.0f, 2.0f);
	const float bzf1 = physx::intrinsics::fsel(deltaz1, 5.0f, 2.0f);
	const float bzf2 = physx::intrinsics::fsel(deltaz2, 5.0f, 2.0f);

	float tnear0, tfar0;
	float tnear1, tfar1;
	float tnear2, tfar2;

	tnear0 = t1x0;
	tnear1 = t1x1;
	tnear2 = t1x2;

	tfar0 = t2x0;
	tfar1 = t2x1;
	tfar2 = t2x2;

	float ret0 = bxf0;
	float ret1 = bxf1;
	float ret2 = bxf2;

	const float delta10 = t1y0 - tnear0;
	const float delta11 = t1y1 - tnear1;
	const float delta12 = t1y2 - tnear2;

	tnear0 = physx::intrinsics::fsel(delta10, t1y0, tnear0);
	tnear1 = physx::intrinsics::fsel(delta11, t1y1, tnear1);
	tnear2 = physx::intrinsics::fsel(delta12, t1y2, tnear2);

	ret0 = physx::intrinsics::fsel(delta10, byf0, ret0);
	ret1 = physx::intrinsics::fsel(delta11, byf1, ret1);
	ret2 = physx::intrinsics::fsel(delta12, byf2, ret2);

	tfar0 = physx::intrinsics::selectMin(tfar0, t2y0);
	tfar1 = physx::intrinsics::selectMin(tfar1, t2y1);
	tfar2 = physx::intrinsics::selectMin(tfar2, t2y2);

	const float delta20 = t1z0 - tnear0;
	const float delta21 = t1z1 - tnear1;
	const float delta22 = t1z2 - tnear2;

	tnear0 = physx::intrinsics::fsel(delta20, t1z0, tnear0);
	tnear1 = physx::intrinsics::fsel(delta21, t1z1, tnear1);
	tnear2 = physx::intrinsics::fsel(delta22, t1z2, tnear2);

	ret0 = physx::intrinsics::fsel(delta20, bzf0, ret0);
	ret1 = physx::intrinsics::fsel(delta21, bzf1, ret1);
	ret2 = physx::intrinsics::fsel(delta22, bzf2, ret2);

	tfar0 = physx::intrinsics::selectMin(tfar0, t2z0);
	tfar1 = physx::intrinsics::selectMin(tfar1, t2z1);
	tfar2 = physx::intrinsics::selectMin(tfar2, t2z2);

	// PT: this fcmp seems cheaper than the alternative LHS below
	if(tnear0<0.0f || tnear0>tfar0 || tfar0<LOCAL_EPSILON_RAY_BOX)
	{
		ret0 = -1;
	}
	if(tnear1<0.0f || tnear1>tfar1 || tfar1<LOCAL_EPSILON_RAY_BOX)
	{
		ret1 = -1;
	}
	if(tnear2<0.0f || tnear2>tfar2 || tfar2<LOCAL_EPSILON_RAY_BOX)
	{
		ret2 = -1;
	}

	float ret = ret0;
	tnear = tnear0;
	hit = 0;

	if(ret1!=-1 && tnear1<tnear0)
	{
		ret = ret1;
		tnear = tnear1;
		hit = 1;
	}
	if(ret2!=-1 && tnear2<tnear0)
	{
		ret = ret2;
		tnear = tnear2;
		hit = 2;
	}

//	ret = physx::intrinsics::fsel(tfar - tnear, ret, -1.0f);
//	ret = physx::intrinsics::fsel(LOCAL_EPSILON_RAY_BOX - tfar, -1.0f, ret);

	return int(ret);
}

#endif



/////////////////////////////////////////////////  sweepCapsule/Sphere  //////////////////////////////////////////////////////

bool Gu::sweepCapsule_SphereGeom(GU_CAPSULE_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eSPHERE);
	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom);

	const Gu::Sphere sphere(pose.p, sphereGeom.radius+inflation);

	if(!sweepSphereCapsule(sphere, lss, -unitDir, distance, sweepHit.distance, sweepHit.position, sweepHit.normal, hintFlags))
		return false;

	sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;

	return true;
}

bool Gu::sweepCapsule_PlaneGeom(GU_CAPSULE_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::ePLANE);
	PX_UNUSED(geom);
//	const PxPlaneGeometry& planeGeom = static_cast<const PxPlaneGeometry&>(geom);

	const PxPlane& worldPlane = Gu::getPlane(pose);

	const PxF32 capsuleRadius = lss.radius + inflation;

	PxU32 index = 0;
	PxVec3 pts[2];

	PxReal minDp = PX_MAX_REAL;

	// Find extreme point on the capsule
	// AP: removed if (lss.p0 == lss.p1 clause because it wasn't properly computing minDp)
	pts[0] = lss.p0;
	pts[1] = lss.p1;
	for(PxU32 i=0; i<2; i++)
	{
		const PxReal dp = pts[i].dot(worldPlane.n);
		if(dp<minDp)
		{
			minDp = dp;
			index = i;
		}
	}

	if(!(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP))
	{
		// test if the capsule initially overlaps with plane
		if(minDp <= capsuleRadius - worldPlane.d)
		{
			sweepHit.flags			= PxHitFlag::eDISTANCE|PxHitFlag::eNORMAL|PxHitFlag::ePOSITION;
			sweepHit.distance		= 0.0f;
			sweepHit.position		= pts[index];	// PT: this is arbitrary
			sweepHit.normal			= -unitDir;
			return true;
		}
	}

	const PxVec3 ptOnCapsule = pts[index] - worldPlane.n*capsuleRadius;

	// Raycast extreme vertex against plane
	bool hitPlane = Gu::intersectRayPlane(ptOnCapsule, unitDir, worldPlane, sweepHit.distance, &sweepHit.position);
	if(hitPlane && sweepHit.distance > 0 && sweepHit.distance <= distance)
	{
		sweepHit.normal = worldPlane.n;
		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
		return true;
	}
	return false;
}

bool Gu::sweepCapsule_CapsuleGeom(GU_CAPSULE_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eCAPSULE);
	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom);

	Gu::Capsule staticCapsule;
	Gu::getCapsule(staticCapsule, capsuleGeom, pose);
	staticCapsule.radius +=inflation;

	PxU16 outFlags;
	if(sweepCapsuleCapsule(lss, staticCapsule, -unitDir, distance, sweepHit.distance, sweepHit.position, sweepHit.normal, hintFlags, outFlags))
	{
		sweepHit.flags = PxHitFlags(outFlags);
		return true;
	}

	return false;
}

bool Gu::sweepCapsule_ConvexGeom(GU_CAPSULE_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eCONVEXMESH);
	
	
	using namespace Ps::aos;

	
	PX_ASSERT(geom.getType() == PxGeometryType::eCONVEXMESH);
	const PxConvexMeshGeometry& convexGeom = static_cast<const PxConvexMeshGeometry&>(geom);


#ifdef __SPU__
	//pxPrintf("sweepCap ptr=%x\n", PxU32(convexGeom.convexMesh));
	PX_COMPILE_TIME_ASSERT(&((Gu::ConvexMesh*)NULL)->getHull()==NULL);
	
	PX_ALIGN_PREFIX(16)  PxU8 convexMeshBuffer[sizeof(Gu::ConvexMesh)+32] PX_ALIGN_SUFFIX(16);
	Gu::ConvexMesh* mesh = memFetchAsync<Gu::ConvexMesh>(convexMeshBuffer, MemFetchPtr(convexGeom.convexMesh), sizeof(Gu::ConvexMesh),1);
	memFetchWait(1); // convexMesh	

	const PxU32 nbPolys = mesh->getNbPolygonsFast();
	const Gu::HullPolygonData* PX_RESTRICT polysEA = mesh->getPolygons();
	const PxU32 polysSize = sizeof(Gu::HullPolygonData)*nbPolys + sizeof(PxVec3)*mesh->getNbVerts();
	
 	//TODO: Need optimization with dma cache --jiayang
	void* hullBuffer = PxAlloca(CELL_ALIGN_SIZE_16(polysSize+32));
	Gu::HullPolygonData* polys = memFetchAsync<Gu::HullPolygonData>(hullBuffer, (uintptr_t)(polysEA), polysSize, 1);

	Gu::ConvexHullData* hullData = &mesh->getHull();
	hullData->mPolygons = polys;

	memFetchWait(1); // convex mesh polygons
	//pxPrintf("sweepCap done\n");
#else
	Gu::ConvexHullData* hullData = &static_cast<Gu::ConvexMesh*>(convexGeom.convexMesh)->getHull();
	const PxU32 nbPolys = hullData->mNbPolygons;
#endif

	PxReal _capsuleHalfHeight = 0.0f;
	const PxTransform capTransform = getCapsuleTransform(lss, _capsuleHalfHeight);

	const Vec3V zeroV = V3Zero();
	const FloatV zero = FZero();
	const FloatV dist = FLoad(distance);
	const Vec3V worldDir = V3LoadU(unitDir);

	const QuatV q0 = QuatVLoadU(&capTransform.q.x);
	const Vec3V p0 = V3LoadU(&capTransform.p.x);

	const QuatV q1 = QuatVLoadU(&pose.q.x);
	const Vec3V p1 = V3LoadU(&pose.p.x);

	const PsTransformV capPose(p0, q0);
	const PsTransformV convexPose(p1, q1);
	const PsMatTransformV aToB(convexPose.transformInv(capPose));

	//const PsMatTransformV aToB(pose.transformInv(capsuleTransform));

	const FloatV capsuleHalfHeight = FLoad(_capsuleHalfHeight);
	const FloatV capsuleRadius = FLoad(lss.radius);

	const Vec3V vScale = Vec3V_From_Vec4V(V4LoadU(&convexGeom.scale.scale.x));
	const QuatV vQuat = QuatVLoadU(&convexGeom.scale.rotation.x);

	
	Gu::CapsuleV capsule(aToB.p, aToB.rotate( V3Scale(V3UnitX(), capsuleHalfHeight)), capsuleRadius);
	//Gu::CapsuleV capsule(zeroV, V3Scale(V3UnitX(), capsuleHalfHeight), capsuleRadius);
	Gu::ConvexHullV convexHull(hullData, zeroV, vScale, vQuat);

	const Vec3V dir = convexPose.rotateInv(V3Neg(V3Scale(worldDir, dist)));

	FloatV toi;
	Vec3V closestA, normal;//closestA and normal is in the local space of convex hull
	bool hit  = Gu::GJKLocalRayCast(capsule, convexHull, zero, zeroV, dir, toi, normal, closestA,
		lss.radius + inflation, !(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP));

	if(hit)
	{
		closestA = V3NegScaleSub(normal, capsuleRadius, closestA);
		const Vec3V worldPointA = convexPose.transform(closestA);

		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;

		if(FAllGrtrOrEq(zero, toi))
		{
			sweepHit.distance	= 0.0f;
			sweepHit.normal		= -unitDir;
			V3StoreU(worldPointA, sweepHit.position);
			//sweepHit.position		= destWorldPointA;	// should be the deepest penetration point
			return true;
		}

		const Vec3V destNormal = V3Normalize(convexPose.rotate(normal));
		const FloatV length = FMul(dist, toi);
		const Vec3V destWorldPointA = V3ScaleAdd(worldDir, length, worldPointA);
		V3StoreU(destNormal, sweepHit.normal);
		V3StoreU(destWorldPointA, sweepHit.position);
		FStore(length, &sweepHit.distance);

		// PT: compute closest polygon using the same tweak as in swept-capsule-vs-mesh
		if(1)
		{
			sweepHit.faceIndex = computeSweepConvexPlane(convexGeom,hullData,nbPolys,pose,sweepHit.position,unitDir);
			//pxPrintf("fi = %d, pos=%.7f %.7f %.7f\n",
			//	sweepHit.faceIndex, sweepHit.position.x, sweepHit.position.y, sweepHit.position.z);
		}
		return true;
	}
	
	return false;
}

bool Gu::sweepCapsule_MeshGeom(GU_CAPSULE_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eTRIANGLEMESH);
	const PxTriangleMeshGeometry& triMeshGeom = static_cast<const PxTriangleMeshGeometry&>(geom);

	// optimize this
	// use the obb sweep collider directly ?
	//const Gu::TriangleMesh& tm = *static_cast<Gu::TriangleMesh*>(triMeshGeom.triangleMesh);

	GU_FETCH_MESH_DATA(triMeshGeom);

	const Gu::RTreeMidphase& collisionModel = meshData->mOpcodeModel;

	Gu::HybridOBBCollider boxCollider;
	boxCollider.SetPrimitiveTests(true);	// ############

	Gu::Capsule inflatedCapsule;
	inflatedCapsule.p0 = lss.p0;
	inflatedCapsule.p1 = lss.p1;
	inflatedCapsule.radius = lss.radius + inflation;


	// Compute swept box
	Box capsuleBox;
	computeBoxAroundCapsule(inflatedCapsule, capsuleBox);

	Box sweptBox;
#ifdef __SPU__
	//Spu accuracy is lower than ppu, use bigger sweep volume in broadphase sweeping test here
	computeSweptBox(capsuleBox.extents, capsuleBox.center, capsuleBox.rot, unitDir, distance+LOCAL_EPSILON, sweptBox);
#else
	computeSweptBox(capsuleBox.extents, capsuleBox.center, capsuleBox.rot, unitDir, distance, sweptBox);
#endif

	const Cm::Matrix34 vertexToWorldSkew = pose * triMeshGeom.scale;

	Gu::Box vertexSpaceBox;
	computeVertexSpaceOBB(vertexSpaceBox, sweptBox, pose, triMeshGeom.scale);

	// Collide OBB against current mesh
	RTreeMidphaseData hmd;	// PT: I suppose doing the "conversion" at runtime is fine
	collisionModel.getRTreeMidphaseData(hmd);

	const PxU32 meshBothSides = hintFlags & PxHitFlag::eMESH_BOTH_SIDES;
	const bool isDoubleSided = (triMeshGeom.meshFlags & PxMeshGeometryFlag::eDOUBLE_SIDED) || meshBothSides;

#ifdef __SPU__

	struct VolumeColliderContainerLessCallback : VolumeColliderTrigCallback
	{		
		PxSweepHit&					hit;
		const Cm::Matrix34&			vertexToWorldSkew;
		PxReal						distance;
		bool						bDoubleSide;		
		const Gu::Capsule&			inflatedCapsule;
		const PxVec3&				unitDir;
		const PxHitFlags&			hintFlags;
		bool						status;

		VolumeColliderContainerLessCallback( PxSweepHit& sweepHit, const Cm::Matrix34& worldMatrix, PxReal distance, bool doubleSide, 
			const Gu::Capsule& capsule, const PxVec3& unitDir, const PxHitFlags& hintFlags) 
			:hit(sweepHit), vertexToWorldSkew(worldMatrix), distance(distance), bDoubleSide(doubleSide), 
			inflatedCapsule(capsule), unitDir(unitDir), hintFlags(hintFlags), status(false)
		{}
		virtual ~VolumeColliderContainerLessCallback() {}

		virtual bool processResults(PxU32 count, const PxVec3* verts, const PxU32* buf, const PxU32* trigVertIndBuf)
		{
			PxTriangle* tmpt = (PxTriangle*)PxAlloca(sizeof(PxTriangle)*count);

			getScaledTriangle(verts, count, vertexToWorldSkew, tmpt);

			//pxPrintf("sct count=%d, buf[0] = %d\n", count, buf[0]);
			float localDist;
			PxVec3 localPos;
			PxVec3 localNormal;
			PxU32 localIndex;
			const bool status1 = sweepCapsuleTriangles(count, tmpt, inflatedCapsule, unitDir, distance, NULL,
				localDist, localNormal, localPos, localIndex, hintFlags, bDoubleSide);
			if(status1)
			{
				distance = localDist;
				hit.distance = localDist;
				hit.normal = localNormal;
				hit.position = localPos;
				hit.faceIndex = buf[localIndex];

//				pxPrintf("hit faceIndex=%d\n", hit.faceIndex);
				hit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
				status = true;
			}
			return true;
		}
		
		void getScaledTriangle( const PxVec3* verts, PxU32 triCount, const Cm::Matrix34& worldMatrix, PxTriangle* output)
		{
			for(PxU32 i=0; i < triCount; i++)
			{
				output[i].verts[0] = worldMatrix.transform(verts[3*i+0]);
				output[i].verts[1] = worldMatrix.transform(verts[3*i+1]);
				output[i].verts[2] = worldMatrix.transform(verts[3*i+2]);
			}
		}
	} callback( sweepHit, vertexToWorldSkew, distance, isDoubleSided, inflatedCapsule, unitDir, hintFlags);

	// AP: careful with changing the template params - can negatively affect SPU_Sweep module size
	boxCollider.Collide<1,1,1>(vertexSpaceBox, hmd, &callback, NULL, NULL);

	return callback.status;
#else

//	Gu::Container tempContainer;	// PT: TODO: get rid of dynamic allocations in that one
	LocalContainer(tempContainer, 128);
	VolumeColliderContainerCallback callback(tempContainer);
	// AP: careful with changing the template params - can negatively affect SPU_Sweep module size
	boxCollider.Collide<1,1,1>(vertexSpaceBox, hmd, &callback, NULL, NULL);

	// Get results
	PxU32 nb = tempContainer.GetNbEntries();
	if(!nb)
		return false;

	const PxU32* PX_RESTRICT indices = tempContainer.GetEntries();

	PxTriangle* PX_RESTRICT tmpt = (PxTriangle*)PX_ALLOC_TEMP(sizeof(PxTriangle)*nb, PX_DEBUG_EXP("Triangle"));
	for(PxU32 i=0; i<nb; i++)
	{
		const PxU32 triangleIndex = *indices++;
		::getScaledTriangle(triMeshGeom, vertexToWorldSkew, tmpt[i], triangleIndex);	// ### move to local space...
	}

	//pxPrintf("sct nb=%d, indices[0] = %d\n", nb, indices[0]);
	bool status = sweepCapsuleTriangles(
		nb, tmpt, inflatedCapsule, unitDir, distance, NULL, sweepHit.distance,
		sweepHit.normal, sweepHit.position, sweepHit.faceIndex, hintFlags, isDoubleSided);
	if(status)
	{
		const PxU32* indices = tempContainer.GetEntries();
		sweepHit.faceIndex = indices[sweepHit.faceIndex];
		//pxPrintf("hit faceIndex=%d\n", sweepHit.faceIndex);
		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
	}
	PX_FREE_AND_RESET(tmpt);
	return status;
#endif
}

//#define USE_DYNAMIC_ARRAY

bool Gu::sweepCapsule_HeightFieldGeom(GU_CAPSULE_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eHEIGHTFIELD);
	const PxHeightFieldGeometry& heightFieldGeom = static_cast<const PxHeightFieldGeometry&>(geom);

	Gu::Capsule inflatedCapsule;
	inflatedCapsule.p0 = lss.p0;
	inflatedCapsule.p1 = lss.p1;
	inflatedCapsule.radius = lss.radius + inflation;

	// Compute swept box
	Gu::Box capsuleBox;
	Gu::computeBoxAroundCapsule(inflatedCapsule, capsuleBox);

	Gu::Box sweptBox;
	computeSweptBox(capsuleBox.extents, capsuleBox.center, capsuleBox.rot, unitDir, distance, sweptBox);

	//### Temp hack until we can directly collide the OBB against the HF
	const PxTransform sweptBoxTR = sweptBox.getTransform();
	const PxBounds3 bounds = PxBounds3::poseExtent(sweptBoxTR, sweptBox.extents);

	const PxU32 flags = PxHfQueryFlags::eWORLD_SPACE;

	struct LocalReport : Gu::EntityReport<PxU32>
	{
		LocalReport()
#ifdef USE_DYNAMIC_ARRAY
			: tmpTris(PX_DEBUG_EXP("localReportTmpTris")),
			tmpIndices(PX_DEBUG_EXP("localReportTmpIndices"))
#else
			: status(false)
#endif
		{

		}
		virtual bool onEvent(PxU32 nb, PxU32* indices)
		{
#ifdef USE_DYNAMIC_ARRAY
			for(PxU32 i=0; i<nb; i++)
			{
				PxU32 triangleIndex = indices[i];

				PxTriangle tmpTri;
				hfUtil->getTriangle(*pose, tmpTri, NULL, NULL, triangleIndex, true);
				tmpTris.pushBack(tmpTri);
				tmpIndices.pushBack(triangleIndex);
			}
#else
			PxU8 tribuf[HF_SWEEP_REPORT_BUFFER_SIZE*sizeof(PxTriangle)];
			PxTriangle* tmpT = (PxTriangle*)tribuf;
			for(PxU32 i=0; i<nb; i++)
			{
				PxU32 triangleIndex = indices[i];
				hfUtil->getTriangle(*pose, tmpT[i], NULL, NULL, triangleIndex, true);
			}

			PxU32 faceIndex = 0xFFFFFFFF;
			PxVec3 normal, impact;
			PxF32 t = PX_MAX_F32;

			bool statusl = sweepCapsuleTriangles(nb, tmpT, *inflatedCapsule, *unitDir, distance, NULL, t, normal, impact, faceIndex, *hintFlags, isDoubleSided);
			//PX_PRINTF("face %d t=%f\n",indices[faceIndex],t);
			if(statusl && (t <= sweepHit->distance))
			{
				sweepHit->faceIndex = indices[faceIndex];
				sweepHit->normal = normal;
				sweepHit->position = impact;
				sweepHit->distance = t;
				sweepHit->flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
				this->status = statusl;
			}
#endif
			return true;
		}

#ifdef USE_DYNAMIC_ARRAY
		Ps::Array<PxTriangle> tmpTris;
		Ps::Array<PxU32> tmpIndices;
#else
		const Gu::Capsule* inflatedCapsule;
		const PxVec3* unitDir;
		PxReal distance;
		PxSweepHit* sweepHit;
		PxHitFlags* hintFlags;
		bool status;
#endif
		const PxTransform* pose;
		Gu::HeightFieldUtil* hfUtil;
		bool isDoubleSided;
	} myReport;

#ifdef __SPU__
	PX_ALIGN_PREFIX(16)  PxU8 heightFieldBuffer[sizeof(Gu::HeightField)+32] PX_ALIGN_SUFFIX(16);
	Gu::HeightField* heightField = memFetchAsync<Gu::HeightField>(heightFieldBuffer, (uintptr_t)(heightFieldGeom.heightField), sizeof(Gu::HeightField), 1);
	memFetchWait(1);

	g_sampleCache.init((uintptr_t)(heightField->getData().samples), heightField->getData().tilesU);

	const_cast<PxHeightFieldGeometry&>(heightFieldGeom).heightField = heightField;
#endif

	Gu::HeightFieldUtil hfUtil(heightFieldGeom);

	//dynamic allocs are bad, but this code was already doing so before...
	myReport.pose = &pose;
	myReport.hfUtil = &hfUtil;
#ifdef USE_DYNAMIC_ARRAY
	myReport.tmpTris.reserve(128);
	myReport.tmpIndices.reserve(128);
#else
	myReport.inflatedCapsule = &inflatedCapsule;
	myReport.unitDir = &unitDir;
	myReport.distance = distance;
	myReport.sweepHit = &sweepHit;
	myReport.hintFlags = &hintFlags;
	myReport.isDoubleSided = heightFieldGeom.heightFieldFlags & PxMeshGeometryFlag::eDOUBLE_SIDED;
	sweepHit.distance = PX_MAX_F32;

#endif
	hfUtil.overlapAABBTriangles(pose, bounds, flags, &myReport);

#ifdef USE_DYNAMIC_ARRAY
	if(myReport.tmpTris.size() <= 0)
		return false;

	bool status = sweepCapsuleTriangles(myReport.tmpTris.size(), &(myReport.tmpTris[0]), inflatedCapsule, unitDir, distance, NULL, sweepHit.distance, sweepHit.normal, sweepHit.position, sweepHit.faceIndex, hintFlags, heightFieldGeom.meshFlags & PxMeshGeometryFlag::eDOUBLE_SIDED);
	if(status)
	{
		sweepHit.faceIndex = myReport.tmpIndices[sweepHit.faceIndex];
		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
	}

	return status;
#else
	return myReport.status;
#endif
}

/////////////////////////////////////////////////  sweepBox  //////////////////////////////////////////////////////

bool Gu::sweepBox_PlaneGeom(GU_BOX_SWEEP_FUNC_PARAMS)
{
	PX_ASSERT(geom.getType() == PxGeometryType::ePLANE);
	PX_UNUSED(geom);
//	const PxPlaneGeometry& planeGeom = static_cast<const PxPlaneGeometry&>(geom);

	PxPlane worldPlane = Gu::getPlane(pose);
	worldPlane.d -=inflation;

	// Find extreme point on the box
	PxVec3 boxPts[8];
	box.computeBoxPoints(boxPts);
	PxU32 index = 0;
	PxReal minDp = PX_MAX_REAL;
	for(PxU32 i=0;i<8;i++)
	{
		const PxReal dp = boxPts[i].dot(worldPlane.n);
	
		if(dp<minDp)
		{
			minDp = dp;
			index = i;
		}
	}

	if(!(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP))
	{
		// test if box initially overlap with plane
		if(minDp <= -worldPlane.d)
		{
			sweepHit.flags			= PxHitFlag::eDISTANCE|PxHitFlag::eNORMAL|PxHitFlag::ePOSITION;
			sweepHit.distance		= 0.0f;
			sweepHit.position		= box.center;	// PT: this is arbitrary
			sweepHit.normal			= -unitDir;
			return true;
		}
	}

	// Raycast extreme vertex against plane
	bool hitPlane = Gu::intersectRayPlane(boxPts[index], unitDir, worldPlane, sweepHit.distance, &sweepHit.position);
	if(hitPlane && sweepHit.distance > 0 && sweepHit.distance <= distance)
	{
		sweepHit.normal = worldPlane.n;
		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
		return true;
	}
	return false;
}


bool Gu::sweepBox_ConvexGeom(GU_BOX_SWEEP_FUNC_PARAMS)
{
	using namespace Ps::aos;
	PX_ASSERT(geom.getType() == PxGeometryType::eCONVEXMESH);
	const PxConvexMeshGeometry& convexGeom = static_cast<const PxConvexMeshGeometry&>(geom);

	PX_ALIGN_PREFIX(16) PxTransform boxTransform PX_ALIGN_SUFFIX(16); boxTransform = box.getTransform();

#ifdef __SPU__
	PX_COMPILE_TIME_ASSERT(&((Gu::ConvexMesh*)NULL)->getHull()==NULL);
	
	//pxPrintf("sweepBox ptr=%x\n", PxU32(convexGeom.convexMesh));
	PX_ALIGN_PREFIX(16)  PxU8 convexMeshBuffer[sizeof(Gu::ConvexMesh)+32] PX_ALIGN_SUFFIX(16);
	Gu::ConvexMesh* mesh = memFetchAsync<Gu::ConvexMesh>(convexMeshBuffer, MemFetchPtr(convexGeom.convexMesh), sizeof(Gu::ConvexMesh),1);
	memFetchWait(1); // convexMesh	

	const PxU32 nbPolys = mesh->getNbPolygonsFast();
	//pxPrintf("nbPolys=%d\n", nbPolys);
	const Gu::HullPolygonData* PX_RESTRICT polysEA = mesh->getPolygons();
	const PxU32 polysSize = sizeof(Gu::HullPolygonData)*nbPolys + sizeof(PxVec3)*mesh->getNbVerts();
	
 	//TODO: Need optimization with dma cache --jiayang
	void* hullBuffer = PxAlloca(CELL_ALIGN_SIZE_16(polysSize+32));
	Gu::HullPolygonData* polys = memFetchAsync<Gu::HullPolygonData>(hullBuffer, (uintptr_t)(polysEA), polysSize, 1);

	Gu::ConvexHullData* hullData = &mesh->getHull();
	hullData->mPolygons = polys;

	memFetchWait(1); // convexMesh
	//pxPrintf("sweepBox done\n");
#else
	Gu::ConvexHullData* hullData = &static_cast<Gu::ConvexMesh*>(convexGeom.convexMesh)->getHull();
	const PxU32 nbPolys = hullData->mNbPolygons;
#endif

	const Vec3V zeroV = V3Zero();
	const FloatV zero = FZero();

	const QuatV q0 = QuatVLoadA(&boxTransform.q.x);
	const Vec3V p0 = V3LoadA(&boxTransform.p.x); // boxTransform is aligned on the stack

	const QuatV q1 = QuatVLoadU(&pose.q.x);
	const Vec3V p1 = V3LoadU(&pose.p.x);

	const PsTransformV boxPose(p0, q0);
	const PsTransformV convexPose(p1, q1);

	const PsMatTransformV aToB(convexPose.transformInv(boxPose));

	const Vec3V boxExtents = V3LoadU(box.extents);

	const Vec3V vScale = Vec3V_From_Vec4V(V4LoadU(&convexGeom.scale.scale.x));
	const QuatV vQuat = QuatVLoadU(&convexGeom.scale.rotation.x);
	
	Gu::BoxV boxV(zeroV, boxExtents);
	Gu::ConvexHullV convexHull(hullData, zeroV, vScale, vQuat);

	const Vec3V worldDir = V3LoadU(unitDir);
	const FloatV dist = FLoad(distance);
	const Vec3V dir =convexPose.rotateInv(V3Neg(V3Scale(worldDir, dist)));

	FloatV toi;
	Vec3V closestA, normal;

	bool hit = Gu::GJKRelativeRayCast(boxV, convexHull, aToB, zero, zeroV, dir, toi, normal, closestA,
		inflation, !(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP));

	if(hit)
	{
		
		const Vec3V worldPointA = convexPose.transform(closestA);
		
		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;

		if(FAllGrtrOrEq(zero, toi))
		{
			sweepHit.distance		= 0.0f;
			sweepHit.normal			= -unitDir;
			V3StoreU(worldPointA, sweepHit.position);
			//sweepHit.position		= destWorldPointA;	// should be the deepest penetration point
			return true;
		}

		const Vec3V destNormal = V3Normalize(convexPose.rotate(normal));
		const FloatV length = FMul(dist, toi);
		const Vec3V destWorldPointA = V3ScaleAdd(worldDir, length, worldPointA);
		V3StoreU(destNormal, sweepHit.normal);
		V3StoreU(destWorldPointA, sweepHit.position);
		FStore(length, &sweepHit.distance);
	
		// PT: compute closest polygon using the same tweak as in swept-capsule-vs-mesh
		if(1)
		{
			sweepHit.faceIndex = computeSweepConvexPlane(convexGeom,hullData,nbPolys,pose,sweepHit.position,unitDir);
		}

		return true;
	}
	return false;
}

// PT: those two are not working well yet
//#define SORT_TRIS
//#define BEST_TRI_FIRST




/////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Gu::SweepCapsuleTriangles(	PxU32 nbTris, const PxTriangle* triangles,
								const PxCapsuleGeometry& capsuleGeom, const PxTransform& capsulePose,
								const PxVec3& dir, const PxReal length, const PxU32* cachedIndex,
								PxVec3& hit, PxVec3& normal, PxReal& d, PxU32& index, const PxReal inflation, PxHitFlags hintFlags)
{
#ifdef CHECK_SWEEP_CAPSULE_TRIANGLES
	PxVec3 defaultHit, defaultNormal;
	PxReal defaultT;
	bool defaultStatus;
	if(1)	// PT: keep around for a while to easily debug failures
	{
		// We skip the extrusion for the case of the sweep direction matching the capsule axis
		// or
		// if the capsule is in fact a sphere

		PxVec3 capsuleAxis = capsulePose.q.getBasisVector0();
		capsuleAxis.normalize();
		PxReal colinearity = PxAbs(capsuleAxis.dot(dir));
		if((colinearity < (1.0f - LOCAL_EPSILON)) && (capsuleGeom.halfHeight > 0.0f))
		{
			// Extrusion dir = capsule segment
			const PxVec3 ExtrusionDir = capsuleAxis * capsuleGeom.halfHeight;

			if(1)
			{
				PX_ALLOCA(Extruded, PxTriangle, nbTris*7);
				PX_ALLOCA(Ids, PxU32, nbTris*7);

				Gu::Capsule capsule;
				Gu::getCapsule(capsule, capsuleGeom, capsulePose);
				capsule.radius +=inflation;

				// Compute swept box
				Gu::Box capsuleBox;
				Gu::computeBoxAroundCapsule(capsule, capsuleBox);
				//Gu::computeBoxAroundCapsule(capsuleGeom, capsulePose, capsuleBox);

				Gu::Box sweptBounds;
				computeSweptBox(capsuleBox.extents, capsuleBox.center, capsuleBox.rot, dir, length, sweptBounds);

				PxU32 NbExtruded = ExtrudeMesh(nbTris, triangles, ExtrusionDir, Extruded, Ids, dir, &sweptBounds);

				// The nice thing with this approach is that we "just" fallback to already existing code
				// PT: we don't use cachedIndex here since it indexes the non-extruded list
				defaultStatus = sweepSphereTriangles(NbExtruded, Extruded, capsulePose.p, capsuleGeom.radius, dir, length, NULL, defaultHit, defaultNormal, defaultT, index, false);

				if(defaultStatus)
				{
					PX_ASSERT(index!=PX_INVALID_U32);
					index = Ids[index];
				}
			}
		}
		else
		{
			const PxReal capsuleRadius = capsuleGeom.radius + inflation;

			const PxVec3 SphereCenter = capsulePose.p + dir * capsuleGeom.halfHeight;

			defaultStatus = sweepSphereTriangles(nbTris, triangles, SphereCenter, capsuleRadius, dir, length, cachedIndex, defaultHit, defaultNormal, defaultT, index, false);

			if(defaultStatus)
			{
				PX_ASSERT(index!=PX_INVALID_U32);
			}
		}
	}
#endif

	// PT: trying to reuse already existing function.
	if(1)
	{
		Gu::Capsule capsule;
		Gu::getCapsule(capsule, capsuleGeom, capsulePose);
		capsule.radius +=inflation;

		// Compute swept box
		Gu::Box capsuleBox;
		Gu::computeBoxAroundCapsule(capsule, capsuleBox);
		//Gu::computeBoxAroundCapsule(capsuleGeom, capsulePose, capsuleBox);

		Gu::Box sweptBounds;
		computeSweptBox(capsuleBox.extents, capsuleBox.center, capsuleBox.rot, dir, length, sweptBounds);

		bool newStatus = sweepCapsuleTriangles(nbTris, triangles, capsule, dir, length, cachedIndex, d, normal, hit, index,
									hintFlags, false, &sweptBounds);
#ifdef CHECK_SWEEP_CAPSULE_TRIANGLES
		assert(newStatus==defaultStatus);
#endif
		return newStatus;
	}
}


// and finally the jump tables
// (MUST be in order of PxGeometryType)


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool sweepConvex_sphere(const PxGeometry& geom, const PxTransform& pose, const PxConvexMeshGeometry& convexGeom, const PxTransform& convexPose,
							   const PxVec3& unitDir, const PxReal distance, PxSweepHit& sweepHit, PxHitFlags hintFlags, const PxReal inflation)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eSPHERE);
	const PxSphereGeometry& sphereGeom = static_cast<const PxSphereGeometry&>(geom);

	//Gu::ConvexHullData* hullData = getHullData(convexMesh);
#ifdef __SPU__
	PX_COMPILE_TIME_ASSERT(&((Gu::ConvexMesh*)NULL)->getHull()==NULL);
	
	PX_ALIGN_PREFIX(16)  PxU8 convexMeshBuffer[sizeof(Gu::ConvexMesh)+32] PX_ALIGN_SUFFIX(16);
	Gu::ConvexMesh* mesh = memFetchAsync<Gu::ConvexMesh>(convexMeshBuffer, MemFetchPtr(convexGeom.convexMesh), sizeof(Gu::ConvexMesh),1);
	memFetchWait(1); // convexMesh	

	PxU32 nPolys = mesh->getNbPolygonsFast();
	const Gu::HullPolygonData* PX_RESTRICT polysEA = mesh->getPolygons();
	const PxU32 polysSize = sizeof(Gu::HullPolygonData)*nPolys + sizeof(PxVec3)*mesh->getNbVerts();
	
 	//TODO: Need optimization with dma cache --jiayang
	void* hullBuffer = PxAlloca(CELL_ALIGN_SIZE_16(polysSize+32));
	Gu::HullPolygonData* polys = memFetchAsync<Gu::HullPolygonData>(hullBuffer, (uintptr_t)(polysEA), polysSize, 1);

	Gu::ConvexHullData* hullData = &mesh->getHull();
	hullData->mPolygons = polys;

	memFetchWait(1); // convexMesh
#else
	Gu::ConvexHullData* hullData = &static_cast<Gu::ConvexMesh*>(convexGeom.convexMesh)->getHull();
#endif

	const Vec3V zeroV = V3Zero();
	const FloatV zero= FZero();

	const Vec3V vScale = Vec3V_From_Vec4V(V4LoadU(&convexGeom.scale.scale.x));
	const QuatV vQuat = QuatVLoadU(&convexGeom.scale.rotation.x);

	const FloatV sphereRadius = FLoad(sphereGeom.radius);

	const QuatV q0 = QuatVLoadU(&pose.q.x);
	const Vec3V p0 = V3LoadU(&pose.p.x);

	const QuatV q1 = QuatVLoadU(&convexPose.q.x);
	const Vec3V p1 = V3LoadU(&convexPose.p.x);

	const PsTransformV sphereTransf(p0, q0);
	const PsTransformV convexTransf(p1, q1);

	const PsMatTransformV aToB(convexTransf.transformInv(sphereTransf));

	const Vec3V worldDir = V3LoadU(unitDir);
	const FloatV dist = FLoad(distance);
	const Vec3V dir = convexTransf.rotateInv(V3Scale(worldDir, dist));

	Gu::ConvexHullV convexHull(hullData, zeroV, vScale, vQuat);
	//Gu::CapsuleV capsule(zeroV, sphereRadius);
	Gu::CapsuleV capsule(aToB.p, sphereRadius);

	
	FloatV toi;
	Vec3V closestA, normal;
	//bool hit = Gu::GJKRelativeRayCast(capsule, convexHull, aToB, zero, zeroV, dir, toi, normal, closestA, inflation);
	bool hit = Gu::GJKLocalRayCast(capsule, convexHull, zero, zeroV, dir, toi, normal, closestA,
		sphereGeom.radius+inflation, !(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP));


	if(hit)
	{
		closestA = V3NegScaleSub(normal, sphereRadius, closestA);
		const Vec3V destWorldPointA = convexTransf.transform(closestA);
		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;

		if(FAllGrtrOrEq(zero, toi))
		{
			sweepHit.distance		= 0.0f;
			sweepHit.normal			= -unitDir;
			V3StoreU(destWorldPointA, sweepHit.position);
			//sweepHit.position		= destWorldPointA;	// PT: this is arbitrary
			return true;
		}
	
		const Vec3V destNormal = V3Neg(V3Normalize(convexTransf.rotate(normal)));
		const FloatV length = FMul(dist, toi);
		V3StoreU(destNormal, sweepHit.normal);
		V3StoreU(destWorldPointA, sweepHit.position);
		FStore(length, &sweepHit.distance);

		return true;
	}

	return false;

}

static bool sweepConvex_plane(const PxGeometry& geom, const PxTransform& pose, const PxConvexMeshGeometry& convexGeom, const PxTransform& convexPose,
							   const PxVec3& unitDir, const PxReal distance, PxSweepHit& sweepHit, PxHitFlags hintFlags, const PxReal inflation)
{
	PX_ASSERT(geom.getType() == PxGeometryType::ePLANE);
	PX_UNUSED(hintFlags);
	PX_UNUSED(geom);

#ifdef __SPU__
	PX_COMPILE_TIME_ASSERT(&((Gu::ConvexMesh*)NULL)->getHull()==NULL);
	
	PX_ALIGN_PREFIX(16)  PxU8 convexMeshBuffer[sizeof(Gu::ConvexMesh)+32] PX_ALIGN_SUFFIX(16);
	Gu::ConvexMesh* mesh = memFetchAsync<Gu::ConvexMesh>(convexMeshBuffer, MemFetchPtr(convexGeom.convexMesh), sizeof(Gu::ConvexMesh),1);
	memFetchWait(1); // convexMesh	

	PxU32 nPolys = mesh->getNbPolygonsFast();
	const Gu::HullPolygonData* PX_RESTRICT polysEA = mesh->getPolygons();
	const PxU32 polysSize = sizeof(Gu::HullPolygonData)*nPolys + sizeof(PxVec3)*mesh->getNbVerts();
	
 	//TODO: Need optimization with dma cache --jiayang
	void* hullBuffer = PxAlloca(CELL_ALIGN_SIZE_16(polysSize+32));
	Gu::HullPolygonData* polys = memFetchAsync<Gu::HullPolygonData>(hullBuffer, (uintptr_t)(polysEA), polysSize, 1);

	Gu::ConvexHullData* hullData = &mesh->getHull();
	hullData->mPolygons = polys;

	memFetchWait(1); // convexMesh
#else
	Gu::ConvexHullData* hullData = &static_cast<Gu::ConvexMesh*>(convexGeom.convexMesh)->getHull();
#endif

	const PxVec3* PX_RESTRICT hullVertices = hullData->getHullVertices();
	PxU32 numHullVertices = hullData->mNbHullVertices;

	const Cm::FastVertex2ShapeScaling convexScaling(convexGeom.scale);

	PxPlane plane = Gu::getPlane(pose);
	plane.d -=inflation;

	sweepHit.distance	= distance;
	sweepHit.faceIndex	= 0;
	bool status = false;
	while(numHullVertices--)
	{
		const PxVec3& vertex = *hullVertices++;
		const PxVec3 worldPt = convexPose.transform(convexScaling * vertex);
		float t;
		PxVec3 pointOnPlane;
		if(intersectRayPlane(worldPt, unitDir, plane, t, &pointOnPlane))
		{	
			if(plane.distance(worldPt) <= 0.0f)
			{
				// Convex touches plane
				sweepHit.distance		= 0.0f;
				sweepHit.flags			= PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
				sweepHit.position		= worldPt;
				sweepHit.normal			= -unitDir;
				return true;
			}
			if(t > 0.0f && t <= sweepHit.distance)
			{
				sweepHit.distance	= t;
				sweepHit.flags		= PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
				sweepHit.position		= pointOnPlane;
				sweepHit.normal		= plane.n;
				status				= true;
			}
		}
	}
	return status;
}

static bool sweepConvex_capsule(const PxGeometry& geom, const PxTransform& pose, const PxConvexMeshGeometry& convexGeom, const PxTransform& convexPose,
							   const PxVec3& unitDir, const PxReal distance, PxSweepHit& sweepHit, PxHitFlags hintFlags, const PxReal inflation)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eCAPSULE);
	const PxCapsuleGeometry& capsuleGeom = static_cast<const PxCapsuleGeometry&>(geom);

	Gu::Capsule capsule;
	Gu::getCapsule(capsule, capsuleGeom, pose);

	if(sweepCapsule_ConvexGeom(convexGeom, convexPose, capsule, -unitDir, distance, sweepHit, hintFlags, inflation))
	{
		sweepHit.position += unitDir * sweepHit.distance;
		sweepHit.normal = -sweepHit.normal;
		return true;
	}
	return false;
}

static bool sweepConvex_box(const PxGeometry& geom, const PxTransform& pose, const PxConvexMeshGeometry& convexGeom, const PxTransform& convexPose,
							const PxVec3& unitDir, const PxReal distance, PxSweepHit& sweepHit, PxHitFlags hintFlags, const PxReal inflation)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eBOX);
	const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom);

	Gu::Box box;
	buildFrom1(box, pose.p, boxGeom.halfExtents, pose.q);

	//pxPrintf("sweepConvex begin\n");
	if(sweepBox_ConvexGeom(convexGeom, convexPose, box, -unitDir, distance, sweepHit, hintFlags, inflation))
	{
		sweepHit.position += unitDir * sweepHit.distance;
		sweepHit.normal = -sweepHit.normal;
		return true;
	}
	return false;
}

static bool sweepConvex_convexMesh(const PxGeometry& geom, const PxTransform& pose, const PxConvexMeshGeometry& convexGeom, const PxTransform& convexPose,
							   const PxVec3& unitDir, const PxReal distance, PxSweepHit& sweepHit, PxHitFlags hintFlags, const PxReal inflation)
{

	using namespace Ps::aos;
	PX_ASSERT(geom.getType() == PxGeometryType::eCONVEXMESH);
	const PxConvexMeshGeometry& otherConvexGeom = static_cast<const PxConvexMeshGeometry&>(geom);
	Gu::ConvexMesh& otherConvexMesh = *static_cast<Gu::ConvexMesh*>(otherConvexGeom.convexMesh);

#ifdef __SPU__
	PX_COMPILE_TIME_ASSERT(&((Gu::ConvexMesh*)NULL)->getHull()==NULL);
	
	PX_ALIGN_PREFIX(16)  PxU8 convexMeshBuffer[sizeof(Gu::ConvexMesh)+32] PX_ALIGN_SUFFIX(16);
	Gu::ConvexMesh* mesh = memFetchAsync<Gu::ConvexMesh>(convexMeshBuffer, MemFetchPtr(convexGeom.convexMesh), sizeof(Gu::ConvexMesh), 1);
	memFetchWait(1); // convexMesh	
	
	const PxU32 nbPolys = mesh->getNbPolygonsFast();
	const Gu::HullPolygonData* PX_RESTRICT polysEA = mesh->getPolygons();
	const PxU32 polysSize = sizeof(Gu::HullPolygonData)*nbPolys + sizeof(PxVec3)*mesh->getNbVerts();
	
 	//TODO: Need optimization with dma cache --jiayang
	void* hullBuffer = PxAlloca(CELL_ALIGN_SIZE_16(polysSize+32));
	Gu::HullPolygonData* polys = memFetchAsync<Gu::HullPolygonData>(hullBuffer, (uintptr_t)(polysEA), polysSize, 1);

	Gu::ConvexHullData* hullData = &mesh->getHull();
	hullData->mPolygons = polys;

	memFetchWait(1); // convexMesh
#else
	Gu::ConvexHullData* hullData = &static_cast<Gu::ConvexMesh*>(convexGeom.convexMesh)->getHull();
	const PxU32 nbPolys = hullData->mNbPolygons;
#endif

#ifdef __SPU__
	PX_COMPILE_TIME_ASSERT(&((Gu::ConvexMesh*)NULL)->getHull()==NULL);
	
	PX_ALIGN_PREFIX(16)  PxU8 otherconvexMeshBuffer[sizeof(Gu::ConvexMesh)+32] PX_ALIGN_SUFFIX(16);
	Gu::ConvexMesh* otherMesh = memFetchAsync<Gu::ConvexMesh>(otherconvexMeshBuffer, (uintptr_t)(&otherConvexMesh), sizeof(Gu::ConvexMesh),1);
	memFetchWait(1); // convexMesh	

	PxU32 otherNPolys = otherMesh->getNbPolygonsFast();
	const Gu::HullPolygonData* PX_RESTRICT otherPolysEA = otherMesh->getPolygons();
	const PxU32 otherPolysSize = sizeof(Gu::HullPolygonData)*otherNPolys + sizeof(PxVec3)*otherMesh->getNbVerts();
	
 	//TODO: Need optimization with dma cache --jiayang
	void* otherHullBuffer = PxAlloca(CELL_ALIGN_SIZE_16(otherPolysSize+32));
	Gu::HullPolygonData* otherPolys = memFetchAsync<Gu::HullPolygonData>(otherHullBuffer, (uintptr_t)(otherPolysEA), otherPolysSize, 1);

	Gu::ConvexHullData* otherHullData = &otherMesh->getHull();
	otherHullData->mPolygons = otherPolys;

	memFetchWait(1); // convexMesh
#else
	Gu::ConvexHullData* otherHullData = &otherConvexMesh.getHull();	
#endif
	
	const Vec3V zeroV = V3Zero();
	const FloatV zero = FZero();

	const Vec3V otherVScale = V3LoadU(otherConvexGeom.scale.scale);
	const QuatV otherVQuat = QuatVLoadU(&otherConvexGeom.scale.rotation.x);

	const Vec3V vScale = Vec3V_From_Vec4V(V4LoadU(&convexGeom.scale.scale.x));
	const QuatV vQuat = QuatVLoadU(&convexGeom.scale.rotation.x);

	const QuatV q0 = QuatVLoadU(&pose.q.x);
	const Vec3V p0 = V3LoadU(&pose.p.x);

	const QuatV q1 = QuatVLoadU(&convexPose.q.x);
	const Vec3V p1 = V3LoadU(&convexPose.p.x);

	const PsTransformV otherTransf(p0, q0);
	const PsTransformV convexTransf(p1, q1);

	const Vec3V worldDir = V3LoadU(unitDir);
	const FloatV dist = FLoad(distance);
	const Vec3V dir = convexTransf.rotateInv(V3Scale(worldDir, dist));

	const PsMatTransformV aToB(convexTransf.transformInv(otherTransf));
	
	Gu::ConvexHullV otherConvexHull(otherHullData, zeroV, otherVScale, otherVQuat);
	Gu::ConvexHullV convexHull(hullData, zeroV, vScale, vQuat);

	
	FloatV toi;
	Vec3V closestA, normal;
	bool hit = Gu::GJKRelativeRayCast(otherConvexHull, convexHull, aToB, zero, zeroV, dir, toi, normal, closestA,
		inflation, !(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP));

	if(hit)
	{
		const Vec3V worldPointA = convexTransf.transform(closestA);

		sweepHit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;

		if(FAllGrtrOrEq(zero, toi))
		{
			sweepHit.distance		= 0.0f;
			sweepHit.normal			= -unitDir;
			V3StoreU(worldPointA, sweepHit.position);
			//sweepHit.position		= destWorldPointA;	// should be the deepest penetration point
			return true;
		}
	
		const Vec3V destNormal = V3Neg(V3Normalize(convexTransf.rotate(normal)));
		const FloatV length = FMul(dist, toi);
		V3StoreU(destNormal, sweepHit.normal);
		V3StoreU(worldPointA, sweepHit.position);
		FStore(length, &sweepHit.distance);

		// PT: compute closest polygon using the same tweak as in swept-capsule-vs-mesh
		if(1)
		{
			sweepHit.faceIndex = computeSweepConvexPlane(convexGeom,hullData,nbPolys,pose,sweepHit.position,unitDir);
		}

		return true;
	}

	return false;

}

// return true if hit, false if no hit
static PX_FORCE_INLINE bool sweepConvexVsTriangle(
	const PxVec3& v0, const PxVec3& v1, const PxVec3& v2,
	Gu::ConvexHullV& convexHull, const Ps::aos::PsMatTransformV& meshToConvex, const Ps::aos::PsTransformV& convexTransfV,
	const Ps::aos::Vec3VArg convexSpaceDir, const PxVec3& unitDir, const PxVec3& meshSpaceUnitDir,
	const Ps::aos::FloatVArg fullDistance, PxU32 hintFlags, PxReal shrunkDistance,
	PxSweepHit& hit, bool isDoubleSided, const PxReal inflation)
{
	using namespace Ps::aos;
	// Create triangle normal
	const PxVec3 denormalizedNormal = (v1 - v0).cross(v2 - v1);

	// Backface culling
	// PT: WARNING, the test is reversed compared to usual because we pass -unitDir to this function
//	const bool culled = !isDoubleSided && (denormalizedNormal.dot(unitDir) > 0.0f);
	const bool culled = !isDoubleSided && (denormalizedNormal.dot(meshSpaceUnitDir) <= 0.0f);
//	const bool culled = !isDoubleSided && (transform1.rotate(denormalizedNormal).dot(unitDir) <= 0.0f);
	

	const Vec3V zeroV = V3Zero();
	const FloatV zero = FZero();

	const Vec3V p0 = V3LoadU(v0); // in mesh local space
	const Vec3V	p1 = V3LoadU(v1);
	const Vec3V p2 = V3LoadU(v2);

	// transform triangle verts from mesh local to convex local space
	Gu::TriangleV triangleV(meshToConvex.transform(p0), meshToConvex.transform(p1), meshToConvex.transform(p2));
	
	//ML: GJK sweep will catch hits at TOI = 0 so no need for extra work here but we do want to exit if we weren't
	//interested in initial overlaps
	// AP: if culled (sweep is facing away from triangle normal) and no test for initial overlap is required, return no hit
	if(culled && (hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP) != 0)
		return false;

	FloatV toi;
	Vec3V closestA,normal;

	// run GJK raycast
	// sweep triangle in convex local space vs convex, closestA will be the impact point in convex local space
	bool gjkHit = Gu::GJKLocalRayCast(
		triangleV, convexHull, zero, zeroV, convexSpaceDir, toi, normal, closestA, inflation,
		!(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP));
	if (gjkHit)
	{
		const FloatV minDist = FLoad(shrunkDistance);
		const Vec3V destWorldPointA = convexTransfV.transform(closestA);
		const Vec3V destNormal = V3Normalize(convexTransfV.rotate(normal));

		hit.flags = PxHitFlag::eDISTANCE | PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;

		if(!(hintFlags & PxHitFlag::eASSUME_NO_INITIAL_OVERLAP)) // is initial overlap enabled?
			if(FAllGrtrOrEq(zero, toi)) // test if shapes initially overlap (toi==0?)
			{
				// If toi=0 shapes initially overlap
				// need to fill distance and normal because GJKsweep can't produce a meaningful normal at toi=0 without EPA
				hit.distance	= 0.0f;
				hit.normal		= -unitDir;
				V3StoreU(destWorldPointA, hit.position);
				return true; // report a hit
			}

		const FloatV dist = FMul(toi, fullDistance); // scale the toi to original full sweep distance
		
		if(FAllGrtr(minDist, dist)) // is current dist < minDist?
		{
			V3StoreU(destWorldPointA, hit.position);
			V3StoreU(destNormal, hit.normal);
			FStore(dist, &hit.distance);
			return true; // report a hit
		}
	}

	return false; // report no hit
}

	struct ConvexVsMeshSweepCallback : VolumeColliderTrigCallback
	{
		ConvexVsMeshSweepCallback(
			const Gu::ConvexHullData& hull, const PxMeshScale& convexScale, const Cm::FastVertex2ShapeScaling& meshScale,
			const PxTransform& convexPose, const PxTransform& meshPose,
			const PxVec3& unitDir, const PxReal distance, PxHitFlags hintFlags, const bool isDoubleSided, const PxReal inflation,
			const bool anyHit) :
				mMeshScale		(meshScale),
				mUnitDir		(unitDir),
				mInflation		(inflation),
				mHintFlags		(hintFlags),
				mAnyHit			(anyHit),
				mIsDoubleSided	(isDoubleSided)
				
		{
			mHit.distance = distance; // this will be shrinking progressively as we sweep and clip the sweep length
			mHit.faceIndex = 0xFFFFffff;

			mMeshSpaceUnitDir = meshPose.rotateInv(unitDir);
		
			const Vec3V worldDir = V3LoadU(unitDir);
			const FloatV dist = FLoad(distance);
			const QuatV q0 = QuatVLoadU(&meshPose.q.x);
			const Vec3V p0 = V3LoadU(&meshPose.p.x);

			const QuatV q1 = QuatVLoadU(&convexPose.q.x);
			const Vec3V p1 = V3LoadU(&convexPose.p.x);

			const PsTransformV meshPoseV(p0, q0);
			const PsTransformV convexPoseV(p1, q1);

			mMeshToConvex = convexPoseV.transformInv(meshPoseV);
			mConvexPoseV = convexPoseV;
			mConvexSpaceDir = convexPoseV.rotateInv(V3Neg(V3Scale(worldDir, dist)));
			mInitialDistance = dist;

			const Vec3V vScale = V3LoadU(convexScale.scale);
			const QuatV vQuat = QuatVLoadU(&convexScale.rotation.x);
			mConvexHull.initialize(&hull, V3Zero(), vScale, vQuat);
		}
		virtual ~ConvexVsMeshSweepCallback()	{}

		// PT: TODO: optimize this
		virtual PxAgain processResults(PxU32 count, const PxVec3* verts, const PxU32* triIndices, const PxU32*)
		{
			//pxPrintf("in ConvexMeshSweepCallback, tricount=%d, hullvcount=%d\n", count, mConvexHull.numVerts);
			for (PxU32 i = 0; i < count; i++)
			{
				const PxVec3 v0 = mMeshScale * verts[0];
				const PxVec3 v1 = mMeshScale * verts[1];
				const PxVec3 v2 = mMeshScale * verts[2];
				verts += 3;

				// mHit will be updated if sweep distance is < input mHit.distance
				const PxReal oldDist = mHit.distance;
				if(sweepConvexVsTriangle(
					v0, v1, v2, mConvexHull, mMeshToConvex, mConvexPoseV, mConvexSpaceDir,
					mUnitDir, mMeshSpaceUnitDir, mInitialDistance, mHintFlags, oldDist, mHit, mIsDoubleSided, mInflation))
				{
					mHit.faceIndex = triIndices[i]; // record the triangle index
					if (mAnyHit)
						return false; // abort traversal
				}
			}
			return true; // continue traversal
		}
	
		Gu::ConvexHullV						mConvexHull;
		PsMatTransformV						mMeshToConvex;
		PsTransformV						mConvexPoseV;
		const Cm::FastVertex2ShapeScaling&	mMeshScale;
		PxSweepHit							mHit; // stores either the closest or any hit depending on value of mAnyHit
		FloatV								mInitialDistance;
		Vec3V								mConvexSpaceDir; // convexPose.rotateInv(-unit*distance)
		PxVec3								mUnitDir;
		PxVec3								mMeshSpaceUnitDir;
		PxReal								mInflation;
		PxU32								mHintFlags;
		const bool							mAnyHit;
		const bool							mIsDoubleSided;

	private:
		ConvexVsMeshSweepCallback& operator=(const ConvexVsMeshSweepCallback&);
	};


static bool sweepConvex_triangleMesh(
	const PxGeometry& aMeshGeom, const PxTransform& meshPose, const PxConvexMeshGeometry& convexGeom,
	const PxTransform& convexPose, const PxVec3& unitDir, const PxReal distance,
	PxSweepHit& sweepHit, PxHitFlags hintFlags, const PxReal inflation)
{
	PX_ASSERT(aMeshGeom.getType() == PxGeometryType::eTRIANGLEMESH);
	const PxTriangleMeshGeometry& meshGeom = static_cast<const PxTriangleMeshGeometry&>(aMeshGeom);

	Gu::ConvexMesh* cm = static_cast<Gu::ConvexMesh*>(convexGeom.convexMesh);

#if __SPU__
	// On SPU: fetch data referenced via pointers
	PX_ALIGN_PREFIX(16)  PxU8 convexMeshBuffer[sizeof(Gu::ConvexMesh)+32] PX_ALIGN_SUFFIX(16);
	cm = memFetchAsync<Gu::ConvexMesh>(convexMeshBuffer, MemFetchPtr(convexGeom.convexMesh), sizeof(Gu::ConvexMesh),1);
	memFetchWait(1); // convexMesh	

	PxU32 nPolys = cm->getNbPolygonsFast();
	const Gu::HullPolygonData* PX_RESTRICT polysEA = cm->getPolygons();
	const PxU32 polysSize = sizeof(Gu::HullPolygonData)*nPolys + sizeof(PxVec3)*cm->getNbVerts();
	//pxPrintf("cmtrimesh polysEA=%x\n", PxU32(polysEA));
	
 	//TODO: Need optimization with dma cache --jiayang
	void* hullBuffer = PxAlloca(CELL_ALIGN_SIZE_16(polysSize+32));
	Gu::HullPolygonData* polys = memFetchAsync<Gu::HullPolygonData>(hullBuffer, (uintptr_t)(polysEA), polysSize, 1);

	Gu::ConvexHullData* hullData = &cm->getHull();
	hullData->mPolygons = polys;

	memFetchWait(1); // convex mesh polygons
	//pxPrintf("sweepCap done\n");
#endif

	GU_FETCH_MESH_DATA(meshGeom);

	const Gu::RTreeMidphase& collisionModel = meshData->mOpcodeModel;

	const bool idtScaleConvex = convexGeom.scale.isIdentity();
	const bool idtScaleMesh = meshGeom.scale.isIdentity();

	Cm::FastVertex2ShapeScaling convexScaling;
	if(!idtScaleConvex)
		convexScaling.init(convexGeom.scale);

	Cm::FastVertex2ShapeScaling meshScaling;
	if(!idtScaleMesh)
		meshScaling.init(meshGeom.scale);

	PX_ASSERT(!cm->getLocalBoundsFast().isEmpty());
	PxBounds3 hullAABB = PxBounds3::transformFast(convexScaling.getVertex2ShapeSkew(), cm->getLocalBoundsFast());

	Gu::Box hullOBB;
	computeHullOBB(hullOBB, hullAABB, 0.0f, convexPose, Cm::Matrix34(convexPose), Cm::Matrix34(meshPose), meshScaling, idtScaleMesh);
	//~PT: TODO: this part similar to convex-vs-overlap test, refactor

	hullOBB.extents.x += inflation;
	hullOBB.extents.y += inflation;
	hullOBB.extents.z += inflation;
	// Now create temporal bounds
	Gu::Box querySweptBox;
	CreateSweptOBB(querySweptBox, hullOBB, meshPose.rotateInv(unitDir), distance);

	// PT: TODO: this part similar to convex-vs-overlap test, refactor
	//Gu::RTreeMidphaseData hmd;	// PT: I suppose doing the "conversion" at runtime is fine
	//tm->mMesh.mData.mOpcodeModel.getRTreeMidphaseData(hmd);

	RTreeMidphaseData hmd;	// PT: I suppose doing the "conversion" at runtime is fine
	collisionModel.getRTreeMidphaseData(hmd);

	Gu::HybridOBBCollider collider;
	collider.SetPrimitiveTests(true);
	//~PT: TODO: this part similar to convex-vs-overlap test, refactor

	const bool isDoubleSided = meshGeom.meshFlags & PxMeshGeometryFlag::eDOUBLE_SIDED;
	ConvexVsMeshSweepCallback cb(
		cm->getHull(), convexGeom.scale, meshScaling, convexPose, meshPose, -unitDir, distance, hintFlags,
		isDoubleSided, inflation, hintFlags.isSet(PxHitFlag::eMESH_ANY));
	// AP: careful with changing the template params - can negatively affect SPU_Sweep module size
	//pxPrintf("Before collide\n");
	collider.Collide<1,1,1>(querySweptBox, hmd, &cb, NULL, NULL);
	//pxPrintf("After collide\n");
	if(cb.mHit.faceIndex != 0xFFFFffff)
	{
		sweepHit = cb.mHit;
		//sweepHit.position += unitDir * sweepHit.distance;
		sweepHit.normal = -sweepHit.normal;
		sweepHit.normal.normalize();
		if(isDoubleSided)
		{
			// PT: make sure the normal is properly oriented when we hit a back-triangle
			if(sweepHit.normal.dot(unitDir)>0.0f)
				sweepHit.normal = -sweepHit.normal;
		}
		//pxPrintf("sweepConvex_triangleMesh returning true\n");
		return true;
	}
	//pxPrintf("sweepConvex_triangleMesh returning false\n");
	return false;
}

class ConvexVsHeightfieldSweep : public Gu::EntityReport<PxU32>
{
public:
		ConvexVsHeightfieldSweep(
			Gu::HeightFieldUtil& hfUtil,
			const Gu::ConvexHullData& hull,
			const PxMeshScale& convexScale,
			const PxTransform& convexTrans,
			const PxTransform& hightFieldTrans,
			const PxVec3& unitDir, const PxReal distance, PxHitFlags hintFlags, const PxReal inflation,
			const bool aAnyHit) :
				mHfUtil			(hfUtil),
				mUnitDir		(unitDir),
				mInflation		(inflation),
				mHintFlags		(hintFlags),
				mAnyHit			(aAnyHit)

		{
			using namespace Ps::aos;
			mHit.faceIndex = 0xFFFFffff;
			mHit.distance = distance;
			const Vec3V worldDir = V3LoadU(unitDir);
			const FloatV dist = FLoad(distance);
			const QuatV q0 = QuatVLoadU(&hightFieldTrans.q.x);
			const Vec3V p0 = V3LoadU(&hightFieldTrans.p.x);

			const QuatV q1 = QuatVLoadU(&convexTrans.q.x);
			const Vec3V p1 = V3LoadU(&convexTrans.p.x);

			const PsTransformV meshTransf(p0, q0);
			const PsTransformV convexTransf(p1, q1);

			mMeshToConvex = convexTransf.transformInv(meshTransf);
			mConvexPoseV = convexTransf;
			mConvexSpaceDir = convexTransf.rotateInv(V3Neg(V3Scale(worldDir, dist)));
			mDistance = dist;

			const Vec3V vScale = V3LoadU(convexScale.scale);
			const QuatV vQuat = QuatVLoadU(&convexScale.rotation.x);

			mMeshSpaceUnitDir = hightFieldTrans.rotateInv(unitDir);
			mConvexHull.initialize(&hull, V3Zero(), vScale, vQuat);
		}

		virtual PxAgain onEvent(PxU32 nbEntities, PxU32* entities)
		{
			const PxTransform idt = PxTransform(PxIdentity);
			for(PxU32 i = 0; i < nbEntities; i++)
			{
				PxTriangle tri;
				mHfUtil.getTriangle(idt, tri, NULL, NULL, entities[i], false, false);  // First parameter not needed if local space triangle is enough

				// use mHit.distance as max sweep distance so far, mHit.distance will be clipped by this function
				if(sweepConvexVsTriangle(tri.verts[0], tri.verts[1], tri.verts[2], mConvexHull, mMeshToConvex, mConvexPoseV,
					mConvexSpaceDir, mUnitDir, mMeshSpaceUnitDir, mDistance, mHintFlags, mHit.distance, mHit, false, mInflation))
				{
					mHit.faceIndex = entities[i]; // update faceIndex
					if (mAnyHit)
						return false; // abort traversal
				}
			}
			return true; // continue traversal
		}

		Gu::HeightFieldUtil&	mHfUtil;
		PsMatTransformV			mMeshToConvex;
		PsTransformV			mConvexPoseV;
		Gu::ConvexHullV			mConvexHull;
		PxSweepHit				mHit;
		Vec3V					mConvexSpaceDir;
		FloatV					mDistance;
		PxVec3					mUnitDir;
		PxVec3					mMeshSpaceUnitDir;
		PxReal					mInflation;
		PxU32					mHintFlags;
		const bool				mAnyHit;

private:
	ConvexVsHeightfieldSweep& operator=(const ConvexVsHeightfieldSweep&);
};

static bool sweepConvex_heightField(
	const PxGeometry& geom, const PxTransform& pose, const PxConvexMeshGeometry& convexGeom, const PxTransform& convexPose,
	const PxVec3& unitDir, const PxReal distance, PxSweepHit& sweepHit, PxHitFlags hintFlags, const PxReal inflation)
{
	PX_ASSERT(geom.getType() == PxGeometryType::eHEIGHTFIELD);
	const PxHeightFieldGeometry& hfGeom = static_cast<const PxHeightFieldGeometry&>(geom);

	const Cm::Matrix34 convexTM(convexPose);
	const Cm::Matrix34 meshTM(pose);

	Gu::ConvexMesh* cm = static_cast<Gu::ConvexMesh*>(convexGeom.convexMesh);
#if __SPU__
	PX_ALIGN_PREFIX(16)  PxU8 convexMeshBuffer[sizeof(Gu::ConvexMesh)+32] PX_ALIGN_SUFFIX(16);
	cm = memFetchAsync<Gu::ConvexMesh>(convexMeshBuffer, MemFetchPtr(convexGeom.convexMesh), sizeof(Gu::ConvexMesh),1);
	memFetchWait(1); // convexMesh	

	PxU32 nPolys = cm->getNbPolygonsFast();
	const Gu::HullPolygonData* PX_RESTRICT polysEA = cm->getPolygons();
	const PxU32 polysSize = sizeof(Gu::HullPolygonData)*nPolys + sizeof(PxVec3)*cm->getNbVerts();
	//pxPrintf("cmtrimesh polysEA=%x\n", PxU32(polysEA));
	
 	//TODO: Need optimization with dma cache --jiayang
	void* hullBuffer = PxAlloca(CELL_ALIGN_SIZE_16(polysSize+32));
	Gu::HullPolygonData* polys = memFetchAsync<Gu::HullPolygonData>(hullBuffer, (uintptr_t)(polysEA), polysSize, 1);

	Gu::ConvexHullData* hullData = &cm->getHull();
	hullData->mPolygons = polys;

	memFetchWait(1); // convex mesh polygons
#endif
//	const Gu::TriangleMesh* tm = static_cast<Gu::TriangleMesh*>(meshGeom.triangleMesh);

	const bool idtScaleConvex = convexGeom.scale.isIdentity();
//	const bool idtScaleMesh = meshGeom.scale.isIdentity();
	const bool idtScaleMesh = true;

	Cm::FastVertex2ShapeScaling convexScaling;
	if(!idtScaleConvex)
		convexScaling.init(convexGeom.scale);

	Cm::FastVertex2ShapeScaling meshScaling;
//	if(!idtScaleMesh)
//		meshScaling.init(meshGeom.scale);

	PX_ASSERT(!cm->getLocalBoundsFast().isEmpty());
	PxBounds3 hullAABB = PxBounds3::transformFast(convexScaling.getVertex2ShapeSkew(), cm->getLocalBoundsFast());

	Gu::Box hullOBB;
	computeHullOBB(hullOBB, hullAABB, 0.0f, convexPose, convexTM, meshTM, meshScaling, idtScaleMesh);

	hullOBB.extents.x += inflation;
	hullOBB.extents.y += inflation;
	hullOBB.extents.z += inflation;
	// Now create temporal bounds
	Gu::Box querySweptBox;
	CreateSweptOBB(querySweptBox, hullOBB, pose.rotateInv(unitDir), distance);

	// from MeshQuery::findOverlapHeightField
	const PxBounds3 bounds = PxBounds3::basisExtent(querySweptBox.center, querySweptBox.rot, querySweptBox.extents);

#ifdef __SPU__
	PX_ALIGN_PREFIX(16)  PxU8 heightFieldBuffer[sizeof(Gu::HeightField)+32] PX_ALIGN_SUFFIX(16);
	Gu::HeightField* heightField = memFetchAsync<Gu::HeightField>(heightFieldBuffer, (uintptr_t)(hfGeom.heightField), sizeof(Gu::HeightField), 1);
	memFetchWait(1);
	g_sampleCache.init((uintptr_t)(heightField->getData().samples), heightField->getData().tilesU);

	const_cast<PxHeightFieldGeometry&>(hfGeom).heightField = heightField;
#endif

	Gu::HeightFieldUtil hfUtil(hfGeom);
	ConvexVsHeightfieldSweep entityReport(
		hfUtil, cm->getHull(), convexGeom.scale, convexPose, pose, -unitDir, distance, hintFlags, inflation,
		hintFlags.isSet(PxHitFlag::eMESH_ANY));

	hfUtil.overlapAABBTriangles(pose, bounds, 0, &entityReport);

	if (entityReport.mHit.faceIndex != 0xFFFFffff)
	{
		sweepHit = entityReport.mHit;
		sweepHit.normal = -sweepHit.normal;
		sweepHit.normal.normalize();
		return true;
	}
	return false;
}

const Gu::SweepConvexFunc* Gu::GetSweepConvexMap()
{
	return &Gu::gSweepConvexMap[0];
}

const Gu::SweepConvexFunc Gu::gSweepConvexMap[7] = 
{
#if 1 // AP: this is for quickly shrinking SPU module size for debugging
	sweepConvex_sphere, // 0
	sweepConvex_plane, // 1
	sweepConvex_capsule, // 2
	sweepConvex_box, // 3
	sweepConvex_convexMesh, // 4
	sweepConvex_triangleMesh, // 5
	sweepConvex_heightField // 6
#else
	sweepConvex_plane,
	sweepConvex_plane,
	sweepConvex_plane,
	sweepConvex_plane,
	sweepConvex_plane,
	sweepConvex_plane,
	sweepConvex_plane,
#endif
};
