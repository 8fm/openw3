#include "build.h"
#include "textureCache.h"
#include "../core/depot.h"
#include "../core/fileLatentLoadingToken.h"
#include "../core/compression/chainedzlib.h"
#include "texture.h"
#include "renderCommands.h"
#include "../core/contentManifest.h"
#include "../core/fileDecompression.h"
#include "../core/ioTags.h"

/////////////////////////////////////////////////////////////////////////////////////////////////


#define MIN_EXPECTED_TEXTURE_SIZE 32
#define MAX_EXPECTED_TEXTURE_SIZE 4096

TextureCacheEntryBase::TextureCacheEntryBase()
	: m_pageOffset( 0 )
	, m_compressedSize( 0 )
	, m_uncompressedSize( 0 )
	, m_baseAlignment( 0 )
	, m_baseWidth( 0 )
	, m_baseHeight( 0 )
	, m_mipCount( 0 )
	, m_sliceCount( 0 )
{
}


TextureCacheEntry::TextureCacheEntry()
	: m_hash( 0 )
	, m_pathStringIndex( 0 )
	, m_encodedFormat( 0 )
	, m_isCube( false )
{
}


Uint32 CTextureCacheQuery::GetCompressedSizeForMip( Uint8 mip ) const
{
	if ( m_cache == nullptr || m_entry == nullptr )
	{
		return 0;
	}
	if ( mip > m_entry->m_info.m_numMipOffsets )
	{
		return 0;
	}

	const Uint32 mipOffset = m_cache->GetMipOffset( m_entry->m_info.m_mipOffsetIndex, mip );
	return m_entry->m_info.m_compressedSize - mipOffset;
}


Uint8 CTextureCacheQuery::GetLowestLoadableMip() const
{
	if ( m_cache == nullptr || m_entry == nullptr )
	{
		return 0xff;
	}
	return static_cast< Uint8 >( m_entry->m_info.m_numMipOffsets );
}


String CTextureCacheQuery::GetPath() const
{
	if ( m_cache == nullptr || m_entry == nullptr )
	{
		return String::EMPTY;
	}
	return m_cache->GetString( m_entry->m_pathStringIndex );
}

ETextureCacheLoadResult CTextureCacheQuery::LoadData( Uint8 startMip, void* outBuffer, Uint32 bufferSize ) const
{
	if ( m_cache == nullptr || m_entry == nullptr )
	{
		return TCLR_InvalidQuery;
	}
	Red::Core::Decompressor::EStatus status = m_cache->Decompress( m_entry->m_info, startMip, outBuffer, bufferSize );

	switch ( status )
	{
	case Red::Core::Decompressor::Status_Success:
		return TCLR_Success;

	case Red::Core::Decompressor::Status_OutPointerNull:
	case Red::Core::Decompressor::Status_OutSizeZero:
	case Red::Core::Decompressor::Status_OutSizeTooSmall:
		return TCLR_BufferTooSmall;

	default:
		return TCLR_Error;
	};
}

CTextureCacheQuery::EResult CTextureCacheQuery::LoadDataAsync( Uint8 startMip, void* buffer, Uint32 bufferSize, const Uint8 ioTag, IFileDecompressionTask*& outTask, Int8 numMips /*= -1*/ ) const
{
	if ( m_cache == nullptr || m_entry == nullptr )
		return eResult_Failed;

	// mip 0 is not included in numMipOffsets
	if ( startMip > m_entry->m_info.m_numMipOffsets )
		return eResult_Failed;

	return m_cache->LoadDataAsync( m_entry->m_info, startMip, numMips, buffer, bufferSize, ioTag, outTask );
}


//////////////////////////////////////////////////////////////////////////


static RED_INLINE Uint64 PagesToBytes( Uint32 pages )
{
	return static_cast< Uint64 >( pages ) * TEXTURE_CACHE_PAGE_SIZE;
}

static RED_INLINE Uint32 BytesToPages( Uint64 bytes )
{
	return static_cast< Uint32 >( ( bytes + (TEXTURE_CACHE_PAGE_SIZE - 1) ) / TEXTURE_CACHE_PAGE_SIZE );
}


/////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef NO_TEXTURECACHE_COOKER
CTextureCacheCooker* GTextureCacheCooker = nullptr;
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef NO_TEXTURECACHE_COOKER


