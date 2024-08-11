/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "chunkedLZ4File.h"
#include "compression/lz4.h"
#include "../redSystem/utilityTimers.h"

//#define ENABLE_COMPRESSION_TIMING

const Uint32 c_headerTestValue = 'CLZF';

CChunkedLZ4FileWriter::CChunkedLZ4FileWriter( Uint32 compressedChunkSize, Uint32 maximumChunks, IFile* streamOut )
	: IFileEx( FF_Writer | FF_NoInlineSkipBlocks )
	, m_streamOut( streamOut )
	, m_bytesCached( 0 )
	, m_fileSizeAfterClose( 0 )
{
	m_chunkBuffer = Red::CreateUniqueBuffer( compressedChunkSize, 8, MC_Temporary );
	RED_FATAL_ASSERT( m_chunkBuffer.Get(), "Failed to allocate memory for compressed chunk" );

	m_chunkHeaderData.Reserve( maximumChunks );
	m_originalFileOffset = static_cast< Uint32 >( streamOut->GetOffset() );

	// Reserve space for the header + seek in the file
	MemSize streamHeaderBytesRequired = sizeof( c_headerTestValue ) + sizeof( Uint32 ) + ( maximumChunks * sizeof( CChunkedLZ4ChunkMetadata ) );
	m_streamOut->Seek( m_originalFileOffset + streamHeaderBytesRequired );

	// First chunk offset immediately after header
	m_chunkStartOffset = static_cast< Uint32 >( m_streamOut->GetOffset() );

	// Reserve space for compression
	MemSize compressionBufferSizeRequired = Red::Core::Compressor::CLZ4::GetRequiredAllocSize( static_cast< Uint32 >( compressedChunkSize ) );
	m_tempCompressedBuffer = Red::CreateUniqueBuffer( static_cast< Uint32 >( compressionBufferSizeRequired ), 8, MC_Temporary );
}

CChunkedLZ4FileWriter::~CChunkedLZ4FileWriter()
{

}

void CChunkedLZ4FileWriter::Close()
{
	FlushToStream();

	// Write the header (uncompressed)
	m_streamOut->Seek( m_originalFileOffset );

	Uint32 magic = c_headerTestValue;
	*m_streamOut << magic;

	Uint32 chunkCount = m_chunkHeaderData.Size();
	*m_streamOut << chunkCount;

	for( auto chunkdata : m_chunkHeaderData )
	{
		m_streamOut->Serialize( &chunkdata, sizeof( chunkdata ) );
	}

	m_fileSizeAfterClose = static_cast< Uint32 >( m_streamOut->GetSize() );
	delete m_streamOut;
	m_streamOut = nullptr;
}

const void* CChunkedLZ4FileWriter::GetBuffer() const
{
	return nullptr;
}

size_t CChunkedLZ4FileWriter::GetBufferCapacity() const
{
	return 0;
}

void CChunkedLZ4FileWriter::FlushToStream()
{
	if( m_bytesCached == 0 )
	{
		return ;		// Don't care, nothing to write
	}

	RED_FATAL_ASSERT( m_chunkHeaderData.Size() + 1 < m_chunkHeaderData.Capacity(), "Maximum chunk count must be increased, this file will be corrupt" );
	RED_FATAL_ASSERT( m_tempCompressedBuffer.Get(), "No compressed buffer reserved" );
	RED_FATAL_ASSERT( m_chunkBuffer.Get(), "No decompressed buffer reserved" );

	if( m_tempCompressedBuffer.Get() == nullptr )
	{
		WARN_CORE( TXT( "Cannot flush CChunkedLZ4FileWriter stream when compressed buffer is empty" ) );
		return;
	}

#ifdef ENABLE_COMPRESSION_TIMING
	Red::System::StopClock compressionTimer;
#endif

	// Compress cache
	MemSize sizeCompressed = Red::Core::Compressor::CLZ4::CompressToPreAllocatedBuffer( m_chunkBuffer.Get(), m_bytesCached, m_tempCompressedBuffer.Get(), m_tempCompressedBuffer.GetSize() );
	
#ifdef ENABLE_COMPRESSION_TIMING
	Double compressTime = compressionTimer.GetDelta(); 
#endif

	// Cache header
	CChunkedLZ4ChunkMetadata metadata;
	metadata.m_compressedOffset = static_cast< Uint32 >( m_streamOut->GetOffset() );
	metadata.m_compressedSize = static_cast< Uint32 >( sizeCompressed ) ;
	metadata.m_uncompressedSize = m_bytesCached;
	m_chunkHeaderData.PushBack( metadata );

	// Stream out
	m_streamOut->Serialize( m_tempCompressedBuffer.Get(), sizeCompressed );

	// Reset state
	m_chunkStartOffset += m_bytesCached;
	m_bytesCached = 0;

#ifdef ENABLE_COMPRESSION_TIMING
	const Double oneMillisecond = 1.0 / 1000.0f;
	LOG_CORE( TXT( "CChunkedLZ4FileWriter::FlushToStream took %1.5fms (compression took %1.5fms )" ),
			 compressionTimer.GetDelta() / oneMillisecond, compressTime / oneMillisecond );
#endif
}

