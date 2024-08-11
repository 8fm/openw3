#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/engine/cookedStrings.h"
#include "../../common/engine/cookedSpeeches.h"
#include "cookDataBase.h"
#include "cookSplitList.h"
#include "jsonFileHelper.h"

class CStringSplitterCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CStringSplitterCommandlet, ICommandlet, 0 );

public:

	CStringSplitterCommandlet( )
	{
		m_commandletName = CName( TXT( "splitstrings" ) );
	}

	virtual bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner( ) const;
	virtual void PrintHelp( ) const;

private:

	struct Settings
	{
		enum ETask
		{
			Task_DoStrings	= FLAG( 0 ),
			Task_DoSpeech	= FLAG( 1 ),
			Task_DoAll		= Task_DoStrings | Task_DoSpeech
		};

		String	m_splitSeedFilePath;
		String	m_idsFilePath;
		String	m_keysFilePath;
		String	m_inDir;
		String	m_outDir;
		Uint32	m_tasks;
		Bool	m_allincontent0;
		Bool	m_allinsamefolder;
		Bool	m_dlcMode;

		TList< String > m_languages;
		TList< String > m_platforms;

		Settings( )
			: m_splitSeedFilePath( TXT( "" ) )
			, m_idsFilePath( TXT( "" ) )
			, m_keysFilePath( TXT( "" ) )
			, m_inDir( TXT( "" ) )
			, m_outDir( TXT( "" ) )
			, m_tasks( 0 )
			, m_allincontent0( false )
			, m_allinsamefolder( false )
			, m_dlcMode( false )
		{ }

		Bool Parse( const ICommandlet::CommandletOptions& options );
	};

	struct LanguagePackage
	{
		CCookedStrings	m_cookedStrings;
		CCookedSpeeches	m_cookedSpeeches;

		THashSet< Uint32 > keysSet;
		THashSet< Uint32 > stringsSet;
		THashSet< Uint32 > speechSet;

		LanguagePackage( ) { }

		LanguagePackage( String lang )
		{
			m_cookedStrings.SetLanguage( lang );
			m_cookedSpeeches.SetLanguage( lang );
		}

		Bool LoadStringsFile( String filePath );
		Bool WriteStringsFile( String srcFilePath, String filePath );

		Bool LoadSpeechFile( String filePath );
		Bool WriteSpeechFile( String srcFilePath, String filePath );

		// Entries are stored encrypted.
		Bool AddKey( Uint32 keyId, Uint32 stringId );
		Bool AddString( const String& string, Uint32 stringId );
		Bool AddSpeech( CCookedSpeeches::SOffset speechOffset, Uint32 speechId );

		void DumpAllStrings( String outFilePath ) const;
		void DumpAllSpeech( String outFilePath ) const;
	};

private:

	typedef TDynArray< Uint32 > TStringIds;
	typedef THashMap< CCookerSplitFileEntry::TChunkID, TStringIds > TChunkStringIds;

	typedef TDynArray< String > TStringKeys;
	typedef THashMap< CCookerSplitFileEntry::TChunkID, TStringKeys > TChunkStringKeys;

	Settings			m_settings;
	CCookerSplitFile	m_splitSeedFile;
	TChunkStringIds		m_chunkStringIds;
	TChunkStringKeys	m_chunkStringKeys;

private:

	Bool LoadSplitFile( );
	Bool LoadIdsFile( );
	Bool LoadKeysFile( );

	void ExecuteTasks( );

	Bool SplitStringFile( const String& language ) const;
	Bool SplitSpeechFile( const String& language, const String& platform ) const;

	void BuildSplitFilePath( String& outPath, String contentName, String language, String platform, String extension ) const;
};

