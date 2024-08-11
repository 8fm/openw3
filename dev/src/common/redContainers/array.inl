#include "containersCommon.h"
#include "algorithms.h"

namespace Red { namespace Containers {
	
	////////////////////////////////////////////////////////////////////
	// ArrayBase CTor
	//
	template< class BufferPolicy >
	RED_INLINE ArrayBase< BufferPolicy >::ArrayBase()
	{
	}

	////////////////////////////////////////////////////////////////////
	// ArrayBase DTor
	//	Cannot call element destructors since we don't know what type they are
	template< class BufferPolicy >
	RED_INLINE ArrayBase< BufferPolicy >::~ArrayBase()
	{
		RED_ASSERT( m_bufferPolicy.Size()==0, TXT( "Array base class destroyed with elements still alive" ) );
	}

	////////////////////////////////////////////////////////////////////
	// Raw Buffer accessor
	//
	template< class BufferPolicy >
	RED_INLINE void* ArrayBase< BufferPolicy >::Data()
	{
		return m_bufferPolicy.Data();
	}

	////////////////////////////////////////////////////////////////////
	// Raw Buffer accessor
	//
	template< class BufferPolicy >
	RED_INLINE const void* ArrayBase< BufferPolicy >::Data() const
	{
		return m_bufferPolicy.Data();
	}

	////////////////////////////////////////////////////////////////////
	// Size
	//	Size of the array (in elements)
	template< class BufferPolicy >
	RED_INLINE Red::System::Uint32 ArrayBase< BufferPolicy >::Size() const
	{
		return m_bufferPolicy.Size();
	}

	////////////////////////////////////////////////////////////////////
	// Capacity
	//	Maximum size of the array at this time (may change depending on buffer policy)
	template< class BufferPolicy >
	RED_INLINE Red::System::Uint32 ArrayBase< BufferPolicy >::Capacity() const
	{
		return m_bufferPolicy.Capacity();
	}

	////////////////////////////////////////////////////////////////////
	// GrowBuffer
	//	Increase the size of the buffer
	template< class BufferPolicy >
	RED_INLINE void ArrayBase< BufferPolicy >::GrowBuffer( Red::System::Uint32 elementsToGrowBy, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass )
	{
		RED_ASSERT( elementsToGrowBy > 0, TXT( "Invalid element growth" ) );
		RED_ASSERT( elementSize > 0, TXT( "Invalid element size" ) );
		m_bufferPolicy.GrowBuffer( elementsToGrowBy, elementSize, memClass );
	}

	////////////////////////////////////////////////////////////////////
	// ShrinkBuffer
	//	Shrink the buffer
	template< class BufferPolicy >
	RED_INLINE void ArrayBase< BufferPolicy >::ShrinkBuffer( Red::System::Uint32 elementsToShrinkBy, Red::System::MemSize elementSize, Red::MemoryFramework::MemoryClass memClass )
	{
		RED_ASSERT( elementsToShrinkBy <= m_bufferPolicy.Size(), TXT( "Attempting to shrink size of array < 0 " ) );
		RED_ASSERT( elementSize > 0, TXT( "Invalid element size" ) );
		m_bufferPolicy.ResizeBuffer( m_bufferPolicy.Size() - elementsToShrinkBy, elementSize, memClass );
	}

	////////////////////////////////////////////////////////////////////
	// Clear
	//	Clear the buffer
	template< class BufferPolicy >
	RED_INLINE void ArrayBase< BufferPolicy >::Clear( Red::MemoryFramework::MemoryClass memClass )
	{
		m_bufferPolicy.ResizeBuffer( 0, 0, memClass );
	}

	////////////////////////////////////////////////////////////////////
	// Default CTor
	//
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Array< ElementType, BufferPolicy, MemClass >::Array()
	{

	}

	////////////////////////////////////////////////////////////////////
	// CTor with initial size
	//
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Array< ElementType, BufferPolicy, MemClass >::Array( Red::System::Uint32 initialSize )
	{
		Resize( initialSize );
	}

