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


#ifndef PXC_THREADCOHERANTCACHE_H
#define PXC_THREADCOHERANTCACHE_H

#include "PsMutex.h"
#include "PsAlignedMalloc.h"
#include "PsSList.h"

namespace physx
{

class PxsContext;
/*!
Controls a pool of large objects which must be thread safe. 
Tries to return the object most recently used by the thread(for better cache coherancy).
Assumes the object has a default contructor.

(Note the semantics are different to a pool because we dont want to construct/destroy each time
an object is requested, which may be expensive).

TODO: add thread coherancy.
*/
template<class T>
class PxcThreadCoherantCache : public Ps::AlignedAllocator<16, Ps::ReflectionAllocator<T> >
{
	PX_NOCOPY(PxcThreadCoherantCache)
	typedef Ps::AlignedAllocator<16, Ps::ReflectionAllocator<T> > Allocator;
public:

	typedef Ps::SListEntry EntryBase;

	PX_INLINE PxcThreadCoherantCache(const Allocator& alloc = Allocator()) : Allocator(alloc)
	{
	}

	PX_INLINE ~PxcThreadCoherantCache()
	{
		T* np = static_cast<T*>(root.pop());

		while(np!=NULL)
		{
			np->~T();
			Allocator::deallocate(np);
			np = static_cast<T*>(root.pop());
		}
	}

	PX_INLINE T* get(PxsContext *context)
	{
		T* rv = static_cast<T*>(root.pop());
		if(rv==NULL)
		{
			rv = (T*)Allocator::allocate(sizeof(T), __FILE__, __LINE__);
			new (rv) T(context);
		}

		return rv;
	}

	PX_INLINE void put(T* item)
	{
		root.push(*item);
	}


private:
	Ps::SList root;
	const char * mDEBUGName;

	template<class T2>
	friend class PxcThreadCoherantCacheIterator;
};

/*!
Used to iterate over all objects controlled by the cache.

Note: The iterator flushes the cache(extracts all items on construction and adds them back on
destruction so we can iterate the list in a safe manner).
*/
template<class T> 
class PxcThreadCoherantCacheIterator
{
public:
	PxcThreadCoherantCacheIterator(PxcThreadCoherantCache<T>& cache) : mCache(cache)
	{
		mNext = cache.root.flush();
		mFirst = mNext;
	}
	~PxcThreadCoherantCacheIterator()
	{
		Ps::SListEntry* np = mFirst;
		while(np != NULL)
		{
			Ps::SListEntry* npNext = np->next();
			mCache.root.push(*np);
			np = npNext;
		}
	}

	PX_INLINE T* getNext()
	{
		if(mNext == NULL)
			return NULL;

		T* rv = static_cast<T*>(mNext);
		mNext = mNext->next();

		return rv;
	}
private:

	PxcThreadCoherantCacheIterator& operator=(const PxcThreadCoherantCacheIterator&);
	PxcThreadCoherantCache<T> &mCache;
	Ps::SListEntry* mNext;
	Ps::SListEntry* mFirst;
	
};

}

#endif
