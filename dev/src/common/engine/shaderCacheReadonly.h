#pragma once

#include "shaderCache.h"
#include "../core/uniqueBuffer.h"

//////////////////////////////////////////////////////////////////////////

class CShaderCacheReadonly;

/// Asynchronous loader for collision cache data
class CShaderCacheAsyncLoader
{
public:
	CShaderCacheAsyncLoader();
	~CShaderCacheAsyncLoader();

	// load data asynchronously from specified file, set the ready flag when done
	// if it fails the ready flag is NOT set
	void StartLoading( const String& absoluteFilePath, ShaderCacheFileInfo* info, volatile Bool* readyFlag, volatile Bool* failedToLoad );
	void FinishLoading();

	void SetCache( CShaderCacheReadonly * cache ); 

private:
	// internal state
	Red::IO::SAsyncReadToken					m_readToken;		//!< Reading token
	void*										m_data;				//!< Loaded data
	volatile Bool*								m_ready;			//!< Ready flag to set
	volatile Bool*								m_failedToLoad;		//!< Error marker
	ShaderCacheFileInfo*						m_info;
	Uint8*										m_shadersBuffer;
	Uint8*										m_materialsBuffer;
	CShaderCacheReadonly *						m_cache;

	// stats - start time
	CTimeCounter						m_timer;

	// async loading integration
	static Red::IO::ECallbackRequest OnInfoLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
	static Red::IO::ECallbackRequest OnShadersLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
	static Red::IO::ECallbackRequest OnMaterialsLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
};

//////////////////////////////////////////////////////////////////////////

class CShaderCacheReadonly : public IShaderCache
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

public:
	CShaderCacheReadonly();
	virtual ~CShaderCacheReadonly();

	//! Methods below can return eResult_Pending if the data is not yet available
	virtual EResult GetShader( const Uint64 hash, ShaderEntry*& entry ) override;
	virtual EResult GetMaterial( const Uint64 hash, MaterialEntry*& entry ) override;

	//! We cannot add entries to readonly cache
	virtual EResult AddShader( const Uint64 hash, ShaderEntry* entry ) override;
	virtual EResult AddMaterial( const Uint64 hash, MaterialEntry* entry ) override;

	virtual EResult RemoveMaterial( const Uint64 hash ) override;

	//! Flush dirty cache content to file, this only works for a cache that is not read only and has a file
	virtual void Flush();

	virtual void GetMaterialEntries_Sync( TDynArray< MaterialEntry* >& entries ) const override;

	// Removing duplicates
	virtual void GetShaderEntriesUsageCount( TShaderEntryUsageCounter& usageCounter ) const override;
	virtual void DecreaseCountersForMaterialEntry( Uint64 materialEntryHash, TShaderEntryUsageCounter& usageCounter ) const override;
	virtual void UnloadUnusedShaderEntries( const TShaderEntryUsageCounter& usageCounter ) override;
	virtual void GetMaterialEntriesHashes_Sync( TDynArray< Uint64 >& materialEntryHashes ) const override;

	virtual Bool Initialize( const String& absoluteFilePath );

	Bool VerifyLoaded();

	void SerializeShaderEntries( Uint32 entryCount, IFile & file );
	void SerializeMaterialEntries(Uint32 entryCount, IFile & file );

private:
	CShaderCacheAsyncLoader	m_loader;
	ShaderCacheFileInfo		m_info;
	THashMap< Uint64, ShaderEntry* >	m_shaderEntries;
	THashMap< Uint64, MaterialEntry* >	m_materialEntries;
	
	TDynArray< ShaderEntry, MC_ShaderCacheEntry > m_shaders;
	TDynArray< MaterialEntry, MC_MaterialCacheEntry > m_materials;

	volatile Bool			m_isReady;
	volatile Bool			m_isLoaded;
	volatile Bool			m_failedToLoad;

};