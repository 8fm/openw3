#include "build.h"
#include "wccVersionControl.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/dependencyLinker.h"
#include "../../common/core/depot.h"
#include "../../common/core/garbageCollector.h"

#include "../../common/engine/swfResource.h"
#include "../../common/engine/swfTexture.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/redIO/redIO.h"
#include "../../common/core/stringConversion.h"
#include "../../common/redSystem/crt.h"
#include "cookerTextEncoder.h"
#include "processRunner.h"

#include <shellapi.h>

//#pragma optimize("",off)

//////////////////////////////////////////////////////////////////////////

const Int32 BASE_TRACK = 0;
const String videoLanguages( TXT("engine\\misc\\video_languages.csv") );
const Uint32 DEFAULT_USM_BITRATE= 16000;
#define ARABIC_LANGUAGE TXT("AR")
#define MEDIANOCHE_EXE TXT(".\\tools\\GFx4\\Tools\\VideoEncoder\\medianoche.exe")

class CVideoEncoderCommandlet: public ICommandlet
{
	DECLARE_ENGINE_CLASS( CVideoEncoderCommandlet, ICommandlet, 0 );

public:
	CVideoEncoderCommandlet();

public:
	// Executes commandlet command
	virtual Bool Execute( const CommandletOptions& options );

	// Returns commandlet one-liner
	virtual const Char* GetOneLiner() const
	{
		return TXT("Create USMs");
	}

	// Prints commandlet help
	virtual void PrintHelp() const
	{
		LOG_WCC( TXT("Creates USMs") );
	}

private:
	enum EAudioChannel
	{
		eAudioChannel_C,
		eAudioChannel_L,
		eAudioChannel_LS,
		eAudioChannel_R,
		eAudioChannel_RS,
		eAudioChannel_LFE,
		eAudioChannel_COUNT,
	};

	struct SLanguageTrackInfo
	{
		static const Uint32 INVALID_TRACK = -1;

		Int32 m_voiceTrack;
		Int32 m_subtitlesTrack;
		Int32 m_altVoiceTrack;
		Int32 m_altSubtitlesTrack;

		SLanguageTrackInfo()
			: m_voiceTrack( INVALID_TRACK )
			, m_subtitlesTrack( INVALID_TRACK )
			, m_altVoiceTrack( INVALID_TRACK )
			, m_altSubtitlesTrack( INVALID_TRACK )
		{}
	};

	enum ECueSubEventType
	{
		eCueSubEventType_Invalid,
		eCueSubEventType_Start,
		eCueSubEventType_End,
	};

	struct SCueSubEvent
	{
		ECueSubEventType	m_type;
		Uint32				m_time;

		SCueSubEvent()
			: m_type( eCueSubEventType_Invalid )
			, m_time( 0 )
		{}

		SCueSubEvent( ECueSubEventType type )
			: m_type( type )
			, m_time( 0 )
		{}

		Bool operator<( const SCueSubEvent& rhs ) const { return m_time < rhs.m_time; } // don't care about type
		Bool operator==( const SCueSubEvent& rhs ) const { return m_type == rhs.m_type && m_time == rhs.m_time; }
	};

	struct SSettings
	{
		String					m_usmDir;
		Uint32					m_bitrate;
		Bool					m_cuePointSubs;
		Bool					m_cookArabic;
		Bool					m_encodeUsm;

		SSettings()
			: m_bitrate( DEFAULT_USM_BITRATE )
			, m_cuePointSubs( false )
			, m_cookArabic( false )
			, m_encodeUsm( false )
		{}
	};

	struct SUsmSource
	{
		String								m_aviFile;
		TDynArray< String >					m_audioFiles;
		TDynArray< String >					m_altAudioFiles;
		TDynArray< String >					m_subsFiles;
		TDynArray< String >					m_altSubsFiles;
	};

	struct SFileMap
	{
		String								m_aviFile;
		String								m_usmFile;
		String								m_cuePointsFile;
		TArrayMap< EAudioChannel, String >	m_channelMusicFileMap;
		THashMap< String, String >			m_langVoiceFileMap;
		THashMap< String, String >			m_langAltVoiceFileMap;
		THashMap< String, String >			m_langSubsFileMap;
		THashMap< String, String >			m_langAltSubsFileMap;
	};

	struct SBaseVoiceTrack
	{
		String				m_language;
		SLanguageTrackInfo	m_trackInfo;
	};

private:
	Bool CreateUsmFileMappings( const String& projDir, const SUsmSource& usmSource, SFileMap& outFileMap, const SSettings& settings );
	Bool CreateVideoEncoderProject( const String& projDir, const SFileMap& fileMap, const SSettings& settings, String& outMedianocheBatFile ) const;
	Bool CopyLooseSubs( const SFileMap& fileMap, const SSettings& settings ) const;
	Bool CreateCuepointsForSubtitles( const String& newCuePointsFile, const String& subsFile, const String& altSubsFile );
	Bool PopulateCueSubEvents( const String& subsFile, TDynArray<SCueSubEvent>& outCueSubEvents );

private:
	Bool GatherUsmSource( const String& dirPath, const SSettings& settings, SUsmSource& outUsmSource );
	Bool EncodeArabicSubs( const String& subsFile, const String& newSubsFile, Bool utf8 );
	Bool EncodeSubs( const String& subsFile, const String& newSubsFile, Bool utf8 );

private:
	Bool InitLanguageTracks();
	Bool GetLanguageFromPath( const String& path, String& outLang, Bool silentError = false );
	Bool GetAudioChannelFromPath( const String& path, EAudioChannel& outChannel );
	Int32 GetMedianoche51ChannelNumber( EAudioChannel channel ) const;
	const Char* GetAudioChannelSuffix( EAudioChannel channel ) const;

private:
	THashMap< String, SLanguageTrackInfo >	m_languageTracks;
	SBaseVoiceTrack							m_baseLanguageTrack;
	THashMap< String, EAudioChannel >		m_audioChannels;
	CCookerTextEncoder						m_textEncoder;
};

DEFINE_SIMPLE_RTTI_CLASS( CVideoEncoderCommandlet, ICommandlet );
IMPLEMENT_ENGINE_CLASS( CVideoEncoderCommandlet );

