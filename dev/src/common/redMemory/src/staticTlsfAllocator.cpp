/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "staticTlsfAllocator.h"
#include "assert.h"
#include "tlsfConstant.h"
#include "utils.h"
#include "tlsfBlock.h"

namespace red
{
namespace memory
{
	StaticTLSFAllocator:: StaticTLSFAllocator()
	{}

	StaticTLSFAllocator::~StaticTLSFAllocator()
	{}

	void StaticTLSFAllocator::Initialize( const StaticTLSFAllocatorParameter & parameter )
	{
		m_parameter = parameter;
		ValidateParameter();
	
		const u64 bufferAddress = reinterpret_cast< u64 >( parameter.buffer );
		VirtualRange range = { bufferAddress, bufferAddress + parameter.bufferSize };
		SystemBlock block = { bufferAddress, parameter.bufferSize };

		TLSFAllocatorParameter implementationParam = 
		{
			range,
			block
		};

		m_allocator.Initialize( implementationParam );
	}

	void StaticTLSFAllocator::Uninitialize()
	{}

	Block StaticTLSFAllocator::Allocate( u32 size )
	{
		return m_allocator.Allocate( size );
	}

	Block StaticTLSFAllocator::AllocateAligned( u32 size, u32 alignment )
	{
		return m_allocator.AllocateAligned( size, alignment );
	}

	void StaticTLSFAllocator::Free( Block & block )
	{
		m_allocator.Free( block );
	}

	Block StaticTLSFAllocator::Reallocate( Block & block, u32 size )
	{
		return m_allocator.Reallocate( block, size );
	}

	bool StaticTLSFAllocator::OwnBlock( u64 block ) const
	{
		return m_allocator.OwnBlock( block );
	}

	u64 StaticTLSFAllocator::GetBlockSize( u64 address ) const
	{
		RED_MEMORY_ASSERT( OwnBlock( address ), "Block is not own by this allocator." );
		return GetTLSFBlockSize( address );
	}

	void StaticTLSFAllocator::ValidateParameter( ) const
	{
		RED_MEMORY_ASSERT( m_parameter.buffer, "Cannot initialize StaticTLSFAllocator with null buffer." );
		RED_MEMORY_ASSERT( IsAligned( m_parameter.buffer, 16 ), "Cannot initialize StaticTLSFAllocator. Provided buffer must be aligned on 16 byte.");
		RED_MEMORY_ASSERT( m_parameter.bufferSize, "Cannot initialize StaticTLSFAllocator with 0 size buffer." );
		RED_MEMORY_ASSERT( m_parameter.bufferSize > ComputeTLSFBookKeepingSize( m_parameter.bufferSize ), 
			"Provided buffer of size %d is not big enough. Need at least %d byte.", m_parameter.bufferSize, ComputeTLSFBookKeepingSize( m_parameter.bufferSize ) );
	}
}
}
