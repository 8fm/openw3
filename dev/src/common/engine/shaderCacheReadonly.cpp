#include "build.h"
#include "shaderCacheReadonly.h"
#include "../core/memoryFileReader.h"
#include "../core/ioTags.h"

//////////////////////////////////////////////////////////////////////////

CShaderCacheAsyncLoader::CShaderCacheAsyncLoader()
	: m_cache( nullptr )
{
}

CShaderCacheAsyncLoader::~CShaderCacheAsyncLoader()
{
}

void CShaderCacheAsyncLoader::StartLoading( const String& absoluteFilePath, ShaderCacheFileInfo* info, volatile Bool* isReady, volatile Bool* failedToLoad )
{
	// setup state
	m_ready = isReady;
	m_failedToLoad = failedToLoad;
	m_info = info;

	// open file
	typedef Red::IO::CAsyncFileHandleCache::TFileHandle	FileHandle;
	const FileHandle file = Red::IO::GAsyncIO.OpenFile( absoluteFilePath.AsChar(), Red::IO::eAsyncFlag_TryCloseFileWhenNotUsed );
	if ( file == Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE )
	{
		RED_LOG_ERROR( Shaders, TXT("ShaderCache error: cache file '%ls' not found"), absoluteFilePath.AsChar() );
		*failedToLoad = true;
		return;
	}

	Uint64 fileSize = Red::IO::GAsyncIO.GetFileSize( file );
	Uint64 offset = fileSize - sizeof( ShaderCacheFileInfo );

	void* buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, sizeof( ShaderCacheFileInfo ) );

	// setup first read
	m_readToken.m_userData = this;
	m_readToken.m_numberOfBytesToRead = sizeof( ShaderCacheFileInfo );
	m_readToken.m_offset = offset;
	m_readToken.m_callback = &OnInfoLoaded;
	m_readToken.m_buffer = buffer;
	Red::IO::GAsyncIO.BeginRead( file, m_readToken, Red::IO::eAsyncPriority_High, eIOTag_ResourceNormal );
	Red::IO::GAsyncIO.ReleaseFile( file );
}

Red::IO::ECallbackRequest CShaderCacheAsyncLoader::OnInfoLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CShaderCacheAsyncLoader* loader = static_cast< CShaderCacheAsyncLoader* >( asyncReadToken.m_userData );

	// data read
	if ( asyncResult == Red::IO::eAsyncResult_Success )
	{
		RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

		CMemoryFileReader infoReader( (Uint8*)asyncReadToken.m_buffer, sizeof( ShaderCacheFileInfo ), 0 );
		loader->m_info->Serialize( infoReader );

		if ( loader->m_info->m_version != SHADER_CACHE_VERSION_CURRENT )
		{
			RED_LOG_WARNING( Shaders, TXT("ShaderCache AsyncIO: SHADER_CACHE_VERSION mismatch (loaded: '%u', expected: '%u')."), loader->m_info->m_version, SHADER_CACHE_VERSION_CURRENT );
		}

		if ( loader->m_info->m_magic != SHADER_CACHE_MAGIC )
		{
			*loader->m_failedToLoad = true;
			RED_LOG_ERROR( Shaders, TXT("ShaderCache AsyncIO: SHADER_CACHE_MAGIC mismatch (loaded: '%u', expected: '%u'), file is corrupted"), loader->m_info->m_magic, SHADER_CACHE_MAGIC );
			return Red::IO::eCallbackRequest_Finish;
		}

		RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, asyncReadToken.m_buffer );

		// allocate buffer for shaders
		loader->m_shadersBuffer = (Uint8*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_TemporaryShaderCache, loader->m_info->m_shadersDataSize );

		// load
		asyncReadToken.m_buffer = loader->m_shadersBuffer;
		asyncReadToken.m_offset = 0; // shaders always start at the beginning of the file
		asyncReadToken.m_numberOfBytesToRead = (Uint32)loader->m_info->m_shadersDataSize;
		asyncReadToken.m_callback = &OnShadersLoaded;
		return Red::IO::eCallbackRequest_More;
	}

	*loader->m_failedToLoad = true;
	RED_LOG_ERROR( Shaders, TXT("ShaderCache AsyncIO: loading of ShaderCacheFileInfo failed.") );
	return Red::IO::eCallbackRequest_Finish;
}

