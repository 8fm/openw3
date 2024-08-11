/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/deferredDataBuffer.h"
#include "terrainTypes.h"
#include "terrainStructs.h"
#include "../core/resource.h"
#include "../engine/renderSettings.h"
#include "collisionContent.h"

#define LOWEST_TILE_MIPMAP_RESOLUTION	8			// POWER OF TWO ONLY!!

#ifndef RED_FINAL_BUILD
#	define TERRAIN_TILE_DEBUG_RENDERING 1
#endif

class CClipMap;

//TODO: clean this up
namespace physx
{
	class PxRigidStatic;
	class PxHeightField;
	struct PxHeightFieldSample;
};
using namespace physx;

//////////////////////////////////////////////////////////////////////////
// Terrain tile update parameters, so while updating it is aware of the whole clipmap setup

struct SClipmapParameters;

struct STerrainTileUpdateParameters
{
	SClipmapParameters*	clipmapParameters;
	Uint32				numTilesPerEdge;
	Uint32				numClipmapLevels;
	Uint32				colormapStartingMip;

	STerrainTileUpdateParameters()
		: clipmapParameters( NULL )
		, numTilesPerEdge( 0 )
		, numClipmapLevels( 0 )
		, colormapStartingMip( 0 )
	{}

	~STerrainTileUpdateParameters();
};

class CPhysicsWorld;
class CWorld;
struct STerrainTextureParameters;


struct STerrainTileMipMap
{
	Uint32				m_resolution;

private:
	// Data buffers containing height, control, color maps.
	DeferredDataBuffer	m_heightMapDataBuffer;
	DeferredDataBuffer	m_controlMapDataBuffer;
	DeferredDataBuffer	m_colorMapDataBuffer;

	// Handles to loaded buffer data.
	BufferHandle		m_heightMapHandle;
	BufferHandle		m_controlMapHandle;
	BufferHandle		m_colorMapHandle;

	// Loading tokens for buffer data, for asynchronous loading.
	BufferAsyncDataHandle	m_heightMapToken;
	BufferAsyncDataHandle	m_controlMapToken;
	BufferAsyncDataHandle	m_colorMapToken;

	Bool				m_wasLoaded;
	Bool				m_wasBoundedWithCookedData;
	mutable Bool		m_wasRequested;


	static Red::Threads::CAtomic< Int32 > m_hackyTerrainVersionCounter;

public:
	// Call before unloading clipmap. Will increment version counter, and sleep a moment to "ensure" any
	// load callbacks have had a chance to do their thing before stuff is unloaded.
	static void HACK_BumpTerrainVersionCounter();
	static Int32 HACK_GetTerrainVersionCounter() { return m_hackyTerrainVersionCounter.GetValue(); }

	STerrainTileMipMap()
		: m_wasLoaded( false )
		, m_wasRequested( false )
		, m_wasBoundedWithCookedData( false )
	{
		m_heightMapDataBuffer.SetMemoryClass( MC_TerrainClipmap );
		m_controlMapDataBuffer.SetMemoryClass( MC_TerrainClipmap );
		m_colorMapDataBuffer.SetMemoryClass( MC_TerrainClipmap );
	}

	void ReallocateBuffers( Uint32 buffers, Uint32 resolution, Bool clearOthers = true );

	void SerializeBuffers( IFile& file, Uint32 tileFileVersion );

	BufferHandle AcquireBufferForWriting( ETerrainBufferType buffer );
	BufferHandle AcquireBufferHandleSync( ETerrainBufferType buffer );
	BufferHandle GetLoadedBufferHandle( ETerrainBufferType buffer );

	// Get the buffer data for single use (sync-loads if needed, does not keep internal reference).
	BufferHandle AcquireBufferHandleSyncNoTrack( ETerrainBufferType buffer ) const;

	Uint32 GetBufferDataSize( ETerrainBufferType buffer ) const;
	Bool HasBufferDataLoaded( ETerrainBufferType buffer ) const;

