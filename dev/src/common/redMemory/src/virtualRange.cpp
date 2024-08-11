
/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "virtualRange.h"

namespace red
{
namespace memory
{
	bool operator==( const VirtualRange & left, const VirtualRange & right )
	{
		return left.start == right.start && left.end == right.end;
	}

	bool operator!=( const VirtualRange & left, const VirtualRange & right )
	{
		return !( left == right );
	}
}
}
