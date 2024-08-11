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


#ifndef PXS_COMPOUND_MANAGER_AUX_H
#define PXS_COMPOUND_MANAGER_AUX_H

#include "PxsBroadPhaseCommon.h"
#include "PxsBroadPhaseConfig.h"
#include "PxcScratchAllocator.h"
#include "PsAllocator.h"
#include "CmBitMap.h"
#include "CmEventProfiler.h"
#ifndef __SPU__
	#include "PxTask.h"
#endif
#ifdef PX_PS3
	#ifndef __SPU__
	#include "PxSpuTask.h"
	#endif
#endif

#ifdef PX_PS3
	#define SPU_AABB 1
	#define FORCE_SINGLE_SPU_AABB 0

	#define SPU_BP_SAP 1
	#define FORCE_SINGLE_SPU_AABB 0

	#define SPU_PROFILE 0
#else
	#define SPU_AABB 0
	#define SPU_BP_SAP 0
#endif //PX_PS3


namespace physx
{

class PxsAABBManager;


#define ALIGN_SIZE_16(size) (((unsigned)(size)+15)&((unsigned)~15))

PX_FORCE_INLINE void copyPodArray(void* tar, const void* src, const PxU32 tarCapacity, const PxU32 srcCapacity, const PxU32 elemSize)
{
	PX_ASSERT(tarCapacity>srcCapacity);
	PX_ASSERT(tar);
	PX_ASSERT(src || 0==srcCapacity);
	PX_ASSERT(0==((uintptr_t)tar & 0x0f));
	PX_ASSERT(0==((uintptr_t)src & 0x0f));
	PX_ASSERT(tarCapacity*elemSize > 0);
	PX_ASSERT(0==((tarCapacity*elemSize) & 15)); 
	if(src)
	{
		PxMemCopy(tar, src, srcCapacity*elemSize);
	}
	PxMemZero(((PxU8*)tar) + srcCapacity*elemSize, (tarCapacity-srcCapacity)*elemSize);
}

void* resizePODArray(const PxU32 oldMaxNb, const PxU32 newMaxNb, const PxU32 elemSize, void* elements);

///////////////////////////////////////////////////////////////////////////////

// ### put this in the bitmap class itself?
PX_FORCE_INLINE Ps::IntBool testBitmap(const Cm::BitMap& bitmap, const PxU32* bitmasks, const PxU32 index) 
{
	PX_ASSERT(index<bitmap.getWordCount()*32);
	return bitmap.getWords()[index>>5] & bitmasks[index&31];
}

PX_FORCE_INLINE void setBitmap(Cm::BitMap& bitmap, const PxU32* bitmasks, const PxU32 index) 
{
	PX_ASSERT(index<bitmap.getWordCount()*32);
	bitmap.getWords()[index>>5] |= bitmasks[index&31];
}

PX_FORCE_INLINE void resetBitmap(Cm::BitMap& bitmap,const PxU32* bitmasks, const  PxU32 index) 
{
	PX_ASSERT(index<bitmap.getWordCount()*32);
	bitmap.getWords()[index>>5] &= ~bitmasks[index&31];
}

///////////////////////////////////////////////////////////////////////////////

//Encode the id for a single aabb so that the id also stores whether the shape belongs to a compound.
//Used by createVolumeV
PX_FORCE_INLINE bool canEncodeForClient(const PxU32 id)						{ return (id < (PX_INVALID_BP_HANDLE>>1));	}
PX_FORCE_INLINE PxcBpHandle	encodeCompoundForClient(const PxU32 id)			{ return (PxcBpHandle)((id+id)|1);							}
PX_FORCE_INLINE PxcBpHandle	encodeSingleForClient(const PxU32 id)			{ return (PxcBpHandle)(id+id);								}

//Decode the encoded id returned by createVolume to reveal the internal element id and whether the shape belongs to a compound.
//Used by releaseVolume etc
PX_FORCE_INLINE PxcBpHandle	decodeCompoundFromClient(const PxcBpHandle bin)	{ PX_ASSERT(bin&1);	return bin>>1;	}
PX_FORCE_INLINE PxcBpHandle	decodeSingleFromClient(const PxcBpHandle id)		{ return id>>1;						}
PX_FORCE_INLINE Ps::IntBool	isClientVolumeCompound(const PxcBpHandle volume)	{ return Ps::IntBool(volume & 1);	}
PX_FORCE_INLINE PxcBpHandle	decodeFromClient(const PxcBpHandle id)			{ return !isClientVolumeCompound(id) ? decodeSingleFromClient(id) : decodeCompoundFromClient(id); }

///////////////////////////////////////////////////////////////////////////////

struct Compound
{
	PxU32			selfCollBitMapWords[MAX_COMPOUND_WORD_COUNT];		
	bool			selfCollide;										
	PxU8			nbElems;													
	PxU8			nbActive;													
	PxU8			pad[1];																					
	PxcBpHandle		bpElemId;											
	PxcBpHandle		headID;															
	PxcBpHandle		group;
	PxU8			pad2[2];
	void*			userData;

	void			reset()
	{
		selfCollide	= true;
		nbElems		= 0;
		nbActive	= 0;
		bpElemId	= PX_INVALID_BP_HANDLE;
		headID		= PX_INVALID_BP_HANDLE;
		group		= PX_INVALID_BP_HANDLE;
		userData	= NULL;
	}

	void			init()
	{
		PxMemZero(selfCollBitMapWords, MAX_COMPOUND_BITMAP_SIZE);
		selfCollide	= true;
		nbElems		= 0;
		nbActive	= 0;
		bpElemId	= PX_INVALID_BP_HANDLE;
		headID		= PX_INVALID_BP_HANDLE;
		group		= PX_INVALID_BP_HANDLE;
		userData	= NULL;
	}
};
#if defined(PX_PS3) || defined(PX_X360) 
PX_COMPILE_TIME_ASSERT(0==(sizeof(Compound) & 0x0f));
#endif


class CompoundFreeElemsWords
{
public:

	CompoundFreeElemsWords()
	{
		clearWords();
	}
	~CompoundFreeElemsWords(){}

	void clearWords()
	{
		for(PxU32 i=0;i<(MAX_COMPOUND_BOUND_SIZE>>5);i++)
		{
			mWords[i]=0;
		}
	}

	bool hasNonZeroWords() const
	{
		for(PxU32 i=0;i<(MAX_COMPOUND_BOUND_SIZE>>5);i++)
		{
			if(mWords[i]!=0) return true;
		}
		return false;
	}

	PxU32* getWords() {return mWords;}
	PxU32 getWordCount() {return MAX_COMPOUND_BOUND_SIZE>>5;}

private:

	PxU32 mWords[MAX_COMPOUND_BOUND_SIZE>>5];
};

// cache for compound AABB collisions
class CompoundCache
{
public:
	struct SortedData
	{
		PxcBPValType	bounds;		
		PxU16		elemIndex;

		PX_FORCE_INLINE bool operator < (const SortedData& data) const
		{
			return bounds < data.bounds;
		}		
	};

