/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/core/2darray.h"
#include "../../common/core/fileSys.h"
#include "../../common/core/scopedPtr.h"
#include "../../common/core/contentManifest.h"
#include "../../common/core/xmlReader.h"

#include "packageProjectBuilderPS4.h"
#include "packageConstants.h"
#include "packageFiles.h"
#include "packageLanguagesPS4.h"
#include "packageSkuPS4.h"
#include "packagePlayGoChunksBuilder.h"

const Uint32 NUM_PREFETCH_PATCH_CHUNKS = 16;

static String MakeForwardSlash( const String& path )
{
	String conformedPath = path;
	conformedPath.ReplaceAll(TXT('\\'), TXT('/'));
	return conformedPath;
}

static String MakeBackslash( const String& path )
{
	String conformedPath = path;
	conformedPath.ReplaceAll(TXT('/'), TXT('\\'));
	return conformedPath;
}

static Bool HasUnmaskedContent( const SPackageGameFiles& gameFiles, const SPackageProjectOptionsPS4& packageOptions )
{
	const Bool retval =
		!gameFiles.m_bundleFiles.Empty() ||
		(!gameFiles.m_stringsFiles.Empty() && !packageOptions.m_languageMaskStrings) ||
		(!gameFiles.m_speechFiles.Empty() && !packageOptions.m_languageMaskSpeech) ||
		!gameFiles.m_scriptFiles.Empty() ||
		!gameFiles.m_cacheFiles.Empty() ||
		!gameFiles.m_miscFiles.Empty();

	return retval;
}

CPackageProjectBuilderPS4::CPackageProjectBuilderPS4( IFile& batchFileWriter, const SPackageProjectOptionsPS4& packageOptions, const SPackageSkuPS4& packageSku, const CName& manifestContentName )
	: m_batchCommandWriter( batchFileWriter, packageSku.m_packageLanguages )
	, m_packageOptions( packageOptions )
	, m_packageSku( packageSku )
	, m_contentType( eContentType_Invalid )
	, m_manifestContentName( manifestContentName )
{
}

struct Chunk
{
	static Bool Split( const String& str, String& outPrefix, Uint32& outNumber )
	{
		const Char* ch = str.AsChar();
		for(; *ch; ++ch )
		{
			if ( *ch >= '0' && *ch <= '9' )
				break;
		}

		if ( !*ch )
		{
			return false; // no number
		}

		const Char* const prefixEndPlusOne = ch;

		Uint32 num = 0;
		if ( !GParseInteger( ch, num ) )
		{
			return false;
		}

		const size_t prefixLen = prefixEndPlusOne - str.AsChar();
		outPrefix.Set( str.AsChar(), prefixLen );
		outNumber = num;
		return true;
	}
};

Bool CPackageProjectBuilderPS4::InitProject( EContentType type )
{
	m_contentManifestMap.Clear();
	m_installOrderChunkIDs.Clear();
	m_isLaunchChunk.ClearAll();

	// TODO: validate that all required speech, strings, and sce_sys were present for the SKU supported languages
	if ( ! Validate( m_packageSku.m_packageType ) )
	{
		return false;
	}

	m_contentType = type;

	return true;
}

Bool CPackageProjectBuilderPS4::BuildPatchProject( TDynArray< SPackageContentManifestPS4 >& outPackageContentManifests )
{
	if ( !InitProject( eContentType_Patch ) )
	{
		return false;
	}

 	const PackageHelpers::SPackageInfo& basePackage = m_packageSku.m_patchParams.m_basePackage;

	const Uint32 INVALID_CHUNK_ID = 0xFFFFFFFF;
	const Uint32 binChunkID = 0;
	const Uint32 binLayer = 0;
	Uint32 prefetchChunkID = INVALID_CHUNK_ID;
	TGamePatchMap gamePatchMap;
	TGameContentMap gameContentMap;
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;

	const size_t patchLen = Red::System::StringLengthCompileTime(TXT("patch"));
	const size_t contentLen = Red::System::StringLengthCompileTime(TXT("content"));

	for ( const auto& it : basePackage.m_chunks )
	{
		if ( it.m_label.EqualsNC(TXT("bin")))
		{
			if ( binChunkID != it.m_chunkID || binLayer != it.m_layer )
			{
				ERR_WCC(TXT("unexpected bin chunkID/layer (%u/%u)"), it.m_chunkID, it.m_layer);
				return false;
			}
		}
		else if ( it.m_label.EqualsNC(TXT("content0_prefetch")) )
		{
			prefetchChunkID = it.m_chunkID;
			if ( it.m_layer != binLayer )
			{
				ERR_WCC(TXT("Unexpected prefetch layer %u"), it.m_layer);
				return false;
			}
		}
		else if ( Red::System::StringCompareNoCase( it.m_label.AsChar(), TXT("content"), contentLen ) == 0 
			&& !it.m_label.ContainsCharacter(TXT('_')) )
		{
			// If content, but not language content (underscore check)

			Uint32 contentNumber = 0;
			const Char* ch = it.m_label.AsChar() + contentLen;
			if ( !GParseInteger( ch, contentNumber ) )
			{
				ERR_WCC(TXT("Failed to parse content number from label '%ls'"),it.m_label.AsChar());
				return false;
			}

			LOG_WCC(TXT("Mapping content%u to chunkID %u"), contentNumber, it.m_chunkID);

			const SChunkLayerInfoPS4 chunkLayerPair( it.m_chunkID, it.m_layer );
			gameContentMap.Insert( contentNumber, chunkLayerPair );
		}
		else if ( Red::System::StringCompareNoCase( it.m_label.AsChar(), TXT("patch"), patchLen ) == 0 )
		{
			Uint32 patchNumber = 0;
			const Char* ch = it.m_label.AsChar() + patchLen;
			if ( !GParseInteger(ch, patchNumber) )
			{
				ERR_WCC(TXT("Failed to parse patch number from label '%ls'"),it.m_label.AsChar());
				return false;
			}
			const SChunkLayerInfoPS4 chunkLayerPair( it.m_chunkID, it.m_layer );
			gamePatchMap.Insert( patchNumber, chunkLayerPair );
		}
	}

	const SChunkLayerInfoPS4 binChunkLayerPair( binChunkID, binLayer );

	if ( ! m_batchCommandWriter.CreatePatchPackage( m_packageSku.m_contentID, m_packageSku.m_passcode, m_packageSku.m_patchParams ) )
	{
		return false;
	}

	if ( ! m_batchCommandWriter.PlayGoUpdateWithRawPlayGoLanguages( basePackage.m_rawSupportedPlayGoLanguages, basePackage.m_rawDefaultPlayGoLanguage ) )
	{
		return false;
	}

	// Because chunk zero magically already exists
	if ( ! m_batchCommandWriter.ChunkUpdate( binChunkLayerPair, TXT("bin") ) )
	{
		return false;
	}

	for ( const auto& it : basePackage.m_chunks )
	{
		// Already automagically exists
		if ( it.m_chunkID == binChunkID )
		{
			continue;
		}

		const SChunkLayerInfoPS4 chunkLayerPair( it.m_chunkID, it.m_layer );
		if ( !m_batchCommandWriter.ChunkAddWithRawPlayGoLanguages( chunkLayerPair, it.m_rawPlayGoLanguages, it.m_label ) )
		{
			return false;
		}
	}

	const PackageHelpers::SPackageScenario& scenario = basePackage.m_scenario;
	if ( !m_batchCommandWriter.ScenarioUpdate( scenario.m_scenarioID, scenario.m_numInitialChunks, scenario.m_installOrderChunkIDs, scenario.m_label ) )
	{
		return false;
	}

	// Add even if a patch file, just in case changes
	if ( ! BatchAddFile( PLAYGO_DAT_FILE, binChunkLayerPair, eFileCompression_Disabled ) )
	{
		ERR_WCC(TXT("Failed to add PlayGo dat file '%ls'"), PLAYGO_DAT_FILE );
		return false;
	}
	
	// Manifest file *should* have changed, in any case, not a big file.
	const String manifestPath = String::Printf(TXT("content\\%ls"), MANIFEST_FILE_NAME);
	if ( ! BatchAddFile( manifestPath, binChunkLayerPair, eFileCompression_Disabled ) )
	{
		ERR_WCC(TXT("Failed to add manifest file '%ls'"), manifestPath.AsChar() );
		return false;
	}

	if ( ! AddBaseFiles( binChunkLayerPair ) )
	{
		return false;
	}

	if ( prefetchChunkID != INVALID_CHUNK_ID )
	{
		const SChunkLayerInfoPS4 prefetchChunkLayerPair( prefetchChunkID, binLayer );
		// Adding directly to avoid manifest generation. Should only be REPLACING these files.
		if ( !BatchAddFiles( packageFiles.m_prefetchFiles, prefetchChunkLayerPair, eFileCompression_Disabled ) )
		{
			return false;
		}
	}

	// Add any new contentX stuff that isn't aware of the patch mechanism or just needs outright replacing instead of going into a patch blob
	// Also add to manifest, since it'll get merged and new files need to be added to it, not just ones replaced
	for ( const auto& it : packageFiles.m_gameContentFilesMap )
	{
		const Uint32 sortedContentNumber = it.m_first;
		const SPackageGameFiles& gameFiles = it.m_second;

		SChunkLayerInfoPS4 chunkLayerPair;
		if ( !gameContentMap.Find( sortedContentNumber, chunkLayerPair ) )
		{
			ERR_WCC(TXT("Failed to find chunkLayerPair for content%u!"), sortedContentNumber );
			return false;
		}

		// Includes any new languages added. No language masking.
		const TDynArray<String>* ar[] = { &gameFiles.m_bundleFiles, &gameFiles.m_scriptFiles, &gameFiles.m_cacheFiles, 
			&gameFiles.m_miscFiles, &gameFiles.m_speechFiles, &gameFiles.m_stringsFiles };

		TDynArray<String> mergedFiles;
		for ( Uint32 i = 0; i < ARRAY_COUNT(ar); ++i )
		{
			mergedFiles.PushBack(*ar[i]);
		}

		const CName manifestContentName(String::Printf(TXT("%ls%u"), GetContentPrefix().AsChar(), sortedContentNumber ) );

		if ( ! BatchAddFiles( mergedFiles, chunkLayerPair, eFileCompression_Disabled ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, mergedFiles, chunkLayerPair ) )
		{
			return false;
		}
	}

	// Possible to have no "patchX" files, just replaced files above
	if ( !AddGamePatchFiles( gamePatchMap ) )
	{
		return false;
	}

	for ( auto it = m_contentManifestMap.Begin(); it != m_contentManifestMap.End(); ++it )
	{
		const CName manifestConentName = it->m_first;
		const SContentManifestBuildInfo& manifestInfo = it->m_second;

		SPackageContentManifestPS4 contentManifest;
		contentManifest.m_contentName = manifestConentName;
		contentManifest.m_baseContentName = manifestInfo.m_baseContentName;
		contentManifest.m_chunkID = manifestInfo.m_chunkLayerPair.m_chunkID;
		contentManifest.m_conformedFilePaths = manifestInfo.m_conformedPaths;
 		contentManifest.m_language = String::EMPTY;
 		contentManifest.m_isLaunchContent = true;
	
		outPackageContentManifests.PushBack( Move( contentManifest ) );
	}

	m_batchCommandWriter.SortFiles();

	return true;
}

