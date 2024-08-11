/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "terrainStructs.h"
#include "../core/deferredDataBuffer.h"

// Those will be removed from here later, after next iteration over LOD implementation.
#define TERRAIN_PATCH_RES 32
#define TERRAIN_TESS_BLOCK_RES 16


/*
The naming in this class, reflects the naming in the paper called "Real-time rendering of large terrains with support for runtime modifications".
*/

//#define USE_LEVEL_BASED_TILE_SET	// More suitable for cooked game. In editor let's have easy to check out tiles.

#define MAX_CLIPMAP_SIZE						65536	// Max precision of the world heightmap
#define MIN_CLIPMAP_SIZE						1024	// Min precision of the world heightmap
#define MIN_CLIPSIZE							256		// This has to be a GPU-supported texture resolution. It is a resolution of the clipmap window moving through the world
#define MAX_NUM_TILES_PER_EDGE					64
#define MIN_NUM_TILES_PER_EDGE					1
#define TERRAIN_STAMP_VISUALIZATION_TEX_SIZE	1024
#define NUM_TERRAIN_TEXTURES_AVAILABLE			31		// Remember to set the corresponding flag on the renderer side
#define NUM_CLIPMAP_CONFIGS						9

#define MAX_COLORMAP_SIZE				(300.0f * 1024.0f * 1024.0f)

const Uint32 g_clipMapConfig[ NUM_CLIPMAP_CONFIGS ][2] = {
	// { CLIPSIZE, TILERES }
	{ 1024, 1024 },
	{ 1024, 512 },
	{ 1024, 256 },
	{ 1024, 128 },
	{ 1024, 64 },
	{ 512, 512 },
	{ 512, 256 },
	{ 512, 128 },
	{ 512, 64 },
};

#define NUM_CLIPMAP_RESOLUTIONS			7
const Uint32 g_clipMapResolutions[ NUM_CLIPMAP_RESOLUTIONS ] = {
	MAX_CLIPMAP_SIZE,
	32768,
	16384,
	8192,
	4096,
	2048,
	MIN_CLIPMAP_SIZE
};

#ifndef NO_HEIGHTMAP_EDIT
typedef Uint8 ImportStatus;
enum EImportStatusFlags
{
	EIS_NothingImported		= 0,
	EIS_HeightmapImported	= FLAG( 1 ),
	EIS_ColormapImported	= FLAG( 2 ),
};
#endif

class CTerrainTile;
class CVegetationBrush;
class IMaterial;
class IRenderProxy;

// World->Clipmap update structure
struct SClipMapUpdateInfo
{
	SClipMapUpdateInfo()
	:	m_forceSync( false )
	,	m_aggressiveEviction( false )
	{}

	Vector			m_viewerPosition;
	Float			m_timeDelta;
	Bool			m_forceSync;
	Bool			m_aggressiveEviction;
};

// Clipmap setup descriptor
struct SClipmapParameters
{
	Uint32	clipmapSize;
	Uint32	clipSize;
	Uint32	tileRes;
	Uint32	highestColorRes;
	Float	terrainSize;
	Float	lowestElevation;
	Float	highestElevation;

	Bool	useCompressedColorMap;					// Not used in Reinit() or IsCompatible()

	Bool operator==( const SClipmapParameters& rhs )
	{
		return clipmapSize == rhs.clipmapSize &&
			clipSize == rhs.clipSize &&
			tileRes == rhs.tileRes &&
			highestColorRes == rhs.highestColorRes &&
			terrainSize == rhs.terrainSize &&
			lowestElevation == rhs.lowestElevation &&
			highestElevation == rhs.highestElevation &&
			useCompressedColorMap == rhs.useCompressedColorMap;
	}
};

#ifdef USE_LEVEL_BASED_TILE_SET
class CClipMapLevel
{
public:

	enum ETerrainTileState
	{
		TTS_Ready,
		TTS_Loading,
		TTS_LoadingFailed,

		TTS_Invalid
	};

