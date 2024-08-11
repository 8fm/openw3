/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_TLSF_ALLOCATOR_H
#define _RED_MEMORY_FRAMEWORK_TLSF_ALLOCATOR_H
#pragma once

#include "../redSystem/types.h"
#include "redMemoryAllocator.h"
#include "redMemoryTlsfImpl.h"

namespace Red { namespace MemoryFramework {

// Default system memory block size. This will probably be set per-platform
const Red::System::MemSize c_defaultSystemBlockSize = 1024 * 1024;

/////////////////////////////////////////////////////////////////////////////
// Two-level segregate fit allocator
//	This wraps the TLSF Implementation in redMemoryTlsfImpl

class ScopedMemory_NoProtection
{
public:
	RED_INLINE ScopedMemory_NoProtection( const TLSFAllocatorImpl* ) { }
	RED_INLINE ~ScopedMemory_NoProtection() { }
};

template < typename TSyncLock, class TScopedMemoryProtection = ScopedMemory_NoProtection >
class TLSFAllocatorBase : public IAllocator
{
public:
	class CreationParameters : public IAllocatorCreationParameters
	{
	public:
		// initialSize - start size of the pool
		// maximumPoolSize - largest the pool can grow
		// systemBlockSize - granularity of allocations from the system allocator
		// segregationGranularity - how many splits to do for each power-of-two 
		CreationParameters( Red::System::MemSize initialSize, 
							Red::System::MemSize maximumPoolSize,
							Red::System::MemSize systemBlockSize = c_defaultSystemBlockSize, 
							Red::System::Uint32 segregationGranularity = TLSF::c_MaximumSecondLevelDivisions )
			: m_initialSize( initialSize )
			, m_maximumSize( maximumPoolSize )
			, m_systemBlockSize( systemBlockSize )
			, m_secondLevelGranularity( segregationGranularity )
		{
		}
		Red::System::MemSize	m_initialSize;
		Red::System::MemSize	m_maximumSize;
		Red::System::MemSize	m_systemBlockSize;
		Red::System::Uint32		m_secondLevelGranularity;
	};

	TLSFAllocatorBase( );
	~TLSFAllocatorBase( );

	EAllocatorInitResults	Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags );
	void					Release( );
	void					RequestAllocatorInfo( AllocatorInfo& info );
	Red::System::MemSize	ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback );
	Red::System::Bool		IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired );

	// 'static' functions
	void*	Allocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass );
	void*	Reallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass );
	EAllocatorFreeResults Free( const void* ptr );
	Red::System::MemSize GetAllocationSize( const void* ptr ) const;
	Red::System::Bool OwnsPointer( const void* ptr ) const;

	// Walk each large area of the allocator
	void WalkAllocator( AllocatorWalker* theWalker );

	// Walk all allocations in a particular area
	void WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker );

	// Output useful info when this pool runs out of memory
	void OnOutOfMemory();

	// Extensive debug output
	void DumpDebugOutput();

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

	// The actual tlsf allocator
	TLSFAllocatorImpl m_theAllocator;

	// Mutex
	mutable TSyncLock m_syncLock;
};

} }

#endif // RED_MEMORY_FRAMEWORK_TLSF_ALLOCATOR_H