	////////////////////////////////////////////////////////////////////
	// Copy CTor
	//	Exploit the = operator so we don't have to write this twice
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Array< ElementType, BufferPolicy, MemClass >::Array( const Array< ElementType, BufferPolicy, MemClass >& other )
	{
		*this = other;
	}

	////////////////////////////////////////////////////////////////////
	// Move CTor
	//	The array is responsible for destroying the old elements, while the
	//  buffer policy handles moving the new buffer (element move ctors are not called) and removing the old one
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Array< ElementType, BufferPolicy, MemClass >::Array( Array< ElementType, BufferPolicy, MemClass >&& other )
	{
		// Destroy the old elements
		TInit::Destruct( TypedData(), m_bufferPolicy.Size() );

		// Move the buffer - done manually as move constructors are not called implicitly in early compilers
		other.m_bufferPolicy.MoveBuffer( m_bufferPolicy, MemClass );
	}

	////////////////////////////////////////////////////////////////////
	// Copy CTor with different types
	//	
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass > template< class OtherBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
	RED_INLINE Array< ElementType, BufferPolicy, MemClass >::Array( const Array< ElementType, OtherBufferPolicy, OtherMemClass >& other )
	{
		PushBack( other );
	}

	////////////////////////////////////////////////////////////////////
	// DTor
	//	Removes all elements and free any memory
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Array< ElementType, BufferPolicy, MemClass >::~Array()
	{
		// Destroy all the elements. Done manually as Resize() calls constructors, which causes issues
		// for objects in arrays with hidden / no default CTor defined
		TInit::Destruct( TypedData(), m_bufferPolicy.Size() );
		m_bufferPolicy.ResizeBuffer( 0, 0, MemClass );
	}

	////////////////////////////////////////////////////////////////////
	// Buffer accessor
	//
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE ElementType* Array< ElementType, BufferPolicy, MemClass >::TypedData()
	{
		return reinterpret_cast< ElementType* >( m_bufferPolicy.Data() );
	}

	////////////////////////////////////////////////////////////////////
	// Buffer accessor
	//
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE const ElementType* Array< ElementType, BufferPolicy, MemClass >::TypedData() const
	{
		return reinterpret_cast< const ElementType* >( m_bufferPolicy.Data() );
	}

	////////////////////////////////////////////////////////////////////
	// Element accessor
	//
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE ElementType& Array< ElementType, BufferPolicy, MemClass >::operator[]( Red::System::Uint32 index )
	{
		RED_ASSERT( m_bufferPolicy.Size() > index, TXT( "Accessing array buffer out of bounds" ) );
		return *( reinterpret_cast< ElementType* >( m_bufferPolicy.Data() ) + index );
	}

	////////////////////////////////////////////////////////////////////
	// Element accessor
	//
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE const ElementType& Array< ElementType, BufferPolicy, MemClass >::operator[]( Red::System::Uint32 index ) const
	{
		RED_ASSERT( m_bufferPolicy.Size() > index, TXT( "Accessing array buffer out of bounds" ) );
		return *( reinterpret_cast< const ElementType* >( m_bufferPolicy.Data() ) + index );
	}

	////////////////////////////////////////////////////////////////////
	// Last element accessor
	//
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE ElementType& Array< ElementType, BufferPolicy, MemClass >::Back()
	{
		RED_ASSERT( m_bufferPolicy.Size() > 0, TXT( "Accessing array buffer out of bounds" ) );
		return *( TypedData() +  m_bufferPolicy.Size() - 1 );
	}

	////////////////////////////////////////////////////////////////////
	// Last element accessor
	//
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE const ElementType& Array< ElementType, BufferPolicy, MemClass >::Back() const
	{
		RED_ASSERT( m_bufferPolicy.Size() > 0, TXT( "Accessing array buffer out of bounds" ) );
		return *( TypedData() +  m_bufferPolicy.Size() - 1 );
	}

	////////////////////////////////////////////////////////////////////
	// DataSize
	//	Size of all the data in the array
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Red::System::MemSize Array< ElementType, BufferPolicy, MemClass >::DataSize() const
	{
		return sizeof( ElementType ) * m_bufferPolicy.Size();
	}