CVideoEncoderCommandlet::CVideoEncoderCommandlet()
{
	m_commandletName = CName( TXT("venc") ); // Video encoder
}

void ConformDir( String& outDir )
{
	outDir.ReplaceAll(TXT('/'), TXT('\\'));
	if ( !outDir.Empty() && !outDir.EndsWith(TXT("\\")) )
	{
		outDir += TXT("\\");
	}
}

static Bool CheckSubsFile( const String& subsFile )
{
	Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( subsFile, FOF_AbsolutePath ) );
	if ( !file )
	{
		ERR_WCC(TXT("Can't open subsFile '%ls'"), subsFile.AsChar());
		return false;
	}

	if ( file->GetSize() < 2 )
	{
		ERR_WCC(TXT("subsFile '%ls' is too small"), subsFile.AsChar());
		return false;
	}

	// Quick check encoding
	Uint8 header[2];
	*file << header[0];
	*file << header[1];

	if ( header[0] != 0xFF || header[1] != 0xFE )
	{
		ERR_WCC(TXT("subsFile '%ls' doesn't seem to be in UCS-2 LE encoding!"), subsFile.AsChar());
		return false;
	}

	return true;
}

Bool CVideoEncoderCommandlet::GatherUsmSource( const String& dirPath, const SSettings& settings, SUsmSource& outUsmSource )
{
	GFileManager->FindFiles( dirPath + TXT("audio\\"), TXT("*.wav"), outUsmSource.m_audioFiles, true );
	GFileManager->FindFiles( dirPath + TXT("altaudio\\"), TXT("*.wav"), outUsmSource.m_altAudioFiles, true );
	GFileManager->FindFiles( dirPath + TXT("subs\\"), TXT("*.txt"), outUsmSource.m_subsFiles, true );
	GFileManager->FindFiles( dirPath + TXT("altsubs\\"), TXT("*.txt"), outUsmSource.m_altSubsFiles, true );

	TDynArray< String > videoFiles;
	GFileManager->FindFiles( dirPath + TXT("video\\"), TXT("*.avi"), videoFiles, true );
	if ( !videoFiles.Empty() )
	{
		if ( videoFiles.Size() > 1 )
		{
			ERR_WCC(TXT("Multiple AVI files found in video directory of '%ls'"), dirPath.AsChar() );
			return false;
		}
		outUsmSource.m_aviFile = videoFiles[0];
	}

	// TODO: Consistency checking - subs without audio etc. Should make sure always has a track zero center audio e.g.,

	for ( const String& subsFile : outUsmSource.m_subsFiles )
	{
		if ( !CheckSubsFile( subsFile ) )
		{
			return false;
		}
	}

	for ( const String& altSubsFile : outUsmSource.m_altSubsFiles )
	{
		if ( !CheckSubsFile( altSubsFile ) )
		{
			return false;
		}
	}

	return true;
}

static Bool ExecuteBatFile( const String& batFile )
{
	LOG_WCC(TXT("Running %ls"), batFile.AsChar() );
	CTimeCounter batTimer;

	SHELLEXECUTEINFO executeInfo = {0};
	executeInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	executeInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	executeInfo.lpVerb = NULL;
	executeInfo.lpFile = batFile.AsChar();
	executeInfo.lpParameters = TXT("");
	executeInfo.nShow = SW_SHOW;
	if ( ::ShellExecuteEx( &executeInfo ) == FALSE )
	{
		const DWORD dwError = ::GetLastError();
		ERR_WCC(TXT("Failed to run %ls. GetLastError() returned 0x%08X"), dwError );
		return false;
	}

	const DWORD waitRet = ::WaitForSingleObject( executeInfo.hProcess, INFINITE );
	if ( waitRet != WAIT_OBJECT_0 )
	{
		const DWORD dwError = ::GetLastError();
		if ( waitRet == WAIT_FAILED )
		{
			ERR_WCC(TXT("Failed to wait for %ls. GetLastError() returned 0x%08X"), dwError );
		}
		else
		{
			ERR_WCC(TXT("Failed to wait for %ls. Returned wait code 0x%08X"), waitRet );
		}
		return false;
	}

	DWORD exitCode = 0;
	if ( ::GetExitCodeProcess( executeInfo.hProcess, &exitCode ) == FALSE )
	{
		const DWORD dwError = ::GetLastError();
		ERR_WCC(TXT("Failed to get process exit code. GetLastError() returned 0x%08X"), dwError );
		return false;
	}

	if ( exitCode != 0 )
	{
		ERR_WCC(TXT("Exit code is non-zero: %u"), exitCode );
		return false;
	}

	LOG_WCC(TXT("Bat file finished in %1.2f sec"), batTimer.GetTimePeriod() );

	return true;
}

