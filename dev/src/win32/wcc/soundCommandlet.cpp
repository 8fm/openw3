/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/engine/soundFileLoader.h"
#include "../../common/core/dependencyMapper.h"
#include "soundBankHelper.h"
#include "cookSplitList.h"
#include "../../common/core/2darray.h"
#include "../../common/core/gatheredResource.h"
#include "playgoHelper.h"
#include "../../common/engine/soundCacheDataFormat.h"
#include "../../common/engine/soundSystem.h"

CGatheredResource resSoundToChunkMapping( TXT("soundbanks\\soundsplitseed.csv"), RGF_NotCooked );
extern CGatheredResource resAlwaysLoadedSoundBankNames;

namespace
{
	const String BANK_FILE_EXTENSION = TXT("bnk");
	const String WAM_FILE_EXTENSION = TXT("wam");
	const String BANK_INFO_FILE_EXTENSION = TXT("txt");
	const CName CONTENT_ZERO_CHUNK = CName(TXT("content0"));

	struct SSoundCachingSummary
	{
		SSoundCachingSummary()
			: bankFilesMissingInInitSplitCSV( 0 )
			, txtFilesMissing( 0 )
			, wemFilesMissing( 0 )
			, wemFilesMissingInTxtDesc( 0 )
			, banksProcessed( 0 )
			, wemsProcessed( 0 )
			, wemsMappedToMoreThanOneChunk( 0 )
		{
			/* Intentionally Empty */
		}

		Uint32 bankFilesMissingInInitSplitCSV;
		Uint32 txtFilesMissing;
		Uint32 wemFilesMissing;
		Uint32 wemFilesMissingInTxtDesc;
		Uint32 banksProcessed;
		Uint32 wemsProcessed;
		Uint32 wemsMappedToMoreThanOneChunk;
	};
}

class CSoundCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CSoundCommandlet, ICommandlet, 0 );

public:
	CSoundCommandlet();

	virtual const Char* GetOneLiner() const;

	virtual bool Execute( const CommandletOptions& options );

	virtual void PrintHelp() const;

	bool CreateCookedSounds( const String& input, const String& output, const String& platform, const String& splitlistFilePath );

	// Sound bank split list creation
	CName GetBankRelatedChunk( const Uint32 rowidx, const C2dArray* initCSV );
	Bool FindBankInInitCSV( const String& fileName, const C2dArray* initCSV, Uint32& outRowIdx );
	Bool ListStreamedAudioForBank( const String& bankResourcePath, const TDynArray< String >& wemPaths,
		TDynArray< String >& outStreamedAudioPaths, SSoundCachingSummary& summary );

};

BEGIN_CLASS_RTTI( CSoundCommandlet )
	PARENT_CLASS( ICommandlet )
	END_CLASS_RTTI()

	IMPLEMENT_ENGINE_CLASS( CSoundCommandlet );

RED_DEFINE_STATIC_NAME( cooksounds )

	CSoundCommandlet::CSoundCommandlet()
{
	m_commandletName = CNAME( cooksounds );
}

bool CSoundCommandlet::Execute( const CommandletOptions& options )
{
	auto arguments = options.GetFreeArguments();

	String outputDir;

	if( options.GetSingleOptionValue( TXT("outdir"), outputDir ) == false )
	{
		ERR_WCC( TXT("No outdir option. Try -outdir=\"directory\\\"") );
		return false;
	}

	// Voice overs source dir
	String soundResourceDirectory;

	if( options.GetSingleOptionValue( TXT("wwiseProj"), soundResourceDirectory ) == false )
	{
		ERR_WCC( TXT("No wwiseProj option. Try -wwiseProj=\"directory\\project\"") );
		return false;
	}

	auto platforms = TList<String>(); 
	if( options.HasOption( TXT( "platform" ) ) )
	{
		platforms = options.GetOptionValues( TXT( "platform" ) );
	}else
	{
		WARN_WCC( TXT( "No target platform was specified. Assuming PC." ) );
		platforms.PushBack( TXT("pc") );
	}

	String splitlist = TXT("");

	if( options.GetSingleOptionValue( TXT("split"), splitlist ) == false )
	{
		WARN_WCC( TXT("No split option. Try -split=\"splitlistfilepath\"") );
	}

	LOG_WCC( TXT("Splitlist file path: %ls"), splitlist.AsChar() );

	for( auto platformsIt = platforms.Begin(); platformsIt != platforms.End(); platformsIt++ )
	{
		if(!CreateCookedSounds( soundResourceDirectory, outputDir, (*platformsIt).ToLower(), splitlist ) )
		{
			return false;
		}
	}

	return true;

}

