#include "build.h"
#include "../../common/core/commandlet.h"
#include "cookDataBase.h"

class CChunkReportCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CChunkReportCommandlet, ICommandlet, 0 );

public:
	CChunkReportCommandlet( );

	virtual bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner( ) const;
	virtual void PrintHelp( ) const;

private:

	struct Settings
	{
		enum EVerbosity
		{
			ShowSumary		= FLAG( 0 ),
			ShowExtensions	= FLAG( 1 ),
			ShowFileChunks	= FLAG( 2 ),
			ShowChunkFiles	= FLAG( 3 ),

			ShowAll = ShowSumary | ShowExtensions | ShowFileChunks | ShowChunkFiles
		};

		String		m_dataBaseFilePath;
		String		m_outputPath;
		Uint8		m_verbosity;

		Settings( );

		Bool Parse( const ICommandlet::CommandletOptions& options );
	};

	struct ChunkInfo
	{
		Uint64					m_totalBytes;
		TDynArray< Uint32 >		m_fileIdxs;

		ChunkInfo( )
			: m_totalBytes( 0 )
		{ }
	};

	typedef THashMap< CName, ChunkInfo > TChunksInfo;
	typedef THashMap< CName, Uint32 > TChunkIDCount;
	typedef THashMap< String, TChunkIDCount > TExtensionChunkIDCount;

private:

	Settings					m_settings;
	CCookerDataBase				m_dataBase;
	TChunksInfo					m_chunksInfo;
	TExtensionChunkIDCount		m_extensionInfo;
};

