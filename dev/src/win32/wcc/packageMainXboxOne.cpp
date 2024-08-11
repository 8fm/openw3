/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "shobjidl.h"
#include "shlguid.h"

#include "../../common/core/contentManifest.h"
#include "../../common/core/contentManifestParser.h"
#include "../../common/core/contentVersion.h"
#include "../../common/core/xvcPackage.h"
#include "../../common/core/xmlReader.h"

#include "packageToolXboxOne.h"
#include "packageMainXboxOne.h"
#include "packageLanguagesXboxOne.h"
#include "packageFiles.h"
#include "packageFileCollectorXboxOne.h"
#include "packageMapFileBuilderXboxOne.h"
#include "packageSkuXboxOne.h"

static Bool InitLanguagesFromConfig( const String& absolutePath, SPackageLanguagesXboxOne& outPackageLanguages );
static Bool CheckOutputPath( const String& absolutePath );
static Bool CreateFolderShortcut( const String& fromFolder, const String& toFolder, const String& shortcutName );

Bool GetPackageToolExePath( String& outExePath )
{
	const Uint32 BUFSIZE = 1024;
	Char xdkRootDir[ BUFSIZE ];
	const DWORD stringLength = ::GetEnvironmentVariable(TXT("DurangoXDK"), xdkRootDir, BUFSIZE );
	const DWORD dwError = ::GetLastError();
	if ( stringLength == 0 || stringLength > BUFSIZE )
	{
		ERR_WCC(TXT("Could not read environment variable %DurangoXDK%. Is the XDK installed? GetLastError() return 0x%08X"), dwError );
		return false;
	}

	if ( ! GFileManager->FileExist( xdkRootDir ) )
	{
		ERR_WCC(TXT("Could not find %DurangoXDK% {'%ls'}. Is the XDK installed or %DurangoXDK% out of date?"), xdkRootDir );
		return false;
	}

	String exePath = String::Printf(TXT("%ls\\bin\\MakePkg.exe"), xdkRootDir );

	if ( ! GFileManager->FileExist( exePath ) )
	{
		ERR_WCC(TXT("Could not find MakePkg.exe at '%ls'. Is the XDK fully installed?"), exePath.AsChar() );
		return false;
	}

	outExePath = Move(exePath);

	return true;
}

static Bool FilterLanguages( SPackageFiles& packageFiles, const SPackageLanguagesXboxOne& packageLanguages, const SOptionsXboxOne& options )
{
	struct SLangSupport: Red::System::NonCopyable
	{
		const SPackageLanguagesXboxOne& m_packageLanguages;
		const TDynArray< String >& m_textLanguages;
		const TDynArray< String >& m_speechLanguages;

		SLangSupport( const SPackageLanguagesXboxOne& packageLanguages, const TDynArray< String >& textLanguages, const TDynArray< String >& speechLanguages )
			: m_packageLanguages( packageLanguages )
			, m_textLanguages( textLanguages )
			, m_speechLanguages( speechLanguages )
		{}

		Bool IsSupportedLanguageFile( const String& languageFile )
		{
			CFilePath filePath( languageFile );
			if ( ! filePath.HasFilename() )
			{
				ERR_WCC(TXT("Language file '%ls' does not specify a file!"), languageFile.AsChar() );
				return false;
			}
			const String fileName = filePath.GetFileNameWithExt();
			String gameLanguage;
			if ( ! m_packageLanguages.m_languageFileToGameLangMap.Find( fileName, gameLanguage ) )
			{
				return false;
			}

			if ( fileName.EndsWith(TXT(".w3strings")) )
			{
				return m_textLanguages.Exist( gameLanguage );
			}
			else if ( fileName.EndsWith(TXT(".w3speech")) )
			{
				return m_speechLanguages.Exist( gameLanguage );
			}

			ERR_WCC(TXT("IsSupportedLanguageFile: Unknown file type for file '%ls'"), fileName.AsChar() );

			return false;	
		}
	};

	// Check if have string, then have language file!
	for ( const String& speechLang : options.m_speechLanguages )
	{
		if ( ! options.m_textLanguages.Exist( speechLang ) )
		{
			ERR_WCC(TXT("Speech language {%ls} was not given a matching strings file on the command line!"), speechLang.AsChar() );
			return false;
		}
	}

	// Filter out text/speech files here vs errors later
	SLangSupport langSupport( packageLanguages, options.m_textLanguages, options.m_speechLanguages );
	for ( auto it = packageFiles.m_gameContentFilesMap.Begin(); it != packageFiles.m_gameContentFilesMap.End(); ++it )
	{
		SPackageGameFiles& gameFiles = it->m_second;
		for ( Int32 j = gameFiles.m_speechFiles.SizeInt()-1; j >= 0; --j )
		{
			const String& speechFile = gameFiles.m_speechFiles[j];
			if ( !langSupport.IsSupportedLanguageFile( speechFile ) )
			{
				LOG_WCC(TXT("Excluding unsupported speech file '%ls'"), speechFile.AsChar() );
				gameFiles.m_speechFiles.RemoveAt( j );
			}
		}
		for ( Int32 j = gameFiles.m_stringsFiles.SizeInt()-1; j >= 0; --j )
		{
			const String& stringsFile = gameFiles.m_stringsFiles[j];
			if ( !langSupport.IsSupportedLanguageFile( stringsFile ) )
			{
				LOG_WCC(TXT("Excluding unsupported strings file '%ls'"), stringsFile.AsChar() );
				gameFiles.m_stringsFiles.RemoveAt( j );
			}
		}
	}

	return true;
}

