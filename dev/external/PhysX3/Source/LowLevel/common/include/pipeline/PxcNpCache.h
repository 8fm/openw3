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

#ifndef PXC_NPCACHE_H
#define PXC_NPCACHE_H

#include "PsIntrinsics.h"
#include "PxcNpCacheStreamPair.h"

#include "PsPool.h"
#ifdef __SPU__
#include "..\..\..\LowLevel\ps3\include\spu\SPUAssert.h"
#include "..\..\..\ps3\include\spu\SpuNpMemBlock.h"
#include "PxsResourceManager.h"
#include "CmMemFetch.h"
//#include "PS3/PxcNpCachePS3.h"
#include "..\..\..\LowLevel\ps3\include\CellNarrowPhaseTask.h"
#else
#include "PsFoundation.h"
#endif
#include "GuContactMethodImpl.h"

namespace physx
{
struct PxcNpCache : Gu::Cache
{
	PxU32 pairData;
	PxU8* ptr;
	PxU16 size;

	PxcNpCache() 
	{
	}

	// legacy
	PX_FORCE_INLINE	PxU32	getPairData()			const	{	return pairData;	}
	PX_FORCE_INLINE	void	setPairData(PxU32 p)			{	pairData = p;		}
	PX_FORCE_INLINE	void	invalidate()					
	{
		pairData = 0;
		size = 0;
		ptr = 0;
		
		if(manifold)
		{
			if(isMultiManifold())
			{
				getMultipleManifold().clearManifold();
			}
			else
			{
				getManifold().clearManifold();
			}
		}
	}
};


// PT: versions without triangles

#ifdef __SPU__

#if USE_SEPARATE_CACHE_RESOURCE_MANAGER_ON_SPU
extern SpuNpMemBlock gMemBlockCaches;
#else
extern SpuNpMemBlock gMemBlockFrictionPatches;
#endif

template <typename T>
PX_FORCE_INLINE 
void PxcNpCacheWrite(PxcNpCacheStreamPair& streams,
					 PxcNpCache& cache,
					 const T& payload)
{
	uintptr_t ea;
	const PxU32 payloadSize = (sizeof(T)+15)&~15;
#if USE_SEPARATE_CACHE_RESOURCE_MANAGER_ON_SPU
	PxU8* ls=gMemBlockCaches.reserve(payloadSize,&ea);
#else
	PxU8* ls=gMemBlockFrictionPatches.reserve(payloadSize,&ea);
#endif
	cache.size=payloadSize;
	cache.ptr = (PxU8*)ea;
	if(ea!=NULL)
	{
		PX_ASSERT(ls);
		*reinterpret_cast<T*>(ls) = payload;
	}
}

template <typename T>
PX_FORCE_INLINE 
bool PxcNpCacheRead(PxcNpCacheStreamPair& streams, const PxcNpCache& cache, T& payload)
{
	if(cache.ptr!=NULL)
	{
		const uintptr_t ea=(uintptr_t)cache.ptr;
		PxU8 payload16[(sizeof(T)+15)&~15];
		Cm::memFetchAlignedAsync((Cm::MemFetchPtr)payload16,(Cm::MemFetchPtr)ea,CELL_ALIGN_SIZE_16(sizeof(T)),1);
		Cm::memFetchWait(1);
		payload = *reinterpret_cast<const T*>(payload16);		
		return true;
	}
	return false;
}

#else

template <typename T>
PX_FORCE_INLINE 
void PxcNpCacheWrite(PxcNpCacheStreamPair& streams,
					 PxcNpCache& cache,
					 const T& payload)
{
	cache.size = (sizeof(payload)+3)&~3;
	cache.ptr = streams.reserve(cache.size);
	if(cache.ptr==NULL)
	{
		PX_WARN_ONCE(true,"Reached limit set by PxSceneDesc::maxNbContactDataBlocks - ran out of buffer space for narrow phase.");
		PX_WARN_ONCE(true,"Either accept dropped contacts or increase buffer size allocated for narrow phase by increasing PxSceneDesc::maxNbContactDataBlocks");
		return;
	}
	*reinterpret_cast<T*>(cache.ptr) = payload;
}

template <typename T>
PX_FORCE_INLINE 
bool PxcNpCacheRead(PxcNpCacheStreamPair& streams, const PxcNpCache& cache, T& payload)
{
	if(cache.ptr==NULL)
		return false;

	payload = *reinterpret_cast<const T*>(cache.ptr);
	return true;
}

#endif

#ifdef __SPU__
// triangle indices-only versions are now only used for height fields

extern PxU8* spuContactCachePtr;
extern PxU32 spuContactCount;

template <typename T>
PX_FORCE_INLINE
void PxcNpCacheWrite(PxcNpCacheStreamPair& streams,
					 PxcNpCache& cache,
					 const T& payload,
					 PxU32 triangleCount, 
					 const PxU32* triangles)
{
	const PxU32 payloadSize = ((sizeof(T)+3)&~3);
	const PxU32 cacheSize = ((payloadSize + (1 + triangleCount)*4 + 15) & ~15);
	cache.size = cacheSize;
	uintptr_t ea;
#if USE_SEPARATE_CACHE_RESOURCE_MANAGER_ON_SPU
	PxU8* ls=gMemBlockCaches.reserve(cacheSize,&ea);
#else
	PxU8* ls=gMemBlockFrictionPatches.reserve(cacheSize,&ea);
#endif
	cache.ptr = (PxU8*)ea;
	if(ea==NULL)
		return;

	*reinterpret_cast<T*>(ls) = payload;
	PxU32* tptr = reinterpret_cast<PxU32*>(ls + payloadSize);
	*tptr++ = triangleCount;
	// PT: memCopy already takes care of the cache internally, so no need to do it ourselves
	if(triangles)
		PxMemCopy(tptr, triangles, triangleCount*sizeof(PxU32));
}

template <typename T>
PX_FORCE_INLINE
bool PxcNpCacheRead(PxcNpCacheStreamPair& streams,
					PxcNpCache& cache,
					T& payload,
					PxU32& triangleCount, 
					const PxU32* PX_RESTRICT & triangleIndices)
{
	const PxU8* ptr = cache.ptr;
	if(ptr==NULL)
	{
		triangleCount = 0;
		triangleIndices = NULL;
		return false;
	}

	//Dma the cached data to ls.
	const uintptr_t ea=(uintptr_t)cache.ptr;
	const PxU32 cacheSize = cache.size;
	PxU8* ls=&spuContactCachePtr[spuContactCount];
	spuContactCount += cacheSize;
	Cm::memFetchAlignedAsync((Cm::MemFetchPtr)ls,(Cm::MemFetchPtr)ea,cacheSize,1);
	Cm::memFetchWait(1);

	ptr=ls;
	payload = *reinterpret_cast<const T*>(ptr);
	const PxU32 payloadSize = ((sizeof(T)+3)&~3);
	ptr += payloadSize;
	const PxU32* tptr = reinterpret_cast<const PxU32*>(ptr);
	triangleCount = *tptr++;
	triangleIndices = tptr;

	PX_ASSERT(payloadSize + (1 + triangleCount)*4 == cache.size);
	return true;
}

#else

template <typename T>
PX_FORCE_INLINE
void PxcNpCacheWrite(PxcNpCacheStreamPair& streams,
					 PxcNpCache& cache,
					 const T& payload,
					 PxU32 triangleCount, 
					 const PxU32* triangles)
{
	const PxU32 payloadSize = (sizeof(payload)+3)&~3;
	cache.size = payloadSize + (1 + triangleCount)*4;
	cache.ptr = streams.reserve(cache.size);
	if(cache.ptr==NULL)
	{
		PX_WARN_ONCE(true,"Reached limit set by PxSceneDesc::maxNbContactDataBlocks - ran out of buffer space for narrow phase.");
		PX_WARN_ONCE(true,"Either accept dropped contacts or increase buffer size allocated for narrow phase by increasing PxSceneDesc::maxNbContactDataBlocks");
		return;
	}

	*reinterpret_cast<T*>(cache.ptr) = payload;
	PxU32* tptr = reinterpret_cast<PxU32*>(cache.ptr + payloadSize);
	*tptr++ = triangleCount;
	// PT: memCopy already takes care of the cache internally, so no need to do it ourselves
	if(triangles)
		PxMemCopy(tptr, triangles, triangleCount*sizeof(PxU32));
}

template <typename T>
PX_FORCE_INLINE
bool PxcNpCacheRead(PxcNpCacheStreamPair& streams,
					PxcNpCache& cache,
					T& payload,
					PxU32& triangleCount, 
					const PxU32* PX_RESTRICT & triangleIndices)
{
	const PxU8* ptr = cache.ptr;
	if(ptr==NULL)
	{
		triangleCount = 0;
		triangleIndices = NULL;
		return false;
	}

	const PxU32 payloadSize = (sizeof(payload)+3)&~3;
	payload = *reinterpret_cast<const T*>(ptr);
	ptr += payloadSize;

	const PxU32* tptr = reinterpret_cast<const PxU32*>(ptr);
	triangleCount = *tptr++;
	triangleIndices = tptr;

	PX_ASSERT(payloadSize + (1 + triangleCount)*4 == cache.size);
	return true;
}

#endif

//
// Narrow phase cache read and write implementations for triangle mesh data (verts + indices)
//
#ifdef __SPU__
extern PxU8 spuCacheBuffer[];
extern PxU32 spuCacheBufferSize;
#endif

// the size has to be know on PPU as well, since when we fall back to PPU
// we don't want to write more data to cache than we can later read on SPU
#define SPU_CACHE_BUFFER_SIZE 1024
#define DEBUG_MESH_NPCACHE 0

template <typename T>
void PxcNpCacheWrite(PxcNpCacheStreamPair& streams,
					 PxcNpCache& cache,
					 const T& payload,
					 PxU32 triCount, 
					 const PxVec3* triVerts,
					 const PxU32* triIndices)
{
	const PxU32 payloadSize = (sizeof(payload)+3)&~3;
	const PxU32 triVertsSize = triCount*3*sizeof(triVerts[0]);
	const PxU32 triIndicesSize = triCount*sizeof(triIndices[0]);
	cache.size = (payloadSize + triVertsSize + triIndicesSize + 4 + 0xF) & ~0xF;

	if (DEBUG_MESH_NPCACHE) { cache.ptr = NULL; return; }

	// AP: we have to do this check both on PPU and SPU
	// because on one frame this object might get assigned to PPU while on the next it may end up on SPU
#ifdef PX_PS3
	if (cache.size >= SPU_CACHE_BUFFER_SIZE)
	{
		// no room for this much data, invalidate the cache
		cache.ptr = NULL;
		return;
	}
#endif

#ifdef __SPU__
	uintptr_t ea;
#if USE_SEPARATE_CACHE_RESOURCE_MANAGER_ON_SPU
	PxU8* ls=gMemBlockCaches.reserve(cache.size,&ea);
#else
	PxU8* ls=gMemBlockFrictionPatches.reserve(cache.size,&ea);
#endif
	cache.ptr = (PxU8*)ea;
	if(ea==NULL)
		return;
#else
	cache.ptr = streams.reserve(cache.size);
	if(cache.ptr==NULL)
	{
		PX_WARN_ONCE(true,"Reached limit set by PxSceneDesc::maxNbContactDataBlocks - ran out of buffer space for narrow phase.");
		PX_WARN_ONCE(true,"Either accept dropped contacts or increase buffer size allocated for narrow phase by increasing PxSceneDesc::maxNbContactDataBlocks");
		return;
	}
	PxU8* ls = cache.ptr;
#endif

	*reinterpret_cast<T*>(ls) = payload;
	*reinterpret_cast<PxU32*>(ls+payloadSize) = triCount;
	// PT: memCopy already takes care of the cache internally, so no need to do it ourselves
	if(triVerts)
		PxMemCopy(ls+payloadSize+sizeof(PxU32), triVerts, triVertsSize);
	if(triIndices)
		PxMemCopy(ls+payloadSize+sizeof(PxU32)+triVertsSize, triIndices, triIndicesSize);
}

template <typename T>
bool PxcNpCacheRead(PxcNpCacheStreamPair& streams,
					PxcNpCache& cache,
					T& payload,
					PxU32& triCount, 
					const PxVec3* PX_RESTRICT& triVerts,
					const PxU32* PX_RESTRICT& triIndices)
{
	const PxU8* ls = cache.ptr;
	if(ls==NULL)
	{
		triCount = 0;
		triVerts = NULL;
		triIndices = NULL;
		return false;
	}

	const PxU32 payloadSize = (sizeof(payload)+3)&~3;

#ifdef __SPU__
	//Dma the cached data to ls.
	const uintptr_t ea = (uintptr_t)cache.ptr;
	const PxU32 cacheSize = cache.size;
	ls=&spuCacheBuffer[0];
	SPU_ASSERT_ALWAYS(cacheSize<spuCacheBufferSize);
	Cm::memFetchAlignedAsync((Cm::MemFetchPtr)ls,(Cm::MemFetchPtr)ea,cacheSize,1);
	Cm::memFetchWait(1);
#endif

	payload = *reinterpret_cast<const T*>(ls);
	triCount = *reinterpret_cast<const PxU32*>(ls+payloadSize);

	const PxU32 triVertsSize = triCount*3*sizeof(triVerts[0]);
#ifdef PX_DEBUG
	const PxU32 triIndicesSize = triCount*sizeof(triIndices[0]);
#endif

	triVerts = reinterpret_cast<const PxVec3*>(ls+payloadSize+sizeof(PxU32));
	triIndices = reinterpret_cast<const PxU32*>(ls+payloadSize+sizeof(PxU32)+triVertsSize);

	PX_ASSERT(cache.size == ((payloadSize + triVertsSize + triIndicesSize + 4 + 0xF)&~0xF));

	return true;
}

template <typename T>
void PxcNpCacheWrite(PxcNpCacheStreamPair& streams,
					 PxcNpCache& cache,
					 const T& payload,
					 PxU32 bytes, 
					 const PxU8* data)
{
	const PxU32 payloadSize = (sizeof(payload)+3)&~3;
	cache.size = (payloadSize + 4 + bytes + 0xF)&~0xF;

	// AP: we have to do this check both on PPU and SPU
	// because on one frame this object might get assigned to PPU while on the next it may end up on SPU
#ifdef PX_PS3
	if (cache.size >= SPU_CACHE_BUFFER_SIZE)
	{
		// no room for this much data, invalidate the cache
		cache.ptr = NULL;
		return;
	}
#endif

#ifdef __SPU__
	uintptr_t ea;
	#if USE_SEPARATE_CACHE_RESOURCE_MANAGER_ON_SPU
	PxU8* ls=gMemBlockCaches.reserve(cache.size,&ea);
	#else
	PxU8* ls=gMemBlockFrictionPatches.reserve(cache.size,&ea);
	#endif
	cache.ptr = (PxU8*)ea;
	if(ea==NULL)
		return;
#else
	PxU8* ls = streams.reserve(cache.size);
	cache.ptr = ls;
	if(ls==NULL || ((PxU8*)(-1))==ls)
	{
		if(ls==NULL)
		{
			PX_WARN_ONCE(true,"Reached limit set by PxSceneDesc::maxNbContactDataBlocks - ran out of buffer space for narrow phase.");
			PX_WARN_ONCE(true,"Either accept dropped contacts or increase buffer size allocated for narrow phase by increasing PxSceneDesc::maxNbContactDataBlocks");
			return;
		}
		else
		{
			PX_WARN_ONCE(true,"Attempting to allocate more than 16K of contact data for a single contact pair in narrowphase.");
			PX_WARN_ONCE(true,"Either accept dropped contacts or simplify collision geometry");
			cache.ptr = NULL;
			ls = NULL;
			return;
		}
	}
#endif

	*reinterpret_cast<T*>(ls) = payload;
	*reinterpret_cast<PxU32*>(ls+payloadSize) = bytes;
	// PT: memCopy already takes care of the cache internally, so no need to do it ourselves
	if(data)
		PxMemCopy(ls+payloadSize+sizeof(PxU32), data, bytes);
}

template <typename T>
bool PxcNpCacheRead(PxcNpCache& cache,
					T& payload,
					PxU32& bytes, 
					const PxU8* PX_RESTRICT& data)
{
	const PxU8* ls = cache.ptr;
	if(ls==NULL)
	{
		bytes = 0;
		data = NULL;
		return false;
	}

	const PxU32 payloadSize = (sizeof(payload)+3)&~3;
#ifdef __SPU__
	//Dma the cached data to ls.
	const uintptr_t ea = (uintptr_t)cache.ptr;
	const PxU32 cacheSize = cache.size;
	ls=&spuCacheBuffer[0];
	SPU_ASSERT_ALWAYS(cacheSize<spuCacheBufferSize);
	//pxPrintf("Reading to %x from %x (%d bytes)\n", ls, ea, cacheSize);
	Cm::memFetchAlignedAsync((Cm::MemFetchPtr)ls,(Cm::MemFetchPtr)ea,cacheSize,1);
	Cm::memFetchWait(1);
#endif
	payload = *reinterpret_cast<const T*>(ls);
	bytes = *reinterpret_cast<const PxU32*>(ls+payloadSize);
	data = reinterpret_cast<const PxU8*>(ls+payloadSize+sizeof(PxU32));
	PX_ASSERT(cache.size == ((payloadSize + 4 + bytes+0xF)&~0xF));

	return true;
}

////


template <typename T>
PxU8* PxcNpCacheWriteInitiate(PxcNpCacheStreamPair& streams, PxcNpCache& cache, const T& payload, PxU32 bytes)
{
	PX_UNUSED(payload);

	const PxU32 payloadSize = (sizeof(payload)+3)&~3;
	cache.size = (payloadSize + 4 + bytes + 0xF)&~0xF;

	// AP: we have to do this check both on PPU and SPU
	// because on one frame this object might get assigned to PPU while on the next it may end up on SPU
#ifdef PX_PS3
	if (cache.size >= SPU_CACHE_BUFFER_SIZE)
	{
		// no room for this much data, invalidate the cache
		cache.ptr = NULL;
		return NULL;
	}
#endif

#ifdef __SPU__
	uintptr_t ea;
	#if USE_SEPARATE_CACHE_RESOURCE_MANAGER_ON_SPU
	PxU8* ls=gMemBlockCaches.reserve(cache.size,&ea);
	#else
	PxU8* ls=gMemBlockFrictionPatches.reserve(cache.size,&ea);
	#endif
	cache.ptr = (PxU8*)ea;
	if(ea==NULL)
		return NULL;
#else
	PxU8* ls = streams.reserve(cache.size);
	cache.ptr = ls;
	if(NULL==ls || (PxU8*)(-1)==ls)
	{
		if(NULL==ls)
		{
			PX_WARN_ONCE(true,"Reached limit set by PxSceneDesc::maxNbContactDataBlocks - ran out of buffer space for narrow phase.");
			PX_WARN_ONCE(true,"Either accept dropped contacts or increase buffer size allocated for narrow phase by increasing PxSceneDesc::maxNbContactDataBlocks");
		}
		else
		{
			PX_WARN_ONCE(true,"Attempting to allocate more than 16K of contact data for a single contact pair in narrowphase.");
			PX_WARN_ONCE(true,"Either accept dropped contacts or simplify collision geometry");
			cache.ptr = NULL;
			ls = NULL;
		}
	}
#endif
	return ls;
}

template <typename T>
PX_FORCE_INLINE void PxcNpCacheWriteFinalize(PxU8* ls, const T& payload, PxU32 bytes, const PxU8* data)
{
	const PxU32 payloadSize = (sizeof(payload)+3)&~3;
	*reinterpret_cast<T*>(ls) = payload;
	*reinterpret_cast<PxU32*>(ls+payloadSize) = bytes;
	// PT: memCopy already takes care of the cache internally, so no need to do it ourselves
	if(data)
		PxMemCopy(ls+payloadSize+sizeof(PxU32), data, bytes);
}


template <typename T>
PX_FORCE_INLINE PxU8* PxcNpCacheRead(PxcNpCache& cache, T*& payload)
{
	PxU8* ls = cache.ptr;
	payload = reinterpret_cast<T*>(ls);
	const PxU32 payloadSize = (sizeof(T)+3)&~3;
	return reinterpret_cast<PxU8*>(ls+payloadSize+sizeof(PxU32));
}

template <typename T>
const PxU8* PxcNpCacheRead2(PxcNpCache& cache, T& payload, PxU32& bytes)
{
	const PxU8* ls = cache.ptr;
	if(ls==NULL)
	{
		bytes = 0;
		return NULL;
	}

	const PxU32 payloadSize = (sizeof(payload)+3)&~3;
#ifdef __SPU__
	//Dma the cached data to ls.
	const uintptr_t ea = (uintptr_t)cache.ptr;
	const PxU32 cacheSize = cache.size;
	ls=&spuCacheBuffer[0];
	SPU_ASSERT_ALWAYS(cacheSize<spuCacheBufferSize);
	//pxPrintf("Reading to %x from %x (%d bytes)\n", ls, ea, cacheSize);
	Cm::memFetchAlignedAsync((Cm::MemFetchPtr)ls,(Cm::MemFetchPtr)ea,cacheSize,1);
	Cm::memFetchWait(1);
#endif
	payload = *reinterpret_cast<const T*>(ls);
	bytes = *reinterpret_cast<const PxU32*>(ls+payloadSize);
	PX_ASSERT(cache.size == ((payloadSize + 4 + bytes+0xF)&~0xF));
	return reinterpret_cast<const PxU8*>(ls+payloadSize+sizeof(PxU32));
}

}

#endif // #ifndef PXC_NPCACHE_H