BEGIN_CLASS_RTTI( CChunkReportCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI( )

IMPLEMENT_ENGINE_CLASS( CChunkReportCommandlet );

CChunkReportCommandlet::CChunkReportCommandlet( )
{
	m_commandletName = CName( TXT( "reportchunks" ) );
}

bool CChunkReportCommandlet::Execute( const CommandletOptions& options )
{
	// Parse options.
	if ( !m_settings.Parse( options ) )
	{
		return false;
	}

	// Load the cook database.
	if ( !m_dataBase.LoadFromFile( m_settings.m_dataBaseFilePath ) )
	{
		ERR_WCC( TXT( "Failed to load cooked DB from file '%ls'!" ), m_settings.m_dataBaseFilePath.AsChar( ) );
		return false;
	}

#if 0
	// Get a local copy of all entries in the database.
	TDynArray< CCookerResourceEntry > cookedEntries;
	m_dataBase.GetFileEntries( cookedEntries );

	LOG_WCC( TXT( "Sorting data.." ) );

	// Sort entries by chunk count and file size (this is horrible!).
	struct CCookerResourceEntryCompFunc
	{
		const CCookerDataBase* m_dataBase;
		CCookerResourceEntryCompFunc( const CCookerDataBase* db ) : m_dataBase( db ) { }
		Bool operator ( ) ( const CCookerResourceEntry& a, const CCookerResourceEntry& b ) const
		{
			CCookerDataBase::TCookerDataBaseID aID = m_dataBase->GetFileEntry( a.GetFilePath( ) );
			CCookerDataBase::TCookerDataBaseID bID = m_dataBase->GetFileEntry( b.GetFilePath( ) );
			Uint32 aDiskSize = 0;
			m_dataBase->GetFileDiskSize( aID, aDiskSize );
			TDynArray< CName > aChunkIDs;
			m_dataBase->GetChunkIDs( aID, aChunkIDs );
			Uint32 bDiskSize = 0;
			m_dataBase->GetFileDiskSize( bID, bDiskSize );
			TDynArray< CName > bChunkIDs;
			m_dataBase->GetChunkIDs( bID, bChunkIDs );
			if( bChunkIDs.Size( ) == aChunkIDs.Size( ) )
			{
				return bDiskSize < aDiskSize;
			}
			return bChunkIDs.Size( ) < aChunkIDs.Size( );
		}
	};
	Sort( cookedEntries.Begin( ), cookedEntries.End( ), CCookerResourceEntryCompFunc( &m_dataBase ) );

	LOG_WCC( TXT( "Building data matrix.." ) );

	// Build a data matrix.
	for( Uint32 i = 0; i < cookedEntries.Size( ); ++i )
	{
		const CCookerResourceEntry& cookedEntry = cookedEntries[ i ];
		CCookerDataBase::TCookerDataBaseID entryID = m_dataBase.GetFileEntry( cookedEntry.GetFilePath( ) );

		Uint32 diskSize = 0;
		m_dataBase.GetFileDiskSize( entryID, diskSize );


		String path = String::Printf( TXT( "%hs" ), cookedEntries[ i ].GetFilePath( ).AsChar( ) );
		TChunkIDCount& extensionChunks = m_extensionInfo.GetRef( StringHelpers::GetFileExtension( path ) );

		if( !chunkIDs.Empty( ) )
		{
			// Update global statistics.
			for( auto chunkID : chunkIDs )
			{
				// Chunk statistics.
				ChunkInfo& chunkInfo = m_chunksInfo.GetRef( chunkID );
				chunkInfo.m_totalBytes += diskSize;
				chunkInfo.m_fileIdxs.PushBack( i );

				// Extension statistics.
				++extensionChunks.GetRef( chunkID );
			}
		}
		else
		{
			// Chunkless statistics.
			ChunkInfo& chunkInfo = m_chunksInfo.GetRef( chunklessName );
			chunkInfo.m_totalBytes += diskSize;
			chunkInfo.m_fileIdxs.PushBack( i );

			// Extensionless statistics.
			++extensionChunks.GetRef( chunklessName );
		}
	}

	// Clean output file.
	GFileManager->SaveStringToFile( m_settings.m_outputPath, TXT( "" ) );

	// Build summary.
	if( m_settings.m_verbosity & Settings::ShowSumary )
	{
		LOG_WCC( TXT( "Building summary.." ) );

		// Header.
		String summaryData = String::Printf( TXT( "Chunk,Files,MBytes\n" ) );

		// Chunkless count first.
		const ChunkInfo& chunklessInfo = m_chunksInfo.GetRef( chunklessName );
		summaryData += String::Printf( TXT( "%s,%d,%llu\n" ),
			chunklessName.AsChar( ), chunklessInfo.m_fileIdxs.Size( ), ( chunklessInfo.m_totalBytes >> 20 ) + ( ( chunklessInfo.m_totalBytes & 0xfffff ) > 0 ) );

		// The rest.
		for( TChunksInfo::const_iterator it = m_chunksInfo.Begin( ); it != m_chunksInfo.End( ); ++it )
		{
			if( it->m_first == chunklessName )
				continue;

			const ChunkInfo& chunkInfo = it->m_second;
			summaryData += String::Printf( TXT( "%s,%d,%llu\n" ),
				it->m_first.AsChar( ), chunkInfo.m_fileIdxs.Size( ), ( chunkInfo.m_totalBytes >> 20 ) + ( ( chunkInfo.m_totalBytes & 0xfffff ) > 0 ) );
		}
		summaryData += TXT( "\n\n" );

		// Happiness.
		GFileManager->SaveStringToFile( m_settings.m_outputPath, summaryData, true );
	}

	// Build extension report.
	if( m_settings.m_verbosity & Settings::ShowExtensions )
	{
		LOG_WCC( TXT( "Building extensions report.." ) );

		const ChunkInfo& chunklessInfo = m_chunksInfo.GetRef( chunklessName );

		// Header.
		String extensionsData = String::Printf( TXT( "Extension,%s," ), chunklessName.AsChar( ) );
		for( TChunksInfo::const_iterator it = m_chunksInfo.Begin( ); it != m_chunksInfo.End( ); ++it )
		{
			if( it->m_first == chunklessName )
				continue;

			extensionsData += String::Printf( TXT( "%s," ), it->m_first.AsChar( ) );
		}
		extensionsData += TXT( "\n" );

		// The rest.
		for( TExtensionChunkIDCount::const_iterator it = m_extensionInfo.Begin( ); it != m_extensionInfo.End( ); ++it )
		{
			extensionsData += String::Printf( TXT( "%s," ), it->m_first.AsChar( ) );

			const TChunkIDCount& extensionChunks = it->m_second;

			// Chunkless first.
			const Uint32* chunklessValue = extensionChunks.FindPtr( chunklessName );
			if( chunklessValue != nullptr )
				extensionsData += String::Printf( TXT( "%d" ), *chunklessValue );
			extensionsData += TXT( "," );

			for( TChunksInfo::const_iterator chunkIt = m_chunksInfo.Begin( ); chunkIt != m_chunksInfo.End( ); ++chunkIt )
			{
				if( chunkIt->m_first == chunklessName )
					continue;

				const Uint32* value = extensionChunks.FindPtr( chunkIt->m_first );
				if( value != nullptr )
				{
					extensionsData += String::Printf( TXT( "%d" ), *value );
				}
				extensionsData += TXT( "," );
			}
			extensionsData += TXT( "\n" );
		}
		extensionsData += TXT( "\n\n" );

		// Happiness.
		GFileManager->SaveStringToFile( m_settings.m_outputPath, extensionsData, true );
	}

	// Build file chunks report.
	if( m_settings.m_verbosity & Settings::ShowFileChunks )
	{
		LOG_WCC( TXT( "Building file/chunks report.." ) );

		// Header.
		String fileChunksData = TXT( "Filename,Bytes,Chunks," );
		for( TChunksInfo::const_iterator it = m_chunksInfo.Begin( ); it != m_chunksInfo.End( ); ++it )
		{
			if( it->m_first == chunklessName )
				continue;

			fileChunksData += String::Printf( TXT( "%s," ), it->m_first.AsChar( ) );
		}
		fileChunksData += TXT( "\n" );

		// Inspect every database entry (SLOW!).
		for( Uint32 i = 0; i < cookedEntries.Size( ); ++i )
		{
			const CCookerResourceEntry& cookedEntry = cookedEntries[ i ];
			CCookerDataBase::TCookerDataBaseID entryID = m_dataBase.GetFileEntry( cookedEntry.GetFilePath( ) );

			Uint32 diskSize = 0;
			m_dataBase.GetFileDiskSize( entryID, diskSize );

			TDynArray< CName > chunkIDs;
			m_dataBase.GetChunkIDs( entryID, chunkIDs );

			fileChunksData = fileChunksData + String::Printf( TXT( "%hs,%d,%d," ), cookedEntry.GetFilePath( ).AsChar( ), diskSize, chunkIDs.Size( ) );

			for( TChunksInfo::const_iterator it = m_chunksInfo.Begin( ); it != m_chunksInfo.End( ); ++it )
			{
				if( it->m_first == chunklessName )
					continue;

				if( chunkIDs.Exist( it->m_first ) )
				{
					fileChunksData += TXT( "1" );
				}
				fileChunksData += TXT( "," );
			}
			fileChunksData += TXT( "\n" );
		}
		fileChunksData += TXT( "\n\n" );

		// Happiness.
		GFileManager->SaveStringToFile( m_settings.m_outputPath, fileChunksData, true );
	}

	// Build chunk files report.
	if( m_settings.m_verbosity & Settings::ShowChunkFiles )
	{
		LOG_WCC( TXT( "Building chunk/files report.." ) );

		String chunkFilesData = TXT( "" );

		// Chunkless first.
		chunkFilesData += String::Printf( TXT( "------ %s ------\n" ), chunklessName.AsChar( ) );
		const ChunkInfo& chunklessChunk = m_chunksInfo.GetRef( chunklessName );
		for( auto fileIdx : chunklessChunk.m_fileIdxs )
			chunkFilesData += String::Printf( TXT( "%hs\n" ), cookedEntries[ fileIdx ].GetFilePath( ).AsChar( ) );
		chunkFilesData += TXT( "\n\n" );

		// The rest (SLOW!).
		for( TChunksInfo::const_iterator it = m_chunksInfo.Begin( ); it != m_chunksInfo.End( ); ++it )
		{
			if( it->m_first == chunklessName )
				continue;

			LOG_WCC( TXT( " Processing '%s' (%d entries).." ), it->m_first.AsChar( ), it->m_second.m_fileIdxs.Size( ) );

			chunkFilesData += String::Printf( TXT( "------ %s ------\n" ), it->m_first.AsChar( ) );
			for( auto fileIdx : it->m_second.m_fileIdxs )
				chunkFilesData += String::Printf( TXT( "%hs\n" ), cookedEntries[ fileIdx ].GetFilePath( ).AsChar( ) );
			chunkFilesData += TXT( "\n" );
		}
		chunkFilesData += TXT( "\n\n" );

		// Happiness.
		GFileManager->SaveStringToFile( m_settings.m_outputPath, chunkFilesData, true );
	}

	// On screen report (basically the summary).
	{
		const ChunkInfo& chunklessInfo = m_chunksInfo.GetRef( chunklessName );

		LOG_WCC( TXT( "+---------------------==---------------------+" ) );
		LOG_WCC( TXT( "|             CHUNK USAGE REPORT             |" ) );
		LOG_WCC( TXT( "+---------------------==---------------------+" ) );
		LOG_WCC( TXT( "| %9s :   %10d files (%6llu MB) |" ), chunklessName.AsChar( ),
			chunklessInfo.m_fileIdxs.Size( ),  ( chunklessInfo.m_totalBytes >> 20 ) + ( ( chunklessInfo.m_totalBytes & 0xfffff ) > 0 ) );
		LOG_WCC( TXT( "+---------------------==---------------------+" ) );
		for( TChunksInfo::const_iterator it = m_chunksInfo.Begin( ); it != m_chunksInfo.End( ); ++it )
		{
			if( it->m_first == chunklessName )
				continue;

			const ChunkInfo& chunkInfo = it->m_second;
			LOG_WCC( TXT( "| %9s :   %10d files (%6llu MB) |" ),
				it->m_first.AsChar( ), chunkInfo.m_fileIdxs.Size( ), ( chunkInfo.m_totalBytes >> 20 ) + ( ( chunkInfo.m_totalBytes & 0xfffff ) > 0 ) );
		}
		LOG_WCC( TXT( "+---------------------==---------------------+" ) );
	}

#endif

	// Done!
	return true;
}

const Char* CChunkReportCommandlet::GetOneLiner( ) const
{
	return TXT( "Reports file usage and chunk distribution of a given cook database." );
}

void CChunkReportCommandlet::PrintHelp( ) const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  reportchunks -db=<filepath> -out=<filepath> <options>") );
	LOG_WCC( TXT("") );
	LOG_WCC( TXT("Options:") );
	LOG_WCC( TXT("  -showall         - Outputs exhaustive report (default).") );
	LOG_WCC( TXT("  -showsumary      - Outputs global statistics.") );
	LOG_WCC( TXT("  -showextensions  - Outputs extension/chunk statistics.") );
	LOG_WCC( TXT("  -showfilechunks  - Outputs file/chunk statistics (slow!).") );
	LOG_WCC( TXT("  -showchunkfiles  - Outputs the list of files in each chunk (slow!).") );
}

