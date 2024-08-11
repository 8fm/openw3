
#pragma once

namespace AnimCompressionUtils
{
	struct DataBuffer
	{
		int			m_numPoints;
		int			m_pointSize;
		float**		m_points; // m_points[ m_numPoints ][ m_sizePoint ]
		bool		m_isEmpty;

		DataBuffer() : m_isEmpty( true ), m_numPoints( 0 ), m_pointSize( 0 ), m_points( NULL )
		{

		}

		~DataBuffer()
		{
			ASSERT( m_isEmpty );
		}

		void Create( int numPoints, int pointSize )
		{
			ASSERT( m_isEmpty );

			m_numPoints = numPoints;
			m_pointSize = pointSize;

			m_points = new float*[ m_numPoints ];

			for ( int i=0; i<m_numPoints; ++i )
			{
				m_points[ i ] = new float[ m_pointSize ];
			}

			m_isEmpty = false;
		}

		void Clear()
		{
			for ( int i=0; i<m_numPoints; ++i )
			{
				delete [] m_points[ i ];
			}

			delete [] m_points;

			m_isEmpty = true;
		}

		float* operator[](int i)
		{
			ASSERT( (i >= 0) && (i < m_numPoints) );
			return m_points[ i ];
		}

		const float* operator[](int i) const
		{
			ASSERT( (i >= 0) && (i < m_numPoints) );
			return m_points[ i ];
		}
	};

	class UsedPointsTable
	{
		bool*	m_usedFlags;
		int		m_size;
		int		m_usedNum;

	public:
		UsedPointsTable() : m_usedFlags( NULL ), m_size( 0 ), m_usedNum( 0 ) {}

		~UsedPointsTable()
		{
			delete m_usedFlags;
			m_usedFlags = NULL;
		}

		void Create( int num )
		{
			m_size = num;
			m_usedFlags = new bool[ m_size ];
			m_usedNum = m_size;

			for ( int i=0; i<m_size; ++i )
			{
				m_usedFlags[ i ] = true;
			}
		}

		int Size() const
		{
			return m_size;
		}

		bool IsUsed( int i ) const
		{
			ASSERT( i >= 0 && i < m_size );
			return m_usedFlags[ i ];
		}

		void SetNotUsed( int i )
		{
			ASSERT( IsUsed( i ) );
			m_usedFlags[ i ] = false;
			m_usedNum--;
			ASSERT( m_usedNum >= 0 );
		}

		int GetNumberOfUsedPoints() const
		{
			return m_usedNum;
		}
	};

	class UsedPointsInterator : Red::System::NonCopyable
	{
		const UsedPointsTable&	m_usedPoints;
		int						m_curr;
		int						m_prev;
		int						m_next;

	public:
		UsedPointsInterator( const UsedPointsTable& usedPoints )
			: m_usedPoints( usedPoints )
			, m_curr( -1 )
			, m_prev( -1 )
			, m_next( 0 )
		{
			GoToNext(); // find next
			GoToNext(); // find curr
		}

		operator bool () const
		{
			return m_next != -1;
		}

		void operator++ ()
		{
			GoToNext();
		}

		int Curr() const
		{
			ASSERT( m_curr >= 0 );
			return m_curr;
		}

		int Prev() const
		{
			ASSERT( m_prev >= 0 );
			return m_prev;
		}

		int Next() const
		{
			ASSERT( m_next >= 0 );
			return m_next;
		}

	private:
		void GoToNext()
		{
			int temp = m_next;

			while ( m_next >= 0 )
			{
				m_next++;

				if ( m_next >= m_usedPoints.Size() )
				{
					m_next = -1;
					break;
				}

				if ( m_usedPoints.IsUsed( m_next ) )
				{
					m_prev = m_curr;
					m_curr = temp;
					break;
				}
			}
		}
	};
}
