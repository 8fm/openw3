/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/shaderCacheManager.h"
#include "../../common/engine/shaderCache.h"

//-----------------------------------------------------------------------------

class CPatchMaterialToken : public IBasePatchContentBuilder::IContentToken
{
public:
	CPatchMaterialToken( MaterialEntry* materialEntry, IShaderCache* cache );
	virtual ~CPatchMaterialToken();

	/// Get the unique token ID (usually hash of the filename)
	virtual const Uint64 GetTokenHash() const override;
	virtual const Uint64 GetDataCRC() const override;
	virtual const Uint64 GetDataSize() const override;
	virtual const Uint64 GetAdditionalData() const override;
	const String GetInfo() const override;
	void DebugDump( const String& dumpPath, const Bool isBase ) const override;

	MaterialEntry*				m_materialEntry;
	TDynArray< ShaderEntry* >	m_shaderEntries;
};

//-----------------------------------------------------------------------------

class CPatchShaderCache : public IBasePatchContentBuilder::IContentGroup
{
public:
	virtual ~CPatchShaderCache();

	/// Interface
	virtual void GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const override;
	virtual const Uint64 GetDataSize() const override;
	virtual const String GetInfo() const override;
	static CPatchShaderCache* LoadContent( const ECookingPlatform platform, const String& baseDirectory );

private:
	String								m_basePath;
	TDynArray< IShaderCache* >			m_caches;
	TDynArray< CPatchMaterialToken* >	m_tokens;
};

//-----------------------------------------------------------------------------

/// Patch builder for shaders
class CPatchBuilder_Shaders : public IBasePatchContentBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( CPatchBuilder_Shaders );

public:
	CPatchBuilder_Shaders();
	~CPatchBuilder_Shaders();

	/// Interface
	virtual String GetContentType() const override;
	virtual IContentGroup* LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )  override;
	virtual Bool SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName ) override;

private:
	CPatchShaderCache* m_patchShaderCache;
};

BEGIN_CLASS_RTTI( CPatchBuilder_Shaders );
	PARENT_CLASS( IBasePatchContentBuilder );
END_CLASS_RTTI();

//-----------------------------------------------------------------------------

class CPatchStaticShaderCacheToken : public IBasePatchContentBuilder::IContentToken
{
public:
	CPatchStaticShaderCacheToken( IStaticShaderCache* cache );
	virtual ~CPatchStaticShaderCacheToken();

	/// Get the unique token ID (usually hash of the filename)
	virtual const Uint64 GetTokenHash() const override;
	virtual const Uint64 GetDataCRC() const override;
	virtual const Uint64 GetDataSize() const override;
	const String GetInfo() const override;
	void DebugDump( const String& dumpPath, const Bool isBase ) const override;

	IStaticShaderCache*			m_cache;
	Uint64						m_size;
	Uint64						m_crc;
	Uint32						m_hash;
};

//-----------------------------------------------------------------------------

class CPatchStaticShaderCache : public IBasePatchContentBuilder::IContentGroup
{
public:
	virtual ~CPatchStaticShaderCache();

	/// Interface
	virtual void GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const override;
	virtual const Uint64 GetDataSize() const override;
	virtual const String GetInfo() const override;

	static CPatchStaticShaderCache* LoadContent( const ECookingPlatform platform, const String& baseDirectory );

private:
	String								m_basePath;
	CPatchStaticShaderCacheToken*		m_token;
};

//-----------------------------------------------------------------------------

/// Patch builder for static shaders
class CPatchBuilder_StaticShaders : public IBasePatchContentBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( CPatchBuilder_StaticShaders );

public:
	/// Interface
	virtual String GetContentType() const override;
	virtual IContentGroup* LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )  override;
	virtual Bool SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName ) override;
};

BEGIN_CLASS_RTTI( CPatchBuilder_StaticShaders );
	PARENT_CLASS( IBasePatchContentBuilder );
END_CLASS_RTTI();

//-----------------------------------------------------------------------------

class CPatchFurShaderCacheToken : public IBasePatchContentBuilder::IContentToken
{
public:
	CPatchFurShaderCacheToken( IFile* cacheFile, const String& absolutePath );
	virtual ~CPatchFurShaderCacheToken();

	/// Get the unique token ID (usually hash of the filename)
	virtual const Uint64 GetTokenHash() const override;
	virtual const Uint64 GetDataCRC() const override;
	virtual const Uint64 GetDataSize() const override;
	virtual const String GetInfo() const override;
	virtual void DebugDump( const String& dumpPath, const Bool isBase ) const override;

	IFile*							m_cacheFile;
	String							m_absolutePath;

	Uint64							m_size;
	Uint64							m_crc;
	Uint32							m_hash;
};

//-----------------------------------------------------------------------------

class CPatchFurShaderCache : public IBasePatchContentBuilder::IContentGroup
{
public:
	CPatchFurShaderCache();
	virtual ~CPatchFurShaderCache();

	/// Interface
	virtual void GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const override;
	virtual const Uint64 GetDataSize() const override;
	virtual const String GetInfo() const override;

	static CPatchFurShaderCache* LoadContent( const ECookingPlatform platform, const String& baseDirectory );

private:
	String							m_basePath;
	CPatchFurShaderCacheToken*		m_token;
};

//-----------------------------------------------------------------------------

/// Patch builder for fur shaders
class CPatchBuilder_FurShaders : public IBasePatchContentBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( CPatchBuilder_FurShaders );

public:
	/// Interface
	virtual String GetContentType() const override;
	virtual IContentGroup* LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )  override;
	virtual Bool SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName ) override;
};

BEGIN_CLASS_RTTI( CPatchBuilder_FurShaders );
	PARENT_CLASS( IBasePatchContentBuilder );
END_CLASS_RTTI();

//-----------------------------------------------------------------------------