BEGIN_CLASS_RTTI( CStringSplitterCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI( )

IMPLEMENT_ENGINE_CLASS( CStringSplitterCommandlet );

bool CStringSplitterCommandlet::Execute( const CommandletOptions& options )
{
	// Parse options.
	if ( !m_settings.Parse( options ) )
		return false;

	// Load the split file.
	if( !LoadSplitFile( ) )
		return false;

	// Load the ids file.
	if( !LoadIdsFile( ) )
		return false;

	if( !m_settings.m_keysFilePath.Empty() )
	{
		// Load the keys file.
		if( !LoadKeysFile() )
			return false;
	}
	
	// Ready! Steady! Go!
	ExecuteTasks( );

	// Done!
	return true;
}

const Char* CStringSplitterCommandlet::GetOneLiner( ) const
{
	return TXT( "Splits strings and speech files for a given language based on JSON descriptor." );
}

void CStringSplitterCommandlet::PrintHelp( ) const
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  splitstrings [-dlc] -splitfile=<filepath> -idsfile=<filepath> -keysfile=<filepath> -indir=<dirpath> -outdir=<dirpath> <options>" ) );
	LOG_WCC( TXT( "    -dlc           - Run in the DLC mode (no per-chunk splitting)" ) );
	LOG_WCC( TXT( "    -splitfile     - Path to the split file (mapping resources to chunks, not required in DLC mode)." ) );
	LOG_WCC( TXT( "    -idsfile       - Path to the ids file (mapping resources to string ID's).") );
	LOG_WCC( TXT( "    -keysfile      - Path to the keys file [optional](mapping resources to string Key's).") );
	LOG_WCC( TXT( "    -indir         - Path to the directory where the language files reside." ) );
	LOG_WCC( TXT( "    -outdir        - Path to the directory where the split files will reside." ) );
	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "Options:" ) );
	LOG_WCC( TXT( "  -dostrings       - Split strings only (everything is done by default)." ) );
	LOG_WCC( TXT( "  -dospeech        - Split speech only (everything is done by default)." ) );
	LOG_WCC( TXT( "  -language <lg>   - Add language <lg> to the splitting plan (default is 'en')." ) );
	LOG_WCC( TXT( "  -platform <pf>   - Add platform <pf> to the splitting plan (default is 'pc')." ) );
	LOG_WCC( TXT( "  -allincontent0   - Merges all chunks into content0 (debug option)." ) );
	LOG_WCC( TXT( "  -allinsamefolder - Outputs all files in the same folder (debug option)." ) );
}

Bool CStringSplitterCommandlet::Settings::Parse( const ICommandlet::CommandletOptions& options )
{
	// Get cook database file path, not used in DLC mode
	if ( options.HasOption( TXT("dlc") ) )
	{
		LOG_WCC( TXT( "The string splitter will work in DLC mode (trim only)" ) );
		m_dlcMode = true;
	}
	else if( !options.GetSingleOptionValue( TXT( "splitfile" ), m_splitSeedFilePath ) )
	{
		ERR_WCC( TXT( "No split file path has been specified!" ) );
		return false;
	}

	// Get source resources file path.
	if( !options.GetSingleOptionValue( TXT( "idsfile" ), m_idsFilePath ) )
	{
		ERR_WCC( TXT( "No ids file path has been specified!" ) );
		return false;
	}

	// Get source resources file path.
	options.GetSingleOptionValue( TXT( "keysfile" ), m_keysFilePath );

	// Get source strings path.
	if( !options.GetSingleOptionValue( TXT( "indir" ), m_inDir ) )
	{
		ERR_WCC( TXT( "No source string files input directory has been specified!" ) );
		return false;
	}

	// Get out path.
	if( !options.GetSingleOptionValue( TXT( "outdir" ), m_outDir ) )
	{
		ERR_WCC( TXT( "No split string files output directory has been specified!" ) );
		return false;
	}

	m_tasks = 0;

	// Get task flags.
	if( options.HasOption( TXT( "dostrings" ) ) )
		m_tasks |= Task_DoStrings;
	if( options.HasOption( TXT( "dospeech" ) ) )
		m_tasks |= Task_DoSpeech;
	if( m_tasks == 0 )
		m_tasks = Task_DoAll;

	if( options.HasOption( TXT( "allincontent0" ) ) )
		m_allincontent0 = true;
	if( options.HasOption( TXT( "allinsamefolder" ) ) )
		m_allinsamefolder = true;

	// Get languages.
	if( options.HasOption( TXT( "language" ) ) )
		m_languages = options.GetOptionValues( TXT( "language" ) );
	if( m_languages.Empty( ) )
	{
		WARN_WCC( TXT( "No languages have been specified! Only 'en' will be processed." ) );
		m_languages.PushBack( TXT( "en" ) );
	}

	// Get platforms.
	if( options.HasOption( TXT( "platform" ) ) )
		m_platforms = options.GetOptionValues( TXT( "platform" ) );
	if( m_platforms.Empty( ) )
	{
		WARN_WCC( TXT( "No platforms have been specified! Only 'pc' will be processed." ) );
		m_platforms.PushBack( TXT( "pc" ) );
	}

	return true;
}

Bool CStringSplitterCommandlet::LanguagePackage::LoadStringsFile( String filePath )
{
	// Create strings file reader.
	Red::TScopedPtr< IFile > stringsFile( GFileManager->CreateFileReader( filePath, FOF_AbsolutePath ) );
	if( !stringsFile )
	{
		ERR_WCC( TXT( "Error creating file reader '%s'!" ), filePath.AsChar( ) );
		return false;
	}

	m_cookedStrings.Load( *stringsFile );

	//DumpAllStrings( TXT( "z:\\in_strings_dump.json" ) );

	return true;
}

