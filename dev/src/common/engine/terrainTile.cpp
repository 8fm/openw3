/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../redSystem/numericalLimits.h"

#include "../core/directory.h"
#include "../core/dataError.h"
#include "../core/mathUtils.h"
#include "../core/ioTags.h"
#include "../core/depot.h"
#include "../core/configVar.h"
#include "../core/messagePump.h"

#ifndef NO_HEIGHTMAP_EDIT
#	include "heightmapUtils.h"
#endif

#include "../physics/PhysXStreams.h"
#include "../physics/physicsWorld.h"
#include "../physics/physXEngine.h"
#include "terrainTile.h"
#include "selectionManager.h"
#include "../physics/physicsSettings.h"
#include "clipMap.h"
#include "foliageEditionController.h"

#include "game.h"
#include "staticMeshComponent.h"
#include "world.h"
#include "../physics/physicsWrapper.h"
#include "physicsTileWrapper.h"
#include "bitmapTexture.h"
#include "terrainUtils.h"
#include "collisionCache.h"
#include "material.h"
#include "textureArray.h"
#include "globalWater.h"
#include "physicsDataProviders.h"

IMPLEMENT_ENGINE_CLASS( CTerrainTile );
IMPLEMENT_RTTI_ENUM( ETerrainTileCollision );

const Float CTerrainTile::TOUCH_MIP_TIMER_RESET = 0.0001f;

//////////////////////////////////////////////////////////////////////////

namespace Config
{
	TConfigVar< Int32, Validation::IntRange< 0, INT_MAX > >		cvTerrainMipMapAsyncLoadTreshold( "Streaming/Terrain", "AsyncLoadMipSizeTreshold", 16*1024 );
}

