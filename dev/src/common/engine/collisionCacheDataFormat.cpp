/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "collisionCacheDataFormat.h"
#include "collisionCachePreloadedData.h"
#include "..\core\ioTags.h"
const Uint32 CCollisionCacheData::HEADER_MAGIC = 'W3CC'; // Wither3 Collision Cache
const Uint32 CCollisionCacheData::HEADER_VERSION = 9;

const Uint32 CCollisionCacheData::BUFFER_SIZE_ALIGNMENT = 4096;

CCollisionCacheData::CCollisionCacheData()
{
}

CCollisionCacheData::~CCollisionCacheData()
{
}

Bool CCollisionCacheData::Load( IFile& file )
{
	// load the index header
	IndexHeader header;
	file.Serialize( &header, sizeof(header) );

	// validate the header integrity
	const Uint32 fileSize = (Uint32) file.GetSize();
	if ( header.m_stringTableOffset + header.m_stringTableCount > fileSize )
	{
		ERR_ENGINE( TXT("Collision cache index file is corrupted: string table spans end of the file") );
		return false;
	}

	// validate token table integrity
	if ( header.m_tokenTableOffset + header.m_tokenTableCount * sizeof( CacheToken ) > fileSize )
	{
		ERR_ENGINE( TXT("Collision cache index file is corrupted: token table spans end of the file") );
		return false;
	}

	// load the string table
	m_strings.Resize( header.m_stringTableCount );
	file.Seek( header.m_stringTableOffset );
	file.Serialize( m_strings.Data(), header.m_stringTableCount );

	// load the token table
	m_tokens.Resize( header.m_tokenTableCount );
	file.Seek( header.m_tokenTableOffset );
	file.Serialize( m_tokens.Data(), header.m_tokenTableCount * sizeof( CacheToken ) );	

	// calculate and validate CRC
	Uint64 crc = Red::CalculateHash64( m_strings.Data(), m_strings.DataSize(), RED_FNV_OFFSET_BASIS64 );
	crc = Red::CalculateHash64( m_tokens.Data(), m_tokens.DataSize(), crc );

	// validate CRC from the runtime data and header
	if ( crc != header.m_dataCRC )
	{
		ERR_ENGINE( TXT("Collision cache index file is corrupted: CRC error") );
		return false;
	}

	// done
	m_header = header;
	return true;	
}

void CCollisionCacheData::Save( IFile& file, const Uint32 endOfFilePosition )
{
	// write initial header
	IndexHeader header;
	file.Seek( sizeof(RawHeader) );
	file.Serialize( &header, sizeof(header) );

	// move to the end of file and store the tables there
	file.Seek( endOfFilePosition );

	// save the string table
	const Uint32 stringTableOffset = (Uint32) file.GetOffset();
	file.Serialize( (void*) m_strings.Data(), m_strings.DataSize() );

	// save the token table
	const Uint32 tokenTableOffset = (Uint32) file.GetOffset();
	file.Serialize( (void*) m_tokens.Data(), m_tokens.DataSize() );

	// compute the maximum compressed and uncompressed data size
	header.m_loadBufferSize = 0;
	header.m_readBufferSize = 0;
	for ( const CacheToken& token : m_tokens )
	{
		RED_ASSERT( token.m_dataOffset > 0, TXT("Corrupted data is being saved") );
		RED_ASSERT( token.m_dataSizeInMemory > 0, TXT("Corrupted data is being saved") );
		RED_ASSERT( token.m_dataSizeOnDisk > 0, TXT("Corrupted data is being saved") );

		header.m_readBufferSize = (Uint32) AlignOffset( Red::Math::NumericalUtils::Max< Uint32 >( header.m_readBufferSize, token.m_dataSizeOnDisk ), BUFFER_SIZE_ALIGNMENT );
		header.m_loadBufferSize = (Uint32) AlignOffset( Red::Math::NumericalUtils::Max< Uint32 >( header.m_loadBufferSize, token.m_dataSizeInMemory ), BUFFER_SIZE_ALIGNMENT );		
	}

	// update header
	header.m_stringTableOffset = stringTableOffset;
	header.m_stringTableCount = m_strings.Size();
	header.m_tokenTableOffset = tokenTableOffset;
	header.m_tokenTableCount = m_tokens.Size();

	// calculate and validate CRC
	Uint64 crc = Red::CalculateHash64( m_strings.Data(), m_strings.DataSize(), RED_FNV_OFFSET_BASIS64 );
	header.m_dataCRC = Red::CalculateHash64( m_tokens.Data(), m_tokens.DataSize(), crc );

	// overwrite new header
	file.Seek( sizeof(RawHeader) );
	file.Serialize( &header, sizeof(header) );

	// save header
	m_header = header;
}