bool CSoundCommandlet::CreateCookedSounds( const String& input, const String& output, const String& platform, const String& splitlistFilePath )
{
	RED_LOG( Cooker, TXT("Parsing arguments...") );

	ECookingPlatform cookingPlatform;
	Bool hadBadArgument = false;
	String soundCacheResultPath = output;
	if ( platform == TXT("pc") )
	{
		cookingPlatform = ECookingPlatform::PLATFORM_PC;
		soundCacheResultPath += TXT("CookedPC\\soundspc.cache");
	}
#ifndef WCC_LITE
	else if ( platform == TXT("xboxone") || platform == TXT("durango") )
	{
		cookingPlatform = ECookingPlatform::PLATFORM_XboxOne;
		soundCacheResultPath += TXT("CookedXboxOne\\soundsxboxone.cache");
	}
	else if ( platform == TXT("ps4") || platform == TXT("orbis") )
	{
		cookingPlatform = ECookingPlatform::PLATFORM_PS4;
		soundCacheResultPath += TXT("CookedPS4\\soundsps4.cache");
	}
#endif
	else
	{
		return false;
	}

	// Create paths
	String sourcePath = GFileManager->GetDataDirectory() + TXT("..\\audio\\" );

	String tempCache = GFileManager->GetBaseDirectory() + TXT("wwiseCacheTemp\\");

	GFileManager->CreatePath( output );
	GFileManager->CreatePath( tempCache );

	String params = TXT( "tools\\wwise\\Win32\\Release\\bin\\WwiseCLI.exe " );
	params += input;
	params += TXT(" -GenerateSoundBanks ");
	params += TXT(" -SoundBankPath " );
#ifndef WCC_LITE
	if ( cookingPlatform == ECookingPlatform::PLATFORM_XboxOne )
	{
		params += TXT( "XboxOne" );
	}
	else if ( cookingPlatform == ECookingPlatform::PLATFORM_PS4 )
	{
		params += TXT( "PS4" );
	}
	else
#endif
	{
		params += TXT( "Windows" );
	}

	params += TXT(" ");
	params += tempCache;

#ifndef WCC_LITE
	if ( cookingPlatform == ECookingPlatform::PLATFORM_XboxOne )
	{
		params += TXT( "XboxOne" );
	}
	else if ( cookingPlatform == ECookingPlatform::PLATFORM_PS4 )
	{
		params += TXT( "PS4" );
	}
	else
#endif
	{
		params += TXT( "Windows" );
	}

	params += TXT(" -Platform ");

#ifndef WCC_LITE
	if ( cookingPlatform == ECookingPlatform::PLATFORM_XboxOne )
	{
		params += TXT( "XboxOne" );
		tempCache += TXT( "XboxOne\\" );
	}
	else if ( cookingPlatform == ECookingPlatform::PLATFORM_PS4 )
	{
		params += TXT( "PS4" );
		tempCache += TXT( "PS4\\" );
	}
	else
#endif
	{
		params += TXT( "Windows" );
		tempCache += TXT( "Windows\\" );
	}

	//params += TXT("-Verbose");

	/*	"c:\Program Files (x86)\Audiokinetic\Wwise v2013.2.8 build 4865\Authoring\x64\Re
	lease\bin\WwiseCLI.exe" D:\lukasz.zieba.audio\depot\W3_Assets\audio\w3_audio\w3_
	audio.wproj -GenerateSoundBanks -SoundBankPath Windows "e:\lukasz.zieba.Main\wwi
	seCacheTemp\Windows" -Platform Windows*/

	//	RED_LOG( CNAME( physx ), params.AsChar() );
	LOG_WCC( TXT("Params: %ls"), params.AsChar() );
	int result = system( UNICODE_TO_ANSI( params.AsChar() ) );

	/******************************/
	/***** Create sound cache *****/
	/******************************/

	IFile* soundCacheOutputFile = GFileManager->CreateFileWriter( soundCacheResultPath, FOF_Buffered | FOF_AbsolutePath );
	if ( !soundCacheOutputFile )
	{
		ERR_WCC( TXT("Failed to create output file '%ls'"), soundCacheResultPath.AsChar() );
		return false;
	}

	/***** Find all banks and wems *****/
	TDynArray< String > bankPaths;
	TDynArray< String > wemPaths;
	TDynArray< String > paths;

	// Move init.bnk to bin/initialdata/sound/...
	// Initial bank must be loaded before everything else, and before bumper videos, so we need to load it before any content is available
	String initBankDestinationPath = GFileManager->GetRootDirectory() + TXT("bin\\initialdata\\sound\\");
	GFileManager->CreatePath( initBankDestinationPath );
	GFileManager->MoveFile( tempCache + TXT("Init.bnk"), initBankDestinationPath + TXT("Init.bnk") );

	GFileManager->FindFiles( tempCache, TXT( "*.bnk" ), bankPaths, false );
	GFileManager->FindFiles( tempCache, TXT( "*.wem" ), wemPaths, false );

	paths.PushBack( bankPaths );
	paths.PushBack( wemPaths );

	/***** Save cache *****/
	CSoundCacheData::WriteHeader( *soundCacheOutputFile );

	// Skip to start of first data block
	const Uint64 writePos = sizeof( CSoundCacheData::RawHeader ) + sizeof( CSoundCacheData::IndexHeader );
	soundCacheOutputFile->Seek( writePos );

	CSoundCacheData soundCacheData;
	CSoundCacheDataBuilder soundCacheDataBuilder( soundCacheData );

	for( String& path : paths )
	{
		// Load sound resource to buffer
		IFile* soundResource = GFileManager->CreateFileReader( path, FOF_Buffered | FOF_AbsolutePath );
		size_t soundResourceSize = static_cast< size_t >( soundResource->GetSize() );
		RED_ASSERT( (Uint64)soundResourceSize == soundResource->GetSize(), TXT("Unexpectedly large file '%s'"), soundResource->GetFileNameForDebug() );
		Uint8* buffer = ( Uint8* ) malloc( soundResourceSize );

		soundResource->Serialize( buffer, soundResourceSize );

		String fileName = CFilePath( path ).GetFileNameWithExt();
		fileName = fileName.ToLowerUnicode();

		// Get write position
		const Uint64 writePos = (Uint64) soundCacheOutputFile->GetOffset();

		// Create cache token
		CSoundCacheData::CacheToken cacheToken;
		cacheToken.m_dataOffset = writePos;
		cacheToken.m_dataSize = (Uint32)soundResource->GetSize();
		cacheToken.m_name = soundCacheDataBuilder.AddString( fileName );
		soundCacheDataBuilder.AddToken( cacheToken );

		// write data to output file
		soundCacheOutputFile->Serialize( buffer, soundResourceSize );
		
		free( buffer );

		delete soundResource;
	}

	// Save tables to file
	const Uint64 endOfFilePos = (Uint64) soundCacheOutputFile->GetOffset();
	soundCacheOutputFile->Seek( sizeof( CSoundCacheData::RawHeader ) );
	soundCacheData.Save( *soundCacheOutputFile, endOfFilePos );

	delete soundCacheOutputFile;


	/*****************************/
	/***** Create split list *****/
	/*****************************/
	if( splitlistFilePath.Empty() )
	{
		return true;		// Just don't fill splitlist data
	}

	CCookerSplitFile splitListFile;
	splitListFile.LoadFromFile( splitlistFilePath );
	THashMap< String, TDynArray< CName > > wemToChunkMapping;

	/***** Load our magic list from sound designers *****/
	C2dArray* soundPerLevel = resSoundToChunkMapping.LoadAndGet<C2dArray>();
	if( soundPerLevel == nullptr )
	{
		ERR_WCC( TXT("Can't load sounds per level file: %ls"), resSoundToChunkMapping.GetPath().ToString().AsChar() );
		return false;
	}

	SSoundCachingSummary summary;

	/***** Iterate though all banks *****/
	for( auto& resourcePath : bankPaths )
	{
		// Skip all non-banks resources
		if( StringHelpers::GetFileExtension( resourcePath ) != BANK_FILE_EXTENSION )
		{
			continue;
		}

		String bankResourceFileName = CFilePath( resourcePath ).GetFileNameWithExt();		// Store only filename in splitlist
		CName finalChunkName;		// Final chunk for particular bank and it's streamed audios
		Uint32 bankRowIdx = 0;		// Find current bank in magic list

		//LOG_WCC( TXT("Caching sound bank: %ls"), bankResourceFileName.AsChar() );

		Bool bankFound = FindBankInInitCSV( bankResourceFileName, soundPerLevel, bankRowIdx );

		if( bankFound == true )
		{
			finalChunkName = GetBankRelatedChunk( bankRowIdx, soundPerLevel );
			//splitListFile.AddEntry( UNICODE_TO_ANSI( bankResourceFileName.AsChar() ), finalChunkName );
			summary.banksProcessed++;
			//LOG_WCC( TXT("SplitList.AddEntry: %ls to %ls"), ANSI_TO_UNICODE( bankResourceFileName.AsChar() ), finalChunkName.AsChar() );
		}
		else
		{
			RED_LOG_ERROR( WCC_SoundSplitListCreation, TXT("Can't find sound bank in initial split csv file, by default it will be in content0 - bank: %ls Please add this bank to proper content chunk in r4data\\soundbanks\\soundsplitseed.csv"), bankResourceFileName.AsChar() );
			summary.bankFilesMissingInInitSplitCSV++;
			finalChunkName = CONTENT_ZERO_CHUNK;
		}

		// Find all streamed audio files (WEMs) related to that bank (BNK)
		TDynArray<String> relatedStreamedAudioPaths;
		if( ListStreamedAudioForBank( resourcePath, wemPaths, relatedStreamedAudioPaths, summary ) )
		{
			for( auto& streamedPath : relatedStreamedAudioPaths )
			{
				auto it = wemToChunkMapping.Find( streamedPath );
				if( it != wemToChunkMapping.End() )
				{
					it.Value().PushBack( finalChunkName );
				}
				else
				{
					TDynArray<CName> chunks;
					chunks.PushBack( finalChunkName );
					wemToChunkMapping.Insert( streamedPath, chunks );
				}
			}
		}

		// if present in soundbanks_always_load.csv then add to content0
		C2dArray* alwaysLoadedBankNames = resAlwaysLoadedSoundBankNames.LoadAndGet< C2dArray >();

		Bool bankAlwaysLoaded = false;

		if ( alwaysLoadedBankNames != nullptr )
		{
			Uint32 count = static_cast<Uint32>( alwaysLoadedBankNames->GetNumberOfRows() );

			for ( Uint32 index = 0; index < count; ++index )
			{
				const String& fileName = alwaysLoadedBankNames->GetValue( 0, index ).ToLower();
				if( fileName == bankResourceFileName )
				{
					bankAlwaysLoaded = true;
				}
			}
		}

		if( bankAlwaysLoaded == true )
		{
			LOG_WCC( TXT("Adding bank entry (bank always loaded): %ls to content0 (it's wems will be added to %ls)"), bankResourceFileName.AsChar(), finalChunkName.AsChar() );
			splitListFile.AddEntry( UNICODE_TO_ANSI( bankResourceFileName.AsChar() ), CONTENT_ZERO_CHUNK );
		}
		else
		{
			LOG_WCC( TXT("Adding bank entry: %ls to %ls"), bankResourceFileName.AsChar(), finalChunkName.AsChar() );
			splitListFile.AddEntry( UNICODE_TO_ANSI( bankResourceFileName.AsChar() ), finalChunkName );
		}
	}

	for( auto& wemChunksPair : wemToChunkMapping )
	{
		PlayGoHelper::CChunkResolver chunkResolver;
		PlayGoHelper::CChunkResolver::SResolveContext chunkResolveContext;
		chunkResolveContext.m_chunks = &wemChunksPair.m_second;

		Bool wemExistsOnDisk = false;
		for( Uint32 i=0; i<wemPaths.Size(); ++i )
		{
			if( wemChunksPair.m_first == CFilePath( wemPaths[i] ).GetFileNameWithExt() )
			{
				wemExistsOnDisk = true;
				break;
			}
		}

		if( wemExistsOnDisk == false )
		{
			summary.wemFilesMissing++;
			ERR_WCC( TXT("Wem file is missing on disk: %ls"), wemChunksPair.m_first.AsChar() );
		}

		if( wemChunksPair.m_second.Size() > 1 )
		{
			summary.wemsMappedToMoreThanOneChunk++;
		}

		CName finalWemChunkName = chunkResolver.ResolveContentChunk( chunkResolveContext );

		LOG_WCC( TXT("Adding wem entry: %ls to %ls"), wemChunksPair.m_first.AsChar(), finalWemChunkName.AsChar() );
		splitListFile.AddEntry( UNICODE_TO_ANSI( wemChunksPair.m_first.AsChar() ), finalWemChunkName );
		summary.wemsProcessed++;
	}

	/***** Save split file *****/

	splitListFile.SaveToFile( splitlistFilePath );

	/***** Sound split list creation summary *****/

	LOG_WCC( TXT( "/************************/" ) );
	LOG_WCC( TXT( "Sound caching summary" ) );
	LOG_WCC( TXT( "    banks processed:                                           %i" ), summary.banksProcessed );
	LOG_WCC( TXT( "    wems processed:                                            %i" ), summary.wemsProcessed );
	LOG_WCC( TXT( "    bank files missing in init split CSV (added to content0):  %i" ), summary.bankFilesMissingInInitSplitCSV );
	LOG_WCC( TXT( "    txt files missing on disk:                                 %i" ), summary.txtFilesMissing );
	LOG_WCC( TXT( "    wem files missing on disk:                                 %i" ), summary.wemFilesMissing );
	LOG_WCC( TXT( "    wem files missing in txt descriptions:                     %i" ), summary.wemFilesMissingInTxtDesc );
	LOG_WCC( TXT( "    wems mapped to more than one chunk:                        %i" ), summary.wemsMappedToMoreThanOneChunk );
	LOG_WCC( TXT( "    entries with None as chunk value will be added to content0 by default" ) );
	LOG_WCC( TXT( "/************************/" ) );

	/*****************************/

	return true;
}