	struct ElementData
	{
		PxU32 elemId;
		PxU16 elemIndex;
		PxcBPValType bounds[6];
	};

	struct CompoundData 
	{
		Compound*	compound;
		PxU32		elemId;
		PxU32		numElements;
		PxU32		numValidElements;
		ElementData*	elemData;
		PxU32*			ranks;
		bool		ownMemory;
	};

	typedef Ps::HashMap<PxU32, CompoundData*>	CompoundCacheMap;
	typedef Ps::Pair<const PxU32,CompoundData*>	CompoundCacheEntry;
	typedef Ps::Array<PxU16>					CompoundPairsArray;

	CompoundCache(PxcScratchAllocator& allocator);
	~CompoundCache();

	CompoundData*		getCompoundData(const PxU32& key, const PxU32 nbElements); // finds compound data in cache or creates new and inserts	
	PX_FORCE_INLINE CompoundPairsArray&	getPairsArray() { return mPairsArray; }
	PX_FORCE_INLINE SortedData*	getSortedData()  
	{ 
		if(!mSortedData)
		{
			mSortedData = static_cast<SortedData*> (mAllocator.alloc(sizeof(SortedData)*256,true));
		}
		return mSortedData; 
	}

	PX_FORCE_INLINE PxU32* getBitmapMemory() 
	{
		if(!mBitmapMemory)
		{
			mBitmapMemory = static_cast<PxU32*> (mAllocator.alloc(sizeof(PxU32)*2048,true));
		}
		return mBitmapMemory;
	}


	void				prepare();	// preallocate compounddata
	void				flush();  // each frame flush the data

private:
	PxcScratchAllocator&			mAllocator;
	PxU32							mCurrentIndex;
	PxU32							mMaxSize;
	CompoundData*					mData;

	CompoundCacheMap				mCacheMap;
	CompoundPairsArray				mPairsArray;
	SortedData*						mSortedData;
	PxU32*							mBitmapMemory;
};

class CompoundManager 
{
private:

	//Buffer that stores mCompounds, mFreeIds, and mFreeCompoundGroups
	PxU8*					mBuffer;

	//Array of compounds.
	Compound*				mCompounds;
	CompoundFreeElemsWords*	mCompoundReleasedElemsWords;
	CompoundFreeElemsWords*	mCompoundAvailableElemsWords;
	PxU32					mCompoundsSize;
	PxU32					mCompoundsCapacity;

	//Free compound ids.
	PxcBpHandle*				mFreeIDs;	
	PxU32					mFreeIDsSize;

	//Free compound group ids.
	PxcBpHandle				mCompoundGroupTide;
	PxcBpHandle*				mFreeCompoundGroups;
	PxU32					mFreeCompoundGroupsSize;

	PX_FORCE_INLINE void reuseCompoundGroup(const PxcBpHandle group) 
	{
		PX_ASSERT(group != PX_INVALID_BP_HANDLE);
		PX_ASSERT(mFreeCompoundGroupsSize < mCompoundsCapacity);
		mFreeCompoundGroups[mFreeCompoundGroupsSize]=group;
		mFreeCompoundGroupsSize++;
	}

	PX_FORCE_INLINE PxcBpHandle	getFreeCompoundGroup()
	{
		PxcBpHandle group=PX_INVALID_BP_HANDLE;
		if(mFreeCompoundGroupsSize)
		{
			group = mFreeCompoundGroups[mFreeCompoundGroupsSize-1];
			mFreeCompoundGroupsSize--;
		}
		else
		{
			group=mCompoundGroupTide;
			mCompoundGroupTide--;
		}
		PX_ASSERT(group!=PX_INVALID_BP_HANDLE);
		return group;
	}

	void resize()
	{
		PX_ASSERT(mCompoundsSize == mCompoundsCapacity);
		const PxU32 oldCompoundsCapacity=mCompoundsCapacity;
		const PxU32 newCompoundsCapacity=mCompoundsCapacity+32;

		//Allocate a single buffer for everything we need (Compounds + free ids + group ids)
		const PxU32 sizeCompounds = ALIGN_SIZE_16(newCompoundsCapacity*sizeof(Compound));
		const PxU32 sizeHandles = ALIGN_SIZE_16(newCompoundsCapacity*sizeof(PxcBpHandle));
		const PxU32 sizeFreeElems = ALIGN_SIZE_16(newCompoundsCapacity*sizeof(CompoundFreeElemsWords));
		PxU8* newBuffer = (PxU8*)PX_ALLOC(sizeCompounds + sizeHandles + sizeHandles + sizeFreeElems + sizeFreeElems, PX_DEBUG_EXP("AABBManager"));

		//compounds.
		Compound* newCompounds = (Compound*)(newBuffer + 0);
		if(mCompounds)
		{
			PxMemCopy(newCompounds, mCompounds, sizeof(Compound)*oldCompoundsCapacity);
		}
		for(PxU32 i=oldCompoundsCapacity;i<newCompoundsCapacity;i++)
		{
			newCompounds[i].init();
		}
		mCompounds=newCompounds;

		//freeIds
		PxcBpHandle* newFreeIds = (PxcBpHandle*)(newBuffer + sizeCompounds);
		copyPodArray(newFreeIds, mFreeIDs, newCompoundsCapacity, oldCompoundsCapacity, sizeof(PxcBpHandle));
		mFreeIDs=newFreeIds;

		//freeCompoundGroups
		PxcBpHandle* newFreeCompoundGroups = (PxcBpHandle*)(newBuffer + sizeCompounds + sizeHandles);
		copyPodArray(newFreeCompoundGroups, mFreeCompoundGroups, newCompoundsCapacity, oldCompoundsCapacity, sizeof(PxcBpHandle));
		mFreeCompoundGroups=newFreeCompoundGroups;

		//released elems
		CompoundFreeElemsWords* newReleasedElems = (CompoundFreeElemsWords*)(newBuffer + sizeCompounds + sizeHandles + sizeHandles);
		copyPodArray(newReleasedElems, mCompoundReleasedElemsWords, newCompoundsCapacity, oldCompoundsCapacity, sizeof(CompoundFreeElemsWords));
		mCompoundReleasedElemsWords=newReleasedElems;

		//available elems
		CompoundFreeElemsWords* newAvailableElems = (CompoundFreeElemsWords*)(newBuffer + sizeCompounds + sizeHandles + sizeHandles + sizeFreeElems);
		copyPodArray(newAvailableElems, mCompoundAvailableElemsWords, newCompoundsCapacity, oldCompoundsCapacity, sizeof(CompoundFreeElemsWords));
		mCompoundAvailableElemsWords=newAvailableElems;

		//Set the new capacity.
		mCompoundsCapacity = newCompoundsCapacity;

		//Free the old buffer.
		PX_FREE(mBuffer);
		mBuffer=newBuffer;
	}

public:

	CompoundManager() :
		mBuffer						(NULL),
		mCompounds					(NULL),
		mCompoundReleasedElemsWords	(NULL),
		mCompoundAvailableElemsWords(NULL),
		mCompoundsSize				(0),
		mCompoundsCapacity			(0),
		mFreeIDs					(NULL),
		mFreeIDsSize				(0),
		mCompoundGroupTide			(PX_INVALID_BP_HANDLE-1),
		mFreeCompoundGroups			(NULL),
		mFreeCompoundGroupsSize		(0)
	{
	}

	~CompoundManager()
	{
		PX_FREE(mBuffer);
	}

	PX_FORCE_INLINE PxU32 getCompoundsCapacity() const 
	{
		return mCompoundsCapacity;
	}

	PX_FORCE_INLINE Compound* getCompound(const PxU32 compoundId) const
	{
		PX_ASSERT(compoundId < mCompoundsCapacity);
		return &mCompounds[compoundId];
	}

	PxcBpHandle createCompound(void* userData, const bool selfCollisions)
	{
		PxU32 compoundId = 0xffffffff;
		Compound* compound = NULL;

		if(mFreeIDsSize)
		{
			//There is a compound that can be reused because it has been used and then released.
			PX_ASSERT(mFreeIDsSize < mCompoundsCapacity);
			compoundId = mFreeIDs[mFreeIDsSize-1];
			PX_ASSERT(compoundId<mCompoundsCapacity);
			mFreeIDsSize--;
			compound = &mCompounds[compoundId];
		}
		else if(mCompoundsSize < mCompoundsCapacity)
		{
			//No compounds to be reused but we have at least one spare unused compound to satisfy this request.
			compoundId = mCompoundsSize;
			compound = &mCompounds[compoundId];
			mCompoundsSize++;
		}
		else
		{
			//No compounds to be reused and no spare unused compounds for this request.
			//Need to resize the array of compounds to make a spare unused compound.
			resize();

			//Now we've got a s spare compound to satisfy this request.
			compoundId = mCompoundsSize;
			compound = &mCompounds[compoundId];
			mCompoundsSize++;
		}

		PX_ASSERT(compoundId < mCompoundsCapacity);
		PX_ASSERT(0xffffffff != compoundId);
		PX_ASSERT(compound);
		PX_ASSERT(0==compound->nbElems);
		PX_ASSERT(0==compound->nbActive);
		PX_ASSERT(PX_INVALID_BP_HANDLE==compound->bpElemId);
		PX_ASSERT(PX_INVALID_BP_HANDLE==compound->headID);
		PX_ASSERT(PX_INVALID_BP_HANDLE==compound->group);
		PX_ASSERT(!mCompoundReleasedElemsWords[compoundId].hasNonZeroWords());
		PX_ASSERT(!mCompoundAvailableElemsWords[compoundId].hasNonZeroWords());

		//Set the compound self-collide flags.
		compound->selfCollide = selfCollisions;

		compound->userData = userData;

		//Get an unused group id for the compound.
		const PxcBpHandle groupId = getFreeCompoundGroup();
		compound->group = groupId;

		return (PxcBpHandle)compoundId;
	}

	PX_FORCE_INLINE void clearCompound(const PxU32 id)
	{
		PX_ASSERT(id < mCompoundsCapacity);
		Compound* compound=getCompound(id);
		PX_ASSERT(0==compound->nbActive);

		//Reset the compound data.
		//reset() resets everything but we don't want to clear the group.
		//just store them temporarily, reset the compound, then reset the group.
		//Note: the group is set in createCompound and is only freed in reuseCompound.
		const PxcBpHandle group=compound->group;
		compound->reset();
		compound->group=group;

		mCompoundReleasedElemsWords[id].clearWords();
		mCompoundAvailableElemsWords[id].clearWords();
	}

	PX_FORCE_INLINE void reuseCompound(const PxU32 id)
	{
		//Reuse the compound id.
		PX_ASSERT(id<mCompoundsCapacity);
		PX_ASSERT(!mCompounds[id].nbActive);
		PX_ASSERT(mFreeIDsSize<mCompoundsCapacity);
		mFreeIDs[mFreeIDsSize]=(PxcBpHandle)id;
		mFreeIDsSize++;

		//Reuse the group id.
		PX_ASSERT(id<mCompoundsCapacity);
		PX_ASSERT(PX_INVALID_BP_HANDLE!=mCompounds[id].group);
		const PxcBpHandle group=mCompounds[id].group;
		reuseCompoundGroup(group);

		//Invalidate the bp and group.
		mCompounds[id].group=PX_INVALID_BP_HANDLE;

		//Clear the words describing the internal elems that can be reused by the compound.
		mCompoundReleasedElemsWords[id].clearWords();
		mCompoundAvailableElemsWords[id].clearWords();
	}

	PX_FORCE_INLINE void releaseElem(const PxU32 id, const PxcBpHandle index)
	{
		PX_ASSERT(id<mCompoundsCapacity);
		Cm::BitMap bitmap;
		bitmap.setWords(mCompoundReleasedElemsWords[id].getWords(), mCompoundReleasedElemsWords[id].getWordCount());
		bitmap.set(index);
	}

	void markReleasedCompoundElemsAsAvailable()
	{
		for(PxU32 i=0;i<mCompoundsCapacity;i++)
		{
			if(mCompoundReleasedElemsWords[i].hasNonZeroWords())
			{
				Cm::BitMap bitmap;
				bitmap.setWords(mCompoundReleasedElemsWords[i].getWords(), mCompoundReleasedElemsWords[i].getWordCount());
				Cm::BitMap bitmap2;
				bitmap2.setWords(mCompoundAvailableElemsWords[i].getWords(),  mCompoundReleasedElemsWords[i].getWordCount());
				bitmap2.combineInPlace<Cm::BitMap::OR>(bitmap);
				mCompoundReleasedElemsWords[i].clearWords();
			}
		}
	}

	PX_FORCE_INLINE PxcBpHandle getAvailableElem(const PxU32 id)
	{
		PX_ASSERT(id<mCompoundsCapacity);
		if(mCompoundAvailableElemsWords[id].hasNonZeroWords())
		{
			Cm::BitMap bitmap;
			bitmap.setWords(mCompoundAvailableElemsWords[id].getWords(),  mCompoundReleasedElemsWords[id].getWordCount());
			PxU32 last = bitmap.findLast();
			bitmap.reset(last);
			return (PxcBpHandle)last;
		}
		else
		{
			return PX_INVALID_BP_HANDLE;
		}
	}
};


struct Single
{
	PxcBpHandle		headID;

	void reset() {headID = PX_INVALID_BP_HANDLE;}
};

class SingleManager 
{
private:

	//Single allocation buffer for singles and freeIds.		
	PxU8*					mBuffer;

	//Array of singles.
	Single*					mSingles;
	PxU32					mSinglesSize;
	PxU32					mSinglesCapacity;

	//Free single ids.
	PxcBpHandle*				mFreeIDs;	
	PxU32					mFreeIDsSize;

public:

	SingleManager()
		: mBuffer(NULL),
	      mSingles(NULL),
		  mSinglesSize(0),
	 	  mSinglesCapacity(0),
		  mFreeIDs(NULL),
		  mFreeIDsSize(0)
	{
	}

	~SingleManager()
	{
		PX_FREE(mBuffer);
	}

	PX_FORCE_INLINE PxU32 getCapacity() const 
	{
		return mSinglesCapacity;
	}

	PX_FORCE_INLINE Single* getSingle(const PxU32 singleId)
	{
		PX_ASSERT(singleId < mSinglesCapacity);
		return &mSingles[singleId];
	}

	void resize(PxU32 newCapacity)
	{
		PX_ASSERT(newCapacity > mSinglesCapacity);

		const PxU32 oldCapacity=mSinglesCapacity;

		const PxU32 sizeSingles = ALIGN_SIZE_16(newCapacity*sizeof(Single));
		const PxU32 sizeHandles = ALIGN_SIZE_16(newCapacity*sizeof(PxcBpHandle));
 		PxU8* newBuffer=(PxU8*)PX_ALLOC(sizeSingles + sizeHandles, PX_DEBUG_EXP("AABBManager"));

		Single* newSingles=(Single*)(newBuffer + 0);
		copyPodArray(newSingles, mSingles, newCapacity, oldCapacity, sizeof(Single));
		
		for(PxU32 i=oldCapacity;i<newCapacity;i++)  // mainly for origin shift but is safer in general anyway
		{
			newSingles[i].reset();
		}
		mSingles=newSingles;

		PxcBpHandle* newFreeIds=(PxcBpHandle*)(newBuffer + sizeSingles);
		copyPodArray(newFreeIds, mFreeIDs, newCapacity, oldCapacity, sizeof(PxcBpHandle));
		mFreeIDs=newFreeIds;

		mSinglesCapacity = newCapacity;

		PX_FREE(mBuffer);
		mBuffer=newBuffer;
	}


	PxcBpHandle createSingle()
	{
		PxU32 singleId=0xffffffff;
		Single* single=NULL;

		if(mFreeIDsSize)
		{
			//There is a compound that can be reused because it has been used and then released.
			PX_ASSERT(mFreeIDsSize <= mSinglesCapacity);
			singleId = mFreeIDs[mFreeIDsSize-1];
			PX_ASSERT(singleId<mSinglesCapacity);
			mFreeIDsSize--;
			single = &mSingles[singleId];
		}
		else if(mSinglesSize < mSinglesCapacity)
		{
			//No compounds to be reused but we have at least one spare unused compound to satisfy this request.
			singleId = mSinglesSize;
			single = &mSingles[singleId];
			mSinglesSize++;
		}
		else
		{
			//No compounds to be reused and no spare unused compounds for this request.
			//Need to resize the array of compounds to make a spare unused compound.
			resize(mSinglesCapacity+32);

			//Now we've got a s spare compound to satisfy this request.
			singleId = mSinglesSize;
			single = &mSingles[singleId];
			mSinglesSize++;
		}

		PX_ASSERT(singleId < mSinglesCapacity);
		PX_ASSERT(0xffffffff != singleId);
		PX_ASSERT(single);
		PX_UNUSED(single);

		return (PxcBpHandle)singleId;
	}

	PX_FORCE_INLINE void clearSingle(const PxU32 id)
	{
		PX_ASSERT(id < mSinglesCapacity);
		Single* single=getSingle(id);
		single->reset();
	}

	PX_FORCE_INLINE void reuseSingle(const PxU32 id)
	{
		//Reuse the compound id.
		PX_ASSERT(id<mSinglesCapacity);
		PX_ASSERT(mFreeIDsSize<mSinglesCapacity);
		mFreeIDs[mFreeIDsSize]=(PxcBpHandle)id;
		mFreeIDsSize++;
	}

};

///////////////////////////////////////////////////////////////////////////////

struct PxsShapeCore;
class PxcRigidBody;
struct PxsRigidCore;

struct PxcAABBDataStatic
{
	void setEmpty()
	{
		mShapeCore = NULL;
		mRigidCore = NULL;
	}
	const PxsShapeCore* mShapeCore;
	const PxsRigidCore* mRigidCore;
};

struct PxcAABBDataDynamic
{
	void setEmpty()
	{
		mShapeCore = NULL;
		mBodyAtom = NULL;
		mRigidCore = NULL;
		mLocalSpaceAABB = NULL;
	}
	const PxsShapeCore* mShapeCore;
	const PxcRigidBody* mBodyAtom;
	const PxsRigidCore* mRigidCore;
	const PxBounds3* mLocalSpaceAABB;
};


struct PxsComputeAABBParams	//these things that are needed for swept bounds computation are normally found in the dynamics context, but we don't get to access that from SPU code.
{
	PxReal dt;
	PxI32* numFastMovingShapes;
};

void updateBodyShapeAABBs
(const PxcBpHandle* PX_RESTRICT updatedAABBHandles, const PxU32 numUPdatedAABBHandles, 
 const PxcBpHandle* PX_RESTRICT aabbDataHandles, const PxcAABBDataStatic* PX_RESTRICT aabbData, 
 const PxsComputeAABBParams& params, const bool secondBroadPhase,
 IntegerAABB* PX_RESTRICT boundsIAABB, const PxU32 maxNumBounds);

PxU32 updateBodyShapeAABBs
(const PxcBpHandle* PX_RESTRICT updatedAABBHandles, const PxU32 numUPdatedAABBHandles, 
 const PxcBpHandle* PX_RESTRICT aabbDataHandles, const PxcAABBDataDynamic* PX_RESTRICT aabbData, 
 const PxsComputeAABBParams& params, const bool secondBroadPhase,
 IntegerAABB* PX_RESTRICT boundsIAABB, const PxU32 maxNumBounds);

//Returns 1 if this is a fast-moving object
PxF32 PxsComputeAABB
(const PxsComputeAABBParams& params, const bool secondBroadphase,
 const PxcAABBDataStatic& aabbDataStatic, 
 PxBounds3& updatedBodyShapeBounds);

PxF32 PxsComputeAABB
(const PxsComputeAABBParams& params, const bool secondBroadphase,
 const PxcAABBDataDynamic& aabbDataDynamic, 
 PxBounds3& updatedBodyShapeBounds);

///////////////////////////////////////////////////////////////////////////////

struct CompoundPair 
{
	PxcBpHandle		mBPElemId0;
	PxcBpHandle		mBPElemId1;
	Cm::BitMap*		compoundCollBitmap;
};

///////////////////////////////////////////////////////////////////////////////

class UpdatedCreatedRemovedList
{
private:

public:

	PxU32* mBitmapWords;
	PxU32 mBitmapWordCount;
	PxcBpHandle* mElems;
	PxU32 mElemsSize;
	PxU32 mElemsCapacity;
	PxU32 mDefaultElemsCapacity;

public:

	PX_FORCE_INLINE const PxcBpHandle* getElems() const {return mElems;}
	PX_FORCE_INLINE PxU32 getElemsSize() const {return mElemsSize;}