	struct STerrainTileEntry
	{
		CTerrainTile*		m_loadedTile;	//!< Tile already loaded
		CJobLoadResource*	m_loadingjob;	//!< Loading CTerrainTile resource
		ETerrainTileState	m_state;		//!< State of this tile
		Uint32				m_gridRow;		//!< Destination row in the tile grid
		Uint32				m_gridCol;		//!< Destination col in the tile grid
	};

protected:
	Uint32							m_resInTiles;		//!< Number of tiles in one dimension
	TDynArray< STerrainTileEntry >	m_neededTiles;		//!< Tiles that are rendered and border tiles being preloaded

public:
	CClipMapLevel( Uint32 resInTiles );
};
#endif

// Engine side logical setup of the clipmap level
struct SClipMapLevel
{
	Int32		m_resolutionOfTheLevel;
	Int32		m_resolutionOfTheClipRegion;
	Int32		m_resolutionOfTheTile;
	Float		m_areaSize;

	Int32		m_lastClipCenterRow;				//!< Last viewer-visited texel of the whole clipmapped texture
	Int32		m_lastClipCenterCol;				//!< Last viewer-visited texel of the whole clipmapped texture

	Rect		m_lastClipRegion;					//!< Last texels rectangle covered by this level

	//PxRigidStatic*						m_physicsActor;

	//TDynArray< CTerrainTile* >	m_tilesContributing;	//!< Tiles that cover the clipregion for this level

	SClipMapLevel() : m_lastClipCenterRow( 999999 ), m_lastClipCenterCol( 999999 ) {}
};

// Structure describing part of the tile, that is subject to the rectangular area selection
// Basically this is used to iterate clipmap data
struct SClipmapHMQueryResult
{
	CTerrainTile*	m_tile;						//!< Subject tile
	Int32			m_row;						//!< Row in the tile grid
	Int32			m_col;						//!< Col in the tile grid
	Rect			m_selectionSubRect;			//!< Subrectangle of the rectangle specified in the image query
	Rect			m_addressingRect;			//!< Describing the area to be addressed in the buffer
	Rect			m_colorSelectionSubRect;	//!< Query's subrectangle in terrain's color texel space
	Rect			m_colorAddressingRect;		//!< Area to be addressed in the color buffer, in texels (NOT compression blocks)
};

// Cached clipmap data for smallest mips - helps with terrain loading on consoles
class CClipMapCookedData : public ISerializable
{
	DECLARE_RTTI_SIMPLE_CLASS( CClipMapCookedData );

public:
	CClipMapCookedData();
	virtual ~CClipMapCookedData();

	// Store data in the cache
	void StoreData( const Uint32 tileX, const Uint32 tileY, const Uint32 mipIndex, BufferHandle heightData, BufferHandle colorData, BufferHandle controlData );

	// Restore data into the clip map
	void RestoreData( CClipMap* intoClipmap ) const;

	// Get size of data stored
	const Uint32 GetTotalDataSize() const;

	// Get number of buffers stored
	const Uint32 GetTotalBufferCount() const;

	static void Hack_InitializeStorage();
	static void Hack_DestroyStorage();

private:
	// Save/Load
	virtual void OnSerialize( IFile& file );

	// buffer entries
	struct BufferEntry
	{
		Uint32		m_dataOffset;
		Uint32		m_dataSize;
	};

	// entries
	struct TileEntry
	{
		Uint8			m_tileX;
		Uint8			m_tileY;
		Uint16			m_mipIndex;
		BufferEntry		m_heightData;
		BufferEntry		m_colorData;
		BufferEntry		m_controlData;
	};

	typedef TDynArray< Uint8, MC_TerrainClipmap > TData;
	typedef TDynArray< TileEntry, MC_TerrainClipmap > TTiles;

	struct Storage
	{
		TData data;
		TTiles tiles;
	};
	
	static Storage m_storage;

	// Store Buffer data
	void StoreBuffer( BufferEntry& outEntry, BufferHandle data );

	// Get data buffer handle - no release on the memory
	BufferHandle RestoreBuffer( const BufferEntry& bufferEntry ) const;
};

