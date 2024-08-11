/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "block.h"

namespace red
{
namespace memory
{
	bool operator==( const Block & left, const Block & right )
	{
		return left.address == right.address && left.size == right.size;
	}

	bool operator!=( const Block & left, const Block & right )
	{
		return !( left == right );
	}
}
}