Bool CVideoEncoderCommandlet::Execute( const CommandletOptions& options )
{
	if ( ! InitLanguageTracks() )
	{
		return false;
	}

	String inDir;
	if ( !options.GetSingleOptionValue( TXT("indir"), inDir ) )
	{
		ERR_WCC(TXT("Option indir missing"));
		PrintHelp();
		return false;
	}
	ConformDir( inDir );

	SSettings settings;

	String usmDir;
	if ( !options.GetSingleOptionValue( TXT("usmdir"), settings.m_usmDir ) )
	{
		ERR_WCC(TXT("Option usmdir missing"));
		PrintHelp();
		return false;
	}
	ConformDir( settings.m_usmDir );

	String strBitrate;
	if ( !options.GetSingleOptionValue( TXT("bitrate"), strBitrate) || !FromString( strBitrate, settings.m_bitrate ))
	{
		ERR_WCC(TXT("Option bitrate missing"));
		PrintHelp();
	}

	String strCuePointSubs;
	if (!options.GetSingleOptionValue( TXT("cue_subs"), strCuePointSubs ) || !FromString( strCuePointSubs, settings.m_cuePointSubs ))
	{
		ERR_WCC(TXT("Option cue_subs missing"));
		PrintHelp();
		return false;
	}

	String strArFont;
	if ( !options.GetSingleOptionValue( TXT("ar_font"), strArFont ) || strArFont.Empty() )
	{
		ERR_WCC(TXT("Option ar_font missing"));
		PrintHelp();
		return false;
	}

	if ( ! m_textEncoder.LoadFont( strArFont ) )
	{
		ERR_WCC(TXT("Failed to load fontPath '%ls'"), strArFont.AsChar() );
		return false;
	}

	m_textEncoder.SelectLanguageScript( CCookerTextEncoder::SCRIPT_Arabic );

	const CFilePath fontFilePath( strArFont );
	const String fontNameKey = fontFilePath.GetFileNameWithExt().ToLower();
	if ( ! m_textEncoder.SelectFont( fontNameKey ) )
	{
		ERR_WCC(TXT("Failed to use font %ls"), fontNameKey.AsChar() );
		return false;
	}

	settings.m_cookArabic = true;

	TDynArray< String > failedDirs;
	TDynArray< String > finishedDirs;
	TDynArray< String > failedBatFiles;
	TDynArray< String > finishedBatFiles;
	String strEncodeUsm;
	if (!options.GetSingleOptionValue( TXT("encode_usm"), strEncodeUsm ) || !FromString( strEncodeUsm, settings.m_encodeUsm ))
	{
		ERR_WCC(TXT("Option encode_usm missing"));
		PrintHelp();
		return false;
	}
	
	TDynArray< String > dirs;
	TDynArray< String > medianocheBatFiles;
	GFileManager->FindDirectories( inDir, dirs );
	for ( const String& dir : dirs )
	{
		LOG_WCC(TXT("Processing '%ls'..."), dir.AsChar());
		SUsmSource usmSource;
		const String absoluteDir = inDir + dir + TXT("\\");
		if (!GatherUsmSource( absoluteDir, settings, usmSource ))
		{
			failedDirs.PushBack( dir );
			continue;
		}

		const String projDir = absoluteDir + TXT("proj\\");

		SFileMap fileMap;
		if ( !CreateUsmFileMappings( projDir, usmSource, fileMap, settings ) )
		{
			failedDirs.PushBack( dir );
			continue;
		}
				
		String medianocheBatFile;
		if ( !CreateVideoEncoderProject( projDir, fileMap, settings, medianocheBatFile ) )
		{
			failedDirs.PushBack( dir );
			continue;
		}

		if ( settings.m_cuePointSubs && !CopyLooseSubs( fileMap, settings ) )
		{
			failedDirs.PushBack( dir );
			continue;
		}

		finishedDirs.PushBack( dir );
		medianocheBatFiles.PushBack( Move( medianocheBatFile ) );
	}
	
	if ( settings.m_encodeUsm )
	{
		LOG_WCC(TXT("Encoding %u USMs... this may take a while"), medianocheBatFiles.Size() );
		LOG_WCC(TXT("=== Video encoding intermediate summary ==="));
		LOG_WCC(TXT("Finished %u video dirs:"), finishedDirs.Size() );
		for ( const String& dir : finishedDirs )
		{
			LOG_WCC(TXT("\t%ls"), dir.AsChar() );
		}
		LOG_WCC(TXT("Failed to process %u video dirs:"), failedDirs.Size() );
		for ( const String& dir : failedDirs )
		{
			LOG_WCC(TXT("\t%ls"), dir.AsChar() );
		}

		CTimeCounter totalEncodeTimer;
		const String exeName = MEDIANOCHE_EXE;
		for ( const String& batFile : medianocheBatFiles )
		{
			if ( !ExecuteBatFile( batFile ) )
			{
				failedBatFiles.PushBack( batFile );
			}
			else
			{
				finishedBatFiles.PushBack( batFile );
			}
		}

		LOG_WCC(TXT("Finished running %u encoding bat files in %1.2f sec"), medianocheBatFiles.Size(), totalEncodeTimer.GetTimePeriod() );
	}

	LOG_WCC(TXT("=== Video encoding final summary ==="));
	LOG_WCC(TXT("Finished %u video dirs:"), finishedDirs.Size() );
	for ( const String& dir : finishedDirs )
	{
		LOG_WCC(TXT("\t%ls"), dir.AsChar() );
	}
	LOG_WCC(TXT("Failed to process %u video dirs:"), failedDirs.Size() );
	for ( const String& dir : failedDirs )
	{
		LOG_WCC(TXT("\t%ls"), dir.AsChar() );
	}
	LOG_WCC(TXT("Finished %u video encodings:"), finishedBatFiles.Size() );
	for ( const String& batFile : finishedBatFiles )
	{
		LOG_WCC(TXT("\t%ls"), batFile.AsChar() );
	}
	LOG_WCC(TXT("Failed %u video encodings:"), failedBatFiles.Size() );
	for ( const String& batFile : failedBatFiles )
	{
		LOG_WCC(TXT("\t%ls"), batFile.AsChar() );
	}

	LOG_WCC( TXT("Done!") );

	if ( !failedDirs.Empty() || !failedBatFiles.Empty() )
	{
		return false;
	}

	return true;
}

Bool CVideoEncoderCommandlet::CopyLooseSubs( const SFileMap& fileMap, const SSettings& settings ) const
{
	if ( !settings.m_cuePointSubs )
	{
		ERR_WCC(TXT("Invalid call to CopyLooseSubs!"));
		return false;
	}

	CFilePath usmFilePath( fileMap.m_usmFile );

	const String subsDir = settings.m_usmDir + TXT("subs\\");
	const String altSubsDir = settings.m_usmDir + TXT("altsubs\\");

	if ( !fileMap.m_langSubsFileMap.Empty() )
	{
		if ( !GSystemIO.CreatePath( subsDir.AsChar() ) )
		{
			ERR_WCC(TXT("Failed to create subs directory '%ls'"), subsDir.AsChar());
			return false;
		}
	}

	if ( !fileMap.m_langAltSubsFileMap.Empty() )
	{
		if ( !GSystemIO.CreatePath( altSubsDir.AsChar() ) )
		{
			ERR_WCC(TXT("Failed to create alt subs directory '%ls'"), altSubsDir.AsChar());
			return false;
		}
	}

	const String subsBaseFile = subsDir + usmFilePath.GetFileName();
	for ( auto it = fileMap.m_langSubsFileMap.Begin(); it != fileMap.m_langSubsFileMap.End(); ++it )
	{
		const String& lang = it->m_first;
		const String& subsFile = it->m_second;
		const String newSubsFile = subsBaseFile + TXT("_") + lang.ToLower() + TXT(".subs");
		if ( !GFileManager->CopyFile( subsFile, newSubsFile, true ) )
		{
			ERR_WCC(TXT("Failed to copy subsFile '%ls' to newSubsFile '%ls'"), subsFile.AsChar(), newSubsFile.AsChar() );
			return false;
		}
	}

	const String altSubsBaseFile = altSubsDir + usmFilePath.GetFileName();
	for ( auto it = fileMap.m_langAltSubsFileMap.Begin(); it != fileMap.m_langAltSubsFileMap.End(); ++it )
	{
		const String& lang = it->m_first;
		const String& altSubsFile = it->m_second;
		const String newAltSubsFile = altSubsBaseFile + TXT("_") + lang.ToLower() + TXT(".subs");
		if ( !GFileManager->CopyFile( altSubsFile, newAltSubsFile, true ) )
		{
			ERR_WCC(TXT("Failed to copy altSubsFile '%ls' to newAltSubsFile '%ls'"), altSubsFile.AsChar(), newAltSubsFile.AsChar() );
			return false;
		}
	}

	return true;
}