Bool CTextureCacheCooker::LoadCacheFile( const String& absoluteFileName )
{
	m_cacheFile.Open( absoluteFileName.AsChar(), Red::IO::eOpenFlag_ReadWrite| Red::IO::eOpenFlag_Create );
	if ( !m_cacheFile.IsValid() )
	{
		RED_LOG_WARNING( TextureCache, TXT("Failed to open texture cache file '%ls'"), absoluteFileName.AsChar() );
		return false;
	}

	RED_LOG( TextureCache, TXT("Loading texture cache from '%ls'"), absoluteFileName.AsChar() );

	// If file is too small to hold the header, just exit out. This is not a full failure, just means we start with an empty cache.
	if ( m_cacheFile.GetFileSize() < sizeof( TextureCacheHeader ) )
	{
#ifdef RED_LOGGING_ENABLED
		// If file is 0 bytes, we just created it. So of course it's empty. Just warn if the file size > 0
		if ( m_cacheFile.GetFileSize() > 0 )
		{
			RED_LOG_WARNING( TextureCache, TXT("Texture cache file is too small. Assuming it's empty.") );
		}
#endif
		return true;
	}

	// Go to the end of the file to read in the header.
	m_cacheFile.Seek( -static_cast< Int64 >( sizeof( TextureCacheHeader ) ), Red::IO::eSeekOrigin_End );

	TextureCacheHeader header;
	Uint32 bytesRead = 0;

	RED_VERIFY( m_cacheFile.Read( &header, sizeof( header ), bytesRead ) );
	if ( bytesRead != sizeof( header ) )
	{
		RED_LOG_WARNING( TextureCache, TXT("Couldn't read texture cache header. Clearing cache.") );
		return true;
	}

	// Texture cache marker
	if ( header.m_magicCode != TEXTURE_CACHE_HEADER )
	{
		RED_LOG_WARNING( TextureCache, TXT("Invalid Magic Code (%u). Clearing cache."), header.m_magicCode );
		return true;
	}

	if ( header.m_version != TEXTURE_CACHE_VERSION )
	{
		RED_LOG_WARNING( TextureCache, TXT("Wrong version (%u). Clearing cache."), header.m_version );
		return true;
	}

	// Seek to the marked position in the file
	Uint64 bytePosition = PagesToBytes( header.m_numUsedPages );
	m_cacheFile.Seek( bytePosition, Red::IO::eSeekOrigin_Set );

	// Calculate next store position
	m_pageIndex = header.m_numUsedPages;
	RED_LOG_SPAM( TextureCache, TXT("Texture cache currently contains %u pages."), m_pageIndex );


	m_strings.Resize( header.m_stringTableSize );
	RED_VERIFY( m_cacheFile.Read( m_strings.Data(), header.m_stringTableSize, bytesRead ) );

	m_entries.Resize( header.m_numEntries );
	RED_VERIFY( m_cacheFile.Read( m_entries.Data(), header.m_numEntries * sizeof( TextureCacheEntry ), bytesRead ) );


	// Check CRC
	{
		Uint64 crc = Red::CalculateHash64( m_strings.Data(), m_strings.DataSize(), RED_FNV_OFFSET_BASIS64 );
		crc = Red::CalculateHash64( m_entries.Data(), m_entries.DataSize(), crc );

		if ( header.m_crc != crc )
		{
			RED_LOG_WARNING( TextureCache, TXT("CRC Mismatch. Stored: %") RED_PRIWx64 TXT("; Calculated: %") RED_PRIWx64, header.m_crc, crc );

			m_strings.Clear();
			m_entries.Clear();
			m_pageIndex = 0;
			return true;
		}
	}


	// Allocate data
	RED_LOG_SPAM( TextureCache, TXT("%u resource entries found."), header.m_numEntries );
	m_entryMap.Reserve( header.m_numEntries );

	// Load entries
	for ( Uint32 i = 0; i < header.m_numEntries; ++i )
	{
		// Load data
		const TextureCacheEntry& entry = m_entries[i];
		if ( entry.m_info.m_pageOffset + BytesToPages( entry.m_info.m_compressedSize ) > header.m_numUsedPages )
		{
			RED_LOG( TextureCache, TXT("Entry %u goes beyond expected data size. Clearing cache."), i );
			m_entries.ClearFast();
			return true;
		}
		if( !m_entryMap.Insert( entry.m_hash, i ) )
		{
			RED_LOG_WARNING( TextureCache, TXT("Duplicate resource entry %ls found"), UNICODE_TO_ANSI( &m_strings[entry.m_pathStringIndex] ) );
		}
	}

	return true;
}


CTextureCacheCooker::CTextureCacheCooker()
	: m_pageIndex( 0 )
	, m_modified( false )
	, m_baker( nullptr )
{
}

CTextureCacheCooker::~CTextureCacheCooker()
{
	if ( m_cacheFile.IsValid() )
	{
		Flush();

		// Delete baker helper
		if ( m_baker )
		{
			m_baker->Stop();
			m_baker->JoinThread();
			delete m_baker;
		}

		m_cacheFile.Close();
	}
}

Bool CTextureCacheCooker::AttachToFile( const String& absoluteFileName )
{
	if ( !LoadCacheFile( absoluteFileName ) )
		return false;

	// create baker
	m_baker = new CAsyncTextureBaker(m_cacheFile, this);
	m_baker->InitThread();
	//m_baker->SetProcessor( 2 );

	m_modified = false;

	return true;
}

