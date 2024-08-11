#include "build.h"
#include "staticShaderCache.h"

//////////////////////////////////////////////////////////////////////////

IStaticShaderCache* IStaticShaderCache::CreateReadOnly( const String& absolutePath )
{
	IStaticShaderCache* cache = new CStaticShaderCacheReadonly();
	if ( cache->Initialize( absolutePath ) )
	{
		return cache;
	}

	// failed to open the cache
	delete cache;
	return nullptr;
}

IStaticShaderCache* IStaticShaderCache::CreateReadWrite( const String& absolutePath )
{
	IStaticShaderCache* cache = new CStaticShaderCacheReadWrite();
	if ( cache->Initialize( absolutePath ) )
	{
		return cache;
	}

	// failed to open the cache
	delete cache;
	return nullptr;
}

IStaticShaderCache* IStaticShaderCache::CreateNull()
{
	return new CStaticShaderCacheNull();
}

//////////////////////////////////////////////////////////////////////////

CStaticShaderCacheReadWrite::CStaticShaderCacheReadWrite()
	: m_dirty( false )
	, m_includesMatch( false )
	, m_shadersMatch( false )
{
}

CStaticShaderCacheReadWrite::~CStaticShaderCacheReadWrite()
{
	CScopedLock lock( m_mutex );

	m_entries.Clear();
}

void CStaticShaderCacheReadWrite::InvalidateStaticShaders()
{
	CScopedLock lock( m_mutex );

	extern Uint64 CalculateDirectoryCRC( const String& path );
	Uint64 currentShadersCRC = CalculateDirectoryCRC( GpuApi::GetShaderRootPath() );
	m_shadersMatch = currentShadersCRC == m_info.m_shadersCRC;
	if ( !m_shadersMatch )
	{
		// in case of out-of-date bin/shaders we don't invalidate the cashe, because we will be checking entries one by one
		RED_LOG( Shaders, TXT("StaticShaderCache '%ls': shaders folder out-of-date (loaded %") RED_PRIWu64 TXT(", actual: %") RED_PRIWu64 TXT(")") , m_absoluteFilePath.AsChar(), m_info.m_shadersCRC, currentShadersCRC );
	}
}

Bool CStaticShaderCacheReadWrite::GetShader( const Uint64 hash, StaticShaderEntry*& entry )
{
	CScopedLock lock( m_mutex );

	return m_entries.Find( hash, entry );
}

Bool CStaticShaderCacheReadWrite::AddShader( const Uint64 hash, StaticShaderEntry* entry )
{
	CScopedLock lock( m_mutex );

	if ( m_entries.Insert( hash, entry ) )
	{
		m_dirty = true;
		return true;
	}

	StaticShaderEntry* existingEntry = nullptr;
	RED_VERIFY( GetShader( hash, existingEntry ) );
	if ( existingEntry->m_contentCRC != entry->m_contentCRC )
	{
		RED_VERIFY( m_entries.Set( hash, entry ) );
		m_dirty = true;
		return true;
	}

	//RED_LOG( Shaders, TXT("ShaderEntry with hash %") RED_PRIWu64 TXT(" already exists in the cache!"), hash );
	return false;
}

