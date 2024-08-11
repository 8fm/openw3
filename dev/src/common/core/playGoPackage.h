/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "hashmap.h"
#include "sortedarray.h"

//////////////////////////////////////////////////////////////////////////
// TPlayGoLanguageFlag
//////////////////////////////////////////////////////////////////////////
typedef Uint64 TPlayGoLanguageFlag;
const TPlayGoLanguageFlag PGLF_Invalid			= 0ULL;
const TPlayGoLanguageFlag PGLF_AllLanguages		= 0xFFFFFFFFFFFFFFFFULL;

static const Uint32 INVALID_PLAYGO_CHUNK_ID = 0xFFFFFFFF;
static const Uint32 BIN_PLAYGO_CHUNK_ID = 0;

struct SPlayGoChunk
{
	TPlayGoLanguageFlag m_languageFlag;
	Uint32				m_chunkID;

	SPlayGoChunk()
		: m_languageFlag( PGLF_Invalid )
		, m_chunkID( INVALID_PLAYGO_CHUNK_ID )
	{}

	SPlayGoChunk( TPlayGoLanguageFlag languageFlag, Uint32 chunkID )
		: m_languageFlag( languageFlag )
		, m_chunkID( chunkID )
	{}

	Bool operator==( const SPlayGoChunk& rhs ) const { return m_chunkID == rhs.m_chunkID; }
};

struct SPlayGoContent
{
	CName								m_contentName;				//!< E.g., 'content0'
	TDynArray< SPlayGoChunk >			m_installOrderChunks;		//!< Split chunks in install order

	SPlayGoContent()
	{}

	SPlayGoContent( CName contentName, const TDynArray< SPlayGoChunk >& installOrderChunks )
		: m_contentName( contentName )
		, m_installOrderChunks( installOrderChunks )
	{}

	Bool operator==( const SPlayGoContent& rhs ) const { return m_contentName == rhs.m_contentName; }
	Bool operator!=( const SPlayGoContent& rhs ) const { return !(*this == rhs); }
	Bool operator<( const SPlayGoContent& rhs ) const { return m_contentName < rhs.m_contentName; }
};

//////////////////////////////////////////////////////////////////////////
// SPlayGoPackage
//////////////////////////////////////////////////////////////////////////
struct SPlayGoPackageHeader
{
	static const Uint32 MAGIC = 'PLGO';
	static const Uint32 VERSION = 1;

	Uint32 m_magic;
	Uint32 m_version;

	SPlayGoPackageHeader()
		: m_magic( MAGIC )
		, m_version( VERSION )
	{}
};

//////////////////////////////////////////////////////////////////////////
// SPlayGoPackage
//////////////////////////////////////////////////////////////////////////
struct SPlayGoPackage
{
	SPlayGoPackageHeader											m_header;
	TArrayMap< StringAnsi, Uint64 >									m_languageFlagMap;			//!< Map of game language names to PlayGo flags. E.g., "en" -> PGLF_English_UnitedStates
	TArrayMap< Uint32 , CName >										m_chunkNameMap;				//!< Map of PlayGo chunk to chunk name, including lanuage. E.g., 99 -> 'content0_en'
	TDynArray< SPlayGoContent >										m_installOrderContent;		//!< List of non-language masked content in install order
	TDynArray< StringAnsi >											m_supportedSpeechLanguages;	//!< E.g., "en", "pl" for *.w3speech
	TDynArray< StringAnsi >											m_supportedTextLanguages;	//!< E.g., "en", "pl" for *.w3strings
	StringAnsi														m_defaultSpeechLanguage;	//!< E.g., "en"
	CName															m_launchContentName;		//!< Aliased by launch0
	Uint32															m_patchLevel;				//!< Zero for base game, incremented in each patch. Possible for use in save game prerequesite.

	SPlayGoPackage()
		: m_patchLevel(0)
	{}
};

#ifdef RED_LOGGING_ENABLED
void DumpPlayGoPackage( const SPlayGoPackage& package );
#endif

//////////////////////////////////////////////////////////////////////////
// Serialization
//////////////////////////////////////////////////////////////////////////
IFile& operator<<( IFile& ar, SPlayGoChunk& chunk );
IFile& operator<<( IFile& ar, SPlayGoContent& content );
IFile& operator<<( IFile& ar, SPlayGoPackageHeader& header );
IFile& operator<<( IFile& ar, SPlayGoPackage& package );
