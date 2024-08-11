/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "rapidjson/rapidjson.h"

//////////////////////////////////////////////////////////////////////////
// CJSONAllocator
// -Our Allocator for RapidJSON
//////////////////////////////////////////////////////////////////////////
#define RED_JSON_MEMORY_DEFINE_ALLOCATOR(POOL_TYPE, MEMORY_CLASS)																	\
class CJSONAllocator_##POOL_TYPE##MEMORY_CLASS     																			\
{																															\
public:																														\
	typedef Red::System::MemSize MemSize;																					\
	static const Bool kNeedFree = true;																						\
	RED_INLINE void* Malloc( MemSize size ) { return RED_MEMORY_ALLOCATE( POOL_TYPE, MEMORY_CLASS, size ); }				\
	RED_INLINE void* Realloc( void* originalPtr, MemSize originalSize, MemSize newSize )									\
	{ RED_UNUSED( originalSize); return RED_MEMORY_REALLOCATE( POOL_TYPE, originalPtr, MEMORY_CLASS, newSize );}			\
	static void Free( void* ptr ) { RED_MEMORY_FREE( POOL_TYPE, MEMORY_CLASS, ptr ); }										\
};																															\
																															\
class CJSONAllocatorHandler_##POOL_TYPE##MEMORY_CLASS																	    \
{																															\
public:																														\
	typedef  CJSONAllocator_##POOL_TYPE##MEMORY_CLASS PoolAllocator;	                									\
	typedef TSingleton< PoolAllocator, TDefaultLifetime, TCreateUsingNew > PoolAllocatorInstance;							\
};

#define RED_JSON_MEMORY_GET_ALLOCATOR(POOL_TYPE, MEMORY_CLASS) CJSONAllocatorHandler_##POOL_TYPE##MEMORY_CLASS

RED_JSON_MEMORY_DEFINE_ALLOCATOR(MemoryPool_Default, MC_Json)