BEGIN_CLASS_RTTI( CClipMapCookedData );
	PARENT_CLASS( ISerializable );
END_CLASS_RTTI();

// Main clipmap object, governing a set of levels and tiles
class CClipMap : public CObject
{
	DECLARE_ENGINE_CLASS( CClipMap, CObject, 0 );

private:
	struct TileCoords;

	typedef TDynArray< THandle< CTerrainTile >, MC_TerrainClipmap >			TTerrainTileHandles;
	typedef TDynArray< CTerrainTile*, MC_TerrainClipmap >					TTerrainTiles;
	typedef TDynArray< TileCoords, MC_TerrainClipmap >						TCoordArray;
	typedef TDataBufferAllocator< MC_TerrainClipmap, MemoryPool_Default >	CookedMipAllocator;

	// Adding MC_TerrainClipmap to these would involve editing quite a few renderer interfaces :(
	typedef TDynArray< SClipmapLevelUpdate* >								TClipmapLevelUpdateArray;
	typedef TDynArray< Vector2 >											TVector2Array;
	typedef TDynArray< Float >												TFloatArray;

public:
	typedef TDynArray< STerrainMipmapEvictionTracker, MC_TerrainClipmap >	TEvictionTrackerArray;

protected:
	Uint32	m_clipSize;					//!< X/Y dimension of each clip map texture
	Uint32	m_clipmapSize;				//!< Width and height of the finest clipmap level, reflecting precision of the world in texels
	Uint32	m_numClipmapStackLevels;	//!< Number of levels in the clipmap stack, meaning number of levels in the clipmap, excluding the Mipmap stack levels (the layer that covers the hole terrain with m_clipSize-esolution texture)
	Uint32	m_tileRes;					//!< Resolution of a single tile on the 0 clipmap level
	Int32	m_colormapStartingMip;
	Float	m_terrainSize;				//!< Size in meters of the terrain represented by this clipmap
	Float	m_lowestElevation;			//!< The world space z value represented by 0 in the heightmap
	Float	m_highestElevation;			//!< The world space z value represented by 65535 in the heightmap

#ifndef NO_HEIGHTMAP_EDIT
	SClipmapStampDataUpdate*	m_stampDataUpdate;
	Bool						m_stampGizmoVisible;
#endif

	Vector							m_terrainCorner;				//!< Offset to the corner of the terrain ( we want the terrain to span from -X to X for the float precision purposes )
	TDynArray< SClipMapLevel >		m_levels;						//!< Infos about each level
	TTerrainTileHandles				m_terrainGrid;					//!< 2d grid of mipmapped terrain tiles

	TTerrainTiles					m_tileCache;					//!< Cache of pointers from m_terrainGrid (Local variable made member to minimise fragmentation)
	TCoordArray						m_TouchAndEvictCoordBuffer;		//!< Used internally during TouchAndEvict() - Allocated once to minimise fragmentation 
	TClipmapLevelUpdateArray		m_clipmapUpdates;

	TVector2Array					m_tileHeightRanges;				//!< Min and max terrain height for each tile.
	TFloatArray						m_minWaterHeight;				//!< Minimum water level for each tile, for changing visibility by depth under water.
	TEvictionTrackerArray			m_mipsToEvict;
	Red::System::MemSize			m_residentMipTileFootprint;

	Uint32							m_numTilesPerEdge;				//!< Number of tiles in one dimension

	THandle< IMaterial >			m_material;						//!< Material graph for this terrain
	IRenderProxy*					m_renderProxy;					//!< Render proxy for the whole terrain

	TDynArray< STerrainTextureParameters >		m_textureParams;	//!< Texture parameters ( like slope-based blend for horizontal textures )
	TDynArray< THandle< CVegetationBrush > >	m_grassBrushes;		//!< Per texture automatic grass brush

	DataBuffer						m_cookedMipStackHeight;
	DataBuffer						m_cookedMipStackControl;
	DataBuffer						m_cookedMipStackColor;

