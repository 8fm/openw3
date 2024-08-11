/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorPrefetchGrid.h"
#include "sectorPrefetchQuery.h"

const Uint32 CSectorPrefetchGrid::MAGIC = 'DIRG'; // GRID

CSectorPrefetchGrid::CSectorPrefetchGrid()
{
}

void CSectorPrefetchGrid::GridLevel::Setup( const Float worldSize, const Uint32 level )
{
	m_cellsPerSide = 1 << level;
	m_cellSize = worldSize / (Float)m_cellsPerSide;
	m_cellScale = 1.0f / m_cellSize; // not safe
	m_cellBase = -worldSize * 0.5f;

	m_cellIndices.Resize( m_cellsPerSide*m_cellsPerSide );
	for ( Uint32 i=0; i<m_cellIndices.Size(); ++i )
		m_cellIndices[i] = -1;
}

Int16 CSectorPrefetchGrid::GridLevel::GetCellIndexForPos( const Vector& x ) const
{
	const Int32 gridX = (Int32)( (x.X - m_cellBase) * m_cellScale );
	const Int32 gridY = (Int32)( (x.Y - m_cellBase) * m_cellScale );
	return GetCellIndex( (Uint32)gridX, (Uint32)gridY );
}

Int16 CSectorPrefetchGrid::GridLevel::GetCellIndex( const Uint32 x, const Uint32 y ) const
{
	if (x >= m_cellsPerSide || y >= m_cellsPerSide)
		return -1;

	return m_cellIndices[ x + y*m_cellsPerSide ];
}

void CSectorPrefetchGrid::GridLevel::SetCellIndex( const Uint32 x, const Uint32 y, const Int16 cellIndex )
{
	if (x >= m_cellsPerSide || y >= m_cellsPerSide)
		return;

	m_cellIndices[ x + y*m_cellsPerSide ] = cellIndex;
}

Bool CSectorPrefetchGrid::Load( const String& absoluteFilePath )
{
	Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( absoluteFilePath, FOF_AbsolutePath ) );

	// file is not there
	if ( !file )
		return false;

	// load header
	GridHeader header;
	Red::MemoryZero( &header, sizeof(header) );
	file->Serialize( &header, sizeof(header ) );
	if ( header.m_magic != MAGIC )
	{
		ERR_ENGINE( TXT("SectorPrefetch grid format is invalid, got magic %08X"), header.m_magic );
		return false;
	}

	// set world size
	m_worldSize = header.m_worldSize;

	// load grid cells
	LOG_ENGINE( TXT("Loading %d grid cells"), header.m_numCells );
	m_cells.Resize( header.m_numCells );
	file->Serialize( m_cells.Data(), m_cells.DataSize() );

	// load grid entries
	LOG_ENGINE( TXT("Loading %d grid entries"), header.m_numEntries );
	m_entries.Resize( header.m_numEntries );
	file->Serialize( m_entries.Data(), m_entries.DataSize() );

	// get max level of the grid
	Uint32 maxGridLevel = 0;
	for ( const auto& cell : m_cells )
	{
		maxGridLevel = Max< Uint32 >( maxGridLevel, cell.m_level );
	}
	LOG_ENGINE( TXT("Max grid level: %d (world size: %f)"), maxGridLevel, header.m_worldSize );

	// prepare grid levels
	m_levels.Resize( maxGridLevel+1 );
	for ( Uint32 i=0; i<m_levels.Size(); ++i )
	{
		auto& level = m_levels[i];
		level.Setup( header.m_worldSize, i );
	}

	// register the cells in the grid for fast lookup
	for ( Uint32 i=0; i<m_cells.Size(); ++i )
	{
		const auto& cell = m_cells[i];
		m_levels[ cell.m_level ].SetCellIndex( cell.m_cellX, cell.m_cellY, i );
	}

	// done
	return true;
}

Bool CSectorPrefetchGrid::Save( const String& absoluteFilePath )
{
	Red::TScopedPtr< IFile > file( GFileManager->CreateFileWriter( absoluteFilePath, FOF_AbsolutePath ) );
	return Save( file.Get() );
}

Bool CSectorPrefetchGrid::Save( IFile* file )
{
	// no file
	if ( !file )
		return false;

	// setup header
	GridHeader header;
	header.m_magic = MAGIC;
	header.m_numCells = m_cells.Size();
	header.m_numEntries = m_entries.Size();
	header.m_worldSize = m_worldSize;
	file->Serialize( &header, sizeof(header) );

	// save cells
	file->Serialize( m_cells.Data(), m_cells.DataSize() );

	// save entries
	file->Serialize( m_entries.Data(), m_entries.DataSize() );
	return true;
}

CSectorPrefetchQueryResult CSectorPrefetchGrid::QueryCellsAtPosition( const Vector& pos ) const
{
	CSectorPrefetchQueryResult ret( this );

	// process each level
	for ( const auto& level : m_levels )
	{
		const Int16 cellIndex = level.GetCellIndexForPos( pos );
		if ( cellIndex != -1 )
			ret.AddCell( cellIndex );
	}

	// return query result
	return ret;
}