static Bool CreateContentManifest( SContentManifest& outContentManifest, EPackageType packageType, const String& appRoot, TDynArray< SPackageContentManifestXboxOne >& manifests, const SOptionsXboxOne &options )
{
	struct Check
	{
		static Bool IsPatch( const Char* str )
		{
			static size_t len = Red::System::StringLengthCompileTime( "patch" );
			return Red::System::StringCompare( str, TXT("patch"), len ) == 0;
		}
	};

	SContentPack newContentPack;
	if ( packageType == ePackageType_Patch )
	{
		newContentPack.m_id = CNAME(patch);
		newContentPack.m_dependency = CNAME(content);
	}
	else
	{
		newContentPack.m_id = ( packageType == ePackageType_Dlc ) ? CNAME(dlc) : CNAME(content);
		newContentPack.m_dependency = ( packageType == ePackageType_Dlc ) ? CNAME(content) : CName::NONE;
	}

	// For now, only set if DLC to keep the change limited
	if ( packageType == ePackageType_Dlc )
	{
		newContentPack.m_minVersion = CONTENT_VERSION; // TBD: override?
	}

	Red::System::DateTime newPackTimestamp;

	// Append "content" whether app or DLC
	String contentRoot;
	contentRoot = appRoot + TXT("content\\");

	//////////////////////////////////////////////////////////////////////////
	// Redundant with PS4
	// Generate manifest files.
	// Sort for easier progress tracking
	Sort( manifests.Begin(), manifests.End(), [](const SPackageContentManifestXboxOne& a, const SPackageContentManifestXboxOne& b ){ return a.m_contentName < b.m_contentName; });
	for ( const SPackageContentManifestXboxOne& manifest : manifests )
	{
		// For now just a stub
		if ( Check::IsPatch( manifest.m_contentName.AsChar() ) && manifest.m_conformedFilePaths.Empty() )
		{
			// Create an empty chunk, don't bother putting the placeholder file into it. It's there just for MakePkg.exe to not complain
			SContentChunk newPatchChunk;
			const CName id = manifest.m_contentName;
			newPatchChunk.m_chunkLabel = id;
			newContentPack.m_contentChunks.PushBack( Move( newPatchChunk ) );
			continue;
		}

		TDynArray< String > absolutePaths;
		for ( const String& path : manifest.m_conformedFilePaths )
		{
			const String absPath = appRoot + path;
			absolutePaths.PushBack( absPath );
		}
		
		SContentChunk newContentChunk;
		Red::System::DateTime timestamp;
		const CName id = manifest.m_contentName;
		//const String dependency = String::EMPTY; // For now
		LOG_WCC(TXT("Creating manifest chunk for '%ls'..."), id.AsChar());
		if ( !Helper::CreateContentChunk( newContentChunk, timestamp, id, absolutePaths, contentRoot, true, options.m_skipCRC ) )
		{
			ERR_WCC(TXT("Failed to create content manifest for %ls"), id.AsChar() );
			return false;
		}
		LOG_WCC(TXT("Done"));

		if ( timestamp > newPackTimestamp )
		{
			newPackTimestamp = timestamp;
		}

		if ( Check::IsPatch( manifest.m_contentName.AsChar() ) )
		{
			// another lame hack
			for ( SContentFile& file : newContentChunk.m_contentFiles )
			{
				file.m_isPatch = true;
			}
		}

		newContentPack.m_contentChunks.PushBack( Move( newContentChunk ) );
	}

	newContentPack.m_timestamp = newPackTimestamp;

	outContentManifest.m_contentPack = Move( newContentPack );

	return true;
}