	BufferHandle					m_cookedMipStackHeightHandle;
	BufferHandle					m_cookedMipStackControlHandle;
	BufferHandle					m_cookedMipStackColorHandle;

	Vector							m_previousViewerPosition;		//!< Viewer position from last update, for predictive async loading.
	Int32							m_nextClipmapLevelToUpdate;		//!< We update one level per frame, to reduce the per-frame memory footprint and also make the update itself a bit cheaper

	Bool							m_enableLoading;				//!< Allow clipmap to be loaded/unloaded

	CClipMapCookedData*				m_cookedData;					//!< Cooked clipmap data (small mips)

#ifndef NO_EDITOR
	Bool							m_isEditing;					//!< Is currently being edited? Delay foliage collision until it's done.
#endif

public:
	CClipMap();
	~CClipMap();

	//////////////////////////////////////////////////////////////////////////
	// CObject overrides
	//////////////////////////////////////////////////////////////////////////

	// Called after object is loaded
	virtual void OnPostLoad() override;

	// Object serialization interface
	virtual void OnSerialize( IFile& file ) override;


#ifndef NO_EDITOR/*NO_RESOURCE_COOKING*/
	virtual void OnCook( class ICookerFramework& cooker ) override;
private:
	void Cook_BuildMipStackBuffer();
	void Cook_ClipmapCache();
#endif

public:

	//////////////////////////////////////////////////////////////////////////
	// Initialize/Import/export
	//////////////////////////////////////////////////////////////////////////

	//! Removes terrain from view - calling Update will recreate proxies 
	void ClearTerrainProxy();

	void InvalidateCollision() const;

#ifdef USE_ANSEL
	Bool EnsureCollisionIsGenerated( const Vector& position );
#endif // USE_ANSEL

	//! Toggle clipmap loading
	void ToggleLoading( const Bool allowLoading );

#ifndef NO_EDITOR
	void SetIsEditing( Bool isEditing ) { m_isEditing = isEditing; }
	Bool IsEditing() const { return m_isEditing; }
#endif

#ifndef NO_HEIGHTMAP_EDIT
	//! Import the whole terrain from the set of tiles
	void ImportTerrain( const TDynArray< String >& tilePaths );

	//! Import specific tile from path
	ImportStatus ImportTile( Uint32 x, Uint32 y, const Bool importHM, const String& hmPath, const Bool importCM, const String& cmPath );

	//! Export specific tile to the path
	Bool OnExportTile( Uint32 x, Uint32 y, Uint32 minX, Uint32 minY, const String& directory, const String& filename, const Bool exportHM, String& hmFileName, const Bool exportCM, String& cmFileName );

	//! Just for testing
	void Test_ImportTerrain();

	//! Destroy old terrain and create
	Bool Reinit( const SClipmapParameters& initInfo );

	//////////////////////////////////////////////////////////////////////////
	// Helpers
	//////////////////////////////////////////////////////////////////////////

	//! Is given parameters set compatible with current tile data
	Bool IsCompatible( const SClipmapParameters& initInfo );

	//! Parse world machine tile filename
	static Bool GetCoordinatesFromWorldMachineFilename( const String& filename, Uint32& x, Uint32& y );

	//void SampleData( const Vector& area, Uint16*& outData ) const;
	//void PasteData( const Rect& targetRect, const Uint16* data );

	//! Set/Get material
	void SetMaterial( IMaterial* material );
	RED_INLINE IMaterial* GetMaterial() const { return m_material.Get(); }

	//! Save all tiles
	void SaveAllTiles();

	void SetStampData( const String& m_heightmapToLoad );		
	void SetStampData( const Uint16* data, Uint32 dataWidth, Uint32 dataHeight );
	void SetStampColorData( const TColorMapType* data, Uint32 dataWidth, Uint32 dataHeight );
	void SetStampControlData( const TControlMapType* data, Uint32 dataWidth, Uint32 dataHeight );
	void SetStampCenter( const Vector2& center );
	void SetStampSize( Float size );
	void SetStampIntensity( Float stampIntensity );
	void SetStampOffset( Float stampOffset );
	void SetStampRotation( Float radians );
	void SetStampModeAdditive( Bool additive );
	void ClearStampData();
	void SetStampGizmoVisible( Bool visible ) { m_stampGizmoVisible = visible; }
	Bool GetStampGizmoVisible() { return m_stampGizmoVisible; }
	
