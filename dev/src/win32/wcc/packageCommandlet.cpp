/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#ifndef NO_EDITOR
#include "../../common/core/commandlet.h"
#include "../../common/core/depot.h"
#include "../../common/core/string.h"
#include "../../common/core/file.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/core/garbageCollector.h"

#include "../../common/engine/gameResource.h"

#include "packageMainPS4.h"
#include "packageMainXboxOne.h"

/////////////////////////////////////////////////////////////////////////////////
const struct SPlatformInfo
{
	ECookingPlatform	m_platform;
	const Char*			m_desc;
	Bool				m_isSupported;
} 
PLATFORM_INFO_LUT[] =
{
	{ PLATFORM_None, TXT("None"), false },
	{ PLATFORM_Null, TXT("Null"), false },
	{ PLATFORM_Resave, TXT("Resave"), false },
	{ PLATFORM_PC, TXT("PC"), false },
#ifndef WCC_LITE
	{ PLATFORM_XboxOne, TXT("Xbox One"), true },
	{ PLATFORM_PS4, TXT("PS4"), true },
#endif
};

class CPackageCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CPackageCommandlet, ICommandlet, 0 );

public:
	CPackageCommandlet();
	~CPackageCommandlet();

	virtual const Char* GetOneLiner() const;

	virtual Bool Execute( const CommandletOptions& options );

	virtual void PrintHelp() const;

private:
	Bool RunPackageMainPS4( const CommandletOptions& options );
	Bool RunPackageMainXboxOne( const CommandletOptions& options );

private:
	Bool RunAppPackageXboxOne( const CommandletOptions& options, SOptionsXboxOne& outOptionsXbone );
	Bool RunDlcPackageXboxOne( const CommandletOptions& options, SOptionsXboxOne& outOptionsXbone );

private:
	Bool RunAppPackagePS4( const CommandletOptions& options, SOptionsPS4& outOptionsPS4 );
	Bool RunDlcPackagePS4( const CommandletOptions& options, SOptionsPS4& outOptionsPS4 );
};

BEGIN_CLASS_RTTI( CPackageCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CPackageCommandlet );


//---------------------------------------------------------------------------

CPackageCommandlet::CPackageCommandlet()
{
	m_commandletName = CName( TXT("package") );
}

CPackageCommandlet::~CPackageCommandlet()
{
}

Bool CPackageCommandlet::Execute( const CommandletOptions& options )
{
	LOG_WCC( TXT("Parsing arguments...") );

	ECookingPlatform cookingPlatform = PLATFORM_None;

	Bool hadBadArgument = false;
	auto arguments = options.GetFreeArguments();
	for ( Uint32 i = 0; i < arguments.Size(); i++ )
	{
		String argument = arguments[i].ToLower();
		if ( argument == TXT("pc") )
		{
			cookingPlatform = PLATFORM_PC;
		}
#ifndef WCC_LITE
		else if ( argument == TXT("xboxone") || argument == TXT("durango") )
		{
			cookingPlatform = PLATFORM_XboxOne;
		}
		else if ( argument == TXT("ps4") || argument == TXT("orbis") )
		{
			cookingPlatform = PLATFORM_PS4;
		}
#endif
	}

	if ( cookingPlatform < 0 || cookingPlatform >= ARRAY_COUNT_U32(PLATFORM_INFO_LUT) )
	{
		ERR_WCC(TXT("Unknown cooking platform enum %u"), (Uint32)cookingPlatform );
		PrintHelp();
		return false;
	}

	if ( ! PLATFORM_INFO_LUT[ cookingPlatform ].m_isSupported )
	{
		ERR_WCC(TXT("Unsupported cooking platform '%ls'"), PLATFORM_INFO_LUT[cookingPlatform].m_desc);
		PrintHelp();
		return false;
	}

	Bool retval = false;
#ifndef WCC_LITE
	if ( cookingPlatform == PLATFORM_PS4 )
	{
		retval = RunPackageMainPS4( options );
	}
	else if ( cookingPlatform == PLATFORM_XboxOne )
	{
		retval = RunPackageMainXboxOne( options );
	}
#endif

	return retval;
}

