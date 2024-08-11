/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "undoTerrain.h"
#include "undoManager.h"
#include "../../common/engine/terrainTile.h"
#include "../../common/engine/foliageEditionController.h"
#include "../../common/engine/terrainUtils.h"


/// Copy the data in rect from srcData to dstData. dstData is assumed to be sized to hold the cropped data. srcWidth
/// is the full width (in elements T) of srcData.
template< typename T >
static void CopyCropped( T* dstData, const T* srcData, Uint32 srcWidth, const Rect& rect )
{
	Uint32 targetWidth = rect.Width();
	ASSERT( targetWidth <= srcWidth );

	T* writePtr = dstData;
	const T* readPtr = &srcData[rect.m_top * srcWidth + rect.m_left];

	if ( srcWidth == targetWidth )
	{
		// If the source and dest have the same width, we can just do a single memcpy!
		Red::System::MemoryCopy( writePtr, readPtr, targetWidth * rect.Height() * sizeof( T ) );
	}
	else
	{
		for ( Int32 sourceRow = rect.m_top; sourceRow < rect.m_bottom; ++sourceRow )
		{
			Red::System::MemoryCopy( writePtr, readPtr, targetWidth * sizeof( T ) );
			writePtr += targetWidth;
			readPtr += srcWidth;
		}
	}
}


//////////////////////////////////////////////////////////////////////////


IMPLEMENT_ENGINE_CLASS( CUndoTerrain );


CUndoTerrain::SToolHistory::SToolHistory( Uint32 gridRow, Uint32 gridCol, CTerrainTile* tile )
	: m_tileGridCol( gridCol )
	, m_tileGridRow( gridRow )
	, m_isCropped( false )
	, m_subjectTile( tile )
	, m_hmData( NULL )
	, m_cmData( NULL )
	, m_colorData( NULL )
	, m_valid( true )
{
	m_tileRes = tile->GetResolution();
	m_colorRes = tile->GetHighestColorMapResolution();

	// Initialize extents of the stroke and brush
	m_strokesExtents = Rect( m_tileRes, 0, m_tileRes, 0 );
	m_strokesExtentsColor = Rect( m_colorRes, 0, m_colorRes, 0 );
}

CUndoTerrain::SToolHistory::SToolHistory( SToolHistory&& other )
	: m_subjectTile( other.m_subjectTile )
	, m_tileGridRow( other.m_tileGridRow )
	, m_tileGridCol( other.m_tileGridCol )
	, m_tileRes( other.m_tileRes )
	, m_colorRes( other.m_colorRes )
	, m_colorMip( other.m_colorMip )
	, m_hmData( other.m_hmData )
	, m_cmData( other.m_cmData )
	, m_colorData( other.m_colorData )
	, m_isCropped( other.m_isCropped )
	, m_strokesExtents( other.m_strokesExtents )
	, m_strokesExtentsColor( other.m_strokesExtentsColor )
	, m_valid( other.m_valid )
{
	// Explicitly clear out other's pointers, so they won't get freed while we're using them
	other.m_hmData		= NULL;
	other.m_cmData		= NULL;
	other.m_colorData	= NULL;
	other.m_subjectTile	= NULL;
}


CUndoTerrain::SToolHistory::~SToolHistory()
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, m_hmData );
	RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, m_cmData );
	RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, m_colorData );
}


