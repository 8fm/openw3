/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "footstepData.h"
#include "game.h"

//////////////////////////////////////////////////////////////////////////

// class CFootstepDataChunk::CDecompressJob : public IJob
// {
// public:
// 	CDecompressJob( CFootstepDataChunk& chunk )
// 		: IJob( JTM_Any, JP_Default )
// 		, m_chunk( chunk )
// 	{
// 
// 	}
// 
// private:
// 	CFootstepDataChunk& m_chunk;
// 
// 	//! Process the job
// 	virtual EJobResult Process()
// 	{
// 		PC_SCOPE( DecompressFootstepDataChunk );
// 
// 		//RED_PROFILING_TIMER( timer );
// 
// 		ASSERT( m_chunk.m_uncompressedData == NULL );
// 		ASSERT( m_chunk.m_compressedData != NULL );
// 
// 		// Create uncompressed data buffer
// 		m_chunk.CreateUncompressedData();
// 		Int32* uncompressedDataPtr = m_chunk.m_uncompressedData;
// 
// 		// Get data
// 		Uint8* data = ( Uint8* ) m_chunk.m_compressedData->GetData();
// 
// 		// Read number of RLE pairs
// 		Uint32 pairs = *( ( Uint16* ) data );
// 		data += 2;
// 
// 		Uint32 bytesWritten = 0;
// 
// 		for( Uint32 i = 0; i < pairs; ++i )
// 		{
// 			// Read num of bytes
// 			Uint8 sameBytesCount = *data++;
// 
// 			// Read actual byte
// 			Uint8 byteToRepeat = *data++;
// 
// 			// Fill memory with bytes
// 			for( Uint32 b = 0; b < sameBytesCount; ++b )
// 			{
// 				*( uncompressedDataPtr++ ) = byteToRepeat;
// 				++bytesWritten;
// 			}
// 		}
// 
// 		ASSERT( bytesWritten == EDGE_SIZE * EDGE_SIZE && "Footstep data!" );
// 		
// 		// Set state to uncompressed
// 		m_chunk.m_isCompressed = false;
// 		m_chunk.m_uncompressing = false;
// 
// 		//LOG_ENGINE( TXT( "DecompressFootstepDataChunk took %.2f ms" ), timer.GetDelta() * 1000.0f );
// 
// 		return JR_Finished;
// 	}
// 
// #ifndef NO_DEBUG_PAGES
// 	virtual const Char* GetDebugName() const { return TXT( "DecompressFootstep" ); }
// 	virtual Color GetDebugColor() const { return Color( 107, 88, 83 ); }
// #endif
// 
// };

//////////////////////////////////////////////////////////////////////////

const Float CFootstepDataChunk::SIZE      = 32.0f;
const Float CFootstepDataChunk::CELL_SIZE = 0.25f;

const Uint32  CFootstepDataChunk::EDGE_SIZE = ( Uint32 ) ( CFootstepDataChunk::SIZE / CFootstepDataChunk::CELL_SIZE );
const Uint32  CFootstepDataChunk::UNCOMPRESSED_DATA_SIZE = CFootstepDataChunk::EDGE_SIZE * CFootstepDataChunk::EDGE_SIZE * sizeof( Int32 );

CFootstepDataChunk::CFootstepDataChunk()
	: m_compressedData( NULL )
	, m_uncompressedData( NULL )
	, m_isCompressed( false )
	, m_uncompressing( false )
{
}

CFootstepDataChunk::~CFootstepDataChunk()
{
	DeleteData();
}

void CFootstepDataChunk::DeleteData()
{
	// Free compressed data
	delete m_compressedData;
	m_compressedData = NULL;

	// Free uncompressed data
	RED_MEMORY_FREE( MemoryPool_Default, MC_FootstepData, m_uncompressedData );
	m_uncompressedData = NULL;

	m_isCompressed = false;
}

void CFootstepDataChunk::DeleteUncompressedData()
{
	ASSERT( ! m_isCompressed );
	ASSERT( m_uncompressedData != NULL );

	// Free uncompressed data
	RED_MEMORY_FREE( MemoryPool_Default, MC_FootstepData, m_uncompressedData );
	m_uncompressedData = NULL;

	// Chunk is now only in compressed form
	m_isCompressed = true;
}

void CFootstepDataChunk::OnSerialize( IFile& file )
{
	// Ignore GC
	if( file.IsGarbageCollector() )
	{
		return;
	}

	if( file.IsReader() )
	{
		DeleteData();

		// Create buffer
		m_compressedData = new DataBuffer( TDataBufferAllocator< MC_FootstepData >::GetInstance() );
	}
	else
	{
		// Make sure data is compressed
		if( ! m_isCompressed )
		{
			Compress();
		}
	}

	ASSERT( m_compressedData != NULL );
	
	// Serialize compressed buffer
	m_compressedData->Serialize( file );

	// Read data is always compressed
	m_isCompressed = true;
}