Bool CVideoEncoderCommandlet::PopulateCueSubEvents( const String& subsFile, TDynArray<SCueSubEvent>& outCueSubEvents )
{
	String subtitlesBlob;
	if ( ! GFileManager->LoadFileToString( subsFile, subtitlesBlob, true ) )
	{
		ERR_WCC(TXT("Could not open subtitles '%ls' for parsing"), subsFile.AsChar() );
		return false;
	}

	TDynArray< String > lines = subtitlesBlob.Split( TXT("\n") );
	if ( lines.Empty() )
	{
		ERR_WCC(TXT("Subtitles '%ls' has no lines for parsing"), subsFile.AsChar() );
		return false;
	}

	Uint32 timeInterval = 0;
	if ( !::FromString( lines[0], timeInterval ) )
	{
		ERR_WCC(TXT("Subtitles '%ls' has no time interval for parsing"), subsFile.AsChar() );
		return false;
	}

	lines.RemoveAt( 0 );

	for ( const String& textLine : lines )
	{
		const Char* ch = textLine.AsChar();
		SCueSubEvent startEvent( eCueSubEventType_Start );
		SCueSubEvent endEvent( eCueSubEventType_End );

		if ( !GParseInteger( ch, startEvent.m_time ) || !GParseKeyword( ch, TXT(",") ) )
		{
			ERR_WCC(TXT("Subtitles '%ls' has no start time for parsing"), subsFile.AsChar() );
			return false;
		}
		if ( !GParseInteger( ch, endEvent.m_time ) || !GParseKeyword( ch, TXT(",") ) )
		{
			ERR_WCC(TXT("Subtitles '%ls' has no end time for parsing"), subsFile.AsChar() );
			return false;
		}

		outCueSubEvents.PushBack( startEvent );
		outCueSubEvents.PushBack( endEvent );
	}

	return true;
}

Bool CVideoEncoderCommandlet::CreateCuepointsForSubtitles( const String& newCuePointsFile, const String& subsFile, const String& altSubsFile )
{
	TDynArray< SCueSubEvent > subEvents;
	TDynArray< SCueSubEvent > altSubEvents;

	if ( !PopulateCueSubEvents( subsFile, subEvents ) )
	{
		return false;
	}
	if ( ! PopulateCueSubEvents( altSubsFile, altSubEvents ) )
	{
		return false;
	}

	Sort( subEvents.Begin(), subEvents.End() );
	Sort( altSubEvents.Begin(), altSubEvents.End() );

	FILE* fp = nullptr;
	const errno_t err = _wfopen_s( &fp, newCuePointsFile.AsChar(), TXT("wt") );
	if ( err != 0 )
	{
		ERR_WCC(TXT("Failed to open file"));
		return false;
	}

	const Uint32 timeInterval = 1000;
	fprintf_s( fp, "%u\n", timeInterval );

	struct SSubContext
	{
		TDynArray< SCueSubEvent >*	m_pEvents;
		const Char*					m_suffix;
	};
	
	TDynArray< SSubContext > subContexts;
	const Bool useUnifiedEvents = ( subEvents == altSubEvents );
	if ( useUnifiedEvents )
	{
		SSubContext ctxt = { &subEvents, TXT("") };
		subContexts.PushBack( ctxt );
	}
	else
	{
		SSubContext mainCtxt = { &subEvents, TXT("_main") };
		subContexts.PushBack( mainCtxt );

		SSubContext altCtxt = { &altSubEvents, TXT("_alt") };
		subContexts.PushBack( altCtxt );
	}

	for ( const SSubContext& ctxt : subContexts )
	{
		Uint32 cueIndex = 0;
		Bool expectStartEvent = true;
		for ( const SCueSubEvent& subEvent : *ctxt.m_pEvents )
		{
			switch( subEvent.m_type )
			{
			case eCueSubEventType_Start:
				{
					if ( !expectStartEvent )
					{
						ERR_WCC(TXT("Mismatched start event for newCuePointsFile"), newCuePointsFile.AsChar() );
						return false;
					}
					expectStartEvent = false;
					fprintf_s( fp, "%u,0,subs_start%ls%u\n", subEvent.m_time, ctxt.m_suffix, cueIndex );
				}
				break;
			case eCueSubEventType_End:
				{
					if ( expectStartEvent )
					{
						ERR_WCC(TXT("Mismatched end event for newCuePointsFile"), newCuePointsFile.AsChar() );
						return false;
					}
					expectStartEvent = true;
					fprintf_s( fp, "%u,1,subs_end%ls%u\n", subEvent.m_time, ctxt.m_suffix, cueIndex );
					++cueIndex;
				}
				break;
			default:
				ERR_WCC(TXT("Unexpected cue-point subtitle point type %u"), subEvent.m_type );
				return false;
				break;
			}
		}
	}

	fclose( fp );

	return true;
}