//////////////////////////////////////////////////////////////////////////
// 
STerrainTileUpdateParameters::~STerrainTileUpdateParameters()
{
	if ( clipmapParameters )
	{
		delete clipmapParameters;
		clipmapParameters = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
// Helper methods

Float DecodeHalfFloat( Uint16 val )
{
	return (Float) val / 65535.0f;
}

Uint16 EncodeHalfNormFloat( Float val )
{
	ASSERT( val >= 0.0f && val <= 1.0f );

	return (Uint16)( val * (Float)65535 );
}

Uint32 ComputeNumberOfMipmapsForResolution( Uint32 resolution, Uint32 lowestResolution )
{
	// If 1 is returned, that means only full precision mipmap (no mipmapping, represented by one mipmap in the array of mipmaps)
	ASSERT( IsPow2( resolution ) );
	ASSERT( IsPow2( lowestResolution ) );
	ASSERT( resolution >= lowestResolution );

	Uint32 retVal = 1;
	while ( resolution > lowestResolution )
	{
		resolution >>= 1;
		++retVal;
	}

	return retVal;
}

Vector Lerp(const Vector & normalVec, const Vector & eye, Float vertexSlope)
{
	return normalVec + (eye - normalVec)*vertexSlope;
}

Float Saturate(Float val)
{
	return Clamp(val, 0.0f, 1.0f);
}

Float LinearStep( Float min, Float max, Float val )
{
	return Saturate( ( val - min ) / ( max - min ) );
}

Float ComputeSlopeTangent( Vector normal, Float lowThreshold, Float highThreshold )
{
	Float nDotUp = Saturate(Vector::Dot3( normal, Vector::EZ ));
	Float slopeAngle = MAcos_safe( nDotUp );
	return LinearStep( lowThreshold, highThreshold, Saturate( MTan( slopeAngle ) ) );
}

//////////////////////////////////////////////////////////////////////////
// Terrain tile as a resource

//#define TERRAIN_VERSION_ADDED_CONTROLMAP					1
//#define TERRAIN_VERSION_TWO_BYTE_CONTROLMAP				2
//#define TERRAIN_VERSION_ADDED_COLORMAP					3
#define TERRAIN_VERSION_KOSHER_COLORMAP						4
// Colormap is applied with overlay blending, allowing darkening and brightening
#define TERRAIN_VERSION_COLOR_OVERLAY_BLENDING				5
// Use DeferredDataBuffer for tile data
#define TERRAIN_VERSION_USING_DEFERRED_BUFFER				6
// Colormap is BC1 compressed
#define TERRAIN_VERSION_BC1_COLORMAP						7
// Undo BC1 compression, they don't like working with it.
// Also piggy-backing initial calculation of highest height value, so we don't need a second resave.
#define TERRAIN_VERSION_BC1_COLORMAP_UNDO_AND_MAX_HEIGHT	8

#define MINIMUM_TERRAIN_TILE_VERSION	TERRAIN_VERSION_KOSHER_COLORMAP
#define CURRENT_TERRAIN_TILE_VERSION	TERRAIN_VERSION_BC1_COLORMAP_UNDO_AND_MAX_HEIGHT

CTerrainTile::CTerrainTile()
	: m_resolution( 0 )
	, m_colormapStartingMip( 0 )
#ifdef TERRAIN_TILE_DEBUG_RENDERING
	, m_performDebugRendering( false )
	, m_debugForceRenderingUpdate( false )
	, m_debugRenderingVersion( 0xffffffff )
#endif
	, m_wrapper( 0 )
	, m_dirty( true )
	, m_tileFileVersion( 0 )
#ifdef USE_PHYSX
	, m_collisionGenJob( nullptr )
#endif
	, m_collisionGenRequested( false )
	, m_collisionType( TTC_AutoOn )
{
}

CTerrainTile::~CTerrainTile()
{
	RED_ASSERT( !m_wrapper );
}

void CTerrainTile::OnSerialize( IFile& file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );

	if ( file.IsReader() && m_tileFileVersion < MINIMUM_TERRAIN_TILE_VERSION )
	{
		RED_HALT( "Terrain tile is too old. version %u, minimum supported %u. Forcing version, but Bad Things are probably going to happen. %ls", m_tileFileVersion, MINIMUM_TERRAIN_TILE_VERSION, GetFriendlyName().AsChar() );
		m_tileFileVersion = MINIMUM_TERRAIN_TILE_VERSION;
	}

	if ( file.IsGarbageCollector() )
	{
		return;
	}

	Uint32 numMipmaps = m_mipmaps.Size();
	file << numMipmaps;

	if ( file.IsReader() )
	{
		m_mipmaps.Resize( numMipmaps );
		m_mipmapTimes.Resize( numMipmaps );
		m_highestMipWithData = numMipmaps;
		m_touchedMipLevel = numMipmaps;
		for ( Uint32 i=0; i<numMipmaps; ++i )
		{
			m_mipmapTimes[i] = -1.0f;
		}
	}

	for ( Uint32 i=0; i<numMipmaps; ++i )
	{
		m_mipmaps[ i ].SerializeBuffers( file, m_tileFileVersion );
		m_mipmapTimes[i] = -1.0f;

		// Serialize mipmap resolution info
		file << m_mipmaps[i].m_resolution;
		ASSERT( m_mipmaps[i].m_resolution > 0 );
		ASSERT( IsPow2( m_mipmaps[i].m_resolution ) );
	}

	// Serialize finest resolution value
	file << m_resolution;
}

#ifndef NO_RESOURCE_COOKING
void CTerrainTile::OnCook( class ICookerFramework& cooker )
{
	// Drop final mip. Clipmap holds the combined data for these.
	if ( m_mipmaps.Size() > 0 )
	{
		m_mipmaps.RemoveAt( m_mipmaps.Size() - 1 );
	}


	// Compress the color map.
	for ( Uint32 m = 0; m < m_mipmaps.Size(); ++m )
	{
		BufferHandle handle = m_mipmaps[m].AcquireBufferHandleSyncNoTrack( TBT_ColorMap );

		if ( !handle )
		{
			continue;
		}

		Uint32 res = m_mipmaps[m].m_resolution;

		TColorMapType* uncompressedData = static_cast< TColorMapType* >( handle->GetData() );

		const Uint32 compressedSize = TerrainUtils::CalcColorMapSize( res, res );
		DataBuffer compressedDataBuffer( TDataBufferAllocator< MC_TerrainTileMips >::GetInstance(), compressedSize );
		TColorMapRawType* compressedData = static_cast< TColorMapRawType* >( compressedDataBuffer.GetData() );
		TerrainUtils::CompressColor( uncompressedData, compressedData, res, res );

		m_mipmaps[m].SetColorMapData( compressedData, compressedSize );
	}

	TBaseClass::OnCook( cooker );
}

#endif

void CTerrainTile::OnPostLoad()
{

}


#ifndef NO_EDITOR
void CTerrainTile::SetCollisionType( ETerrainTileCollision type )
{
	m_collisionType = type;
	// Clear out collision if it's off. If we've turned it on, it'll be created automatically during update.
	if ( m_collisionType == TTC_AutoOff || m_collisionType == TTC_ForceOff )
	{
		InvalidateCollision();
	}

	// Terrain has a bit of a hack to force rendering of tiles when they have collision. If a tile has collision, then CClipMap sets the
	// minimum water level for that tile to the minimum terrain elevation. This way it can't go below the "water", and will always render.
	// So, if we've changed collision then we need to refresh the minimum water levels.
	if ( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetGlobalWater() )
	{
		GGame->GetActiveWorld()->GetGlobalWater()->NotifyTerrainOfLocalWaterChange();
	}
}
#endif


#ifndef NO_HEIGHTMAP_EDIT

void CTerrainTile::SetRes( Uint32 res, Uint32 minTileMipMapRes, Uint32 colorMapStartingMip )
{
	RED_ASSERT( !IsCooked() );
	RED_ASSERT( IsPow2( res ) );
	RED_ASSERT( res >= LOWEST_TILE_MIPMAP_RESOLUTION );

	if ( IsPow2( res ) && res >= LOWEST_TILE_MIPMAP_RESOLUTION )
	{
		m_resolution = res;
		m_colormapStartingMip = colorMapStartingMip;
		// TODO: get rid of this hacky assumption, that colormap is 4 times smaller than heightmap
		Uint32 numMipmaps = ComputeNumberOfMipmapsForResolution( m_resolution, minTileMipMapRes );
		m_mipmaps.ClearFast();
		m_mipmaps.Resize( numMipmaps );
		m_mipmapTimes.Resize( numMipmaps );
		for ( Float& time : m_mipmapTimes )
		{
			time = 0.0001f;
		}

		for ( Uint32 m=0; m<m_mipmaps.Size(); ++m )
		{
			Uint32 buffers = TBT_HeightMap | TBT_ControlMap;
			if ( m >= colorMapStartingMip )
			{
				buffers |= TBT_ColorMap;
			}

			m_mipmaps[m].ReallocateBuffers( buffers, res );

			res /= 2;
		}
	}

	// set highest version of file, because all necessary buffers were allocated
	m_tileFileVersion = CURRENT_TERRAIN_TILE_VERSION;
}

void CTerrainTile::SetData( const Rect& targetRect, const Rect& sourceRect, const Uint16* sourceTexels, const TControlMapType* sourceCMTexels, Uint32 sourcePitch )
{
	ASSERT( targetRect.Width() == sourceRect.Width() && targetRect.Height() == sourceRect.Height() );
	ASSERT( m_mipmaps.Size() < ComputeNumberOfMipmapsForResolution( m_resolution, LOWEST_TILE_MIPMAP_RESOLUTION ) );	// For now let's assume this here
	ASSERT( m_mipmaps.Size() > 0 );	// For now let's assume this here

	if ( !MarkModified() )
	{
		// Can't get the rights to edit
		return;
	}

	// Before doing anything, make sure we have all mipmaps data available
	if ( !LoadAllMipmapsSync() )
	{
		// Some mipmaps can't be loaded. Break now.
		ERR_ENGINE( TXT("CTerrainTile::SetData() : Can't load mipmap data. Operation aborted.") );
		return;
	}

	// Get tile texel data ptr
	BufferHandle bufferHandle;
	BufferHandle cmtBufferHandle;
	Uint16* targetTexels = NULL;
	if ( sourceTexels )
	{
		bufferHandle = m_mipmaps[0].AcquireBufferForWriting( TBT_HeightMap );
		targetTexels = static_cast< Uint16 * >( bufferHandle->GetData() );
	}
	TControlMapType* targetCMTexels = NULL;
	if ( sourceCMTexels )
	{
		cmtBufferHandle = m_mipmaps[0].AcquireBufferForWriting( TBT_ControlMap );
		targetCMTexels = static_cast< Uint16 * >( cmtBufferHandle->GetData() );
	}
	ASSERT( sourceTexels || sourceCMTexels );

	// Copy rows of data
	Uint32 numRowsToCopy = (Uint32)targetRect.Height();
	Uint32 pitch = targetRect.Width();
	for ( Uint32 i=0; i<numRowsToCopy; ++i )
	{
		if ( sourceTexels )
		{
			void* dstPtr = (void*)&targetTexels[ ( targetRect.m_top + i ) * m_resolution + targetRect.m_left ];
			void* srcPtr = (void*)&sourceTexels[ ( sourceRect.m_top + i ) * sourcePitch + sourceRect.m_left ];

			Red::System::MemoryCopy( dstPtr, srcPtr, pitch * sizeof( Uint16 ) );
		}
		
		if ( sourceCMTexels )
		{
			void* dstPtr = (void*)&targetCMTexels[ ( targetRect.m_top + i ) * m_resolution + targetRect.m_left ];
			void* srcPtr = (void*)&sourceCMTexels[ ( sourceRect.m_top + i ) * sourcePitch + sourceRect.m_left ];

			Red::System::MemoryCopy( dstPtr, srcPtr, pitch * sizeof( TControlMapType ) );
		}
	}

	// Recreate mipmaps
	RebuildMipmaps();

	SetDirty( true );
}

void CTerrainTile::SetColorMapData( const Rect& targetRect, const Rect& sourceRect, const TColorMapType* sourceColorData, Uint32 sourcePitch, Uint32 startingMip /*=0*/ )
{
	RED_ASSERT( !IsCooked() );
	RED_ASSERT( targetRect.Width() == sourceRect.Width() && targetRect.Height() == sourceRect.Height() );
	RED_ASSERT( m_mipmaps.Size() < ComputeNumberOfMipmapsForResolution( m_resolution, LOWEST_TILE_MIPMAP_RESOLUTION ) );	// For now let's assume this here
	RED_ASSERT( m_mipmaps.Size() > 0 );	// For now let's assume this here

	if ( !MarkModified() )
	{
		// Can't get the rights to edit
		return;
	}

	m_colormapStartingMip = startingMip;

	// Before doing anything, make sure we have all mipmaps data available
	if ( !LoadAllMipmapsSync() )
	{
		// Some mipmaps can't be loaded. Break now.
		ERR_ENGINE( TXT("CTerrainTile::SetData() : Can't load mipmap data. Operation aborted.") );
		return;
	}

	// Get tile texel data ptr
	BufferHandle bufferHandle = m_mipmaps[startingMip].AcquireBufferForWriting( TBT_ColorMap );
	void* targetColorData = bufferHandle->GetData();
	if ( !sourceColorData || !targetColorData )
	{
		return;
	}

	// Copy rows of data
	Uint32 numRowsToCopy = (Uint32)targetRect.Height();
	Uint32 bytesPerRow = targetRect.Width() * sizeof( TColorMapType );
	Uint32 dstPitch = m_mipmaps[startingMip].m_resolution * sizeof( TColorMapType );

	const Uint32 elementSize = sizeof( TColorMapType );
	for ( Uint32 i=0; i<numRowsToCopy; ++i )
	{
		void* dstPtr = OffsetPtr( targetColorData, ( targetRect.m_top + i ) * dstPitch + targetRect.m_left * elementSize );
		const void* srcPtr = OffsetPtr( sourceColorData, ( sourceRect.m_top + i ) * sourcePitch + sourceRect.m_left * elementSize );

		Red::System::MemoryCopy( dstPtr, srcPtr, bytesPerRow );
	}

	// Recreate mipmaps
	RebuildMipmaps();

	SetDirty( true );
}

void CTerrainTile::RebuildMipmaps()
{
	RED_ASSERT( !IsCooked() );
	RED_ASSERT( m_mipmaps.Size() < ComputeNumberOfMipmapsForResolution( m_resolution, LOWEST_TILE_MIPMAP_RESOLUTION ) );	// For now let's assume this here
	RED_ASSERT( m_mipmaps.Size() > 0 );	// For now let's assume this here

	BufferHandle dataBufferHandle = m_mipmaps[0].AcquireBufferHandleSync( TBT_HeightMap );
	BufferHandle controlMapBuffer = m_mipmaps[0].AcquireBufferHandleSync( TBT_ControlMap );
	BufferHandle colorMapBuffer = m_mipmaps[0].AcquireBufferHandleSync( TBT_ColorMap );
	BufferHandle colorMapStartingMipBuffer =  m_mipmaps[m_colormapStartingMip].AcquireBufferHandleSync( TBT_ColorMap );

	Bool canRebuildHMandCM = ( dataBufferHandle->GetData() != NULL ) && ( controlMapBuffer->GetData() != NULL );
	Bool canRebuildColorMap = ( colorMapStartingMipBuffer->GetData() != NULL );

	// Call it only after successful modification of the 0 full detail data
	RED_ASSERT( GetFile() ? GetFile()->IsModified() : true );

	// The first mipmap is assumed to be full res data. We rebuild starting from the second mipmap.
	Uint16* sourceTexels = static_cast< Uint16* >( dataBufferHandle->GetData() );
	TControlMapType* sourceCMTexels = (TControlMapType*)controlMapBuffer->GetData();
	TColorMapType* sourceColorMapTexels = (TColorMapType*)colorMapStartingMipBuffer->GetData();

	Uint32 mipTexelStep = 2;
	for ( Uint32 m=1; m<m_mipmaps.Size(); ++m )
	{
		ASSERT( m_mipmaps[m].m_resolution == m_resolution >> m );
		if ( canRebuildHMandCM )
		{
			BufferHandle writeDataHandle = m_mipmaps[m].AcquireBufferForWriting( TBT_HeightMap );
			BufferHandle writeControlMapHandle = m_mipmaps[m].AcquireBufferForWriting( TBT_ControlMap );

			if ( writeDataHandle->GetData() != NULL && writeControlMapHandle->GetData() != NULL )
			{
				// Get destination texels
				Uint16* destTexels = static_cast< Uint16* >( writeDataHandle->GetData() );
				TControlMapType* destCMTexels = (TControlMapType*)writeControlMapHandle->GetData();

				// Copy texels every second x/y
				const Uint32 mipRes = m_mipmaps[m].m_resolution;
				const Uint32 numTargetTexels = mipRes*mipRes;

				// Create mipmap
				for ( Uint32 r=0,rs=0; r<mipRes; ++r,rs+=mipTexelStep)
				{
					for ( Uint32 c=0,cs=0; c<mipRes; ++c,cs+=mipTexelStep )
					{
						destTexels[ r * mipRes + c ] = sourceTexels[ rs * m_resolution + cs ];
						destCMTexels[ r * mipRes + c ] = sourceCMTexels[ rs * m_resolution + cs ];			
					}
				}
			}
		}
		
		Bool rebuildColorMap = canRebuildColorMap && m > m_colormapStartingMip;
		if ( rebuildColorMap )
		{
			BufferHandle writeColorMap = m_mipmaps[m].AcquireBufferForWriting( TBT_ColorMap );

			if ( writeColorMap->GetData() != NULL )
			{
				Uint32 cmTexelStep = 1 << ( m - m_colormapStartingMip );
				TColorMapType* destColorMapTexels = (TColorMapType*)writeColorMap->GetData();
				Uint32 dstRes = m_mipmaps[m].m_resolution;
				Uint32 srcRes = m_mipmaps[m_colormapStartingMip].m_resolution;

				// Create colormap mipmap
				for ( Uint32 r=0,rcm=0; r<dstRes; ++r,rcm+=cmTexelStep)
				{
					for ( Uint32 c=0,ccm=0; c<dstRes; ++c,ccm+=cmTexelStep )
					{
						destColorMapTexels[ r * dstRes + c ] = sourceColorMapTexels[ rcm * srcRes + ccm ];
					}
				}
			}
		}

		// Start eviction counter
		m_mipmapTimes[m] = TOUCH_MIP_TIMER_RESET;

		// Double the texels dropped
		mipTexelStep *= 2;
	}


	UpdateHeightRange();
}

Bool CTerrainTile::LoadAllMipmapsSync()
{
	Bool retVal = true;
	for ( Uint32 i=0; i<m_mipmaps.Size(); ++i )
	{
		retVal &= m_mipmaps[i].LoadSync();

		// No excuses, start eviction count :)
		m_mipmapTimes[i] = TOUCH_MIP_TIMER_RESET;
	}

	return retVal;
}

Bool CTerrainTile::ImportData( const String& absolutePath )
{
	Uint32 width = 0, height = 0;
	SHeightmapImageEntry<Uint16> entry;
	if ( !SHeightmapUtils::GetInstance().LoadHeightmap( absolutePath, entry, &width, &height ) )
	{
		return false;		
	}

	if ( !IsPow2( width ) || !IsPow2( height ) )
	{
		ERR_ENGINE( TXT("Terrain tile import: texture size should be power of two, but is [%i x %i]"), width, height );
		return false;
	}
	if ( width != m_resolution || height != m_resolution )
	{
		ERR_ENGINE( TXT("Terrain tile import: imported tile size [%i x %i] differs from current tile resolution setup [%d x %d]"), width, height, m_resolution, m_resolution );
		return false;
	}

	Rect sourceAndTargetRect( 0, m_resolution, 0, m_resolution );
	SetData( sourceAndTargetRect, sourceAndTargetRect, entry.m_data, NULL, m_resolution );

	// Mark tile as modified since last clipmap update
	SetDirty( true );
	return true;
}

Bool CTerrainTile::ImportColorMap( const String& absolutePath )
{
	RED_ASSERT( !IsCooked() );

	Uint32 width = 0, height = 0;
	SHeightmapImageEntry<Uint32> entry;
	if ( !SHeightmapUtils::GetInstance().LoadColor( absolutePath, entry, &width, &height ) )
	{
		return false;
	}

	if ( !IsPow2( width ) || !IsPow2( height ) )
	{
		ERR_ENGINE( TXT("Terrain tile import: colormap size should be power of two not [%i x %i]"), width, height );
		return false;
	}

	Uint32 desiredResolution = m_mipmaps[ m_colormapStartingMip ].m_resolution;

	if ( width != desiredResolution || height != desiredResolution )
	{
		ERR_ENGINE( TXT("Terrain tile import: imported colormap size [%i x %i] differs from max colormap resolution setup [%d x %d]"), width, height, desiredResolution, desiredResolution );
		return false;
	}

	Rect sourceAndTargetRect( 0, desiredResolution, 0, desiredResolution );
	SetColorMapData( sourceAndTargetRect, sourceAndTargetRect, entry.m_data, desiredResolution*sizeof(TColorMapType), m_colormapStartingMip );

	// Mark tile as modified since last clipmap update
	SetDirty( true );
	return true;
}


Bool CTerrainTile::ExportData( const String& absolutePath )
{
	ASSERT( m_mipmaps.Size() );

	BufferHandle handle = m_mipmaps[ 0 ].AcquireBufferHandleSync( TBT_HeightMap );

	if ( !handle )
	{
		return false;
	}
	Uint16* data = static_cast< Uint16* >( handle->GetData() );

	if ( !SHeightmapUtils::GetInstance().SaveImage( absolutePath, data, m_resolution, m_resolution ) )
	{
		ERR_ENGINE( TXT( "Terrain tile export: saving image '%ls' failed." ), absolutePath.AsChar() );
		return false;
	}
	return true;
}

Bool CTerrainTile::ExportColorMap( const String& absolutePath )
{
	RED_ASSERT( !IsCooked() );
	RED_ASSERT( m_mipmaps.Size() );

	BufferHandle handle = m_mipmaps[ m_colormapStartingMip ].AcquireBufferHandleSync( TBT_ColorMap );
	if ( !handle )
	{
		return false;
	}

	TColorMapType* data = static_cast< TColorMapType* >( handle->GetData() );

	if ( !SHeightmapUtils::GetInstance().SaveImage( absolutePath, data, m_mipmaps[m_colormapStartingMip].m_resolution, m_mipmaps[m_colormapStartingMip].m_resolution ) )
	{
		ERR_ENGINE( TXT( "Terrain tile export: saving colormap '%ls' failed." ), absolutePath.AsChar() );
		return false;
	}
	return true;
}


void CTerrainTile::UpdateTileStructure( CClipMap* clipmap )
{
	RED_ASSERT( !IsCooked() );

	if ( m_tileFileVersion < CURRENT_TERRAIN_TILE_VERSION )
	{
		LOG_ENGINE( TXT("Terrain tile %ls has old tile version (has %u, current %u). Upgrading."), GetDepotPath().AsChar(), m_tileFileVersion, CURRENT_TERRAIN_TILE_VERSION );
	}


	m_colormapStartingMip = clipmap->GetColormapStartingMip();

	if ( m_tileFileVersion < TERRAIN_VERSION_KOSHER_COLORMAP )
	{
		RED_ASSERT( m_colormapStartingMip < m_mipmaps.Size() );

		// previous version has data in following layout:
		// colormap's resolution in mipmap[i] equals resolution/4 of heightmap in mipmap[i]
		// new layout requires that all resolutions match in mipmap levels to simplify the computations
		// some of the highest mipmap levels can have empty colormap in order not to take too much space
		// the following code tries to preserve as much data as possible, transferring the buffers to appropriate mipmap levels

		// 1) transfer the buffers to their appropriate place (for matching resolutions)
		//		a) if desired starting mip is higher than 2 (0 or 1), we have to provide higher resolution data than we actually have, just stretch
		// 2) discard all mipmap levels above the starting one

		if ( m_mipmaps.Size() > 0 )
		{
			if ( m_mipmaps.Size() >= 3 )
			{
				// fill mipmap stack starting from lowest mip with data of appropriate size
				for ( Int32 i = m_mipmaps.Size() - 1 - 2; i >= 0; --i )
				{
					Uint32 srcIndex = i;
					Uint32 dstIndex = i + 2;

					BufferHandle srcHandle = m_mipmaps[srcIndex].AcquireBufferHandleSync( TBT_ColorMap );
					ASSERT( srcHandle );

					m_mipmaps[dstIndex].SetColorMapData( srcHandle->GetData(), srcHandle->GetSize() );
				}
			}

			BufferHandle color0 = m_mipmaps[0].AcquireBufferHandleSync( TBT_ColorMap );

			// fill second mip (upscale)
			if ( m_mipmaps.Size() >= 2 )
			{
				SHeightmapImageEntry<Uint32> entry;
				if ( SHeightmapUtils::GetInstance().ResizeImage( (Uint32*)color0->GetData(), m_mipmaps[0].m_resolution/4, m_mipmaps[0].m_resolution/4, m_mipmaps[1].m_resolution, m_mipmaps[1].m_resolution, entry ) )
				{
					Uint32 size = m_mipmaps[1].m_resolution;
					size *= size;
					size *= sizeof(Uint32);
			
					m_mipmaps[1].SetColorMapData( entry.m_data, size );
				}
			}
			// fill first mip (upscale)
			if ( m_mipmaps.Size() >= 1 )
			{
				SHeightmapImageEntry<Uint32> entry;
				if ( SHeightmapUtils::GetInstance().ResizeImage( (Uint32*)color0->GetData(), m_mipmaps[0].m_resolution/4, m_mipmaps[0].m_resolution/4, m_mipmaps[0].m_resolution, m_mipmaps[0].m_resolution, entry ) )
				{
					Uint32 size = m_mipmaps[0].m_resolution;
					size *= size;
					size *= sizeof(Uint32);

					m_mipmaps[0].SetColorMapData( entry.m_data, size );
				}
			}
		}
	}


	// Rescale color data so old [0, 1] fits into [0, 0.5]
	if ( m_tileFileVersion < TERRAIN_VERSION_COLOR_OVERLAY_BLENDING )
	{
		for ( Uint32 m = m_colormapStartingMip; m < m_mipmaps.Size(); ++m )
		{
			// Make sure the color data is loaded.
			BufferHandle handle = m_mipmaps[m].AcquireBufferForWriting( TBT_ColorMap );

			// Rescale all elements. For simplicity, this includes the alpha channel, which is now unused.
			// So, alpha could contain anything. If it's used again later, make sure to clear it out!
			Uint8* colorData = (Uint8*)handle->GetData();
			Uint32 numElements = handle->GetSize();
			for ( Uint32 i = 0; i < numElements; ++i )
			{
				colorData[i] /= 2;
			}
		}
	}

	// Undo color compression. The previous upgrade _to_ BC1_COLORMAP is removed, since we don't need to compress then decompress.
	if ( m_tileFileVersion == TERRAIN_VERSION_BC1_COLORMAP )
	{
		// Check if we have the terrain tiles from before compression was added. If so, we'll extract colormaps from there.
		CFilePath thisPath( GetDepotPath() );
		thisPath.PopDirectory();
		thisPath.PushDirectory( TXT("terrain_tiles_colsrc") );

		String newTilePath = thisPath.ToString();

		CTerrainTile* beforeTile = LoadResource< CTerrainTile >( newTilePath );
		if ( beforeTile != nullptr )
		{
			for ( Uint32 m = 0; m < m_mipmaps.Size(); ++m )
			{
				BufferHandle colorHandle = beforeTile->m_mipmaps[m].AcquireBufferHandleSync( TBT_ColorMap );
				if ( colorHandle )
				{
					m_mipmaps[m].SetColorMapData( colorHandle->GetData(), colorHandle->GetSize() );
				}
				else
				{
					m_mipmaps[m].SetColorMapData( nullptr, 0 );
				}
			}
		}
		// Couldn't restore old colormap, so just decompress what we have.
		else
		{
			for ( Uint32 m = 0; m < m_mipmaps.Size(); ++m )
			{
				BufferHandle handle = m_mipmaps[m].AcquireBufferHandleSyncNoTrack( TBT_ColorMap );

				if ( !handle )
				{
					continue;
				}

				Uint32 res = m_mipmaps[m].m_resolution;

				TColorMapRawType* compressedData = static_cast< TColorMapRawType* >( handle->GetData() );

				const Uint32 decompressedSize = res*res*sizeof( TColorMapType );
				DataBuffer decompressedDataBuffer( TDataBufferAllocator< MC_Temporary >::GetInstance(), decompressedSize );
				TColorMapType* decompressedData = static_cast< TColorMapType* >( decompressedDataBuffer.GetData() );

				TerrainUtils::DecompressColor( compressedData, decompressedData, res, res );

				m_mipmaps[m].SetColorMapData( decompressedData, decompressedSize );
			}
		}
	}

	// Make sure we have a proper maximum height value.
	if ( m_tileFileVersion < TERRAIN_VERSION_BC1_COLORMAP_UNDO_AND_MAX_HEIGHT )
	{
		UpdateHeightRange();
	}

	//dex--: resaving is done once

	// resave when version changed
	if ( m_tileFileVersion != CURRENT_TERRAIN_TILE_VERSION )
	{
		// Set tile file version to newest
		m_tileFileVersion = CURRENT_TERRAIN_TILE_VERSION;
		Save();
	}

	// data size check
	{
		Int32 res = m_resolution;
		for ( Uint32 i=0; i<m_mipmaps.Size(); ++i )
		{
			const Uint32 colorMapDataSize = m_mipmaps[i].GetBufferDataSize( TBT_ColorMap );
			const Uint32 controlMapDataSize = m_mipmaps[i].GetBufferDataSize( TBT_ControlMap );
			const Uint32 heightmapDataSize = m_mipmaps[i].GetBufferDataSize( TBT_HeightMap );
			if ( i >= m_colormapStartingMip )
			{
				ASSERT( colorMapDataSize == (res*res*sizeof(TColorMapType)) );
			}
			ASSERT( controlMapDataSize == (res*res*sizeof(TControlMapType)) );
			ASSERT( heightmapDataSize == (res*res*sizeof(Uint16)) );
			res /= 2;
		}
	}
}

void CTerrainTile::SaveFirstTime( CWorld* world, Uint32 row, Uint32 col, const String& directoryName )
{
	ASSERT( world );

	CDirectory* tilesDir = world->GetFile()->GetDirectory()->CreateNewDirectory( directoryName );
	ASSERT( tilesDir );

	const String tileName = String::Printf( TXT("tile_%i_x_%i_res%i.w2ter"), row, col, m_resolution );
	SaveAs( tilesDir, tileName );
}

void CTerrainTile::UnloadMipmaps()
{
	for ( Uint32 m=0; m<m_mipmaps.Size(); ++m )
	{
		STerrainTileMipMap& mipMap = m_mipmaps[m];

		mipMap.Unload();

		if ( m == 0 )
		{
			InvalidateCollision();
		}

		// Don't count eviction timeout
		m_mipmapTimes[m] = -1.0f;
	}
}

#endif // !NO_HEIGHTMAP_EDIT


void CTerrainTile::TouchMip( Uint32 mipmapLevel )
{
	if ( mipmapLevel < m_mipmaps.Size() )
	{
		m_touchedMipLevel = Min<Int32>( m_touchedMipLevel, mipmapLevel );
		m_highestMipWithData = Min<Int32>( m_highestMipWithData, mipmapLevel );
		m_mipmapTimes[mipmapLevel] = TOUCH_MIP_TIMER_RESET;
	}
}


Bool CTerrainTile::LoadSync( Uint32 mipmapLevel )
{
	if ( mipmapLevel >= m_mipmaps.Size() )
	{
		return false;
	}

	TouchMip( mipmapLevel );

	STerrainTileMipMap& mip = m_mipmaps[mipmapLevel];
	return mip.LoadSync();
}

void CTerrainTile::StartAsyncLoad( Uint32 mipmapLevel )
{
	if ( mipmapLevel >= m_mipmaps.Size() )
	{
		return;
	}

	STerrainTileMipMap& mip = m_mipmaps[mipmapLevel];
	mip.RequestAsyncLoad();

	TouchMip( mipmapLevel );
}


Bool CTerrainTile::IsLoaded( Uint32 mipmapLevel ) const
{
	if ( mipmapLevel >= m_mipmaps.Size() )
	{
		return false;
	}
	const STerrainTileMipMap& mip = m_mipmaps[mipmapLevel];
	return mip.IsLoaded();
}

Bool CTerrainTile::IsPartiallyLoaded( Uint32 mipmapLevel ) const
{
	if ( mipmapLevel >= m_mipmaps.Size() )
	{
		return false;
	}
	const STerrainTileMipMap& mip = m_mipmaps[mipmapLevel];
	return mip.IsPartiallyLoaded();
}


Bool CTerrainTile::IsLoadingAsync( Uint32 mipmapLevel ) const
{
	if ( mipmapLevel >= m_mipmaps.Size() )
	{
		return false;
	}
	const STerrainTileMipMap& mip = m_mipmaps[mipmapLevel];
	return mip.IsLoading();
}

Bool CTerrainTile::IsLoadingAnyAsync() const
{
	for ( Uint32 i = 0; i < m_mipmaps.Size(); ++i )
	{
		if ( m_mipmaps[i].IsLoading() )
		{
			return true;
		}
	}
	return false;
}


Bool CTerrainTile::WasLoaded( Uint32 mipmapLevel ) const
{
	if ( mipmapLevel >= m_mipmaps.Size() )
	{
		return false;
	}
	const STerrainTileMipMap& mip = m_mipmaps[mipmapLevel];
	return mip.WasLoaded();
}

void CTerrainTile::UpdateWasLoaded( Uint32 mipmapLevel )
{
	if ( mipmapLevel < m_mipmaps.Size() )
	{
		m_mipmaps[mipmapLevel].UpdateWasLoaded();
	}
}


Bool CTerrainTile::HasDataLoaded( ETerrainBufferType buffer ) const
{
	for ( Uint32 m = 0; m < m_mipmaps.Size(); ++m )
	{
		if ( m_mipmaps[m].HasBufferDataLoaded( buffer ) )
		{
			return true;
		}
	}
	return false;
}

void CTerrainTile::BindCookedData( const Uint32 mipmapLevel, BufferHandle height, BufferHandle contorl, BufferHandle color )
{
	RED_FATAL_ASSERT( mipmapLevel < m_mipmaps.Size(), "Mipmap index out of range" );
	m_mipmaps[ mipmapLevel ].BindCookedData( height, contorl, color );
}

BufferHandle CTerrainTile::GetBufferHandleSync( Uint32 mipmapLevel, ETerrainBufferType type )
{
	ASSERT( mipmapLevel < m_mipmaps.Size() );

	if ( mipmapLevel < m_mipmaps.Size() )
	{
		BufferHandle handle = m_mipmaps[mipmapLevel].AcquireBufferHandleSync( type );
		if ( handle )
		{
			TouchMip( mipmapLevel );
			return handle;
		}

		LOG_ENGINE( TXT("Couldn't load data buffer for mipmap level %i, tile %s"), mipmapLevel, GetFile()->GetFileName().AsChar() );
	}

	return BufferHandle();
}

const Uint16* CTerrainTile::GetLevelSyncHM( Uint32 mipmapLevel )
{
	BufferHandle handle = GetBufferHandleSync( mipmapLevel, TBT_HeightMap );
	return ( handle )? static_cast< const Uint16* >( handle->GetData() ) : nullptr;
}

Uint16* CTerrainTile::GetLevelWriteSyncHM( Uint32 mipmapLevel )
{
	ASSERT( mipmapLevel < m_mipmaps.Size() );

	if ( mipmapLevel < m_mipmaps.Size() )
	{
		BufferHandle handle = m_mipmaps[mipmapLevel].AcquireBufferForWriting( TBT_HeightMap );
		if ( handle )
		{
			TouchMip( mipmapLevel );
			return static_cast< Uint16* >( handle->GetData() );
		}

		LOG_ENGINE( TXT("Couldn't load data buffer for mipmap level %i, tile %s"), mipmapLevel, GetFile()->GetFileName().AsChar() );
	}

	return nullptr;
}

const Uint16* CTerrainTile::GetAnyLevelHM( Uint32& level )
{
	for ( Uint32 m=0; m<m_mipmaps.Size(); ++m )
	{
		BufferHandle handle = m_mipmaps[m].GetLoadedBufferHandle( TBT_HeightMap );
		if ( handle )
		{
			TouchMip( m );
			level = m;
			return static_cast< const Uint16* >( handle->GetData() );
		}
	}

	return nullptr;
}

const TControlMapType* CTerrainTile::GetLevelSyncCM( Uint32 mipmapLevel )
{
	BufferHandle handle = GetLevelSyncCMBufferHandle( mipmapLevel );
	return ( handle )? static_cast< const Uint16* >( handle->GetData() ) : nullptr;
}

TControlMapType* CTerrainTile::GetLevelWriteSyncCM( Uint32 mipmapLevel )
{
	ASSERT( mipmapLevel < m_mipmaps.Size() );
	if ( mipmapLevel < m_mipmaps.Size() )
	{
		BufferHandle handle = m_mipmaps[mipmapLevel].AcquireBufferForWriting( TBT_ControlMap );
		if ( handle )
		{
			TouchMip( mipmapLevel );
			return static_cast< TControlMapType* >( handle->GetData() );
		}

		LOG_ENGINE( TXT("Couldn't load data buffer for mipmap level %i, tile %s"), mipmapLevel, GetFile()->GetFileName().AsChar() );
	}

	return nullptr;
}

const TControlMapType* CTerrainTile::GetAnyLevelCM( Uint32& level )
{
	for ( Uint32 m=0; m < m_mipmaps.Size(); ++m )
	{
		BufferHandle handle = m_mipmaps[m].GetLoadedBufferHandle( TBT_ControlMap );
		if ( handle )
		{
			TouchMip( m );
			level = m;
			return static_cast< const TControlMapType* >( handle->GetData() );
		}
	}

	return nullptr;
}

const void* CTerrainTile::GetLevelSyncColorMap( Uint32 mipmapLevel )
{
	BufferHandle handle = GetBufferHandleSync( mipmapLevel, TBT_ColorMap );
	return ( handle )? handle->GetData() : nullptr;
}

void* CTerrainTile::GetLevelWriteSyncColorMap( Uint32 mipmapLevel )
{
	ASSERT( mipmapLevel < m_mipmaps.Size() );
	if ( mipmapLevel < m_mipmaps.Size() )
	{
		BufferHandle handle = m_mipmaps[mipmapLevel].AcquireBufferForWriting( TBT_ColorMap );
		if ( handle )
		{
			TouchMip( mipmapLevel );
			return handle->GetData();
		}

		LOG_ENGINE( TXT("Couldn't load data buffer for mipmap level %i, tile %s"), mipmapLevel, GetFile()->GetFileName().AsChar() );
	}

	return nullptr;
}

const void* CTerrainTile::GetAnyLevelColorMap( Uint32& level )
{
	for ( Uint32 m=m_colormapStartingMip; m<m_mipmaps.Size(); ++m )
	{
		BufferHandle handle = m_mipmaps[m].GetLoadedBufferHandle( TBT_ColorMap );
		if ( handle )
		{
			TouchMip( m );
			level = m;
			return handle->GetData();
		}
	}

	return nullptr;
}

Uint32 CTerrainTile::GetHighestColorMapResolution()
{
	if ( m_colormapStartingMip < m_mipmaps.Size() )
	{
		return m_mipmaps[m_colormapStartingMip].m_resolution;
	}

	return 0;
}

Uint32 CTerrainTile::GetColorMapResolutionForMipMapLevel( Uint32 mipMapLevel )
{
	ASSERT( mipMapLevel >= m_colormapStartingMip );
	ASSERT( mipMapLevel < m_mipmaps.Size() );

	return m_mipmaps[ mipMapLevel ].m_resolution;
}

const void* CTerrainTile::GetHighestColorMapData( Uint32* resolution /* = NULL */, Uint32* mipMapLevel /* = NULL */ )
{
	if ( m_colormapStartingMip < m_mipmaps.Size() )
	{
		BufferHandle handle = m_mipmaps[m_colormapStartingMip].AcquireBufferHandleSync( TBT_ColorMap );
		if ( handle )
		{
			if ( mipMapLevel )
			{
				*mipMapLevel = m_colormapStartingMip;
			}
			if ( resolution )
			{
				*resolution = m_mipmaps[m_colormapStartingMip].m_resolution;
			}
			TouchMip( m_colormapStartingMip );

			return handle->GetData();
		}
		
		LOG_ENGINE( TXT("Couldn't load data buffer for mipmap level %i, tile %s"), m_colormapStartingMip, GetFile()->GetFileName().AsChar() );
	}

	return nullptr;
}


void* CTerrainTile::GetHighestColorMapDataWrite( Uint32* resolution /*= NULL*/, Uint32* mipMapLevel /*= NULL*/ )
{
	if ( m_colormapStartingMip < m_mipmaps.Size() )
	{
		BufferHandle handle = m_mipmaps[m_colormapStartingMip].AcquireBufferForWriting( TBT_ColorMap );
		if ( handle )
		{
			if ( mipMapLevel )
			{
				*mipMapLevel = m_colormapStartingMip;
			}
			if ( resolution )
			{
				*resolution = m_mipmaps[m_colormapStartingMip].m_resolution;
			}
			TouchMip( m_colormapStartingMip );

			return handle->GetData();
		}

		LOG_ENGINE( TXT("Couldn't load data buffer for mipmap level %i, tile %s"), m_colormapStartingMip, GetFile()->GetFileName().AsChar() );
	}

	return nullptr;
}


Bool CTerrainTile::HasDataForTileResolution( Uint32 resolution ) const
{
	for ( Uint32 i=0; i<m_mipmaps.Size(); ++i )
	{
		if ( m_mipmaps[i].m_resolution == resolution )
		{
			// Found
			return true;
		}
	}

	// Not found
	return false;
}

Red::System::MemSize CTerrainTile::GetMipmapResidentMemory() const
{
	const Uint32 mipCount = m_mipmaps.Size();
	Red::System::MemSize totalResident = 0;
	for ( Uint32 m=0; m<mipCount; ++m )
	{
		const STerrainTileMipMap& mipMap = m_mipmaps[m];
		totalResident += mipMap.GetResidentMemory();
	}

	return totalResident;
}

Bool CTerrainTile::DidRequestAsyncSinceLastTime( Uint32 mipmapLevel ) const
{
	if ( mipmapLevel >= m_mipmaps.Size() )
	{
		return false;
	}

	return m_mipmaps[mipmapLevel].DidRequestAsyncSinceLastTime();
}


void CTerrainTile::EvictMipmapData( STerrainTileMipMap* mipMapData )
{
	if ( IsModified() )
	{
#ifdef NO_RESOURCE_IMPORT
		RED_HALT( "Cannot resave terrain tile when resource import is disabled" );
#else
		Save();
#endif
	}

	const Uint32 mipIndex = GetMipLevel( mipMapData );

	// Unload data
	mipMapData->Unload();

	if ( mipIndex == 0 )
	{
		InvalidateCollision();
	}

	m_mipmapTimes[mipIndex] = -1.0f;

	m_highestMipWithData = m_mipmaps.SizeInt();
}

void CTerrainTile::ForceEvictMipmaps()
{
	for ( Uint32 m=0; m<m_mipmaps.Size(); ++m )
	{
		STerrainTileMipMap* mipMap = &m_mipmaps[m];
		EvictMipmapData( mipMap );
	}

	m_touchedMipLevel = m_mipmaps.SizeInt();
	m_highestMipWithData = m_mipmaps.SizeInt();
}

void CTerrainTile::GetPotentialMipsToUnload( Bool aggressiveEviction, Float timeDelta, TDynArray< STerrainMipmapEvictionTracker, MC_TerrainClipmap >& mipsList, Red::System::MemSize& loadedMipFootprint )
{
	RED_ASSERT( timeDelta < 0.25f, TXT("TerrainTile getting a large timeDelta %0.3f. This should be limited, to avoid causing all tiles to be unloaded after a long frame."), timeDelta );

	Float minTimeout = STerrainTileMipMap::GetMinimumTimeout();

	Red::System::MemSize tileMipsFootprint = 0;

	const Int32 numMips = m_mipmaps.SizeInt();


	// Don't need to worry about cases where we don't have any mips with data, because we keep the smallest mip alive.
	// So in situations where we know a value (i.e. not after an eviction), this will definitely be < numMips.
	const Bool needToFindHighestMip = m_highestMipWithData >= numMips;

	const Int32 firstToCheck = needToFindHighestMip ? 0 : m_highestMipWithData;

	if ( needToFindHighestMip )
	{
		m_highestMipWithData = m_touchedMipLevel;
	}

	// We don't need to check the touched mip, because we know it can't be evicted. It was touched this frame...
	// We assume that if a mip is touched, then every smaller mip is also touched. This will generally be the case, since the
	// clip windows are layered, and we don't leave a hole in the middle where the next window is.


	for ( Int32 m = firstToCheck; m < m_touchedMipLevel; ++m )
	{
		if ( needToFindHighestMip && ( m_mipmaps[m].IsPartiallyLoaded() || m_mipmaps[m].IsLoading() ) )
		{
			m_highestMipWithData = Min( m_highestMipWithData, m );
		}


		Float time = m_mipmapTimes[m];

		// Update the timeout for any mips that are loaded
		if( time >= 0.0f )
		{
			time += timeDelta;
			m_mipmapTimes[m] = time;

			// If CanEvict returns true, the mip has been resident for long enough that we can evict it if needed
			if ( time > minTimeout || aggressiveEviction )
			{
				STerrainTileMipMap& mipMap = m_mipmaps[m];

				STerrainMipmapEvictionTracker evictionData;
				evictionData.m_ownerTile = this;
				evictionData.m_mipMap = &mipMap;
				evictionData.m_dataSize = mipMap.GetResidentMemory();
				evictionData.m_timeRemaining = mipMap.GetTimeout() - m_mipmapTimes[m];
				evictionData.m_touchedThisUpdateTick = MAbs( m_mipmapTimes[m] - TOUCH_MIP_TIMER_RESET ) < Red::System::NumericLimits< Float >::Epsilon();

				tileMipsFootprint += evictionData.m_dataSize;

				// Add this mip to the list of (potential) mips to unload
				mipsList.PushBack( evictionData );
			}
		}
	}

	m_touchedMipLevel = numMips;

	loadedMipFootprint = tileMipsFootprint;
}

const STerrainTileMipMap* CTerrainTile::GetMipMap( Uint32 level ) const
{
	if ( level >= m_mipmaps.Size() )
	{
		return nullptr;
	}
	return &m_mipmaps[level];
}


namespace TerrainUtils
{
	// return true if coordinate is invalid, and make a proper correction to it by the way
	inline Bool validateUV( Int32& uv, Int32 r )
	{
		const Bool invalid( uv < 0 );
		uv = invalid ? uv + r : uv;
		return invalid;
	}

	// get one sample 
	Uint16 sample( const Uint16** data, const TControlMapType** controlMaps, Int32 u, Int32 v, Int32 r, TControlMapType& controlMapVal )
	{
		union BoolsToIndexChanger // my little magic union to change 2 boolean values into one index in range of 0..3
		{
			struct  
			{
				Bool invalidU : 1;
				Bool invalidV : 1;
			} bits;
			Uint8 index;
		};																								

		// if U or V is negative, then we need to change the sampled tile and correct U or V or both 
		// we do it by this simple trick:
		BoolsToIndexChanger uvStatus;
		uvStatus.index = 0;
		uvStatus.bits.invalidU = TerrainUtils::validateUV( u, r );	// if u is invalid, then take previous row tile
		uvStatus.bits.invalidV = TerrainUtils::validateUV( v, r );	// if v is invalid, then take previous col tile

		// assert if anything went wrong
		ASSERT( uvStatus.index < 4 );
		ASSERT( u < r && u >= 0 );
		ASSERT( v < r && v >= 0 );

		// return the control map value
		controlMapVal = controlMaps[ uvStatus.index ][ u * r + v ];

		// return the value from a proper heightmap
		return data[ uvStatus.index ][ u * r + v ];
	}

	// get the average from 4 samples as int
	inline Int32 avgSamples( Uint16 s1, Uint16 s2, Uint16 s3, Uint16 s4 )
	{
		return ( Int32( s1 ) + Int32( s2 ) + Int32( s3 ) + Int32( s4 ) ) / 4; 	
	}
}

CPhysicsTileWrapper* CTerrainTile::CreateCollisionPlaceholder( CWorld* world, const Box& area, Int32 x, Int32 y, Int32 r )
{
	const Vector boxSize = area.CalcSize();
	Box2 box = Box2( ( area.Min ).AsVector2(), area.Max.AsVector2() );
	CPhysicsWorld* physicsWorld = nullptr;
	if( !world->GetPhysicsWorld( physicsWorld ) ) return nullptr;
	return physicsWorld->GetWrappersPool< CPhysicsTileWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( nullptr ), physicsWorld, box );
}