	//! Offset tiles
	void OffsetTiles( Int32 rowOffset, Int32 colOffset );
	void SetTextureParam( Int32 textureIndex, Int32 paramIndex, Float value );
	Float GetTextureParam( Int32 textureIndex, Int32 paramIndex );

	const TDynArray< STerrainTextureParameters >& GetTextureParams() { return m_textureParams; }

	//! Attach a grass brush to given texture
	void SetGrassBrush( Int32 textureIndex, CVegetationBrush* brush );
#endif

	RED_INLINE IRenderProxy* GetTerrainProxy() const { return m_renderProxy; }

	//! Get grass brush attached to given texture
	CVegetationBrush* GetGrassBrush( Int32 textureIndex ) const;

	void GetTiles( const Box& extents, TTerrainTiles& tiles ) const;

	//! Get a set of data blocks, composing the area described with the input rect
	void GetRectangleOfData( const Rect& rect, TDynArray< SClipmapHMQueryResult >& parts, Rect* outColorRect = NULL );

	//! Get normalized position in texel space
	RED_INLINE Vector2 GetTexelSpaceNormalizedPosition( const Vector& position, Bool clamp = true ) const
	{
		// Compute position in terrain space of the full heightmap
		// Terrain space means from 0,0 to terrainSize,terrainSize
		Vector2 viewPosTerrainSpace( position.X - m_terrainCorner.X, position.Y - m_terrainCorner.Y );

		if ( clamp )
		{
			viewPosTerrainSpace.X = Clamp< Float >( viewPosTerrainSpace.X, 0.0f, m_terrainSize );
			viewPosTerrainSpace.Y = Clamp< Float >( viewPosTerrainSpace.Y, 0.0f, m_terrainSize );
		}

		// Normalize the terrain space position
		return Vector2( viewPosTerrainSpace.X / m_terrainSize, viewPosTerrainSpace.Y / m_terrainSize );
	}

	CTerrainTile* GetTile( Uint32 x, Uint32 y ) const;

	//! Get box
	Box GetBoxForTile( Int32 x, Int32 y, Float z ) const;

	//! Get maximum height for a tile.
	Float GetTileMaximumHeight( CTerrainTile* tile ) const;
	//! Get minimum height for a tile.
	Float GetTileMinimumHeight( CTerrainTile* tile ) const;


	//! Determine number of levels for given clipmap specs
	static Uint32 ComputeNumberOfClipmapStackLevels( Uint32 clipmapSize, Uint32 clipSize );

	Uint32 GetNumClipmapStackLevels() const { return m_numClipmapStackLevels; }

	//! Reupload generic grass rendering parameters to the renderer
	void UpdateGrassRendering();

	//////////////////////////////////////////////////////////////////////////
	// Getters
	//////////////////////////////////////////////////////////////////////////

	RED_INLINE void GetClipmapParameters( SClipmapParameters* parameters ) const
	{
		if ( parameters )
		{
			parameters->clipmapSize				= m_clipmapSize;
			parameters->clipSize				= m_clipSize;
			parameters->tileRes					= m_tileRes;
			parameters->highestColorRes			= m_tileRes >> m_colormapStartingMip;
			parameters->terrainSize				= m_terrainSize;
			parameters->lowestElevation			= m_lowestElevation;
			parameters->highestElevation		= m_highestElevation;
			parameters->useCompressedColorMap	= IsCooked();
		}
	}

