/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "region_allocator.h"
#include <stdio.h>
using namespace sce;

#define GNM_MEMORY_STORE_INHIB_MACRO
#ifndef GNM_EARLY_LIMITATION
#define GNM_EARLY_LIMITATION
#endif
#define DISABLE_MEMORY_CLEAR

#define GNM_MEMORY_DEBUG_OVERFLOW_CHECK 0
#define GNM_MEMORY_DEBUG_ALIGNMENT_CHECK 0

// If enabled, GNM will record file and line number information for each memory allocation.
#define GNM_MEMORY_STORE_ALLOC_FILE 0

void sce::Gnmx::Toolkit::mapMemory(uint64_t* system, uint32_t* systemSize, uint64_t* gpuShared, uint32_t* gpuSharedSize, off_t *systemOffset, off_t *gpuOffset)
{
	SCE_GNM_ASSERT(system && systemSize && gpuShared && gpuSharedSize);
	//----------------------------------------------------------------------
	// Setup the memory pools:
#ifdef GNM_EARLY_LIMITATION
	const uint32_t	systemPoolSize	   = 1024 * 1024 * 256;
	const uint32_t	sharedGpuPoolSize  = 1024 * 1024 * 512;
#else // GNM_EARLY_LIMITATION
	const uint32_t	systemPoolSize	   = 1024 * 1024 * 1024;
	const uint32_t	sharedGpuPoolSize  = 1024 * 1024 * 192;
#endif // GNM_EARLY_LIMITATION

	const uint32_t  systemAlignment		= 2 * 1024 * 1024;
	const uint32_t  shaderGpuAlignment	= 2 * 1024 * 1024;

	off_t temp[2];
	off_t *systemPoolOffset = systemOffset ? systemOffset : &temp[0];
	off_t *sharedGpuPoolOffset = gpuOffset ? gpuOffset : &temp[1];

	void*	systemPoolPtr	  = NULL;
	void*	sharedGpuPoolPtr  = NULL;

	uint32_t result = SCE_GNM_OK;

	int retSys = sceKernelAllocateDirectMemory(0,
		SCE_KERNEL_MAIN_DMEM_SIZE,
		systemPoolSize,
		systemAlignment, // alignment
		SCE_KERNEL_WB_ONION,
		systemPoolOffset);
	if ( !retSys )
	{
		retSys = sceKernelMapDirectMemory(&systemPoolPtr,
			systemPoolSize,
			SCE_KERNEL_PROT_CPU_READ|SCE_KERNEL_PROT_CPU_WRITE|SCE_KERNEL_PROT_GPU_ALL,
			0,						//flags
			*systemPoolOffset,
			systemAlignment);
	}
	if ( retSys )
	{
		fprintf(stderr, "sce::Gnm::Initialize Error: System Memory Allocation Failed (%d)\n", retSys);
		result = SCE_GNM_ERROR_FAILURE;
	}


	//


	int retShr = sceKernelAllocateDirectMemory(0,
		SCE_KERNEL_MAIN_DMEM_SIZE,
		sharedGpuPoolSize,
		shaderGpuAlignment,	// alignment
		SCE_KERNEL_WC_GARLIC,
		sharedGpuPoolOffset);

	if ( !retShr )
	{
		retShr = sceKernelMapDirectMemory(&sharedGpuPoolPtr,
			sharedGpuPoolSize,
			SCE_KERNEL_PROT_CPU_READ|SCE_KERNEL_PROT_CPU_WRITE|SCE_KERNEL_PROT_GPU_ALL,
			0,						//flags
			*sharedGpuPoolOffset,
			shaderGpuAlignment);
	}
	if ( retShr )
	{
		fprintf(stderr, "sce::Gnm::Initialize Error: Shared Gpu Memory Allocation Failed (%d)\n", retShr);
		result = SCE_GNM_ERROR_FAILURE;
	}
	//

	if ( result == SCE_GNM_OK )
	{
		
		*gpuShared = (uint64_t)sharedGpuPoolPtr;
		*gpuSharedSize    = sharedGpuPoolSize;

		*system = (uint64_t)systemPoolPtr;
		*systemSize    = systemPoolSize;
	}
	
#ifndef DISABLE_MEMORY_CLEAR
	// clear all shared memory (including the memory we just used to clear private mem)
	// We clear to 0x7n because 0x7n7n7n7n is an obviously-invalid (Type-1) PM4 packet
	memset(system, 0x7d, systemSize);
	memset(gpuShared,  0x11, gpuSharedSize);
#endif // DISABLE_MEMORY_CLEAR

	fprintf(stderr,"Shared System size    : 0x%010lX\n", static_cast<uint64_t>(systemPoolSize));
	fprintf(stderr,"Shared System CPU Addr: 0x%010lX\n", uintptr_t(system));
	fprintf(stderr,"Shared Video size     : 0x%010lX\n", static_cast<uint64_t>(sharedGpuPoolSize));
	fprintf(stderr,"Shared Video CPU Addr : 0x%010lX\n", uintptr_t(gpuShared));
}


