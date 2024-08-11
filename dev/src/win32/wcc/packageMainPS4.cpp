/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "shobjidl.h"
#include "shlguid.h"

#include "../../common/core/contentManifest.h"
#include "../../common/core/contentManifestParser.h"
#include "../../common/core/contentVersion.h"
#include "../../common/core/xmlReader.h"
#include "../../common/core/playGoPackage.h"

#include "packageProjectBuilderPS4.h"
#include "packageFileCollectorPS4.h"
#include "packageFiles.h"
#include "packageConstants.h"
#include "packageLanguagesPS4.h"
#include "packageSkuPS4.h"
#include "packageToolPS4.h"
#include "packagePlayGoChunksBuilder.h"
#include "packageChunkLayerPairPS4.h"
#include "packageMainPS4.h"
#include "packagePlayGoChunksBuilder.h"

const SChunkLayerInfoPS4 SChunkLayerInfoPS4::INVALID;

//////////////////////////////////////////////////////////////////////////

static Bool InitLanguagesFromConfig( const String& absolutePath, SPackageLanguagesPS4& outPackageLanguages );
static Bool CheckOutputPath( const String& absolutePath );
static Bool CreateFolderShortcut( const String& fromFolder, const String& toFolder, const String& shortcutName );
static Bool GetPlayGoLanguageFlag( const Char* playGoLanguage, TPlayGoLanguageFlag& outFlag );
static Bool GetPackageToolExePath( String& outExePath, const SOptionsPS4& options );
static Bool FilterLanguages( SPackageFiles& packageFiles, const SPackageLanguagesPS4& packageLanguages, const SOptionsPS4& options );
static void RemoveBuildServerCrap( SPackageFiles& packageFiles);
static Bool CreatePrefetchGroup( const SOptionsPS4& options, SPackageFiles& packageFiles );
static Bool CreateStubPlayGoPackage( const String& playGoDatFilePath );
static Bool CreateContentManifest( SContentManifest& outManifest, EPackageType packageType, const String& appRoot, TDynArray< SPackageContentManifestPS4 >& manifests, const SOptionsPS4 &options );
static Bool SaveContentManifest( const SContentManifest& contentManifest, const String& absoluteManifestFilePath );
static Bool PlayGoPackage_InitLang( const SOptionsPS4& options, const SPackageSkuPS4& packageSku, SPlayGoPackage& playGoPackage );
static Bool PlayGoPackage_InitLaunch( TDynArray< SPackageContentManifestPS4 >& manifests, SPlayGoPackage& playGoPackage, TDynArray< SPlayGoContent >& playGoContent );
static Bool PlayGoPackage_InitContent( SPlayGoPackage& playGoPackage, TDynArray< SPlayGoContent >& playGoContent, const TDynArray< SPackageContentManifestPS4 >& manifests, const SPackageLanguagesPS4& packageLanguages );
static void PlayGoPackage_InitLanguages( SPlayGoPackage &playGoPackage, const SOptionsPS4 &options );
static Bool PlayGoPackage_VerifyChunks( const SPlayGoPackage& playGoPackage );
static Bool PlayGoPackage_Init( SPlayGoPackage& playGoPackage, const SOptionsPS4& options, const SPackageSkuPS4& packageSku, TDynArray< SPackageContentManifestPS4 >& manifests, const SPackageLanguagesPS4& packageLanguages );
static Bool PlayGoPackage_InitLanguageOnly( SPlayGoPackage& playGoPackage, const SOptionsPS4& options, const SPackageSkuPS4& packageSku, TDynArray< SPackageContentManifestPS4 >& manifests, const SPackageLanguagesPS4& packageLanguages );
static const Bool SavePlayGoPackage( const String& playGoDatFilePath, const SPlayGoPackage& playGoPackage );
static Bool CreateSubmissionPackage( const SOptionsPS4& options, CPackageToolPS4& packageTool, const String& gp4ProjectPath );
static void CreateBuildShortcut( const SOptionsPS4& options );
static Bool LoadPreviousContentManifest( SContentManifest& outManifest, const SPackageSkuPS4& packageSku, const SOptionsPS4 &options, CPackageToolPS4& packageTool, const String relativeContentManifestFilePath );
static void MergeContentFiles( SContentChunk& mergeTo, const SContentChunk& mergeFrom );
static Bool MergeWithPreviousContentManifest( SContentManifest& newContentManifest, const String& relativeContentManifestFilePath, const SPackageSkuPS4& packageSku, const SOptionsPS4& options, CPackageToolPS4& packageTool );
static Bool MergeWithPreviousPlayGoPackage( SPlayGoPackage& newPlayGoPackage, const String& relativePackageFilePath, const SPackageSkuPS4& packageSku, const SOptionsPS4& options, CPackageToolPS4& packageTool );
static Bool LoadBasePackage( PackageHelpers::SPackageInfo& basePackage, const SOptionsPS4 &options );
static void MakePatchManifest( SContentManifest &contentManifest );

//////////////////////////////////////////////////////////////////////////

