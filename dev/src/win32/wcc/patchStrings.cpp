/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "patchBuilder.h"
#include "patchStrings.h"

//-----------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CPatchBuilder_Strings );

//-----------------------------------------------------------------------------

CPatchStringToken::CPatchStringToken( Uint64 hash, Uint64 crc, Uint64 size, TPatchStringCachePair* cache )
	: m_id( hash )
	, m_crc( crc )
	, m_size( size )
	, m_cache( cache )
{ }

CPatchStringToken::~CPatchStringToken( ) { }

const Uint64 CPatchStringToken::GetTokenHash( ) const
{
	return m_id;
}

const Uint64 CPatchStringToken::GetDataCRC( ) const
{
	return m_crc;
}

const Uint64 CPatchStringToken::GetDataSize( ) const
{
	return m_size;
}

const String CPatchStringToken::GetInfo( ) const
{
	const Bool detailed = false;
	if( detailed )
	{
		Bool isKey = m_id & 0x100000000 ? true : false;
		Uint32 tokenId = m_id & 0xffffffff;

		String decryptedText = String::EMPTY;
		if( isKey )
		{
			CCookedStrings::TStringId stringId = 0;
			m_cache->m_cache.GetStringIdByKey( tokenId, stringId );
			decryptedText = String::Printf( TXT( "%08X" ), stringId );
		}
		else
		{
			m_cache->m_cache.GetString( tokenId, decryptedText );
		}

		return String::Printf( TXT( "%s '%s' entry %08X : '%ls'" ),
			isKey ? TXT( "Key" ) : TXT( "String" ),
			m_cache->m_lang.AsChar( ),
			m_id & 0xffffffff,
			decryptedText.AsChar( ) );
	}

	return String::Printf( TXT( "%s '%s' entry %08X" ),
		( m_id & 0x100000000 ) ? TXT( "Key" ) : TXT( "String" ),
		m_cache->m_lang.AsChar( ),
		m_id & 0xffffffff );
}

void CPatchStringToken::DebugDump( const String& dumpPath, const Bool isBase ) const
{
	Bool isKey = m_id & 0x100000000 ? true : false;
	Uint32 tokenId = m_id & 0xffffffff;

	String absolutePath = dumpPath;
	absolutePath += String::Printf( TXT( "%08X.%s.%s.%s.txt" ),
		tokenId,
		isKey ? TXT( "key" ) : TXT( "string" ),
		m_cache->m_lang.AsChar( ),
		isBase ? TXT( "base" ) : TXT( "current" ) );

	String outString = String::EMPTY;

	if( isKey )
	{
		CCookedStrings::TStringId stringId = 0;
		m_cache->m_cache.GetStringIdByKey( tokenId, stringId );
		outString = String::Printf( TXT( "%08X - %08X" ), tokenId, stringId );
	}
	else
	{
		m_cache->m_cache.GetString( tokenId, outString );
		outString = String::Printf( TXT( "%08X - " ), tokenId ) + outString;
	}

	GFileManager->SaveStringToFileWithUTF8( absolutePath, outString );
}

//-----------------------------------------------------------------------------

CPatchStringsCache::~CPatchStringsCache( )
{
	m_caches.ClearPtr( );
	m_tokens.ClearPtr( );
}

void CPatchStringsCache::GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const
{
	outTokens.Reserve( m_tokens.Size( ) );
	for( auto* ptr : m_tokens )
	{
		outTokens.PushBack( ptr );
	}
}

const Uint64 CPatchStringsCache::GetDataSize( ) const
{
	Uint64 totalSize = 0;
	for( auto* ptr : m_tokens )
		totalSize += ptr->GetDataSize( );
	return totalSize;
}

const String CPatchStringsCache::GetInfo( ) const
{
	return TXT( "Strings" );
}