void CUndoTerrain::SToolHistory::InitFromTile( Bool hm, Bool cm, Bool color, Bool collisionType )
{
	CTerrainTile* tile = m_subjectTile.Get();
	if ( !tile ) return;

	if ( hm )
	{
		size_t hmSize = m_tileRes * m_tileRes * sizeof( Uint16 );
		m_hmData = static_cast< Uint16* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, hmSize ) );
		const Uint16* srcHM = tile->GetLevelSyncHM( 0 );
		Red::System::MemoryCopy( m_hmData, srcHM, hmSize );
	}

	if ( cm )
	{
		size_t cmSize = m_tileRes * m_tileRes * sizeof( TControlMapType );
		m_cmData = static_cast< TControlMapType* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, cmSize ) );
		const TControlMapType* srcCM = tile->GetLevelSyncCM( 0 );
		Red::System::MemoryCopy( m_cmData, srcCM, cmSize );
	}

	if ( color )
	{
		size_t colorSize = m_colorRes * m_colorRes * sizeof( TColorMapType );
		m_colorData = static_cast< TColorMapType* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, colorSize ) );
		const TColorMapType* srcColor = static_cast< const TColorMapType* >( tile->GetHighestColorMapData( nullptr, &m_colorMip ) );
		Red::System::MemoryCopy( m_colorData, srcColor, colorSize );
	}

	if ( collisionType )
	{
		m_collisionType = tile->GetCollisionType();
	}
	m_haveCollisionType = collisionType;
}

void CUndoTerrain::SToolHistory::CropData()
{
	ASSERT( !m_isCropped );
	if ( m_isCropped ) return;

	if ( m_hmData )
	{
		size_t hmSize = m_strokesExtents.Width() * m_strokesExtents.Height() * sizeof( Uint16 );
		Uint16* croppedHM = ( Uint16* )RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, hmSize );
		CopyCropped( croppedHM, m_hmData, m_tileRes, m_strokesExtents );
		RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, m_hmData );
		m_hmData = croppedHM;
	}
	if ( m_cmData )
	{
		size_t cmSize = m_strokesExtents.Width() * m_strokesExtents.Height() * sizeof( TControlMapType );
		TControlMapType* croppedCM = ( TControlMapType* )RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, cmSize );
		CopyCropped( croppedCM, m_cmData, m_tileRes, m_strokesExtents );
		RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, m_cmData );
		m_cmData = croppedCM;
	}
	if ( m_colorData )
	{
		size_t colorSize = m_strokesExtentsColor.Width() * m_strokesExtentsColor.Height() * sizeof( TColorMapType );
		TColorMapType* croppedColor = ( TColorMapType* )RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, colorSize );
		CopyCropped( croppedColor, m_colorData, m_colorRes, m_strokesExtentsColor );
		RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, m_colorData );
		m_colorData = croppedColor;
	}

	m_isCropped = true;
}

void CUndoTerrain::SToolHistory::SwapWithTile()
{
	ASSERT( m_isCropped );
	if ( !m_isCropped ) return;

	// Get data to modify
	CTerrainTile* tile = m_subjectTile.Get();
	if ( !tile ) return;

	// Grab current data from the tile.
	Uint16* tempHM = NULL;
	if ( m_hmData )
	{
		size_t hmSize = m_strokesExtents.Width() * m_strokesExtents.Height() * sizeof( Uint16 );
		tempHM = ( Uint16* )RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, hmSize );
		const Uint16* srcHM = tile->GetLevelSyncHM( 0 );
		CopyCropped( tempHM, srcHM, m_tileRes, m_strokesExtents );
	}

	TControlMapType* tempCM = NULL;
	if ( m_cmData )
	{
		size_t cmSize = m_strokesExtents.Width() * m_strokesExtents.Height() * sizeof( TControlMapType );
		tempCM = ( TControlMapType* )RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, cmSize );
		const TControlMapType* srcCM = tile->GetLevelSyncCM( 0 );
		CopyCropped( tempCM, srcCM, m_tileRes, m_strokesExtents );
	}

	TColorMapType* tempColor = NULL;
	if ( m_colorData )
	{
		size_t colorSize = m_strokesExtentsColor.Width() * m_strokesExtentsColor.Height() * sizeof( TColorMapType );
		tempColor = ( TColorMapType* )RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, colorSize );
		const TColorMapType* srcColor = static_cast< const TColorMapType* >( tile->GetLevelSyncColorMap( m_colorMip ) );
		CopyCropped( tempColor, srcColor, m_colorRes, m_strokesExtentsColor );
	}


	// Set the stored data into the tile.
	if ( m_hmData || m_cmData )
	{
		Rect sourceBufferRect( 0, m_strokesExtents.Width(), 0, m_strokesExtents.Height() );
		tile->SetData( m_strokesExtents, sourceBufferRect, m_hmData, m_cmData, m_strokesExtents.Width() );
		tile->InvalidateCollision();
	}
	if ( m_colorData )
	{
		Rect sourceColorRect( 0, m_strokesExtentsColor.Width(), 0, m_strokesExtentsColor.Height() );
		tile->SetColorMapData( m_strokesExtentsColor, sourceColorRect, m_colorData, m_strokesExtentsColor.Width() * sizeof( TColorMapType ), m_colorMip );
	}

	// Delete old HM/CM data, and set pointers to the data we just grabbed. Swap success!
	RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, m_hmData );
	m_hmData = tempHM;

	RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, m_cmData );
	m_cmData = tempCM;

	RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, m_colorData );
	m_colorData = tempColor;

	// Setting data will automatically cause mipmap rebuild and stuff...

	if ( m_haveCollisionType )
	{
		ETerrainTileCollision temp = tile->GetCollisionType();
		tile->SetCollisionType( m_collisionType );
		m_collisionType = temp;
	}
}