	////////////////////////////////////////////////////////////////////
	// SizeOfAllElements
	//	Size of everything allocated by the array
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Red::System::MemSize Array< ElementType, BufferPolicy, MemClass >::SizeOfAllElements() const
	{
		return sizeof( ElementType ) * m_bufferPolicy.Capacity();
	}

	////////////////////////////////////////////////////////////////////
	// Empty
	//	Returns true if size = 0
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::Empty() const
	{
		return m_bufferPolicy.Size() == 0 ? true : false;
	}

	////////////////////////////////////////////////////////////////////
	// Reserve
	//	Resizes the internal buffer, but does not call constructors / destructors on existing objects
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::Reserve( Red::System::Uint32 size )
	{
		m_bufferPolicy.Reserve( size, sizeof( ElementType ), MemClass );
	}

	////////////////////////////////////////////////////////////////////
	// Resize
	//	Handles object lifetimes and buffer memory
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::Resize( Red::System::Uint32 size )
	{
		const Red::System::MemSize oldSize = m_bufferPolicy.Size();

		if( size != oldSize )
		{
			// Destruct any elements that will be removed
			if( size < oldSize )
			{
				TInit::Destruct( TypedData() + size, oldSize - size );
			}

			m_bufferPolicy.ResizeBuffer( size, sizeof( ElementType ), MemClass );

			// If any new objects were added, call their default constructor
			if( m_bufferPolicy.Size() > oldSize )
			{
				TInit::Construct( TypedData() + oldSize, m_bufferPolicy.Size() - oldSize );
			}
		}
	}

	////////////////////////////////////////////////////////////////////
	// ResizeFast
	//	Fast path of resize (usually involves no reallocation of buffer)
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::ResizeFast( Red::System::Uint32 size )
	{
		if( size < m_bufferPolicy.Capacity() )
		{
			const Red::System::MemSize oldSize = m_bufferPolicy.Size();

			// Destruct any elements that will be removed
			if( size < oldSize )
			{
				TInit::Destruct( TypedData() + size, oldSize - size );
			}
			m_bufferPolicy.ResizeFast( size, sizeof( ElementType ), MemClass );

			// If any new objects were added, call their default constructor
			if( m_bufferPolicy.Size() > oldSize )
			{
				TInit::Construct( TypedData() + oldSize, m_bufferPolicy.Size() - oldSize );
			}
		}
		else
		{
			Resize( size );
		}
	}

	////////////////////////////////////////////////////////////////////
	// Clear
	//	Destroy all elements, clear out the buffer, reserve a new size
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::Clear( Red::System::Uint32 sizeToReserve )
	{
		TInit::Destruct( TypedData(), m_bufferPolicy.Size() );
		m_bufferPolicy.ResizeFast( 0, sizeof( ElementType ), MemClass );
		m_bufferPolicy.Reserve( sizeToReserve, sizeof( ElementType ), MemClass );
	}

	////////////////////////////////////////////////////////////////////
	// Clear
	//	Remove all elements, clear out the buffer
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::Clear()
	{
		TInit::Destruct( TypedData(), m_bufferPolicy.Size() );
		m_bufferPolicy.ResizeBuffer( 0, sizeof( ElementType ), MemClass );
	}

	////////////////////////////////////////////////////////////////////
	// ClearFast
	//	Remove all elements
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::ClearFast()
	{
		TInit::Destruct( TypedData(), m_bufferPolicy.Size() );
		m_bufferPolicy.ResizeFast( 0, sizeof( ElementType ), MemClass );
	}

	////////////////////////////////////////////////////////////////////
	// ClearPtr
	//	Delete all elements, then remove them
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::ClearPtr()
	{
		for ( Red::System::Uint32 i = 0; i < m_bufferPolicy.Size(); ++i )
		{
			delete TypedData()[i];
		}
		Resize(0);
	}

