/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/deferredDataBuffer.h"

#include "renderObject.h"
#include "renderObjectPtr.h"
#include "terrainTypes.h"

struct STerrainTileMipMap;
class CTerrainTile;

// Parameters defining behavior of terrain textures
struct STerrainTextureParameters
{
	DECLARE_RTTI_STRUCT( STerrainTextureParameters );

	Vector				m_val;
	Vector				m_val2;
};

BEGIN_CLASS_RTTI( STerrainTextureParameters );	
PROPERTY( m_val );
PROPERTY( m_val2 );
END_CLASS_RTTI();

struct SClipmapStampDataUpdate
{
	Uint16* m_heightData;
	Uint32	m_heightDataSize;		//!< size, in bytes, of m_heightData.
	Uint32	m_heightDataPitch;
	Bool	m_heightDataDirty;

	TColorMapType* m_colorData;
	Uint32	m_colorDataSize;		//!< size, in bytes, of m_colorData.
	Uint32	m_colorPitch;
	Bool	m_colorDataDirty;

	TControlMapType* m_controlData;
	Uint32	m_controlDataSize;		//!< size, in bytes, of m_controlData.
	Uint32	m_controlPitch;
	Bool	m_controlDataDirty;

	Vector2	m_center;				//!< 2D world-space center of the stamp
	Float	m_size;					//!< world-space size of the stamp, assumed square
	Float	m_heightScale;
	Float	m_heightOffset;			//!< offset applied to normalized, scaled, stamp height values.
	Float	m_radians;
	Bool	m_additive;				//!< if true, stamp will be added to existing terrain. false, stamp will replace existing.
	Uint32	m_originalDataSize;		//!< size, in elements, of original stamp data, before it was resized to fit the texture.

	Bool	m_isValid;

	SClipmapStampDataUpdate()
		: m_heightData( NULL )
		, m_heightDataSize( 0 )
		, m_heightDataPitch( 0 )
		, m_colorData( NULL )
		, m_colorDataSize( 0 )
		, m_colorPitch( 0 )
		, m_controlData( NULL )
		, m_controlDataSize( 0 )
		, m_controlPitch( 0 )
		, m_center( 0.0f, 0.0f )
		, m_size( 0.0f )
		, m_heightScale( 1.0f )
		, m_heightOffset( 0.0f )
		, m_radians( 0.0f )
		, m_additive( true )
		, m_originalDataSize( 0 )
		, m_isValid( false )
	{ }
	~SClipmapStampDataUpdate()
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_BufferBitmap, m_heightData );
		RED_MEMORY_FREE( MemoryPool_Default, MC_BufferBitmap, m_colorData );
		RED_MEMORY_FREE( MemoryPool_Default, MC_BufferBitmap, m_controlData );
	}

	Bool	IsValid() const { return m_isValid; }
};

struct SClipRegionRectangleData
{
	Int32			m_offsetInTileU;
	Int32			m_offsetInTileV;
	Uint32			m_clipRegionXWriteOffset;
	Uint32			m_clipRegionYWriteOffset;
	Int32			m_texelsToCopyFromTile_Horizontal;
	Int32			m_texelsToCopyFromTile_Vertical;
};

// Rectangle of texels to be uploaded to a specific region of the specific clipmap level on the renderer side.
struct SClipRegionUpdateRectangle
{
	BufferHandle	m_heightMapBuffer;
	BufferHandle	m_controlMapBuffer;

	Uint32			m_resolution;									//!< Resolution of the texture on the tile this rect refers to
	Uint32			m_commonRectDataIndex;
	
	SClipRegionUpdateRectangle()
	{
	}

	~SClipRegionUpdateRectangle() {}
};

// Rectangle of texels of colormap to be uploaded to a specific region of the specific colormap level on the renderer side.
struct SColorMapRegionUpdateRectangle
{
	BufferHandle	m_buffer;										//!< Texels of colormap

	Uint32			m_resolution;									//!< Resolution of the texture on the tile this rect refers to
	Uint32			m_commonRectDataIndex;

	Bool			m_cooked;

	SColorMapRegionUpdateRectangle()
	:	m_cooked( true )
	{
	}

	~SColorMapRegionUpdateRectangle()
	{
	}
};

// Set of update rectangles for the specific clipmap level on the renderer side.
struct SClipmapLevelUpdate
{
	Uint32	m_level;																		//!< Level this update applies to
	Uint32	m_resolution;																	//!< Resolution of the texture on the level this update refers to
	Box		m_worldSpaceCoveredByThisLevel;													//!< Area that can be represented with the data from this level
	Rect	m_validTexels;																	//!< The area of the clipmap data that contains valid data

	TDynArray< SClipRegionRectangleData, MC_TerrainClipmap > m_commonRectData;
	TDynArray< SClipRegionUpdateRectangle, MC_TerrainClipmap >	m_updateRects;				//!< Array of rectangles with new data
	TDynArray< SColorMapRegionUpdateRectangle, MC_TerrainClipmap > m_colormapUpdateRects;

	Bool m_complete;																		// Copy the entire contents rather than going line by line

	SClipmapLevelUpdate( Uint32 level, Uint32 resolution )
	:	m_level( level )
	,	m_resolution( resolution )
	,	m_complete( false )
	{
	}

	~SClipmapLevelUpdate()
	{
	}

	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_TerrainClipmap );
};

// A lightweight structure describing an entry of automatic grass settings
struct SAutomaticGrassDesc
{
private:
	RenderObjectHandle m_resource;
	Float			m_radiusScale;
	Float			m_radiusScaleSquare;
	Float			m_size;
	Float			m_sizeVar;

public:
	SAutomaticGrassDesc( RenderObjectHandle res ) 
		: m_resource( res )
	{}

	void SetRadiusScale( Float radiusScale )
	{
		m_radiusScale = Max( 1.0f, radiusScale );
		m_radiusScaleSquare = m_radiusScale * m_radiusScale;
	}
	void SetSize( Float size ) { m_size = size; }
	void SetSizeVar( Float sizeVar ) { m_sizeVar = sizeVar; }

	RED_FORCE_INLINE IRenderObject* GetResource() const { return m_resource.Get(); }
	RED_FORCE_INLINE Float GetRadiusScale() const { return m_radiusScale; }
	RED_FORCE_INLINE Float GetRadiusScaleSqr() const { return m_radiusScaleSquare; }
	RED_FORCE_INLINE Float GetSize() const { return m_size; }
	RED_FORCE_INLINE Float GetSizeVar() const { return m_sizeVar; }
};

// Struct used to track tile mips that can be unloaded
struct STerrainMipmapEvictionTracker
{
	CTerrainTile* m_ownerTile;
	STerrainTileMipMap* m_mipMap;
	size_t m_dataSize;
	Float m_timeRemaining;
	Bool m_touchedThisUpdateTick;
};
