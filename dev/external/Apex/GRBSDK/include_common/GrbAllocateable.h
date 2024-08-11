#ifndef GRB_ALLOCATEABLE_H
#define GRB_ALLOCATEABLE_H

#define PRB_TRACK_ALLOCATIONS	0

// TODO avoroshilov: rename calling conv
#if defined(WIN32)
#define ALLOC_CALL_CONV __cdecl
#define ALLOC_API extern "C" __declspec(dllexport)
#else
#define ALLOC_CALL_CONV /* void */
#define ALLOC_API extern "C"
#endif

ALLOC_API void * ALLOC_CALL_CONV GrbAllocateableMalloc(size_t size);
ALLOC_API void * ALLOC_CALL_CONV GrbAllocateableMallocDEBUG(size_t size, const char* fileName, int line, const char* className, int type);
ALLOC_API void ALLOC_CALL_CONV GrbAllocateableFree(void * mem);

#if PRB_TRACK_ALLOCATIONS
#define GRB_MALLOC(x) GrbAllocateableMallocDEBUG(x,__FILE__,__LINE__,"Undefined", 0)
#else
#define GRB_MALLOC(x) GrbAllocateableMalloc(x)
#endif

#define GRB_FREE(x) GrbAllocateableFree(x)

#define GRB_DELETE(x) delete x
#define GRB_DELETE_SINGLE(x) if (x) { delete x;		x = NULL; }
#define GRB_DELETE_ARRAY(x) if (x) { delete []x;	x = NULL; }

#if PRB_TRACK_ALLOCATIONS
	#define NX_NEW_RRB(x)	new((const char *)__FILE__, __LINE__, #x, 1) x
#else
	#define NX_NEW_RRB(x)	new(1) x
#endif

// Simple allocateable class
class GrbAllocateable
{
public:
	// Allocation operators
	void* operator new(size_t size)
	{
		return GrbAllocateableMalloc(size);
	}

	void* operator new[](size_t size)
	{
		return GrbAllocateableMalloc(size);
	}

	void* operator new(size_t size, int)
	{
		return GrbAllocateableMalloc(size);
	}

	void* operator new[](size_t size, int)
	{
		return GrbAllocateableMalloc(size);
	}

	void* operator new(size_t size, const char* fileName, int line, const char* className, int type)
	{
		return GrbAllocateableMallocDEBUG( size, fileName, line, className, type );
	}

	void* operator new[](size_t size, const char* fileName, int line, const char* className, int type)
	{
		return GrbAllocateableMallocDEBUG( size, fileName, line, className, type );
	}

	void* operator new( size_t, void* mem )
	{
		return mem;
	}
	
	void operator delete(void* p)
	{
		GrbAllocateableFree(p);
	}

	void operator delete[](void* p)
	{
		GrbAllocateableFree(p);
	}
};

#endif