	RED_INLINE Uint32 GetNumTilesPerEdge() const { return m_numTilesPerEdge; }
	RED_INLINE Float GetTileSize() const { return m_terrainSize / m_numTilesPerEdge; }
	RED_INLINE Float GetTerrainSize() const { return m_terrainSize; }
	RED_INLINE Uint32 GetTilesMaxResolution() const { return m_tileRes; }
	RED_INLINE const Vector& GetTerrainCorner() const { return m_terrainCorner; }
	RED_INLINE Float GetHeighestElevation() const { return m_highestElevation; }
	RED_INLINE Float GetLowestElevation() const { return m_lowestElevation; }
	RED_INLINE Uint32 GetClipmapSize() const { return m_clipmapSize; }
	
	RED_INLINE Int32 GetColormapStartingMip() const { return m_colormapStartingMip == -1 ? CalculateColormapStartingMip( m_numClipmapStackLevels, m_numTilesPerEdge, m_tileRes ) : m_colormapStartingMip; }

	// Get terrain data from the best loaded mip level.
	Bool GetHeightForWorldPosition( const Vector& position, Float& height, Uint16* texelHeight = nullptr ) const;
	TControlMapType GetControlmapValueForWorldPosition( const Vector& position ) const;
	TColorMapType GetColormapValueForWorldPosition( const Vector& position ) const;

	// Get terrain data from a specific mip level, loading synchronously if the data is not there.
	void GetHeightForWorldPositionSync( const Vector& position, Uint32 level, Float& height, Uint16* texelHeight = nullptr ) const;
	TControlMapType GetControlmapValueForWorldPositionSync( const Vector& position, Uint32 level ) const;
	TColorMapType GetColormapValueForWorldPositionSync( const Vector& position, Uint32 level ) const;

#ifndef NO_HEIGHTMAP_EDIT
	Vector GetNormalForWorldPosition( const Vector& position ) const;

	// Get height for the best loaded mip level.
	void GetHeightForTexelPosition( const Vector2& position, Float& height, Uint16* texelHeight = nullptr ) const;
#endif

	RED_INLINE CTerrainTile* GetTileFromPosition( const Vector& position, Int32& tileX, Int32& tileY, Bool clamp = true ) const
	{
		Vector2 viewPosTerrainSpaceNorm = GetTexelSpaceNormalizedPosition( position, clamp );

		// We will need clip region for each level, mapped to the level0 dimensions, for determining the contributing set of tiles
		Int32 clipCenterLevel0SpaceCol = (Int32)( viewPosTerrainSpaceNorm.X * ( m_clipmapSize - 1 ) );
		Int32 clipCenterLevel0SpaceRow = (Int32)( viewPosTerrainSpaceNorm.Y * ( m_clipmapSize - 1 ) );

		tileX = clipCenterLevel0SpaceCol >= 0 ? clipCenterLevel0SpaceCol / m_tileRes : -1;
		tileY = clipCenterLevel0SpaceRow >= 0 ? clipCenterLevel0SpaceRow / m_tileRes : -1;

		if ( tileX < 0 || tileX >= (Int32)m_numTilesPerEdge || tileY < 0 || tileY >= (Int32)m_numTilesPerEdge )
		{
			return NULL;
		}

		return m_terrainGrid[ tileY * m_numTilesPerEdge + tileX ].Get();
	}

	//! Get the region of tiles that are touched by a given world-space rectangle. The output may have values going outside
	//! the terrain's tile range. Returns true if the world-space rectangle touches the terrain, false if it's entirely off.
	Bool GetTilesInWorldArea( const Vector2& vmin, const Vector2& vmax, Rect& out ) const;
	Float TexelHeightToHeight( Uint16 h ) const;
	Uint16 HeightToTexelHeight( Float h ) const;

	//! Get the ray intersection point. Tests against the currently loaded data.
	Bool Intersect( const Vector& rayOrigin, const Vector& rayDirection, Vector& intersection ) const;


	//! Return the clip window's area in level 0's texel space. Optionally return the area and center position in the level's space.
	Rect GetClipWindowRect( const Vector& centerPosition, Uint32 whichLevel, Rect* outLevelSpace = nullptr, Int32* outCenterCol = nullptr, Int32* outCenterRow = nullptr ) const;


	//! Check if any terrain tiles are currently loading.
	Bool IsLoadingTiles() const;