Bool CVideoEncoderCommandlet::CreateUsmFileMappings( const String& projDir, const SUsmSource& usmSource, SFileMap& outFileMap, const SSettings& settings )
{
	outFileMap.m_aviFile = usmSource.m_aviFile;

	CFilePath aviFilePath( outFileMap.m_aviFile );
	String base = aviFilePath.GetFileName();
	base.ReplaceAll(TXT(' '), TXT('_'));
	base = base.ToLower();
	outFileMap.m_usmFile = settings.m_usmDir + base + TXT(".usm");

	TArrayMap< EAudioChannel, String >&	channelMusicFileMap		= outFileMap.m_channelMusicFileMap;
	THashMap< String, String >&			langVoiceFileMap		= outFileMap.m_langVoiceFileMap;
	THashMap< String, String >&			langAltVoiceFileMap		= outFileMap.m_langAltVoiceFileMap;
	THashMap< String, String >&			langSubsFileMap			= outFileMap.m_langSubsFileMap;
	THashMap< String, String >&			langAltSubsFileMap		= outFileMap.m_langAltSubsFileMap;

	// Subtitles read in as UCS-2 LE, and loose subs files need to stay that way, but utf8 if used as normal subtitles
	const Bool utf8 = !settings.m_cuePointSubs;

	// Get main audio
	for ( const String& audioFile : usmSource.m_audioFiles )
	{
		EAudioChannel channel;
		if ( !GetAudioChannelFromPath( audioFile, channel ) )
		{
			return false;
		}

		if ( channel == eAudioChannel_C )
		{
			String lang;
			if (!GetLanguageFromPath( audioFile, lang ))
			{
				return false;
			}
			if ( !langVoiceFileMap.Insert( lang, audioFile ) )
			{
				ERR_WCC(TXT("Duplicate language %ls for audioFile '%ls'"), lang.AsChar(), audioFile.AsChar() );
				return false;
			}
		}
		else 
		{
			if ( channelMusicFileMap.Find( channel ) != channelMusicFileMap.End() )
			{
				ERR_WCC(TXT("Duplicate channel %ls for audioFile '%ls'"), GetAudioChannelSuffix( channel ), audioFile.AsChar() );
				return false;
			}
			channelMusicFileMap.Insert( channel, audioFile );
		}
	}

	for ( const String& subsFile : usmSource.m_subsFiles )
	{
		String lang;
		if ( !GetLanguageFromPath(subsFile, lang) )
		{
			return false;
		}

		CFilePath filePath( subsFile );
		const String encodedSubsFile = projDir + TXT("enc_subs\\") + filePath.GetFileNameWithExt();
		if ( settings.m_cookArabic && lang.EqualsNC(ARABIC_LANGUAGE) )
		{
			if ( !EncodeArabicSubs( subsFile, encodedSubsFile, utf8 ) )
			{
				return false;
			}
		}
		else
		{
			if ( !EncodeSubs( subsFile, encodedSubsFile, utf8 ) )
			{
				return false;
			}
		}

		if ( !langSubsFileMap.Insert( lang, encodedSubsFile ) )
		{
			ERR_WCC(TXT("Duplicate language %ls for subsFile '%ls'"), lang.AsChar(), encodedSubsFile.AsChar() );
			return false;
		}
	}

	// Alt voice tracks
	for ( const String& altAudioFile : usmSource.m_altAudioFiles )
	{
		EAudioChannel channel;
		if ( !GetAudioChannelFromPath( altAudioFile, channel ) )
		{
			return false;
		}
		if ( channel != eAudioChannel_C )
		{
			ERR_WCC(TXT("Audio channel %ls must be center voice track for altAudioFile '%ls'"), GetAudioChannelSuffix( channel ), altAudioFile.AsChar() );
			return false;
		}
		String lang;
		if (!GetLanguageFromPath( altAudioFile, lang ))
		{
			return false;
		}
		if ( !langAltVoiceFileMap.Insert( lang, altAudioFile ) )
		{
			ERR_WCC(TXT("Duplicate language %ls for altAudioFile '%ls'"), lang.AsChar(), altAudioFile.AsChar() );
			return false;
		}
	}

	for ( const String& altSubsFile : usmSource.m_altSubsFiles )
	{
		String lang;
		if ( !GetLanguageFromPath(altSubsFile, lang) )
		{
			return false;
		}

		CFilePath filePath( altSubsFile );
		const String encodedAltSubsFile = projDir + TXT("enc_altsubs\\") + filePath.GetFileNameWithExt();
		if ( settings.m_cookArabic && lang.EqualsNC(ARABIC_LANGUAGE) )
		{
			if ( !EncodeArabicSubs( altSubsFile, encodedAltSubsFile, utf8 ) )
			{
				return false;
			}
		}
		else
		{
			if ( !EncodeSubs( altSubsFile, encodedAltSubsFile, utf8 ) )
			{
				return false;
			}
		}

		if ( !langAltSubsFileMap.Insert( lang, encodedAltSubsFile ) )
		{
			ERR_WCC(TXT("Duplicate language %ls for altSubsFile '%ls'"), lang.AsChar(), encodedAltSubsFile.AsChar() );
			return false;
		}
	}

	// Create cue-point driven subtitles if requested
	if ( settings.m_cuePointSubs && !usmSource.m_subsFiles.Empty() )
	{
		const String cueSubsFile = projDir + TXT("cue_subs.txt");
		const String& subsFile = usmSource.m_subsFiles[0];
		const String& altSubsFile = !usmSource.m_altSubsFiles.Empty() ? usmSource.m_altSubsFiles[0] : subsFile;

		if ( !CreateCuepointsForSubtitles( cueSubsFile, subsFile, altSubsFile ) )
		{
			return false;
		}

		outFileMap.m_cuePointsFile = cueSubsFile;
	}

	return true;
}

struct SFileGuard : Red::System::NonCopyable
{
	FILE* m_fp;

	SFileGuard(FILE* fp )
		: m_fp(fp)
	{}

	~SFileGuard()
	{
		if ( m_fp )
		{
			fclose(m_fp);
		}
	}
};

