/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "shaderCache.h"
#include "shaderCacheData.h"
#include "shaderCacheBuilder.h"
#include "shaderCacheReadonly.h"

#include "../core/dependencyMapper.h"
#include "../core/memoryFileReader.h"
#include "../core/depot.h"

//////////////////////////////////////////////////////////////////////////

Uint64 CalculateFileCRC( const String& fileAbsolutePath, Uint64 crc )
{
	IFile* file = GFileManager->CreateFileReader( fileAbsolutePath, FOF_AbsolutePath );
	if ( file )
	{
		size_t size = static_cast< size_t >( file->GetSize() );
		RED_ASSERT( (Uint64)size == file->GetSize(), TXT("Unexpectedly large file '%ls'"), file->GetFileNameForDebug() );
		void* mem = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, size );
		if ( mem )
		{
			file->Serialize( mem, size );
			crc = ACalcBufferHash64Merge( mem, size, crc );
			RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, mem );
		}

		delete file;
	}

	return crc;
}

Uint64 CalculateDirectoryCRC( const String& path )
{
	const String directoryAbsolutePath = GFileManager->GetBaseDirectory() + path.AsChar();

	// Get shader files
	TDynArray< String > shaderPaths;
	GFileManager->FindFiles( directoryAbsolutePath, TXT( "*.fx" ), shaderPaths, true );

	Sort( shaderPaths.Begin(), shaderPaths.End() );

	// Calculate merged CRC
	Uint64 mergedShaderCRC = 0;
	for ( Uint32 i = 0; i < shaderPaths.Size(); ++i )
	{
		const String& filePath = shaderPaths[ i ];
		mergedShaderCRC = CalculateFileCRC( filePath, mergedShaderCRC );
	}

	return mergedShaderCRC;
}

//////////////////////////////////////////////////////////////////////////

IShaderCache* IShaderCache::CreateReadOnly( const String& absolutePath )
{
	IShaderCache* cache = new CShaderCacheReadonly();
	if ( cache->Initialize( absolutePath ) )
	{
		return cache;
	}

	// failed to open the cache
	delete cache;
	return nullptr;
}

IShaderCache* IShaderCache::CreateReadWrite( const String& absolutePath )
{
	IShaderCache* cache = new CShaderCacheReadWrite();
	if ( cache->Initialize( absolutePath ) )
	{
		return cache;
	}

	// failed to open the cache
	delete cache;
	return nullptr;
}

IShaderCache* IShaderCache::CreateNull()
{
	return new CShaderCacheNull();
}
