/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_SYSTEM_ALLOCATOR_ORBIS_HELPER_H_
#define _RED_MEMORY_SYSTEM_ALLOCATOR_ORBIS_HELPER_H_

#include "systemBlock.h"

namespace red { namespace memory { struct VirtualRange; } }

namespace red
{
namespace memory
{
namespace internal
{
	SystemBlock CommitBlockToDirectMemory( const SystemBlock & block, u32 flags );
	SystemBlock CommitBlockToFlexibleMemory( const SystemBlock & block, u32 flags );

	u64 DecommitRange( const VirtualRange & range );
	void DecommitBlock( const SystemBlock & block );
}
}
}

#endif
