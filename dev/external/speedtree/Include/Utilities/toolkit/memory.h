/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef _SCE_GNM_TOOLKIT_MEMORY_H
#define _SCE_GNM_TOOLKIT_MEMORY_H

#include <gnm/common.h>
#include <gnm/error.h>

namespace sce
{
	namespace Gnmx
	{
		namespace Toolkit
		{
			// Helper structure to encapsulate a block of CPU/GPU shared memory
			class MemBlock
			{
			public:
				uint32_t *m_ptr; //! CPU-addressible pointer
				uint32_t  m_size;  //! Size of the block (in bytes)
				uint64_t  m_addr; //! 40-bit GPU-addressible address

			};
			//SCE_GNM_API_DEPRECATED_CLASS_MSG("Unified memory architecture makes this class obsolete")
		}
	}
}

#include "region_allocator.h" // THIS IS ONLY TO SUPPORT THE DEPRECATED Toolkit::Allocator TYPE

#endif // _SCE_GNM_TOOLKIT_MEMORY_H