static Bool GetNormalizedPath( const String& shortVer, const String& longVer, String& outPath, const ICommandlet::CommandletOptions& options )
{
	if ( ! options.GetSingleOptionValue( shortVer, longVer, outPath ) )
	{
		return false;
	}
	outPath.ReplaceAll( TXT("/"), TXT("\\") );
	if ( ! outPath.EndsWith( TXT("\\") ) )
	{
		outPath += TXT("\\");
	}
		
	CFilePath filePath( outPath );
	const String dirPath = filePath.GetPathString();

	if ( !GSystemIO.CreateDirectory( dirPath.AsChar() ) )
	{
		ERR_WCC(TXT("Failed to validate option path '%ls'"), outPath.AsChar() );
		return false;
	}

	return true;
}

static Bool GetNormalizedFile( const String& shortVer, const String& longVer, String& outFile, const ICommandlet::CommandletOptions& options )
{
	if ( ! options.GetSingleOptionValue( shortVer, longVer, outFile ) )
	{
		return false;
	}
	outFile.ReplaceAll( TXT("/"), TXT("\\") );

	CFilePath filePath( outFile );
	if ( ! filePath.HasFilename() )
	{
		ERR_WCC(TXT("Failed to validate option file '%ls'"), outFile.AsChar() );
		return false;
	}

	if ( !GSystemIO.FileExist( outFile.AsChar() ) )
	{
		ERR_WCC(TXT("Failed to validate option file '%ls'"), outFile.AsChar() );
		return false;
	}

	return true;
}

static Bool GetLanguages( TDynArray< String >& outGameLanguages, const ICommandlet::CommandletOptions& options )
{
	if ( ! options.HasOption( TXT("langs") ) )
	{
		return false;
	}

	TList< String > vals = options.GetOptionValues( TXT("langs") );
	for ( auto it = vals.Begin(); it != vals.End(); ++it )
	{
		String lang = *it;
		CUpperToLower conv( lang.TypedData(), lang.Size() );
		outGameLanguages.PushBack( Move( lang ) );
	}

	return true;
}

static Bool GetDefaultLanguage( String& outGameLanguage, const ICommandlet::CommandletOptions& options )
{
	if ( ! options.GetSingleOptionValue( TXT("dftlang"), outGameLanguage ) )
	{
		return false;
	}

	CUpperToLower conv( outGameLanguage.TypedData(), outGameLanguage.Size() );

	return true;
}

static Bool GetDefaultSpeech( String& outGameSpeech, const ICommandlet::CommandletOptions& options )
{
	if ( ! options.GetSingleOptionValue( TXT("dftspeech"), outGameSpeech ) )
	{
		return false;
	}

	CUpperToLower conv( outGameSpeech.TypedData(), outGameSpeech.Size() );

	return true;
}

static Bool GetStrings( TDynArray< String >& outGameStrings, const ICommandlet::CommandletOptions& options )
{
	if ( ! options.HasOption( TXT("strings") ) )
	{
		return false;
	}

	TList< String > vals = options.GetOptionValues( TXT("strings") );
	for ( auto it = vals.Begin(); it != vals.End(); ++it )
	{
		String strings = *it;
		CUpperToLower conv( strings.TypedData(), strings.Size() );
		outGameStrings.PushBack( Move( strings ) );
	}

	return true;
}

static Bool GetSpeeches( TDynArray< String >& outGameSpeeches, const ICommandlet::CommandletOptions& options )
{
	if ( ! options.HasOption( TXT("speeches") ) )
	{
		return false;
	}

	TList< String > vals = options.GetOptionValues( TXT("speeches") );
	for ( auto it = vals.Begin(); it != vals.End(); ++it )
	{
		String speech = *it;
		CUpperToLower conv( speech.TypedData(), speech.Size() );
		outGameSpeeches.PushBack( Move( speech ) );
	}

	return true;
}

static Bool GetContentID( String& outContentID, const ICommandlet::CommandletOptions& options )
{
	return options.GetSingleOptionValue( TXT("contentid"), outContentID );
}

static Bool GetPasscode( String& outPasscode, const ICommandlet::CommandletOptions& options )
{
	return options.GetSingleOptionValue( TXT("passcode"), outPasscode );
}

static Bool GetEntitlementKey( String& outEntitlementKey, const ICommandlet::CommandletOptions& options )
{
	return options.GetSingleOptionValue( TXT("entitlement_key"), outEntitlementKey );
}

static Bool GetLaunchContentNumber( Uint32& outLaunchContentNumber, const ICommandlet::CommandletOptions& options )
{
	String temp;
	if ( ! options.GetSingleOptionValue( TXT("launch"), temp ) )
	{
		return false;
	}

	// Not trusting that it won't modify it on fail
	Uint32 launchContentNumber;
	if ( Red::System::StringToInt(launchContentNumber, temp.AsChar(), nullptr, Red::System::BaseTen ) )
	{
		outLaunchContentNumber = launchContentNumber;
		return true;
	}

	return false;
}