Bool CStringSplitterCommandlet::LanguagePackage::WriteStringsFile( String srcFilePath, String filePath )
{
	//DumpAllStrings( TXT( "z:\\out_strings_dump.json" ) );

	// Create strings file writer.
	Red::TScopedPtr< IFile > stringsFile( GFileManager->CreateFileWriter( filePath, FOF_AbsolutePath ) );
	if( !stringsFile )
	{
		ERR_WCC( TXT( "Error creating file writer '%s'!" ), filePath.AsChar( ) );
		return false;
	}

	m_cookedStrings.Save( *stringsFile );

	return true;
}

Bool CStringSplitterCommandlet::LanguagePackage::LoadSpeechFile( String filePath )
{
	// Create speech file reader.
	Red::TScopedPtr< IFile > speechFile( GFileManager->CreateFileReader( filePath, FOF_AbsolutePath ) );
	if( !speechFile )
	{
		ERR_WCC( TXT( "Error creating file reader '%s'!" ), filePath.AsChar( ) );
		return false;
	}

	m_cookedSpeeches.Load( *speechFile );

	//DumpAllSpeech( TXT( "z:\\in_speech_dump.json" ) );

	return true;
}

Bool CStringSplitterCommandlet::LanguagePackage::WriteSpeechFile( String srcFilePath, String filePath )
{
	//DumpAllSpeech( TXT( "z:\\in_speech_dump.json" ) );

	// Create speech file reader.
	Red::TScopedPtr< IFile > srcFile( GFileManager->CreateFileReader( srcFilePath, FOF_AbsolutePath ) );
	if( !srcFile )
	{
		ERR_WCC( TXT( "Error creating file reader '%s'!" ), srcFilePath.AsChar( ) );
		return false;
	}

	// Create speech file writer.
	Red::TScopedPtr< IFile > speechFile( GFileManager->CreateFileWriter( filePath, FOF_AbsolutePath ) );
	if( !speechFile )
	{
		ERR_WCC( TXT( "Error creating file writer '%s'!" ), filePath.AsChar( ) );
		return false;
	}

	m_cookedSpeeches.Save( *speechFile, *srcFile );

	return true;
}

Bool CStringSplitterCommandlet::LanguagePackage::AddKey( Uint32 keyId, Uint32 stringId )
{
	if( !keysSet.Insert( keyId ) )
		return false;

	m_cookedStrings.AddKey( keyId, stringId );

	return true;
}

Bool CStringSplitterCommandlet::LanguagePackage::AddString( const String& string, Uint32 stringId )
{
	if( !stringsSet.Insert( stringId ) )
		return false;

	m_cookedStrings.AddString( string, stringId );

	return true;
}

Bool CStringSplitterCommandlet::LanguagePackage::AddSpeech( CCookedSpeeches::SOffset speechOffset, Uint32 speechId )
{
	if( !speechSet.Insert( speechId ) )
		return false;

	m_cookedSpeeches.AddOffset( speechOffset, speechId );

	return true;
}

void CStringSplitterCommandlet::LanguagePackage::DumpAllStrings( String outFilePath ) const
{
	IFile* outputFile = GFileManager->CreateFileWriter( outFilePath, FOF_AbsolutePath );
	if( outputFile == nullptr )
	{
		ERR_WCC( TXT( "Error creating file writer '%s'!" ), outFilePath.AsChar( ) );
		return;
	}

	// Scoped JSONStreamWriter (needs to be destroyed before IFile is deleted).
	{
		typedef JSONFileHelper::JSONStreamWriter< 4096 > TFileStream;

		TFileStream fileStream( outputFile );
		rapidjson::PrettyWriter< TFileStream > writer( fileStream );

		writer.StartObject( );
		{
			writer.String( "strings" );
			writer.Uint( m_cookedStrings.m_offsetMap.Size( ) );
			writer.String( "entries" );
			writer.StartArray( );

			for( CCookedStrings::TOffsetMap::const_iterator it = m_cookedStrings.m_offsetMap.Begin( ); it != m_cookedStrings.m_offsetMap.End( ); ++it )
			{
				String decryptedText;
				m_cookedStrings.GetString( it->m_first ^ m_cookedStrings.m_langKey, decryptedText );

				writer.StartObject( );
				writer.Uint( it->m_first ^ m_cookedStrings.m_langKey );
				writer.String( UNICODE_TO_ANSI( decryptedText.AsChar( ) ) );
				writer.EndObject( );
			}
			writer.EndArray( );
		}
		writer.EndObject( );
		writer.StartObject( );
		{
			writer.String( "keys" );
			writer.Uint( m_cookedStrings.m_keysMap.Size( ) );
			writer.String( "entries" );
			writer.StartArray( );

			for( CCookedStrings::TKeysMap::const_iterator it = m_cookedStrings.m_keysMap.Begin( ); it != m_cookedStrings.m_keysMap.End( ); ++it )
			{
				Uint32 decryptedID( it->m_second ^ m_cookedStrings.m_langKey );

				writer.StartObject( );
				writer.Uint( it->m_first );
				writer.Uint( decryptedID );
				writer.EndObject( );
			}
			writer.EndArray( );
		}
		writer.EndObject( );
	}

	delete outputFile;
}

