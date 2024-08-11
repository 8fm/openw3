/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef RED_CONTAINER_BUFFER_ALLOCATOR_STATIC_H
#define RED_CONTAINER_BUFFER_ALLOCATOR_STATIC_H
#pragma once

#include "containersCommon.h"

namespace Red { namespace Containers {

#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
	// If the markers don't match this in the DTor, then something has probably stomped on memory or we have overran the buffer
	const Red::System::Uint32 c_StaticArrayUnderRunMarker = 0x3A3A3A3A;
	const Red::System::Uint32 c_StaticArrayOverFlowMarker = 0xDEDEDEDE;
	const Red::System::MemSize c_StaticArrayProtectionSize = 32;
#endif

	// Allocation policy for a static array. Define ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION to check for potential overuns
	template< class Type, Red::System::Uint32 MaximumElements >
	class BufferAllocatorStatic
	{
	public:
		RED_INLINE BufferAllocatorStatic()	
		: m_elementCount( 0 )
		{ 
#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
			for( Red::System::MemSize i=0;i<c_StaticArrayProtectionSize;++i )
			{
				m_underRunProtValue[i] = c_StaticArrayUnderRunMarker;
				m_overRunProtValue[i] = c_StaticArrayOverFlowMarker;
			}
#endif
		}

		RED_INLINE ~BufferAllocatorStatic() 
		{
			VerifyInternal();
		}

		RED_INLINE void VerifyInternal() const
		{
#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
			for( Red::System::MemSize i=0;i<c_StaticArrayProtectionSize;++i )
			{
				RED_ASSERT( m_underRunProtValue[i] == c_StaticArrayUnderRunMarker, TXT( "Buffer underflow" ) );
				RED_ASSERT( m_overRunProtValue[i] == c_StaticArrayOverFlowMarker, TXT( "Buffer overflow" ) );
			}
#endif
		}

		RED_INLINE void MoveBuffer( BufferAllocatorStatic< Type, MaximumElements >& other, Red::MemoryFramework::MemoryClass otherMemClass )
		{
			RED_HALT( "Move constructor is not available in static arrays as ownership of the buffers cannot be changed!"  );
			RED_UNUSED( other );
			RED_UNUSED( otherMemClass );
			m_elementCount = 0;
			VerifyInternal();
		}

		// ResizeBuffer for static array does nothing but assert if the count is bigger than capacity
		// Note that this is not responsible for the element construction / destruction, it is simply memory related
		RED_INLINE void ResizeBuffer( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass )
		{
			RED_UNUSED( memClass );
#ifdef RED_FINAL_BUILD
			RED_UNUSED( elementSize );
#endif
			RED_ASSERT( elementCount <= MaximumElements, TXT( "Static Array Buffer cannot be resized over MaxElements" ) );
			RED_ASSERT( elementSize <= sizeof( Type ), TXT( "Element size mismatch!" ) );
			if( elementCount <= MaximumElements )
			{
				m_elementCount = elementCount;
			}
			VerifyInternal();
		}

		// ResizeFast for static array just changes size internally
		RED_INLINE void ResizeFast( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass )
		{
			RED_UNUSED( memClass );
#ifdef RED_FINAL_BUILD
			RED_UNUSED( elementSize );
#endif
			RED_ASSERT( elementSize == sizeof( Type ), TXT( "Element size mismatch!" ) );
			RED_ASSERT( elementCount <= MaximumElements, TXT( "Static Array Buffer cannot be resized over MaxElements" ) );
			if( elementCount <= MaximumElements )
			{
				m_elementCount = elementCount;
			}
			VerifyInternal();
		}

		// Reserve does nothing in statics except warn when the value is too big
		RED_INLINE void Reserve( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass )
		{
			RED_UNUSED( memClass );
#ifdef RED_FINAL_BUILD
			RED_UNUSED( elementSize );
			RED_UNUSED( elementCount );
#endif
			RED_ASSERT( elementSize == sizeof( Type ), TXT( "Element size mismatch!" ) );
			RED_ASSERT( elementCount < MaximumElements, TXT( "Cannot reserve a size > capacity in a static array" ) );
			VerifyInternal();
		}

		// Grow is the same as resize for static buffers!
		RED_INLINE Red::System::Uint32 GrowBuffer( Red::System::Uint32 elementCount, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass )
		{
			RED_UNUSED( memClass );
			RED_ASSERT( elementSize == sizeof( Type ), TXT( "Element size mismatch!" ) );
			Red::System::Uint32 oldElementCount = Size();
			Red::System::Uint32 newElementCount = oldElementCount + elementCount;
			ResizeBuffer( newElementCount, elementSize, memClass );
			VerifyInternal();

			return oldElementCount;
		}

		// Raw data accessors
		RED_INLINE void* Data()
		{
			VerifyInternal();
			return static_cast< void* >( m_buffer );
		}

		RED_INLINE const void* Data() const
		{
			VerifyInternal();
			return static_cast< const void* >( m_buffer );
		}

		// Size (Element count)
		RED_INLINE Red::System::Uint32 Size() const
		{
			VerifyInternal();
			return m_elementCount;
		}

		// Capacity (Current allocated elements)
		RED_INLINE Red::System::Uint32 Capacity() const
		{
			VerifyInternal();
			return MaximumElements;
		}

	private:

		Red::System::Uint32 m_elementCount;

#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
		Red::System::Uint32 m_underRunProtValue[c_StaticArrayProtectionSize];
#endif
		Red::System::Uint8 m_buffer[ sizeof( Type ) * MaximumElements ];			// No alignment taken into account!
#ifdef ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION
		Red::System::Uint32 m_overRunProtValue[c_StaticArrayProtectionSize];
#endif
	};

} }

#endif