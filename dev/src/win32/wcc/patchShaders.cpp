/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "patchBuilder.h"
#include "patchShaders.h"
#include "patchUtils.h"

#include "../../common/core/hash.h"
#include "../../common/core/dependencyMapper.h"

#define SAFE_DELETE( item ) { if ( item ) { delete item; item = nullptr; } }

//-----------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CPatchBuilder_Shaders );
IMPLEMENT_ENGINE_CLASS( CPatchBuilder_StaticShaders );
IMPLEMENT_ENGINE_CLASS( CPatchBuilder_FurShaders );

//-----------------------------------------------------------------------------

CPatchMaterialToken::CPatchMaterialToken( MaterialEntry* materialEntry, IShaderCache* cache )
	: m_materialEntry( materialEntry )
{
	for ( Uint64 shaderHash : m_materialEntry->m_shaderHashes )
	{
		ShaderEntry* entry = nullptr;
		auto res = cache->GetShader( shaderHash, entry );
		if ( res == IShaderCache::eResult_Valid && entry )
		{
			m_shaderEntries.PushBack( entry );
		}
	}
}

CPatchMaterialToken::~CPatchMaterialToken()
{
}

/// Get the unique token ID (usually hash of the filename)
const Uint64 CPatchMaterialToken::GetTokenHash() const
{
	return m_materialEntry->m_hash;
}

/// Get the CRC of the data
const Uint64 CPatchMaterialToken::GetDataCRC() const
{
	return m_materialEntry->m_crc;
}

const Uint64 CPatchMaterialToken::GetAdditionalData() const
{
	return m_materialEntry->m_includesCRC;
}

/// Get the estimated data size
const Uint64 CPatchMaterialToken::GetDataSize() const
{
	Uint64 size = 0;
	for ( ShaderEntry* entry : m_shaderEntries )
	{
		size += entry->m_data.GetSize();
	}
	size += sizeof( MaterialEntry );
	size += m_shaderEntries.Size() * sizeof( ShaderEntry );
	return size;
}

const String CPatchMaterialToken::GetInfo() const
{
	return String::Printf( TXT("Material entry '%s'"), m_materialEntry->m_path.AsChar() );
}

void CPatchMaterialToken::DebugDump( const String& dumpPath, const Bool isBase ) const
{
	String absoluePath = dumpPath;
	absoluePath += m_materialEntry->m_path;
	absoluePath += isBase ? TXT(".base") : TXT(".current");

	Red::TScopedPtr< IFile > dstFile( GFileManager->CreateFileWriter( absoluePath.AsChar(), FOF_AbsolutePath ) );
	if ( dstFile )
	{
		Uint32 fileSize = (Uint32)GetDataSize();
		LOG_WCC( TXT("Dumping %d bytes to '%ls'"), fileSize, absoluePath.AsChar() );
		m_materialEntry->Serialize( *dstFile );
	}
}

//-----------------------------------------------------------------------------

CPatchShaderCache::~CPatchShaderCache()
{
	m_caches.ClearPtr();
	m_tokens.ClearPtr();
}

void CPatchShaderCache::GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const
{
	outTokens.Reserve( m_tokens.Size() );
	for ( auto* ptr : m_tokens )
	{
		outTokens.PushBack( ptr );
	}
}

const Uint64 CPatchShaderCache::GetDataSize() const
{
	Uint64 totalSize = 0;
	for ( auto* ptr : m_tokens )
		totalSize += ptr->GetDataSize();
	return totalSize;
}

const String CPatchShaderCache::GetInfo() const
{
	return TXT("Shaders");
}