Bool CPackageProjectBuilderPS4::BuildProject( TDynArray< SPackageContentManifestPS4 >& outPackageContentManifests )
{
	switch ( m_packageSku.m_packageType )
	{
	case ePackageType_App:		return BuildAppProject( outPackageContentManifests );
	case ePackageType_Patch:	return BuildPatchProject( outPackageContentManifests );
	case ePackageType_Dlc:		return BuildDlcProject( outPackageContentManifests );
	default:
		return false;
	}

	return false;
}

Bool CPackageProjectBuilderPS4::BuildAppProject( TDynArray< SPackageContentManifestPS4 >& outPackageContentManifests )
{
	if ( !InitProject( eContentType_App ) )
	{
		return false;
	}

	// TODO: Updated in creating package plan!
	// Splitter should create a layer0 and layer1 directory
	// Probably put all extra chunks on layer zero, at least like digigtal distrib and what sense do layers make in a patch?
	const Uint32 binChunkID = 0;
	const Uint32 binLayer = 0;
	const Uint32 startChunkID = 1;
	const Uint32 defaultScenarioID = 0;
	const Uint32 layer = 0;
	const Uint32 languageLayer = 1;

	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;
	const SPackageLanguagesPS4 packageLanguages = m_packageSku.m_packageLanguages;

	if ( ! m_batchCommandWriter.CreateAppPackage( m_packageSku.m_contentID, m_packageSku.m_passcode ) )
	{
		return false;
	}

	if ( ! m_batchCommandWriter.PlayGoUpdate( packageLanguages.m_supportedSpeechLanguages, packageLanguages.m_defaultSpeechLanguage ) )
	{
		return false;
	}

	// Because chunk zero magically already exists
	if ( ! m_batchCommandWriter.ChunkUpdate( SChunkLayerInfoPS4( binChunkID, layer ), TXT("bin") ) )
	{
		return false;
	}

	/* Generate a layout for bluray like this

	Layer 0: bin content0 content1 content0_langs content1_langs content2 content3 content4...
	Layer 1: content12_langs content11_langs etc

	For layer zero this is to try to move the initial payload onto the outer layer and make the game content as contiguous as possible
	The language-masked content shouldn't be staggered like bin content0 content0_langs content1 content1_langs because then we're leaving
	gaps on the outer disc for languages that are masked out

	On layer one, can't make all language content contiguous because then would be shafting some language by moving all its content into the inner layer

	*/

	const Uint32 blurayInitContentCount = 2;
	Uint32 nextFreeChunkID = startChunkID;
	TGameContentMap gameContentMap;
	TLanguageMap langMap;

	// Another hack for retrofitting in patch prefetch chunks
	// When we create a patch package, we'll put files in the project
	// but for now just reserve the chunks and installer.dat
	const Uint32 patchChunkIDStart = nextFreeChunkID;
	for ( Uint32 i = 0; i < NUM_PREFETCH_PATCH_CHUNKS; ++i )
	{
		const Uint32 inOrderPatchNumber = i;
		const Uint32 patchChunkID = nextFreeChunkID++;
		SChunkLayerInfoPS4 prefetchPatchChunkLayerPair( patchChunkID, binLayer );
		if ( ! m_batchCommandWriter.ChunkAdd( prefetchPatchChunkLayerPair, String::Printf(TXT("patch%u"), inOrderPatchNumber) ) )
		{
			return false;
		}
	}

	// Hack for retrofitting in prefetch chunks to help guide disc position
	Uint32 prefetchChunkID = 0xFFFFFFFF;
	const Bool hasPrefetchChunk = !m_packageSku.m_packageFiles.m_prefetchFiles.Empty();
	if ( hasPrefetchChunk )
	{
		prefetchChunkID = nextFreeChunkID++;
		SChunkLayerInfoPS4 prefetchChunkLayerPair( prefetchChunkID, binLayer );
		if ( ! m_batchCommandWriter.ChunkAdd( prefetchChunkLayerPair, TXT("content0_prefetch")) )
		{
			return false;
		}
	}

	if ( ! BatchAddGameContentChunksNoLanguageMask( 0, blurayInitContentCount, nextFreeChunkID, layer, gameContentMap, nextFreeChunkID ) )
	{
		return false;
	}

	if ( ! BatchAddLanguageMaskChunks( 0, blurayInitContentCount, true, nextFreeChunkID, binLayer, langMap, nextFreeChunkID ) )
	{
		return false;
	}

	if ( nextFreeChunkID > MAX_CHUNK_ID )
	{
		ERR_WCC(TXT("Exceeded number of allowable chunks generating language mask chunks!"));
		return false;
	}

	if ( ! BatchAddGameContentChunksNoLanguageMask( blurayInitContentCount, 999, nextFreeChunkID, layer, gameContentMap, nextFreeChunkID ) )
	{
		return false;
	}

	if ( ! BatchAddLanguageMaskChunks( blurayInitContentCount, 999, true, nextFreeChunkID, languageLayer, langMap, nextFreeChunkID ) )
	{
		return false;
	}

	if ( nextFreeChunkID > MAX_CHUNK_ID )
	{
		ERR_WCC(TXT("Exceeded number of allowable chunks generating language mask chunks!"));
		return false;
	}

	const Uint32 fillerChunkIDStart = nextFreeChunkID;
	const Uint32 fillerChunkIDEnd = MAX_CHUNK_ID;
	if ( ! BatchAddFillerChunks( fillerChunkIDStart, fillerChunkIDEnd, layer ) )
	{
		return false;
	}

	if ( ! BatchScenarioUpdate( defaultScenarioID, hasPrefetchChunk, gameContentMap, langMap, TXT("Single Player") ) )
	{
		return false;
	}

	const EFileCompression installerDatFileCompression = m_packageOptions.m_compressInstallerDataFile ? eFileCompression_Enabled : eFileCompression_Disabled;
	const CName binManifestContentName( String::Printf(TXT("%ls0"), GetContentPrefix().AsChar()) );

	// Add even if a patch file, just in case changes
	if ( ! BatchAddFile( PLAYGO_DAT_FILE, SChunkLayerInfoPS4( binChunkID, layer ), installerDatFileCompression ) )
	{
		ERR_WCC(TXT("Failed to add PlayGo dat file '%ls'"), PLAYGO_DAT_FILE );
		return false;
	}

	const String manifestPath = String::Printf(TXT("content\\%ls"), MANIFEST_FILE_NAME);
	if ( ! BatchAddFile( manifestPath, SChunkLayerInfoPS4( binChunkID, layer ), eFileCompression_Disabled ) )
	{
		ERR_WCC(TXT("Failed to add manifest file '%ls'"), manifestPath.AsChar() );
		return false;
	}

	if ( ! AddBaseFiles( SChunkLayerInfoPS4( binChunkID, binLayer ) ) )
	{
		return false;
	}

	if ( hasPrefetchChunk && ! AddPrefetchFiles( SChunkLayerInfoPS4( prefetchChunkID, binLayer ) ) )
	{
		return false;
	}

	if ( ! AddGameContentFilesNoLanguageMask( gameContentMap ) )
	{
		return false;
	}

	if ( ! AddGameLanguageMaskFiles( langMap ) )
	{
		return false;
	}
	
	// Add so reserved chunks as stubs, even if not a patch package
	for ( Uint32 i = 0; i < NUM_PREFETCH_PATCH_CHUNKS; ++i )
	{
		const CName patchContentName(CName(String::Printf(TXT("patch%u"), i)));
		const Uint32 patchChunkID = patchChunkIDStart + i;
		SPackageContentManifestPS4 contentManifest;
		contentManifest.m_contentName = patchContentName;
		contentManifest.m_baseContentName = patchContentName;
		contentManifest.m_chunkID = patchChunkID;
		contentManifest.m_isLaunchContent = m_isLaunchChunk.Get( patchChunkID );
		for ( Uint32 i = 0; i < m_installOrderChunkIDs.Size(); ++i )
		{
			if ( contentManifest.m_chunkID == m_installOrderChunkIDs[i] )
			{
				contentManifest.m_installOrder = i;
				break;
			}
		}

		outPackageContentManifests.PushBack( Move(contentManifest) );
	}

	for ( auto it = m_contentManifestMap.Begin(); it != m_contentManifestMap.End(); ++it )
	{
		const CName manifestConentName = it->m_first;
		const SContentManifestBuildInfo& manifestInfo = it->m_second;

		SPackageContentManifestPS4 contentManifest;
		contentManifest.m_contentName = manifestConentName;
		contentManifest.m_baseContentName = manifestInfo.m_baseContentName;
		contentManifest.m_chunkID = manifestInfo.m_chunkLayerPair.m_chunkID;
		contentManifest.m_conformedFilePaths = manifestInfo.m_conformedPaths;
		contentManifest.m_language = manifestInfo.m_language;
		contentManifest.m_isLaunchContent = m_isLaunchChunk.Get( manifestInfo.m_chunkLayerPair.m_chunkID );
		for ( Uint32 i = 0; i < m_installOrderChunkIDs.Size(); ++i )
		{
			if ( contentManifest.m_chunkID == m_installOrderChunkIDs[i] )
			{
				contentManifest.m_installOrder = i;
				break;
			}
		}

		outPackageContentManifests.PushBack( Move( contentManifest ) );
	}

	m_batchCommandWriter.SortFiles();

	return true;
}