CChunkReportCommandlet::Settings::Settings( )
{
	m_dataBaseFilePath  = TXT( "" );
	m_outputPath		= TXT( "" );
	m_verbosity			= 0;
}

Bool CChunkReportCommandlet::Settings::Parse( const ICommandlet::CommandletOptions& options )
{
	// Get cook database path.
	if( !options.GetSingleOptionValue( TXT( "db" ), m_dataBaseFilePath ) )
	{
		ERR_WCC( TXT( "No database path has been specified!" ) );
		return false;
	}

	// Get report file path.
	if( !options.GetSingleOptionValue( TXT( "out" ), m_outputPath ) )
	{
		ERR_WCC( TXT( "No output path has been specified!" ) );
		return false;
	}

	m_verbosity = 0;

	// Get verbosity parameters.
	if( options.HasOption( TXT( "showall" ) ) )
	{
		m_verbosity = Settings::ShowAll;
	}
	else
	{
		if( options.HasOption( TXT( "showsumary" ) ) )
			m_verbosity |= Settings::ShowSumary;
		if( options.HasOption( TXT( "showextensions" ) ) )
			m_verbosity |= Settings::ShowExtensions;
		if( options.HasOption( TXT( "showfilechunks" ) ) )
			m_verbosity |= Settings::ShowFileChunks;
		if( options.HasOption( TXT( "showchunkfiles" ) ) )
			m_verbosity |= Settings::ShowChunkFiles;
	}

	if( m_verbosity == 0 )
		m_verbosity = Settings::ShowSumary;

	return true;
}