/// Load all of the bundles from given directory
CPatchShaderCache* CPatchShaderCache::LoadContent( const ECookingPlatform platform, const String& baseDirectory )
{
	// enumerate shader cache files at given directory for different content directories
	TDynArray< String > shaderCachePaths;
	if ( platform == PLATFORM_PC )
	{
		GFileManager->FindFiles( baseDirectory, TXT("shader.cache"), shaderCachePaths, true );
	}
#ifndef WCC_LITE
	else if ( platform == PLATFORM_PS4 )
	{
		GFileManager->FindFiles( baseDirectory, TXT("shaderps4.cache"), shaderCachePaths, true );
	}
	else if ( platform == PLATFORM_XboxOne )
	{
		GFileManager->FindFiles( baseDirectory, TXT("shaderxboxone.cache"), shaderCachePaths, true );
	}
#endif
	else
	{
		ERR_WCC( TXT("Invalid/Unknown platform") );
	}

	// map with current tokens
	THashMap< Uint64, CPatchMaterialToken* > fileTokensMap;

	// loaded data
	CPatchShaderCache* patchCache = new CPatchShaderCache();
	patchCache->m_basePath = baseDirectory;

	Uint32 numberOfCombinedEntries = 0;

	// load the caches
	LOG_WCC( TXT("Found %d shader caches in build '%ls'"), shaderCachePaths.Size(), baseDirectory.AsChar() );
	for ( const String& cachePath : shaderCachePaths )
	{
		// load the cache
		LOG_WCC( TXT("Loading cache '%ls'..."), cachePath.AsChar() );
		IShaderCache* cache = IShaderCache::CreateReadOnly( cachePath );
		if ( !cache )
		{
			delete patchCache;
			patchCache = nullptr;
			ERR_WCC( TXT("Failed to load cache from '%ls'. The build is not valid."), cachePath.AsChar() );
			return nullptr;
		}

		// add to cache list
		patchCache->m_caches.PushBack( cache );

		// process cache entries
		TDynArray< MaterialEntry* > materialEntries;
		cache->GetMaterialEntries_Sync( materialEntries );
		const Uint32 numEntriesInCache = materialEntries.Size();
		numberOfCombinedEntries += numEntriesInCache;
		LOG_WCC( TXT("Found %d material entries in cache '%ls'"), numEntriesInCache, cachePath.AsChar() );
		for ( MaterialEntry* entry : materialEntries )
		{
			// already has a token?
			if ( fileTokensMap.KeyExist( entry->m_hash ) )
			{
				continue;
			}

			CPatchMaterialToken* materialToken = new CPatchMaterialToken( entry, cache );

			// insert into token map
			fileTokensMap.Insert( entry->m_hash, materialToken );
			patchCache->m_tokens.PushBack( materialToken );
		}
	}
		
	LOG_WCC( TXT("Overall: %d entries, %d unique ones"), numberOfCombinedEntries, patchCache->m_tokens.Size() );

	// return loaded data
	return patchCache;
}

//-----------------------------------------------------------------------------

CPatchBuilder_Shaders::CPatchBuilder_Shaders()
	: m_patchShaderCache( nullptr )
{
}

CPatchBuilder_Shaders::~CPatchBuilder_Shaders()
{
	SAFE_DELETE( m_patchShaderCache );
}

String CPatchBuilder_Shaders::GetContentType() const
{
	return TXT("shaders");
}

CPatchBuilder_Shaders::IContentGroup* CPatchBuilder_Shaders::LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )
{
	m_patchShaderCache = CPatchShaderCache::LoadContent( platform, absoluteBuildPath );
	return m_patchShaderCache;
}

Bool CPatchBuilder_Shaders::SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName )
{
	String outputFilePath = String::EMPTY;

	if ( platform == PLATFORM_PC )
	{
		outputFilePath = absoluteBuildPath + patchName + TXT("\\shader.cache");
	}
#ifndef WCC_LITE
	else if ( platform == PLATFORM_PS4 )
	{
		outputFilePath = absoluteBuildPath + patchName + TXT("\\shaderps4.cache");
	}
	else if ( platform == PLATFORM_XboxOne )
	{
		outputFilePath = absoluteBuildPath + patchName + TXT("\\shaderxboxone.cache");
	}
#endif
	else
	{
		ERR_WCC( TXT("Invalid platform, cannot save.") );
		return false;
	}

	// create writeable cache
	IShaderCache* outputCache = IShaderCache::CreateReadWrite( outputFilePath );
	if ( !outputCache )
	{
		ERR_WCC( TXT("Failed to create output cache '%ls'."), outputFilePath.AsChar() );
		return false;
	}

	// loop through entries
	for ( IContentToken* token : patchContent )
	{
		CPatchMaterialToken* materialToken = static_cast< CPatchMaterialToken* >( token );
		
		// add material entries and shaders connected with the material
		outputCache->AddMaterial( materialToken->m_materialEntry->m_hash, materialToken->m_materialEntry );
		for ( ShaderEntry* shaderEntry : materialToken->m_shaderEntries )
		{
			outputCache->AddShader( shaderEntry->m_hash, shaderEntry );
		}
	}

	// save the cache
	outputCache->Flush();

	// cleanup
	SAFE_DELETE( outputCache );

	// Done, patch data saved!
	return true;
}