static Bool SaveContentManifest( const SContentManifest& contentManifest, const String& absoluteManifestFilePath )
{
	CContentManifestWriter manifestWriter( contentManifest );
	String manifestXml;
	if ( !manifestWriter.ParseManifest( manifestXml ) )
	{
		ERR_WCC(TXT("Failed to parse manifest XML!"));
		return false;
	}

	if ( ! GFileManager->SaveStringToFile( absoluteManifestFilePath, manifestXml ) )
	{
		ERR_WCC(TXT("Failed to save content manifest for file '%ls'"), absoluteManifestFilePath.AsChar() );
		return false;
	}

	return true;
}

Bool PackageMainXboxOne( const SOptionsXboxOne& options )
{
	String exePath;
	if ( !GetPackageToolExePath( exePath ) )
	{
		return false;
	}

	CPackageToolXboxOne packageTool( exePath );
// 	if ( ! packageTool.Version() )
// 	{
// 		ERR_WCC(TXT("Failed to invoke the packageTool '%ls'"), exePath.AsChar() );
// 		return false;
// 	}

	EPackageType packageType = ePackageType_App;
	if (options.m_packageType.EqualsNC(TXT("app")))
	{
		packageType = ePackageType_App;
	}
	else if (options.m_packageType.EqualsNC(TXT("patch")))
	{
		packageType = ePackageType_Patch;
	}
	else if (options.m_packageType.EqualsNC(TXT("dlc")))
	{
		packageType = ePackageType_Dlc;
	}
	else
	{
		ERR_WCC(TXT("Unsupported packageType %ls"), options.m_packageType.AsChar());
		return false;
	}

	SPackageLanguagesXboxOne packageLanguages;

	if ( packageType != ePackageType_Dlc )
	{
		if ( ! InitLanguagesFromConfig( options.m_langDir, packageLanguages ) )
		{
			return false;
		}

		packageLanguages.m_supportedSpeechLanguages = options.m_speechLanguages;
		packageLanguages.m_defaultSpeechLanguage = options.m_defaultSpeech;
	}

	const String appRoot = options.m_inDir;

	SPackageFiles packageFiles;
	CPackageFileCollectorXboxOne packageFileCollector( packageType );
	if ( ! packageFileCollector.CollectPackageFiles( appRoot, packageFiles ) )
	{
		WARN_WCC(TXT("Failed to collect package files!"));
		return false;
	}

	if ( packageType != ePackageType_Dlc )
	{
		if ( !FilterLanguages( packageFiles, packageLanguages, options ) )
		{
			return false;
		}
	}

	SPackageMapFileOptionsXboxOne packageMapFileOptions( options.m_exeName );
	packageMapFileOptions.m_launchContentNumber = options.m_launchContentNumber;

	SPackageSkuXboxOne packageSku( packageType, packageFiles, packageLanguages );

	const String& chunksXmlPath = options.m_tempDir + TXT("XboxOne_AppChunks.xml");
	IFile* xmlFile = GFileManager->CreateFileWriter( chunksXmlPath, FOF_AbsolutePath | FOF_Buffered );
	if ( ! xmlFile )
	{
		ERR_WCC(TXT("Failed to create chunks XML file '%ls'"), chunksXmlPath.AsChar() );
		return false;
	}

	TDynArray< SPackageContentManifestXboxOne > manifests;
	CName manifestContentName( options.m_manifestContentName.AsChar() );
	CPackageMapFileBuilderXboxOne packageMapFileBuilder( *xmlFile, packageMapFileOptions, packageSku, manifestContentName );
	if ( ! packageMapFileBuilder.BuildProject( manifests ) )
	{
		delete xmlFile;
		return false;
	}

	delete xmlFile;
	xmlFile = nullptr;
	
	// Generate manifest files in own top level directory, since agnostic of how chunks are formed from content dirs
	// and defaulting to content0 if not a default content dir. TBD: patches and DLCs
	String absoluteManifestFilePath = appRoot + TXT("content\\") + MANIFEST_FILE_NAME;

	SContentManifest newContentManifest;
	if ( !CreateContentManifest(newContentManifest, packageType, appRoot, manifests, options) )
	{
		return false;
	}

	if ( !SaveContentManifest(newContentManifest, absoluteManifestFilePath) )
	{
		return false;
	}

	// On Xbox, need to reconstruct the whole package, so build installer.dat even if just a patch
	if ( packageType != ePackageType_Dlc )
	{
		// Text languages because they should be a superset of speech languages
		SXvcPackage xvcPackage;
		for ( const String& gameLang : options.m_textLanguages )
		{
			String localeLang;
			if ( ! packageSku.m_packageLanguages.m_gameToLocaleMap.Find( gameLang, localeLang ) )
			{
				ERR_WCC(TXT("Failed to get locale for game language language '%ls'"), gameLang.AsChar() );
				return false;
			}

			const SXvcLocale xvcLocale( UNICODE_TO_ANSI(localeLang.ToLower().AsChar()) );
			xvcPackage.m_languageLocaleMap.Insert( UNICODE_TO_ANSI( gameLang.ToLower().AsChar() ), xvcLocale );
		}

		// Sort by install order to populate playGoPackage.m_installOrderGameContent
		::Sort( manifests.Begin(), manifests.End(), [](const SPackageContentManifestXboxOne& a, const SPackageContentManifestXboxOne& b ) { return a.m_installOrder < b.m_installOrder; } );
		// Precreate all non language chunks for N^2 look up creating language chunk dependencies
		// FIXME: Gotten a bit messy how it works, "contentName" and "baseContentName" should go and just simply be
		// content can be composed of multiple chunks
		TDynArray< SXvcContent > xvcContent;
		CName launchContentName;
		for ( const SPackageContentManifestXboxOne& manifest : manifests )
		{
			const Uint32 chunkID = manifest.m_chunkID;
			const CName manifestContentName = manifest.m_contentName;
			const CName manifestBaseContentName = manifest.m_baseContentName;
			const String& language = manifest.m_language;
			if ( manifestContentName == manifestBaseContentName )
			{
				if ( !language.Empty() )
				{
					ERR_WCC(TXT("Non split Language chunk %ls? Shouldn't exist %ls/%ls"), language.AsChar(), manifestBaseContentName.AsChar(), manifestContentName.AsChar() );
					return false;
				}

				const Bool isLaunchContent = manifest.m_isLaunchContent;
				TDynArray< SXvcLanguageChunk > langChunks; // To be filled in next step
				SXvcContent content( manifest.m_contentName, manifest.m_chunkID, langChunks );
				xvcContent.PushBack( content );
				if ( isLaunchContent )
				{
					launchContentName = manifestContentName;
				}
			}
		}

		xvcPackage.m_binChunkID = BIN_CHUNK_ID_XBOX;

		if ( ! launchContentName )
		{
			ERR_WCC(TXT("No launchContentName!"));
			return false;
		}
		xvcPackage.m_launchContentName = launchContentName;

		for ( const SPackageContentManifestXboxOne& manifest : manifests )
		{
			const Uint32 chunkID = manifest.m_chunkID;
			const CName manifestContentName = manifest.m_contentName;
			const CName manifestBaseContentName = manifest.m_baseContentName;
			const String& language = manifest.m_language;

			xvcPackage.m_chunkNameMap.Insert( chunkID, manifestContentName );

			if ( manifestContentName != manifestBaseContentName )
			{
				if ( language.Empty() )
				{
					ERR_WCC(TXT("Split non-language chunk? Shouldn't exist %ls/%ls"), manifestBaseContentName.AsChar(), manifestContentName.AsChar() );
					return false;
				}

				String localeName;
				if ( !packageLanguages.m_gameToLocaleMap.Find( manifest.m_language, localeName ) )
				{
					ERR_WCC(TXT("Could not find locale for game language %ls"), manifest.m_language.AsChar());
					return false;
				}

				const SXvcLocale xvcLocale( UNICODE_TO_ANSI( localeName.AsChar() ) );
				for ( SXvcContent& content : xvcContent )
				{
					if ( content.m_contentName == manifestBaseContentName )
					{
						SXvcLanguageChunk langChunk( xvcLocale, chunkID );
						if ( !content.m_languageChunks.PushBackUnique( langChunk ) )
						{
							ERR_WCC(TXT("Non unique language chunk %u"), chunkID);
							return false;
						}
					}
				}
			}
		}

		// Make the languaqge install order pretty. Doesn't really matter since language masked.
		for ( SXvcContent& content : xvcContent )
		{
			TDynArray< SXvcLanguageChunk >& langChunks = content.m_languageChunks;
			::Sort( langChunks.Begin(), langChunks.End(), [](const SXvcLanguageChunk& a, const SXvcLanguageChunk& b){ return a.m_chunkID < b.m_chunkID; } );
		}
		xvcPackage.m_installOrderContent = Move( xvcContent );

		for ( const String& gameLang : options.m_speechLanguages )
		{
			xvcPackage.m_supportedSpeechLanguages.PushBack( UNICODE_TO_ANSI( gameLang.ToLower().AsChar() ) );
		}
		for ( const String& gameLang : options.m_textLanguages )
		{
			xvcPackage.m_supportedTextLanguages.PushBack( UNICODE_TO_ANSI( gameLang.ToLower().AsChar() ) );
		}

		xvcPackage.m_defaultSpeechLanguage = UNICODE_TO_ANSI( options.m_defaultSpeech.ToLower().AsChar() );

		const String xvcDatFilePath = appRoot + INSTALLER_DAT_FILE;
		IFile* writer = GFileManager->CreateFileWriter( xvcDatFilePath, FOF_AbsolutePath | FOF_Buffered );
		if ( ! writer )
		{
			ERR_WCC(TXT("Could not open '%ls' for writing"), xvcDatFilePath.AsChar() );
			return false;
		}

		*writer << xvcPackage;
		delete writer;
		writer = nullptr;

#ifdef RED_LOGGING_ENABLED
		::DumpXvcPackage( xvcPackage );
#endif

	} // packageType != ePackageType_Dlc

	return true;
}