static Bool GetPatchAppPkgPath( String& outPkgPath, const ICommandlet::CommandletOptions& options )
{
	return options.GetSingleOptionValue( TXT("patch_pkg_path"), outPkgPath );
}

static Bool GetLatestPatchPath( String& outLatestPatchPath, const ICommandlet::CommandletOptions& options )
{
	return options.GetSingleOptionValue( TXT("patch_latest_path"), outLatestPatchPath );
}

static Bool GetIsDayOnePatch( const ICommandlet::CommandletOptions& options )
{
	return options.HasOption( TXT("patch_day_one") );
}

static Bool GetNoCreatePackage( const ICommandlet::CommandletOptions& options )
{
	return options.HasOption( TXT("nopkg") );
}
static Bool GetCreateIso( const ICommandlet::CommandletOptions& options )
{
	return options.HasOption( TXT("iso") );
}
static Bool GetCreateSubmissionMaterials( const ICommandlet::CommandletOptions& options )
{
	return options.HasOption( TXT("subitem") );
}
static Bool GetMoveOuter( const ICommandlet::CommandletOptions& options )
{
	return options.HasOption( TXT("move_outer") );
}
static Bool GetSkipDigest( const ICommandlet::CommandletOptions& options )
{
	return options.HasOption( TXT("skip_digest") );
}
static Bool GetSkipCRC( const ICommandlet::CommandletOptions& options )
{
	return options.HasOption( TXT("skip_crc") );
}

static Bool GetElfName( String& outElfName, const ICommandlet::CommandletOptions& options )
{
	if ( ! options.GetSingleOptionValue( TXT("elf"), outElfName ) )
	{
		return false;
	}

	CUpperToLower conv( outElfName.TypedData(), outElfName.Size() );

	return true;
}

static Bool GetExeName( String& outExeName, const ICommandlet::CommandletOptions& options )
{
	return options.GetSingleOptionValue( TXT("exe"), outExeName );
}

static Bool GetReleaseName( String& outExeName, const ICommandlet::CommandletOptions& options )
{
	return options.GetSingleOptionValue( TXT("release"), outExeName );
}

static Bool GetPackageType( String& outPackageType, const ICommandlet::CommandletOptions& options )
{
	Uint32 typeCount = 0;
	
	String packageType;
	const TDynArray< String >& freeArgs = options.GetFreeArguments();
	for ( const String& arg : freeArgs )
	{
		if (arg.EqualsNC(TXT("app")))
		{
			packageType = TXT("app");
			++typeCount;
		}
		else if (arg.EqualsNC(TXT("patch")))
		{
			packageType = TXT("patch");
			++typeCount;
		}
		else if (arg.EqualsNC(TXT("dlc")))
		{
			packageType = TXT("dlc");
			++typeCount;
		}
	}
	
	if ( typeCount < 1 )
	{
		ERR_WCC(TXT("No package type specified!"));
		return false;
	}

	if ( typeCount > 1 )
	{
		ERR_WCC(TXT("Multiple package types specified!"));
		return false;
	}

	outPackageType = Move( packageType );

	return true;
}

static Bool GetManifestContentName( String& manifestContentName, const ICommandlet::CommandletOptions& options )
{
	return options.GetSingleOptionValue( TXT("manifest_content_name"), manifestContentName );
}

Bool CPackageCommandlet::RunPackageMainPS4( const CommandletOptions& options )
{
	SOptionsPS4 optionsPS4;

	const TDynArray< String >& freeArgs = options.GetFreeArguments();
	
 	if ( ! GetPackageType( optionsPS4.m_packageType, options ) )
	{
		ERR_WCC(TXT("Could not get package type"));
		return false;
	}
	
	if ( ! GetNormalizedPath( TXT("t"), TXT("tempdir"), optionsPS4.m_tempDir, options ) )
	{
		const String tempDir = GFileManager->GetTempDirectory() + TXT("packageCommandlet\\");
		LOG_WCC(TXT("No default tempdir specified. Defaulting to '%ls'"), tempDir.AsChar() );
		optionsPS4.m_tempDir = tempDir;
	}
	
	if ( !GetNormalizedPath(TXT("i"), TXT("indir"), optionsPS4.m_inDir, options ) )
	{
		ERR_WCC(TXT("Option 'indir' not specified"));
		PrintHelp();
		return false;
	}
	
	if ( ! GetNormalizedPath( TXT("o"), TXT("outdir"), optionsPS4.m_outDir, options ) )
	{
		ERR_WCC(TXT("Option 'outdir' not specified"));
		PrintHelp();
		return false;
	}

	String manifestContentName;
	if ( GetManifestContentName(manifestContentName, options ) )
	{
		optionsPS4.m_manifestContentName = manifestContentName;
	}

	if ( optionsPS4.m_packageType.EqualsNC(TXT("app")) || optionsPS4.m_packageType.EqualsNC(TXT("patch")) )
	{
		return RunAppPackagePS4( options, optionsPS4 );
	}
	else if ( optionsPS4.m_packageType.EqualsNC(TXT("dlc")) )
	{
		return RunDlcPackagePS4( options, optionsPS4 );
	}

	ERR_WCC(TXT("Unknown app type '%ls'"), optionsPS4.m_packageType.AsChar());
	return false;
}


