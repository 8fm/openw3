/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "patchBuilder.h"
#include "patchSpeeches.h"
#include "patchUtils.h"

//-----------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CPatchBuilder_Speeches );

void PatchSpeechHelper::CleanRIFFHeader( void* buffer )
{
	struct RIFFHeader
	{
		Uint32 m_fileSize;
		Uint32 m_riffMagic;
		Uint32 m_riffChunkLength;
		Uint32 m_waveMagic;
		Uint32 m_fmtMagic;
		Uint32 m_fmtSize;
		Uint16 m_fmtFormatCode;
		Uint16 m_fmtChannels;
		Uint32 m_fmtSampleRate;
		Uint32 m_fmtBytesPerSecond;
		Uint16 m_fmtBytesPerSample;
		Uint16 m_fmtBitsPerSample;

		Bool IsValid( )
		{
			return m_riffMagic == 'FFIR' && m_waveMagic == 'EVAW' && m_fmtMagic == ' tmf';
		}
	};

	// Memzero undocumented extension chunk in RIFF header.
	RIFFHeader* riffHeader = reinterpret_cast< RIFFHeader* >( buffer );
	if( riffHeader->IsValid( ) && riffHeader->m_fmtSize > 0x10 )
	{
		Red::MemoryZero( reinterpret_cast< void* >( reinterpret_cast< Uint8* >( riffHeader ) + sizeof( RIFFHeader ) ), riffHeader->m_fmtSize - 0x10 );
	}
}

//-----------------------------------------------------------------------------

CPatchSpeechToken::CPatchSpeechToken( Uint64 hash, Uint64 crc, Uint64 size, TPatchSpeechCacheTrio* cache )
	: m_id( hash )
	, m_crc( crc )
	, m_size( size )
	, m_cache( cache )
{ }

CPatchSpeechToken::~CPatchSpeechToken( ) { }

const Uint64 CPatchSpeechToken::GetTokenHash( ) const
{
	return m_id;
}

const Uint64 CPatchSpeechToken::GetDataCRC( ) const
{
	return m_crc;
}

const Uint64 CPatchSpeechToken::GetDataSize( ) const
{
	return m_size;
}

const String CPatchSpeechToken::GetInfo( ) const
{
	return String::Printf( TXT( "Speech entry %08X" ), m_id & 0xffffffff );
}

void CPatchSpeechToken::DebugDump( const String& dumpPath, const Bool isBase ) const
{
	Uint32 tokenId = m_id & 0xffffffff;

	String absolutePath = dumpPath;
	absolutePath += String::Printf( TXT( "%08X.speech.%s.%s.txt" ),
		tokenId,
		m_cache->m_lang.AsChar( ),
		isBase ? TXT( "base" ) : TXT( "current" ) );

	Red::TScopedPtr< IFile > outFile( GFileManager->CreateFileWriter( absolutePath.AsChar( ), FOF_AbsolutePath ) );
	if( outFile )
	{
		CCookedSpeeches::SOffset offset;
		m_cache->m_cache.GetOffset( tokenId, offset );
		if( offset.m_voSize > 0 )
		{
			Uint8* buffer = new Uint8 [ offset.m_voSize ];
			m_cache->m_file->Seek( offset.m_voOffset );
			m_cache->m_file->Serialize( buffer, offset.m_voSize );
			PatchSpeechHelper::CleanRIFFHeader( buffer );
			outFile->Serialize( buffer, offset.m_voSize );
			delete [ ] buffer;
		}
		if( offset.m_lipsyncSize > 0 )
		{
			Uint8* buffer = new Uint8 [ offset.m_lipsyncSize ];
			m_cache->m_file->Seek( offset.m_lipsyncOffset );
			m_cache->m_file->Serialize( buffer, offset.m_lipsyncSize );
			outFile->Serialize( buffer, offset.m_lipsyncSize );
			delete [ ] buffer;
		}
	}
}

//-----------------------------------------------------------------------------

CPatchSpeechesCache::~CPatchSpeechesCache( )
{
	for( Uint32 i = 0; i < m_caches.Size( ); ++i )
	{
		if( m_caches[ i ]->m_file != nullptr )
			delete m_caches[ i ]->m_file;
		m_caches[ i ]->m_file = nullptr;
	}
	m_caches.ClearPtr( );
	m_tokens.ClearPtr( );
}

void CPatchSpeechesCache::GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const
{
	outTokens.Reserve( m_tokens.Size( ) );
	for( auto* ptr : m_tokens )
	{
		outTokens.PushBack( ptr );
	}
}

const Uint64 CPatchSpeechesCache::GetDataSize( ) const
{
	Uint64 totalSize = 0;
	for( auto* ptr : m_tokens )
		totalSize += ptr->GetDataSize( );
	return totalSize;
}

const String CPatchSpeechesCache::GetInfo( ) const
{
	return TXT( "Speeches" );
}

