/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "soundCacheDataFormat.h"
#include "..\core\ioTags.h"

const Uint32 CSoundCacheData::HEADER_MAGIC = 'W3SC'; // Wither3 Sound Cache
const Uint32 CSoundCacheData::HEADER_VERSION = 2;
const Uint32 CSoundCacheData::BUFFER_SIZE_ALIGNMENT = 4096;



CSoundCacheData::CSoundCacheData()
	: m_version( HEADER_VERSION )
{

}

CSoundCacheData::~CSoundCacheData()
{

}

Bool CSoundCacheData::Load(IFile& file)
{
	// load the index header
	IndexHeader header;

	//We need to handle caches that were made before 64bit support was added
	if( m_version == 1 )
	{
		OldIndexHeader oldHeader;
		file.Serialize( &oldHeader, sizeof(oldHeader) );
		header.m_tokenTableOffset = oldHeader.m_tokenTableOffset;
		header.m_tokenTableCount = oldHeader.m_tokenTableCount;
		header.m_stringTableOffset = oldHeader.m_stringTableOffset;
		header.m_stringTableCount = oldHeader.m_stringTableCount;
		header.m_readBufferSize = oldHeader.m_readBufferSize;
		header.m_dataCRC = oldHeader.m_dataCRC;
	}
	else
	{
		file.Serialize( &header, sizeof(header) );
	}

	// validate the header integrity
	const Uint64 fileSize = file.GetSize();
	if ( header.m_stringTableOffset + header.m_stringTableCount > fileSize )
	{
		ERR_ENGINE( TXT("Sound cache index file is corrupted: string table spans end of the file") );
		return false;
	}


	Uint64 tokenSize = sizeof(CacheToken);

	//Again handling caches built before 64bit support was added
	if(m_version == 1)
	{
		tokenSize = sizeof(OldCacheToken);
	}

	// validate token table integrity
	if ( header.m_tokenTableOffset + header.m_tokenTableCount * tokenSize > fileSize )
	{
		ERR_ENGINE( TXT("Sound cache index file is corrupted: token table spans end of the file") );
		return false;
	}

	// load the string table
	m_strings.Resize( header.m_stringTableCount );
	file.Seek( header.m_stringTableOffset );
	file.Serialize( m_strings.Data(), header.m_stringTableCount );

	m_tokens.Resize( header.m_tokenTableCount );
	file.Seek( header.m_tokenTableOffset );

	// calculate and validate CRC
	Uint64 crc = Red::CalculateHash64( m_strings.Data(), m_strings.DataSize(), RED_FNV_OFFSET_BASIS64 );

	//Deal with files that were created before soundcache supported 64bit
	if( m_version == 1 )
	{
		TDynArray<OldCacheToken> oldTokens;
		oldTokens.Resize( header.m_tokenTableCount );
		file.Serialize( oldTokens.Data(), header.m_tokenTableCount * sizeof( OldCacheToken ) );
		crc = Red::CalculateHash64( oldTokens.Data(), oldTokens.DataSize(), crc );

		for(auto token : oldTokens)
		{
			CacheToken newToken;
			newToken.m_name = token.m_name;
			newToken.m_dataSize = token.m_dataSize;
			newToken.m_dataOffset = token.m_dataOffset;

			m_tokens.PushBack(newToken);
		}

	}
	else
	{
		// load the token table
		file.Serialize( m_tokens.Data(), header.m_tokenTableCount * sizeof( CacheToken ) );
		crc = Red::CalculateHash64( m_tokens.Data(), m_tokens.DataSize(), crc );
	}



	// validate CRC from the runtime data and header
	if ( crc != header.m_dataCRC )
	{
		ERR_ENGINE( TXT("Sound cache index file is corrupted: CRC error") );
		return false;
	}

	// done
	m_header = header;
	return true;	
}

