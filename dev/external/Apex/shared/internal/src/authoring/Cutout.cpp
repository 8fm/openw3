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
#ifdef _MANAGED
#pragma managed(push, off)
#endif

#include "NxApex.h"
#include "foundation/PxMath.h"
#include "PsShare.h"
#include "ApexSharedUtils.h"
#include "FractureTools.h"

#include "authoring/Fracturing.h"

#ifndef WITHOUT_APEX_AUTHORING
#include "ApexSharedSerialization.h"

#define CUTOUT_DISTANCE_THRESHOLD	(0.7f)

#define CUTOUT_DISTANCE_EPS			(0.01f)

struct POINT2D
{
	POINT2D() {}
	POINT2D(physx::PxI32 _x, physx::PxI32 _y) : x(_x), y(_y) {}

	physx::PxI32 x;
	physx::PxI32 y;
};

// Unsigned modulus
PX_INLINE physx::PxU32 mod(physx::PxI32 n, physx::PxU32 modulus)
{
	const physx::PxI32 d = n/(physx::PxI32)modulus;
	const physx::PxI32 m = n - d*modulus;
	return m >= 0 ? m : m + modulus;
}

PX_INLINE physx::PxF32 square(physx::PxF32 x)
{
	return x * x;
}

// 2D cross product
PX_INLINE physx::PxF32 dotXY(const physx::PxVec3& v, const physx::PxVec3& w)
{
	return v.x * w.x + v.y * w.y;
}

// Z-component of cross product
PX_INLINE physx::PxF32 crossZ(const physx::PxVec3& v, const physx::PxVec3& w)
{
	return v.x * w.y - v.y * w.x;
}

// z coordinates may be used to store extra info - only deal with x and y
PX_INLINE physx::PxF32 perpendicularDistanceSquared(const physx::PxVec3& v0, const physx::PxVec3& v1, const physx::PxVec3& v2)
{
	const physx::PxVec3 base = v2 - v0;
	const physx::PxVec3 leg = v1 - v0;

	const physx::PxF32 baseLen2 = dotXY(base, base);

	return baseLen2 > PX_EPS_F32 * dotXY(leg, leg) ? square(crossZ(base, leg)) / baseLen2 : 0.0f;
}

// z coordinates may be used to store extra info - only deal with x and y
PX_INLINE physx::PxF32 perpendicularDistanceSquared(const physx::Array< physx::PxVec3 >& cutout, physx::PxU32 index)
{
	const physx::PxU32 size = cutout.size();
	return perpendicularDistanceSquared(cutout[(index + size - 1) % size], cutout[index], cutout[(index + 1) % size]);
}


struct CutoutVert
{
	physx::PxI32	cutoutIndex;
	physx::PxI32	vertIndex;

	void	set(physx::PxI32 _cutoutIndex, physx::PxI32 _vertIndex)
	{
		cutoutIndex = _cutoutIndex;
		vertIndex = _vertIndex;
	}
};

struct NewVertex
{
	CutoutVert	vertex;
	physx::PxF32		edgeProj;
};

static int compareNewVertices(const void* a, const void* b)
{
	const physx::PxI32 cutoutDiff = ((NewVertex*)a)->vertex.cutoutIndex - ((NewVertex*)b)->vertex.cutoutIndex;
	if (cutoutDiff)
	{
		return cutoutDiff;
	}
	const physx::PxI32 vertDiff = ((NewVertex*)a)->vertex.vertIndex - ((NewVertex*)b)->vertex.vertIndex;
	if (vertDiff)
	{
		return vertDiff;
	}
	const physx::PxF32 projDiff = ((NewVertex*)a)->edgeProj - ((NewVertex*)b)->edgeProj;
	return projDiff ? (projDiff < 0.0f ? -1 : 1) : 0;
}

template<typename T>
class Map2d
{
public:
	Map2d() : mMem(NULL) {}
	Map2d(physx::PxU32 width, physx::PxU32 height) : mMem(NULL)
	{
		create_internal(width, height, NULL);
	}
	Map2d(physx::PxU32 width, physx::PxU32 height, T fillValue) : mMem(NULL)
	{
		create_internal(width, height, &fillValue);
	}
	Map2d(const Map2d& map)
	{
		*this = map;
	}
	~Map2d()
	{
		delete [] mMem;
	}

	Map2d&		operator = (const Map2d& map)
	{
		delete [] mMem;
		mMem = NULL;
		if (map.mMem)
		{
			create_internal(map.mWidth, map.mHeight, NULL);
			memcpy(mMem, map.mMem, mWidth * mHeight);
		}
		return *this;
	}

	void		create(physx::PxU32 width, physx::PxU32 height)
	{
		return create_internal(width, height, NULL);
	}
	void		create(physx::PxU32 width, physx::PxU32 height, T fillValue)
	{
		create_internal(width, height, &fillValue);
	}

	void		clear(const T value)
	{
		T* mem = mMem;
		T* stop = mMem + mWidth * mHeight;
		while (mem < stop)
		{
			*mem++ = value;
		}
	}

	void		setOrigin(physx::PxU32 x, physx::PxU32 y)
	{
		mOriginX = x;
		mOriginY = y;
	}

	const T&	operator()(physx::PxI32 x, physx::PxI32 y) const
	{
		x = mod(x+mOriginX, mWidth);
		y = mod(y+mOriginY, mHeight);
		return mMem[x + y * mWidth];
	}
	T&			operator()(physx::PxI32 x, physx::PxI32 y)
	{
		x = mod(x+mOriginX, mWidth);
		y = mod(y+mOriginY, mHeight);
		return mMem[x + y * mWidth];
	}

private:

	void	create_internal(physx::PxU32 width, physx::PxU32 height, T* val)
	{
		delete [] mMem;
		mWidth = width;
		mHeight = height;
		mMem = new T[mWidth * mHeight];
		mOriginX = 0;
		mOriginY = 0;
		if (val)
		{
			clear(*val);
		}
	}

	T*		mMem;
	physx::PxU32	mWidth;
	physx::PxU32	mHeight;
	physx::PxU32	mOriginX;
	physx::PxU32	mOriginY;
};

class BitMap
{
public:
	BitMap() : mMem(NULL) {}
	BitMap(physx::PxU32 width, physx::PxU32 height) : mMem(NULL)
	{
		create_internal(width, height, NULL);
	}
	BitMap(physx::PxU32 width, physx::PxU32 height, bool fillValue) : mMem(NULL)
	{
		create_internal(width, height, &fillValue);
	}
	BitMap(const BitMap& map)
	{
		*this = map;
	}
	~BitMap()
	{
		delete [] mMem;
	}

	BitMap&	operator = (const BitMap& map)
	{
		delete [] mMem;
		mMem = NULL;
		if (map.mMem)
		{
			create_internal(map.mWidth, map.mHeight, NULL);
			memcpy(mMem, map.mMem, mHeight * mRowBytes);
		}
		return *this;
	}

	void	create(physx::PxU32 width, physx::PxU32 height)
	{
		return create_internal(width, height, NULL);
	}
	void	create(physx::PxU32 width, physx::PxU32 height, bool fillValue)
	{
		create_internal(width, height, &fillValue);
	}

	void	clear(bool value)
	{
		memset(mMem, value ? 0xFF : 0x00, mRowBytes * mHeight);
	}

	void	setOrigin(physx::PxU32 x, physx::PxU32 y)
	{
		mOriginX = x;
		mOriginY = y;
	}

	bool	read(physx::PxI32 x, physx::PxI32 y) const
	{
		x = mod(x+mOriginX, mWidth);
		y = mod(y+mOriginY, mHeight);
		return ((mMem[(x >> 3) + y * mRowBytes] >> (x & 7)) & 1) != 0;
	}
	void	set(physx::PxI32 x, physx::PxI32 y)
	{
		x = mod(x+mOriginX, mWidth);
		y = mod(y+mOriginY, mHeight);
		mMem[(x >> 3) + y * mRowBytes] |= 1 << (x & 7);
	}
	void	reset(physx::PxI32 x, physx::PxI32 y)
	{
		x = mod(x+mOriginX, mWidth);
		y = mod(y+mOriginY, mHeight);
		mMem[(x >> 3) + y * mRowBytes] &= ~(1 << (x & 7));
	}

private:

	void	create_internal(physx::PxU32 width, physx::PxU32 height, bool* val)
	{
		delete [] mMem;
		mRowBytes = (width + 7) >> 3;
		const physx::PxU32 bytes = mRowBytes * height;
		if (bytes == 0)
		{
			mWidth = mHeight = 0;
			mMem = NULL;
			return;
		}
		mWidth = width;
		mHeight = height;
		mMem = new physx::PxU8[bytes];
		mOriginX = 0;
		mOriginY = 0;
		if (val)
		{
			clear(*val);
		}
	}

	physx::PxU8*	mMem;
	physx::PxU32	mWidth;
	physx::PxU32	mHeight;
	physx::PxU32	mRowBytes;
	physx::PxU32	mOriginX;
	physx::PxU32	mOriginY;
};


PX_INLINE physx::PxI32 taxicabSine(physx::PxI32 i)
{
	// 0 1 1 1 0 -1 -1 -1
	return (physx::PxI32)((0x01A9 >> ((i & 7) << 1)) & 3) - 1;
}

// Only looks at x and y components
PX_INLINE bool directionsXYOrderedCCW(const physx::PxVec3& d0, const physx::PxVec3& d1, const physx::PxVec3& d2)
{
	const bool ccw02 = crossZ(d0, d2) > 0.0f;
	const bool ccw01 = crossZ(d0, d1) > 0.0f;
	const bool ccw21 = crossZ(d2, d1) > 0.0f;
	return ccw02 ? ccw01 && ccw21 : ccw01 || ccw21;
}

PX_INLINE physx::PxF32 compareTraceSegmentToLineSegment(const physx::Array<POINT2D>& trace, int start, int delta, physx::PxF32 distThreshold, physx::PxU32 width, physx::PxU32 height, bool hasBorder)
{
	if (delta < 2)
	{
		return 0.0f;
	}

	const int size = trace.size();

	int end = (start + delta) % size;

	const bool startIsOnBorder = hasBorder && (trace[start].x == -1 || trace[start].x == (int)width || trace[start].y == -1 || trace[start].y == (int)height);
	const bool endIsOnBorder = hasBorder && (trace[end].x == -1 || trace[end].x == (int)width || trace[end].y == -1 || trace[end].y == (int)height);

	if (startIsOnBorder || endIsOnBorder)
	{
		if ((trace[start].x == -1 && trace[end].x == -1) ||
		        (trace[start].y == -1 && trace[end].y == -1) ||
		        (trace[start].x == (int)width && trace[end].x == (int)width) ||
		        (trace[start].y == (int)height && trace[end].y == (int)height))
		{
			return 0.0f;
		}
		return PX_MAX_F32;
	}

	physx::PxVec3 orig((physx::PxF32)trace[start].x, (physx::PxF32)trace[start].y, 0);
	physx::PxVec3 dest((physx::PxF32)trace[end].x, (physx::PxF32)trace[end].y, 0);
	physx::PxVec3 dir = dest - orig;

	dir.normalize();

	physx::PxF32 aveError = 0.0f;

	for (;;)
	{
		if (++start >= size)
		{
			start = 0;
		}
		if (start == end)
		{
			break;
		}
		physx::PxVec3 testDisp((physx::PxF32)trace[start].x, (physx::PxF32)trace[start].y, 0);
		testDisp -= orig;
		aveError += (physx::PxF32)(physx::PxAbs(testDisp.x * dir.y - testDisp.y * dir.x) >= distThreshold);
	}

	aveError /= delta - 1;

	return aveError;
}

// Segment i starts at vi and ends at vi+ei
// Tests for overlap in segments' projection onto xy plane
// Returns distance between line segments.  (Negative value indicates overlap.)
PX_INLINE physx::PxF32 segmentsIntersectXY(const physx::PxVec3& v0, const physx::PxVec3& e0, const physx::PxVec3& v1, const physx::PxVec3& e1)
{
	const physx::PxVec3 dv = v1 - v0;

	physx::PxVec3 d0 = e0;
	d0.normalize();
	physx::PxVec3 d1 = e1;
	d1.normalize();

	const physx::PxF32 c10 = crossZ(dv, d0);
	const physx::PxF32 d10 = crossZ(e1, d0);

	physx::PxF32 a1 = physx::PxAbs(c10);
	physx::PxF32 b1 = physx::PxAbs(c10 + d10);

	if (c10 * (c10 + d10) < 0.0f)
	{
		if (a1 < b1)
		{
			a1 = -a1;
		}
		else
		{
			b1 = -b1;
		}
	}

	const physx::PxF32 c01 = crossZ(d1, dv);
	const physx::PxF32 d01 = crossZ(e0, d1);

	physx::PxF32 a2 = physx::PxAbs(c01);
	physx::PxF32 b2 = physx::PxAbs(c01 + d01);

	if (c01 * (c01 + d01) < 0.0f)
	{
		if (a2 < b2)
		{
			a2 = -a2;
		}
		else
		{
			b2 = -b2;
		}
	}

	return physx::PxMax(physx::PxMin(a1, b1), physx::PxMin(a2, b2));
}

// If point projects onto segment, returns true and proj is set to a
// value in the range [0,1], indicating where along the segment (from v0 to v1)
// the projection lies, and dist2 is set to the distance squared from point to
// the line segment.  Otherwise, returns false.
// Note, if v1 = v0, then the function returns true with proj = 0.
PX_INLINE bool projectOntoSegmentXY(physx::PxF32& proj, physx::PxF32& dist2, const physx::PxVec3& point, const physx::PxVec3& v0, const physx::PxVec3& v1, physx::PxF32 margin)
{
	const physx::PxVec3 seg = v1 - v0;
	const physx::PxVec3 x = point - v0;
	const physx::PxF32 seg2 = dotXY(seg, seg);
	const physx::PxF32 d = dotXY(x, seg);

	if (d < 0.0f || d > seg2)
	{
		return false;
	}

	const physx::PxF32 margin2 = margin * margin;

	const physx::PxF32 p = seg2 > 0.0f ? d / seg2 : 0.0f;
	const physx::PxF32 lineDist2 = d * p;

	if (lineDist2 < margin2)
	{
		return false;
	}

	const physx::PxF32 pPrime = 1.0f - p;
	const physx::PxF32 dPrime = seg2 - d;
	const physx::PxF32 lineDistPrime2 = dPrime * pPrime;

	if (lineDistPrime2 < margin2)
	{
		return false;
	}

	proj = p;
	dist2 = dotXY(x, x) - lineDist2;
	return true;
}

PX_INLINE bool isOnBorder(const physx::PxVec3& v, physx::PxU32 width, physx::PxU32 height)
{
	return v.x < -0.5f || v.x >= width - 0.5f || v.y < -0.5f || v.y >= height - 0.5f;
}

