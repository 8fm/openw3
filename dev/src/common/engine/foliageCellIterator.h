/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef _ENGINE_FOLIAGE_CELL_ITERATOR_H_
#define _ENGINE_FOLIAGE_CELL_ITERATOR_H_

#include "foliageForward.h"
#include "foliageGrid.h"

//! This class allows to iterate through ALL cells of the foliage. Notice, that only one cell at time
//! (current) is guaranteed to be accessible, so do not store references to them for later use.
class CFoliageCellIterator
{
public:
	CFoliageCellIterator( CFoliageGrid* grid, const Box & box );

	CFoliageCell& operator* ( );

	const CFoliageCell& operator* ( ) const;

	CFoliageCell* operator-> ( );

	const CFoliageCell* operator-> ( ) const;

	operator Bool() const;

	void operator ++ ( );

	CellHandle Get();

	Uint32 GetTotalCellCount() const;

private:
	void Acquire();

	CFoliageGrid::CellCoordinate m_begin;
	CFoliageGrid::CellCoordinate m_end;
	CFoliageGrid::CellCoordinate m_current;

	CFoliageGrid* m_grid;
	CellHandle m_currentCell;
};


#endif