Bool PackageMainPS4( const SOptionsPS4& options )
{
	String exePath;
	if ( !GetPackageToolExePath( exePath, options ) )
	{
		return false;
	}

	CPackageToolPS4 packageTool( exePath );
	if ( ! packageTool.Version() )
	{
		ERR_WCC(TXT("Failed to invoke the packageTool '%ls'"), exePath.AsChar() );
		return false;
	}

	EPackageType packageType = ePackageType_App;
	if (options.m_packageType.EqualsNC(TXT("app")))
	{
		packageType = ePackageType_App;
	}
	else if ( options.m_packageType.EqualsNC(TXT("dlc")))
	{
		packageType = ePackageType_Dlc;
	}
	else if ( options.m_packageType.EqualsNC(TXT("patch")))
	{
		packageType = ePackageType_Patch;
	}
	else
	{
		ERR_WCC(TXT("Unsupported package type '%ls'"), options.m_packageType.AsChar() );
		return false;
	}

	SPackageLanguagesPS4 packageLanguages;

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
	CPackageFileCollectorPS4 packageFileCollector( packageType );
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

	RemoveBuildServerCrap( packageFiles );

	if ( packageType != ePackageType_Dlc )
	{
		if (!CreatePrefetchGroup(options, packageFiles))
		{
			return false;
		}
	}

	SPackageProjectOptionsPS4 packageProjectOptions( options.m_elfName );	
	packageProjectOptions.m_languageMaskSpeech = true;
	packageProjectOptions.m_launchContentNumber = options.m_launchContentNumber;
	packageProjectOptions.m_compressSpeechFiles = true;
	
	SPackageSkuPS4 packageSku( packageType, packageFiles, packageLanguages );
	packageSku.m_contentID = options.m_contentID;
	packageSku.m_passcode = options.m_passcode;
	packageSku.m_entitlementKey = options.m_dlcEntitlementKey;
	packageSku.m_patchParams.m_appPkgPath = options.m_appPkgPath;
	packageSku.m_patchParams.m_isDayOne = options.m_isDayOne;
	packageSku.m_patchParams.m_latestPatchPath = options.m_latestPatchPath;

	const String& batchPath = options.m_tempDir + packageSku.m_contentID + TXT("_GP4Batch.txt");
	IFile* batchFileWriter = GFileManager->CreateFileWriter( batchPath, FOF_AbsolutePath | FOF_Buffered );
	if ( ! batchFileWriter )
	{
		ERR_WCC(TXT("Failed to create package batch file '%ls'"), batchPath.AsChar() );
		return false;
	}

	if ( packageType == ePackageType_Patch )
	{
		if ( !LoadBasePackage( packageSku.m_patchParams.m_basePackage, options) )
		{
			return false;
		}
	}
	
	TDynArray< SPackageContentManifestPS4 > manifests;

	CName manifestContentName( options.m_manifestContentName.AsChar() );

	CPackageProjectBuilderPS4 packageProjectBuilder( *batchFileWriter, packageProjectOptions, packageSku, manifestContentName );
	if ( ! packageProjectBuilder.BuildProject( manifests ) )
	{
		delete batchFileWriter;
		return false;
	}

	// Create an empty PLAYGO_DAT_FILE or else the batch processor will complain about the missing file
	const String playGoDatFilePath = appRoot + PLAYGO_DAT_FILE;

	if ( packageType != ePackageType_Dlc )
	{
		if ( !CreateStubPlayGoPackage(playGoDatFilePath) )
		{
			return false;
		}
	}

	SContentManifest newContentManifest;
	if ( !CreateContentManifest( newContentManifest, packageType, appRoot, manifests, options) )
	{
		return false;
	}

	if ( packageType == ePackageType_Patch )
	{
		MakePatchManifest(newContentManifest);
	}

	const String relativeContentManifestFilePath = String::Printf(TXT("content\\%ls"), MANIFEST_FILE_NAME);

	if ( packageType == ePackageType_Patch )
	{
		if ( !MergeWithPreviousContentManifest( newContentManifest, relativeContentManifestFilePath, packageSku, options, packageTool ) )
		{
			return false;
		}
	}

	// Generate manifest files in own top level directory, since agnostic of how chunks are formed from content dirs
	// and defaulting to content0 if not a default content dir. TBD: patches and DLCs
	String absoluteManifestFilePath = appRoot + relativeContentManifestFilePath;
	if ( !SaveContentManifest(newContentManifest, absoluteManifestFilePath) )
	{
		return false;
	}

	delete batchFileWriter;
	batchFileWriter = nullptr;

	const String gp4ProjectPath( options.m_outDir + packageSku.m_contentID + TXT(".gp4"));
	if ( ! CheckOutputPath( gp4ProjectPath ) )
	{
		return false;
	}

	if ( ! packageTool.CreateProject( appRoot, batchPath, gp4ProjectPath ) )
	{
		ERR_WCC(TXT("Failed to create project file '%ls'"), gp4ProjectPath.AsChar() );
		return false;
	}
	
	LOG_WCC(TXT("Created package file '%ls'"), gp4ProjectPath.AsChar() );

	if ( packageType != ePackageType_Dlc )
	{
		SPlayGoPackage playGoPackage;

		if ( packageType == ePackageType_Patch )
		{
			if ( !PlayGoPackage_InitLanguageOnly( playGoPackage, options, packageSku, manifests, packageLanguages) )
			{
				return false;
			}
			if ( !MergeWithPreviousPlayGoPackage( playGoPackage, PLAYGO_DAT_FILE, packageSku, options, packageTool ) )
			{
				return false;
			}
		}
		else if ( !PlayGoPackage_Init( playGoPackage, options, packageSku, manifests, packageLanguages) )
		{
			return false;
		}

		if ( !SavePlayGoPackage(playGoDatFilePath, playGoPackage) )
		{
			return false;
		}

#ifdef RED_LOGGING_ENABLED
		::DumpPlayGoPackage( playGoPackage );
#endif

	} // if ( packageType != ePackageType_Dlc )

	// SHOULD VERIFY THAT THE playgo-redat is in the packge!!!

	LOG_WCC(TXT("Creating submission package. This may take a while..."));
	
	if ( !CreateSubmissionPackage(options, packageTool, gp4ProjectPath) )
	{
		return false;
	}

	CreateBuildShortcut(options);

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

static Bool InitLanguagesFromConfig( const String& absolutePath, SPackageLanguagesPS4& outPackageLanguages )
{
	SPackageLanguagesPS4 newPackageLanguage;

	const String playgoLangMapPath( absolutePath + TXT("playgo-languagemap.csv") );
	if ( ! PopulateLanguages( playgoLangMapPath, TXT("GameLanguage"), TXT("PlayGoLanguage"), newPackageLanguage.m_gameToPlayGoLangMap ) )
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

//////////////////////////////////////////////////////////////////////////


static Bool GetPackageToolExePath( String& outExePath, const SOptionsPS4& options )
{
	const Uint32 BUFSIZE = 1024;
	Char sceRootDir[ BUFSIZE ];
	const DWORD stringLength = ::GetEnvironmentVariable(TXT("SCE_ROOT_DIR"), sceRootDir, BUFSIZE );
	const DWORD dwError = ::GetLastError();
	if ( stringLength == 0 || stringLength > BUFSIZE )	{
		ERR_WCC(TXT("Could not read environment variable %SCE_ROOT_DIR%. Is the PS4 SDK installed? GetLastError() return 0x%08X"), dwError );
		return false;
	}

	if ( ! GFileManager->FileExist( sceRootDir ) )
	{
		ERR_WCC(TXT("Could not find %SCE_ROOT_DIR% {'%ls'}. Is the PS4 SDK installed or %SCE_ROOT_DIR% out of date?"), sceRootDir );
		return false;
	}

	const String exePath = String::Printf(TXT("%ls\\ORBIS\\Tools\\Publishing Tools\\bin\\orbis-pub-cmd.exe"), sceRootDir );
	if ( ! GFileManager->FileExist( exePath ) )
	{
		ERR_WCC(TXT("Could not find orbis-pub-cmd.exe at '%ls'. Are the PS4 SDK publishing tools fully installed?"), exePath.AsChar() );
		return false;
	}

	outExePath = Move(exePath);

	return true;
}

static Bool FilterLanguages( SPackageFiles& packageFiles, const SPackageLanguagesPS4& packageLanguages, const SOptionsPS4& options )
{
	struct SLangSupport: Red::System::NonCopyable
	{
		const SPackageLanguagesPS4& m_packageLanguages;
		const TDynArray< String >& m_textLanguages;
		const TDynArray< String >& m_speechLanguages;

		SLangSupport( const SPackageLanguagesPS4& packageLanguages, const TDynArray< String >& textLanguages, const TDynArray< String >& speechLanguages )
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

	// Text languages because they should be a superset of speech languages
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

static void RemoveBuildServerCrap( SPackageFiles& packageFiles)
{
	// Remove more crap files!
	for ( Int32 j = packageFiles.m_gameBinFiles.SizeInt()-1; j >= 0; --j )
	{
		const String& fileName = packageFiles.m_gameBinFiles[j];
		if ( fileName.ContainsSubstring(TXT("shaders_directx11")) || fileName.ContainsSubstring(TXT("shaders_durango")) )
		{
			LOG_WCC(TXT("Removing crap file %ls"), fileName.AsChar() );
			packageFiles.m_gameBinFiles.RemoveAt( j );
		}
	}
}

static Bool CreatePrefetchGroup( const SOptionsPS4& options, SPackageFiles& packageFiles )
{
	// Move content0 files into prefetch, if any
	THashSet< String > prefetchFileNames;
	if ( !options.m_prefetchFile.Empty() )
	{
		String buf;
		if ( ! GFileManager->LoadFileToString( options.m_prefetchFile, buf, true ) )
		{
			ERR_WCC(TXT("Failed to open prefetch file '%ls'"), options.m_prefetchFile.AsChar() );
			return false;
		}
		TDynArray< String > lines = buf.Split(TXT("\r\n"));
		for ( String& line : lines )
		{
			line.Trim();
			if ( line.Empty() )
			{
				continue;
			}
			CFilePath filePath( line );
			const String& fileName = filePath.GetFileNameWithExt().ToLower();
			if ( fileName.EndsWith(TXT(".w3speech")) /*|| fileName.EndsWith(TXT(".w3strings"))*/)
			{
				// Need to change language assumptions and content mapping in the cooked stringsdb, and compressing files
				// If strings not split, go for it and prefetch
				ERR_WCC(TXT("Prefetching speech files not supported!"));
				return false;
			}
			prefetchFileNames.Insert( fileName );
			LOG_WCC(TXT("Will attempt to prefetch file: %ls"), fileName.AsChar() );
		}

		if ( packageFiles.m_gameContentFilesMap.Empty() || packageFiles.m_gameContentFilesMap.Begin()->m_first != 0 )
		{
			ERR_WCC(TXT("No content0 for prefetch!"));
			return false;
		}

		SPackageGameFiles& contentZero = packageFiles.m_gameContentFilesMap.Begin()->m_second;
		TDynArray< String >* fileGroups[] = { &contentZero.m_bundleFiles, &contentZero.m_scriptFiles, &contentZero.m_cacheFiles, 
			&contentZero.m_miscFiles,  &contentZero.m_stringsFiles }; // speech files are not prefetchable at this time

		for ( auto group : fileGroups )
		{
			TDynArray< String >& files = *group;

			for ( Int32 j = files.SizeInt()-1; j >= 0; --j )
			{
				const String& path = files[j];
				const String& fileName = CFilePath( path ).GetFileNameWithExt().ToLower();
				if ( prefetchFileNames.Exist( fileName ) )
				{
					packageFiles.m_prefetchFiles.PushBack( path );
					files.RemoveAt( j );
				}
			}
		}
	}

	return true;
}

static Bool CreateStubPlayGoPackage( const String& playGoDatFilePath )
{
	IFile* emptyFile = GFileManager->CreateFileWriter( playGoDatFilePath, FOF_AbsolutePath );
	if ( ! emptyFile )
	{
		ERR_WCC(TXT("Could not create empty PlayGo dat file '%ls'"), playGoDatFilePath.AsChar() );
		return false;
	}
	delete emptyFile;
	emptyFile = nullptr;
	if ( !GFileManager->FileExist( playGoDatFilePath ) || GFileManager->GetFileSize( playGoDatFilePath ) > 0 )
	{
		ERR_WCC(TXT("Could not create empty PlayGo dat file '%ls'"), playGoDatFilePath.AsChar() );
		return false;
	}

	return true;
}

static Bool CreateContentManifest( SContentManifest& outManifest, EPackageType packageType, const String& appRoot, TDynArray< SPackageContentManifestPS4 >& manifests, const SOptionsPS4 &options )
{
	Red::System::DateTime newPackTimestamp;

	// Append "content" whether app or DLC
	String contentRoot;
	contentRoot = appRoot + TXT("content\\");

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

	// Generate manifest files.
	// Sort for easier progress tracking
	Sort( manifests.Begin(), manifests.End(), [](const SPackageContentManifestPS4& a, const SPackageContentManifestPS4& b ){ return a.m_contentName < b.m_contentName; });
	for ( const SPackageContentManifestPS4& manifest : manifests )
	{
		// Patch stub chunks, process these later or even in another commandlet.
		if ( Check::IsPatch( manifest.m_contentName.AsChar() ) && manifest.m_conformedFilePaths.Empty() )
		{
			SContentChunk newPatchContentChunk;
			const CName id = manifest.m_contentName;
			newPatchContentChunk.m_chunkLabel = id;
			newContentPack.m_contentChunks.PushBack( Move( newPatchContentChunk ) );
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

		newContentPack.m_contentChunks.PushBack( Move( newContentChunk ) );
	}

	newContentPack.m_timestamp = newPackTimestamp;

	outManifest.m_contentPack = Move( newContentPack );

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

static Bool PlayGoPackage_InitLang( const SOptionsPS4& options, const SPackageSkuPS4& packageSku, SPlayGoPackage& playGoPackage )
{
	for ( const String& gameLang : options.m_textLanguages )
	{
		String playGoLang;
		if ( ! packageSku.m_packageLanguages.m_gameToPlayGoLangMap.Find( gameLang, playGoLang ) )
		{
			// Can't do anything for text languages not supported by PlayGo, so it's no an error if they don't exist in the language map.
			// Just means we can't automatically select subtitles for this language
			if ( !options.m_speechLanguages.Exist( gameLang ) )
			{
				continue;
			}

			ERR_WCC(TXT("Failed to get PlayGo language for game language language '%ls'"), gameLang.AsChar() );
			return false;
		}

		TPlayGoLanguageFlag langFlag;
		if ( ! GetPlayGoLanguageFlag( playGoLang.AsChar(), langFlag ) )
		{
			ERR_WCC(TXT("Failed to get PlayGo language flag for PlayGo language '%ls'"), playGoLang.AsChar() );
			return false;
		}

		playGoPackage.m_languageFlagMap.Insert( UNICODE_TO_ANSI( gameLang.ToLower().AsChar() ), langFlag );
	}

	return true;
}

static Bool PlayGoPackage_InitLaunch( TDynArray< SPackageContentManifestPS4 >& manifests, SPlayGoPackage& playGoPackage, TDynArray< SPlayGoContent >& playGoContent )
{
	// Sort by install order to populate playGoPackage.m_installOrderGameContent
	::Sort( manifests.Begin(), manifests.End(), [](const SPackageContentManifestPS4& a, const SPackageContentManifestPS4& b ) { return a.m_installOrder < b.m_installOrder; } );
	// Precreate all non language chunks for N^2 look up creating language chunk dependencies
	// FIXME: Gotten a bit messy how it works, "contentName" and "baseContentName" should go and just simply be
	// content can be composed of multiple chunks
	CName launchContentName;
	for ( const SPackageContentManifestPS4& manifest : manifests )
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
			TDynArray< SPlayGoChunk > chunks; // To be filled more in next step with language and prefetch chunks
			chunks.PushBack( SPlayGoChunk( PGLF_AllLanguages, manifest.m_chunkID ) );
			SPlayGoContent content( manifest.m_contentName, chunks );
			playGoContent.PushBack( content );
			if ( isLaunchContent )
			{
				launchContentName = manifestContentName;
			}
		}
	}

	if ( ! launchContentName )
	{
		ERR_WCC(TXT("No launchContentName!"));
		return false;
	}
	playGoPackage.m_launchContentName = launchContentName;

	return true;
}

static Bool PlayGoPackage_InitContent( SPlayGoPackage& playGoPackage, TDynArray< SPlayGoContent >& playGoContent, const TDynArray< SPackageContentManifestPS4 >& manifests, const SPackageLanguagesPS4& packageLanguages )
{
	for ( const SPackageContentManifestPS4& manifest : manifests )
	{
		const Uint32 chunkID = manifest.m_chunkID;
		const CName manifestContentName = manifest.m_contentName;
		const CName manifestBaseContentName = manifest.m_baseContentName;
		const String& language = manifest.m_language;

		playGoPackage.m_chunkNameMap.Insert( chunkID, manifestContentName );

		if ( manifestContentName != manifestBaseContentName )
		{
			if ( language.Empty() )
			{
				ERR_WCC(TXT("Split non-language chunk? Shouldn't exist %ls/%ls"), manifestBaseContentName.AsChar(), manifestContentName.AsChar() );
				return false;
			}

			String playGoLang = TXT("all");
			TPlayGoLanguageFlag languageFlag = PGLF_AllLanguages;
			if ( manifest.m_language != TXT("prefetch") && !packageLanguages.m_gameToPlayGoLangMap.Find( manifest.m_language, playGoLang ) )
			{
				ERR_WCC(TXT("Could not find PlayGo language for game language %ls"), manifest.m_language.AsChar());
				return false;
			}
			if ( !GetPlayGoLanguageFlag( playGoLang.AsChar(), languageFlag ) )
			{
				ERR_WCC(TXT("Could not find PlayGo language flag for PlayGo language %ls"), playGoLang.AsChar());
				return false;
			}

			for ( SPlayGoContent& content : playGoContent )
			{
				if ( content.m_contentName == manifestBaseContentName )
				{
					SPlayGoChunk chunk( languageFlag, chunkID );
					if ( !content.m_installOrderChunks.PushBackUnique( chunk ) )
					{
						ERR_WCC(TXT("Non unique language chunk %u"), chunkID);
						return false;
					}
				}
			}
		}
	}

	// Chunk order within content is also the install order, since we want it to match the bluray layout
	for ( SPlayGoContent& content : playGoContent )
	{
		TDynArray< SPlayGoChunk >& chunks = content.m_installOrderChunks;
		::Sort( chunks.Begin(), chunks.End(), [](const SPlayGoChunk& a, const SPlayGoChunk& b){ return a.m_chunkID < b.m_chunkID; } );
	}
	playGoPackage.m_installOrderContent = Move( playGoContent );

	return true;
}

static void PlayGoPackage_InitLanguages( SPlayGoPackage &playGoPackage, const SOptionsPS4 &options )
{
	for ( const String& gameLang : options.m_speechLanguages )
	{
		playGoPackage.m_supportedSpeechLanguages.PushBack( UNICODE_TO_ANSI( gameLang.ToLower().AsChar() ) );
	}
	for ( const String& gameLang : options.m_textLanguages )
	{
		playGoPackage.m_supportedTextLanguages.PushBack( UNICODE_TO_ANSI( gameLang.ToLower().AsChar() ) );
	}
	playGoPackage.m_defaultSpeechLanguage = UNICODE_TO_ANSI( options.m_defaultSpeech.ToLower().AsChar() );
}

static Bool PlayGoPackage_VerifyChunks( const SPlayGoPackage& playGoPackage )
{
	for ( const SPlayGoContent content : playGoPackage.m_installOrderContent )
	{
		if ( content.m_installOrderChunks.Empty() )
		{
			ERR_WCC(TXT("content %ls has no chunks!"), content.m_contentName.AsChar() );
			return false;
		}
	}

	return true;
}

// Lame hack for patches to just make sure won't error out trying to init other stuff
static Bool PlayGoPackage_InitLanguageOnly( SPlayGoPackage& playGoPackage, const SOptionsPS4& options, const SPackageSkuPS4& packageSku, TDynArray< SPackageContentManifestPS4 >& manifests, const SPackageLanguagesPS4& packageLanguages )
{
	// Text languages because they should be a superset of speech languages
	if ( !PlayGoPackage_InitLang(options, packageSku, playGoPackage) )
	{
		return false;
	}

	PlayGoPackage_InitLanguages( playGoPackage, options );

	return true;
}

static Bool PlayGoPackage_Init( SPlayGoPackage& playGoPackage, const SOptionsPS4& options, const SPackageSkuPS4& packageSku, TDynArray< SPackageContentManifestPS4 >& manifests, const SPackageLanguagesPS4& packageLanguages )
{
	// Text languages because they should be a superset of speech languages
	if ( !PlayGoPackage_InitLang(options, packageSku, playGoPackage) )
	{
		return false;
	}

	TDynArray< SPlayGoContent > playGoContent;
	if ( !PlayGoPackage_InitLaunch(manifests, playGoPackage, playGoContent) )
	{
		return false;
	}

	if ( !PlayGoPackage_InitContent( playGoPackage, playGoContent, manifests, packageLanguages ) )
	{
		return false;
	}

	PlayGoPackage_InitLanguages( playGoPackage, options );

	if ( !PlayGoPackage_VerifyChunks(playGoPackage) )
	{
		return false;
	}

	return true;
}

static const Bool SavePlayGoPackage( const String& playGoDatFilePath, const SPlayGoPackage& playGoPackage )
{
	IFile* writer = GFileManager->CreateFileWriter( playGoDatFilePath, FOF_AbsolutePath | FOF_Buffered );
	if ( ! writer )
	{
		ERR_WCC(TXT("Could not open '%ls' for writing"), playGoDatFilePath.AsChar() );
		return false;
	}

	*writer << const_cast< SPlayGoPackage& >( playGoPackage );
	delete writer;
	writer = nullptr;

	return true;
}

static Bool CreateSubmissionPackage( const SOptionsPS4& options, CPackageToolPS4& packageTool, const String& gp4ProjectPath )
{
	String pkgOutputPath(options.m_outDir);
	if ( !CheckOutputPath( pkgOutputPath ) )
	{
		return false;
	}

	CTimeCounter packageTimer;

	Uint32 formatFlags = ePackageFormatFlags_None;
	Uint32 formatOptions = ePackageFormatOptions_None;

	if ( options.m_createPkg )
	{
		formatFlags |= ePackageFormatFlags_Package;
	}
	if ( options.m_createIso )
	{
		formatFlags |= ePackageFormatFlags_ISO;
	}
	if ( options.m_createSubmissionMaterials )
	{
		formatFlags |= ePackageFormatFlags_SubmissionMaterials;
	}
	if ( options.m_moveOuter )
	{
		formatOptions |= ePackageFormatOptions_MoveOuter;
	}
	if ( options.m_skipDigest )
	{
		formatOptions |= ePackageFormatOptions_SkipDigest;
	}

	if ( options.m_createPkg )
	{
		if ( ! packageTool.CreatePackage( gp4ProjectPath, pkgOutputPath, formatFlags, formatOptions ) )
		{
			ERR_WCC(TXT("Failed to create submission package from %ls!"), gp4ProjectPath.AsChar() );
			return false;
		}
	}
	else
	{
		LOG_WCC(TXT("Package creation not specified. Skipping submission package creation."));
	}

	LOG_WCC(TXT("Submission package created in '%ls' in %1.2f sec"), pkgOutputPath.AsChar(), packageTimer.GetTimePeriod() );

	return true;
}

static void CreateBuildShortcut( const SOptionsPS4& options )
{
	const String shortcutLnk = options.m_outDir + TXT("Build.lnk");
	String buildDir = options.m_inDir;

	buildDir.Replace(TXT("C:\\cdprs-build\\"), TXT("\\\\cdprs-build\\"));// Build server path hack

	const String& packageDir = options.m_outDir;
	if ( ! CreateFolderShortcut( packageDir, buildDir, TXT("Build") ) )
	{
		WARN_WCC(TXT("Failed to create shortcut from '%ls' to '%ls'"), packageDir.AsChar(), buildDir.AsChar());
	}
	else
	{
		LOG_WCC(TXT("Created shortcut from '%ls' to '%ls'"), packageDir.AsChar(), buildDir.AsChar());
	}
}

static Bool LoadPreviousPlayGoPackage( SPlayGoPackage& outPlayGoPackage, const SPackageSkuPS4& packageSku, const SOptionsPS4 &options, CPackageToolPS4& packageTool, const String relativeContentManifestFilePath )
{
	const String outTempInstallerDatFile = options.m_tempDir + INSTALLER_DAT_FILE;
	GFileManager->DeleteFile( outTempInstallerDatFile ); // delete any previous version if present

	// For adding new base language support:
	// "Legacy" patches might not have a new installer dat, so fall back to the base file if necessary.
	// But patches from now on always carry forward the new installer dat as well.
	TStaticArray<String,2> packPathsStack;
	packPathsStack.PushBack( packageSku.m_patchParams.m_appPkgPath );
	if ( !packageSku.m_patchParams.m_latestPatchPath.Empty() )
	{
		packPathsStack.PushBack( packageSku.m_patchParams.m_latestPatchPath );
	}

	if ( packPathsStack.Empty() )
	{
		ERR_WCC(TXT("No latest pack path for patch file!"));
		return false;
	}

	// Re-use passcode, assume it doesn't change since base version. If does, well reconsider.
	Bool packFound = false;
	for(;;)
	{
		if ( packPathsStack.Empty() )
			break;
		
		const String& packPath = packPathsStack.Back();
		if ( packageTool.ExtractFile( packPath, relativeContentManifestFilePath, outTempInstallerDatFile, packageSku.m_passcode ) )
		{
			LOG_WCC(TXT("Found old installer.dat in %ls"), packPath.AsChar());
			packFound = true;
			break;
		}
		packPathsStack.PopBack();
	}

	if ( !packFound )
	{
		ERR_WCC(TXT("Failed to extract previous %ls for patch"), INSTALLER_DAT_FILE);
		return false;
	}

	Red::TScopedPtr<IFile> file( GFileManager->CreateFileReader(outTempInstallerDatFile,FOF_AbsolutePath|FOF_Buffered));
	if (!file)
	{
		ERR_WCC(TXT("Failed to open %ls for reading"), outTempInstallerDatFile.AsChar());
		return false;
	}

	*file << outPlayGoPackage;

	return true;
}

static Bool MergeWithPreviousPlayGoPackage( SPlayGoPackage& newPlayGoPackage, const String& relativePackageFilePath, const SPackageSkuPS4& packageSku, const SOptionsPS4& options, CPackageToolPS4& packageTool )
{
	// For now, just merge languages, expected at least called PlayGoPackage_InitLanguageOnly on the new package

	SPlayGoPackage oldPlayGoPackage;
	if ( !LoadPreviousPlayGoPackage( oldPlayGoPackage, packageSku, options, packageTool, relativePackageFilePath) )
	{
		return false;
	}

#ifdef RED_LOGGING_ENABLED
	LOG_WCC(TXT("== MergeWithPreviousPlayGoPackage: PlayGo package before merge! =="));
	::DumpPlayGoPackage( oldPlayGoPackage );
#endif

	for ( const auto& it : newPlayGoPackage.m_languageFlagMap )
	{
		const StringAnsi& lang = it.m_first;
		const Uint64 flag = it.m_second;
		if ( oldPlayGoPackage.m_languageFlagMap.Find(lang) == oldPlayGoPackage.m_languageFlagMap.End() )
		{
			oldPlayGoPackage.m_languageFlagMap.Insert(lang, flag);
		}
	}
	for ( const StringAnsi& lang : newPlayGoPackage.m_supportedSpeechLanguages )
	{
		oldPlayGoPackage.m_supportedSpeechLanguages.PushBackUnique(lang);
	}
	for ( const StringAnsi& lang : newPlayGoPackage.m_supportedTextLanguages )
	{
		oldPlayGoPackage.m_supportedTextLanguages.PushBackUnique(lang);
	}

	// Update with merged version
	newPlayGoPackage = Move( oldPlayGoPackage );

	return true;
}

static Bool LoadPreviousContentManifest( SContentManifest& outManifest, const SPackageSkuPS4& packageSku, const SOptionsPS4 &options, CPackageToolPS4& packageTool, const String relativeContentManifestFilePath )
{
	String latestPackPath;
	// Get latest manifest
	if ( !packageSku.m_patchParams.m_latestPatchPath.Empty() )
	{
		latestPackPath = packageSku.m_patchParams.m_latestPatchPath;
	}
	else
	{
		latestPackPath = packageSku.m_patchParams.m_appPkgPath;
	}

	if ( latestPackPath.Empty() )
	{
		ERR_WCC(TXT("No latest pack path for patch file!"));
		return false;
	}

	const String outTempManifestFile = options.m_tempDir + MANIFEST_FILE_NAME;
	GFileManager->DeleteFile( outTempManifestFile ); // delete any previous version if present

	// Re-use passcode, assume it doesn't change since base version. If does, well reconsider.
	if ( !packageTool.ExtractFile( latestPackPath, relativeContentManifestFilePath, outTempManifestFile, packageSku.m_passcode ) )
	{
		ERR_WCC(TXT("Failed to extract previous %ls for patch"), MANIFEST_FILE_NAME);
		return false;
	}

	String buf;
	if ( !GFileManager->LoadFileToString( outTempManifestFile, buf, true ) )
	{
		ERR_WCC(TXT("Failed to read %ls for patch"), outTempManifestFile.AsChar());
		return false;
	}

	CContentManifestReader reader( buf );
	if ( ! reader.ParseManifest( outManifest ) )
	{
		ERR_WCC(TXT("Failed to parse old content manifest %ls for patch"), outTempManifestFile.AsChar());
		return false;
	}

	return true;
}

static void MergeContentFiles( SContentChunk& mergeTo,  const SContentChunk& mergeFrom )
{
	TDynArray< SContentFile > newFiles;
	for ( const SContentFile& fromIt : mergeFrom.m_contentFiles )
	{
		Bool newFile = true;
		for ( SContentFile& toIt : mergeTo.m_contentFiles )
		{
			if ( fromIt.m_path == toIt.m_path )
			{
				// Replace with from. E.g., updating from patch
				toIt = fromIt;
				newFile = false;
				break;
			}
		}
		if ( newFile )
		{
			newFiles.PushBack( fromIt );
		}
	}

	mergeTo.m_contentFiles.PushBack( newFiles );
}

static Bool MergeWithPreviousContentManifest( SContentManifest& newContentManifest, const String& relativeContentManifestFilePath, const SPackageSkuPS4& packageSku, const SOptionsPS4& options, CPackageToolPS4& packageTool )
{
	SContentManifest oldManifest;
	if ( !LoadPreviousContentManifest( oldManifest, packageSku, options, packageTool, relativeContentManifestFilePath) )
	{
		return false;
	}

	SContentPack& oldPack = oldManifest.m_contentPack;
	SContentPack& newPack = newContentManifest.m_contentPack;
	for ( const SContentChunk& newChunk : newPack.m_contentChunks )
	{
		Bool oldChunkFound = false;
		for ( SContentChunk& oldChunk : oldPack.m_contentChunks )
		{
			if ( newChunk.m_chunkLabel == oldChunk.m_chunkLabel )
			{
				MergeContentFiles( oldChunk, newChunk );
				oldChunkFound = true;
				break;
			}
		}

		if ( !oldChunkFound )
		{
			ERR_WCC(TXT("No matching old chunk for %ls"), newChunk.m_chunkLabel.AsChar());
			return false;
		}
	}

	// Update with merged version
	newContentManifest = Move( oldManifest );

	return true;
}

static Bool LoadBasePackage( PackageHelpers::SPackageInfo& basePackage, const SOptionsPS4 &options )
{
	CFilePath filePath( options.m_appPkgPath );
	const String gp4Path = filePath.GetPathString(true) + filePath.GetFileName() + TXT(".gp4");

	String xmlDat;
	if ( !GFileManager->LoadFileToString( gp4Path, xmlDat, true ) )
	{
		ERR_WCC(TXT("Can't open app gp4 '%ls'"), gp4Path.AsChar());
		return false;
	}

	const Bool ignoreHeader = true; // ucs-2le vs utf8
	CXMLReader xmlReader( xmlDat, ignoreHeader );
	PackageHelpers::CPackagePlayGoChunksBuilder chunksBuilder( xmlReader );
	if ( !chunksBuilder.ParsePackage( basePackage ) )
	{
		ERR_WCC(TXT("Can't parse app gp4 '%ls'"), gp4Path.AsChar());
		return false;
	}

	return true;
}

static void MakePatchManifest( SContentManifest &contentManifest )
{
	for ( SContentChunk& chunk : contentManifest.m_contentPack.m_contentChunks )
	{
		// Another hack: if not a patch chunk, don't add the patch flag. This way we can replace files in the contentX chunks without setting this flag
		// and keep some consistency between Xbox and PS4. Should work even with the flag, but let's not chance it. "isPatch" flag is really more like
		// "insert at front of chain".
		if ( 0 != Red::System::StringCompareNoCase( chunk.m_chunkLabel.AsChar(), TXT("patch"), Red::System::StringLengthCompileTime(TXT("patch")) ) )
			continue;

		for ( SContentFile& file : chunk.m_contentFiles )
		{
			file.m_isPatch = true;
		}
	}
}

//////////////////////////////////////////////////////////////////////////



// Moved into WCC so adding additional languages can't ever require a change to the game EXE
// by having these values referenced directly vs obtained from data

// systemLang analogous with SCE_SYSTEM_PARAM_LANG_*
#define PLAYGO_FLAG64(systemLang) (1ULL<<(64-(systemLang)-1))
const TPlayGoLanguageFlag PGLF_Japanese					= PLAYGO_FLAG64(0);
const TPlayGoLanguageFlag PGLF_English_UnitedStates		= PLAYGO_FLAG64(1);
const TPlayGoLanguageFlag PGLF_French					= PLAYGO_FLAG64(2);
const TPlayGoLanguageFlag PGLF_Spanish					= PLAYGO_FLAG64(3);
const TPlayGoLanguageFlag PGLF_German					= PLAYGO_FLAG64(4);
const TPlayGoLanguageFlag PGLF_Italian					= PLAYGO_FLAG64(5);
const TPlayGoLanguageFlag PGLF_Dutch					= PLAYGO_FLAG64(6);
const TPlayGoLanguageFlag PGLF_Portuguese_Portugal		= PLAYGO_FLAG64(7);
const TPlayGoLanguageFlag PGLF_Russian					= PLAYGO_FLAG64(8);
const TPlayGoLanguageFlag PGLF_Korean					= PLAYGO_FLAG64(9);
const TPlayGoLanguageFlag PGLF_Chinese_traditional		= PLAYGO_FLAG64(10);
const TPlayGoLanguageFlag PGLF_Chinese_simplified		= PLAYGO_FLAG64(11);
const TPlayGoLanguageFlag PGLF_Finnish					= PLAYGO_FLAG64(12);
const TPlayGoLanguageFlag PGLF_Swedish					= PLAYGO_FLAG64(13);
const TPlayGoLanguageFlag PGLF_Danish					= PLAYGO_FLAG64(14);
const TPlayGoLanguageFlag PGLF_Norwegian				= PLAYGO_FLAG64(15);
const TPlayGoLanguageFlag PGLF_Polish					= PLAYGO_FLAG64(16);
const TPlayGoLanguageFlag PGLF_Portuguese_Brazil		= PLAYGO_FLAG64(17);
const TPlayGoLanguageFlag PGLF_English_United_Kingdom	= PLAYGO_FLAG64(18);
const TPlayGoLanguageFlag PGLF_Turkish					= PLAYGO_FLAG64(19);
const TPlayGoLanguageFlag PGLF_Spanish_Latin_America	= PLAYGO_FLAG64(20);
const TPlayGoLanguageFlag PGLF_Arabic					= PLAYGO_FLAG64(21);

struct SPlayGoToLanguageFlagMap
{
	Uint32					m_number;
	const Char*				m_languageName;
	const Char*				m_altLanguageName;
	TPlayGoLanguageFlag		m_languageFlag;
};

static const SPlayGoToLanguageFlagMap PLAYGO_LANGUAGE_FLAGS[]=
{
	{ 0,	TXT("ja"),		TXT(""),	PGLF_Japanese },
	{ 1,	TXT("en-US"),	TXT("en"),	PGLF_English_UnitedStates },
	{ 2,	TXT("fr"),		TXT(""),	PGLF_French },
	{ 3,	TXT("es-ES"),	TXT("es"),	PGLF_Spanish },
	{ 4,	TXT("de"),		TXT(""),	PGLF_German },
	{ 5,	TXT("it"),		TXT(""),	PGLF_Italian },
	{ 6,	TXT("nl"),		TXT(""),	PGLF_Dutch },
	{ 7,	TXT("pt-PT"),	TXT("pt"),	PGLF_Portuguese_Portugal },
	{ 8,	TXT("ru"),		TXT(""),	PGLF_Russian },	
	{ 9,	TXT("ko"),		TXT(""),	PGLF_Korean },
	{ 10,	TXT("zh-Hant"),	TXT(""),	PGLF_Chinese_traditional },
	{ 11,	TXT("zh-Hans"),	TXT(""),	PGLF_Chinese_simplified },
	{ 12,	TXT("fi"),		TXT(""),	PGLF_Finnish },
	{ 13,	TXT("sv"),		TXT(""),	PGLF_Swedish },
	{ 14,	TXT("da"),		TXT(""),	PGLF_Danish },
	{ 15,	TXT("no"),		TXT(""),	PGLF_Norwegian },
	{ 16,	TXT("pl"),		TXT(""),	PGLF_Polish },
	{ 17,	TXT("pt-BR"),	TXT(""),	PGLF_Portuguese_Brazil },
	{ 18,	TXT("en-GB"),	TXT(""),	PGLF_English_United_Kingdom },
	{ 19,	TXT("tr"),		TXT(""),	PGLF_Turkish },
	{ 20,	TXT("es-LA"),	TXT(""),	PGLF_Spanish_Latin_America },
	{ 21,	TXT("ar"),		TXT(""),	PGLF_Arabic },
	{ 99,	TXT("all"),		TXT(""),	PGLF_AllLanguages },
};

static Bool GetPlayGoLanguageFlag( const Char* playGoLanguage, TPlayGoLanguageFlag& outFlag )
{
	for ( Uint32 i = 0; i < ARRAY_COUNT_U32( PLAYGO_LANGUAGE_FLAGS ); ++i )
	{
		if ( Red::System::StringCompareNoCase( playGoLanguage, PLAYGO_LANGUAGE_FLAGS[i].m_languageName ) == 0 ||
			Red::System::StringCompareNoCase( playGoLanguage, PLAYGO_LANGUAGE_FLAGS[i].m_altLanguageName ) == 0 )
		{
			outFlag = PLAYGO_LANGUAGE_FLAGS[i].m_languageFlag;
			return true;
		}
	}

	return false;
}
