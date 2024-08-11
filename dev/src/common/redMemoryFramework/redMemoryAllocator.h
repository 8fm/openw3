/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_ALLOCATOR_H
#define _RED_MEMORY_FRAMEWORK_ALLOCATOR_H
#pragma once

#include "../redSystem/utility.h"
#include "redMemoryFrameworkTypes.h"

namespace Red { namespace MemoryFramework {

	class AllocatorInfo;
	class AllocatorWalker;
	class PoolAreaWalker;
	class AllocatorAreaCallback;
	class IAllocator;

	// Derive new IAllocatorCreationParameters for each allocator type to pass init options
	class RED_PURE_INTERFACE(IAllocatorCreationParameters)
	{
	protected:
		IAllocatorCreationParameters() {}
		virtual ~IAllocatorCreationParameters() {}
	};

	// Allocator base class
	class IAllocator : protected Red::System::NonCopyable
	{
	public:
		IAllocator();
		virtual ~IAllocator();

		// Creation / destruction
		virtual EAllocatorInitResults Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags ) = 0;
		virtual void Release( ) = 0;

		// Allocate more system memory if required
		virtual Red::System::Bool IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired ) = 0;

		// Release free memory to the operating system
		virtual Red::System::MemSize ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback ) = 0;

		// Allocator / pool information
		virtual void RequestAllocatorInfo( AllocatorInfo& info ) = 0;

		// Walk each large area of the allocator
		virtual void WalkAllocator( AllocatorWalker* theWalker ) = 0;

		// Walk all allocations in a particular area
		virtual void WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker ) = 0;

		// Generic allocator flags
		RED_INLINE Red::System::Uint32 GetFlags() const			{ return m_flags; }
		RED_INLINE void SetFlags( Red::System::Uint32 flags )		{ m_flags = flags; }

		// This is called on OOM. Used to add additional debug info to the crash logs
		virtual void OnOutOfMemory() = 0;

		// Required allocations functions. DO Not use deep inheritance as we want to avoid vtable jumps whereever we can!
		virtual void* RuntimeAllocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass ) = 0;
		virtual void* RuntimeReallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass ) = 0;
		virtual EAllocatorFreeResults RuntimeFree( void* ptr ) = 0;
		virtual Red::System::MemSize RuntimeGetAllocationSize( void* ptr ) const = 0;
		virtual Red::System::Bool RuntimeOwnsPointer( void* ptr ) const = 0;

		// Output extended debug info to log
		virtual void DumpDebugOutput() = 0;

	protected:
		Red::System::Uint32	m_flags;					// Generic allocator flags used by the MemoryManager
	};

} } // namespace Red { namespace MemoryFramework {

#endif // RED_MEMORY_FRAMEWORK_ALLOCATOR_H