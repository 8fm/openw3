/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef _SCE_GNM_TOOLKIT_ALLOCATOR_H_
#define _SCE_GNM_TOOLKIT_ALLOCATOR_H_

#include <gnm.h>

namespace sce
{
	namespace Gnmx
	{
		namespace Toolkit
		{
			struct IAllocator
			{
				void* m_instance;
				void *(*m_allocate)(void *instance, uint32_t size, sce::Gnm::AlignmentType alignment);
				void (*m_release)(void *instance, void *pointer);

				void *allocate(uint32_t size, sce::Gnm::AlignmentType alignment)
				{
					return m_allocate(m_instance, size, alignment);
				}
				SCE_GNM_API_CHANGED
				void *allocate(uint32_t size, sce::Gnm::AlignmentType alignment, int type)
				{
					SCE_GNM_UNUSED(type);
					return allocate(size, alignment);
				}

				void *allocate(sce::Gnm::SizeAlign sizeAlign) 
				{
					return m_allocate(m_instance, sizeAlign.m_size, sizeAlign.m_align);
				}
				SCE_GNM_API_CHANGED
				void *allocate(sce::Gnm::SizeAlign sizeAlign, int type)
				{
					SCE_GNM_UNUSED(type);
					return allocate(sizeAlign);
				}

				void release(void *pointer)
				{
					if(pointer != NULL)
						m_release(m_instance, pointer);
				}
			};
		}
	}
}

#endif /* _SCE_GNM_TOOLKIT_ALLOCATOR_H_ */