void CTextureCacheCooker::Flush()
{
	// Close the file
	if ( m_cacheFile.IsValid() && m_modified )
	{
		m_baker->Flush();

		// Go to the end of the file
		m_cacheFile.Seek( PagesToBytes( m_pageIndex ), Red::IO::eSeekOrigin_Set );

		Uint32 bytesWritten;

		RED_VERIFY( m_cacheFile.Write( m_mipOffsets.Data(), (Uint32)m_mipOffsets.DataSize(), bytesWritten ) );
		RED_VERIFY( m_cacheFile.Write( m_strings.Data(), (Uint32)m_strings.DataSize(), bytesWritten ) );
		RED_VERIFY( m_cacheFile.Write( m_entries.Data(), (Uint32)m_entries.DataSize(), bytesWritten ) );

		// Calculate CRC from strings and entries
		Uint64 crc = Red::CalculateHash64( m_mipOffsets.Data(), m_mipOffsets.DataSize(), RED_FNV_OFFSET_BASIS64 );
		crc = Red::CalculateHash64( m_strings.Data(), m_strings.DataSize(), crc );
		crc = Red::CalculateHash64( m_entries.Data(), m_entries.DataSize(), crc );

		// Write header
		TextureCacheHeader header;
		header.m_magicCode				= TEXTURE_CACHE_HEADER;
		header.m_version				= TEXTURE_CACHE_VERSION;
		header.m_crc					= crc;
		header.m_numUsedPages			= static_cast< Uint32 >( m_pageIndex );
		header.m_numEntries				= m_entries.Size();
		header.m_stringTableSize		= m_strings.Size();
		header.m_mipOffsetTableSize		= m_mipOffsets.Size();

		// If write position is not at the end of the file, we need to seek to the end. This could happen if we failed loading an existing
		// texture cache...
		// TODO : Better would be to truncate the file here, but that doesn't seem to be available.
		if ( static_cast< Uint64 >( m_cacheFile.Tell() + sizeof( header ) ) < m_cacheFile.GetFileSize() )
		{
			m_cacheFile.Seek( -static_cast< Int64 >( sizeof( header ) ), Red::IO::eSeekOrigin_End );
		}

		RED_VERIFY( m_cacheFile.Write( &header, sizeof( header ), bytesWritten ) );

		m_cacheFile.Flush();

		m_modified = false;

		RED_LOG( TextureCache, TXT("Flushed Texture Cache Cooker") );
		RED_LOG( TextureCache, TXT("   Total size: %") RED_PRIWu64, m_cacheFile.GetFileSize() );
		RED_LOG( TextureCache, TXT("     Header: %u"), sizeof( TextureCacheHeader ) );
		RED_LOG( TextureCache, TXT("     Mip offset table: %u"), m_mipOffsets.DataSize() );
		RED_LOG( TextureCache, TXT("     String table: %u"), m_strings.DataSize() );
		RED_LOG( TextureCache, TXT("     Resource entry table: %u"), m_entries.DataSize() );

		Uint64 totalPadding = 0;
		for ( const auto& entry : m_entries )
		{
			totalPadding += PagesToBytes( BytesToPages( entry.m_info.m_compressedSize ) ) - entry.m_info.m_compressedSize;
		}
		RED_LOG( TextureCache, TXT("   Space lost to padding: %") RED_PRIWu64, totalPadding );
	}
}


void CTextureCacheCooker::PadFileToPageSize()
{
	// If we're not page-aligned, find out how far into a page we currently are, and add enough to push us to the next.
	const Uint64 pos = m_cacheFile.Tell();
	const Uint32 leftOver = static_cast< Uint32 >( pos % TEXTURE_CACHE_PAGE_SIZE );
	if ( leftOver > 0 )
	{
		const Uint32 amountOfPadding = TEXTURE_CACHE_PAGE_SIZE - leftOver;

		Uint8 zeroBuffer[ TEXTURE_CACHE_PAGE_SIZE ] = {};

		Uint32 bytesWritten;
		RED_VERIFY( m_cacheFile.Write( zeroBuffer, amountOfPadding, bytesWritten ) );
		RED_WARNING( bytesWritten == amountOfPadding, "Not all bytes written?" );
	}
}


void CTextureCacheCooker::WriteEntryData( Uint32 entryHash, const void* data, Uint32 compressedSize, Uint32 uncompressedSize, Uint32 baseAlignment, const TDynArray< Uint32 >& mipOffsets )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_entryMutex );

	Uint32 entryIndex = m_entryMap.GetRef( entryHash, m_entries.Size() );
	if ( entryIndex == m_entries.Size() )
	{
		m_entries.Grow( 1 );
	}

	TextureCacheEntry& entry = m_entries[ entryIndex ];

	if ( data == nullptr || compressedSize == 0 )
	{
		entry.m_info.m_pageOffset = 0;
		entry.m_info.m_compressedSize = 0;
		entry.m_info.m_uncompressedSize = 0;
		return;
	}

	const Uint32 sizeInPages = BytesToPages( compressedSize );
	const Uint32 existingSizeInPages = BytesToPages( entry.m_info.m_compressedSize );

	// If the new data is too big for the existing entry, we need to append to the cache file. If the new data fits in the existing
	// entry, then we can just overwrite it!
	if ( sizeInPages > existingSizeInPages )
	{
		entry.m_info.m_pageOffset = m_pageIndex;
	}

	entry.m_info.m_compressedSize = compressedSize;
	entry.m_info.m_uncompressedSize = uncompressedSize;
	entry.m_info.m_baseAlignment = baseAlignment;

	// Track position in the mip offset table. We won't add the first entry, because it will always be 0.
	RED_ASSERT( mipOffsets.Size() > 0 && mipOffsets[0] == 0, TXT("First mip offset should be 0") );
	entry.m_info.m_mipOffsetIndex = m_mipOffsets.Size();
	entry.m_info.m_numMipOffsets = mipOffsets.Size() - 1;
	// If there's at least one non-zero offset, copy them to the table.
	if ( mipOffsets.Size() > 1 )
	{
		m_mipOffsets.Grow( entry.m_info.m_numMipOffsets );
		Red::System::MemoryCopy( &m_mipOffsets[ entry.m_info.m_mipOffsetIndex ], &mipOffsets[ 1 ], sizeof( Uint32 ) * entry.m_info.m_numMipOffsets );
	}

	const Uint32 endInPages = entry.m_info.m_pageOffset + sizeInPages;

	// Move to the write pos
	const Int64 offset = PagesToBytes( entry.m_info.m_pageOffset );
	m_cacheFile.Seek( offset, Red::IO::eSeekOrigin_Set );

	// Store data
	Uint32 bytesWritten;
	RED_VERIFY( m_cacheFile.Write( data, compressedSize, bytesWritten ) );
	RED_WARNING( bytesWritten == compressedSize, "Not all bytes written?" );

	PadFileToPageSize();

	// Update page index. If we added to the end, then endInPages will be bigger. Otherwise we filled existing data and pageIndex
	// need not change
	m_pageIndex = Max( endInPages, m_pageIndex );
}


