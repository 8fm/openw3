#include "build.h"

#include "shaderCacheManager.h"

#include "renderer.h"
#include "shaderCache.h"
#include "staticShaderCache.h"

#include "../core/contentManifest.h"
#include "../core/cacheFilenames.h"

CShaderCacheResolver::CShaderCacheResolver()
	: m_staticShaderCache( nullptr )
	, m_writableShaderCache( nullptr )
	, m_includesCRC( 0 )
	, m_isReadonly( true )
	, m_enableSaving( false )
{
}

CShaderCacheResolver::~CShaderCacheResolver()
{
}

Bool CShaderCacheResolver::Init()
{
	if (m_staticShaderCache)
		return true;

	CacheDirectoriesCRC();

	const Bool forceWrite = ( nullptr != Red::System::StringSearch( SGetCommandLine(), TXT("-forceshadercachewrite") ) );
	if ( forceWrite || !GFileManager->IsReadOnly() )
	{
		LOG_CORE(TXT("CShaderCacheResolver: enabling saving"));
		m_isReadonly = false;
		m_enableSaving = true;
	}

#if defined( RED_PLATFORM_DURANGO ) && !defined( RED_FINAL_BUILD )
	if ( !IsReadonly() )
	{
		InitXboxOneWritableShaderCaches();
	}
#endif

	// Durango might use the temp on D: instead
	if ( !m_staticShaderCache )
	{
		const String staticShaderCacheFile = GFileManager->GetDataDirectory() + STATIC_SHADER_CACHE_FILENAME;
		LOG_ENGINE(TXT("Using static shader cache from '%ls'"), staticShaderCacheFile.AsChar() );
		m_staticShaderCache = IsReadonly() ? IStaticShaderCache::CreateReadOnly( staticShaderCacheFile ) : IStaticShaderCache::CreateReadWrite( staticShaderCacheFile );
	}

	RED_FATAL_ASSERT( m_staticShaderCache, "No static shader cache" );

	return true;
}

#if defined( RED_PLATFORM_DURANGO ) && !defined( RED_FINAL_BUILD )
void CShaderCacheResolver::InitXboxOneWritableShaderCaches()
{
	const String staticShaderCacheFile = String::Printf( TXT("d:\\%ls"), STATIC_SHADER_CACHE_FILENAME );
	m_staticShaderCache = IStaticShaderCache::CreateReadWrite( staticShaderCacheFile );
	RED_FATAL_ASSERT( m_staticShaderCache, "CShaderCacheResolver::InitXboxOneWritableShaderCaches: failed to create static shader cache on '%ls'", staticShaderCacheFile.AsChar() );
	LOG_ENGINE(TXT("CShaderCacheResolver::InitXboxOneWritableShaderCaches: Using temp static shader cache '%ls'"), staticShaderCacheFile.AsChar() );

	const String shaderCacheFile = String::Printf( TXT("d:\\%ls"), SHADER_CACHE_FILENAME );
	AttachCache( shaderCacheFile, eAttachPolicy_Back );
	InitWritableShaderCache();
}
#endif

void CShaderCacheResolver::InitWritableShaderCache()
{
	RED_FATAL_ASSERT( !m_writableShaderCache, "Already have a writable shader cache!" );
	if ( m_shaderCacheChain.Begin() != m_shaderCacheChain.End() )
	{
		m_writableShaderCache = *m_shaderCacheChain.Begin();
	}
	else
	{
		WARN_ENGINE(TXT("CShaderCacheResolver: No writable shader cache available!"));
	}
}

void CShaderCacheResolver::Shutdown()
{
	for ( IShaderCache* shaderCache : m_shaderCacheChain )
	{
		delete shaderCache;
	}

	delete m_staticShaderCache;
	m_staticShaderCache = nullptr;

	m_writableShaderCache = nullptr;
}

void CShaderCacheResolver::OnContentAvailable( const SContentInfo& contentInfo )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Mounting off the main thread" );

	// Ignore the static shader cache. It needs to be set sooner than content is normally attached.
	for ( const SContentFile* contentFile : contentInfo.m_contentFiles )
	{
		const EAttachPolicy policy = contentFile->m_isPatch ? eAttachPolicy_Front : eAttachPolicy_Back;
		const String shaderCacheFile = String::Printf( TXT("%ls%hs"), contentInfo.m_mountPath.AsChar(), contentFile->m_path.AsChar() );

		if ( contentFile->m_path.ContainsSubstring( "furshader.cache" ) )
		{
			AttachFurShaderCache( shaderCacheFile );
		}
		else
		{
			AttachCache( shaderCacheFile, policy );
		}
	}

	// Hacks for runtime material compilation
	// Set it up here and now while the base engine is still initializing
	// Durango can use its hacked shader caches on D:\\ that were already set up
#ifndef RED_PLATFORM_CONSOLE
	static Bool DoOnce = false;
	if ( !DoOnce )
	{
		DoOnce = true;
		InitWritableShaderCache();
	}
#endif
}

