/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/core/2darray.h"
#include "../../common/core/fileSys.h"
#include "../../common/core/scopedPtr.h"

#include "packageConstants.h"
#include "packageFiles.h"
#include "packageMapFileBuilderXboxOne.h"
#include "../../common/core/contentManifest.h"

const Uint32 NUM_PREFETCH_PATCH_CHUNKS = 16;

static String MakeBackslash( const String& path )
{
	String conformedPath = path;
	conformedPath.ReplaceAll(TXT('/'), TXT('\\'));
	return conformedPath;
}

static Bool HasUnmaskedContent( const SPackageGameFiles& gameFiles )
{
	const Bool retval =
		!gameFiles.m_bundleFiles.Empty() ||
		!gameFiles.m_stringsFiles.Empty() ||
		!gameFiles.m_scriptFiles.Empty() ||
		!gameFiles.m_cacheFiles.Empty() ||
		!gameFiles.m_miscFiles.Empty();

	return retval;
}

CPackageMapFileBuilderXboxOne::CPackageMapFileBuilderXboxOne( IFile& xmlFile, const SPackageMapFileOptionsXboxOne& packageOptions, const SPackageSkuXboxOne& packageSku, const CName& manifestContentName )
	: m_xmlWriter( xmlFile )
	, m_packageOptions( packageOptions )
	, m_packageSku( packageSku )
	, m_contentType( eContentType_Invalid )
	, m_manifestContentName( manifestContentName )
{
}

