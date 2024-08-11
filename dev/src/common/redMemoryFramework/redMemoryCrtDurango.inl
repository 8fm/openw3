/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_CRT_DURANGO_INL
#define _RED_MEMORY_FRAMEWORK_CRT_DURANGO_INL
#pragma once

namespace Red { namespace MemoryFramework { namespace DurangoAPI {

inline void* AlignedMalloc( Red::System::MemSize size, Red::System::MemSize alignment )				{ return malloc( size ); RED_UNUSED( alignment ); }
inline void AlignedFree( void* ptr )																{ free( ptr ); }
inline void* AlignedRealloc( void* ptr, Red::System::MemSize size, Red::System::Uint32 alignment )	{ return realloc( ptr, size ); RED_UNUSED( alignment ); }

} } }

#endif