#pragma once

// Makes TCustomIntrusiveList::Exist() (used to assert on Add/Remove) check all elements until it finds given one
//#define ENABLE_DEBUG_INTRUSIVE_LIST_SLOW

template < typename TYPE >
struct TCustomIntrusiveListPtr
{
	TYPE* m_ptr;
	RED_FORCE_INLINE TCustomIntrusiveListPtr() : m_ptr( nullptr ) {}
	RED_FORCE_INLINE ~TCustomIntrusiveListPtr() { ASSERT( !m_ptr, TXT("Element has not been removed from the TCustomIntrusiveList.") ); }
};

/**
 *	General purpose (doubly linked) intrusive list of elements.
 *
 *	Note: Intrusive indicates that the 'next' and 'previous' are stored inside of the elements.
 *	No memory allocation is done while managing the list.
 *
 *	Features:
 *	- supports adding and removing elements during iteration
 *	- resumable iterator
 *	- thanks to TCustomIntrusiveListElementAccessor it is possible to store single element on several
 *	  different intrusive lists; for each one the TCustomIntrusiveListElementAccessor should implement
 *	  appropriate accessors for previous and next elements
 */
template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
class TCustomIntrusiveList
{
public:
	class safe_iterator;

private:
	ELEMENT_TYPE*						m_first;		// Pointer to the first element
	ELEMENT_TYPE*						m_last;			// Pointer to the last element
	TCustomIntrusiveListElementAccessor	m_accessor;		// Next and previous element accessor
	Uint32								m_length;		// List length

	safe_iterator*						m_safeIterator;	// Safe iterator (only single instance allowed) or nullptr

public:
	// Default iterator
	class iterator
	{
		friend class TCustomIntrusiveList;
	private:
		TCustomIntrusiveList* m_list;
		ELEMENT_TYPE* m_current;
		RED_FORCE_INLINE iterator( TCustomIntrusiveList* list ) : m_list( list ), m_current( list->m_first ) {}
	public:
		RED_FORCE_INLINE iterator() : m_list( nullptr ), m_current( nullptr ) {}
		RED_FORCE_INLINE iterator( const iterator& it ) : m_list( it.m_list ), m_current( it.m_current ) {}
		RED_FORCE_INLINE iterator& operator = ( iterator& it ) { m_list = it.m_list, m_current = it.m_current; return *this; }
		RED_FORCE_INLINE ELEMENT_TYPE* operator*() const { return m_current; }
		RED_FORCE_INLINE ELEMENT_TYPE* operator->() const { return m_current; }
		RED_FORCE_INLINE Bool operator == ( const iterator& it ) const { return m_current == it.m_current; }
		RED_FORCE_INLINE Bool operator != ( const iterator& it ) const { return m_current != it.m_current; }
		RED_FORCE_INLINE iterator& operator ++ () { m_current = m_list->m_accessor.GetNext( m_current ); return *this; }
	};

