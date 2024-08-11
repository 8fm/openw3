#include "build.h"

#include "../../common/engine/localizationManager.h"
#include "../../common/engine/cookedbinStringDBDataAccess.h"
#include "../../common/engine/cookedLocaleKeys.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/depot.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/core/garbageCollector.h"

#include "cookerTextEncoder.h"

#include <shellapi.h>

class CStringCookerCommandlet: public ICommandlet
{
	DECLARE_ENGINE_CLASS( CStringCookerCommandlet, ICommandlet, 0 );

public:

	CStringCookerCommandlet( )
	{
		m_commandletName = CNAME( cookstrings );
	}

	virtual Bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner( ) const;
	virtual void PrintHelp( ) const;

private:

	struct TSettings
	{
		String				m_outputDir;
		String				m_speechResourceDirectory;
		String				m_stringsView;
		TList< String >		m_languages;
		TList< String >		m_platforms;
		Bool				m_skipSpeech;
		Bool				m_rebuild;

		TSettings( ) { }

		Bool Parse( const ICommandlet::CommandletOptions& options );
	};

	struct TSpeechMetaData
	{
		Uint64 m_voTime;
		Uint64 m_lipsyncTime;

		friend IFile& operator << ( IFile& file, TSpeechMetaData& speechData )
		{
			file << speechData.m_voTime;
			file << speechData.m_lipsyncTime;
			return file;
		}
	};

	typedef THashMap< Uint32, TSpeechMetaData > TMetaDataMap;

	struct TSpeechMetadataFile
	{
		static const Uint32 FILE_MAGIC = 'SMET';

		static Bool Load( IFile& file, TMetaDataMap& outMap )
		{
			Uint64 baseOffset = file.GetOffset( );

			Uint32 magic = 0, version = 0;
			file << magic;

			// We could validate old content by decrypting it, but oh well..
			if( magic != FILE_MAGIC )
				return false;

			file << version;
			if( version > VER_CURRENT )
				return false;

			file << outMap;

			return true;
		}

		static void Save( IFile& file, TMetaDataMap& outMap )
		{
			Uint32 magic = FILE_MAGIC;
			Uint32 version = VER_CURRENT;

			file << magic;
			file << version;
			file << outMap;
		}
	};

private:

	TSettings			m_settings;
	String				m_voiceoverExtension;
	String				m_lipsyncExtension;
	CCookerTextEncoder	m_textEncoder;

private:

	Bool CookStrings( const String& language );
	Bool CreateCookedFiles( THashMap<Uint32, LanguagePack*>& packs, const String& lang );
	Bool CreateCookedStrings( const THashMap<Uint32, LanguagePack*>& packs, const String& lang );
	Bool CreateCookedSpeech( const THashMap<Uint32, LanguagePack*>& packs, const String& lang, const String& platform );

	Bool FillVoiceover( LanguagePack& pack, const String& language );
	Bool FillLipsync( LanguagePack& pack, const String& language );

	TSpeechMetaData GetMetaData( const LanguagePack& pack, const String& lang );

	RED_INLINE TString< Char > GetVoiceoverPath( const String &language, const String &voiceOverFileName )
	{
		return String::Printf( TXT( "%s\\%s\\audio\\%s.%s" ), 
			m_settings.m_speechResourceDirectory.AsChar(), 
			language.AsChar(), 
			voiceOverFileName.AsChar(), 
			m_voiceoverExtension.AsChar() );
	}

	RED_INLINE TString< Char > GetLipsyncPath( const String &language, const String &voiceOverFileName )
	{
		return  String::Printf( TXT( "%s\\%s\\lipsync\\%s.%s" ), 
			m_settings.m_speechResourceDirectory.AsChar(), 
			language.AsChar(), 
			voiceOverFileName.AsChar(), 
			m_lipsyncExtension.AsChar() );
	}
};

