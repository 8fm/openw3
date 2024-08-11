/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_DEBUG_ALLOCATOR_H
#define _RED_MEMORY_FRAMEWORK_DEBUG_ALLOCATOR_H
#pragma once

#include "../redSystem/types.h"
#include "redMemoryAllocator.h"
#include "redMemoryRegionAllocator.h"

namespace Red { namespace MemoryFramework {

	// DebugAllocator
	//	Use this to detect memory stomp
	class DebugAllocator : public IAllocator
	{
	public:
		class CreationParameters : public IAllocatorCreationParameters
		{
		public:
			CreationParameters()	{ }
			CreationParameters( Red::System::MemSize, Red::System::MemSize,	Red::System::MemSize )	{ }
		};

		DebugAllocator();
		virtual ~DebugAllocator();

		void					RequestAllocatorInfo( AllocatorInfo& info );
		EAllocatorInitResults	Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags );
		void					Release( );
		Red::System::MemSize	ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback );
		Red::System::Bool		IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired );

		// 'Static' functions
		void*	Allocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass );
		void*	Reallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass );
		EAllocatorFreeResults Free( const void* ptr );
		Red::System::MemSize GetAllocationSize( const void* ptr ) const;
		Red::System::Bool OwnsPointer( const void* ptr ) const;

		// Walk each large area of the allocator
		void WalkAllocator( AllocatorWalker* theWalker ) { RED_UNUSED( theWalker ); }

		// Walk all allocations in a particular area
		void WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker ) { RED_UNUSED( startAddress ); RED_UNUSED( size ); RED_UNUSED( theWalker ); }

		void OnOutOfMemory() { }

		void DumpDebugOutput() { }

		MemoryRegionHandle AllocateRegion( Red::System::MemSize size, Red::System::MemSize alignment, RegionLifetimeHint lifetimeHint );
		EAllocatorFreeResults FreeRegion( MemoryRegionHandle handle );

	private:

		// 'runtime' functions
		virtual void* RuntimeAllocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass )
		{
			return Allocate( allocSize, allocAlignment, allocatedSize, memoryClass );
		}

		virtual void* RuntimeReallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass )
		{
			return Reallocate( ptr, allocSize, allocAlignment, allocatedSize, freedSize, memoryClass );
		}

		virtual EAllocatorFreeResults RuntimeFree( void* ptr )
		{
			return Free( ptr );
		}

		virtual Red::System::MemSize RuntimeGetAllocationSize( void* ptr ) const
		{
			return GetAllocationSize( ptr );
		}

		virtual Red::System::Bool RuntimeOwnsPointer( void* ptr ) const
		{
			return OwnsPointer( ptr );
		}
	};

} } // namespace Red { namespace MemoryFramework {

#endif // RED_MEMORY_FRAMEWORK_NULL_ALLOCATOR_H