void CCollisionCacheData::WriteHeader( IFile& file, const Red::System::DateTime& outDateTimeOverride/* = Red::System::DateTime()*/ )
{
	// setup
	RawHeader header;
	header.m_magic = HEADER_MAGIC;
	header.m_version = HEADER_VERSION;

	// setup time
	if ( outDateTimeOverride.IsValid() )
	{
		header.m_timeStamp = outDateTimeOverride;
	}
	else
	{
		// use current time
		Red::System::Clock::GetInstance().GetUTCTime( header.m_timeStamp );
	}

	// write to file
	file.Serialize( &header, sizeof(header) );
}

Bool CCollisionCacheData::ValidateHeader( const RawHeader& header )
{
	// verify the magic
	if ( header.m_magic != HEADER_MAGIC )
		return false;

	// verify the version
	if ( header.m_version != HEADER_VERSION )
		return false;

	// header is valid
	return true;
}

Bool CCollisionCacheData::ValidateHeader( IFile& file, Red::System::DateTime& outDateTime )
{
	// load header from file
	RawHeader header;
	Red::MemoryZero( &header, sizeof(header) );
	file.Serialize( &header, sizeof(header) );

	// validate
	if ( !ValidateHeader( header ) )
		return false;

	// header is valid
	outDateTime = header.m_timeStamp;
	return true;
}

//----

CCollisionCacheDataBuilder::CCollisionCacheDataBuilder( CCollisionCacheData& data )
	: m_data( &data )
{
}

const Uint32 CCollisionCacheDataBuilder::AddString( const String& str )
{
	return AddString( UNICODE_TO_ANSI( str.AsChar() ) );
}

const Uint32 CCollisionCacheDataBuilder::AddString( const StringAnsi& str )
{
	const Uint32 length = str.GetLength();
	const Uint32 offset = m_data->m_strings.Size();
	m_data->m_strings.Grow( length + 1 );
	Red::MemoryCopy( &m_data->m_strings[ offset ], str.AsChar(), sizeof(AnsiChar) * (length+1) );
	return offset;
}

void CCollisionCacheDataBuilder::AddToken( const CCollisionCacheData::CacheToken& tokenData )
{
	m_data->m_tokens.PushBack( tokenData );
}

//----

CCollisionCacheDataAsyncLoader::CCollisionCacheDataAsyncLoader( CCollisionCacheData& data, CCollisionCachePreloadedData& preloadedData )
	: m_data( &data )
	, m_preloadedData( &preloadedData )
	, m_ready( nullptr )
	, m_error( false )
	, m_preloadTokenSizeLimit( 0 )
	, m_preloadDataOffset( 0 )
	, m_preloadDataSize( 0 )
{
}

CCollisionCacheDataAsyncLoader::~CCollisionCacheDataAsyncLoader()
{
}

void CCollisionCacheDataAsyncLoader::StartLoading( Red::IO::CAsyncFileHandleCache::TFileHandle file, const Uint32 preloadTokenSizeLimit, volatile Bool* readyFlag )
{
	// setup state
	Red::MemoryZero( &m_rawHeader, sizeof(m_rawHeader) );
	m_ready = readyFlag;
	m_error = false;
	m_file = file;
	m_timer.ResetTimer();

	// setup preload state
	m_preloadTokenSizeLimit = preloadTokenSizeLimit;
	m_preloadDataOffset = 0;
	m_preloadDataSize = 0;

	// setup first read
	m_readToken.m_userData = this;
	m_readToken.m_numberOfBytesToRead = sizeof( CCollisionCacheData::RawHeader );
	m_readToken.m_offset = 0;
	m_readToken.m_callback = &OnHeaderLoaded;
	m_readToken.m_buffer = &m_rawHeader;
	Red::IO::GAsyncIO.BeginRead( file, m_readToken, Red::IO::eAsyncPriority_High, eIOTag_CollisionNormal );
}

