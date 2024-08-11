/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../include/utils.h"
#include "utils.h"
#include "block.h"
#include "defaultAllocator.h"
#include <xutility>

namespace red
{
namespace memory
{
	void RegisterCurrentThread()
	{
		AcquireDefaultAllocator().RegisterCurrentThread();
	}

	void MemcpyBlock( u64 destination, u64 source, u64 size )
	{
		const void * sourceAddr = reinterpret_cast< const void* >( source );
		void * destinationAddr = reinterpret_cast< void* >( destination );
		Memcpy( destinationAddr, sourceAddr, size );
	}

	void MemcpyBlock( Block & destination, const Block & source )
	{
		const void * sourceAddr = reinterpret_cast< const void* >( source.address );
		void * destinationAddr = reinterpret_cast< void* >( destination.address );
		Memcpy( destinationAddr, sourceAddr, std::min( destination.size, source.size ) );
	}
}
}
