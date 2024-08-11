/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/redIO/redIO.h"
#include "../../common/redSystem/clock.h"
#include "../core/cooker.h"
#include "../core/hashmap.h"
#include "../core/contentListener.h"
#include "../core/lazyCacheChain.h"
#include "../core/compression/compression.h"

/////////////////////////////////////////////////////////////////////////////////////////////////

#define TEXTURE_CACHE_PAGE_SIZE			4096
#define TEXTURE_CACHE_HEADER			'TXCH'
#define TEXTURE_CACHE_VERSION			6


class CTextureCacheLoader;
class IFileDecompressionTask;

typedef Red::IO::CAsyncFileHandleCache::TFileHandle	TextureCacheFile;

namespace TextureCacheHelpers
{
	RED_INLINE String FixNonResourcePath( const String& path )
	{
		String temp1;
		return CFilePath::ConformPath( path, temp1 );
	}
}

/// Base class of TextureCache Entry that are common for both resource and nonresource onces
struct TextureCacheEntryBase
{
public:

	Uint32				m_pageOffset;				//!< Offset to first stored mipmap, always in 4 KB pages
	Uint32				m_compressedSize;			//!< Size on disk (compressed data), in bytes
	Uint32				m_uncompressedSize;			//!< Uncompressed size, in bytes
	Uint32				m_baseAlignment;			//!< Alignment required for loading in-place 
	Uint16				m_baseWidth;			//!< Width of the first mipmap
	Uint16				m_baseHeight;			//!< Height of the first mipmap
	Uint16				m_mipCount;				//!< Number of mipmaps stored
	Uint16				m_sliceCount;			//!< Number of array slices. e.g. cubemaps will hold 6
	
	Uint32				m_mipOffsetIndex;		//!< Index into mip offset table for this entry. There are m_mipCount elements in the table.
	Uint16				m_numMipOffsets;		//!< Number of stored offsets. Mip 0 is not stored, because it is always offset 0.
	
	TextureCacheEntryBase();
};

/// Entry in texture cache
struct TextureCacheEntry
{
public:
	Uint32					m_hash;						//!< Hash, calculated using m_path
	Uint32					m_pathStringIndex;

	TextureCacheEntryBase	m_info;

	Red::System::DateTime	m_timestamp;				//!< Original file's timestamp when texture was cooked
	Uint16					m_encodedFormat;			//!< Texture format, encoded with ITexture::EncodeTextureFormat
	Bool					m_isCube;					//!< Whether this texture is a cube. If true, sliceCount is multiple of 6.

	TextureCacheEntry();
};

// Actually sits at the end of the texture cache, but still call it a header :)
struct TextureCacheHeader
{
	// The magic code and version are intentionally at the end. Since the header goes at the end of the file, this allows us to add to the
	// header without affecting our ability to check version of old cache files.

	Uint64 m_crc;						// CRC of entry tables and string table, basic error checking.

	Uint32 m_numUsedPages;				// The number of pages used by the texture data. Writing additional textures can begin after here.
	Uint32 m_numEntries;
	Uint32 m_stringTableSize;
	Uint32 m_mipOffsetTableSize;

	Uint32 m_magicCode;					// Should be TEXTURE_CACHE_HEADER. Otherwise, cache might be corrupt or something.
	Uint32 m_version;					// Should be TEXTURE_CACHE_VERSION. Loading a cache that is out-of-date will cause it to be cleared.
};


//////////////////////////////////////////////////////////////////////////


// Result of loading an entry's data from texture cache
enum ETextureCacheLoadResult
{
	TCLR_Success,				// Load was successfull
	TCLR_BufferTooSmall,		// Input buffer was not big enough to hold uncompressed data, or was null
	TCLR_InvalidQuery,			// Attempted LoadData on an invalid query
	TCLR_Error					// Some other error occurred. Maybe bad data?
};

class CTextureCacheQuery
{
private:
	const TextureCacheEntry*	m_entry;
	CTextureCacheLoader*		m_cache;

public:
	CTextureCacheQuery()
		: m_entry( nullptr )
		, m_cache( nullptr )
	{}