Red::IO::ECallbackRequest CShaderCacheAsyncLoader::OnShadersLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CShaderCacheAsyncLoader* loader = static_cast< CShaderCacheAsyncLoader* >( asyncReadToken.m_userData );

	// data read
	if ( asyncResult == Red::IO::eAsyncResult_Success )
	{
		RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

		// allocate buffer for materials
		loader->m_materialsBuffer = (Uint8*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_TemporaryMaterialCache, loader->m_info->m_materialsDataSize );

		CMemoryFileReader shadersBufferReader( loader->m_shadersBuffer, loader->m_info->m_shadersDataSize, 0 );
		shadersBufferReader.m_version = loader->m_info->m_version;
		loader->m_cache->SerializeShaderEntries( loader->m_info->m_numShaderEntries, shadersBufferReader );

		// load
		asyncReadToken.m_buffer = loader->m_materialsBuffer;
		asyncReadToken.m_offset = loader->m_info->m_materialsOffset;
		asyncReadToken.m_numberOfBytesToRead = (Uint32)loader->m_info->m_materialsDataSize;
		asyncReadToken.m_callback = &OnMaterialsLoaded;
		return Red::IO::eCallbackRequest_More;
	}

	*loader->m_failedToLoad = true;
	RED_LOG_ERROR( Shaders, TXT("ShaderCache AsyncIO: loading ShaderEntries failed.") );
	return Red::IO::eCallbackRequest_Finish;
}

Red::IO::ECallbackRequest CShaderCacheAsyncLoader::OnMaterialsLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CShaderCacheAsyncLoader* loader = static_cast< CShaderCacheAsyncLoader* >( asyncReadToken.m_userData );

	// data read
	if ( asyncResult == Red::IO::eAsyncResult_Success )
	{
		RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial read is not supported here" );

		CMemoryFileReader materialsBufferReader( loader->m_materialsBuffer, loader->m_info->m_materialsDataSize, 0 );
		loader->m_cache->SerializeMaterialEntries( loader->m_info->m_numMaterialEntries, materialsBufferReader );

		// set ready flag
		*loader->m_ready = true;

		// free temporary buffers used for loading the files from drive
		loader->FinishLoading();

		// done
		return Red::IO::eCallbackRequest_Finish;
	}

	// something went wrong
	*loader->m_failedToLoad = true;
	RED_LOG_ERROR( Shaders, TXT("ShaderCache AsyncIO: loading MaterialEntries failed.") );
	return Red::IO::eCallbackRequest_Finish;
}

void CShaderCacheAsyncLoader::FinishLoading()
{
	if ( m_shadersBuffer )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_TemporaryShaderCache, m_shadersBuffer );
		m_shadersBuffer = nullptr;
	}
	if ( m_materialsBuffer )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_TemporaryMaterialCache, m_materialsBuffer );
		m_materialsBuffer = nullptr;
	}
}

void CShaderCacheAsyncLoader::SetCache( CShaderCacheReadonly * cache )
{
	m_cache = cache;
}

CShaderCacheReadonly::CShaderCacheReadonly()
	: m_isLoaded( false )
	, m_isReady( false )
	, m_failedToLoad( false )
{
	m_loader.SetCache( this );
}

CShaderCacheReadonly::~CShaderCacheReadonly()
{
	// until done or failed. 
	while( !VerifyLoaded() && !m_failedToLoad )
	{
		continue;
	}
}