#ifdef USE_PHYSX

PxVec3 CalculateNormalSafe(const Uint32 & i, const Uint32 & j, PxHeightField* heightfield, const Uint32 & res, const Uint32 & triangleXOffset = 0)
{
	Uint32 pos = i * ( res * 2 ) + (j * 2 ) + triangleXOffset;

	PxVec3 normal = heightfield->getTriangleNormal( pos );
	return normal;
}

PxVec3 CalculateNormal(const Uint32 & i, const Uint32 & j, PxHeightField* heightfield,const Uint32 & res)
{
	PxVec3 normal = PxVec3(0,0,0);
	const Bool jMoreZero = j > 0;
	const Bool jLessRes = j < res;
	const Bool iMoreZero = i > 0;
	const Bool iLessRes = i < res;
	float normalCnt = 0;
	if(iLessRes && jLessRes)
	{
		normal += CalculateNormalSafe(i,j, heightfield, res);
		normalCnt += 1;

		if(iMoreZero && jMoreZero)
		{
			normal += CalculateNormalSafe(i - 1,j - 1, heightfield, res, 1);
			normalCnt += 1;
		}		
	}
	else if (iLessRes) // j == res
	{
		if(iMoreZero)
		{
			normal += CalculateNormalSafe(i - 1,j - 1, heightfield, res, 1);
			normalCnt += 1;
		}
		else
		{
			normal += CalculateNormalSafe(i,j - 1, heightfield, res, 1);
			normalCnt += 1;
		}
	}
	else if (jLessRes) // i == res
	{
		if(jMoreZero)
		{
			normal += CalculateNormalSafe(i - 1,j - 1, heightfield, res, 1);
			normalCnt += 1;
		}
		else
		{
			normal += CalculateNormalSafe(i - 1,j, heightfield, res, 1);
			normalCnt += 1;
		}
	}

	return normal / normalCnt;
}