Bool CPackageProjectBuilderPS4::BuildDlcProject( TDynArray< SPackageContentManifestPS4 >& outPackageContentManifests )
{
	if ( !InitProject( eContentType_Dlc ) )
	{
		return false;
	}

	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;
	const SPackageLanguagesPS4 packageLanguages = m_packageSku.m_packageLanguages;

	if ( ! m_batchCommandWriter.CreateDlcPackage( m_packageSku.m_contentID, m_packageSku.m_passcode, m_packageSku.m_entitlementKey ) )
	{
		return false;
	}

	const String manifestPath = String::Printf(TXT("content\\%ls"), MANIFEST_FILE_NAME);
	if ( ! BatchAddFileNoChunk( manifestPath, eFileCompression_Disabled ) )
	{
		ERR_WCC(TXT("Failed to add manifest file '%ls'"), MANIFEST_FILE_NAME );
		return false;
	}

	for ( const String& sysFile : packageFiles.m_sysFiles )
	{
		const String& origPath = sysFile;
		if ( ! m_batchCommandWriter.AddFileNoChunk( origPath, MakeForwardSlash( origPath ) ) )
		{
			return false;
		}
	}

	if ( ! AddGameContentFilesForDlc() )
	{
		return false;
	}

	for ( auto it = m_contentManifestMap.Begin(); it != m_contentManifestMap.End(); ++it )
	{
		const CName manifestConentName = it->m_first;
		const SContentManifestBuildInfo& manifestInfo = it->m_second;

		SPackageContentManifestPS4 contentManifest;
		contentManifest.m_contentName = manifestConentName;
		contentManifest.m_baseContentName = manifestInfo.m_baseContentName;
		contentManifest.m_chunkID = manifestInfo.m_chunkLayerPair.m_chunkID;
		contentManifest.m_conformedFilePaths = manifestInfo.m_conformedPaths;
		contentManifest.m_language = manifestInfo.m_language;

		outPackageContentManifests.PushBack( Move( contentManifest ) );
	}

	return true;
}

Bool CPackageProjectBuilderPS4::BatchScenarioUpdate( Uint32 defaultScenarioID, Bool hasPrefetchChunk, const TGameContentMap& gameContentMap, const TLanguageMap& langMap, const String& label )
{
	TDynArray< Uint32 > sortedContentNumbers;
	gameContentMap.GetKeys( sortedContentNumbers );

	// Sort so content is installed in the correct order
	::Sort( sortedContentNumbers.Begin(), sortedContentNumbers.End() );

	const SPackageLanguagesPS4& packageLanguages = m_packageSku.m_packageLanguages;
	TDynArray< String > supportedGameLanguages = packageLanguages.m_supportedSpeechLanguages;

	const Uint32 binChunkID = 0;
	m_installOrderChunkIDs.PushBack( binChunkID ); // First chunkID must always be zero for PlayGo
	m_isLaunchChunk.Set( binChunkID );

	Uint32 chunkID = 1;
	for ( Uint32 i = 0; i < NUM_PREFETCH_PATCH_CHUNKS; ++i )
	{
		const Uint32 patchChunkID = chunkID++;
		m_installOrderChunkIDs.PushBack( patchChunkID );
		m_isLaunchChunk.Set( patchChunkID );
	}

	// Hack
	if ( hasPrefetchChunk )
	{
		const Uint32 prefetchChunkID = chunkID++;
		m_installOrderChunkIDs.PushBack( prefetchChunkID );
		m_isLaunchChunk.Set( prefetchChunkID );
	}
	
	Uint32 numInitialChunks = 0;
	for ( Uint32 sortedContentNumber : sortedContentNumbers )
	{
		const Bool isLaunchChunk = sortedContentNumber <= m_packageOptions.m_launchContentNumber;

		SChunkLayerInfoPS4 chunkLayerPair;
		if ( ! gameContentMap.Find( sortedContentNumber, chunkLayerPair ) )
		{
			ERR_WCC(TXT("BatchScenarioUpdate: failed to find chunkLayerPair for %u"), sortedContentNumber );
			return false;
		}
		m_installOrderChunkIDs.PushBack( chunkLayerPair.m_chunkID );
		if ( isLaunchChunk )
		{
			m_isLaunchChunk.Set( chunkLayerPair.m_chunkID );
		}

		TDynArray< Uint32 > langChunkIDs;
		for ( const String& gameLang : supportedGameLanguages )
		{
			const SGameContentLanguage contentLanguage( gameLang, sortedContentNumber );
			SChunkLayerInfoPS4 langChunkLayerPair;
			if ( langMap.Find( contentLanguage, langChunkLayerPair ) )
			{
				langChunkIDs.PushBack( langChunkLayerPair.m_chunkID );
				if ( isLaunchChunk )
				{
					m_isLaunchChunk.Set( langChunkLayerPair.m_chunkID );
				}
			}
		}
		::Sort( langChunkIDs.Begin(), langChunkIDs.End() );
		m_installOrderChunkIDs.PushBack( langChunkIDs );

		if ( isLaunchChunk )
		{
			numInitialChunks = m_installOrderChunkIDs.Size();
		}
	}

	return m_batchCommandWriter.ScenarioUpdate( defaultScenarioID, numInitialChunks, m_installOrderChunkIDs, label );
}

