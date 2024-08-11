#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/engine/cookedStrings.h"

class CStringCharsetDumperCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CStringCharsetDumperCommandlet, ICommandlet, 0 );

public:

	CStringCharsetDumperCommandlet( )
	{
		m_commandletName = CName( TXT( "dumpcharset" ) );
	}

	virtual bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner( ) const;
	virtual void PrintHelp( ) const;

private:

	struct Settings
	{
		String	m_inStringsFilePath;
		String	m_outCharsetFilePath;
		String	m_outStringEntriesFilePath;
		String	m_baseDirectory;
		String	m_targetLanguage;

		Settings( )
			: m_inStringsFilePath( String::EMPTY )
			, m_outCharsetFilePath( String::EMPTY )
			, m_outStringEntriesFilePath( String::EMPTY )
			, m_baseDirectory( String::EMPTY )
			, m_targetLanguage( String::EMPTY )
		{ }

		Bool Parse( const ICommandlet::CommandletOptions& options );
	};

private:

	Settings m_settings;

private:

	Bool ExploreDirectory( String basePath, String currentDir, String lang, String& charset, Bool recursive );

	Bool DumpCharset( String filePath, String& charset );
	Bool SaveCharset( String filePath, String& charset );

	Bool DumpStringEntries( String inFilePath, String outFilePath );
};

BEGIN_CLASS_RTTI( CStringCharsetDumperCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI( )

IMPLEMENT_ENGINE_CLASS( CStringCharsetDumperCommandlet );

bool CStringCharsetDumperCommandlet::Execute( const CommandletOptions& options )
{
	// Parse options.
	if ( !m_settings.Parse( options ) )
		return false;

	String charset = String::EMPTY;

	// Do the thing.
	if( m_settings.m_baseDirectory != String::EMPTY && m_settings.m_targetLanguage != String::EMPTY )
	{
		if( !ExploreDirectory( m_settings.m_baseDirectory, String::EMPTY, m_settings.m_targetLanguage, charset, true ) )
		{
			ERR_WCC( TXT( "No string files found in path '%s' for language '%s'!" ), m_settings.m_baseDirectory.AsChar( ), m_settings.m_targetLanguage.AsChar( ) );
			return false;
		}
	}
	else
	{
		if( !DumpCharset( m_settings.m_inStringsFilePath, charset ) )
			return false;

		// Optional.
		DumpStringEntries( m_settings.m_inStringsFilePath, m_settings.m_outStringEntriesFilePath );
	}

	// Save the results.
	if( !SaveCharset( m_settings.m_outCharsetFilePath, charset ) )
		return false;

	// Done!
	return true;
}

const Char* CStringCharsetDumperCommandlet::GetOneLiner( ) const
{
	return TXT( "Dumps the charset used in a given strings file (*.w3strings)." );
}

void CStringCharsetDumperCommandlet::PrintHelp( ) const
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  dumpcharset -instringsfile=<filepath> -outcharsetfile=<filepath> [options]" ) );
	LOG_WCC( TXT( "    -instringsfile     - Path to the input strings file." ) );
	LOG_WCC( TXT( "    -outcharsetfile    - Path to the out charset file (if not given, default is used).") );
	LOG_WCC( TXT( "" ) );
	LOG_WCC( TXT( "Options:" ) );
	LOG_WCC( TXT( "  -outentriesfile=<filepath>  - Dumps all string entries into filepath. Only in single file mode." ) );
	LOG_WCC( TXT( "  -directory=<filepath>       - Path to base directory for recursive dump. Used with targetlanguage." ) );
	LOG_WCC( TXT( "  -language=<language>        - Target language for recursive dump. Used with targetlanguage." ) );
}

Bool CStringCharsetDumperCommandlet::Settings::Parse( const ICommandlet::CommandletOptions& options )
{
	options.GetSingleOptionValue( TXT( "instringsfile" ), m_inStringsFilePath );

	if( !options.GetSingleOptionValue( TXT( "outcharsetfile" ), m_outCharsetFilePath ) )
	{
		ERR_WCC( TXT( "No out file path has been specified!" ) );
		return false;
	}

	options.GetSingleOptionValue( TXT( "outentriesfile" ), m_outStringEntriesFilePath );

	Bool directory = options.GetSingleOptionValue( TXT( "directory" ), m_baseDirectory );
	Bool language = directory && options.GetSingleOptionValue( TXT( "language" ), m_targetLanguage );
	Bool singleFile = !( directory && language );

	if( directory && !language )
	{
		WARN_WCC( TXT( "No target language has been specified!" ) );
	}
	else if( !directory && language )
	{
		WARN_WCC( TXT( "No base directory has been specified!" ) );
	}

	if( singleFile && m_inStringsFilePath == String::EMPTY )
	{
		ERR_WCC( TXT( "No in file path has been specified!" ) );
		return false;
	}

	return true;
}