Bool CVideoEncoderCommandlet::CreateVideoEncoderProject( const String& projDir, const SFileMap& fileMap, const SSettings& settings, String& outMedianocheBatFile ) const
{
	if (!GSystemIO.CreatePath(projDir.AsChar()))
	{
		ERR_WCC(TXT("Failed to create project directory '%ls'"), projDir.AsChar());
		return false;
	}

	// The Scaleform video encoder tool will also gray out the encode button until this folder exists
	if ( !GSystemIO.CreatePath(settings.m_usmDir.AsChar()))
	{
		ERR_WCC(TXT("Failed to create USM output directory '%ls'"), settings.m_usmDir.AsChar());
		return false;
	}

	CFilePath projFilePath( projDir );
	CFilePath aviFilePath( fileMap.m_aviFile );

	String svesPath = projFilePath.GetPathString( true ) + aviFilePath.GetFileName() + TXT(".sves");
	svesPath.ReplaceAll( TXT(' '), TXT('_') );
	svesPath = svesPath.ToLower();

	FILE* fp = nullptr;
	errno_t err = _wfopen_s( &fp, svesPath.AsChar(), TXT("wt") );
	if ( err != 0 )
	{
		ERR_WCC(TXT("Failed to open sves file '%ls'"), svesPath.AsChar() );
		return false;
	}

	String batPath = projFilePath.GetPathString( true ) + aviFilePath.GetFileName() + TXT(".bat");
	batPath.ReplaceAll( TXT(' '), TXT('_') );
	batPath = batPath.ToLower();

	FILE* batf = nullptr;
	err = _wfopen_s( &batf, batPath.AsChar(), TXT("wt") );
	if ( err != 0 )
	{
		ERR_WCC(TXT("Failed to open bat file '%ls'"), batPath.AsChar());
		return false;
	}
	
	SFileGuard fg( fp );
	SFileGuard fgBat( batf );

	fprintf_s( fp, "; SFVES 1.00 - Do not edit or delete this line\n");
	fprintf_s( fp, "input_file = %ls\n", fileMap.m_aviFile.AsChar() );
	fprintf_s( fp, "output_file = %ls\n", fileMap.m_usmFile.AsChar() );
	fprintf_s( fp, "cuepoints_file = %ls\n", fileMap.m_cuePointsFile.AsChar() );

	const String mediaEncodeExePath = GFileManager->GetBaseDirectory() + MEDIANOCHE_EXE;
	fprintf_s( batf, "\"%ls\" -video00=\"%ls\" -output=\"%ls\"",
		mediaEncodeExePath.AsChar(),
		fileMap.m_aviFile.AsChar(),
		fileMap.m_usmFile.AsChar() );

	if ( !fileMap.m_cuePointsFile.Empty() )
	{
		fprintf_s( batf, " -cuepoint=\"%ls\"", fileMap.m_cuePointsFile.AsChar() );
	}

	if ( !settings.m_cuePointSubs )
	{
		// Add main subtitle tracks
		for ( auto it = fileMap.m_langSubsFileMap.Begin(); it != fileMap.m_langSubsFileMap.End(); ++it )
		{
			const String& lang = it->m_first;
			const String& subsFile = it->m_second;

			const SLanguageTrackInfo* trackInfo = m_languageTracks.FindPtr( lang );
			if ( ! trackInfo )
			{
				ERR_WCC(TXT("Failed to find trackInfo for %ls"), lang.AsChar() );
				return false;
			}
			const Int32 subsTrack = trackInfo->m_subtitlesTrack;
			fprintf_s( fp, "%d_subtitles = %ls\n", subsTrack, subsFile.AsChar() );
			fprintf_s( batf, " -subtitle%02d=\"%ls\"", subsTrack, subsFile.AsChar() );
		}

		// Add alt subtitle tracks
		for ( auto it = fileMap.m_langAltSubsFileMap.Begin(); it != fileMap.m_langAltSubsFileMap.End(); ++it )
		{
			const String& lang = it->m_first;
			const String& altSubsFile = it->m_second;

			const SLanguageTrackInfo* trackInfo = m_languageTracks.FindPtr( lang );
			if ( ! trackInfo )
			{
				ERR_WCC(TXT("Failed to find trackInfo for %ls"), lang.AsChar() );
				return false;
			}
			const Int32 altSubsTrack = trackInfo->m_altSubtitlesTrack;
			fprintf_s( fp, "%d_subtitles = %ls\n", altSubsTrack, altSubsFile.AsChar() );
			fprintf_s( batf, " -subtitle%02d=\"%ls\"", altSubsTrack, altSubsFile.AsChar() );
		}
	}

	// Track zero including voice is 5.1ch
	fprintf_s( fp, "0_audio_type = 2\n");
	const String& baseLang = m_baseLanguageTrack.m_language;
	String voiceFile;
	if ( !fileMap.m_langVoiceFileMap.Find( baseLang, voiceFile ) )
	{
		ERR_WCC(TXT("Missing base language voiceFile"));
		return false;
	}
	fprintf_s( fp, "0_audio_file_%ls = %ls\n", GetAudioChannelSuffix(eAudioChannel_C), voiceFile.AsChar() );
	fprintf_s( batf, " -mca00_%02d=\"%ls\"", GetMedianoche51ChannelNumber(eAudioChannel_C), voiceFile.AsChar() );

	const EAudioChannel musicChannels[] = { eAudioChannel_L, eAudioChannel_LS, eAudioChannel_R, eAudioChannel_RS, eAudioChannel_LFE, };
	for ( Uint32 i = 0; i < ARRAY_COUNT_U32(musicChannels); ++i )
	{
		const EAudioChannel channel = musicChannels[i];
		const Char* suffix = GetAudioChannelSuffix( channel );
		auto musicFindIt = fileMap.m_channelMusicFileMap.Find( channel );
		if ( musicFindIt == fileMap.m_channelMusicFileMap.End() )
		{
			ERR_WCC(TXT("Missing audioFile for channel %ls"), suffix );
			return false;
		}
		const String& audioFile = musicFindIt->m_second;
		fprintf_s( fp, "0_audio_file_%ls = %ls\n", suffix, audioFile.AsChar() );
		fprintf_s( batf, " -mca00_%02d=\"%ls\"", GetMedianoche51ChannelNumber(channel), audioFile.AsChar() );
	}

	// Add other main language voice tracks
	for ( auto it = fileMap.m_langVoiceFileMap.Begin(); it != fileMap.m_langVoiceFileMap.End(); ++it )
	{
		const String& lang = it->m_first;
		const String& voiceFile = it->m_second;

		// Already added as 5.1ch center track
		if ( lang == m_baseLanguageTrack.m_language )
		{
			continue;
		}

		const SLanguageTrackInfo* trackInfo = m_languageTracks.FindPtr( lang );
		if ( ! trackInfo )
		{
			ERR_WCC(TXT("Failed to find trackInfo for %ls"), lang.AsChar() );
			return false;
		}
		const Int32 voiceTrack = trackInfo->m_voiceTrack;
		fprintf_s( fp, "%d_audio_type = 1\n", voiceTrack );
		fprintf_s( fp, "%d_audio_file = %ls\n", voiceTrack, voiceFile.AsChar() );
		fprintf_s( batf, " -audio%02d=\"%ls\"", voiceTrack, voiceFile.AsChar() );
	}
	
	// Add alt language voice tracks
	for ( auto it = fileMap.m_langAltVoiceFileMap.Begin(); it != fileMap.m_langAltVoiceFileMap.End(); ++it )
	{
		const String& lang = it->m_first;
		const String& altVoiceFile = it->m_second;

		const SLanguageTrackInfo* trackInfo = m_languageTracks.FindPtr( lang );
		if ( ! trackInfo )
		{
			ERR_WCC(TXT("Failed to find trackInfo for %ls"), lang.AsChar() );
			return false;
		}
		const Int32 altVoiceTrack = trackInfo->m_altVoiceTrack;
		fprintf_s( fp, "%d_audio_type = 1\n", altVoiceTrack );
		fprintf_s( fp, "%d_audio_file = %ls\n", altVoiceTrack, altVoiceFile.AsChar() );
		fprintf_s( batf, " -audio%02d=\"%ls\"", altVoiceTrack, altVoiceFile.AsChar() );
	}

	fprintf_s( fp, "bitrate = %u\n", settings.m_bitrate);
 	fprintf_s( fp, "framerate_base = 30\n" );
 	fprintf_s( fp, "framerate_scale = 1\n" );
 
 	fprintf_s( fp, "use_input_framerate = 0\n" );
 	fprintf_s( fp, "use_audio_track = 0\n" );
 	fprintf_s( fp, "use_alpha_channel = 0\n" );
 	fprintf_s( fp, "enable_resize = 0\n" );
 	fprintf_s( fp, "resize_width = 0\n" );
 	fprintf_s( fp, "resize_height = 0\n" );
 	fprintf_s( fp, "resize_height = 0\n" );
 	fprintf_s( fp, "use_use_hca_codec = 0\n" );
 	fprintf_s( fp, "hca_quality = 4\n" );

	fprintf_s( batf, " -bitrate=%u", settings.m_bitrate );
	fprintf_s( batf, " -framerate=30,1" );

	outMedianocheBatFile.PushBack( batPath );

	return true;
}

