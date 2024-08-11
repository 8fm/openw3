#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/engine/cookedStrings.h"
#include "cookDataBase.h"
#include "cookSplitList.h"

class CStringStripperCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CStringStripperCommandlet, ICommandlet, 0 );

public:

	CStringStripperCommandlet( )
	{
		m_commandletName = CName( TXT( "stripstrings" ) );
	}

	virtual bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner( ) const;
	virtual void PrintHelp( ) const;

private:

	struct Settings
	{
		String	m_inStringsFilePath;
		String	m_inBlackListFilePath;
		String	m_outStringsFilePath;

		TDynArray< String >	m_discardablePatterns;

		Settings( )
			: m_inStringsFilePath( TXT( "" ) )
			, m_inBlackListFilePath( TXT( "" ) )
			, m_outStringsFilePath( TXT( "" ) )
			, m_discardablePatterns( )
		{ }

		Bool Parse( const ICommandlet::CommandletOptions& options );
	};

	Settings m_settings;

private:

	Bool LoadStringCacheFile( const String& filePath, CCookedStrings& cookedStrings );
	Bool LoadBlackListFile( const String& filePath, TDynArray< Uint32 >& stringIds );
	void StripCookedStrings( const CCookedStrings& inCookedStrings, TDynArray< Uint32 >& stringIds, CCookedStrings& outCookedStrings, const TDynArray< String >& patternList );
	Bool SaveStringCacheFile( const String& filePath, CCookedStrings& cookedStrings );
};

BEGIN_CLASS_RTTI( CStringStripperCommandlet )
	PARENT_CLASS( ICommandlet )
	END_CLASS_RTTI( )

	IMPLEMENT_ENGINE_CLASS( CStringStripperCommandlet );

bool CStringStripperCommandlet::Execute( const CommandletOptions& options )
{
	// Parse options.
	if ( !m_settings.Parse( options ) )
		return false;

	CCookedStrings inCookedStrings;

	// Load the strings cache file.
	if( !LoadStringCacheFile( m_settings.m_inStringsFilePath, inCookedStrings ) )
		return false;

	TDynArray< Uint32 > blackListedIds;

	// Load the black list file.
	if( !LoadBlackListFile( m_settings.m_inBlackListFilePath, blackListedIds ) && m_settings.m_discardablePatterns.Empty( ) )
		return false;

	CCookedStrings outCookedStrings;

	StripCookedStrings( inCookedStrings, blackListedIds, outCookedStrings, m_settings.m_discardablePatterns );

	// Save the stripped strings cache file.
	if( !SaveStringCacheFile( m_settings.m_outStringsFilePath, outCookedStrings ) )
		return false;

	// Done!
	return true;
}

const Char* CStringStripperCommandlet::GetOneLiner( ) const
{
	return TXT( "Removes from a string cache all the entries listed in a black list." );
}

void CStringStripperCommandlet::PrintHelp( ) const
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  stripstrings -incachefile=<filepath> -blacklistfile=<filepath> -outcachefile=<filepath> -patternlist=..." ) );
	LOG_WCC( TXT( "    -incachefile   - Path to the strings cache file." ) );
	LOG_WCC( TXT( "    -blacklistfile - Path to the black list file (txt file with one string id per line)." ) );
	LOG_WCC( TXT( "    -outcachefile  - Path to the stripped strings cache file." ) );
	LOG_WCC( TXT( "    -patternlist   - Comma separated list of string patterns that can be discarded." ) );
}

Bool CStringStripperCommandlet::Settings::Parse( const ICommandlet::CommandletOptions& options )
{
	// Get input strings cache file path.
	if( !options.GetSingleOptionValue( TXT( "incachefile" ), m_inStringsFilePath ) )
	{
		ERR_WCC( TXT( "No input strings cache file path has been specified!" ) );
		return false;
	}

	// Get input strings cache file path.
	options.GetSingleOptionValue( TXT( "blacklistfile" ), m_inBlackListFilePath );

	// Get output strings cache file path.
	if( !options.GetSingleOptionValue( TXT( "outcachefile" ), m_outStringsFilePath ) )
	{
		ERR_WCC( TXT( "No output strings cache file path has been specified!" ) );
		return false;
	}

	// Get discardable pattern list.
	TList< String > patternList;
	if( options.HasOption( TXT( "patternlist" ) ) )
	{
		patternList = options.GetOptionValues( TXT( "patternlist" ) );
		for( const String& pattern : patternList )
			m_discardablePatterns.PushBackUnique( pattern );
	}

	// Check that we have all we need.
	if( m_inBlackListFilePath == String::EMPTY && m_discardablePatterns.Empty( ) )
	{
		ERR_WCC( TXT( "No black list file path or pattern list have been specified!" ) );
		return false;
	}

	return true;
}