Bool CPackageProjectBuilderPS4::BatchAddLanguageMaskChunks( Uint32 skipContent, Uint32 maxContentCount, Bool chunkAscending, Uint32 startChunkID, Uint32 layer, TLanguageMap& outLangMap, Uint32& outNextFreeChunkID )
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;
	const SPackageLanguagesPS4& packageLanguages = m_packageSku.m_packageLanguages;

	outNextFreeChunkID = startChunkID;

	Uint32 contentCount = 0;
	// For each content dir: for each new language: create a language chunk
	for ( auto it = packageFiles.m_gameContentFilesMap.Begin(); it != packageFiles.m_gameContentFilesMap.End(); ++it )
	{
		if ( skipContent > 0 )
		{
			--skipContent;
			continue;
		}

		if ( contentCount >= maxContentCount )
		{
			return true;
		}
		++contentCount;

		const Uint32 sortedContentNumber = it->m_first;
		const SPackageGameFiles& gameFiles = it->m_second;

		TDynArray< String > languageFiles;

		if ( m_packageOptions.m_languageMaskSpeech )
		{
			languageFiles.PushBack( gameFiles.m_speechFiles );
		}

		if ( m_packageOptions.m_languageMaskStrings )
		{
			languageFiles.PushBack( gameFiles.m_stringsFiles );
		}

		// Sort so chunkID order doesn't depend on file discovery order. It will still depend on whether any which languages are present though.
		Sort( languageFiles.Begin(), languageFiles.End(), 
			[this](const String& a, const String& b){ String langA; String langB; GetGameLanguage(a, langA); GetGameLanguage(b, langB); return langA < langB; } );

		for ( const String& languageFile : languageFiles )
		{
			String gameLanguage;
			if ( ! GetGameLanguage( languageFile, gameLanguage ) )
			{
				return false;
			}

			SGameContentLanguage gameContentLanguage( gameLanguage, sortedContentNumber );
			SChunkLayerInfoPS4 langChunkLayerPair;
			if ( ! outLangMap.Find( gameContentLanguage, langChunkLayerPair ) )
			{
				const Uint32 chunkID = outNextFreeChunkID;
				outNextFreeChunkID = outNextFreeChunkID + ( chunkAscending ? 1  : -1 );

// 				if ( chunkID < langMinChunkID )
// 				{
// 					ERR_WCC(TXT("Exceeded min chunk ID %u while generating language chunks!"), langMinChunkID );
// 					return false;
// 				}

				langChunkLayerPair = SChunkLayerInfoPS4( chunkID, layer ); // For manifest generation, tied back to the original sortedContentNumber not ChunkIDToContentNumber(chunkID)
				outLangMap.Insert( gameContentLanguage, langChunkLayerPair );

				const String& label = String::Printf(TXT("content%u_%ls"), sortedContentNumber, gameLanguage.AsChar() );
				TDynArray< String > languages;
				languages.PushBack( gameLanguage );
				if ( ! m_batchCommandWriter.ChunkAdd( langChunkLayerPair, languages, label ) )
				{
					return false;
				}
			}
		}
	}

	return true;
}

Bool CPackageProjectBuilderPS4::BatchAddGameContentChunksNoLanguageMask( Uint32 skipContent, Uint32 maxContentCount, Uint32 startChunkID, Uint32 layer, TGameContentMap& outGameContentMap, Uint32& outNextFreeChunkID )
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;
	const SPackageLanguagesPS4& packageLanguages = m_packageSku.m_packageLanguages;

	outNextFreeChunkID = startChunkID;

	Uint32 contentCount = 0;
	for ( auto it = packageFiles.m_gameContentFilesMap.Begin(); it != packageFiles.m_gameContentFilesMap.End(); ++it )
	{
		if ( skipContent > 0 )
		{
			--skipContent;
			continue;
		}

		if ( contentCount >= maxContentCount )
		{
			return true;
		}
		++contentCount;

		const Uint32 sortedContentNumber = it->m_first;
		const SPackageGameFiles& gameFiles = it->m_second;

		if ( !HasUnmaskedContent( gameFiles, m_packageOptions ) )
		{
			continue;
		}

		Uint32 chunkID = outNextFreeChunkID++;
		if ( chunkID > MAX_CHUNK_ID )
		{
			ERR_WCC(TXT("Exceeded maximum chunk ID %u while generating game content chunks!"), MAX_CHUNK_ID );
			return false;
		}

		SChunkLayerInfoPS4 chunkLayerPair( chunkID, layer );
		if ( ! outGameContentMap.Insert( sortedContentNumber, chunkLayerPair ) )
		{
			// Internal error
			ERR_WCC(TXT("Duplicate chunkLayer pair (%u,%u) for content%u"), chunkID, layer, sortedContentNumber );
		}

		const String& label = String::Printf(TXT("content%u"), sortedContentNumber );
		if ( ! m_batchCommandWriter.ChunkAdd( chunkLayerPair, label ) )
		{
			return false;
		}
	}

	return true;
}

Bool CPackageProjectBuilderPS4::BatchAddFillerChunks( Uint32 startChunkID, Uint32 endChunkID, Uint32 layer )
{
	for ( Uint32 chunkID = startChunkID; chunkID <= endChunkID; ++chunkID )
	{
		if ( ! m_batchCommandWriter.ChunkAdd( SChunkLayerInfoPS4( chunkID, layer ), TXT("filler") ) )
		{
			return false;
		}
	}

	return true;
}

Bool CPackageProjectBuilderPS4::GetGameLanguage( const String& languageFile, String& outGameLanguage ) const
{
	const SPackageLanguagesPS4& packageLanguages = m_packageSku.m_packageLanguages;

	CFilePath filePath( languageFile );
	if ( ! filePath.HasFilename() )
	{
		ERR_WCC(TXT("Language file '%ls' does not specify a file!"), languageFile.AsChar() );
		return false;
	}

	String gameLanguage;
	const String fileName = filePath.GetFileNameWithExt();
	if ( ! packageLanguages.m_languageFileToGameLangMap.Find( fileName, gameLanguage ) )
	{
		ERR_WCC(TXT("Language file {%ls} '%ls' not mapped to any game language"), fileName.AsChar(), languageFile.AsChar() );
		ERR_WCC(TXT("Game language mappings..."));
		for ( auto it = packageLanguages.m_languageFileToGameLangMap.Begin(); it != packageLanguages.m_languageFileToGameLangMap.End(); ++it )
		{
			ERR_WCC(TXT("FileName {%ls} -> GameLanguage {%ls}"), it->m_first.AsChar(), it->m_second.AsChar() );
		}

		return false;
	}

	outGameLanguage = Move( gameLanguage );
	return true;
}