void CChunkedLZ4FileWriter::Serialize( void* buffer, size_t size )
{
	size_t bytesWritten = 0;
	while( bytesWritten < size )
	{
		size_t bytesToWrite = Red::Math::NumericalUtils::Min( m_chunkBuffer.GetSize() - m_bytesCached, static_cast< Uint32 >( size - bytesWritten ) );
		if( bytesToWrite > 0 )
		{
			Red::System::MemoryCopy( OffsetPtr( m_chunkBuffer.Get(), m_bytesCached ), reinterpret_cast< void* >( reinterpret_cast< MemUint >( buffer ) + bytesWritten ), bytesToWrite );
			m_bytesCached += static_cast< Uint32 >( bytesToWrite );
			bytesWritten += bytesToWrite;
		}
		if( m_bytesCached == m_chunkBuffer.GetSize() )
		{
			FlushToStream();
		}
	}
}

Uint64 CChunkedLZ4FileWriter::GetOffset() const
{
	return m_chunkStartOffset + m_bytesCached;
}

Uint64 CChunkedLZ4FileWriter::GetSize() const
{
	if( m_streamOut != nullptr )
	{
		return m_streamOut->GetSize();
	}
	else
	{
		return m_fileSizeAfterClose;
	}
}

void CChunkedLZ4FileWriter::Seek( Int64 offset )
{
	RED_FATAL_ASSERT( false, "This file writer interface does not support seeking" );
}

//////////////////////////////////////////////////////////////////////////

CChunkedLZ4FileReader::CChunkedLZ4FileReader( IFile* streamIn )
	: IFile( FF_Reader | FF_NoInlineSkipBlocks )
	, m_rawStream( streamIn )
	, m_currentChunk( m_chunkHeaderData.End() )
{
	if( LoadChunkMetadata() )
	{
		PrepareForReading();

		if( m_chunkHeaderData.Size() > 0 )		// Pre-cache first block
		{
			DecompressDataForReading( m_chunkHeaderData.Begin() );
		}
	}
}

CChunkedLZ4FileReader::~CChunkedLZ4FileReader()
{
	delete m_rawStream;
}

void CChunkedLZ4FileReader::PrepareForReading()
{
	// Calculate the largest decompressed buffer required, then pre-allocate
	Uint32 maxDecompressedBufferSize = 0;
	Uint32 maxCompressedBufferSize = 0;
	Uint32 firstChunkOffset = (Uint32)-1;
	for( auto chunk : m_chunkHeaderData )
	{
		maxDecompressedBufferSize = Red::Math::NumericalUtils::Max( maxDecompressedBufferSize, chunk.m_uncompressedSize );
		maxCompressedBufferSize = Red::Math::NumericalUtils::Max( maxCompressedBufferSize, chunk.m_compressedSize );
		firstChunkOffset = Red::Math::NumericalUtils::Min( firstChunkOffset, chunk.m_compressedOffset );
	}

	// Pre-calculate uncompressed offsets
	Uint32 uncompressedOffset = firstChunkOffset;
	for( auto& chunk : m_chunkHeaderData )
	{
		chunk.m_uncompressedOffset = uncompressedOffset;
		uncompressedOffset += chunk.m_uncompressedSize;
	}
	m_decompressedFileSize = uncompressedOffset;

	RED_LOG( Save, TXT( "Pre-allocating %d bytes for CChunkedLZ4FileReader buffer (%d compressed, %d decompressed)" ), 
					    maxCompressedBufferSize + maxDecompressedBufferSize, maxCompressedBufferSize, maxDecompressedBufferSize );

	m_decompressedBuffer.Reserve( maxDecompressedBufferSize );
	m_decompressedBuffer.Resize( maxDecompressedBufferSize );

	m_compressedBuffer = Red::CreateUniqueBuffer( maxCompressedBufferSize, 8, MC_Temporary );

	m_currentChunk = m_chunkHeaderData.End();
}

