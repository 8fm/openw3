/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/cookedStrings.h"

//-----------------------------------------------------------------------------

struct TPatchStringCachePair
{
	CCookedStrings	m_cache;
	String			m_lang;
};

//-----------------------------------------------------------------------------

class CPatchStringToken : public IBasePatchContentBuilder::IContentToken
{
public:

	CPatchStringToken( Uint64 hash, Uint64 crc, Uint64 size, TPatchStringCachePair* cache );
	virtual ~CPatchStringToken( );

	virtual const Uint64 GetTokenHash( ) const override;
	virtual const Uint64 GetDataCRC( ) const override;
	virtual const Uint64 GetDataSize( ) const override;
	const String GetInfo( ) const override;
	void DebugDump( const String& dumpPath, const Bool isBase ) const override;

	// Member m_id is built as follows,
	//  - bits 63-33 : MSBs of the hashed name of the language.
	//  - bit     32 : Set if the entry is an index, clear if it's a string.
	//  - bits 31-00 : Unencrypted string id or index id.

	Uint64						m_id;
	Uint64						m_crc;
	Uint64						m_size;
	TPatchStringCachePair*		m_cache;
};

//-----------------------------------------------------------------------------

class CPatchStringsCache : public IBasePatchContentBuilder::IContentGroup
{
public:

	virtual ~CPatchStringsCache( );

	virtual void GetTokens( TDynArray< IBasePatchContentBuilder::IContentToken* >& outTokens ) const override;
	virtual const Uint64 GetDataSize( ) const override;
	virtual const String GetInfo( ) const override;
	static CPatchStringsCache* LoadContent( const String& baseDirectory );

private:

	TDynArray< TPatchStringCachePair* >	m_caches;
	TDynArray< CPatchStringToken* >		m_tokens;
};

//-----------------------------------------------------------------------------

class CPatchBuilder_Strings : public IBasePatchContentBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( CPatchBuilder_Strings );

public:

	CPatchBuilder_Strings( );
	~CPatchBuilder_Strings( );

	virtual String GetContentType( ) const override;
	virtual IContentGroup* LoadContent( const ECookingPlatform platform, const String& absoluteBuildPath )  override;
	virtual Bool SaveContent( const ECookingPlatform platform, IContentGroup* baseGroup, IContentGroup* patchGroup, TDynArray< IContentToken* >& patchContent, const String& absoluteBuildPath, const String& patchName ) override;

private:

	CPatchStringsCache* m_patchStringsCache;
};

BEGIN_CLASS_RTTI( CPatchBuilder_Strings );
	PARENT_CLASS( IBasePatchContentBuilder );
END_CLASS_RTTI();

//-----------------------------------------------------------------------------