class CTerrainTileCollisionJob : public CTask
{
private:

	CTerrainTile* m_tile;

	TDynArray< char >					m_physicalMaterials;
	TDynArray< STerrainTextureParameters > m_textureParameters;
	Uint32								m_resolution;
	Vector								m_boxSize;


	TDynArray< PxHeightFieldGeometry >	m_sectorGeometry;


	const SPhysicalMaterial* m_singleMaterial;
	TDynArray< PxHeightField* > m_sectorHeightfield;

	CTerrainTile* m_nextRowTile;
	CTerrainTile* m_nextColTile;
	CTerrainTile* m_nextRowColTile;


public:
	CTerrainTileCollisionJob( CTerrainTile* tile, CTerrainTile* nextRowTile, CTerrainTile* nextColTile, CTerrainTile* nextRowColTile, Uint32 resolution, const Vector& boxSize, const TDynArray< CName >& textureNames, const TDynArray< STerrainTextureParameters >& textureParameters )
		: m_tile( tile )
		, m_textureParameters( textureParameters )
		, m_resolution( resolution )
		, m_boxSize( boxSize )
		, m_singleMaterial( nullptr )
		, m_nextRowTile( nextRowTile )
		, m_nextColTile( nextColTile )
		, m_nextRowColTile( nextRowColTile )
	{
		for( Uint32 i = 0; i != textureNames.Size(); ++i )
		{
			char physicalMaterialIndex = GPhysicEngine->GetTexturePhysicalMaterialIndex( textureNames[ i ] );
			if( physicalMaterialIndex == -1 )
			{
				DATA_HALT( DES_Major, tile, TXT("Terrain collision generation"), TXT("Texture ( %s ) used to paint terrain isn't mapped to any physical material in terrain_textures_physical_materials.csv file"), textureNames[ i ].AsString().AsChar() );
			}
			m_physicalMaterials.PushBack( physicalMaterialIndex );
		}

	}

	~CTerrainTileCollisionJob()
	{
		for ( Uint32 i = 0; i < m_sectorHeightfield.Size(); ++i )
		{
			if ( m_sectorHeightfield[i] )
			{
				m_sectorHeightfield[i]->release();
			}
		}
	}

#ifndef NO_DEBUG_PAGES
	virtual const Char*	GetDebugName() const { return TXT("Generate TerrainTile collision"); }
	virtual Uint32		GetDebugColor() const override { return Color( 127, 192, 192 ).ToUint32(); }
#endif


	const SPhysicalMaterial* GetSingleMaterial() const { return m_singleMaterial; }

	TDynArray< PxHeightFieldGeometry >& GetSectorGeometry() { return m_sectorGeometry; }

	Bool HasResultData() { return !m_sectorGeometry.Empty(); }

