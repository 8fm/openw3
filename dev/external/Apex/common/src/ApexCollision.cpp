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

#include "NxApexDefs.h"
#include "ApexCollision.h"

#if NX_SDK_VERSION_MAJOR == 2
#include "NxBox.h"
#include "NxCapsule.h"
#include "NxFromPx.h"
#endif

namespace physx
{
namespace apex
{

#if NX_SDK_VERSION_MAJOR == 2
bool capsuleCapsuleIntersection(const NxCapsule& worldCaps0, const NxCapsule& worldCaps1, PxF32 tolerance)
{
	Capsule caps0, caps1;
	caps0.p0 = PxFromNxVec3Fast(worldCaps0.p0);
	caps0.p1 = PxFromNxVec3Fast(worldCaps0.p1);
	caps0.radius = worldCaps0.radius;

	caps1.p0 = PxFromNxVec3Fast(worldCaps1.p0);
	caps1.p1 = PxFromNxVec3Fast(worldCaps1.p1);
	caps1.radius = worldCaps1.radius;

	return capsuleCapsuleIntersection(caps0, caps1, tolerance);
}

bool boxBoxIntersection(const NxBox& worldBox0, const NxBox& worldBox1)
{
	Box box0, box1;
	box0.center = PxFromNxVec3Fast(worldBox0.center);
	box0.extents = PxFromNxVec3Fast(worldBox0.extents);
	worldBox0.rot.getColumnMajor(&box0.rot.column0.x);

	box1.center = PxFromNxVec3Fast(worldBox1.center);
	box1.extents = PxFromNxVec3Fast(worldBox1.extents);
	worldBox1.rot.getColumnMajor(&box1.rot.column0.x);

	return boxBoxIntersection(box0, box1);
}

PxU32 APEX_RayCapsuleIntersect(const PxVec3& origin, const PxVec3& dir, const NxCapsule& capsule, PxF32 s[2])
{
	Capsule caps;
	caps.p0 = PxFromNxVec3Fast(capsule.p0);
	caps.p1 = PxFromNxVec3Fast(capsule.p1);
	caps.radius = capsule.radius;

	return APEX_RayCapsuleIntersect(origin, dir, caps, s);
}


#endif

bool capsuleCapsuleIntersection(const Capsule& worldCaps0, const Capsule& worldCaps1, PxF32 tolerance)
{
	PxF32 s, t;
	PxF32 squareDist = APEX_segmentSegmentSqrDist(worldCaps0, worldCaps1, &s, &t);

	PxF32 totRad = (worldCaps0.radius * tolerance) + (worldCaps1.radius * tolerance);	//incl a bit of tolerance.
	return squareDist < totRad * totRad;
}

//----------------------------------------------------------------------------//

/// \todo replace this hack
bool boxBoxIntersection(const Box& worldBox0, const Box& worldBox1)
{
	Capsule worldCaps0, worldCaps1;
	worldCaps0.p0 = worldBox0.center - worldBox0.rot * PxVec3(0, worldBox0.extents.y, 0);
	worldCaps0.p1 = worldBox0.center + worldBox0.rot * PxVec3(0, worldBox0.extents.y, 0);
	worldCaps0.radius = worldBox0.extents.x / 0.7f;
	worldCaps1.p0 = worldBox1.center - worldBox1.rot * PxVec3(0, worldBox1.extents.y, 0);
	worldCaps1.p1 = worldBox1.center + worldBox1.rot * PxVec3(0, worldBox1.extents.y, 0);
	worldCaps1.radius = worldBox1.extents.x / 0.7f;

	PxF32 s, t;
	PxF32 squareDist = APEX_segmentSegmentSqrDist(worldCaps0, worldCaps1, &s, &t);

	PxF32 totRad = (worldCaps0.radius * 1.2f) + (worldCaps1.radius * 1.2f);	//incl a bit of tolerance.
	return squareDist < totRad * totRad;
}

//----------------------------------------------------------------------------//

PxF32 APEX_pointTriangleSqrDst(const Triangle& triangle, const PxVec3& position)
{
	PxVec3 d1 = triangle.v1 - triangle.v0;
	PxVec3 d2 = triangle.v2 - triangle.v0;
	PxVec3 pp1 = position - triangle.v0;
	PxF32 a = d1.dot(d1);
	PxF32 b = d2.dot(d1);
	PxF32 c = pp1.dot(d1);
	PxF32 d = b;
	PxF32 e = d2.dot(d2);
	PxF32 f = pp1.dot(d2);
	PxF32 det = a * e - b * d;
	if (det != 0.0f)
	{
		PxF32 s = (c * e - b * f) / det;
		PxF32 t = (a * f - c * d) / det;
		if (s > 0.0f && t > 0.0f && (s + t) < 1.0f)
		{
			PxVec3 q = triangle.v0 + d1 * s + d2 * t;
			return (q  - position).magnitudeSquared();
		}
	}
	Segment segment;
	segment.p0 = triangle.v0;
	segment.p1 = triangle.v1;
	PxF32 dist = APEX_pointSegmentSqrDist(segment	, position, NULL);
	segment.p0 = triangle.v1;
	segment.p1 = triangle.v2;
	dist = PxMin(dist, APEX_pointSegmentSqrDist(segment, position, NULL));
	segment.p0 = triangle.v2;
	segment.p1 = triangle.v0;
	dist = PxMin(dist, APEX_pointSegmentSqrDist(segment, position, NULL));
	return dist;

}

//----------------------------------------------------------------------------//

#define PARALLEL_TOLERANCE	1e-02f

PxF32 APEX_segmentSegmentSqrDist(const Segment& seg0, const Segment& seg1, PxF32* s, PxF32* t)
{
	PxVec3 rkSeg0Direction	= seg0.p1 - seg0.p0;
	PxVec3 rkSeg1Direction	= seg1.p1 - seg1.p0;

	PxVec3 kDiff	= seg0.p0 - seg1.p0;
	PxF32 fA00	= rkSeg0Direction.magnitudeSquared();
	PxF32 fA01	= -rkSeg0Direction.dot(rkSeg1Direction);
	PxF32 fA11	= rkSeg1Direction.magnitudeSquared();
	PxF32 fB0	= kDiff.dot(rkSeg0Direction);
	PxF32 fC	= kDiff.magnitudeSquared();
	PxF32 fDet	= PxAbs(fA00 * fA11 - fA01 * fA01);

	PxF32 fB1, fS, fT, fSqrDist, fTmp;

	if (fDet >= PARALLEL_TOLERANCE)
	{
		// line segments are not parallel
		fB1 = -kDiff.dot(rkSeg1Direction);
		fS = fA01 * fB1 - fA11 * fB0;
		fT = fA01 * fB0 - fA00 * fB1;

		if (fS >= 0.0f)
		{
			if (fS <= fDet)
			{
				if (fT >= 0.0f)
				{
					if (fT <= fDet) // region 0 (interior)
					{
						// minimum at two interior points of 3D lines
						PxF32 fInvDet = 1.0f / fDet;
						fS *= fInvDet;
						fT *= fInvDet;
						fSqrDist = fS * (fA00 * fS + fA01 * fT + 2.0f * fB0) +
						           fT * (fA01 * fS + fA11 * fT + 2.0f * fB1) + fC;
					}
					else  // region 3 (side)
					{
						fT = 1.0f;
						fTmp = fA01 + fB0;
						if (fTmp >= 0.0f)
						{
							fS = 0.0f;
							fSqrDist = fA11 + 2.0f * fB1 + fC;
						}
						else if (-fTmp >= fA00)
						{
							fS = 1.0f;
							fSqrDist = fA00 + fA11 + fC + 2.0f * (fB1 + fTmp);
						}
						else
						{
							fS = -fTmp / fA00;
							fSqrDist = fTmp * fS + fA11 + 2.0f * fB1 + fC;
						}
					}
				}
				else  // region 7 (side)
				{
					fT = 0.0f;
					if (fB0 >= 0.0f)
					{
						fS = 0.0f;
						fSqrDist = fC;
					}
					else if (-fB0 >= fA00)
					{
						fS = 1.0f;
						fSqrDist = fA00 + 2.0f * fB0 + fC;
					}
					else
					{
						fS = -fB0 / fA00;
						fSqrDist = fB0 * fS + fC;
					}
				}
			}
			else
			{
				if (fT >= 0.0)
				{
					if (fT <= fDet)    // region 1 (side)
					{
						fS = 1.0f;
						fTmp = fA01 + fB1;
						if (fTmp >= 0.0f)
						{
							fT = 0.0f;
							fSqrDist = fA00 + 2.0f * fB0 + fC;
						}
						else if (-fTmp >= fA11)
						{
							fT = 1.0f;
							fSqrDist = fA00 + fA11 + fC + 2.0f * (fB0 + fTmp);
						}
						else
						{
							fT = -fTmp / fA11;
							fSqrDist = fTmp * fT + fA00 + 2.0f * fB0 + fC;
						}
					}
					else  // region 2 (corner)
					{
						fTmp = fA01 + fB0;
						if (-fTmp <= fA00)
						{
							fT = 1.0f;
							if (fTmp >= 0.0f)
							{
								fS = 0.0f;
								fSqrDist = fA11 + 2.0f * fB1 + fC;
							}
							else
							{
								fS = -fTmp / fA00;
								fSqrDist = fTmp * fS + fA11 + 2.0f * fB1 + fC;
							}
						}
						else
						{
							fS = 1.0f;
							fTmp = fA01 + fB1;
							if (fTmp >= 0.0f)
							{
								fT = 0.0f;
								fSqrDist = fA00 + 2.0f * fB0 + fC;
							}
							else if (-fTmp >= fA11)
							{
								fT = 1.0f;
								fSqrDist = fA00 + fA11 + fC + 2.0f * (fB0 + fTmp);
							}
							else
							{
								fT = -fTmp / fA11;
								fSqrDist = fTmp * fT + fA00 + 2.0f * fB0 + fC;
							}
						}
					}
				}
				else  // region 8 (corner)
				{
					if (-fB0 < fA00)
					{
						fT = 0.0f;
						if (fB0 >= 0.0f)
						{
							fS = 0.0f;
							fSqrDist = fC;
						}
						else
						{
							fS = -fB0 / fA00;
							fSqrDist = fB0 * fS + fC;
						}
					}
					else
					{
						fS = 1.0f;
						fTmp = fA01 + fB1;
						if (fTmp >= 0.0f)
						{
							fT = 0.0f;
							fSqrDist = fA00 + 2.0f * fB0 + fC;
						}
						else if (-fTmp >= fA11)
						{
							fT = 1.0f;
							fSqrDist = fA00 + fA11 + fC + 2.0f * (fB0 + fTmp);
						}
						else
						{
							fT = -fTmp / fA11;
							fSqrDist = fTmp * fT + fA00 + 2.0f * fB0 + fC;
						}
					}
				}
			}
		}
		else
		{
			if (fT >= 0.0f)
			{
				if (fT <= fDet)    // region 5 (side)
				{
					fS = 0.0f;
					if (fB1 >= 0.0f)
					{
						fT = 0.0f;
						fSqrDist = fC;
					}
					else if (-fB1 >= fA11)
					{
						fT = 1.0f;
						fSqrDist = fA11 + 2.0f * fB1 + fC;
					}
					else
					{
						fT = -fB1 / fA11;
						fSqrDist = fB1 * fT + fC;
					}
				}
				else  // region 4 (corner)
				{
					fTmp = fA01 + fB0;
					if (fTmp < 0.0f)
					{
						fT = 1.0f;
						if (-fTmp >= fA00)
						{
							fS = 1.0f;
							fSqrDist = fA00 + fA11 + fC + 2.0f * (fB1 + fTmp);
						}
						else
						{
							fS = -fTmp / fA00;
							fSqrDist = fTmp * fS + fA11 + 2.0f * fB1 + fC;
						}
					}
					else
					{
						fS = 0.0f;
						if (fB1 >= 0.0f)
						{
							fT = 0.0f;
							fSqrDist = fC;
						}
						else if (-fB1 >= fA11)
						{
							fT = 1.0f;
							fSqrDist = fA11 + 2.0f * fB1 + fC;
						}
						else
						{
							fT = -fB1 / fA11;
							fSqrDist = fB1 * fT + fC;
						}
					}
				}
			}
			else   // region 6 (corner)
			{
				if (fB0 < 0.0f)
				{
					fT = 0.0f;
					if (-fB0 >= fA00)
					{
						fS = 1.0f;
						fSqrDist = fA00 + 2.0f * fB0 + fC;
					}
					else
					{
						fS = -fB0 / fA00;
						fSqrDist = fB0 * fS + fC;
					}
				}
				else
				{
					fS = 0.0f;
					if (fB1 >= 0.0f)
					{
						fT = 0.0f;
						fSqrDist = fC;
					}
					else if (-fB1 >= fA11)
					{
						fT = 1.0f;
						fSqrDist = fA11 + 2.0f * fB1 + fC;
					}
					else
					{
						fT = -fB1 / fA11;
						fSqrDist = fB1 * fT + fC;
					}
				}
			}
		}
	}
	else
	{
		// line segments are parallel
		if (fA01 > 0.0f)
		{
			// direction vectors form an obtuse angle
			if (fB0 >= 0.0f)
			{
				fS = 0.0f;
				fT = 0.0f;
				fSqrDist = fC;
			}
			else if (-fB0 <= fA00)
			{
				fS = -fB0 / fA00;
				fT = 0.0f;
				fSqrDist = fB0 * fS + fC;
			}
			else
			{
				fB1 = -kDiff.dot(rkSeg1Direction);
				fS = 1.0f;
				fTmp = fA00 + fB0;
				if (-fTmp >= fA01)
				{
					fT = 1.0f;
					fSqrDist = fA00 + fA11 + fC + 2.0f * (fA01 + fB0 + fB1);
				}
				else
				{
					fT = -fTmp / fA01;
					fSqrDist = fA00 + 2.0f * fB0 + fC + fT * (fA11 * fT + 2.0f * (fA01 + fB1));
				}
			}
		}
		else
		{
			// direction vectors form an acute angle
			if (-fB0 >= fA00)
			{
				fS = 1.0f;
				fT = 0.0f;
				fSqrDist = fA00 + 2.0f * fB0 + fC;
			}
			else if (fB0 <= 0.0f)
			{
				fS = -fB0 / fA00;
				fT = 0.0f;
				fSqrDist = fB0 * fS + fC;
			}
			else
			{
				fB1 = -kDiff.dot(rkSeg1Direction);
				fS = 0.0f;
				if (fB0 >= -fA01)
				{
					fT = 1.0f;
					fSqrDist = fA11 + 2.0f * fB1 + fC;
				}
				else
				{
					fT = -fB0 / fA01;
					fSqrDist = fC + fT * (2.0f * fB1 + fA11 * fT);
				}
			}
		}
	}

	if (s)
	{
		*s = fS;
	}
	if (t)
	{
		*t = fT;
	}

	return PxAbs(fSqrDist);

}

//----------------------------------------------------------------------------//

PxF32 APEX_pointSegmentSqrDist(const Segment& seg, const PxVec3& point, PxF32* param)
{
	PxVec3 Diff = point - seg.p0;
	PxVec3 segExtent = seg.p1 - seg.p0;
	PxF32 fT = Diff.dot(segExtent);

	if (fT <= 0.0f)
	{
		fT = 0.0f;
	}
	else
	{
		PxF32 SqrLen = (seg.p1 - seg.p0).magnitudeSquared();
		if (fT >= SqrLen)
		{
			fT = 1.0f;
			Diff -= segExtent;
		}
		else
		{
			fT /= SqrLen;
			Diff -= fT * segExtent;
		}
	}

	if (param)
	{
		*param = fT;
	}

	return Diff.magnitudeSquared();
}

//----------------------------------------------------------------------------//

PxU32 APEX_RayCapsuleIntersect(const PxVec3& origin, const PxVec3& dir, const Capsule& capsule, PxF32 s[2])
{
	// set up quadratic Q(t) = a*t^2 + 2*b*t + c

	PxVec3 kU, kV, kW;
	const PxVec3 capsDir = capsule.p1 - capsule.p0;
	kW = capsDir;

	PxF32 fWLength = kW.normalize();

	// generate orthonormal basis

	PxF32 fInvLength;
	if (PxAbs(kW.x) >= PxAbs(kW.y))
	{
		// W.x or W.z is the largest magnitude component, swap them
		fInvLength = 1.0f / PxSqrt(kW.x * kW.x + kW.z * kW.z);
		kU.x = -kW.z * fInvLength;
		kU.y = 0.0f;
		kU.z = +kW.x * fInvLength;
	}
	else
	{
		// W.y or W.z is the largest magnitude component, swap them
		fInvLength = 1.0f / PxSqrt(kW.y * kW.y + kW.z * kW.z);
		kU.x = 0.0f;
		kU.y = +kW.z * fInvLength;
		kU.z = -kW.y * fInvLength;
	}
	kV = kW.cross(kU);
	kV.normalize();	// PT: fixed november, 24, 2004. This is a bug in Magic.

	// compute intersection

	PxVec3 kD(kU.dot(dir), kV.dot(dir), kW.dot(dir));
	PxF32 fDLength = kD.normalize();

	PxF32 fInvDLength = 1.0f / fDLength;
	PxVec3 kDiff = origin - capsule.p0;
	PxVec3 kP(kU.dot(kDiff), kV.dot(kDiff), kW.dot(kDiff));
	PxF32 fRadiusSqr = capsule.radius * capsule.radius;

	PxF32 fInv, fA, fB, fC, fDiscr, fRoot, fT, fTmp;

	// Is the velocity parallel to the capsule direction? (or zero)
	if (PxAbs(kD.z) >= 1.0f - PX_EPS_F32 || fDLength < PX_EPS_F32)
	{

		PxF32 fAxisDir = dir.dot(capsDir);

		fDiscr = fRadiusSqr - kP.x * kP.x - kP.y * kP.y;
		if (fAxisDir < 0 && fDiscr >= 0.0f)
		{
			// Velocity anti-parallel to the capsule direction
			fRoot = PxSqrt(fDiscr);
			s[0] = (kP.z + fRoot) * fInvDLength;
			s[1] = -(fWLength - kP.z + fRoot) * fInvDLength;
			return 2;
		}
		else if (fAxisDir > 0  && fDiscr >= 0.0f)
		{
			// Velocity parallel to the capsule direction
			fRoot = PxSqrt(fDiscr);
			s[0] = -(kP.z + fRoot) * fInvDLength;
			s[1] = (fWLength - kP.z + fRoot) * fInvDLength;
			return 2;
		}
		else
		{
			// sphere heading wrong direction, or no velocity at all
			return 0;
		}
	}

	// test intersection with infinite cylinder
	fA = kD.x * kD.x + kD.y * kD.y;
	fB = kP.x * kD.x + kP.y * kD.y;
	fC = kP.x * kP.x + kP.y * kP.y - fRadiusSqr;
	fDiscr = fB * fB - fA * fC;
	if (fDiscr < 0.0f)
	{
		// line does not intersect infinite cylinder
		return 0;
	}

	int iQuantity = 0;

	if (fDiscr > 0.0f)
	{
		// line intersects infinite cylinder in two places
		fRoot = PxSqrt(fDiscr);
		fInv = 1.0f / fA;
		fT = (-fB - fRoot) * fInv;
		fTmp = kP.z + fT * kD.z;
		if (0.0f <= fTmp && fTmp <= fWLength)
		{
			s[iQuantity++] = fT * fInvDLength;
		}

		fT = (-fB + fRoot) * fInv;
		fTmp = kP.z + fT * kD.z;
		if (0.0f <= fTmp && fTmp <= fWLength)
		{
			s[iQuantity++] = fT * fInvDLength;
		}

		if (iQuantity == 2)
		{
			// line intersects capsule wall in two places
			return 2;
		}
	}
	else
	{
		// line is tangent to infinite cylinder
		fT = -fB / fA;
		fTmp = kP.z + fT * kD.z;
		if (0.0f <= fTmp && fTmp <= fWLength)
		{
			s[0] = fT * fInvDLength;
			return 1;
		}
	}

	// test intersection with bottom hemisphere
	// fA = 1
	fB += kP.z * kD.z;
	fC += kP.z * kP.z;
	fDiscr = fB * fB - fC;
	if (fDiscr > 0.0f)
	{
		fRoot = PxSqrt(fDiscr);
		fT = -fB - fRoot;
		fTmp = kP.z + fT * kD.z;
		if (fTmp <= 0.0f)
		{
			s[iQuantity++] = fT * fInvDLength;
			if (iQuantity == 2)
			{
				return 2;
			}
		}

		fT = -fB + fRoot;
		fTmp = kP.z + fT * kD.z;
		if (fTmp <= 0.0f)
		{
			s[iQuantity++] = fT * fInvDLength;
			if (iQuantity == 2)
			{
				return 2;
			}
		}
	}
	else if (fDiscr == 0.0f)
	{
		fT = -fB;
		fTmp = kP.z + fT * kD.z;
		if (fTmp <= 0.0f)
		{
			s[iQuantity++] = fT * fInvDLength;
			if (iQuantity == 2)
			{
				return 2;
			}
		}
	}

	// test intersection with top hemisphere
	// fA = 1
	fB -= kD.z * fWLength;
	fC += fWLength * (fWLength - 2.0f * kP.z);

	fDiscr = fB * fB - fC;
	if (fDiscr > 0.0f)
	{
		fRoot = PxSqrt(fDiscr);
		fT = -fB - fRoot;
		fTmp = kP.z + fT * kD.z;
		if (fTmp >= fWLength)
		{
			s[iQuantity++] = fT * fInvDLength;
			if (iQuantity == 2)
			{
				return 2;
			}
		}

		fT = -fB + fRoot;
		fTmp = kP.z + fT * kD.z;
		if (fTmp >= fWLength)
		{
			s[iQuantity++] = fT * fInvDLength;
			if (iQuantity == 2)
			{
				return 2;
			}
		}
	}
	else if (fDiscr == 0.0f)
	{
		fT = -fB;
		fTmp = kP.z + fT * kD.z;
		if (fTmp >= fWLength)
		{
			s[iQuantity++] = fT * fInvDLength;
			if (iQuantity == 2)
			{
				return 2;
			}
		}
	}

	return iQuantity;
}


//----------------------------------------------------------------------------//

bool APEX_RayTriangleIntersect(const PxVec3& orig, const PxVec3& dir, const PxVec3& a, const PxVec3& b, const PxVec3& c, PxF32& t, PxF32& u, PxF32& v)
{
	PxVec3 edge1 = b - a;
	PxVec3 edge2 = c - a;
	PxVec3 pvec = dir.cross(edge2);

	// if determinant is near zero, ray lies in plane of triangle
	PxF32 det = edge1.dot(pvec);

	if (det == 0.0f)
	{
		return false;
	}

	PxF32 inv_det = 1.0f / det;

	// calculate distance from vert0 to ray origin
	PxVec3 tvec = orig - a;

	// calculate U parameter and test bounds
	u = tvec.dot(pvec) * inv_det;
	if (u < 0.0f || u > 1.0f)
	{
		return false;
	}

	// prepare to test V parameter
	PxVec3 qvec = tvec.cross(edge1);

	// calculate V parameter and test bounds
	v = dir.dot(qvec) * inv_det;
	if (v < 0.0f || u + v > 1.0f)
	{
		return false;
	}

	// calculate t, ray intersects triangle
	t = edge2.dot(qvec) * inv_det;

	return true;
}

} // namespace apex
} // namespace physx