static void createCutout(physx::Cutout& cutout, const physx::Array<POINT2D>& trace, physx::PxF32 snapThreshold, physx::PxU32 width, physx::PxU32 height, bool hasBorder)
{
	cutout.vertices.reset();

	const physx::PxU32 traceSize = trace.size();

	if (traceSize == 0)
	{
		return;	// Nothing to do
	}

	physx::PxU32 size = traceSize;

	physx::Array<int> vertexIndices;

	const physx::PxF32 errorThreshold = 0.1f;

	const physx::PxF32 pixelCenterOffset = hasBorder ? 0.5f : 0.0f;

	// Find best segment
	physx::PxU32 start = 0;
	physx::PxU32 delta = 0;
	for (physx::PxU32 iStart = 0; iStart < size; ++iStart)
	{
		physx::PxU32 iDelta = (size >> 1) + (size & 1);
		for (; iDelta > 1; --iDelta)
		{
			physx::PxF32 fit = compareTraceSegmentToLineSegment(trace, iStart, iDelta, CUTOUT_DISTANCE_THRESHOLD, width, height, hasBorder);
			if (fit < errorThreshold)
			{
				break;
			}
		}
		if (iDelta > delta)
		{
			start = iStart;
			delta = iDelta;
		}
	}
	cutout.vertices.pushBack(physx::PxVec3((physx::PxF32)trace[start].x + pixelCenterOffset, (physx::PxF32)trace[start].y + pixelCenterOffset, 0));

	// Now complete the loop
	while ((size -= delta) > 0)
	{
		start = (start + delta) % traceSize;
		cutout.vertices.pushBack(physx::PxVec3((physx::PxF32)trace[start].x + pixelCenterOffset, (physx::PxF32)trace[start].y + pixelCenterOffset, 0));
		if (size == 1)
		{
			delta = 1;
			break;
		}
		for (delta = size - 1; delta > 1; --delta)
		{
			physx::PxF32 fit = compareTraceSegmentToLineSegment(trace, start, delta, CUTOUT_DISTANCE_THRESHOLD, width, height, hasBorder);
			if (fit < errorThreshold)
			{
				break;
			}
		}
	}

	const physx::PxF32 snapThresh2 = square(snapThreshold);

	// Use the snapThreshold to clean up
	while ((size = cutout.vertices.size()) >= 4)
	{
		bool reduced = false;
		for (physx::PxU32 i = 0; i < size; ++i)
		{
			const physx::PxU32 i1 = (i + 1) % size;
			const physx::PxU32 i2 = (i + 2) % size;
			const physx::PxU32 i3 = (i + 3) % size;
			physx::PxVec3& v0 = cutout.vertices[i];
			physx::PxVec3& v1 = cutout.vertices[i1];
			physx::PxVec3& v2 = cutout.vertices[i2];
			physx::PxVec3& v3 = cutout.vertices[i3];
			const physx::PxVec3 d0 = v1 - v0;
			const physx::PxVec3 d1 = v2 - v1;
			const physx::PxVec3 d2 = v3 - v2;
			const physx::PxF32 den = crossZ(d0, d2);
			if (den != 0)
			{
				const physx::PxF32 recipDen = 1.0f / den;
				const physx::PxF32 s0 = crossZ(d1, d2) * recipDen;
				const physx::PxF32 s2 = crossZ(d0, d1) * recipDen;
				if (s0 >= 0 || s2 >= 0)
				{
					if (d0.magnitudeSquared()*s0* s0 <= snapThresh2 && d2.magnitudeSquared()*s2* s2 <= snapThresh2)
					{
						v1 += d0 * s0;

						physx::PxU32 index = (physx::PxU32)(&v2 - cutout.vertices.begin());

						cutout.vertices.remove(index);

						reduced = true;
						break;
					}
				}
			}
		}
		if (!reduced)
		{
			break;
		}
	}
}

static void splitTJunctions(physx::CutoutSet& cutoutSet, physx::PxF32 threshold)
{
	// Set bounds reps
	physx::Array<physx::BoundsRep> bounds;
	physx::Array<CutoutVert> cutoutMap;	// maps bounds # -> ( cutout #, vertex # ).
	physx::Array<physx::IntPair> overlaps;

	const physx::PxF32 distThreshold2 = threshold * threshold;

	// Split T-junctions
	physx::PxU32 edgeCount = 0;
	for (physx::PxU32 i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		edgeCount += cutoutSet.cutouts[i].vertices.size();
	}

	bounds.resize(edgeCount);
	cutoutMap.resize(edgeCount);

	edgeCount = 0;
	for (physx::PxU32 i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		physx::Cutout& cutout = cutoutSet.cutouts[i];
		const physx::PxU32 cutoutSize = cutout.vertices.size();
		for (physx::PxU32 j = 0; j < cutoutSize; ++j)
		{
			bounds[edgeCount].aabb.include(cutout.vertices[j]);
			bounds[edgeCount].aabb.include(cutout.vertices[(j + 1) % cutoutSize]);
			PX_ASSERT(!bounds[edgeCount].aabb.isEmpty());
			bounds[edgeCount].aabb.fattenFast(threshold);
			cutoutMap[edgeCount].set(i, j);
			++edgeCount;
		}
	}

	// Find bounds overlaps
	if (bounds.size() > 0)
	{
		boundsCalculateOverlaps(overlaps, physx::Bounds3XY, &bounds[0], bounds.size(), sizeof(bounds[0]));
	}

	physx::Array<NewVertex> newVertices;
	for (physx::PxU32 overlapIndex = 0; overlapIndex < overlaps.size(); ++overlapIndex)
	{
		const physx::IntPair& mapPair = overlaps[overlapIndex];
		const CutoutVert& seg0Map = cutoutMap[mapPair.i0];
		const CutoutVert& seg1Map = cutoutMap[mapPair.i1];

		if (seg0Map.cutoutIndex == seg1Map.cutoutIndex)
		{
			// Only split based on vertex/segment junctions from different cutouts
			continue;
		}

		NewVertex newVertex;
		physx::PxF32 dist2 = 0;

		const physx::Cutout& cutout0 = cutoutSet.cutouts[seg0Map.cutoutIndex];
		const physx::PxU32 cutoutSize0 = cutout0.vertices.size();
		const physx::Cutout& cutout1 = cutoutSet.cutouts[seg1Map.cutoutIndex];
		const physx::PxU32 cutoutSize1 = cutout1.vertices.size();

		if (projectOntoSegmentXY(newVertex.edgeProj, dist2, cutout0.vertices[seg0Map.vertIndex], cutout1.vertices[seg1Map.vertIndex], cutout1.vertices[(seg1Map.vertIndex + 1) % cutoutSize1], 0.25f))
		{
			if (dist2 <= distThreshold2)
			{
				newVertex.vertex = seg1Map;
				newVertices.pushBack(newVertex);
			}
		}

		if (projectOntoSegmentXY(newVertex.edgeProj, dist2, cutout1.vertices[seg1Map.vertIndex], cutout0.vertices[seg0Map.vertIndex], cutout0.vertices[(seg0Map.vertIndex + 1) % cutoutSize0], 0.25f))
		{
			if (dist2 <= distThreshold2)
			{
				newVertex.vertex = seg0Map;
				newVertices.pushBack(newVertex);
			}
		}
	}

	if (newVertices.size())
	{
		// Sort new vertices
		qsort(newVertices.begin(), newVertices.size(), sizeof(NewVertex), compareNewVertices);

		// Insert new vertices
		physx::PxU32 lastCutoutIndex = 0xFFFFFFFF;
		physx::PxU32 lastVertexIndex = 0xFFFFFFFF;
		physx::PxF32 lastProj = 1.0f;
		for (physx::PxU32 newVertexIndex = newVertices.size(); newVertexIndex--;)
		{
			const NewVertex& newVertex = newVertices[newVertexIndex];
			if (newVertex.vertex.cutoutIndex != (physx::PxI32)lastCutoutIndex)
			{
				lastCutoutIndex = newVertex.vertex.cutoutIndex;
				lastVertexIndex = 0xFFFFFFFF;
			}
			if (newVertex.vertex.vertIndex != (physx::PxI32)lastVertexIndex)
			{
				lastVertexIndex = newVertex.vertex.vertIndex;
				lastProj = 1.0f;
			}
			physx::Cutout& cutout = cutoutSet.cutouts[newVertex.vertex.cutoutIndex];
			const physx::PxF32 proj = lastProj > 0.0f ? newVertex.edgeProj / lastProj : 0.0f;
			const physx::PxVec3 pos = (1.0f - proj) * cutout.vertices[newVertex.vertex.vertIndex] + proj * cutout.vertices[(newVertex.vertex.vertIndex + 1) % cutout.vertices.size()];
			cutout.vertices.insert();
			for (physx::PxU32 n = cutout.vertices.size(); --n > (physx::PxU32)newVertex.vertex.vertIndex + 1;)
			{
				cutout.vertices[n] = cutout.vertices[n - 1];
			}
			cutout.vertices[newVertex.vertex.vertIndex + 1] = pos;
			lastProj = newVertex.edgeProj;
		}
	}
}

