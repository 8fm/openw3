/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "clipMap.h"
#include "terrainTile.h"
#include "terrainUtils.h"
#include "vegetationBrush.h"
#include "renderCommands.h"
#ifndef NO_HEIGHTMAP_EDIT
	#include "heightmapUtils.h"
#endif

#include "../core/directory.h"
#include "../core/feedback.h"
#include "baseTree.h"
#include "foliageEditionController.h"
#include "textureArray.h"
#include "renderObject.h"
#include "renderProxy.h"
#include "../core/diskFile.h"
#include "world.h"
#include "material.h"
#include "../core/configVar.h"
#include "../core/configVarLegacyWrapper.h"
#include "../core/deferredDataBuffer.h"
#include "../engine/renderSettings.h"

#ifdef USE_ANSEL
#include "../engine/anselIncludes.h"
#endif // USE_ANSEL

// For OnCook
#include "layerGroup.h"
#include "layer.h"
#include "layerInfo.h"
#include "entity.h"
#include "globalWater.h"
#include "globalWaterUpdateParams.h"
#include "waterComponent.h"
#include "../physics/physicsWorld.h"

namespace Config
{
	TConfigVar< Bool >		cvTerrainStreamingEnabled( "Streaming/Terrain", "StreamingEnabled", true );
}

//////////////////////////////////////////////////////////////////////////

// Copy source data into a new buffer, resizing and resampling.
// outDestPtr will be freed if it is non-NULL, and allocated to fit the new data.
// The destination buffer will be sized for the terrain stamp texture (TERRAIN_STAMP_TEX_SIZE x TERRAIN_STAMP_TEX_SIZE)
template< typename T, typename _Filter >
static void CreateStampDataResized( const T* srcPtr, Uint32 srcWidth, Uint32 srcHeight, T*& outDestPtr, Uint32& outDataSize, Uint32& outPitch )
{
	if ( outDestPtr )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_BufferBitmap, outDestPtr );
		outDestPtr = NULL;
		outDataSize = 0;
		outPitch = 0;
	}

	if ( !srcPtr ) return;

	const Uint32 width = TERRAIN_STAMP_VISUALIZATION_TEX_SIZE;
	const Uint32 height = TERRAIN_STAMP_VISUALIZATION_TEX_SIZE;

	Uint32 size = width * height * sizeof( T );
	outDestPtr = (T*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_BufferBitmap, size );
	if ( !outDestPtr ) return;

	outPitch = width * sizeof( T );
	outDataSize = size;

	for ( Uint32 j = 0; j < height; ++j )
	{
		for ( Uint32 i = 0; i < width; ++i )
		{
			Float si = ( ( (Float)i / (Float)width ) * srcWidth );
			Float sj = ( ( (Float)j / (Float)height ) * srcHeight );

			outDestPtr[i + j*width] = TerrainUtils::Sample< T, _Filter >( srcPtr, si, sj, srcWidth, srcHeight );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

// This struct is used to sort clip-map levels that could be evicted from memory into a priority queue
struct MipsSortPred
{
	Bool operator()( STerrainMipmapEvictionTracker& c1, STerrainMipmapEvictionTracker& c2 ) const
	{
		Float timeDifference = c1.m_timeRemaining - c2.m_timeRemaining;
		if( timeDifference != 0.0f )
		{
			return timeDifference < 0.0f ? true : false; 
		}

		// Same time difference, instead sort by size biggest to smallest 
		return c1.m_dataSize > c2.m_dataSize;
	}
};

//////////////////////////////////////////////////////////////////////////

const Int32 c_storageDataCapacity = 12 * 1024 * 1024;

CClipMapCookedData::Storage CClipMapCookedData::m_storage;

CClipMapCookedData::CClipMapCookedData()
{
	m_storage.data.ClearFast();
	m_storage.tiles.ClearFast();
}

CClipMapCookedData::~CClipMapCookedData()
{}

void CClipMapCookedData::OnSerialize( IFile& file )
{
	COMPILE_ASSERT( sizeof(BufferEntry) == 8 );
	COMPILE_ASSERT( sizeof(TileEntry) == 28 );

	m_storage.data.SerializeBulkFast( file );
	m_storage.tiles.SerializeBulkFast( file );

	RED_FATAL_ASSERT( c_storageDataCapacity == m_storage.data.Capacity(), "Storage data has change. Increase buffer size or Debug!" ); 
}

void CClipMapCookedData::StoreData( const Uint32 tileX, const Uint32 tileY, const Uint32 mipIndex, BufferHandle heightData, BufferHandle colorData, BufferHandle controlData )
{
	TileEntry* tileEntry = new ( m_storage.tiles ) TileEntry;
	tileEntry->m_tileX = tileX;
	tileEntry->m_tileY = tileY;
	tileEntry->m_mipIndex = mipIndex;

	StoreBuffer( tileEntry->m_colorData, colorData );
	StoreBuffer( tileEntry->m_heightData, heightData );
	StoreBuffer( tileEntry->m_controlData, controlData );
}

void CClipMapCookedData::StoreBuffer( BufferEntry& outEntry, BufferHandle data )
{
	if ( data->GetSize() > 0 && data->GetData() != nullptr )
	{
		const Uint32 offset = (const Uint32) m_storage.data.Grow( data->GetSize() );
		Red::MemoryCopy( &m_storage.data[offset], data->GetData(), data->GetSize() );

		outEntry.m_dataOffset = offset;
		outEntry.m_dataSize = data->GetSize();
	}
	else
	{
		outEntry.m_dataOffset = 0;
		outEntry.m_dataSize = 0;
	}
}

BufferHandle CClipMapCookedData::RestoreBuffer( const BufferEntry& bufferEntry ) const
{
	BufferHandle ret;

	if ( bufferEntry.m_dataSize > 0 )
	{
		const Uint8* bufferData = &m_storage.data[ bufferEntry.m_dataOffset ];
		ret.Reset( new BufferProxy( (void*)bufferData, bufferEntry.m_dataSize, [](void*) { /* no release */ } ) );
	}

	return ret;
}

void CClipMapCookedData::RestoreData( CClipMap* intoClipmap ) const
{
	PC_SCOPE_PIX( ClipMapCookedData RestoreData );

	for ( const auto& tileInfo : m_storage.tiles )
	{
		CTerrainTile* tile = intoClipmap->GetTile( tileInfo.m_tileX, tileInfo.m_tileY );
		if ( tile != nullptr )
		{
			const STerrainTileMipMap* mipmap = tile->GetMipMap( tileInfo.m_mipIndex );
			if ( mipmap )
			{
				BufferHandle colorData = RestoreBuffer( tileInfo.m_colorData );
				BufferHandle heightData = RestoreBuffer( tileInfo.m_heightData );
				BufferHandle controlData = RestoreBuffer( tileInfo.m_controlData );

				const_cast< STerrainTileMipMap* >(mipmap)->BindCookedData( heightData, controlData, colorData );
			}
		}
	}
}

const Uint32 CClipMapCookedData::GetTotalDataSize() const
{
	return (Uint32) m_storage.data.DataSize();
}

const Uint32 CClipMapCookedData::GetTotalBufferCount() const
{
	Uint32 numBuffers = 0;
	for ( const auto& tile : m_storage.tiles )
	{
		if ( tile.m_colorData.m_dataSize > 0)
			numBuffers += 1;
		if ( tile.m_controlData.m_dataSize > 0)
			numBuffers += 1;
		if ( tile.m_heightData.m_dataSize > 0)
			numBuffers += 1;
	}
	return numBuffers;
}

void CClipMapCookedData::Hack_InitializeStorage()
{
	m_storage.data.Reserve( c_storageDataCapacity );
}

void CClipMapCookedData::Hack_DestroyStorage()
{
	m_storage.data.Clear();
	m_storage.tiles.Clear();
}

//////////////////////////////////////////////////////////////////////////

/*
The naming in this class, reflects the naming in the paper called "Real-time rendering of large terrains with support for runtime modifications".
*/

#define ASSUME_CLIPMAP_CHECKED_OUT \
			ASSERT( GetWorld() );\
			ASSERT( GetWorld()->IsModified() );

IMPLEMENT_ENGINE_CLASS( STerrainTextureParameters );
IMPLEMENT_ENGINE_CLASS( CClipMap );
IMPLEMENT_ENGINE_CLASS( CClipMapCookedData );

CClipMap::CClipMap()
	: m_clipmapSize( 0 )
	, m_clipSize( 0 )
	, m_tileRes( 0 )
	, m_numClipmapStackLevels( 0 )
	, m_colormapStartingMip( -1 )
	, m_terrainSize( 0.0f )	
	, m_numTilesPerEdge( 0 )
	, m_renderProxy( NULL )
	, m_material( NULL )
	, m_cookedMipStackHeight( CookedMipAllocator::GetInstance() )
	, m_cookedMipStackControl( CookedMipAllocator::GetInstance() )
	, m_cookedMipStackColor( CookedMipAllocator::GetInstance() )
	, m_nextClipmapLevelToUpdate( 0 )
	, m_enableLoading( true )
#ifndef NO_HEIGHTMAP_EDIT
	, m_stampDataUpdate( NULL )
	, m_stampGizmoVisible( true )
#endif
#ifndef NO_EDITOR
	, m_isEditing( false )
#endif
{
	m_textureParams.Resize( NUM_TERRAIN_TEXTURES_AVAILABLE );
	m_grassBrushes.Reserve( NUM_TERRAIN_TEXTURES_AVAILABLE );
	for ( Uint32 t=0; t<NUM_TERRAIN_TEXTURES_AVAILABLE; ++t )
	{
		m_textureParams[t].m_val.Set4( 0.0f, 0.0f, 0.0f, 0.0f );
		m_textureParams[t].m_val2.Set4( 0.0f, 0.0f, 0.0f, 0.0f );
		m_grassBrushes.PushBack( NULL );
	}
}

CClipMap::~CClipMap()
{
	ClearTerrainProxy();

	if ( m_cookedData != nullptr )
	{
		delete m_cookedData;
		m_cookedData = nullptr;
	}
}

void CClipMap::OnPostLoad()
{
	// Pass to base class
	TBaseClass::OnPostLoad();

	SwitchDataBuffersToBufferHandles();

#ifndef NO_HEIGHTMAP_EDIT
	// Make sure tiles are updated to latest version
	for ( THandle< CTerrainTile > tile : m_terrainGrid )
	{
		if ( tile )
		{
			tile->UpdateTileStructure( this );
		}
	}
#endif

	GenerateLevels();

	// Restore cooked data
	if ( m_cookedData != nullptr )
	{
		m_cookedData->RestoreData( this );
	}

	m_TouchAndEvictCoordBuffer.Reserve( m_numClipmapStackLevels );
	m_clipmapUpdates.Reserve( m_numClipmapStackLevels + 1 );		// +1 comes from Update() -> "if ( !m_renderProxy )"
	m_mipsToEvict.Reserve( 512 );
}

void CClipMap::OnSerialize( IFile& file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );

	// Read/write number of tiles
	Uint32 numTiles = m_terrainGrid.Size();
	file << numTiles;

	if ( file.IsReader() )
	{
		m_terrainGrid.Resize( numTiles );
		m_tileCache.Resize( numTiles );
	}

	for ( Uint32 i=0; i<numTiles; ++i )
	{
		file << m_terrainGrid[i];
	}
}

void CClipMap::ClearTerrainProxy() 
{
	if ( m_renderProxy )
	{
		( new CRenderCommand_RemoveTerrainProxyFromScene( GetWorld()->GetRenderSceneEx(), m_renderProxy ) )->Commit();
		m_renderProxy->Release();
		m_renderProxy = NULL;
	}
}

#ifndef NO_HEIGHTMAP_EDIT

void CClipMap::ImportTerrain( const TDynArray< String >& tilePaths )
{
	if ( !tilePaths.Size() )
	{
		return;
	}

	GFeedback->BeginTask( TXT("Importing terrain..."), false );

	Bool importHM = true;
	Bool importCM = false;

	for ( Uint32 i = 0; i < tilePaths.Size(); ++i )
	{
		Uint32 x;
		Uint32 y;
		if ( GetCoordinatesFromWorldMachineFilename( tilePaths[ i ], x, y ) )
		{
			ImportTile( x, y, importHM, tilePaths[ i ], importCM, tilePaths[ i ] );
			GFeedback->UpdateTaskProgress( i + 1, tilePaths.Size() );
		}
	}
	GFeedback->EndTask();

	OnDataChanged();
}

ImportStatus CClipMap::ImportTile( Uint32 x, Uint32 y, const Bool importHM, const String& hmPath, const Bool importCM, const String& cmPath )
{
	ImportStatus res = EIS_NothingImported;

	// TODO: check edges for changes, if changed - there can be discontinuity in the mesh
	Uint32 index = y * m_numTilesPerEdge + x;
	if ( index < 0 || index >= m_terrainGrid.Size() )
	{
		return res;
	}

	if ( importHM )
	{
		Bool hmRes = m_terrainGrid[ index ]->ImportData( hmPath );
		if ( !hmRes )
		{
			WARN_ENGINE( TXT( "Loading heightmap from [%s] failed." ), hmPath.AsChar() );
		}
		else
		{
			res |= EIS_HeightmapImported;
		}
	}
	if ( importCM )
	{
		Bool cmRes = m_terrainGrid[ index ]->ImportColorMap( cmPath );
		if ( !cmRes )
		{
			WARN_ENGINE( TXT( "Loading colormap from [%s] failed." ), cmPath.AsChar() );
		}
		else
		{
			res |= EIS_ColormapImported;
		}
	}

	if ( res & EIS_ColormapImported || res & EIS_HeightmapImported )
	{
		m_terrainGrid[ index ]->Save();
	}

	return res;
}

Bool CClipMap::OnExportTile( Uint32 x, Uint32 y, Uint32 minX, Uint32 minY, const String& directory, const String& filename, const Bool exportHM, String& hmFileName, const Bool exportCM, String& cmFileName )
{
	Uint32 index = y * m_numTilesPerEdge + x;

	ASSERT( index >= 0 );
	ASSERT( index < m_terrainGrid.Size() );
	ASSERT( m_terrainGrid[ index ] );

	x -= minX;
	y -= minY;

	Bool res = true;

	// format the filename like: [filename without extension] + "_x" + [x index] + "_y" + [y index] + ".png"
	if ( exportHM )
	{
		String indexedHMFilename = String::Printf( TXT( "%s_x%d_y%d.png" ), filename.AsChar(), x, y );
		hmFileName = String::Printf( TXT("%s\\%s_output\\%s"), directory.AsChar(), filename.AsChar(), indexedHMFilename.AsChar() );

		res &= m_terrainGrid[ index ]->ExportData( hmFileName );
	}

	if ( exportCM )
	{
		String indexedCMFilename = String::Printf( TXT( "%s_cm_x%d_y%d.png" ), filename.AsChar(), x, y );
		cmFileName = String::Printf( TXT("%s\\%s_output\\%s"), directory.AsChar(), filename.AsChar(), indexedCMFilename.AsChar() );

		res &= m_terrainGrid[ index ]->ExportColorMap( cmFileName );
	}

	return res;
}

void CClipMap::Test_ImportTerrain()
{
	ASSUME_CLIPMAP_CHECKED_OUT

		String absPathBase = GetWorld()->GetFile()->GetDirectory()->GetAbsolutePath();
	absPathBase += TXT("external_tiles\\");

	GFeedback->BeginTask( TXT("Importing terrain..."), false );
	for ( Uint32 i=0; i<m_terrainGrid.Size(); ++i )
	{
		String externalTilePath = absPathBase;
		externalTilePath += TXT("puget_tile-");
		externalTilePath += ToString( i );
		externalTilePath += TXT(".png");

		m_terrainGrid[i]->ImportData( externalTilePath );
		m_terrainGrid[i]->Save();
		GFeedback->UpdateTaskProgress( i+1, m_terrainGrid.Size() );
	}
	GFeedback->EndTask();

	OnDataChanged();
}

Bool CClipMap::Reinit( const SClipmapParameters& initInfo )
{
	ASSERT( initInfo.clipmapSize >= MIN_CLIPMAP_SIZE && initInfo.clipmapSize <= MAX_CLIPMAP_SIZE );
	ASSERT( IsPow2( initInfo.clipSize ) );
	ASSERT( initInfo.clipSize <= initInfo.clipmapSize );
	ASSERT( initInfo.clipSize >= MIN_CLIPSIZE );

	SClipmapParameters currentParams;
	GetClipmapParameters( &currentParams );
	if ( currentParams == initInfo )
	{
		return true;
	}

	// Check out if not local
	if ( !MarkModified() )
	{
		// Can't modify
		RED_LOG( RED_LOG_CHANNEL( TerrainClipMaps ), TXT("CClipMap::Reinit() : Couldn't reinit the terrain clipmap. The reason is that the world couldn't be checked out.") );
		return false;
	}

	if ( !IsCompatible( initInfo ) )
	{
		// We have to change the grid configuration. So, first make sure we have write access to each and every tile.
		if ( !PerformFullGridCheckout() )
		{
			RED_LOG( RED_LOG_CHANNEL( TerrainClipMaps ), TXT("CClipMap::Reinit() : Couldn't reinit the terrain clipmap. The reason is that not all terrain tiles could be accessed for write (couldn't check out?)") );
			return false;
		}

		// By now we have all tiles checked out. We can preserve their data, destroy them, and build a new set of tiles.
		BuildTerrainTiles( initInfo );
	}

	// Copy new setup params
	m_clipmapSize = initInfo.clipmapSize;
	m_clipSize = initInfo.clipSize;
	m_tileRes = initInfo.tileRes;
	m_terrainSize = initInfo.terrainSize;
	m_lowestElevation = initInfo.lowestElevation;
	m_highestElevation = initInfo.highestElevation;

	// Compute number of tiles on the edge (square gives number of tiles)
	m_numTilesPerEdge = m_clipmapSize / m_tileRes;

	// Compute clipmap stack size
	m_numClipmapStackLevels = ComputeNumberOfClipmapStackLevels( m_clipmapSize, m_clipSize );

	// Compute position of the terrain left-bottom corner
	m_terrainCorner = Vector( -m_terrainSize/2.0f, -m_terrainSize/2.0f, 0.0f );

	m_tileHeightRanges.Clear();
	m_minWaterHeight.Clear();

	// Create new clipmap levels infos
	GenerateLevels();

	ClearTerrainProxy();

	GetWorld()->Save();

	// Done
	return true;
}

Bool CClipMap::IsCompatible( const SClipmapParameters& initInfo )
{
	Bool compatible = true;
	
	compatible &= m_tileRes == initInfo.tileRes;
	compatible &= m_clipmapSize == initInfo.clipmapSize;
	compatible &= m_clipSize == initInfo.clipSize;

	return compatible;
}

Bool CClipMap::GetCoordinatesFromWorldMachineFilename( const String& filename, Uint32& x, Uint32& y )
{
	size_t xPos;
	filename.FindSubstring( TXT("_x"), xPos, true );
	size_t yPos;
	filename.FindSubstring( TXT("_y"), yPos, true );
	size_t dotPos; 
	filename.FindCharacter( TXT( '.' ), dotPos, true );

	size_t firstNumberLength	= yPos		- xPos - 2 /*Red::System::StringLength("_x")*/;
	size_t secondNumberLength	= dotPos	- yPos - 2 /*Red::System::StringLength("_y")*/;

	String xSubstr = filename.MidString( xPos + 2 /* Red::System::StringLength("_x") */, firstNumberLength );
	String ySubstr = filename.MidString( yPos + 2 /* Red::System::StringLength("_y") */, secondNumberLength );

	return FromString< Uint32 >( xSubstr, x ) && FromString< Uint32 >( ySubstr, y );
}

void CClipMap::SetMaterial( IMaterial* material )
{
	ASSERT( material );
	m_material = material;
}

void CClipMap::SaveAllTiles()
{
	for ( Uint32 t=0; t<m_terrainGrid.Size(); ++t )
	{
		CTerrainTile* tile = m_terrainGrid[t].Get();
		if ( tile && tile->IsModified() )
		{
			tile->Save();
		}
	}
}

#ifndef NO_HEIGHTMAP_EDIT
void CClipMap::SetStampData( const String& heightmapToLoad )
{
	ASSERT( !heightmapToLoad.Empty(), TXT("SetStampData() given empty heightmap path") );
	if ( heightmapToLoad.Empty() ) return;

	if ( !m_stampDataUpdate )
	{
		m_stampDataUpdate = new SClipmapStampDataUpdate();
	}

	if ( m_stampDataUpdate->m_heightData )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_BufferBitmap, m_stampDataUpdate->m_heightData );
		m_stampDataUpdate->m_heightData = NULL;
		m_stampDataUpdate->m_heightDataSize = 0;
	}
	// Clear out unused data buffers
	if ( m_stampDataUpdate->m_colorData )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_BufferBitmap, m_stampDataUpdate->m_colorData );
		m_stampDataUpdate->m_colorData = NULL;
		m_stampDataUpdate->m_colorDataSize = 0;
	}
	if ( m_stampDataUpdate->m_controlData )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_BufferBitmap, m_stampDataUpdate->m_controlData );
		m_stampDataUpdate->m_controlData = NULL;
		m_stampDataUpdate->m_controlDataSize = 0;
	}

	SHeightmapImageEntry<Uint16> entry;
	entry.m_params.normalizeImage = true;

	Uint32 srcWidth, srcHeight;
	m_stampDataUpdate->m_isValid = SHeightmapUtils::GetInstance().LoadHeightmap( heightmapToLoad, entry, &srcWidth, &srcHeight );
	if ( m_stampDataUpdate->IsValid() )
	{
		CreateStampDataResized< Uint16, TerrainUtils::FilterHeightMap >( entry.m_data, srcWidth, srcHeight, m_stampDataUpdate->m_heightData, m_stampDataUpdate->m_heightDataSize, m_stampDataUpdate->m_heightDataPitch );
		m_stampDataUpdate->m_originalDataSize = Max( srcWidth, srcHeight );
		m_stampDataUpdate->m_heightDataDirty = true;
		m_stampDataUpdate->m_colorDataDirty = true;
		m_stampDataUpdate->m_controlDataDirty = true;
	}
}