//-----------------------------------------------------------------------------

namespace Helper
{
	Bool DumpReadonlyCacheToFile( IStaticShaderCache* cache, const String& path )
	{
		if ( !cache )
		{
			ERR_WCC( TXT("No cache to dump") );
			return false;
		}

		IStaticShaderCache* outputCache = IStaticShaderCache::CreateReadWrite( path );
		if ( !outputCache )
		{
			ERR_WCC( TXT("Cannot create output static shader cache '%hs'"), path.AsChar() );
			return false;
		}
		// copy entries
		TDynArray< StaticShaderEntry* > entries;
		cache->GetEntries( entries );
		for ( auto entry : entries )
		{
			outputCache->AddShader( entry->m_hash, entry );
		}

		// save the cache
		outputCache->Flush();

		// cleanup
		SAFE_DELETE( outputCache );

		return true;
	}
}


CPatchStaticShaderCacheToken::CPatchStaticShaderCacheToken( IStaticShaderCache* cache )
	: m_cache( cache )
{
	m_size = GFileManager->GetFileSize( cache->GetPath() );
	m_crc = cache->GetCRC();

	// There's only a single static shader cache in a build, so no need to worry about using hashes to differentiate them
	m_hash = 0;
}

CPatchStaticShaderCacheToken::~CPatchStaticShaderCacheToken()
{
	SAFE_DELETE( m_cache );
}

/// Get the unique token ID (usually hash of the filename)
const Uint64 CPatchStaticShaderCacheToken::GetTokenHash() const
{
	return (Uint64)m_hash;
}

/// Get the CRC of the data
const Uint64 CPatchStaticShaderCacheToken::GetDataCRC() const
{
	return m_crc;
}

/// Get the estimated data size
const Uint64 CPatchStaticShaderCacheToken::GetDataSize() const
{
	return m_size;
}

const String CPatchStaticShaderCacheToken::GetInfo() const
{
	return String::Printf( TXT("Static shader cache '%s'"), m_cache->GetPath().AsChar() );
}

void CPatchStaticShaderCacheToken::DebugDump( const String& dumpPath, const Bool isBase ) const
{
	String absoluePath = dumpPath + ( isBase ? TXT(".base") : TXT(".current") );
	Helper::DumpReadonlyCacheToFile( m_cache, absoluePath );
}

//-----------------------------------------------------------------------------

CPatchStaticShaderCache::~CPatchStaticShaderCache()
{
	SAFE_DELETE( m_token );
}

void CPatchStaticShaderCache::GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const
{
	outTokens.PushBack( m_token );
}

const Uint64 CPatchStaticShaderCache::GetDataSize() const
{
	return m_token->GetDataSize();
}

const String CPatchStaticShaderCache::GetInfo() const
{
	return TXT("Static shaders");
}