Bool CPackageMapFileBuilderXboxOne::BuildProject( TDynArray< SPackageContentManifestXboxOne >& outPackageContentManifests )
{
	// TODO: validate that all required speech, strings, and sce_sys were present for the SKU supported languages
// 	if ( ! Validate() )
// 	{
// 		return false;
// 	}
	m_contentManifestMap.Clear();
	m_installOrderChunkIDs.Clear();
	m_isLaunchChunk.ClearAll();

	// FIXME: why two similar enums?!
	switch (m_packageSku.m_packageType)
	{
	case ePackageType_App:
		m_contentType = eContentType_App;
		break;
	case ePackageType_Dlc:
		m_contentType = eContentType_Dlc;
		break;
	case ePackageType_Patch:
		m_contentType = eContentType_Patch;
		break;
	default:
		RED_FATAL( "Unhandled packageType %u", m_packageSku.m_packageType );
		break;
	}

	// TODO: Updated in creating package plan!
	// Splitter should create a layer0 and layer1 directory
	// Probably put all extra chunks on layer zero, at least like digigtal distrib and what sense do layers make in a patch?
	const ChunkID binChunkID = MIN_CHUNK_ID_XBOX;

	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;
	const SPackageLanguagesXboxOne packageLanguages = m_packageSku.m_packageLanguages;

	ChunkID nextFreeChunkID = MIN_CHUNK_ID_XBOX;
	if ( m_contentType == eContentType_App || m_contentType == eContentType_Patch )
	{
		if ( ! AddBinChunk( binChunkID, nextFreeChunkID ) )
		{
			return false;
		}
	}

	Uint32 patchChunkIDStart = 0xFFFFFFFF;

	TGamePatchMap gamePatchMap;
	// If app, just stub patch chunks
	if ( m_contentType == eContentType_App || m_contentType == eContentType_Patch )
	{
		patchChunkIDStart = nextFreeChunkID;
		AddPatchChunks( nextFreeChunkID, NUM_PREFETCH_PATCH_CHUNKS, gamePatchMap, nextFreeChunkID );
	}

	const Uint32 contentStartChunkID = nextFreeChunkID;

	// DLC isn't stream installed, so don't bother with a separate chunk
	const Bool forceSpeeches = m_contentType == eContentType_Dlc;

	TGameContentMap gameContentMap;
	if ( ! AddGameContentChunksNoLanguages( contentStartChunkID, gameContentMap, nextFreeChunkID, forceSpeeches ) )
	{
		return false;
	}

	// Create content chunks in order for each language after game chunks. We'll skip to the needed language
	// chunks dynamically as needed and then automatically resume installer lower game-only chunks
	// E.g., chunkX: Prologue+EN, chunkX+1: Prologue+JP, chunkX+2: Epilogue+EN, chunkX+3: Epilogue+JP
	const ChunkID langStartChunkID = nextFreeChunkID;
	TLanguageMap langMap;
	if ( m_contentType == eContentType_App || m_contentType == eContentType_Patch )
	{
		if ( ! AddLanguageChunks( langStartChunkID, langMap, nextFreeChunkID ) )
		{
			return false;
		}
	}

	if ( nextFreeChunkID > MAX_USED_CHUNK_ID_XBOX )
	{
		ERR_WCC(TXT("Adding language chunks used all available chunkIDs!"));
		return false;
	}

	const Uint32 initialChunkCount = nextFreeChunkID;

	// Set default launch marker chunk
	if ( ! m_xmlWriter.SetLaunchMarkerChunk( nextFreeChunkID-1) )
	{
		return false;
	}

	if ( m_contentType == eContentType_App || m_contentType == eContentType_Patch )
	{
		if ( ! m_xmlWriter.AddFile( INSTALLER_DAT_FILE, binChunkID ) )
		{
			ERR_WCC(TXT("Failed to add installer dat file '%ls'"), INSTALLER_DAT_FILE);
			return false;
		}
		if ( ! AddBaseFiles( binChunkID ) ) // exe and dll files
		{
			return false;
		}
	}

	const String manifestPath = String::Printf(TXT("content\\%ls"), MANIFEST_FILE_NAME);
	if ( ! m_xmlWriter.AddFile( manifestPath, binChunkID ) )
	{
		ERR_WCC(TXT("Failed to add MANIFEST_FILE_NAME '%ls'"), manifestPath.AsChar() );
		return false;
	}

	// Even if app, add the placeholder files
	if ( m_contentType == eContentType_App || m_contentType == eContentType_Patch )
	{
		if ( ! AddGamePatchFiles( gamePatchMap ) )
		{
			return false;
		}
	}

	if ( ! AddGameContentFilesNoLanguages( gameContentMap, forceSpeeches ) )
	{
		return false;
	}

	if ( m_contentType == eContentType_App || m_contentType == eContentType_Patch )
	{
		if ( ! AddGameLanguageFiles( langMap ) )
		{
			return false;
		}
	}

	for ( auto it = m_contentManifestMap.Begin(); it != m_contentManifestMap.End(); ++it )
	{
		const CName manifestConentName = it->m_first;
		const SContentManifestBuildInfo& manifestInfo = it->m_second;

		SPackageContentManifestXboxOne contentManifest;
		contentManifest.m_contentName = manifestConentName;
		contentManifest.m_baseContentName = manifestInfo.m_baseContentName;
		contentManifest.m_chunkID = manifestInfo.m_chunkID;
		contentManifest.m_conformedFilePaths = manifestInfo.m_conformedPaths;
		contentManifest.m_language = manifestInfo.m_language;
		contentManifest.m_isLaunchContent = m_isLaunchChunk.Get( manifestInfo.m_chunkID );

		// Get manifest install order
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

	// Sets the last install order chunkID as the launch chunk. Note language chunks cannot be launch markers.
	for ( Uint32 chunkID : m_installOrderChunkIDs )
	{
		if ( m_isLaunchChunk.Get( chunkID ) )
		{
			if ( ! m_xmlWriter.SetLaunchMarkerChunk( chunkID ) )
			{
				return false;
			}
		}
	}

	return m_xmlWriter.WriteXml( m_packageSku.m_packageType );
}

Bool CPackageMapFileBuilderXboxOne::AddBinChunk( ChunkID binChunkID, ChunkID& outNextFreeChunkID )
{
	outNextFreeChunkID = binChunkID;
	const Uint32 chunkID = outNextFreeChunkID++;
	return m_xmlWriter.ChunkAdd( chunkID );

	m_installOrderChunkIDs.PushBack( binChunkID );
	m_isLaunchChunk.Set( binChunkID );
}

Bool CPackageMapFileBuilderXboxOne::AddPatchChunks( ChunkID startChunkID, Uint32 numPatchChunks, TGamePatchMap& outGamePatchMap, ChunkID& outNextFreeChunkID )
{
	outNextFreeChunkID = startChunkID;
	for ( Uint32 patchNumber = 0; patchNumber < numPatchChunks; ++patchNumber )
	{
		const Uint32 chunkID = outNextFreeChunkID++;
		m_installOrderChunkIDs.PushBack( chunkID );
		m_isLaunchChunk.Set( chunkID );
		outGamePatchMap.Insert( patchNumber, chunkID );
		if ( !m_xmlWriter.ChunkAdd( chunkID ) )
		{
			return false;
		}
	}
	return true;
}

Bool CPackageMapFileBuilderXboxOne::AddLanguageChunks( ChunkID startChunkID, TLanguageMap& outLangMap, ChunkID& outNextFreeChunkID )
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;
	const SPackageLanguagesXboxOne& packageLanguages = m_packageSku.m_packageLanguages;

	outNextFreeChunkID = startChunkID;

	// For each content dir: for each new language: create a language chunk
	for ( auto it = packageFiles.m_gameContentFilesMap.Begin(); it != packageFiles.m_gameContentFilesMap.End(); ++it )
	{
		const Uint32 sortedContentNumber = it->m_first;
		const SPackageGameFiles& gameFiles = it->m_second;

		TDynArray< String > languageFiles = gameFiles.m_speechFiles;
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
			ChunkID chunkID = INVALID_CHUNK_ID;
			if ( ! outLangMap.Find( gameContentLanguage, chunkID ) )
			{
				const Uint32 chunkID = outNextFreeChunkID++;
				if ( chunkID > MAX_USED_CHUNK_ID_XBOX )
				{
					ERR_WCC(TXT("Exceeded maximum chunk ID %u while generating language chunks!"), MAX_USED_CHUNK_ID_XBOX );
					return false;
				}
				outLangMap.Insert( gameContentLanguage, chunkID );

				const String& label = String::Printf(TXT("content%u - %ls"), sortedContentNumber, gameLanguage.AsChar() );
				TDynArray< String > languages;
				languages.PushBack( gameLanguage );
				if ( ! m_xmlWriter.ChunkAdd( chunkID ) )
				{
					return false;
				}
			}
		}
	}
	return true;
}