String CPackageProjectBuilderPS4::GetContentPrefix() const
{
	String prefix = TXT("content");
	switch (m_packageSku.m_packageType)
	{
	case ePackageType_App:
	case ePackageType_Patch: /* fall through, yes even for patch, because it's in the content dir*/
		prefix = TXT("content");
		break;
	case ePackageType_Dlc:
		prefix = TXT("dlc");
		break;
	default:
		RED_FATAL("CPackageProjectBuilderPS4[GetContentPrefix]: Unhandled package type %u", m_packageSku.m_packageType );
		break;
	}

	return prefix;
}

Bool CPackageProjectBuilderPS4::AddBaseFiles( const SChunkLayerInfoPS4& chunkLayerPair )
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;

	String elfFileForPackage;
	for ( const String& elfFile : packageFiles.m_exeFiles )
	{
		if ( CFilePath( elfFile ).GetFileNameWithExt() == m_packageOptions.m_elfName.ToLower() )
		{
			elfFileForPackage = elfFile;
			break;
		}
	}

	if ( elfFileForPackage.Empty() && m_contentType != eContentType_Patch )
	{
		ERR_WCC(TXT("Could not find ELF '%ls' for package"), m_packageOptions.m_elfName.AsChar() );
		return false;
	}

	const CName binManifestContentName(String::Printf(TXT("%ls0"), GetContentPrefix().AsChar()));

	if ( !elfFileForPackage.Empty() )
	{
		const String exeTargetName(TXT("eboot.bin"));
		if ( ! m_batchCommandWriter.AddFile( elfFileForPackage, exeTargetName, chunkLayerPair ) )
		{
			return false;
		}
	}

	for ( const String& sysFile : packageFiles.m_sysFiles )
	{
		const String& origPath = sysFile;
		if ( ! m_batchCommandWriter.AddFile( origPath, MakeForwardSlash( origPath ), chunkLayerPair ) )
		{
			return false;
		}
	}

	//if ( m_contentType != eContentType_Patch )
	{
		for ( const String& prxFile : packageFiles.m_dynLibFiles )
		{
			String origPath = prxFile;

			// FIXME: quick hack for the validator
			origPath.Replace(TXT("libscefios2.prx"), TXT("libSceFios2.prx"));
			if ( ! m_batchCommandWriter.AddFile( origPath, MakeForwardSlash( origPath ), chunkLayerPair ) )
			{
				return false;
			}
		}
	}

	for ( const String& binFile : packageFiles.m_gameBinFiles )
	{
		// Hack!
		const Bool compress = binFile.EndsWith(TXT(".store")); // metadata.store
		const String& origPath = binFile;
		if ( ! m_batchCommandWriter.AddFile( origPath, MakeForwardSlash( origPath ), chunkLayerPair, compress ) )
		{
			return false;
		}
	}

	return true;
}


// FIXME: Meh, if strings aren't masked then they won't be sorted into their own bucket, so just use a global hack here...
static Bool GCompressStringHack = false;
static Bool GCompressScriptsHack = true;

Bool CPackageProjectBuilderPS4::AddPrefetchFiles( const SChunkLayerInfoPS4& chunkLayerPair )
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;
	const EFileCompression bundleFileCompression = m_packageOptions.m_compressBundleFiles ? eFileCompression_Enabled : eFileCompression_Disabled;
	const EFileCompression cacheFileCompression = m_packageOptions.m_compressCacheFiles ? eFileCompression_Enabled : eFileCompression_Disabled;
	EFileCompression scriptsFileCompression = m_packageOptions.m_compressScriptFiles ? eFileCompression_Enabled : eFileCompression_Disabled;
	EFileCompression stringsFileCompression = m_packageOptions.m_compressStringsFiles ? eFileCompression_Enabled : eFileCompression_Disabled;

	//////////////////////////////////////////////////////////////////////////
	// FIXME: Small hack!
	Red::System::ScopedFlag<Bool> hack( GCompressStringHack = true, false );
	const CName manifestContentName(String::Printf(TXT("%ls0_prefetch"), GetContentPrefix().AsChar()));
	const CName manifestBaseContentName(String::Printf(TXT("%ls0"), GetContentPrefix().AsChar()));

	if ( ! BatchAddFiles( packageFiles.m_prefetchFiles, chunkLayerPair, eFileCompression_Disabled ) )
	{
		return false;
	}
	if ( ! ManifestAddFiles( manifestContentName, manifestBaseContentName, packageFiles.m_prefetchFiles, chunkLayerPair, TXT("prefetch") ) )
	{
		return false;
	}

	return true;
}

Bool CPackageProjectBuilderPS4::AddGameLanguageMaskFiles( const TLanguageMap& langMap )
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;
	const EFileCompression stringsFileCompression = m_packageOptions.m_compressStringsFiles ? eFileCompression_Enabled : eFileCompression_Disabled;
	const EFileCompression speechFileCompression = m_packageOptions.m_compressSpeechFiles ? eFileCompression_Enabled : eFileCompression_Disabled;

	struct SLanguageFileGroup
	{
		TDynArray< String >	m_languageFiles;
		EFileCompression	m_compression;

		SLanguageFileGroup()
			: m_compression( eFileCompression_Disabled )
		{}

		SLanguageFileGroup( const TDynArray< String >& languageFiles, EFileCompression compression )
			: m_languageFiles( languageFiles )
			, m_compression( compression )
		{}
	};

	for ( auto it = packageFiles.m_gameContentFilesMap.Begin(); it != packageFiles.m_gameContentFilesMap.End(); ++it )
	{
		const Uint32 sortedContentNumber = it->m_first;
		const SPackageGameFiles& gameFiles = it->m_second;

		TDynArray< SLanguageFileGroup > languageFileGroups;

		if ( m_packageOptions.m_languageMaskSpeech )
		{
			languageFileGroups.PushBack( SLanguageFileGroup( gameFiles.m_speechFiles, speechFileCompression ) );
		}
		if ( m_packageOptions.m_languageMaskStrings )
		{
			languageFileGroups.PushBack( SLanguageFileGroup( gameFiles.m_stringsFiles, stringsFileCompression ) );
		}

		for ( const SLanguageFileGroup& languageFileGroup : languageFileGroups )
		{
			const EFileCompression languageFileCompression = languageFileGroup.m_compression;
			for ( const String& languageFile : languageFileGroup.m_languageFiles )
			{
				String gameLanguage;
				if ( ! GetGameLanguage( languageFile, gameLanguage ) )
				{
					return false;
				}

				const CName manifestContentName(String::Printf(TXT("%ls%u_%ls"), GetContentPrefix().AsChar(), sortedContentNumber, gameLanguage.AsChar() ) );
				const CName manifestBaseContentName(String::Printf(TXT("%ls%u"), GetContentPrefix().AsChar(), sortedContentNumber ) );

				SGameContentLanguage gameContentLanguage( gameLanguage, sortedContentNumber );
				SChunkLayerInfoPS4 langChunkLayerPair;
				if ( ! langMap.Find( gameContentLanguage, langChunkLayerPair) )
				{
					ERR_WCC(TXT("Failed to find chunkID for language file '%ls' content%u, game language {%ls}"), languageFile.AsChar(), sortedContentNumber, gameLanguage.AsChar() );
					return false;
				}

				if ( ! BatchAddFile( languageFile, langChunkLayerPair, languageFileCompression ) )
				{
					return false;
				}
				if ( ! ManifestAddFile( manifestContentName, manifestBaseContentName, languageFile, langChunkLayerPair, gameLanguage ) )
				{
					return false;
				}
			}
		}
	}

	return true;
}

Bool CPackageProjectBuilderPS4::AddGamePatchFiles( const TGamePatchMap& gamePatchMap )
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;

	// Compression?
	for ( auto it = packageFiles.m_gamePatchFilesMap.Begin(); it != packageFiles.m_gamePatchFilesMap.End(); ++it )
	{
		const Uint32 sortedPatchNumber = it->m_first;
		const TDynArray< String >& files = it->m_second;

		const CName manifestContentName(String::Printf(TXT("patch%u"), sortedPatchNumber ));
		SChunkLayerInfoPS4 chunkLayerPair;
		if ( ! gamePatchMap.Find( sortedPatchNumber, chunkLayerPair ) )
		{
			ERR_WCC(TXT("Failed to find chunkID for patch%u"), sortedPatchNumber);
			return false;
		}
		if ( ! BatchAddFiles( files, chunkLayerPair, eFileCompression_Disabled ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, files, chunkLayerPair ) )
		{
			return false;
		}
	}

	return true;
}