	virtual void Run() override
	{
		PC_SCOPE_PIX( CTerrainTileCollisionJob )

		const Int32 count = ( m_resolution + 1 ) * ( m_resolution + 1 );
		const Uint32 stridePhysData = (m_resolution + 1);

		// Get BufferHandle for each of the buffers we're going to want. In case the tile unloads something while this job is still running,
		// need to be sure that the data will remain.
		BufferHandle tileHMTileHandle = m_tile->GetLevelSyncHMBufferHandle( 0 );
		BufferHandle tileHMnextRowTileHandle = m_nextRowTile->GetLevelSyncHMBufferHandle( 0 );
		BufferHandle tileHMnextColTileHandle = m_nextColTile->GetLevelSyncHMBufferHandle( 0 );
		BufferHandle tileHMnextRowColTileHandle = m_nextRowColTile->GetLevelSyncHMBufferHandle( 0 );
		
		BufferHandle tileCMTileHandle = m_tile->GetLevelSyncCMBufferHandle( 0 );
		BufferHandle tileCMnextRowTileHandle = m_nextRowTile->GetLevelSyncCMBufferHandle( 0 );
		BufferHandle tileCMnextColTileHandle = m_nextColTile->GetLevelSyncCMBufferHandle( 0 );
		BufferHandle tileCMnextRowColTileHandle = m_nextRowColTile->GetLevelSyncCMBufferHandle( 0 );


		const Uint16* tileHMTile = tileHMTileHandle ? (const Uint16*)tileHMTileHandle->GetData() : nullptr;

		const Uint16* tileHMnextRowTile = tileHMnextRowTileHandle ? (const Uint16*)tileHMnextRowTileHandle->GetData() : nullptr;
		if(!tileHMnextRowTile)
		{
			tileHMnextRowTile = tileHMTile;
		}

		const Uint16* tileHMnextColTile = tileHMnextColTileHandle ? (const Uint16*)tileHMnextColTileHandle->GetData() : nullptr;
		if(!tileHMnextColTile)
		{
			tileHMnextColTile = tileHMTile;
		}

		const Uint16* tileHMnextRowColTile = tileHMnextRowColTileHandle ? (const Uint16*)tileHMnextRowColTileHandle->GetData() : nullptr;
		if(!tileHMnextRowColTile)
		{
			tileHMnextRowColTile = tileHMTile;
		}


		const TControlMapType* tileCMTile = tileCMTileHandle ? (const TControlMapType*)tileCMTileHandle->GetData() : nullptr;

		const TControlMapType* tileCMnextRowTile = tileCMnextRowTileHandle ? (const TControlMapType*)tileCMnextRowTileHandle->GetData() : nullptr;
		if(!tileCMnextRowTile)
		{
			tileCMnextRowTile = tileCMTile;
		}

		const TControlMapType* tileCMnextColTile = tileCMnextColTileHandle ? (const TControlMapType*)tileCMnextColTileHandle->GetData() : nullptr;
		if(!tileCMnextColTile)
		{
			tileCMnextColTile = tileCMTile;
		}

		const TControlMapType* tileCMnextRowColTile = tileCMnextRowColTileHandle ? (const TControlMapType*)tileCMnextRowColTileHandle->GetData() : nullptr;
		if(!tileCMnextRowColTile)
		{
			tileCMnextRowColTile = tileCMTile;
		}
		//if( !tileHMTile || !tileHMnextRowTile || !tileHMnextColTile || !tileHMnextRowColTile || !tileCMTile || !tileCMnextRowTile || !tileCMnextColTile || !tileCMnextRowColTile ) return;
		const Bool sameRowCol = Bool(tileHMnextRowColTile == tileHMTile);
		const Bool sameRow = Bool(tileHMnextRowTile == tileHMTile);
		const Bool sameCol = Bool(tileHMnextColTile == tileHMTile);

		TDynArray< PxHeightFieldSample > physData( count );

		{
			CTerrainTile::GenerateCollisionHeightMapPass(stridePhysData, sameRowCol, tileHMnextRowColTile, tileCMnextRowColTile, sameRow, tileHMnextRowTile, tileCMnextRowTile, sameCol, tileHMnextColTile, tileCMnextColTile, tileHMTile, tileCMTile, physData, m_resolution);
		}

		// heightfield description
		PxHeightFieldDesc desc;
		{
			desc.nbColumns = m_resolution + 1;
			desc.nbRows = m_resolution + 1;
			desc.format = PxHeightFieldFormat::eS16_TM;
			desc.flags = PxHeightFieldFlag::eNO_BOUNDARY_EDGES; // no "bump" on tile edge
			desc.thickness = GGame->GetGameplayConfig().m_physicsTerrainThickness;
			desc.samples.data = physData.Data();
			desc.samples.stride = sizeof( PxHeightFieldSample );
		}

		// create the geometry object
		PxHeightField* heightfield = GPhysXEngine->GetPxPhysics()->createHeightField( desc );
		ASSERT( heightfield );

		if( !heightfield ) return;

		float multipler = ( ( m_boxSize.X / (float)m_resolution ) + ( m_boxSize.Y / (float)m_resolution ) ) / 2;
		{		 
			CTerrainTile::GenerateCollisionsMaterialPass(stridePhysData, sameRowCol, tileCMnextRowColTile, sameRow, tileCMnextRowTile, sameCol, tileCMnextColTile, tileCMTile,physData, count, heightfield, m_physicalMaterials, m_textureParameters, m_resolution);
			
			heightfield->release();
			
			if ( IsCancelled() )
			{
				return;
			}

		}

		//TO REFACTOR

		Box2 box;
		{
			PC_SCOPE_PHYSICS( CTerrainTile GenerateCollision TileWrapper );

			m_sectorGeometry.Resize( 64 );
			m_sectorHeightfield.Resize( 64 );

			Int32 sectorSize = m_resolution / 8;
			for( Uint8 y = 0; y != 8 ; ++y )
			{
				for( Uint8 x = 0; x != 8; ++x )
				{

					if ( IsCancelled() )
					{
						return;
					}

					TDynArray< PxHeightFieldSample > sectorPhysData( ( sectorSize + 1 ) * ( sectorSize + 1 ) );

					for( Int32 i = 0; i != sectorSize + 1 ; ++i )
					{
						Int32 startIndex = x * (sectorSize ) + y * (( sectorSize ) * stridePhysData) + i * (stridePhysData);
						Uint32 size = ( sectorSize + 1 );

						Red::MemoryCopy( &sectorPhysData[ i * ( sectorSize + 1 ) ], &physData[ startIndex ], sizeof( PxHeightFieldSample ) * size );
					}

					PxHeightFieldDesc sectorDesc;
					{
						sectorDesc.nbColumns = sectorSize + 1;
						sectorDesc.nbRows = sectorSize + 1;
						sectorDesc.format = PxHeightFieldFormat::eS16_TM;
						sectorDesc.flags = PxHeightFieldFlag::eNO_BOUNDARY_EDGES; // no "bump" on tile edge
						sectorDesc.thickness = GGame->GetGameplayConfig().m_physicsTerrainThickness;
						sectorDesc.samples.data = sectorPhysData.Data();
						sectorDesc.samples.stride = sizeof( PxHeightFieldSample );
					}

					m_sectorHeightfield[x + y*8] = GPhysXEngine->GetPxPhysics()->createHeightField( sectorDesc );

					const Vector sectorTexelSize( m_boxSize.X / m_resolution, m_boxSize.Y / m_resolution, ( m_boxSize.Z > 0.0f ? m_boxSize.Z / 65535.f : 1.0f ) );

					// Size of single heightmap texel in world space
					PxHeightFieldGeometry* heightFieldGeometry = &( m_sectorGeometry[x + y*8] );
					heightFieldGeometry = ::new(heightFieldGeometry) PxHeightFieldGeometry( m_sectorHeightfield[x + y*8], PxMeshGeometryFlags(), sectorTexelSize.Z, sectorTexelSize.X, sectorTexelSize.Y );
				}
			}
		}
	}

};

#endif // USE_PHYSX


void CTerrainTile::GenerateCollision( CClipMap* clipMap, CWorld* world, const Box& area, const TDynArray< CName >& textureNames, TDynArray< STerrainTextureParameters >& textureParameters, CTerrainTile* nextRowTile, CTerrainTile* nextColTile, CTerrainTile* nextRowColTile )
{

#ifndef RED_FINAL_BUILD
	if( SPhysicsSettings::m_dontCreateHeightmap )
	{
		return;
	}
#endif
#ifdef USE_PHYSX

	if ( !GGame->IsActive()
#ifndef NO_EDITOR	
		&& !GGame->GetActiveWorld()
#endif		
		)
	{
		return;
	}
	if( !nextRowTile) 
	{
		nextRowTile = this;
	}
	if( !nextColTile) 
	{
		nextColTile = this;
	}
	if( !nextRowColTile) 
	{
		nextRowColTile = this;
	}

	if ( !IsCollisionEnabled() )
	{
		return;
	}

	const SPhysicalMaterial* singleMaterial = nullptr;

	CPhysicsWorld* physicsWorld = nullptr;
	world->GetPhysicsWorld( physicsWorld );
	Box2 box = Box2( ( area.Min ).AsVector2(), area.Max.AsVector2() );
	const Vector boxSize = area.CalcSize();

#ifndef NO_EDITOR
	// If we're already running a generation job, just mark that we want another one.
	if ( m_collisionGenJob != nullptr && m_collisionGenJob->IsRunning() )
	{
		m_collisionGenRequested = true;
		return;
	}

	if ( m_collisionGenJob == nullptr )
	{
		// Start new generation job.
		m_collisionGenJob = new ( CTask::Root ) CTerrainTileCollisionJob( this, nextRowTile, nextColTile, nextRowColTile, m_resolution, area.CalcSize(), textureNames, textureParameters );

		// While running in-game, we need synchronous collision generation (avoid things falling through the world while we wait.
		if ( !SPhysicsSettings::m_doAsyncTerrainCollisionInEditor || ( GGame && GGame->IsActive() ) )
		{
			m_collisionGenJob->Run();
			m_collisionGenJob->MarkFinished();
		}
		else
		{
			GTaskManager->Issue( *m_collisionGenJob );
		}
		m_collisionGenRequested = false;
	}
	
	if ( m_collisionGenJob->IsCancelled() )
	{
		// If it was cancelled, just discard the results.
		SAFE_RELEASE( m_collisionGenJob );
		return;
	}

	if( !m_collisionGenJob->IsFinished() ) return;
	singleMaterial = m_collisionGenJob->GetSingleMaterial();

	if( m_collisionGenJob && !m_collisionGenJob->HasResultData() )
	{
		return;
	}

	if( m_collisionGenJob->GetSectorGeometry().Empty() ) return;

#else
	// Get physics tile for this terrain tile.

	CompiledCollisionPtr compiledCollision;

	String depotPath = GetDepotPath();
	Red::System::DateTime fileTime = GetFileTime();

	Box2 bounds = Box2::ZERO;

	if( !GCollisionCache->HasCollision_Sync( depotPath, fileTime ) )
	{
		GCollisionCache->Compile_Sync( compiledCollision, this, depotPath, fileTime, clipMap );
	}
	else if( GCollisionCache->FindCompiled( compiledCollision, depotPath, fileTime, &bounds ) != ICollisionCache::eResult_Valid )
	{
		physicsWorld->MarkSectorAsUnready( area.CalcCenter() );
		return;
	}

	if( compiledCollision.Get() )
	{
		if( compiledCollision->GetGeometries().Size() )
		{
			singleMaterial = GPhysicEngine->GetMaterial( compiledCollision->GetGeometries()[ 0 ].m_physicalSingleMaterial );
		}
	}
	else
	{
		return;
	}
#endif


	// Discard old physics bodies
	if( m_wrapper )
	{
		m_wrapper->Dispose( this );
		m_wrapper = nullptr;
	}


	if( !m_wrapper )
	{
		CPhysicsTileWrapper* terrain = physicsWorld->GetWrappersPool< CPhysicsTileWrapper, SWrapperContext >()->GetWrapperFirst();
		while( terrain )
		{
			CPhysicsWorld* world = nullptr;
			terrain->GetPhysicsWorld( world );

			if( terrain->GetBounds2D() == box && world == physicsWorld )
			{
				m_wrapper = terrain;
				break;
			}
			terrain = physicsWorld->GetWrappersPool< CPhysicsTileWrapper, SWrapperContext >()->GetWrapperNext( terrain );
		}
	}
	if( !m_wrapper )
	{
		m_wrapper = physicsWorld->GetWrappersPool< CPhysicsTileWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( nullptr ), physicsWorld, box );
	}

	Vector2 tileDimensions = area.Max - area.Min;
	Vector2 sectorsDimensions = tileDimensions / 8;

	Int32 sectorSize = m_resolution / 8;
	for( Uint8 y = 0; y != 8 ; ++y )
	{
		for( Uint8 x = 0; x != 8; ++x )
		{
			Vector sectorPosition( area.Min );
			sectorPosition.Z = area.Min.Z + boxSize.Z / 2.0f + GGame->GetGameplayConfig().m_physicsTerrainAdditionalElevation; 
			sectorPosition.X += sectorsDimensions.X * x;
			sectorPosition.Y += sectorsDimensions.Y * y;

#ifndef NO_EDITOR
			PxHeightFieldGeometry* compiled = &m_collisionGenJob->GetSectorGeometry()[ x + y*8 ];
#else
			const SCachedGeometry& geometry = compiledCollision->GetGeometries()[ x + y*8 ];
			PxHeightFieldGeometry* compiled = ( PxHeightFieldGeometry* ) geometry.GetGeometry();
#endif
			Uint32 aInd = m_wrapper->AddTerrainBody( CPhysicsWrapperParentResourceProvider( this ), sectorPosition, singleMaterial, compiled );

		}
	}

	SAFE_RELEASE( m_collisionGenJob );

#endif // USE_PHYSX

}


Bool CTerrainTile::IsCollisionGenerationPending() const
{
	return m_collisionGenRequested 
#ifdef USE_PHYSX
		|| ( m_collisionGenJob != nullptr && m_collisionGenJob->IsFinished() )
#endif
		;
}


void CTerrainTile::GetTerrainGeometryVertexes( const Box& extents, const CClipMap* clipMap, Vector& tileCorner, Int32& minX, Int32& minY, Int32& maxX, Int32& maxY, TDynArray< Vector >& outVertexes )
{
	const Int32 res = GetResolution();

	Float terrainSize = clipMap->GetTerrainSize();
	Float tileSize = clipMap->GetTileSize();
	Float quadSize = tileSize / Float( res );

	Vector terrainCorner( (Float)-terrainSize/2.0f, (Float)-terrainSize/2.0f, 0.0f );

	// Calculate relative position
	const Float relativeMinX = Clamp<Float>( ( ( extents.Min.X - tileCorner.X ) - 0.5f ) / tileSize, 0.0f, 1.0f );
	const Float relativeMaxX = Clamp<Float>( ( ( extents.Max.X - tileCorner.X ) - 0.5f ) / tileSize, 0.0f, 1.0f );
	const Float relativeMinY = Clamp<Float>( ( ( extents.Min.Y - tileCorner.Y ) - 0.5f ) / tileSize, 0.0f, 1.0f );
	const Float relativeMaxY = Clamp<Float>( ( ( extents.Max.Y - tileCorner.Y ) - 0.5f ) / tileSize, 0.0f, 1.0f );

	// Convert to ints
	minX = Clamp( Int32( relativeMinX * Float( res - 1 ) + 0 ), 0, (Int32)res );
	maxX = Clamp( Int32( relativeMaxX * Float( res - 1 ) + 1 ), 0, (Int32)res );
	minY = Clamp( Int32( relativeMinY * Float( res - 1 ) + 0 ), 0, (Int32)res );
	maxY = Clamp( Int32( relativeMaxY * Float( res - 1 ) + 1 ), 0, (Int32)res );

	Int32 spanX = maxY - minY + 1;
	Int32 spanY = maxX - minX + 1;

	// prepare the tables of data to use in the loop
	// tebles are here to avoid using tons of ifs inside the loop
	const Uint16* tileHM = GetLevelSyncHM( 0 );
	const TControlMapType* tileCM = GetLevelSyncCM( 0 );

	Uint32 vertsCount = spanX * spanY;
	outVertexes.Resize( vertsCount );

	Uint32 vertsIt = 0;
	for ( Int32 y = minY; y <= maxY; ++y )
	{ 
		Float vertY = tileCorner.Y + Float( y ) * quadSize;
		for ( Int32 x = minX; x <= maxX; ++x )
		{
			Float vertX = tileCorner.X + Float( x ) * quadSize;

			Int16 hmVal = tileHM[ Min(y,res-1) * res + Min(x,res-1) ];
			Int16 cmVal = tileCM[ Min(y,res-1) * res + Min(x,res-1) ];

			// fill the physX sample with calculated value
			Float height;
			if ( cmVal == 0 )
			{
				height = FLT_MAX;
			}
			else
			{
				height = clipMap->TexelHeightToHeight( hmVal );
			}
			outVertexes[ vertsIt++ ].Set3(
				vertX,
				vertY,
				height );
		}
	}
	ASSERT( vertsIt == vertsCount );
}

void CTerrainTile::InvalidateCollision()
{
	if ( !m_wrapper )
	{
		return;
	}

	m_wrapper->Dispose( this );
	m_wrapper = 0;

#ifdef USE_PHYSX
	if ( m_collisionGenJob != nullptr )
	{
		m_collisionGenJob->TryCancel();
		SAFE_RELEASE( m_collisionGenJob );
	}
#endif
	m_collisionGenRequested = false;
}

Bool CTerrainTile::IsQuadTriangulatedNW2SE( Int32 x, Int32 y )
{
	// NOTICE: as x = 0, y = 0 i take lower left corner of the tile
	Int32 val = (x >> 5) ^ (y >> 5);
	return !(val & 1);
}

Bool CTerrainTile::IsCollisionGenerated() const
{
	return m_wrapper != 0;
}

