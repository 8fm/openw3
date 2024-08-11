/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_VIRTUAL_RANGE_HPP_
#define _RED_MEMORY_VIRTUAL_RANGE_HPP_

namespace red
{
namespace memory
{
	RED_MEMORY_INLINE VirtualRange NullVirtualRange()
	{
		VirtualRange range = { 0, 0 };
		return range;
	}

	RED_MEMORY_INLINE u64 GetVirtualRangeSize( const VirtualRange & range )
	{
		return range.end - range.start;
	}
}
}

#endif
