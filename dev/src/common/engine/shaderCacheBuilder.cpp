
#include "build.h"
#include "shaderCacheBuilder.h"

CShaderCacheReadWrite::CShaderCacheReadWrite()
	: m_dirty( false )
{
}

CShaderCacheReadWrite::~CShaderCacheReadWrite()
{
	CScopedLock lock( m_mutex );

	m_shaderEntries.Clear();
	m_materialEntries.Clear();
}

//! Methods below can return eResult_Pending if the data is not yet available
IShaderCache::EResult CShaderCacheReadWrite::GetShader( const Uint64 hash, ShaderEntry*& entry )
{
	CScopedLock lock( m_mutex );

	if ( m_shaderEntries.Find( hash, entry ) )
	{
		return IShaderCache::eResult_Valid;
	}
	return IShaderCache::eResult_Invalid;
}

IShaderCache::EResult CShaderCacheReadWrite::GetMaterial( const Uint64 hash, MaterialEntry*& entry )
{
	CScopedLock lock( m_mutex );

	if ( m_materialEntries.Find( hash, entry ) )
	{
		return IShaderCache::eResult_Valid;
	}
	return IShaderCache::eResult_Invalid;
}

//! Methods below can return eResult_Pending if the data is not yet available
IShaderCache::EResult CShaderCacheReadWrite::AddShader( const Uint64 hash, ShaderEntry* entry )
{
	CScopedLock lock( m_mutex );

	if ( m_shaderEntries.Insert( hash, entry ) )
	{
		m_dirty = true;
		return IShaderCache::eResult_Valid;
	}

	//RED_LOG( Shaders, TXT("ShaderEntry with hash %") RED_PRIWu64 TXT(" already exists in the cache!"), hash );
	return IShaderCache::eResult_Invalid;
}

IShaderCache::EResult CShaderCacheReadWrite::AddMaterial( const Uint64 hash, MaterialEntry* entry )
{
	CScopedLock lock( m_mutex );

	if ( m_materialEntries.Insert( hash, entry ) )
	{
		m_dirty = true;
		return IShaderCache::eResult_Valid;
	}

	//RED_LOG( Shaders, TXT("MaterialEntry with hash %") RED_PRIWu64 TXT(" already exists in the cache!"), hash );
	return IShaderCache::eResult_Invalid;
}

IShaderCache::EResult CShaderCacheReadWrite::RemoveMaterial( const Uint64 hash )
{
	CScopedLock lock( m_mutex );

	if ( m_materialEntries.Erase( hash ) )
	{
		RED_LOG( Shaders, TXT( "Removed material: [%") RED_PRIWu64 TXT("]" ), hash );
		m_dirty = true;
		return IShaderCache::eResult_Valid;
	}
	else
	{
		RED_LOG( Shaders, TXT( "Could not find material to remove: [%") RED_PRIWu64 TXT("]" ), hash );
	}

	return IShaderCache::eResult_Invalid;
}

//! Flush dirty cache content to file, this only works for a cache that is not read only and has a file
void CShaderCacheReadWrite::Flush()
{
	CScopedLock lock( m_mutex );

	// save synchronously
	if ( !m_dirty )
	{
		RED_LOG( Shaders, TXT( "MaterialCache is up-to-date, no need for saving" ) );
		return;
	}

	String tempFileName = m_absoluteFilePath;

#ifndef RED_PLATFORM_CONSOLE
	//////////////////////////////////////////////////////////////////////////
	// if we reached this point, that means that the cache file doesn't contain something that it should
	// this situation can happen when there are local shader changes or if there is some other mismatch with the shader cache
	// since we want to keep this file in perforce, we have to remove the readonly flag
	if ( GSystemIO.FileExist( m_absoluteFilePath.AsChar() ) )
	{
		VERIFY( GSystemIO.SetFileReadOnly( m_absoluteFilePath.AsChar(), false ) );
	}

	// use temp file on PC to avoid data loss in case of crash
	tempFileName = m_absoluteFilePath + TXT("_temp.cache");
#endif // !RED_PLATFORM_CONSOLE

	IFile* file = GFileManager->CreateFileWriter( tempFileName.AsChar(), FOF_AbsolutePath | FOF_Buffered );
	if ( !file )
	{
		RED_LOG_ERROR( Shaders, TXT("MaterialCache: cannot open '%ls' for saving." ), tempFileName.AsChar() );
		return;
	}

	CTimeCounter timer;

	m_info.m_magic = SHADER_CACHE_MAGIC;
	m_info.m_version = SHADER_CACHE_VERSION_CURRENT;
	m_info.m_includesCRC = 0;

	file->m_version = SHADER_CACHE_VERSION_CURRENT;

	m_info.m_numShaderEntries = m_shaderEntries.Size();
	for ( auto entryPair : m_shaderEntries )
	{
		entryPair.m_second->Serialize( *file );
	}
	Uint64 currentOffset = file->GetOffset();
	m_info.m_materialsOffset = currentOffset;
	m_info.m_shadersDataSize = currentOffset;
	m_info.m_numMaterialEntries = m_materialEntries.Size();
	for ( auto entryPair : m_materialEntries )
	{
		entryPair.m_second->Serialize( *file );
	}
	currentOffset = file->GetOffset();
	m_info.m_materialsDataSize = currentOffset - m_info.m_materialsOffset;

	m_info.Serialize( *file );

	m_dirty = false;

	delete file;
	file = nullptr;

#ifndef RED_PLATFORM_CONSOLE
	// swap temp and proper shader cache files
	RED_VERIFY( GFileManager->CopyFileW( tempFileName, m_absoluteFilePath, true ) );
	RED_VERIFY( GFileManager->DeleteFileW( tempFileName ) );
#endif

	RED_LOG( Shaders, TXT( "MaterialCache: saved %u shaders, %u materials, took %1.2fms" ), m_info.m_numShaderEntries, m_info.m_numMaterialEntries, timer.GetTimePeriodMS() );
}