#if 0
static void mergeVertices(CutoutSet& cutoutSet, physx::PxF32 threshold, physx::PxU32 width, physx::PxU32 height)
{
	// Set bounds reps
	physx::Array<BoundsRep> bounds;
	physx::Array<CutoutVert> cutoutMap;	// maps bounds # -> ( cutout #, vertex # ).
	physx::Array<IntPair> overlaps;

	const physx::PxF32 threshold2 = threshold * threshold;

	physx::PxU32 vertexCount = 0;
	for (physx::PxU32 i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		vertexCount += cutoutSet.cutouts[i].vertices.size();
	}

	bounds.resize(vertexCount);
	cutoutMap.resize(vertexCount);

	vertexCount = 0;
	for (physx::PxU32 i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		Cutout& cutout = cutoutSet.cutouts[i];
		for (physx::PxU32 j = 0; j < cutout.vertices.size(); ++j)
		{
			physx::PxVec3& vertex = cutout.vertices[j];
			physx::PxVec3 min(vertex.x - threshold, vertex.y - threshold, 0.0f);
			physx::PxVec3 max(vertex.x + threshold, vertex.y + threshold, 0.0f);
			bounds[vertexCount].aabb.set(min, max);
			cutoutMap[vertexCount].set(i, j);
			++vertexCount;
		}
	}

	// Find bounds overlaps
	overlaps.reset();
	if (bounds.size() > 0)
	{
		boundsCalculateOverlaps(overlaps, Bounds3XY, &bounds[0], bounds.size(), sizeof(bounds[0]));
	}

	const physx::PxU32 overlapCount = overlaps.size();
	if (overlapCount)
	{
		// Sort overlaps by index0 and index1
		qsort(overlaps.begin(), overlaps.size(), sizeof(IntPair), IntPair::compare);

		// Process overlaps: merge vertices
		physx::PxU32 groupStart = 0;
		physx::PxU32 groupStop;
		do
		{
			const physx::PxI32 groupI0 = overlaps[groupStart].i0;
			groupStop = groupStart;
			while (++groupStop < overlapCount)
			{
				const physx::PxI32 i0 = overlaps[groupStop].i0;
				if (i0 != groupI0)
				{
					break;
				}
			}
			// Process group
			physx::PxVec3 straightV(0.0f);
			physx::PxU32 straightCount = 0;
			physx::PxVec3 borderV(0.0f);
			physx::PxU32 borderCount = 0;
			physx::PxVec3 v(0.0f);
			physx::PxF32 weight = 0.0f;
			// Include i0
			const CutoutVert& vertexMap = cutoutMap[overlaps[groupStart].i0];
			Cutout& cutout = cutoutSet.cutouts[vertexMap.cutoutIndex];
			physx::PxF32 dist2 = perpendicularDistanceSquared(cutout.vertices, vertexMap.vertIndex);
			if (isOnBorder(cutout.vertices[vertexMap.vertIndex], width, height))
			{
				borderV += cutout.vertices[vertexMap.vertIndex];
				++borderCount;
			}
			else if (dist2 < threshold2)
			{
				straightV += cutout.vertices[vertexMap.vertIndex];
				++straightCount;
			}
			else
			{
				const physx::PxF32 recipDist2 = 1.0f / dist2;
				weight += recipDist2;
				v += cutout.vertices[vertexMap.vertIndex] * recipDist2;
			}
			for (physx::PxU32 i = groupStart; i < groupStop; ++i)
			{
				const CutoutVert& vertexMap = cutoutMap[overlaps[i].i1];
				Cutout& cutout = cutoutSet.cutouts[vertexMap.cutoutIndex];
				dist2 = perpendicularDistanceSquared(cutout.vertices, vertexMap.vertIndex);
				if (isOnBorder(cutout.vertices[vertexMap.vertIndex], width, height))
				{
					borderV += cutout.vertices[vertexMap.vertIndex];
					++borderCount;
				}
				else if (dist2 < threshold2)
				{
					straightV += cutout.vertices[vertexMap.vertIndex];
					++straightCount;
				}
				else
				{
					const physx::PxF32 recipDist2 = 1.0f / dist2;
					weight += recipDist2;
					v += cutout.vertices[vertexMap.vertIndex] * recipDist2;
				}
			}
			if (borderCount)
			{
				// If we have any borderVertices, these will be the only ones considered
				v = (1.0f / borderCount) * borderV;
			}
			else if (straightCount)
			{
				// Otherwise if we have any straight angles, these will be the only ones considered
				v = (1.0f / straightCount) * straightV;
			}
			else
			{
				v *= 1.0f / weight;
			}
			// Now replace all group vertices by v
			{
				const CutoutVert& vertexMap = cutoutMap[overlaps[groupStart].i0];
				cutoutSet.cutouts[vertexMap.cutoutIndex].vertices[vertexMap.vertIndex] = v;
				for (physx::PxU32 i = groupStart; i < groupStop; ++i)
				{
					const CutoutVert& vertexMap = cutoutMap[overlaps[i].i1];
					cutoutSet.cutouts[vertexMap.cutoutIndex].vertices[vertexMap.vertIndex] = v;
				}
			}
		}
		while ((groupStart = groupStop) < overlapCount);
	}
}
#else
static void mergeVertices(physx::CutoutSet& cutoutSet, physx::PxF32 threshold, physx::PxU32 width, physx::PxU32 height)
{
	// Set bounds reps
	physx::PxU32 vertexCount = 0;
	for (physx::PxU32 i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		vertexCount += cutoutSet.cutouts[i].vertices.size();
	}

	physx::Array<physx::BoundsRep> bounds;
	physx::Array<CutoutVert> cutoutMap;	// maps bounds # -> ( cutout #, vertex # ).
	bounds.resize(vertexCount);
	cutoutMap.resize(vertexCount);

	vertexCount = 0;
	for (physx::PxU32 i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		physx::Cutout& cutout = cutoutSet.cutouts[i];
		for (physx::PxU32 j = 0; j < cutout.vertices.size(); ++j)
		{
			physx::PxVec3& vertex = cutout.vertices[j];
			physx::PxVec3 min(vertex.x - threshold, vertex.y - threshold, 0.0f);
			physx::PxVec3 max(vertex.x + threshold, vertex.y + threshold, 0.0f);
			bounds[vertexCount].aabb = physx::PxBounds3(min, max);
			cutoutMap[vertexCount].set(i, j);
			++vertexCount;
		}
	}

	// Find bounds overlaps
	physx::Array<physx::IntPair> overlaps;
	if (bounds.size() > 0)
	{
		boundsCalculateOverlaps(overlaps, physx::Bounds3XY, &bounds[0], bounds.size(), sizeof(bounds[0]));
	}
	physx::PxU32 overlapCount = overlaps.size();

	if (overlapCount == 0)
	{
		return;
	}

	// Sort by first index
	qsort(overlaps.begin(), overlapCount, sizeof(physx::IntPair), physx::IntPair::compare);

	const physx::PxF32 threshold2 = threshold * threshold;

	physx::Array<physx::IntPair> pairs;

	// Group by first index
	physx::Array<physx::PxU32> lookup;
	physx::createIndexStartLookup(lookup, 0, vertexCount, &overlaps.begin()->i0, overlapCount, sizeof(physx::IntPair));
	for (physx::PxU32 i = 0; i < vertexCount; ++i)
	{
		const physx::PxU32 start = lookup[i];
		const physx::PxU32 stop = lookup[i + 1];
		if (start == stop)
		{
			continue;
		}
		const CutoutVert& cutoutVert0 = cutoutMap[overlaps[start].i0];
		const physx::PxVec3& vert0 = cutoutSet.cutouts[cutoutVert0.cutoutIndex].vertices[cutoutVert0.vertIndex];
		const bool isOnBorder0 = !cutoutSet.periodic && isOnBorder(vert0, width, height);
		for (physx::PxU32 j = start; j < stop; ++j)
		{
			const CutoutVert& cutoutVert1 = cutoutMap[overlaps[j].i1];
			if (cutoutVert0.cutoutIndex == cutoutVert1.cutoutIndex)
			{
				// No pairs from the same cutout
				continue;
			}
			const physx::PxVec3& vert1 = cutoutSet.cutouts[cutoutVert1.cutoutIndex].vertices[cutoutVert1.vertIndex];
			const bool isOnBorder1 = !cutoutSet.periodic && isOnBorder(vert1, width, height);
			if (isOnBorder0 != isOnBorder1)
			{
				// No border/non-border pairs
				continue;
			}
			if ((vert0 - vert1).magnitudeSquared() > threshold2)
			{
				// Distance outside threshold
				continue;
			}
			// A keeper.  Keep a symmetric list
			physx::IntPair overlap = overlaps[j];
			pairs.pushBack(overlap);
			const physx::PxI32 i0 = overlap.i0;
			overlap.i0 = overlap.i1;
			overlap.i1 = i0;
			pairs.pushBack(overlap);
		}
	}

	// Sort by first index
	qsort(pairs.begin(), pairs.size(), sizeof(physx::IntPair), physx::IntPair::compare);

	// For every vertex, only keep closest neighbor from each cutout
	physx::createIndexStartLookup(lookup, 0, vertexCount, &pairs.begin()->i0, pairs.size(), sizeof(physx::IntPair));
	for (physx::PxU32 i = 0; i < vertexCount; ++i)
	{
		const physx::PxU32 start = lookup[i];
		const physx::PxU32 stop = lookup[i + 1];
		if (start == stop)
		{
			continue;
		}
		const CutoutVert& cutoutVert0 = cutoutMap[pairs[start].i0];
		const physx::PxVec3& vert0 = cutoutSet.cutouts[cutoutVert0.cutoutIndex].vertices[cutoutVert0.vertIndex];
		physx::PxU32 groupStart = start;
		while (groupStart < stop)
		{
			physx::PxU32 next = groupStart;
			const CutoutVert& cutoutVert1 = cutoutMap[pairs[next].i1];
			physx::PxI32 currentOtherCutoutIndex = cutoutVert1.cutoutIndex;
			const physx::PxVec3& vert1 = cutoutSet.cutouts[currentOtherCutoutIndex].vertices[cutoutVert1.vertIndex];
			physx::PxU32 keep = groupStart;
			physx::PxF32 minDist2 = (vert0 - vert1).magnitudeSquared();
			while (++next < stop)
			{
				const CutoutVert& cutoutVertNext = cutoutMap[pairs[next].i1];
				if (currentOtherCutoutIndex != cutoutVertNext.cutoutIndex)
				{
					break;
				}
				const physx::PxVec3& vertNext = cutoutSet.cutouts[cutoutVertNext.cutoutIndex].vertices[cutoutVertNext.vertIndex];
				const physx::PxF32 dist2 = (vert0 - vertNext).magnitudeSquared();
				if (dist2 < minDist2)
				{
					pairs[keep].set(-1, -1);	// Invalidate
					keep = next;
					minDist2 = dist2;
				}
				else
				{
					pairs[next].set(-1, -1);	// Invalidate
				}
			}
			groupStart = next;
		}
	}

	// Eliminate invalid pairs (compactify)
	physx::PxU32 pairCount = 0;
	for (physx::PxU32 i = 0; i < pairs.size(); ++i)
	{
		if (pairs[i].i0 >= 0 && pairs[i].i1 >= 0)
		{
			pairs[pairCount++] = pairs[i];
		}
	}
	pairs.resize(pairCount);

	// Snap points together
	physx::Array<bool> pinned;
	pinned.resize(vertexCount);
	memset(pinned.begin(), 0, pinned.size()*sizeof(bool));

	for (physx::PxU32 i = 0; i < pairCount; ++i)
	{
		const physx::PxI32 i0 = pairs[i].i0;
		bool& pinned0 = pinned[i0];
		if (pinned0)
		{
			continue;
		}
		const CutoutVert& cutoutVert0 = cutoutMap[i0];
		physx::PxVec3& vert0 = cutoutSet.cutouts[cutoutVert0.cutoutIndex].vertices[cutoutVert0.vertIndex];
		const physx::PxI32 i1 = pairs[i].i1;
		bool& pinned1 = pinned[i1];
		const CutoutVert& cutoutVert1 = cutoutMap[i1];
		physx::PxVec3& vert1 = cutoutSet.cutouts[cutoutVert1.cutoutIndex].vertices[cutoutVert1.vertIndex];
		const physx::PxVec3 disp = vert1 - vert0;
		// Move and pin
		pinned0 = true;
		if (pinned1)
		{
			vert0 = vert1;
		}
		else
		{
			vert0 += 0.5f * disp;
			vert1 = vert0;
			pinned1 = true;
		}
	}
}
#endif