void CStringSplitterCommandlet::LanguagePackage::DumpAllSpeech( String outFilePath ) const
{
	IFile* outputFile = GFileManager->CreateFileWriter( outFilePath, FOF_AbsolutePath );
	if( outputFile == nullptr )
	{
		ERR_WCC( TXT( "Error creating file writer '%s'!" ), outFilePath.AsChar( ) );
		return;
	}

	// Scoped JSONStreamWriter (needs to be destroyed before IFile is deleted).
	{
		typedef JSONFileHelper::JSONStreamWriter< 4096 > TFileStream;

		TFileStream fileStream( outputFile );
		rapidjson::PrettyWriter< TFileStream > writer( fileStream );

		writer.StartObject( );
		{
			writer.String( "speech_entries" );
			writer.Uint( m_cookedSpeeches.m_offsetMap.Size( ) );
			writer.String( "entries" );
			writer.StartArray( );

			for( CCookedSpeeches::TOffsetMap::const_iterator it = m_cookedSpeeches.m_offsetMap.Begin( ); it != m_cookedSpeeches.m_offsetMap.End( ); ++it )
			{
				Uint32 decryptedID( it->m_first ^ m_cookedSpeeches.m_langKey );
				const CCookedSpeeches::SOffset& data = it->m_second;
				writer.StartObject( );
				writer.String( "id" );
				writer.Uint( decryptedID );
				writer.String( "vo_offset" );
				writer.Uint64( data.m_voOffset );
				writer.String( "vo_size" );
				writer.Uint64( data.m_voSize );
				writer.String( "lip_offset" );
				writer.Uint64( data.m_lipsyncOffset );
				writer.String( "lip_size" );
				writer.Uint64( data.m_lipsyncSize );
				writer.EndObject( );
			}

			writer.EndArray( );
		}
		writer.EndObject( );
	}

	delete outputFile;
}

Bool CStringSplitterCommandlet::LoadSplitFile( )
{
	if ( !m_settings.m_dlcMode )
	{
		if ( !m_splitSeedFile.LoadFromFile( m_settings.m_splitSeedFilePath ) )
		{
			ERR_WCC( TXT( "Failed to load split file '%ls'!" ), m_settings.m_splitSeedFilePath.AsChar( ) );
			return false;
		}
	}

	return true;
}

Bool CStringSplitterCommandlet::LoadIdsFile( )
{
	// Load ids data json file into a memory buffer.
	TDynArray< Uint8 > resourcesFileData;
	if( !GFileManager->LoadFileToBuffer( m_settings.m_idsFilePath, resourcesFileData, true ) )
	{
		ERR_WCC( TXT( "Failed to load resources data from file '%ls'!" ), m_settings.m_idsFilePath.AsChar( ) );
		return false;
	}

	// Parse resources data json file.
	JSONFileHelper::JSONDocument jsonDocument;
	jsonDocument.Parse< 0 >( static_cast< AnsiChar* >( resourcesFileData.Data( ) ) );
	if( jsonDocument.HasParseError( ) )
	{
		ERR_WCC( TXT( "Failed to parse resources data from file '%ls'! Error: %s" ), m_settings.m_idsFilePath.AsChar( ), jsonDocument.GetParseError( ) );
		return false;
	}

	// Grab the whole database.
	const JSONFileHelper::JSONValue& jsonDataBase = jsonDocument[ "files" ];
	const rapidjson::SizeType dataBaseFileCount = jsonDataBase.Size( );

	// Build a database of string IDs for each chunk.
	for( rapidjson::SizeType dbEntryIdx = 0; dbEntryIdx < dataBaseFileCount; ++dbEntryIdx )
	{
		const JSONFileHelper::JSONValue& fileEntry = jsonDataBase[ dbEntryIdx ];
		StringAnsi entryPath = JSONFileHelper::GetAttrStr( fileEntry, "path" ).AsChar( );
		CCookerSplitFileEntry::TChunkID chunkID = CName( TXT( "content0" ) );

		// We need content splitting only in base mode
		if ( !m_settings.m_dlcMode )
		{
			// Check the file exists in the cooked database (should this be an assert?).
			const CCookerSplitFileEntry* seedEntry = m_splitSeedFile.GetEntry( entryPath );
			if( seedEntry == nullptr )
			{
				WARN_WCC( TXT( "Cannot find file '%s' in split file! Defaulting to '%s'." ), ANSI_TO_UNICODE( entryPath.AsChar( ) ), chunkID.AsChar( ) );
			}

			// Get its chunkID.
			if( seedEntry != nullptr && !m_settings.m_allincontent0 )
			{
				chunkID = seedEntry->GetFileChunkID( );
			}
		}

		TStringIds& stringIds = m_chunkStringIds.GetRef( chunkID );

		// Process each string id the current entry has.
		const JSONFileHelper::JSONValue& stringIdsArray = fileEntry[ "string_ids" ];
		if( stringIdsArray.IsArray( ) )
		{
			const Uint32 stringIdsArraySize = stringIdsArray.Size();
			for( rapidjson::SizeType i = 0; i < stringIdsArraySize; ++i )
			{
				Uint32 stringId = stringIdsArray[ i ].GetUint( );
				if( static_cast< Int32 >( stringId ) <= 0 )
				{
					WARN_WCC( TXT( "Ignoring unstable string %d from resource '%s'!" ), static_cast< Int32 >( stringId ), ANSI_TO_UNICODE( entryPath.AsChar( ) ) );
					continue;
				}

				// Add this non-encrypted string ID on each chunk in which current file is included.
				stringIds.PushBackUnique( stringId );
			}
		}
	}

	return true;
}

