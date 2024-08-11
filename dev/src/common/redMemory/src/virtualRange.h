/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_VIRTUAL_RANGE_H_
#define _RED_MEMORY_VIRTUAL_RANGE_H_

namespace red
{
namespace memory
{
	struct VirtualRange
	{
		u64 start;
		u64 end;
	};

	VirtualRange NullVirtualRange();

	u64 GetVirtualRangeSize( const VirtualRange & range );

	RED_MEMORY_API bool operator==( const VirtualRange & left, const VirtualRange & right );
	RED_MEMORY_API bool operator!=( const VirtualRange & left, const VirtualRange & right );
}
}

#include "virtualRange.hpp"

#endif