void CSoundCacheData::Save(IFile& file, const Uint64 endOfFilePosition)
{
	// write initial header
	IndexHeader header;
	file.Seek( sizeof(RawHeader) );
	file.Serialize( &header, sizeof(header) );

	// move to the end of file and store the tables there
	file.Seek( endOfFilePosition );

	// save the string table
	const Uint64 stringTableOffset =  file.GetOffset();
	file.Serialize( (void*) m_strings.Data(), m_strings.DataSize() );

	// save the token table
	const Uint64 tokenTableOffset =  file.GetOffset();
	file.Serialize( (void*) m_tokens.Data(), m_tokens.DataSize() );

	// compute the maximum compressed and uncompressed data size
	header.m_readBufferSize = 0;
	for ( const CacheToken& token : m_tokens )
	{
		RED_ASSERT( token.m_dataOffset > 0, TXT("Corrupted data is being saved") );
		RED_ASSERT( token.m_dataSize > 0, TXT("Corrupted data is being saved") );

		header.m_readBufferSize = AlignOffset( Red::Math::NumericalUtils::Max< Uint64 >( header.m_readBufferSize, token.m_dataSize ), BUFFER_SIZE_ALIGNMENT );
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

void CSoundCacheData::WriteHeader(IFile& file, const Red::System::DateTime& outDateTimeOverride /*= Red::System::DateTime() */)
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

Bool CSoundCacheData::ValidateHeader(IFile& file, Red::System::DateTime& outDateTime, Uint32& outVersion )
{
	// load header from file
	RawHeader header;
	Red::MemoryZero( &header, sizeof(header) );
	file.Serialize( &header, sizeof(header) );

	// validate
	if ( !ValidateHeader( header, outVersion ) )
		return false;

	outVersion = header.m_version;

	// header is valid
	outDateTime = header.m_timeStamp;
	return true;
}

Bool CSoundCacheData::ValidateHeader(const RawHeader& header, Uint32& outVersion )
{
	// verify the magic
	if ( header.m_magic != HEADER_MAGIC )
		return false;

	outVersion = header.m_version;

	// header is valid
	return true;
}

//----

CSoundCacheDataBuilder::CSoundCacheDataBuilder( CSoundCacheData& data )
	: m_data( &data )
{
}

const Uint32 CSoundCacheDataBuilder::AddString( const String& str )
{
	return AddString( UNICODE_TO_ANSI( str.AsChar() ) );
}

const Uint32 CSoundCacheDataBuilder::AddString( const StringAnsi& str )
{
	const Uint32 length = str.GetLength();
	const Uint32 offset = m_data->m_strings.Size();
	m_data->m_strings.Grow( length + 1 );
	Red::MemoryCopy( &m_data->m_strings[ offset ], str.AsChar(), sizeof(AnsiChar) * (length+1) );
	return offset;
}

void CSoundCacheDataBuilder::AddToken( const CSoundCacheData::CacheToken& tokenData )
{
	m_data->m_tokens.PushBack( tokenData );
}

//----

CSoundCacheDataAsyncLoader::CSoundCacheDataAsyncLoader( CSoundCacheData& data )
	: m_data( &data )
	, m_ready( nullptr )
	, m_error( false )
{

}

CSoundCacheDataAsyncLoader::~CSoundCacheDataAsyncLoader()
{

}

void CSoundCacheDataAsyncLoader::StartLoading( Red::IO::CAsyncFileHandleCache::TFileHandle file, volatile Bool* readyFlag )
{
	// setup state
	Red::MemoryZero( &m_rawHeader, sizeof(m_rawHeader) );
	m_ready = readyFlag;
	m_error = false;
	m_file = file;
	m_timer.ResetTimer();

	// setup first read
	m_readToken.m_userData = this;
	m_readToken.m_numberOfBytesToRead = sizeof( CSoundCacheData::RawHeader );
	m_readToken.m_offset = 0;
	m_readToken.m_callback = &OnHeaderLoaded;
	m_readToken.m_buffer = &m_rawHeader;
	Red::IO::GAsyncIO.BeginRead( file, m_readToken, Red::IO::eAsyncPriority_Critical, eIOTag_SoundImmediate );
}

Red::IO::ECallbackRequest CSoundCacheDataAsyncLoader::OnHeaderLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CSoundCacheDataAsyncLoader* loader = static_cast< CSoundCacheDataAsyncLoader* >( asyncReadToken.m_userData );

	// data read
	if ( asyncResult == Red::IO::eAsyncResult_Success )
	{
		RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

		Uint32 fileVersion = 0;

		// validate header
		if ( CSoundCacheData::ValidateHeader( loader->m_rawHeader, fileVersion ) )
		{
			// read normal header
			loader->m_data->m_version = fileVersion;
			asyncReadToken.m_buffer = &loader->m_data->m_header;

			Uint32 headerSize = sizeof( CSoundCacheData::IndexHeader );
			if(fileVersion == 1)
			{
				headerSize = sizeof( CSoundCacheData::OldIndexHeader );
				asyncReadToken.m_buffer = &loader->m_data->m_oldHeader;
			}
			
			asyncReadToken.m_offset = sizeof( CSoundCacheData::RawHeader );
			//Handle cache files created before 64bit support
			
			
			asyncReadToken.m_numberOfBytesToRead = headerSize;
			asyncReadToken.m_callback = &OnDataHeaderLoaded;
			return Red::IO::eCallbackRequest_More;
		}
		else
		{
			ERR_ENGINE( TXT("Sound cache AsyncIO: header validation failed") );
		}
	}

	// something went wrong
	loader->m_error = true;
	ERR_ENGINE( TXT("Sound cache AsyncIO: header loading failed") );
	return Red::IO::eCallbackRequest_Finish;
}

Red::IO::ECallbackRequest CSoundCacheDataAsyncLoader::OnDataHeaderLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CSoundCacheDataAsyncLoader* loader = static_cast< CSoundCacheDataAsyncLoader* >( asyncReadToken.m_userData );

	if ( asyncResult == Red::IO::eAsyncResult_Success )
	{
		//Support for non-64 bit caches
		if(loader->m_data->m_version == 1)
		{
			loader->m_data->m_header.m_dataCRC = loader->m_data->m_oldHeader.m_dataCRC;
			loader->m_data->m_header.m_readBufferSize = loader->m_data->m_oldHeader.m_readBufferSize;
			loader->m_data->m_header.m_stringTableCount = loader->m_data->m_oldHeader.m_stringTableCount;
			loader->m_data->m_header.m_stringTableOffset = loader->m_data->m_oldHeader.m_stringTableOffset;
			loader->m_data->m_header.m_tokenTableCount = loader->m_data->m_oldHeader.m_tokenTableCount;
			loader->m_data->m_header.m_tokenTableOffset = loader->m_data->m_oldHeader.m_tokenTableOffset;

		}

		RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

		// allocate strings
		loader->m_data->m_strings.Resize( loader->m_data->m_header.m_stringTableCount );

		// start reading strings
		asyncReadToken.m_buffer = loader->m_data->m_strings.Data();
		asyncReadToken.m_numberOfBytesToRead = (Uint32) loader->m_data->m_strings.DataSize();
		asyncReadToken.m_offset = loader->m_data->m_header.m_stringTableOffset;
		asyncReadToken.m_callback = &OnStringsLoaded;
		return Red::IO::eCallbackRequest_More;
	}

	// something went wrong
	loader->m_error = true;
	ERR_ENGINE( TXT("Sound cache AsyncIO: data header loading failed") );
	return Red::IO::eCallbackRequest_Finish;
}