void CUndoTerrain::SToolHistory::AddStroke( const Rect& strokeExtents, const Rect& strokeExtentsColor )
{
	if ( !strokeExtents.IsEmpty() ) // prevent void extends (e.g. EMPTY which is (0,0,0,0)) from affecting the combined result
	{
		m_strokesExtents.m_left			= Min( strokeExtents.m_left,		m_strokesExtents.m_left );
		m_strokesExtents.m_right		= Max( strokeExtents.m_right,		m_strokesExtents.m_right );
		m_strokesExtents.m_top			= Min( strokeExtents.m_top,			m_strokesExtents.m_top );
		m_strokesExtents.m_bottom		= Max( strokeExtents.m_bottom,		m_strokesExtents.m_bottom );
		ASSERT( m_strokesExtents.Width() >= 0 && m_strokesExtents.Height() >= 0 );
	}

	if ( !strokeExtentsColor.IsEmpty() )
	{
		m_strokesExtentsColor.m_left	= Min( strokeExtentsColor.m_left,	m_strokesExtentsColor.m_left );
		m_strokesExtentsColor.m_right	= Max( strokeExtentsColor.m_right,	m_strokesExtentsColor.m_right );
		m_strokesExtentsColor.m_top		= Min( strokeExtentsColor.m_top,	m_strokesExtentsColor.m_top );
		m_strokesExtentsColor.m_bottom	= Max( strokeExtentsColor.m_bottom,	m_strokesExtentsColor.m_bottom );
		ASSERT( m_strokesExtentsColor.Width() >= 0 && m_strokesExtentsColor.Height() >= 0 );
	}
}


void CUndoTerrain::CreateStep( CEdUndoManager* undoManager, const String& toolName, CClipMap* terrain, Bool hm, Bool cm, Bool color, Bool collisionType )
{
	ASSERT( undoManager );

	CUndoTerrain* stepToAdd = Cast< CUndoTerrain >( undoManager->GetStepToAdd() );
	if ( !stepToAdd )
	{
		undoManager->SetStepToAdd( stepToAdd = new CUndoTerrain( *undoManager, toolName ) );
	}

	ASSERT( stepToAdd );
	if ( !stepToAdd )
	{
		return;
	}

	ASSERT( !stepToAdd->m_initFinished );
	if ( stepToAdd->m_initFinished )
	{
		stepToAdd->PushStep();
		return;
	}

	stepToAdd->m_useHM = hm;
	stepToAdd->m_useCM = cm;
	stepToAdd->m_useColor = color;
	stepToAdd->m_useCollisionType = collisionType;
}

