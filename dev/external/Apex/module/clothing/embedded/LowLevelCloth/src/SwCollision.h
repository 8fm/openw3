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

#pragma once

#include "Types.h"
#include "StackAllocator.h"
#include "Simd4i.h"

#ifdef PX_PROFILE
#include "PxProfileEventSender.h"
#include "PxProfileZone.h"
#endif

namespace physx
{

class PxProfileZone;

namespace cloth
{

#ifdef PX_PROFILE

	struct ProfileZone
	{
		ProfileZone(const char* name, PxProfileZone* profiler) 
			: mSender(profiler),
			mEventId(profiler ? profiler->getEventIdForName(name) : PxU16(-1))
		{
			if(mSender)
				mSender->startEvent(mEventId, intptr_t(this));
		}

		~ProfileZone() 
		{
			if(mSender)
				mSender->stopEvent(mEventId, intptr_t(this));
		}

		void setValue(PxI64 value) const
		{
			if(mSender)
				mSender->eventValue(mEventId, intptr_t(this), value);
		}

		PxProfileEventSender* mSender;
		PxU16 mEventId;
	};

#else // PX_PROFILE

	struct ProfileZone
	{
		ProfileZone(const char*, PxProfileZone*) {}
		void setValue(PxI64) const {}
	};

#endif // PX_PROFILE


class SwCloth;
struct SwClothData;
template <typename> struct IterationState;
struct IndexPair;
struct MaskPair;
struct SphereData;
struct ConeData;
struct TriangleData;

typedef StackAllocator<16> SwKernelAllocator;

/**
   Collision handler for SwSolver.
 */
template <typename Simd4f>
class SwCollision
{
	typedef typename Simd4fToSimd4i<Simd4f>::Type Simd4i;

public:
	struct ShapeMask 
	{
		Simd4i mCones;
		Simd4i mSpheres;

		ShapeMask& operator=(const ShapeMask&);
		ShapeMask& operator&=(const ShapeMask&);
	};

	struct CollisionData
	{
		CollisionData();
		SphereData* mSpheres;
		ConeData* mCones;
	};

	struct ImpulseAccumulator;

public:
	SwCollision(SwClothData& clothData, SwKernelAllocator& alloc, PxProfileZone* profiler);
	~SwCollision();

	void operator()(const IterationState<Simd4f>& state);

	static size_t estimateTemporaryMemory(const SwCloth& cloth);
	static size_t estimatePersistentMemory(const SwCloth& cloth);

private:
	SwCollision& operator=(const SwCollision&); // not implemented
	void allocate(CollisionData&);
	void deallocate(const CollisionData&);

	void computeBounds();

	void buildSphereAcceleration(const SphereData*);
	void buildConeAcceleration();
	static void mergeAcceleration(uint32_t*);
	bool buildAcceleration();

	static ShapeMask getShapeMask(const Simd4f&,
		const Simd4i*, const Simd4i*);
	ShapeMask getShapeMask(const Simd4f*) const;
	ShapeMask getShapeMask(const Simd4f*, const Simd4f*) const;

	void collideSpheres(const Simd4i&, 
		const Simd4f*, ImpulseAccumulator&) const;
	Simd4i collideCones(
		const Simd4f*, ImpulseAccumulator&) const;

	void collideSpheres(const Simd4i&, const Simd4f*, 
		Simd4f*, ImpulseAccumulator&) const;
	Simd4i collideCones(const Simd4f*, 
		Simd4f*, ImpulseAccumulator&) const;

	void collideParticles();
	void collideVirtualParticles();
	void collideContinuousParticles();

	void collideConvexes(const IterationState<Simd4f>&);
	void collideConvexes(const Simd4f*, Simd4f*, ImpulseAccumulator&);

	void collideTriangles(const IterationState<Simd4f>&);
	void collideTriangles(const TriangleData*, Simd4f*, ImpulseAccumulator&);

public:

	// acceleration structure
	static const uint32_t sGridSize = 8;
	Simd4i mSphereGrid[6*sGridSize/4];
	Simd4i mConeGrid[6*sGridSize/4];
	Simd4f mGridScale, mGridBias;

	CollisionData mPrevData;
	CollisionData mCurData;

	SwClothData& mClothData;
	SwKernelAllocator& mAllocator;

	uint32_t mNumCollisions;

	PxProfileZone* mProfiler;
	
	static const Simd4f sSkeletonWidth;
};

}

}