	// Iterator supporting removal and adding elements during iteration; NOTE: Only single instance of valid (i.e. non end) safe iterator per list allowed at any time
	class safe_iterator
	{
		friend class TCustomIntrusiveList;
	private:
		TCustomIntrusiveList* m_list;
		ELEMENT_TYPE* m_current;
		ELEMENT_TYPE* m_end;
	public:
		RED_FORCE_INLINE safe_iterator() : m_list( nullptr ), m_current( nullptr ), m_end( nullptr ) {}
		RED_FORCE_INLINE safe_iterator( const safe_iterator& it ) : m_list( nullptr ), m_current( nullptr ), m_end( nullptr ) { ASSERT( !it.m_current, TXT("Can not have more than 1 valid (i.e. non End()) safe_iterator instances at the same time.") ); }	// Forbidden
		RED_FORCE_INLINE safe_iterator( TCustomIntrusiveList* list ) { Init( list ); }
		RED_FORCE_INLINE void Init( TCustomIntrusiveList* list ) { ASSERT( !list->m_safeIterator, TXT("Can not have more than 1 valid (i.e. non End()) safe_iterator instances at the same time.") ); list->m_safeIterator = this; m_list = list; m_current = list->m_first; m_end = m_current; }
		RED_FORCE_INLINE ~safe_iterator() { if ( m_list ) { m_list->m_safeIterator = nullptr; } }
		RED_FORCE_INLINE ELEMENT_TYPE* operator*() const { return m_current; }
		RED_FORCE_INLINE ELEMENT_TYPE* operator->() const { return m_current; }
		RED_FORCE_INLINE safe_iterator& operator = ( const safe_iterator& it ) { m_list = nullptr; m_current = nullptr; m_end = nullptr; ASSERT( !it.m_current, TXT("Can not have more than 1 valid (i.e. non End()) safe_iterator instances at the same time.") ); return *this; }
		RED_FORCE_INLINE Bool operator == ( const safe_iterator& it ) const { return m_current == it.m_current; }
		RED_FORCE_INLINE Bool operator != ( const safe_iterator& it ) const { return m_current != it.m_current; }
		RED_FORCE_INLINE Bool IsEnd() const { return !m_current; }
		RED_FORCE_INLINE safe_iterator& operator ++ ()
		{
			if ( !m_current )
			{
				return *this;
			}

			if ( !( m_current = m_list->m_accessor.GetNext( m_current ) ) )
			{
				m_current = m_list->m_first;
			}
			if ( m_current == m_end )
			{
				m_current = nullptr;
			}
			return *this;
		}
		RED_FORCE_INLINE void Begin( Bool fromStart = false )
		{
			if ( !m_list->m_first )
			{
				m_current = nullptr;
				return;
			}
			if ( !m_current || fromStart )
			{
				m_current = m_list->m_first;
			}
			m_end = m_current;
		}
	};

	TCustomIntrusiveList();
	~TCustomIntrusiveList();

	// Sets element accessor
	void SetElementAccessor( TCustomIntrusiveListElementAccessor& accessor );
	// Gets element accessor
	TCustomIntrusiveListElementAccessor& GetElementAccessor();
	// Gets list length
	RED_FORCE_INLINE Uint32 Length() const;
	// Clears the list
	void Clear();
	// Adds stored elements to given array; NOTE: does not clear given array
	void Fill( TDynArray< ELEMENT_TYPE* >& elements );

	// Adds new element at the end of the list
	RED_FORCE_INLINE void PushBack( ELEMENT_TYPE* element );
	// Removes random element from the list; fixes up iterator if needed
	RED_FORCE_INLINE void Remove( ELEMENT_TYPE* element );
	// Gets whether given element is on the list NOTE: only works assuming given element is either on this list or is not on any list (won't work if it is on other list)
	RED_FORCE_INLINE Bool Exist( ELEMENT_TYPE* element );

	RED_FORCE_INLINE iterator Begin();
	RED_FORCE_INLINE iterator End();

	// Safe iterator: supports adding/removing elements during iteration

	// NOTE: BeginSafe() is commented out on purpose (no support for copy operator); to initialize safe iterator use safe_iterator's constructor or Init() function
	// RED_FORCE_INLINE safe_iterator BeginSafe();
	RED_FORCE_INLINE safe_iterator EndSafe();
};

// Enable C++11 range-based for loop

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
typename TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::iterator begin( TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >& list ) { return list.Begin(); }
template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
typename TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::iterator end( TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >& list ) { return list.End(); }