const Char* CSoundCommandlet::GetOneLiner() const
{
	return TXT( "No operation" );
}

void CSoundCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Use: " ) );
	LOG_WCC( TXT( "wcc cooksounds output_dir sound_resource_dir platforms" ) );
}

CName CSoundCommandlet::GetBankRelatedChunk( const Uint32 rowIdx, const C2dArray* initCSV )
{
	TDynArray<CName> belongToChunkList;

	Uint32 colNum = initCSV->GetNumberOfColumns();

	for( Uint32 c=1; c<colNum; ++c )
	{
		String doBelongToChunk = initCSV->GetValue( c, rowIdx );
		if( doBelongToChunk == TXT("1") )
		{
			String chunkStr = initCSV->GetHeader( c );
			CName chunkName = CName( chunkStr );
			belongToChunkList.PushBack( chunkName );
			//LOG_WCC( TXT("Add to content chunk: %ls"), chunkName.AsChar() );
		}
	}

	// Resolve chunk and add entry to split list file
	PlayGoHelper::CChunkResolver chunkResolver;
	PlayGoHelper::CChunkResolver::SResolveContext chunkResolveContext;
	chunkResolveContext.m_chunks = &belongToChunkList;
	return chunkResolver.ResolveContentChunk( chunkResolveContext );
}