//! Methods below can return eResult_Pending if the data is not yet available
IShaderCache::EResult CShaderCacheReadonly::GetShader( const Uint64 hash, ShaderEntry*& entry )
{
	if ( !VerifyLoaded() )
	{
		return IShaderCache::eResult_Pending;
	}

	if ( m_failedToLoad )
	{
		return IShaderCache::eResult_Invalid;
	}

	if ( m_shaderEntries.Find( hash, entry ) )
	{
		return IShaderCache::eResult_Valid;
	}
	return IShaderCache::eResult_Invalid;
}

IShaderCache::EResult CShaderCacheReadonly::GetMaterial( const Uint64 hash, MaterialEntry*& entry )
{
	if ( !VerifyLoaded() )
	{
		return IShaderCache::eResult_Pending;
	}

	if ( m_failedToLoad )
	{
		return IShaderCache::eResult_Invalid;
	}

	if ( m_materialEntries.Find( hash, entry ) )
	{
		return IShaderCache::eResult_Valid;
	}
	return IShaderCache::eResult_Invalid;
}

IShaderCache::EResult CShaderCacheReadonly::AddShader( const Uint64 hash, ShaderEntry* entry )
{
	RED_HALT( "Trying to add ShaderEntry to readonly cache." );
	return IShaderCache::eResult_Valid;
}
IShaderCache::EResult CShaderCacheReadonly::AddMaterial( const Uint64 hash, MaterialEntry* entry )
{
	RED_HALT( "Trying to add MaterialEntry to readonly cache." );
	return IShaderCache::eResult_Valid;
}

IShaderCache::EResult CShaderCacheReadonly::RemoveMaterial( const Uint64 hash )
{
	RED_HALT( "Trying to remove Material from readonly cache." );
	return IShaderCache::eResult_Valid;
}

//! Flush dirty cache content to file, this only works for a cache that is not read only and has a file
void CShaderCacheReadonly::Flush()
{
	RED_HALT( "Trying to Flush() readonly cache." );
}

Bool CShaderCacheReadonly::Initialize( const String& absoluteFilePath )
{
	m_loader.StartLoading( absoluteFilePath, &m_info, &m_isLoaded, &m_failedToLoad );
	if ( m_failedToLoad )
	{
		// Failed to even start
		return false;
	}

	return true;
}

Bool CShaderCacheReadonly::VerifyLoaded()
{
	// already done
	if ( m_isReady )
	{
		return true;
	}

	// not loaded yet
	if ( !m_isLoaded )
	{
		RED_LOG_SPAM( Shaders, TXT("ShaderCache: still loading") );
		return false;
	}

	RED_LOG( Shaders, TXT("Readonly ShaderCache loaded %u shaders, %u materials"), m_shaderEntries.Size(), m_materialEntries.Size() );

	// we are ready now to service requests
	m_isReady = true;
	return true;
}

void CShaderCacheReadonly::GetMaterialEntries_Sync( TDynArray< MaterialEntry* >& entries ) const
{
	// until done or failed
	while( !const_cast< CShaderCacheReadonly* >( this )->VerifyLoaded() && !m_failedToLoad )
	{
		// Sleep for a sec. No need to spam VerifyLoaded ... this function is also only use in wcc so no worries
		Red::Threads::SleepOnCurrentThread( 1000 ); 
		continue;
	}
	entries.Reserve( m_materialEntries.Size() );
	for ( auto entryPair : m_materialEntries )
	{
		entries.PushBack( entryPair.m_second );
	}
}