static void eliminateStraightAngles(physx::CutoutSet& cutoutSet)
{
	// Eliminate straight angles
	for (physx::PxU32 i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		physx::Cutout& cutout = cutoutSet.cutouts[i];
		physx::PxU32 oldSize;
		do
		{
			oldSize = cutout.vertices.size();
			for (physx::PxU32 j = 0; j < cutout.vertices.size();)
			{
//				if( isOnBorder( cutout.vertices[j], width, height ) )
//				{	// Don't eliminate border vertices
//					++j;
//					continue;
//				}
				if (perpendicularDistanceSquared(cutout.vertices, j) < CUTOUT_DISTANCE_EPS * CUTOUT_DISTANCE_EPS)
				{
					cutout.vertices.remove(j);
				}
				else
				{
					++j;
				}
			}
		}
		while (cutout.vertices.size() != oldSize);
	}
}

static void simplifyCutoutSet(physx::CutoutSet& cutoutSet, physx::PxF32 threshold, physx::PxU32 width, physx::PxU32 height)
{
	splitTJunctions(cutoutSet, 1.0f);
	mergeVertices(cutoutSet, threshold, width, height);
	eliminateStraightAngles(cutoutSet);
}

static void cleanCutout(physx::Cutout& cutout, physx::PxU32 loopIndex, physx::PxF32 tolerance)
{
	physx::ConvexLoop& loop = cutout.convexLoops[loopIndex];
	const physx::PxF32 tolerance2 = tolerance * tolerance;
	physx::PxU32 oldSize;
	do
	{
		oldSize = loop.polyVerts.size();
		physx::PxU32 size = oldSize;
		for (physx::PxU32 i = 0; i < size; ++i)
		{
			physx::PolyVert& v0 = loop.polyVerts[(i + size - 1) % size];
			physx::PolyVert& v1 = loop.polyVerts[i];
			physx::PolyVert& v2 = loop.polyVerts[(i + 1) % size];
			if (perpendicularDistanceSquared(cutout.vertices[v0.index], cutout.vertices[v1.index], cutout.vertices[v2.index]) <= tolerance2)
			{
				loop.polyVerts.remove(i);
				--size;
				--i;
			}
		}
	}
	while (loop.polyVerts.size() != oldSize);
}

