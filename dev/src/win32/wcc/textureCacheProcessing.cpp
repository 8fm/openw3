/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "textureCacheProcessing.h"
#include "patchUtils.h"




static RED_INLINE Uint32 RoundUpToPage( Uint32 byteSize )
{
	return ( ( byteSize - 1 ) | ( TEXTURE_CACHE_PAGE_SIZE - 1 ) ) + 1;
}



CTextureCacheData::CTextureCacheData()
	: m_inputFile( nullptr )
{
	Red::System::MemoryZero( &m_header, sizeof( m_header ) );
}


CTextureCacheData::~CTextureCacheData()
{
	if ( m_inputFile )
	{
		delete m_inputFile;
		m_inputFile = nullptr;
	}
}


Bool CTextureCacheData::LoadFromFile( const String& absolutePath )
{
	// open file
	m_inputFile = GFileManager->CreateFileReader( absolutePath, FOF_AbsolutePath | FOF_Buffered );
	if ( !m_inputFile )
	{
		return false;
	}

	// If file is too small to hold the header, just exit out. This is not a full failure, just means we start with an empty cache.
	if ( m_inputFile->GetSize() < sizeof( TextureCacheHeader ) )
	{
#ifdef RED_LOGGING_ENABLED
		// If file is 0 bytes, we just created it. So of course it's empty. Just warn if the file size > 0
		if ( m_inputFile->GetSize() > 0 )
		{
			RED_LOG_WARNING( TextureCache, TXT("Texture cache file is too small. Assuming it's empty.") );
			return true;
		}
#endif
		return false;
	}

	// Go to the end of the file to read in the header.
	m_inputFile->Seek( m_inputFile->GetSize() - sizeof( TextureCacheHeader ) );

	m_inputFile->Serialize( &m_header, sizeof( m_header ) );

	// Texture cache marker
	if ( m_header.m_magicCode != TEXTURE_CACHE_HEADER )
	{
		RED_LOG_WARNING( TextureCache, TXT("Invalid Magic Code (%u)."), m_header.m_magicCode );
		return false;
	}

	if ( m_header.m_version != TEXTURE_CACHE_VERSION )
	{
		RED_LOG_WARNING( TextureCache, TXT("Wrong version (%u)."), m_header.m_version );
		return false;
	}

	// Seek to the marked position in the file
	Uint64 bytePosition = (Uint64)m_header.m_numUsedPages * TEXTURE_CACHE_PAGE_SIZE;
	m_inputFile->Seek( bytePosition );

	m_mipOffsets.Resize( m_header.m_mipOffsetTableSize );
	m_inputFile->Serialize( m_mipOffsets.Data(), m_mipOffsets.DataSize() );

	m_strings.Resize( m_header.m_stringTableSize );
	m_inputFile->Serialize( m_strings.Data(), m_strings.DataSize() );

	m_entries.Resize( m_header.m_numEntries );
	m_inputFile->Serialize( m_entries.Data(), m_entries.DataSize() );

	m_sourceFilePath = absolutePath;

	return true;
}



const TextureCacheEntry* CTextureCacheData::GetEntry( Uint32 index ) const
{
	return &m_entries[ index ];
}

String CTextureCacheData::GetEntryName( Uint32 index ) const
{
	const TextureCacheEntry& entry = m_entries[ index ];
	return ANSI_TO_UNICODE( &m_strings[ entry.m_pathStringIndex ] );
}

Uint32 CTextureCacheData::GetEntryDiskSize( Uint32 index ) const
{
	const TextureCacheEntry& entry = m_entries[ index ];
	return RoundUpToPage( entry.m_info.m_compressedSize );
}

Uint32 CTextureCacheData::GetEntryDiskSizeUnpadded( Uint32 index ) const
{
	const TextureCacheEntry& entry = m_entries[ index ];
	return entry.m_info.m_compressedSize;
}

Uint32 CTextureCacheData::GetEntryHash( Uint32 index ) const
{
	const TextureCacheEntry& entry = m_entries[ index ];
	return entry.m_hash;
}


IFile* CTextureCacheData::ResumeEntryData( Uint32 index )
{
	if ( m_inputFile == nullptr )
	{
		RED_HALT( "Cannot resume texture cache entry data, no input file" );
		return false;
	}

	const TextureCacheEntry& entry = m_entries[ index ];

	return new PatchUtils::COffsetFileReader( m_inputFile, entry.m_info.m_pageOffset * TEXTURE_CACHE_PAGE_SIZE, entry.m_info.m_compressedSize );
}


//////////////////////////////////////////////////////////////////////////


CTextureCacheDataSaver::CTextureCacheDataSaver( const String& absolutePath )
{
	m_outputFile = GFileManager->CreateFileWriter( absolutePath, FOF_Buffered | FOF_AbsolutePath );
	if ( !m_outputFile )
	{
		ERR_WCC( TXT("Failed to create output file '%ls'"), absolutePath.AsChar() );
	}

	// preallocate read buffer
	m_readBuffer.Resize( 1 << 20 );

	// Initialize header
	// Constant values, just initialize once.
	m_header.m_magicCode			= TEXTURE_CACHE_HEADER;
	m_header.m_version				= TEXTURE_CACHE_VERSION;
	// Accumulated while saving out entries.
	m_header.m_numUsedPages			= 0;
	// Filled in when flushing the output file.
	m_header.m_crc					= 0;
	m_header.m_numEntries			= 0;
	m_header.m_stringTableSize		= 0;
	m_header.m_mipOffsetTableSize	= 0;
}