void CShaderCacheReadonly::GetShaderEntriesUsageCount( TShaderEntryUsageCounter& usageCounter ) const
{
	// until done or failed
	while( !const_cast< CShaderCacheReadonly* >( this )->VerifyLoaded() && !m_failedToLoad )
	{
		// Sleep for a sec. No need to spam VerifyLoaded ... this function is also only use in wcc so no worries
		Red::Threads::SleepOnCurrentThread( 1000 ); 
		continue;
	}
	for ( auto entryPair : m_materialEntries )
	{
		const MaterialEntry* materialEntry = entryPair.m_second;
		
#ifndef RED_OPTIMIZED_MATERIAL_ENTRY
		for ( Uint32 shaderType = 0; shaderType < GpuApi::ShaderTypeMax; ++shaderType )
		{
			Uint64 shaderEntryKey = materialEntry->m_shaderHashes[ shaderType ];
			if ( usageCounter.KeyExist( shaderEntryKey ) )
			{
				++usageCounter[ shaderEntryKey ];
			}
			else
			{
				usageCounter.Insert( shaderEntryKey, 1 );
			}
		}
#else
		// VS
		if ( usageCounter.KeyExist( materialEntry->m_shaderHashVS ) )
		{
			++usageCounter[ materialEntry->m_shaderHashVS ];
		}
		else
		{
			usageCounter.Insert( materialEntry->m_shaderHashVS, 1 );
		}
		// PS
		if ( usageCounter.KeyExist( materialEntry->m_shaderHashPS ) )
		{
			++usageCounter[ materialEntry->m_shaderHashPS ];
		}
		else
		{
			usageCounter.Insert( materialEntry->m_shaderHashPS, 1 );
		}
		// DS
		if ( usageCounter.KeyExist( materialEntry->m_shaderHashDS ) )
		{
			++usageCounter[ materialEntry->m_shaderHashDS ];
		}
		else
		{
			usageCounter.Insert( materialEntry->m_shaderHashDS, 1 );
		}
		// HS
		if ( usageCounter.KeyExist( materialEntry->m_shaderHashHS ) )
		{
			++usageCounter[ materialEntry->m_shaderHashHS ];
		}
		else
		{
			usageCounter.Insert( materialEntry->m_shaderHashHS, 1 );
		}
#endif
	}
}

void CShaderCacheReadonly::DecreaseCountersForMaterialEntry( Uint64 materialEntryHash, TShaderEntryUsageCounter& usageCounter ) const
{
	MaterialEntry* materialEntry = nullptr;
	if ( m_materialEntries.Find( materialEntryHash, materialEntry ) )
	{
#ifndef RED_OPTIMIZED_MATERIAL_ENTRY
		for ( Uint32 shaderType = 0; shaderType < GpuApi::ShaderTypeMax; ++shaderType )
		{
			Uint64 shaderEntryKey = materialEntry->m_shaderHashes[ shaderType ];
			RED_ASSERT( usageCounter.KeyExist( shaderEntryKey ), TXT("") );
			--usageCounter[ shaderEntryKey ];
			RED_ASSERT( usageCounter[ shaderEntryKey ] >= 0, TXT("") );
		}
#else
		// VS
		if ( usageCounter.KeyExist( materialEntry->m_shaderHashVS ) )
		{
			--usageCounter[ materialEntry->m_shaderHashVS ];
			RED_ASSERT( usageCounter[ materialEntry->m_shaderHashVS ] >= 0, TXT("") );
		}
		// PS
		if ( usageCounter.KeyExist( materialEntry->m_shaderHashPS ) )
		{
			--usageCounter[ materialEntry->m_shaderHashPS ];
			RED_ASSERT( usageCounter[ materialEntry->m_shaderHashPS ] >= 0, TXT("") );
		}
		// DS
		if ( usageCounter.KeyExist( materialEntry->m_shaderHashDS ) )
		{
			--usageCounter[ materialEntry->m_shaderHashDS ];
			RED_ASSERT( usageCounter[ materialEntry->m_shaderHashDS ] >= 0, TXT("") );
		}
		// HS
		if ( usageCounter.KeyExist( materialEntry->m_shaderHashHS ) )
		{
			--usageCounter[ materialEntry->m_shaderHashHS ];
			RED_ASSERT( usageCounter[ materialEntry->m_shaderHashHS ] >= 0, TXT("") );
		}
#endif
	}
}