BEGIN_CLASS_RTTI( CStringCookerCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI( )

IMPLEMENT_ENGINE_CLASS( CStringCookerCommandlet );

Bool CStringCookerCommandlet::Execute( const CommandletOptions& options )
{
	if( !m_settings.Parse( options ) )
		return false;

	if( !SLocalizationManager::GetInstance( ).OpenSQLDataAccess( true ) )
	{
		ERR_WCC( TXT( "Can't open SQL data access for the localization manager" ) );
		return false;
	}

	SLocalizationManager::GetInstance( ).SetIgnoreLanguagePackResources( true );
	
	for( TList< String >::iterator langIter = m_settings.m_languages.Begin( ); langIter != m_settings.m_languages.End( ); ++langIter )
	{
		if( !CookStrings( *langIter ) )
		{
			return false;
		}
	}

	return true;
}

const Char* CStringCookerCommandlet::GetOneLiner( ) const
{
	return TXT( "Cooks strings and speech database, given a list of languages and platforms." );
};

void CStringCookerCommandlet::PrintHelp( ) const 
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  cookstrings <out_dir> <source_dir> <db_string_view> -languages=<langs> -platforms=<plats> <options>" ) );
	LOG_WCC( TXT( "    out_dir        - Path to the directory where the caches will be stored." ) );
	LOG_WCC( TXT( "    source_dir     - Path to the base directory where speech data resides." ) );
	LOG_WCC( TXT( "    db_string_view - SQL query." ) );
	LOG_WCC( TXT( "    languages      - List of languages to cook." ) );
	LOG_WCC( TXT( "    platforms      - List of platforms to cook for." ) );
	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "Options:" ) );
	LOG_WCC( TXT( "  -skipspeech      - Cooks only strings." ) );
};

Bool CStringCookerCommandlet::TSettings::Parse( const ICommandlet::CommandletOptions& options )
{
	auto arguments = options.GetFreeArguments( );

	if( arguments.Size( ) < 3 )
	{
		ERR_WCC( TXT( "Insuficient input arguments! Check commandlet usage help." ) );
		return false;
	}

	m_outputDir = arguments[ 0 ].ToLower( );
	m_speechResourceDirectory = arguments[ 1 ].ToLower( );
	m_stringsView = arguments[ 2 ].ToUpper( );

	if( options.HasOption( TXT( "languages" ) ) )
	{
		m_languages = options.GetOptionValues( TXT( "languages" ) );
	}
	else
	{
		WARN_WCC( TXT( "Cooking all languages: EN DE FR CZ ES ESMX HU IT RU ZH AR" ) );
		m_languages.PushBack( TXT( "en" ) );
		m_languages.PushBack( TXT( "de" ) );
		m_languages.PushBack( TXT( "fr" ) );
		m_languages.PushBack( TXT( "cz" ) );
		m_languages.PushBack( TXT( "es" ) );
		m_languages.PushBack( TXT( "esmx" ) );
		m_languages.PushBack( TXT( "hu" ) );
		m_languages.PushBack( TXT( "it" ) );
		m_languages.PushBack( TXT( "ru" ) );
		m_languages.PushBack( TXT( "zh" ) );
		m_languages.PushBack( TXT( "ar" ) );
	}

	if( options.HasOption( TXT( "platforms" ) ) )
	{
		m_platforms = options.GetOptionValues( TXT( "platforms" ) );
	}
	else
	{
		WARN_WCC( TXT( "No target platform was specified! Assuming PC." ) );
		m_platforms.PushBack( TXT( "pc" ) );
	}

	m_skipSpeech = options.HasOption( TXT( "skipspeech" ) );

	if( options.HasOption( TXT( "rebuild" ) ) )
	{
		m_rebuild = true;
	}

	return true;
}

Bool CStringCookerCommandlet::CookStrings( const String& language )
{
	THashMap< Uint32, LanguagePack* > localizedPacks;
	Uint32 langId;

	if( SLocalizationManager::GetInstance( ).FindLocaleId( language.ToUpper( ), langId ) == false )
	{
		ERR_WCC( TXT( "Unable to find language: %s" ), language.AsChar( ) );
		return false;
	}

	SLocalizationManager::GetInstance( ).GetPacksToCook( langId, m_settings.m_stringsView, localizedPacks );

	auto lang = language.ToLower( );

	if( !CreateCookedFiles( localizedPacks, lang ) )
	{
		return false;
	}

	Uint32 fileKey = 0, secureKey = 0;
	CookedLocaleKeys::GetSecureKeys( lang, fileKey, secureKey );

	// Clear resources.
	for( auto keyIter = localizedPacks.Begin( ); keyIter != localizedPacks.End( ); ++keyIter )
	{
		auto stringId = keyIter->m_first;
		auto pack = keyIter->m_second;

		stringId ^= secureKey;

		SLocalizationManager::GetInstance( ).ReleaseLanguagePack( stringId );
		SLocalizationManager::GetInstance( ).DeleteLanguagePack( pack );

		keyIter->m_second = NULL;
	}

	SGarbageCollector::GetInstance( ).CollectNow( );

	return true;
}