void CStaticShaderCacheReadWrite::Flush()
{
	CScopedLock lock( m_mutex );

	if ( !m_dirty )
	{
		RED_LOG( Shaders, TXT( "StaticShaderCache is up-to-date, no need for saving" ) );
		return;
	}

#ifndef RED_PLATFORM_CONSOLE
	//////////////////////////////////////////////////////////////////////////
	// if we reached this point, that means that the cache file doesn't contain something that it should
	// this situation can happen when there are local shader changes or if there is some other mismatch with the shader cache
	// since we want to keep this file in perforce, we have to remove the readonly flag
	if ( GSystemIO.FileExist( m_absoluteFilePath.AsChar() ) )
	{
		VERIFY( GSystemIO.SetFileReadOnly( m_absoluteFilePath.AsChar(), false ) );
	}
#endif // !RED_PLATFORM_CONSOLE

	Red::TScopedPtr< IFile > file( GFileManager->CreateFileWriter( m_absoluteFilePath.AsChar(), FOF_AbsolutePath | FOF_Buffered ) );
	if ( !file )
	{
		RED_LOG_ERROR( Shaders, TXT("StaticShaderCache: cannot open '%ls' for saving." ), m_absoluteFilePath.AsChar() );
		return;
	}

	CTimeCounter timer;

	m_info.m_magic = SHADER_CACHE_MAGIC;
	m_info.m_version = SHADER_CACHE_VERSION_CURRENT;
	file->m_version = SHADER_CACHE_VERSION_CURRENT;

	extern Uint64 CalculateDirectoryCRC( const String& path );
	m_info.m_includesCRC = CalculateDirectoryCRC( GpuApi::GetShaderIncludePath() );
	m_info.m_shadersCRC = CalculateDirectoryCRC( GpuApi::GetShaderRootPath() );

	m_info.m_numEntries = m_entries.Size();
	for ( auto entryPair : m_entries )
	{
		entryPair.m_second->Serialize( *file );
	}
	m_info.Serialize( *file );

	m_includesMatch = true;
	m_shadersMatch = true;
	m_dirty = false;

	RED_LOG( Shaders, TXT("StaticShaderCache: saved %u entries, took %1.2fms" ), m_info.m_numEntries, timer.GetTimePeriodMS() );
	RED_LOG( Shaders, TXT("    includesCRC: %") RED_PRIWu64 TXT(", shadersCRC: %") RED_PRIWu64, m_info.m_includesCRC, m_info.m_shadersCRC );
}

Bool CStaticShaderCacheReadWrite::ValidateCache_NoLock()
{
	if ( m_info.m_magic != SHADER_CACHE_MAGIC )
	{
		RED_LOG_ERROR( Shaders, TXT( "StaticShaderCache '%ls': magic mismatch (loaded: '%u', expected: '%u')" ) , m_absoluteFilePath.AsChar(), m_info.m_magic, SHADER_CACHE_MAGIC );
		return false;
	}
	if ( m_info.m_version != SHADER_CACHE_VERSION_CURRENT )
	{
		RED_LOG_WARNING( Shaders, TXT("StaticShaderCache '%ls': version mismatch (loaded: '%u', expected: '%u')") , m_absoluteFilePath.AsChar(), m_info.m_version, SHADER_CACHE_VERSION_CURRENT );
	}
	if ( m_info.m_numEntries <= 0 )
	{
		RED_LOG_ERROR( Shaders, TXT("StaticShaderCache '%ls': no entries (loaded: '%u')") , m_absoluteFilePath.AsChar(), m_info.m_numEntries );
		return false;
	}

	extern Uint64 CalculateDirectoryCRC( const String& path );
	Uint64 currentIncludesCRC = CalculateDirectoryCRC( GpuApi::GetShaderIncludePath() );
	m_includesMatch = currentIncludesCRC == m_info.m_includesCRC;
	if ( !m_includesMatch )
	{
		// in case of out-of-date includes invalidate the whole cache, because the includes folder has impact on everything
		RED_LOG_WARNING( Shaders, TXT("StaticShaderCache '%ls': includes out-of-date (loaded %") RED_PRIWu64 TXT(", actual: %") RED_PRIWu64 TXT(")") , m_absoluteFilePath.AsChar(), m_info.m_includesCRC, currentIncludesCRC );
		return false;
	}
	
	InvalidateStaticShaders();

	return true;
}

Bool CStaticShaderCacheReadWrite::Initialize( const String& absoluteFilePath )
{
	CScopedLock lock( m_mutex );

	m_absoluteFilePath = absoluteFilePath;
	m_dirty = true;

	Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( absoluteFilePath.AsChar(), FOF_AbsolutePath | FOF_Buffered ) );
	if ( file )
	{
		const Uint64 size = file->GetSize();
		const Uint64 infoSize = sizeof( StaticShaderCacheFileInfo );
		if ( size >= infoSize )
		{
			file->Seek( size - infoSize );
			m_info.Serialize( *file );

			file->m_version = m_info.m_version;

			m_dirty = !ValidateCache_NoLock();
			if ( !m_dirty )
			{
				file->Seek( 0 );
				for ( Uint32 i = 0; i < m_info.m_numEntries; ++i )
				{
					StaticShaderEntry* entry = new StaticShaderEntry();
					entry->Serialize( *file );
					m_entries.Insert( entry->m_hash, entry );
				}
			}
		}
		else
		{
			RED_LOG_ERROR( Shaders, TXT("StaticShaderCache '%ls': size invalid (loaded: %") RED_PRIWu64 TXT(", expected more than: %") RED_PRIWu64 TXT(")"), absoluteFilePath.AsChar(), size, infoSize );
		}
	}
	else
	{
		if ( GIsCooker )
		{
			RED_LOG( Shaders, TXT( "StaticShaderCache: could not find '%ls', will create a new one." ), absoluteFilePath.AsChar() );
		}
		else
		{
			RED_LOG_ERROR( Shaders, TXT("StaticShaderCache: could not find '%ls'" ), absoluteFilePath.AsChar() );
		}
	}

	return true;
}

