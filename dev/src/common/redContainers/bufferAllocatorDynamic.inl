/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red { namespace Containers {

static Red::System::Bool s_doVerifyOnAll = true;

#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
	const Red::System::Uint32 c_DynamicArrayUnderRunMarker = 0x1B1B1B1B;
	const Red::System::Uint32 c_DynamicArrayOverRunMarker =  0xEFEFEFEF;
	const Red::System::Uint32 c_OverrunDataSize = 8;		// this * sizeof(int32)
#endif

	///////////////////////////////////////////////////////////////////
	// Default CTor
	//	
	template< class BufferAllocator >
	RED_INLINE BufferAllocatorDynamic< BufferAllocator >::BufferAllocatorDynamic()
		: m_capacity( 0 )
		, m_size( 0 )
		, m_buffer( nullptr )
#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
		, m_alignedDataBuffer( nullptr )
		, m_debugElementSize( 0 )
#endif
	{
	}

	///////////////////////////////////////////////////////////////////
	// DTor
	//	
	template< class BufferAllocator >
	RED_INLINE BufferAllocatorDynamic< BufferAllocator >::~BufferAllocatorDynamic()
	{
		RED_ASSERT( !m_buffer );
		VerifyInternal();
	}

	///////////////////////////////////////////////////////////////////
	// Move
	//	Move function (used when the compiler cannot generate implicit move ctor calls)
	template< class BufferAllocator >
	RED_INLINE void BufferAllocatorDynamic< BufferAllocator >::MoveBuffer( BufferAllocatorDynamic< BufferAllocator >& other, Red::MemoryFramework::MemoryClass otherMemClass )
	{
		VerifyInternal();

		// If the other has a buffer, remove it
		other.ResizeBuffer( 0, 0, otherMemClass );

		// Give other ownership of my buffer
		other.m_buffer = m_buffer;
		other.m_size = m_size;
		other.m_capacity = m_capacity;

		// Remove my ownership of the buffer
		m_buffer = nullptr;
		m_size = 0;
		m_capacity = 0;

#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
		other.m_alignedDataBuffer = m_alignedDataBuffer;
		m_alignedDataBuffer = nullptr;
#endif
	}

	///////////////////////////////////////////////////////////////////
	// VerifyInternal
	//	Check for overruns
	template< class BufferAllocator >
	RED_INLINE void BufferAllocatorDynamic< BufferAllocator >::VerifyInternal() const
	{
#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
		if( m_alignedDataBuffer != nullptr && m_debugElementSize > 0 )
		{
			// Fill under / overrun buffers
			Red::System::MemSize overrunDataSize = sizeof( Red::System::Uint32 ) * c_OverrunDataSize;
			Red::System::Uint32* underRun = reinterpret_cast< Red::System::Uint32* >( m_alignedDataBuffer );
			while( ( underRun - reinterpret_cast< Red::System::Uint32* >( m_alignedDataBuffer ) ) < c_OverrunDataSize )
			{
				RED_ASSERT( *underRun == c_DynamicArrayUnderRunMarker, TXT( "Buffer underrun has occured" ) );
				underRun++;
			}
			Red::System::Uint32 overrunCount = 0;
			Red::System::Uint32* overRun = reinterpret_cast< Red::System::Uint32* >( reinterpret_cast< Red::System::MemUint >( m_alignedDataBuffer ) + ( m_capacity * m_debugElementSize ) + overrunDataSize );
			while( overrunCount++ < c_OverrunDataSize )
			{
				RED_ASSERT( *overRun == c_DynamicArrayOverRunMarker, TXT( "Buffer overrun has occured" ) );
				overRun++;
			}
		}
#endif
	}

	///////////////////////////////////////////////////////////////////
	// ResizeBuffer
	//	Resize to a specific count. No capacity growing, etc
	template< class BufferAllocator >
	RED_INLINE void BufferAllocatorDynamic< BufferAllocator >::ResizeBuffer( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass )
	{
		VerifyInternal();
#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
		m_debugElementSize = elementSize;
		if( elementCount > 0 )
		{
			Red::System::MemSize overrunDataSize = sizeof( Red::System::Uint32 ) * c_OverrunDataSize;
			m_alignedDataBuffer = BufferAllocator::Reallocate( m_alignedDataBuffer, ( elementCount * elementSize ) + ( overrunDataSize * 2 ), memClass );
			m_buffer = reinterpret_cast< void* >( reinterpret_cast< Red::System::MemUint >( m_alignedDataBuffer ) + overrunDataSize );
			
			// Fill under / overrun buffers
			Red::System::Uint32* underRun = reinterpret_cast< Red::System::Uint32* >( m_alignedDataBuffer );
			while( ( underRun - reinterpret_cast< Red::System::Uint32* >( m_alignedDataBuffer ) ) < c_OverrunDataSize )
			{
				*underRun++ = c_DynamicArrayUnderRunMarker;
			}
			Red::System::Uint32 overrunCount = 0;
			Red::System::Uint32* overRun = reinterpret_cast< Red::System::Uint32* >( reinterpret_cast< Red::System::MemUint >( m_alignedDataBuffer ) + ( elementCount * elementSize ) + overrunDataSize );
			while( overrunCount++ < c_OverrunDataSize )
			{
				*overRun++ = c_DynamicArrayOverRunMarker;
			}
		}
		else
		{
			m_alignedDataBuffer = BufferAllocator::Reallocate( m_alignedDataBuffer, 0, memClass );
			m_buffer = nullptr;
		}
#else
		m_buffer = BufferAllocator::Reallocate( m_buffer, elementCount * elementSize, memClass );
#endif

		if( elementCount == 0 )
		{
			RED_ASSERT( m_buffer == nullptr, TXT( "Failed to free buffer?" ) );
			m_capacity = m_size = 0;
		}
		else
		{
			RED_ASSERT( m_buffer != nullptr, TXT( "Failed to allocate dynamic array buffer. Expect crashes to follow" ) );
			m_capacity = m_size = m_buffer != nullptr ? elementCount : 0;
		}		
	}

	///////////////////////////////////////////////////////////////////
	// Reserve
	//	Increase capacity, does not change the size
	template< class BufferAllocator >
	RED_INLINE void BufferAllocatorDynamic< BufferAllocator >::Reserve( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass )
	{
		VerifyInternal();
#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
		m_debugElementSize = elementSize;
#endif
		if( elementCount > m_capacity )
		{
			Red::System::Uint32 oldSize = m_size;
			ResizeBuffer( elementCount, elementSize, memClass );
			m_size = oldSize;	// Restore old size as resizeBuffer increases it
		}
	}

	///////////////////////////////////////////////////////////////////
	// ResizeFast
	//	Sticks to the element count. Avoids reallocation where possible
	template< class BufferAllocator >
	RED_INLINE void BufferAllocatorDynamic< BufferAllocator >::ResizeFast( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass )
	{
		VerifyInternal();
#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
		m_debugElementSize = elementSize;
#endif
		if( elementCount <= m_capacity )
		{
			m_size = elementCount;
		}
		else
		{
			ResizeBuffer( elementCount, elementSize, memClass );
		}
	}

	///////////////////////////////////////////////////////////////////
	// GrowBuffer
	//	Tries to grow a bit intelligently to take into account lots of 
	//  grows happening in short amount of time
	template< class BufferAllocator >
	RED_INLINE Red::System::Uint32 BufferAllocatorDynamic< BufferAllocator >::GrowBuffer( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass )
	{
		VerifyInternal();
#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
		m_debugElementSize = elementSize;
#endif
		Red::System::Uint32 oldSize = m_size;
		Red::System::Uint32 newSize = oldSize + elementCount;
		if( newSize > m_capacity )
		{
			// Buffer is too small and needs to reallocate
			// Increase by 1.5x requested size
			ResizeBuffer( newSize + ( newSize >> 1 ), elementSize, memClass );
			m_size = m_buffer != nullptr ? newSize : 0;
		}
		else
		{
			m_size = newSize;
		}

		return oldSize;
	}

	///////////////////////////////////////////////////////////////////
	// Data
	//	Raw data accessor
	template< class BufferAllocator >
	RED_INLINE void* BufferAllocatorDynamic< BufferAllocator >::Data()
	{
		if( s_doVerifyOnAll )
			VerifyInternal();
		return m_buffer;
	}

	///////////////////////////////////////////////////////////////////
	// Data
	//	Raw data accessor
	template< class BufferAllocator >
	RED_INLINE const void* BufferAllocatorDynamic< BufferAllocator >::Data() const
	{
		if( s_doVerifyOnAll )
			VerifyInternal();
		return m_buffer;
	}
	
	///////////////////////////////////////////////////////////////////
	// Size
	//	Returns number of elements in buffer
	template< class BufferAllocator >
	RED_INLINE Red::System::Uint32 BufferAllocatorDynamic< BufferAllocator >::Size() const
	{
		if( s_doVerifyOnAll )
			VerifyInternal();
		return m_size;
	}

	///////////////////////////////////////////////////////////////////
	// Capacity
	//	Returns total elements (variable as the buffer resizes)
	template< class BufferAllocator >
	RED_INLINE Red::System::Uint32 BufferAllocatorDynamic< BufferAllocator >::Capacity() const
	{
		if( s_doVerifyOnAll )
			VerifyInternal();
		return m_capacity;
	}

} }