static bool decomposeCutoutIntoConvexLoops(physx::Cutout& cutout, physx::PxF32 cleanupTolerance = 0.0f)
{
	const physx::PxU32 size = cutout.vertices.size();

	if (size < 3)
	{
		return false;
	}

	// Initialize to one loop, which may not be convex
	cutout.convexLoops.resize(1);
	cutout.convexLoops[0].polyVerts.resize(size);

	// See if the winding is ccw:

	// Scale to normalized size to avoid overflows
	physx::PxBounds3 bounds;
	bounds.setEmpty();
	for (physx::PxU32 i = 0; i < size; ++i)
	{
		bounds.include(cutout.vertices[i]);
	}
	physx::PxVec3 center = bounds.getCenter();
	physx::PxVec3 extent = bounds.getExtents();
	if (extent[0] < PX_EPS_F32 || extent[1] < PX_EPS_F32)
	{
		return false;
	}
	const physx::PxVec3 scale(1.0f / extent[0], 1.0f / extent[1], 0.0f);

	// Find "area" (it will only be correct in sign!)
	physx::PxVec3 prevV = (cutout.vertices[size - 1] - center).multiply(scale);
	physx::PxF32 area = 0.0f;
	for (physx::PxU32 i = 0; i < size; ++i)
	{
		const physx::PxVec3 v = (cutout.vertices[i] - center).multiply(scale);
		area += crossZ(prevV, v);
		prevV = v;
	}

	if (physx::PxAbs(area) < PX_EPS_F32 * PX_EPS_F32)
	{
		return false;
	}

	const bool ccw = area > 0.0f;

	for (physx::PxU32 i = 0; i < size; ++i)
	{
		physx::PolyVert& vert = cutout.convexLoops[0].polyVerts[i];
		vert.index = (physx::PxU16)(ccw ? i : size - i - 1);
		vert.flags = 0;
	}

	const physx::PxF32 cleanupTolerance2 = square(cleanupTolerance);

	// Find reflex vertices
	for (physx::PxU32 i = 0; i < cutout.convexLoops.size();)
	{
		physx::ConvexLoop& loop = cutout.convexLoops[i];
		const physx::PxU32 loopSize = loop.polyVerts.size();
		if (loopSize <= 3)
		{
			++i;
			continue;
		}
		physx::PxU32 j = 0;
		for (; j < loopSize; ++j)
		{
			const physx::PxVec3& v0 = cutout.vertices[loop.polyVerts[(j + loopSize - 1) % loopSize].index];
			const physx::PxVec3& v1 = cutout.vertices[loop.polyVerts[j].index];
			const physx::PxVec3& v2 = cutout.vertices[loop.polyVerts[(j + 1) % loopSize].index];
			const physx::PxVec3 e0 = v1 - v0;
			if (crossZ(e0, v2 - v1) < 0.0f)
			{
				// reflex
				break;
			}
		}
		if (j < loopSize)
		{
			// Find a vertex
			physx::PxF32 minLen2 = PX_MAX_F32;
			physx::PxF32 maxMinDist = -PX_MAX_F32;
			physx::PxU32 kToUse = 0;
			physx::PxU32 mToUse = 2;
			bool cleanSliceFound = false;	// A transversal is parallel with an edge
			for (physx::PxU32 k = 0; k < loopSize; ++k)
			{
				const physx::PxVec3& vkPrev = cutout.vertices[loop.polyVerts[(k + loopSize - 1) % loopSize].index];
				const physx::PxVec3& vk = cutout.vertices[loop.polyVerts[k].index];
				const physx::PxVec3& vkNext = cutout.vertices[loop.polyVerts[(k + 1) % loopSize].index];
				const physx::PxU32 mStop = k ? loopSize : loopSize - 1;
				for (physx::PxU32 m = k + 2; m < mStop; ++m)
				{
					const physx::PxVec3& vmPrev = cutout.vertices[loop.polyVerts[(m + loopSize - 1) % loopSize].index];
					const physx::PxVec3& vm = cutout.vertices[loop.polyVerts[m].index];
					const physx::PxVec3& vmNext = cutout.vertices[loop.polyVerts[(m + 1) % loopSize].index];
					const physx::PxVec3 newEdge = vm - vk;
					if (!directionsXYOrderedCCW(vk - vkPrev, newEdge, vkNext - vk) ||
					        !directionsXYOrderedCCW(vm - vmPrev, -newEdge, vmNext - vm))
					{
						continue;
					}
					const physx::PxF32 len2 = newEdge.magnitudeSquared();
					physx::PxF32 minDist = PX_MAX_F32;
					for (physx::PxU32 l = 0; l < loopSize; ++l)
					{
						const physx::PxU32 l1 = (l + 1) % loopSize;
						if (l == k || l1 == k || l == m || l1 == m)
						{
							continue;
						}
						const physx::PxVec3& vl = cutout.vertices[loop.polyVerts[l].index];
						const physx::PxVec3& vl1 = cutout.vertices[loop.polyVerts[l1].index];
						const physx::PxF32 dist = segmentsIntersectXY(vl, vl1 - vl, vk, newEdge);
						if (dist < minDist)
						{
							minDist = dist;
						}
					}
					if (minDist <= 0.0f)
					{
						if (minDist > maxMinDist)
						{
							maxMinDist = minDist;
							kToUse = k;
							mToUse = m;
						}
					}
					else
					{
						if (perpendicularDistanceSquared(vkPrev, vk, vm) <= cleanupTolerance2 ||
						        perpendicularDistanceSquared(vk, vm, vmNext) <= cleanupTolerance2)
						{
							if (!cleanSliceFound)
							{
								minLen2 = len2;
								kToUse = k;
								mToUse = m;
							}
							else
							{
								if (len2 < minLen2)
								{
									minLen2 = len2;
									kToUse = k;
									mToUse = m;
								}
							}
							cleanSliceFound = true;
						}
						else if (!cleanSliceFound && len2 < minLen2)
						{
							minLen2 = len2;
							kToUse = k;
							mToUse = m;
						}
					}
				}
			}
			physx::ConvexLoop& newLoop = cutout.convexLoops.insert();
			physx::ConvexLoop& oldLoop = cutout.convexLoops[i];
			newLoop.polyVerts.resize(mToUse - kToUse + 1);
			for (physx::PxU32 n = 0; n <= mToUse - kToUse; ++n)
			{
				newLoop.polyVerts[n] = oldLoop.polyVerts[kToUse + n];
			}
			newLoop.polyVerts[mToUse - kToUse].flags = 1;	// Mark this vertex (and edge that follows) as a split edge
			oldLoop.polyVerts[kToUse].flags = 1;	// Mark this vertex (and edge that follows) as a split edge
			oldLoop.polyVerts.removeRange(kToUse + 1, (mToUse - (kToUse + 1)));
			if (cleanupTolerance > 0.0f)
			{
				cleanCutout(cutout, i, cleanupTolerance);
				cleanCutout(cutout, cutout.convexLoops.size() - 1, cleanupTolerance);
			}
		}
		else
		{
			if (cleanupTolerance > 0.0f)
			{
				cleanCutout(cutout, i, cleanupTolerance);
			}
			++i;
		}
	}

	return true;
}

static void traceRegion(physx::Array<POINT2D>& trace, Map2d<physx::PxU32>& regions, Map2d<physx::PxU8>& pathCounts, physx::PxU32 regionIndex, const POINT2D& startPoint)
{
	POINT2D t = startPoint;
	trace.reset();
	trace.pushBack(t);
	++pathCounts(t.x, t.y);	// Increment path count
	// Find initial path direction
	physx::PxI32 dirN;
	for (dirN = 1; dirN < 8; ++dirN)
	{
		const POINT2D t1 = POINT2D(t.x + taxicabSine(dirN + 2), t.y + taxicabSine(dirN));
		if (regions(t1.x, t1.y) != regionIndex)
		{
			break;
		}
	}
	bool done = false;
	do
	{
		for (physx::PxI32 i = 1; i < 8; ++i)	// Skip direction we just came from
		{
			--dirN;
			const POINT2D t1 = POINT2D(t.x + taxicabSine(dirN + 2), t.y + taxicabSine(dirN));
			if (regions(t1.x, t1.y) != regionIndex)
			{
				if (t1.x == trace[0].x && t1.y == trace[0].y)
				{
					done = true;
					break;
				}
				trace.pushBack(t1);
				t = t1;
				++pathCounts(t.x, t.y);	// Increment path count
				dirN += 4;
				break;
			}
		}
	}
	while (!done);
}

