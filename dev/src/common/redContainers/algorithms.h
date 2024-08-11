/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef ALGORITHMS_H
#define ALGORITHMS_H

/////////////////////////////////////////////////////////////
// Common algorithms used by the various container classes

namespace Red { namespace Containers { namespace Algorithms {

	///////////////////////////////////////////////////////////////////
	// Swap
	//	Swap the value of 2 elements
	template < class ElementType > 
	RED_INLINE void Swap(ElementType& a, ElementType& b)								
	{ 
		const ElementType tmp = a;
		a = b;	
		b = tmp; 
	}

	///////////////////////////////////////////////////////////////////
	// Find
	//	Linear search for element via iterators
	template <class _Iter, class _Val>
	RED_INLINE _Iter Find( _Iter begin, _Iter end, const _Val &val )
	{
		while( begin != end )
		{
			if ( *begin == val )
				return begin;

			++begin;
		}
		return end;
	}

	////////////////////////////////////////////////////////////////////
	// LowerBound
	//	Binary search for lower bound value
	//	Comparator object should have overloaded() operator
	template<class Iterator, class ElementType, class Comparator>
	RED_INLINE Iterator LowerBound( Iterator begin, Iterator end, const ElementType &val, const Comparator &compare )
	{
		Red::System::MemSize count = (end - begin);
		while( count > 0 )
		{
			Red::System::MemSize halfCount = count >> 1;
			Iterator middle = begin + halfCount;
			if ( compare( *middle, val ) )
			{
				begin = middle+1;
				count = count - halfCount - 1;
			}
			else
			{
				count = halfCount;
			}	
		}
		return begin;
	}

	////////////////////////////////////////////////////////////////////////////
	// PushHeap
	//	Push itEnd - 1 element into a container as a heap
	template < class Comparator, class Iterator >
	RED_INLINE void PushHeap( Iterator itBegin, Iterator itEnd )
	{
		Iterator itLookatElem = itEnd - 1;
		Red::System::Uint32 nIndex = static_cast< Red::System::Uint32 >(itLookatElem - itBegin);
		Comparator compare;

		// While until the root
		while (nIndex > 0)
		{
			// Go 1 level deeper into the heap
			nIndex = (nIndex - 1) >> 1;
			Iterator itNextNode = itBegin + nIndex;
			// Check if the heap order is secured
			if ( compare( *itNextNode, *itLookatElem ) )
			{
				return;
			}
			Swap(*itLookatElem,*itNextNode);
			itLookatElem = itNextNode;
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// PopHeap
	//	Move the element at itBegin to the end of the heap array (itEnd - 1)
	template < class Comparator, class Iterator >
	RED_INLINE void PopHeap( Iterator itBegin, Iterator itEnd )
	{
		Comparator compare;
		Iterator itLastElem = itEnd - 1;
		if (itBegin == itLastElem)
			return;

		// Move the element to the end, then rebuild the heap for elements after it (i.e. nodes smaller than the value)
		Swap( *itBegin,*itLastElem );

		// Get first element smaller then currently inserted one
		Red::System::Uint32 nIndex = 0;
		const Red::System::Uint32 nEndIndex = static_cast< Red::System::Uint32 >(itEnd - itBegin) - 1;			// -1 because last element just got removed
		Iterator it = itBegin;
		while( true )
		{
			// Check if we are in leaf or single connected node
			Red::System::Uint32 nIndex1 = (nIndex << 1) + 1;
			Red::System::Uint32 nIndex2 = (nIndex << 1) + 2;
			if (nIndex2 >= nEndIndex)
			{
				if (nIndex1 < nEndIndex)
				{
					Iterator it1 = itBegin + nIndex1;
					if (compare(*it1,*it))
					{
						Swap( *it, *it1 );
					}
				}
				break;
			}
			Iterator it1 = itBegin + nIndex1;
			Iterator it2 = itBegin + nIndex2;
			// Get smallest element
			if ( compare( *it1, *it ) )
			{
				// it1 < it
				if ( compare( *it2, *it1 ) )
				{
					// it2 < it1 < it
					Swap( *it, *it2 );
					nIndex = nIndex2;
					it = it2;
				}
				else
				{
					// it1 <= it2 < it
					Swap( *it, *it1 );
					nIndex = nIndex1;
					it = it1;
				}
			}
			else if (compare( *it2, *it ))
			{
				// it2 < it && !(it1 < it)
				Swap( *it, *it2 );
				nIndex = nIndex2;
				it = it2;
			}
			else
			{
				// !(it2 < it || it1 < it)
				break;
			}
		}
	}

} } }


#endif