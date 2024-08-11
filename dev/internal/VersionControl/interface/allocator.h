/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __VERCON_ALLOCATOR_H__
#define __VERCON_ALLOCATOR_H__

#include <cstdlib>

namespace VersionControl
{
	class Allocator
	{
	public:
		typedef void* (*AllocatorFunc)( size_t size );
		typedef void (*FreeFunc)( void* );

		static void SetCustom( AllocatorFunc afunc, FreeFunc ffunc );

		static void* Malloc( size_t size )
		{
			if( m_alloc )
			{
				return m_alloc( size );
			}

			return malloc( size );
		}

		static void Free( void* ptr )
		{
			if( m_free )
			{
				m_free( ptr );
			}

			free( ptr );
		}

	private:
		static AllocatorFunc m_alloc;
		static FreeFunc m_free;
	};
}

#endif // __VERCON_ALLOCATOR_H__
