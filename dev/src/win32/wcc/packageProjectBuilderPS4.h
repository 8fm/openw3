/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/redSystem/utility.h"
#include "../../common/core/contentType.h"
#include "packageConstants.h"
#include "packageChunkLayerPairPS4.h"
#include "packageBatchCommandWriterPS4.h"

class CPackageBatchCommandWriterPS4;
struct SPackageSkuPS4;
struct SPackageFiles;
struct SPackageLanguagesPS4;

struct SPackageProjectOptionsPS4
{
	String	m_elfName;					//!< The ELF to use as eboot.bin. Must have been found in the bin directory.
	Uint32	m_launchContentNumber;		//!< Launch chunk. E.g., if "bin, content0, content1, content3" then 3
	Bool	m_compressInstallerDataFile:1;
	Bool	m_compressBundleFiles:1;
	Bool	m_compressCacheFiles:1;
	Bool	m_compressScriptFiles:1;
	Bool	m_compressStringsFiles:1;
	Bool	m_compressSpeechFiles:1;
	Bool	m_compressContentManifests:1;
	Bool	m_languageMaskSpeech:1;
	Bool	m_languageMaskStrings:1;

	SPackageProjectOptionsPS4( const String& elfName )
		: m_elfName( elfName )
		, m_launchContentNumber(0xFFFFFFFF) // everything in initial chunk by default
		, m_compressInstallerDataFile( false )
		, m_compressBundleFiles( false )
		, m_compressCacheFiles( false )
		, m_compressScriptFiles( false )
		, m_compressStringsFiles( false )
		, m_compressSpeechFiles( false )
		, m_compressContentManifests( false )
		, m_languageMaskSpeech( false )
		, m_languageMaskStrings( false )
	{}
};

struct SPackageContentManifestPS4
{
	CName				m_contentName;
	CName				m_baseContentName;
	String				m_language;
	Uint32				m_installOrder;
	Uint32				m_chunkID;
	TDynArray< String > m_conformedFilePaths;
	Bool				m_isLaunchContent;

	SPackageContentManifestPS4()
		: m_installOrder(0xFFFFFFFF)
		, m_chunkID(0xFFFFFFFF)
		, m_isLaunchContent( false )
	{}
};

struct SScopedDummyBatchCommandWriterController;

class CPackageProjectBuilderPS4 : private Red::System::NonCopyable
{
	friend SScopedDummyBatchCommandWriterController;

private:
	enum EFileCompression
	{
		eFileCompression_Disabled,
		eFileCompression_Enabled,
	};

	struct SGameContentLanguage
	{
		String m_gameLanguage;
		Uint32 m_contentNumber;

		Bool operator==( const SGameContentLanguage& rhs ) const
			{ return m_gameLanguage == rhs.m_gameLanguage && m_contentNumber == rhs.m_contentNumber; }

		RED_FORCE_INLINE Uint32 CalcHash() const { return m_gameLanguage.CalcHash() ^ m_contentNumber; }

		SGameContentLanguage()
			: m_contentNumber(0xFFFFFFFF)
		{}

		SGameContentLanguage( const String& gameLanguage, Uint32 contentNumber )
			: m_gameLanguage( gameLanguage )
			, m_contentNumber( contentNumber )
		{}
	};

	struct SContentManifestBuildInfo
	{
		CName							m_contentName;
		CName							m_baseContentName;
		SChunkLayerInfoPS4				m_chunkLayerPair;
		String							m_language;
		TDynArray< String >				m_conformedPaths;

		SContentManifestBuildInfo()
		{}

		SContentManifestBuildInfo( CName contentName, CName baseContentName, SChunkLayerInfoPS4 chunkLayerPair, const String& language )
			: m_contentName( contentName )
			, m_baseContentName( baseContentName )
			, m_chunkLayerPair( chunkLayerPair )
			, m_language( language )
		{}

		Bool operator==( const SContentManifestBuildInfo& rhs ) const { return m_contentName == rhs.m_contentName; }
		Bool operator!=( const SContentManifestBuildInfo& rhs ) const { return !(*this == rhs ); }
		Bool operator<(const SContentManifestBuildInfo& rhs ) const { return m_contentName < rhs.m_contentName; }
	};

	typedef Uint32 ContentNumber;
	typedef THashMap< SGameContentLanguage, SChunkLayerInfoPS4 >	TLanguageMap;
	typedef THashMap< ContentNumber, SChunkLayerInfoPS4 >			TGameContentMap;
	typedef THashMap< ContentNumber, SChunkLayerInfoPS4 >			TGamePatchMap;
	typedef THashMap< CName, SContentManifestBuildInfo >			TContentManifestMap;

private:
	CPackageBatchCommandWriterPS4		m_batchCommandWriter;
	const SPackageProjectOptionsPS4&	m_packageOptions;
	const SPackageSkuPS4&				m_packageSku;

private:
	TContentManifestMap					m_contentManifestMap;
	TDynArray< Uint32 >					m_installOrderChunkIDs;
	TBitSet< MAX_CHUNK_ID + 1 >			m_isLaunchChunk;
	EContentType						m_contentType;