static Bool CreateFolderShortcut( const String& fromFolder, const String& toFolder, const String& shortcutName )
{
	IShellLink* psl = nullptr;

	String normFromFolder = fromFolder;
	normFromFolder.ReplaceAll(TXT("/"), TXT("\\"));
	normFromFolder.ReplaceAll(TXT("\\\\"), TXT("\\"));
	if ( ! normFromFolder.EndsWith(TXT("\\")) )
	{
		normFromFolder += TXT("\\");
	}

	String normToFolder = toFolder;
	normToFolder.ReplaceAll(TXT("/"), TXT("\\"));
	normToFolder.ReplaceAll(TXT("\\\\"), TXT("\\"));
	if ( ! normToFolder.EndsWith(TXT("\\")) )
	{
		normToFolder += TXT("\\");
	}

	if ( FAILED(::CoInitialize(nullptr)) )
	{
		return false;
	}

	HRESULT hr = ::CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl );
	if (SUCCEEDED(hr))
	{ 
		IPersistFile* ppf = nullptr;

		psl->SetPath(normToFolder.AsChar()); 
		psl->SetDescription(TXT("Build location")); 

		hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
		if (SUCCEEDED(hr)) 
		{ 
			const String savePath = normFromFolder + shortcutName + TXT(".lnk");
			hr = ppf->Save(savePath.AsChar(), TRUE); 
			ppf->Release(); 
		} 
		psl->Release(); 
	} 

	::CoUninitialize();

	return SUCCEEDED(hr);
}