void CClipMap::SetStampData( const Uint16* data, Uint32 dataWidth, Uint32 dataHeight )
{
	if ( !m_stampDataUpdate )
	{
		m_stampDataUpdate = new SClipmapStampDataUpdate();
	}

	CreateStampDataResized< Uint16, TerrainUtils::FilterHeightMap >( data, dataWidth, dataHeight, m_stampDataUpdate->m_heightData, m_stampDataUpdate->m_heightDataSize, m_stampDataUpdate->m_heightDataPitch );
	m_stampDataUpdate->m_originalDataSize = Max( dataWidth, dataHeight );
	m_stampDataUpdate->m_isValid = ( m_stampDataUpdate->m_heightData != NULL ) || ( m_stampDataUpdate->m_colorData != NULL ) || ( m_stampDataUpdate->m_controlData != NULL );
	m_stampDataUpdate->m_heightDataDirty = true;
}

void CClipMap::SetStampColorData( const TColorMapType* data, Uint32 dataWidth, Uint32 dataHeight )
{
	if ( !m_stampDataUpdate )
	{
		m_stampDataUpdate = new SClipmapStampDataUpdate();
	}

	CreateStampDataResized< TColorMapType, TerrainUtils::FilterColorMap >( data, dataWidth, dataHeight, m_stampDataUpdate->m_colorData, m_stampDataUpdate->m_colorDataSize, m_stampDataUpdate->m_colorPitch );
	m_stampDataUpdate->m_isValid = ( m_stampDataUpdate->m_heightData != NULL ) || ( m_stampDataUpdate->m_colorData != NULL ) || ( m_stampDataUpdate->m_controlData != NULL );
	m_stampDataUpdate->m_colorDataDirty = true;
}


void CClipMap::SetStampControlData( const TControlMapType* data, Uint32 dataWidth, Uint32 dataHeight )
{
	if ( !m_stampDataUpdate )
	{
		m_stampDataUpdate = new SClipmapStampDataUpdate();
	}

	CreateStampDataResized< TControlMapType, TerrainUtils::FilterControlMap >( data, dataWidth, dataHeight, m_stampDataUpdate->m_controlData, m_stampDataUpdate->m_controlDataSize, m_stampDataUpdate->m_controlPitch );
	m_stampDataUpdate->m_isValid = ( m_stampDataUpdate->m_heightData != NULL ) || ( m_stampDataUpdate->m_colorData != NULL ) || ( m_stampDataUpdate->m_controlData != NULL );
	m_stampDataUpdate->m_controlDataDirty = true;
}


void CClipMap::SetStampCenter( const Vector2& center )
{
	if ( !m_stampDataUpdate )
	{
		m_stampDataUpdate = new SClipmapStampDataUpdate();
	}
	m_stampDataUpdate->m_center = center;
}

void CClipMap::SetStampSize( Float size )
{
	if ( !m_stampDataUpdate )
	{
		m_stampDataUpdate = new SClipmapStampDataUpdate();
	}
	m_stampDataUpdate->m_size = size;
}

void CClipMap::SetStampIntensity( Float stampIntensity )
{
	if ( !m_stampDataUpdate )
	{
		m_stampDataUpdate = new SClipmapStampDataUpdate();
	}
	m_stampDataUpdate->m_heightScale = stampIntensity;
}

void CClipMap::SetStampOffset( Float stampOffset )
{
	if ( !m_stampDataUpdate )
	{
		m_stampDataUpdate = new SClipmapStampDataUpdate();
	}
	m_stampDataUpdate->m_heightOffset = stampOffset;
}

void CClipMap::SetStampRotation( Float radians )
{
	if ( !m_stampDataUpdate )
	{
		m_stampDataUpdate = new SClipmapStampDataUpdate();
	}
	m_stampDataUpdate->m_radians = radians;
}

void CClipMap::SetStampModeAdditive( Bool additive )
{
	if ( !m_stampDataUpdate )
	{
		m_stampDataUpdate = new SClipmapStampDataUpdate();
	}
	m_stampDataUpdate->m_additive = additive;
}

void CClipMap::ClearStampData()
{
	if ( m_stampDataUpdate )
	{
		// Can safely delete, since a copy was made for the renderer.
		delete m_stampDataUpdate;
		m_stampDataUpdate = NULL;
	}
}

#endif	// !NO_HEIGHTMAP_EDIT

void CClipMap::OffsetTiles( Int32 rowOffset, Int32 colOffset )
{
	// Check out if not local
	if ( !MarkModified() )
	{
		// Can't modify
		RED_LOG( RED_LOG_CHANNEL( TerrainClipMaps ), TXT("CClipMap::OffsetTiles() : Couldn't offset the terrain clipmap. The reason is that the world couldn't be checked out.") );
		return;
	}

	// We have to change the grid configuration. So, first make sure we have write access to each and every tile.
	if ( !PerformFullGridCheckout() )
	{
		RED_LOG( RED_LOG_CHANNEL( TerrainClipMaps ), TXT("CClipMap::OffsetTiles() : Couldn't offset the terrain clipmap. The reason is that not all terrain tiles could be accessed for write (couldn't check out?)") );
		return;
	}

	GFeedback->BeginTask( TXT("Offsetting terrain..."), false );
	Int32 numProgressSteps = ( m_numTilesPerEdge - rowOffset ) * ( m_numTilesPerEdge - colOffset );
	Int32 progress = 0;
	for ( Uint32 r=abs(rowOffset); r<m_numTilesPerEdge; ++r )
	{
		for ( Uint32 c=abs(colOffset); c<m_numTilesPerEdge; ++c )
		{
			// Get the tile that we are offsetting
			CTerrainTile* tile1 = m_terrainGrid[ r * m_numTilesPerEdge + c ].Get();
			ASSERT( tile1 );
			
			// Get the tile that we are offsetting to
			Int32 targetRow = abs( (Int32) ( r + rowOffset ) % (Int32)m_numTilesPerEdge );
			Int32 targetCol = abs( (Int32) ( c + colOffset ) % (Int32)m_numTilesPerEdge );
			CTerrainTile* tile2 = m_terrainGrid[ targetRow * m_numTilesPerEdge + targetCol ].Get();
			ASSERT( tile2 );
			
			if ( tile1 != tile2 )
			{
				CDiskFile* file1 = tile1->GetFile();
				CDiskFile* file2 = tile2->GetFile();

				tile1->LoadAllMipmapsSync();
				tile2->LoadAllMipmapsSync();

				// HURRRAAAAA!!!
				file1->Rebind( tile2 );
				file2->Rebind( tile1 );

				tile1->Save();
				tile2->Save();

				tile1->SetDirty( true );
				tile2->SetDirty( true );

				m_terrainGrid[ r * m_numTilesPerEdge + c ] = tile2;
				m_terrainGrid[ targetRow * m_numTilesPerEdge + targetCol ] = tile1;

				GFeedback->UpdateTaskProgress( progress++, numProgressSteps );
			}
		}
	}
	GFeedback->EndTask();

	GetWorld()->Save();
}

void CClipMap::SetTextureParam( Int32 textureIndex, Int32 paramIndex, Float value )
{
	ASSERT( textureIndex >= 0 && textureIndex < NUM_TERRAIN_TEXTURES_AVAILABLE );
	ASSERT( paramIndex >= 0 && paramIndex < 8 );

	Vector& val = m_textureParams[textureIndex].m_val;
	Vector& val2 = m_textureParams[textureIndex].m_val2;
	switch( paramIndex )
	{
	case 0: val.X = value; break;
	case 1: val.Y = value; break;
	case 2: val.Z = value; break;
	case 3: val.W = value; break;
	case 4: val2.X = value; break;
	case 5: val2.Y = value; break;
	case 6: val2.Z = value; break;
	case 7: val2.W = value; break;
	}
}

Float CClipMap::GetTextureParam( Int32 textureIndex, Int32 paramIndex )
{
	ASSERT( textureIndex >= 0 && textureIndex < NUM_TERRAIN_TEXTURES_AVAILABLE );
	ASSERT( paramIndex >= 0 && paramIndex < 8 );

	const Vector& val = m_textureParams[textureIndex].m_val;
	const Vector& val2 = m_textureParams[textureIndex].m_val2;
	switch( paramIndex )
	{
	case 0: return val.X;
	case 1: return val.Y;
	case 2: return val.Z;
	case 3: return val.W;
	case 4: return val2.X;
	case 5: return val2.Y;
	case 6: return val2.Z;
	case 7: return val2.W;
	}

	return 0.0f;
}

void CClipMap::SetGrassBrush( Int32 textureIndex, CVegetationBrush* brush )
{
	m_grassBrushes[ textureIndex ] = brush;

	UpdateGrassRendering();
}

#endif

CVegetationBrush* CClipMap::GetGrassBrush( Int32 textureIndex ) const
{
	return m_grassBrushes[ textureIndex ].Get();
}

// Get loaded terrain tiles overlapping with extents
void CClipMap::GetTiles( const Box& extents, TTerrainTiles& tiles ) const
{
	// Reset list
	tiles.ClearFast();

	// Test loaded tiles
	for ( Uint32 y = 0; y < m_numTilesPerEdge; ++y )
	for ( Uint32 x = 0; x < m_numTilesPerEdge; ++x )
	{
		// Get tile box
		const Float tileMinX = m_terrainCorner.X + x * m_tileRes;
		const Float tileMinY = m_terrainCorner.Y + y * m_tileRes;
		const Float tileMaxX = tileMinX + m_tileRes;
		const Float tileMaxY = tileMinY + m_tileRes;

		// Is in range ?
		if ( extents.Max.X >= tileMinX && extents.Max.Y >= tileMinY && extents.Min.X <= tileMaxX && extents.Min.Y <= tileMaxY )
		{
			CTerrainTile* tile = m_terrainGrid[ y * m_numTilesPerEdge + x ].Get();
			RED_ASSERT( tile, TXT("Tile %u,%u not loaded"), x, y );
			if ( tile )
			{
				tiles.PushBack( tile );
			}
		}
	}
}

void CClipMap::GetRectangleOfData( const Rect& rect, TDynArray< SClipmapHMQueryResult >& parts, Rect* outColorRect )
{
	ASSERT( parts.Empty() );

	const Uint32 highestColorMapResolution = m_tileRes >> m_colormapStartingMip;

	const Rect brushRectClipmapSpace = rect;

	if ( outColorRect )
	{
		Float scale = (Float)highestColorMapResolution / m_tileRes;

		outColorRect->m_left = (Int32)MRound( (Float)rect.m_left * scale );
		outColorRect->m_right = outColorRect->m_left + (Int32)MRound( (Float)rect.Width() * scale );

		outColorRect->m_top = (Int32)MRound( (Float)rect.m_top * scale );
		outColorRect->m_bottom = outColorRect->m_top + (Int32)MRound( (Float)rect.Height() * scale );
	}

	Float divisor = ( Float )( 1 << m_colormapStartingMip );

	// Determine which tiles contribute to the queried area
	const Int32 firstTileGridCol = brushRectClipmapSpace.m_left < 0 ? 0 : brushRectClipmapSpace.m_left / (Int32)m_tileRes;
	const Int32 firstTileGridRow = brushRectClipmapSpace.m_top  < 0 ? 0 : brushRectClipmapSpace.m_top / (Int32)m_tileRes;
	Int32 tileGridCol = firstTileGridCol;
	Int32 tileGridRow = firstTileGridRow;

	// Compute the offset in the left-most and top-most tiles of the contributing tiles set.
	Int32 firstTexelInTileV = brushRectClipmapSpace.m_top	< 0 ? 0	: brushRectClipmapSpace.m_top	% (Int32)m_tileRes;

	// Start counting the texels remaining
	Int32 texelsLeftToProcessV = brushRectClipmapSpace.Height();
	if ( brushRectClipmapSpace.m_bottom >= (Int32)(m_numTilesPerEdge*m_tileRes) )
	{
		texelsLeftToProcessV -= brushRectClipmapSpace.m_bottom - m_numTilesPerEdge*m_tileRes + 1;
	}


	Int32 firstCTexelInTileV = (Int32)MRound( firstTexelInTileV / divisor );
	Int32 ctexelsLeftToProcessV = (Int32)MRound( texelsLeftToProcessV / divisor );

	// Start counting the texels already processed
	Int32 texelsProcessedV = brushRectClipmapSpace.m_top < 0 ? -brushRectClipmapSpace.m_top : 0;
	Int32 ctexelsProcessedV = (Int32)MRound( texelsProcessedV / divisor );

	texelsLeftToProcessV -= texelsProcessedV;
	ctexelsLeftToProcessV -= ctexelsProcessedV;

	while ( texelsLeftToProcessV > 0 ) // rows
	{
		Uint32 numTexelsToProcessFromTileV = Min( (Int32)m_tileRes - firstTexelInTileV, texelsLeftToProcessV );
		Int32 firstTexelInTileU = brushRectClipmapSpace.m_left < 0 ? 0 : brushRectClipmapSpace.m_left % (Int32)m_tileRes;
		Int32 texelsLeftToProcessU = brushRectClipmapSpace.Width();
		if ( brushRectClipmapSpace.m_right >= (Int32)(m_numTilesPerEdge*m_tileRes) )
		{
			texelsLeftToProcessU -= brushRectClipmapSpace.m_right - m_numTilesPerEdge*m_tileRes + 1;
		}

		Uint32 numCTexelsToProcessFromTileV = Min( (Int32)highestColorMapResolution - firstCTexelInTileV, ctexelsLeftToProcessV );
		Int32 firstCTexelInTileU = (Int32)MRound( firstTexelInTileU / divisor );
		Int32 ctexelsLeftToProcessU = (Int32)MRound( texelsLeftToProcessU / divisor );

		Int32 texelsProcessedU = brushRectClipmapSpace.m_left < 0 ? -brushRectClipmapSpace.m_left : 0;
		Int32 ctexelsProcessedU = (Int32)MRound( texelsProcessedU / divisor );

		texelsLeftToProcessU -= texelsProcessedU;
		ctexelsLeftToProcessU -= ctexelsProcessedU;

		while ( texelsLeftToProcessU > 0 ) // cols
		{
			Uint32 numTexelsToProcessFromTileU = Min( (Int32)m_tileRes - firstTexelInTileU, texelsLeftToProcessU );
			Uint32 numCTexelsToProcessFromTileU = Min( (Int32)highestColorMapResolution - firstCTexelInTileU, ctexelsLeftToProcessU );

			CTerrainTile* tile = GetTile( tileGridCol, tileGridRow );
			ASSERT( tile );
			if ( tile )
			{
				// Add new part to the result
				parts.Grow();
				SClipmapHMQueryResult& part = parts.Back();

				part.m_row = tileGridRow;
				part.m_col = tileGridCol;
				part.m_selectionSubRect = Rect( texelsProcessedU, texelsProcessedU + numTexelsToProcessFromTileU,
					texelsProcessedV, texelsProcessedV + numTexelsToProcessFromTileV);
				part.m_addressingRect = Rect( firstTexelInTileU, firstTexelInTileU + numTexelsToProcessFromTileU,
					firstTexelInTileV, firstTexelInTileV + numTexelsToProcessFromTileV );

				part.m_colorSelectionSubRect = Rect( ctexelsProcessedU, ctexelsProcessedU + numCTexelsToProcessFromTileU,
					ctexelsProcessedV, ctexelsProcessedV + numCTexelsToProcessFromTileV);
				part.m_colorAddressingRect = Rect( firstCTexelInTileU, firstCTexelInTileU + numCTexelsToProcessFromTileU,
					firstCTexelInTileV, firstCTexelInTileV + numCTexelsToProcessFromTileV );

				part.m_tile = tile;
			}

			++tileGridCol;
			texelsLeftToProcessU -= numTexelsToProcessFromTileU;
			texelsProcessedU += numTexelsToProcessFromTileU;
			firstTexelInTileU = 0;

			ctexelsLeftToProcessU = Max< Int32 >( ctexelsLeftToProcessU - numCTexelsToProcessFromTileU, 0 );
			ctexelsProcessedU += numCTexelsToProcessFromTileU;
			firstCTexelInTileU = 0;
		}
		tileGridCol = firstTileGridCol;
		++tileGridRow;
		texelsProcessedV += numTexelsToProcessFromTileV;
		texelsLeftToProcessV -= numTexelsToProcessFromTileV;
		firstTexelInTileV = 0;

		ctexelsProcessedV += numCTexelsToProcessFromTileV;
		ctexelsLeftToProcessV = Max< Int32 >( ctexelsLeftToProcessV - numCTexelsToProcessFromTileV, 0 );
		firstCTexelInTileV = 0;
	}
}

Box CClipMap::GetBoxForTile( Int32 x, Int32 y, Float z ) const
{
	Vector vmin( m_terrainCorner.X, m_terrainCorner.Y, z );
	vmin.X += x * ( m_terrainSize / m_numTilesPerEdge );
	vmin.Y += y * ( m_terrainSize / m_numTilesPerEdge );
	Vector vmax( m_terrainCorner.X, m_terrainCorner.Y, z + 100.0f );
	vmax.X += (x + 1) * ( m_terrainSize / m_numTilesPerEdge );
	vmax.Y += (y + 1) * ( m_terrainSize / m_numTilesPerEdge );
	return Box( vmin, vmax );
}

