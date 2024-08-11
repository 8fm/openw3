/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef RED_CONTAINER_BUFFER_ALLOCATOR_DYNAMIC_H
#define RED_CONTAINER_BUFFER_ALLOCATOR_DYNAMIC_H
#pragma once

#include "containersCommon.h"
#include "../../common/redMemoryFramework/redMemoryFramework.h"

namespace Red { namespace Containers {

	// Dynamic buffer allocator policy
	// Used for dynamic arrays
	//	BufferAllocator - policy with static Reallocate(ptr, size, memclass) function
	// Enable ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION to test for under / overruns
	template< class BufferAllocator >
	class BufferAllocatorDynamic
	{
	public:
		BufferAllocatorDynamic();
		~BufferAllocatorDynamic();		

		void MoveBuffer( BufferAllocatorDynamic< BufferAllocator >& other, Red::MemoryFramework::MemoryClass otherMemClass );

		// Buffer resizing
		void ResizeBuffer( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass );
		void ResizeFast( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass );
		void Reserve( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass );

		// Grow function return the *old* size. 
		Red::System::Uint32 GrowBuffer( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass );

		// Element Buffer accessors
		void* Data();
		const void* Data() const;

		// Buffer size properties
		Red::System::Uint32 Size() const;			// Size in elements
		Red::System::Uint32 Capacity() const;		// Capacity in elements

	private:
		Red::System::Uint32 m_capacity;							// Amount allocated
		Red::System::Uint32 m_size;								// Number of contiguous elements in buffer
		void* m_buffer;											// Data buffer ptr

#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
		void* m_alignedDataBuffer;								// When overrun protection is on, this points to the full buffer
		Red::System::MemSize m_debugElementSize;				// Need to cache element size in order to test for overruns
#endif

		void VerifyInternal() const;
	};

} }

#include "bufferAllocatorDynamic.inl"

#endif