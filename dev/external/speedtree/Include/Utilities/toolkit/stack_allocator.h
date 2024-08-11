/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef _SCE_GNM_TOOLKIT_STACK_ALLOCATOR_H_
#define _SCE_GNM_TOOLKIT_STACK_ALLOCATOR_H_

#include "allocator.h"
#include <sys/dmem.h>

namespace sce
{
	namespace Gnmx
	{
		namespace Toolkit
		{
			struct StackAllocator
			{
				enum {kMaximumAllocations = 8192};
				uint8_t *m_allocation[kMaximumAllocations];
				uint32_t m_allocations;
				uint8_t *m_base;
				uint32_t m_alignment;
				off_t m_offset;
				size_t m_size;
				off_t m_top;
				SceKernelMemoryType m_type;
				void init(SceKernelMemoryType type, uint32_t size);
				void *allocate(uint32_t size, uint32_t alignment);
				void *allocate(Gnm::SizeAlign sizeAlign)
				{
					return allocate(sizeAlign.m_size, sizeAlign.m_align);
				}

				SCE_GNM_API_CHANGED
				void *allocate(uint32_t size, uint32_t alignment, SceKernelMemoryType type)
				{
					SCE_GNM_UNUSED(type);
					return allocate(size, alignment);
				}

				SCE_GNM_API_CHANGED
				void *allocate(Gnm::SizeAlign sizeAlign, SceKernelMemoryType type)
				{
					SCE_GNM_UNUSED(type);
					return allocate(sizeAlign.m_size, sizeAlign.m_align);
				}

				void release(void* pointer);
				void deinit();
			};

			IAllocator GetInterface(StackAllocator *stackAllocator);
		}
	}
}

#endif /* _SCE_GNM_TOOLKIT_STACK_ALLOCATOR_H_ */