static Bool PopulateLanguages( const String& absolutePath, const String& keyHeader, const String& valueHeader, THashMap<String, String>& outHashMap )
{
	// Don't clear with new hashmap, append
	THandle< C2dArray > ar = C2dArray::CreateFromString( absolutePath );
	if ( ! ar )
	{
		ERR_WCC(TXT("Failed to create CSV from file '%ls'"), absolutePath.AsChar());
		return false;
	}

	Uint32 keyColumn = 0;
	if ( ! ar->FindHeader( keyHeader, keyColumn ) )
	{
		ERR_WCC(TXT("Failed to find %ls column in file '%ls'"), keyHeader.AsChar(), absolutePath.AsChar() );
		return false;
	}

	Uint32 valueColumn = 0;
	if ( ! ar->FindHeader( valueHeader, valueColumn ) )
	{
		ERR_WCC(TXT("Failed to find %ls column in file '%ls'"), valueHeader.AsChar(), absolutePath.AsChar() );
		return false;
	}

	const Uint32 numCols = ar->GetNumberOfColumns();
	const Uint32 numRows = ar->GetNumberOfRows();
	for (Uint32 i = 0; i < numRows; ++i)
	{
		const String& key = ar->GetValueRef( keyColumn, i );
		const String& value = ar->GetValueRef( valueColumn, i );
		if ( key.Empty() )
		{
			continue;
		}
		if ( value.Empty() )
		{
			WARN_WCC(TXT("%ls '%ls' is not mapped to any %ls in file '%ls'"), keyHeader.AsChar(), key.AsChar(), valueHeader.AsChar(), absolutePath.AsChar());
			continue;
		}

		if ( ! outHashMap.Insert( key.ToLower(), value ) )
		{
			ERR_WCC(TXT("Failed to uniquely map %ls '%ls' to %ls '%ls' in file '%ls'"), keyHeader.AsChar(), key.AsChar(), valueHeader.AsChar(), value.AsChar(), absolutePath.AsChar() );
			return false;
		}
	}

	return true;
}