void CUndoTerrain::FinishStep( CEdUndoManager* undoManager )
{
	ASSERT( undoManager );
	CUndoTerrain* stepToAdd = Cast< CUndoTerrain >( undoManager->GetStepToAdd() );

	// If there is no stepToAdd than no changes were made (no CreateStepInit called)
	if ( !stepToAdd )
	{
		return;
	}

	if ( stepToAdd->m_history.Empty() )
	{
		return;
	}

	ASSERT( !stepToAdd->m_initFinished );
	stepToAdd->m_initFinished = true;

	// Invalidate empty strokes
	Uint32 valids = stepToAdd->m_history.Size();
	for ( Int32 i=stepToAdd->m_history.SizeInt() - 1; i >= 0; --i )
	{
		if ( stepToAdd->m_history[i].m_strokesExtents.IsEmpty() && stepToAdd->m_history[i].m_strokesExtentsColor.IsEmpty()  )
		{
			stepToAdd->m_history[i].m_valid = false;
			valids--;
		}
	}

	// Check again for empty history
	if ( valids == 0 )
	{
		return;
	}

	for ( Uint32 i = 0; i < stepToAdd->m_history.Size(); ++i )
	{
		SToolHistory& history = stepToAdd->m_history[i];
		if ( history.m_valid )
		{
			history.CropData();
		}
	}

	stepToAdd->PushStep();
}


void CUndoTerrain::AddStroke( CEdUndoManager* undoManager, CTerrainTile* tile, Uint32 gridRow, Uint32 gridCol, const Rect& strokeExtents, const Rect& strokeExtentsColor )
{
	CUndoTerrain* currentStep = Cast< CUndoTerrain >( undoManager->GetStepToAdd() );
	// If there is no currentStep, then we can't add a stroke to anything
	if ( !currentStep ) return;

	for ( Uint32 i = 0; i < currentStep->m_history.Size(); ++i )
	{
		SToolHistory& history = currentStep->m_history[i];
		if ( history.m_tileGridRow == gridRow && history.m_tileGridCol == gridCol )
		{
			history.AddStroke( strokeExtents, strokeExtentsColor );
			return;
		}
	}

	currentStep->m_history.PushBack( SToolHistory( gridRow, gridCol, tile ) );
	currentStep->m_history.Back().InitFromTile( currentStep->m_useHM, currentStep->m_useCM, currentStep->m_useColor, currentStep->m_useCollisionType );
	currentStep->m_history.Back().AddStroke( strokeExtents, strokeExtentsColor );
}


CUndoTerrain::CUndoTerrain( CEdUndoManager& undoManager, const String& toolName )
	: IUndoStep( undoManager )
	, m_initFinished( false )
	, m_toolName( toolName )
	, m_useHM( false )
	, m_useCM( false )
	, m_useColor( false )
{}

void CUndoTerrain::DoUndo()
{
	for ( Uint32 i = 0; i < m_history.Size(); ++i )
	{
		SToolHistory& history = m_history[i];
		if ( history.m_valid )
		{
			history.SwapWithTile();
		}
	}
	if ( m_undoManager->GetWorld() )
	{
		CFoliageEditionController & foliage = m_undoManager->GetWorld()->GetFoliageEditionController();
		foliage.RefreshGrassMask();
	}
}

void CUndoTerrain::DoRedo()
{
	for ( Uint32 i = 0; i < m_history.Size(); ++i )
	{
		SToolHistory& history = m_history[i];
		if ( history.m_valid )
		{
			history.SwapWithTile();
		}
	}
	if ( m_undoManager->GetWorld() )
	{
		CFoliageEditionController & foliage = m_undoManager->GetWorld()->GetFoliageEditionController();
		foliage.RefreshGrassMask();
	}
}


String CUndoTerrain::GetName()
{
	return m_toolName;
}