	////////////////////////////////////////////////////////////////////
	// ClearPtrFast
	//	Delete all elements, then remove them quickly
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::ClearPtrFast()
	{
		for ( Red::System::Uint32 i = 0; i < m_bufferPolicy.Size(); ++i )
		{
			delete TypedData()[i];
		}
		ResizeFast(0);
	}

	////////////////////////////////////////////////////////////////////
	// ClearPtrRev
	//	Delete all elements, then remove them, in reverse order
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::ClearPtrRev()
	{
		Red::System::Uint32 count = m_bufferPolicy.Size();
		while( count != 0 )
		{
			delete TypedData()[count-1];
			count--;
		}
		Resize(0);
	}

	////////////////////////////////////////////////////////////////////
	// PushBack
	//	Push a single element to the end of the array
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::PushBack( const ElementType& element )
	{
		Red::System::Uint32 newIndex = m_bufferPolicy.GrowBuffer( 1, sizeof( ElementType ), MemClass );
		TInit::CopyConstruct( TypedData() + newIndex, &element, 1 );
	}

	////////////////////////////////////////////////////////////////////
	// PushBack
	//	Move an element to the end of the array
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::PushBack( ElementType&& element )
	{
		Red::System::Uint32 newIndex = m_bufferPolicy.GrowBuffer( 1, sizeof( ElementType ), MemClass );
		TInit::MoveConstruct( TypedData() + newIndex, &element, 1 );
	}

	////////////////////////////////////////////////////////////////////
	// PushBack
	//	Copy another array to the end of this array
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass > template< class OtherBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass > 
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::PushBack( const Array< ElementType, OtherBufferPolicy, OtherMemClass >& other )
	{
		Red::System::Uint32 arrSize = other.Size();
		Red::System::Uint32 newIndex = m_bufferPolicy.GrowBuffer( arrSize, sizeof( ElementType ), MemClass );
		TInit::CopyConstruct( TypedData() + newIndex, other.TypedData(), arrSize );
	}

	////////////////////////////////////////////////////////////////////
	// Insert
	//	Add an element at specified position
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::Insert( const Red::System::Uint32 position, const ElementType& element )
	{
		if ( position <= m_bufferPolicy.Size() )
		{
			Red::System::Uint32 oldSize = Grow( 1 );
			TCopy::MoveForwardsInsert( TypedData() + position, oldSize - position, &element, 1 );

			return true;
		}
		else
		{
			return false;
		}
	}

	////////////////////////////////////////////////////////////////////
	// Insert
	//	Move an element to a specific position
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::Insert( const Red::System::Uint32 position, ElementType&& element )
	{
		if ( position <= m_bufferPolicy.Size() )
		{
			Red::System::Uint32 oldSize = Grow( 1 );
			TCopy::MoveForwardsInsert( TypedData() + position, oldSize - position, &element, 1 );

			return true;
		}
		else
		{
			return false;
		}
	}

	////////////////////////////////////////////////////////////////////
	// PushBack
	//	Push a single element to the end of the array if it is unique
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::PushBackUnique( const ElementType& element )
	{
		if ( Algorithms::Find( Begin(), End(), element ) == End() )
		{
			PushBack( element );
			return true;
		}
		else
		{
			return false;
		}
	}

	////////////////////////////////////////////////////////////////////
	// PushBackUnique
	//	Move an element to the end of the array if it is unique
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::PushBackUnique( ElementType&& element )
	{
		if ( Algorithms::Find( Begin(), End(), element ) == End() )
		{
			PushBack( ElementMoveConstruct( element ) );
			return true;
		}
		else
		{
			return false;
		}
	}

	////////////////////////////////////////////////////////////////////
	// PushBackUnique
	//	Copy the unique elements from another array into this one
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass > template< class OtherBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::PushBackUnique( const Array< ElementType, OtherBufferPolicy, OtherMemClass >& other )
	{
		for( Red::System::Uint32 i = 0; i < other.Size(); ++i ) 
		{
			if ( Algorithms::Find( Begin(), End(), other[i] ) == End() )
			{
				PushBack( other[i] );
			}
		}
	}

