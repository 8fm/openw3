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


#include "PxcPoolMalloc.h"
#include "PsMutex.h"
#include "PsPool.h"

using namespace physx;

/*

Implementation of a global pool allocator for casses where a specialized pool allocator does not make sense.
For example general containers.

Note: Init/Term should not be called concurrently. However PoolMalloc/PoolFree are safe.

Also note we do not make PoolMallocData static because on some platforms synchronization objects cannot
be created from static constructors(eg PS3/Linux:-(

TODO: It would be beneficial to impliment spin counts for Ps::Mutex if we expect high contention on
the pool allocator lock. (probably not a problem as it is mostly called during init at present)

TODO: Maybe there is some benefit in generalizing the pool sizes. But probably not.
*/

struct PxcPoolMallocData
{
	static const int BLOCK_SIZE = 1024 * 4;

	PxcPoolMallocData() 
	:	mPool8("mPool8", BLOCK_SIZE/8), 
		mPool16("mPool16", BLOCK_SIZE/16), 
		mPool32("mPool32", BLOCK_SIZE/32)
	{}

	~PxcPoolMallocData() {}
	
	Ps::Mutex mPoolMutex;

	struct PoolData8 { PxU8 mData[8]; };
	struct PoolData16 { PxU8 mData[16]; };
	struct PoolData32 { PxU8 mData[32]; };

	Ps::Pool<PoolData8> mPool8;
	Ps::Pool<PoolData16> mPool16;
	Ps::Pool<PoolData32> mPool32;

};

static PxcPoolMallocData* gPoolMallocData = NULL;


void PxcPoolMallocInit()
{
	gPoolMallocData = (PxcPoolMallocData *) PX_ALLOC(sizeof(PxcPoolMallocData), PX_DEBUG_EXP("PxcPoolMallocData"));
	new (gPoolMallocData) PxcPoolMallocData();
}

void PxcPoolMallocTerm()
{
	if(gPoolMallocData != NULL)
	{
		gPoolMallocData->~PxcPoolMallocData();
		PX_FREE(gPoolMallocData);

		gPoolMallocData = NULL;
	}
}

void* PxcPoolMalloc(size_t size)
{
	PX_ASSERT(gPoolMallocData != NULL);

	Ps::Mutex::ScopedLock(gPoolMallocData->mPoolMutex);

	if(size <= 8)
	{
		PxcPoolMallocData::PoolData8 *mem = gPoolMallocData->mPool8.allocate();
		return mem;
	}
	else if(size <= 16)
	{
		PxcPoolMallocData::PoolData16 *mem = gPoolMallocData->mPool16.allocate();
		return mem;
	}
	else if(size <= 32)
	{
		PxcPoolMallocData::PoolData32 *mem = gPoolMallocData->mPool32.allocate();
		return mem;
	}
	else
	{
		return PX_ALLOC(size, PX_DEBUG_EXP("PoolMallocData"));
	}
}

void PxcPoolFree(void* ptr, size_t size)
{
	PX_ASSERT(gPoolMallocData != NULL);

	Ps::Mutex::ScopedLock(gPoolMallocData->mPoolMutex);

	if(size <= 8)
	{
		PxcPoolMallocData::PoolData8 *mem = (PxcPoolMallocData::PoolData8 *) ptr;
		gPoolMallocData->mPool8.deallocate(mem);
	}
	else if(size <= 16)
	{
		PxcPoolMallocData::PoolData16 *mem = (PxcPoolMallocData::PoolData16 *) ptr;
		gPoolMallocData->mPool16.deallocate(mem);
	}
	else if(size <= 32)
	{
		PxcPoolMallocData::PoolData32 *mem = (PxcPoolMallocData::PoolData32 *) ptr;
		gPoolMallocData->mPool32.deallocate(mem);
	}
	else
	{
		PX_FREE(ptr);
	}
}
