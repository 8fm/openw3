#error DO NOT USE THIS YET. MALLOC CANONT BE REPLACED AT THIS TIME AS THE MANAGER RELIES ON FIOS BEING INITIALISED

#ifndef RED_MEMORY_ORBIS_MALLOC_MANAGER
	#error RED_MEMORY_ORBIS_MALLOC_MANAGER must be defined in order to define a malloc replacement for Orbis
#endif

#ifndef RED_MEMORY_ORBIS_MALLOC_POOLNAME
	#error RED_MEMORY_ORBIS_MALLOC_POOLNAME must be defined in order to define a malloc replacement for Orbis
#endif

#ifndef RED_MEMORY_ORBIS_MALLOC_MEMORYCLASS
	#error RED_MEMORY_ORBIS_MALLOC_MEMORYCLASS must be defined in order to define a malloc replacement for Orbis
#endif

#include <mspace.h>		// Must be included for SceLibcMallocManagedSize
#include <errno.h>

///////////////////////////////////////////////////////////////////////////
// Malloc replacements declarations
extern "C" int user_malloc_init(void);
extern "C" int user_malloc_finalize(void);
extern "C" void *user_malloc(size_t size);
extern "C" void user_free(void *ptr);
extern "C" void *user_calloc(size_t nelem, size_t size);
extern "C" void *user_realloc(void *ptr, size_t size);
extern "C" void *user_memalign(size_t boundary, size_t size);
extern "C" int user_posix_memalign(void **ptr, size_t boundary, size_t size);
extern "C" void *user_reallocalign(void *ptr, size_t size, size_t boundary);
extern "C" int user_malloc_stats(SceLibcMallocManagedSize *mmsize);
extern "C" int user_malloc_stats_fast(SceLibcMallocManagedSize *mmsize);
extern "C" size_t user_malloc_usable_size(void *ptr);

///////////////////////////////////////////////////////////////////////////
// user_malloc_init
//	Called on CRT startup
extern "C" int user_malloc_init(void)
{	
	// We don't need to do any initialisation as the manager will handle it
	return 0;		
}

///////////////////////////////////////////////////////////////////////////
// user_malloc_finalize
//	Called on CRT shutdown
extern "C" int user_malloc_finalize(void)
{
	// We don't do anything, the manager handles shutting itself down
	return 0;
}

///////////////////////////////////////////////////////////////////////////
// user_malloc
//	malloc wrapper
extern "C" void *user_malloc(size_t size)
{
	return INTERNAL_RED_MEMORY_ALLOCATE( RED_MEMORY_ORBIS_MALLOC_MANAGER, RED_MEMORY_ORBIS_MALLOC_POOLNAME, RED_MEMORY_ORBIS_MALLOC_MEMORYCLASS, size );
}

///////////////////////////////////////////////////////////////////////////
// user_free
//	free wrapper
extern "C" void user_free(void *ptr)
{
	INTERNAL_RED_MEMORY_FREE( RED_MEMORY_ORBIS_MALLOC_MANAGER, RED_MEMORY_ORBIS_MALLOC_POOLNAME, RED_MEMORY_ORBIS_MALLOC_MEMORYCLASS, ptr );
}

///////////////////////////////////////////////////////////////////////////
// user_calloc
//	calloc wrapper
extern "C" void *user_calloc(size_t nelem, size_t size)
{
	size_t totalSize = nelem * size;
	void* resultPtr = INTERNAL_RED_MEMORY_ALLOCATE( RED_MEMORY_ORBIS_MALLOC_MANAGER, RED_MEMORY_ORBIS_MALLOC_POOLNAME, RED_MEMORY_ORBIS_MALLOC_MEMORYCLASS, totalSize );
	if( resultPtr != nullptr )		// C Standards dictate that the memory be initialised to zero
	{
		Red::System::MemoryZero( resultPtr, totalSize );
	}

	return resultPtr;
}