	////////////////////////////////////////////////////////////////////
	// PopBack
	//	Pop element from the back of the array, resize down
	//	Note, this can be slow with dynamic buffers due to all the memory reallocations
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE ElementType Array< ElementType, BufferPolicy, MemClass >::PopBack()
	{
		RED_ASSERT( m_bufferPolicy.Size() > 0, TXT( "Attempting to pop back from an empty array, resize is probably going to run out of memory due to overflow" ) );
		
		Red::System::Uint32 backIndex = m_bufferPolicy.Size() - 1;
		ElementType elem = TypedData()[ backIndex ];
		Resize( backIndex );

		return elem;
	}

	////////////////////////////////////////////////////////////////////
	// PopBackFast
	//	Pop element from the back of the array, resize down fast
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE ElementType Array< ElementType, BufferPolicy, MemClass >::PopBackFast()
	{
		RED_ASSERT( m_bufferPolicy.Size() > 0, TXT( "Attempting to pop back from an empty array, resize is probably going to run out of memory due to overflow" ) );

		Red::System::Uint32 backIndex = m_bufferPolicy.Size() - 1;
		ElementType elem = TypedData()[ backIndex ];
		TInit::Destruct( TypedData() + backIndex, 1 );
		ResizeFast( backIndex );

		return elem;
	}

	////////////////////////////////////////////////////////////////////
	// Swap
	//	Swap the 2 elements at the specified indices
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::Swap( Red::System::Uint32 element1Index, Red::System::Uint32 element2Index )
	{
		TInit::SwapElements( &TypedData()[element1Index], &TypedData()[element2Index] );
	}

	////////////////////////////////////////////////////////////////////
	// Swap
	//	Swap the 2 elements pointed at by the iterators
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::Swap( iterator element1, iterator element2 )
	{
		TInit::SwapElements( element1, element2 );
	}

	////////////////////////////////////////////////////////////////////
	// Rewind
	//	Acts like a fast resize but with no growth
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::Rewind( Red::System::Uint32 size )
	{
		RED_ASSERT( size < m_bufferPolicy.Size(), TXT( "Trying to rewind but size < new size" ) );
		TInit::Destruct( TypedData() + size, m_bufferPolicy.Size() - size );
		m_bufferPolicy.ResizeFast( size, sizeof( ElementType ), MemClass );
	}

	////////////////////////////////////////////////////////////////////
	// Grow
	//	Increase the size of the array (or attempt to)
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Red::System::Uint32 Array< ElementType, BufferPolicy, MemClass >::Grow( Red::System::Uint32 elementsToGrowBy )
	{
		Red::System::Uint32 oldSize = m_bufferPolicy.GrowBuffer( elementsToGrowBy, sizeof( ElementType ), MemClass );
		TInit::Construct(TypedData() + oldSize, elementsToGrowBy);
		return oldSize;
	}

	////////////////////////////////////////////////////////////////////
	// Shrink
	//	Where possible, resizes the array buffer down so capacity = size
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::Shrink()
	{
		if( m_bufferPolicy.Size() < m_bufferPolicy.Capacity() )
		{
			m_bufferPolicy.ResizeBuffer( m_bufferPolicy.Size(), sizeof( ElementType ), MemClass );
		}
	}

	////////////////////////////////////////////////////////////////////
	// Equality test
	//	Supports arrays with different buffer types as long as their element types match
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass > template < class OtherArrayBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::operator==(const Array< ElementType, OtherArrayBufferPolicy, OtherMemClass >& other) const
	{
		return (m_bufferPolicy.Size() == other.Size() && TCompare::Equal(TypedData(), other.TypedData(), m_bufferPolicy.Size()) );
	}

	////////////////////////////////////////////////////////////////////
	// Inequality test
	//	Supports arrays with different buffer types as long as their element types match
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass > template < class OtherArrayBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::operator!=(const Array< ElementType, OtherArrayBufferPolicy, OtherMemClass >& other) const
	{
		return !(*this == other);
	}