CTextureCacheDataSaver::~CTextureCacheDataSaver()
{
	if ( m_outputFile )
	{
		// Write entry tables
		m_outputFile->Serialize( m_mipOffsets.Data(), m_mipOffsets.DataSize() );
		m_outputFile->Serialize( m_strings.Data(), m_strings.DataSize() );
		m_outputFile->Serialize( m_entries.Data(), m_entries.DataSize() );

		// Calculate CRC from strings and entries
		Uint64 crc = Red::CalculateHash64( m_mipOffsets.Data(), m_mipOffsets.DataSize(), RED_FNV_OFFSET_BASIS64 );
		crc = Red::CalculateHash64( m_strings.Data(), m_strings.DataSize(), crc );
		crc = Red::CalculateHash64( m_entries.Data(), m_entries.DataSize(), crc );

		// Write footer
		m_header.m_crc						= crc;
		m_header.m_numEntries				= m_entries.Size();
		m_header.m_stringTableSize			= m_strings.Size();
		m_header.m_mipOffsetTableSize		= m_mipOffsets.Size();
		m_outputFile->Serialize( &m_header, sizeof( TextureCacheHeader ) );


		delete m_outputFile;
		m_outputFile = nullptr;
	}
}

Bool CTextureCacheDataSaver::SaveEntry( Uint32 sourceIndex, const CTextureCacheData& sourceData )
{
	if ( m_outputFile == nullptr )
	{
		RED_HALT( "Cannot save texture cache entry, output file was not created." );
		return false;
	}
	if ( sourceData.m_inputFile == nullptr )
	{
		RED_HALT( "Cannot save texture cache entry, source data has no input file." );
		return false;
	}
	if ( sourceIndex >= sourceData.m_entries.Size() )
	{
		RED_HALT( "Cannot save texture cache entry, index out of range. %u >= %u", sourceIndex, sourceData.m_entries.Size() );
		return false;
	}


	const TextureCacheEntry& entry = sourceData.m_entries[ sourceIndex ];

	if ( !m_hashToIndex.Insert( entry.m_hash, m_entries.Size() ) )
	{
		Uint32 existingIndex = m_hashToIndex[ entry.m_hash ];
		const TextureCacheEntry& existing = m_entries[ existingIndex ];

		String existingPath = ANSI_TO_UNICODE( &m_strings[ existing.m_pathStringIndex ] );

		ERR_WCC( TXT("Adding entry with duplicate hash. hash: %08x; existing: %ls; new: %ls"), entry.m_hash, existingPath.AsChar(), sourceData.GetEntryName( sourceIndex ).AsChar() );
		return false;
	}


	Uint32 readPage, readNum;

	Uint32 stringStart;
	const Uint32 newStringStart = m_strings.Size();

	Uint32 mipOffsetStart, mipOffsetNum;
	const Uint32 newMipOffsetStart = m_mipOffsets.Size();

	TextureCacheEntry newEntry = entry;
	readPage = entry.m_info.m_pageOffset;
	readNum = RoundUpToPage( newEntry.m_info.m_compressedSize );
	stringStart = entry.m_pathStringIndex;
	newEntry.m_pathStringIndex = newStringStart;

	mipOffsetStart = entry.m_info.m_mipOffsetIndex;
	mipOffsetNum = entry.m_info.m_numMipOffsets;
	newEntry.m_info.m_mipOffsetIndex = newMipOffsetStart;

	newEntry.m_info.m_pageOffset = m_header.m_numUsedPages;
	m_entries.PushBack( newEntry );


	// Add new mip offsets
	if ( mipOffsetNum > 0 )
	{
		m_mipOffsets.Grow( mipOffsetNum );
		Red::System::MemoryCopy( &m_mipOffsets[ newMipOffsetStart ], &sourceData.m_mipOffsets[ mipOffsetStart ], sizeof( Uint32 ) * mipOffsetNum );
	}

	// Add to new string table
	size_t stringLength = Red::System::StringLength( &sourceData.m_strings[ stringStart ] ) + 1;
	m_strings.Grow( stringLength );
	Red::System::MemoryCopy( &m_strings[ newStringStart ], &sourceData.m_strings[ stringStart ], sizeof(AnsiChar) * stringLength );

	// Copy data
	Uint32 totalSize = readNum;
	if ( totalSize > m_readBuffer.Size() )
	{
		m_readBuffer.Resize( totalSize );
	}

	sourceData.m_inputFile->Seek( (Uint64)readPage * TEXTURE_CACHE_PAGE_SIZE );
	sourceData.m_inputFile->Serialize( m_readBuffer.Data(), totalSize );

	m_outputFile->Serialize( m_readBuffer.Data(), totalSize );

	m_header.m_numUsedPages += readNum / TEXTURE_CACHE_PAGE_SIZE;
	++m_header.m_numEntries;

	return true;
}