Int8 CFootstepDataChunk::GetValue( Float relativeX, Float relativeY )
{
	// If data is not yet decompressed - do nothing
	if( m_isCompressed )
	{
		if( ! m_uncompressing )
		{
			UncompressAsync();
		}
		return -1;
	}

	Uint32 x = ( Uint32 ) ( relativeX / CELL_SIZE );
	Uint32 y = ( Uint32 ) ( relativeY / CELL_SIZE );

	ASSERT( x < EDGE_SIZE );
	ASSERT( y < EDGE_SIZE );

	// Get key from table
	return ( Int8 ) m_uncompressedData[ y * EDGE_SIZE + x ];
}

void CFootstepDataChunk::InsertValue( Float relativeX, Float relativeY, Int8 soundMaterial )
{
	ASSERT( ! m_isCompressed );
	ASSERT( soundMaterial < 64 );
	if( m_uncompressedData == NULL )
	{
		CreateUncompressedData();
	}

	Uint32 x = ( Uint32 ) ( relativeX / CELL_SIZE );
	Uint32 y = ( Uint32 ) ( relativeY / CELL_SIZE );

	ASSERT( x < EDGE_SIZE );
	ASSERT( y < EDGE_SIZE );

	m_uncompressedData[ y * EDGE_SIZE + x ] = soundMaterial;
}

void CFootstepDataChunk::CreateUncompressedData()
{
	m_uncompressedData = ( Int32* ) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_FootstepData, UNCOMPRESSED_DATA_SIZE );
	
	// Insert default value
	for( Uint32 i = 0; i < EDGE_SIZE * EDGE_SIZE; ++i )
	{
		m_uncompressedData[ i ] = 0xFF; // it will be truncated to -1 when converting to Int8
	}
}

void CFootstepDataChunk::Compress()
{
	ASSERT( m_uncompressedData != NULL );
	
	// Create temp stream
	TDynArray< Uint8 > bytes;
	bytes.Reserve( EDGE_SIZE * EDGE_SIZE + 2 );

	// Here number of RLE pairs will be written ( 2 bytes )
	bytes.PushBack( 0 );
	bytes.PushBack( 0 );

	// Perform basic RLE compression
	Uint8  sameByteCounter = 1;
	Uint16 pairs = 0;
	Uint16 totalBytes = 0;
	
	for( Uint32 currentByte = 1; currentByte <= EDGE_SIZE * EDGE_SIZE; ++currentByte )
	{
		if( currentByte < EDGE_SIZE * EDGE_SIZE 
			&& m_uncompressedData[ currentByte ] == m_uncompressedData[ currentByte -1 ] )
		{
			// Bytes are the same
			
			// Check if we can increase more
			if( sameByteCounter != 0xFF )
			{
				++sameByteCounter;
				continue;
			}
		}

		// Save previous bytes to stream
		bytes.PushBack( sameByteCounter );
		bytes.PushBack( ( Uint8 ) m_uncompressedData[ currentByte - 1 ] );
		totalBytes += sameByteCounter;

		// Reset same bytes counter
		sameByteCounter = 1;

		// Increase pair counter
		++pairs;
		ASSERT( pairs != 0xFFFF && "Footstep data!" );
	}

	ASSERT( totalBytes == EDGE_SIZE * EDGE_SIZE && "Footstep data!" );

	// Fill number of RLE pairs
	*( ( Uint16* ) bytes.TypedData() ) = pairs;

	ASSERT( bytes.DataSize() % 2 == 0 && "Footstep data!" );

	// Create buffer
	ASSERT( m_compressedData == NULL );
	m_compressedData = new DataBuffer( TDataBufferAllocator< MC_FootstepData >::GetInstance(), static_cast< Uint32 >( bytes.DataSize() ), bytes.TypedData() );

	// Delete uncompressed data
	RED_MEMORY_FREE( MemoryPool_Default, MC_FootstepData, m_uncompressedData );
	m_uncompressedData = NULL;

	// Update flag
	m_isCompressed = true;
}

void CFootstepDataChunk::UncompressAsync()
{
/*	ASSERT( ! m_uncompressing );
	ASSERT( m_isCompressed );

	// Set flag
	m_uncompressing = true;

	// Create job
	VERIFY( SJobManager::GetInstance().Issue( new CDecompressJob( *this ) ) );*/
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CFootstepData )

CFootstepData::CFootstepData()
	: m_chunks( NULL )
	, m_size( 0 )
	, m_lastChunkXY( TPair< Int32, Int32 >( -1, -1 ) )
	, m_sweepCounter( 0 )
{

}

CFootstepData::~CFootstepData()
{
	DeleteChunks();
}