void CShaderCacheReadWrite::GetMaterialEntries_Sync( TDynArray< MaterialEntry* >& entries ) const
{
	CScopedLock lock( m_mutex );

	for ( auto entryPair : m_materialEntries )
	{
		entries.PushBack( entryPair.m_second );
	}
}

Bool CShaderCacheReadWrite::Initialize( const String& absoluteFilePath )
{
	CScopedLock lock( m_mutex );

	m_absoluteFilePath = absoluteFilePath;
	Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( absoluteFilePath.AsChar(), FOF_AbsolutePath | FOF_Buffered ) );
	if ( file )
	{
		const Uint64 size = file->GetSize();
		const Uint64 infoSize = sizeof( ShaderCacheFileInfo );
		if ( size >= infoSize )
		{
			file->Seek( size - infoSize );
			m_info.Serialize( *file );
			file->m_version = m_info.m_version;
			if ( m_info.m_version >= SHADER_CACHE_VERSION_COMPRESSED_ENTRIES )
			{
				RED_LOG( Shaders, TXT("File '%ls' - contains compressed and uncompressed entries.") , absoluteFilePath.AsChar() );
			}
			else
			{
				RED_LOG( Shaders, TXT("File '%ls' - contains ONLY uncompressed entries.") , absoluteFilePath.AsChar() );
			}

			if ( m_info.m_magic == SHADER_CACHE_MAGIC && m_info.m_numShaderEntries > 0 && m_info.m_numMaterialEntries > 0 )
			{
				file->Seek( 0 );
				for ( Uint32 i = 0; i < m_info.m_numShaderEntries; ++i )
				{
					ShaderEntry* entry = new ShaderEntry();
					entry->Serialize( *file );
					m_shaderEntries.Insert( entry->m_hash, entry );
				}
				for ( Uint32 i = 0; i < m_info.m_numMaterialEntries; ++i )
				{
					MaterialEntry* entry = new MaterialEntry();
					entry->Serialize( *file );
					m_materialEntries.Insert( entry->m_hash, entry );
				}
			}
			else
			{
				RED_LOG_ERROR( Shaders, TXT("ShaderCache '%ls': magic (loaded: '%u', expected: '%u'), version (loaded: '%u', expected: '%u'), shaders: %u materials: %u") , absoluteFilePath.AsChar(), m_info.m_magic, SHADER_CACHE_MAGIC, m_info.m_version, m_info.m_version, m_info.m_numShaderEntries, m_info.m_numMaterialEntries );
			}
		}
		else
		{
			RED_LOG_ERROR( Shaders, TXT("ShaderCache '%ls': size invalid (loaded: %") RED_PRIWu64 TXT(", expected more than: %") RED_PRIWu64 TXT(")"), absoluteFilePath.AsChar(), size, infoSize );
		}
	}
	else
	{
		if ( GIsCooker )
		{
			RED_LOG( Shaders, TXT("ShaderCache: could not find '%ls', will create a new one." ), absoluteFilePath.AsChar() );
		}
		else
		{
			RED_LOG_ERROR( Shaders, TXT("ShaderCache: could not find '%ls'" ), absoluteFilePath.AsChar() );
		}
	}
	m_dirty = false;
	return true;
}