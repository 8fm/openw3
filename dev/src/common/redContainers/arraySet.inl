namespace Red { namespace Containers {

	//////////////////////////////////////////////////////////////////////
	// CTor
	//
	template< class ArrayType, class Comparator >
	ArraySet<ArrayType, Comparator>::ArraySet()
	{
		
	}

	//////////////////////////////////////////////////////////////////////
	// Copy CTor
	//
	template< class ArrayType, class Comparator >
	ArraySet<ArrayType, Comparator>::ArraySet( const ArraySet& other )
		: m_array( other.m_array )
	{

	}

	//////////////////////////////////////////////////////////////////////
	// Move CTor
	//
	template< class ArrayType, class Comparator >
	ArraySet<ArrayType, Comparator>::ArraySet( ArraySet&& other )
		: m_array( ElementMoveConstruct( other ) )
	{

	}

	//////////////////////////////////////////////////////////////////////
	// DTor
	//
	template< class ArrayType, class Comparator >
	ArraySet<ArrayType, Comparator>::~ArraySet()
	{

	}

	//////////////////////////////////////////////////////////////////////
	// Copy assign
	//
	template< class ArrayType, class Comparator >
	ArraySet<ArrayType, Comparator>& ArraySet<ArrayType, Comparator>::operator=( const ArraySet<ArrayType, Comparator>& other )
	{
		m_array = other.m_array;
	}

	//////////////////////////////////////////////////////////////////////
	// Move assign
	//
	template< class ArrayType, class Comparator >
	ArraySet<ArrayType, Comparator>& ArraySet<ArrayType, Comparator>::operator=( ArraySet<ArrayType, Comparator>&& other )
	{
		m_array = ElementMoveConstruct( other );
	}

	//////////////////////////////////////////////////////////////////////
	// Insert
	//
	template< class ArrayType, class Comparator >
	Red::System::Bool ArraySet<ArrayType, Comparator>::Insert( const value_type& value )
	{
		if( m_array.Exist( value ) )
		{
			return false;
		}

		return m_array.Insert( value ) != m_array.End();
	}

	//////////////////////////////////////////////////////////////////////
	// Find
	//
	template< class ArrayType, class Comparator >
	typename ArraySet<ArrayType, Comparator>::iterator ArraySet<ArrayType, Comparator>::Find( const value_type& value )
	{
		Red::System::Uint32 elementIndex = m_array.GetIndex( value );
		if( elementIndex == (Red::System::Uint32)-1 )
		{
			return m_array.End();
		}
		else
		{
			return m_array.Begin() + elementIndex;
		}
	}

	//////////////////////////////////////////////////////////////////////
	// Find
	//
	template< class ArrayType, class Comparator >
	typename ArraySet<ArrayType, Comparator>::const_iterator ArraySet<ArrayType, Comparator>::Find( const value_type& value ) const
	{
		Red::System::Uint32 elementIndex = m_array.GetIndex( value );
		if( elementIndex == (Red::System::Uint32)-1 )
		{
			return m_array.End();
		}
		else
		{
			return m_array.Begin() + elementIndex;
		}
	}

} }