Bool CVideoEncoderCommandlet::InitLanguageTracks()
{
	m_audioChannels[ TXT("C") ] = eAudioChannel_C;
	m_audioChannels[ TXT("L") ] = eAudioChannel_L;
	m_audioChannels[ TXT("LFE") ] = eAudioChannel_LFE;
	m_audioChannels[ TXT("LS") ] = eAudioChannel_LS;
	m_audioChannels[ TXT("R") ] = eAudioChannel_R;
	m_audioChannels[ TXT("RS") ] = eAudioChannel_RS;
	
	THandle< C2dArray > ar = LoadResource< C2dArray >( videoLanguages );
	if ( ! ar )
	{
		ERR_WCC(TXT("Failed to load language track settings!"));
		return false;
	}

	struct SCsvMap
	{
		const Char*		m_header;
		Uint32*			m_column;
	};

	struct ConvertTrack
	{
		Int32 operator()( const String& val ) const 
		{
			Int32 track = SLanguageTrackInfo::INVALID_TRACK;
			::FromString( val, track );

			return track;
		}
	};

	Uint32 colLanguage;
	Uint32 colVoiceTrack;
	Uint32 colSubtitlesTrack;
	Uint32 colAltVoiceTrack;
	Uint32 colAltSubtitlesTrack;
	SCsvMap headers[] = { { TXT("Language"), &colLanguage },
						  { TXT("VoiceTrack"), &colVoiceTrack }, { TXT("SubtitlesTrack"), &colSubtitlesTrack},
						  { TXT("AltVoiceTrack"), &colAltVoiceTrack }, { TXT("AltSubtitlesTrack"), &colAltSubtitlesTrack },
	};

	for ( Uint32 i = 0; i < ARRAY_COUNT_U32(headers); ++i )
	{
		const SCsvMap& csvMap = headers[i];
		if ( !ar->FindHeader( csvMap.m_header, *(csvMap.m_column) ) )
		{
			ERR_WCC(TXT("Failed to load language track settings! Can't find header %ls"), csvMap.m_header );
			return false;
		}
	}

	Bool foundBaseTrack = false;
	ConvertTrack convTrack;
	for ( Uint32 i = 0; i < ar->GetNumberOfRows(); ++i )
	{
		const String& language = ar->GetValueRef( colLanguage, i );
		SLanguageTrackInfo trackInfo;

		trackInfo.m_voiceTrack = convTrack( ar->GetValue( colVoiceTrack, i ) );
		trackInfo.m_subtitlesTrack = convTrack( ar->GetValue( colSubtitlesTrack, i ) );
		trackInfo.m_altVoiceTrack = convTrack( ar->GetValue( colAltVoiceTrack, i ) );
		trackInfo.m_altSubtitlesTrack = convTrack( ar->GetValue( colAltSubtitlesTrack, i ) );

		if ( !m_languageTracks.Insert( language.ToUpper(), trackInfo ) )
		{
			ERR_WCC(TXT("langauge %ls already mapped to tracks!"), language.AsChar() );
			return false;
		}

		if ( trackInfo.m_voiceTrack == BASE_TRACK )
		{
			if ( foundBaseTrack )
			{
				ERR_WCC(TXT("Multiple tracks with base track (%u)!"), BASE_TRACK);
				return false;
			}
			foundBaseTrack = true;
			m_baseLanguageTrack.m_language = language.ToUpper();
			m_baseLanguageTrack.m_trackInfo = trackInfo;
		}
	}

	if ( !foundBaseTrack )
	{
		ERR_WCC(TXT("Failed to find base track (%u) in video settings"), BASE_TRACK);
		return false;
	}

	return true;
}

