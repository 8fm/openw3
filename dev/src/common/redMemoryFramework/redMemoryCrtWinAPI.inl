/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_CRT_WINAPI_INL
#define _RED_MEMORY_FRAMEWORK_CRT_WINAPI_INL
#pragma once

// Switch between debug crt allocator and standard malloc
#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#else
	#include <malloc.h>
#endif

namespace Red { namespace MemoryFramework { namespace WinAPI {

inline void* AlignedMalloc( Red::System::MemSize size, Red::System::MemSize alignment )				{ return ::_aligned_malloc( size, alignment ); }
inline void AlignedFree( void* ptr )																{ ::_aligned_free( ptr ); }
inline void* AlignedRealloc( void* ptr, Red::System::MemSize size, Red::System::Uint32 alignment )	{ return ::_aligned_realloc( ptr, size, alignment ); }

} } } // namespace Red { namespace MemoryFramework { WinAPI

#endif // RED_MEMORY_FRAMEWORK_CRT_WINAPI_INL