void CShaderCacheResolver::AttachCache( const String& shaderCacheFile, EAttachPolicy policy )
{
	typedef IShaderCache* (*CreateFunc)(const String&);
	const CreateFunc CreateShaderCache = IsReadonly() ? &IShaderCache::CreateReadOnly : &IShaderCache::CreateReadWrite;

	typedef Bool (CacheChain::* PushFunc)(IShaderCache*);
	const PushFunc Push = ( policy == eAttachPolicy_Front ) ? &CacheChain::PushFront : &CacheChain::PushBack;

	LOG_ENGINE(TXT("CShaderCacheResolver: attaching '%ls'"), shaderCacheFile.AsChar() );

	IShaderCache* cache = CreateShaderCache( shaderCacheFile );
	if ( cache )
	{
		cache->SetIsFromPatch( policy == eAttachPolicy_Front );
		if ( ! (m_shaderCacheChain.*Push)( cache ) )
		{
			delete cache;
			ERR_ENGINE(TXT("Failed to create shader cache '%ls'. Reached chain length limit %u"), shaderCacheFile.AsChar(), MAX_CACHE_CHAIN_LENGTH );
			return;
		}

		if ( cache->IsFromPatch() )
		{
			// attaching patch cache
			for ( IShaderCache* contentCache : m_shaderCacheChain )
			{
				if ( !contentCache->IsFromPatch() )
				{
					EP2_RemoveDuplicates( cache, contentCache );
				}
			}
		}
		else
		{
			// attaching content cache
			for ( IShaderCache* patchCache : m_shaderCacheChain )
			{
				if ( patchCache->IsFromPatch() )
				{
					EP2_RemoveDuplicates( patchCache, cache );
				}
			}

		}
		
	}
	else
	{
		ERR_ENGINE( TXT("Failed to create shader cache '%ls'"), shaderCacheFile.AsChar() );
	}
}



void CShaderCacheResolver::EP2_RemoveDuplicates( const IShaderCache* patchCache, IShaderCache* contentCache )
{
	RED_FATAL_ASSERT( patchCache != contentCache, "" );
	if ( patchCache == contentCache )
	{
		// do not remove from itself
		return;
	}
	RED_FATAL_ASSERT( patchCache->IsFromPatch(), "" );
	RED_FATAL_ASSERT( !contentCache->IsFromPatch(), "" );

	TShaderEntryUsageCounter usageCounter;
	{
		// counting pass
		CTimeCounter timer;			
		contentCache->GetShaderEntriesUsageCount( usageCounter );
		LOG_ENGINE( TXT("Counting took: %1.3f ms"), timer.GetTimePeriodMS() );
	}
		
	{
		// decreasing counters phase
		CTimeCounter timer;
		TDynArray< Uint64 > materialEntryHashes;
		patchCache->GetMaterialEntriesHashes_Sync( materialEntryHashes );
		for ( Uint64 materialEntryHash : materialEntryHashes )
		{
			contentCache->DecreaseCountersForMaterialEntry( materialEntryHash, usageCounter );
		}
		LOG_ENGINE( TXT("Decreasing counters took: %1.3f ms"), timer.GetTimePeriodMS() );
	}
		
	{
		// unloading phase
		CTimeCounter timer;
		contentCache->UnloadUnusedShaderEntries( usageCounter );
		LOG_ENGINE( TXT("Unloading took: %1.3f ms"), timer.GetTimePeriodMS() );
	}
}

void CShaderCacheResolver::AttachFurShaderCache( const String& shaderCacheFile )
{
	// TODO : Maybe if GRender (or GRenderThread or something) is created, load data here and send it off in a render command?
	if ( GRender != nullptr )
	{
		WARN_ENGINE( TXT("Attaching fur shader cache from %ls, but GRender is already created. This cache file will not be used!"), shaderCacheFile.AsChar() );
	}

	m_furShaderCaches.PushBack( shaderCacheFile );
}


void CShaderCacheResolver::GetAllFurShaderCaches( TDynArray< String >& absolutePaths ) const
{
	absolutePaths.PushBack( m_furShaderCaches );
}



void CShaderCacheResolver::CacheDirectoriesCRC()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only" );

	m_includesCRC = CalculateDirectoryCRC( GpuApi::GetShaderIncludePath() );
}

Bool CShaderCacheResolver::HasShader( Uint64 hash ) const
{
	RED_FATAL_ASSERT( m_staticShaderCache, "No static shader cache!");
	StaticShaderEntry* staticEntry = nullptr;
	if ( m_staticShaderCache->GetShader( hash, staticEntry ) )
	{
		return true;
	}

	ShaderEntry* entry = nullptr;
	for ( IShaderCache* shaderCache : m_shaderCacheChain )
	{
		if ( shaderCache->GetShader( hash, entry ) == IShaderCache::eResult_Valid )
		{
			return true;
		}
	}

	// not found or loading of one of the cache files is in progress
	return false;
}

Bool CShaderCacheResolver::GetStaticShader( Uint64 hash, StaticShaderEntry*& entry )
{
	RED_FATAL_ASSERT( m_staticShaderCache, "No static shader cache!" );
	return m_staticShaderCache->GetShader( hash, entry );
}

