/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "resourceGrid.h"

#pragma optimize ("",off)

namespace ResourceGrid {

//---------------------------------------------------------------------------

CResourceRef::CResourceRef( const String& depotPath )
	: m_depotPath( depotPath )
	, m_numUniqueRefs( 0 )
	, m_numSharedRefs( 0 )
{
}

void CResourceRef::AddUniqueRef()
{
	m_numUniqueRefs += 1;
}

void CResourceRef::AddSharedRef()
{
	m_numSharedRefs += 1;
}

void CResourceRef::RemoveSharedRef()
{
	m_numSharedRefs -= 1;
}


//---------------------------------------------------------------------------

CGridCell::CGridCell()
	: m_level(nullptr)
	, m_cellX(0)
	, m_cellY(0)
{
	// hierarchy
	m_parent = nullptr;
	m_children[0] = nullptr;
	m_children[1] = nullptr;
	m_children[2] = nullptr;
	m_children[3] = nullptr;
}

void CGridCell::Setup( CGridLevel* level, const Uint32 gridX, const Uint32 gridY )
{
	m_level = level;
	m_cellX = gridX;
	m_cellY = gridY;
}

void CGridCell::Link( CGridCell* parentCell, Uint32 childIndex )
{
	RED_FATAL_ASSERT( parentCell != nullptr, "Invalid parent cell" );
	RED_FATAL_ASSERT( m_parent == nullptr, "Cell already linked to parent" );
	RED_FATAL_ASSERT( childIndex <= 3, "Invalid child index" );
	RED_FATAL_ASSERT( parentCell->m_children[childIndex] == nullptr, "Parent cell already linked with child" );

	m_parent = parentCell;
	parentCell->m_children[ childIndex ] = this;
}

const Bool CGridCell::AddResourceReference( CResourceRef* res )
{
	if ( m_resourcesSet.Insert( res ) )
	{
		m_resources.PushBack( res );
		res->AddSharedRef();
		return true;
	}

	return false;
}

const Bool CGridCell::RemoveResourceReference( CResourceRef* res )
{
	if ( m_resourcesSet.Erase( res ) )
	{
		m_resources.Remove( res );
		res->RemoveSharedRef();
		return true;
	}

	return false;
}

const Bool CGridCell::HasResourceReference( CResourceRef* res ) const
{
	return m_resourcesSet.Exist( res );
}

//---------------------------------------------------------------------------

CGridLevel::CGridLevel( const Float worldSize, const Uint32 levelIndex )
	: m_levelIndex( levelIndex )
	, m_cellsPerSize( 1 << m_levelIndex )
{
	// initialize cells
	m_cells.Resize( m_cellsPerSize*m_cellsPerSize );

	// scale factors
	m_gridCellSize = worldSize / m_cellsPerSize;
	m_gridBase = -worldSize * 0.5f;
	m_gridScale = 1.0f / m_gridCellSize;

	// set them up
	for ( Uint32 y=0; y<m_cellsPerSize; ++y )
	{
		for ( Uint32 x=0; x<m_cellsPerSize; ++x )
		{
			auto& cell = m_cells[ x + y*m_cellsPerSize ];
			cell.Setup( this, x, y );
		}
	}
}

CGridLevel::~CGridLevel()
{
}

CGridCell* CGridLevel::GetCell( const Uint32 gridX, const Uint32 gridY )
{
	if ( gridX >= m_cellsPerSize || gridY >= m_cellsPerSize )
		return nullptr;

	return &m_cells[ gridX + gridY*m_cellsPerSize ];
}

namespace Helper
{
	const Bool CheckRangeOverlap( const Box2& box, const Vector& pos, const Float radius )
	{
		// center in rectangle
		if ( box.Contains( pos.X, pos.Y ) )
			return true;

		// one of the corners are in the circle
		const Float radSq = radius*radius;
		if ( pos.DistanceSquaredTo2D( Vector( box.Min.X, box.Min.Y, 0 ) ) < radSq )
			return true;
		if ( pos.DistanceSquaredTo2D( Vector( box.Min.X, box.Max.Y, 0 ) ) < radSq )
			return true;
		if ( pos.DistanceSquaredTo2D( Vector( box.Max.X, box.Min.Y, 0 ) ) < radSq )
			return true;
		if ( pos.DistanceSquaredTo2D( Vector( box.Max.X, box.Max.Y, 0 ) ) < radSq )
			return true;

		return false;
	}
}

const Bool CGridLevel::AddResourceReference( CResourceRef* res, const Vector& pos, const Float radius )
{
	const Int32 gridMinX = Clamp< Int32 >( (Int32)( (pos.X - radius - m_gridBase) * m_gridScale ) - 1, 0, m_cellsPerSize-1 );
	const Int32 gridMinY = Clamp< Int32 >( (Int32)( (pos.Y - radius - m_gridBase) * m_gridScale ) - 1, 0, m_cellsPerSize-1 );
	const Int32 gridMaxX = Clamp< Int32 >( (Int32)( (pos.X + radius - m_gridBase) * m_gridScale ) + 1, 0, m_cellsPerSize-1 );
	const Int32 gridMaxY = Clamp< Int32 >( (Int32)( (pos.Y + radius - m_gridBase) * m_gridScale ) + 1, 0, m_cellsPerSize-1 );

	Bool added = false;
	for ( Int32 gridY=gridMinY; gridY <= gridMaxY; ++gridY )
	{
		for ( Int32 gridX=gridMinX; gridX <= gridMaxX; ++gridX )
		{
			auto& cell = m_cells[ gridX + gridY*m_cellsPerSize ];

			// calculate accurate grid bbox
			Box2 cellBox;
			cellBox.Min.X = m_gridBase + gridX*m_gridCellSize;
			cellBox.Min.Y = m_gridBase + gridY*m_gridCellSize;
			cellBox.Max.X = cellBox.Min.X + m_gridCellSize;
			cellBox.Max.Y = cellBox.Min.Y + m_gridCellSize;

			// do accurate test to prevent shitty cells from being reported
			if ( Helper::CheckRangeOverlap( cellBox, pos, radius ) )
			{
				added |= m_cells[ gridX + gridY*m_cellsPerSize ].AddResourceReference( res );
			}
		}
	}

	return added;
}

void CGridLevel::GetFilledCells( TDynArray< const CGridCell* >& outGridCells ) const
{
	for ( const auto& cell : m_cells )
	{
		if ( !cell.IsEmpty() )
		{
			outGridCells.PushBack( &cell );
		}
	}
}

//---------------------------------------------------------------------------

CGrid::CGrid( const Float worldSize, const Float minCellLevel )
	: m_worldSize( worldSize )
	, m_minCellSize( minCellLevel )
{
	Uint32 level = 0;
	Float size = m_worldSize;

	CGridLevel* prevGridLevel = nullptr;
	while ( size > m_minCellSize )
	{
		// create new grid level
		CGridLevel* curLevel = new CGridLevel( worldSize, level );
		m_levels.PushBack( curLevel );

		// link this grid with parent cells
		if ( prevGridLevel != nullptr )
		{
			const Uint32 gridSize = curLevel->GetGridSize();
			for ( Uint32 gridY=0; gridY<gridSize; ++gridY )
			{
				for ( Uint32 gridX=0; gridX<gridSize; ++gridX )
				{
					// coordinates of the parent grid cell
					const Uint32 parentGridX = gridX / 2;
					const Uint32 parentGridY = gridY / 2;

					// child index (0-3)
					const Uint32 childIndexX = gridX & 1;
					const Uint32 childIndexY = gridY & 1;
					const Uint32 childIndex = childIndexX + childIndexY*2;

					// get parent cell
					CGridCell* parentCell = prevGridLevel->GetCell( parentGridX, parentGridY );
					RED_FATAL_ASSERT( parentCell != nullptr, "Parent grid is messed up" );

					// get this 
					CGridCell* currentCell = curLevel->GetCell( gridX, gridY );
					RED_FATAL_ASSERT( currentCell != nullptr, "Current grid is messed up" );

					// link shit
					currentCell->Link( parentCell, childIndex );
				}
			}
		}

		// step down
		prevGridLevel = curLevel;
		size /= 2.0f;
		level += 1;
	}
}

CGrid::~CGrid()
{
	m_levels.ClearPtr();
	m_allResources.ClearPtr();
	m_resourcesMap.Clear();
}

CGridLevel* CGrid::GetRootLevel() const
{
	return m_levels[0];
}

CGridCell* CGrid::GetRootCell() const
{
	return m_levels[0]->GetCell(0,0);
}

CResourceRef* CGrid::GetResource( const String& depotPath )
{
	CResourceRef* ret = nullptr;
	if ( !m_resourcesMap.Find( depotPath, ret ) )
	{
		ret = new CResourceRef( depotPath );
		m_resourcesMap.Insert( depotPath, ret );
		m_allResources.PushBack( ret );
	}

	return ret;
}

const Bool CGrid::AddResourceReference( CResourceRef* res, const Vector& pos, const Float radius )
{
	// find the grid level matching the resource radius
	Uint32 levelIndex = 0;
	Float cellSize = m_worldSize;
	while ( levelIndex < (m_levels.Size()-1) )
	{
		if ( radius > cellSize )
			break;

		levelIndex += 1;
		cellSize /= 2.0f;
	}

	// add to level
	auto* level = m_levels[ levelIndex ];
	return level->AddResourceReference( res, pos, radius );
}

void CGrid::GetFilledCells( TDynArray< const CGridCell* >& outGridCells ) const
{
	for ( const auto* level : m_levels )
		level->GetFilledCells( outGridCells );
}

const Box CGrid::ComputeBox( const Uint32 gridLevel, const Uint32 gridX, const Uint32 gridY ) const
{
	const Float cellSize = m_worldSize / (Float)(1 << gridLevel);

	Box ret;
	ret.Min.X = m_gridBase + cellSize * gridX;
	ret.Min.Y = m_gridBase + cellSize * gridY;
	ret.Min.Z = -m_worldSize;
	ret.Max.X = ret.Min.X + cellSize;
	ret.Max.Y = ret.Min.Y + cellSize;
	ret.Min.Z = m_worldSize;
	return ret;
}

//---------------------------------------------------------------------------

} // namespace ResourceGrid

//---------------------------------------------------------------------------
