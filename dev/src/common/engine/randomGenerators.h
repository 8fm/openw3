
#pragma once

class CRandomIndexPool
{
private:
	Uint32				m_min;
	Uint32				m_max;
	TDynArray< Bool >	m_history;
	Uint32				m_freeSize;

#ifndef NDEBUG
	TDynArray< Uint32 >	m_temp;
#endif

public:
	RED_INLINE CRandomIndexPool()
	{
		Setup( 0, 1 );
	}

	RED_INLINE CRandomIndexPool( Uint32 min, Uint32 max ) 
	{
		Setup( min, max );
	}

	RED_INLINE void Setup( Uint32 min, Uint32 max )
	{
		m_min = min;
		m_max = max;

		Uint32 size = m_max - m_min;

		m_history.Resize( size );

		Reset();
	}

	RED_INLINE void Reset()
	{
		m_freeSize = m_history.Size();
		Red::System::MemoryZero( m_history.Data(), m_history.DataSize() );
	}

	// This is not const operation
	Uint32 Rand();

private:
	RED_INLINE void Reset( Uint32 lastIndex )
	{
		Reset();

		const Uint32 size = m_history.Size();
		if ( size > 1 )
		{
			m_history[ lastIndex ] = true;

			--m_freeSize;
		}

#ifndef NDEBUG
		m_temp.Clear();
#endif
	}
};