CPatchStringsCache* CPatchStringsCache::LoadContent( const String& baseDirectory )
{
	TDynArray< String > stringsCachePaths;
	GFileManager->FindFiles( baseDirectory, TXT( "*.w3strings" ), stringsCachePaths, true );

	CPatchStringsCache* patchCache = new CPatchStringsCache( );
	THashMap< Uint64, CPatchStringToken* > fileTokensMap;

	LOG_WCC( TXT( "Found %d string caches in build '%ls'."), stringsCachePaths.Size( ), baseDirectory.AsChar( ) );

	Uint32 totalEntries = 0;

	for( const String& cachePath : stringsCachePaths )
	{
		LOG_WCC( TXT( "Loading strings cache '%ls'..." ), cachePath.AsChar( ) );

		String lang = CFilePath( cachePath ).GetFileName( );
		Uint64 langHash = ( static_cast< Uint64 >( lang.CalcHash( ) ) & 0xfffffffe ) << 32;

		Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( cachePath, FOF_Buffered | FOF_AbsolutePath ) );
		if( !file )
		{
			ERR_WCC( TXT( "Failed to open cache from '%ls'!" ), cachePath.AsChar( ) );
			delete patchCache;
			return nullptr;
		}

		TPatchStringCachePair* cachePair = new TPatchStringCachePair( );
		if( !cachePair->m_cache.Load( *file ) )
		{
			ERR_WCC( TXT( "Failed to load cache contents from '%ls'!" ), cachePath.AsChar( ) );
			delete cachePair;
			delete patchCache;
			return nullptr;
		}

		cachePair->m_lang = lang;
		patchCache->m_caches.PushBack( cachePair );

		CCookedStrings& cache = cachePair->m_cache;

		totalEntries += cache.m_offsetMap.Size( ) + cache.m_keysMap.Size( );

		for( CCookedStrings::TOffsetMap::const_iterator it = cache.m_offsetMap.Begin( ); it != cache.m_offsetMap.End( ); ++it )
		{
			CCookedStrings::TStringId decryptedId = it->m_first ^ cache.m_langKey;
			Uint64 stringId = langHash | decryptedId;

			if( fileTokensMap.KeyExist( stringId ) )
				continue;

			String decryptedText;
			cache.GetString( decryptedId, decryptedText );
			Uint64 tokenCRC = decryptedText.CalcHash( );
			Uint64 tokenSize = it->m_second.length;

			CPatchStringToken* stringToken = new CPatchStringToken( stringId, tokenCRC, tokenSize, cachePair );
			patchCache->m_tokens.PushBack( stringToken );

			fileTokensMap.Insert( stringId, stringToken );
		}

		for( CCookedStrings::TKeysMap::const_iterator it = cache.m_keysMap.Begin( ); it != cache.m_keysMap.End( ); ++it )
		{
			Uint64 keyId = langHash | 0x100000000 | it->m_first;

			if( fileTokensMap.KeyExist( keyId ) )
				continue;

			CCookedStrings::TStringId decryptedId = it->m_second ^ cache.m_langKey;

			CPatchStringToken* stringToken = new CPatchStringToken( keyId, decryptedId, sizeof( decryptedId ), cachePair );
			patchCache->m_tokens.PushBack( stringToken );

			fileTokensMap.Insert( keyId, stringToken );
		}
	}

	LOG_WCC( TXT( "Strings cache summary: %d entries, %d unique ones" ), totalEntries, patchCache->m_tokens.Size( ) );

	return patchCache;
}

//-----------------------------------------------------------------------------

CPatchBuilder_Strings::CPatchBuilder_Strings( )
	: m_patchStringsCache( nullptr )
{
}

CPatchBuilder_Strings::~CPatchBuilder_Strings( )
{
	if ( m_patchStringsCache )
	{
		delete m_patchStringsCache;
		m_patchStringsCache;
	}
}

String CPatchBuilder_Strings::GetContentType( ) const
{
	return TXT( "strings" );
}

CPatchBuilder_Strings::IContentGroup* CPatchBuilder_Strings::LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )
{
	m_patchStringsCache = CPatchStringsCache::LoadContent( absoluteBuildPath );
	return m_patchStringsCache;
}

Bool CPatchBuilder_Strings::SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName )
{
	if( patchContent.Empty( ) )
	{
		ERR_WCC( TXT( "There's no string data to output!" ) );
		return false;
	}

	typedef THashMap< String, CCookedStrings* > TFinalCacheMap;
	TFinalCacheMap finalCaches;

	for( IContentToken* token : patchContent )
	{
		CPatchStringToken* stringToken = static_cast< CPatchStringToken* >( token );
		const String& lang = stringToken->m_cache->m_lang;

		CCookedStrings* cache = nullptr;
		if( !finalCaches.Find( lang, cache ) )
		{
			cache = new CCookedStrings( );
			cache->m_fileKey = stringToken->m_cache->m_cache.m_fileKey;
			cache->m_langKey = stringToken->m_cache->m_cache.m_langKey;
			finalCaches.Insert( lang, cache );
		}

		RED_ASSERT( cache->m_langKey == stringToken->m_cache->m_cache.m_langKey );

		Uint32 tokenId = stringToken->m_id & 0xffffffff;

		// If the token is a key, we also add the associated string.
		if( stringToken->m_id & 0x100000000 )
		{
			cache->AddKey( tokenId, static_cast< Uint32 >( stringToken->m_crc ) );
			tokenId = static_cast< Uint32 >( stringToken->m_crc );
		}

		// If the token is a string and it has an associated key, we also add the key.
		else
		{
			CCookedStrings::TKeyId keyId = 0;
			if( stringToken->m_cache->m_cache.GetKeyByStringId( tokenId, keyId ) )
			{
				cache->AddKey( keyId, tokenId );
			}
		}

		String decryptedString = String::EMPTY;
		stringToken->m_cache->m_cache.GetString( tokenId, decryptedString );
		cache->AddString( decryptedString, tokenId );
	}

	for( TFinalCacheMap::iterator it = finalCaches.Begin( ); it != finalCaches.End( ); ++it )
	{
		const String outputFilePath = absoluteBuildPath + patchName + String::Printf( TXT( "\\%s.w3strings" ), it->m_first.AsChar( ) );

		Red::TScopedPtr< IFile > stringsFile( GFileManager->CreateFileWriter( outputFilePath, FOF_AbsolutePath ) );
		if( !stringsFile )
		{
			ERR_WCC( TXT( "Failed to create output cache '%ls'!" ), outputFilePath.AsChar( ) );
		}
		else
		{
			it->m_second->Save( *stringsFile );
		}

		delete it->m_second;
		it->m_second = nullptr;
	}

	return true;
}

//-----------------------------------------------------------------------------