Float CClipMap::GetTileMaximumHeight( CTerrainTile* tile ) const
{
	Uint16 maxHeightValue = tile->GetMaximumHeight();
	return TexelHeightToHeight( maxHeightValue );
}

Float CClipMap::GetTileMinimumHeight( CTerrainTile* tile ) const
{
	Uint16 minHeightValue = tile->GetMinimumHeight();
	return TexelHeightToHeight( minHeightValue );
}


Uint32 CClipMap::ComputeNumberOfClipmapStackLevels( Uint32 clipmapSize, Uint32 clipSize )
{
	Uint32 tempSize = clipmapSize;
	Uint32 numClipmapStackLevels = 0;
	
	while ( tempSize > clipSize )
	{
		tempSize /= 2;
		++numClipmapStackLevels;
	}

	return numClipmapStackLevels;
}


Bool CClipMap::ShouldSampleFromTile( CTerrainTile* tile, ETerrainBufferType bufferType ) const
{
	return !IsCooked() || tile->HasDataLoaded( bufferType );
}


Bool CClipMap::GetHeightForWorldPosition( const Vector& position, Float& height, Uint16* texelHeight ) const
{
	// setup default values
	height = position.Z;
	if ( texelHeight )
	{
		*texelHeight = HeightToTexelHeight( height );
	}
	Vector2 tilePosition;
	CTerrainTile* tile = WorldPositionToTile( position, &tilePosition );
	if ( tile != nullptr )
	{
		if ( ShouldSampleFromTile( tile, TBT_HeightMap ) )
		{
			return tile->GetHeightForLocalPosition( this, tilePosition, height, texelHeight );
		}
		else
		{
			const Uint16* data = static_cast< const Uint16* >( m_cookedMipStackHeightHandle->GetData() );
			RED_FATAL_ASSERT( data, "m_cookedMipStackHeightHandle has not been initialised!" );

			const Uint32 mipMapStackTileRes = m_tileRes >> m_numClipmapStackLevels;
			const Uint32 cookedDataRes = m_numTilesPerEdge * mipMapStackTileRes;

			const Vector2 texelPos = GetTexelSpaceNormalizedPosition( position, true ) * (Float)cookedDataRes;
			const Uint32 texelXMin = Clamp<Uint32>( Uint32( texelPos.X ), 0, cookedDataRes - 1 );
			const Uint32 texelYMin = Clamp<Uint32>( Uint32( texelPos.Y ), 0, cookedDataRes - 1 );

			const Uint16 h = data[ texelYMin * cookedDataRes + texelXMin ];

			if ( texelHeight ) *texelHeight = h;
			height = TexelHeightToHeight( h );

			return false;
		}
	}
	return false;
}

void CClipMap::GetHeightForWorldPositionSync( const Vector& position, Uint32 level, Float& height, Uint16* texelHeight /*= nullptr*/ ) const
{
	// setup default values
	height = position.Z;
	if ( texelHeight )
	{
		*texelHeight = HeightToTexelHeight( height );
	}

	Vector2 tilePosition;
	CTerrainTile* tile = WorldPositionToTile( position, &tilePosition );
	if ( tile != nullptr )
	{
		tile->GetHeightForLocalPositionSync( this, tilePosition, level, height, texelHeight );
	}
}

TControlMapType CClipMap::GetControlmapValueForWorldPosition( const Vector& position ) const
{
	Vector2 tilePosition;
	CTerrainTile* tile = WorldPositionToTile( position, &tilePosition );
	if ( tile != nullptr )
	{
		if ( ShouldSampleFromTile( tile, TBT_ControlMap ) )
		{
			return tile->GetControlmapValueForLocalPosition( this, tilePosition );
		}
		else
		{
			const TControlMapType* data = static_cast< const TControlMapType* >( m_cookedMipStackControlHandle->GetData() );
			RED_FATAL_ASSERT( data, "m_cookedMipStackControlHandle has not been initialised!" );

			const Uint32 mipMapStackTileRes = m_tileRes >> m_numClipmapStackLevels;
			const Uint32 cookedDataRes = m_numTilesPerEdge * mipMapStackTileRes;

			const Vector2 texelPos = GetTexelSpaceNormalizedPosition( position, true ) * (Float)cookedDataRes;
			const Uint32 texelXMin = Clamp<Uint32>( Uint32( texelPos.X ), 0, cookedDataRes - 1 );
			const Uint32 texelYMin = Clamp<Uint32>( Uint32( texelPos.Y ), 0, cookedDataRes - 1 );

			return data[ texelYMin * cookedDataRes + texelXMin ];
		}
	}

	return 0;
}

TControlMapType CClipMap::GetControlmapValueForWorldPositionSync( const Vector& position, Uint32 level ) const
{
	Vector2 tilePosition;
	CTerrainTile* tile = WorldPositionToTile( position, &tilePosition );
	if ( tile != nullptr )
	{
		return tile->GetControlmapValueForLocalPositionSync( this, tilePosition, level );
	}

	return 0;
}

TColorMapType CClipMap::GetColormapValueForWorldPosition( const Vector& position ) const
{
	Vector2 tilePosition;
	CTerrainTile* tile = WorldPositionToTile( position, &tilePosition );
	if ( tile != nullptr )
	{
		if ( ShouldSampleFromTile( tile, TBT_ColorMap ) )
		{
			return tile->GetColormapValueForLocalPosition( this, tilePosition );
		}
		else
		{
			const TColorMapRawType* data = static_cast< const TColorMapRawType* >( m_cookedMipStackColorHandle->GetData() );
			RED_FATAL_ASSERT( data, "m_cookedMipStackColorHandle has not been initialised!" );

			const Uint32 mipMapStackTileRes	= m_tileRes >> m_numClipmapStackLevels;
			const Uint32 cookedDataRes		= m_numTilesPerEdge * mipMapStackTileRes;
			const Uint32 blockRes			= TerrainUtils::ColorMapTexelToBlock( cookedDataRes );

			const Vector2 texelPos = GetTexelSpaceNormalizedPosition( position, true ) * (Float)cookedDataRes;
			const Uint32 texelXMin = Clamp<Uint32>( Uint32( texelPos.X ), 0, cookedDataRes - 1 );
			const Uint32 texelYMin = Clamp<Uint32>( Uint32( texelPos.Y ), 0, cookedDataRes - 1 );

			const Uint32 blockX		= TerrainUtils::ColorMapTexelToBlock( texelXMin );
			const Uint32 blockY		= TerrainUtils::ColorMapTexelToBlock( texelYMin );
			const Uint32 subTexelX	= TerrainUtils::ColorMapTexelToSubTexel( texelXMin );
			const Uint32 subTexelY	= TerrainUtils::ColorMapTexelToSubTexel( texelYMin );

			return TerrainUtils::DecodeColorMap( data[ blockY * blockRes + blockX ], subTexelX, subTexelY );
		}
	}

	return 0;
}

TColorMapType CClipMap::GetColormapValueForWorldPositionSync( const Vector& position, Uint32 level ) const
{
	Vector2 tilePosition;
	CTerrainTile* tile = WorldPositionToTile( position, &tilePosition );
	if ( tile != nullptr )
	{
		return tile->GetColormapValueForLocalPositionSync( this, tilePosition, level );
	}

	return 0;
}


#ifndef NO_HEIGHTMAP_EDIT
Vector CClipMap::GetNormalForWorldPosition( const Vector& position ) const
{
	TDynArray< Float > heights = ComputeHeightsAroundWorldPosition( position );

	//			<v0>	<v1>
	//
	//	<v2>	<v3>	<v4>	<v5>
	//				 <p>
	//	<v6>	<v7>	<v8>	<v9>
	//
	//			<v10>	<v11>

	Vector v3L = Vector( -1.0f,  0.0f, heights[2] - heights[3] ).Normalized3();
	Vector v3R = Vector(  1.0f,  0.0f, heights[4] - heights[3] ).Normalized3();
	Vector v3T = Vector(  0.0f,  1.0f, heights[0] - heights[3] ).Normalized3();
	Vector v3B = Vector(  0.0f, -1.0f, heights[7] - heights[3] ).Normalized3();

	Vector n3TL = Vector::Cross( v3T, v3L ).Normalized3();
	Vector n3TR = Vector::Cross( v3R, v3T ).Normalized3();
	Vector n3BR = Vector::Cross( v3B, v3R ).Normalized3();
	Vector n3BL = Vector::Cross( v3L, v3B ).Normalized3();

	Vector n3 = ( n3TL + n3TR + n3BR + n3BL ) / 4.0f;

	Vector v4L = Vector( -1.0f,  0.0f, heights[3] - heights[4] ).Normalized3();
	Vector v4R = Vector(  1.0f,  0.0f, heights[5] - heights[4] ).Normalized3();
	Vector v4T = Vector(  0.0f,  1.0f, heights[1] - heights[4] ).Normalized3();
	Vector v4B = Vector(  0.0f, -1.0f, heights[8] - heights[4] ).Normalized3();

	Vector n4TL = Vector::Cross( v4T, v4L ).Normalized3();
	Vector n4TR = Vector::Cross( v4R, v4T ).Normalized3();
	Vector n4BR = Vector::Cross( v4B, v4R ).Normalized3();
	Vector n4BL = Vector::Cross( v4L, v4B ).Normalized3();

	Vector n4 = ( n4TL + n4TR + n4BR + n4BL ) / 4.0f;

	Vector v7L = Vector( -1.0f,  0.0f, heights[6] - heights[7] ).Normalized3();
	Vector v7R = Vector(  1.0f,  0.0f, heights[8] - heights[7] ).Normalized3();
	Vector v7T = Vector(  0.0f,  1.0f, heights[3] - heights[7] ).Normalized3();
	Vector v7B = Vector(  0.0f, -1.0f, heights[10] - heights[7] ).Normalized3();

	Vector n7TL = Vector::Cross( v7T, v7L ).Normalized3();
	Vector n7TR = Vector::Cross( v7R, v7T ).Normalized3();
	Vector n7BR = Vector::Cross( v7B, v7R ).Normalized3();
	Vector n7BL = Vector::Cross( v7L, v7B ).Normalized3();

	Vector n7 = ( n7TL + n7TR + n7BR + n7BL ) / 4.0f;

	Vector v8L = Vector( -1.0f,  0.0f, heights[7] - heights[8] ).Normalized3();
	Vector v8R = Vector(  1.0f,  0.0f, heights[9] - heights[8] ).Normalized3();
	Vector v8T = Vector(  0.0f,  1.0f, heights[4] - heights[8] ).Normalized3();
	Vector v8B = Vector(  0.0f, -1.0f, heights[11] - heights[8] ).Normalized3();

	Vector n8TL = Vector::Cross( v8T, v8L ).Normalized3();
	Vector n8TR = Vector::Cross( v8R, v8T ).Normalized3();
	Vector n8BR = Vector::Cross( v8B, v8R ).Normalized3();
	Vector n8BL = Vector::Cross( v8L, v8B ).Normalized3();

	Vector n8 = ( n8TL + n8TR + n8BR + n8BL ) / 4.0f;

	return ( n3 + n4 + n7 + n8 ) / 4.0f;
}

void CClipMap::GetHeightForTexelPosition( const Vector2& position, Float& height, Uint16* texelHeight ) const
{
	Int32 u = (Int32)position.X;
	Int32 v = (Int32)position.Y;
	CTerrainTile* tile = GetTile( u / m_tileRes, v / m_tileRes );
	if ( tile )
	{
		Uint32 level = 0;
		const Uint16* tileTexels = tile->GetAnyLevelHM( level );

		if ( tileTexels )
		{
			Vector2 tileUV;
			tileUV.X = MFract( position.X / m_tileRes );
			tileUV.Y = MFract( position.Y / m_tileRes );
			CTerrainTile::GetHeightFromHeightmap( tileTexels, tileUV, 1.0f, m_tileRes, m_lowestElevation, m_highestElevation - m_lowestElevation, height, texelHeight );
		}
		else
		{
			// return default values
			height = (m_highestElevation + m_lowestElevation) * 0.5f;
			if ( texelHeight )
			{
				*texelHeight = 32767; // 65535 (max of Uint16) / 2
			}
		}
	}
}
#endif

Bool CClipMap::GetTilesInWorldArea( const Vector2& vmin, const Vector2& vmax, Rect& out ) const
{
	Vector p00( vmin.X, vmin.Y, 0.0f );
	Vector p01( vmax.X, vmin.Y, 0.0f );
	Vector p10( vmin.X, vmax.Y, 0.0f );
	Vector p11( vmax.X, vmax.Y, 0.0f );

	Int32 minX = INT_MAX;
	Int32 maxX = INT_MIN;
	Int32 minY = INT_MAX;
	Int32 maxY = INT_MIN;

	Int32 x, y;
	GetTileFromPosition( p00, x, y, false );
	minX = Min<Int32>( minX, x ); minY = Min<Int32>( minY, y );
	maxX = Max<Int32>( maxX, x ); maxY = Max<Int32>( maxY, y );
	GetTileFromPosition( p01, x, y, false );
	minX = Min<Int32>( minX, x ); minY = Min<Int32>( minY, y );
	maxX = Max<Int32>( maxX, x ); maxY = Max<Int32>( maxY, y );
	GetTileFromPosition( p10, x, y, false );
	minX = Min<Int32>( minX, x ); minY = Min<Int32>( minY, y );
	maxX = Max<Int32>( maxX, x ); maxY = Max<Int32>( maxY, y );
	GetTileFromPosition( p11, x, y, false );
	minX = Min<Int32>( minX, x ); minY = Min<Int32>( minY, y );
	maxX = Max<Int32>( maxX, x ); maxY = Max<Int32>( maxY, y );

	out = Rect( minX, maxX, minY, maxY );
	Bool res = out.Intersects( Rect( 0, m_numTilesPerEdge, 0, m_numTilesPerEdge ) );
	return res;
}

Float CClipMap::TexelHeightToHeight( Uint16 texelHeight ) const
{
	Float heightRange = m_highestElevation - m_lowestElevation;
	return m_lowestElevation + heightRange * ( (Float)texelHeight / 65535.f );
}

Uint16 CClipMap::HeightToTexelHeight( Float height ) const
{
	Float heightRange = m_highestElevation - m_lowestElevation;
	Float texelHeight = ( height - m_lowestElevation ) / heightRange;
	texelHeight *= 65535.0f;
	return (Uint16)texelHeight;
}

Bool CClipMap::Intersect( const Vector& rayOrigin, const Vector& rayDirection, Vector& intersection ) const 
{
	// Nothing to intersect with, fail
	if ( m_tileRes == 0 || m_terrainSize == 0.0f )
	{
		return false;
	}

	//Pseudo
	// intersect with lowest resolution and than refine
	// get first point where the ray is under the terrain (try to make this as accurate as possible)
	// with this point calculate the intersection on the pixel plane
	// with this intersection point refine on the higher resolution textures

	Vector direction = rayDirection.Normalized3();
	Vector pos = rayOrigin;
	Vector prevPos = rayOrigin;

	const Float RAYLENGTH = 500.f + ( 1.0f - direction.Z ) * 1000.0f;
	const Float DELTA_IN_METERS = 3.0f - direction.Z;
	const Uint32 STEPCOUNT = (Uint32)( RAYLENGTH / DELTA_IN_METERS );

	Float height = 0.f;
	Bool hitted = false;

	for ( Uint32 i = 0; i < STEPCOUNT; ++i )
	{
		// TODO: Here might be some further optimizations to skip whole terrain tile depend of highest position

		GetHeightForWorldPosition(pos, height );
		if (height > pos.Z)
		{
			//this point is under
			hitted = ( i > 0 );
			break;
		}
		prevPos = pos;
		pos = pos + direction * DELTA_IN_METERS;
	}

	if( !hitted )
	{
		return false;
	}

	// bisection, until precision is reached
	const static Uint32	BISECTION_STEPS = 100;
	const static Float	DISQUALIFYING_DIFF = 10.f;
	const Float precision = ::Max( ( m_highestElevation - m_lowestElevation ) / 65536.f , 0.01f );
	Vector halfPos = pos;
	Float halfHeight = height;
	Uint32 j = 0;
	do
	{
		halfPos = prevPos + ( pos - prevPos ) / 2.f;
		GetHeightForWorldPosition( halfPos, halfHeight );
		
		if ( halfHeight > halfPos.Z )
		{
			// intersection between prevPos and halfPos
			pos = halfPos;
		}
		else
		{
			// intersection between halfPos and pos
			prevPos = halfPos;
		}
	}
	while ( MAbs( halfPos.Z - halfHeight ) > precision && j++ < BISECTION_STEPS );
	
	if ( (halfPos - rayOrigin).Mag3() >= RAYLENGTH || MAbs( halfPos.Z - halfHeight ) >= DISQUALIFYING_DIFF )
	{
		return false;
	}

	intersection = halfPos;
	return true;
}

void CClipMap::GenerateCollisionForTile( Uint32 row, Uint32 col )
{
	CTerrainTile* tile = m_terrainGrid[ row * m_numTilesPerEdge + col ].Get();

	// No collision needed for this tile.
	if ( !tile->IsCollisionEnabled() )
	{
		return;
	}

#ifndef NO_EDITOR
	// If we're currently being edited, defer the collision gen until it's done.
	if ( IsEditing() )
	{
		tile->RequestCollisionGen();
		return;
	}
#endif

	Box collisionTileBB = GetBoxForTile( col, row, 0.f );

	collisionTileBB.Min.Z = m_lowestElevation;
	collisionTileBB.Max.Z = m_highestElevation;

	CTerrainTile* nextRowTile = NULL;
	if ( row < m_numTilesPerEdge - 1 )
	{
		nextRowTile = m_terrainGrid[ (row + 1) * m_numTilesPerEdge + col ].Get();
	}

	CTerrainTile* nextColTile = NULL;
	if ( col < m_numTilesPerEdge - 1 )
	{
		nextColTile = m_terrainGrid[ row * m_numTilesPerEdge + (col + 1) ].Get();
	}

	CTerrainTile* nextRowColTile = NULL;
	if ( row < m_numTilesPerEdge - 1 && col < m_numTilesPerEdge - 1 )
	{
		nextRowColTile = m_terrainGrid[ (row + 1 ) * m_numTilesPerEdge + (col + 1)].Get();
	}

	Uint32 mipMapStackTileRes = m_tileRes >> m_numClipmapStackLevels;

	TDynArray< CName > textureNames;
	if ( m_material )
	{
		THandle< CTextureArray > texture;
		if ( m_material->ReadParameter( CNAME(diffuse), texture ) )
		{
			if( texture.Get() )
			{
				texture.Get()->GetTextureNames( textureNames );
			}
		}
	}

	tile->GenerateCollision( this, GetWorld(), collisionTileBB, textureNames, m_textureParams, nextRowTile, nextColTile, nextRowColTile );
}

void CClipMap::InvalidateCollision() const
{
	for ( Uint32 i=0; i<m_terrainGrid.Size(); ++i )
	{
		if( m_terrainGrid[ i ] )
		{
			m_terrainGrid[ i ]->InvalidateCollision();
		}
		else
		{
			RED_ASSERT( false, TXT( "Probably terrain grid was still supposed to be valid at this point?" ) );
		}
	}
}

