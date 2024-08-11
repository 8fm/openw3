namespace Red { namespace Containers {

	///////////////////////////////////////////////////////////////////
	// Default CTor
	//
	template< class ArrayType, class Comparator >
	RED_INLINE ArrayMultiMap< ArrayType, Comparator >::ArrayMultiMap()
		: SortedArray< ArrayType, Comparator >()
	{
	}

	///////////////////////////////////////////////////////////////////
	// Move CTor
	//
	template< class ArrayType, class Comparator >
	RED_INLINE ArrayMultiMap< ArrayType, Comparator >::ArrayMultiMap( ArrayMultiMap< ArrayType, Comparator >&& other )
		: SortedArray< ArrayType, Comparator >( (SortedArray< ArrayType, Comparator >&&)other )
	{
	}

	///////////////////////////////////////////////////////////////////
	// Copy CTor
	//
	template< class ArrayType, class Comparator >
	RED_INLINE ArrayMultiMap< ArrayType, Comparator >::ArrayMultiMap( const ArrayMultiMap< ArrayType, Comparator >& other )
		: SortedArray< ArrayType, Comparator >( ( const SortedArray< ArrayType, Comparator >& )other )
	{
	}

	///////////////////////////////////////////////////////////////////
	// DTor
	//
	template< class ArrayType, class Comparator >
	RED_INLINE ArrayMultiMap< ArrayType, Comparator >::~ArrayMultiMap()
	{
	}

	///////////////////////////////////////////////////////////////////
	// Copy assignment
	//
	template< class ArrayType, class Comparator >
	RED_INLINE ArrayMultiMap< ArrayType, Comparator >& ArrayMultiMap< ArrayType, Comparator >::operator=( const ArrayMultiMap< ArrayType, Comparator >& other )
	{
		SortedArray< ArrayType, Comparator >::operator=( ( const SortedArray< ArrayType, Comparator >& )other );
	}

	///////////////////////////////////////////////////////////////////
	// Move assignment
	//
	template< class ArrayType, class Comparator >
	RED_INLINE ArrayMultiMap< ArrayType, Comparator >& ArrayMultiMap< ArrayType, Comparator >::operator=( const ArrayMultiMap< ArrayType, Comparator >&& other )
	{
		SortedArray< ArrayType, Comparator >::operator=( ( SortedArray< ArrayType, Comparator >&& )other );
	}

	///////////////////////////////////////////////////////////////////
	// Insert
	//	Insert a pair
	template< class ArrayType, class Comparator >
	RED_INLINE typename ArrayType::iterator ArrayMultiMap< ArrayType, Comparator >::Insert(const key_type& cKey, const elem_type& cElem)
	{
		return SortedArray< ArrayType, Comparator >::Insert( typename ArrayType::value_type( cKey, cElem ) );
	}

	///////////////////////////////////////////////////////////////////
	// Find
	//	Find a pair iterator based on key
	template< class ArrayType, class Comparator >
	RED_INLINE typename ArrayType::iterator ArrayMultiMap< ArrayType, Comparator >::Find(const key_type& cKey)
	{
		Comparator comparator;
		typename ArrayType::iterator foundIt = Algorithms::LowerBound( ArrayType::Begin(), ArrayType::End(), cKey, comparator );
		if( foundIt != ArrayType::End() && !comparator( cKey, *foundIt ) )
		{
			return foundIt;
		}
		else
		{
			return ArrayType::End();
		}
	}

	///////////////////////////////////////////////////////////////////
	// Find
	//	Find a const iterator based on key
	template< class ArrayType, class Comparator >
	RED_INLINE typename ArrayType::const_iterator ArrayMultiMap< ArrayType, Comparator >::Find(const key_type& cKey) const
	{
		Comparator comparator;
		typename ArrayType::const_iterator foundIt = Algorithms::LowerBound( ArrayType::Begin(), ArrayType::End(), cKey, comparator );
		if( foundIt != ArrayType::End() && !comparator( cKey, *foundIt ) )
		{
			return foundIt;
		}
		else
		{
			return ArrayType::End();
		}
	}

	///////////////////////////////////////////////////////////////////
	// Erase
	//	Remove an element based on key, return true if erase success
	template< class ArrayType, class Comparator >
	RED_INLINE Red::System::Bool ArrayMultiMap< ArrayType, Comparator >::Erase(const key_type& cKey)
	{
		Comparator comparator;
		typename ArrayType::iterator foundIt = Algorithms::LowerBound( ArrayType::Begin(), ArrayType::End(), cKey, comparator );
		if( foundIt != ArrayType::End() && !comparator( cKey, *foundIt ) )
		{
			ArrayType::Erase( foundIt );
			return true;
		}
		else
		{
			return false;
		}
	}

	///////////////////////////////////////////////////////////////////
	// Erase
	//	Remove an element based on iterator
	template< class ArrayType, class Comparator >
	void ArrayMultiMap< ArrayType, Comparator >::Erase( typename ArrayType::iterator it )
	{
		ArrayType::Erase( it );
	}

} }