CPatchSpeechesCache* CPatchSpeechesCache::LoadContent( const String& baseDirectory )
{
	TDynArray< String > speechesCachePaths;
	GFileManager->FindFiles( baseDirectory, TXT( "*.w3speech" ), speechesCachePaths, true );

	CPatchSpeechesCache* patchCache = new CPatchSpeechesCache( );
	THashMap< Uint64, CPatchSpeechToken* > fileTokensMap;

	LOG_WCC( TXT( "Found %d speech caches in build '%ls'."), speechesCachePaths.Size( ), baseDirectory.AsChar( ) );

	Uint32 totalEntries = 0;

	for( const String& cachePath : speechesCachePaths )
	{
		LOG_WCC( TXT( "Loading speeches cache '%ls'..." ), cachePath.AsChar( ) );

		String lang = CFilePath( cachePath ).GetFileName( );
		Uint64 langHash = static_cast< Uint64 >( lang.CalcHash( ) ) << 32;

		IFile* file = GFileManager->CreateFileReader( cachePath, FOF_Buffered | FOF_AbsolutePath );
		if( !file )
		{
			ERR_WCC( TXT( "Failed to open cache from '%ls'!" ), cachePath.AsChar( ) );
			delete patchCache;
			return nullptr;
		}

		TPatchSpeechCacheTrio* cacheTrio = new TPatchSpeechCacheTrio( );
		if( !cacheTrio->m_cache.Load( *file ) )
		{
			ERR_WCC( TXT( "Failed to load cache contents from '%ls'!" ), cachePath.AsChar( ) );
			delete file;
			delete cacheTrio;
			delete patchCache;
			return nullptr;
		}

		cacheTrio->m_lang = lang;
		cacheTrio->m_file = file;

		patchCache->m_caches.PushBack( cacheTrio );

		CCookedSpeeches& cache = cacheTrio->m_cache;

		totalEntries += cache.m_offsetMap.Size( );

		for( CCookedSpeeches::TOffsetMap::const_iterator it = cache.m_offsetMap.Begin( ); it != cache.m_offsetMap.End( ); ++it )
		{
			CCookedSpeeches::TSpeechId decryptedId = it->m_first ^ cache.m_langKey;
			Uint64 speechId = decryptedId | langHash;

			if( fileTokensMap.KeyExist( speechId ) )
				continue;

			CCookedSpeeches::SOffset speechOffset;
			cache.GetOffset( decryptedId, speechOffset );

			Uint64 tokenCRC = 0;

			if( speechOffset.m_voSize != 0 )
			{
				Uint8* buffer = new Uint8 [ speechOffset.m_voSize ];
				file->Seek( speechOffset.m_voOffset );
				file->Serialize( buffer, speechOffset.m_voSize );
				PatchSpeechHelper::CleanRIFFHeader( buffer );
				tokenCRC = Red::CalculateHash64( buffer, speechOffset.m_voSize, tokenCRC );
				delete [ ] buffer;
			}
			if( speechOffset.m_lipsyncSize != 0 )
			{
				file->Seek( speechOffset.m_lipsyncOffset );

				// lipsync animations are fucked up, we hash them manually

				DependencyLoadingContext context;
				context.m_getAllLoadedObjects = true;
				CDependencyLoader loader( *file, nullptr );
				if( !loader.LoadObjects( context ) )
				{
					ERR_WCC( TXT( "Lipsync deserialization failed!" ) );
				}
				else
				{
					CSkeletalAnimation* lipsync = Cast< CSkeletalAnimation >( context.m_loadedObjects[ 0 ].GetSerializablePtr( ) );
					tokenCRC = lipsync->GetAnimBufferCRC( tokenCRC );
					delete context.m_loadedObjects[ 0 ].GetSerializablePtr( );
				}
			}

			Uint64 tokenSize = it->m_second.m_voSize + it->m_second.m_lipsyncSize;

			CPatchSpeechToken* speechToken = new CPatchSpeechToken( speechId, tokenCRC, tokenSize, cacheTrio );
			patchCache->m_tokens.PushBack( speechToken );

			fileTokensMap.Insert( speechId, speechToken );
		}
	}

	LOG_WCC( TXT( "Speeches cache summary: %d entries, %d unique ones" ), totalEntries, patchCache->m_tokens.Size( ) );

	return patchCache;
}

//-----------------------------------------------------------------------------

CPatchBuilder_Speeches::CPatchBuilder_Speeches( )
	: m_patchSpeechesCache( nullptr )
{
}

CPatchBuilder_Speeches::~CPatchBuilder_Speeches( )
{
	if ( m_patchSpeechesCache )
	{
		delete m_patchSpeechesCache;
		m_patchSpeechesCache;
	}
}

String CPatchBuilder_Speeches::GetContentType( ) const
{
	return TXT( "speeches" );
}