CompiledCollisionPtr CTerrainTile::CompileCollision( CObject* parent ) const
{
#ifndef USE_PHYSX
	return CompiledCollisionPtr();
#else
	if ( !IsCollisionEnabled() )
	{
		return CompiledCollisionPtr();
	}

	CClipMap* clipMap = Cast< CClipMap >( parent );
	if( !clipMap ) return CompiledCollisionPtr();

	Int32 col = -1;
	Int32 row = -1;
	String name =  GetFile()->GetFileName();
	if( name.Empty() ) return CompiledCollisionPtr();
	size_t pos = 0;
	name.FindSubstring( TXT( "tile_" ), pos );
	String temp = name.RightString( name.Size() - pos - 6 );
	temp.FindSubstring( TXT( "_x_" ), pos );

	FromString( temp.LeftString( pos ), col );
	
	temp = temp.RightString( temp.Size() - pos - 4 );

	temp.FindSubstring( TXT( "_res" ), pos );

	FromString( temp.LeftString( pos ), row );


	Box collisionTileBB = clipMap->GetBoxForTile( row, col, 0.f );

	collisionTileBB.Min.Z = clipMap->GetLowestElevation();
	collisionTileBB.Max.Z = clipMap->GetHeighestElevation();

	CTerrainTile* tile = clipMap->GetTile( row, col );
	if( !tile )
	{
		return CompiledCollisionPtr();
	}

	CTerrainTile* nextRowTile = NULL;
	if ( col < (Int32)clipMap->GetNumTilesPerEdge() - 1 )
	{
		nextRowTile = clipMap->GetTile( row, col + 1);
	}

	if( !nextRowTile )
	{
		nextRowTile = tile;
	}

	CTerrainTile* nextColTile = NULL;
	if ( row < (Int32)clipMap->GetNumTilesPerEdge() - 1 )
	{
		nextColTile = clipMap->GetTile( row + 1, col );
	}

	if( !nextColTile )
	{
		nextColTile = tile;
	}

	CTerrainTile* nextRowColTile = NULL;
	if ( row < (Int32)clipMap->GetNumTilesPerEdge() - 1 && col < (Int32)clipMap->GetNumTilesPerEdge() - 1 )
	{
		nextRowColTile = clipMap->GetTile( row + 1, col + 1);
	}

	if( !nextRowColTile )
	{
		nextRowColTile = tile;
	}

	Uint32 mipMapStackTileRes = clipMap->GetTilesMaxResolution() >> clipMap->GetNumClipmapStackLevels();

	TDynArray< char > physicalMaterials;

#ifndef NO_HEIGHTMAP_EDIT
	TDynArray< CName > textureNames;
	if ( clipMap->GetMaterial() )
	{
		THandle< CTextureArray > texture;
		if ( clipMap->GetMaterial()->ReadParameter( CNAME(diffuse), texture ) )
		{
			if( texture.Get() )
			{
				texture.Get()->GetTextureNames( textureNames );
			}
		}
	}

	for( Uint32 i = 0; i != textureNames.Size(); ++i )
	{
		char physicalMaterialIndex = GPhysicEngine->GetTexturePhysicalMaterialIndex( textureNames[ i ] );
		if( physicalMaterialIndex == -1 )
		{
			DATA_HALT( DES_Major, tile, TXT("Terrain collision generation"), TXT("Texture ( %s ) used to paint terrain isn't mapped to any physical material in terrain_textures_physical_materials.csv file"), textureNames[ i ].AsString().AsChar() );
		}
		//hack reject one texture
		size_t pos = 0;
		String name = textureNames[ i ].AsString();
		if( name.FindSubstring( TXT( "sand_rim.xbm" ), pos ) )
		{
			physicalMaterialIndex = -1;
		}
		//hack reject one texture
		physicalMaterials.PushBack( physicalMaterialIndex );
	}
#endif
	const Int32 count = ( m_resolution + 1 ) * ( m_resolution + 1 );
	const Uint32 stridePhysData = (m_resolution + 1);

	const Uint16* tileHMTile = tile->GetLevelSyncHM( 0 );
	const Uint16* tileHMnextRowTile = nextRowTile->GetLevelSyncHM( 0 );
	const Uint16* tileHMnextColTile = nextColTile->GetLevelSyncHM( 0 );
	const Uint16* tileHMnextRowColTile = nextRowColTile->GetLevelSyncHM( 0 );

	const TControlMapType* tileCMTile = tile->GetLevelSyncCM( 0 );
	const TControlMapType* tileCMnextRowTile = nextRowTile->GetLevelSyncCM( 0 );
	const TControlMapType* tileCMnextColTile = nextColTile->GetLevelSyncCM( 0 );
	const TControlMapType* tileCMnextRowColTile = nextRowColTile->GetLevelSyncCM( 0 );

	const Bool sameRowCol = Bool(tileHMnextRowColTile == tileHMTile);
	const Bool sameRow = Bool(tileHMnextRowTile == tileHMTile);
	const Bool sameCol = Bool(tileHMnextColTile == tileHMTile);

	CompiledCollisionPtr compiled( new CCompiledCollision() );

#ifdef USE_PHYSX
	TDynArray< PxHeightFieldSample > physData( count );

	{
		GenerateCollisionHeightMapPass(stridePhysData, sameRowCol, tileHMnextRowColTile, tileCMnextRowColTile, sameRow, tileHMnextRowTile, tileCMnextRowTile, sameCol, tileHMnextColTile, tileCMnextColTile, tileHMTile, tileCMTile, physData, m_resolution);
	}

	// heightfield description
	PxHeightFieldDesc desc;
	{
		desc.nbColumns = m_resolution + 1;
		desc.nbRows = m_resolution + 1;
		desc.format = PxHeightFieldFormat::eS16_TM;
		desc.flags = PxHeightFieldFlag::eNO_BOUNDARY_EDGES; // no "bump" on tile edge
		desc.thickness = GGame->GetGameplayConfig().m_physicsTerrainThickness;
		desc.samples.data = physData.Data();
		desc.samples.stride = sizeof( PxHeightFieldSample );
	}

	// create the geometry object
	PxHeightField* heightfield = GPhysXEngine->GetPxPhysics()->createHeightField( desc );
	ASSERT( heightfield );

	const SPhysicalMaterial* singleMaterial = nullptr;
	float multipler = ( ( collisionTileBB.CalcSize().X / (float)m_resolution ) + ( collisionTileBB.CalcSize().Y / (float)m_resolution ) ) / 2;
	{
#ifndef NO_HEIGHTMAP_EDIT
		singleMaterial = GenerateCollisionsMaterialPass(stridePhysData, sameRowCol, tileCMnextRowColTile, sameRow, tileCMnextRowTile, sameCol, tileCMnextColTile, tileCMTile,physData, count, heightfield, physicalMaterials, clipMap->GetTextureParams(), m_resolution);
#endif // !NO_HEIGHTMAP_EDIT

		heightfield->release();
	}

	PxCooking* cooking = GPhysXEngine->GetCooking();


	Box2 box;
	{
		PC_SCOPE_PHYSICS( CTerrainTile GenerateCollision TileWrapper );

		Int32 sectorSize = m_resolution / 8;
		for( Uint8 y = 0; y != 8 ; ++y )
		{
			for( Uint8 x = 0; x != 8; ++x )
			{

				TDynArray< PxHeightFieldSample > sectorPhysData( ( sectorSize + 1 ) * ( sectorSize + 1 ) );

				for( Int32 i = 0; i != sectorSize + 1 ; ++i )
				{
					Int32 startIndex = x * (sectorSize ) + y * (( sectorSize ) * stridePhysData) + i * (stridePhysData);
					Uint32 size = ( sectorSize + 1 );

					Red::MemoryCopy( &sectorPhysData[ i * ( sectorSize + 1 ) ], &physData[ startIndex ], sizeof( PxHeightFieldSample ) * size );
				}

				PxHeightFieldDesc sectorDesc;
				{
					sectorDesc.nbColumns = sectorSize + 1;
					sectorDesc.nbRows = sectorSize + 1;
					sectorDesc.format = PxHeightFieldFormat::eS16_TM;
					sectorDesc.flags = PxHeightFieldFlag::eNO_BOUNDARY_EDGES; // no "bump" on tile edge
					sectorDesc.thickness = GGame->GetGameplayConfig().m_physicsTerrainThickness;
					sectorDesc.samples.data = sectorPhysData.Data();
					sectorDesc.samples.stride = sizeof( PxHeightFieldSample );
				}

				const Vector sectorTexelSize( collisionTileBB.CalcSize().X / m_resolution, collisionTileBB.CalcSize().Y / m_resolution, ( collisionTileBB.CalcSize().Z > 0.0f ? collisionTileBB.CalcSize().Z / 65535.f : 1.0f ) );

				SCachedGeometry& geometry = compiled->InsertGeometry();
				geometry.m_geometryType = ( char ) PxGeometryType::eHEIGHTFIELD;
				geometry.m_densityScaler = 1;
				geometry.m_pose.SetIdentity();
				geometry.m_pose.SetScale33( Vector( sectorTexelSize.Z, sectorTexelSize.X, sectorTexelSize.Y ) );

				MemoryWriteBuffer buf;
				if ( cooking->cookHeightField( sectorDesc, buf ) )
				{
					MemoryReadBuffer saveBuffer(buf.data);
					geometry.AllocateCompiledData( buf.maxSize );
					saveBuffer.read(geometry.GetCompiledData(), buf.maxSize);
					if( singleMaterial )
					{
						geometry.m_physicalSingleMaterial = singleMaterial->m_name;
					}

					Vector scale = geometry.m_pose.GetScale33();
					MemoryReadBuffer mrb((PxU8*)geometry.GetCompiledData());
					PxHeightField* highfield = GPhysXEngine->GetPxPhysics()->createHeightField( mrb );
					geometry.SetGeometry( ( char ) PxGeometryType::eHEIGHTFIELD, PxHeightFieldGeometry( highfield, PxMeshGeometryFlags(), scale.X, scale.Y, scale.Z ) );
				}
				else
				{
					geometry.AllocateCompiledData( 0 );
				}
			}
		}
	}
#endif // USE_PHYSX

	return compiled;
#endif
}

// Get height map coordinates overlapping with world space bounding box (only x and y are considered)
Bool CTerrainTile::GetHeightMapExtents( const Box& extents, const SClipmapParameters& parameters, Int32& minX, Int32& maxX, Int32& minY, Int32& maxY ) const
{
	Vector terrainCorner( (Float)-parameters.terrainSize/2.0f, (Float)-parameters.terrainSize/2.0f, 0.0f );

	// Calculate relative position
	const Float relativeMinX = Clamp<Float>( ( ( extents.Min.X - terrainCorner.X ) - 0.5f ) / parameters.terrainSize, 0.0f, 1.0f );
	const Float relativeMaxX = Clamp<Float>( ( ( extents.Max.X - terrainCorner.X ) - 0.5f ) / parameters.terrainSize, 0.0f, 1.0f );
	const Float relativeMinY = Clamp<Float>( ( ( extents.Min.Y - terrainCorner.Y ) - 0.5f ) / parameters.terrainSize, 0.0f, 1.0f );
	const Float relativeMaxY = Clamp<Float>( ( ( extents.Max.Y - terrainCorner.Y ) - 0.5f ) / parameters.terrainSize, 0.0f, 1.0f );

	if ( relativeMinX < 0.0f || relativeMinX > 1.0f  || relativeMaxX < 0.0f || relativeMaxX > 1.0f || relativeMinY < 0.0f || relativeMinY > 1.0f  || relativeMaxY < 0.0f || relativeMaxY > 1.0f )
	{
		return false;
	}

	// Convert to ints
	minX = Clamp( Int32( relativeMinX * Float( m_resolution - 1 ) + 0 ), 0, (Int32)m_resolution - 1 );
	maxX = Clamp( Int32( relativeMaxX * Float( m_resolution - 1 ) + 1 ), 0, (Int32)m_resolution - 1 );
	minY = Clamp( Int32( relativeMinY * Float( m_resolution - 1 ) + 0 ), 0, (Int32)m_resolution - 1 );
	maxY = Clamp( Int32( relativeMaxY * Float( m_resolution - 1 ) + 1 ), 0, (Int32)m_resolution - 1 );

	return true;
}

void CTerrainTile::GetHeightFromHeightmap( const Uint16* tileTexels, const Vector2& position, Float tileSize, Uint32 tileRes, Float lowestElevation, Float heightRange, Float& height, Uint16* texelHeight /*=NULL*/ )
{
	Vector2 mipmapNormalizedPosition = position * ( Float(tileRes) / tileSize );
	mipmapNormalizedPosition.X = Clamp( mipmapNormalizedPosition.X, 0.f, Float( tileRes-1 ) );
	mipmapNormalizedPosition.Y = Clamp( mipmapNormalizedPosition.Y, 0.f, Float( tileRes-1 ) );

	Uint32 texelXMin = Clamp<Uint32>( Uint32( mipmapNormalizedPosition.X ), 0, tileRes - 1 );
	Uint32 texelYMin = Clamp<Uint32>( Uint32( mipmapNormalizedPosition.Y ), 0, tileRes - 1 );
	Uint32 texelXMax = texelXMin == tileRes - 1 ? tileRes - 1 : texelXMin+1;
	Uint32 texelYMax = texelYMin == tileRes - 1 ? tileRes - 1 : texelYMin+1;

	// Calculate heights
	Uint16 heightLL = tileTexels[ texelYMin * tileRes + texelXMin ];
	Uint16 heightHL = tileTexels[ texelYMin * tileRes + texelXMax ];
	Uint16 heightLH = tileTexels[ texelYMax * tileRes + texelXMin ];
	Uint16 heightHH = tileTexels[ texelYMax * tileRes + texelXMax ];

	// renormalize height and get the real value
	Float heightNormLL = lowestElevation + heightRange * ( (Float)heightLL / 65536.f );
	Float heightNormHL = lowestElevation + heightRange * ( (Float)heightHL / 65536.f );
	Float heightNormLH = lowestElevation + heightRange * ( (Float)heightLH / 65536.f );
	Float heightNormHH = lowestElevation + heightRange * ( (Float)heightHH / 65536.f );

	Float u = mipmapNormalizedPosition.X - Red::Math::MFloor( mipmapNormalizedPosition.X );
	Float v = mipmapNormalizedPosition.Y - Red::Math::MFloor( mipmapNormalizedPosition.Y );

	Float xlow = heightNormLL + (heightNormLH - heightNormLL) * v;
	Float xhigh = heightNormHL + (heightNormHH - heightNormHL) * v;
	height = xlow + (xhigh - xlow) * u;

	if ( texelHeight )
	{
		Uint16 xlowTexel = heightLL + (Uint16)( ( heightLH - heightLL ) * v );
		Uint16 xhighTexel = heightHL + (Uint16)( ( heightHH - heightHL ) * v );
		*texelHeight = xlowTexel + (Uint16)( ( xhighTexel - xlowTexel ) * u );
	}	
}

