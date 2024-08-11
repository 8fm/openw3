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

#ifndef SQ_BUCKETPRUNER_H
#define SQ_BUCKETPRUNER_H

#include "SqPruningPool.h"
#include "Ice/IceRevisitedRadix2.h"
#include "PsHashMap.h"

#define FREE_PRUNER_SIZE	16

namespace physx
{
namespace Sq
{
	typedef PxU32	BucketWord;

	PX_ALIGN_PREFIX(16)	struct BucketBox
	{
		PxVec3	mCenter;
		PxU32	mData0;
		PxVec3	mExtents;
		PxU32	mData1;
#ifndef PX_PS3
	#ifdef _DEBUG
		// PT: we need the original min value for debug checks. Using the center/extents version
		// fails because recomputing the min from them introduces FPU accuracy errors in the values.
		float	mDebugMin;
	#endif
#endif

		PX_FORCE_INLINE	PxVec3	getMin()	const
		{
			return mCenter - mExtents;
		}

		PX_FORCE_INLINE	PxVec3	getMax()	const
		{
			return mCenter + mExtents;
		}

		PX_FORCE_INLINE void	setEmpty()
		{
			mCenter = PxVec3(0.0f);
			mExtents = PxVec3(-PX_MAX_BOUNDS_EXTENTS);
#ifndef PX_PS3
	#ifdef _DEBUG
			mDebugMin = PX_MAX_BOUNDS_EXTENTS;
	#endif
#endif
		}
	}PX_ALIGN_SUFFIX(16);

	PX_ALIGN_PREFIX(16) struct BoxBucket
	{
					BoxBucket();

		void		classifyBoxes(	float limitX, float limitZ,
									PxU32 nb,
									BucketBox* PX_RESTRICT boxes,
									const PrunerPayload* PX_RESTRICT objects,
									BucketBox* PX_RESTRICT sortedBoxes,
									PrunerPayload* PX_RESTRICT sortedObjects,
									bool isCrossBucket, PxU32 sortAxis);

		PX_FORCE_INLINE	void	initCounters()
		{
			for(PxU32 i=0;i<5;i++)
				mCounters[i] = 0;
			for(PxU32 i=0;i<5;i++)
				mOffsets[i] = 0;
		}

		BucketWord	mCounters[5];
		BucketWord	mOffsets[5];
//		PX_ALIGN(16, 	BucketBox	mBucketBox[5]);
		BucketBox	mBucketBox[5];
		PxU16		mOrder[8];
	}PX_ALIGN_SUFFIX(16);

	PX_FORCE_INLINE PxU32 hash64(const PxU64 key)
	{
		PxU64 k = key;
		k += ~(k << 32);
		k ^= (k >> 22);
		k += ~(k << 13);
		k ^= (k >> 8);
		k += (k << 3);
		k ^= (k >> 15);
		k += ~(k << 27);
		k ^= (k >> 31);
		return (PxU32)(PX_MAX_U32 & k);
	}

	PX_FORCE_INLINE PxU32 hash(const PrunerPayload& payload)
	{
#if defined(PX_X64)
		const PxU32 h0 = Ps::hash((const void*)payload.data[0]);
		const PxU32 h1 = Ps::hash((const void*)payload.data[1]);
		return hash64(PxU64(h0)|(PxU64(h1)<<32));
#else
		return hash64(PxU64(payload.data[0])|(PxU64(payload.data[1])<<32));
#endif
	}

	typedef Ps::HashMap<PrunerPayload, PxU32> BucketPrunerMap;

	class BucketPrunerCore : public Ps::UserAllocated
	{
		public:
											BucketPrunerCore(bool externalMemory=true);
											~BucketPrunerCore();

						void				release(bool releaseMemory);

						void				preallocate(PxU32 nb);
						void				setExternalMemory(PxU32 nbObjects, PxBounds3* boxes, PrunerPayload* objects);

						bool				addObject(const PrunerPayload& object, const PxBounds3& worldAABB);
						bool				removeObject(const PrunerPayload& object);
						bool				updateObject(const PxBounds3& worldAABB, const PrunerPayload& object);

						PxAgain				raycast(const PxVec3& origin, const PxVec3& unitDir, PxReal& inOutDistance, PrunerCallback&) const;
						PxAgain				overlap(const ShapeData& queryVolume, PrunerCallback&) const;
						PxAgain				sweep(const ShapeData& queryVolume, const PxVec3& unitDir, PxReal& inOutDistance, PrunerCallback&) const;

						void				shiftOrigin(const PxVec3& shift);

						void				visualize(Cm::RenderOutput& out, PxU32 color) const;

		PX_FORCE_INLINE	void				build()					{ classifyBoxes();	}

//		private:
//	Gu::RadixSortBuffered rs;	// ###TODO: some allocs here, remove

						PxU32				mCoreNbObjects;
						PxU32				mCoreCapacity;
						PxBounds3*			mCoreBoxes;
						PrunerPayload*		mCoreObjects;
						PxU32*				mCoreRemap;
						BucketBox*			mSortedWorldBoxes;
						PrunerPayload*		mSortedObjects;
						PxU32				mNbFree;
						PrunerPayload		mFreeObjects[FREE_PRUNER_SIZE];
						PxBounds3			mFreeBounds[FREE_PRUNER_SIZE];
						BucketPrunerMap		mMap;
						PxU32				mSortedNb;
						PxU32				mSortedCapacity;
						PxU32				mSortAxis;

//		PX_ALIGN(16, 	BucketBox			mGlobalBox);
						BucketBox			mGlobalBox;
						BoxBucket			mLevel1;
						BoxBucket			mLevel2[5];
						BoxBucket			mLevel3[5][5];

						bool				mDirty;
						bool				mOwnMemory;
		private:
						void				classifyBoxes();
						void				allocateSortedMemory(PxU32 nb);
						void				resizeCore();
	};

	class BucketPruner
#if !PX_IS_SPU
		: public Pruner
#endif
	{
		public:
											BucketPruner();
		virtual								~BucketPruner();

						bool				addObjects(PrunerHandle* results, const PxBounds3* bounds, const PrunerPayload* payload, PxU32 count);
						void				removeObjects(const PrunerHandle* handles, PxU32 count);
						void				updateObjects(const PrunerHandle* handles, const PxBounds3* newBounds, PxU32 count);

						PxAgain				raycast(const PxVec3& origin, const PxVec3& unitDir, PxReal& inOutDistance, PrunerCallback&) const;
						PxAgain				overlap(const ShapeData& queryVolume, PrunerCallback&) const;
						PxAgain				sweep(const ShapeData& queryVolume, const PxVec3& unitDir, PxReal& inOutDistance, PrunerCallback&) const;

						const PrunerPayload& getPayload(const PrunerHandle& h) const { return mPool.getPayload(h); }
						
						void				preallocate(PxU32 entries) { mPool.preallocate(entries); }

		virtual			void				commit();

		virtual			void				shiftOrigin(const PxVec3& shift);

		virtual			void				visualize(Cm::RenderOutput& out, PxU32 color) const;

		private:
						BucketPrunerCore	mCore;
						PruningPool			mPool;
	};

} // namespace Sq

}

#endif // SQ_BUCKETPRUNER_H