Bool CStringCookerCommandlet::CreateCookedSpeech( const THashMap<Uint32, LanguagePack*>& packs, const String& lang, const String& platform )
{
	String voiceoverLanguageRoot = String::Printf( TXT( "%s\\%s\\audio\\" ), m_settings.m_speechResourceDirectory.AsChar( ), lang.AsChar( ) );
	if( !GFileManager->FileExist( voiceoverLanguageRoot ) )
	{
		WARN_WCC( TXT( "Language: '%s' has no audio files, will use english." ), lang.AsChar( ) );
		return true;
	}

	String speechPathPostfix;
	if( platform == TXT( "xboxone" ) || platform == TXT( "durango" ) )
	{
		m_voiceoverExtension = TXT( "xma" );
		speechPathPostfix = CLocalizationManager::SPEECH_FILE_PATH_POSTFIX_DURANGO;
	}
	else if( platform == TXT( "ps4" ) || platform == TXT( "orbis" ) )
	{
		m_voiceoverExtension = TXT( "atr" );
		speechPathPostfix = CLocalizationManager::SPEECH_FILE_PATH_POSTFIX_ORBIS;
	}
	else if( platform == TXT( "pc" ) || platform == TXT( "windows" ) )
	{
		m_voiceoverExtension = TXT( "ogg" );
		speechPathPostfix = CLocalizationManager::SPEECH_FILE_PATH_POSTFIX_PC;
	}
	else
	{
		ERR_WCC( TXT( "Unsupported platform: '%s'" ), platform.AsChar( ) );
		return false;
	}

	m_lipsyncExtension = TXT( "re" );

	String speechFileName( String::Printf( TXT( "%s%s%s" ), m_settings.m_outputDir.AsChar( ), lang.AsChar( ), speechPathPostfix.AsChar( ) ) );
	String metaFileName( String::Printf( TXT( "%s.meta" ), speechFileName.AsChar( ) ) );
	String oldSpeechFileName( String::Printf( TXT( "%s.old" ), speechFileName.AsChar( ) ) );
	String tempSpeechFileName( String::Printf( TXT( "%s.temp" ), speechFileName.AsChar( ) ) );

	// Save some stats.
	Uint64 voices = 0;
	Uint64 newVoices = 0;
	Uint64 voicesSize = 0;
	Uint64 newVoicesSize = 0;
	Uint64 lipsync = 0;
	Uint64 newLipsync = 0;
	Uint64 lipsyncSize = 0;
	Uint64 newLipsyncSize = 0;

	TMetaDataMap metaDataMap;
	metaDataMap.Reserve( packs.Size( ) );

	CCookedSpeeches newCookedSpeeches;
	newCookedSpeeches.SetLanguage( lang );

	CCookedSpeeches oldCookedSpeeches;

	// Rename current speech file name as old.
	GFileManager->MoveFile( speechFileName, oldSpeechFileName );

	// Get old speeches file.
	Bool useOldMap = false;
	Red::TScopedPtr< IFile > oldSpeechFile( GFileManager->CreateFileReader( oldSpeechFileName, FOF_AbsolutePath ) );
	if( oldSpeechFile && !m_settings.m_rebuild )
	{
		useOldMap = oldCookedSpeeches.Load( *oldSpeechFile );
	}

	// And old metadata file.
	TMetaDataMap oldMetaDataMap;
	if( useOldMap ) 
	{
		Red::TScopedPtr< IFile > oldMetaDataFile( GFileManager->CreateFileReader( metaFileName, FOF_AbsolutePath ) );
		if( oldMetaDataFile )
			useOldMap = TSpeechMetadataFile::Load( *oldMetaDataFile, oldMetaDataMap );
	}

	// Create temporary speech file.
	Red::TScopedPtr< IFile > tempSpeechFile( GFileManager->CreateFileWriter( tempSpeechFileName, FOF_AbsolutePath ) );
	if( !tempSpeechFile )
	{
		ERR_WCC( TXT( "Error creating temporary file '%s'" ), tempSpeechFileName.AsChar( ) );
		return false;
	}
	tempSpeechFile->SetCooker( true );

	Bool anyChanged = false;

	for( auto keyIter = packs.Begin( ); keyIter != packs.End( ); ++keyIter )
	{
		Uint32 stringId = keyIter->m_first;
		LanguagePack* pack = keyIter->m_second;

		CCookedSpeeches::SOffset speechData;
		TSpeechMetaData metaData = GetMetaData( *pack, lang );

		// No need to process files that no longer exist (wtf!?).
		if( metaData.m_voTime == 0 && metaData.m_lipsyncTime == 0 )
			continue;

		Bool useNewVoiceOverFile = true;
		Bool useNewLipsyncFile = true;
		if( useOldMap )
		{
			TSpeechMetaData oldMetaData;
			CCookedSpeeches::SOffset oldSpeechData;
			if( oldCookedSpeeches.GetOffset( stringId, oldSpeechData ) && oldMetaDataMap.Find( stringId, oldMetaData ) )
			{
				if( oldSpeechData.m_voSize != 0 && oldMetaData.m_voTime != 0 && oldMetaData.m_voTime == metaData.m_voTime ) 
				{
					speechData.m_voSize = oldSpeechData.m_voSize;
					speechData.m_voOffset = tempSpeechFile->GetOffset( );
					oldSpeechFile->CopyToFile( *tempSpeechFile, oldSpeechData.m_voOffset, oldSpeechData.m_voSize );

					++voices;
					voicesSize += speechData.m_voSize;
					useNewVoiceOverFile = false;
				}
				if( oldSpeechData.m_lipsyncSize != 0 && oldMetaData.m_lipsyncTime != 0 && oldMetaData.m_lipsyncTime == metaData.m_lipsyncTime ) 
				{
					speechData.m_lipsyncSize = oldSpeechData.m_lipsyncSize;
					speechData.m_lipsyncOffset = tempSpeechFile->GetOffset( );
					oldSpeechFile->CopyToFile( *tempSpeechFile, oldSpeechData.m_lipsyncOffset, oldSpeechData.m_lipsyncSize );

					++lipsync;
					lipsyncSize += speechData.m_lipsyncSize;
					useNewLipsyncFile = false;
				}
			}
		}
		if( useNewVoiceOverFile && metaData.m_voTime != 0 && !pack->GetVoiceoverFileName( ).Empty( ) )
		{
			FillVoiceover( *pack, lang );
			if( pack->GetSpeechBuffer( ).GetSize( ) > 0 )
			{
				speechData.m_voOffset = tempSpeechFile->GetOffset();

				auto before = tempSpeechFile->GetOffset( );
				pack->GetSpeechBuffer( ).Serialize( *tempSpeechFile );
				auto after = tempSpeechFile->GetOffset( );
				speechData.m_voSize = after - before;

				pack->GetSpeechBuffer( ).Clear( );

				++voices;
				++newVoices;
				voicesSize += after - before;
				newVoicesSize += after - before;

				anyChanged = true;
			}
		}
		if( useNewLipsyncFile && metaData.m_lipsyncTime != 0 && !pack->GetLipsyncFileName( ).Empty( ) )
		{
			FillLipsync( *pack, lang );
			if( pack->GetLipsync( ) != NULL )
			{
				speechData.m_lipsyncOffset = tempSpeechFile->GetOffset( );

				auto before = tempSpeechFile->GetOffset( );

				CSkeletalAnimation* anim = pack->GetLipsync( );
				RED_ASSERT( anim != nullptr );
				anim->DiscardSourceAnimData( );

				DependencySavingContext context( anim );
				CDependencySaver saver( *tempSpeechFile, nullptr );
				if( !saver.SaveObjects( context ) )
				{
					ERR_WCC( TXT( "Error while saving lipsync data!" ) );
					return false;
				}

				auto after = tempSpeechFile->GetOffset() ;
				speechData.m_lipsyncSize = after - before;

				anim = pack->GetLipsync( );
				pack->SetLipsync( NULL );
				delete anim;

				ASSERT( speechData.m_lipsyncOffset >= 0 );
				ASSERT( speechData.m_lipsyncSize >= 0 );

				++lipsync;
				++newLipsync;
				lipsyncSize += after - before;
				newLipsyncSize += after - before;

				anyChanged = true;
			}
		}

		metaDataMap.Insert( stringId, metaData );
		newCookedSpeeches.AddOffset( speechData, stringId );
	}

	// Close file handles.
	oldSpeechFile.Reset( );
	tempSpeechFile.Reset( );

	if( anyChanged )
	{
		// Create new speech file.
		Red::TScopedPtr< IFile > speechFile( GFileManager->CreateFileWriter( speechFileName, FOF_AbsolutePath ) );
		if( !speechFile )
		{
			ERR_WCC( TXT( "Error creating new speech file '%s'!" ), speechFileName.AsChar( ) );
			return false;
		}

		// Reopen temp file as reader.
		Red::TScopedPtr< IFile > tempSpeechFileReader( GFileManager->CreateFileReader( tempSpeechFileName, FOF_AbsolutePath ) );
		if( !tempSpeechFileReader )
		{
			ERR_WCC( TXT( "Error creating new speech file '%s'!" ), speechFileName.AsChar( ) );
			return false;
		}

		// Dump cooked data from temp file into new file.
		newCookedSpeeches.Save( *speechFile, *tempSpeechFileReader );

		// Do this, or else we won't be able to delete them later.
		GFileManager->SetFileReadOnly( metaFileName, false );
		GFileManager->SetFileReadOnly( oldSpeechFileName, false );

		GFileManager->DeleteFile( oldSpeechFileName );

		// Save new metadata file.
		Red::TScopedPtr< IFile > metaDataFile( GFileManager->CreateFileWriter( metaFileName, FOF_AbsolutePath ) );
		if( metaDataFile )
			TSpeechMetadataFile::Save( *metaDataFile, metaDataMap );

		LOG_WCC( TXT( "%s file created." ), speechFileName.AsChar( ) );
	}
	else
	{
		// Nothing changed, rename old file as current.
		GFileManager->MoveFile( oldSpeechFileName, speechFileName );
		LOG_WCC( TXT( "%s has not changed." ), speechFileName.AsChar() );
	}


	// Delete temporary file.
	GFileManager->DeleteFile( tempSpeechFileName );
	
	// Show some stats
	LOG_WCC( TXT( "\tVoices new/all: %d/%d (%.2f/%.2f MB)" ), newVoices, voices, newVoicesSize / 1048576.0f, voicesSize / 1048576.0f );
	LOG_WCC( TXT( "\tLipsync new/all: %d/%d (%.2f/%.2f MB)" ), newLipsync, lipsync, newLipsyncSize / 1048576.0f, lipsyncSize / 1048576.0f );

	return true;
}

