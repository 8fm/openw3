/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "memory.h"

ReallocatorFunc g_allocFunc = nullptr;
FreeFunc g_freeFunc = nullptr;

void DDI::SetAllocators( ReallocatorFunc reallocFunc, FreeFunc freeFunc )
{
	g_allocFunc = reallocFunc;
	g_freeFunc = freeFunc;
}

void* DDI::Realloc( void* ptr, size_t size )
{
	return g_allocFunc( ptr, size );
}

void DDI::Free( const void* ptr )
{
	g_freeFunc( ptr );
}
