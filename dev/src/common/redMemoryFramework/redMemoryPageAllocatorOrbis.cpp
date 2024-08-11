/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redMemoryPageAllocatorOrbis.h"
#include "redMemoryFrameworkTypes.h"
#include "redMemoryAssert.h"
#include <kernel.h>

namespace Red { namespace MemoryFramework { namespace OrbisAPI {

const Red::System::MemSize c_MemoryPageSize = 16 * 1024;			// 16k page size

/////////////////////////////////////////////////////////////////////
// CTor
//
PageAllocatorImpl::PageAllocatorImpl()
	: m_directMemoryPagesAllocated( 0 )
	, m_flexibleMemoryPagesAllocated( 0 )
{
}

/////////////////////////////////////////////////////////////////////
// DTor
//
PageAllocatorImpl::~PageAllocatorImpl()
{
}

/////////////////////////////////////////////////////////////////////
// IsExtraDebugMemoryAvailable
//	Returns true if there is > 5gb memory available 
Red::System::Bool PageAllocatorImpl::IsExtraDebugMemoryAvailable()
{
	Red::System::MemSize directMemoryAvailable = sceKernelGetDirectMemorySize();
	return ( directMemoryAvailable > ( 1024ull * 1024ull * 1024ull * 5ull ) );
}

/////////////////////////////////////////////////////////////////////
// GetDirectMemoryAllocated
//
Red::System::MemSize PageAllocatorImpl::GetDirectMemoryAllocated()
{
	return m_directMemoryPagesAllocated * c_MemoryPageSize;
}

/////////////////////////////////////////////////////////////////////
// GetFlexibleMemoryAllocated
//
Red::System::MemSize PageAllocatorImpl::GetFlexibleMemoryAllocated()
{
	return m_flexibleMemoryPagesAllocated * c_MemoryPageSize;
}

/////////////////////////////////////////////////////////////////////
// ValidateFlags
//	Returns false if the combination of flags is incompatible with Orbis
Red::System::Bool PageAllocatorImpl::ValidateFlags( Red::System::Uint32 flags )
{
	Red::System::Bool isDirectMemory = flags & ::Red::MemoryFramework::Allocator_DirectMemory;
	Red::System::Bool isFlexibleMemory = flags & ::Red::MemoryFramework::Allocator_FlexibleMemory;
	Red::System::Bool isOnionBus = flags & ::Red::MemoryFramework::Allocator_UseOnionBus;
	Red::System::Bool isGarlicBus = flags & ::Red::MemoryFramework::Allocator_UseGarlicBus;

	if( ( isDirectMemory ^ isFlexibleMemory ) == 0 )
	{
		RED_MEMORY_HALT( "Pool must be specified as using either Direct or Flexible memory" );
		return false;
	}

	if( ( isGarlicBus ^ isOnionBus ) == 0 )	
	{
		RED_MEMORY_HALT( "Pool must be specified as using either Garlic or Onion bus" );
		return false;
	}

	if( isFlexibleMemory && isGarlicBus )		// Flexible memory can only be accessed via the Onion (CPU) memory bus
	{
		RED_MEMORY_HALT( "Flexible memory can only use the Onion bus" );
		return false;
	}

	// Make sure at least 1 read/write type is specified
	Red::System::Uint32 readWriteMask = ::Red::MemoryFramework::Allocator_AccessCpuReadWrite | ::Red::MemoryFramework::Allocator_AccessGpuReadWrite;
	if( ( flags & readWriteMask ) == 0 )
	{
		RED_MEMORY_HALT( "You must specify read/write permissions" );
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////
// Initialise 
Red::System::Bool PageAllocatorImpl::Initialise()
{
	return true;
}

/////////////////////////////////////////////////////////////////////
// GetPageSize
//	Get the size of a single page
Red::System::MemSize PageAllocatorImpl::GetPageSize()
{
	return c_MemoryPageSize;
}

/////////////////////////////////////////////////////////////////////
// GetAllocatedPages
//	Returns the num. pages allocated
Red::System::MemSize PageAllocatorImpl::GetAllocatedPages()
{
	return m_directMemoryPagesAllocated + m_flexibleMemoryPagesAllocated;
}

/////////////////////////////////////////////////////////////////////
// GetNewPage
//	Request a single page of memory
void* PageAllocatorImpl::GetNewPage( Red::System::Uint32 flags )
{
	if( !ValidateFlags( flags ) )
	{
		return nullptr;
	}

	Red::System::MemSize actualAllocated = 0;
	if( flags & ::Red::MemoryFramework::Allocator_DirectMemory )
	{
		return AllocateDirectMemory( c_MemoryPageSize, flags, actualAllocated );
	}
	else if( flags & ::Red::MemoryFramework::Allocator_FlexibleMemory )
	{
		return AllocateFlexibleMemory( c_MemoryPageSize, flags, actualAllocated );
	}

	return nullptr;
}

/////////////////////////////////////////////////////////////////////
// GetPagedMemory
//	Request some continuous paged memory
void* PageAllocatorImpl::GetPagedMemory( Red::System::MemSize size, Red::System::MemSize& actualSize, Red::System::Uint32 flags )
{
	Red::System::MemSize sizeRoundedToPageSize = (size % c_MemoryPageSize == 0) ? size : size + c_MemoryPageSize - (size % c_MemoryPageSize);

	if( !ValidateFlags( flags ) )
	{
		return nullptr;
	}

	if( flags & ::Red::MemoryFramework::Allocator_DirectMemory )
	{
		return AllocateDirectMemory( sizeRoundedToPageSize, flags, actualSize );
	}
	else if( flags & ::Red::MemoryFramework::Allocator_FlexibleMemory )
	{
		return AllocateFlexibleMemory( sizeRoundedToPageSize, flags, actualSize );
	}

	return nullptr;
}

/////////////////////////////////////////////////////////////////////
// FreePagedMemory
//	Free the allocated memory
void PageAllocatorImpl::FreePagedMemory( void* ptr, Red::System::MemSize size, Red::System::Uint32 flags )
{
	if( !ValidateFlags( flags ) )
	{
		return;
	}

	Red::System::MemSize sizeRoundedToPageSize = (size % c_MemoryPageSize == 0) ? size : size + c_MemoryPageSize - (size % c_MemoryPageSize);

	if( flags & ::Red::MemoryFramework::Allocator_DirectMemory )
	{
		ReleaseDirectMemory( ptr, sizeRoundedToPageSize );
	}
	else if( flags & ::Red::MemoryFramework::Allocator_FlexibleMemory )
	{
		ReleaseFlexibleMemory( ptr, sizeRoundedToPageSize );
	}
}

/////////////////////////////////////////////////////////////////////
// AllocateDirectMemory
//	Allocate direct memory and map it to virtual address space
void* PageAllocatorImpl::AllocateDirectMemory( Red::System::MemSize size, Red::System::Uint32 flags, Red::System::MemSize& actualSize )
{
	off_t foundAreaAddress = 0;
	Red::System::Int32 memoryType = 0;
	memoryType |= ( flags & ::Red::MemoryFramework::Allocator_UseOnionBus ) ? SCE_KERNEL_WB_ONION : 0;
	memoryType |= ( flags & ::Red::MemoryFramework::Allocator_UseGarlicBus ) ? SCE_KERNEL_WC_GARLIC : 0;

	const size_t directMemoryAlignment = ( flags & ::Red::MemoryFramework::Allocator_TextureMemory ) ? 65536 : 0;
	const size_t mappedMemoryAlignment = ( flags & ::Red::MemoryFramework::Allocator_TextureMemory ) ? 65536 : 0;


	// First allocate the physical memory
	Red::System::Int32 result = sceKernelAllocateDirectMemory( 0,								// Search the entire physical memory space
															   SCE_KERNEL_MAIN_DMEM_SIZE,		//  ...
															   size,
															   directMemoryAlignment,			// Zero lets the system align the area (it will be at least 16k)
															   memoryType,
															   &foundAreaAddress );
	if( result != SCE_OK )
	{
		if( result == SCE_KERNEL_ERROR_EINVAL )
		{
			RED_MEMORY_HALT( "Invalid parameter passed to sceKernelAllocateDirectMemory" );
		}
		else
		{
			RED_MEMORY_HALT( "Failed to allocate Direct memory, probably out of memory" );
		}

		return nullptr;
	}

	// Now it must be mapped so the cpu / gpu can access it
	void* virtualAddress = nullptr;
	Red::System::Int32 memProtection = 0;
	memProtection |= ( flags & ::Red::MemoryFramework::Allocator_AccessCpuRead ) ? SCE_KERNEL_PROT_CPU_READ : 0;
	memProtection |= ( flags & ::Red::MemoryFramework::Allocator_AccessCpuWrite ) ? SCE_KERNEL_PROT_CPU_WRITE : 0;
	memProtection |= ( flags & ::Red::MemoryFramework::Allocator_AccessGpuRead ) ? SCE_KERNEL_PROT_GPU_READ : 0;
	memProtection |= ( flags & ::Red::MemoryFramework::Allocator_AccessGpuWrite ) ? SCE_KERNEL_PROT_GPU_WRITE : 0;
	result = sceKernelMapDirectMemory( &virtualAddress, size, memProtection, SCE_KERNEL_MAP_NO_OVERWRITE, foundAreaAddress, mappedMemoryAlignment );
	if( result != SCE_OK )
	{
		if( result == SCE_KERNEL_ERROR_EACCES )
		{
			RED_MEMORY_HALT( "Access is prohibited for the area specified as the map destination" );
		}
		else if( result == SCE_KERNEL_ERROR_EINVAL )
		{
			RED_MEMORY_HALT( "Invalid parameters passed to sceKernelMapDirectMemory" );
		}
		else
		{
			RED_MEMORY_HALT( "Failed to find free space to allocate direct memory. System out of memory!" );
		}
		ReleaseUnmappedDirectMemory( static_cast< Red::System::MemUint >( foundAreaAddress ), size );
		return nullptr;
	}

	m_directMemoryPagesAllocated += size / c_MemoryPageSize;
	actualSize = size;

	return virtualAddress;
}

/////////////////////////////////////////////////////////////////////
// AllocateFlexibleMemory
//	Allocate flexible memory and map it to virtual address space
void* PageAllocatorImpl::AllocateFlexibleMemory( Red::System::MemSize size, Red::System::Uint32 flags, Red::System::MemSize& actualSize )
{
	void* startSearchAddress = 0;			// Start searching from address 0 for available memory
	Red::System::Int32 allocFlags = SCE_KERNEL_MAP_NO_OVERWRITE;	// Since we are searching for free addresses, stop overlapping address space from being mapped
	Red::System::Int32 memProtection = 0;
	memProtection |= ( flags & ::Red::MemoryFramework::Allocator_AccessCpuRead ) ? SCE_KERNEL_PROT_CPU_READ : 0;
	memProtection |= ( flags & ::Red::MemoryFramework::Allocator_AccessCpuWrite ) ? SCE_KERNEL_PROT_CPU_WRITE : 0;
	memProtection |= ( flags & ::Red::MemoryFramework::Allocator_AccessGpuRead ) ? SCE_KERNEL_PROT_GPU_READ : 0;
	memProtection |= ( flags & ::Red::MemoryFramework::Allocator_AccessGpuWrite ) ? SCE_KERNEL_PROT_GPU_WRITE : 0;
	Red::System::Int32 result = sceKernelMapFlexibleMemory( &startSearchAddress, size, memProtection, allocFlags );
	if( result != SCE_OK )
	{
		if( result == SCE_KERNEL_ERROR_EINVAL )
		{
			RED_MEMORY_HALT( "Invalid parameters passed to sceKernelMapFlexibleMemory" );
		}
		else
		{
			RED_MEMORY_HALT( "Failed to map flexible memory - SCE_KERNEL_ERROR_ENOMEM" );
		}
		return nullptr;
	}

	if( startSearchAddress != nullptr )
	{
		m_flexibleMemoryPagesAllocated += size / c_MemoryPageSize;
		actualSize = size;
	}

	return startSearchAddress;
}

/////////////////////////////////////////////////////////////////////
// ReleaseUnmappedDirectMemory
//	Release direct memory from its physical offset, not its virtual address
void PageAllocatorImpl::ReleaseUnmappedDirectMemory( Red::System::MemUint ptr, Red::System::MemSize size )
{
	Red::System::Int32 result = sceKernelReleaseDirectMemory( ptr, size );
	if( result == SCE_OK )
	{
		m_directMemoryPagesAllocated -= size / c_MemoryPageSize;
	}
	else
	{
		RED_MEMORY_HALT( "Invalid argument passed to sceKernelReleaseDirectMemory" );
	}
}

/////////////////////////////////////////////////////////////////////
// ReleaseDirectMemory
//	Release direct memory + unmap address space
void PageAllocatorImpl::ReleaseDirectMemory( void* ptr, Red::System::MemSize size )
{
	// In order to release direct memory based on its virtual address, it is necessary to
	// call virtualQuery in order to get the mapped physical offset
	SceKernelVirtualQueryInfo virtualMemoryInfo;
	Red::System::MemoryZero( &virtualMemoryInfo, sizeof( virtualMemoryInfo ) );

	Red::System::Int32 result = sceKernelVirtualQuery( ptr, 0, &virtualMemoryInfo, sizeof( virtualMemoryInfo ) );
	if( result != SCE_OK )
	{
		if( result == SCE_KERNEL_ERROR_EACCES )
		{
			RED_MEMORY_HALT( "Memory specified by ptr is not mapped" );
		}
		else if( result == SCE_KERNEL_ERROR_EINVAL )
		{
			RED_MEMORY_HALT( "Ptr address is out of virtual memory address space" );
		}
		else
		{
			RED_MEMORY_HALT( "An unknown error occurred while attempting to query a virtual address" );
		}
	}
	else
	{
		RED_MEMORY_ASSERT( virtualMemoryInfo.isDirectMemory,  "Memory is not mapped to direct memory!" );
		if( virtualMemoryInfo.isDirectMemory )
		{
			ReleaseUnmappedDirectMemory( virtualMemoryInfo.offset, size );
		}
	}
}

/////////////////////////////////////////////////////////////////////
// ReleaseFlexibleMemory
//	Release flexible memory + unmap address space
void PageAllocatorImpl::ReleaseFlexibleMemory( void* ptr, Red::System::MemSize size )
{
	Red::System::Int32 result = sceKernelMunmap( ptr, size );
	if( result != SCE_OK )
	{
		RED_MEMORY_ASSERT( size > 0, "Cannot unmap area of size 0" );
		RED_MEMORY_ASSERT( ( reinterpret_cast< Red::System::MemUint >( ptr ) % c_MemoryPageSize ) == 0,  "Attempting to free misaligned area" );
		RED_MEMORY_HALT( "ReleaseFlexibleMemory failed. This needs to be checked immediately!" );
	}
	else
	{
		m_flexibleMemoryPagesAllocated -= size / c_MemoryPageSize;
	}
}

/////////////////////////////////////////////////////////////////////
// AnnotateMemoryRegion
//	Label memory. Name = 32 characters max
void PageAllocatorImpl::AnnotateMemoryRegion( Red::System::MemUint address, Red::System::MemSize size, const Red::System::AnsiChar* name )
{
	int32_t result = sceKernelSetVirtualRangeName( reinterpret_cast< void* >( address ), size, name );
	RED_MEMORY_ASSERT( result == SCE_OK, "Bad parameters passed to AnnotateMemoryRegion" );
	RED_UNUSED( result );
}

/////////////////////////////////////////////////////////////////////
// ProtectMemoryRegion
//	Enable / disable r/w access for GPU or CPU to a region of memory
void PageAllocatorImpl::ProtectMemoryRegion( Red::System::MemUint address, Red::System::MemSize regionSize, Red::System::Uint32 poolFlags, Red::System::Bool allowCpuAccess )
{
	Red::System::Int32 memProtection = 0;	
	memProtection |= ( poolFlags & ::Red::MemoryFramework::Allocator_AccessGpuRead ) ? SCE_KERNEL_PROT_GPU_READ : 0;
	memProtection |= ( poolFlags & ::Red::MemoryFramework::Allocator_AccessGpuWrite ) ? SCE_KERNEL_PROT_GPU_WRITE : 0;

	if( allowCpuAccess )
	{
		memProtection |= ( poolFlags & ::Red::MemoryFramework::Allocator_AccessCpuWrite ) ? SCE_KERNEL_PROT_CPU_WRITE : 0;
		memProtection |= ( poolFlags & ::Red::MemoryFramework::Allocator_AccessCpuRead ) ? SCE_KERNEL_PROT_CPU_READ : 0;
	}

	int32_t result = sceKernelMprotect( reinterpret_cast< void* >( address ), regionSize, memProtection );
	RED_MEMORY_ASSERT( result == SCE_OK, "Failed to set protection flags for address. Address is probably invalid" );
	RED_UNUSED( result );
}

} } }