Bool CStringSplitterCommandlet::LoadKeysFile( )
{
	// Load keys data json file into a memory buffer.
	TDynArray< Uint8 > resourcesFileData;
	if( !GFileManager->LoadFileToBuffer( m_settings.m_keysFilePath, resourcesFileData, true ) )
	{
		ERR_WCC( TXT( "Failed to load resources data from file '%ls'!" ), m_settings.m_idsFilePath.AsChar( ) );
		return false;
	}

	// Parse resources data json file.
	JSONFileHelper::JSONDocument jsonDocument;
	jsonDocument.Parse< 0 >( static_cast< AnsiChar* >( resourcesFileData.Data( ) ) );
	if( jsonDocument.HasParseError( ) )
	{
		ERR_WCC( TXT( "Failed to parse resources data from file '%ls'! Error: %s" ), m_settings.m_idsFilePath.AsChar( ), jsonDocument.GetParseError( ) );
		return false;
	}

	// Grab the whole database.
	const JSONFileHelper::JSONValue& jsonDataBase = jsonDocument[ "files" ];
	const rapidjson::SizeType dataBaseFileCount = jsonDataBase.Size( );

	// Build a database of string IDs for each chunk.
	for( rapidjson::SizeType dbEntryIdx = 0; dbEntryIdx < dataBaseFileCount; ++dbEntryIdx )
	{
		const JSONFileHelper::JSONValue& fileEntry = jsonDataBase[ dbEntryIdx ];
		StringAnsi entryPath = JSONFileHelper::GetAttrStr( fileEntry, "path" ).AsChar( );
		CCookerSplitFileEntry::TChunkID chunkID = CName( TXT( "content0" ) );

		// We need content splitting only in base mode
		if ( !m_settings.m_dlcMode )
		{
			// Check the file exists in the cooked database (should this be an assert?).
			const CCookerSplitFileEntry* seedEntry = m_splitSeedFile.GetEntry( entryPath );
			if( seedEntry == nullptr )
			{
				WARN_WCC( TXT( "Cannot find file '%s' in split file! Defaulting to '%s'." ), ANSI_TO_UNICODE( entryPath.AsChar( ) ), chunkID.AsChar( ) );
			}

			// Get its chunkID.
			if( seedEntry != nullptr && !m_settings.m_allincontent0 )
			{
				chunkID = seedEntry->GetFileChunkID( );
			}
		}

		if( !m_chunkStringIds.KeyExist( chunkID ) )
		{
			TStringIds tmpArray;
			m_chunkStringIds.Set(chunkID,tmpArray);
		}

		TStringKeys& stringkeys = m_chunkStringKeys.GetRef( chunkID );

		// Process each string id the current entry has.
		const JSONFileHelper::JSONValue& stringKeysArray = fileEntry[ "string_keys" ];
		if( stringKeysArray.IsArray( ) )
		{
			const Uint32 stringKeysArraySize = stringKeysArray.Size();
			for( rapidjson::SizeType i = 0; i < stringKeysArraySize; ++i )
			{
				String stringKey = ANSI_TO_UNICODE( stringKeysArray[ i ].GetString( ) );
				if( stringKey.Empty() )
				{
					continue;
				}

				// Add this non-encrypted string key on each chunk in which current file is included.
				stringkeys.PushBackUnique( stringKey );
			}
		}
	}

	return true;
}

