/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_BLOCK_HPP_
#define _RED_MEMORY_BLOCK_HPP_

namespace red
{
namespace memory
{
	RED_MEMORY_INLINE Block NullBlock()
	{
		Block invalid = {};
		return invalid;
	}
}
}

#endif