	UpdatedCreatedRemovedList()
		: mBitmapWords(NULL),
		  mBitmapWordCount(0),
		  mElems(NULL),
		  mElemsSize(0),
		  mElemsCapacity(0),
		  mDefaultElemsCapacity(64)
	{
	}

	~UpdatedCreatedRemovedList()
	{
		PX_FREE(mBitmapWords);
		PX_FREE(mElems);
	}

private:

	void growElems(const PxU32 _newCapacity)
	{
		PX_ASSERT(_newCapacity>mElemsCapacity);
		const PxU32 oldCapacity=mElemsCapacity;
		const PxU32 newCapacity = ((_newCapacity + 31) & ~31);
		mElems=(PxcBpHandle*)resizePODArray(oldCapacity,newCapacity,sizeof(PxcBpHandle),mElems);
		mElemsCapacity=newCapacity;
	}

public:

	void setDefaultElemsCapacity(const PxU32 defaultCapacity)
	{
		mDefaultElemsCapacity=PxMax(mDefaultElemsCapacity,defaultCapacity);
		if(mDefaultElemsCapacity>mElemsCapacity)
		{
			growElems(mDefaultElemsCapacity);
		}
	}

	void free()
	{
		if(mElemsCapacity>mDefaultElemsCapacity)
		{
			PX_FREE(mElems);
			mElems=(PxcBpHandle*)PX_ALLOC(sizeof(PxcBpHandle)*mDefaultElemsCapacity, PX_DEBUG_EXP("AABBManager"));
			mElemsCapacity=mDefaultElemsCapacity;
		}
		mElemsSize=0;

		if(mBitmapWords)
		{
			PX_ASSERT(mBitmapWordCount);
			PxMemZero(mBitmapWords, sizeof(PxU32)*mBitmapWordCount);
		}
	}

	PX_FORCE_INLINE void addElem(const PxcBpHandle id)
	{
		PX_ASSERT(!isInList(id));
		PxU32 mask = 1<<(id&31);
		PxU32& word = mBitmapWords[id>>5];
		word |= mask;
	}

	PX_FORCE_INLINE void removeElem(const PxcBpHandle id)
	{
		PX_ASSERT(isInList(id));
		PxU32 mask = 1<<(id&31);
		PxU32& word = mBitmapWords[id>>5];
		word &= ~mask;
	}

	PX_FORCE_INLINE Ps::IntBool isInList(const PxcBpHandle id) const
	{
		PX_ASSERT(mBitmapWords && mBitmapWordCount);
		PX_ASSERT((id >> 5) < (PxcBpHandle)mBitmapWordCount); 
		PxU32 mask = 1<<(id&31);
		PxU32& word = mBitmapWords[id>>5];
		return word&mask;
	}

	void growBitmap(const PxU32 maxNumElems)
	{
		const PxU32 newbitmapWordCount = ((maxNumElems >> 5) + 3) & (~3);
		PX_ASSERT(newbitmapWordCount>0);
		if(newbitmapWordCount > mBitmapWordCount)
		{
			mBitmapWords=(PxU32*)resizePODArray(mBitmapWordCount, newbitmapWordCount, sizeof(PxU32), mBitmapWords);
			mBitmapWordCount = newbitmapWordCount;
		}
	}

	void computeList()
	{
		if(NULL==mBitmapWords)
		{
			PX_ASSERT(0==mBitmapWordCount);
			return;
		}

		Cm::BitMap bm;
		bm.setWords(mBitmapWords,mBitmapWordCount);
		const PxU32* PX_RESTRICT bitmapWords=mBitmapWords;
		const PxU32 lastSetBit = bm.findLast();

		//First work out the size required.
		PxU32 numElems=0;
		for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
		{
			for(PxU32 b = bitmapWords[w]; b; b &= b-1)
			{
				numElems++;
			}
		}
		numElems++;

		//Grow to the correct size.
		if(numElems>mElemsCapacity)
		{
			growElems(numElems);
		}

		//Compute the list.
		numElems=0;
		for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
		{
			for(PxU32 b = bitmapWords[w]; b; b &= b-1)
			{
				const PxU32 index = (w<<5|Ps::lowestSetBit(b));
				PX_ASSERT(numElems<mElemsCapacity);
				mElems[numElems]=(PxcBpHandle)index;
				numElems++;
			}
		}
		mElemsSize=numElems;
	}
};

template <class T> class AABBDataManager
{
public:

	AABBDataManager()
		: mData(0),
		  mCapacity(0),
		  mFirstFreeElem(PX_INVALID_BP_HANDLE)
	{
	}

	~AABBDataManager()
	{
		PX_FREE(mData);
	}

	PX_FORCE_INLINE const T& get(const PxcBpHandle id) const {PX_ASSERT(id<mCapacity); return mData[id];}
	PX_FORCE_INLINE void set(const PxcBpHandle id, const T& aabbData) {PX_ASSERT(id<mCapacity); mData[id]=aabbData;}

	PX_FORCE_INLINE PxU32 getCapacity() const {return mCapacity;}

	PX_FORCE_INLINE PxcBpHandle getAvailableElem()
	{
		if(PX_INVALID_BP_HANDLE==mFirstFreeElem)
		{
			grow(PxMax(2*mCapacity, (PxU32)64));
		}

		const PxcBpHandle firstFreeElem=(PxcBpHandle)mFirstFreeElem;
		mFirstFreeElem=*(PxcBpHandle*)&mData[firstFreeElem];
		mData[firstFreeElem].setEmpty();
		return firstFreeElem;
	}

	PX_FORCE_INLINE void freeElem(const PxcBpHandle id) 
	{
		PX_ASSERT(id<mCapacity);
		(*(PxcBpHandle*)&mData[id])=(PxcBpHandle)mFirstFreeElem;
		mFirstFreeElem=id;
	}

	PX_FORCE_INLINE const T* getArray() const 
	{
		return mData;
	}

	PX_FORCE_INLINE void setDefaultCapacity(const PxU32 newCapacity)
	{
		if(mCapacity < newCapacity)
			grow(newCapacity);
	}

private:

	PxU32 countNumFreeElems() const
	{
		if(!mData)
		{
			PX_ASSERT(0==mCapacity);
			return 0;
		}

		PxU32 count=0;
		PxU32 freeElem=mFirstFreeElem;
		while(freeElem!=PX_INVALID_BP_HANDLE)
		{
			count++;
			freeElem=*(PxcBpHandle*)&mData[freeElem];
		}
		return count;
	}

	void grow(const PxU32 newCapacity)
	{
#ifndef NDEBUG
		const PxU32 numFreeElemsStart=countNumFreeElems();
#endif
		const PxU32 oldCapacity=mCapacity;

		T* newData=(T*)PX_ALLOC(sizeof(T)*newCapacity, );

		if(mData)copyPodArray(newData,mData,newCapacity,oldCapacity,sizeof(T));

		*(PxcBpHandle*)&newData[newCapacity-1]=(PxcBpHandle)mFirstFreeElem;
		for(PxU32 i=oldCapacity;i<(newCapacity-1);i++)
		{
			*(PxcBpHandle*)&newData[i] = (PxcBpHandle)(i+1);
		}
		mFirstFreeElem=mCapacity;

		PX_FREE(mData);
		mData=newData;

#ifndef NDEBUG
		const PxU32 numFreeElemsEnd=countNumFreeElems();
		PX_ASSERT((numFreeElemsStart + newCapacity-mCapacity) == numFreeElemsEnd);
#endif

		mCapacity=newCapacity;
	}

	T* mData;
	PxU32 mCapacity;
	PxU32 mFirstFreeElem;
};

///////////////////////////////////////////////////////////////////////////////

class Elems
{
protected:

	PxU8* mBuffer;
	IntegerAABB* mBounds;
	void** mUserDatas;
	PxcBpHandle* mGroups;
	PxcBpHandle* mOwnerIds; //id of compound array if it is a compound, id of single array if it is a single
	PxcBpHandle* mElemNextIds;	//definitely need this
	PxcBpHandle* mAABBDataHandles;
	PxU32 mCapacity;
	PxU32 mFirstFreeElem;

	AABBDataManager<PxcAABBDataStatic> mStaticAABBDataManager;
	AABBDataManager<PxcAABBDataDynamic> mDynamicAABBDataManager;

public:

	PX_FORCE_INLINE PxcBpHandle getFirstFreeElem() const { return (PxcBpHandle)mFirstFreeElem; }

	PX_FORCE_INLINE void freeElem(const PxcBpHandle id) 
	{
		const PxcBpHandle aabbDataHandle=mAABBDataHandles[id];
		if(PX_INVALID_BP_HANDLE!=aabbDataHandle)
		{
			const PxcBpHandle group=mGroups[id];
			if(0==group)
				mStaticAABBDataManager.freeElem(aabbDataHandle);
			else
				mDynamicAABBDataManager.freeElem(aabbDataHandle);
		}
		resetElem(id);

		PX_ASSERT(id < mCapacity);
		mGroups[id] = (PxcBpHandle)mFirstFreeElem;
		mFirstFreeElem = id;
	}

	PX_FORCE_INLINE PxcBpHandle useFirstFreeElem()
	{
		PX_ASSERT(PX_INVALID_BP_HANDLE != mFirstFreeElem);
		const PxU32 elemId = mFirstFreeElem;
		mFirstFreeElem = mGroups[mFirstFreeElem];
		resetElem(elemId);
		return (PxcBpHandle)elemId;
	}

	void setDefaultCapacity(const PxU32 nbStatics, const PxU32 nbDynamics)
	{
		if(nbStatics+nbDynamics > mCapacity)
		{
			grow(nbStatics+nbDynamics);
		}
		mStaticAABBDataManager.setDefaultCapacity(nbStatics);
		mDynamicAABBDataManager.setDefaultCapacity(nbDynamics);
	}

	void grow(const PxU32 newCapacity)
	{
		PX_ASSERT(newCapacity > mCapacity);
		PX_ASSERT(0==(newCapacity & 31));

#ifndef NDEBUG
		const PxU32 numFreeElemsStart=countNumFreeelems();
#endif

		const PxU32 sizeBounds = ALIGN_SIZE_16(newCapacity*sizeof(IntegerAABB)); 
		const PxU32 sizeVoidStars = ALIGN_SIZE_16(newCapacity*sizeof(void*));
		const PxU32 sizeHandles = ALIGN_SIZE_16(newCapacity*sizeof(PxcBpHandle));
		const PxU32 sizeAABBDataHandles = ALIGN_SIZE_16(newCapacity*sizeof(PxcBpHandle));
		const PxU32 newByteSize = sizeBounds + sizeVoidStars + sizeHandles +  sizeHandles +  sizeAABBDataHandles + sizeHandles;

		PxU8* newBuffer = (PxU8*)PX_ALLOC(newByteSize, PX_DEBUG_EXP("AABBManager"));

		IntegerAABB* newBounds=(IntegerAABB*)(newBuffer + 0); 
		copyPodArray(newBounds, mBounds, newCapacity, mCapacity, sizeof(IntegerAABB));
		mBounds=newBounds;

		void** newUserDatas=(void**)(newBuffer + sizeBounds);
		copyPodArray(newUserDatas, mUserDatas, newCapacity, mCapacity, sizeof(void*));
		mUserDatas=newUserDatas;

		PxcBpHandle* newGroups=(PxcBpHandle*)(newBuffer + sizeBounds + sizeVoidStars);
		copyPodArray(newGroups, mGroups, newCapacity, mCapacity, sizeof(PxcBpHandle));
		mGroups=newGroups;

		PxcBpHandle* newOwnerIds=(PxcBpHandle*)(newBuffer + sizeBounds + sizeVoidStars + sizeHandles);
		copyPodArray(newOwnerIds, mOwnerIds, newCapacity, mCapacity, sizeof(PxcBpHandle));
		mOwnerIds=newOwnerIds;

		PxcBpHandle* newAABBDataHandles=(PxcBpHandle*)(newBuffer + sizeBounds + sizeVoidStars + sizeHandles + sizeHandles);
		copyPodArray(newAABBDataHandles, mAABBDataHandles, newCapacity, mCapacity, sizeof(PxcBpHandle));
		mAABBDataHandles=newAABBDataHandles;

		PxcBpHandle* newElemNextIds=(PxcBpHandle*)(newBuffer + sizeBounds + sizeVoidStars + sizeHandles + sizeHandles + sizeAABBDataHandles);
		copyPodArray(newElemNextIds, mElemNextIds, newCapacity, mCapacity, sizeof(PxcBpHandle));
		mElemNextIds=newElemNextIds;

		//Set up the free elements.
		mGroups[newCapacity-1]=(PxcBpHandle)mFirstFreeElem;
		for(PxU32 i=mCapacity;i<(newCapacity-1);i++)
		{
			mGroups[i]=(PxcBpHandle)(i+1);
		}
		mFirstFreeElem=mCapacity;

#ifndef NDEBUG
		const PxU32 numFreeElemsEnd=countNumFreeelems();
		PX_ASSERT(numFreeElemsStart + (newCapacity-mCapacity) == numFreeElemsEnd);
#endif

		PX_FREE(mBuffer);
		mBuffer=newBuffer;
		mCapacity=newCapacity;

	}