	////////////////////////////////////////////////////////////////////
	// < test
	//	Supports arrays with different buffer types as long as their element types match
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass > template < class OtherArrayBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::operator<(const Array< ElementType, OtherArrayBufferPolicy, OtherMemClass >& other) const
	{
		return TCompare::Less(TypedData(), m_bufferPolicy.Size(), other.TypedData(), other.Size());
	}

	////////////////////////////////////////////////////////////////////
	// > test
	//	Supports arrays with different buffer types as long as their element types match
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass > template < class OtherArrayBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::operator>(const Array< ElementType, OtherArrayBufferPolicy, OtherMemClass >& other) const
	{
		return TCompare::Less(other.TypedData(), other.Size(), TypedData(), m_bufferPolicy.Size());
	}

	////////////////////////////////////////////////////////////////////
	// <= test
	//	Supports arrays with different buffer types as long as their element types match
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass > template < class OtherArrayBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::operator<=(const Array< ElementType, OtherArrayBufferPolicy, OtherMemClass >& other) const
	{
		return !TCompare::Less(other.TypedData(), other.Size(), TypedData(), m_bufferPolicy.Size());
	}

	////////////////////////////////////////////////////////////////////
	// >= test
	//	Supports arrays with different buffer types as long as their element types match
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass > template < class OtherArrayBufferPolicy, Red::MemoryFramework::MemoryClass OtherMemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::operator>=(const Array< ElementType, OtherArrayBufferPolicy, OtherMemClass >& other) const
	{
		return !TCompare::Less(TypedData(), m_bufferPolicy.Size(), other.TypedData(), other.Size());
	}

	////////////////////////////////////////////////////////////////////
	// Assignment
	//
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE const Array< ElementType, BufferPolicy, MemClass >& Array< ElementType, BufferPolicy, MemClass >::operator=( const Array< ElementType, BufferPolicy, MemClass >& other )
	{
		if ( &other != this )
		{
			TInit::Destruct( TypedData(), m_bufferPolicy.Size() );		// Destroy current elements
			m_bufferPolicy.ResizeBuffer( other.Size(), sizeof( ElementType ), MemClass );
			TInit::CopyConstruct( TypedData(), other.TypedData(), m_bufferPolicy.Size() );
		}

		return *this;
	}

	////////////////////////////////////////////////////////////////////
	// 'Move' Assignment
	//	Essentially acts like a move constructor
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Array< ElementType, BufferPolicy, MemClass >& Array< ElementType, BufferPolicy, MemClass >::operator=( Array< ElementType, BufferPolicy, MemClass >&& other )
	{
		// Destroy the old elements
		TInit::Destruct( TypedData(), m_bufferPolicy.Size() );

		// Move the buffer - done manually as move constructors are not called implicitly in early compilers
		other.m_bufferPolicy.MoveBuffer( m_bufferPolicy, MemClass );

		return *this;
	}

	////////////////////////////////////////////////////////////////////
	// Begin
	//	Iterator stuff
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE typename Array< ElementType, BufferPolicy, MemClass >::iterator Array< ElementType, BufferPolicy, MemClass >::Begin()
	{
		return TypedData();
	}

	////////////////////////////////////////////////////////////////////
	// Begin
	//	Iterator stuff
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE typename Array< ElementType, BufferPolicy, MemClass >::const_iterator Array< ElementType, BufferPolicy, MemClass >::Begin() const
	{
		return TypedData();
	}

	////////////////////////////////////////////////////////////////////
	// End
	//	Iterator stuff
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE typename Array< ElementType, BufferPolicy, MemClass >::iterator Array< ElementType, BufferPolicy, MemClass >::End()
	{
		return TypedData() + m_bufferPolicy.Size();
	}

	////////////////////////////////////////////////////////////////////
	// End
	//	Iterator stuff
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE typename Array< ElementType, BufferPolicy, MemClass >::const_iterator Array< ElementType, BufferPolicy, MemClass >::End() const
	{
		return TypedData() + m_bufferPolicy.Size();
	}