	CTextureCacheQuery( CTextureCacheLoader* cache, const TextureCacheEntry* entry )
		: m_entry( entry )
		, m_cache( cache )
	{}

	operator Bool() const { return m_cache != nullptr && m_entry != nullptr; }


	const TextureCacheEntry& GetEntry() const
	{
		RED_ASSERT( operator Bool() );
		return *m_entry;
	}

	enum EResult
	{
		eResult_OK,			// Task was accepted
		eResult_Failed,		// Task could not be accepted at all (invalid params)
		eResult_NotReady,	// Internal decompression resources are full, task cannot be processed now
	};

	// Get byte size of compressed data for the mip chain starting with the given mip.
	Uint32 GetCompressedSizeForMip( Uint8 mip ) const;
	Uint32 GetCompressedSizeForMipRange( Uint8 start, Uint8 num ) const { return GetCompressedSizeForMip( start ) - GetCompressedSizeForMip( start + num ); }

	Uint8 GetLowestLoadableMip() const;

	String GetPath() const;
	ETextureCacheLoadResult LoadData( Uint8 startMip, void* outBuffer, Uint32 bufferSize ) const;
	EResult LoadDataAsync( Uint8 startMip, void* buffer, Uint32 bufferSize, const Uint8 ioTag, IFileDecompressionTask*& outTask, Int8 numMips = -1 ) const;
};

#ifndef NO_TEXTURECACHE_COOKER

class CTextureCacheCooker;
struct STextureBakerTask;
enum ECookingPlatform;

class ITextureBakerSource
{
public:
	virtual ~ITextureBakerSource() {}

	virtual Uint16 GetMipCount() const = 0;
	virtual Uint16 GetSliceCount() const = 0;

	virtual const void* GetMipData( Uint16 mip, Uint16 slice ) const = 0;
	virtual Uint32 GetMipDataSize( Uint16 mip, Uint16 slice ) const = 0;

	virtual Uint32 GetMipPitch( Uint16 mip, Uint16 slice ) const = 0;

	virtual Uint16 GetBaseWidth() const = 0;
	virtual Uint16 GetBaseHeight() const = 0;

	virtual GpuApi::eTextureFormat GetTextureFormat() const = 0;
	virtual GpuApi::eTextureType GetTextureType() const = 0;

	virtual Bool IsLooseFileTexture() const = 0;
};


// Simple wrapper around a data buffer.
class CTextureBakerOutput
{
private:
	Uint32 m_dataAlignment;						//!< Base alignment the data must adhere to
	TDynArray< Uint8 > m_data;					//!< Cooked data buffer
	TDynArray< Uint32 > m_mipOffsets;			//!< Offset into the cooked data for each mip level

public:
	CTextureBakerOutput();
	~CTextureBakerOutput();

	RED_INLINE Uint32 GetTotalDataSize() const { return m_data.Size(); }
	RED_INLINE const void* GetData() const { return m_data.Data(); }
	RED_INLINE Uint32 GetDataAlignment() const { return m_dataAlignment; }

	// Get table of mip offsets. Cooker function should fill it in with byte offsets into the cooked data.
	RED_INLINE TDynArray< Uint32 >& GetMipOffsets() { return m_mipOffsets; }

	// Write a chunk of data into the output buffer. If data is null, this will grow the buffer, filling the space with 0's.
	void WriteData( const void* data, Uint32 dataSize );

	RED_INLINE void SetDataAlignment( Uint32 alignment ) { m_dataAlignment = alignment; }
};