Bool CTextureCacheCooker::AddEntry( Uint32 entryHash, const String& path, const ITextureBakerSource& textureSource )
{
	RED_ASSERT( entryHash != 0 );

	// Initialize entry
	TextureCacheEntry entry;

	Uint32 entryIndex = m_entries.Size();
	Bool didExist = false;
	{
		// Locked access
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_entryMutex );
		if ( m_entryMap.Find( entryHash, entryIndex ) )
		{
			entry = m_entries[ entryIndex ];
			didExist = true;
		}
	}

	if ( didExist && path != ANSI_TO_UNICODE( &m_strings[entry.m_pathStringIndex] ) )
	{
		RED_LOG_WARNING( TextureCache, TXT("Existing entry found for %ls, with different path %ls"), path.AsChar(), ANSI_TO_UNICODE( &m_strings[entry.m_pathStringIndex] ) );
	}

	entry.m_info.m_baseWidth		= textureSource.GetBaseWidth();
	entry.m_info.m_baseHeight		= textureSource.GetBaseHeight();
	entry.m_info.m_mipCount			= textureSource.GetMipCount();
	entry.m_info.m_sliceCount		= textureSource.GetSliceCount();
	entry.m_hash					= entryHash;
	entry.m_pathStringIndex			= AddString( path );
	entry.m_encodedFormat			= ITexture::EncodeTextureFormat( textureSource.GetTextureFormat() );
	entry.m_isCube					= textureSource.GetTextureType() == GpuApi::TEXTYPE_CUBE;
	entry.m_info.m_mipOffsetIndex	= 0;
	entry.m_info.m_numMipOffsets	= 0;

	// If we had an existing entry, don't clear out the offset and size. That way, maybe we can reuse the same space in the cache file,
	// without increasing the size.
	if ( !didExist )
	{
		entry.m_info.m_pageOffset	= 0;
		entry.m_info.m_compressedSize	= 0;
		entry.m_info.m_uncompressedSize	= 0;
	}

	if ( textureSource.IsLooseFileTexture() )	// For loose files textures we track a timestamp, so we can ignore the ones in the cache if they are outdated
	{
		CDiskFile* file = GDepot->FindFile( path );
		if ( file != nullptr )
		{
			entry.m_timestamp = file->GetFileTime();
		}
		else
		{
			RED_LOG_WARNING( TextureCache, TXT("Couldn't get file timestamp for %ls"), path.AsChar() );
			entry.m_timestamp.SetRaw( 0 );
		}
	}
	else // We want to make sure that in case of the CResource based textures, we don't put small textures in the cache - ones that should be resident.
	{
		if ( entry.m_info.m_baseWidth < MIN_EXPECTED_TEXTURE_SIZE && entry.m_info.m_baseHeight < MIN_EXPECTED_TEXTURE_SIZE )
		{
			RED_LOG_WARNING( TextureCache, TXT("Storing small texture in cache %ux%u %ls"), entry.m_info.m_baseWidth, entry.m_info.m_baseHeight, path.AsChar() );
		}
	}
	
	// Also, for any texture - either resource or loose file, we want to limit the size of textures we cook.
	if ( entry.m_info.m_baseWidth > MAX_EXPECTED_TEXTURE_SIZE && entry.m_info.m_baseHeight > MAX_EXPECTED_TEXTURE_SIZE )
	{
		RED_LOG_WARNING( TextureCache, TXT("Storing large texture in cache %ux%u %ls"), entry.m_info.m_baseWidth, entry.m_info.m_baseHeight, path.AsChar() );
	}

	{
		// Locked access
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_entryMutex );
		if ( didExist )
		{
			m_entries[ entryIndex ] = entry;
		}
		else
		{
			m_entryMap.Set( entryHash, m_entries.Size() );
			m_entries.PushBack( entry );
		}
	}

	return didExist;
}

void CTextureCacheCooker::StoreTextureData( Uint32 entryHash, const String& path, const ITextureBakerSource& textureSource, CAsyncTextureBaker::CookFunctionPtr cookerPtr )
{
	// Unable to store
	if ( !m_cacheFile.IsValid() )
	{
		RED_LOG_WARNING( TextureCache, TXT("Invalid texture cache handle") );
		return;
	}

	// Allocate entry
	AddEntry( entryHash, path, textureSource );

	//add task to queue for baking on another thread
	m_baker->Bake( path, entryHash, textureSource, cookerPtr );

	// Mark as modified
	m_modified = true;
}