CPatchBuilder_Speeches::IContentGroup* CPatchBuilder_Speeches::LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )
{
	m_patchSpeechesCache = CPatchSpeechesCache::LoadContent( absoluteBuildPath );
	return m_patchSpeechesCache;
}

Bool CPatchBuilder_Speeches::SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName )
{
	if( patchContent.Empty( ) )
	{
		ERR_WCC( TXT( "There's no speech data to output!" ) );
		return false;
	}

	struct TFinalCacheEntry
	{
		CCookedSpeeches m_cache;
		IFile*			m_tempFile;
	};

	typedef THashMap< String, TFinalCacheEntry* > TFinalCacheMap;
	TFinalCacheMap finalCaches;

	for( IContentToken* token : patchContent )
	{
		CPatchSpeechToken* speechToken = static_cast< CPatchSpeechToken* >( token );
		const String& lang = speechToken->m_cache->m_lang;

		TFinalCacheEntry* cacheEntry = nullptr;
		if( !finalCaches.Find( lang, cacheEntry ) )
		{
			cacheEntry = new TFinalCacheEntry( );

			cacheEntry->m_cache.m_fileKey = speechToken->m_cache->m_cache.m_fileKey;
			cacheEntry->m_cache.m_langKey = speechToken->m_cache->m_cache.m_langKey;

			finalCaches.Insert( lang, cacheEntry );

			const String tempFilePath = absoluteBuildPath + patchName + String::Printf( TXT( "\\%s.temp.w3speech" ), lang.AsChar( ) );
			cacheEntry->m_tempFile = GFileManager->CreateFileWriter( tempFilePath, FOF_AbsolutePath );
			if( cacheEntry->m_tempFile == nullptr )
			{
				ERR_WCC( TXT( "Failed to open temporary file '%ls'!" ), tempFilePath.AsChar( ) );
				continue;
			}
		}

		RED_ASSERT( cacheEntry->m_cache.m_langKey == speechToken->m_cache->m_cache.m_langKey );

		Uint32 tokenId = speechToken->m_id & 0xffffffff;

		CCookedSpeeches::SOffset speechOffset;
		speechToken->m_cache->m_cache.GetOffset( tokenId, speechOffset );

		if( speechOffset.m_voSize != 0 )
		{
			Uint8* buffer = new Uint8 [ speechOffset.m_voSize ];
			speechToken->m_cache->m_file->Seek( speechOffset.m_voOffset );
			speechToken->m_cache->m_file->Serialize( buffer, speechOffset.m_voSize );
			speechOffset.m_voOffset = cacheEntry->m_tempFile->GetOffset( );
			cacheEntry->m_tempFile->Serialize( buffer, speechOffset.m_voSize );
			delete [ ] buffer;
		}
		if( speechOffset.m_lipsyncSize != 0 )
		{
			Uint8* buffer = new Uint8 [ speechOffset.m_lipsyncSize ];
			speechToken->m_cache->m_file->Seek( speechOffset.m_lipsyncOffset );
			speechToken->m_cache->m_file->Serialize( buffer, speechOffset.m_lipsyncSize );
			speechOffset.m_lipsyncOffset = cacheEntry->m_tempFile->GetOffset( );
			cacheEntry->m_tempFile->Serialize( buffer, speechOffset.m_lipsyncSize );
			delete [ ] buffer;
		}

		cacheEntry->m_cache.AddOffset( speechOffset, tokenId );
	}

	for( TFinalCacheMap::iterator it = finalCaches.Begin( ); it != finalCaches.End( ); ++it )
	{
		delete it->m_second->m_tempFile;
		it->m_second->m_tempFile = nullptr;

		const String tempFilePath = absoluteBuildPath + patchName + String::Printf( TXT( "\\%s.temp.w3speech" ), it->m_first.AsChar( ) );

		// Scoped, so that file handles get destroyed to allow their physical deletion.
		{
			Red::TScopedPtr< IFile > tempFile( GFileManager->CreateFileReader( tempFilePath, FOF_AbsolutePath ) );
			if( !tempFile )
			{
				ERR_WCC( TXT( "Failed to open temporary file '%ls'!" ), tempFilePath.AsChar( ) );
			}
			else
			{
				const String outputFilePath = absoluteBuildPath + patchName + String::Printf( TXT( "\\%s.w3speech" ), it->m_first.AsChar( ) );
				Red::TScopedPtr< IFile > speechesFile( GFileManager->CreateFileWriter( outputFilePath, FOF_AbsolutePath ) );
				if( !speechesFile )
				{
					ERR_WCC( TXT( "Failed to create output cache '%ls'!" ), outputFilePath.AsChar( ) );
				}
				else
				{
					it->m_second->m_cache.Save( *speechesFile, *tempFile );
				}
			}
		}

		GFileManager->SetFileReadOnly( tempFilePath, false );
		GFileManager->DeleteFile( tempFilePath );

		delete it->m_second;
		it->m_second = nullptr;
	}

	return true;
}

//-----------------------------------------------------------------------------