Bool CVideoEncoderCommandlet::GetLanguageFromPath( const String& path, String& outLang, Bool silentError /*= false*/ )
{
	String lang = path.StringAfter(TXT("_"), true ).ToUpper();
	lang = lang.StringBefore(TXT("."), true );
	
	if ( lang.Empty() )
	{
		if (!silentError)
		{
			ERR_WCC(TXT("Path '%ls' doesn't have a language code separated by an underscore at the end. Example expected format XXXXX_EN.wav"), path.AsChar() );
		}
		return false;
	}
	SLanguageTrackInfo* trackInfo = m_languageTracks.FindPtr( lang );
	if ( !trackInfo )
	{
		if (!silentError)
		{
			ERR_WCC(TXT("No language track info found for path '%ls', language %ls"), path.AsChar(), lang.AsChar() );
		}
		return false;
	}

	outLang = lang;
	return true;
}

Int32 CVideoEncoderCommandlet::GetMedianoche51ChannelNumber( EAudioChannel channel ) const
{
	Int32 channelNumber = -1;
	// From sf_4.3_CRI_encoder.pdf
	switch (channel)
	{
	case eAudioChannel_C:
		channelNumber = 4;
		break;
	case eAudioChannel_L:
		channelNumber = 0;
		break;
	case eAudioChannel_LS:
		channelNumber = 2;
		break;
	case eAudioChannel_R:
		channelNumber = 1;
		break;
	case eAudioChannel_RS:
		channelNumber = 3;
		break;
	case eAudioChannel_LFE:
		channelNumber = 5;
		break;
	default:
		ERR_WCC(TXT("Unexpected channel %u"), channel );
		break;
	}

	return channelNumber;
}

Bool CVideoEncoderCommandlet::GetAudioChannelFromPath( const String& path, EAudioChannel& outChannel )
{
	String lang;
	String channel;
	if ( GetLanguageFromPath( path, lang, true ) )
	{
		// Get the channel name between the last two underscores (e.g., XXX_C_BR.wav)
		channel = path.StringBefore( TXT("_"), true);
		channel = channel.StringAfter( TXT("_"), true );
	}
	else
	{
		// Get the channel name from end of the file name
		channel = path.StringAfter(TXT("_"), true ).ToUpper();
		channel = channel.StringBefore(TXT("."), true );
	}
	
	if ( channel.Empty() )
	{
		ERR_WCC(TXT("Path '%ls' doesn't have a track code separated by an underscore at the end. Example expected format XXXXX_C_EN.wav or XXXXX_C.wav"), path.AsChar() );
		return false;
	}

	if ( !m_audioChannels.Find( channel.ToUpper(), outChannel ) )
	{
		ERR_WCC(TXT("Path '%ls' with unknown channel %ls. Expected format XXXXX_C_EN.wav or XXXXX_C.wav"), path.AsChar(), channel.AsChar() );
		return false;
	}

	return true;
}

const Char* CVideoEncoderCommandlet::GetAudioChannelSuffix( EAudioChannel channel ) const
{
	const Char* suffix = TXT("<Error>");
	switch ( channel )
	{
	case eAudioChannel_C:
		suffix = TXT("C");
		break;
	case eAudioChannel_L:
		suffix = TXT("L");
		break;
	case eAudioChannel_LS:
		suffix = TXT("LS");
		break;
	case eAudioChannel_R:
		suffix = TXT("R");
		break;
	case eAudioChannel_RS:
		suffix = TXT("RS");
		break;
	case eAudioChannel_LFE:
		suffix = TXT("LFE");
		break;
	default:
		break;
	}

	return suffix;
}

Bool CVideoEncoderCommandlet::EncodeArabicSubs( const String& subsFile, const String& newSubsFile, Bool utf8 )
{
	String subtitlesBlob;
	if ( ! GFileManager->LoadFileToString( subsFile, subtitlesBlob, true ) )
	{
		ERR_WCC(TXT("Could not open '%ls' for XML parsing"), subsFile.AsChar() );
		return false;
	}

	String outBlob;
	TDynArray< String > lines = subtitlesBlob.Split( TXT("\n") );

	for ( const String& line : lines )
	{
		String outText;
		if ( m_textEncoder.ShapeText( line, outText ) )
		{
			// Put text back into logical (vs presentation) order so Scaleform can reverse it more fine grained using its bidi alg
			// and not break HTML tags
			m_textEncoder.ReverseTextInPlace( outText );
		}

		if ( outText == line )
		{
			WARN_WCC(TXT("Line unchanged: %ls"), line.AsChar() );
		}

		outBlob += outText;
		outBlob += TXT("\r\n");
	}
	
	if ( utf8 )
	{
		if ( ! GFileManager->SaveStringToFileWithUTF8( newSubsFile, outBlob ) )
		{
			ERR_WCC(TXT("Failed to save file '%ls'"), newSubsFile.AsChar() );
			return false;
		}
	}
	else
	{
		IFile* file = GFileManager->CreateFileWriter( newSubsFile, FOF_Buffered | FOF_AbsolutePath );
		if ( !file || !GFileManager->SaveStringToFileWithUTF16( *file, outBlob ) )
		{
			ERR_WCC(TXT("Failed to save file '%ls'"), newSubsFile.AsChar() );
			return false;
		}
		delete file;
	}

	// Done
	return true;
}

Bool CVideoEncoderCommandlet::EncodeSubs( const String& subsFile, const String& newSubsFile, Bool utf8 )
{
	String subtitlesBlob;
	if ( ! GFileManager->LoadFileToString( subsFile, subtitlesBlob, true ) )
	{
		ERR_WCC(TXT("Could not open '%ls' for XML parsing"), subsFile.AsChar() );
		return false;
	}

	String outBlob = Move( subtitlesBlob );
	if ( utf8 )
	{
		if ( ! GFileManager->SaveStringToFileWithUTF8( newSubsFile, outBlob ) )
		{
			ERR_WCC(TXT("Failed to save file '%ls'"), newSubsFile.AsChar() );
			return false;
		}
	}
	else
	{
		IFile* file = GFileManager->CreateFileWriter( newSubsFile, FOF_Buffered | FOF_AbsolutePath );
		if ( !file || !GFileManager->SaveStringToFileWithUTF16( *file, outBlob ) )
		{
			ERR_WCC(TXT("Failed to save file '%ls'"), newSubsFile.AsChar() );
			return false;
		}
		delete file;
	}

	return true;
}
