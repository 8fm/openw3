/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_NETWORK_MEMORY_H_
#define _RED_NETWORK_MEMORY_H_

namespace Red
{
	namespace Network
	{
		namespace Memory
		{
			typedef void* (*ReallocatorFunc)( void* ptr, size_t size );
			typedef void (*FreeFunc)( void* ptr );

			void* Realloc( void* ptr, size_t size );
			void Free( void* ptr );

			void Set( ReallocatorFunc allocFunc, FreeFunc freeFunc );
		}
	}
}

#endif // _RED_NETWORK_MEMORY_H_