Bool CPackageMapFileBuilderXboxOne::AddGameContentChunksNoLanguages( ChunkID startChunkID, TGameContentMap& outGameContentMap, ChunkID& outNextFreeChunkID, Bool forceSpeeches /*= false*/ )
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;
	const SPackageLanguagesXboxOne& packageLanguages = m_packageSku.m_packageLanguages;

	outNextFreeChunkID = startChunkID;

	for ( auto it = packageFiles.m_gameContentFilesMap.Begin(); it != packageFiles.m_gameContentFilesMap.End(); ++it )
	{
		const Uint32 sortedContentNumber = it->m_first;
		const SPackageGameFiles& gameFiles = it->m_second;

		if ( !HasUnmaskedContent( gameFiles ) && (!forceSpeeches || gameFiles.m_speechFiles.Empty()) )
		{
			continue;
		}

		Uint32 chunkID = outNextFreeChunkID++;
		if ( chunkID > MAX_USED_CHUNK_ID_XBOX )
		{
			ERR_WCC(TXT("Exceeded maximum chunk ID %u while generating game content chunks!"), MAX_USED_CHUNK_ID_XBOX );
			return false;
		}

		if ( ! outGameContentMap.Insert( sortedContentNumber, chunkID ) )
		{
			// Internal error
			ERR_WCC(TXT("Duplicate chunk %u for content%u"), chunkID, sortedContentNumber );
		}

		if ( ! m_xmlWriter.ChunkAdd( chunkID ) )
		{
			return false;
		}

		// Language files can't be launch chunks or part of the normal install-order because there's no language mask support!
		m_installOrderChunkIDs.PushBack( chunkID );
		if ( sortedContentNumber <= m_packageOptions.m_launchContentNumber )
		{
			m_isLaunchChunk.Set( chunkID );
		}
	}

	return true;
}

Bool CPackageMapFileBuilderXboxOne::AddBaseFiles( ChunkID chunkID )
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;

	if ( ! m_xmlWriter.AddFile( m_packageOptions.m_exeName, chunkID ) )
	{
		return false;
	}

	for ( const String& dllFile : packageFiles.m_dynLibFiles )
	{
		const String& origPath = dllFile;
		const String targPath = CFilePath( origPath ).GetFileNameWithExt().AsChar();
		if ( ! m_xmlWriter.AddFile( origPath, targPath, chunkID ) )
		{
			return false;
		}
	}

	for ( const String& binFile : packageFiles.m_gameBinFiles )
	{
		const String& origPath = binFile;
		if ( ! m_xmlWriter.AddFile( origPath, chunkID ) )
		{
			return false;
		}
	}

	return true;
}

