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


#ifndef PX_PHYSICS_NP_QUERYSHARED
#define PX_PHYSICS_NP_QUERYSHARED

//========================================================================================================================
// these partial template specializations are used to generalize the query code to be reused for all permutations of
// hit type=(raycast, overlap, sweep) x query type=(ANY, SINGLE, MULTIPLE)
template <typename HitType> struct HitTypeSupport { enum { IsRaycast = 0, IsSweep = 0, IsOverlap = 0 }; };
template <> struct HitTypeSupport<PxRaycastHit>
{
	enum { IsRaycast = 1, IsSweep = 0, IsOverlap = 0 };
	static PX_FORCE_INLINE PxReal getDistance(PxQueryHit& hit) { return static_cast<PxRaycastHit&>(hit).distance; }
};
template <> struct HitTypeSupport<PxSweepHit>
{
	enum { IsRaycast = 0, IsSweep = 1, IsOverlap = 0 };
	static PX_FORCE_INLINE PxReal getDistance(PxQueryHit& hit) { return static_cast<PxSweepHit&>(hit).distance; }
};
template <> struct HitTypeSupport<PxOverlapHit>
{
	enum { IsRaycast = 0, IsSweep = 0, IsOverlap = 1 };
	static PX_FORCE_INLINE PxReal getDistance(PxQueryHit&) { return -1.0f; }
};

#define HITDIST(hit) HitTypeSupport<HitType>::getDistance(hit)

template <typename HitType>
PX_FORCE_INLINE HitType readHit(HitType* ppuHitBuffer, PxU32 hitIndex)
{
	#if PX_IS_SPU
		static MemFetchBufferA<HitType> buf;
		HitType* ptr = memFetchAsync<HitType>(MemFetchPtr(ppuHitBuffer+hitIndex), 5, buf);
		memFetchWait(5);
		return *ptr;
	#else
		return ppuHitBuffer[hitIndex];
	#endif
}

template <typename HitType>
PX_FORCE_INLINE void writeHit(HitType* ppuHitBuffer, PxU32 hitIndex, const HitType& hit)
{
	#if PX_IS_SPU
		memStoreAsync(&hit, MemFetchPtr(ppuHitBuffer+hitIndex), 5);
		memStoreWait(5); // AP TODO: make asynchronous
	#else
		ppuHitBuffer[hitIndex] = hit;
	#endif
}


template<typename HitType>
static PxU32 clipHitsToNewMaxDist(HitType* ppuHits, PxU32 count, PxReal newMaxDist)
{
	PxU32 newCount = count;
	for (PxU32 i = 0; i < count; i++)
	{
		HitType hit = readHit<HitType>(ppuHits, i);
		if (HITDIST(hit) > newMaxDist) // overwrite this hit with last hit in the buffer
		{
			HitType lastHit = readHit<HitType>(ppuHits, newCount-1);
			writeHit(ppuHits, i, lastHit); // hits[i] = hits[newCount-1];
			newCount--;
		}
	}
	return newCount;
}


#endif // PX_PHYSICS_NP_SCENE