Bool CShaderCacheResolver::AddStaticShader( Uint64 hash, StaticShaderEntry* entry )
{
	RED_FATAL_ASSERT( m_staticShaderCache, "No static shader cache!" );
	return m_staticShaderCache->AddShader( hash, entry );
}

Bool CShaderCacheResolver::AddStaticShader( Uint64 hash, Uint64 contentCRC, const DataBuffer& shaderData )
{
	StaticShaderEntry* entry = new StaticShaderEntry();
	entry->m_hash = hash;
	entry->m_contentCRC = contentCRC;
	entry->m_data = shaderData;
	return AddStaticShader( hash, entry );
}

// returns
// eResult_Valid when entry is found
// eResult_Pending if not found in any loaded caches and one or more are still loading
// eResult_Invalid when not found in any of the caches
IShaderCache::EResult CShaderCacheResolver::GetShader( Uint64 hash, ShaderEntry*& entry )
{
	IShaderCache::EResult result = IShaderCache::eResult_Invalid;
	for ( IShaderCache* shaderCache : m_shaderCacheChain )
	{
		result = shaderCache->GetShader( hash, entry );
		if ( result != IShaderCache::eResult_Invalid )
		{
			break;
		}
	}
	return result;
}

IShaderCache::EResult CShaderCacheResolver::GetShaderData( Uint64 hash, DataBuffer& data )
{
	ShaderEntry* entry = nullptr;
	IShaderCache::EResult result = IShaderCache::eResult_Invalid;
	for ( IShaderCache* shaderCache : m_shaderCacheChain )
	{
		result = shaderCache->GetShader( hash, entry );
		if ( result != IShaderCache::eResult_Invalid )
		{
			break;
		}
	}

	// decompress
	if ( result == IShaderCache::eResult_Valid )
	{
		if ( !entry->DecompressToBuffer( data ) )
		{
			result = IShaderCache::eResult_Invalid;
		}
	}
	return result;
}

IShaderCache::EResult CShaderCacheResolver::AddShader( Uint64 hash, ShaderEntry* entry )
{
	RED_ASSERT( m_writableShaderCache, TXT( "AddShader: No writable shader cache!" ) );
	return m_writableShaderCache ? m_writableShaderCache->AddShader( hash, entry ) : IShaderCache::eResult_Invalid;
}

IShaderCache::EResult CShaderCacheResolver::AddShader( Uint64 hash, const DataBuffer& shaderData )
{
	ShaderEntry* entry = new ShaderEntry();
	entry->m_hash = hash;
	entry->SetData( shaderData );
	return AddShader( hash, entry );
}

// returns
// eResult_Valid when entry is found
// eResult_Pending if not found in any loaded caches and one or more are still loading
// eResult_Invalid when not found in any of the caches
IShaderCache::EResult CShaderCacheResolver::GetMaterial( Uint64 hash, MaterialEntry*& entry )
{
	IShaderCache::EResult result = IShaderCache::eResult_Invalid;
	for ( IShaderCache* shaderCache : m_shaderCacheChain )
	{
		result = shaderCache->GetMaterial( hash, entry );
		if ( result != IShaderCache::eResult_Invalid )
		{
			break;
		}
	}

	return result;
}

IShaderCache::EResult CShaderCacheResolver::AddMaterial( Uint64 hash, MaterialEntry* entry )
{
	RED_ASSERT( m_writableShaderCache, TXT( "AddMaterial: No writable shader cache!" ) );
	return m_writableShaderCache ? m_writableShaderCache->AddMaterial( hash, entry ) : IShaderCache::eResult_Invalid;
}

void CShaderCacheResolver::Flush()
{
	if ( !m_enableSaving )
	{
		// even if this is a read-write cache, don't save
		return;
	}

	// flush static caches
	RED_FATAL_ASSERT( m_staticShaderCache, "No static shader cache!" );
	m_staticShaderCache->Flush();

	// flush material caches
	for ( IShaderCache* shaderCache : m_shaderCacheChain )
	{
		shaderCache->Flush();
	}
}

// Really only called at well defined points, so not a synch issue
void CShaderCacheResolver::EnableSaving( Bool enableSaving )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only. m_enableSaving not synchronized" );

	if ( m_isReadonly )
	{
		// cannot modify readonly cache
		return;
	}

	m_enableSaving = enableSaving;
}

void CShaderCacheResolver::GetAllMaterials_Sync( TDynArray< MaterialEntry* >& entries ) const
{
	for ( const IShaderCache* shaderCache : m_shaderCacheChain )
	{
		shaderCache->GetMaterialEntries_Sync( entries );
	}
}

void CShaderCacheResolver::RemoveMaterial( Uint64 hash )
{
	for ( IShaderCache* shaderCache : m_shaderCacheChain )
	{
		shaderCache->RemoveMaterial( hash );
	}
}

static CShaderCacheResolver GShaderCacheResolver;
CShaderCacheResolver* GShaderCache = &GShaderCacheResolver;
