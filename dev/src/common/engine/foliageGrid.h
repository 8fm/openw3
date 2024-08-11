/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _ENGINE_FOLIAGE_GRID_H_
#define _ENGINE_FOLIAGE_GRID_H_

#include "foliageForward.h"
#include "../core/uniquePtr.h"
#include "../core/weakPtr.h"

struct SFoliageGridSetupParameters
{
	Vector2 dimension;
	Vector2 cellDimension;
	const IFoliageResourceLoader * cellLoader;
};

class CFoliageGrid
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:

	CFoliageGrid();
	RED_MOCKABLE ~CFoliageGrid();

	void Setup( const SFoliageGridSetupParameters & param );

	RED_MOCKABLE CellHandleContainer AcquireVisibleCells( const Vector2 & position, Int32 depth );  // C++03 RVO or C++11 move constructor will be use here.
	RED_MOCKABLE CellHandle AcquireCell( const Vector2 & position );
	CellHandleContainer AcquireCells( const Box & box );

	Int32 GetCellCountX() const;
	Int32 GetCellCountY() const;

private:
	friend class CFoliageCellIterator;

	typedef Red::TWeakPtr< CFoliageCell > InternalCellHandle;
	typedef TDynArray< InternalCellHandle > CellContainer;
	typedef TPair< Int32, Int32 > CellCoordinate;

	CellHandleContainer AcquireCellRange( const CellCoordinate & begin, const CellCoordinate & end );
	CellHandle AcquireCell( const CellCoordinate& pos );

	CellCoordinate PositionToGridCoordinate( const Vector2 & position ) const;
	Int32 GridCoordinateToArrayIndex( const CellCoordinate& coordinate ) const;
	Vector2 GridCoordinateToPosition( const CellCoordinate& coordinate ) const;
	
	SFoliageGridSetupParameters m_param;
	CellContainer m_cellContainer;

	Int32 m_cellCountX;
	Int32 m_cellCountY;
};

Red::TUniquePtr< CFoliageGrid > CreateFoliageGrid( const SFoliageGridSetupParameters & param );

#endif
