#pragma once

#include "..\core\dataBuffer.h"

// we need specialized data buffer allocator because we use custom pool and alignment
template< EMemoryClass memoryClass >
class PhysXDataBufferAllocator : public IDataBufferAllocator
{
public:
	const static Uint32 ALIGNMENT = 128;

	virtual void* Allocate( const Uint32 size ) const
	{
		return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Physics, memoryClass, size, ALIGNMENT );
	}

	virtual void Free( void* ptr, const Uint32 size ) const
	{
		RED_MEMORY_FREE( MemoryPool_Physics, memoryClass, ptr );
	}

	static const IDataBufferAllocator& GetInstance()
	{
		static PhysXDataBufferAllocator< memoryClass > theInstance;
		return theInstance;
	}
};