	CName								m_manifestContentName;

public:
	CPackageProjectBuilderPS4( IFile& batchFileWriter, const SPackageProjectOptionsPS4& packageOptions, const SPackageSkuPS4& packageSku, const CName& manifestContentName );
	
public:
	Bool BuildProject( TDynArray< SPackageContentManifestPS4 >& outPackageContentManifests );

private:
	Bool BuildAppProject( TDynArray< SPackageContentManifestPS4 >& outPackageContentManifests );
	Bool BuildPatchProject( TDynArray< SPackageContentManifestPS4 >& outPackageContentManifests );
	Bool BuildDlcProject( TDynArray< SPackageContentManifestPS4 >& outPackageContentManifests );

private:
	Bool InitProject( EContentType type );

private:
	Bool AddBaseFiles( const SChunkLayerInfoPS4& chunkLayerPair );
	Bool AddPrefetchFiles( const SChunkLayerInfoPS4& chunkLayerPair );
	Bool AddGameLanguageMaskFiles( const TLanguageMap& langMap );
	Bool AddGameContentFilesNoLanguageMask( const TGameContentMap& gameContentMap );
	
private:
	Bool AddGamePatchFiles( const TGamePatchMap& gamePatchMap );

private:
	Bool AddGameContentFilesForDlc();

private:
	Bool BatchAddLanguageMaskChunks( Uint32 skipContent, Uint32 maxContentCount, Bool chunkAscending, Uint32 startChunkID, Uint32 layer, TLanguageMap& outLangMap, Uint32& outNextFreeChunkID );
	Bool BatchAddGameContentChunksNoLanguageMask( Uint32 skipContent, Uint32 maxContentCount, Uint32 startChunkID, Uint32 layer, TGameContentMap& outGameContentMap, Uint32& outNextFreeChunkID );
	Bool BatchAddFillerChunks( Uint32 startChunkID, Uint32 endChunkID, Uint32 layer );

private:
	Bool BatchScenarioUpdate( Uint32 defaultScenarioID, Bool hasPrefetchChunk, const TGameContentMap& gameContentMap, const TLanguageMap& langMap, const String& label );

private:
	Bool BatchAddFiles( const TDynArray< String >& files, const SChunkLayerInfoPS4& chunkLayerPair, EFileCompression fileCompression );
	Bool BatchAddFile( const String& file, const SChunkLayerInfoPS4& chunkLayerPair, EFileCompression fileCompression );

private:
	Bool BatchAddFilesNoChunk( const TDynArray< String >& files, EFileCompression fileCompression );
	Bool BatchAddFileNoChunk( const String& file, EFileCompression fileCompression );

private:
	Bool ManifestAddFiles( CName manifestContentName, CName manifestBaseContentName, const TDynArray< String >& files, const SChunkLayerInfoPS4& chunkLayerPair, const String& language );
	Bool ManifestAddFile( CName manifestContentName, CName manifestBaseContentName, const String& file, const SChunkLayerInfoPS4& chunkLayerPair, const String& language );
	Bool ManifestAddFiles( CName manifestContentName, const TDynArray< String >& files, const SChunkLayerInfoPS4& chunkLayerPair );
	Bool ManifestAddFile( CName manifestContentName, const String& file, const SChunkLayerInfoPS4& chunkLayerPair  );

private:
	Bool GetGameLanguage( const String& languageFile, String& outGameLanguage ) const;
	String	GetContentPrefix() const;

private:
	Bool PopulateAllPackageFiles( const SPackageFiles& packageFiles, TDynArray< String >& outAllFiles );

private:
	Bool Validate( EPackageType packageType ) const;
	Bool ValidatePackageLanguages( const SPackageLanguagesPS4& packageLanguages ) const;
	Bool ValidateCommonPackageFiles( const SPackageFiles& packageFiles, EPackageType packageType ) const;
	Bool ValidateAppPackageFiles( const SPackageFiles& packageFiles ) const;
	Bool ValidatePatchPackageFiles( const SPackageFiles& packageFiles ) const;
	Bool ValidateDlcPackageFiles( const SPackageFiles& packageFiles ) const;
	void GenerateFileEntryStats( const SPackageFiles& packageFiles, Uint32& outNumFiles, Uint32& outNumDirs ) const;

private:
	Bool ValidateContentID( const String& contentID ) const;
	Bool ValidatePasscode( const String& passcode ) const;
	Bool ValidateEntitlementKey( const String& entitlementKey ) const;
};