	//////////////////////////////////////////////////////////////////////////
	// Frame-by-frame
	//////////////////////////////////////////////////////////////////////////
	//! Update data streaming
	void Update( const SClipMapUpdateInfo& updateInfo );

private:
#ifndef NO_HEIGHTMAP_EDIT

	//			<0>		<1>
	//
	//	<2>		<3>		<4>		<5>
	//			    <wp>
	//	<6>		<7>		<8>		<9>
	//
	//			<10>	<11>
	// world position is affected by 4 texels, we compute another 8 to be able to calculate
	// properly weighted normals in 4 surrounding texels and in the end - normal in position
	TDynArray< Float > ComputeHeightsAroundWorldPosition( const Vector& position ) const;

public:
	void UnloadAll();
#endif

	//////////////////////////////////////////////////////////////////////////
	// Clipmap mips eviction
	//////////////////////////////////////////////////////////////////////////
public:
	// Allows us to unload all tiles no longer required before we perform synchronous loading of the new tiles
	void TouchAndEvict( const SClipMapUpdateInfo& updateInfo );

private:
	void SortMipsForEviction( TDynArray< STerrainMipmapEvictionTracker, MC_TerrainClipmap >& evictionQueue );
	void EvictOldMips( const SClipMapUpdateInfo& updateInfo, TDynArray< STerrainMipmapEvictionTracker, MC_TerrainClipmap >& evictionQueue, Red::System::MemSize footprint, Red::System::MemSize maxFootprint );

	// Helpers
	void PreEviction();
	void EvictionTileCheck( const SClipMapUpdateInfo& updateInfo, CTerrainTile* tile );
	void PostEviction( const SClipMapUpdateInfo& updateInfo );

	// Process predictive async loading of tile data.
	void ProcessAsyncLoading( Uint32 clipLevel, const Rect& level0ClipRegion, Int32 clipCenterCol, Int32 clipCenterRow, const Vector& viewerPosition );

	// Can we sample data from the given tile? If false, we can sample the cooked mip stack.
	Bool ShouldSampleFromTile( CTerrainTile* tile, ETerrainBufferType bufferType ) const;

	//////////////////////////////////////////////////////////////////////////
	// Dataset creation/deletion helpers
	//////////////////////////////////////////////////////////////////////////

#ifndef NO_HEIGHTMAP_EDIT
	//! Create tile files
	void BuildTerrainTiles( const SClipmapParameters& initInfo );

	//! Destroy terrain tiles, removing tile files from disc. If at least on tile can't be removed, return false
	Bool DestroyTerrainTiles();

	//! Checkout the whole terrain data (all tiles). Return false if at least one tiles can't be accessed for write.
	Bool PerformFullGridCheckout();
#endif

	//////////////////////////////////////////////////////////////////////////
	// Collision
	//////////////////////////////////////////////////////////////////////////

	void GenerateCollisionForTile( Uint32 row, Uint32 col );

	//////////////////////////////////////////////////////////////////////////
	// Other helpers
	//////////////////////////////////////////////////////////////////////////

	//! Get parent world
	CWorld* GetWorld()
	{
		CWorld* world = FindParent< CWorld >();
		ASSERT( world );
		return world;
	}

	//! Generate logical clipmap level structures
	void GenerateLevels();

	CTerrainTile* WorldPositionToTile( const Vector& worldPosition, Vector2* tileLocalPosition ) const;
	Bool ValidateLevelUpdate( SClipmapLevelUpdate* update );

	//////////////////////////////////////////////////////////////////////////
	// Internal callbacks
	//////////////////////////////////////////////////////////////////////////

	//! Data was modified
	void OnDataChanged();
	const Uint32 CalculateColormapStartingMip( const Uint32 numClipmapStackLevels, const Uint32 numTilesPerEdge, const Uint32 tileRes ) const;

	void UpdateEachLevel( const SClipMapUpdateInfo &updateInfo, Bool firstframe, Box &terrainCollisionBounding, TDynArray< SClipmapLevelUpdate* > &levelUpdates );
	void UpdateMipmapStack( Bool firstframe, Box &terrainCollisionBounding, const SClipMapUpdateInfo &updateInfo, TDynArray< SClipmapLevelUpdate* > &levelUpdates );

