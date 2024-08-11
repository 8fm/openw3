/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "foliageGrid.h"
#include "foliageCell.h"

const Float cellPositionEpsilon = 0.001f;

CFoliageGrid::CFoliageGrid()
	: m_cellCountX( 0 )
	, m_cellCountY( 0 )
{}

CFoliageGrid::~CFoliageGrid()
{}

void CFoliageGrid::Setup( const SFoliageGridSetupParameters & param )
{
	RED_FATAL_ASSERT( param.cellDimension.X >= 1.0f && param.cellDimension.Y >= 1.0f, "Cell dimension must be at least 1" );

	m_param = param;
	
	m_cellCountX = Max( 1, static_cast< Int32 >( MCeil( m_param.dimension.X / m_param.cellDimension.X ) ) );
	m_cellCountY = Max( 1, static_cast< Int32 >( MCeil( m_param.dimension.Y / m_param.cellDimension.Y ) ) );
	m_cellContainer.Resize( m_cellCountX * m_cellCountY );
}

CellHandleContainer CFoliageGrid::AcquireVisibleCells( const Vector2 & position, Int32 depth )
{
	if( !m_cellContainer.Empty() )
	{
		CellCoordinate coordinate = PositionToGridCoordinate( position );
		const CellCoordinate begin( coordinate.m_first - depth, coordinate.m_second - depth );
		const CellCoordinate end( coordinate.m_first + depth, coordinate.m_second + depth );

		return AcquireCellRange( begin, end );
	}

	return CellHandleContainer();
}

CellHandle CFoliageGrid::AcquireCell( const Vector2 & position )
{
	if( !m_cellContainer.Empty() )
	{
		auto coordinate = PositionToGridCoordinate( position );
		return AcquireCell( coordinate );
	}

	return CellHandle();
}

CellHandleContainer CFoliageGrid::AcquireCells( const Box & box )
{
	if( !m_cellContainer.Empty() )
	{
		Float eps = Min( box.CalcSize().Mag2(), cellPositionEpsilon );
		CellCoordinate begin = PositionToGridCoordinate( box.Min );
		CellCoordinate end = PositionToGridCoordinate( box.Max - eps );

		return AcquireCellRange( begin, end );
	}

	return CellHandleContainer();
}

CellHandleContainer CFoliageGrid::AcquireCellRange( const CellCoordinate & begin, const CellCoordinate & end )
{
	const Int32 startX = Max( 0, begin.m_first );
	const Int32 startY = Max( 0, begin.m_second );
	const Int32 endX = Min( end.m_first + 1, m_cellCountX ); 
	const Int32 endY = Min( end.m_second + 1, m_cellCountY ); 

	CellHandleContainer cells;
	cells.Reserve( ( endX - startX ) * ( endY - startY ) );
	for( Int32 indexY = startY; indexY != endY; ++indexY )
	{
		for( Int32 indexX = startX; indexX != endX; ++indexX )
		{
			cells.PushBack( AcquireCell( CellCoordinate( indexX, indexY ) ) );
		}
	}

	return cells;
}

CellHandle CFoliageGrid::AcquireCell( const CellCoordinate & coordinate )
{
	const Int32 index = GridCoordinateToArrayIndex( coordinate ); 
	CellHandle handle = m_cellContainer[ index ].Lock();
	if( !handle )
	{
		const Vector2 position = GridCoordinateToPosition( coordinate );
		handle = CreateFoliageCell( position, m_param.cellLoader );
		m_cellContainer[ index ] = handle;
	}

	return handle;
}

CFoliageGrid::CellCoordinate CFoliageGrid::PositionToGridCoordinate( const Vector2 & position ) const
{
	Int32 x = static_cast< Int32 >( MFloor( ( position.X + m_param.dimension.X / 2 ) / m_param.cellDimension.X ) );
	Int32 y = static_cast< Int32 >( MFloor( ( position.Y + m_param.dimension.Y / 2 ) / m_param.cellDimension.Y ) );

	x = Max( 0, Min( x, m_cellCountX - 1 ) );
	y = Max( 0, Min( y, m_cellCountY - 1 ) );

	return MakePair( x, y );
}

Int32 CFoliageGrid::GridCoordinateToArrayIndex( const CellCoordinate & coordinate ) const
{
	return coordinate.m_second * m_cellCountX + coordinate.m_first;
}

Vector2 CFoliageGrid::GridCoordinateToPosition( const CellCoordinate & coordinate ) const
{
	const Float positionX = coordinate.m_first  * m_param.cellDimension.X - ( m_param.dimension.X / 2 );
	const Float positionY = coordinate.m_second * m_param.cellDimension.Y - ( m_param.dimension.Y / 2 );
	return Vector2( positionX, positionY );
}

Int32 CFoliageGrid::GetCellCountX() const
{
	return m_cellCountX;
}

Int32 CFoliageGrid::GetCellCountY() const
{
	return m_cellCountY;
}

Red::TUniquePtr< CFoliageGrid > CreateFoliageGrid( const SFoliageGridSetupParameters & param )
{
	Red::TUniquePtr< CFoliageGrid > grid( new CFoliageGrid );
	grid->Setup( param );
	return grid;
}