	////////////////////////////////////////////////////////////////////
	// Erase
	//	Remove element at iterator and move any subsequent elements back 1 place
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::Erase( iterator it )
	{
		RED_ASSERT( it != End(), TXT( "Trying to erase invalid element" ) );
		TCopy::MoveBackwards( it, 1, static_cast< Red::System::Int32 >( End() - it - 1 ) );
		m_bufferPolicy.ResizeFast( m_bufferPolicy.Size() - 1, sizeof( ElementType ), MemClass );
	}

	////////////////////////////////////////////////////////////////////
	// Erase
	//	Remove any elements between start and end
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::Erase( iterator first, iterator last )
	{
		Red::System::Uint32 elementCount = static_cast< Red::System::Uint32 >( last - first );
		TCopy::MoveBackwards( first, last - first,  End() - first - elementCount );
		m_bufferPolicy.ResizeFast( m_bufferPolicy.Size() - elementCount, sizeof( ElementType ), MemClass );
	}

	////////////////////////////////////////////////////////////////////
	// EraseFast
	//	Remove element at iterator, avoid moving all subsequent elements
	//	Note, this changes the order of elements in the array
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::EraseFast( iterator it )
	{
		RED_ASSERT( it != End(), TXT( "Trying to erase invalid element" ) );
		if( it != ( End() - 1 ) )
		{ 
			*it = ElementMoveConstruct( *( End() - 1 ) );
		}
		TInit::Destruct( End() - 1, 1 );
		m_bufferPolicy.ResizeFast( m_bufferPolicy.Size() - 1, sizeof( ElementType ), MemClass );
	}

	////////////////////////////////////////////////////////////////////
	// Remove
	//	Remove single element with this value 
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::Remove( const ElementType& theElement )
	{
		iterator i = Algorithms::Find( Begin(), End(), theElement );
		if ( i != End() )
		{
			Erase( i );
			return true;
		}
		else
		{
			return false;
		}
	}

	////////////////////////////////////////////////////////////////////
	// RemoveFast
	//	Remove single element with this value, fastpath
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::RemoveFast( const ElementType& theElement )
	{
		iterator i = Algorithms::Find( Begin(), End(), theElement );
		if ( i != End() )
		{
			EraseFast( i );
			return true;
		}
		else
		{
			return false;
		}
	}

	////////////////////////////////////////////////////////////////////
	// RemoveAt
	//	Remove element at the index
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::RemoveAt( Red::System::Uint32 index )
	{
		RED_ASSERT( index < m_bufferPolicy.Size(), TXT( "Invalid index passed to RemoveAt()" ) );
		TCopy::MoveBackwards( TypedData() + index, 1, static_cast< Red::System::Int32 >(m_bufferPolicy.Size() - index - 1) );
		m_bufferPolicy.ResizeFast( m_bufferPolicy.Size() - 1, sizeof( ElementType ), MemClass );
	}

	////////////////////////////////////////////////////////////////////
	// RemoveAtFast
	//	Remove element at the index, fastpath
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::RemoveAtFast( Red::System::Uint32 index )
	{
		RED_ASSERT( index < m_bufferPolicy.Size(), TXT( "Invalid index passed to RemoveAt()" ) );
		TypedData()[ index ] = ElementMoveConstruct( Back() );
		m_bufferPolicy.ResizeFast( m_bufferPolicy.Size() - 1, sizeof( ElementType ), MemClass );
	}

	////////////////////////////////////////////////////////////////////
	// CopyFast
	//	Avoids resizes where possible
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Array< ElementType, BufferPolicy, MemClass >& Array< ElementType, BufferPolicy, MemClass >::CopyFast( const Array< ElementType, BufferPolicy, MemClass >& other )
	{
		if( &other != this )
		{
			TInit::Destruct( TypedData(), m_bufferPolicy.Size() );
			m_bufferPolicy.ResizeFast( other.Size(), sizeof( ElementType ), MemClass );
			TInit::CopyConstruct( TypedData(), other.TypedData(), m_bufferPolicy.Size() );
		}
		return *this;
	}

