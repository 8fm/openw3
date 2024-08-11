/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __RED_ALLOCATOR_PROXY_H__
#define __RED_ALLOCATOR_PROXY_H__

#include "../../redSystem/os.h"
#include "../../redSystem/compilerExtensions.h"
#include "../../redSystem/error.h"

namespace Red
{
	class AllocatorProxy
	{
	public:
		typedef void* (*ReallocatorFunc)( void* ptr, size_t size );
		typedef void (*FreeFunc)( void* ptr );

	public:
		AllocatorProxy( ReallocatorFunc afunc, FreeFunc ffunc );
		AllocatorProxy( const AllocatorProxy* other );
		AllocatorProxy( const AllocatorProxy& other );
		~AllocatorProxy();

		RED_INLINE void* Realloc( void* ptr, size_t size )
		{
			RED_ASSERT( m_realloc, TXT( "Cannot Allocate using an uninitialised AllocatorProxy" ) );
			return m_realloc( ptr, size );
		}

		RED_INLINE void Free( void* ptr )
		{
			RED_ASSERT( m_realloc, TXT( "Cannot Free using an uninitialised AllocatorProxy" ) );
			m_free( ptr );
		}

	private:
		ReallocatorFunc m_realloc;
		FreeFunc m_free;
	};
}

#endif // __RED_ALLOCATOR_PROXY_H__