Bool CPackageProjectBuilderPS4::AddGameContentFilesNoLanguageMask( const TGameContentMap& gameContentMap )
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;
	const EFileCompression bundleFileCompression = m_packageOptions.m_compressBundleFiles ? eFileCompression_Enabled : eFileCompression_Disabled;
	const EFileCompression cacheFileCompression = m_packageOptions.m_compressCacheFiles ? eFileCompression_Enabled : eFileCompression_Disabled;
	EFileCompression scriptsFileCompression = m_packageOptions.m_compressScriptFiles ? eFileCompression_Enabled : eFileCompression_Disabled;
	EFileCompression stringsFileCompression = m_packageOptions.m_compressStringsFiles ? eFileCompression_Enabled : eFileCompression_Disabled;
	
	const EFileCompression speechFileCompression = m_packageOptions.m_compressSpeechFiles ? eFileCompression_Enabled : eFileCompression_Disabled;

	for ( auto it = packageFiles.m_gameContentFilesMap.Begin(); it != packageFiles.m_gameContentFilesMap.End(); ++it )
	{
		const Uint32 sortedContentNumber = it->m_first;
		const SPackageGameFiles& gameFiles = it->m_second;

		if ( !HasUnmaskedContent( gameFiles, m_packageOptions ) )
		{
			continue;
		}

		//////////////////////////////////////////////////////////////////////////
		// FIXME: Small hack!
		Red::System::ScopedFlag<Bool> hack( GCompressStringHack, false );
		if ( sortedContentNumber == 0 )
		{
			GCompressStringHack = true;
		}

		const CName manifestContentName(String::Printf(TXT("%ls%u"), GetContentPrefix().AsChar(), sortedContentNumber ));
		SChunkLayerInfoPS4 chunkLayerPair;
		if ( ! gameContentMap.Find( sortedContentNumber, chunkLayerPair ) )
		{
			ERR_WCC(TXT("Failed to find chunkID for content%u"), sortedContentNumber);
			return false;
		}
		if ( ! BatchAddFiles( gameFiles.m_bundleFiles, chunkLayerPair, bundleFileCompression ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_bundleFiles, chunkLayerPair ) )
		{
			return false;
		}

		if ( ! BatchAddFiles( gameFiles.m_scriptFiles, chunkLayerPair, eFileCompression_Disabled ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_scriptFiles, chunkLayerPair ) )
		{
			return false;
		}

		if ( ! BatchAddFiles( gameFiles.m_cacheFiles, chunkLayerPair, cacheFileCompression ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_cacheFiles, chunkLayerPair ) )
		{
			return false;
		}

		if ( ! BatchAddFiles( gameFiles.m_miscFiles, chunkLayerPair, eFileCompression_Disabled ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_miscFiles, chunkLayerPair ) )
		{
			return false;
		}

		if ( ! m_packageOptions.m_languageMaskStrings )
		{
			if ( ! BatchAddFiles( gameFiles.m_stringsFiles, chunkLayerPair, stringsFileCompression ) )
			{
				return false;
			}
			if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_stringsFiles, chunkLayerPair ) )
			{
				return false;
			}
		}

		if ( ! m_packageOptions.m_languageMaskSpeech )
		{
			if ( ! BatchAddFiles( gameFiles.m_speechFiles, chunkLayerPair, speechFileCompression ) )
			{
				return false;
			}
			if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_speechFiles, chunkLayerPair ) )
			{
				return false;
			}
		}
	}
	
	return true;
}

Bool CPackageProjectBuilderPS4::AddGameContentFilesForDlc()
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;
	for ( auto it = packageFiles.m_gameContentFilesMap.Begin(); it != packageFiles.m_gameContentFilesMap.End(); ++it )
	{
		const Uint32 sortedContentNumber = it->m_first;
		const SPackageGameFiles& gameFiles = it->m_second;

		const CName manifestContentName(String::Printf(TXT("%ls%u"), GetContentPrefix().AsChar(), sortedContentNumber ));
		if ( ! BatchAddFilesNoChunk( gameFiles.m_bundleFiles, eFileCompression_Disabled ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_bundleFiles, SChunkLayerInfoPS4::INVALID ) )
		{
			return false;
		}

		if ( ! BatchAddFilesNoChunk( gameFiles.m_scriptFiles, eFileCompression_Disabled ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_scriptFiles, SChunkLayerInfoPS4::INVALID ) )
		{
			return false;
		}

		if ( ! BatchAddFilesNoChunk( gameFiles.m_cacheFiles, eFileCompression_Disabled ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_cacheFiles, SChunkLayerInfoPS4::INVALID ) )
		{
			return false;
		}

		if ( ! BatchAddFilesNoChunk( gameFiles.m_miscFiles, eFileCompression_Disabled ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_miscFiles, SChunkLayerInfoPS4::INVALID ) )
		{
			return false;
		}

		if ( ! BatchAddFilesNoChunk( gameFiles.m_stringsFiles, eFileCompression_Disabled ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_stringsFiles, SChunkLayerInfoPS4::INVALID ) )
		{
			return false;
		}

		if ( ! BatchAddFilesNoChunk( gameFiles.m_speechFiles, eFileCompression_Disabled ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_speechFiles, SChunkLayerInfoPS4::INVALID ) )
		{
			return false;
		}
	}

	return true;
}

Bool CPackageProjectBuilderPS4::BatchAddFiles( const TDynArray< String >& files, const SChunkLayerInfoPS4& chunkLayerPair, EFileCompression fileCompression )
{
	for ( const String& file : files )
	{
		const Bool forceHackCompression = (GCompressStringHack && file.EndsWith(TXT(".w3strings")))	|| file.EndsWith(TXT(".redscripts"));
		const Bool compress = ( fileCompression == eFileCompression_Enabled ) || forceHackCompression;
		if ( ! m_batchCommandWriter.AddFile( file, MakeForwardSlash(file), chunkLayerPair, compress ) )
		{
			return false;
		}
	}

	return true;
}

Bool CPackageProjectBuilderPS4::BatchAddFile( const String& file, const SChunkLayerInfoPS4& chunkLayerPair, EFileCompression fileCompression )
{
	TDynArray< String > files;
	files.PushBack( file );
	return BatchAddFiles( files, chunkLayerPair, fileCompression );
}

Bool CPackageProjectBuilderPS4::BatchAddFilesNoChunk( const TDynArray< String >& files, EFileCompression fileCompression )
{
	for ( const String& file : files )
	{
		if ( ! m_batchCommandWriter.AddFileNoChunk( file, MakeForwardSlash(file), false ) )
		{
			return false;
		}
	}

	return true;
}

Bool CPackageProjectBuilderPS4::BatchAddFileNoChunk( const String& file, EFileCompression fileCompression )
{
	TDynArray< String > files;
	files.PushBack( file );
	return BatchAddFilesNoChunk( files, fileCompression );
}

Bool CPackageProjectBuilderPS4::ManifestAddFiles( CName manifestContentName, CName manifestBaseContentName, const TDynArray< String >& files, const SChunkLayerInfoPS4& chunkLayerPair, const String& language )
{
	for ( const String& file : files )
	{
		const String conformedPath = MakeBackslash( file );
		const CName& resolvedManifestContentName  = m_manifestContentName == CName::NONE ? manifestContentName : m_manifestContentName;
		SContentManifestBuildInfo& info = m_contentManifestMap.GetRef( resolvedManifestContentName, SContentManifestBuildInfo( resolvedManifestContentName, manifestBaseContentName, chunkLayerPair, language ) );
		info.m_conformedPaths.PushBack( conformedPath );
	}

	return true;
}

Bool CPackageProjectBuilderPS4::ManifestAddFiles( CName manifestContentName, const TDynArray< String >& files, const SChunkLayerInfoPS4& chunkLayerPair )
{
	return ManifestAddFiles( manifestContentName, manifestContentName, files, chunkLayerPair, String::EMPTY );
}