#ifdef USE_ANSEL
Bool CClipMap::EnsureCollisionIsGenerated( const Vector& position )
{
	Int32 clipCenterCol, clipCenterRow;
	Rect clipRegion;

	Rect clipRegionInLevel0Space;
	clipRegionInLevel0Space = GetClipWindowRect( position, 0, &clipRegion, &clipCenterCol, &clipCenterRow );

	TileCoords coords( clipRegionInLevel0Space, static_cast< Int32 >( m_tileRes ) );

	for ( coords.m_row.Reset(); coords.m_row.IsValid(); ++coords.m_row )
	{
		for ( coords.m_column.Reset(); coords.m_column.IsValid(); ++coords.m_column )
		{
			const Uint32 row = coords.m_row.m_current;
			const Uint32 col = coords.m_column.m_current;

			CTerrainTile* tile = m_terrainGrid[ row * m_numTilesPerEdge + col ].Get();
			if ( !tile || !tile->IsCollisionEnabled() )
			{
				continue;
			}
			if ( tile->IsCollisionGenerated() )
			{
				continue;
			}

			GenerateCollisionForTile( row, col );

			if ( !tile->IsCollisionGenerated() )
			{
				return false;
			}
		}
	}

	return true;
}
#endif // USE_ANSEL

void CClipMap::ToggleLoading( const Bool allowLoading )
{
	m_enableLoading = allowLoading;
}

#ifdef USE_ANSEL
extern EAnselState anselState;
#endif // USE_ANSEL

void CClipMap::Update( const SClipMapUpdateInfo& updateInfo )
{
	if ( !m_enableLoading )
	{
		return;
	}

#ifdef USE_ANSEL
	if ( anselState == EAS_FirstFrameOfDeactivation )
	{
		InvalidateCollision();
	}
#endif // USE_ANSEL

	// do not update if streaming is disabled
	if ( m_renderProxy && !Config::cvTerrainStreamingEnabled.Get() )
		return;

	Bool firstframe = false;
	if ( !m_renderProxy )
	{
		if ( m_colormapStartingMip == -1 )
		{
			// calculate starting mip of colormap
			m_colormapStartingMip = CalculateColormapStartingMip( m_numClipmapStackLevels, m_numTilesPerEdge, m_tileRes );
		}

#ifndef NO_HEIGHTMAP_EDIT
		// Perform tiles validity check
		for ( Uint32 i=0; i<m_terrainGrid.Size(); ++i )
		{
			if ( !m_terrainGrid[i] )
			{
				RED_LOG( RED_LOG_CHANNEL( TerrainClipMaps ), TXT("Oops. Some terrain tiles are missing!") );
				// Just return from here every frame.
				return;
			}
		}
#endif

		ASSERT( m_numClipmapStackLevels == m_levels.Size() );
		firstframe = true;

		m_clipmapUpdates.ClearFast();
		m_clipmapUpdates.Resize( m_levels.Size() + 1 );

		// Fill clipmap stack levels
		for ( Uint32 l=0; l<m_levels.Size(); ++l )
		{
			// Just set the resolution, and read nothing else in the proxy ctor
			m_clipmapUpdates[l] = new SClipmapLevelUpdate( l, m_levels[l].m_resolutionOfTheClipRegion );
		}

		// Fill mipmap stack level
		Uint32 mipmapLevelRes = m_clipmapSize;
		if ( !m_levels.Empty() )
		{
			mipmapLevelRes = m_levels.Back().m_resolutionOfTheLevel / 2;
		}
		m_clipmapUpdates[ m_numClipmapStackLevels ] = new SClipmapLevelUpdate( m_numClipmapStackLevels, mipmapLevelRes );

		// Create update object for the initial proxy setup
		SClipmapStampDataUpdate* stampUpdate = NULL;
#ifndef NO_HEIGHTMAP_EDIT
		if ( m_stampGizmoVisible && m_stampDataUpdate && m_stampDataUpdate->IsValid() )
		{
			stampUpdate = m_stampDataUpdate;
		}
#endif
		Vector colormapParams( (Float)m_colormapStartingMip, 0.0f, 0.0f );
		IRenderObject* renderUpdateData = GRender->CreateTerrainUpdateData( m_clipmapUpdates, NULL, NULL, stampUpdate, &colormapParams );
		ASSERT( renderUpdateData );

		// Create proxy itself
		SClipmapParameters params;
		GetClipmapParameters( &params );
		m_renderProxy = GRender->CreateTerrainProxy( renderUpdateData, params );
		ASSERT( m_renderProxy );
		ASSERT( GetWorld() );

		// No need for the update object anymore
		renderUpdateData->Release();
		renderUpdateData = NULL;

		if ( !m_minWaterHeight.Empty() )
		{
			RED_ASSERT( m_minWaterHeight.Size() == m_terrainGrid.Size(), TXT("Number of min water levels (%u) != number of tiles (%u)"), m_minWaterHeight.Size(), m_terrainGrid.Size() );
			( new CRenderCommand_UpdateTerrainWaterLevels( m_renderProxy, m_minWaterHeight ) )->Commit();
		}
		if ( !m_tileHeightRanges.Empty() )
		{
			RED_ASSERT( m_tileHeightRanges.Size() == m_terrainGrid.Size(), TXT("Number of height ranges (%u) != number of tiles (%u)"), m_tileHeightRanges.Size(), m_terrainGrid.Size() );
			( new CRenderCommand_UpdateTerrainTileHeightRanges( m_renderProxy, m_tileHeightRanges ) )->Commit();
		}


		// Attach proxy to the scene
		( new CRenderCommand_SetTerrainProxyToScene( GetWorld()->GetRenderSceneEx(), m_renderProxy ) )->Commit();

		// Start rendering grass coverage
		UpdateGrassRendering();
	}

	Box terrainCollisionBounding( Box::RESET_STATE );
	terrainCollisionBounding.Min.Z = -FLT_MAX;
	terrainCollisionBounding.Max.Z = FLT_MAX;

	// Update each level
	m_clipmapUpdates.ClearFast();
	UpdateEachLevel(updateInfo, firstframe, terrainCollisionBounding, m_clipmapUpdates);

	// Now update the mipmap stack. That requires fetching lowest level data from all tiles.
	UpdateMipmapStack(firstframe, terrainCollisionBounding, updateInfo, m_clipmapUpdates);

	// update collision boundings
	if ( terrainCollisionBounding.Max.X > terrainCollisionBounding.Min.X )
	{
		CWorld* world = GetWorld();
		world->OnTerrainCollisionDataBoundingUpdated( terrainCollisionBounding );
	}

	// With no editor, or on console, we don't need to worry about changes to material or other parameters.
	// Only update if we actually have something to update.
#if defined( NO_EDITOR ) || defined( RED_PLATFORM_CONSOLE )
	if ( !m_clipmapUpdates.Empty() )
#endif
	{
		// Create update object for the render side
		IRenderObject* renderMaterial = NULL;
		if ( m_material )
		{
			renderMaterial = m_material->GetRenderResource();
		}

		SClipmapStampDataUpdate* stampUpdate = NULL;
#ifndef NO_HEIGHTMAP_EDIT
		if ( m_stampGizmoVisible && m_stampDataUpdate && m_stampDataUpdate->IsValid() )
		{
			stampUpdate = m_stampDataUpdate;
		}
#endif
		Vector colormapParams( (Float)m_colormapStartingMip, 0.0f, 0.0f );

		IRenderObject* renderUpdateData = nullptr;

#ifndef NO_EDITOR
		if ( m_isEditing )
		{
			renderUpdateData = GRender->CreateEditedTerrainUpdateData( m_clipmapUpdates, renderMaterial, &m_textureParams[0], stampUpdate, &colormapParams );
		}
		else
#endif
		{
			renderUpdateData = GRender->CreateTerrainUpdateData( m_clipmapUpdates, renderMaterial, &m_textureParams[0], stampUpdate, &colormapParams );
		}
		ASSERT( renderUpdateData );

		// Submit update data to the render side
		( new CRenderCommand_UpdateClipmap( m_renderProxy, renderUpdateData ) )->Commit();

		if ( stampUpdate )
		{
			stampUpdate->m_heightDataDirty = false;
			stampUpdate->m_colorDataDirty = false;
			stampUpdate->m_controlDataDirty = false;
		}

		renderUpdateData->Release();
	}
	firstframe = false;
}

void CClipMap::SortMipsForEviction( TDynArray< STerrainMipmapEvictionTracker, MC_TerrainClipmap >& evictionQueue )
{
	MipsSortPred sortMipsByAgeAndSize;
	Sort( evictionQueue.Begin(), evictionQueue.End(), sortMipsByAgeAndSize );
}

void CClipMap::EvictOldMips( const SClipMapUpdateInfo& updateInfo, TDynArray< STerrainMipmapEvictionTracker, MC_TerrainClipmap >& evictionQueue, Red::System::MemSize footprint, Red::System::MemSize maxFootprint )
{
	// Evict all mips that definitely need unloading, then any others that push us over our limit
	Uint32 evictedCount = 0;
	Red::System::MemSize bytesEvicted = 0;

	for( Uint32 evictCounter = 0; evictCounter < evictionQueue.Size(); ++evictCounter )
	{
		STerrainMipmapEvictionTracker& tracker = evictionQueue[ evictCounter ];

		// Check to see if it's been a while since this mip was used
		// OR if we're aggressively evicting tiles, those which weren't touched this update loop
		if( tracker.m_timeRemaining <= 0.0f || ( updateInfo.m_aggressiveEviction && !tracker.m_touchedThisUpdateTick ) )
		{
#ifdef RED_LOGGING_ENABLED
			if( updateInfo.m_aggressiveEviction && !tracker.m_touchedThisUpdateTick )
			{
				RED_LOG( RED_LOG_CHANNEL( TerrainClipMaps ), TXT( "Aggressively evicting mip (level %u) for tile '%ls' Time remaining: % .3f, Memory: %" ) RED_PRIWsize_t, tracker.m_ownerTile->GetMipLevel( tracker.m_mipMap ), tracker.m_ownerTile->GetDepotPath().AsChar(), tracker.m_timeRemaining, tracker.m_mipMap->GetResidentMemory() );
			}
#endif

			footprint -= tracker.m_dataSize;
			bytesEvicted += tracker.m_dataSize;
			++evictedCount;
			tracker.m_ownerTile->EvictMipmapData( tracker.m_mipMap );
			continue;
		}
		else
		{
			if( footprint < maxFootprint )
			{
				break;		// We are in budget, stop trying to evict mips!
			}

			// By this point we are over budget; start dropping old mips
			RED_LOG( RED_LOG_CHANNEL( TerrainClipMaps ), TXT( "Clipmap memory footprint too big (%u bytes). Dropping %u bytes" ), footprint, tracker.m_dataSize );
			footprint -= tracker.m_dataSize;
			bytesEvicted += tracker.m_dataSize;
			++evictedCount;
			tracker.m_ownerTile->EvictMipmapData( tracker.m_mipMap );
		}
	}
}

#ifndef NO_HEIGHTMAP_EDIT
TDynArray< Float > CClipMap::ComputeHeightsAroundWorldPosition( const Vector& position ) const
{
	TDynArray< Float > res;
	res.Resize(12);
	if ( m_tileRes == 0 || m_terrainSize == 0.0f )
	{
		ASSERT( false, TXT("tileRes or terrainSize wrong, There will be crash or data will be fucked up") );
		return res;
	}
	
	Vector2 viewPosTerrainSpaceNorm = GetTexelSpaceNormalizedPosition( position, false );
	if ( viewPosTerrainSpaceNorm.X < 0.0f || viewPosTerrainSpaceNorm.X > 1.0f || viewPosTerrainSpaceNorm.Y < 0.0f || viewPosTerrainSpaceNorm.Y > 1.0f )
	{
		ASSERT( false, TXT("invalid position, There will be crash or data will be fucked up") );
		return res;
	}

	// We will need clip region for each level, mapped to the level0 dimensions, for determining the contributing set of tiles
	Int32 clipCenterLevel0SpaceCol = (Int32)( viewPosTerrainSpaceNorm.X * ( m_clipmapSize - 1 ) );
	Int32 clipCenterLevel0SpaceRow = (Int32)( viewPosTerrainSpaceNorm.Y * ( m_clipmapSize - 1 ) );

	Int32 dx[] = { 0, 1, -1, 0, 1, 2, -1, 0, 1, 2, 0, 1 };
	Int32 dy[] = { 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, -1, -1 };
	//			<v0>	<v1>
	//
	//	<v2>	<v3>	<v4>	<v5>
	//				 <p>
	//	<v6>	<v7>	<v8>	<v9>
	//
	//			<v10>	<v11>

	for ( Uint32 i = 0; i < 12; ++i )
	{
		Int32 pX = Clamp<Int32>( clipCenterLevel0SpaceCol + dx[i], 0, m_clipmapSize - 1 );
		Int32 pY = Clamp<Int32>( clipCenterLevel0SpaceRow + dy[i], 0, m_clipmapSize - 1 );

		Int32 tileX = pX / m_tileRes;
		Int32 tileY = pY / m_tileRes;

		Int32 X = pX - tileX * m_tileRes;
		Int32 Y = pY - tileY * m_tileRes;

		CTerrainTile* tile = m_terrainGrid[ m_numTilesPerEdge * tileY + tileX ].Get();
		if ( !tile )
		{
			ASSERT( false, TXT("invalid tile, There will be crash or data will be fucked up") );
			return res;
		}

		const Uint16* tileTexels = tile->GetLevelSyncHM( 0 );
		if ( !tileTexels )
		{
			ASSERT( false, TXT("no tile data, There will be crash or data will be fucked up") );
			return res;
		}

		Uint16 height = tileTexels[ Y * m_tileRes + X ];

		// renormalize height and get the real value
		Float heightRange = m_highestElevation - m_lowestElevation;
		Float heightNorm = m_lowestElevation + heightRange * ( (Float)height / 65536.f );

		res[i] = heightNorm;
	}

	return res;
}

void CClipMap::UnloadAll()
{
#ifndef NO_HEIGHTMAP_EDIT
	for ( Uint32 i = 0; i < m_terrainGrid.Size(); ++i )
	{
		if ( m_terrainGrid[ i ] )
		{
			m_terrainGrid[ i ]->UnloadMipmaps();
		}
	}
#endif
}
#endif

#ifndef NO_HEIGHTMAP_EDIT

CDirectory* ClearTempDirectory( CWorld* world, const String& dirName )
{
	ASSERT( world );

	// Get Parent dir (world dir)
	CDirectory* worldDirectory = world->GetFile()->GetDirectory();
	ASSERT( worldDirectory );

	// Find/create temp dir
	CDirectory* tempDir = worldDirectory->CreateNewDirectory( dirName.AsChar() );
	ASSERT( tempDir );

	// Clean it
	tempDir->Repopulate();
	const TFiles tempFiles = tempDir->GetFiles();
	for ( CDiskFile* tempFile : tempFiles )
	{
		if ( tempFile->GetFileName().ContainsSubstring( TXT("tempTile") ) )
		{
			tempFile->Delete( false, false );
		}
	}
	tempDir->Repopulate();

	return tempDir;
}

void CClipMap::BuildTerrainTiles( const SClipmapParameters& initInfo /*Uint32 tileRes, Uint32 numTilesPerEdge, Uint32 clipSize*/ )
{
	Uint32 numTilesPerEdge = initInfo.clipmapSize / initInfo.tileRes;
	
	ASSUME_CLIPMAP_CHECKED_OUT

	ASSERT( !IsCooked() );
	ASSERT( initInfo.tileRes > 0 );
	ASSERT( IsPow2( initInfo.tileRes ) );
	ASSERT( numTilesPerEdge > 0 );
	ASSERT( initInfo.clipSize >= MIN_CLIPSIZE );
	ASSERT( IsPow2( initInfo.clipSize ) );
	ASSERT( initInfo.tileRes * numTilesPerEdge >= MIN_CLIPMAP_SIZE );
	ASSERT( initInfo.tileRes * numTilesPerEdge <= MAX_CLIPMAP_SIZE );

	GFeedback->BeginTask( TXT("Building tiles (preserving old data if possible)..."), false );

	CWorld* world = GetWorld();
	ASSERT( world );

	const String tempDirName = TXT( "terrain_tiles_temp" );
	const String finalTilesDirName = TXT( "terrain_tiles" );
	String tempFileName;

	// Find/Create terrain tiles temp folder, and make sure it is empty
	CDirectory* tempDirectory = ClearTempDirectory( world, tempDirName );
	ASSERT( tempDirectory );

	// Find/Create final terrain tiles folder
	CDirectory* tilesDir = world->GetFile()->GetDirectory()->CreateNewDirectory( finalTilesDirName );
	ASSERT( tilesDir );

	// Initialize a new grid
	TDynArray< CDiskFile* > newGrid( numTilesPerEdge * numTilesPerEdge );

	// Create initial data matrices for heightmap and control map
	Uint16* initTexels = new Uint16[ initInfo.tileRes * initInfo.tileRes ];
	TControlMapType* initCMTexels = new TControlMapType[ initInfo.tileRes * initInfo.tileRes ];
	for ( Uint32 i = 0; i < initInfo.tileRes * initInfo.tileRes; ++i )
	{
		initTexels[ i ] = 32767;
		initCMTexels[ i ] = ( 1 ) | ( 1 << 5 );
	}

	Uint32 numClipmapStackLevels = ComputeNumberOfClipmapStackLevels( initInfo.clipmapSize, initInfo.clipSize );
	m_colormapStartingMip = CalculateColormapStartingMip( numClipmapStackLevels, numTilesPerEdge, initInfo.tileRes );
	Uint32 colorMapRes = initInfo.tileRes >> m_colormapStartingMip;
	TColorMapType* initColorMapTexels = new TColorMapType[ colorMapRes * colorMapRes ];
	for ( Uint32 i = 0; i < colorMapRes * colorMapRes; ++i )
	{
		initColorMapTexels[ i ] = DEFAULT_COLOR_MAP_VALUE;
	}

	Uint32 progress = 0;
	Uint32 totalProgress = 2 * numTilesPerEdge * numTilesPerEdge;
	const Bool preserveOldData = !m_terrainGrid.Empty();
	for ( Uint32 r=0; r<numTilesPerEdge; ++r )
	{
		for ( Uint32 c=0; c<numTilesPerEdge; ++c )
		{
			// Create tile and setup it's dimensions
			CTerrainTile* tile = ::CreateObject< CTerrainTile >( (CObject*)NULL );
			ASSERT( tile );
			Uint32 minTileMipMapRes = initInfo.tileRes >> ComputeNumberOfClipmapStackLevels( initInfo.tileRes * numTilesPerEdge, initInfo.clipSize );
			ASSERT( minTileMipMapRes >= LOWEST_TILE_MIPMAP_RESOLUTION );
			ASSERT( IsPow2( minTileMipMapRes ) );
			tile->SetRes( initInfo.tileRes, minTileMipMapRes, m_colormapStartingMip );

			// Put initial values in the tile
			Rect sourceAndTargetRect( 0, initInfo.tileRes, 0, initInfo.tileRes );
			tile->SetData( sourceAndTargetRect, sourceAndTargetRect, initTexels, initCMTexels, initInfo.tileRes );

			Rect sourceAndTargetColorRect( 0, colorMapRes, 0, colorMapRes );
			tile->SetColorMapData( sourceAndTargetColorRect, sourceAndTargetColorRect, initColorMapTexels, colorMapRes*sizeof( TColorMapType), m_colormapStartingMip );

			// Old data preservation: determine a set of old tiles, that contribute into this new tiles
			// It will overwrite the initial values set a call ago
			if ( preserveOldData )
			{
				// Prepare a query, to get data associated with the region covered by current tile
				TDynArray< SClipmapHMQueryResult > parts;
				Rect queryRect( c * initInfo.tileRes, (c+1) * initInfo.tileRes, r * initInfo.tileRes, (r+1) * initInfo.tileRes );
				GetRectangleOfData( queryRect, parts );

				for ( Uint32 p=0; p<parts.Size(); ++p )
				{
					SClipmapHMQueryResult& part = parts[p];
					const Uint16* hmData = part.m_tile->GetLevelSyncHM( 0 );
					const TControlMapType* cmData = part.m_tile->GetLevelSyncCM( 0 );
					tile->SetData( part.m_selectionSubRect, part.m_addressingRect, hmData, cmData, part.m_tile->GetResolution() );

					// TODO: preserve colormap data!!
				}

				// OOM: Unload old tile data so we don't run out of memory
				for ( Uint32 p=0; p<parts.Size(); ++p )
				{
					parts[p].m_tile->UnloadMipmaps();
				}
			}

			// OOM: Bind the tile with a local temp tile file, so we can temporarily store it on a disk
			tempFileName = String::Printf( TXT("tempTile_%ix%i.w2ter"), c, r );
			CDiskFile* tempFile = new CDiskFile( tempDirectory, tempFileName, tile );
			tempFile->SetLocal();
			tempDirectory->AddFile( tempFile );
			tempFile->Save();
			tile->UnloadMipmaps();
			
			// Put tile in a grid
			newGrid[ r * numTilesPerEdge + c ] = tempFile;
			GFeedback->UpdateTaskProgress( ++progress, totalProgress );
		}
	}

	delete[] initTexels;
	delete[] initCMTexels;

	// We preserved what we could from the old tiles
	GFeedback->UpdateTaskInfo( TXT("Deleting old tiles...") );
	DestroyTerrainTiles();
	ASSERT( m_terrainGrid.Empty() );
	m_terrainGrid.Resize( numTilesPerEdge * numTilesPerEdge );
	m_tileCache.Resize( numTilesPerEdge * numTilesPerEdge );

	// Copy tiles to the final directory
	GFeedback->UpdateTaskInfo( TXT("Saving tiles to the final location...") );
	for ( Uint32 r=0; r<numTilesPerEdge; ++r )
	{
		for ( Uint32 c=0; c<numTilesPerEdge; ++c )
		{
			const String tileName = String::Printf( TXT("tile_%i_x_%i_res%i.w2ter"), r, c, initInfo.tileRes );
			newGrid[ r * numTilesPerEdge + c ]->Copy( tilesDir, tileName );
			newGrid[ r * numTilesPerEdge + c ]->Unload();
			newGrid[ r * numTilesPerEdge + c ]->Delete( false, false );
			newGrid[ r * numTilesPerEdge + c ] = NULL;
			
			CDiskFile* tileFile = tilesDir->FindLocalFile( tileName.AsChar() );
			ASSERT( tileFile );
			tileFile->Load();
			ASSERT( tileFile->GetResource() );
			m_terrainGrid[ r * numTilesPerEdge + c ] = static_cast< CTerrainTile* >( tileFile->GetResource() );

			GFeedback->UpdateTaskProgress( ++progress, totalProgress );
		}
	}

	ClearTempDirectory( world, tempDirName );
		 
	GFeedback->EndTask();

	//world->OnTerrainTilesCreation();
}