Bool CPackageCommandlet::RunAppPackagePS4( const CommandletOptions& options, SOptionsPS4& outOptionsPS4 )
{
	if ( ! GetNormalizedPath( TXT("l"), TXT("langdir"), outOptionsPS4.m_langDir, options ) )
	{
		ERR_WCC(TXT("Option 'langdir' not specified"));
		PrintHelp();
		return false;
	}

	if ( ! GetNormalizedFile( TXT("p"), TXT("prefetch"), outOptionsPS4.m_prefetchFile, options ) )
	{
		WARN_WCC(TXT("No prefetch file specified"));
	}

	if ( ! GetStrings( outOptionsPS4.m_textLanguages, options ) || outOptionsPS4.m_textLanguages.Empty() ) 
	{
		ERR_WCC(TXT("No strings specified!"));
		PrintHelp();
		return false;
	}

	if ( ! GetSpeeches( outOptionsPS4.m_speechLanguages, options ) || outOptionsPS4.m_speechLanguages.Empty() ) 
	{
		ERR_WCC(TXT("No speeches specified!"));
		PrintHelp();
		return false;
	}

	if ( ! GetDefaultSpeech( outOptionsPS4.m_defaultSpeech, options ) )
	{
		ERR_WCC(TXT("No default speech specified!"));
		PrintHelp();
		return false;
	}

	if ( ! GetElfName( outOptionsPS4.m_elfName, options ) )
	{
		ERR_WCC(TXT("No ELF specified! E.g., witcher3game.elf"));
		PrintHelp();
		return false;
	}

	if ( ! GetContentID( outOptionsPS4.m_contentID, options ) )
	{
		ERR_WCC(TXT("No content ID specified! E.g., V0002-NPXS29038_00-SIMPLESHOOTINGAM"));
		PrintHelp();
		return false;
	}

	if ( ! GetPasscode( outOptionsPS4.m_passcode, options ) )
	{
		ERR_WCC(TXT("No passcode specified! E.g., vE6xCpZxd96scOUGuLPbuLp8O800B0s"));
		PrintHelp();
		return false;
	}

	if ( GetLaunchContentNumber( outOptionsPS4.m_launchContentNumber, options ) )
	{
		LOG_WCC(TXT("Using launch content number: %u"), outOptionsPS4.m_launchContentNumber );
	}

	const Bool isPatch = outOptionsPS4.m_packageType.EqualsNC(TXT("patch"));
	if ( isPatch )
	{
		if ( !GetPatchAppPkgPath( outOptionsPS4.m_appPkgPath, options ) )
		{
			ERR_WCC(TXT("No patch app pkg specified!"));
			PrintHelp();
			return false;
		}
		LOG_WCC(TXT("Using patch pkg: %ls"), outOptionsPS4.m_appPkgPath.AsChar());

		GetLatestPatchPath( outOptionsPS4.m_latestPatchPath, options );
		LOG_WCC(TXT("Latest patch path: '%ls' (can be empty)"), outOptionsPS4.m_latestPatchPath.AsChar());

		outOptionsPS4.m_isDayOne = GetIsDayOnePatch( options );
		LOG_WCC(TXT("Is Day1 patch: %d"), outOptionsPS4.m_isDayOne);

		if ( outOptionsPS4.m_isDayOne && !outOptionsPS4.m_latestPatchPath.Empty())
		{
			ERR_WCC(TXT("Day1 patch with a latest QA patch? Don't think so."));
			PrintHelp();
			return false;
		}
	}

	outOptionsPS4.m_createPkg = !GetNoCreatePackage( options );
	outOptionsPS4.m_createIso = GetCreateIso( options );
	outOptionsPS4.m_createSubmissionMaterials = GetCreateSubmissionMaterials( options );
	outOptionsPS4.m_moveOuter = GetMoveOuter( options );
	outOptionsPS4.m_skipDigest = GetSkipDigest( options );
	outOptionsPS4.m_skipCRC = GetSkipCRC( options );

	return PackageMainPS4( outOptionsPS4 );
}