static void createCutoutSet(physx::CutoutSet& cutoutSet, const physx::PxU8* pixelBuffer, physx::PxU32 bufferWidth, physx::PxU32 bufferHeight, physx::PxF32 snapThreshold, bool periodic)
{
	cutoutSet.cutouts.reset();
	cutoutSet.periodic = periodic;
	cutoutSet.dimensions = physx::PxVec2((physx::PxF32)bufferWidth, (physx::PxF32)bufferHeight);

	if (!periodic)
	{
		cutoutSet.dimensions[0] += 1.0f;
		cutoutSet.dimensions[1] += 1.0f;
	}

	if (pixelBuffer == NULL || bufferWidth == 0 || bufferHeight == 0)
	{
		return;
	}

	const int borderPad = periodic ? 0 : 2;	// Padded for borders if not periodic
	const int originCoord = periodic ? 0 : 1;

	BitMap map(bufferWidth + borderPad, bufferHeight + borderPad, 0);
	map.setOrigin(originCoord, originCoord);

	for (physx::PxU32 y = 0; y < bufferHeight; ++y)
	{
		for (physx::PxU32 x = 0; x < bufferWidth; ++x)
		{
			const physx::PxU32 pix = 5033165 * (physx::PxU32)pixelBuffer[0] + 9898557 * (physx::PxU32)pixelBuffer[1] + 1845494 * (physx::PxU32)pixelBuffer[2];
			pixelBuffer += 3;
			if ((pix >> 28) != 0)
			{
				map.set(x, y);
			}
		}
	}

	// Add borders if not tiling
	if (!periodic)
	{
		for (physx::PxI32 x = -1; x <= (physx::PxI32)bufferWidth; ++x)
		{
			map.set(x, -1);
			map.set(x, bufferHeight);
		}
		for (physx::PxI32 y = -1; y <= (physx::PxI32)bufferHeight; ++y)
		{
			map.set(-1, y);
			map.set(bufferWidth, y);
		}
	}

	// Now search for regions

	// Create a region map
	Map2d<physx::PxU32> regions(bufferWidth + borderPad, bufferHeight + borderPad, 0xFFFFFFFF);	// Initially an invalid value
	regions.setOrigin(originCoord, originCoord);

	// Create a path counting map
	Map2d<physx::PxU8> pathCounts(bufferWidth + borderPad, bufferHeight + borderPad, 0);
	pathCounts.setOrigin(originCoord, originCoord);

	// Bump path counts on borders
	if (!periodic)
	{
		for (physx::PxI32 x = -1; x <= (physx::PxI32)bufferWidth; ++x)
		{
			pathCounts(x, -1) = 1;
			pathCounts(x, bufferHeight) = 1;
		}
		for (physx::PxI32 y = -1; y <= (physx::PxI32)bufferHeight; ++y)
		{
			pathCounts(-1, y) = 1;
			pathCounts(bufferWidth, y) = 1;
		}
	}

	physx::Array<POINT2D> stack;
	physx::Array<POINT2D> traceStarts;
	physx::Array< physx::Array<POINT2D>* > traces;

	// Initial fill of region maps and path maps
	for (physx::PxI32 y = 0; y < (physx::PxI32)bufferHeight; ++y)
	{
		for (physx::PxI32 x = 0; x < (physx::PxI32)bufferWidth; ++x)
		{
			if (map.read(x-1, y) && !map.read(x, y))
			{
				// Found an empty spot next to a filled spot
				POINT2D t(x - 1, y);
				const physx::PxU32 regionIndex = traceStarts.size();
				traceStarts.pushBack(t);	// Save off initial point
				traces.insert();	// This must be the same size as traceStarts
				traces.back() = (physx::Array<POINT2D>*)PX_ALLOC(sizeof(physx::Array<POINT2D>), PX_DEBUG_EXP("CutoutPoint2DSet"));
				new(traces.back()) physx::Array<POINT2D>;
				// Flood fill region map
				stack.pushBack(POINT2D(x, y));
				do
				{
					const POINT2D s = stack.back();
					stack.popBack();
					map.set(s.x, s.y);
					regions(s.x, s.y) = regionIndex;
					POINT2D n;
					for (physx::PxI32 i = 0; i < 4; ++i)
					{
						const physx::PxI32 i0 = i & 1;
						const physx::PxI32 i1 = (i >> 1) & 1;
						n.x = s.x + i0 - i1;
						n.y = s.y + i0 + i1 - 1;
						if (!map.read(n.x, n.y))
						{
							stack.pushBack(n);
						}
					}
				}
				while (stack.size());
				// Trace region
				PX_ASSERT(map.read(t.x, t.y));
				physx::Array<POINT2D>& trace = *traces[regionIndex];
				traceRegion(trace, regions, pathCounts, regionIndex, t);
			}
		}
	}

	physx::PxU32 cutoutCount = traces.size();

	// Now expand regions until the paths completely overlap
	bool somePathChanged;
	int sanityCounter = 1000;
	bool abort = false;
	do
	{
		somePathChanged = false;
		for (physx::PxU32 i = 0; i < cutoutCount; ++i)
		{
			bool pathChanged = false;
			physx::Array<POINT2D>& trace = *traces[i];
			for (physx::PxU32 j = 0; j < trace.size(); ++j)
			{
				const POINT2D& t = trace[j];
				if (pathCounts(t.x, t.y) == 1)
				{
					regions(t.x, t.y) = i;
					pathChanged = true;
				}
			}
			if (pathChanged)
			{
				// Recalculate cutout
				// Decrement pathCounts
				for (physx::PxU32 j = 0; j < trace.size(); ++j)
				{
					const POINT2D& t = trace[j];
					--pathCounts(t.x, t.y);
				}
				// Erase trace
				// Calculate new start point
				POINT2D& t = traceStarts[i];
				int stop = (int)cutoutSet.dimensions.x;
				while (regions(t.x, t.y) == i)
				{
					--t.x;
					if(--stop < 0)
					{
						// There is an error; abort
						break;
					}
				}
				if(stop < 0)
				{
					// Release traces and abort
					abort = true;
					somePathChanged = false;
					break;
				}
				traceRegion(trace, regions, pathCounts, i, t);
				somePathChanged = true;
			}
		}
		if (--sanityCounter <= 0)
		{
			abort = true;
			break;
		}
	}
	while (somePathChanged);

	if (abort)
	{
		for (physx::PxU32 i = 0; i < cutoutCount; ++i)
		{
			traces[i]->~Array<POINT2D>();
			PX_FREE(traces[i]);
		}
		cutoutCount = 0;
	}

	// Create cutouts
	cutoutSet.cutouts.resize(cutoutCount);
	for (physx::PxU32 i = 0; i < cutoutCount; ++i)
	{
		createCutout(cutoutSet.cutouts[i], *traces[i], snapThreshold, bufferWidth, bufferHeight, !cutoutSet.periodic);
	}

	simplifyCutoutSet(cutoutSet, snapThreshold, bufferWidth, bufferHeight);

	// Release traces
	for (physx::PxU32 i = 0; i < cutoutCount; ++i)
	{
		traces[i]->~Array<POINT2D>();
		PX_FREE(traces[i]);
	}

	// Decompose each cutout in the set into convex loops
	physx::PxU32 cutoutSetSize = 0;
	for (physx::PxU32 i = 0; i < cutoutSet.cutouts.size(); ++i)
	{
		bool success = decomposeCutoutIntoConvexLoops(cutoutSet.cutouts[i]);
		if (success)
		{
			if (cutoutSetSize != i)
			{
				cutoutSet.cutouts[cutoutSetSize] = cutoutSet.cutouts[i];
			}
			++cutoutSetSize;
		}
	}
	cutoutSet.cutouts.resize(cutoutSetSize);
}

class Matrix22
{
public:
	//! Default constructor
	Matrix22()
	{}

	//! Construct from two base vectors
	Matrix22(const physx::PxVec2& col0, const physx::PxVec2& col1)
		: column0(col0), column1(col1)
	{}

	//! Construct from float[4]
	explicit Matrix22(physx::PxReal values[]):
		column0(values[0],values[1]),
		column1(values[2],values[3])
	{
	}

	//! Copy constructor
	Matrix22(const Matrix22& other)
		: column0(other.column0), column1(other.column1)
	{}

	//! Assignment operator
	Matrix22& operator=(const Matrix22& other)
	{
		column0 = other.column0;
		column1 = other.column1;
		return *this;
	}

	//! Set to identity matrix
	static Matrix22 createIdentity()
	{
		return Matrix22(physx::PxVec2(1,0), physx::PxVec2(0,1));
	}

	//! Set to zero matrix
	static Matrix22 createZero()
	{
		return Matrix22(physx::PxVec2(0.0f), physx::PxVec2(0.0f));
	}

	//! Construct from diagonal, off-diagonals are zero.
	static Matrix22 createDiagonal(const physx::PxVec2& d)
	{
		return Matrix22(physx::PxVec2(d.x,0.0f), physx::PxVec2(0.0f,d.y));
	}


	//! Get transposed matrix
	Matrix22 getTranspose() const
	{
		const physx::PxVec2 v0(column0.x, column1.x);
		const physx::PxVec2 v1(column0.y, column1.y);

		return Matrix22(v0,v1);   
	}

