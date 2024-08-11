
#include "build.h"
#include "foliageCellIterator.h"
#include "foliageCell.h"

CFoliageCellIterator::CFoliageCellIterator( CFoliageGrid* grid, const Box & box )
	: m_grid( grid )
{
	m_begin = m_grid->PositionToGridCoordinate( box.Min );
	m_end   = m_grid->PositionToGridCoordinate( box.Max );
	m_current = m_begin;

	if ( m_begin.m_first < m_grid->GetCellCountX() && m_begin.m_second < m_grid->GetCellCountY() )
	{
		Acquire();
	}
}

CFoliageCell& CFoliageCellIterator::operator* ( ) 
{ 
	return *m_currentCell.Get(); 
}

const CFoliageCell& CFoliageCellIterator::operator* ( ) const 
{ 
	return *m_currentCell.Get(); 
}

CFoliageCell* CFoliageCellIterator::operator-> ( ) 
{ 
	return m_currentCell.Get(); 
}

const CFoliageCell* CFoliageCellIterator::operator-> ( ) const 
{
	return m_currentCell.Get(); 
}

CellHandle CFoliageCellIterator::Get( )
{
	return m_currentCell;
}

Uint32 CFoliageCellIterator::GetTotalCellCount() const
{
	return ( m_end.m_first - m_begin.m_first ) * ( m_end.m_second - m_begin.m_second );
}

CFoliageCellIterator::operator Bool( ) const
{
	return m_currentCell;
}

void CFoliageCellIterator::operator ++ ( )
{
	if ( m_currentCell )
	{
		if ( ++m_current.m_first > m_end.m_first )
		{
			m_current.m_first = m_begin.m_first;
			++m_current.m_second;
		}

		Acquire();
	}
}

void CFoliageCellIterator::Acquire()
{
	if ( m_current.m_first <= m_end.m_first && m_current.m_second <= m_end.m_second )
	{
		m_currentCell = m_grid->AcquireCell( m_current );
	}
	else
	{
		m_currentCell.Reset(); // iterated beyond the container
	}
}