TControlMapType CTerrainTile::GetValueFromControlmap( const TControlMapType* tileTexels, const Vector2& position, Float tileSize, Uint32 tileRes )
{
	Vector2 mipmapNormalizedPosition = position * ( Float(tileRes) / tileSize );
	mipmapNormalizedPosition.X = Clamp( mipmapNormalizedPosition.X, 0.f, Float( tileRes-1 ) );
	mipmapNormalizedPosition.Y = Clamp( mipmapNormalizedPosition.Y, 0.f, Float( tileRes-1 ) );

	Uint32 texelXMin = Clamp<Uint32>( Uint32( mipmapNormalizedPosition.X ), 0, tileRes - 1 );
	Uint32 texelYMin = Clamp<Uint32>( Uint32( mipmapNormalizedPosition.Y ), 0, tileRes - 1 );

	return tileTexels[ texelYMin * tileRes + texelXMin ];
}


Bool CTerrainTile::GetHeightForLocalPosition( const CClipMap* clipMap, const Vector2& position, Float& height, Uint16* texelHeight /*=NULL*/ )
{
	Uint32 level = 0;
	if ( GetAnyLevelHM( level ) != nullptr )
	{
		RED_WARNING( m_mipmaps[level].GetLoadedBufferHandle( TBT_HeightMap ), "Tile mip level %u not loaded, but given from GetAnyLevel", level );
		// Call the sync version. We know that level is already loaded, so no worries about accidentally triggering a load.
		GetHeightForLocalPositionSync( clipMap, position, level, height, texelHeight );
		return true;
	}
	return false;
}

void CTerrainTile::GetHeightForLocalPositionSync( const CClipMap* clipMap, const Vector2& position, Uint32 level, Float& height, Uint16* texelHeight /*= NULL*/ )
{
	const Uint16* tileTexels = GetLevelSyncHM( level );

	Float heighestElevation = clipMap->GetHeighestElevation();
	Float lowestElevation = clipMap->GetLowestElevation();

	if ( tileTexels )
	{
		Float tileSize = clipMap->GetTileSize();
		Uint32 tileMipMapRes = clipMap->GetTilesMaxResolution() >> level;

		GetHeightFromHeightmap( tileTexels, position, tileSize, tileMipMapRes, lowestElevation, heighestElevation - lowestElevation, height, texelHeight );
	}
	else
	{
		// return default values
		height = (heighestElevation + lowestElevation) * 0.5f;
		if ( texelHeight )
		{
			*texelHeight = 32767; // 65535 (max of Uint16) / 2
		}
	}
}


TControlMapType CTerrainTile::GetControlmapValueForLocalPosition( const CClipMap* clipMap, const Vector2& position )
{
	Uint32 level = 0;
	if ( GetAnyLevelCM( level ) != nullptr )
	{
		RED_WARNING( m_mipmaps[level].GetLoadedBufferHandle( TBT_ControlMap ), "Tile mip level %u not loaded, but given from GetAnyLevel", level );

		// Call the sync version. We know that level is already loaded, so no worries about accidentally triggering a load.
		return GetControlmapValueForLocalPositionSync( clipMap, position, level );
	}
	return 0;
}

TControlMapType CTerrainTile::GetControlmapValueForLocalPositionSync( const CClipMap* clipMap, const Vector2& position, Uint32 level )
{
	const TControlMapType* tileTexels = GetLevelSyncCM( level );
	if ( tileTexels != nullptr )
	{
		Float tileSize = clipMap->GetTileSize();
		Uint32 tileMipMapRes = clipMap->GetTilesMaxResolution() >> level;
		return GetValueFromControlmap( tileTexels, position, tileSize, tileMipMapRes );
	}
	return 0;
}


TColorMapType CTerrainTile::GetColormapValueForLocalPosition( const CClipMap* clipMap, const Vector2& position )
{
	Uint32 level = 0;
	if ( GetAnyLevelColorMap( level ) != nullptr )
	{
		RED_WARNING( m_mipmaps[level].GetLoadedBufferHandle( TBT_ColorMap ), "Tile mip level %u not loaded, but given from GetAnyLevel", level );

		// Call the sync version. We know that level is already loaded, so no worries about accidentally triggering a load.
		return GetColormapValueForLocalPositionSync( clipMap, position, level );
	}
	return 0xFFFFFFFF;
}

TColorMapType CTerrainTile::GetColormapValueForLocalPositionSync( const CClipMap* clipMap, const Vector2& position, Uint32 level )
{
	level = Max( level, m_colormapStartingMip );
	const void* tileTexels = GetLevelSyncColorMap( level );

	if ( !tileTexels )
	{
		// just return white
		return 0xFFFFFFFF;
	}

	Uint32 resolution = m_mipmaps[level].m_resolution;

	Vector2 mipmapNormalizedPosition = position / clipMap->GetTileSize() * (Float)resolution;
	mipmapNormalizedPosition.X = Clamp( mipmapNormalizedPosition.X, 0.f, Float( resolution - 1 ) );
	mipmapNormalizedPosition.Y = Clamp( mipmapNormalizedPosition.Y, 0.f, Float( resolution - 1 ) );

	Uint32 texelXMin = Clamp<Uint32>( Uint32( mipmapNormalizedPosition.X ), 0, resolution - 1 );
	Uint32 texelYMin = Clamp<Uint32>( Uint32( mipmapNormalizedPosition.Y ), 0, resolution - 1 );

	if ( IsCooked() )
	{
		Uint32 blockX		= TerrainUtils::ColorMapTexelToBlock( texelXMin );
		Uint32 blockY		= TerrainUtils::ColorMapTexelToBlock( texelYMin );
		Uint32 subTexelX	= TerrainUtils::ColorMapTexelToSubTexel( texelXMin );
		Uint32 subTexelY	= TerrainUtils::ColorMapTexelToSubTexel( texelYMin );

		Uint32 blockRes = TerrainUtils::ColorMapTexelToBlock( resolution );

		Uint32 blockIdx = blockY * blockRes + blockX;

		const TColorMapRawType* bcTileTexels = static_cast< const TColorMapRawType* >( tileTexels );

		return TerrainUtils::DecodeColorMap( bcTileTexels[ blockIdx ], subTexelX, subTexelY );
	}
	else
	{
		const TColorMapType* rgbTileTexels = static_cast< const TColorMapType* >( tileTexels );
		return rgbTileTexels[ texelYMin * resolution + texelXMin ];
	}
}


Float CTerrainTile::GetMipCountdownPercent( Uint32 mipmapLevel ) const
{
	if ( mipmapLevel >= m_mipmaps.Size() )
	{
		return 0.0f;
	}

	const Float timeout = m_mipmaps[mipmapLevel].GetTimeout();
	const Float time = m_mipmapTimes[mipmapLevel];
	return Clamp( ( timeout - time ) / timeout, 0.0f, 1.0f );
}

#ifndef NO_HEIGHTMAP_EDIT
void CTerrainTile::UpdateHeightRange()
{
	m_minHeightValue = 65535;
	m_maxHeightValue = 0;
	BufferHandle mip0Height = m_mipmaps[0].AcquireBufferHandleSyncNoTrack( TBT_HeightMap );
	if ( mip0Height && mip0Height->GetData() )
	{
		const Uint16* hm = (const Uint16*)mip0Height->GetData();
		for (Uint32 i = 0; i < m_resolution*m_resolution; ++i )
		{
			m_maxHeightValue = Max( m_maxHeightValue, hm[i] );
			m_minHeightValue = Min( m_minHeightValue, hm[i] );
		}
	}
}
#endif

//////////////////////////////////////////////////////////////////////////



Red::Threads::CAtomic< Int32 > STerrainTileMipMap::m_hackyTerrainVersionCounter( 0 );

void STerrainTileMipMap::HACK_BumpTerrainVersionCounter()
{
	m_hackyTerrainVersionCounter.Increment();
	Red::Threads::SleepOnCurrentThread( 1 );
}



void STerrainTileMipMap::ReallocateBuffers( Uint32 buffers, Uint32 resolution, Bool clearOthers /*= true*/ )
{
	if ( buffers & TBT_HeightMap )
	{
		m_heightMapDataBuffer.ReallocateBuffer( resolution*resolution*sizeof(Uint16) );
	}
	else if ( clearOthers )
	{
		m_heightMapDataBuffer.ReallocateBuffer( 0 );
	}
	if ( buffers & TBT_ControlMap )
	{
		m_controlMapDataBuffer.ReallocateBuffer( resolution*resolution*sizeof(TControlMapType) );
	}
	else if ( clearOthers )
	{
		m_controlMapDataBuffer.ReallocateBuffer( 0 );
	}
	if ( buffers & TBT_ColorMap )
	{
		m_colorMapDataBuffer.ReallocateBuffer( resolution * resolution * sizeof(TColorMapType));
	}
	else if ( clearOthers )
	{
		m_colorMapDataBuffer.ReallocateBuffer( 0 );
	}
	m_resolution = resolution;
}


void STerrainTileMipMap::SerializeBuffers( IFile& file, Uint32 tileFileVersion )
{
	if( tileFileVersion < TERRAIN_VERSION_USING_DEFERRED_BUFFER )
	{
		SerializeDeferredDataBufferAsLatentDataBufferData( file, m_heightMapDataBuffer );
		SerializeDeferredDataBufferAsLatentDataBufferData( file, m_controlMapDataBuffer );
		SerializeDeferredDataBufferAsLatentDataBufferData( file, m_colorMapDataBuffer );
	}
	else
	{
		file << m_heightMapDataBuffer;
		file << m_controlMapDataBuffer;
		file << m_colorMapDataBuffer;
	}
}


BufferHandle STerrainTileMipMap::AcquireBufferForWriting( ETerrainBufferType buffer )
{
	BufferHandle handle;

	switch ( buffer )
	{
	case TBT_HeightMap:
		m_heightMapHandle = m_heightMapDataBuffer.AcquireBufferHandleForWritingSync();
		handle = m_heightMapHandle;
		break;
	case TBT_ControlMap:
		m_controlMapHandle = m_controlMapDataBuffer.AcquireBufferHandleForWritingSync();
		handle = m_controlMapHandle;
		break;
	case TBT_ColorMap:
		m_colorMapHandle = m_colorMapDataBuffer.AcquireBufferHandleForWritingSync();
		handle = m_colorMapHandle;
		break;
	default:
		RED_HALT( "Invalid buffer type: %d", buffer );
	}

	return handle;
}

BufferHandle STerrainTileMipMap::AcquireBufferHandleSync( ETerrainBufferType buffer )
{
	BufferHandle handle;

	switch ( buffer )
	{
	case TBT_HeightMap:
		if ( !m_heightMapHandle )
		{
			m_heightMapHandle = m_heightMapDataBuffer.AcquireBufferHandleSync();
		}
		handle = m_heightMapHandle;
		break;
	case TBT_ControlMap:
		if ( !m_controlMapHandle )
		{
			m_controlMapHandle = m_controlMapDataBuffer.AcquireBufferHandleSync();
		}
		handle = m_controlMapHandle;
		break;
	case TBT_ColorMap:
		if ( !m_colorMapHandle )
		{
			m_colorMapHandle = m_colorMapDataBuffer.AcquireBufferHandleSync();
		}
		handle = m_colorMapHandle;
		break;
	default:
		RED_HALT( "Invalid buffer type: %d", buffer );
	}

	return handle;
}

BufferHandle STerrainTileMipMap::GetLoadedBufferHandle( ETerrainBufferType buffer )
{
	BufferHandle handle;

	switch ( buffer )
	{
	case TBT_HeightMap:
		handle = m_heightMapHandle;
		break;
	case TBT_ControlMap:
		handle = m_controlMapHandle;
		break;
	case TBT_ColorMap:
		handle = m_colorMapHandle;
		break;
	default:
		RED_HALT( "Invalid buffer type: %d", buffer );
	}

	return handle;
}

BufferHandle STerrainTileMipMap::AcquireBufferHandleSyncNoTrack( ETerrainBufferType buffer ) const
{
	switch ( buffer )
	{
	case TBT_HeightMap:		return const_cast< DeferredDataBuffer& >( m_heightMapDataBuffer ).AcquireBufferHandleSync();
	case TBT_ControlMap:	return const_cast< DeferredDataBuffer& >( m_controlMapDataBuffer ).AcquireBufferHandleSync();
	case TBT_ColorMap:		return const_cast< DeferredDataBuffer& >( m_colorMapDataBuffer ).AcquireBufferHandleSync();
	}

	RED_HALT( "Invalid buffer type: %d", buffer );
	return BufferHandle();
}



Uint32 STerrainTileMipMap::GetBufferDataSize( ETerrainBufferType buffer ) const
{
	switch ( buffer )
	{
	case TBT_HeightMap:		return m_heightMapDataBuffer.GetSize();
	case TBT_ControlMap:	return m_controlMapDataBuffer.GetSize();
	case TBT_ColorMap:		return m_colorMapDataBuffer.GetSize();
	}

	RED_HALT( "Invalid buffer type: %d", buffer );
	return 0;
}

Bool STerrainTileMipMap::HasBufferDataLoaded( ETerrainBufferType buffer ) const
{
	switch ( buffer )
	{
	case TBT_HeightMap:		return m_heightMapHandle;
	case TBT_ControlMap:	return m_controlMapHandle;
	case TBT_ColorMap:		return m_colorMapHandle;
	}

	RED_HALT( "Invalid buffer type: %d", buffer );
	return false;
}

void STerrainTileMipMap::BindCookedData( BufferHandle height, BufferHandle control, BufferHandle color )
{
	m_wasBoundedWithCookedData = true;
	m_heightMapHandle = height;
	m_colorMapHandle = color;
	m_controlMapHandle = control;
}

void STerrainTileMipMap::ExtractCookedData( const Bool isCooked, BufferHandle& outCookedHeight, BufferHandle& outCookedColor, BufferHandle& outCookedControl ) const
{
	if ( isCooked )
	{
		outCookedColor = AcquireBufferHandleSyncNoTrack(TBT_ColorMap);
	}
	else
	{
		// cook on the fly
		BufferHandle rawHandle = AcquireBufferHandleSyncNoTrack( TBT_ColorMap );
		if ( rawHandle )
		{
			const TColorMapType* uncompressedData = static_cast< TColorMapType* >( rawHandle->GetData() );
			const Uint32 compressedSize = TerrainUtils::CalcColorMapSize( m_resolution, m_resolution );

			DeferredDataBuffer resultBuffer;
			resultBuffer.ReallocateBuffer(compressedSize);
			outCookedColor = resultBuffer.AcquireBufferHandleForWritingSync();

			TColorMapRawType* compressedData = static_cast< TColorMapRawType* >( outCookedColor->GetData() );
			TerrainUtils::CompressColor( uncompressedData, compressedData, m_resolution, m_resolution );
		}
	}

	outCookedHeight = AcquireBufferHandleSyncNoTrack(TBT_HeightMap);
	outCookedControl = AcquireBufferHandleSyncNoTrack(TBT_ControlMap);
}