void CStringSplitterCommandlet::ExecuteTasks( )
{
	CTimeCounter processTimer;
	LOG_WCC( TXT( "Launching splitting tasks.." ) );

	// Just for statistics.
	Uint32 currLang = 0;

	// Split requested languages using the common database. This should be parallelized.
	for( TList< String >::iterator langIt = m_settings.m_languages.Begin( ); langIt != m_settings.m_languages.End( ); ++langIt )
	{
		const String& lang = *langIt;

		CTimeCounter languageTimer;
		LOG_WCC( TXT( "Processing '%s' (%d/%d) tasks.." ), lang.AsChar( ), currLang + 1, m_settings.m_languages.Size( ) );
		++currLang;

		if( m_settings.m_tasks & Settings::Task_DoStrings )
		{
			CTimeCounter timer;
			LOG_WCC( TXT( "Splitting '%s' strings.." ), lang.AsChar( ) );
			SplitStringFile( lang );
			LOG_WCC( TXT( "Splitting '%s' strings took %.2f seconds!" ), lang.AsChar( ), timer.GetTimePeriod( ) );
		}

		if( m_settings.m_tasks & Settings::Task_DoSpeech )
		{
			for( TList< String >::iterator platIt = m_settings.m_platforms.Begin( ); platIt != m_settings.m_platforms.End( ); ++platIt )
			{
				const String& plat = *platIt;

				CTimeCounter timer;
				LOG_WCC( TXT( "Splitting '%s%s' speech.." ), lang.AsChar( ), plat.AsChar( ) );
				SplitSpeechFile( lang, plat );
				LOG_WCC( TXT( "Splitting '%s%s' speech took %.2f seconds!" ), lang.AsChar( ), plat.AsChar( ), timer.GetTimePeriod( ) );
			}
		}

		LOG_WCC( TXT( "Splitting tasks for '%s' took %.2f seconds!" ), lang.AsChar( ), languageTimer.GetTimePeriod( ) );
	}
	LOG_WCC( TXT( "Splitting tasks took %.2f seconds!" ), processTimer.GetTimePeriod( ) );
}