void CFootstepData::OnSerialize( IFile& file )
{
	if( file.IsGarbageCollector() )
	{
		return;
	}

	Uint32 currentOffset = static_cast< Uint32 >( file.GetOffset() );

	// Size
	file << m_size;

	if( file.IsReader() )
	{
		m_chunks = new CFootstepDataChunk*[ m_size * m_size ];
	}

#ifdef NO_TERRAIN_FOOTSTEP_DATA

	// Reset chunks
	Red::System::MemorySet( m_chunks, 0, m_size * m_size * sizeof( CFootstepDataChunk* ) );

	return;

#else

	// Serialize chunks
	for( Uint32 i = 0; i < m_size * m_size; ++i )
	{
		Bool isChunkPresent;

		if( file.IsWriter() )
		{
			// Check if chunk is present
			isChunkPresent = m_chunks[ i ] != NULL;
		}

		// If chunk is present
		file << isChunkPresent;
		if( ! isChunkPresent )
		{
			continue;
		}

		if( file.IsReader() )
		{
			// Create empty chunk ready for serializing in
			m_chunks[ i ] = new CFootstepDataChunk();
		}


		m_chunks[ i ]->OnSerialize( file );
	}

	Uint32 size = static_cast< Uint32 >( file.GetOffset() ) - currentOffset;
	if( size > 0 )
	{
		LOG_ENGINE( TXT( "Footstep data compressed size:: %.2f kB" ), size / 1000.0f );
	}

#endif
}

void CFootstepData::Reset( Uint32 chunksBySize )
{
	// Delete old data
	DeleteChunks();

	// Set size
	m_size = chunksBySize;

	// Create chunks array
	m_chunks = new CFootstepDataChunk*[ m_size * m_size ];
	Red::System::MemorySet( m_chunks, 0, m_size * m_size * sizeof( CFootstepDataChunk* ) );
}

Int8 CFootstepData::GetValue( const Vector& position )
{
	PC_SCOPE( CFootstepDataGetValue );

	Float x = position.X;
	Float y = position.Y;

	// Get chunk (it will modify x and y)
	CFootstepDataChunk* chunk = GetChunk( x, y, false );
	
	if( chunk != NULL )
	{
		// Ask chunk for data
		return chunk->GetValue( x, y );
	}
	else
	{
		return -1;
	}
}

void CFootstepData::InsertValue( Float relativeX, Float relativeY, Int8 soundMaterial )
{
	PC_SCOPE( CFootstepDataInsertValue );

	// Get chunk (it will modify relativeX and Y)
	CFootstepDataChunk* chunk = GetChunk( relativeX, relativeY, true );

	if( chunk != NULL )
	{
		// Set data in chunk
		return chunk->InsertValue( relativeX, relativeY, soundMaterial );
	}
}

void CFootstepData::DeleteChunks()
{
	if( m_chunks == NULL )
	{
		return;
	}

	// Delete chunks
	for( Uint32 i = 0; i < m_size * m_size; ++i )
	{
		delete m_chunks[ i ];
	}

	delete[] m_chunks;
	m_chunks = NULL;
}

CFootstepDataChunk* CFootstepData::GetChunk( Float& relativeX, Float& relativeY, Bool createIfNotPresent )
{
	PC_SCOPE( CFootstepDataGetChunk );

	if( GGame != NULL && GGame->IsActive() )
	{
		// Check if it is time to sweep unused chunks
		++m_sweepCounter;
		if( m_sweepCounter > 500 )
		{
			Sweep();
			m_sweepCounter = 0;
		}
	}

	Uint32 xChunk = ( Uint32 )( relativeX / CFootstepDataChunk::SIZE );
	Uint32 yChunk = ( Uint32 )( relativeY / CFootstepDataChunk::SIZE );

	relativeX -= xChunk * CFootstepDataChunk::SIZE;
	relativeY -= yChunk * CFootstepDataChunk::SIZE;

	if( xChunk < m_size && yChunk < m_size )
	{
		// Store chunk coordinates
		m_lastChunkXY.m_first = xChunk;
		m_lastChunkXY.m_second = yChunk;

		if( createIfNotPresent && m_chunks[ yChunk * m_size + xChunk ] == NULL )
		{
			m_chunks[ yChunk * m_size + xChunk ] = new CFootstepDataChunk();
		}

		// Return chunk
		return m_chunks[ yChunk * m_size + xChunk ];
	}
	else
	{
		ASSERT( 0 && "CFootstepData::GetChunk() request outside terrain!" );
		return NULL;
	}
}

void CFootstepData::Sweep()
{
	// Check if last usage data is valid
	Int32 lastX = m_lastChunkXY.m_first;
	Int32 lastY = m_lastChunkXY.m_second;
	if( lastX < 0 || lastY < 0 )
	{
		return;
	}

	// Go through all chunks
	for( Uint32 y = 0; y < m_size; ++y )
	{
		for( Uint32 x = 0; x < m_size; ++x )
		{
			CFootstepDataChunk* chunk = m_chunks[ y * m_size + x ];
			
			// Ignore empty chunks
			if( chunk == NULL )
			{
				continue;
			}

			// Ignore compressed chunks
			if( chunk->IsCompressed() )
			{
				continue;
			}

			// Ignore chunks close enough to last used
			if( Abs< Int32 >( lastX - x ) < 3 && Abs< Int32 >( lastY - y ) < 3 )
			{
				continue;
			}

			// Delete uncompressed data from chunk
			chunk->DeleteUncompressedData();
		}
	}
}