static Bool InitLanguagesFromConfig( const String& absolutePath, SPackageLanguagesXboxOne& outPackageLanguages )
{
	SPackageLanguagesXboxOne newPackageLanguage;

	const String localeLangMapPath( absolutePath + TXT("locale-languagemap.csv") );
	if ( !PopulateLanguages( localeLangMapPath, TXT("GameLanguage"), TXT("XboxLocale"), newPackageLanguage.m_gameToLocaleMap ) )
	{
		return false;
	}

	const String filesLanguageMap( absolutePath + TXT("files-languagemap.csv") );
	if ( !PopulateLanguages( filesLanguageMap, TXT("Strings"), TXT("GameLanguage"), newPackageLanguage.m_languageFileToGameLangMap ) )
	{
		return false;
	}

	if ( !PopulateLanguages( filesLanguageMap, TXT("Speech"), TXT("GameLanguage"), newPackageLanguage.m_languageFileToGameLangMap ) )
	{
		return false;
	}

	for ( auto it = newPackageLanguage.m_languageFileToGameLangMap.Begin(); it != newPackageLanguage.m_languageFileToGameLangMap.End(); ++it )
	{
		String& value = it->m_second;
		CUpperToLower conv( value.TypedData(), value.Size() );
	}

	outPackageLanguages = Move( newPackageLanguage );

	return true;
}

static Bool CheckOutputPath( const String& absolutePath )
{
	CFilePath filePath( absolutePath );
	if ( filePath.GetNumberOfDirectories() < 1 )
	{
		return false;
	}

	// Get the directory if given a path to a file
	const String& pathToCheck = filePath.GetPathString();
	if ( !GSystemIO.CreateDirectory( pathToCheck.AsChar() ) )
	{
		ERR_WCC(TXT("Failed to get directory path '%ls'"), absolutePath.AsChar());
		return false;
	}

	if ( filePath.HasFilename() )
	{
		// Should probably devise a better test, but hopefully would catch things like the file being used by another process etc
		// Of course not a good test if there's an "attacker"
		IFile* writer = GFileManager->CreateFileWriter( absolutePath, FOF_AbsolutePath );
		if ( ! writer )
		{
			ERR_WCC(TXT("Path not writable '%ls'"), absolutePath.AsChar() );
			return false;
		}
		delete writer;
		writer = nullptr;

		if ( ! GFileManager->DeleteFile( absolutePath ) )
		{
			ERR_WCC(TXT("Failed to cleanup test file '%ls'"), absolutePath.AsChar() );
			return false;
		}
	}

	return true;
}
