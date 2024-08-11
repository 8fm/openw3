/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "systemBlock.h"

namespace red
{
namespace memory
{
	SystemBlock NullSystemBlock()
	{
		SystemBlock nullBlock = { 0, 0 };
		return nullBlock;
	}

	bool operator==( const SystemBlock & left, const SystemBlock & right )
	{
		return left.address == right.address && left.size == right.size;
	}

	bool operator!=( const SystemBlock & left, const SystemBlock & right )
	{
		return !( left == right );
	}
}
}