	PX_FORCE_INLINE void setAABB(const PxU32 bpElemId, const IntegerAABB& aabb){PX_ASSERT(bpElemId<mCapacity);	mBounds[bpElemId] = aabb;}
	PX_FORCE_INLINE void setUserData(const PxU32 id, void* userData) {PX_ASSERT(id < mCapacity); mUserDatas[id] = userData;}
	PX_FORCE_INLINE void setGroup(const PxU32 id, const PxcBpHandle group) { PX_ASSERT(id < mCapacity); mGroups[id] = group; }
	PX_FORCE_INLINE void setCompoundOwnerId(const PxU32 id, const PxcBpHandle compoundOwnerId) { PX_ASSERT(id < mCapacity); PX_ASSERT(compoundOwnerId != PX_INVALID_BP_HANDLE); mOwnerIds[id] = encodeCompoundForClient(compoundOwnerId); }
	PX_FORCE_INLINE void setSingleOwnerId(const PxU32 id, const PxcBpHandle singleOwnerId) { PX_ASSERT(id < mCapacity); PX_ASSERT(singleOwnerId != PX_INVALID_BP_HANDLE); mOwnerIds[id] = encodeSingleForClient(singleOwnerId); }
	PX_FORCE_INLINE void setNextId(const PxU32 id, const PxcBpHandle nextId) { PX_ASSERT(id < mCapacity); mElemNextIds[id] = nextId;}

	PX_FORCE_INLINE void* getUserData(const PxU32 id) const { PX_ASSERT(id < mCapacity); return mUserDatas[id]; }
	PX_FORCE_INLINE PxcBpHandle getGroup(const PxU32 id) const { PX_ASSERT(id < mCapacity); return mGroups[id]; }
	PX_FORCE_INLINE Ps::IntBool isOwnerCompound(const PxU32 id) const { PX_ASSERT(id < mCapacity); PX_ASSERT(mOwnerIds[id] != PX_INVALID_BP_HANDLE); return isClientVolumeCompound(mOwnerIds[id]);}
	PX_FORCE_INLINE PxcBpHandle getCompoundOwnerId(const PxU32 id) const { PX_ASSERT(id < mCapacity); PX_ASSERT(isOwnerCompound(id)); return decodeCompoundFromClient(mOwnerIds[id]); }
	PX_FORCE_INLINE PxcBpHandle getSingleOwnerId(const PxU32 id) const { PX_ASSERT(id < mCapacity); PX_ASSERT(!isOwnerCompound(id)); return decodeSingleFromClient(mOwnerIds[id]); }
	PX_FORCE_INLINE PxcBpHandle getNextId(const PxU32 id) const { PX_ASSERT(id < mCapacity); return mElemNextIds[id];}
	PX_FORCE_INLINE const IntegerAABB& getAABB(const PxU32 bpElemId) const{PX_ASSERT(bpElemId<mCapacity);return mBounds[bpElemId];}

	PX_FORCE_INLINE void setEmptyAABB(const PxU32 id) 
	{ 
		PX_ASSERT(id < mCapacity); 
		mAABBDataHandles[id]=PX_INVALID_BP_HANDLE;
	}

	PX_FORCE_INLINE const PxcBpHandle* getGroups() const { return mGroups; }
	PX_FORCE_INLINE const PxcBpHandle* getAABBDataHandles() const {return mAABBDataHandles;}
	PX_FORCE_INLINE IntegerAABB* getBounds() const {return mBounds;}
	PX_FORCE_INLINE PxU32 getCapacity() const { return mCapacity; }


	PX_FORCE_INLINE void setDynamicAABBData(const PxcBpHandle id, const PxcAABBDataDynamic& aabbData)
	{
		PX_ASSERT(id<mCapacity);
		PX_ASSERT(0!=mGroups[id]);
		const PxcBpHandle handle=mDynamicAABBDataManager.getAvailableElem();
		mDynamicAABBDataManager.set(handle,aabbData);
		mAABBDataHandles[id]=handle;
	}
	PX_FORCE_INLINE void setStaticAABBData(const PxcBpHandle id, const PxcAABBDataStatic& aabbData)
	{
		PX_ASSERT(id<mCapacity);
		PX_ASSERT(0==mGroups[id]);
		const PxcBpHandle handle=mStaticAABBDataManager.getAvailableElem();
		mStaticAABBDataManager.set(handle,aabbData);
		mAABBDataHandles[id]=handle;
	}
	PX_FORCE_INLINE const PxcRigidBody* getBodyAtom(const PxcBpHandle id) const
	{
		PX_ASSERT(id<mCapacity);
		return (0==mGroups[id] ? NULL : mDynamicAABBDataManager.get(mAABBDataHandles[id]).mBodyAtom); 
	}
	PX_FORCE_INLINE const PxsShapeCore* getShapeCore(const PxcBpHandle id) const
	{
		PX_ASSERT(id<mCapacity);
		return (0==mGroups[id] ? mStaticAABBDataManager.get(mAABBDataHandles[id]).mShapeCore : mDynamicAABBDataManager.get(mAABBDataHandles[id]).mShapeCore);
	}

	PX_FORCE_INLINE const PxcAABBDataStatic* getStaticAABBDataArray() const {return mStaticAABBDataManager.getArray();}
	PX_FORCE_INLINE PxU32 getStaticAABBDataArrayCapacity() const {return mStaticAABBDataManager.getCapacity();}
	PX_FORCE_INLINE const PxcAABBDataDynamic* getDynamicAABBDataArray() const {return mDynamicAABBDataManager.getArray();}
	PX_FORCE_INLINE PxU32 getDynamicAABBDataArrayCapacity() const {return mDynamicAABBDataManager.getCapacity();}

	PX_FORCE_INLINE const PxcAABBDataStatic& getStaticAABBData(const PxcBpHandle id) const
	{
		PX_ASSERT(id<mCapacity);
		PX_ASSERT(0==mGroups[id]);
		return mStaticAABBDataManager.get(mAABBDataHandles[id]);
	}
	PX_FORCE_INLINE const PxcAABBDataDynamic& getDynamicAABBData(const PxcBpHandle id) const
	{
		PX_ASSERT(id<mCapacity);
		PX_ASSERT(0!=mGroups[id]);
		PX_ASSERT(PX_INVALID_BP_HANDLE!=mAABBDataHandles[id]);
		return mDynamicAABBDataManager.get(mAABBDataHandles[id]);
	}


private:

	PX_FORCE_INLINE void resetElem(const PxU32 id)
	{
		PX_ASSERT(id<mCapacity);
		mBounds[id].setEmpty();
		mUserDatas[id] = NULL;
		mGroups[id] = PX_INVALID_BP_HANDLE;
		mOwnerIds[id] = PX_INVALID_BP_HANDLE;
		mAABBDataHandles[id] = PX_INVALID_BP_HANDLE;
		mElemNextIds[id] = PX_INVALID_BP_HANDLE;
	}

	PxU32 countNumFreeelems() const
	{
		if(!mGroups)
		{
			PX_ASSERT(0==mCapacity);
			return 0;
		}

		PxU32 count=0;
		PxU32 freeElem=mFirstFreeElem;
		while(freeElem!=PX_INVALID_BP_HANDLE)
		{
			count++;
			freeElem=mGroups[freeElem];
		}
		return count;
	}

protected:

	Elems()
		: mBuffer(NULL), 
		  mBounds(NULL),
		  mUserDatas(NULL),
		  mGroups(NULL),
		  mOwnerIds(NULL),
		  mElemNextIds(NULL),
		  mAABBDataHandles(NULL),
		  mCapacity(0),
		  mFirstFreeElem(PX_INVALID_BP_HANDLE)
	{
	}