Bool CTextureCacheCooker::StoreNonResourceTextureData( const String& path, const ITextureBakerSource& textureSource, CAsyncTextureBaker::CookFunctionPtr cookerPtr )
{
	// Unable to store
	if ( !m_cacheFile.IsValid() )
	{
		RED_LOG_WARNING( TextureCache, TXT("Invalid texture cache handle") );
		return false;
	}

	// Fix the path conformity and compute a hash out of it, so we can reference non-resource texture data the same way as we do with the resource textures
	String fixedPath = TextureCacheHelpers::FixNonResourcePath( path );
	Uint32 entryHash = GetHash( fixedPath );

	// Allocate entry
	AddEntry( entryHash, fixedPath, textureSource );

	//add task to queue for baking on another thread
	m_baker->Bake( fixedPath, entryHash, textureSource, cookerPtr );

	// Mark as modified
	m_modified = true;

	// Return texture entry index
	return true;
}


Uint32 CTextureCacheCooker::AddString( const String& str )
{
	return AddString( UNICODE_TO_ANSI( str.AsChar() ) );
}

Uint32 CTextureCacheCooker::AddString( const StringAnsi& str )
{
	const Uint32 length = str.GetLength();
	const Uint32 offset = m_strings.Size();
	m_strings.Grow( length + 1 );
	Red::MemoryCopy( &m_strings[ offset ], str.AsChar(), sizeof(AnsiChar) * (length+1) );
	return offset;
}



#endif // !NO_TEXTURECACHE_COOKER


/////////////////////////////////////////////////////////////////////////////////////////////////


struct TextureCacheLoadContext
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Temporary );

	CTextureCacheLoader*		m_cache;
	TextureCacheHeader			m_header;
	CTimeCounter				m_timer;			// May not be exact, across threads/cores, but an approximation of time taken.
};



CTextureCacheLoader::CTextureCacheLoader()
	: m_asyncFile( Red::IO::CAsyncIO::INVALID_FILE_HANDLE )
	, m_isLoaded( false )
	, m_allowLooseFileOverride( false )
{
	// Allow the overrides for the non resource textures
#ifndef RED_FINAL_BUILD
	m_allowLooseFileOverride = ( Red::StringSearch( SGetCommandLine(), TXT("-allowtexturecacheoverride") ) != nullptr );
#endif
}

CTextureCacheLoader::~CTextureCacheLoader()
{
	if ( m_asyncFile )
	{
		Red::IO::GAsyncIO.ReleaseFile( m_asyncFile );
		m_asyncFile = Red::IO::CAsyncIO::INVALID_FILE_HANDLE;
	}
}



void CTextureCacheLoader::LoadAsync_OnFailure()
{
	m_mipOffsets.Clear();
	m_strings.Clear();
	m_entries.Clear();
}

void CTextureCacheLoader::LoadAsync_OnSuccess()
{
	m_isLoaded.SetValue( true );

	// Notify renderer that there may be new texture data available.
	if ( GRender && GRender->GetRenderThread() )
	{
		( new CRenderCommand_TextureCacheAttached() )->Commit();
	}
}


void CTextureCacheLoader::StartLoading( const String& absoluteFileName )
{
	// Open sync file on demand.
	m_absoluteFileName = absoluteFileName;

	// Open async file handle
	m_asyncFile = Red::IO::GAsyncIO.OpenFile( absoluteFileName.AsChar(), Red::IO::eAsyncFlag_TryCloseFileWhenNotUsed );
	if ( m_asyncFile == Red::IO::CAsyncIO::INVALID_FILE_HANDLE )
	{
		RED_LOG_WARNING( TextureCache, TXT("Could not open async access to texture cache file %ls"), absoluteFileName.AsChar() );
		return;
	}

	RED_LOG_SPAM( TextureCache, TXT("Started loading texture cache %ls"), absoluteFileName.AsChar() );

	Uint64 fileSize = Red::IO::GAsyncIO.GetFileSize( m_asyncFile );
	if ( fileSize <= sizeof( TextureCacheHeader ) )
	{
		RED_LOG_WARNING( TextureCache, TXT("Empty texture cache file %ls"), absoluteFileName.AsChar() );
		Red::IO::GAsyncIO.ReleaseFile( m_asyncFile );
		m_asyncFile = Red::IO::CAsyncIO::INVALID_FILE_HANDLE;
		return;
	}

	Uint64 offset = fileSize - sizeof( TextureCacheHeader );

	TextureCacheLoadContext* context = new TextureCacheLoadContext();
	context->m_cache = this;

	// Read in header.
	m_loadingReadToken.m_userData				= context;
	m_loadingReadToken.m_callback				= &LoadAsync_OnHeaderDone;
	m_loadingReadToken.m_offset					= offset;
	m_loadingReadToken.m_numberOfBytesToRead	= sizeof( TextureCacheHeader );
	m_loadingReadToken.m_buffer					= &context->m_header;
	Red::IO::GAsyncIO.BeginRead( m_asyncFile, m_loadingReadToken, Red::IO::eAsyncPriority_Critical, eIOTag_TexturesImmediate );
}