Bool CSoundCommandlet::FindBankInInitCSV( const String& fileName, const C2dArray* initCSV, Uint32& outRowIdx )
{
	Uint32 rowNum = initCSV->GetNumberOfRows();

	for( Uint32 r=0; r<rowNum; ++r )
	{
		String bankName = initCSV->GetValue( 0, r );

		if( bankName == fileName )
		{
			outRowIdx = r;
			return true;
		}
	}

	return false;
}

Bool CSoundCommandlet::ListStreamedAudioForBank( const String& bankResourcePath, const TDynArray< String >& wemPaths,
												TDynArray< String >& outStreamedAudioPaths, SSoundCachingSummary& summary )
{
	// Get related txt file, load it to 2darray with '\t' separators
	String txtFilePath = CFilePath( bankResourcePath ).GetPathString() + TXT("\\") + CFilePath( bankResourcePath ).GetFileName();
	txtFilePath += TXT(".") + BANK_INFO_FILE_EXTENSION;

	String txtFileContent;

	//LOG_WCC( TXT("Description file: %ls"), txtFilePath.AsChar() );
	if( GFileManager->LoadFileToString( txtFilePath, txtFileContent, true ) )
	{
		TDynArray<CWWiseStreamedAudioEntry> streamedSounds;
		CWWiseDescriptionParser::GetStreamedAudioEntries( txtFileContent, streamedSounds );

		// Fill entry desc with streamed sound resources
		for( auto& streamedSound : streamedSounds )
		{
			Bool wemFound = false;

			for( auto& streamedResourcePath : wemPaths )
			{
				String streamedResourceFileName = CFilePath( streamedResourcePath ).GetFileNameWithExt();
				if( streamedResourceFileName == streamedSound.generatedAudioFile )
				{
					//LOG_WCC( TXT("SplitList.AddEntry: %ls to %ls"), streamedResourceFileName.AsChar(), finalChunkName.AsChar() );
					outStreamedAudioPaths.PushBack( streamedResourceFileName );
					wemFound = true;
					break;
				}
			}

			if( wemFound == false )
			{
				WARN_WCC( TXT("Can't find wem file in txt description - wem: %ls"), streamedSound.generatedAudioFile.AsChar() );
				summary.wemFilesMissingInTxtDesc++;
			}
		}
	}
	else
	{
		ERR_WCC( TXT("Can't load description file: %ls"), txtFilePath.AsChar() );
		summary.txtFilesMissing++;

		return false;
	}

	return true;
}
