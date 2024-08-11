#pragma once

////////////////////////////////////////////////////////////////////////////
// For debug purposes
template < class Compare, class Iterator >
Bool IsHeap(Iterator itBegin,Iterator itEnd,const Compare& cOrder)
{
	if (itBegin == itEnd)
		return true;
	Uint32 nIndex = 0;
	for (Iterator it = itBegin; ; it++, nIndex++)
	{
		// Check if this node sons are higher by given order
		Iterator it1 = itBegin + (nIndex << 1) + 1;
		if (it1 == itEnd)
			return true;
		if (cOrder(*it1,*it))
			return false;
		Iterator it2 = itBegin + (nIndex << 1) + 2;
		if (it2 == itEnd)
			return true;
		if (cOrder(*it2,*it))
			return false;
	}
}

////////////////////////////////////////////////////////////////////////////
// PushHeap
// Push itEnd - 1 element into the heap.
template < class Compare, class Iterator >
RED_INLINE void PushHeap(Iterator itBegin, Iterator itEnd, const Compare& cOrder)
{
	Iterator itLookatElem = itEnd - 1;
	Uint32 nIndex = Uint32(itLookatElem - itBegin);
	// While until the root
	while (nIndex > 0)
	{
		// Go 1 level deeper into the heap
		nIndex = (nIndex - 1) >> 1;
		Iterator itNextNode = itBegin + nIndex;
		// Check if the heap order is secured
		if (cOrder(*itNextNode,*itLookatElem))
		{
			return;
		}
		::Swap(*itLookatElem,*itNextNode);
		itLookatElem = itNextNode;
	}
}

////////////////////////////////////////////////////////////////////////////
// PopHeap
// Push itBegin element out of heap (to itEnd - 1 position).
template < class Compare, class Iterator >
void PopHeap(Iterator itBegin, Iterator itEnd, const Compare& cOrder)
{
	Iterator itLastElem = itEnd - 1;
	if (itBegin == itLastElem)
		return;
	::Swap(*itBegin,*itLastElem);

	// Get first element smaller then currently inserted one
	Uint32 nIndex = 0;
	const Uint32 nEndIndex = Uint32(itEnd - itBegin) - 1;			// -1 because last element just 'poped'
	Iterator it = itBegin;
	for( ;; )
	{
		// Check if we are in leaf or single connected node
		Uint32 nIndex1 = (nIndex << 1) + 1;
		Uint32 nIndex2 = (nIndex << 1) + 2;
		if (nIndex2 >= nEndIndex)
		{
			if (nIndex1 < nEndIndex)
			{
				Iterator it1 = itBegin + nIndex1;
				if (cOrder(*it1,*it))
				{
					::Swap(*it,*it1);
				}
			}
			break;
		}
		Iterator it1 = itBegin + nIndex1;
		Iterator it2 = itBegin + nIndex2;
		// Get smallest element
		if (cOrder(*it1,*it))
		{
			// it1 < it
			if (cOrder(*it2,*it1))
			{
				// it2 < it1 < it
				::Swap(*it,*it2);
				nIndex = nIndex2;
				it = it2;
			}
			else
			{
				// it1 <= it2 < it
				::Swap(*it,*it1);
				nIndex = nIndex1;
				it = it1;
			}
		}
		else if (cOrder(*it2,*it))
		{
			// it2 < it && !(it1 < it)
			::Swap(*it,*it2);
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

template < class Compare, class Iterator >
void MakeHeap( Iterator itBegin, Iterator itEnd, const Compare& cOrder )
{
	for (Iterator it = itBegin + 1; it != itEnd + 1; ++it)
	{
		::PushHeap(itBegin,it,cOrder);
	}
}
////////////////////////////////////////////////////////////////////////////
// Random access collection reversing
template < class Iterator >
RED_INLINE void Reverse(Iterator itBegin, Iterator itEnd)
{
	itEnd--;
	while (itBegin < itEnd)
	{
		::Swap(*itBegin,*itEnd);
		itBegin++;
		itEnd--;
	}
};

////////////////////////////////////////////////////////////////////////////
// Heap structure
template < class T, class CompareFunc = DefaultCompareFunc< T >, EMemoryClass MC_Type = MC_HeapContainer >
class THeap : public TDynArray< T, MC_Type >
{
	struct SPred
	{
		template< class _Type1, class _Type2 >
		RED_INLINE Bool operator()( const _Type1& lhs, const _Type2& rhs ) const
		{
			return CompareFunc::Less( lhs, rhs );
		}
	};

private:
	typedef TDynArray< T, MC_Type > tSuper;

public:
	typedef typename tSuper::iterator iterator;
	typedef typename tSuper::const_iterator const_iterator;

public:
	using tSuper::Begin;
	using tSuper::End;
	using tSuper::PopBackFast;
	using tSuper::TypedData;
	using tSuper::PushBack;
	using tSuper::Size;

public:
	THeap() : tSuper() {}
	THeap( EMemoryClass memoryClass ) : tSuper( memoryClass ) {}
	THeap( Uint32 nArraySize ) : tSuper( nArraySize ) {}
	THeap( THeap&& heap )
		: tSuper( std::forward< THeap >( heap ) )
	{
	}

	void PushHeap( T&& c )
	{
		PushBack( c );
		::PushHeap( Begin(), End(), SPred() );
	}

	void PushHeap(const T& c)
	{
		PushBack(c);
		::PushHeap(Begin(),End(),SPred());
	}

	void PopHeap()
	{
		::PopHeap(Begin(),End(),SPred());
		PopBackFast();
	}
	// After direct-accessing heap with DynArray methods we can make it heap in place
	template< typename Iterator >
	void MakeHeap()
	{
		SPred order;
		iterator itBegin = Begin();
		iterator itEnd = End();
		for (Iterator it = itBegin + 1; it != itEnd + 1; ++it)
		{
			::PushHeap(itBegin,it,order);
		}
	}

	// GOTCHA: The reference to the front element is only valid until the heap is modified !
	T& Front()
	{
		return TypedData()[0];
	}

	// GOTCHA: The reference to the front element is only valid until the heap is modified !
	const T& Front() const
	{
		return TypedData()[0];
	}

	Bool CheckHeap() const
	{
		return ::IsHeap(Begin(),End(),SPred());
	}
	// This is special functionality to update heap when order function for given element has changed.
	// Ye I know it looks ugly.
	void UpdateHeapElement(iterator it)
	{
		SPred comperator;
		T* dataArray = TypedData();
		Uint32 i = ( Uint32)( it - Begin() );
		Uint32 size = Size();
		// Update heap
		// if element position 'got better'?
		if (i > 0 && comperator(dataArray[i], dataArray[(i-1) >> 1]))
		{
			do 
			{
				::Swap(dataArray[(i-1) >> 1], dataArray[i]);
				i = (i-1) >> 1;
			}
			while (i > 0 && comperator(dataArray[i], dataArray[(i-1) >> 1]));
			return;
		}
		// if element position 'got worse'?
		while ((i << 1)+1 < size)
		{
			Uint32 j;
			if ( (i << 1)+2 < size && comperator(dataArray[(i << 1)+2], dataArray[(i << 1)+1]))
			{
				j = (i << 1) + 2;
			}
			else
			{
				j = (i << 1) + 1;
			}
			if (comperator(dataArray[j], dataArray[i]))
			{
				::Swap(dataArray[j], dataArray[i]);
				i = j;
			}
			else
			{
				break;
			}
		}
	}
};