Red::IO::ECallbackRequest CTextureCacheLoader::LoadAsync_OnHeaderDone( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	TextureCacheLoadContext* context = static_cast< TextureCacheLoadContext* >( asyncReadToken.m_userData );
	CTextureCacheLoader* cache = context->m_cache;

	if ( asyncResult != Red::IO::eAsyncResult_Success )
	{
		RED_LOG_ERROR( TextureCache, TXT("TextureCache AsyncIO: loading of TextureCacheHeader failed.") );
		cache->LoadAsync_OnFailure();
		asyncReadToken.m_userData = nullptr;
		delete context;
		return Red::IO::eCallbackRequest_Finish;
	}

	RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

	if ( context->m_header.m_magicCode != TEXTURE_CACHE_HEADER )
	{
		RED_LOG_ERROR( TextureCache, TXT("Invalid header id in texture cache {%x}. Expected {%x}"), context->m_header.m_magicCode, TEXTURE_CACHE_HEADER );
		cache->LoadAsync_OnFailure();
		asyncReadToken.m_userData = nullptr;
		delete context;
		return Red::IO::eCallbackRequest_Finish;
	}

	if ( context->m_header.m_version != TEXTURE_CACHE_VERSION )
	{
		RED_LOG_ERROR( TextureCache, TXT("Invalid version %u. Expected %u"), context->m_header.m_version, TEXTURE_CACHE_VERSION );
		cache->LoadAsync_OnFailure();
		asyncReadToken.m_userData = nullptr;
		delete context;
		return Red::IO::eCallbackRequest_Finish;
	}

	cache->m_mipOffsets.Resize( context->m_header.m_mipOffsetTableSize );
	cache->m_strings.Resize( context->m_header.m_stringTableSize );
	cache->m_entries.Resize( context->m_header.m_numEntries );


	// Read in mip offsets.
	asyncReadToken.m_callback				= &LoadAsync_OnOffsetsDone;
	asyncReadToken.m_offset					= PagesToBytes( context->m_header.m_numUsedPages );
	asyncReadToken.m_numberOfBytesToRead	= (Uint32)cache->m_mipOffsets.DataSize();
	asyncReadToken.m_buffer					= cache->m_mipOffsets.Data();

	return Red::IO::eCallbackRequest_More;
}

Red::IO::ECallbackRequest CTextureCacheLoader::LoadAsync_OnOffsetsDone( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	TextureCacheLoadContext* context = static_cast< TextureCacheLoadContext* >( asyncReadToken.m_userData );
	CTextureCacheLoader* cache = context->m_cache;

	if ( asyncResult != Red::IO::eAsyncResult_Success )
	{
		RED_LOG_ERROR( TextureCache, TXT("TextureCache AsyncIO: loading of Mip Offset Table failed.") );
		cache->LoadAsync_OnFailure();
		asyncReadToken.m_userData = nullptr;
		delete context;
		return Red::IO::eCallbackRequest_Finish;
	}

	RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

	// Read in string table.
	asyncReadToken.m_callback				= &LoadAsync_OnStringsDone;
	asyncReadToken.m_offset					+= asyncReadToken.m_numberOfBytesToRead;
	asyncReadToken.m_numberOfBytesToRead	= (Uint32)cache->m_strings.DataSize();
	asyncReadToken.m_buffer					= cache->m_strings.Data();

	return Red::IO::eCallbackRequest_More;
}

Red::IO::ECallbackRequest CTextureCacheLoader::LoadAsync_OnStringsDone( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	TextureCacheLoadContext* context = static_cast< TextureCacheLoadContext* >( asyncReadToken.m_userData );
	CTextureCacheLoader* cache = context->m_cache;

	if ( asyncResult != Red::IO::eAsyncResult_Success )
	{
		RED_LOG_ERROR( TextureCache, TXT("TextureCache AsyncIO: loading of String Table failed.") );
		cache->LoadAsync_OnFailure();
		asyncReadToken.m_userData = nullptr;
		delete context;
		return Red::IO::eCallbackRequest_Finish;
	}

	RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

	// Read in string table.
	asyncReadToken.m_callback				= &LoadAsync_OnEntriesDone;
	asyncReadToken.m_offset					+= asyncReadToken.m_numberOfBytesToRead;
	asyncReadToken.m_numberOfBytesToRead	= (Uint32)cache->m_entries.DataSize();
	asyncReadToken.m_buffer					= cache->m_entries.Data();

	return Red::IO::eCallbackRequest_More;
}