Bool CPackageMapFileBuilderXboxOne::AddGameLanguageFiles( const TLanguageMap& langMap )
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;
	for ( auto it = packageFiles.m_gameContentFilesMap.Begin(); it != packageFiles.m_gameContentFilesMap.End(); ++it )
	{
		const Uint32 sortedContentNumber = it->m_first;
		const SPackageGameFiles& gameFiles = it->m_second;

		for ( const String& languageFile : gameFiles.m_speechFiles )
		{
			String gameLanguage;
			if ( ! GetGameLanguage( languageFile, gameLanguage ) )
			{
				return false;
			}

			const CName manifestContentName(String::Printf(TXT("%ls%u_%ls"), GetContentPrefix().AsChar(), sortedContentNumber, gameLanguage.AsChar() ) );
			const CName manifestBaseContentName(String::Printf(TXT("%ls%u"), GetContentPrefix().AsChar(), sortedContentNumber ) );

			SGameContentLanguage gameContentLanguage( gameLanguage, sortedContentNumber );
			ChunkID chunkID = INVALID_CHUNK_ID;
			if ( ! langMap.Find( gameContentLanguage, chunkID ) )
			{
				ERR_WCC(TXT("Failed to find chunkID for language file '%ls' content%u, game language {%ls}"), languageFile.AsChar(), sortedContentNumber, gameLanguage.AsChar() );
				return false;
			}

			if ( ! AddFile( languageFile, chunkID ) )
			{
				return false;
			}
			if ( ! ManifestAddFile( manifestContentName, manifestBaseContentName, languageFile, chunkID, gameLanguage ) )
			{
				return false;
			}
		}
	}
	return true;
}

Bool CPackageMapFileBuilderXboxOne::AddGameContentFilesNoLanguages( const TGameContentMap& gameContentMap, Bool forceSpeeches /*= false*/ )
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;

	for ( auto it = packageFiles.m_gameContentFilesMap.Begin(); it != packageFiles.m_gameContentFilesMap.End(); ++it )
	{
		const Uint32 sortedContentNumber = it->m_first;
		const SPackageGameFiles& gameFiles = it->m_second;

		if ( ! HasUnmaskedContent( gameFiles ) && (!forceSpeeches || gameFiles.m_speechFiles.Empty()) )
		{
			continue;
		}

		const CName manifestContentName(String::Printf(TXT("%ls%u"), GetContentPrefix().AsChar(), sortedContentNumber ));

		ChunkID chunkID = INVALID_CHUNK_ID;
		if ( ! gameContentMap.Find( sortedContentNumber, chunkID ) )
		{
			ERR_WCC(TXT("Failed to find chunkID for content%u"), sortedContentNumber);
			return false;
		}
		if ( ! AddFiles( gameFiles.m_bundleFiles, chunkID ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_bundleFiles, chunkID ) )
		{
			return false;
		}

		if ( ! AddFiles( gameFiles.m_scriptFiles, chunkID ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_scriptFiles, chunkID ) )
		{
			return false;
		}

		if ( ! AddFiles( gameFiles.m_cacheFiles, chunkID ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_cacheFiles, chunkID ) )
		{
			return false;
		}

		if ( ! AddFiles( gameFiles.m_miscFiles, chunkID ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_miscFiles, chunkID ) )
		{
			return false;
		}

		if ( ! AddFiles( gameFiles.m_stringsFiles, chunkID ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_stringsFiles, chunkID ) )
		{
			return false;
		}

		if ( forceSpeeches )
		{
			if ( ! AddFiles( gameFiles.m_speechFiles, chunkID ) )
			{
				return false;
			}
			if ( ! ManifestAddFiles( manifestContentName, gameFiles.m_speechFiles, chunkID ) )
			{
				return false;
			}
		}
	}

	return true;
}