/// Load all of the bundles from given directory
CPatchStaticShaderCache* CPatchStaticShaderCache::LoadContent( const ECookingPlatform platform, const String& baseDirectory )
{
	// enumerate shader cache files at given directory for different content directories
	TDynArray< String > shaderCachePaths;

	if ( platform == PLATFORM_PC )
	{
		GFileManager->FindFiles( baseDirectory, TXT("staticshader.cache"), shaderCachePaths, true );
	}
#ifndef WCC_LITE
	else if ( platform == PLATFORM_PS4 )
	{
		GFileManager->FindFiles( baseDirectory, TXT("staticshaderps4.cache"), shaderCachePaths, true );
	}
	else if ( platform == PLATFORM_XboxOne )
	{
		GFileManager->FindFiles( baseDirectory, TXT("staticshaderxboxone.cache"), shaderCachePaths, true );
	}
#endif

	if ( shaderCachePaths.Size() != 1 )
		return nullptr;

	// loaded data
	CPatchStaticShaderCache* patchCache = new CPatchStaticShaderCache();
	patchCache->m_basePath = baseDirectory;

	// load the cache
	const String& cachePath = shaderCachePaths[0];
	LOG_WCC( TXT("Loading cache '%ls'..."), cachePath.AsChar() );
	IStaticShaderCache* cache = IStaticShaderCache::CreateReadOnly( cachePath );
	if ( !cache )
	{
		ERR_WCC( TXT("Failed to load cache from '%ls'. The build is not valid."), cachePath.AsChar() );
		return nullptr;
	}

	// process cache entries
	CPatchStaticShaderCacheToken* staticCacheToken = new CPatchStaticShaderCacheToken( cache );
	patchCache->m_token = staticCacheToken;

	// return loaded data
	return patchCache;
}

//-----------------------------------------------------------------------------

String CPatchBuilder_StaticShaders::GetContentType() const
{
	return TXT("staticshaders");
}

CPatchBuilder_StaticShaders::IContentGroup* CPatchBuilder_StaticShaders::LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )
{
	return CPatchStaticShaderCache::LoadContent( platform, absoluteBuildPath );
}

