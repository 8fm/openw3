/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../core/configVar.h"
#include "scaleformMemory.h"
#include "scaleformMemoryDebug.h"

#ifdef USE_SCALEFORM

#ifdef SCALEFORM_MEMORY_STATS

static Red::Threads::CMutex GrabTheStackMutex;

static void RED_FORCE_INLINE GrabTheStack( Red::System::Error::Callstack& stack )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( GrabTheStackMutex );
	Red::System::Error::Handler::GetInstance()->GetCallstack( stack );
}

# define SCALEFORM_RECORD_ALLOC( size, align )\
	do\
	{\
		Red::System::Error::Callstack stack;\
		GrabTheStack( stack );\
		ScaleformDebugHelpers::CallstackTable.AddAlloc( stack, (size), (align) );\
	}\
	while(0)

# define SCALEFORM_DUMP_ALLOCS()\
	do\
	{\
		if ( Config::cvScaleformDebugDumpMemAllocs.Get() )\
		{\
			Config::cvScaleformDebugDumpMemAllocs.Set(false);\
			ScaleformDebugHelpers::CallstackTable.Dump();\
		}\
	}\
	while(0)
#else
# define SCALEFORM_RECORD_ALLOC(size,align) do {} while(false)
# define SCALEFORM_DUMP_ALLOCS() do {} while(false)
#endif

// The non-paged allocator causes too much churn on blocks > 512 bytes
#define USE_SCALEFORM_PAGED_ALLOC
namespace Config
{
	TConfigVar<Int32> cvScaleformAllocPagedMinAlign( "Scaleform/AllocPaged", "MinAlign", 1 );
	TConfigVar<Int32> cvScaleformAllocPagedMaxAlign( "Scaleform/AllocPaged", "MaxAlign", 0 ); // handle any align

	// 64K is too small to avoid constant allocations. SF seems to rest with 128 until loading something.
	TConfigVar<Int32> cvScaleformAllocPagedGranularity( "Scaleform/AllocPaged", "Granularity", 128*1024 ); // SF will try to use at least this value for alloc requests
}
#ifdef USE_SCALEFORM_PAGED_ALLOC
static CScaleformSysAllocPaged GScaleformSysAlloc;
#else
static CScaleformSysAlloc GScaleformSysAlloc;
#endif

SF::SysAllocBase* CreateScaleformSysAlloc()
{
	return &GScaleformSysAlloc;
}

void DestroyScaleformSysAlloc()
{
	// placeholder for any cleanup
}

//////////////////////////////////////////////////////////////////////////
// CScaleformSysAlloc
//////////////////////////////////////////////////////////////////////////
void* CScaleformSysAlloc::Alloc( SF::UPInt size, SF::UPInt align )
{
 	SCALEFORM_RECORD_ALLOC( size, align );
 	SCALEFORM_DUMP_ALLOCS();

	void* mem = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Scaleform, size, align);
	ASSERT ( mem );
	ASSERT ( ( reinterpret_cast<SF::UPInt>(mem) & (align - 1) ) == 0 );

	return mem;
}

void CScaleformSysAlloc::Free(void* ptr, SF::UPInt size, SF::UPInt align)
{
 	RED_MEMORY_FREE( MemoryPool_Default, MC_Scaleform, ptr );
 	SCALEFORM_DUMP_ALLOCS();
}

void* CScaleformSysAlloc::Realloc(void* oldPtr, SF::UPInt oldSize, SF::UPInt newSize, SF::UPInt align)
{
	// Happens a lot due to clearing small arrays but the padded alloc size means it requests the same amount of memory anyway
	// Check here instead of going through the global allocator just to return
	if ( oldSize == newSize )
	{
		// If ever hit, could just make this part of the above condition whether to return the old ptr
		RED_FATAL_ASSERT((reinterpret_cast<uintptr_t>(oldPtr) & (align-1)) == 0, "Realloc to stricter align");
		return oldPtr;
	}

 	SCALEFORM_RECORD_ALLOC( newSize, align );
 	SCALEFORM_DUMP_ALLOCS();
	void* mem = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Scaleform, newSize, align );

	Red::System::MemoryCopy( mem, oldPtr, Min(oldSize, newSize) );
	RED_MEMORY_FREE( MemoryPool_Default, MC_Scaleform, oldPtr );

	ASSERT ( mem );
	ASSERT ( ( reinterpret_cast<SF::UPInt>(mem) & (align - 1) ) == 0 );
	return mem;
}

//////////////////////////////////////////////////////////////////////////
// CScaleformSysAllocPaged
//////////////////////////////////////////////////////////////////////////
void CScaleformSysAllocPaged::GetInfo( Info* i ) const
{
	i->MinAlign = Config::cvScaleformAllocPagedMinAlign.Get();
	i->MaxAlign = Config::cvScaleformAllocPagedMaxAlign.Get();
	i->Granularity = Config::cvScaleformAllocPagedGranularity.Get();
	i->HasRealloc = false;
	//i->MaxHeapGranularity=;
	//i->SysDirectThreshold=
}

void* CScaleformSysAllocPaged::Alloc( SF::UPInt size, SF::UPInt align )
{
	SCALEFORM_RECORD_ALLOC( size, align );
	SCALEFORM_DUMP_ALLOCS();

	void* mem = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Scaleform, size, align);
	RED_FATAL_ASSERT ( ( reinterpret_cast<SF::UPInt>(mem) & (align - 1) ) == 0, "Misaligned mem!" );

	return mem;
}

SFBool CScaleformSysAllocPaged::Free( void* ptr, SF::UPInt size, SF::UPInt align )
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_Scaleform, ptr );
	SCALEFORM_DUMP_ALLOCS();

	return true;
}

// TBD: Implement if only to check if oldSize == newSize and return oldPtr, else return false?
SFBool CScaleformSysAllocPaged::ReallocInPlace( void* oldPtr, SF::UPInt oldSize, SF::UPInt newSize, SF::UPInt align )
{
	RED_UNUSED(oldPtr);
	RED_UNUSED(oldSize);
	RED_UNUSED(newSize);
	RED_UNUSED(align);

	return false;
}

#endif // USE_SCALEFORM
