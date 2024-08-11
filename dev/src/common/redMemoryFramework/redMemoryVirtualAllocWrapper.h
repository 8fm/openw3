/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_FRAMEWORK_VIRTUAL_ALLOC_WRAPPER_H
#define _RED_MEMORY_FRAMEWORK_VIRTUAL_ALLOC_WRAPPER_H
#pragma once

#include "../redSystem/types.h"
#include "redMemoryAllocator.h"
#include "redMemoryListHelpers.h"

// VirtualAlloc wrapper which can be used to handle allocating straight from system memory
namespace Red { namespace MemoryFramework {

	template< typename TSyncLock >
	class VirtualAllocWrapperAllocator : public IAllocator 
	{
	public:
		VirtualAllocWrapperAllocator();
		virtual ~VirtualAllocWrapperAllocator();

		class CreationParameters : public IAllocatorCreationParameters
		{
		public:
			CreationParameters() { }
		};

		virtual EAllocatorInitResults Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags );
		virtual void Release( );

		virtual Red::System::Bool IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired );
		virtual Red::System::MemSize ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback );

		virtual void RequestAllocatorInfo( AllocatorInfo& info );
		virtual void WalkAllocator( AllocatorWalker* theWalker );
		virtual void WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker );

		void* Allocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass );
		void* Reallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass );
		EAllocatorFreeResults Free( const void* ptr );
		Red::System::MemSize GetAllocationSize( const void* ptr ) const;
		Red::System::Bool OwnsPointer( const void* ptr ) const;

		virtual void DumpDebugOutput();

		virtual void* RuntimeAllocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass );
		virtual void* RuntimeReallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass );
		virtual EAllocatorFreeResults RuntimeFree( void* ptr );
		virtual Red::System::MemSize RuntimeGetAllocationSize( void* ptr ) const;
		virtual Red::System::Bool RuntimeOwnsPointer( void* ptr ) const;
		virtual void OnOutOfMemory();

	private:
		// Track active allocations in a small pool.
		struct ActiveAllocation : public Utils::ListNode< ActiveAllocation >
		{
			Red::System::MemUint m_startAddress;
			Red::System::MemSize m_totalSize;
		};

		static const Uint32 c_maxAllocRecords = 1024;		// Arbitrary

		ActiveAllocation* m_allocRecordPool;

		ActiveAllocation* m_allocRecordPoolHead;
		ActiveAllocation* m_allocRecordPoolTail;

		ActiveAllocation* m_activeAllocsHead;
		ActiveAllocation* m_activeAllocsTail;
		
		Uint32 m_flags;
		mutable TSyncLock m_syncLock;
	};

} }

#endif