// Template implementation

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::TCustomIntrusiveList()
	: m_first( nullptr )
	, m_last( nullptr )
	, m_length( 0 )
	, m_safeIterator( nullptr )
{}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::~TCustomIntrusiveList()
{
	if ( m_safeIterator )
	{
		m_safeIterator->m_list = nullptr;
	}
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
typename TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::iterator TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::Begin()
{
	return iterator( this );
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
typename TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::iterator TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::End()
{
	return iterator();
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
typename TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::safe_iterator TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::EndSafe()
{
	return safe_iterator();
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
void TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::SetElementAccessor( TCustomIntrusiveListElementAccessor& accessor )
{
	m_accessor = accessor;
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
TCustomIntrusiveListElementAccessor& TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::GetElementAccessor()
{
	return m_accessor;
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
RED_FORCE_INLINE Uint32 TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::Length() const
{
	return m_length;
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
void TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::Clear()
{
	ELEMENT_TYPE* current = m_first;
	while ( current )
	{
		ELEMENT_TYPE* next = m_accessor.GetNext( current );
		m_accessor.SetPrev( current, nullptr );
		m_accessor.SetNext( current, nullptr );
		current = next;
	}

	m_first = m_last = nullptr;
	m_length = 0;
	if ( m_safeIterator )
	{
		m_safeIterator->m_current = m_safeIterator->m_end = nullptr;
	}
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
void TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::Fill( TDynArray< ELEMENT_TYPE* >& elements )
{
	ELEMENT_TYPE* current = m_first;
	while ( current )
	{
		elements.PushBack( current );
		current = m_accessor.GetNext( current );
	}
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
RED_FORCE_INLINE Bool TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::Exist( ELEMENT_TYPE* element )
{
#ifdef ENABLE_DEBUG_INTRUSIVE_LIST_SLOW
	ELEMENT_TYPE* current = m_first;
	while ( current )
	{
		if ( current == element )
		{
			return true;
		}
		current = m_accessor.GetNext( current );
	}
	return false;
#else
	return																	// We have 2 cases here:
		m_accessor.GetPrev( element ) || m_accessor.GetNext( element ) ||	// 1) 2+ elements on the list -> must have valid next or prev
		m_first == element;													// 2) 1 element on the list -> must be the first one
#endif
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
RED_FORCE_INLINE void TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::PushBack( ELEMENT_TYPE* element )
{
	ASSERT( !Exist( element ) );

	if ( !m_last )
	{
		m_last = m_first = element;
	}
	else
	{
		m_accessor.SetNext( m_last, element );
		m_accessor.SetPrev( element, m_last );
		m_last = element;
	}

	++m_length;
}

template < typename ELEMENT_TYPE, typename TCustomIntrusiveListElementAccessor >
RED_FORCE_INLINE void TCustomIntrusiveList< ELEMENT_TYPE, TCustomIntrusiveListElementAccessor >::Remove( ELEMENT_TYPE* element )
{
	ASSERT( Exist( element ) );

	ELEMENT_TYPE* prev = m_accessor.GetPrev( element );
	ELEMENT_TYPE* next = m_accessor.GetNext( element );

	// Remove element from the list 

	if ( m_first == element )
	{
		m_first = next;
	}
	if ( m_last == element )
	{
		m_last = prev;
	}

	if ( prev )
	{
		m_accessor.SetNext( prev, next );
		m_accessor.SetPrev( element, nullptr );
	}

	if ( next )
	{
		m_accessor.SetPrev( next, prev );
		m_accessor.SetNext( element, nullptr );
	}

	--m_length;

	// Update iterator in case removed element affects it

	if ( m_safeIterator )
	{
		if ( m_safeIterator->m_current == element )
		{
			m_safeIterator->m_current = next ? next : m_first;
		}

		if ( m_safeIterator->m_end == element )
		{
			m_safeIterator->m_end = next ? next : m_first;
		}
	}
}

//! Gets the name of intrusive list element accessor for a given type and a 'list purpose name'
#define CUSTOM_INTRUSIVE_LIST_ELEMENT_ACCESSOR_CLASS( type, name )			\
	type::CustomIntrusiveListElementAccessor_##name

//! Defines default intrusive list element accessor for a given type and a 'list purpose name'; to be inserted inside of the body of the stored list element
#define DEFINE_CUSTOM_INTRUSIVE_LIST_ELEMENT( type, name )					\
private:																	\
	TCustomIntrusiveListPtr< type > m_prevElement_##name;					\
	TCustomIntrusiveListPtr< type > m_nextElement_##name;					\
public:																		\
	struct CustomIntrusiveListElementAccessor_##name						\
	{																		\
		friend class type;													\
		RED_FORCE_INLINE type* GetPrev( type* element ) const				\
		{																	\
			return element->m_prevElement_##name.m_ptr;						\
		}																	\
		RED_FORCE_INLINE void SetPrev( type* element, type* prev )			\
		{																	\
			element->m_prevElement_##name.m_ptr = prev;						\
		}																	\
		RED_FORCE_INLINE type* GetNext( type* element ) const				\
		{																	\
			return element->m_nextElement_##name.m_ptr;						\
		}																	\
		RED_FORCE_INLINE void SetNext( type* element, type* next )			\
		{																	\
			element->m_nextElement_##name.m_ptr = next;						\
		}																	\
	};