Bool CStringCharsetDumperCommandlet::ExploreDirectory( String basePath, String currentDir, String lang, String& charset, Bool recursive )
{
	String currentPath = basePath + currentDir;

	String filePath = currentPath + lang + TXT( ".w3strings" );
	Bool done = false;

	if( GFileManager->FileExist( filePath ) )
		done = DumpCharset( filePath, charset );

	if( done )
	{
		LOG_WCC( TXT( "Processed file '%s'." ), filePath.AsChar( ) );
	}

	if( recursive )
	{
		TDynArray< String > directories;
		GFileManager->FindDirectories( currentPath, directories );
		for( String directory : directories )
		{
			done = ExploreDirectory( basePath, currentDir + directory + TXT( "\\" ), lang, charset, recursive ) || done;
		}
	}

	return done;
}

Bool CStringCharsetDumperCommandlet::DumpCharset( String filePath, String& charset )
{
	Red::TScopedPtr< IFile > stringsFile( GFileManager->CreateFileReader( filePath, FOF_AbsolutePath ) );
	if( !stringsFile )
	{
		ERR_WCC( TXT( "Error creating file reader '%s'!" ), filePath.AsChar( ) );
		return false;
	}

	CCookedStrings m_cookedStrings;
	m_cookedStrings.Load( *stringsFile );

	for( CCookedStrings::TOffsetMap::const_iterator it = m_cookedStrings.m_offsetMap.Begin( ); it != m_cookedStrings.m_offsetMap.End( ); ++it )
	{
		String str = String::EMPTY;
		if( !m_cookedStrings.GetString( it->m_first ^ m_cookedStrings.m_langKey, str ) )
		{
			WARN_WCC( TXT( "String id %d does not exist in string table! WTF?!" ), it->m_first );
			continue;
		}

		// Some DB entries are fucked up.
		if( str == String::EMPTY )
			continue;

		for( Uint32 c = 0; c < str.Size( ) - 1; ++c )
		{
			charset.PushBackUnique( str[ c ] );
		}
	}

	charset.PushBack( '\0' );

	return true;
}

Bool CStringCharsetDumperCommandlet::SaveCharset( String filePath, String& charset )
{
	Sort( charset.Begin( ), charset.End( ), [ ]( const Char& a, const Char& b ) { return Red::StringCompare( &a, &b ) < 0; } );

	if( !GFileManager->SaveStringToFileWithUTF8( filePath, charset ) )
	{
		ERR_WCC( TXT( "Error creating file writer '%s'!" ), filePath.AsChar( ) );
		return false;
	}

	return true;
}

Bool CStringCharsetDumperCommandlet::DumpStringEntries( String inFilePath, String outFilePath )
{
	if( inFilePath == String::EMPTY || outFilePath == String::EMPTY )
		return false;

	CCookedStrings m_cookedStrings;

	{
		Red::TScopedPtr< IFile > stringsFile( GFileManager->CreateFileReader( inFilePath, FOF_AbsolutePath ) );
		if( !stringsFile )
		{
			ERR_WCC( TXT( "Error creating file reader '%s'!" ), inFilePath.AsChar( ) );
			return false;
		}
		m_cookedStrings.Load( *stringsFile );
	}

	String entries = TXT( "--- STRINGS ---\n" );

	for( CCookedStrings::TOffsetMap::const_iterator it = m_cookedStrings.m_offsetMap.Begin( ); it != m_cookedStrings.m_offsetMap.End( ); ++it )
	{
		String str = String::EMPTY;
		if( !m_cookedStrings.GetString( it->m_first ^ m_cookedStrings.m_langKey, str ) )
		{
			WARN_WCC( TXT( "String id %u does not exist in string table! WTF?!" ), it->m_first );
			continue;
		}

		// Some DB entries are fucked up.
		if( str == String::EMPTY )
			continue;

		// Not ideal, I know.
		entries += String::Printf( TXT( "%u - %ls\n" ), it->m_first ^ m_cookedStrings.m_langKey, str.AsChar( ) );
	}

	entries += TXT( "--- KEYS ---\n" );

	for( CCookedStrings::TKeysMap::const_iterator it = m_cookedStrings.m_keysMap.Begin( ); it != m_cookedStrings.m_keysMap.End( ); ++it )
	{
		String str = String::EMPTY;
		if( !m_cookedStrings.GetString( it->m_second ^ m_cookedStrings.m_langKey, str ) )
		{
			WARN_WCC( TXT( "String id %u does not exist in string table! WTF?!" ), it->m_second );
			continue;
		}

		// Some DB entries are fucked up.
		if( str == String::EMPTY )
			continue;

		// Not ideal, I know.
		entries += String::Printf( TXT( "%u - %u\n" ), it->m_first ^ m_cookedStrings.m_langKey, it->m_second ^ m_cookedStrings.m_langKey );
	}

	if( !GFileManager->SaveStringToFileWithUTF8( outFilePath, entries ) )
	{
		ERR_WCC( TXT( "Error creating file writer '%s'!" ), outFilePath.AsChar( ) );
		return false;
	}

	return true;
}