Bool CChunkedLZ4FileReader::LoadChunkMetadata()
{
	Uint32 magic = 0;
	*m_rawStream << magic;
	if( magic != c_headerTestValue )
	{
		RED_ASSERT( false, TXT( "Save game data is corrupt or incomplete!" ) );
		ERR_CORE( TXT( "Failed to load compressed chunk metadata" ) );
		return false;
	}

	Uint32 chunkCount = 0;
	*m_rawStream << chunkCount;

	for( Uint32 c = 0; c < chunkCount; ++c )
	{
		CChunkedLZ4ChunkMetadata metadata;
		m_rawStream->Serialize( &metadata, sizeof( metadata ) );

		// Uncompressed offset calculated later
		ChunkMetadata chunkData;
		chunkData.m_compressedOffset = metadata.m_compressedOffset;
		chunkData.m_compressedSize = metadata.m_compressedSize;
		chunkData.m_uncompressedSize = metadata.m_uncompressedSize;
		m_chunkHeaderData.PushBack( chunkData );
	}

	auto sortMetadataByStartOffset = []( const ChunkMetadata& a, const ChunkMetadata& b )
	{
		return a.m_compressedOffset < b.m_compressedOffset;
	};
	Sort( m_chunkHeaderData.Begin(), m_chunkHeaderData.End(), sortMetadataByStartOffset );

#ifndef RED_FINAL_BUILD
	// Validate that all compressed data sits flush next to each other
	for( Uint32 c=0; c + 1 < m_chunkHeaderData.Size(); ++c )
	{
		RED_FATAL_ASSERT( ( m_chunkHeaderData[c].m_compressedOffset + m_chunkHeaderData[c].m_compressedSize ) == m_chunkHeaderData[ c + 1 ].m_compressedOffset, "Chunks are not contiguous" );
	}
#endif

	return true;
}

Bool CChunkedLZ4FileReader::DecompressDataForReading( ChunkHeaderMetadata::iterator chunk )
{
#ifdef ENABLE_COMPRESSION_TIMING
	Red::System::StopClock compressionTimer;
	Double readTime = 0;
#endif

	if( chunk != m_chunkHeaderData.End() )
	{
		Uint32 bufferSizeRequired = chunk->m_uncompressedSize;
		RED_FATAL_ASSERT( bufferSizeRequired <= m_decompressedBuffer.Capacity(), "Decompression buffer is too small?!" );
		m_decompressedBuffer.ResizeFast( bufferSizeRequired );

		Uint32 compressedBufferSize = chunk->m_compressedSize;
		RED_FATAL_ASSERT( compressedBufferSize <= m_compressedBuffer.GetSize(), "Compression buffer is too small?!" );

		m_rawStream->Seek( chunk->m_compressedOffset );
		m_rawStream->Serialize( m_compressedBuffer.Get(), compressedBufferSize );

#ifdef ENABLE_COMPRESSION_TIMING
		readTime = compressionTimer.GetDelta();
#endif

		Red::Core::Decompressor::CLZ4 decompressor;
		auto decompressResult = decompressor.Initialize( m_compressedBuffer.Get(), m_decompressedBuffer.TypedData(), compressedBufferSize, bufferSizeRequired );
		if( decompressResult != Red::Core::Decompressor::Status_Success )
		{
			RED_FATAL_ASSERT( false, "Failed to initialise decompressor" );
			ERR_CORE( TXT( "Failed to initialise LZ4 decompressor - %d" ), decompressResult );
			return false;
		}
		decompressResult = decompressor.Decompress();
		if( decompressResult != Red::Core::Decompressor::Status_Success )
		{
			RED_FATAL_ASSERT( false, "Failed to decompress data from file" );
			ERR_CORE( TXT( "Failed to decompress data from file - %d" ), decompressResult );
			return false;
		}
		m_currentChunk = chunk;
		m_localChunkOffset = 0;
	}

#ifdef ENABLE_COMPRESSION_TIMING
	const Double oneMillisecond = 1.0 / 1000.0f;
	LOG_CORE( TXT( "CChunkedLZ4FileReader::DecompressDataForReading took %1.5fms (serialisation took %1.5fms )" ),
		compressionTimer.GetDelta() / oneMillisecond, readTime / oneMillisecond );
#endif

	return true;
}