Bool CStringSplitterCommandlet::SplitStringFile( const String& language ) const
{
	const CName chunkContent0( TXT( "content0" ) );

	String stringsFileName = String::Printf( TXT( "%s%s.w3strings" ), m_settings.m_inDir.AsChar( ), language.AsChar( ) );

	LanguagePackage langPak;

	// Load strings file.
	if( !langPak.LoadStringsFile( stringsFileName ) )
	{
		LOG_WCC( TXT( "Cannot load strings file '%s'!" ), stringsFileName.AsChar( ) );
		return false;
	}

	CCookedStrings& strings = langPak.m_cookedStrings;

	// Build an "entry usage" table (just for DEBUG purposes).
	THashMap< Uint32, Uint32 > entryUsageMap( strings.m_offsetMap.Size( ) );
	for( CCookedStrings::TOffsetMap::const_iterator it = strings.m_offsetMap.Begin( ); it != strings.m_offsetMap.End( ); ++it )
		entryUsageMap.GetRef( it->m_first ^ strings.m_langKey ) = 1;

	THashMap< CName, TDynArray< CCookedStrings::TKeyId > > keyIdsMap;

	for( CCookedStrings::TKeysMap::const_iterator keyIt = strings.m_keysMap.Begin( ); keyIt != strings.m_keysMap.End( ); ++keyIt )
	{
		const CCookedStrings::TKeyId&  keyId = keyIt->m_first;

		Bool foundKeyIdInChunk = false;
		for( TChunkStringKeys::const_iterator chunkIt = m_chunkStringKeys.Begin( ); chunkIt != m_chunkStringKeys.End( ); ++chunkIt )
		{
			const CName& chunkName = chunkIt->m_first;

			for( TStringKeys::const_iterator stringKeyIt = chunkIt->m_second.Begin( ); stringKeyIt != chunkIt->m_second.End( ); ++stringKeyIt )
			{
				const String& stringKeyInChunkArray = *stringKeyIt;
				
				if( keyId == stringKeyInChunkArray.CalcHash( ) )
				{
					TDynArray< CCookedStrings::TKeyId >* keyIdsArray =  keyIdsMap.FindPtr( chunkName );
					if( keyIdsArray )
					{
						keyIdsArray->PushBackUnique( keyId );
					}
					else
					{
						TDynArray< CCookedStrings::TKeyId > keyIdsArray;
						keyIdsArray.PushBack( keyId );
						keyIdsMap.Set( chunkName, keyIdsArray );
					}
					foundKeyIdInChunk = true;
				}
			}
		}
		if( !m_settings.m_dlcMode ) //! we do not include default string keys to dlc (only string keys from -keysfile)
		{
			if( foundKeyIdInChunk == false )
			{
				TDynArray< CCookedStrings::TKeyId >* keyIdsArray =  keyIdsMap.FindPtr( chunkContent0 );
				if( keyIdsArray )
				{
					keyIdsArray->PushBackUnique( keyId );
				}
				else
				{
					TDynArray< CCookedStrings::TKeyId > keyIdsArray;
					keyIdsArray.PushBack( keyId );
					keyIdsMap.Set( chunkContent0, keyIdsArray );
				}
			}
		}
	}

	// Just for statistics.
	Uint32 currentChunkID = 0;

	// So now, build each string file. This could have been done on-the-fly when analyzing
	// the databases, but the analysis is super fast and the code is more clear this way.
	for( TChunkStringIds::const_iterator it = m_chunkStringIds.Begin( ); it != m_chunkStringIds.End( ); ++it )
	{
		CTimeCounter timer;
		LOG_WCC( TXT( "Processing chunk %d/%d (%d entries).." ), currentChunkID + 1, m_chunkStringIds.Size( ), it->m_second.Size( ) );
		++currentChunkID;

		LanguagePackage newLangPak( language );

		// Populate the new strings file data tables. Very expensive (mainly due to the SORTED hash).
		const TStringIds& stringIds = it->m_second;
		for( Uint32 stringId : stringIds )
		{
			String string = String::EMPTY;
			if( !strings.GetString( stringId, string ) )
			{
				// This might be due to debug LocalizedStrings being cooked, or no longer used entries in the DB.
				//WARN_WCC( TXT( "Cannot find id %d in database!" ), stringId );
				continue;
			}
			newLangPak.AddString( string, stringId );
			CCookedStrings::TKeyId keyId;
			if( strings.GetKeyByStringId( stringId, keyId ) )
			{
				newLangPak.AddKey( keyId, stringId );
			}

			// Mark this entry as used.
			entryUsageMap.GetRef( stringId ) = 0;
		}

		TDynArray< CCookedStrings::TKeyId >* keyIdsArray = keyIdsMap.FindPtr( it->m_first );
		if( keyIdsArray )
		{
			TDynArray< CCookedStrings::TKeyId >::const_iterator end = keyIdsArray->End();
			for( TDynArray< CCookedStrings::TKeyId >::const_iterator keyIt = keyIdsArray->Begin( ); keyIt != end; ++keyIt )
			{
				CCookedStrings::TStringId stringId = 0;
				strings.GetStringIdByKey( *keyIt, stringId );
				String string = String::EMPTY;
				if( !strings.GetString( stringId, string ) )
				{
					// This should be clearly an assert! All keys MUST match a string entry.
					WARN_WCC( TXT( "Cannot find string id %d in database!" ), stringId );
					continue;
				}
				newLangPak.AddString( string, stringId );
				newLangPak.AddKey( *keyIt, stringId );

				// Mark this entry as used.
				entryUsageMap.GetRef( stringId ) = 0;
			}
		}

		String newStringsFilePath;
		BuildSplitFilePath( newStringsFilePath, it->m_first.AsString( ), language, TXT( "" ), TXT( "w3strings" ) );

		// There we go.
		LOG_WCC( TXT( "Writting file '%s'" ), newStringsFilePath.AsChar( ) );
		newLangPak.WriteStringsFile( stringsFileName, newStringsFilePath );

		LOG_WCC( TXT( "Chunk file generated in %.2f seconds!" ), timer.GetTimePeriod( ) );
	}

	// Report entry usage.
	Uint32 totalDiscarded = 0;
	for( THashMap< Uint32, Uint32 >::const_iterator it = entryUsageMap.Begin( ); it != entryUsageMap.End( ); ++it )
	{
		if( it->m_second == 1 )
			++totalDiscarded;
	}
	Float discardRatio = static_cast< Float >( totalDiscarded ) / static_cast< Float >( entryUsageMap.Size( ) );
	LOG_WCC( TXT( "Discarded %d out of %d string entries (%.1f%%)!" ), totalDiscarded, entryUsageMap.Size( ), discardRatio * 100.0f );

	// Done!
	return true;
}

