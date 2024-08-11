/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "shaderCacheData.h"
#include "../../common/redIO/redIO.h"
#include "../redIO/redIOAsyncIO.h"
#include "../redIO/redIOAsyncFileHandleCache.h"
#include "../redIO/redIOAsyncReadToken.h"

#pragma pack(push)
#pragma pack(1)
struct ShaderCacheFileInfo
{
	Uint32 m_numShaderEntries;
	Uint32 m_numMaterialEntries;
	Uint32 m_magic;
	Uint32 m_version;
	Uint64 m_includesCRC;
	Uint64 m_shadersDataSize;	// size of the materials data buffer to serialize (shader entries + material entries)
	Uint64 m_materialsDataSize;	// size of the materials data buffer to serialize (shader entries + material entries)
	Uint64 m_materialsOffset;	// offset in the file where materials start

	void Serialize( IFile& file )
	{
		file << m_numShaderEntries;
		file << m_numMaterialEntries;
		file << m_includesCRC;

		file << m_shadersDataSize;
		file << m_materialsDataSize;
		file << m_materialsOffset;

		file << m_magic;
		file << m_version;
	}
};
#pragma pack(pop)

static_assert( sizeof( ShaderCacheFileInfo ) == 4 * sizeof( Uint32 ) + 4 * sizeof( Uint64 ), "Invalid ShaderCacheFileInfo struct size" );

typedef THashMap< Uint64, Int32 > TShaderEntryUsageCounter;

//////////////////////////////////////////////////////////////////////////

// General shader cache interface
class IShaderCache
{
public:
	virtual ~IShaderCache() {}

	// Operation result
	enum EResult
	{
		eResult_Pending	= -1,	//!< Operation is still being processed, you must wait
		eResult_Invalid = 0,	//!< Operation failed (and/or) result is invalid
		eResult_Valid	= 1,	//!< Operation succeeded (and/or) result is valid
	};

	RED_INLINE void SetIsFromPatch( Bool isFromPatch ) { m_isFromPatch = isFromPatch; }
	RED_INLINE Bool IsFromPatch() const { return m_isFromPatch; }

	//! Methods below can return eResult_Pending if the data is not yet available
	virtual EResult GetShader( const Uint64 hash, ShaderEntry*& entry ) = 0;
	virtual EResult GetMaterial( const Uint64 hash, MaterialEntry*& entry ) = 0;

	//! Methods below can return eResult_Pending if the data is not yet available
	virtual EResult AddShader( const Uint64 hash, ShaderEntry* entry ) = 0;
	virtual EResult AddMaterial( const Uint64 hash, MaterialEntry* entry ) = 0;

	virtual EResult RemoveMaterial( const Uint64 hash ) = 0;

	//! Mark as dirty when data has been changed externally
	virtual void ForceDirty( ) { }

	//! Flush dirty cache content to file, this only works for a cache that is not read only and has a file
	virtual void Flush() = 0;

	virtual void SetAbsolutePath( const String& absoluteFilePath ) { }

public:
	virtual void GetMaterialEntries_Sync( TDynArray< MaterialEntry* >& entries ) const = 0;

	// Removing duplicate shader binaries, I'm not entirely proud of the code in that methods
	virtual void GetShaderEntriesUsageCount( TShaderEntryUsageCounter& usageCounter ) const { }
	virtual void DecreaseCountersForMaterialEntry( Uint64 materialEntryHash, TShaderEntryUsageCounter& usageCounter ) const { }
	virtual void UnloadUnusedShaderEntries( const TShaderEntryUsageCounter& usageCounter ) { }
	virtual void GetMaterialEntriesHashes_Sync( TDynArray< Uint64 >& materialEntryHashes ) const { }

public:
	// Create read only cache
	static IShaderCache* CreateReadOnly( const String& absolutePath );

	// Create runtime read/write cache (editor-only)
	static IShaderCache* CreateReadWrite( const String& absolutePath );

	// Create NULL cache
	static IShaderCache* CreateNull();

protected:
	virtual Bool Initialize( const String& absoluteFilePath ) = 0;

	Bool	m_isFromPatch;
};

//////////////////////////////////////////////////////////////////////////

class CShaderCacheNull : public IShaderCache
{
public:
	virtual ~CShaderCacheNull()
	{}

	virtual EResult GetStaticShader( const Uint64 hash, ShaderEntry*& entry )
	{
		return IShaderCache::eResult_Invalid;
	}

	virtual EResult GetShader( const Uint64 hash, ShaderEntry*& entry )
	{
		return IShaderCache::eResult_Invalid;
	}

	virtual EResult GetMaterial( const Uint64 hash, MaterialEntry*& entry )
	{
		return IShaderCache::eResult_Invalid;
	}

	virtual EResult AddStaticShader( const Uint64 hash, ShaderEntry* entry )
	{
		return IShaderCache::eResult_Invalid;
	}

	virtual EResult AddShader( const Uint64 hash, ShaderEntry* entry )
	{
		return IShaderCache::eResult_Invalid;
	}

	virtual EResult AddMaterial( const Uint64 hash, MaterialEntry* entry )
	{
		return IShaderCache::eResult_Invalid;
	}

	virtual EResult RemoveMaterial( const Uint64 hash )
	{
		return IShaderCache::eResult_Invalid;
	}

	virtual void Flush()
	{
	}

	virtual void GetMaterialEntries_Sync( TDynArray< MaterialEntry* >& entries ) const
	{
	}

	virtual Bool Initialize( const String& absoluteFilePath )
	{
		return true;
	}
};

//////////////////////////////////////////////////////////////////////////

Uint64 CalculateFileCRC( const String& fileAbsolutePath, Uint64 crc );
Uint64 CalculateDirectoryCRC( const String& path );