/////////////////////////////////////////////////////////////////////////////////////////////////
/// Asynchronous texture baker
class CAsyncTextureBaker : public Red::Threads::CThread
{
public:
	typedef Bool (*CookFunctionPtr)( const ITextureBakerSource& textureSource, CTextureBakerOutput& output );

protected:
	Red::Threads::CMutex			m_listMutex;			//!< Internal access mutex	
	TDynArray< STextureBakerTask* >	m_tasks;				//!< List of tasks
	Red::Threads::CSemaphore		m_wakeUpSemaphore;		//!< Semaphore for signaling that async thread should wake up
	Red::Threads::CSemaphore		m_workDoneSemaphore;	//!< Semaphore signaling that the work on the async thread is done
	CTextureCacheCooker*			m_cooker;				//!< Cooker
	Red::Threads::CAtomic< Bool >	m_loop;					//!< Thread loop control

public:
	//! Do we have any tasks pending ?
	RED_INLINE Bool HasTasks() const
	{
		return m_tasks.Size() > 0;
	}

public:
	CAsyncTextureBaker( Red::IO::CNativeFileHandle& cacheFile, CTextureCacheCooker* cooker )
		: Red::Threads::CThread( "Texture baker" )
		, m_wakeUpSemaphore( 0, INT_MAX )
		, m_workDoneSemaphore( 0, INT_MAX )
		, m_cooker( cooker )
		, m_loop( true )
	{
	}

	//! Load resource asynchronously
	void Bake( const String& debugName, Uint32 entry, const ITextureBakerSource& textureSource, CookFunctionPtr cookerPtr );

	//! For non-resource textures
	void Bake( const String& debugName, const String& path, const ITextureBakerSource& textureSource, CookFunctionPtr cookerPtr );

	//! Flush all tasks ( wait until all loading tasks are finished )
	void Flush();

	void Stop();

protected:
	//! Thread entry point
	virtual void ThreadFunc();

private:
	void BakeTask( STextureBakerTask* task );
	STextureBakerTask* PopTask();
};


/////////////////////////////////////////////////////////////////////////////////////////////////
/// Texture cache
class CTextureCacheCooker
{
protected:
	TDynArray< TextureCacheEntry >				m_entries;

	THashMap< Uint32, Uint32 >					m_entryMap;					//!< Hash -> index in m_entries

	TDynArray< AnsiChar >						m_strings;
	TDynArray< Uint32 >							m_mipOffsets;

	Uint32										m_pageIndex;				//!< First free page index
	Red::IO::CNativeFileHandle					m_cacheFile;				//!< Direct access to cache file
	Bool										m_modified;					//!< File is modified
	CAsyncTextureBaker*							m_baker;					//!< texture baker
	Red::Threads::CMutex						m_entryMutex;				//!< Internal access mutex

public:
	CTextureCacheCooker();
	virtual ~CTextureCacheCooker();

	//! Open/Create cache at given location
	Bool AttachToFile( const String& absoluteFileName );

	//! Flush the cache
	void Flush();

	//! Fill texture page data
	void WriteEntryData( Uint32 entryHash, const void* data, Uint32 compressedSize, Uint32 uncompressedSize, Uint32 baseAlignment, const TDynArray< Uint32 >& mipOffsets );

	//! Store data in the file. entryHash should be unique to this texture. path does not need to be unique, but should be consistent
	//! for the texture (storing two times with the same entryHash will expect the same path).
	void StoreTextureData( Uint32 entryHash, const String& path, const ITextureBakerSource& textureSource, CAsyncTextureBaker::CookFunctionPtr cookerPtr );

	//! Store data in the file. Used for non-resource textures (DDS's, PNG's, etc), which can later be looked up by path (instead of an index).
	Bool StoreNonResourceTextureData( const String& path, const ITextureBakerSource& textureSource, CAsyncTextureBaker::CookFunctionPtr cookerPtr );

	//! Write 0's into the cache file until the file pointer is aligned to the page size.
	void PadFileToPageSize();

	//! Get the default cook function for the given cooking options (mainly just platform, but maybe could add others?). Needs to be
	//! implemented by a subclass, since the cook function can easily require other platform-specific libraries and tools, and we don't
	//! want that in the engine project!
	virtual CAsyncTextureBaker::CookFunctionPtr GetDefaultCookFunction( const ECookingPlatform platform ) = 0;

private:
	Bool LoadCacheFile( const String& absoluteFileName );

	Bool AddEntry( Uint32 entryHash, const String& path, const ITextureBakerSource& textureSource );

	void AddNonResourceEntry( const String& path, const ITextureBakerSource& textureSource );