Bool CClipMap::PerformFullGridCheckout()
{
	// Checkout all tiles
	for ( Uint32 t=0; t<m_terrainGrid.Size(); ++t )
	{
		if ( m_terrainGrid[t] )
		{
			CDiskFile* file = m_terrainGrid[t]->GetFile();
			file->GetStatus();
			if ( !file->IsLocal() && !file->IsCheckedOut() && !file->CheckOut() )
			{
				// At least one tile can't be accessed for write
				return false;
			}
		}
	}

	// All tiles seem to have write access available
	return true;
}

Bool CClipMap::DestroyTerrainTiles()
{
	ASSUME_CLIPMAP_CHECKED_OUT

	//CWorld* world = GetWorld();
	//world->OnTerrainTilesDestruction();

	// Now delete
	for ( Uint32 t=0; t<m_terrainGrid.Size(); ++t )
	{
		if ( m_terrainGrid[t] )
		{
			CDiskFile* file = m_terrainGrid[t]->GetFile();

			// Discard resource
			m_terrainGrid[t]->Discard();
			m_terrainGrid[t] = NULL;

			// Delete tile file the way suitable for their P4 status.
			if ( file->IsLocal() )
			{
				file->Delete( false );
			}
			else if ( file->IsCheckedOut() )
			{
				if ( file->Revert( true ) )
				{
					file->Delete( false, false );
				}
			}
			else
			{
				file->Delete( false, false );
			}
		}
	}
	m_terrainGrid.Clear();

	// All tiles should be removed now
	return true;
}

#endif

CTerrainTile* CClipMap::GetTile( Uint32 x, Uint32 y ) const
{
	if ( x < m_numTilesPerEdge && y < m_numTilesPerEdge )
	{
		return m_terrainGrid[ y * m_numTilesPerEdge + x ].Get();
	}

	// Invalid index
	return NULL;
}

void CClipMap::GenerateLevels()
{
	// This stuff is not serialized, so expect calling it after each setup and on postload

	m_levels.ClearFast();
	m_levels.Resize( m_numClipmapStackLevels );
	Float areaSize = m_clipSize * ( m_terrainSize / m_clipmapSize );
	Uint32 levelRes = m_clipmapSize;
	for ( Uint32 i=0; i<m_numClipmapStackLevels; ++i )
	{
		m_levels[i].m_resolutionOfTheLevel = levelRes;
		m_levels[i].m_areaSize = areaSize;
		m_levels[i].m_resolutionOfTheClipRegion = m_clipSize;	// This is constant for all levels, at least for now
		m_levels[i].m_resolutionOfTheTile = m_tileRes >> i;
		areaSize *= 2.0f;
		levelRes /= 2;
		RED_LOG( RED_LOG_CHANNEL( TerrainClipMaps ), TXT("Generated Clipmap %i-level info, with resolution %i and area extents of %f."), i, m_levels[i].m_resolutionOfTheLevel, m_levels[i].m_areaSize );
	}
}

void CClipMap::OnDataChanged()
{
	// Invalidate all levels
	for ( Uint32 l=0; l<m_levels.Size(); ++l )
	{
		m_levels[l].m_lastClipCenterCol = 999999;
		m_levels[l].m_lastClipCenterRow = 999999;
	}
}

const Uint32 CClipMap::CalculateColormapStartingMip( const Uint32 numClipmapStackLevels, const Uint32 numTilesPerEdge, const Uint32 tileRes ) const
{
	Uint32 desiredStartingMip = 0;
	Uint32 numberOfTiles = numTilesPerEdge * numTilesPerEdge;
	Uint32 texelSize = sizeof(Float);
	Float colorMapSize = 0.0f;
	do
	{
		Uint32 startingRes = tileRes >> desiredStartingMip;
		Uint32 numTexelsPerTile = 0;
		Uint32 currentRes = startingRes;
		Uint32 i = desiredStartingMip;
		while ( i <= numClipmapStackLevels && currentRes > 1 )
		{
			numTexelsPerTile += currentRes * currentRes;
			currentRes /= 2;
			++i;
		}
		colorMapSize = (Float)numberOfTiles * (Float)numTexelsPerTile * (Float)texelSize;

		if ( colorMapSize < MAX_COLORMAP_SIZE )
		{
			RED_LOG( RED_LOG_CHANNEL( TerrainClipMaps ), TXT( "Colormap data starts from %d-th mipmap level [%d x %d], size of colormap data: [%1.4f] (%1.4f MB) (%1.4f GB)" ),
				desiredStartingMip, startingRes, startingRes,
				colorMapSize, colorMapSize / (1024.0f * 1024.0f), colorMapSize / (1024.0f * 1024.0f * 1024.0f) );
			break;
		}
		++desiredStartingMip;
	}
	while ( desiredStartingMip < numClipmapStackLevels );

	return desiredStartingMip;
}

void CClipMap::UpdateGrassRendering()
{
#ifdef USE_SPEED_TREE
	if ( m_renderProxy  )
	{
		CFoliageEditionController & controller = GetWorld()->GetFoliageEditionController();
		controller.UpdateGrassMask( m_renderProxy );

		// Update grass setup on the renderer side
		TDynArray< SAutomaticGrassDesc > descriptorsPerTexture[ NUM_TERRAIN_TEXTURES_AVAILABLE ];
		for ( Uint32 i=0; i<NUM_TERRAIN_TEXTURES_AVAILABLE; ++i )
		{
			TDynArray< SAutomaticGrassDesc >& descriptors = descriptorsPerTexture[i];
			if ( m_grassBrushes[ i ].IsValid() && m_grassBrushes[ i ]->HasValidEntries() )
			{
				TDynArray< CVegetationBrushEntry* > brushEntries;
				m_grassBrushes[ i ]->GetEntries( brushEntries );

				for ( Uint32 j=0; j<brushEntries.Size(); ++j )
				{
					CVegetationBrushEntry* entry = brushEntries[j];
					new ( descriptors ) SAutomaticGrassDesc( entry->GetBaseTree()->AcquireRenderObject() );
					SAutomaticGrassDesc& desc = descriptors.Back();
					desc.SetRadiusScale( entry->m_radiusScale );
					desc.SetSize( entry->m_size );
					desc.SetSizeVar( entry->m_sizeVar );
				}
			}
		}

		TRenderObjectPtr< IRenderObject > renderUpdateData( GRender->CreateGrassUpdateData( descriptorsPerTexture ) );
		controller.UpdateGrassSetup( m_renderProxy, renderUpdateData.Get() );
	}
#endif
}

void CClipMap::UpdateMipmapStack( Bool firstframe, Box &terrainCollisionBounding, const SClipMapUpdateInfo &updateInfo, TDynArray< SClipmapLevelUpdate* > &levelUpdates )
{
	Uint32 levelRes = m_clipmapSize;
	if ( !m_levels.Empty() )
	{
		levelRes = m_levels.Back().m_resolutionOfTheLevel;
	}

	const Uint32 mipMapStackTileRes = m_tileRes >> m_numClipmapStackLevels;

	// In the first frame, we upload the entire mip stack. After that, the only reason we should have for sending anything here is
	// if the terrain is edited.
	if ( firstframe )
	{
		const Uint32 texRes = mipMapStackTileRes*m_numTilesPerEdge;
		if ( IsCooked() )
		{
			SClipmapLevelUpdate* levelupdateInfo = new SClipmapLevelUpdate( m_numClipmapStackLevels, texRes );

			levelupdateInfo->m_worldSpaceCoveredByThisLevel = Box( m_terrainCorner, m_terrainCorner + m_terrainSize );
			levelupdateInfo->m_validTexels = Rect( 0, texRes, 0, texRes );
			levelupdateInfo->m_complete = false;

			//////////////////////////////////////////////////////////////////////////
			// Common
			const Uint32 commonRectIndex = levelupdateInfo->m_commonRectData.Size();
			levelupdateInfo->m_commonRectData.PushBack( SClipRegionRectangleData() );
			SClipRegionRectangleData& commonRegionRectData = levelupdateInfo->m_commonRectData.Back();

			commonRegionRectData.m_clipRegionXWriteOffset = 0;
			commonRegionRectData.m_clipRegionYWriteOffset = 0;

			commonRegionRectData.m_offsetInTileU = 0;
			commonRegionRectData.m_offsetInTileV = 0;

			commonRegionRectData.m_texelsToCopyFromTile_Vertical	= texRes;
			commonRegionRectData.m_texelsToCopyFromTile_Horizontal	= texRes;

			//////////////////////////////////////////////////////////////////////////
			// Buffer validation
			RED_ASSERT( m_cookedMipStackHeightHandle && m_cookedMipStackHeightHandle->GetData() );
			RED_ASSERT( m_cookedMipStackControlHandle && m_cookedMipStackControlHandle->GetData() );
			RED_ASSERT( m_cookedMipStackColorHandle && m_cookedMipStackColorHandle->GetData() );

			RED_ASSERT( m_cookedMipStackHeightHandle->GetSize() == texRes * texRes * sizeof( Uint16 ) );
			RED_ASSERT( m_cookedMipStackControlHandle->GetSize() == texRes * texRes * sizeof( TControlMapType ) );
			RED_ASSERT( m_cookedMipStackColorHandle->GetSize() == TerrainUtils::CalcColorMapSize( texRes, texRes ) );

			//////////////////////////////////////////////////////////////////////////
			// Height and Control
			levelupdateInfo->m_updateRects.PushBack( SClipRegionUpdateRectangle() );
			SClipRegionUpdateRectangle& updateRectDesc = levelupdateInfo->m_updateRects.Back();

			updateRectDesc.m_heightMapBuffer = m_cookedMipStackHeightHandle;
			updateRectDesc.m_controlMapBuffer = m_cookedMipStackControlHandle;
			updateRectDesc.m_resolution = texRes;
			updateRectDesc.m_commonRectDataIndex = commonRectIndex;

			//////////////////////////////////////////////////////////////////////////
			// Colour
			levelupdateInfo->m_colormapUpdateRects.PushBack( SColorMapRegionUpdateRectangle() );
			SColorMapRegionUpdateRectangle& colorMapUpdateRectDesc = levelupdateInfo->m_colormapUpdateRects.Back();
			colorMapUpdateRectDesc.m_buffer = m_cookedMipStackColorHandle;
			colorMapUpdateRectDesc.m_resolution = texRes;
			colorMapUpdateRectDesc.m_commonRectDataIndex = commonRectIndex;
			colorMapUpdateRectDesc.m_cooked = true;

			//////////////////////////////////////////////////////////////////////////
			// Append
			RED_FATAL_ASSERT( ValidateLevelUpdate( levelupdateInfo ), "Level update is invalid (see log for details)" );
			levelUpdates.PushBack( levelupdateInfo );

			// Clear Dirty flag on each tile. Otherwise, we'll never clear it and end up continually refreshing stuff :)
			for ( Uint32 r=0; r<m_numTilesPerEdge; ++r )
			{
				for ( Uint32 c=0; c<m_numTilesPerEdge; ++c )
				{
					CTerrainTile* tile = m_terrainGrid[ r * m_numTilesPerEdge + c ].Get();
					if ( !tile )
					{
						ERR_ENGINE( TXT("Tile missing. Clipmap update failed.") );
						continue;
					}

					tile->SetDirty( false );
				}
			}
		}
		else
		{
			SClipmapLevelUpdate* levelupdateInfo = new SClipmapLevelUpdate( m_numClipmapStackLevels, levelRes );
			levelupdateInfo->m_worldSpaceCoveredByThisLevel = Box( m_terrainCorner, m_terrainCorner + m_terrainSize );
			ASSERT( mipMapStackTileRes > 0 );
			levelupdateInfo->m_validTexels = Rect( 0, texRes, 0, texRes );
			levelupdateInfo->m_complete = true;

			levelupdateInfo->m_updateRects.PushBack( SClipRegionUpdateRectangle() );
			SClipRegionUpdateRectangle& updateRectDesc = levelupdateInfo->m_updateRects.Back();
			updateRectDesc.m_resolution = texRes;

			levelupdateInfo->m_colormapUpdateRects.PushBack( SColorMapRegionUpdateRectangle() );
			SColorMapRegionUpdateRectangle& colorMapUpdateRectDesc = levelupdateInfo->m_colormapUpdateRects.Back();
			colorMapUpdateRectDesc.m_resolution = texRes;

			const Uint32 destTexelsSize		= texRes * texRes * sizeof( Uint16 );
			const Uint32 destControlMapSize	= texRes * texRes * sizeof( TControlMapType );
			Uint16* destTexels				= (Uint16*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_TerrainClipmap, destTexelsSize );
			TControlMapType* destControlMap	= (TControlMapType*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_TerrainClipmap, destControlMapSize );

			auto deallocateFunc = []( void* ptr ){ RED_MEMORY_FREE( MemoryPool_Default, MC_TerrainClipmap, ptr ); };
			updateRectDesc.m_heightMapBuffer = BufferHandle( new BufferProxy( destTexels, destTexelsSize, deallocateFunc ) );
			updateRectDesc.m_controlMapBuffer = BufferHandle( new BufferProxy( destControlMap, destControlMapSize, deallocateFunc ) );

			const Uint32 colorMapSize = texRes * texRes * sizeof( TColorMapType );
			void* destColor = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_TerrainClipmap, colorMapSize );
			colorMapUpdateRectDesc.m_buffer = BufferHandle( new BufferProxy( destColor, colorMapSize, deallocateFunc ) );

			TColorMapType* colorUpdateData = static_cast< TColorMapType* >( destColor );

			for ( Uint32 r=0; r<m_numTilesPerEdge; ++r )
			{
				for ( Uint32 c=0; c<m_numTilesPerEdge; ++c )
				{
					CTerrainTile* tile = m_terrainGrid[ r * m_numTilesPerEdge + c ].Get();
					if ( !tile )
					{
						ERR_ENGINE( TXT("Tile missing. Clipmap update failed.") );
						continue;
					}

					// Since we're uncooked, our tiles had better be uncooked as well.
					RED_ASSERT( !tile->IsCooked() );

					// Use the sync loading functions here, because we need to always have the mipmap stack available.
					const Uint16* sourceTexels = tile->GetLevelSyncHM( m_numClipmapStackLevels );
					const TControlMapType* sourceCMTexels = tile->GetLevelSyncCM( m_numClipmapStackLevels );
					const TColorMapType* sourceColorMapTexels = static_cast< const TColorMapType* >( tile->GetLevelSyncColorMap( m_numClipmapStackLevels ) );

					ASSERT( sourceTexels );
					ASSERT( sourceCMTexels );
					ASSERT( sourceColorMapTexels );

					if ( !sourceTexels || !sourceCMTexels || !sourceColorMapTexels )
					{
						ERR_ENGINE( TXT("Tile data is missing for %ls. Clipmap update failed."), tile->GetFriendlyName().AsChar() );
						continue;
					}

					Uint16* heightTarget = destTexels + texRes * ( r * mipMapStackTileRes ) + ( c * mipMapStackTileRes );
					TControlMapType* controlTarget = destControlMap + texRes * ( r * mipMapStackTileRes ) + ( c * mipMapStackTileRes );
					TColorMapType* colorTarget = colorUpdateData + texRes * ( r * mipMapStackTileRes ) + ( c * mipMapStackTileRes );
					for ( Uint32 y = 0; y < mipMapStackTileRes; ++y )
					{
						Red::System::MemoryCopy( heightTarget + y * texRes, sourceTexels + y * mipMapStackTileRes, mipMapStackTileRes * sizeof( Uint16 ) );
						Red::System::MemoryCopy( controlTarget + y * texRes, sourceCMTexels + y * mipMapStackTileRes, mipMapStackTileRes * sizeof( TControlMapType ) );
						Red::System::MemoryCopy( colorTarget + y * texRes, sourceColorMapTexels + y * mipMapStackTileRes, mipMapStackTileRes * sizeof( TColorMapType ));
					}

					// Mark not dirty, so we don't copy it again unless it actually changes.
					tile->SetDirty( false );
				}
			}


			RED_FATAL_ASSERT( ValidateLevelUpdate( levelupdateInfo ), "Level update is invalid (see log for details)" );
			levelUpdates.PushBack( levelupdateInfo );
		}
	}


	// Mipmap eviction tracking data
	PreEviction();

	SClipmapLevelUpdate* perTileUpdateInfo = nullptr;

	// We need to update height ranges if we're not cooked, and either have no data or on the first update. Make sure we have up-to-date
	// ranges and not just whatever we may have been saved with before. If we're cooked, we already have proper ranges saved and don't
	// need to do the extra work.
	Bool needHeightRangeUpdate = !IsCooked() && ( m_tileHeightRanges.Empty() || firstframe );

	for ( Uint32 r=0; r<m_numTilesPerEdge; ++r )
	{
		for ( Uint32 c=0; c<m_numTilesPerEdge; ++c )
		{
			CTerrainTile* tile = m_terrainGrid[ r * m_numTilesPerEdge + c ].Get();
			if ( !tile )
			{
				//ERR_ENGINE( TXT("CClipMap::Update() : Tile data is missing. Clipmap update failed.") );
				break;
			}

			if ( !IsCooked() )
			{
				// Keep the final mip alive, so we always have something available for heightmap queries.
				// We don't need this when cooked, because in that case we've got a compiled buffer will the full mip stack.
				tile->TouchMip( m_numClipmapStackLevels );

				if ( tile->IsDirty() && !firstframe )
				{
					needHeightRangeUpdate = true;

					// Fetch data form the level below the clipmap stack. The mipmap stack begins there.
					// It may be one level deep, but for the sake of usual clipmap technique naming consistency, we call it a mipmap stack.

					// Use the sync loading functions here, because we need to always have the mipmap stack available.
					BufferHandle sourceTexels			= tile->GetLevelSyncHMBufferHandle( m_numClipmapStackLevels );
					BufferHandle sourceCMTexels			= tile->GetLevelSyncCMBufferHandle( m_numClipmapStackLevels );
					BufferHandle sourceColorMapTexels	= tile->GetLevelSyncColorMapBufferHandle( m_numClipmapStackLevels );

					ASSERT( sourceTexels );
					ASSERT( sourceCMTexels );
					ASSERT( sourceColorMapTexels );

					if ( sourceTexels && sourceCMTexels && sourceColorMapTexels )
					{
						if ( perTileUpdateInfo == nullptr )
						{
							perTileUpdateInfo = new SClipmapLevelUpdate( m_numClipmapStackLevels, levelRes );
							perTileUpdateInfo->m_worldSpaceCoveredByThisLevel = Box( m_terrainCorner, m_terrainCorner + m_terrainSize );
							ASSERT( mipMapStackTileRes > 0 );
							perTileUpdateInfo->m_validTexels = Rect( 0, mipMapStackTileRes*m_numTilesPerEdge, 0, mipMapStackTileRes*m_numTilesPerEdge );

							perTileUpdateInfo->m_commonRectData.Reserve( m_numTilesPerEdge * m_numTilesPerEdge );
							perTileUpdateInfo->m_updateRects.Reserve( m_numTilesPerEdge * m_numTilesPerEdge );
							perTileUpdateInfo->m_colormapUpdateRects.Reserve( m_numTilesPerEdge * m_numTilesPerEdge );
						}

						SClipRegionRectangleData commonData;

						commonData.m_offsetInTileU = 0;
						commonData.m_offsetInTileV = 0;

						commonData.m_clipRegionXWriteOffset = c * mipMapStackTileRes;
						commonData.m_clipRegionYWriteOffset = r * mipMapStackTileRes;

						commonData.m_texelsToCopyFromTile_Horizontal = mipMapStackTileRes;
						commonData.m_texelsToCopyFromTile_Vertical = mipMapStackTileRes;

						const Uint32 commonRectIndex = perTileUpdateInfo->m_commonRectData.Size();
						perTileUpdateInfo->m_commonRectData.PushBack( commonData );

						perTileUpdateInfo->m_updateRects.PushBack( SClipRegionUpdateRectangle() );
						SClipRegionUpdateRectangle& updateRectDesc = perTileUpdateInfo->m_updateRects.Back();
						updateRectDesc.m_heightMapBuffer = sourceTexels;
						updateRectDesc.m_controlMapBuffer = sourceCMTexels;
						updateRectDesc.m_resolution = mipMapStackTileRes;
						updateRectDesc.m_commonRectDataIndex = commonRectIndex;

						perTileUpdateInfo->m_colormapUpdateRects.PushBack( SColorMapRegionUpdateRectangle() );
						SColorMapRegionUpdateRectangle& colorMapUpdateRectDesc = perTileUpdateInfo->m_colormapUpdateRects.Back();
						colorMapUpdateRectDesc.m_buffer = sourceColorMapTexels;
						colorMapUpdateRectDesc.m_resolution = mipMapStackTileRes;
						colorMapUpdateRectDesc.m_commonRectDataIndex = commonRectIndex;

						tile->SetDirty( false );
					}
				}
			}

			if ( m_numClipmapStackLevels == 0 )
			{
				Box collisionTileBB = GetBoxForTile( c, r, 0.f );
				terrainCollisionBounding.AddBox( collisionTileBB );

				// This is a trivial clipmap. Generate collision here
				// Make sure that the closest (l==0) tiles have collision data generated and uptodate.
				if ( !tile->IsCollisionGenerated() || tile->IsDirty() || tile->IsCollisionGenerationPending() )
				{
					collisionTileBB.Min.Z = m_lowestElevation;
					collisionTileBB.Max.Z = m_highestElevation;

					TDynArray< CName > textureNames;
					if ( m_material )
					{
						THandle< CTextureArray > texture;
						if ( m_material->ReadParameter( CNAME(diffuse), texture ) )
						{
							CTextureArray* textureArray = Cast< CTextureArray >( texture.Get() );
							if( textureArray )
							{
								textureArray->GetTextureNames( textureNames );
							}
						}
					}

					tile->GenerateCollision( this, GetWorld(), collisionTileBB, textureNames, m_textureParams, tile, tile, tile );
				}
			}

			EvictionTileCheck( updateInfo, tile );
		}
	}

	PostEviction( updateInfo );

	if ( perTileUpdateInfo != nullptr )
	{
		RED_FATAL_ASSERT( ValidateLevelUpdate( perTileUpdateInfo ), "Level update is invalid (see log for details)" );
		levelUpdates.PushBack( perTileUpdateInfo );
	}

	if ( needHeightRangeUpdate )
	{
		RED_ASSERT( !IsCooked(), TXT("Cooked clipmap needs to update height range? Somehow a tile was marked dirty or something...") );

		const Uint32 numTiles = m_terrainGrid.Size();
		m_tileHeightRanges.Resize( numTiles );
		for ( Uint32 i = 0; i < numTiles; ++i )
		{
			CTerrainTile* tile = m_terrainGrid[i].Get();
			if ( tile != nullptr )
			{
				m_tileHeightRanges[i] = Vector2( GetTileMinimumHeight( tile ), GetTileMaximumHeight( tile ) );
			}
			else
			{
				m_tileHeightRanges[i] = Vector2( m_lowestElevation, m_highestElevation );
			}
		}


		if ( m_renderProxy != nullptr )
		{
			( new CRenderCommand_UpdateTerrainTileHeightRanges( m_renderProxy, m_tileHeightRanges ) )->Commit();
		}
	}
}