Bool CPackageCommandlet::RunDlcPackagePS4( const CommandletOptions& options, SOptionsPS4& outOptionsPS4 )
{
	if ( ! GetContentID( outOptionsPS4.m_contentID, options ) )
	{
		ERR_WCC(TXT("No content ID specified! E.g., V0002-NPXS29038_00-SIMPLESHOOTINGAM"));
		PrintHelp();
		return false;
	}

	if ( ! GetPasscode( outOptionsPS4.m_passcode, options ) )
	{
		ERR_WCC(TXT("No passcode specified! E.g., vE6xCpZxd96scOUGuLPbuLp8O800B0s"));
		PrintHelp();
		return false;
	}

	if ( ! GetEntitlementKey( outOptionsPS4.m_dlcEntitlementKey, options ) )
	{
		ERR_WCC(TXT("No entitlement key specified! E.g., 00112233445566778899aabbccddeeff"));
	}

	outOptionsPS4.m_createPkg = !GetNoCreatePackage( options );
	outOptionsPS4.m_createSubmissionMaterials = GetCreateSubmissionMaterials( options );
	outOptionsPS4.m_skipCRC = GetSkipCRC( options );

	return PackageMainPS4( outOptionsPS4 );
}

Bool CPackageCommandlet::RunPackageMainXboxOne( const CommandletOptions& options )
{
	SOptionsXboxOne optionsXboxOne;

	const TDynArray< String >& freeArgs = options.GetFreeArguments();

	if ( ! GetPackageType( optionsXboxOne.m_packageType, options ) )
	{
		ERR_WCC(TXT("Could not get package type"));
		return false;
	}

	if ( ! GetNormalizedPath( TXT("t"), TXT("tempdir"), optionsXboxOne.m_tempDir, options ) )
	{
		const String tempDir = GFileManager->GetTempDirectory() + TXT("packageCommandlet\\");
		LOG_WCC(TXT("No default tempdir specified. Defaulting to '%ls'"), tempDir.AsChar() );
		optionsXboxOne.m_tempDir = tempDir;
	}

	if ( !GetNormalizedPath(TXT("i"), TXT("indir"), optionsXboxOne.m_inDir, options ) )
	{
		ERR_WCC(TXT("Option 'indir' not specified"));
		PrintHelp();
		return false;
	}

	if ( ! GetNormalizedPath( TXT("o"), TXT("outdir"), optionsXboxOne.m_outDir, options ) )
	{
		ERR_WCC(TXT("Option 'outdir' not specified"));
		PrintHelp();
		return false;
	}

	String manifestContentName;
	if ( GetManifestContentName(manifestContentName, options ) )
	{
		optionsXboxOne.m_manifestContentName = manifestContentName;
	}

	optionsXboxOne.m_skipCRC = GetSkipCRC( options );

	if ( optionsXboxOne.m_packageType.EqualsNC(TXT("app")) || optionsXboxOne.m_packageType.EqualsNC(TXT("patch")) )
	{
		return RunAppPackageXboxOne( options, optionsXboxOne );
	}
	else if ( optionsXboxOne.m_packageType.EqualsNC(TXT("dlc")) )
	{
		return RunDlcPackageXboxOne( options, optionsXboxOne );
	}

	return false;
}