void CStaticShaderCacheReadWrite::GetEntries( TDynArray< StaticShaderEntry* >& entries ) const
{
	entries.Reserve( m_entries.Size() );
	for ( auto entry : m_entries )
	{
		entries.PushBack( entry.m_second );
	}
}

//////////////////////////////////////////////////////////////////////////

CStaticShaderCacheReadonly::CStaticShaderCacheReadonly()
{
}

CStaticShaderCacheReadonly::~CStaticShaderCacheReadonly()
{
	m_entries.Clear();
}

Bool CStaticShaderCacheReadonly::GetShader( const Uint64 hash, StaticShaderEntry*& entry )
{
	return m_entries.Find( hash, entry );
}

Bool CStaticShaderCacheReadonly::AddShader( const Uint64 hash, StaticShaderEntry* entry )
{
	RED_HALT( "Trying to add StaticShaderEntry to readonly cache." );
	return false;
}

void CStaticShaderCacheReadonly::Flush()
{
	RED_HALT( "Trying to Flush readonly StaticShaderCache." );
}

Bool CStaticShaderCacheReadonly::Initialize( const String& absoluteFilePath )
{
	m_absoluteFilePath = absoluteFilePath;

	Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( absoluteFilePath.AsChar(), FOF_AbsolutePath | FOF_Buffered ) );
	if ( !file )
	{
		RED_LOG_ERROR( Shaders, TXT("StaticShaderCache: could not find '%ls'" ), absoluteFilePath.AsChar() );
		return false;
	}

	const Uint64 size = file->GetSize();
	const Uint64 infoSize = sizeof( StaticShaderCacheFileInfo );
	if ( size < infoSize )
	{
		RED_LOG_ERROR( Shaders, TXT("StaticShaderCache '%ls': size invalid (loaded: %") RED_PRIWu64 TXT(", expected more than: %") RED_PRIWu64 TXT(")"), absoluteFilePath.AsChar(), size, infoSize );
		return false;
	}

	file->Seek( size - infoSize );
	m_info.Serialize( *file );
	file->m_version = m_info.m_version;

	if ( m_info.m_version != SHADER_CACHE_VERSION_CURRENT )
	{
		RED_LOG_WARNING( Shaders, TXT("StaticShaderCache '%ls': version mismatch (loaded: '%u', expected: '%u')") , absoluteFilePath.AsChar(), m_info.m_version, SHADER_CACHE_VERSION_CURRENT );
	}

	if ( m_info.m_magic != SHADER_CACHE_MAGIC )
	{
		RED_LOG_ERROR( Shaders, TXT("StaticShaderCache '%ls': magic mismatch (loaded: '%u', expected: '%u'), numEntries: %u") , absoluteFilePath.AsChar(), m_info.m_magic, SHADER_CACHE_MAGIC, m_info.m_numEntries );
		return false;
	}

	file->Seek( 0 );
	for ( Uint32 i = 0; i < m_info.m_numEntries; ++i )
	{
		StaticShaderEntry* entry = new StaticShaderEntry();
		entry->Serialize( *file );
		m_entries.Insert( entry->m_hash, entry );
	}

	return true;
}

void CStaticShaderCacheReadonly::GetEntries( TDynArray< StaticShaderEntry* >& entries ) const
{
	entries.Reserve( m_entries.Size() );
	for ( auto entry : m_entries )
	{
		entries.PushBack( entry.m_second );
	}
}