	// Get any loaded data, but don't load if it isn't already there.
	Uint16* GetHeightMapData() { return m_heightMapHandle ? (Uint16*)m_heightMapHandle->GetData() : nullptr; }
	TControlMapType* GetControlMapData() { return m_controlMapHandle ? (TControlMapType*)m_controlMapHandle->GetData() : nullptr; }
	void* GetColorMapData() { return m_colorMapHandle ? m_colorMapHandle->GetData() : nullptr; }

	// This function is used to pass in the cooked data for this terrain tile mip, do not use it outside cooking
	void BindCookedData( BufferHandle height, BufferHandle control, BufferHandle color );
	void ExtractCookedData( const Bool isCooked, BufferHandle& outCookedHeight, BufferHandle& outCookedColor, BufferHandle& outCookedControl ) const;

	void RequestAsyncLoad();

	Bool IsLoading() const;
	// Is the full data loaded?
	Bool IsLoaded() const;
	// Is some of the data (at least one buffer) loaded?
	Bool IsPartiallyLoaded() const;
	Bool LoadSync();
	void Unload();

	Bool WasLoaded() const { return m_wasLoaded; }
	void UpdateWasLoaded();

	// Was an async request made since the last time this function was called?
	Bool DidRequestAsyncSinceLastTime() const { Bool res = m_wasRequested; m_wasRequested = false; return res; }

	Red::System::MemSize GetResidentMemory() const;


	// This function exists just for upgrading terrain data in CTerrainTile::UpdateTileStructure(), and for cooking. DO NOT USE IT OTHERWISE
	void SetColorMapData( const void* data, Uint32 size );


	Float GetTimeout() const { return IsLoaded() ? GetResidentTimeout() : GetLoadingTimeout(); }

	static Float GetResidentTimeout() { return Config::cvTerrainTileTimeout.Get(); }
	static Float GetLoadingTimeout() { return Config::cvTerrainTileLoadingTimeout.Get(); }
	static Float GetMinimumTimeout() { return Config::cvTerrainTileMinTimeout.Get(); }

};


enum ETerrainTileCollision
{
	TTC_AutoOn,
	TTC_AutoOff,
	TTC_ForceOn,
	TTC_ForceOff,
};
BEGIN_ENUM_RTTI( ETerrainTileCollision );
ENUM_OPTION( TTC_AutoOn );
ENUM_OPTION( TTC_AutoOff );
ENUM_OPTION( TTC_ForceOn );
ENUM_OPTION( TTC_ForceOff );
END_ENUM_RTTI();



//////////////////////////////////////////////////////////////////////////
// Terrain tile as a resource
class CTerrainTile : public CResource, public ICollisionContent
{
	DECLARE_ENGINE_RESOURCE_CLASS( CTerrainTile, CResource, "w2tile", "Terrain Tile" );

public:
	
	Uint32								m_tileFileVersion;

#ifdef TERRAIN_TILE_DEBUG_RENDERING
	Bool								m_performDebugRendering;
	Bool								m_debugForceRenderingUpdate;
	TDynArray< TControlMapType >		m_debugRenderingColorMap;
	Uint32								m_debugRenderingVersion;
#endif
protected:
	

	TDynArray< STerrainTileMipMap, MC_TerrainTileMips >	m_mipmaps;
	TDynArray< Float, MC_TerrainTileMips >				m_mipmapTimes;

	Int32								m_touchedMipLevel;
	Int32								m_highestMipWithData;

	Uint32								m_resolution;
	Uint32								m_colormapStartingMip;

	class CPhysicsTileWrapper*			m_wrapper;

	Bool								m_dirty;

#ifdef USE_PHYSX
	class CTerrainTileCollisionJob*		m_collisionGenJob;
#endif

	Bool								m_collisionGenRequested;

	ETerrainTileCollision				m_collisionType;
	Uint16								m_minHeightValue;
	Uint16								m_maxHeightValue;

	static const Float					TOUCH_MIP_TIMER_RESET;

public:
	CTerrainTile();
	~CTerrainTile();

	//! Serialize mipmaps
	virtual void OnSerialize( IFile& file ) override;