Bool CClipMap::IsLoadingTiles() const
{
	for ( Uint32 i = 0; i < m_terrainGrid.Size(); ++i )
	{
		if ( m_terrainGrid[i]->IsLoadingAnyAsync() )
		{
			return true;
		}
	}
	return false;
}


Rect CClipMap::GetClipWindowRect( const Vector& centerPosition, Uint32 whichLevel, Rect* outLevelSpace, Int32* outCenterCol, Int32* outCenterRow ) const
{
	RED_ASSERT( whichLevel < m_levels.Size() );
	if ( whichLevel >= m_levels.Size() )
	{
		return Rect();
	}

	const SClipMapLevel& level = m_levels[ whichLevel ];

	ASSERT( level.m_resolutionOfTheLevel > 0 );
	ASSERT( IsPow2( level.m_resolutionOfTheClipRegion ) );
	ASSERT( level.m_resolutionOfTheClipRegion > 0 );
	ASSERT( level.m_resolutionOfTheClipRegion < level.m_resolutionOfTheLevel );

	Vector viewerPosition = centerPosition;

	// Snap viewer position to the tessellation block size
	{
		Int32 numTessBlocks = level.m_resolutionOfTheLevel / TERRAIN_TESS_BLOCK_RES; 
		Float tessBlockSize = m_terrainSize / numTessBlocks;

		viewerPosition.X = ( (Int32)( viewerPosition.X / tessBlockSize ) ) * tessBlockSize;
		viewerPosition.Y = ( (Int32)( viewerPosition.Y / tessBlockSize ) ) * tessBlockSize;
	}

	// Compute position in texel space of the full heightmap
	// Terrain space means from 0,0 to terrainSize,terrainSize
	Vector2 viewPosTerrainSpace( viewerPosition.X - m_terrainCorner.X, viewerPosition.Y - m_terrainCorner.Y );

	viewPosTerrainSpace.X = Clamp< Float >( viewPosTerrainSpace.X + 0.5f, 0.0f, m_terrainSize );
	viewPosTerrainSpace.Y = Clamp< Float >( viewPosTerrainSpace.Y + 0.5f, 0.0f, m_terrainSize );

	// Normalize the terrain space position
	Vector2 viewPosTerrainSpaceNorm( viewPosTerrainSpace.X / m_terrainSize, viewPosTerrainSpace.Y / m_terrainSize );

	// Compute new clipcenter
	Int32 clipCenterCol = (Int32)( viewPosTerrainSpaceNorm.X * ( level.m_resolutionOfTheLevel - 1 ) );
	Int32 clipCenterRow = (Int32)( viewPosTerrainSpaceNorm.Y * ( level.m_resolutionOfTheLevel - 1 ) );

	// Correct clipcenter, considering the edge of the terrain
	clipCenterCol = Clamp< Int32 >( clipCenterCol, level.m_resolutionOfTheClipRegion/2, ( level.m_resolutionOfTheLevel - 1) - level.m_resolutionOfTheClipRegion/2 );
	clipCenterRow = Clamp< Int32 >( clipCenterRow, level.m_resolutionOfTheClipRegion/2, ( level.m_resolutionOfTheLevel - 1) - level.m_resolutionOfTheClipRegion/2 );

	ASSERT( clipCenterCol < level.m_resolutionOfTheLevel );
	ASSERT( clipCenterRow < level.m_resolutionOfTheLevel );

	// Compute clipregion rectangle (in texels)
	Rect clipRegion( clipCenterCol - level.m_resolutionOfTheClipRegion / 2, clipCenterCol + level.m_resolutionOfTheClipRegion / 2,
		clipCenterRow - level.m_resolutionOfTheClipRegion / 2, clipCenterRow + level.m_resolutionOfTheClipRegion / 2 );

	if ( IsCooked() )
	{
		// Make sure the region is on 4-texel boundaries, to work with BC textures.
		// This will switch to Uint32, but that is not a problem since we've already limited the region to be non-negative.
		clipRegion.m_left	= TerrainUtils::MakeExactlyColorMapBlock( clipRegion.m_left );
		clipRegion.m_right	= TerrainUtils::MakeExactlyColorMapBlock( clipRegion.m_right );
		clipRegion.m_top	= TerrainUtils::MakeExactlyColorMapBlock( clipRegion.m_top );
		clipRegion.m_bottom	= TerrainUtils::MakeExactlyColorMapBlock( clipRegion.m_bottom );
	}

	// Compute clip region rectangle in level0 space also, to determine a set of contributing tiles
	if ( outLevelSpace != nullptr )
	{
		*outLevelSpace = clipRegion;
	}
	if ( outCenterCol != nullptr )
	{
		*outCenterCol = clipCenterCol;
	}
	if ( outCenterRow != nullptr )
	{
		*outCenterRow = clipCenterRow;
	}

	Uint32 multiplier = 1 << whichLevel;
	return Rect( clipRegion.m_left * multiplier, clipRegion.m_right * multiplier, clipRegion.m_top * multiplier, clipRegion.m_bottom * multiplier );
}

void CClipMap::TouchAndEvict( const SClipMapUpdateInfo& updateInfo )
{
	RED_FATAL_ASSERT( updateInfo.m_forceSync, "This function must only be used during periods of synchronous loading (i.e. fast travel)" );
	ASSERT( m_numClipmapStackLevels == m_levels.Size() );
	RED_FATAL_ASSERT( m_tileCache.Size() == m_terrainGrid.Size(), "m_tileCache has not been properly initialised!" );
	RED_ASSERT( m_TouchAndEvictCoordBuffer.Capacity() == m_numClipmapStackLevels, TXT( "Coord buffer hasn't been preallocated!" ) );

	for ( Uint32 l = 0; l < m_numClipmapStackLevels; ++l )
	{
		SClipMapLevel& level = m_levels[ l ];

		ASSERT( IsPow2( level.m_resolutionOfTheClipRegion ) );
		ASSERT( level.m_resolutionOfTheClipRegion > 0 );
		ASSERT( IsPow2( level.m_resolutionOfTheTile ) );
		ASSERT( level.m_resolutionOfTheTile > 0 );

		Bool updateColorMap = ((Int32)l >= m_colormapStartingMip);

		Int32 clipCenterCol, clipCenterRow;
		Rect clipRegion;
		Rect clipRegionInLevel0Space;
		clipRegionInLevel0Space = GetClipWindowRect( updateInfo.m_viewerPosition, l, &clipRegion, &clipCenterCol, &clipCenterRow );

		m_TouchAndEvictCoordBuffer.PushBack( TileCoords( clipRegionInLevel0Space, static_cast< Int32 >( m_tileRes ) ) );
	}

	PreEviction();

	for ( Uint32 r = 0; r < m_numTilesPerEdge; ++r )
	{
		for ( Uint32 c = 0; c < m_numTilesPerEdge; ++c )
		{
			// Ensure Get() is called only once
			const Uint32 index = r * m_numTilesPerEdge + c;
			CTerrainTile* tile = m_terrainGrid[ index ].Get();
			m_tileCache[ index ] = tile;

			// Touch each used mip in this tile
			for ( Uint32 l = 0; l < m_numClipmapStackLevels; ++l )
			{
				TileCoords& coordsToTest = m_TouchAndEvictCoordBuffer[ l ];
				
				if( r >= coordsToTest.m_row.m_first && r <= coordsToTest.m_row.m_last && c >= coordsToTest.m_column.m_first && c <= coordsToTest.m_column.m_last )
				{
					tile->TouchMip( l );
				}
			}

			EvictionTileCheck( updateInfo, tile );
		}
	}

	PostEviction( updateInfo );
}

void CClipMap::PreEviction()
{
	m_residentMipTileFootprint = 0;
	m_mipsToEvict.ClearFast();
}

void CClipMap::EvictionTileCheck( const SClipMapUpdateInfo& updateInfo, CTerrainTile* tile )
{
	// Eviction pass 1: Update mip-map eviction timers for all tiles, collect any mips with loaded data that
	// could potentially be unloaded. Also take a count of how much memory is used by the tiles
	Red::System::MemSize tileFootprint = 0;
	tile->GetPotentialMipsToUnload( updateInfo.m_aggressiveEviction, updateInfo.m_timeDelta, m_mipsToEvict, tileFootprint );
	m_residentMipTileFootprint += tileFootprint;
}

void CClipMap::PostEviction( const SClipMapUpdateInfo& updateInfo )
{
	if ( !m_mipsToEvict.Empty() )
	{
		// Eviction pass 2: Sort the list of potential mips to evict by time remaining and size.
		// This ensures that we evict all timed-out mips first, then if we need to drop any more, the biggest are dropped first
		SortMipsForEviction( m_mipsToEvict );

		// Eviction final pass. First destroy any mips that definitely need destroying (timed out)
		// After that, if we still need to remove more, them keep going until we are in budget
		const Uint32 maxBytes = Config::cvTerrainTileMipDataBudget.Get() * 1024 * 1024;
		EvictOldMips( updateInfo, m_mipsToEvict, m_residentMipTileFootprint, maxBytes );
	}
}

void CClipMap::UpdateEachLevel( const SClipMapUpdateInfo &updateInfo, Bool firstframe, Box &terrainCollisionBounding, TDynArray< SClipmapLevelUpdate* > &levelUpdates)
{
	Bool enableSyncLoading = updateInfo.m_forceSync;
#ifndef NO_EDITOR
	if ( !GGame || !GGame->IsActive() )
	{
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("Editor"), TXT("AllowTerrainSyncLoading"), enableSyncLoading );
	}
#endif

	ASSERT( m_numClipmapStackLevels == m_levels.Size() );

#ifdef NO_EDITOR		// In the game, we want to update one clipmap level per frame, not all of them.
	Int32 l = m_nextClipmapLevelToUpdate;
	m_nextClipmapLevelToUpdate = (++m_nextClipmapLevelToUpdate) % m_numClipmapStackLevels;

	Int32 lastLevelToUpdate = l;
	CPhysicsWorld* physicsWorld = nullptr;
	if ( enableSyncLoading )
	{
		l = 0;
		lastLevelToUpdate = m_numClipmapStackLevels - 1;
	}
	else if( GetWorld()->GetPhysicsWorld( physicsWorld ) )
	{
		Int32 clipCenterCol, clipCenterRow;
		Rect clipRegion;

		Rect clipRegionInLevel0Space;
		clipRegionInLevel0Space = GetClipWindowRect( updateInfo.m_viewerPosition, 0, &clipRegion, &clipCenterCol, &clipCenterRow );

		TileCoords coords( clipRegionInLevel0Space, static_cast< Int32 >( m_tileRes ) );

		UELPrefetchTiles( coords, m_tileCache );

		for ( coords.m_row.Reset(); coords.m_row.IsValid(); ++coords.m_row )
		{
			for ( coords.m_column.Reset(); coords.m_column.IsValid(); ++coords.m_column )
			{
				const Uint32 r = coords.m_row.m_current;
				const Uint32 c = coords.m_column.m_current;
				CTerrainTile* tile = m_tileCache[ r * m_numTilesPerEdge + c ];

				if ( tile )
				{
					Box collisionTileBB = GetBoxForTile( c, r, 0.f );

					if ( !tile->IsCollisionGenerated() && tile->IsCollisionEnabled() )
					{
						Box collisionTileBB = GetBoxForTile( c, r, 0.f );

						collisionTileBB.Min.Z = m_lowestElevation;
						collisionTileBB.Max.Z = m_highestElevation;

						physicsWorld->MarkSectorAsUnready( collisionTileBB.CalcCenter() );
					}
				}
			}
		}
	}

	levelUpdates.Reserve( lastLevelToUpdate - l );

	for (; l<=lastLevelToUpdate; ++l)
#else					// In the editor, we want to respond to changes immediately
	for ( Uint32 l=0; l<m_numClipmapStackLevels; ++l )
