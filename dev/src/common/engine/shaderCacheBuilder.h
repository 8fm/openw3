#pragma once

#include "shaderCache.h"

//////////////////////////////////////////////////////////////////////////

class CShaderCacheReadWrite : public IShaderCache
{
public:
	CShaderCacheReadWrite();
	virtual ~CShaderCacheReadWrite();

	//! Methods below can return eResult_Pending if the data is not yet available
	virtual EResult GetShader( const Uint64 hash, ShaderEntry*& entry ) override;
	virtual EResult GetMaterial( const Uint64 hash, MaterialEntry*& entry ) override;

	//! Methods below can return eResult_Pending if the data is not yet available
	virtual EResult AddShader( const Uint64 hash, ShaderEntry* entry ) override;
	virtual EResult AddMaterial( const Uint64 hash, MaterialEntry* entry ) override;

	virtual EResult RemoveMaterial( const Uint64 hash ) override;

	RED_FORCE_INLINE virtual void ForceDirty( ) override { m_dirty = true; }

	//! Flush dirty cache content to file, this only works for a cache that is not read only and has a file
	virtual void Flush() override;

	RED_FORCE_INLINE virtual void SetAbsolutePath( const String& absoluteFilePath ) override { m_absoluteFilePath = absoluteFilePath; }

	virtual void GetMaterialEntries_Sync( TDynArray< MaterialEntry* >& entries ) const override;

	virtual Bool Initialize( const String& absoluteFilePath ) override;

private:
	String								m_absoluteFilePath;
	ShaderCacheFileInfo					m_info;
	Bool								m_dirty;

	THashMap< Uint64, ShaderEntry* >	m_shaderEntries;
	THashMap< Uint64, MaterialEntry* >	m_materialEntries;

private:
	typedef Red::Threads::CMutex CMutex;
	typedef Red::Threads::CScopedLock< CMutex > CScopedLock;

private:
	mutable CMutex						m_mutex;
};