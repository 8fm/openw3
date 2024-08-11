/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//----

#include "../redIO/redIOAsyncIO.h"
#include "../redIO/redIOAsyncFileHandleCache.h"
#include "../redIO/redIOAsyncReadToken.h"

//----

class CCollisionCachePreloadedData;

//----

/// Type of collision object
enum EColllisionTokenType : Uint8
{
	RTT_Unknown,
	RTT_Terrain,
	RTT_Mesh,
	RTT_ApexCloth,
	RTT_ApexDestruction,
	RTT_Destruction
};

/// Data format for collision cache index file
class CCollisionCacheData
{
public:

	/// Cache token
	struct CacheToken
	{
		Uint32									m_name;					//!< Index in the string table
		Uint64									m_nameHash;				//!< Name hash
		Uint32									m_dataOffset;			//!< Offset to compiled mesh data
		Uint32									m_dataSizeOnDisk;		//!< Size of the compressed data (in cache)
		Uint32									m_dataSizeInMemory;		//!< Size of the decompressed data (in memory)
		Uint64									m_diskCRC;				//!< CRC of compressed data on disk
		Red::System::DateTime					m_dateTime;				//!< Time stamp of the mesh
		Box2									m_boundingArea;
		EColllisionTokenType					m_collisionType;

		RED_INLINE CacheToken()
			: m_name()
			, m_nameHash( 0 )
			, m_dataOffset( 0 )
			, m_dataSizeOnDisk( 0 )
			, m_dataSizeInMemory( 0 )
			, m_diskCRC( 0 )
			, m_boundingArea( Box2::ZERO )
			, m_collisionType( RTT_Unknown )
		{
		};
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

	/// Cache header
	struct IndexHeader
	{
		Uint32						m_tokenTableOffset;			//!< Offset to the token table
		Uint32						m_tokenTableCount;			//!< Number of tokens
		Uint32						m_stringTableOffset;		//!< Offset to the string table
		Uint32						m_stringTableCount;			//!< Number of strings
		Uint32						m_readBufferSize;			//!< Maximum size of compressed data (on disk, IO buffer)
		Uint32						m_loadBufferSize;			//!< Maximum size of uncompressed data (serialization buffer)
		Uint64						m_dataCRC;					//!< CRC of the data buffers

		RED_INLINE IndexHeader()
			: m_tokenTableCount( 0 )
			, m_tokenTableOffset( 0 )
			, m_readBufferSize( 0 )
			, m_loadBufferSize( 0 )
			, m_dataCRC( 0 )
		{}
	};

	typedef TDynArray< CacheToken, MC_CompiledCollision, MemoryPool_Physics > TTokenTable;
	typedef TDynArray< AnsiChar, MC_CompiledCollision, MemoryPool_Physics > TStringsTable;

	TTokenTable						m_tokens;
	TStringsTable					m_strings;
	IndexHeader						m_header;

	static const Uint32 HEADER_MAGIC;
	static const Uint32 HEADER_VERSION;

	static const Uint32 BUFFER_SIZE_ALIGNMENT;

public:
	CCollisionCacheData();
	~CCollisionCacheData();

	// Read data from file stream
	Bool Load( IFile& file );

	// Save data to file stream, NOTE: the token table is always appended at the end
	void Save( IFile& file, const Uint32 endOfFilePosition );

	// Save the raw data header
	static void WriteHeader( IFile& file, const Red::System::DateTime& outDateTimeOverride = Red::System::DateTime() );

	// Validate header
	static Bool ValidateHeader( IFile& file, Red::System::DateTime& outDateTime );

	// Validate header
	static Bool ValidateHeader( const RawHeader& header );
};

//----

/// Builder for data format of collision cache
class CCollisionCacheDataBuilder
{
public:
	CCollisionCacheDataBuilder( CCollisionCacheData& data );

	// Add string
	const Uint32 AddString( const String& str );

	// Add string
	const Uint32 AddString( const StringAnsi& str );

	// Add token 
	void AddToken( const CCollisionCacheData::CacheToken& tokenData );

private:
	CCollisionCacheData*		m_data;
};

//----

/// Asynchronous loader for collision cache data
class CCollisionCacheDataAsyncLoader
{
public:
	CCollisionCacheDataAsyncLoader( CCollisionCacheData& data, CCollisionCachePreloadedData& preloadedData );
	~CCollisionCacheDataAsyncLoader();

	// load data asynchronously from specified file, set the ready flag when done
	// if it fails the ready flag is NOT set
	void StartLoading( Red::IO::CAsyncFileHandleCache::TFileHandle file, const Uint32 preloadTokenSizeLimit, volatile Bool* readyFlag );

private:
	// internal state
	Red::IO::CAsyncFileHandleCache::TFileHandle	m_file;						//!< File being read
	Red::IO::SAsyncReadToken					m_readToken;				//!< Reading token
	CCollisionCacheData*						m_data;						//!< Loaded data
	CCollisionCachePreloadedData*				m_preloadedData;			//!< Preloaded data block
	volatile Bool*								m_ready;					//!< Ready flag to set
	Bool										m_error;					//!< Internal error

	// preloading state
	Uint32										m_preloadTokenSizeLimit;	//!< Tokens smaller than the limit are always loaded
	Uint32										m_preloadDataOffset;		//!< Offset in file of the data block to preload
	Uint32										m_preloadDataSize;			//!< Total size of the data block to preload

	// stats - start time
	CTimeCounter						m_timer;

	// raw header - for validation
	CCollisionCacheData::RawHeader		m_rawHeader;

	// async loading integration
	static Red::IO::ECallbackRequest OnHeaderLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
	static Red::IO::ECallbackRequest OnDataHeaderLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
	static Red::IO::ECallbackRequest OnStringsLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
	static Red::IO::ECallbackRequest OnTokensLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
	static Red::IO::ECallbackRequest OnLocalDataLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );

	// loading has finished
	void LoadingFinished();
};

//----
