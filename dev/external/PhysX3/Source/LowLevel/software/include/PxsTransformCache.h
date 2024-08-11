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

#ifndef PXS_TRANSFORM_CACHE_H
#define PXS_TRANSFORM_CACHE_H

#include "CmPhysXCommon.h"
#include "CmIDPool.h"
#include "CmBitMap.h"
#include "PsUserAllocated.h"

#define PX_DEFAULT_CACHE_SIZE 512

namespace physx
{

	class PxsTransformCache : public Ps::UserAllocated
	{
		typedef PxU32 RefCountType;

	public:
		PxsTransformCache()
		{
			mTransformCache.reserve(PX_DEFAULT_CACHE_SIZE);
			mRefCounts.reserve(PX_DEFAULT_CACHE_SIZE);
			mRefCounts.forceSize_Unsafe(PX_DEFAULT_CACHE_SIZE);
			PxMemZero(mRefCounts.begin(), mRefCounts.capacity() * sizeof(RefCountType));
			mTransformCache.forceSize_Unsafe(PX_DEFAULT_CACHE_SIZE);
		}

		PX_FORCE_INLINE PxU32			createID()						
		{ 
			PxU32 oldCapacity = mTransformCache.capacity();
			PxU32 newId = mIDPool.getNewID();
			PX_ASSERT(newId <= mTransformCache.capacity());
			if(newId == oldCapacity)
			{
				PxU32 newCapacity = oldCapacity * 2;
				mTransformCache.reserve(newCapacity);
				mRefCounts.reserve(newCapacity);
				mTransformCache.forceSize_Unsafe(newCapacity);
				mRefCounts.forceSize_Unsafe(newCapacity);
				PxMemZero(mRefCounts.begin() + oldCapacity, (newCapacity - oldCapacity) * sizeof(RefCountType));
			}
			mRefCounts[newId] = 0;
			return newId;
		}
		PX_FORCE_INLINE void			releaseID(PxU32 id)				
		{ 
			PX_ASSERT(mRefCounts[id] == 0);
			mIDPool.freeID(id);
		}

		PX_FORCE_INLINE void reset()
		{
			mIDPool.freeAll();
			PxMemZero(mRefCounts.begin(), mRefCounts.capacity() * sizeof(RefCountType));
		}

		PX_FORCE_INLINE void setTransformCache(const PxTransform& transform, const PxU32 index)
		{
			mTransformCache[index] = transform;
		}

		PX_FORCE_INLINE const PxTransform& getTransformCache(const PxU32 index) const
		{
			return mTransformCache[index];
		}

		PX_FORCE_INLINE PxU32 getReferenceCount(const PxU32 index) const
		{
			return mRefCounts[index];
		}

		PX_FORCE_INLINE void incReferenceCount(const PxU32 index)
		{
			mRefCounts[index]++;
		}

		PX_FORCE_INLINE bool decReferenceCount(const PxU32 index)
		{
			PX_ASSERT(mRefCounts[index] != 0);
			PxU32 newCount = mRefCounts[index]-1;
			mRefCounts[index] = newCount;
			return newCount == 0;
		}

		PX_FORCE_INLINE PxTransform& getTransformCache(const PxU32 index)
		{
			return mTransformCache[index];
		}

		PX_FORCE_INLINE void shiftTransforms(const PxVec3& shift)
		{
			for(PxU32 i=0; i < mTransformCache.capacity(); i++)
			{
				if (mRefCounts[i])
				{
					mTransformCache[i].p += shift;
				}
			}
		}

	private:
		Cm::IDPool					mIDPool;
		Ps::Array<PxTransform>		mTransformCache;
		Ps::Array<RefCountType>		mRefCounts;
	};
}

#endif