void CChunkedLZ4FileReader::Serialize( void* buffer, size_t size )
{
	RED_ASSERT( m_currentChunk != m_chunkHeaderData.End(), TXT( "No chunk to read from. Savegame is probably incomplete or corrupt" ) );
	RED_ASSERT( m_decompressedBuffer.Size(), TXT( "No decompressed data to read from. Savegame is probably incomplete or corrupt" ) );

	Uint32 bytesRead = 0;
	Uint8* targetBuffer = static_cast< Uint8* >( buffer );
	while( bytesRead < size && m_decompressedBuffer.Size() > 0 && m_currentChunk != m_chunkHeaderData.End() )
	{
		// Clamp bytes to read to buffer remaining
		Uint32 bytesToRead = Red::Math::NumericalUtils::Min( (Uint32)size - bytesRead, m_decompressedBuffer.Size() - m_localChunkOffset );

		// Read from local buffer
		if( bytesToRead > 0 )
		{
			Red::System::MemoryCopy( targetBuffer, m_decompressedBuffer.TypedData() + m_localChunkOffset, bytesToRead );
			m_localChunkOffset += bytesToRead;
			targetBuffer += bytesToRead;
			bytesRead += bytesToRead;
		}

		// If there is more to read in the next chunk, decompress it now
		if( m_localChunkOffset >= m_decompressedBuffer.Size() && bytesRead < size )
		{
			DecompressDataForReading( m_currentChunk + 1 );
		}
	}
}

Uint64 CChunkedLZ4FileReader::GetOffset() const
{
	RED_ASSERT( m_chunkHeaderData.Size() > 0 && m_currentChunk != m_chunkHeaderData.End(), TXT( "No current chunk decompressed. Something went very wrong" ) );
	return m_currentChunk != m_chunkHeaderData.End() ? ( m_currentChunk->m_uncompressedOffset + m_localChunkOffset ) : 0;
}

Uint64 CChunkedLZ4FileReader::GetSize() const
{
	return m_decompressedFileSize;
}

void CChunkedLZ4FileReader::Seek( Int64 offset )
{
	Uint32 offset32 = static_cast< Uint32 >( offset ) ;

	// Fast path - seeking in same chunk
	if( m_currentChunk != m_chunkHeaderData.End() )
	{
		if( ( offset32 >= m_currentChunk->m_uncompressedOffset ) && ( offset32 < m_currentChunk->m_uncompressedOffset + m_currentChunk->m_uncompressedSize ) )
		{
			// Internal seek
			m_localChunkOffset = static_cast< Uint32 >( offset32 ) - m_currentChunk->m_uncompressedOffset;
			return;
		}
	}

	// Slow path, search for matching chunk
	for( auto chunk = m_chunkHeaderData.Begin(); chunk != m_chunkHeaderData.End(); ++chunk )
	{
		if( ( offset32 >= chunk->m_uncompressedOffset ) && ( offset32 < chunk->m_uncompressedOffset + chunk->m_uncompressedSize ) )
		{
			// Found a match, decompress and seek internally
			DecompressDataForReading( chunk );
			m_localChunkOffset = offset32 - chunk->m_uncompressedOffset;
			return;
		}
	}

	// If we reach this point, then we can't handle the seek
	ERR_CORE( TXT( "CChunkedLZ4FileReader::Seek attempted to seek (%d) outside of usable area. Corrupt save-game data?" ), (Uint32)offset );
	RED_FATAL_ASSERT( m_chunkHeaderData.Size() == 0, "Attempted to seek outside of usable area (only compressed data should be seeked to)" );
}