	void SwitchDataBuffersToBufferHandles();


	//////////////////////////////////////////////////////////////////////////
	// Culling tiles by water depth
	//////////////////////////////////////////////////////////////////////////
public:
	void UpdateWaterLevels( Float baseLevel, const TDynArray< Box >& localWaterBounds );

	//////////////////////////////////////////////////////////////////////////
	// Update Each Level internals
	//////////////////////////////////////////////////////////////////////////
private:
	struct TileCoords
	{
		struct Details
		{
			const Uint32 m_first;
			const Uint32 m_last;
			Uint32 m_current;

			Details( Uint32 first, Uint32 last )
			:	m_first( first )
			,	m_last( last )
			,	m_current( first )
			{
			}

			Details( const Details& other )
			:	m_first( other.m_first)
			,	m_last( other.m_last )
			,	m_current( other.m_current )
			{
			}

			RED_INLINE void Reset() { m_current = m_first; }
			RED_INLINE Uint32 operator++() { ++m_current; return m_current; }
			RED_INLINE Bool IsValid() const { return m_current <= m_last; }
		};

		Details m_column;
		Details m_row;

		RED_INLINE TileCoords( Rect region, Int32 tileRes )
		:	m_column( region.m_left / tileRes, region.m_right / tileRes )
		,	m_row( region.m_top / tileRes, region.m_bottom / tileRes )
		{
		}

		RED_INLINE TileCoords( const TileCoords& other )
		:	m_column( other.m_column )
		,	m_row( other.m_row )
		{

		}

		RED_INLINE void ResetCurrent()
		{
			m_column.m_current = m_column.m_first;
			m_row.m_current = m_row.m_first;
		}
	};

	void UELPrefetchTiles( TileCoords& coords, TTerrainTiles& tiles ) const;
	Bool UELNeedsUpdating( Int32 biggestOffset, Float updateCriterion, TileCoords& coords, const TTerrainTiles& tiles, Uint32 l ) const;
	void UELGenerateCollisionForTile( Uint32 l, TileCoords& coords, Box& terrainCollisionBounding, CTerrainTile* tile );
	void UELTrimBounds( Vector& boxMin, Vector& boxMax, Rect& validTexels, const TileCoords& coords, SClipMapLevel& level, Float unitsPerTexel, SClipRegionRectangleData& commonRegionRectData );
	void UELAppendUpdateRects( SClipmapLevelUpdate* levelupdateInfo, CTerrainTile* tile, Uint32 l, Bool updateLevelRenderResource, Bool updateColorMap, const SClipMapLevel& level, const SClipRegionRectangleData& commonRegionRectData );


	// Returns if tile is currently loaded
	//Bool TickHandleTileLoad( CTerrainTile* tile, Bool synchronous,  );
};

BEGIN_CLASS_RTTI( CClipMap );
PARENT_CLASS( CObject );
	PROPERTY( m_clipSize );
	PROPERTY( m_clipmapSize );
	PROPERTY( m_numClipmapStackLevels );
	PROPERTY( m_tileRes );
	PROPERTY( m_colormapStartingMip );
	PROPERTY( m_terrainSize );
	PROPERTY( m_terrainCorner );
	PROPERTY( m_numTilesPerEdge );
	PROPERTY( m_lowestElevation );
	PROPERTY( m_highestElevation );
	PROPERTY( m_material );
	PROPERTY( m_textureParams );
	PROPERTY( m_grassBrushes );
	// These are serialized, mainly for cooking. When not cooked, we'll be calculating them at runtime.
	PROPERTY( m_tileHeightRanges );
	PROPERTY( m_minWaterHeight );

	PROPERTY( m_cookedMipStackHeight );
	PROPERTY( m_cookedMipStackControl );
	PROPERTY( m_cookedMipStackColor );
	PROPERTY( m_cookedData );
END_CLASS_RTTI();