// Read data in from file. If there is no data to read, return NULL. If the size of the data doesn't match
// the given expectedSize, file will seek forward to end of stored data, and return NULL.
template< typename T >
static T* ReadBuffer( IFile& file, size_t expectedSize )
{
	ASSERT( file.IsReader() );
	if ( !file.IsReader() ) return NULL;

	T* buffer = NULL;

	size_t sizeInFile = 0;
	file << sizeInFile;
	if ( sizeInFile > 0 )
	{
		ASSERT( expectedSize == sizeInFile, TXT("Size of saved buffer (%d) does not match what we expected (%d). Skipping data in file, but not creating buffer."), sizeInFile, expectedSize );
		if ( sizeInFile != expectedSize )
		{
			file.Seek( file.GetOffset() + sizeInFile );
		}
		else
		{
			buffer = static_cast< T* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, sizeInFile ) );
			file.Serialize( buffer, sizeInFile );
		}
	}

	return buffer;
}

// Write data out to file, and free the allocated memory.
template< typename T >
static void WriteBuffer( IFile& file, T*& data, size_t dataSize )
{
	ASSERT( file.IsWriter() );
	if ( !file.IsWriter() ) return;

	file << dataSize;
	if ( dataSize > 0 )
	{
		file.Serialize( data, dataSize );

		RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, data );
		data = NULL;
	}
}

void CUndoTerrain::OnSerialize( IFile& file )
{
	IUndoStep::OnSerialize( file );

	if ( file.IsGarbageCollector() ) return;

	if ( !file.IsReader() && !file.IsWriter() ) return;

	Uint32 numHistory = m_history.Size();
	file << numHistory;

	if ( file.IsReader() )
	{
		ASSERT( numHistory == m_history.Size() );
	}

	for ( Uint32 i = 0; i < numHistory; ++i )
	{
		SToolHistory& history = m_history[i];

		file << history.m_valid;

		if ( !history.m_valid )
		{
			continue;
		}

		size_t fullWidth	= history.m_isCropped ? history.m_strokesExtents.Width() : history.m_tileRes;
		size_t fullHeight	= history.m_isCropped ? history.m_strokesExtents.Height() : history.m_tileRes;
		size_t colorWidth	= history.m_isCropped ? history.m_strokesExtentsColor.Width() : history.m_colorRes;
		size_t colorHeight	= history.m_isCropped ? history.m_strokesExtentsColor.Height() : history.m_colorRes;

		// If we're reading, we put how big we expect the data to be. If there's a different amount, then the pointer
		// will be NULL'd and the data skipped. This works for when that buffer didn't exist during writing, since its
		// recorded size will be 0, which will be different from the size found here, so we still won't have the buffer
		// after reading.
		size_t hmSize		= ( file.IsReader() || history.m_hmData ) ? fullWidth * fullHeight * sizeof( Uint16 ) : 0;
		size_t cmSize		= ( file.IsReader() || history.m_cmData ) ? fullWidth * fullHeight * sizeof( TControlMapType ) : 0;
		size_t colorSize	= ( file.IsReader() || history.m_colorData ) ? colorWidth * colorHeight * sizeof( TColorMapType ) : 0;

		if ( file.IsReader() )
		{
			// If we're reading back, we shouldn't have any buffers allocated.
			ASSERT( !history.m_hmData && !history.m_cmData && !history.m_colorData );
			RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, history.m_hmData );
			RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, history.m_cmData );
			RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, history.m_colorData );

			history.m_hmData =		ReadBuffer< Uint16 >( file, hmSize );
			history.m_cmData =		ReadBuffer< TControlMapType >( file, cmSize );
			history.m_colorData =	ReadBuffer< TColorMapType >( file, colorSize );
		}
		else
		{
			WriteBuffer( file, history.m_hmData, hmSize );
			WriteBuffer( file, history.m_cmData, cmSize );
			WriteBuffer( file, history.m_colorData, colorSize );
		}
	}
}