void sce::Gnmx::Toolkit::RegionAllocator::init(uint64_t memStart, uint32_t memSize, Region* regions, uint32_t numRegions)
{
	m_memoryStart = memStart;
	m_memoryEnd = memStart + memSize;
	m_allRegions = regions;
	m_numRegions = numRegions;
	m_freeRegions = NULL;
	m_usedRegions = NULL;

	memset(m_allRegions,0,sizeof(Toolkit::Region)*m_numRegions);
	if( memSize )
	{
		Region* r = findUnusedRegion();
		r->m_next = NULL;
		r->m_guard = 0;
		r->m_typeAndStart = m_memoryStart | kSystemRegionType;
		r->m_size = memSize;
		m_freeRegions = r;
		m_usedRegions = NULL;
	}
}

sce::Gnmx::Toolkit::Region* sce::Gnmx::Toolkit::RegionAllocator::findUnusedRegion() const
{
	for (uint32_t i = 0; i < m_numRegions; ++i)
	{
		Toolkit::Region *r = &m_allRegions[i];
		if ((r->m_typeAndStart & Toolkit::kRegionTypeMask) == Toolkit::kUnusedRegionType)
			return r;
	}
	return 0;
}

void* sce::Gnmx::Toolkit::RegionAllocator::allocate(uint32_t size, Gnm::AlignmentType alignment)
{
	Toolkit::Region *r = allocateRegion( size, kSystemRegionType, alignment);
	if (r != NULL)
	{
		uint32_t *start = (uint32_t *)uintptr_t(r->m_typeAndStart & kRegionStartMask);
		// We've had reports of allocate*Memory() returning blocks that aren't fully within GPU-visible RAM.
		// Let's catch that!
#if GNM_MEMORY_DEBUG_OVERFLOW_CHECK==1
		SCE_GNM_ASSERT(alignment == r->m_guard);

		// Note: end may not be aligned to a word size!!
		uintptr_t start_u8 = (uintptr_t)start;
		uintptr_t end_u8   = start_u8 + (size + alignment);
		uint32_t  *end     = (uint32_t*)end_u8;

		for (uint32_t i=0; i<alignment/4; ++i)
		{
			start[i] = 0xdea110c8;
			end[i] = 0xdea110c8;
		}
		start += alignment/4;
#endif
		return start;
	}
	else
		return NULL;
}

void sce::Gnmx::Toolkit::RegionAllocator::release(void* ptr)
{
	if(ptr)
	{
		uint64_t gpuAddr = uintptr_t(ptr);
		releaseRegion(gpuAddr, kSystemRegionType);
	}
}
sce::Gnmx::Toolkit::Region* sce::Gnmx::Toolkit::RegionAllocator::allocateRegion(uint32_t size, uint64_t type, uint32_t alignment)
{
	// must be power of 2
	SCE_GNM_ASSERT((alignment & (alignment-1)) == 0);

	uint32_t guard = 0;
#if GNM_MEMORY_DEBUG_OVERFLOW_CHECK==1
	guard = alignment;
	size += alignment * 2;
#endif

	Toolkit::Region *r, *rp = NULL;
	for (r = m_freeRegions; r; rp = r, r = r->m_next)
	{
		// the following code ensures that all allocations with alignment<512 are aligned to 512, skipped
		// forward by 1024, then skipped back by the requested alignment. this ensures MISALIGNMENT
		// to (alignment..512] which makes alignment-truncated addresses point as far backwards
		// into memory "landfill" as is practical.

		uint32_t miss;
#if GNM_MEMORY_DEBUG_ALIGNMENT_CHECK==1
		enum { kMaximumIntentionalMisalignment = 256 };
		if( alignment < kMaximumIntentionalMisalignment*2 )
		{
			const uint32_t begin = (uint32_t)(r->m_typeAndStart & Toolkit::kRegionStartMask);
			uint32_t end = begin;
			end &= ~(kMaximumIntentionalMisalignment*2-1); // align to 512
			end +=   kMaximumIntentionalMisalignment*2*2;  // skip forward by 1024
			end -= alignment; // subtract desired alignment
			miss = end - begin; // calculate difference
		}
		else
#endif
		{
			miss = static_cast<uint32_t>(r->m_typeAndStart & (alignment-1));
			if(miss != 0)
				miss = alignment - miss;
		}

		if (r->m_size >= size + miss)
		{
			if (miss != 0)
			{
				// Align the region start
				Toolkit::Region *n = findUnusedRegion();
				n->m_size = miss;
				n->m_typeAndStart = r->m_typeAndStart;
				n->m_next = r;
				n->m_guard = 0;
				SCE_GNM_ASSERT((r->m_typeAndStart & Toolkit::kRegionStartMask) + miss <= Toolkit::kRegionStartMask); // optimization check
				r->m_typeAndStart += miss;
				r->m_size -= miss;
				if (rp)
					rp->m_next = n;
				else
					m_freeRegions = n;
				rp = n;
			}

			uint64_t off = r->m_typeAndStart & Toolkit::kRegionStartMask;
			SCE_GNM_ASSERT((r->m_typeAndStart & Toolkit::kRegionStartMask) + size <= Toolkit::kRegionStartMask); // optimization check
			r->m_typeAndStart += size;
			r->m_size -= size;
			SCE_GNM_ASSERT((off & (alignment-1)) == 0);

			if (r->m_size == 0)
			{
				// Unlink me
				if (rp)
					rp->m_next = r->m_next;
				else
					m_freeRegions = r->m_next;

				// Re-link as used region
				Toolkit::Region *u = r;
				u->m_next = m_usedRegions;
				u->m_size = size;
				u->m_typeAndStart = off | type;
				u->m_guard = guard;
				m_usedRegions = u;
				//printf("a %08x %08x %08x\n", u->m_typeAndStart, size, alignment);
				return u;
			}
			else
			{
				// Link as used region
				Toolkit::Region *u = findUnusedRegion();
				u->m_next = m_usedRegions;
				u->m_size = size;
				u->m_guard = guard;
				u->m_typeAndStart = off | type;
				m_usedRegions = u;
				//printf("b %08x %08x %08x\n", u->m_typeAndStart, size, alignment);
				return u;
			}
		}
	}

	return NULL;
}

