/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/cookedSpeeches.h"

//-----------------------------------------------------------------------------

struct TPatchSpeechCacheTrio
{
	CCookedSpeeches	m_cache;
	String			m_lang;
	IFile*			m_file;
};

namespace PatchSpeechHelper
{
	void CleanRIFFHeader( void* buffer );
}

//-----------------------------------------------------------------------------

class CPatchSpeechToken : public IBasePatchContentBuilder::IContentToken
{
public:

	CPatchSpeechToken( Uint64 hash, Uint64 crc, Uint64 size, TPatchSpeechCacheTrio* cache );
	virtual ~CPatchSpeechToken( );

	virtual const Uint64 GetTokenHash( ) const override;
	virtual const Uint64 GetDataCRC( ) const override;
	virtual const Uint64 GetDataSize( ) const override;
	const String GetInfo( ) const override;
	void DebugDump( const String& dumpPath, const Bool isBase ) const override;

	// Member m_id is built as follows,
	//  - bits 63-32 : Hashed name of the language.
	//  - bits 31-00 : Unencrypted string id or index id.

	Uint64						m_id;
	Uint64						m_crc;
	Uint64						m_size;
	TPatchSpeechCacheTrio*		m_cache;
};

//-----------------------------------------------------------------------------

class CPatchSpeechesCache : public IBasePatchContentBuilder::IContentGroup
{
public:

	virtual ~CPatchSpeechesCache( );

	virtual void GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const override;
	virtual const Uint64 GetDataSize( ) const override;
	virtual const String GetInfo( ) const override;
	static CPatchSpeechesCache* LoadContent( const String& baseDirectory );

private:

	TDynArray< TPatchSpeechCacheTrio* >	m_caches;
	TDynArray< CPatchSpeechToken* >		m_tokens;
};

//-----------------------------------------------------------------------------

class CPatchBuilder_Speeches : public IBasePatchContentBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( CPatchBuilder_Speeches );

public:

	CPatchBuilder_Speeches( );
	~CPatchBuilder_Speeches( );

	virtual String GetContentType( ) const override;
	virtual IContentGroup* LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )  override;
	virtual Bool SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName ) override;

private:

	CPatchSpeechesCache* m_patchSpeechesCache;
};

BEGIN_CLASS_RTTI( CPatchBuilder_Speeches );
	PARENT_CLASS( IBasePatchContentBuilder );
END_CLASS_RTTI();

//-----------------------------------------------------------------------------