Bool CStringCookerCommandlet::CreateCookedStrings( const THashMap<Uint32, LanguagePack*>& packs, const String& lang )
{
	CCookedStrings cookedStrings;
	cookedStrings.SetLanguage( lang );

	const Bool isArabic = lang.EqualsNC( TXT( "ar" ) );
	if ( isArabic )
	{
		const String fontPath = GFileManager->GetDataDirectory( ) + TXT( "tmp\\arial.ttf" );
		m_textEncoder.LoadFont( fontPath );
		m_textEncoder.SelectLanguageScript( CCookerTextEncoder::SCRIPT_Arabic );
		m_textEncoder.SelectFont( TXT( "arial.ttf" ) );
	}

	for( auto keyIter = packs.Begin(); keyIter != packs.End(); ++keyIter )
	{
		Uint32 stringId = keyIter->m_first;
		LanguagePack* pack = keyIter->m_second;

		// Skip invalid entries
		if( pack == NULL )
			continue;

		String packText = pack->GetText( );

		// Don't even bother with empty entries.
		if( packText == String::EMPTY )
			continue;
		
		// FIXME: Refactor into another function so not checking "isArabic" all the time when it's not going to change!!!
		// FIXME: Hardcoding arial.ttf for now. Need to sort out how we're handling other fonts!
		if ( isArabic )
		{
#ifdef _DEBUG
			if ( packText.GetLength() < 1 )
			{
				WARN_WCC(TXT("Arabic stringId '%u' has empty packText"), stringId );
			}
#endif
			String outText;
			if ( m_textEncoder.ShapeText( packText, outText ) )
			{
				// Put text back into logical (vs presentation) order so Scaleform can reverse it more fine grained using its bidi alg
				// and not break HTML tags
				m_textEncoder.ReverseTextInPlace( outText );
				packText = Move( outText );
#ifdef _DEBUG
				for ( Uint32 i = 0; i < packText.GetLength( ); ++i )
				{
					if ( packText[ i ] == 0xFFFF )
					{
						WARN_WCC( TXT( "Arabic stringId '%u' has invalid char at index '%u': packText='%ls'. Original packText='%ls'" ), stringId, i, packText.AsChar( ), pack->GetText( ).AsChar( ) );
						break;
					}
				}
#endif
			}
			else
			{
				ERR_WCC( TXT( "Failed to shape Arabic stringId '%u'" ), stringId );
			}
		}

		// If has a key, inset into key map
		if( !pack->GetStringKey( ).Empty( ) )
		{
			cookedStrings.AddKey( pack->GetStringKey( ).ToLower( ), stringId );
		}

		// Insert text into strings map
		cookedStrings.AddString( packText, stringId );
	}

	String stringsFileName( String::Printf( TXT( "%s%s%s" ), m_settings.m_outputDir.AsChar( ), lang.AsChar( ), CLocalizationManager::STRINGS_FILE_PATH_POSTFIX.AsChar( ) ) );

	// Remove read only flag, so if it exists we can overwrite it.
	GFileManager->SetFileReadOnly( stringsFileName, false );

	// Create strings file.
	Red::TScopedPtr< IFile > stringsFile( GFileManager->CreateFileWriter( stringsFileName, FOF_AbsolutePath ) );
	if( !stringsFile )
	{
		ERR_WCC( TXT( "Error creating file '%s'!" ), stringsFileName.AsChar( ) );
		return false;
	}

	cookedStrings.Save( *stringsFile );

	LOG_WCC( TXT( "%s file created." ), stringsFileName.AsChar( ) );

	return true;
}