	static void GenerateCollisionHeightMapPass(const Uint32 stridePhysData, const Bool sameRowCol, const Uint16* tileHMnextRowColTile, const TControlMapType* tileCMnextRowColTile, const Bool sameRow, const Uint16* tileHMnextRowTile, const TControlMapType* tileCMnextRowTile, const Bool sameCol, const Uint16* tileHMnextColTile, const TControlMapType* tileCMnextColTile, const Uint16* tileHMTile, const TControlMapType* tileCMTile, TDynArray< PxHeightFieldSample >& physData, Uint32 resolution);

	static const SPhysicalMaterial* GenerateCollisionsMaterialPass(const Uint32 stridePhysData, const Bool sameRowCol, const TControlMapType* tileCMnextRowColTile, const Bool sameRow, const TControlMapType* tileCMnextRowTile, const Bool sameCol, const TControlMapType* tileCMnextColTile, const TControlMapType* tileCMTile, TDynArray< PxHeightFieldSample >& physData, const Int32 count, PxHeightField* heightfield,const TDynArray< char > &physicalMaterials, const TDynArray< STerrainTextureParameters >& textureParams, Uint32 resolution);

	//! Cooking
#ifndef NO_RESOURCE_COOKING
	// Cook this tile
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif

	virtual void OnPostLoad() override;

	Red::System::MemSize GetMipmapResidentMemory() const;

	//! Hacky, but only used for debug window.
	Bool DidRequestAsyncSinceLastTime( Uint32 mipmapLevel ) const;

	//! Request collision generation, but don't do it now.
	void RequestCollisionGen() { m_collisionGenRequested = true; }

	RED_INLINE Bool IsCollisionEnabled() const { return m_collisionType == TTC_AutoOn || m_collisionType == TTC_ForceOn; }
	RED_INLINE ETerrainTileCollision GetCollisionType() const { return m_collisionType; }

	RED_INLINE const Uint32 GetNumMipLevels() const { return m_mipmaps.Size(); }

#ifndef NO_EDITOR
	void SetCollisionType( ETerrainTileCollision type );
#endif


#ifndef NO_HEIGHTMAP_EDIT

	//! Set resolution of this tile
	void SetRes( Uint32 res, Uint32 minTileMipMapRes, Uint32 colorMapStartingMip );

	//! Replace tile area with new data; numTexels should comply with the target rect dimensions.
	void SetData( const Rect& targetRect, const Rect& sourceRect, const Uint16* sourceTexels, const TControlMapType* sourceCMTexels, Uint32 sourcePitch );

	//! Set a region of color data. The rectangles are in texels, sourcePitch is in bytes.
	void SetColorMapData( const Rect& targetRect, const Rect& sourceRect, const TColorMapType* sourceColorData, Uint32 sourcePitch, Uint32 startingMip = 0 );

	void RebuildMipmaps();
	Bool LoadAllMipmapsSync();

	Bool ImportData( const String& absolutePath );
	Bool ExportData( const String& absolutePath );

	Bool ImportColorMap( const String& absolutePath );
	Bool ExportColorMap( const String& absolutePath );

	void UpdateTileStructure( CClipMap* clipmap );

	void SaveFirstTime( CWorld* world, Uint32 row, Uint32 col, const String& directoryName );

	void UnloadMipmaps();
	
#endif

	//! Reset TTL on the mip level, to prevent that level from being unloaded. If the mip is not counting, does nothing.
	void					TouchMip( Uint32 mipmapLevel );
	Bool 					IsMipCounting( Uint32 mipmapLevel ) const { return m_mipmapTimes[mipmapLevel] >= 0.0f; }

	//! Load a mip level synchronously.
	Bool					LoadSync( Uint32 mipmapLevel );

	void					StartAsyncLoad( Uint32 mipmapLevel );
	Bool					IsLoaded( Uint32 mipmapLevel ) const;
	Bool					IsPartiallyLoaded( Uint32 mipmapLevel ) const;
	Bool					IsLoadingAsync( Uint32 mipmapLevel ) const;
	Bool					IsLoadingAnyAsync() const;

	//! Kind of hacky, we need to track current and previous Loaded state, so we can update clipmap when a new tile is loaded.
	//! Basically, WasLoaded says whether the mip was previously in a fully loaded state, and UpdateWasLoaded should be called
	//! each frame to update that.
	Bool					WasLoaded( Uint32 mipmapLevel ) const;
	void					UpdateWasLoaded( Uint32 mipmapLevel );

