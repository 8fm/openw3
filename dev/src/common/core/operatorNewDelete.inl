#include "memory.h"
#include <new>

///////////////////////////////////////////////////////////////////////////
// Global operator new / delete overrides
// Define the macros OP_NEW, OP_DELETE, OP_NEW_ARRAY, OP_DELETE_ARRAY and THROWS_BAD_ALLOC for the platform being built
// e.g. #define OP_NEW operator new, #define THROWS_BAD_ALLOC throw(), etc

#if !defined( OP_NEW ) || !defined( OP_NEW_ARRAY ) || !defined( OP_DELETE ) || !defined( OP_DELETE_ARRAY ) || !defined( THROWS_BAD_ALLOC )
	#error Please define the operators to override (operator new, delete, etc)
#endif

// Standard operator new - nothrow
void* OP_NEW( size_t size, const std::nothrow_t& ) throw()
{	
	return RED_MEMORY_ALLOCATE_HYBRID( MemoryPool_Default, MC_OperatorNew, size );
}	

// These should throw bad_alloc on fail, but since nobody here really uses exception handling, we just return null for consistency
void* OP_NEW( size_t size ) THROWS_BAD_ALLOC
{	
	return RED_MEMORY_ALLOCATE_HYBRID( MemoryPool_Default, MC_OperatorNew, size );
}	

// Array operator new - nothrow
void* OP_NEW_ARRAY( size_t size, const std::nothrow_t& ) throw()		
{	
	// Operator new should ALWAYS return a valid pointer, even if size = 0 (according to the C++ standard)
	// In order to ensure the pointer will be valid, we allocate a system word at minimum
	const size_t c_minimumSizeToAllocate = sizeof(size_t);
	size = Red::Math::NumericalUtils::Max( c_minimumSizeToAllocate, size );
	return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_OperatorNew, size, 16 ); // ctremblay Orbis don't take to kindly to alignment error.
}

// Array operator new
void* OP_NEW_ARRAY( size_t size ) THROWS_BAD_ALLOC
{	
	// Operator new should ALWAYS return a valid pointer, even if size = 0 (according to the C++ standard)
	// In order to ensure the pointer will be valid, we allocate a system word at minimum
	const size_t c_minimumSizeToAllocate = sizeof(size_t);
	size = Red::Math::NumericalUtils::Max( c_minimumSizeToAllocate, size );
	return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_OperatorNew, size, 16 ); // ctremblay Orbis don't take to kindly to alignment error.
}

// Standard operator delete
void OP_DELETE( void *ptr ) throw()
{
	RED_MEMORY_FREE_HYBRID( MemoryPool_Default, MC_OperatorNew, ptr );
}

// Standard operator delete - nothrow
void OP_DELETE( void *ptr, const std::nothrow_t& ) throw()
{	
	RED_MEMORY_FREE_HYBRID( MemoryPool_Default, MC_OperatorNew, ptr );
}

// Array operator delete
void OP_DELETE_ARRAY( void *ptr ) throw()
{	 
	if ( ptr ) 
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_OperatorNew, ptr );
	}
}

// Array operator delete - nothrow
void OP_DELETE_ARRAY( void *ptr, const std::nothrow_t& ) throw()
{	 
	if ( ptr ) 
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_OperatorNew, ptr );
	}
}
