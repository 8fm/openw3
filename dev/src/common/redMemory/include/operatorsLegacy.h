/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#include "redMemoryPublic.h"

#ifdef RED_MEMORY_LEGACY_OPERATORS_DEFINED
	#error operatorsLegacy.h can only be included once and only for Orbis platform. Never include this. ever. 
#endif

#define RED_MEMORY_LEGACY_OPERATORS_DEFINED

// PS4 has its own hooks for global new / delete because... Sony. 
// Note, it MUST be linked to main (but not defined in the main object file) to link correctly to other PRXs, CRT, etc
#if defined( RED_PLATFORM_ORBIS )

	#define OP_NEW user_new
	#define OP_NEW_ARRAY user_new_array
	#define OP_DELETE user_delete
	#define OP_DELETE_ARRAY user_delete_array
	#define THROWS_BAD_ALLOC throw(std::bad_alloc)

#else

	#define OP_NEW			operator new
	#define OP_NEW_ARRAY	operator new[]
	#define OP_DELETE		operator delete
	#define OP_DELETE_ARRAY operator delete[]
	#define THROWS_BAD_ALLOC throw()

#endif

// Standard operator new - nothrow
void* OP_NEW( size_t size, const std::nothrow_t& ) throw()
{	
	return RED_ALLOCATE( red::memory::PoolLegacy, size ); // ctremblay Orbis don't take to kindly to alignment error.
}	

// These should throw bad_alloc on fail, but since nobody here really uses exception handling, we just return null for consistency
void* OP_NEW( size_t size ) THROWS_BAD_ALLOC
{	
	return RED_ALLOCATE( red::memory::PoolLegacy, size ); // ctremblay Orbis don't take to kindly to alignment error.
}	

// Array operator new - nothrow
void* OP_NEW_ARRAY( size_t size, const std::nothrow_t& ) throw()		
{	
	return RED_ALLOCATE( red::memory::PoolLegacy, size ); // ctremblay Orbis don't take to kindly to alignment error.
}

// Array operator new
void* OP_NEW_ARRAY( size_t size ) THROWS_BAD_ALLOC
{	
	return RED_ALLOCATE( red::memory::PoolLegacy, size ); // ctremblay Orbis don't take to kindly to alignment error.
}

// Standard operator delete
void OP_DELETE( void *ptr ) throw()
{	
	if ( ptr )
	{
		RED_FREE( red::memory::PoolLegacy, ptr );
	}
}

// Standard operator delete - nothrow
void OP_DELETE( void *ptr, const std::nothrow_t& ) throw()
{	
	if ( ptr )
	{
		RED_FREE( red::memory::PoolLegacy, ptr );
	}
}

// Array operator delete
void OP_DELETE_ARRAY( void *ptr ) throw()
{	 
	if ( ptr ) 
	{
		RED_FREE( red::memory::PoolLegacy, ptr );
	}
}

// Array operator delete - nothrow
void OP_DELETE_ARRAY( void *ptr, const std::nothrow_t& ) throw()
{	 
	if ( ptr ) 
	{
		RED_FREE( red::memory::PoolLegacy, ptr );
	}
}