///////////////////////////////////////////////////////////////////////////
// user_realloc
//	realloc wrapper
extern "C" void *user_realloc(void *ptr, size_t size)
{
	return INTERNAL_RED_MEMORY_REALLOCATE( RED_MEMORY_ORBIS_MALLOC_MANAGER, RED_MEMORY_ORBIS_MALLOC_POOLNAME, ptr, RED_MEMORY_ORBIS_MALLOC_MEMORYCLASS, size );
}

///////////////////////////////////////////////////////////////////////////
// user_memalign
//	memalign wrapper
extern "C" void *user_memalign(size_t boundary, size_t size)
{
	return INTERNAL_RED_MEMORY_ALLOCATE_ALIGNED( RED_MEMORY_ORBIS_MALLOC_MANAGER, RED_MEMORY_ORBIS_MALLOC_POOLNAME, RED_MEMORY_ORBIS_MALLOC_MEMORYCLASS, size, boundary );
}

///////////////////////////////////////////////////////////////////////////
// user_posix_memalign
//	posix_memalign wrapper
extern "C" int user_posix_memalign(void **ptr, size_t boundary, size_t size)
{
	if( ptr == nullptr || ( boundary & 1 ) )		// Non power of 2 alignment, or null result pointer
	{
		return EINVAL;
	}

	*ptr = INTERNAL_RED_MEMORY_ALLOCATE_ALIGNED( RED_MEMORY_ORBIS_MALLOC_MANAGER, RED_MEMORY_ORBIS_MALLOC_POOLNAME, RED_MEMORY_ORBIS_MALLOC_MEMORYCLASS, size, boundary );
	if( *ptr == nullptr )
	{
		return ENOMEM;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////
// user_reallocalign
//	reallocalign wrapper
extern "C" void *user_reallocalign(void *ptr, size_t size, size_t boundary)
{
	return INTERNAL_RED_MEMORY_REALLOCATE_ALIGNED( RED_MEMORY_ORBIS_MALLOC_MANAGER, RED_MEMORY_ORBIS_MALLOC_POOLNAME, ptr, RED_MEMORY_ORBIS_MALLOC_MEMORYCLASS, size, boundary );
}

///////////////////////////////////////////////////////////////////////////
// user_malloc_stats
//	malloc_stats wrapper
extern "C" int user_malloc_stats(SceLibcMallocManagedSize *mmsize)
{
	if( mmsize != nullptr )
	{
		// Get the in-use metrics from the runtime metrics
		const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics = RED_MEMORY_ORBIS_MALLOC_MANAGER.GetMetricsCollector().GetMetricsForPool( RED_MEMORY_ORBIS_MALLOC_POOLNAME );
		mmsize->currentInuseSize = poolMetrics.m_allocatedBytesPerMemoryClass[ RED_MEMORY_ORBIS_MALLOC_MEMORYCLASS ];
		 
		// Get the total size from the pool info
		Red::MemoryFramework::AllocatorInfo poolInfo;
		INTERNAL_RED_MEMORY_GET_ALLOCATOR( RED_MEMORY_ORBIS_MALLOC_MANAGER, RED_MEMORY_ORBIS_MALLOC_POOLNAME )->RequestAllocatorInfo( poolInfo );
		mmsize->currentSystemSize = poolInfo.GetBudget();

		// Since we don't track maximum values internally, the best we can do is copy the current ones
		mmsize->maxInuseSize = mmsize->currentInuseSize;
		mmsize->maxSystemSize = mmsize->currentSystemSize;

		return 0;
	}
	else
	{
		return 1;
	}
}

///////////////////////////////////////////////////////////////////////////
// user_malloc_stats_fast
//	malloc_stats_fast wrapper
extern "C" int user_malloc_stats_fast(SceLibcMallocManagedSize *mmsize)
{
	return user_malloc_stats( mmsize );
}

///////////////////////////////////////////////////////////////////////////
// user_malloc_usable_size
//	malloc_usable_size wrapper
extern "C" size_t user_malloc_usable_size(void *ptr)
{
	return INTERNAL_RED_MEMORY_GET_ALLOCATOR( RED_MEMORY_ORBIS_MALLOC_MANAGER, RED_MEMORY_ORBIS_MALLOC_POOLNAME )->GetAllocationSize( ptr );
}