#endif
	{
		SClipMapLevel& level = m_levels[l];

		ASSERT( IsPow2( level.m_resolutionOfTheClipRegion ) );
		ASSERT( level.m_resolutionOfTheClipRegion > 0 );
		ASSERT( IsPow2( level.m_resolutionOfTheTile ) );
		ASSERT( level.m_resolutionOfTheTile > 0 );

		Bool updateColorMap = ((Int32)l >= m_colormapStartingMip);

		Int32 clipCenterCol, clipCenterRow;
		Rect clipRegion;
		Rect clipRegionInLevel0Space;
		clipRegionInLevel0Space = GetClipWindowRect( updateInfo.m_viewerPosition, l, &clipRegion, &clipCenterCol, &clipCenterRow );


		// TODO: toroidal mapping

		SClipRegionRectangleData commonRegionRectData;


		// Keep track of the region of the clip window that contains valid data (in case some tiles have not yet loaded)
		Rect validTexels( 0, level.m_resolutionOfTheClipRegion, 0, level.m_resolutionOfTheClipRegion );


		// Compute offset of the clipcenter since the previous update
		Int32 clipCenterColOffset = Abs( clipCenterCol - level.m_lastClipCenterCol );
		Int32 clipCenterRowOffset = Abs( clipCenterRow - level.m_lastClipCenterRow );

		// Currently: Update clipmap region if we moved a big distance
		Int32 biggestOffset = Max( clipCenterColOffset, clipCenterRowOffset );
		static Float updateCriterion = (Float)(32>>l);

		// Compute world space covered by this level
		// Shrink by two units, to avoid artifacts
		Float unitsPerTexel = level.m_areaSize / level.m_resolutionOfTheClipRegion;
		Vector boxMin;
		Vector boxMax;
		boxMin.X = clipRegion.m_left * unitsPerTexel + m_terrainCorner.X;
		boxMin.Y = clipRegion.m_top * unitsPerTexel + m_terrainCorner.Y;
		boxMin.Z = NumericLimits< Float >::Min();
		boxMax.X = clipRegion.m_right * unitsPerTexel + m_terrainCorner.X;
		boxMax.Y = clipRegion.m_bottom * unitsPerTexel + m_terrainCorner.Y;
		boxMax.Z = NumericLimits< Float >::Max();

		commonRegionRectData.m_clipRegionXWriteOffset = 0;
		commonRegionRectData.m_clipRegionYWriteOffset = 0;

		// Iterate tiles that contribute
		Int32 beginningOffsetInTileU = clipRegion.m_left % (Int32)level.m_resolutionOfTheTile;
		Int32 beginningOffsetInTileV = clipRegion.m_top % (Int32)level.m_resolutionOfTheTile;

		commonRegionRectData.m_offsetInTileU = beginningOffsetInTileU;
		commonRegionRectData.m_offsetInTileV = beginningOffsetInTileV;

		commonRegionRectData.m_texelsToCopyFromTile_Vertical	= level.m_resolutionOfTheTile - commonRegionRectData.m_offsetInTileV;
		commonRegionRectData.m_texelsToCopyFromTile_Horizontal	= level.m_resolutionOfTheTile - commonRegionRectData.m_offsetInTileU;

		Int32 texelsLeftToCopyU = level.m_resolutionOfTheClipRegion;
		Int32 texelsLeftToCopyV = level.m_resolutionOfTheClipRegion;

		// Compute a set of tiles, that contribute into the clipregion at this particular level. We need all of them, as we are updating the whole region.
		TileCoords coords( clipRegionInLevel0Space, static_cast< Int32 >( m_tileRes ) );

		// Grab tiles in this window. Only get if we haven't already, so we're call Get() only once per tile.
		UELPrefetchTiles( coords, m_tileCache );

		// See if we need to update render side clipmap window
		Bool needsUpdating = UELNeedsUpdating( biggestOffset, updateCriterion, coords, m_tileCache, l );

		Bool updateLevelRenderResource = firstframe || ( biggestOffset > updateCriterion ) || needsUpdating;

		if ( !enableSyncLoading )
			ProcessAsyncLoading( l, clipRegionInLevel0Space, clipCenterCol, clipCenterRow, updateInfo.m_viewerPosition );

		// Allocate update buffer if necessary
		SClipmapLevelUpdate* levelupdateInfo = nullptr;

		if ( updateLevelRenderResource )
		{
			levelupdateInfo = new SClipmapLevelUpdate( l, level.m_resolutionOfTheClipRegion );

			// 
			const Uint32 maximumReserveSize = ( ( coords.m_row.m_last - coords.m_row.m_first ) + 1 ) * ( ( coords.m_column.m_last - coords.m_column.m_first ) + 1 );

			levelupdateInfo->m_commonRectData.Reserve( maximumReserveSize );
			levelupdateInfo->m_updateRects.Reserve( maximumReserveSize );

			if( updateColorMap )
			{
				levelupdateInfo->m_colormapUpdateRects.Reserve( maximumReserveSize );
			}
		}

		// Iterate the tiles because it is convenient for now, but do the buffer operations only if necessary
		// TODO: Think about splitting the update and render upload stages
		for ( coords.m_row.Reset(); coords.m_row.IsValid(); ++coords.m_row )
		{
			commonRegionRectData.m_texelsToCopyFromTile_Horizontal = level.m_resolutionOfTheTile - commonRegionRectData.m_offsetInTileU;

			for ( coords.m_column.Reset(); coords.m_column.IsValid(); ++coords.m_column )
			{
				// From each tile, we must copy the specific rectangle of source texels, into the specific rectangle of destination texels in the clipmap level window.
				CTerrainTile* tile = m_tileCache[ coords.m_row.m_current * m_numTilesPerEdge + coords.m_column.m_current ];
				ASSERT( tile );

				if ( tile )
				{
					tile->UpdateWasLoaded( l );

					UELGenerateCollisionForTile( l, coords, terrainCollisionBounding, tile );

					if ( enableSyncLoading && !tile->IsLoaded( l ) )
					{
						tile->LoadSync( l );
					}

					if ( !tile->IsLoaded( l ) )
					{
						// If this tile isn't loaded yet, make sure it's loading, and trim validTexels and worldRect so the missing data isn't
						// included in this clip level.
						tile->StartAsyncLoad( l );

						UELTrimBounds( boxMin, boxMax, validTexels, coords, level, unitsPerTexel, commonRegionRectData );
					}
					else
					{
						UELAppendUpdateRects( levelupdateInfo, tile, l, updateLevelRenderResource, updateColorMap, level, commonRegionRectData );
					}
				}

				// Update write position
				commonRegionRectData.m_clipRegionXWriteOffset += commonRegionRectData.m_texelsToCopyFromTile_Horizontal;

				// We are done with the first column, so we don't need the horizontal offset of noncontributing texels (until next row)
				commonRegionRectData.m_offsetInTileU = 0;

				// Keep track of the texels left to copy
				texelsLeftToCopyU -= commonRegionRectData.m_texelsToCopyFromTile_Horizontal;

				// From the next column, we copy texels until we have all we need (texelsLeftToCopyU, or until there are no more texels in the tile (which means it still isn't the last column)
				commonRegionRectData.m_texelsToCopyFromTile_Horizontal = Min( level.m_resolutionOfTheTile, texelsLeftToCopyU );
			}

			// Update write position
			commonRegionRectData.m_clipRegionYWriteOffset += commonRegionRectData.m_texelsToCopyFromTile_Vertical;
			commonRegionRectData.m_clipRegionXWriteOffset = 0;

			// We go through the next row, beginning from the first column of contributing tiles. That means we apply a horizontal offset of noncontributing texels again.
			commonRegionRectData.m_offsetInTileU = beginningOffsetInTileU;

			// We are done with the first row, so we don't need the vertical offset of noncontributing texels again.
			commonRegionRectData.m_offsetInTileV = 0;

			// Keep track of texels to copy left
			texelsLeftToCopyV -= commonRegionRectData.m_texelsToCopyFromTile_Vertical;

			// Horizontally, we will begin from the scratch (since it will be a new row)
			texelsLeftToCopyU = level.m_resolutionOfTheClipRegion;

			// From the next row, we copy texels until we have all we need (texelsLeftToCopyV) or until there are no more texels in the tile (which means it still isn't the last row)
			commonRegionRectData.m_texelsToCopyFromTile_Vertical = Min( level.m_resolutionOfTheTile, texelsLeftToCopyV );
		}

		ASSERT( texelsLeftToCopyV == 0 );
		ASSERT( commonRegionRectData.m_texelsToCopyFromTile_Vertical == 0 );
		ASSERT( commonRegionRectData.m_texelsToCopyFromTile_Horizontal == 0 );

		if ( updateLevelRenderResource )
		{
			if ( levelupdateInfo->m_commonRectData.Size() == 0 )
			{
				//nothing to update
				delete levelupdateInfo;
				continue;
			}

			RED_LOG( CClipMap, TXT( "Updating clipmap for level %u" ), l );

			RED_FATAL_ASSERT( levelupdateInfo, "Should have been created earlier in function" );

			// If we have a valid box, set it. Otherwise, we can just clear the world box.
			if ( boxMin.X <= boxMax.X && boxMin.Y <= boxMax.Y )
			{
				levelupdateInfo->m_worldSpaceCoveredByThisLevel = Box( boxMin, boxMax );
			}
			else
			{
				levelupdateInfo->m_worldSpaceCoveredByThisLevel.Clear();
			}

			levelupdateInfo->m_validTexels = validTexels;

			RED_FATAL_ASSERT( ValidateLevelUpdate( levelupdateInfo ), "Level update is invalid (see log for details)" );
			levelUpdates.PushBack( levelupdateInfo );

			// Store new clipcenter
			level.m_lastClipCenterCol = clipCenterCol;
			level.m_lastClipCenterRow = clipCenterRow;
			level.m_lastClipRegion = clipRegion;

#ifdef RED_LOGGING_ENABLED
			// Log a message if the valid texel rect is not full.
			if ( levelupdateInfo->m_validTexels.m_left > 0 && levelupdateInfo->m_validTexels.m_right < level.m_resolutionOfTheClipRegion
				&& levelupdateInfo->m_validTexels.m_top > 0 && levelupdateInfo->m_validTexels.m_bottom < level.m_resolutionOfTheClipRegion )
			{
				LOG_ENGINE( TXT("Terrain tiles from clip level %u still loading. This can show up if moving fast in the editor, but you shouldn't see this in-game!"), l );
			}
#endif
		}
	}

	if( levelUpdates.Size() > 0 )
	{
		RED_LOG( CClipMap, TXT( "Total number of level updates %u" ), levelUpdates.Size() );

		for( Uint32 i = 0; i < levelUpdates.Size(); ++i )
		{
			SClipmapLevelUpdate* update = levelUpdates[ i ];

			RED_LOG( CClipMap, TXT( "Total number of update rects: %u, colour update rects: %u" ), update->m_updateRects.Size(), update->m_colormapUpdateRects.Size() );
		}
	}

	m_previousViewerPosition = updateInfo.m_viewerPosition;
}

CTerrainTile* CClipMap::WorldPositionToTile( const Vector& worldPosition, Vector2* tileLocalPosition ) const
{
	if ( m_tileRes != 0 && m_terrainSize != 0.0f )
	{
		// Compute position in texel space of the full heightmap
		// Terrain space means from 0,0 to terrainSize,terrainSize
		Int32 tileX;
		Int32 tileY;
		CTerrainTile* tile = GetTileFromPosition( worldPosition, tileX, tileY );
		ASSERT( tile );
		if ( tile )
		{
			Vector2 tileCorner(
				m_terrainCorner.X + tileX * ( m_terrainSize / m_numTilesPerEdge ),
				m_terrainCorner.Y + tileY * ( m_terrainSize / m_numTilesPerEdge ) );

			if ( tileLocalPosition )
			{
				*tileLocalPosition = worldPosition - tileCorner;
			}

			return tile;
		}
	}

	return nullptr;
}


void CClipMap::ProcessAsyncLoading( Uint32 clipLevel, const Rect& level0ClipRegion, Int32 clipCenterCol, Int32 clipCenterRow, const Vector& viewerPosition )
{
	SClipMapLevel& level = m_levels[ clipLevel ];

	// Tiles that the clip region covers.
	Uint32 firstCol = level0ClipRegion.m_left / (Int32)m_tileRes;
	Uint32 lastCol  = level0ClipRegion.m_right / (Int32)m_tileRes;
	Uint32 firstRow = level0ClipRegion.m_top / (Int32)m_tileRes;
	Uint32 lastRow  = level0ClipRegion.m_bottom / (Int32)m_tileRes;


	// Texel coordinates within the edge tiles where the clip region boundaries are.
	Int32 begInL0U = level0ClipRegion.m_left - ( firstCol * m_tileRes );
	Int32 endInL0U = level0ClipRegion.m_right - ( lastCol * m_tileRes );
	Int32 begInL0V = level0ClipRegion.m_top - ( firstRow * m_tileRes );
	Int32 endInL0V = level0ClipRegion.m_bottom - ( lastRow * m_tileRes );

	// How far ahead we want to begin loading.
	const Float lookAheadDistance = Config::cvTerrainReadAheadDistance.Get();
	const Float unitsPerTexel = m_terrainSize / m_clipmapSize;
	const Float unitsPerTile = unitsPerTexel * m_tileRes;
	const Int32 lookAheadL0 = ( Int32 )Clamp< Float >( lookAheadDistance / unitsPerTexel, 0.0f, ( Float )m_tileRes );

	// Based on what direction we're moving, check if we're close enough to start loading the next set of tiles.
	Vector movementDirection = viewerPosition - m_previousViewerPosition;

	Bool L = false, R = false, T = false, B = false;

	if ( movementDirection.X < 0 )
	{
		L = begInL0U < ( Int32 )( lookAheadL0 ) && firstCol > 0;
	}
	else if ( movementDirection.X > 0 )
	{
		R = endInL0U > ( Int32 )( m_tileRes - lookAheadL0 ) && lastCol < m_numTilesPerEdge - 1;
	}

	if ( movementDirection.Y < 0 )
	{
		T = begInL0V < ( Int32 )( lookAheadL0 ) && firstRow > 0;
	}
	else if ( movementDirection.Y > 0 )
	{
		B = endInL0V > ( Int32 )( m_tileRes - lookAheadL0 ) && lastRow < m_numTilesPerEdge - 1;
	}

	// If needed, begin preloading tiles
	if ( L )
	{
		Uint32 r0 = T ? firstRow - 1 : firstRow;
		Uint32 r1 = B ? lastRow + 1 : lastRow;
		for ( Uint32 row = r0; row <= r1; ++row )
		{
			CTerrainTile* tile = m_terrainGrid[ row * m_numTilesPerEdge + firstCol - 1 ].Get();
			RED_ASSERT( tile, TXT("Tile %u,%u not loaded"), (firstCol - 1), row );
			if ( tile )
			{
				tile->StartAsyncLoad( clipLevel );
			}
		}
	}
	if ( R )
	{
		Uint32 r0 = T ? firstRow - 1 : firstRow;
		Uint32 r1 = B ? lastRow + 1 : lastRow;
		for ( Uint32 row = r0; row <= r1; ++row )
		{
			CTerrainTile* tile = m_terrainGrid[ row * m_numTilesPerEdge + lastCol + 1 ].Get();
			RED_ASSERT( tile, TXT("Tile %u,%u not loaded"), (lastCol + 1), row );
			if ( tile )
			{
				tile->StartAsyncLoad( clipLevel );
			}
		}
	}
	if ( T )
	{
		for ( Uint32 col = firstCol; col <= lastCol; ++col )
		{
			CTerrainTile* tile = m_terrainGrid[ ( firstRow - 1 ) * m_numTilesPerEdge + col ].Get();
			RED_ASSERT( tile, TXT("Tile %u,%u not loaded"), col, (firstRow - 1) );
			if ( tile )
			{
				tile->StartAsyncLoad( clipLevel );
			}
		}
	}
	if ( B )
	{
		for ( Uint32 col = firstCol; col <= lastCol; ++col )
		{
			CTerrainTile* tile = m_terrainGrid[ ( lastRow + 1 ) * m_numTilesPerEdge + col ].Get();
			RED_ASSERT( tile, TXT("Tile %u,%u not loaded"), col, (lastRow + 1) );
			if ( tile )
			{
				tile->StartAsyncLoad( clipLevel );
			}
		}
	}
}


void CClipMap::UpdateWaterLevels( Float baseLevel, const TDynArray< Box >& localWaterBounds )
{
	// We have this stuff cooked.
	if ( IsCooked() )
	{
		return;
	}

	// TODO? If/when we have visibility depth in environments, then we might be able to grab that, instead of the water level.

	// Make sure we have space for a height per tile. Most of the time, this won't do anything since the terrain size
	// doesn't really change.
	m_minWaterHeight.Resize( m_numTilesPerEdge * m_numTilesPerEdge );

	// Fill with base water level
	for ( Uint32 y = 0; y < m_numTilesPerEdge; ++y )
	{
		for ( Uint32 x = 0; x < m_numTilesPerEdge; ++x )
		{
			Uint32 idx = x + y * m_numTilesPerEdge;

			// HACK : Apply forced visibility. Basically, any tiles that have collision forced will also be forced visible.
			// If collision has been forced, then there's probably a good reason (i.e. you're able to reach the ground).
			if ( m_terrainGrid[ idx ].Get() && m_terrainGrid[ idx ]->GetCollisionType() == TTC_ForceOn )
			{
				// Set water height to the lowest point on terrain. While not where the water actually it, it means when
				// we're testing for deep terrain, the tile will not be skipped.
				m_minWaterHeight[ idx ] = m_lowestElevation;
			}
			else
			{
				m_minWaterHeight[ idx ] = baseLevel;
			}
		}
	}

	// Add in each local shape.
	for ( const Box& bounds : localWaterBounds )
	{
		Rect tileRect;
		if ( GetTilesInWorldArea( bounds.Min.AsVector2(), bounds.Max.AsVector2(), tileRect ) )
		{
			Int32 tileMinX = Clamp( tileRect.m_left,   0, (Int32)m_numTilesPerEdge - 1 );
			Int32 tileMaxX = Clamp( tileRect.m_right,  0, (Int32)m_numTilesPerEdge - 1 );
			Int32 tileMinY = Clamp( tileRect.m_top,    0, (Int32)m_numTilesPerEdge - 1 );
			Int32 tileMaxY = Clamp( tileRect.m_bottom, 0, (Int32)m_numTilesPerEdge - 1 );

			for ( Int32 y = tileMinY; y <= tileMaxY; ++y )
			{
				for ( Int32 x = tileMinX; x <= tileMaxX; ++x )
				{
					Float& minWater = m_minWaterHeight[ x + y * m_numTilesPerEdge ];
					minWater = Min( minWater, bounds.Min.Z );
				}
			}
		}
	}

	if ( m_renderProxy != nullptr )
	{
		( new CRenderCommand_UpdateTerrainWaterLevels( m_renderProxy, m_minWaterHeight ) )->Commit();
	}
}


#ifndef NO_EDITOR/*NO_RESOURCE_COOKING*/
void CClipMap::OnCook( class ICookerFramework& cooker )
{
	// Fill in maximum height for each tile.
	const Uint32 numTiles = m_terrainGrid.Size();
	m_tileHeightRanges.Resize( numTiles );
	for ( Uint32 i = 0; i < numTiles; ++i )
	{
		CTerrainTile* tile = m_terrainGrid[i].Get();
		if ( tile != nullptr )
		{
			m_tileHeightRanges[i] = Vector2( GetTileMinimumHeight( tile ), GetTileMaximumHeight( tile ) );
		}
		else
		{
			m_tileHeightRanges[i] = Vector2( m_lowestElevation, m_highestElevation );
		}
	}

	// Fill minimum water levels. Need to go through world and find all local water. Then we can pass it off to UpdateWaterLevels().
	/*{
		TDynArray< Box > localWaterBounds;

		TDynArray< CLayerInfo* > layers;
		GetWorld()->GetWorldLayers()->GetLayers( layers, false, true, true );
		for ( CLayerInfo* layerInfo : layers )
		{
			Bool wasLoaded = layerInfo->IsLoaded();
			if ( !wasLoaded )
			{
				if ( !layerInfo->SyncLoadUnattached() )
				{
					ERR_ENGINE( TXT("Failed to load layer %ls"), layerInfo->GetDepotPath().AsChar() );
					continue;
				}
			}

			TDynArray< CEntity* > entities;
			layerInfo->GetLayer()->GetEntities( entities );

			for ( CEntity* entity : entities )
			{
				if ( entity )
				{
					entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
				}	

				for ( ComponentIterator< CWaterComponent > it( entity ); it; ++it )
				{
					const CAreaComponent::TAreaPoints& points = (*it)->GetWorldPoints();
					if ( !points.Empty() )
					{
						Box bounds( Box::RESET_STATE );

						for ( const Vector& p : points )
						{
							bounds.AddPoint( p );
						}

						localWaterBounds.PushBack( bounds );
					}
				}
			}

			if ( !wasLoaded )
			{
				layerInfo->SyncUnloadUnattached();
			}
		}

		UpdateWaterLevels( WATER_DEFAULT_LEVEL, localWaterBounds );
	}*/

	Cook_ClipmapCache();

	Cook_BuildMipStackBuffer();

	TBaseClass::OnCook( cooker );
}

