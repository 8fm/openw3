/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_SYSTEM_BLOCK_H_
#define _RED_MEMORY_SYSTEM_BLOCK_H_

#include "../include/utils.h"

namespace red
{
namespace memory
{
	// Currently, 16k for Orbis, 4K PC, 64k Durango... So we go with Durango number unfortunately.
	const u64 c_systemBlockSizeFactor = RED_KILO_BYTE( 64 ); 

	struct SystemBlock
	{
		u64 address;
		u64 size;
	};

	SystemBlock NullSystemBlock();

	RED_MEMORY_API bool operator==( const SystemBlock & left, const SystemBlock & right );
	bool operator!=( const SystemBlock & left, const SystemBlock & right );
}
}


#endif

