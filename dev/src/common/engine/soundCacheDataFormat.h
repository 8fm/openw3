/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//----

#include "../redIO/redIOAsyncIO.h"
#include "../redIO/redIOAsyncFileHandleCache.h"
#include "../redIO/redIOAsyncReadToken.h"

//----

class CSoundCacheData
{
public:
	/// Cache token
	struct CacheToken
	{
		Uint32 m_name;			//!< Index in the string table
		Uint64 m_dataOffset;	//!< Offset to sound resource (bnk or wem)
		Uint64 m_dataSize;		//!< Size of sound resource (bnk or wem)

		CacheToken()
			: m_name()
			, m_dataOffset( 0 )
			, m_dataSize( 0 )
		{
			/* Intentionally Empty */
		}
	};

	//For caches built before 64bit support
	struct OldCacheToken
	{
		Uint32 m_name;		
		Uint32 m_dataOffset;
		Uint32 m_dataSize;	

		OldCacheToken()
			: m_name()
			, m_dataOffset( 0 )
			, m_dataSize( 0 )
		{
			/* Intentionally Empty */
		}
	};

	/// Raw header
	struct RawHeader
	{
		Uint32						m_magic;							//!< File magic number
		Uint32						m_version;							//!< File version
		Red::System::DateTime		m_timeStamp;						//!< File generation time stamp

		RED_INLINE RawHeader()
			: m_magic( 0 )
			, m_version( 0 )
		{}
	};

	//Index header before 64bit support was added
	struct OldIndexHeader
	{
		Uint32 m_tokenTableOffset;
		Uint32 m_tokenTableCount;
		Uint32 m_stringTableOffset;
		Uint32 m_stringTableCount;
		Uint32 m_readBufferSize;
		Uint64 m_dataCRC;
	};

	/// Cache header
	struct IndexHeader
	{
		Uint64						m_tokenTableOffset;			//!< Offset to the token table
		Uint32						m_tokenTableCount;			//!< Number of tokens
		Uint64						m_stringTableOffset;		//!< Offset to the string table
		Uint32						m_stringTableCount;			//!< Number of strings
		Uint64						m_readBufferSize;			//!< Maximum size of compressed data (on disk, IO buffer)
		Uint64						m_dataCRC;					//!< CRC of the data buffers

		RED_INLINE IndexHeader()
			: m_tokenTableCount( 0 )
			, m_tokenTableOffset( 0 )
			, m_readBufferSize( 0 )
			, m_dataCRC( 0 )
		{}
	};

	TDynArray<CacheToken>	m_tokens;
	TDynArray<OldCacheToken> m_oldTokens;
	TDynArray<AnsiChar>		m_strings;
	IndexHeader				m_header;
	OldIndexHeader			m_oldHeader; //used to load in caches made before 64bit was supported
	Uint32					m_version;

	static const Uint32 HEADER_MAGIC;
	static const Uint32 HEADER_VERSION;

	static const Uint32 BUFFER_SIZE_ALIGNMENT;

public:
	CSoundCacheData();
	~CSoundCacheData();

	// Read data from file stream
	Bool Load( IFile& file );

	// Save data to file stream, NOTE: the token table is always appended at the end
	void Save( IFile& file, const Uint64 endOfFilePosition );

	// Save the raw data header
	static void WriteHeader( IFile& file, const Red::System::DateTime& outDateTimeOverride = Red::System::DateTime() );

	// Validate header
	static Bool ValidateHeader( IFile& file, Red::System::DateTime& outDateTime, Uint32& outVersion );

	// Validate header
	static Bool ValidateHeader( const RawHeader& header, Uint32& outVersion );
};

//----

/// Builder for data format of sound cache
class CSoundCacheDataBuilder
{
public:
	CSoundCacheDataBuilder( CSoundCacheData& data );

	// Add string
	const Uint32 AddString( const String& str );

	// Add string
	const Uint32 AddString( const StringAnsi& str );

	// Add token 
	void AddToken( const CSoundCacheData::CacheToken& tokenData );

private:
	CSoundCacheData*		m_data;
};

//----

/// Asynchronous loader for sound cache data
class CSoundCacheDataAsyncLoader
{
public:
	CSoundCacheDataAsyncLoader( CSoundCacheData& data );
	~CSoundCacheDataAsyncLoader();

	// load data asynchronously from specified file, set the ready flag when done
	// if it fails the ready flag is NOT set
	void StartLoading( Red::IO::CAsyncFileHandleCache::TFileHandle file, volatile Bool* readyFlag );

private:
	// internal state
	Red::IO::CAsyncFileHandleCache::TFileHandle	m_file;				//!< File being read
	Red::IO::SAsyncReadToken					m_readToken;		//!< Reading token
	CSoundCacheData*							m_data;				//!< Loaded data
	volatile Bool*								m_ready;			//!< Ready flag to set
	Bool										m_error;			//!< Internal error

	// stats - start time
	CTimeCounter						m_timer;

	// raw header - for validation
	CSoundCacheData::RawHeader			m_rawHeader;

	// async loading integration
	static Red::IO::ECallbackRequest OnHeaderLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
	static Red::IO::ECallbackRequest OnDataHeaderLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
	static Red::IO::ECallbackRequest OnStringsLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
	static Red::IO::ECallbackRequest OnTokensLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
};

//----