Bool CPackageProjectBuilderPS4::ManifestAddFile( CName manifestContentName, CName manifestBaseContentName, const String& file, const SChunkLayerInfoPS4& chunkLayerPair, const String& language /*= String::EMPTY*/ )
{
	const String conformedPath = MakeBackslash( file );
	const CName& resolvedManifestContentName  = m_manifestContentName == CName::NONE ? manifestContentName : m_manifestContentName;
	SContentManifestBuildInfo& info = m_contentManifestMap.GetRef( resolvedManifestContentName, SContentManifestBuildInfo( resolvedManifestContentName, manifestBaseContentName, chunkLayerPair, language ) );
	info.m_conformedPaths.PushBack( conformedPath );

	return true;
}

Bool CPackageProjectBuilderPS4::ManifestAddFile( CName manifestContentName, const String& file, const SChunkLayerInfoPS4& chunkLayerPair )
{
	return ManifestAddFile( manifestContentName, manifestContentName, file, chunkLayerPair, String::EMPTY );
}

Bool CPackageProjectBuilderPS4::Validate( EPackageType packageType ) const
{
	if ( packageType == ePackageType_App )
	{
		if ( m_packageOptions.m_elfName.Empty() )
		{
			ERR_WCC(TXT("ELF name must be set"));
			return false;
		}

		Bool hasMatchingElfFile = false;
		for ( const String& file : m_packageSku.m_packageFiles.m_exeFiles )
		{
			CFilePath filePath( file );
			if ( filePath.GetFileNameWithExt() == m_packageOptions.m_elfName.ToLower() )
			{
				hasMatchingElfFile = true;
				break;
			}
		}

		if ( ! hasMatchingElfFile )
		{
			ERR_WCC(TXT("Package option ELF file '%ls' was not found in package files"), m_packageOptions.m_elfName.AsChar() );
			ERR_WCC(TXT("Candidate ELF files..."));
			for ( const String& file : m_packageSku.m_packageFiles.m_exeFiles )
			{
				ERR_WCC(TXT("ELF file: {%ls} %ls"), CFilePath( file ).GetFileNameWithExt().AsChar(), file.AsChar() );
			}

			return false;
		}

		if ( ! ValidatePackageLanguages( m_packageSku.m_packageLanguages ) )
		{
			return false;
		}
	} // ePackageType_App

	if ( ! ValidateContentID( m_packageSku.m_contentID ) )
	{
		return false;
	}

	if ( ! ValidatePasscode( m_packageSku.m_passcode ) )
	{
		return false;
	}

	if ( packageType == ePackageType_Dlc )
	{
		if ( ! ValidateEntitlementKey( m_packageSku.m_entitlementKey ) )
		{
			return false;
		}
	}

	Bool validFiles = false;

	switch (m_packageSku.m_packageType)
	{
	case ePackageType_App:
		validFiles = ValidateAppPackageFiles( m_packageSku.m_packageFiles );
		break;
	case ePackageType_Patch:
		validFiles = ValidatePatchPackageFiles( m_packageSku.m_packageFiles );
		break;
	case ePackageType_Dlc:
		validFiles = ValidateDlcPackageFiles( m_packageSku.m_packageFiles );
		break;
	default:
		RED_FATAL("CPackageProjectBuilderPS4[Validate]: Unhandled package type %u", m_packageSku.m_packageType );
		break;
	}

	if ( ! validFiles )
	{
		return false;
	}

	LOG_WCC(TXT("Package file passed validation checks"));

	return true;
}

Bool CPackageProjectBuilderPS4::ValidateCommonPackageFiles( const SPackageFiles& packageFiles, EPackageType packageType ) const
{
	THashSet< String > seenDir;

	Uint32 maxPathComponents = 0;
	switch ( packageType )
	{
	case ePackageType_App:
		maxPathComponents = MAX_PATH_COMPONENTS_APP;
		break;
	case ePackageType_Patch:
		maxPathComponents = MAX_PATH_COMPONENTS_PATCH;
		break;
	case ePackageType_Dlc:
		maxPathComponents = MAX_PATH_COMPONENTS_DLC;
		break;
	default:
		RED_FATAL( "CPackageProjectBuilderPS4[ValidateCommonPackageFiles]: Unhandled package type %u", packageType );
		break;
	}

	Uint32 totalNumFiles = 0;
	Uint32 totalNumDirs = 0;
	
	GenerateFileEntryStats( packageFiles, totalNumFiles, totalNumDirs );

	// TBD: Confirm this!
	const Uint32 totalNumPathComponents = totalNumFiles + totalNumDirs;
	if ( totalNumPathComponents > maxPathComponents )
	{
		ERR_WCC(TXT("Total number of file and directory entries %u exceeds allowed %u. (%u files, %u directories)"),
			totalNumPathComponents, maxPathComponents, totalNumFiles, totalNumDirs );
		return false;
	}

	LOG_WCC(TXT("Total number of file and directory entries: %u/%u. (%u files, %u directories)"),
		totalNumPathComponents, maxPathComponents, totalNumFiles, totalNumDirs );

	return true;
}

Bool CPackageProjectBuilderPS4::ValidateAppPackageFiles( const SPackageFiles& packageFiles ) const
{
	// TODO: validate mandatory sce_sys files

	if ( ! ValidateCommonPackageFiles( packageFiles, ePackageType_App ) )
	{
		return false;
	}

	if ( packageFiles.m_exeFiles.Empty() )
	{
		ERR_WCC(TXT("No ELF files found!"));
		return false;
	}

	if ( packageFiles.m_sysFiles.Empty() )
	{
		ERR_WCC(TXT("No sce_sys files found!"));
		return false;
	}

	// Minimally libc.prx
	if ( packageFiles.m_dynLibFiles.Empty() )
	{
		ERR_WCC(TXT("No PRX files found!"));
		return false;
	}

	return true;
}

Bool CPackageProjectBuilderPS4::ValidatePatchPackageFiles( const SPackageFiles& packageFiles ) const
{
	// TODO
	return true;
}

Bool CPackageProjectBuilderPS4::ValidateDlcPackageFiles( const SPackageFiles& packageFiles ) const
{
	if ( ! ValidateCommonPackageFiles( packageFiles, ePackageType_Dlc ) )
	{
		return false;
	}

	if ( !packageFiles.m_exeFiles.Empty() )
	{
		ERR_WCC(TXT("DLC cannot contain executable files!"));
		for ( const String& file : packageFiles.m_exeFiles )
		{
			LOG_WCC(TXT("File: %ls"), file.AsChar() );
		}
		return false;
	}

	if ( packageFiles.m_sysFiles.Empty() )
	{
		ERR_WCC(TXT("DLC still requires sce_sys files!"));
		return false;
	}

	if ( !packageFiles.m_dynLibFiles.Empty() )
	{
		ERR_WCC(TXT("DLC cannot contain PRX fiels!"));
		for ( const String& file : packageFiles.m_dynLibFiles )
		{
			LOG_WCC(TXT("File: %ls"), file.AsChar() );
		}
		return false;
	}

	return true;
}