Bool CStringStripperCommandlet::LoadStringCacheFile( const String& filePath, CCookedStrings& cookedStrings )
{
	// Create strings file reader.
	Red::TScopedPtr< IFile > stringsFile( GFileManager->CreateFileReader( filePath, FOF_AbsolutePath ) );
	if( !stringsFile )
	{
		ERR_WCC( TXT( "Error creating file reader '%ls'!" ), filePath.AsChar( ) );
		return false;
	}

	cookedStrings.Load( *stringsFile );

	return true;
}

Bool CStringStripperCommandlet::LoadBlackListFile( const String& filePath, TDynArray< Uint32 >& stringIds )
{
	stringIds.Clear( );

	String blackListContents = String::EMPTY;
	if( !GFileManager->LoadFileToString( filePath, blackListContents, true ) )
	{
		ERR_WCC( TXT( "Error reading black list file '%ls'!" ), filePath.AsChar( ) );
		return false;
	}

	TDynArray< String > blackListTokens;
	blackListContents.Slice( blackListTokens, TXT( "\n" ) );

	if( blackListTokens.Empty( ) )
	{
		ERR_WCC( TXT( "The black list is empty!" ) );
		return false;
	}

	for( const String& token : blackListTokens )
	{
		stringIds.PushBackUnique( atoi( UNICODE_TO_ANSI( token.AsChar( ) ) ) );
	}

	ERR_WCC( TXT( "Found %d entries in the black list." ), stringIds.Size( ) );

	return true;
}

void CStringStripperCommandlet::StripCookedStrings( const CCookedStrings& inCookedStrings, TDynArray< Uint32 >& stringIds, CCookedStrings& outCookedStrings, const TDynArray< String >& patternList )
{
	// Dammit, removing strings is a bit more involved than I thought. I'll go the lazy way for now.

	outCookedStrings.m_langKey = inCookedStrings.m_langKey;
	outCookedStrings.m_fileKey = inCookedStrings.m_fileKey;

	// Add strings to the new cache.
	for( CCookedStrings::TOffsetMap::const_iterator it = inCookedStrings.m_offsetMap.Begin( ); it != inCookedStrings.m_offsetMap.End( ); ++it )
	{
		const CCookedStrings::TStringId strId = it->m_first ^ inCookedStrings.m_langKey;

		String str = String::EMPTY;
		if( !inCookedStrings.GetString( strId, str ) )
		{
			WARN_WCC( TXT( "Cannot find string id %d in database!" ), strId );
			continue;
		}

		if( !patternList.Empty( ) )
		{
			for( const String& pattern : patternList )
			{
				if( str.BeginsWith( pattern ) )
				{
					LOG_WCC( TXT( "String id %d has been added to the black list due to the pattern '%ls'." ), strId, pattern.AsChar( ) );
					stringIds.PushBackUnique( strId );
					break;
				}
			}
		}

		if( stringIds.FindPtr( strId ) == nullptr )
			outCookedStrings.AddString( str, strId );
	}

	// Add keys to the new cache.
	for( CCookedStrings::TKeysMap::const_iterator it = inCookedStrings.m_keysMap.Begin( ); it != inCookedStrings.m_keysMap.End( ); ++it )
	{
		const CCookedStrings::TStringId strId = it->m_second ^ inCookedStrings.m_langKey;
		const CCookedStrings::TKeyId keyId = it->m_first;

		if( stringIds.FindPtr( strId ) == nullptr )
			outCookedStrings.AddKey( keyId, strId );
	}

	LOG_WCC( TXT( "Original strings cache has %d strings and %d keys." ), inCookedStrings.m_offsetMap.Size( ), inCookedStrings.m_keysMap.Size( ) );
	LOG_WCC( TXT( "Stripped strings cache has %d strings and %d keys." ), outCookedStrings.m_offsetMap.Size( ), outCookedStrings.m_keysMap.Size( ) );
}

Bool CStringStripperCommandlet::SaveStringCacheFile( const String& filePath, CCookedStrings& cookedStrings )
{
	// Create strings file writer.
	Red::TScopedPtr< IFile > stringsFile( GFileManager->CreateFileWriter( filePath, FOF_AbsolutePath ) );
	if( !stringsFile )
	{
		ERR_WCC( TXT( "Error creating file writer '%ls'!" ), filePath.AsChar( ) );
		return false;
	}

	cookedStrings.Save( *stringsFile );

	return true;
}