Bool CStringCookerCommandlet::CreateCookedFiles( THashMap<Uint32, LanguagePack*>& packs, const String& lang )
{
	GSystemIO.CreateDirectory( m_settings.m_outputDir.AsChar() );

	LOG_WCC( TXT( "Cooking '%s' strings..." ), lang.AsChar( ) );

	if( !CreateCookedStrings( packs, lang ) )
	{
		return false;
	}

	LOG_WCC( TXT( "Done cooking '%s' strings!" ), lang.AsChar( ) );

	if( m_settings.m_skipSpeech )
	{
		return true;
	}

	LOG_WCC( TXT( "Cooking '%s' speech..." ), lang.AsChar( ) );

	for( auto platformsIt = m_settings.m_platforms.Begin( ); platformsIt != m_settings.m_platforms.End( ); platformsIt++ )
	{
		SGarbageCollector::GetInstance( ).CollectNow( );
		if( !CreateCookedSpeech( packs, lang, ( *platformsIt ).ToLower( ) ) )
		{
			return false;
		}
	}

	LOG_WCC( TXT( "Done cooking '%s' speech!" ), lang.AsChar( ) );

	return true;
}

Bool CStringCookerCommandlet::FillVoiceover( LanguagePack& pack, const String& language )
{
	pack.GetSpeechBuffer( ).Clear( );

	String voiceoverPath = GetVoiceoverPath( language, pack.GetVoiceoverFileName( ) );

	return pack.GetSpeechBuffer( ).LoadFromFile( voiceoverPath );
}

