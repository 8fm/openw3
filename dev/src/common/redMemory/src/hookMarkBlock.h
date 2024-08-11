/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_HOOK_MARK_BLOCK_H_
#define _RED_MEMORY_HOOK_MARK_BLOCK_H_

namespace red
{
namespace memory
{
	struct HookPreParameter;
	struct HookPostParameter;

	void MarkFreeBlock( HookPreParameter & param, void* userdata );
	void MarkAllocatedBlock( HookPostParameter & param, void* userdata );
}
}

#endif