Red::IO::ECallbackRequest CSoundCacheDataAsyncLoader::OnStringsLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CSoundCacheDataAsyncLoader* loader = static_cast< CSoundCacheDataAsyncLoader* >( asyncReadToken.m_userData );

	if ( asyncResult == Red::IO::eAsyncResult_Success )
	{
		RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

		// allocate tokens

		if(loader->m_data->m_version == 1) //handle old cache files
		{
			loader->m_data->m_oldTokens.Resize( loader->m_data->m_header.m_tokenTableCount );
		}
		
		loader->m_data->m_tokens.Resize( loader->m_data->m_header.m_tokenTableCount );

		// start reading tokens
		if(loader->m_data->m_version == 1) //handle old cache files
		{
			asyncReadToken.m_buffer = loader->m_data->m_oldTokens.Data();
			asyncReadToken.m_numberOfBytesToRead = (Uint32) loader->m_data->m_oldTokens.DataSize();
		}
		else
		{
			asyncReadToken.m_buffer = loader->m_data->m_tokens.Data();
			asyncReadToken.m_numberOfBytesToRead = (Uint32) loader->m_data->m_tokens.DataSize();
		}

		asyncReadToken.m_offset = loader->m_data->m_header.m_tokenTableOffset;
		asyncReadToken.m_callback = &OnTokensLoaded;
		return Red::IO::eCallbackRequest_More;
	}

	// something went wrong
	loader->m_error = true;
	ERR_ENGINE( TXT("Sound cache AsyncIO: string loading failed") );
	return Red::IO::eCallbackRequest_Finish;
}

Red::IO::ECallbackRequest CSoundCacheDataAsyncLoader::OnTokensLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CSoundCacheDataAsyncLoader* loader = static_cast< CSoundCacheDataAsyncLoader* >( asyncReadToken.m_userData );

	if ( asyncResult == Red::IO::eAsyncResult_Success )
	{
		RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

		// calculate CRC of the loaded data and validate stuff
		Uint64 crc = Red::CalculateHash64( loader->m_data->m_strings.Data(), loader->m_data->m_strings.DataSize(), RED_FNV_OFFSET_BASIS64 );

		if(loader->m_data->m_version == 1) //handle old cache files
		{
			Uint32 i = 0;
			for(auto token : loader->m_data->m_oldTokens)
			{
				CSoundCacheData::CacheToken newToken;
				newToken.m_name = token.m_name;
				newToken.m_dataSize = token.m_dataSize;
				newToken.m_dataOffset = token.m_dataOffset;

				loader->m_data->m_tokens[i++] = newToken;
			}

			crc = Red::CalculateHash64( loader->m_data->m_oldTokens.Data(), loader->m_data->m_oldTokens.DataSize(), crc );
		}
		else
		{
			crc = Red::CalculateHash64( loader->m_data->m_tokens.Data(), loader->m_data->m_tokens.DataSize(), crc );
		}


		// CRC is the same - use the data
		if ( crc == loader->m_data->m_header.m_dataCRC )
		{
			// log success information
			LOG_ENGINE( TXT("Sound cache AsyncIO: finished after %1.2fms, loaded %d tokens"),
				loader->m_timer.GetTimePeriodMS(), loader->m_data->m_tokens.Size() );

			// set ready flag
			*loader->m_ready = true;

			// done
			return Red::IO::eCallbackRequest_Finish;
		}
		else
		{
			ERR_ENGINE( TXT("Sound cache AsyncIO: CRC failed") );
			return Red::IO::eCallbackRequest_Finish;
		}
	}

	// something went wrong
	loader->m_error = true;
	ERR_ENGINE( TXT("Sound cache AsyncIO: token loading failed") );
	return Red::IO::eCallbackRequest_Finish;
}

