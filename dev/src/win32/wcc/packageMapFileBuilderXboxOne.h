/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/redSystem/utility.h"
#include "../../common/core/hashmap.h"

#include "packageConstants.h"
#include "packageSkuXboxOne.h"
#include "packageXmlWriterXboxOne.h"
#include "../../common/core/contentType.h"

class CPackageXmlWriterXboxOne;
struct SPackageSkuPS4;
struct SPackageFiles;

const Uint32 INVALID_CHUNK_ID = 0xFFFFFFFF;
const Uint32 ALIGNMENT_CHUNK_ID = 0x3FFFFFFF;
const Uint32 REGISTRATION_CHUNK_ID = INVALID_CHUNK_ID-1;

struct SPackageMapFileOptionsXboxOne
{
	String	m_exeName;					//!< The EXE to use. Must have been moved into the package root dir (beside AppxManifest.xml)
	Uint32	m_launchContentNumber;		//!< Launch chunk. E.g., if "bin, content0, content1, content3" then 3

	SPackageMapFileOptionsXboxOne( const String& exeName )
		: m_exeName( exeName )
		, m_launchContentNumber(0xFFFFFFFF) // everything in initial chunk by default
	{}
};

struct SPackageContentManifestXboxOne
{
	CName				m_contentName;
	CName				m_baseContentName;
	String				m_language;
	Uint32				m_installOrder;
	Uint32				m_chunkID;
	TDynArray< String > m_conformedFilePaths;
	Bool				m_isLaunchContent;

	SPackageContentManifestXboxOne()
		: m_installOrder(0xFFFFFFFF)
		, m_chunkID(0xFFFFFFFF)
		, m_isLaunchContent( false )
	{}
};

class CPackageMapFileBuilderXboxOne : private Red::System::NonCopyable
{
private:
	const SPackageMapFileOptionsXboxOne&	m_packageOptions;
	const SPackageSkuXboxOne&				m_packageSku;
	CPackageXmlWriterXboxOne				m_xmlWriter;
	CName									m_manifestContentName;
public:
	CPackageMapFileBuilderXboxOne( IFile& xmlFile, const SPackageMapFileOptionsXboxOne& packageOptions, const SPackageSkuXboxOne& packageSku, const CName& manifestContentName );

public:
	Bool BuildProject( TDynArray< SPackageContentManifestXboxOne >& outPackageContentManifests );

private:
	typedef Uint32 ChunkID;
	typedef Uint32 ContentID;

private:
	struct SGameContentLanguage
	{
		String		m_gameLanguage;
		ContentID	m_contentNumber;

		Bool operator==( const SGameContentLanguage& rhs ) const
		{ return m_gameLanguage == rhs.m_gameLanguage && m_contentNumber == rhs.m_contentNumber; }

		RED_FORCE_INLINE Uint32 CalcHash() const { return m_gameLanguage.CalcHash() ^ m_contentNumber; }

		SGameContentLanguage()
			: m_contentNumber(0xFFFFFFFF)
		{}

		SGameContentLanguage( const String& gameLanguage, ContentID contentNumber )
			: m_gameLanguage( gameLanguage )
			, m_contentNumber( contentNumber )
		{}
	};

	struct SContentManifestBuildInfo
	{
		CName							m_contentName;
		CName							m_baseContentName;
		ChunkID							m_chunkID;
		String							m_language;
		TDynArray< String >				m_conformedPaths;

		SContentManifestBuildInfo()
			: m_chunkID( INVALID_CHUNK_ID )
		{}

		SContentManifestBuildInfo( CName contentName, CName baseContentName, ChunkID chunkID, const String& language )
			: m_contentName( contentName )
			, m_baseContentName( baseContentName )
			, m_chunkID( chunkID )
			, m_language( language )
		{}

		Bool operator==( const SContentManifestBuildInfo& rhs ) const { return m_contentName == rhs.m_contentName; }
		Bool operator!=( const SContentManifestBuildInfo& rhs ) const { return !(*this == rhs ); }
		Bool operator<(const SContentManifestBuildInfo& rhs ) const { return m_contentName < rhs.m_contentName; }
	};

private:
	typedef THashMap< SGameContentLanguage, ChunkID >				TLanguageMap;
	typedef THashMap< ContentID, ChunkID >							TGameContentMap;
	typedef THashMap< ContentID, ChunkID >							TGamePatchMap;
	typedef THashMap< CName, SContentManifestBuildInfo >			TContentManifestMap;

private:
	TContentManifestMap						m_contentManifestMap;
	EContentType							m_contentType;
	TBitSet< MAX_USED_CHUNK_ID_XBOX + 1 >	m_isLaunchChunk;
	TDynArray< Uint32 >						m_installOrderChunkIDs;

private:
	Bool AddBinChunk( ChunkID binChunkID, ChunkID& outNextFreeChunkID );
	Bool AddPatchChunks( ChunkID startChunkID, Uint32 numPatchChunkds, TGamePatchMap& outGamePatchMap, ChunkID& outNextFreeChunkID );
	Bool AddLanguageChunks( ChunkID startChunkID, TLanguageMap& outLangMap, ChunkID& outNextFreeChunkID );
	Bool AddGameContentChunksNoLanguages( ChunkID startChunkID, TGameContentMap& outGameContentMap, ChunkID& outNextFreeChunkID, Bool forceSpeeches = false );
	Bool AddBaseFiles( ChunkID chunkID );
	Bool AddGameLanguageFiles( const TLanguageMap& langMap );
	Bool AddGameContentFilesNoLanguages( const TGameContentMap& gameContentMap, Bool forceSpeeches = false );
	Bool AddGamePatchFiles( const TGamePatchMap& gamePatchMap );


private:
	Bool AddFiles( const TDynArray< String >& files, ChunkID chunkID );
	Bool AddFile( const String& file, ChunkID chunkID );

private:
	Bool ManifestAddFiles( CName manifestContentName, CName manifestBaseContentName, const TDynArray< String >& files, ChunkID chunkID, const String& language );
	Bool ManifestAddFile( CName manifestContentName, CName manifestBaseContentName, const String& file, ChunkID chunkID, const String& language );
	Bool ManifestAddFiles( CName manifestContentName, const TDynArray< String >& files, ChunkID chunkID );
	Bool ManifestAddFile( CName manifestContentName, const String& file, ChunkID chunkID );

private:
	Bool GetGameLanguage( const String& languageFile, String& outGameLanguage ) const;
	String GetContentPrefix() const;
};
