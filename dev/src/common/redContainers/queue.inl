namespace Red { namespace Containers {

	//////////////////////////////////////////////////////////////////////
	// CTor
	//
	template< class ArrayType >
	Queue<ArrayType>::Queue()
		: m_numKeys( 0 )
		, m_backIndex( 0 )
		, m_frontIndex( 0 )
	{

	}

	//////////////////////////////////////////////////////////////////////
	// Copy Ctor
	//
	template< class ArrayType >
	Queue<ArrayType>::Queue( const Queue<ArrayType>& other )
		: m_numKeys( other.m_numKeys )
		, m_backIndex( other.m_backIndex )
		, m_frontIndex( other.m_frontIndex )
		, m_array( other.m_array )
	{

	}

	//////////////////////////////////////////////////////////////////////
	// Move Ctor 
	//
	template< class ArrayType >
	Queue<ArrayType>::Queue( Queue<ArrayType>&& other )
		: m_numKeys( other.m_numKeys )
		, m_backIndex( other.m_backIndex )
		, m_frontIndex( other.m_frontIndex )
		, m_array( ElementMoveConstruct( other.m_array ) )
	{
		other.m_numKeys = 0;
		other.m_backIndex = 0;
		other.m_frontIndex = 0;
	}

	//////////////////////////////////////////////////////////////////////
	// Dtor 
	//
	template< class ArrayType >
	Queue<ArrayType>::~Queue()
	{

	}

	//////////////////////////////////////////////////////////////////////
	// Copy assign 
	//
	template< class ArrayType >
	Queue<ArrayType>& Queue<ArrayType>::operator=( const Queue<ArrayType>& other )
	{
		m_array = other.m_array;
		m_numKeys = other.m_numKeys;
		m_backIndex = other.m_backIndex;
		m_frontIndex = other.m_frontIndex;
	}

	//////////////////////////////////////////////////////////////////////
	// Move assignment 
	//
	template< class ArrayType >
	Queue<ArrayType>& Queue<ArrayType>::operator=( Queue<ArrayType>&& other )
	{
		m_array = ElementMoveConstruct( other.m_array );
		m_numKeys = other.m_numKeys;
		m_backIndex = other.m_backIndex;
		m_frontIndex = other.m_frontIndex;
		other.m_numKeys = 0;
		other.m_backIndex = 0;
		other.m_frontIndex = 0;
	}

	//////////////////////////////////////////////////////////////////////
	// Size
	//	Returns number of keys (not the size of the array)
	template< class ArrayType >
	Red::System::Uint32 Queue<ArrayType>::Size()
	{
		return m_numKeys;
	}

	//////////////////////////////////////////////////////////////////////
	// Capacity 
	//
	template< class ArrayType >
	Red::System::Uint32 Queue<ArrayType>::Capacity()
	{
		return m_array.Capacity();
	}

	//////////////////////////////////////////////////////////////////////
	// Empty
	//
	template< class ArrayType >
	Red::System::Bool Queue<ArrayType>::Empty()
	{
		return m_numKeys == 0;
	}

	//////////////////////////////////////////////////////////////////////
	// Reserve
	//
	template< class ArrayType >
	void Queue<ArrayType>::Reserve( Red::System::Uint32 elementCount )
	{
		m_array.Reserve( elementCount );
	}

	//////////////////////////////////////////////////////////////////////
	// Clear
	//	Remove all elements
	template< class ArrayType >
	void Queue<ArrayType>::Clear()
	{
		m_array.Clear();
		m_numKeys = 0;
		m_backIndex = 0;
		m_frontIndex = 0;
	}

	//////////////////////////////////////////////////////////////////////
	// Push
	//
	template< class ArrayType >
	void Queue<ArrayType>::Push( const typename Queue<ArrayType>::value_type& value )
	{
		if( m_numKeys == 0 )
		{
			if( m_array.Size() == 0 )
			{
				m_array.PushBack( value );
			}
			else
			{
				m_array[m_backIndex] = value;
			}
		}
		else
		{
			if( m_backIndex == m_frontIndex )
			{
				// Grow array, because we are out of space also increment frontIndex because we moved elements
				m_array.Insert( m_backIndex, value );
				m_frontIndex++;
			}
			else
			{
				m_array[m_backIndex] = value;
			}
		}
		m_backIndex = (m_backIndex+1) % m_array.Size();
		m_numKeys++;
	}

	//////////////////////////////////////////////////////////////////////
	// Pop
	//
	template< class ArrayType >
	void Queue<ArrayType>::Pop()
	{
		RED_ASSERT( m_numKeys > 0 );
		m_frontIndex = (m_frontIndex+1) % m_array.Size();
		m_numKeys--;
	}

	//////////////////////////////////////////////////////////////////////
	// Front element const accessor
	//
	template< class ArrayType >
	const typename Queue<ArrayType>::value_type& Queue<ArrayType>::Front() const
	{
		RED_ASSERT( m_numKeys > 0 );
		return m_array[ m_frontIndex ];
	}

	//////////////////////////////////////////////////////////////////////
	// Front element accessor 
	//
	template< class ArrayType >
	typename Queue<ArrayType>::value_type& Queue<ArrayType>::Front()
	{
		RED_ASSERT( m_numKeys > 0 );
		return m_array[ m_frontIndex ];
	}

	//////////////////////////////////////////////////////////////////////
	// Back element const accessor
	//
	template< class ArrayType >
	const typename Queue<ArrayType>::value_type& Queue<ArrayType>::Back() const
	{
		RED_ASSERT( m_numKeys > 0 );
		return m_array[ (m_array.Size()+m_backIndex-1) % m_array.Size() ];
	}

	//////////////////////////////////////////////////////////////////////
	// Back element accessor 
	//
	template< class ArrayType >
	typename Queue<ArrayType>::value_type& Queue<ArrayType>::Back()
	{
		RED_ASSERT( m_numKeys > 0 );
		return m_array[ (m_array.Size()+m_backIndex-1) % m_array.Size() ];
	}

	//////////////////////////////////////////////////////////////////////
	// Equality operator
	//
	template< class ArrayType >
	Red::System::Bool Queue<ArrayType>::operator==( const Queue<ArrayType>& other )
	{
		if( m_numKeys == other.m_numKeys )
		{
			for( Red::System::Uint32 i = 0; i < m_numKeys; ++i )
			{
				if ( m_array[ ( m_frontIndex+i ) % m_array.Size() ] != other.m_array[ ( other.m_frontIndex+i ) % other.m_array.Size() ] )
				{
					return false;
				}
			}
			return true;
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////
	// Inequality operator
	//
	template< class ArrayType >
	Red::System::Bool Queue<ArrayType>::operator!=( const Queue<ArrayType>& other )
	{
		return !( *this == other );
	}

} } 