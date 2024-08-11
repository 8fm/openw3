/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_SLAB_LIST_H_
#define _RED_MEMORY_SLAB_LIST_H_

#include "intrusiveList.h"

namespace red
{
namespace memory
{
	typedef IntrusiveDoubleLinkedList SlabList;
	typedef IntrusiveSingleLinkedList SlabFreeList;
}
}

#endif