	~Elems()
	{
	}

	void free()
	{
		PX_FREE(mBuffer);
	}
};

class BPElems : public Elems
{
public:

	BPElems() 
		: Elems()
	{
	}

	~BPElems()
	{
		free();
	}

	PX_FORCE_INLINE void initAsSingle(const PxU32 bpElemId, void* userData, const PxcBpHandle group, const IntegerAABB& aabb)
	{
		setUserData(bpElemId, userData);
		setGroup(bpElemId,group);
		setAABB(bpElemId,aabb);
		PX_ASSERT(bpElemId < mCapacity);
		PX_ASSERT(PX_INVALID_BP_HANDLE==mOwnerIds[bpElemId]);
	}

	PX_FORCE_INLINE void initAsCompound(const PxU32 bpElemId, const IntegerAABB& aabb)
	{
		setAABB(bpElemId,aabb);
		PX_ASSERT(NULL == getUserData(bpElemId));
		PX_ASSERT(PX_INVALID_BP_HANDLE != getGroup(bpElemId));
		PX_ASSERT(bpElemId < mCapacity);
		PX_ASSERT(PX_INVALID_BP_HANDLE!=mOwnerIds[bpElemId]);
		PX_ASSERT(isOwnerCompound(bpElemId));
	}

	PX_FORCE_INLINE bool isSingle(const PxcBpHandle id) const 
	{
		PX_ASSERT(id < mCapacity);
		PX_ASSERT( !isOwnerCompound(id) == (mUserDatas[id] ? true : false));
		return !isOwnerCompound(id);
	}

private:
};

class CompoundElems : public Elems
{
public:

	CompoundElems()
		: Elems()
	{
	}

	~CompoundElems()
	{
		free();
	}

	PX_FORCE_INLINE void reinit(const PxcBpHandle id, void* userData, const PxcBpHandle group, const IntegerAABB& aabb, const PxcBpHandle compoundOwnerId, const PxcBpHandle)
	{
		PX_ASSERT(id < getCapacity());
		setUserData(id,userData);
		setGroup(id, group);
		setAABB(id,aabb);
		setCompoundOwnerId(id,compoundOwnerId);
		mAABBDataHandles[id]=PX_INVALID_BP_HANDLE;
	}

	PX_FORCE_INLINE void init(const PxcBpHandle id, void* userData, const PxcBpHandle group, const IntegerAABB& aabb, const PxcBpHandle compoundOwnerId, const PxcBpHandle nextId)
	{
		PX_ASSERT(id < getCapacity());
		setUserData(id,userData);
		setGroup(id, group);
		setAABB(id,aabb);
		setCompoundOwnerId(id,compoundOwnerId);
		setNextId(id,nextId);
		mAABBDataHandles[id]=PX_INVALID_BP_HANDLE;
	}

	void resize(const PxU32 nb)
	{
		if(nb > mCapacity)
		{
			mFirstFreeElem = PX_INVALID_BP_HANDLE;
			grow(nb);
		}
	}

private:
};


///////////////////////////////////////////////////////////////////////////////

#ifndef __SPU__

class AABBUpdateTaskSPU;
class AABBUpdateTask;

class AABBUpdateWorkTask: public physx::PxLightCpuTask
{
public:

	enum
	{
		eMAX_NUM_TASKS=6
	};

	enum
	{
		eTASK_UNIT_WORK_SIZE=256
	};

	AABBUpdateWorkTask(Cm::EventProfiler& eventProfiler);
	virtual ~AABBUpdateWorkTask();

	void setAABBMgr(PxsAABBManager* AABBMgr) 
	{
		mAABBMgr = AABBMgr;
	}

	void set(const PxU32 numCpuTasks, const PxU32 numSpus, const PxsComputeAABBParams& params, const bool secondBroadPhase)
	{
		mNumCpuTasks = numCpuTasks;
		mNumSpus = numSpus;
		mParams = params;
		mSecondBroadPhase = secondBroadPhase;
	}

	virtual void run();

	virtual const char* getName() const { return "AABBUpdateWorkTask"; }

	void updateNumFastMovingShapes() const;

	void setNumFastMovingShapes(const PxU32 task, const PxU32 numFastMovingShapes);

private:

	AABBUpdateWorkTask& operator=(const AABBUpdateWorkTask&);

	Cm::EventProfiler& mEventProfiler;

	PxsAABBManager* mAABBMgr;
	PxU32 mNumCpuTasks;
	PxU32 mNumSpus;
	PxsComputeAABBParams mParams;
	bool mSecondBroadPhase;

	//Keep as references to avoid pulling in unnecessary headers
#ifdef PX_PS3
	AABBUpdateTaskSPU* mAABBUpdateTaskSPU;
#endif
	AABBUpdateTask* mAABBUpdateTask[eMAX_NUM_TASKS];

	static void computeTaskWork(const PxU32 numUpdatedAABBs, const PxU32 numTasks, PxU32* starts, PxU32* counts);
};

class AABBUpdateWorkEndTask: public physx::PxLightCpuTask
{
public:

	AABBUpdateWorkEndTask(Cm::EventProfiler& eventProfiler)
		: mEventProfiler(eventProfiler)
	{
	}

	virtual ~AABBUpdateWorkEndTask()
	{
	}

	void setAABBMgr(PxsAABBManager* AABBMgr) 
	{
		mAABBMgr = AABBMgr;
	}

	void set(const PxU32 numCpuTasks, const PxU32 numSpus)
	{
		mNumCpuTasks = numCpuTasks;
		mNumSpus = numSpus;
	}

	virtual void run();

	virtual const char* getName() const { return "AABBUpdateWorkEndTask"; }

private:
	AABBUpdateWorkEndTask& operator=(const AABBUpdateWorkEndTask&);
	Cm::EventProfiler& mEventProfiler;
	PxsAABBManager* mAABBMgr;
	PxU32 mNumCpuTasks;
	PxU32 mNumSpus;
};


class BPUpdateFinalizeTask: public physx::PxLightCpuTask
{
public:

	BPUpdateFinalizeTask(Cm::EventProfiler& eventProfiler)
		: mEventProfiler(eventProfiler)
	{
	}

	void setAABBMgr(PxsAABBManager* AABBMgr) 
	{
		mAABBMgr = AABBMgr;
	}

	void set(const PxU32 numCpuTasks, const PxU32 numSpusSap){ mNumCpuTasks=numCpuTasks; mNumSpusSap=numSpusSap; }

	virtual void run();

	virtual const char* getName() const { return "AABBUpdateFinalizeTask"; }

private:
	BPUpdateFinalizeTask& operator=(const BPUpdateFinalizeTask&);
	Cm::EventProfiler& mEventProfiler;

	PxsAABBManager* mAABBMgr;
	PxU32 mNumCpuTasks;
	PxU32 mNumSpusSap;
};

#endif //!__SPU__


} //namespace physx

#endif //PXS_COMPOUND_MANAGER_AUX_H
