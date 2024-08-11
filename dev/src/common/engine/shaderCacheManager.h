/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "shaderCache.h"
#include "staticShaderCache.h"

#include "../core/contentListener.h"
#include "../core/lazyCacheChain.h"

class CShaderCacheResolver : public IContentListener
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	static const Uint32 MAX_CACHE_CHAIN_LENGTH = 64;

public:
	CShaderCacheResolver();
	virtual ~CShaderCacheResolver();

	Bool Init();
	void Shutdown();

public:
	RED_INLINE Uint64	GetCachedIncludesCRC() const { return m_includesCRC; }
	RED_INLINE Bool		IsReadonly() const { return m_isReadonly; }
	Bool				StaticShadersMatch() const { RED_FATAL_ASSERT( m_staticShaderCache, "No static shader cache!"); return m_staticShaderCache->ShadersMatch(); }
	void				InvalidateStaticShaders() { RED_FATAL_ASSERT( m_staticShaderCache, "No static shader cache!"); m_staticShaderCache->InvalidateStaticShaders(); }

public:
	Bool HasShader( Uint64 hash ) const;

public:
	Bool GetStaticShader( Uint64 hash, StaticShaderEntry*& entry );
	Bool AddStaticShader( Uint64 hash, StaticShaderEntry* entry );
	Bool AddStaticShader( Uint64 hash, Uint64 contentCRC, const DataBuffer& shaderData );

	//! Methods below can return eResult_Pending if the data is not yet available
	IShaderCache::EResult GetShader( Uint64 hash, ShaderEntry*& entry );
	IShaderCache::EResult GetShaderData( Uint64 hash, DataBuffer& data ); //!< User is responsible for freeing the data

	IShaderCache::EResult AddShader( Uint64 hash, ShaderEntry* entry );
	IShaderCache::EResult AddShader( Uint64 hash, const DataBuffer& shaderData );
	IShaderCache::EResult GetMaterial( Uint64 hash, MaterialEntry*& entry );
	IShaderCache::EResult AddMaterial( Uint64 hash, MaterialEntry* entry );

	//! Flush dirty cache content to file, this only works for a cache that is not read only and has a file
	void Flush();

public:
	void EnableSaving( Bool enableSaving );

public:
	void GetAllMaterials_Sync( TDynArray< MaterialEntry* >& entries ) const;
	void RemoveMaterial( Uint64 hash );
	void CacheDirectoriesCRC();

	//! Get absolute paths for all discovered fur shader caches.
	void GetAllFurShaderCaches( TDynArray< String >& absolutePaths ) const;

private:
	virtual const Char* GetName() const override { return TXT("CShaderCacheResolver"); }
	virtual void OnContentAvailable( const SContentInfo& contentInfo ) override;

private:
	enum EAttachPolicy
	{
		eAttachPolicy_Front,
		eAttachPolicy_Back,
	};

private:
	void AttachCache( const String& shaderCacheFile, EAttachPolicy policy );
	void AttachFurShaderCache( const String& shaderCacheFile );

#if defined( RED_PLATFORM_DURANGO ) && !defined( RED_FINAL_BUILD )
	void InitXboxOneWritableShaderCaches();
#endif

	void EP2_RemoveDuplicates( const IShaderCache* patchCache, IShaderCache* contentCache );

private:
	void InitWritableShaderCache();

private:
	Uint64									m_includesCRC;
	Bool									m_isReadonly;
	Bool									m_enableSaving;

private:
	IStaticShaderCache* m_staticShaderCache;
	IShaderCache*		m_writableShaderCache;

	typedef Helper::CLazyCacheChain< IShaderCache, MAX_CACHE_CHAIN_LENGTH > CacheChain;
	mutable CacheChain m_shaderCacheChain;


	TDynArray< String > m_furShaderCaches;
};

extern CShaderCacheResolver* GShaderCache;
