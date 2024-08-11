/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_MULTI_ALLOCATOR_H
#define _RED_MEMORY_FRAMEWORK_MULTI_ALLOCATOR_H
#pragma once

#include "../redSystem/types.h"
#include "redMemoryAllocator.h"

namespace Red { namespace MemoryFramework {

	// MultiAllocator
	//	Routes allocations through two allocators, depending on the size requested
	class MultiAllocator : public IAllocator
	{
	public:
		// Small / large allocators must go through this interface
		typedef Bool ( *OwnershipFn )( const void* ptr );			// Test ownership of ptr
		typedef void* ( *AllocateFn )( Red::System::MemSize, Red::System::MemSize );
		typedef EAllocatorFreeResults ( *FreeFn )( const void* );	
		typedef Red::System::MemSize ( *MemorySizeFn )( const void* ptr );
		class CreationParameters : public IAllocatorCreationParameters
		{
		public:
			CreationParameters( OwnershipFn smallOwnershipFn, MemorySizeFn smallSizeFn, AllocateFn smallAllocateFn, FreeFn smallFreeFn,
								OwnershipFn largeOwnershipFn, MemorySizeFn largeSizeFn, AllocateFn largeAllocateFn, FreeFn largeFreeFn,
								Red::System::MemSize smallAllocationCutoff )
				: m_smallOwnershipFn( smallOwnershipFn )
				, m_smallSizeFn( smallSizeFn )
				, m_smallAllocateFn( smallAllocateFn )
				, m_smallFreeFn( smallFreeFn )
				, m_largeOwnershipFn( largeOwnershipFn )
				, m_largeSizeFn( largeSizeFn )
				, m_largeAllocateFn( largeAllocateFn )
				, m_largeFreeFn( largeFreeFn )
				, m_smallAllocationCutoff( smallAllocationCutoff )
			{
			}
			CreationParameters() : m_smallOwnershipFn(nullptr), m_smallSizeFn( nullptr ), m_smallAllocateFn(nullptr), m_smallFreeFn(nullptr)
								 , m_largeOwnershipFn(nullptr), m_largeSizeFn( nullptr ), m_largeAllocateFn(nullptr), m_largeFreeFn(nullptr)
								 , m_smallAllocationCutoff( 0 )
			{
			}
			OwnershipFn m_smallOwnershipFn;
			MemorySizeFn m_smallSizeFn;
			AllocateFn m_smallAllocateFn;
			FreeFn m_smallFreeFn;
			OwnershipFn m_largeOwnershipFn;
			MemorySizeFn m_largeSizeFn;
			AllocateFn m_largeAllocateFn;
			FreeFn m_largeFreeFn;
			Red::System::MemSize m_smallAllocationCutoff;
		};

		MultiAllocator();
		virtual ~MultiAllocator();

		void					RequestAllocatorInfo( AllocatorInfo& info );
		EAllocatorInitResults	Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags );
		void					Release( );
		Red::System::MemSize	ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback );
		Red::System::Bool		IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired );

		void*	Allocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass );
		void*	Reallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass );
		EAllocatorFreeResults Free( const void* ptr );
		Red::System::MemSize GetAllocationSize( const void* ptr ) const;
		Red::System::Bool OwnsPointer( const void* ptr ) const;
		void WalkAllocator( AllocatorWalker* theWalker );
		void WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker );
		void OnOutOfMemory();
		void DumpDebugOutput();

	private:

		CreationParameters m_creationParameters;

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