void STerrainTileMipMap::RequestAsyncLoad()
{
	m_wasRequested = true;

	// As long as we keep requesting this tile mip, we want to keep it alive.
	if ( !m_heightMapHandle && !m_heightMapToken )
	{
		const Int32 terrainVersion = HACK_GetTerrainVersionCounter();
		m_heightMapToken = m_heightMapDataBuffer.AcquireBufferHandleAsync( eIOTag_TerrainHeight, [this,terrainVersion]( BufferHandle result )
		{
			if ( terrainVersion != HACK_GetTerrainVersionCounter() )
			{
				RED_LOG_WARNING( ClipMap, TXT("TerrainTileMipMap buffer load finished, but terrain version has changed since then! 'this' has probably been destroyed, so loaded data is being ignored.") );
				return;
			}

			m_heightMapHandle = result;
			//m_heightMapToken.Reset();
		} );
	}
	if ( !m_controlMapHandle && !m_controlMapToken )
	{
		const Int32 terrainVersion = HACK_GetTerrainVersionCounter();
		m_controlMapToken = m_controlMapDataBuffer.AcquireBufferHandleAsync( eIOTag_TerrainControl, [this,terrainVersion]( BufferHandle result )
		{
			if ( terrainVersion != HACK_GetTerrainVersionCounter() )
			{
				RED_LOG_WARNING( ClipMap, TXT("TerrainTileMipMap buffer load finished, but terrain version has changed since then! 'this' has probably been destroyed, so loaded data is being ignored.") );
				return;
			}

			m_controlMapHandle = result;
			//m_controlMapToken.Reset();
		} );
	}
	if ( !m_colorMapHandle && !m_colorMapToken )
	{
		const Int32 terrainVersion = HACK_GetTerrainVersionCounter();
		m_colorMapToken = m_colorMapDataBuffer.AcquireBufferHandleAsync( eIOTag_TerrainColor, [this,terrainVersion]( BufferHandle result )
		{
			if ( terrainVersion != HACK_GetTerrainVersionCounter() )
			{
				RED_LOG_WARNING( ClipMap, TXT("TerrainTileMipMap buffer load finished, but terrain version has changed since then! 'this' has probably been destroyed, so loaded data is being ignored.") );
				return;
			}

			m_colorMapHandle = result;
			//m_colorMapToken.Reset();
		} );
	}
}

Bool STerrainTileMipMap::IsLoading() const
{
	// It's possible we have a token and also handle, e.g. maybe if we request async loading on a buffer that's already loaded (callback is
	// called immediately, and then token assigned). So also check for missing buffer handle.
	return ( m_heightMapToken && !m_heightMapHandle )
		|| ( m_controlMapToken && !m_controlMapHandle )
		|| ( m_colorMapToken && !m_colorMapHandle && m_colorMapDataBuffer.GetSize() > 0 );
}

Bool STerrainTileMipMap::IsLoaded() const
{
	return m_heightMapHandle && m_controlMapHandle && ( m_colorMapDataBuffer.GetSize() == 0 || m_colorMapHandle );
}

Bool STerrainTileMipMap::IsPartiallyLoaded() const
{
	return m_heightMapHandle || m_controlMapHandle || ( m_colorMapDataBuffer.GetSize() > 0 && m_colorMapHandle );
}

Bool STerrainTileMipMap::LoadSync()
{
	PUMP_MESSAGES_DURANGO_CERTHACK();

	Bool neededLoad = false;

	if ( !m_heightMapHandle )
	{
		neededLoad = true;
		m_heightMapHandle = m_heightMapDataBuffer.AcquireBufferHandleSync();
		m_heightMapToken.Reset();
	}
	if ( !m_controlMapHandle )
	{
		neededLoad = true;
		m_controlMapHandle = m_controlMapDataBuffer.AcquireBufferHandleSync();
		m_controlMapToken.Reset();
	}
	if ( !m_colorMapHandle && m_colorMapDataBuffer.GetSize() > 0 )
	{
		neededLoad = true;
		m_colorMapHandle = m_colorMapDataBuffer.AcquireBufferHandleSync();
		m_colorMapToken.Reset();
	}

	PUMP_MESSAGES_DURANGO_CERTHACK();

	return IsLoaded();
}

void STerrainTileMipMap::Unload()
{
	// we never unload the cooked data
	if ( !m_wasBoundedWithCookedData )
	{
		m_heightMapHandle.Reset();
		m_controlMapHandle.Reset();
		m_colorMapHandle.Reset();
	}

	m_heightMapToken.Reset();
	m_controlMapToken.Reset();
	m_colorMapToken.Reset();

	m_wasLoaded = false;
}

void STerrainTileMipMap::UpdateWasLoaded()
{
	// Only update if we have switched from unloaded to loaded, to limit the amount we need to call IsLoaded.
	// We reset m_wasLoaded separately in Unload.
	if ( !m_wasLoaded )
	{
		m_wasLoaded = IsLoaded();
	}
}


Red::System::MemSize STerrainTileMipMap::GetResidentMemory() const 
{
	Red::System::MemSize memoryUsed = 0;
	if ( m_heightMapHandle )
	{
		memoryUsed += m_heightMapHandle->GetSize();
	}
	else if ( m_heightMapToken )
	{
		memoryUsed += m_heightMapDataBuffer.GetSize();
	}
	if( m_controlMapHandle )
	{
		memoryUsed += m_controlMapHandle->GetSize();
	}
	else if ( m_controlMapToken )
	{
		memoryUsed += m_controlMapDataBuffer.GetSize();
	}
	if( m_colorMapHandle )
	{
		memoryUsed += m_colorMapHandle->GetSize();
	}
	else if ( m_colorMapToken )
	{
		memoryUsed += m_colorMapDataBuffer.GetSize();
	}

	return memoryUsed;
}


void STerrainTileMipMap::SetColorMapData( const void* data, Uint32 size )
{
	m_colorMapDataBuffer.SetBufferContent( data, size );

	// Clear any existing handles, so we aren't still referencing the old color data.
	m_colorMapHandle.Reset();
	m_colorMapToken.Reset();
}


void CTerrainTile::GenerateCollisionHeightMapPass(const Uint32 stridePhysData, const Bool sameRowCol, const Uint16* tileHMnextRowColTile, const TControlMapType* tileCMnextRowColTile, const Bool sameRow, const Uint16* tileHMnextRowTile, const TControlMapType* tileCMnextRowTile, const Bool sameCol, const Uint16* tileHMnextColTile, const TControlMapType* tileCMnextColTile, const Uint16* tileHMTile, const TControlMapType* tileCMTile, TDynArray< PxHeightFieldSample >& physData, Uint32 resolution) 
{
#ifdef USE_PHYSX
	PC_SCOPE_PHYSICS(CTerrainTile GenerateCollision heightmapPass );

#ifdef USE_PHYSX

	for ( Uint32 u = 0; u < stridePhysData; ++u )
	{ 
		for ( Uint32 v = 0; v < stridePhysData; ++v )
		{

			TControlMapType controlMapVal = 0;
			Int32 val = 0;
			if( ( u == resolution ) && ( v == resolution ) )
			{
				int nextTileIndex = 0;
				if(sameRowCol)
				{
					nextTileIndex =  (resolution * resolution) - 1; // (resolution - 1) * resolution + resolution - 1
				}
				val = tileHMnextRowColTile[ nextTileIndex ];
				controlMapVal = tileCMnextRowColTile[nextTileIndex];
			}
			else if( u == resolution )
			{
				int nextTileIndex = v;
				if(sameRow)
				{
					nextTileIndex = (u - 1) * resolution + v;
				}
				val = tileHMnextRowTile[ nextTileIndex ];
				controlMapVal = tileCMnextRowTile[ nextTileIndex ];
			}
			else if( v == resolution )
			{
				int nextTileIndex = u * resolution;
				if(sameCol)
				{
					nextTileIndex = u * resolution + resolution - 1;
				}
				val = tileHMnextColTile[nextTileIndex];
				controlMapVal = tileCMnextColTile[ nextTileIndex];
			}
			else
			{
				val = tileHMTile[ u * ( resolution  ) + v ];
				controlMapVal = tileCMTile[ u * ( resolution ) + v ];
			}

			// fill the physX sample with calculated value  
			PxHeightFieldSample& sampl = physData[ u * stridePhysData + v ];
			sampl.height = PxI16( val - 32768 + 1 );
			if ( controlMapVal == 0 )
			{
				sampl.materialIndex0 = PxHeightFieldMaterial::eHOLE;
				sampl.materialIndex1 = PxHeightFieldMaterial::eHOLE;
			}
		}
	}
#endif // USE_PHYSX
#endif
}

const SPhysicalMaterial* CTerrainTile::GenerateCollisionsMaterialPass(const Uint32 stridePhysData, const Bool sameRowCol, const TControlMapType* tileCMnextRowColTile, const Bool sameRow, const TControlMapType* tileCMnextRowTile, const Bool sameCol, const TControlMapType* tileCMnextColTile, const TControlMapType* tileCMTile, TDynArray< PxHeightFieldSample >& physData, const Int32 count, PxHeightField* heightfield,const TDynArray< char > &physicalMaterials, const TDynArray< STerrainTextureParameters >& textureParams, Uint32 resolution)
{
#ifndef USE_PHYSX
	return nullptr;
#else
	PC_SCOPE_PHYSICS(CTerrainTile GenerateCollision materialsPass );

	Uint64 selectedPhysicalMaterialsMask = 0;
	const SPhysicalMaterial* singleMaterial = nullptr;

#ifdef USE_PHYSX
#ifndef RED_PLATFORM_ORBIS
	for ( Uint32 i = 0; i < stridePhysData; ++i )
	{ 
		for ( Uint32 j = 0; j < stridePhysData; ++j )
		{

			Int32 sampleIndex = i * stridePhysData + j;
			if( sampleIndex >= count )
			{
				continue;
			}

			PxHeightFieldSample& sample = physData[sampleIndex];

			PxVec3 normal = CalculateNormal(i, j, heightfield, resolution);

			normal.normalizeSafe();

			Vector normalVec = TO_VECTOR( normal );
			Float vertexSlope = Vector::Dot3(normalVec, Vector::EZ );
			Vector flattenedCombinedVerticalNormal = Lerp(normalVec, Vector::EZ, vertexSlope);

			TControlMapType controlMapVal;

			if( ( i == resolution ) && ( j == resolution ) )
			{
				int nextTileIndex = 0;
				if(sameRowCol)
				{
					nextTileIndex =  (resolution * resolution) - 1; // (resolution - 1) * resolution + resolution - 1
				}
				controlMapVal = tileCMnextRowColTile[nextTileIndex];
			}
			else if( i == resolution )
			{
				int nextTileIndex = j;
				if(sameRow)
				{
					nextTileIndex = (i - 1) * resolution + j;
				}
				controlMapVal = tileCMnextRowTile[ nextTileIndex ];
			}
			else if( j == resolution )
			{
				int nextTileIndex = i * resolution;
				if(sameCol)
				{
					nextTileIndex = i * resolution + resolution - 1;
				}
				controlMapVal = tileCMnextColTile[ nextTileIndex];
			}
			else
			{
				controlMapVal = tileCMTile[ i * ( resolution ) + j ];
			}

			PxBitAndByte materialIndex = 0;
			if ( controlMapVal != 0 )
			{
				int textureIndex1 = controlMapVal & 0x0000001F;
				controlMapVal = controlMapVal >> 5;
				int textureIndex2 = controlMapVal & 0x0000001F;
				controlMapVal = controlMapVal >> 5;
				int textureRatio = controlMapVal & 0x00000007;

				if( ( textureIndex1 > 0 ) && ( textureIndex2 > 0 ) && ( ( int ) physicalMaterials.Size() > textureIndex1 ) && ( ( int ) physicalMaterials.Size() > textureIndex2 ) )
				{
					//Check terrainMaterialDX11.fx for the source of that solution
					Float slopeBasedDamp = textureParams[ textureIndex2 - 1 ].m_val.Y;
					Float horizontalToVerticalBlendSharpness  = textureParams[ textureIndex1 - 1 ].m_val.X;

					Vector biasedFlattenedCombinedVerticalNormal = Lerp(normalVec, flattenedCombinedVerticalNormal, slopeBasedDamp).Normalized3();

					const Float slopeThresholds[8] = { 0.0f, 0.125f, 0.25, 0.375f, 0.5f, 0.625f, 0.75f, 1.0f };
					Float slopeThreshold = slopeThresholds[ textureRatio ];

					Float verticalSurfaceTangent = ComputeSlopeTangent( biasedFlattenedCombinedVerticalNormal, slopeThreshold, Saturate( slopeThreshold + horizontalToVerticalBlendSharpness ) );
					Bool useHorz = verticalSurfaceTangent < 0.5f;


					Uint16 resultIndex = useHorz ? textureIndex1 - 1 : textureIndex2 - 1;
					unsigned char physicalMaterialIndex = physicalMaterials[ resultIndex ];
					if( physicalMaterialIndex != -1 )
					{
						if( physicalMaterials[ resultIndex ] == -1 )
						{
							//this is to make able to create highmap when there is texture used whichout physical material assigment
							physicalMaterialIndex = 0;
						}
						materialIndex = PxBitAndByte( physicalMaterialIndex );
					}
				}

				if( sample.materialIndex0 != PxHeightFieldMaterial::eHOLE )
				{
					selectedPhysicalMaterialsMask |= ( ( Uint64 )1 ) << materialIndex;
					sample.materialIndex0 = materialIndex;
				}

				if( sample.materialIndex1 != PxHeightFieldMaterial::eHOLE )
				{
					selectedPhysicalMaterialsMask |= ( ( Uint64 )1 ) << materialIndex;
					sample.materialIndex1 = materialIndex;
				}

			}
		}
	}
#endif
#endif // USE_PHYSX
	
	if( !selectedPhysicalMaterialsMask )
	{
		singleMaterial = GPhysicEngine->GetMaterial();
	}
	else for( Uint64 i = 0; i != 63; i++ )
	{
		if( selectedPhysicalMaterialsMask == ( ( ( Uint64 ) 1 ) << i ) )
		{
			singleMaterial = GPhysicEngine->GetMaterial( ( Uint32 ) i );
			break;
		}
	}

#ifdef USE_PHYSX
	if( singleMaterial )
	{
		//rare case when whole heightmap have one material only
		for( Int32 i = 0; i != count; ++i )
		{
			PxHeightFieldSample& sample = physData[ i ];
			if( sample.materialIndex0 != PxHeightFieldMaterial::eHOLE )
			{
				sample.materialIndex0 = 0;
			}

			if( sample.materialIndex1 != PxHeightFieldMaterial::eHOLE )
			{
				sample.materialIndex1 = 0;
			}
		}
	}	
#endif // USE_PHYSX

	return singleMaterial;
#endif
}