Red::IO::ECallbackRequest CCollisionCacheDataAsyncLoader::OnHeaderLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CCollisionCacheDataAsyncLoader* loader = static_cast< CCollisionCacheDataAsyncLoader* >( asyncReadToken.m_userData );

	// data read
	if ( asyncResult == Red::IO::eAsyncResult_Success )
	{
		RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

		// validate header
		if ( CCollisionCacheData::ValidateHeader( loader->m_rawHeader ) )
		{
			// read normal header
			asyncReadToken.m_buffer = &loader->m_data->m_header;
			asyncReadToken.m_offset = sizeof( CCollisionCacheData::RawHeader );
			asyncReadToken.m_numberOfBytesToRead = sizeof( CCollisionCacheData::IndexHeader );
			asyncReadToken.m_callback = &OnDataHeaderLoaded;
			return Red::IO::eCallbackRequest_More;
		}
		else
		{
			ERR_ENGINE( TXT("Collsion cache AsyncIO: header validation failed") );
		}
	}

	// something went wrong
	loader->m_error = true;
	ERR_ENGINE( TXT("Collision cache AsyncIO: header loading failed") );
	return Red::IO::eCallbackRequest_Finish;
}

Red::IO::ECallbackRequest CCollisionCacheDataAsyncLoader::OnDataHeaderLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CCollisionCacheDataAsyncLoader* loader = static_cast< CCollisionCacheDataAsyncLoader* >( asyncReadToken.m_userData );

	if ( asyncResult == Red::IO::eAsyncResult_Success )
	{
		RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

		// allocate memory
		loader->m_data->m_strings.Resize( loader->m_data->m_header.m_stringTableCount );
		loader->m_data->m_tokens.Resize( loader->m_data->m_header.m_tokenTableCount );

		// start reading strings
		asyncReadToken.m_buffer = loader->m_data->m_strings.Data();
		asyncReadToken.m_numberOfBytesToRead = (Uint32) loader->m_data->m_strings.DataSize();
		asyncReadToken.m_offset = loader->m_data->m_header.m_stringTableOffset;
		asyncReadToken.m_callback = &OnStringsLoaded;
		return Red::IO::eCallbackRequest_More;
	}

	// something went wrong
	loader->m_error = true;
	ERR_ENGINE( TXT("Collision cache AsyncIO: data header loading failed") );
	return Red::IO::eCallbackRequest_Finish;
}

Red::IO::ECallbackRequest CCollisionCacheDataAsyncLoader::OnStringsLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CCollisionCacheDataAsyncLoader* loader = static_cast< CCollisionCacheDataAsyncLoader* >( asyncReadToken.m_userData );

	if ( asyncResult == Red::IO::eAsyncResult_Success )
	{
		RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

		// start reading strings
		asyncReadToken.m_buffer = loader->m_data->m_tokens.Data();
		asyncReadToken.m_numberOfBytesToRead = (Uint32) loader->m_data->m_tokens.DataSize();
		asyncReadToken.m_offset = loader->m_data->m_header.m_tokenTableOffset;
		asyncReadToken.m_callback = &OnTokensLoaded;
		return Red::IO::eCallbackRequest_More;
	}

	// something went wrong
	loader->m_error = true;
	ERR_ENGINE( TXT("Collision cache AsyncIO: string loading failed") );
	return Red::IO::eCallbackRequest_Finish;
}