Bool CStringCookerCommandlet::FillLipsync( LanguagePack& pack, const String& language )
{
	pack.SetLipsync( NULL );

	String lipsyncPath = GetLipsyncPath( language, pack.GetVoiceoverFileName( ) );

	CFilePath filePath( lipsyncPath );

	CSkeletalAnimation* lipsyncAnimation = NULL;

	ISkeletalAnimationImporter* importer = ISkeletalAnimationImporter::FindImporter( filePath.GetExtension( ) );
	if( importer )
	{
		AnimImporterParams params;
		params.m_filePath = lipsyncPath;
		params.m_preferBetterQuality = true;

		CSkeletalAnimation* lipsyncAnimation = importer->DoImport( params );
		pack.SetLipsync( lipsyncAnimation );
		return lipsyncAnimation != NULL;
	}

	return false;
}

CStringCookerCommandlet::TSpeechMetaData CStringCookerCommandlet::GetMetaData( const LanguagePack& pack, const String& lang )
{
	TSpeechMetaData metaData;
	metaData.m_voTime = metaData.m_lipsyncTime = 0;
	if( !pack.GetVoiceoverFileName( ).Empty( ) )
	{
		metaData.m_voTime = GFileManager->GetFileTime( GetVoiceoverPath( lang, pack.GetVoiceoverFileName( ) ).AsChar( ) ).GetRaw( );
	}
	if( !pack.GetLipsyncFileName( ).Empty( ) ) 
	{
		metaData.m_lipsyncTime = GFileManager->GetFileTime( GetLipsyncPath( lang, pack.GetVoiceoverFileName( ) ).AsChar( ) ).GetRaw( );
	}
	return metaData;
}