Red::IO::ECallbackRequest CTextureCacheLoader::LoadAsync_OnEntriesDone( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	TextureCacheLoadContext* context = static_cast< TextureCacheLoadContext* >( asyncReadToken.m_userData );
	CTextureCacheLoader* cache = context->m_cache;

	if ( asyncResult != Red::IO::eAsyncResult_Success )
	{
		RED_LOG_ERROR( TextureCache, TXT("TextureCache AsyncIO: loading of Entry Table failed.") );
		cache->LoadAsync_OnFailure();
		asyncReadToken.m_userData = nullptr;
		delete context;
		return Red::IO::eCallbackRequest_Finish;
	}

	RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

	// All data is now loaded, so just need to process it.
	
	// TODO : This callback might block async IO for a bit. It would be nice to make this stuff faster or something.
	//   Simple idea would be to offload it to a CTask. But GTaskManager might not be created yet...


#ifndef RED_FINAL_BUILD
	// Check CRC
	{
		Uint64 crc = Red::CalculateHash64( cache->m_mipOffsets.Data(), cache->m_mipOffsets.DataSize(), RED_FNV_OFFSET_BASIS64 );
		crc = Red::CalculateHash64( cache->m_strings.Data(), cache->m_strings.DataSize(), crc );
		crc = Red::CalculateHash64( cache->m_entries.Data(), cache->m_entries.DataSize(), crc );

		if ( context->m_header.m_crc != crc )
		{
			RED_LOG_ERROR( TextureCache, TXT("CRC Mismatch. Stored: %") RED_PRIWx64 TXT("; Calculated: %") RED_PRIWx64, context->m_header.m_crc, crc );
			cache->LoadAsync_OnFailure();
			asyncReadToken.m_userData = nullptr;
			delete context;
			return Red::IO::eCallbackRequest_Finish;
		}
	}
#endif

	const Uint32 numEntries = cache->m_entries.Size();
	cache->m_entryMap.Reserve( numEntries );
	for ( Uint32 i = 0; i < numEntries; ++i )
	{
		// Load entry
		const TextureCacheEntry& entry = cache->m_entries[i];

		if ( !entry.m_timestamp.IsValid() )	// TODO: This is a hacky check for whether this is a loose file texture. Change it so it uses a proper flag!
		{
			// Make sure what's stored in the texture cache makes sense.
			if ( entry.m_info.m_baseWidth < MIN_EXPECTED_TEXTURE_SIZE && entry.m_info.m_baseHeight < MIN_EXPECTED_TEXTURE_SIZE )
			{
				RED_LOG_WARNING( TextureCache, TXT("Small texture found in cache %ux%u %ls"), entry.m_info.m_baseWidth, entry.m_info.m_baseHeight, cache->GetString( entry.m_pathStringIndex ).AsChar() );
			}
		}

		if ( entry.m_info.m_baseWidth > MAX_EXPECTED_TEXTURE_SIZE && entry.m_info.m_baseHeight > MAX_EXPECTED_TEXTURE_SIZE )
		{
			RED_LOG_WARNING( TextureCache, TXT("Large texture found in cache %ux%u %ls"), entry.m_info.m_baseWidth, entry.m_info.m_baseHeight, cache->GetString( entry.m_pathStringIndex ).AsChar() );
		}

		// Check that the entry doesn't run beyond how many pages we are supposed to have. If it does, we will abort loading. Any current
		// entries will remain, but we won't try to load more. This way we don't need to handle broken entries in the middle of the array
		// (which shouldn't happen anyways), but we can still use what we already have.
		if ( entry.m_info.m_pageOffset + BytesToPages( entry.m_info.m_compressedSize ) > context->m_header.m_numUsedPages )
		{
			RED_LOG_ERROR( TextureCache, TXT("Entry in texture cache goes beyond expected size, aborting! %ls"), cache->GetString( entry.m_pathStringIndex ).AsChar() );
			break;
		}

		if ( !cache->m_entryMap.Insert( entry.m_hash, i ) )
		{
			RED_LOG_WARNING( TextureCache, TXT("Duplicate resource entry %ls found"), cache->GetString( entry.m_pathStringIndex ).AsChar() );
		}
	}


	// Stats
	RED_LOG_SPAM( TextureCache, TXT("Found %u textures in texture cache file '%ls' after %0.3fms."), 
		numEntries, 
		Red::IO::GAsyncIO.GetFileName( cache->m_asyncFile ), 
		context->m_timer.GetTimePeriodMS() );

	cache->LoadAsync_OnSuccess();

	asyncReadToken.m_userData = nullptr;
	delete context;

	return Red::IO::eCallbackRequest_Finish;
}



CTextureCacheQuery CTextureCacheLoader::FindEntry( Uint32 hash )
{
	if ( !m_isLoaded.GetValue() )
	{
		return CTextureCacheQuery();
	}


	const TextureCacheEntry* entry = nullptr;
	{
		Uint32 index;
		if ( m_entryMap.Find( hash, index ) )
		{
			entry = &m_entries[ index ];
		}
	}
	if ( entry == nullptr )
	{
		return CTextureCacheQuery();
	}

	if ( entry->m_info.m_pageOffset == 0 && entry->m_info.m_compressedSize == 0 )
	{
		return CTextureCacheQuery();
	}

	return CTextureCacheQuery( this, entry );
}


CTextureCacheQuery CTextureCacheLoader::FindNonResourceEntry( const String& path )
{
	if ( !m_isLoaded.GetValue() )
	{
		return CTextureCacheQuery();
	}


	String fixedPath = TextureCacheHelpers::FixNonResourcePath( path );
	Uint32 entryHash = GetHash( fixedPath );

	CTextureCacheQuery query = FindEntry( entryHash );

#ifndef RED_FINAL_BUILD
	if ( query )
	{
		if ( m_allowLooseFileOverride )
		{
			// When accessing the depot always use safe paths!
			String temp1;
			const String& conformedSafePath = CFilePath::ConformPath( path, temp1 );

			// Timestamp check against original file, in case cache is out of date. If original file is newer, we treat it as though it's not in the
			// cache at all.
			CDiskFile* file = GDepot->FindFile( conformedSafePath );
			if ( file != nullptr )
			{
				Red::System::DateTime fileTimestamp = file->GetFileTime();
				if ( fileTimestamp > query.GetEntry().m_timestamp )
				{
					return CTextureCacheQuery();
				}
			}
		}
	}
#endif

	return query;
}


