/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_BLOCK_H_
#define _RED_MEMORY_BLOCK_H_

namespace red
{
namespace memory
{
	struct Block
	{
		u64 address;	
		u64 size;
	};

	Block NullBlock();

	RED_MEMORY_API bool operator==( const Block & left, const Block & right );
	RED_MEMORY_API bool operator!=( const Block & left, const Block & right );
}
}

#include "block.hpp"

#endif
