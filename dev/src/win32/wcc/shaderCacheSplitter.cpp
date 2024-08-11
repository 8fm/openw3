/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "baseCacheSplitter.h"
#include "../../common/core/commandlet.h"
#include "../../common/engine/shaderCacheManager.h"
#include "../../common/engine/shaderCache.h"

//////////////////////////////////////////////////////////////////////////

class CShaderCacheSplitter : public IBaseCacheSplitter
{
	DECLARE_RTTI_SIMPLE_CLASS( CShaderCacheSplitter );

public:
	CShaderCacheSplitter();
	~CShaderCacheSplitter();

	// interface
	virtual Bool Initialize( const ICommandlet::CommandletOptions& additonalOptions ) override;
	virtual Bool LoadInput( const String& absolutePath ) override;
	virtual void GetEntries( TDynArray< IBaseCacheEntry* >& allEntries ) const override;
	virtual Bool SaveOutput( const String& absolutePath, const TDynArray< IBaseCacheEntry* >& allEntries ) const override;

	// description
	virtual const Char* GetName() const override{ return TXT("shaders"); }
	virtual const Char* GetDescription() const override { return TXT("Split shader cache"); }

private:
	IShaderCache*		m_cache;
};

BEGIN_CLASS_RTTI( CShaderCacheSplitter )
	PARENT_CLASS( IBaseCacheSplitter );
END_CLASS_RTTI()

class CShaderCacheSplitEntry : public IBaseCacheEntry
{
public:
	CShaderCacheSplitEntry( MaterialEntry* materialEntry, IShaderCache* cache )
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

	virtual String GetResourcePath() const override
	{
		return m_materialEntry->m_path;
	}

	virtual Uint32 GetApproxSize() const override
	{
		Uint32 size = 0;
		for ( ShaderEntry* entry : m_shaderEntries )
		{
			size += entry->m_data.GetSize();
		}
		size += sizeof( MaterialEntry );
		size += m_shaderEntries.Size() * sizeof( ShaderEntry );
		return size;
	}

	MaterialEntry*				m_materialEntry;
	TDynArray< ShaderEntry* >	m_shaderEntries;
};

///------

IMPLEMENT_ENGINE_CLASS( CShaderCacheSplitter );

CShaderCacheSplitter::CShaderCacheSplitter()
{
}

CShaderCacheSplitter::~CShaderCacheSplitter()
{
	if ( m_cache )
	{
		delete m_cache;
		m_cache = nullptr;
	}
}

Bool CShaderCacheSplitter::Initialize( const ICommandlet::CommandletOptions& additonalOptions )
{
	return true;
}

Bool CShaderCacheSplitter::LoadInput( const String& absolutePath )
{
	m_cache = IShaderCache::CreateReadOnly( absolutePath );
	return m_cache != nullptr;
}

void CShaderCacheSplitter::GetEntries( TDynArray< IBaseCacheEntry* >& allEntries ) const
{
	// create an entry wrapper for each cache entry
	TDynArray< MaterialEntry* > materialEntries;
	m_cache->GetMaterialEntries_Sync( materialEntries );
	for ( auto entry : materialEntries )
	{
		allEntries.PushBack( new CShaderCacheSplitEntry( entry, m_cache ) );
	}
}

Bool CShaderCacheSplitter::SaveOutput( const String& absolutePath, const TDynArray< IBaseCacheEntry* >& allEntries ) const
{
	IShaderCache* localCache = IShaderCache::CreateReadWrite( absolutePath );
	if ( !localCache )
	{
		ERR_WCC( TXT("Failed to create output file '%ls'"), absolutePath.AsChar() );
		return false;
	}

	// transfer data from entries
	for ( Uint32 i=0; i<allEntries.Size(); ++i )
	{
		const CShaderCacheSplitEntry* entry = static_cast< CShaderCacheSplitEntry* >( allEntries[i] );
		localCache->AddMaterial( entry->m_materialEntry->m_hash, entry->m_materialEntry );
		for ( auto shaderEntry : entry->m_shaderEntries )
		{
			localCache->AddShader( shaderEntry->m_hash, shaderEntry );
		}
	}

	localCache->Flush();

	delete localCache;
	localCache = nullptr;

	return true;
}