	//! Do we have any data already loaded for the given buffer type?
	Bool					HasDataLoaded( ETerrainBufferType buffer ) const;

	//! This function is used to pass in the cooked data for this terrain tile mip, do not use it outside cooking
	void					BindCookedData( const Uint32 mipmapLevel, BufferHandle height, BufferHandle contorl, BufferHandle color );

	//! Get data for the highest loaded mip level.
	const Uint16*			GetAnyLevelHM( Uint32& level );
	const TControlMapType*	GetAnyLevelCM( Uint32& level );
	const void*				GetAnyLevelColorMap( Uint32& level );

	//! Get data for the given mip level, loading as needed.
	BufferHandle			GetBufferHandleSync( Uint32 mipmapLevel, ETerrainBufferType type );
	BufferHandle			GetLevelSyncHMBufferHandle( Uint32 mipmapLevel );
	BufferHandle			GetLevelSyncCMBufferHandle( Uint32 mipmapLevel );
	BufferHandle			GetLevelSyncColorMapBufferHandle( Uint32 mipmapLevel );

	const Uint16*			GetLevelSyncHM( Uint32 mipmapLevel );
	const TControlMapType*	GetLevelSyncCM( Uint32 mipmapLevel );
	const void*				GetLevelSyncColorMap( Uint32 mipmapLevel );

	//! Get data suitable for writing, loading as needed.
	Uint16*					GetLevelWriteSyncHM( Uint32 mipmapLevel );
	TControlMapType*		GetLevelWriteSyncCM( Uint32 mipmapLevel );
	void*					GetLevelWriteSyncColorMap( Uint32 mipmapLevel );

	Uint32					GetHighestColorMapResolution();
	Uint32					GetColorMapResolutionForMipMapLevel( Uint32 mipMapLevel );

	//! Causes synchronous loading
	const void*				GetHighestColorMapData( Uint32* resolution = NULL, Uint32* mipMapLevel = NULL );
	void*					GetHighestColorMapDataWrite( Uint32* resolution = NULL, Uint32* mipMapLevel = NULL );


	Uint32 GetResolution() const { return m_resolution; }
	Bool HasDataForTileResolution( Uint32 resolution ) const;
	static Bool IsQuadTriangulatedNW2SE( Int32 x, Int32 y );

	RED_INLINE Uint32 GetMipLevel( const STerrainTileMipMap* mip ) const
	{
		// Fast index lookup. Assumes that the mips array does Not change during the eviction process
		Red::System::MemUint mipIndex = mip - &( m_mipmaps[ 0 ] );
		RED_FATAL_ASSERT( mipIndex < m_mipmaps.Size(), "Mip does not belong to this tile or invalid pointer" );

		return static_cast< Uint32 >( mipIndex );
	}

	void GetPotentialMipsToUnload( Bool aggressiveEviction, Float timeDelta, TDynArray< STerrainMipmapEvictionTracker, MC_TerrainClipmap >& mipsList, Red::System::MemSize& loadedMipFootprint );
	void EvictMipmapData( STerrainTileMipMap* mipMapData );
	void ForceEvictMipmaps();

	const STerrainTileMipMap* GetMipMap( Uint32 level ) const;

	//! Get the amount of time left before the mip level is evicted, in range [0,1] -- percent of time, not actual time.
	//! (also 0 if it's not loaded)
	Float GetMipCountdownPercent( Uint32 mipmapLevel ) const;
	

#ifdef TERRAIN_TILE_DEBUG_RENDERING
	Bool IsDirty() const { return m_dirty || m_debugForceRenderingUpdate; }
	void SetDirty( Bool flag ) { if ( flag ) { m_dirty = true; } else { m_dirty = false; m_debugForceRenderingUpdate = false; } }
#else
	Bool IsDirty() const { return m_dirty; }
	void SetDirty( Bool flag ) { m_dirty = flag; }
#endif

	Bool IsCollisionGenerationPending() const;

