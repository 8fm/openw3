/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "../redSystem/types.h"
#include "../redSystem/error.h"

#include "memory.h"

namespace Red
{
	namespace Network
	{
		namespace Memory
		{
			ReallocatorFunc	m_realloc	= nullptr;
			FreeFunc		m_free		= nullptr;

			void* Realloc( void* ptr, size_t size )
			{
				RED_ASSERT( m_realloc, TXT( "Red Network: Cannot Allocate using an uninitialised realloc" ) );
				return m_realloc( ptr, size );
			}

			void Free( void* ptr )
			{
				RED_ASSERT( m_realloc, TXT( "Red Network: Cannot Free using an uninitialised free" ) );
				m_free( ptr );
			}

			void Set( ReallocatorFunc allocFunc, FreeFunc freeFunc )
			{
				m_realloc = allocFunc;
				m_free = freeFunc;
			}

		}
	}
}