	Uint32 AddString( const String& str );
	Uint32 AddString( const StringAnsi& str );
};

#endif // !NO_TEXTURECACHE_COOKER


/////////////////////////////////////////////////////////////////////////////////////////////////
/// Texture cache loader
class CTextureCacheLoader
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

	friend CTextureCacheQuery;

public:
	CTextureCacheLoader();
	~CTextureCacheLoader();

	void StartLoading( const String& absoluteFileName );

	RED_INLINE Bool IsLoaded( ) const { return m_isLoaded.GetValue(); }

	CTextureCacheQuery FindEntry( Uint32 hash );
	CTextureCacheQuery FindNonResourceEntry( const String& path );

private:
	// Each section of the cache must be loaded in a separate ayncio operation. If we do as noted below, and
	// stick all tables into one chunk of memory, we could get away with just header and everything else.
	static Red::IO::ECallbackRequest LoadAsync_OnHeaderDone( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
	static Red::IO::ECallbackRequest LoadAsync_OnOffsetsDone( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
	static Red::IO::ECallbackRequest LoadAsync_OnStringsDone( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
	static Red::IO::ECallbackRequest LoadAsync_OnEntriesDone( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
	void LoadAsync_OnFailure();
	void LoadAsync_OnSuccess();

	RED_INLINE String GetString( Uint32 index ) const { return ANSI_TO_UNICODE( &m_strings[index] ); }
	RED_INLINE Uint32 GetMipOffset( Uint32 startIndex, Uint8 mip ) const { return mip == 0 ? 0 : m_mipOffsets[ startIndex + mip - 1 ]; }

	Red::Core::Decompressor::EStatus Decompress( const TextureCacheEntryBase& entry, Uint8 startMip, void* outBuffer, Uint32 bufferSize );

	CTextureCacheQuery::EResult LoadDataAsync( const TextureCacheEntryBase& entry, Uint8 startMip, Int8 numMips, void* buffer, Uint32 bufferSize, const Uint8 ioTag, IFileDecompressionTask*& outTask ) const;

	THashMap< Uint32, Uint32 >					m_entryMap;					//!< Hash -> index in m_entries

	// TODO : These three could actually be one large buffer, just keep pointers to each segment.
	// That would let us load them all with a single async read.
	TDynArray< TextureCacheEntry >				m_entries;
	TDynArray< AnsiChar >						m_strings;
	TDynArray< Uint32 >							m_mipOffsets;

	TextureCacheFile							m_asyncFile;				//!< Async file handle
	Red::Threads::CMutex						m_mutex;					//!< Access mutex (only one thread can read at once)
	String										m_absoluteFileName;

	Red::IO::SAsyncReadToken					m_loadingReadToken;			//!< During initial load, async token used to read in stuff

	Red::Threads::CAtomic< Bool >				m_isLoaded;

	Bool										m_allowLooseFileOverride;	//!< Allow the non resource texture to be overridden by loose files
};

/////////////////////////////////////////////////////////////////////////////////////////////////

/// Texture cache
#ifndef NO_TEXTURECACHE_COOKER
extern CTextureCacheCooker* GTextureCacheCooker;
#endif

//////////////////////////////////////////////////////////////////////////

/// Resolver for texture caches
class CTextureCacheResolver : public IContentListener
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	static const Uint32 MAX_CACHE_CHAIN_LENGTH = 64;

public:
	CTextureCacheResolver();
	virtual ~CTextureCacheResolver();
	void Shutdown();

	CTextureCacheQuery FindEntry( Uint32 hash );
	CTextureCacheQuery FindNonResourceEntry( const String& path );

private:
	virtual const Char* GetName() const override { return TXT("CTextureCacheResolver"); }
	virtual void OnContentAvailable( const SContentInfo& contentInfo ) override;

private:
	typedef Helper::CLazyCacheChain< CTextureCacheLoader, MAX_CACHE_CHAIN_LENGTH > CacheChain;
	CacheChain m_textureCacheChain;
};

extern CTextureCacheResolver* GTextureCache;

/////////////////////////////////////////////////////////////////////////////////////////////////