void CShaderCacheReadonly::UnloadUnusedShaderEntries( const TShaderEntryUsageCounter& usageCounter )
{
	Int64 beforeUnload = Memory::GetAllocatedBytesPerMemoryClass< MemoryPool_Default >( MC_ShaderCacheEntry );

	Uint32 removedEntries = 0;
	for ( const auto usageCounterPair : usageCounter )
	{
		if ( usageCounterPair.m_second <= 0 )
		{
			Uint64 shaderEntryHash = usageCounterPair.m_first;

			ShaderEntry* shaderEntry = nullptr;
			if ( m_shaderEntries.Find( shaderEntryHash, shaderEntry ) )
			{
				if ( shaderEntry && shaderEntryHash == shaderEntry->m_hash )
				{
					Bool removed = false;
					for ( Uint32 i = 0; i < m_shaders.Size(); ++i )
					{
						if ( m_shaders[i].m_hash == shaderEntry->m_hash )
						{
							m_shaders[i].m_data.Clear();
							removed = true;
							++removedEntries;
							break;
						}
					}

					RED_ASSERT( removed, TXT("") );
					if ( removed )
					{
						m_shaderEntries.Erase( shaderEntryHash );	
					}					
				}
			}
		}
	}

	Int64 afterUnload = Memory::GetAllocatedBytesPerMemoryClass< MemoryPool_Default >( MC_ShaderCacheEntry );
	Int64 delta = afterUnload - beforeUnload;
	String sBeforeUnload	= String::FormatByteNumber( beforeUnload, TString<Char>::BNF_Precise );
	String sAfterUnload		= String::FormatByteNumber( afterUnload, TString<Char>::BNF_Precise );
	String sDelta			= String::Printf( TXT("%s%s"), delta < 0 ? TXT("-") : TXT(""), String::FormatByteNumber( delta < 0 ? -delta : delta, TString<Char>::BNF_Precise ).AsChar() );
	LOG_ENGINE( TXT("Unloaded: %d entries; MemoryBefore: %ls; MemoryAfter: %ls; MemoryDelta: %ls"), removedEntries, sBeforeUnload.AsChar(), sAfterUnload.AsChar(), sDelta.AsChar() );
}

void CShaderCacheReadonly::GetMaterialEntriesHashes_Sync( TDynArray< Uint64 >& materialEntryHashes ) const
{
	// until done or failed
	while( !const_cast< CShaderCacheReadonly* >( this )->VerifyLoaded() && !m_failedToLoad )
	{
		// Sleep for a sec. No need to spam VerifyLoaded ... this function is also only use in wcc so no worries
		Red::Threads::SleepOnCurrentThread( 1000 ); 
		continue;
	}
	materialEntryHashes.Reserve( m_materialEntries.Size() );
	for ( auto entryPair : m_materialEntries )
	{
		materialEntryHashes.PushBack( entryPair.m_first );
	}
}

void CShaderCacheReadonly::SerializeShaderEntries( Uint32 entryCount, IFile & file )
{
	m_shaderEntries.Reserve( entryCount );
	m_shaders.Resize( entryCount );

	for( Uint32 index = 0; index != entryCount; ++index )
	{
		ShaderEntry & shader = m_shaders[ index ];
		shader.Serialize( file );
		m_shaderEntries.Insert( shader.m_hash, &shader );
	}
}

void CShaderCacheReadonly::SerializeMaterialEntries( Uint32 entryCount, IFile & file )
{
	m_materialEntries.Reserve( entryCount );
	m_materials.Resize( entryCount );
	for( Uint32 index = 0; index != entryCount; ++index )
	{
		MaterialEntry & entry = m_materials[ index ];
		entry.Serialize( file );
		m_materialEntries.Insert( entry.m_hash, &entry );
	}
}

