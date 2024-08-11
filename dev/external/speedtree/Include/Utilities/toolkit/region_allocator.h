/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef _SCE_GNM_TOOLKIT_REGION_ALLOCATOR_H
#define _SCE_GNM_TOOLKIT_REGION_ALLOCATOR_H

#include <gnm/common.h>
#include <sys/dmem.h>
#include <kernel.h>
#include "allocator.h"
#include "memory.h"

namespace sce
{
	namespace Gnmx
	{
		namespace Toolkit
		{
			static const uint64_t kRegionTypeMask = 0xE000000000000000ULL;
			static const uint64_t kRegionStartMask = 0x1FFFFFFFFFFFFFFFULL;

			static const uint64_t kUnusedRegionType      = 0x0000000000000000ULL;
			static const uint64_t kSystemRegionType      = 0x2000000000000000ULL;
			static const uint64_t kSharedVideoRegionType = 0x4000000000000000ULL;
			static const uint64_t kPrivateVideoRegionType= 0x8000000000000000ULL;

			class Region
			{
			public:
				uint64_t m_typeAndStart;
				uint32_t m_size;
				uint32_t m_guard;
				Region *m_next;

		#if GNM_MEMORY_STORE_ALLOC_FILE
				const char *m_file;
				uint32_t    m_line;
		#endif // GNM_MEMORY_STORE_ALLOC_FILE
			};

			class RegionAllocator 
			{
				//SceKernelMemoryType m_type;
				Region *m_freeRegions;
				Region *m_usedRegions;
				Region *m_allRegions;
				uint32_t m_numRegions;
				uint64_t m_memoryStart;
				uint64_t m_memoryEnd;

				Region* findUnusedRegion() const;
				Region* allocateRegion(uint32_t size, uint64_t type, uint32_t alignment);
				void	releaseRegion(uint64_t start, uint64_t type);
			public:
				void	init(uint64_t memStart, uint32_t memSize, Region* regions, uint32_t numRegions);

				SCE_GNM_API_CHANGED
				void	init(uint64_t memStart, uint32_t memSize, Region* regions, uint32_t numRegions, SceKernelMemoryType type)
				{
					SCE_GNM_UNUSED(type);
					init(memStart, memSize, regions, numRegions);
				}

				SCE_GNM_API_CHANGED
				void*	allocate(uint32_t size, Gnm::AlignmentType alignment, SceKernelMemoryType type)
				{
					SCE_GNM_UNUSED(type);
					return allocate(size, alignment);
				}
				SCE_GNM_API_CHANGED
				void*	allocate(sce::Gnm::SizeAlign sz, SceKernelMemoryType type) 
				{ 
					return allocate(sz.m_size, sz.m_align, type); 
				}

				void*	allocate(uint32_t size, Gnm::AlignmentType alignment);
				void*	allocate(sce::Gnm::SizeAlign sz) { return allocate(sz.m_size, sz.m_align); }
				void	release(void*);
			};

			/** @brief Maps system and shared memory.
			@param system         The base system memory address.
			@param systemSize     The size of the system memory window.
			@param gpuShared      The base shared video memory address.
			@param gpuSharedSize  The size of the shared video memory window.
			@param systemOffset   The offset of the system memory, for use in deallocation.
			@param gpuOffset      The offset of the gpu memory, for use in deallocation.
			*/
			void mapMemory(uint64_t *system, uint32_t *systemSize, uint64_t *gpuShared, uint32_t *gpuSharedSize, off_t *systemOffset = 0, off_t *gpuOffset = 0);

		#if GNM_MEMORY_STORE_ALLOC_FILE
		#ifndef GNM_MEMORY_STORE_INHIB_MACRO
		#define allocateSystemSharedMemory(...)  recordFileNameAndLineInRegion(Toolkit::allocateSystemSharedMemory(__VA_ARGS__), __FILE__, __LINE__)
		#define allocateVideoSharedMemory(...)   recordFileNameAndLineInRegion(Toolkit::allocateVideoSharedMemory(__VA_ARGS__), __FILE__, __LINE__)
		#endif // GNM_MEMORY_STORE_INHIB_MACRO

		#endif // GNM_MEMORY_STORE_ALLOC_FILE

			IAllocator GetInterface(RegionAllocator *regionAllocator);

			typedef RegionAllocator Allocator; // THIS EXISTS TEMPORARILY, AND ONLY BECAUSE THE OLD "Allocator" IS NOW CALLED "RegionAllocator"
		}
	}
}
#endif // _SCE_GNM_TOOLKIT_REGION_ALLOCATOR_H
