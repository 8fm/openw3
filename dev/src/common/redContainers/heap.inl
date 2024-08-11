#include "containersCommon.h"
#include "algorithms.h"

namespace Red { namespace Containers {

	///////////////////////////////////////////////////////////////////
	// Default CTor
	//
	template< class ArrayType, class Comparator >
	RED_INLINE Heap< ArrayType, Comparator >::Heap()
	{
	}

	///////////////////////////////////////////////////////////////////
	// Copy Ctor
	//
	template< class ArrayType, class Comparator >
	RED_INLINE Heap< ArrayType, Comparator >::Heap( const Heap& other )
		: m_array( other.m_array )
	{
	}

	///////////////////////////////////////////////////////////////////
	// Move CTor
	//
	template< class ArrayType, class Comparator >
	RED_INLINE Heap< ArrayType, Comparator >::Heap( Heap&& other )
		: m_array( ElementMoveConstruct( other.m_array ) )
	{
	}

	///////////////////////////////////////////////////////////////////
	// DTor
	//
	template< class ArrayType, class Comparator >
	RED_INLINE Heap< ArrayType, Comparator >::~Heap()
	{
	}

	///////////////////////////////////////////////////////////////////
	// Copy assign
	//
	template< class ArrayType, class Comparator >
	RED_INLINE Heap< ArrayType, Comparator >& Heap< ArrayType, Comparator >::operator=( const Heap& other )
	{
		m_array = other;
	}

	///////////////////////////////////////////////////////////////////
	// Move assign
	//
	template< class ArrayType, class Comparator >
	RED_INLINE Heap< ArrayType, Comparator >& Heap< ArrayType, Comparator >::operator=( Heap&& other )
	{
		m_array = ElementMoveConstruct( other );
	}

	///////////////////////////////////////////////////////////////////
	// Push 
	//  Add an element + rebuild the heap
	template< class ArrayType, class Comparator >
	RED_INLINE void Heap< ArrayType, Comparator >::Push( const typename ArrayType::value_type& element )
	{
		m_array.PushBack( element );
		Algorithms::PushHeap< Comparator, iterator >( m_array.Begin(), m_array.End() );
	}

	///////////////////////////////////////////////////////////////////
	// Pop
	//	Remove element with lowest index
	template< class ArrayType, class Comparator >
	RED_INLINE typename ArrayType::value_type Heap< ArrayType, Comparator >::Pop()
	{
		Algorithms::PopHeap< Comparator, iterator >( m_array.Begin(), m_array.End() );		// Moves the element to the end + reorders
		return m_array.PopBackFast();	
	}

	///////////////////////////////////////////////////////////////////
	// Resort
	//	Rebuilds the heap
	template< class ArrayType, class Comparator >
	RED_INLINE void Heap< ArrayType, Comparator >::Resort()
	{
		Comparator compare;
		for (typename ArrayType::iterator it = m_array.Begin() + 1; it != m_array.End() + 1; ++it)
		{
			Algorithms::PushHeap(m_array.Begin(),it,compare);
		}
	}

	///////////////////////////////////////////////////////////////////
	// UpdateElement
	//	Updates the heap when the chosen iterator element has changed.
	//	Faster than doing Pop() / Push() again
	template< class ArrayType, class Comparator >
	RED_INLINE void Heap< ArrayType, Comparator >::UpdateElement( iterator it )
	{
		Comparator compare;

		typename ArrayType::value_type* dataArray = m_array.TypedData();
		Red::System::Uint32 i = static_cast< Red::System::Uint32 >( it - Begin() );
		Red::System::Uint32 size = m_array.Size();

		// Update heap
		// if element position 'got better'?
		if (i > 0 && compare(dataArray[i], dataArray[(i-1) >> 1]))
		{
			do 
			{
				Algorithms::Swap(dataArray[(i-1) >> 1], dataArray[i]);
				i = (i-1) >> 1;
			}
			while (i > 0 && compare(dataArray[i], dataArray[(i-1) >> 1]));
		}
		else
		{
			// if element position 'got worse'?
			while ((i << 1)+1 < size)
			{
				Red::System::Uint32 j;
				if ( (i << 1)+2 < size && compare(dataArray[(i << 1)+2], dataArray[(i << 1)+1]))
				{
					j = (i << 1) + 2;
				}
				else
				{
					j = (i << 1) + 1;
				}
				if (compare(dataArray[j], dataArray[i]))
				{
					Algorithms::Swap(dataArray[j], dataArray[i]);
					i = j;
				}
				else
				{
					break;
				}
			}
		}
	}
	
	///////////////////////////////////////////////////////////////////
	// Begin
	//	Returns root node of the heap (Has highest value)
	template< class ArrayType, class Comparator >
	RED_INLINE typename ArrayType::iterator Heap< ArrayType, Comparator >::Begin()
	{
		return m_array.Begin();
	}

	///////////////////////////////////////////////////////////////////
	// End
	//	Returns end node (lowest value)
	template< class ArrayType, class Comparator >
	RED_INLINE typename ArrayType::iterator Heap< ArrayType, Comparator >::End()
	{
		return m_array.End();
	}

	///////////////////////////////////////////////////////////////////
	// Begin
	//	Returns root node of the heap (Has highest value)
	template< class ArrayType, class Comparator >
	RED_INLINE typename ArrayType::const_iterator Heap< ArrayType, Comparator >::Begin() const
	{
		return m_array.Begin();
	}

	///////////////////////////////////////////////////////////////////
	// End
	//	Returns end node (lowest value)
	template< class ArrayType, class Comparator >
	RED_INLINE typename ArrayType::const_iterator Heap< ArrayType, Comparator >::End() const
	{
		return m_array.End();
	}

	///////////////////////////////////////////////////////////////////
	// Element accessor
	//	Resort() / UpdateElement() should be called if the element is modified
	template< class ArrayType, class Comparator >
	RED_INLINE typename ArrayType::value_type& Heap< ArrayType, Comparator >::operator[]( Red::System::Uint32 index )
	{
		return m_array[ index ];
	}

	///////////////////////////////////////////////////////////////////
	// Const Element accessor
	//	
	template< class ArrayType, class Comparator >
	RED_INLINE const typename ArrayType::value_type& Heap< ArrayType, Comparator >::operator[]( Red::System::Uint32 index ) const
	{
		return m_array[ index ];
	}
} }