Red::Core::Decompressor::EStatus CTextureCacheLoader::Decompress( const TextureCacheEntryBase& entry, Uint8 startMip, void* outBuffer, Uint32 bufferSize )
{
	if ( startMip > entry.m_numMipOffsets )
	{
		RED_HALT( "Cannot load from mip %u. Only %u compressed chunks.", startMip, ( entry.m_numMipOffsets + 1 ) );
		return Red::Core::Decompressor::Status_InvalidData;
	}

	const Uint32 mipOffset = GetMipOffset( entry.m_mipOffsetIndex, startMip );

	// Read in the compressed data.
	DataBuffer compressedData( TDataBufferAllocator< MC_Temporary >::GetInstance(), entry.m_compressedSize - mipOffset );
	{
		// Locking around a local... at this point just limiting the number of concurrent handles
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
		Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( m_absoluteFileName, FOF_AbsolutePath | FOF_Buffered ) );
		if ( !file )
		{
			RED_LOG_WARNING( TextureCache, TXT("Could not open texture cache file %ls"), m_absoluteFileName.AsChar() );
			return Red::Core::Decompressor::Status_Uninitialized;
		}

		file->Seek( PagesToBytes( entry.m_pageOffset ) + mipOffset );
		file->Serialize( compressedData.GetData(), compressedData.GetSize() );
	}

	// Decompress it.
	Red::Core::Decompressor::CChainedZLib decompressor;
	decompressor.Initialize( compressedData.GetData(), outBuffer, compressedData.GetSize(), bufferSize );

	return decompressor.Decompress();
}

CTextureCacheQuery::EResult CTextureCacheLoader::LoadDataAsync( const TextureCacheEntryBase& entry, Uint8 startMip, Int8 numMips, void* buffer, Uint32 bufferSize, const Uint8 ioTag, IFileDecompressionTask*& outTask ) const
{
	if ( startMip > entry.m_numMipOffsets )
	{
		RED_HALT( "Cannot load from mip %u. Only %u compressed chunks.", startMip, ( entry.m_numMipOffsets + 1 ) );
		return CTextureCacheQuery::eResult_Failed;
	}

	const Uint32 startMipOffset = GetMipOffset( entry.m_mipOffsetIndex, startMip );
	const Uint32 endMipOffset = ( numMips >= 0 && startMip + numMips <= entry.m_numMipOffsets )
		? GetMipOffset( entry.m_mipOffsetIndex, startMip + numMips )
		: entry.m_compressedSize;

	// prepare task info
	CFileDecompression::TaskInfo info;
	info.m_asyncFile = m_asyncFile;
	info.m_compressionType = Red::Core::Bundle::CT_ChainedZlib;
	info.m_ioTag = ioTag;
	info.m_offset = PagesToBytes( entry.m_pageOffset ) + startMipOffset;
	info.m_compressedSize = endMipOffset - startMipOffset;
	info.m_uncompressedMemory = buffer;
	info.m_uncompressedSize = bufferSize;

	// start task - may fail due to not enough resources
	return (CTextureCacheQuery::EResult) GFileManager->GetDecompressionEngine()->DecompressAsyncFile( info, outTask );
}

/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
// Texture cache resolver
CTextureCacheResolver::CTextureCacheResolver()
{
}

CTextureCacheResolver::~CTextureCacheResolver()
{
}

void CTextureCacheResolver::Shutdown()
{
	m_textureCacheChain.ClearPtr();
}



CTextureCacheQuery CTextureCacheResolver::FindEntry( Uint32 hash )
{
	CTextureCacheQuery query;
	for ( CTextureCacheLoader* cache : m_textureCacheChain )
	{
		query = cache->FindEntry( hash );
		if ( query )
		{
			break;
		}
	}
	return query;
}

CTextureCacheQuery CTextureCacheResolver::FindNonResourceEntry( const String& path )
{
	CTextureCacheQuery query;
	for ( CTextureCacheLoader* cache : m_textureCacheChain )
	{
		query = cache->FindNonResourceEntry( path );
		if ( query )
		{
			break;
		}
	}
	return query;
}

void CTextureCacheResolver::OnContentAvailable( const SContentInfo& contentInfo )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Mounting supported on main thread only" );

	typedef Bool (CacheChain::* PushFunc)(CTextureCacheLoader*);

	for ( const SContentFile* contentFile : contentInfo.m_contentFiles )
	{
		const PushFunc Push = contentFile->m_isPatch ? &CacheChain::PushFront : &CacheChain::PushBack;
		const String textureCacheFile = String::Printf( TXT("%ls%hs"), contentInfo.m_mountPath.AsChar(), contentFile->m_path.AsChar() );
		LOG_ENGINE(TXT("CTextureCacheResolver: attaching '%ls'"), textureCacheFile.AsChar() );
		CTextureCacheLoader* cache = new CTextureCacheLoader();
		if ( ! (m_textureCacheChain.*Push)( cache ) )
		{
			delete cache;
			ERR_ENGINE(TXT("Failed to add texture cache '%ls'. Reached chain length limit %u"), textureCacheFile.AsChar(), MAX_CACHE_CHAIN_LENGTH );
			return;
		}

		cache->StartLoading( textureCacheFile );
	}
}

static CTextureCacheResolver GTextureCacheResolver;
CTextureCacheResolver* GTextureCache = &GTextureCacheResolver;