Bool CPatchBuilder_StaticShaders::SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName )
{
	if ( patchContent.Empty() )
	{
		ERR_WCC( TXT("Empty list of content to save") );
		return false;
	}
	if ( patchContent.Size() > 1 )
	{
		ERR_WCC( TXT("More than one static shader cache???? Only the first one will be saved") );
	}

	IContentToken* token = patchContent[ 0 ];
	CPatchStaticShaderCacheToken* staticShaderCacheToken = static_cast< CPatchStaticShaderCacheToken* >( token );

	if ( platform == PLATFORM_PC )
	{
		const String outputFilePath = absoluteBuildPath + TXT("content\\content0\\staticshader.cache");
		return Helper::DumpReadonlyCacheToFile( staticShaderCacheToken->m_cache, outputFilePath );
	}
#ifndef WCC_LITE
	else if ( platform == PLATFORM_PS4 )
	{
		const String outputFilePath = absoluteBuildPath + TXT("content\\content0\\staticshaderps4.cache");
		return Helper::DumpReadonlyCacheToFile( staticShaderCacheToken->m_cache, outputFilePath );
	}
	else if ( platform == PLATFORM_XboxOne )
	{
		const String outputFilePath = absoluteBuildPath + TXT("content\\content0\\staticshaderxboxone.cache");
		return Helper::DumpReadonlyCacheToFile( staticShaderCacheToken->m_cache, outputFilePath );
	}
#endif
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------


CPatchFurShaderCacheToken::CPatchFurShaderCacheToken( IFile* cacheFile, const String& absolutePath )
	: m_cacheFile( cacheFile )
	, m_absolutePath( absolutePath )
{
	RED_FATAL_ASSERT( m_cacheFile != nullptr, "Must have a cache file" );

	m_size = m_cacheFile->GetSize();

	m_crc = PatchUtils::CalcFileBlockCRC( m_cacheFile, m_cacheFile->GetSize(), RED_FNV_OFFSET_BASIS64 );

	// There's only a single fur shader cache in a build, so no need to worry about using hashes to differentiate them
	m_hash = 0;
}

CPatchFurShaderCacheToken::~CPatchFurShaderCacheToken()
{
	delete m_cacheFile;
}

/// Get the unique token ID (usually hash of the filename)
const Uint64 CPatchFurShaderCacheToken::GetTokenHash() const
{
	return (Uint64)m_hash;
}

/// Get the CRC of the data
const Uint64 CPatchFurShaderCacheToken::GetDataCRC() const
{
	return m_crc;
}

/// Get the estimated data size
const Uint64 CPatchFurShaderCacheToken::GetDataSize() const
{
	return m_size;
}

const String CPatchFurShaderCacheToken::GetInfo() const
{
	return String::Printf( TXT("Fur shader cache '%s'"), m_absolutePath.AsChar() );
}

void CPatchFurShaderCacheToken::DebugDump( const String& dumpPath, const Bool isBase ) const
{
	String absoluteDumpPath = dumpPath + ( isBase ? TXT(".base") : TXT(".current") );
	PatchUtils::CopyFileContent( *m_cacheFile, absoluteDumpPath );
}

//-----------------------------------------------------------------------------

CPatchFurShaderCache::CPatchFurShaderCache()
	: m_token( nullptr )
{
}

CPatchFurShaderCache::~CPatchFurShaderCache()
{
	SAFE_DELETE( m_token );
}

void CPatchFurShaderCache::GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const
{
	if ( m_token != nullptr )
	{
		outTokens.PushBack( m_token );
	}
}

const Uint64 CPatchFurShaderCache::GetDataSize() const
{
	return m_token != nullptr ? m_token->GetDataSize() : 0;
}

const String CPatchFurShaderCache::GetInfo() const
{
	return TXT("Fur shaders");
}

/// Load all of the bundles from given directory
CPatchFurShaderCache* CPatchFurShaderCache::LoadContent( const ECookingPlatform platform, const String& baseDirectory )
{
	// enumerate shader cache files at given directory for different content directories
	TDynArray< String > shaderCachePaths;

	if ( platform == PLATFORM_PC )
	{
		GFileManager->FindFiles( baseDirectory, TXT("furshader.cache"), shaderCachePaths, true );
	}

	// If there are no paths returned, then we're patching a new fur cache onto a build that didn't have one. Still need
	// a CPatchFurShaderCache so that the patch doesn't fail, but it will have no token.

	CPatchFurShaderCache* patchCache = new CPatchFurShaderCache();
	patchCache->m_basePath = baseDirectory;

	if ( shaderCachePaths.Size() > 0 )
	{
		if ( shaderCachePaths.Size() > 1 )
		{
			ERR_WCC( TXT("More than one fur shader cache???? Only the first one will be saved") );
		}

		// load the cache
		const String& cachePath = shaderCachePaths[0];
		IFile* cacheFile = GFileManager->CreateFileReader( cachePath, FOF_AbsolutePath );
		if ( cacheFile == nullptr )
		{
			ERR_WCC( TXT("Failed to load cache from '%ls'. The build is not valid."), cachePath.AsChar() );
			return nullptr;
		}

		// process cache entries
		CPatchFurShaderCacheToken* furCacheToken = new CPatchFurShaderCacheToken( cacheFile, cachePath );
		patchCache->m_token = furCacheToken;
	}

	// return loaded data
	return patchCache;
}

//-----------------------------------------------------------------------------

String CPatchBuilder_FurShaders::GetContentType() const
{
	return TXT("furshaders");
}

CPatchBuilder_FurShaders::IContentGroup* CPatchBuilder_FurShaders::LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )
{
	return CPatchFurShaderCache::LoadContent( platform, absoluteBuildPath );
}

Bool CPatchBuilder_FurShaders::SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName )
{
	if ( patchContent.Empty() )
	{
		ERR_WCC( TXT("Empty list of content to save") );
		return false;
	}
	if ( patchContent.Size() > 1 )
	{
		ERR_WCC( TXT("More than one fur shader cache???? Only the first one will be saved") );
	}

	IContentToken* token = patchContent[ 0 ];
	CPatchFurShaderCacheToken* staticShaderCacheToken = static_cast< CPatchFurShaderCacheToken* >( token );

	if ( platform == PLATFORM_PC )
	{
		const String outputFilePath = absoluteBuildPath + TXT("content\\content0\\furshader.cache");
		return PatchUtils::CopyFileContent( *staticShaderCacheToken->m_cacheFile, outputFilePath );
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------