	static class CPhysicsTileWrapper* CreateCollisionPlaceholder( CWorld* world, const Box& area, Int32 x, Int32 y, Int32 r );
	void GenerateCollision( CClipMap* clipMap, CWorld* world, const Box& area, const TDynArray< CName >& textureNames, TDynArray< STerrainTextureParameters >& textureParameters, CTerrainTile* nextRowTile, CTerrainTile* nextColTile, CTerrainTile* nextRowColTile );
	Bool IsCollisionGenerated() const;

	CompiledCollisionPtr CompileCollision( CObject* parent ) const override final;

	void GetTerrainGeometryVertexes( const Box& extents, const CClipMap* clipMap, Vector& tileCorner, Int32& minX, Int32& minY, Int32& maxX, Int32& maxY, TDynArray< Vector >& outVertexes );

	void InvalidateCollision();

	// Get height map coordinates overlapping with world space bounding box (only x and y are considered)
	Bool GetHeightMapExtents( const Box& extents, const SClipmapParameters& parameters, Int32& minX, Int32& maxX, Int32& minY, Int32& maxY ) const;

	// Get data from the best loaded mip level.
	Bool GetHeightForLocalPosition( const CClipMap* clipMap, const Vector2& position, Float& height, Uint16* texelHeight = NULL );
	TControlMapType GetControlmapValueForLocalPosition( const CClipMap* clipMap, const Vector2& position );
	TColorMapType GetColormapValueForLocalPosition( const CClipMap* clipMap, const Vector2& position );

	// Get data from the given mip level, loading synchronously as needed.
	void GetHeightForLocalPositionSync( const CClipMap* clipMap, const Vector2& position, Uint32 level, Float& height, Uint16* texelHeight = NULL );
	TControlMapType GetControlmapValueForLocalPositionSync( const CClipMap* clipMap, const Vector2& position, Uint32 level );
	TColorMapType GetColormapValueForLocalPositionSync( const CClipMap* clipMap, const Vector2& position, Uint32 level );
	
	static void GetHeightFromHeightmap( const Uint16* tileTexels, const Vector2& position, Float tileSize, Uint32 tileRes, Float lowestElevation, Float heightRange, Float& height, Uint16* texelHeight = NULL );
	static TControlMapType GetValueFromControlmap( const TControlMapType* tileTexels, const Vector2& position, Float tileSize, Uint32 tileRes );

	Uint16 GetMinimumHeight() const { return m_minHeightValue; }
	Uint16 GetMaximumHeight() const { return m_maxHeightValue; }

#ifndef NO_HEIGHTMAP_EDIT
private:
	void UpdateHeightRange();
#endif
};

//////////////////////////////////////////////////////////////////////////
// Inline
RED_INLINE BufferHandle CTerrainTile::GetLevelSyncHMBufferHandle( Uint32 mipmapLevel )
{
	return GetBufferHandleSync( mipmapLevel, TBT_HeightMap );
}

RED_INLINE BufferHandle CTerrainTile::GetLevelSyncCMBufferHandle( Uint32 mipmapLevel )
{
#ifdef TERRAIN_TILE_DEBUG_RENDERING
	if ( mipmapLevel == 0 && m_performDebugRendering )
	{
		BufferProxy* proxy = new BufferProxy( m_debugRenderingColorMap.TypedData(), m_debugRenderingColorMap.Size() * sizeof( TControlMapType ), []( void* ){} );
		return BufferHandle( proxy );
	}
#endif

	return GetBufferHandleSync( mipmapLevel, TBT_ControlMap );
}

RED_INLINE BufferHandle CTerrainTile::GetLevelSyncColorMapBufferHandle( Uint32 mipmapLevel )
{
	return GetBufferHandleSync( mipmapLevel, TBT_ColorMap );
}

//////////////////////////////////////////////////////////////////////////
// RTTI
BEGIN_CLASS_RTTI( CTerrainTile );
	PARENT_CLASS( CResource );
	PROPERTY( m_tileFileVersion );
	PROPERTY( m_collisionType );
	PROPERTY( m_maxHeightValue );
	PROPERTY( m_minHeightValue );
END_CLASS_RTTI();


