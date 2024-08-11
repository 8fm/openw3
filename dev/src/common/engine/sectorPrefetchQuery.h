#pragma once

class CSectorPrefetchGrid;

/// World grid query support
/// TEMPORARY SOLUTION FOR W3 ONLY
class CSectorPrefetchQueryResult
{
public:
	CSectorPrefetchQueryResult();
	CSectorPrefetchQueryResult( const CSectorPrefetchGrid* grid );

	static const Uint32 MAX_CELLS = 16;

	const CSectorPrefetchGrid*	m_grid;

	Int16						m_cells[ MAX_CELLS ];
	Uint32						m_numCells;

	// compare if two query results are equal
	RED_FORCE_INLINE const Bool operator==( const CSectorPrefetchQueryResult& other ) const
	{
		if ( m_numCells != other.m_numCells )
			return false;

		for ( Uint32 i=0; i<m_numCells; ++i )
		{
			if ( m_cells[i] != other.m_cells[i] )
				return false;
		}

		return true;
	}

	// add cell to query
	RED_FORCE_INLINE void AddCell( const Int16 index )
	{
		if ( m_numCells < MAX_CELLS )
		{
			m_cells[ m_numCells ] = index;
			m_numCells += 1;
		}
	}
};