void sce::Gnmx::Toolkit::RegionAllocator::releaseRegion(uint64_t start, uint64_t type)
{
	// Find the region in the used list
	Toolkit::Region *r, *rp = NULL;
	for (r = m_usedRegions; r; rp = r, r = r->m_next)
	{
		if ((r->m_typeAndStart & Toolkit::kRegionStartMask) == start)
		{
			SCE_GNM_ASSERT((r->m_typeAndStart & Toolkit::kRegionTypeMask) == type);// "Released with different type than allocated with! %08x %08x", r->m_typeAndStart & Toolkit::kRegionTypeMask, type
			//printf("f: %08x %08x\n", r->m_typeAndStart, r->m_size);

			// Unlink
			if (rp)
				rp->m_next = r->m_next;
			else
				m_usedRegions = r->m_next;

			uint64_t re = (r->m_typeAndStart & Toolkit::kRegionStartMask) + r->m_size;

			// Add to free list, find insertion point
			Toolkit::Region *f, *fp = NULL;
			for (f = m_freeRegions; f; fp = f, f = f->m_next)
			{
				if ((f->m_typeAndStart & Toolkit::kRegionStartMask) > start)
				{
					// Merge nearby blocks?
					if (fp && ((fp->m_typeAndStart & Toolkit::kRegionStartMask) + fp->m_size == (r->m_typeAndStart & Toolkit::kRegionStartMask)))
					{
						//printf("p\n");
						// Merge with previous
						fp->m_size += r->m_size;
						r->m_typeAndStart &= Toolkit::kRegionStartMask;

						// Merge prev with next?
						if ((fp->m_typeAndStart & Toolkit::kRegionStartMask) + fp->m_size == (f->m_typeAndStart & Toolkit::kRegionStartMask))
						{
							//printf("pn\n");
							fp->m_size += f->m_size;
							fp->m_next = f->m_next;
							f->m_typeAndStart &= Toolkit::kRegionStartMask;
						}
					}
					else if (re == (f->m_typeAndStart & Toolkit::kRegionStartMask))
					{
						//printf("n\n");
						// Merge with next
						f->m_typeAndStart -= r->m_size;
						f->m_size += r->m_size;
						r->m_typeAndStart &= Toolkit::kRegionStartMask;
					}
					else
					{
						// Insert
						r->m_next = f;
						if (fp)
							fp->m_next = r;
						else
							m_freeRegions = r;
					}
					break;
				}
			}

			if (f == NULL)
			{
				// Add to end
				r->m_next = NULL;
				if (fp)
					fp->m_next = r;
				else
					m_freeRegions = r;
			}

			return;
		}
	}

	SCE_GNM_ASSERT(!"Pointer Not Allocated With GNM");
}

namespace
{
	void *allocate(void *instance, uint32_t size, Gnm::AlignmentType alignment)
	{
		return static_cast<Gnmx::Toolkit::RegionAllocator*>(instance)->allocate(size, alignment);
	}
	void release(void *instance, void *pointer)
	{
		static_cast<Gnmx::Toolkit::RegionAllocator*>(instance)->release(pointer);
	}
};

Gnmx::Toolkit::IAllocator Gnmx::Toolkit::GetInterface(Toolkit::RegionAllocator *regionAllocator)
{
	Toolkit::IAllocator allocator;
	allocator.m_instance = regionAllocator;
	allocator.m_allocate = allocate;
	allocator.m_release = release;
	return allocator;
}

