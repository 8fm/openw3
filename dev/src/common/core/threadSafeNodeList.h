/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma once

template< Int32 N, class T >
class ThreadSafeNodeList
{
private:
	Red::Threads::CAtomic< Int32 >		m_count;
	T*									m_list[ N ];
	volatile bool						m_overflow;

public:
	ThreadSafeNodeList()
		: m_count( 0 )
		, m_overflow( false )
	{};

	void AddAsync( T* node )
	{
		Int32 index = m_count.Increment();
		if ( index < N )
		{
			// the -1 is here becaues increment returns post incremented value
			m_list[ index-1 ] = node;
		}
		else
		{
			m_overflow = true;
		}
	}

	Bool IsOverflow() const
	{
		return m_overflow;
	}

	void Reset()
	{
		m_overflow = false;
		m_count.SetValue( 0 );
	}

	Int32 Size() const
	{
		return m_count.GetValue();
	}

	T* operator[]( Int32 index ) const
	{
		RED_FATAL_ASSERT( index < N, "" );
		return m_list[ index ];
	}

	Bool ResetAt( Int32 index )
	{
		Int32 currIndex = m_count.GetValue();
		if ( currIndex > index )
		{
			m_list[ index ] = NULL;
			return true;
		}
		return false;
	}
};