	////////////////////////////////////////////////////////////////////
	// SubArray
	//	Extract part of an array and creates a new array containing those elements
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Array< ElementType, BufferPolicy, MemClass > Array< ElementType, BufferPolicy, MemClass >::SubArray( Red::System::Uint32 startIndex, Red::System::Uint32 count ) const
	{
		RED_ASSERT( startIndex + count <= m_bufferPolicy.Size(), TXT( "SubArray parameters are out of array bounds" ) );
		Red::System::Uint32 start = Red::Math::NumericalUtils::Min( startIndex, m_bufferPolicy.Size() );
		Red::System::Uint32 end = count == 0 ? m_bufferPolicy.Size() : Red::Math::NumericalUtils::Min( startIndex + count, m_bufferPolicy.Size() );

		Array< ElementType, BufferPolicy, MemClass > destArray;
		destArray.Reserve( count );
		for( Red::System::Uint32 i = start; i < end; ++i )
		{
			destArray.PushBack( *( TypedData() + i ) );
		}

		return destArray;
	}

	////////////////////////////////////////////////////////////////////
	// RemoveEmptyPointers
	//	Remove any elements that are null pointers from the array (changes the size / capacity!)
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE void Array< ElementType, BufferPolicy, MemClass >::RemoveEmptyPointers()
	{
		// Remove all empty elements from array
		// Uses int64 to iterate as it avoids unsigned - signed int overflow when counting down
		Red::System::Int64 size = static_cast< Red::System::Int64 >( m_bufferPolicy.Size() );
		for ( Red::System::Int64 i=size-1; i>=0; --i )
		{
			if ( *( TypedData() + i ) == nullptr )
			{
				Erase( Begin() + i );
			}
		}

		// Shrink to reduce memory
		Shrink();
	}

	////////////////////////////////////////////////////////////////////
	// FindPtr
	//	Search the array for an element, returns a pointer to it if found
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE const ElementType* Array< ElementType, BufferPolicy, MemClass >::FindPtr( const ElementType& element ) const
	{
		iterator it = Algorithms::Find( Begin(), End(), element );
		if( it != End() )
		{
			return it;
		}

		return nullptr;
	}

	////////////////////////////////////////////////////////////////////
	// FindPtr
	//	Search the array for an element, returns a pointer to it if found
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE ElementType* Array< ElementType, BufferPolicy, MemClass >::FindPtr( const ElementType& element )
	{
		iterator it = Algorithms::Find( Begin(), End(), element );
		if( it != End() )
		{
			return it;
		}

		return nullptr;
	}

	////////////////////////////////////////////////////////////////////
	// GetIndex
	//	Returns index of the found element, or -1 if it doesn't exist
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Red::System::Uint32 Array< ElementType, BufferPolicy, MemClass >::GetIndex( const ElementType& theElement ) const
	{
		const_iterator i = Algorithms::Find( Begin(), End(), theElement );
		if ( i != End() )
		{
			return static_cast< Red::System::Uint32 >( i - Begin() );
		}
		else
		{
			return (Red::System::Uint32)-1;
		}
	}

	////////////////////////////////////////////////////////////////////
	// GetIndex
	//	Returns index of the found element, or -1 if it doesn't exist
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Red::System::Uint32 Array< ElementType, BufferPolicy, MemClass >::GetIndex( const_iterator theElement ) const
	{
		if ( theElement != End() )
		{
			return static_cast< Red::System::Uint32 >( theElement - Begin() );
		}
		else
		{
			return (Red::System::Uint32)-1;
		}
	}

	////////////////////////////////////////////////////////////////////
	// Exist
	//	Returns true if the element exists in the array
	template< class ElementType, class BufferPolicy, Red::MemoryFramework::MemoryClass MemClass >
	RED_INLINE Red::System::Bool Array< ElementType, BufferPolicy, MemClass >::Exist( const ElementType& element ) const
	{
		return Algorithms::Find( Begin(), End(), element ) != End();
	}

} }