Bool CPackageMapFileBuilderXboxOne::AddGamePatchFiles( const TGamePatchMap& gamePatchMap )
{
	const SPackageFiles& packageFiles = m_packageSku.m_packageFiles;

	for ( auto it = packageFiles.m_gamePatchFilesMap.Begin(); it != packageFiles.m_gamePatchFilesMap.End(); ++it )
	{
		const Uint32 sortedPatchNumber = it->m_first;
		const TDynArray< String >& patchFiles = it->m_second;

		const CName manifestContentName(String::Printf(TXT("patch%u"), sortedPatchNumber ));

		ChunkID chunkID = INVALID_CHUNK_ID;
		if ( ! gamePatchMap.Find( sortedPatchNumber, chunkID ) )
		{
			ERR_WCC(TXT("Failed to find chunkID for patch%u"), sortedPatchNumber);
			return false;
		}
		if ( ! AddFiles( patchFiles, chunkID ) )
		{
			return false;
		}
		if ( ! ManifestAddFiles( manifestContentName, patchFiles, chunkID ) )
		{
			return false;
		}
	}

	// Add even for empty chunks, that's the whole point of these placeholders so the MakePkg.exe tool doesn't complain
	for ( auto it = gamePatchMap.Begin(); it != gamePatchMap.End(); ++it )
	{
		const Uint32 patchNumber = it->m_first;
		const Uint32 placeholderChunkID = it->m_second;
		const CName manifestContentName(String::Printf(TXT("patch%u"), patchNumber ));
		const String placeholderFile = String::Printf(TXT("patch%u.txt"), patchNumber);
		if ( !AddFile( placeholderFile, placeholderChunkID ) )
		{
			return false;
		}

		// HACK - create directly, but don't push add placeholder file to content manifest. Reserve the chunk later at least.
		// This way we don't have to filter the file out later either
		const String& lang = String::EMPTY;
		SContentManifestBuildInfo& info = m_contentManifestMap.GetRef( manifestContentName, SContentManifestBuildInfo( manifestContentName, manifestContentName, placeholderChunkID, lang ) );
		RED_UNUSED(info);	 
	}

	return true;
}

Bool CPackageMapFileBuilderXboxOne::AddFiles( const TDynArray< String >& files, ChunkID chunkID )
{
	for ( const String& file : files )
	{
		if ( ! m_xmlWriter.AddFile( file, chunkID ) )
		{
			return false;
		}
	}

	return true;
}

Bool CPackageMapFileBuilderXboxOne::AddFile( const String& file, ChunkID chunkID )
{
	TDynArray< String > files;
	files.PushBack( file );
	return AddFiles( files, chunkID );
}

Bool CPackageMapFileBuilderXboxOne::ManifestAddFiles( CName manifestContentName, CName manifestBaseContentName, const TDynArray< String >& files, ChunkID chunkID, const String& language )
{
	for ( const String& file : files )
	{
		const String conformedPath = MakeBackslash( file );
		const CName& resolvedManifestContentName  = m_manifestContentName == CName::NONE ? manifestContentName : m_manifestContentName;
		SContentManifestBuildInfo& info = m_contentManifestMap.GetRef( resolvedManifestContentName, SContentManifestBuildInfo( resolvedManifestContentName, manifestBaseContentName, chunkID, language ) );
		info.m_conformedPaths.PushBack( conformedPath );
	}

	return true;
}

Bool CPackageMapFileBuilderXboxOne::ManifestAddFiles( CName manifestContentName, const TDynArray< String >& files, ChunkID chunkID )
{
	return ManifestAddFiles( manifestContentName, manifestContentName, files, chunkID, String::EMPTY );
}

Bool CPackageMapFileBuilderXboxOne::ManifestAddFile( CName manifestContentName, CName manifestBaseContentName, const String& file, ChunkID chunkID, const String& language /*= String::EMPTY*/ )
{
	const String conformedPath = MakeBackslash( file );
	const CName& resolvedManifestContentName  = m_manifestContentName == CName::NONE ? manifestContentName : m_manifestContentName;
	SContentManifestBuildInfo& info = m_contentManifestMap.GetRef( resolvedManifestContentName, SContentManifestBuildInfo( resolvedManifestContentName, manifestBaseContentName, chunkID, language ) );
	info.m_conformedPaths.PushBack( conformedPath );

	return true;
}

Bool CPackageMapFileBuilderXboxOne::ManifestAddFile( CName manifestContentName, const String& file, ChunkID chunkID )
{
	return ManifestAddFile( manifestContentName, manifestContentName, file, chunkID, String::EMPTY );
}

Bool CPackageMapFileBuilderXboxOne::GetGameLanguage( const String& languageFile, String& outGameLanguage ) const
{
	const SPackageLanguagesXboxOne& packageLanguages = m_packageSku.m_packageLanguages;

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

String CPackageMapFileBuilderXboxOne::GetContentPrefix() const
{
	String prefix = TXT("content");
	switch (m_packageSku.m_packageType)
	{
	case ePackageType_App:
	case ePackageType_Patch: /* fall through, yes still content for patch. More cosmetic at this point anyway */
		prefix = TXT("content");
		break;
	case ePackageType_Dlc:
		prefix = TXT("dlc");
		break;
	default:
		RED_FATAL("Unhandled package type %u", m_packageSku.m_packageType );
		break;
	}

	return prefix;
}