	//! Get the real inverse
	Matrix22 getInverse() const
	{
		const physx::PxReal det = getDeterminant();
		Matrix22 inverse;

		if(det != 0)
		{
			const physx::PxReal invDet = 1.0f/det;

			inverse.column0[0] = invDet * column1[1];						
			inverse.column0[1] = invDet * (-column0[1]);

			inverse.column1[0] = invDet * (-column1[0]);
			inverse.column1[1] = invDet * column0[0];

			return inverse;
		}
		else
		{
			return createIdentity();
		}
	}

	//! Get determinant
	physx::PxReal getDeterminant() const
	{
		return column0[0] * column1[1] - column0[1] * column1[0];
	}

	//! Unary minus
	Matrix22 operator-() const
	{
		return Matrix22(-column0, -column1);
	}

	//! Add
	Matrix22 operator+(const Matrix22& other) const
	{
		return Matrix22( column0+other.column0,
					  column1+other.column1);
	}

	//! Subtract
	Matrix22 operator-(const Matrix22& other) const
	{
		return Matrix22( column0-other.column0,
					  column1-other.column1);
	}

	//! Scalar multiplication
	Matrix22 operator*(physx::PxReal scalar) const
	{
		return Matrix22(column0*scalar, column1*scalar);
	}
	
	//! Matrix vector multiplication (returns 'this->transform(vec)')
	physx::PxVec2 operator*(const physx::PxVec2& vec) const
	{
		return transform(vec);
	}

	//! Matrix multiplication
	Matrix22 operator*(const Matrix22& other) const
	{
		//Rows from this <dot> columns from other
		//column0 = transform(other.column0) etc
		return Matrix22(transform(other.column0), transform(other.column1));
	}

	// a <op>= b operators

	//! Equals-add
	Matrix22& operator+=(const Matrix22& other)
	{
		column0 += other.column0;
		column1 += other.column1;
		return *this;
	}

	//! Equals-sub
	Matrix22& operator-=(const Matrix22& other)
	{
		column0 -= other.column0;
		column1 -= other.column1;
		return *this;
	}

	//! Equals scalar multiplication
	Matrix22& operator*=(physx::PxReal scalar)
	{
		column0 *= scalar;
		column1 *= scalar;
		return *this;
	}

	//! Element access, mathematical way!
	physx::PxReal operator()(unsigned int row, unsigned int col) const
	{
		return (*this)[col][row];
	}

	//! Element access, mathematical way!
	physx::PxReal& operator()(unsigned int row, unsigned int col)
	{
		return (*this)[col][row];
	}

	// Transform etc
	
	//! Transform vector by matrix, equal to v' = M*v
	physx::PxVec2 transform(const physx::PxVec2& other) const
	{
		return column0*other.x + column1*other.y;
	}

	physx::PxVec2& operator[](int num)			{return (&column0)[num];}
	const	physx::PxVec2& operator[](int num) const	{return (&column0)[num];}

	//Data, see above for format!

	physx::PxVec2 column0, column1; //the two base vectors
};

PX_INLINE bool calculateUVMapping(const physx::NxExplicitRenderTriangle& triangle, physx::PxMat33& theResultMapping)
{
	physx::PxMat33 rMat;
	physx::PxMat33 uvMat;
	for (int col = 0; col < 3; ++col)
	{
		rMat[col] = triangle.vertices[col].position;
		uvMat[col] = physx::PxVec3(triangle.vertices[col].uv[0][0], triangle.vertices[col].uv[0][1], 1.0f);
	}

	if (uvMat.getDeterminant() == 0.0f)
	{
		return false;
	}

	theResultMapping = rMat*uvMat.getInverse();

	return true;
}

static bool calculateUVMapping(physx::IExplicitHierarchicalMesh& theHMesh, const physx::PxVec3& theDir, physx::PxMat33& theResultMapping)
{	
	physx::PxVec3 cutoutDir( theDir );
	cutoutDir.normalize( );

	const physx::PxF32 cosineThreshold = physx::PxCos(3.141593f / 180);	// 1 degree

	physx::NxExplicitRenderTriangle* triangleToUse = NULL;
	physx::PxF32 greatestCosine = -PX_MAX_F32;
	physx::PxF32 greatestArea = 0.0f;	// for normals within the threshold
	for ( physx::PxU32 partIndex = 0; partIndex < theHMesh.partCount(); ++partIndex )
	{
		physx::NxExplicitRenderTriangle* theTriangles = theHMesh.meshTriangles( partIndex );
		physx::PxU32 triangleCount = theHMesh.meshTriangleCount( partIndex );
		for ( physx::PxU32 tIndex = 0; tIndex < triangleCount; ++tIndex )
		{			
			physx::NxExplicitRenderTriangle& theTriangle = theTriangles[tIndex];
			physx::PxVec3 theEdge1 = theTriangle.vertices[1].position - theTriangle.vertices[0].position;
			physx::PxVec3 theEdge2 = theTriangle.vertices[2].position - theTriangle.vertices[0].position;
			physx::PxVec3 theNormal = theEdge1.cross( theEdge2 );
			physx::PxReal theArea = theNormal.normalize();	// twice the area, but that's ok

			if (theArea == 0.0f)
			{
				continue;
			}

			const physx::PxF32 cosine = cutoutDir.dot(theNormal);

			if (cosine < cosineThreshold)
			{
				if (cosine > greatestCosine && greatestArea == 0.0f)
				{
					greatestCosine = cosine;
					triangleToUse = &theTriangle;
				}
			}
			else
			{
				if (theArea > greatestArea)
				{
					greatestArea = theArea;
					triangleToUse = &theTriangle;
				}
			}
		}
	}

	if (triangleToUse == NULL)
	{
		return false;
	}

	return calculateUVMapping(*triangleToUse, theResultMapping);
}

namespace physx
{
namespace apex
{

PX_INLINE void serialize(physx::PxFileBuf& stream, const PolyVert& p)
{
	stream << p.index << p.flags;
}

PX_INLINE void deserialize(physx::PxFileBuf& stream, physx::PxU32 version, PolyVert& p)
{
	// original version
	PX_UNUSED(version);
	stream >> p.index >> p.flags;
}

PX_INLINE void serialize(physx::PxFileBuf& stream, const ConvexLoop& l)
{
	apex::serialize(stream, l.polyVerts);
}

PX_INLINE void deserialize(physx::PxFileBuf& stream, physx::PxU32 version, ConvexLoop& l)
{
	// original version
	apex::deserialize(stream, version, l.polyVerts);
}

PX_INLINE void serialize(physx::PxFileBuf& stream, const Cutout& c)
{
	apex::serialize(stream, c.vertices);
	apex::serialize(stream, c.convexLoops);
}

PX_INLINE void deserialize(physx::PxFileBuf& stream, physx::PxU32 version, Cutout& c)
{
	// original version
	apex::deserialize(stream, version, c.vertices);
	apex::deserialize(stream, version, c.convexLoops);
}

void CutoutSet::serialize(physx::PxFileBuf& stream) const
{
	stream << (physx::PxU32)Current;

	apex::serialize(stream, cutouts);
}

void CutoutSet::deserialize(physx::PxFileBuf& stream)
{
	const physx::PxU32 version = stream.readDword();

	apex::deserialize(stream, version, cutouts);
}

}
} // end namespace physx::apex

namespace FractureTools
{
ICutoutSet* createCutoutSet()
{
	return new physx::CutoutSet();
}

void buildCutoutSet(ICutoutSet& cutoutSet, const physx::PxU8* pixelBuffer, physx::PxU32 bufferWidth, physx::PxU32 bufferHeight, physx::PxF32 snapThreshold, bool periodic)
{
	::createCutoutSet(*(physx::CutoutSet*)&cutoutSet, pixelBuffer, bufferWidth, bufferHeight, snapThreshold, periodic);
}

bool calculateCutoutUVMapping(physx::IExplicitHierarchicalMesh& hMesh, const physx::PxVec3& targetDirection, physx::PxMat33& theMapping)
{
	return ::calculateUVMapping(hMesh, targetDirection, theMapping);
}

bool calculateCutoutUVMapping(const physx::NxExplicitRenderTriangle& targetDirection, physx::PxMat33& theMapping)
{
	return ::calculateUVMapping(targetDirection, theMapping);
}
} // namespace FractureTools

#endif

#ifdef _MANAGED
#pragma managed(pop)
#endif