Red::IO::ECallbackRequest CCollisionCacheDataAsyncLoader::OnTokensLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CCollisionCacheDataAsyncLoader* loader = static_cast< CCollisionCacheDataAsyncLoader* >( asyncReadToken.m_userData );

	if ( asyncResult == Red::IO::eAsyncResult_Success )
	{
		RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

		// preload all tokens that are smaller than the limit
		// NOTE: this takes advantage from the fact, that tokens are SORTED by size in the file 
		{
			Uint32 prefetchEndOffset = 0;

			const auto& tokens = loader->m_data->m_tokens;
			for ( const auto& token : tokens )
			{
				if ( token.m_dataSizeOnDisk > loader->m_preloadTokenSizeLimit )
					break;

				prefetchEndOffset = Red::Math::NumericalUtils::Max< Uint32 >( prefetchEndOffset, token.m_dataOffset + token.m_dataSizeOnDisk );
			}

			// something to preload ?
			if ( prefetchEndOffset > 0 )
			{
				RED_FATAL_ASSERT( prefetchEndOffset >= tokens[0].m_dataOffset, "Crap" );

				// create data buffer, the loading happens later on using AsyncIO
				loader->m_preloadDataOffset = tokens[0].m_dataOffset;
				loader->m_preloadDataSize = prefetchEndOffset - tokens[0].m_dataOffset;

				// stats
				LOG_ENGINE( TXT("Collision cache will preload %1.2fKB data"), loader->m_preloadDataSize  / 1024.0f );
			}
		}

		// load the preloaded block or finish loading if theres no such block
		if ( loader->m_preloadDataSize == 0 )
		{
			loader->LoadingFinished();
			return Red::IO::eCallbackRequest_Finish;
		}
		else
		{
			// allocate preloaded data block
			void* preloadedData = loader->m_preloadedData->Initialize( loader->m_preloadDataOffset, loader->m_preloadDataSize );
			if ( preloadedData != nullptr )
			{
				asyncReadToken.m_buffer = preloadedData;
				asyncReadToken.m_numberOfBytesToRead = loader->m_preloadDataSize;
				asyncReadToken.m_offset = loader->m_preloadDataOffset;
				asyncReadToken.m_callback = &OnLocalDataLoaded;
				return Red::IO::eCallbackRequest_More;
			}
			else
			{
				ERR_ENGINE( TXT("Collision cache AsyncIO: Failed to allocate data for the preloaded block") );

				loader->LoadingFinished();
				return Red::IO::eCallbackRequest_Finish;
			}
		}
	}

	// something went wrong
	loader->m_error = true;
	ERR_ENGINE( TXT("Collision cache AsyncIO: string loading failed") );
	return Red::IO::eCallbackRequest_Finish;
}

Red::IO::ECallbackRequest CCollisionCacheDataAsyncLoader::OnLocalDataLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CCollisionCacheDataAsyncLoader* loader = static_cast< CCollisionCacheDataAsyncLoader* >( asyncReadToken.m_userData );

	if ( asyncResult == Red::IO::eAsyncResult_Success )
	{
		RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

		loader->LoadingFinished();
		return Red::IO::eCallbackRequest_Finish;
	}

	// something went wrong
	loader->m_error = true;
	ERR_ENGINE( TXT("Collision cache AsyncIO: token loading failed") );
	return Red::IO::eCallbackRequest_Finish;
}

void CCollisionCacheDataAsyncLoader::LoadingFinished()
{
	// calculate CRC of the loaded data and validate stuff
	Uint64 crc = Red::CalculateHash64( m_data->m_strings.Data(), m_data->m_strings.DataSize(), RED_FNV_OFFSET_BASIS64 );
	crc = Red::CalculateHash64( m_data->m_tokens.Data(), m_data->m_tokens.DataSize(), crc );

	// CRC is the same - use the data
	if ( crc == m_data->m_header.m_dataCRC )
	{
		// log success information
		LOG_ENGINE( TXT("Collision cache AsyncIO: finished after %1.2fms, loaded %d tokens"),
			m_timer.GetTimePeriodMS(), m_data->m_tokens.Size() );

		// set ready flag
		*m_ready = true;
	}
	else
	{
		ERR_ENGINE( TXT("Collision cache AsyncIO: CRC failed - collision cache is UNUSABLE") );
		m_error = true;
	}
}