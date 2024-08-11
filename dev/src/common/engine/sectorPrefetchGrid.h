#pragma once

class CSectorPrefetchQueryResult;

/// World grid with cached prefetch data for resources
/// TEMPORARY SOLUTION FOR W3 ONLY
class CSectorPrefetchGrid
{
public:
	CSectorPrefetchGrid();

	/// Load grid from file
	Bool Load( const String& absoluteFilePath );

	/// Save grid to file
	Bool Save( const String& absoluteFilePath );
	Bool Save( IFile* file );

	/// Get cells for given position
	CSectorPrefetchQueryResult QueryCellsAtPosition( const Vector& pos ) const;

public: // tempshit
	struct GridHeader
	{
		Uint32		m_magic;
		Float		m_worldSize;
		Uint32		m_numCells;
		Uint32		m_numEntries;
	};

	struct GridCellEntry
	{
		Uint32		m_dataOffset;
		Uint32		m_dataSize;
		Uint32		m_uncompressedSize;
		Uint16		m_compressionType;
		Uint16		m_cellIndex;
		Uint64		m_resourceHash;
	};

	struct GridCell
	{
		Uint32		m_level;
		Uint32		m_cellX;
		Uint32		m_cellY;

		Uint32		m_firstEntry; // index of the first entry in this cell
		Uint32		m_numEntries; // number of entries in this grid cell

		Uint32		m_dataSize;		// size of the grid cell data on disk
		Uint64		m_dataOffset;	// offset of the cell data on disk
	};

	struct GridLevel
	{
		TDynArray< Int16 >	m_cellIndices;
		Uint32				m_cellsPerSide;
		Float				m_cellSize;
		Float				m_cellScale;
		Float				m_cellBase;

		void Setup( const Float worldSize, const Uint32 level );

		Int16 GetCellIndexForPos( const Vector& x ) const;

		Int16 GetCellIndex( const Uint32 x, const Uint32 y ) const;
		void SetCellIndex( const Uint32 x, const Uint32 y, const Int16 cellIndex );
	};

	Float						m_worldSize;

	TDynArray< GridCell >		m_cells;
	TDynArray< GridCellEntry >	m_entries;
	TDynArray< GridLevel >		m_levels;

private:
	static const Uint32 MAGIC;
};