Bool CPackageCommandlet::RunAppPackageXboxOne( const CommandletOptions& options, SOptionsXboxOne& outOptionsXboxOne )
{
	if ( ! GetNormalizedPath( TXT("l"), TXT("langdir"), outOptionsXboxOne.m_langDir, options ) )
	{
		ERR_WCC(TXT("Option 'langdir' not specified"));
		PrintHelp();
		return false;
	}

	if ( ! GetStrings( outOptionsXboxOne.m_textLanguages, options ) || outOptionsXboxOne.m_textLanguages.Empty() ) 
	{
		ERR_WCC(TXT("No strings specified!"));
		PrintHelp();
		return false;
	}

	if ( ! GetSpeeches( outOptionsXboxOne.m_speechLanguages, options ) || outOptionsXboxOne.m_speechLanguages.Empty() ) 
	{
		ERR_WCC(TXT("No speech languages specified!"));
		PrintHelp();
		return false;
	}

	if ( ! GetDefaultSpeech( outOptionsXboxOne.m_defaultSpeech, options ) )
	{
		ERR_WCC(TXT("No default speech specified!"));
		PrintHelp();
		return false;
	}

	if ( GetLaunchContentNumber( outOptionsXboxOne.m_launchContentNumber, options ) )
	{
		LOG_WCC(TXT("Using launch content number: %u"), outOptionsXboxOne.m_launchContentNumber );
	}

	if ( ! GetExeName( outOptionsXboxOne.m_exeName, options ) )
	{
		ERR_WCC(TXT("No EXE specified! E.g., witcher3game.exe"));
		PrintHelp();
		return false;
	}

	if ( ! GetReleaseName( outOptionsXboxOne.m_releaseName, options ) )
	{
		ERR_WCC(TXT("No release name specified! E.g., ReleaseGame"));
		PrintHelp();
		return false;
	}

	return PackageMainXboxOne( outOptionsXboxOne );

}

Bool CPackageCommandlet::RunDlcPackageXboxOne( const CommandletOptions& options, SOptionsXboxOne& outOptionsXbone )
{
	return PackageMainXboxOne( outOptionsXbone );
}

const Char* CPackageCommandlet::GetOneLiner() const
{
	return TXT( "Packs a world directories into a single dzip for faster loading and deployment on consoles." );
}

void CPackageCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Creates streaming installer data for use by the game.") );
	LOG_WCC( TXT("Supported cooking platforms: PS4.") );
	LOG_WCC( TXT("Use: wcc package (ps4|orbis) app [ps4 app options]") );
	LOG_WCC( TXT("   ps4 app options:") );
	LOG_WCC( TXT("      -t=<path>   | --tempdir=<path>             -- temp directory. Optional") );
	LOG_WCC( TXT("      -i=<path>   | --indir=<path>               -- absolute path to final deliverable. E.g., z:\\Build_109641Change_652889\\PS4") );
	LOG_WCC( TXT("      -o=<path>   | --outdir=<path>              -- absolute path where to create gp4, pkg and iso") );
	LOG_WCC( TXT("      -l=<path>   | --langdir=<path>             -- absolute path to the language definitions. E.g., z:\\dev\\PublisherSpecific\\R4\\InternalDevelopment\\PS4\\language") );
	LOG_WCC( TXT("                  | --langs=[language list]>     -- comma separated list of game languages to support. E.g., en,pl") );
	LOG_WCC( TXT("                  | --dftlang=[language]          -- default game langauge. Must also be listed in the languages option. E.g., en") );

// 	LOG_WCC( TXT("Use: wcc package patch [options] (ps4|orbis)") );
// 	LOG_WCC( TXT("Use: wcc package dlc [options] (ps4|orbis)") );

// 
// 	LOG_WCC( TXT("Use: wcc package [options] (ps4|orbis)") );
// 	LOG_WCC( TXT("TODO: wcc package [options] (xboxone|durango)") );
// 	LOG_WCC( TXT("") );
// 	LOG_WCC( TXT("  [options] -- optional arguments: ") );
// 	LOG_WCC( TXT("") );
// 	LOG_WCC( TXT("    -v        | --verbose       -- verbose logging ") );
// 	LOG_WCC( TXT("    -dd       | --debugdata     -- generate debug data for package") );
// 	LOG_WCC( TXT("    -i=<path> | --indir=<path>  -- in directory for pack source, absolute path to playgo-chunks.xml") );
// 	LOG_WCC( TXT("    -o=<path> | --outdir=<path> -- out directory for packs, absolute path for playgo-chunks.dat") );
}

// static String FormatSize( Uint64 size )
// {
// 	Float KB = 1024.f;
// 	Float MB = KB * KB;
// 	Float GB = KB * KB * KB;
// 
// 	if ( size < KB )
// 	{
// 		return String::Printf(TXT("%llu bytes"), size );
// 	}
// 	else if ( size < MB )
// 	{
// 		return String::Printf(TXT("%.2f KB"), size / KB );
// 	}
// 	else if ( size < GB )
// 	{
// 		return String::Printf(TXT("%.2f MB"), size / MB );
// 	}
// 
// 	return String::Printf(TXT("%.2f GB"), size / GB );
// }

#endif