void CClipMap::Cook_ClipmapCache()
{
	m_cookedData = new CClipMapCookedData();

	// extract data from tiles
	for ( Uint32 y=0; y<m_numTilesPerEdge; ++y )
	{
		for ( Uint32 x=0; x<m_numTilesPerEdge; ++x )
		{
			CTerrainTile* tile = GetTile(x,y);
			if ( tile != nullptr )
			{
				for ( Uint32 mipLevel = 0; mipLevel < tile->GetNumMipLevels(); ++mipLevel )
				{
					const Uint32 mipRes = tile->GetMipMap(mipLevel)->m_resolution;
					if ( (m_clipSize / mipRes) >= 32 )
					{
						BufferHandle colorData, controlData, heightData;
						tile->GetMipMap(mipLevel)->ExtractCookedData( tile->IsCooked(), heightData, colorData, controlData );

						// copy out data
						if ( heightData->GetSize() || colorData->GetSize() || controlData->GetSize() )
						{
							m_cookedData->StoreData( x, y, mipLevel, heightData, colorData, controlData );
						}
					}
				}
			}
		}
	}

	// stats
	LOG_CORE( TXT("Extracted %d terrain buffers (%1.2fKB in total)"), 
		m_cookedData->GetTotalBufferCount(), m_cookedData->GetTotalDataSize() / 1024.0f );
}

void CClipMap::Cook_BuildMipStackBuffer()
{
	const Uint32 mipMapStackTileRes = m_tileRes >> m_numClipmapStackLevels;
	const Uint32 texRes = mipMapStackTileRes*m_numTilesPerEdge;

	// Can't run these through the texture baker, because this data ends up in the same texture array as the clip windows, which
	// needs to be done at runtime.

	m_cookedMipStackHeight.Allocate( texRes * texRes * sizeof( Uint16 ) );
	m_cookedMipStackControl.Allocate( texRes * texRes * sizeof( TControlMapType ) );
	m_cookedMipStackColor.Allocate( TerrainUtils::CalcColorMapSize( texRes, texRes ) );

	Uint16* bakedHeight = static_cast< Uint16* >( m_cookedMipStackHeight.GetData() );
	TControlMapType* bakedControl = static_cast< TControlMapType* >( m_cookedMipStackControl.GetData() );

	// The uncooked tiles hold uncompressed RGBA color maps, so we need to copy it into a temporary buffer, and then compress it
	// into the final baked result.
	TDynArray< TColorMapType > tempColor( texRes * texRes );

	for ( Uint32 r=0; r<m_numTilesPerEdge; ++r )
	{
		for ( Uint32 c=0; c<m_numTilesPerEdge; ++c )
		{
			Bool didLoad = false;
			CTerrainTile* tile = m_terrainGrid[ r * m_numTilesPerEdge + c ].Get();
			if ( !tile )
			{
				ERR_ENGINE( TXT("Tile data for [%u,%u] is missing. Cooked clipmap may have holes."), c, r );
				continue;
			}

			// If the tile has already been cooked, we need to load in the original uncooked version.
			if ( tile->IsCooked() )
			{

				CDiskFile* diskFile = tile->GetFile();
				if ( diskFile == nullptr )
				{
					ERR_ENGINE( TXT("Couldn't get CDiskFile for tile %ls"), tile->GetFriendlyName().AsChar() );
					continue;
				}

				IFile* reader = diskFile->CreateReader();
				if ( reader == nullptr )
				{
					ERR_ENGINE( TXT("Couldn't create reader for tile %ls"), tile->GetFriendlyName().AsChar() );
					continue;
				}

				CDependencyLoader loader( *reader, nullptr );

				DependencyLoadingContext loadingContext;
				loadingContext.m_parent = this;
				if ( !loader.LoadObjects( loadingContext ) )
				{
					delete reader;
					ERR_ENGINE( TXT("Couldn't load tile %ls"), tile->GetFriendlyName().AsChar() );
					continue;
				}

				delete reader;

				if ( loadingContext.m_loadedRootObjects.Size() != 1 )
				{
					ERR_ENGINE( TXT("Unexpected number of loaded objects %u"), loadingContext.m_loadedRootObjects.Size() );
					continue;
				}

				tile = Cast< CTerrainTile >( loadingContext.m_loadedRootObjects[0] );
				if ( tile == nullptr )
				{
					ERR_ENGINE( TXT("Couldn't get re-loaded tile %ls"), tile->GetFriendlyName().AsChar() );
					continue;
				}

				didLoad = true;
			}

			RED_ASSERT( !tile->IsCooked(), TXT("CClipMap must be cooked before its w2ter tiles!") );

			STerrainTileMipMap* mip = const_cast< STerrainTileMipMap* >( tile->GetMipMap( m_numClipmapStackLevels ) );
			if ( mip == nullptr )
			{
				ERR_ENGINE( TXT("Couldn't get mip %u from tile %ls"), m_numClipmapStackLevels, tile->GetFriendlyName() );
				continue;
			}

			BufferHandle heightHandle = mip->AcquireBufferHandleSyncNoTrack( TBT_HeightMap );
			BufferHandle controlHandle = mip->AcquireBufferHandleSyncNoTrack( TBT_ControlMap );
			BufferHandle colorHandle = mip->AcquireBufferHandleSyncNoTrack( TBT_ColorMap );

			const Uint16* sourceTexels = static_cast< const Uint16* >( heightHandle->GetData() );
			const TControlMapType* sourceCMTexels = static_cast< const TControlMapType* >( controlHandle->GetData() );
			const TColorMapType* sourceColorMapTexels = static_cast< const TColorMapType* >( colorHandle->GetData() );

			if ( !sourceTexels || !sourceCMTexels || !sourceColorMapTexels )
			{
				ERR_ENGINE( TXT("Failed to load data for [%u,%u]. Cooked clipmap may have holes."), c, r );
				continue;
			}

			// Copy into compiled buffers
			Uint16* heightTarget = bakedHeight + texRes * ( r * mipMapStackTileRes ) + ( c * mipMapStackTileRes );
			TControlMapType* controlTarget = bakedControl + texRes * ( r * mipMapStackTileRes ) + ( c * mipMapStackTileRes );
			TColorMapType* colorTarget = tempColor.TypedData() + texRes * ( r * mipMapStackTileRes ) + ( c * mipMapStackTileRes );
			for ( Uint32 y = 0; y < mipMapStackTileRes; ++y )
			{
				Red::System::MemoryCopy( heightTarget + y * texRes, sourceTexels + y * mipMapStackTileRes, mipMapStackTileRes * sizeof( Uint16 ) );
				Red::System::MemoryCopy( controlTarget + y * texRes, sourceCMTexels + y * mipMapStackTileRes, mipMapStackTileRes * sizeof( TControlMapType ) );
				// Into temporary buffer, for later compression.
				Red::System::MemoryCopy( colorTarget + y * texRes, sourceColorMapTexels + y * mipMapStackTileRes, mipMapStackTileRes * sizeof( TColorMapType ));
			}


			// If we loaded it in ourselves, destroy it.
			if ( didLoad )
			{
				tile->Discard();
			}
		}
	}

	// Compress tempColor into bakedColor.
	TColorMapRawType* bakedColor = static_cast< TColorMapRawType* >( m_cookedMipStackColor.GetData() );
	TerrainUtils::CompressColor( tempColor.TypedData(), bakedColor, texRes, texRes );
}

#endif // !NO_EDITOR

void CClipMap::SwitchDataBuffersToBufferHandles()
{
	// We do this to ensure that any buffers currently in use
	// by the render proxy (on another thread) aren't invalidated
	// if the clipmap is deleted
	if( !m_cookedMipStackHeightHandle )
	{
		std::function< void( void* ) > deallocator = []( void* ptr )
		{
			// Function takes the size of the buffer, but it's unused 
			CookedMipAllocator::GetInstance().Free( ptr, 0 );
		};

		RED_FATAL_ASSERT( !m_cookedMipStackControlHandle, "Inconsistent clipmap buffer" );
		RED_FATAL_ASSERT( !m_cookedMipStackColorHandle, "Inconsistent clipmap buffer" );

		m_cookedMipStackHeightHandle = BufferHandle( new BufferProxy( m_cookedMipStackHeight.GetData(), m_cookedMipStackHeight.GetSize(), deallocator ) );
		m_cookedMipStackControlHandle = BufferHandle( new BufferProxy( m_cookedMipStackControl.GetData(), m_cookedMipStackControl.GetSize(), deallocator ) );
		m_cookedMipStackColorHandle = BufferHandle( new BufferProxy( m_cookedMipStackColor.GetData(), m_cookedMipStackColor.GetSize(), deallocator ) );

		m_cookedMipStackHeight.ClearWithoutFree();
		m_cookedMipStackControl.ClearWithoutFree();
		m_cookedMipStackColor.ClearWithoutFree();
	}
}

void CClipMap::UELPrefetchTiles( TileCoords& coords, TTerrainTiles& tiles ) const
{
	RED_FATAL_ASSERT( tiles.Size() >= m_terrainGrid.Size(), "Destination cache too small" );

	for ( coords.m_row.Reset(); coords.m_row.IsValid(); ++coords.m_row )
	{
		for ( coords.m_column.Reset(); coords.m_column.IsValid(); ++coords.m_column )
		{
			const Uint32 r = coords.m_row.m_current;
			const Uint32 c = coords.m_column.m_current;

			const Uint32 index = r * m_numTilesPerEdge + c;
			if ( tiles[ index ] == nullptr )
			{
				CTerrainTile* tile = m_terrainGrid[ index ].Get();
				RED_ASSERT( tile, TXT("Tile %u,%u not loaded"), c, r );

				tiles[ index ] = tile;
			}
		}
	}
}

Bool CClipMap::UELNeedsUpdating( Int32 biggestOffset, Float updateCriterion, TileCoords& coords, const TTerrainTiles& tiles, Uint32 l ) const
{
	if ( biggestOffset <= updateCriterion )
	{
		// We didn't move much, but maybe some of the contributing tiles were modified
		for ( coords.m_row.Reset(); coords.m_row.IsValid(); ++coords.m_row )
		{
			for ( coords.m_column.Reset(); coords.m_column.IsValid(); ++coords.m_column )
			{
				const Uint32 r = coords.m_row.m_current;
				const Uint32 c = coords.m_column.m_current;

				const CTerrainTile* tile = tiles[ r * m_numTilesPerEdge + c ];
				RED_ASSERT( tile, TXT("Tile %u,%u not loaded"), c, r );
				if ( tile )
				{
					if ( tile->IsDirty() )
					{
						return true;
					}
					else if ( !tile->WasLoaded( l ) && tile->IsLoaded( l ) )
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

void CClipMap::UELGenerateCollisionForTile( Uint32 l, TileCoords& coords, Box& terrainCollisionBounding, CTerrainTile* tile )
{
	// Make sure that the closest (l==0) tiles have collision data generated and up-to-date. 
	// This can only happen in editor builds!
	// This needs to be done even if we don't have level 0 loaded yet, to make sure things aren't going to fall through
	// the ground (e.g. after a teleport). So, the first time a tile is visited at level 0, this causes a synchronous
	// load... :( But, when we have cooked terrain collision, and no runtime generation, we won't need to worry about it!
	// Also, since we're still trying to load tiles before they're needed, we still shouldn't run into sync loading during
	// normal gameplay.
	if ( l==0 )
	{
		Box collisionTileBB = GetBoxForTile( coords.m_column.m_current, coords.m_row.m_current, 0.f );
		terrainCollisionBounding.AddBox( collisionTileBB );

		if ( !tile->IsCollisionGenerated() || tile->IsDirty() || tile->IsCollisionGenerationPending() )
		{
			GenerateCollisionForTile( coords.m_row.m_current, coords.m_column.m_current );

			// If this tile is dirty, need to regenerate neighbors too. If it's not dirty, then we might have a good reason
			// for not having any collision (collision disabled for the tile), so we don't want to continually generate
			// the neighbors.
			if ( tile->IsDirty() )
			{
				if ( coords.m_row.m_current != coords.m_row.m_last )
				{
					GenerateCollisionForTile( coords.m_row.m_current + 1, coords.m_column.m_current );
				}

				if ( coords.m_column.m_current != coords.m_column.m_last )
				{
					GenerateCollisionForTile( coords.m_row.m_current, coords.m_column.m_current + 1 );
				}

				if ( coords.m_row.m_current != coords.m_row.m_last && coords.m_column.m_current != coords.m_column.m_last )
				{
					GenerateCollisionForTile( coords.m_row.m_current + 1, coords.m_column.m_current + 1 );
				}
			}
		}
	}
}

void CClipMap::UELTrimBounds( Vector& boxMin, Vector& boxMax, Rect& validTexels, const TileCoords& coords, SClipMapLevel& level, Float unitsPerTexel, SClipRegionRectangleData& commonRegionRectData )
{
	// Trim in the direction that's furthest from the tile center
	// TODO : Maybe trim the side that results in the smallest area being excluded?
	Int32 cellOffsetCol = coords.m_column.m_current - ( coords.m_column.m_first + coords.m_column.m_last ) / 2;
	Int32 cellOffsetRow = coords.m_row.m_current - ( coords.m_row.m_first + coords.m_row.m_last ) / 2;

	if ( Abs( cellOffsetCol ) > Abs( cellOffsetRow ) )
	{
		if ( cellOffsetCol < 0 )
		{
			boxMin.X = Max( boxMin.X, ( coords.m_column.m_current + 1 ) * level.m_resolutionOfTheTile * unitsPerTexel + m_terrainCorner.X );
			validTexels.m_left = Max< Int32 >( validTexels.m_left, commonRegionRectData.m_clipRegionXWriteOffset + ( level.m_resolutionOfTheTile - commonRegionRectData.m_offsetInTileU ) );
		}
		else
		{
			boxMax.X = Min( boxMax.X, ( coords.m_column.m_current ) * level.m_resolutionOfTheTile * unitsPerTexel + m_terrainCorner.X );
			validTexels.m_right = Min< Int32 >( validTexels.m_right, commonRegionRectData.m_clipRegionXWriteOffset );
		}
	}
	else
	{
		if ( cellOffsetRow < 0 )
		{
			boxMin.Y = Max( boxMin.Y, ( coords.m_row.m_current + 1 ) * level.m_resolutionOfTheTile * unitsPerTexel + m_terrainCorner.Y );
			validTexels.m_top = Max< Int32 >( validTexels.m_top, commonRegionRectData.m_clipRegionYWriteOffset + ( level.m_resolutionOfTheTile - commonRegionRectData.m_offsetInTileV ) );
		}
		else
		{
			boxMax.Y = Min( boxMax.Y, ( coords.m_row.m_current ) * level.m_resolutionOfTheTile * unitsPerTexel + m_terrainCorner.Y );
			validTexels.m_bottom = Min< Int32 >( validTexels.m_bottom, commonRegionRectData.m_clipRegionYWriteOffset );
		}
	}
}

void CClipMap::UELAppendUpdateRects( SClipmapLevelUpdate* levelupdateInfo, CTerrainTile* tile, Uint32 l, Bool updateLevelRenderResource, Bool updateColorMap, const SClipMapLevel& level, const SClipRegionRectangleData& commonRegionRectData )
{
	ASSERT( tile->HasDataForTileResolution( level.m_resolutionOfTheTile ) );

	// Prevent the tile from unloading this mip level.
	tile->TouchMip( l );

	if ( updateLevelRenderResource )
	{
		Uint32 commonRectDataIndex = levelupdateInfo->m_commonRectData.Size();
		levelupdateInfo->m_commonRectData.PushBack( commonRegionRectData );

		levelupdateInfo->m_updateRects.PushBack( SClipRegionUpdateRectangle() );
		SClipRegionUpdateRectangle& updateRectDesc = levelupdateInfo->m_updateRects.Back();

		// Call Sync version, but it won't cause a load, because we've JUST checked that it's loaded.
		updateRectDesc.m_heightMapBuffer		= tile->GetLevelSyncHMBufferHandle( l );
		updateRectDesc.m_controlMapBuffer		= tile->GetLevelSyncCMBufferHandle( l );
		ASSERT( updateRectDesc.m_heightMapBuffer );
		ASSERT( updateRectDesc.m_controlMapBuffer );

		updateRectDesc.m_resolution			= level.m_resolutionOfTheTile;
		updateRectDesc.m_commonRectDataIndex	= commonRectDataIndex;

		if( updateColorMap )
		{
			levelupdateInfo->m_colormapUpdateRects.PushBack( SColorMapRegionUpdateRectangle() );
			SColorMapRegionUpdateRectangle& colorMapUpdateRectDesc = levelupdateInfo->m_colormapUpdateRects.Back();

			colorMapUpdateRectDesc.m_buffer					= tile->GetLevelSyncColorMapBufferHandle( l );

			colorMapUpdateRectDesc.m_resolution			= level.m_resolutionOfTheTile;
			colorMapUpdateRectDesc.m_commonRectDataIndex	= commonRectDataIndex;
			colorMapUpdateRectDesc.m_cooked					= IsCooked();
		}
	}
}

Bool CClipMap::ValidateLevelUpdate( SClipmapLevelUpdate* update )
{
	Bool success = true;

	const Uint32 numCommonRects = update->m_commonRectData.Size();


	if( update->m_complete )
	{
		if( numCommonRects != 0 )
		{
			RED_LOG_ERROR( RED_LOG_CHANNEL( TerrainClipMaps ), TXT("CClipMap::ValidateLevelUpdate() : %hs with %i common rect instances (should be complete and ==0 or incomplete and >0)"), ( update->m_complete )? "complete" : "incomplete", numCommonRects );
			success = false;
		}
	}
	else
	{
		if( numCommonRects == 0 )
		{
			RED_LOG_ERROR( RED_LOG_CHANNEL( TerrainClipMaps ), TXT("CClipMap::ValidateLevelUpdate() : %hs with %i common rect instances (should be complete and ==0 or incomplete and >0)"), ( update->m_complete )? "complete" : "incomplete", numCommonRects );
			success = false;
		}
	}

	const Uint32 numUpdateRects = update->m_updateRects.Size();
	if( !update->m_complete )
	{
		for( Uint32 i = 0; i < numUpdateRects; ++i )
		{
			SClipRegionUpdateRectangle& rect = update->m_updateRects[ i ];

			if( rect.m_commonRectDataIndex >= numCommonRects )
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( TerrainClipMaps ), TXT("CClipMap::ValidateLevelUpdate() : Invalid common rects array index. Index: %u, Size: %u, updateRect index: %u"), rect.m_commonRectDataIndex, numCommonRects, i );
				success = false;
			}
		}
	}


	return success;
}
