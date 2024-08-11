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


#ifndef PXS_PARTICLE_CONTACT_MANAGER_STREAM_H
#define PXS_PARTICLE_CONTACT_MANAGER_STREAM_H

#include "CmMemFetch.h"

namespace physx
{

class PxvParticleShape;
struct PxsRigidCore;

struct PxsParticleStreamContactManager
{
	const PxsRigidCore*	rigidCore;
	const PxsShapeCore*	shapeCore;
	const PxTransform*	w2sOld;
	bool				isDrain;
	bool				isDynamic;
};

struct PxsParticleStreamShape
{
	const PxvParticleShape*					particleShape;
	PxU32									numContactManagers;
	const PxsParticleStreamContactManager*	contactManagers;
};

class PxsParticleContactManagerStreamWriter
{
public:
	static PxU32 getStreamSize(PxU32 numParticleShapes, PxU32 numContactManagers)
	{
		return	sizeof(PxU32) +														//particle shape count
				sizeof(PxU32) +														//stream size
				numParticleShapes*(sizeof(PxvParticleShape*) + sizeof(PxU32)) +		//shape ll pointer + contact manager count per shape
				numContactManagers*sizeof(PxsParticleStreamContactManager);			//contact manager data
	}

	PX_FORCE_INLINE PxsParticleContactManagerStreamWriter(PxU8* stream, PxU32 numParticleShapes, PxU32 numContactManagers) : 
		mStream(stream), 
		mNumContactsPtr(NULL)
	{
		PX_ASSERT(mStream);
		*reinterpret_cast<PxU32*>(mStream) = numParticleShapes;
		mStream += sizeof(PxU32);
		
		*reinterpret_cast<PxU32*>(mStream) = getStreamSize(numParticleShapes, numContactManagers);
		mStream += sizeof(PxU32);		
	}

	PX_FORCE_INLINE void addParticleShape(const PxvParticleShape* particleShape)
	{
		*reinterpret_cast<const PxvParticleShape**>(mStream) = particleShape;
		mStream += sizeof(const PxvParticleShape*);

		mNumContactsPtr = reinterpret_cast<PxU32*>(mStream);
		mStream += sizeof(PxU32);

		*mNumContactsPtr = 0;
	}

	PX_FORCE_INLINE void addContactManager(const PxsRigidCore* rigidCore, const PxsShapeCore* shapeCore, const PxTransform* w2sOld, bool isDrain, bool isDynamic)
	{
		PxsParticleStreamContactManager& cm = *reinterpret_cast<PxsParticleStreamContactManager*>(mStream);
		mStream += sizeof(PxsParticleStreamContactManager);

		cm.rigidCore = rigidCore;
		cm.shapeCore = shapeCore;
		cm.w2sOld = w2sOld;
		cm.isDrain = isDrain;
		cm.isDynamic = isDynamic;

		PX_ASSERT(mNumContactsPtr);
		(*mNumContactsPtr)++; 
	}

private:
	PxU8* mStream;
	PxU32* mNumContactsPtr;
};

class PxsParticleContactManagerStreamIterator
{
public:
	PX_FORCE_INLINE PxsParticleContactManagerStreamIterator() {}
	PX_FORCE_INLINE PxsParticleContactManagerStreamIterator(const PxU8* stream) : mStream(stream) {}
	
	PxsParticleContactManagerStreamIterator getNext(PxsParticleStreamShape& next)
	{
		Cm::MemFetchSmallBuffer buf;
		const PxvParticleShape** tmp0 = Cm::memFetchAsync<const PxvParticleShape*>((Cm::MemFetchPtr)mStream, 4, buf);
		mStream += sizeof(PxvParticleShape*);
		PxU32* tmp1 = Cm::memFetchAsync<PxU32>((Cm::MemFetchPtr)mStream, 5, buf);
		Cm::memFetchWait(4);
		next.particleShape = *tmp0;
		Cm::memFetchWait(5);
		next.numContactManagers = *tmp1;
		mStream += sizeof(PxU32);

		next.contactManagers = reinterpret_cast<const PxsParticleStreamContactManager*>(mStream);
		mStream += next.numContactManagers*sizeof(PxsParticleStreamContactManager);

		return PxsParticleContactManagerStreamIterator(mStream);
	}

	PX_FORCE_INLINE PxsParticleContactManagerStreamIterator getNext()
	{
		mStream += sizeof(PxvParticleShape*);
		PxU32 numContactManagers = *reinterpret_cast<const PxU32*>(mStream);
	
		mStream += sizeof(PxU32);
		mStream += numContactManagers*sizeof(PxsParticleStreamContactManager);
		return PxsParticleContactManagerStreamIterator(mStream);
	}

	PX_FORCE_INLINE bool operator == (const PxsParticleContactManagerStreamIterator& it)
	{
		return mStream == it.mStream;
	}
	
	PX_FORCE_INLINE bool operator != (const PxsParticleContactManagerStreamIterator& it)
	{
		return mStream != it.mStream;
	}

	PX_FORCE_INLINE const PxU8* getStream()
	{
		return mStream;
	}

private:	
	friend class PxsParticleContactManagerStreamReader;

private:
	const PxU8* mStream;
};

class PxsParticleContactManagerStreamReader
{
public:

	/*
	Reads header of stream consisting of shape count and stream end pointer
	*/
	PX_FORCE_INLINE PxsParticleContactManagerStreamReader(const PxU8* stream)
	{
		PX_ASSERT(stream);
		mStreamDataBegin = stream;
		mNumParticleShapes = *reinterpret_cast<const PxU32*>(mStreamDataBegin);
		mStreamDataBegin += sizeof(PxU32);
		PxU32 streamSize = *reinterpret_cast<const PxU32*>(mStreamDataBegin);
		mStreamDataBegin += sizeof(PxU32);
		mStreamDataEnd = stream + streamSize;
	}
	
	PX_FORCE_INLINE PxsParticleContactManagerStreamIterator getBegin()	const { return PxsParticleContactManagerStreamIterator(mStreamDataBegin); }
	PX_FORCE_INLINE PxsParticleContactManagerStreamIterator getEnd() const { return PxsParticleContactManagerStreamIterator(mStreamDataEnd); }
	PX_FORCE_INLINE PxU32 getNumParticleShapes() const	{ return mNumParticleShapes; }

private:
	PxU32		mNumParticleShapes;
	const PxU8*	mStreamDataBegin;
	const PxU8*	mStreamDataEnd;
};

}

#endif // PXS_PARTICLE_CONTACT_MANAGER_STREAM_H
