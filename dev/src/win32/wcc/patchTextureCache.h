/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "patchBuilder.h"
#include "textureCacheProcessing.h"


class CPatchTextureCacheEntryToken : public IBasePatchContentBuilder::IContentToken, Red::System::NonCopyable
{
private:
	CTextureCacheData&							m_data;
	Uint32										m_entryIndex;

	Uint64										m_dataCRC;

public:
	CPatchTextureCacheEntryToken( Uint32 index, CTextureCacheData& data );

	virtual const Uint64 GetTokenHash() const override;
	virtual const Uint64 GetDataCRC() const override;
	virtual const Uint64 GetDataSize() const override;
	virtual const String GetInfo() const override;
	virtual void DebugDump( const String& dumpPath, const Bool isBase ) const override;

	Bool CalculateCRC();

	Uint32 GetEntryIndex() const { return m_entryIndex; }
	const CTextureCacheData& GetSourceData() const { return m_data; }

	String GetEntryName() const { return m_data.GetEntryName( m_entryIndex ); }
};


//////////////////////////////////////////////////////////////////////////


class CPatchTextureCache : public IBasePatchContentBuilder::IContentGroup
{
private:
	TDynArray< CTextureCacheData* >				m_textureCaches;
	TDynArray< CPatchTextureCacheEntryToken* >	m_tokens;

public:
	CPatchTextureCache();
	~CPatchTextureCache();

	virtual void GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const override;
	virtual const Uint64 GetDataSize() const override;
	virtual const String GetInfo() const override;


	static CPatchTextureCache* LoadTextureCaches( const String& baseDirectory );
};


//////////////////////////////////////////////////////////////////////////


class CPatchBuilder_TextureCache : public IBasePatchContentBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( CPatchBuilder_TextureCache );

private:
	TDynArray< CPatchTextureCache* >				m_loadedContent;		// Builder owns the content, so we need to hang on to it to free.

public:
	CPatchBuilder_TextureCache();
	~CPatchBuilder_TextureCache();

	virtual Bool CanUseWithMods() const override { return true; }
	virtual String GetContentType() const override;
	virtual IContentGroup* LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )  override;
	virtual Bool SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName ) override;

};


BEGIN_CLASS_RTTI( CPatchBuilder_TextureCache );
	PARENT_CLASS( IBasePatchContentBuilder );
END_CLASS_RTTI();