Bool CPackageProjectBuilderPS4::ValidatePackageLanguages( const SPackageLanguagesPS4& packageLanguages ) const
{
	if ( packageLanguages.m_gameToPlayGoLangMap.Empty() )
	{
		ERR_WCC(TXT("PlayGo language mappings must be set!"));
		return false;
	}

	if ( packageLanguages.m_languageFileToGameLangMap.Empty() )
	{
		ERR_WCC(TXT("Language files mappings must be set!"));
		return false;
	}

	if ( packageLanguages.m_supportedSpeechLanguages.Empty() )
	{
		ERR_WCC(TXT("Supported game languages must be set!"));
		return false;
	}

	if ( packageLanguages.m_defaultSpeechLanguage.Empty() )
	{
		ERR_WCC(TXT("Supported game language must be set!"));
		return false;
	}

	TDynArray< String > languageFileGameLangs;
	packageLanguages.m_languageFileToGameLangMap.GetValues( languageFileGameLangs );
	for ( auto it = packageLanguages.m_gameToPlayGoLangMap.Begin(); it != packageLanguages.m_gameToPlayGoLangMap.End(); ++it )
	{
		const String& gameLang = it->m_first;
		if ( ! languageFileGameLangs.Exist( gameLang ) )
		{
			ERR_WCC(TXT("Game language {%ls} mapped in PlayGo but not in speech and languages files config!"), gameLang.AsChar() );
			return false;
		}
	}

	THashSet< String > duplicateCheck;
	for ( auto it = packageLanguages.m_supportedSpeechLanguages.Begin(); it != packageLanguages.m_supportedSpeechLanguages.End(); ++it )
	{
		const String& gameLang = *it;
		if ( ! packageLanguages.m_gameToPlayGoLangMap.KeyExist( gameLang ) )
		{
			ERR_WCC(TXT("Supported game language {%ls} not mapped in PlayGo config!"), gameLang.AsChar() );
			return false;
		}
		if ( ! duplicateCheck.Insert( gameLang ) )
		{
			ERR_WCC(TXT("Supported game language {%ls} duplicated in list!"), gameLang.AsChar() );
			return false;
		}
	}

	if ( ! packageLanguages.m_gameToPlayGoLangMap.KeyExist( packageLanguages.m_defaultSpeechLanguage ) )
	{
		ERR_WCC(TXT("Default game language {%ls} not mapped in PlayGo config!"), packageLanguages.m_defaultSpeechLanguage.AsChar() );
		return false;
	}

	return true;
}

Bool CPackageProjectBuilderPS4::ValidateContentID( const String& contentID ) const
{
	const String expectedFormat(TXT("XXYYYY-XXXXYYYYY_00-ZZZZZZZZZZZZZZZZ"));

	if ( contentID.GetLength() != expectedFormat.GetLength() )
	{
		ERR_WCC(TXT("Content ID '%ls' of length %u, expected length %u"), contentID.AsChar(), contentID.GetLength(), expectedFormat.GetLength() );
		return false;
	}

	for ( Uint32 i = 0; i < contentID.GetLength(); ++i )
	{
		switch( expectedFormat[i] )
		{
		case TXT('X'):
			if ( contentID[i] < TXT('A') || contentID[i] > TXT('Z') )
			{
				ERR_WCC(TXT("Content ID '%ls' invalid at position %u (character %lc). Expected upper case letter"), contentID.AsChar(), contentID[i] );
				return false;
			}
			break;
		case TXT('Y'):
			if ( contentID[i] < TXT('0') || contentID[i] > TXT('9') )
			{
				ERR_WCC(TXT("Content ID '%ls' invalid at position %u (character %lc). Expected number."), contentID.AsChar(), contentID[i] );
				return false;
			}
			break;
		case TXT('Z'):
			if ( (contentID[i] < TXT('A') || contentID[i] > TXT('Z')) && (contentID[i] < TXT('0') || contentID[i] > TXT('9')) )
			{
				ERR_WCC(TXT("Content ID '%ls' invalid at position %u (character %lc). Expected upper case letter or number"), contentID.AsChar(), contentID[i] );
				return false;
			}
			break;
		case TXT('-'): /* fall through */
		case TXT('_'): /* fall through */
		case TXT('0'):
			if ( contentID[i] != expectedFormat[i] )
			{
				ERR_WCC(TXT("Content ID '%ls' invalid at position %u (character %lc). Expected character '%lc'"), contentID.AsChar(), contentID[i], expectedFormat[i] );
				return false;
			}
			break;
		default:
			{
				RED_FATAL("Unexpected format character %lc", expectedFormat[i] );
				return false;
			}
			break;
		}
	}

	return true;
}

Bool CPackageProjectBuilderPS4::ValidatePasscode( const String& passcode ) const
{
	const Uint32 expectedPasscodeLength = 32;
	if ( passcode.GetLength() != expectedPasscodeLength )
	{
		ERR_WCC(TXT("Passcode '%ls' length incorrect. Length %u, expected length %u"), passcode.AsChar(), passcode.GetLength(), expectedPasscodeLength );
		return false;
	}

	for ( Uint32 i = 0; i < passcode.GetLength(); ++i )
	{
		const Char ch = passcode[i];
		if ( ch != '-' && ch != '_' && (ch<TXT('0')||ch>TXT('9')) && (ch<TXT('A')||ch>TXT('Z')) && (ch<TXT('a')||ch>TXT('z')) )
		{
			ERR_WCC(TXT("Passcode '%ls' invalid at position %u (character %lc). Expected 0-9, A-Z, a-z, - and _"), passcode.AsChar(), i, ch );
			return false;
		}
	}

	return true;
}

Bool CPackageProjectBuilderPS4::ValidateEntitlementKey( const String& entitlementKey ) const
{
	const Uint32 expectedKeyLength = 32;
	if ( entitlementKey.GetLength() != expectedKeyLength )
	{
		ERR_WCC(TXT("Entitlement key '%ls' length incorrect. Length %u, expected length %u"), entitlementKey.AsChar(), entitlementKey.GetLength(), expectedKeyLength );
		return false;
	}

	for ( Uint32 i = 0; i < entitlementKey.GetLength(); ++i )
	{
		// Publishing_Tools_CL-Users_Guide_e.pdf doesn't say upper case A-F is allowed, while it makes case distinctions in other places.
		const Char ch = entitlementKey[i];
		if ( (ch<TXT('0')||ch>TXT('9')) && (ch<TXT('a')||ch>TXT('f')) )
		{
			ERR_WCC(TXT("Entitlement key '%ls' invalid at position %u (character %lc). Expected 0-9, a-f"), entitlementKey.AsChar(), i, ch );
			return false;
		}
	}

	return true;
}

static void GenerateFileEntryStatsHelper( const TDynArray< String >& files, THashSet< String >& outSeenDirContext, Uint32& outNumFilesAccum, Uint32& outNumDirsAccum )
{
	for ( const String& file : files )
	{
		CFilePath filePath( file );

		// Should probably use a "trie" or something less retarded...
		Uint32 numDirs = 0;
		String curDirPath;
		TDynArray< String > dirsParts = filePath.GetDirectories();
		for ( const String& dir : dirsParts )
		{
			curDirPath += TXT("\\") + dir;
			if ( outSeenDirContext.Insert(curDirPath) )
			{
				++numDirs;
			}
		}

		outNumDirsAccum += numDirs;
		if ( filePath.HasFilename() )
		{
			++outNumFilesAccum;
		}
	}
}

void CPackageProjectBuilderPS4::GenerateFileEntryStats( const SPackageFiles& packageFiles, Uint32& outNumFiles, Uint32& outNumDirs ) const
{
	THashSet< String > seenDirContext;
	GenerateFileEntryStatsHelper( packageFiles.m_gameBinFiles, seenDirContext, outNumFiles, outNumDirs );
	GenerateFileEntryStatsHelper( packageFiles.m_dynLibFiles, seenDirContext, outNumFiles, outNumDirs );
	GenerateFileEntryStatsHelper( packageFiles.m_exeFiles, seenDirContext, outNumFiles, outNumDirs );
	GenerateFileEntryStatsHelper( packageFiles.m_sysFiles, seenDirContext, outNumFiles, outNumDirs );

	for ( auto it = packageFiles.m_gameContentFilesMap.Begin(); it != packageFiles.m_gameContentFilesMap.End(); ++it )
	{
		const SPackageGameFiles& gameFiles = it->m_second;

		GenerateFileEntryStatsHelper( gameFiles.m_bundleFiles, seenDirContext, outNumFiles, outNumDirs );
		GenerateFileEntryStatsHelper( gameFiles.m_cacheFiles, seenDirContext, outNumFiles, outNumDirs );
		GenerateFileEntryStatsHelper( gameFiles.m_scriptFiles, seenDirContext, outNumFiles, outNumDirs );
		GenerateFileEntryStatsHelper( gameFiles.m_miscFiles, seenDirContext, outNumFiles, outNumDirs );
		GenerateFileEntryStatsHelper( gameFiles.m_stringsFiles, seenDirContext, outNumFiles, outNumDirs );
		GenerateFileEntryStatsHelper( gameFiles.m_speechFiles, seenDirContext, outNumFiles, outNumDirs );
	}
}

