/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace ResourceGrid
{
	class CResourceRef;
	class CGridLevel;
	class CGrid;

	/// Resource reference
	class CResourceRef
	{
	public:
		CResourceRef( const String& depotPath );

		RED_FORCE_INLINE const String& GetDepotPath() const { return m_depotPath; }
		RED_FORCE_INLINE const Uint32 GetNumUniqueRefs() const { return m_numUniqueRefs; }
		RED_FORCE_INLINE const Uint32 GetNumSharedRefs() const { return m_numSharedRefs; }

		void AddUniqueRef();
		void AddSharedRef();
		void RemoveSharedRef();

	private:
		String		m_depotPath;

		Uint32		m_numUniqueRefs; // unique refs - one ofr each source instance
		Uint32		m_numSharedRefs; // shared refs - one for each grid cell with this resource
	};

	/// Resource grid cell
	class CGridCell
	{
	public:
		CGridCell();

		// get the actual grid cell level
		RED_FORCE_INLINE CGridLevel* GetLevel() const { return m_level; }

		// get the cell coordinates in the grid level
		RED_FORCE_INLINE const Uint32 GetGridX() const { return m_cellX; }  
		RED_FORCE_INLINE const Uint32 GetGridY() const { return m_cellY; }  
		
		// get all referenced resources (flat list)
		RED_FORCE_INLINE const TDynArray< CResourceRef* >& GetResources() const { return m_resources; }

		// do we have nothing in this cell?
		RED_FORCE_INLINE const Bool IsEmpty() const { return m_resources.Empty(); }

		// get parent cell
		RED_FORCE_INLINE CGridCell* GetParent() { return m_parent; }

		// get number of children in the cell
		RED_FORCE_INLINE const Uint32 GetNumChildren() const { return ARRAY_COUNT(m_children); }

		// get child cell
		RED_FORCE_INLINE CGridCell* GetChild( const Uint32 childIndex ) { RED_FATAL_ASSERT( childIndex<ARRAY_COUNT(m_children), "Invalid child index" ); return m_children[childIndex]; }

		// setup grid cell
		void Setup( CGridLevel* level, const Uint32 gridX, const Uint32 gridY );

		// link with parent cell
		void Link( CGridCell* parentCell, Uint32 childIndex );

		// add resource reference, returns true if added
		const Bool AddResourceReference( CResourceRef* res ); 

		// remove resource reference, returns true if removed
		const Bool RemoveResourceReference( CResourceRef* res );

		// do we have a reference to this resource ?
		const Bool HasResourceReference( CResourceRef* res ) const;

	private:
		THashSet< CResourceRef* >	m_resourcesSet; // for fast tests
		TDynArray< CResourceRef* >	m_resources; // flat list

		CGridLevel* m_level; // 0 - top level

		Uint32		m_cellX; // x coordinates (max = 1<<m_level - 1)
		Uint32		m_cellY; // y coordinates (max = 1<<m_level - 1)

		CGridCell*	m_parent;		// parent cell (NULL only for the top level)
		CGridCell*	m_children[4];	// children cells (NULL only at the bottom level)
	};

	/// Sector data cell
	class CGridLevel
	{
	public:
		CGridLevel( const Float worldSize, const Uint32 levelIndex );
		~CGridLevel();

		// Get index of this level
		RED_FORCE_INLINE const Uint32 GetLevelIndex() const { return m_levelIndex; }

		// Get dimension of the grid
		RED_FORCE_INLINE const Uint32 GetGridSize() const { return m_cellsPerSize; }

		// Get world space size of single cell in this grid
		RED_FORCE_INLINE const Float GetCellWorldSize() const { return m_gridCellSize; }

		// Get cell at grid xy (returns null for values out of range)
		CGridCell* GetCell( const Uint32 gridX, const Uint32 gridY );

		// Get cell at grid xy (returns null for values out of range)
		const CGridCell* GetCell( const Uint32 gridX, const Uint32 gridY ) const;

		// add reference to resource at given location
		const Bool AddResourceReference( CResourceRef* res, const Vector& pos, const Float radius );

		// extract all non empty grid cells
		void GetFilledCells( TDynArray< const CGridCell* >& outGridCells ) const;

	private:
		Uint32							m_levelIndex;
		Uint32							m_cellsPerSize;

		Float							m_gridBase;
		Float							m_gridScale; // 1/cellsize
		Float							m_gridCellSize;

		TDynArray< CGridCell >	m_cells;
	};

	/// Sector data grid
	class CGrid
	{
	public:
		CGrid( const Float worldSize, const Float minCellLevel );
		~CGrid();

		// get resource descriptor for given path
		CResourceRef* GetResource( const String& depotPath );

		// get root level
		CGridLevel* GetRootLevel() const;

		// get root cell (the top most cell)
		CGridCell* GetRootCell() const;

		// add reference to resource at given location
		const Bool AddResourceReference( CResourceRef* res, const Vector& pos, const Float radius );

		// get world size
		RED_FORCE_INLINE const Float GetWorldSize() const { return m_worldSize; }

		// get all used resources
		RED_FORCE_INLINE const TDynArray< CResourceRef* >& GetAllResources() const { return m_allResources; }

		// compute world space bounding box
		const Box ComputeBox( const Uint32 gridLevel, const Uint32 gridX, const Uint32 gridY ) const;

		// extract all non empty grid cells
		void GetFilledCells( TDynArray< const CGridCell* >& outGridCells ) const;

	private:
		TDynArray< CGridLevel* >				m_levels;

		THashMap< String, CResourceRef* >	m_resourcesMap;
		TDynArray< CResourceRef* >			m_allResources;

		Float										m_worldSize;
		Float										m_gridBase; // -(m_worldSize/2)
		Float										m_minCellSize;
	};

} // ResourceGrid