Bool CStringSplitterCommandlet::SplitSpeechFile( const String& language, const String& platform ) const
{
	String stringsFileName = String::Printf( TXT( "%s%s.w3strings" ), m_settings.m_inDir.AsChar( ), language.AsChar( ) );
	String speechFileName = String::Printf( TXT( "%s%s%s.w3speech" ), m_settings.m_inDir.AsChar( ), language.AsChar( ), platform.AsChar( ) );

	LanguagePackage langPak;

	// Load strings file (only needed for content0 key entries).
	if( !langPak.LoadStringsFile( stringsFileName ) )
	{
		LOG_WCC( TXT( "Cannot load strings file '%s'!" ), stringsFileName.AsChar( ) );
		return false;
	}

	// Load speech file.
	if( !langPak.LoadSpeechFile( speechFileName ) )
	{
		LOG_WCC( TXT( "Cannot load speech file '%s'!" ), speechFileName.AsChar( ) );
		return false;
	}

	CCookedStrings& strings = langPak.m_cookedStrings;
	CCookedSpeeches& speeches = langPak.m_cookedSpeeches;

	// Build an "entry usage" table (just for DEBUG purposes).
	THashMap< Uint32, Uint32 > entryUsageMap( speeches.m_offsetMap.Size( ) );
	for( CCookedSpeeches::TOffsetMap::const_iterator it = speeches.m_offsetMap.Begin( ); it != speeches.m_offsetMap.End( ); ++it )
		entryUsageMap.GetRef( it->m_first ^ speeches.m_langKey ) = 1;

	// Just for statistics.
	Uint32 currentChunkID = 0;

	// So now, build each speech file. This could have been done on-the-fly when analyzing
	// the databases, but the analysis is super fast and the code is more clear this way.
	for( TChunkStringIds::const_iterator it = m_chunkStringIds.Begin( ); it != m_chunkStringIds.End( ); ++it )
	{
		CTimeCounter timer;
		LOG_WCC( TXT( "Processing chunk %d/%d (%d entries).." ), currentChunkID + 1, m_chunkStringIds.Size( ), it->m_second.Size( ) );
		++currentChunkID;

		LanguagePackage newLangPak( language );

		// Populate the new split file data table. Very expensive (mainly due to the SORTED hash).
		const TStringIds& stringIds = it->m_second;
		for( Uint32 stringId : stringIds )
		{
			CCookedSpeeches::SOffset offset;
			if( !speeches.GetOffset( stringId, offset ) )
				continue;
			newLangPak.AddSpeech( offset, stringId );

			// Mark this entry as used.
			entryUsageMap.GetRef( stringId ) = 0;
		}

		// Default content chunk must have ALL keys.
		if( it->m_first == CName( TXT( "content0" ) ) )
		{
			for( CCookedStrings::TKeysMap::const_iterator keyIt = strings.m_keysMap.Begin( ); keyIt != strings.m_keysMap.End( ); ++keyIt )
			{
				CCookedStrings::TStringId stringId = 0;
				strings.GetStringIdByKey( keyIt->m_first, stringId );
				CCookedSpeeches::SOffset offset;
				if( !speeches.GetOffset( stringId, offset ) )
					continue;
				newLangPak.AddSpeech( offset, stringId );

				// Mark this entry as used.
				entryUsageMap.GetRef( stringId ) = 0;
			}
		}

		String newSpeechFilePath;
		BuildSplitFilePath( newSpeechFilePath, it->m_first.AsString( ), language, platform, TXT( "w3speech" ) );

		// There we go.
		LOG_WCC( TXT( "Writting file '%s'" ), newSpeechFilePath.AsChar( ) );
		newLangPak.WriteSpeechFile( speechFileName, newSpeechFilePath );

		LOG_WCC( TXT( "Chunk file generated in %.2f seconds!" ), timer.GetTimePeriod( ) );
	}

	// Report entry usage.
	Uint32 totalDiscarded = 0;
	for( THashMap< Uint32, Uint32 >::const_iterator it = entryUsageMap.Begin( ); it != entryUsageMap.End( ); ++it )
	{
		if( it->m_second == 1 )
			++totalDiscarded;
	}
	Float discardRatio = static_cast< Float >( totalDiscarded ) / static_cast< Float >( entryUsageMap.Size( ) );
	LOG_WCC( TXT( "Discarded %d out of %d speech entries (%.1f%%)!" ), totalDiscarded, entryUsageMap.Size( ), discardRatio * 100.0f );

	// Done!
	return true;
}

void CStringSplitterCommandlet::BuildSplitFilePath( String& outPath, String contentName, String language, String platform, String extension ) const
{
	if ( m_settings.m_dlcMode )
	{
		outPath = String::Printf
		(
			TXT( "%s%s%s.%s" ),
			m_settings.m_outDir.AsChar( ), language.AsChar( ), platform.AsChar( ), extension.AsChar( )
		);
	}
	else if( !m_settings.m_allinsamefolder )
	{
		// Make sure directory exists.
		String newSplitFileDirectory = String::Printf
		(
			TXT( "%s%s\\" ),
			m_settings.m_outDir.AsChar( ), contentName.AsChar( )
		);
		GFileManager->CreatePath( newSplitFileDirectory );

		// Build the complete path.
		outPath = String::Printf
		(
			TXT( "%s%s%s.%s" ),
			newSplitFileDirectory.AsChar( ), language.AsChar( ), platform.AsChar( ), extension.AsChar( )
		);
	}
	else
	{
		outPath = String::Printf
		(
			TXT( "%s%s%s_%s.%s" ),
			m_settings.m_outDir.AsChar( ), language.AsChar( ), platform.AsChar( ), contentName.AsChar( ), extension.AsChar( )
		);
	}
}