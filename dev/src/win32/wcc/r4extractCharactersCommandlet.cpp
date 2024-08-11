#include "build.h"
#include "../../common/core/commandlet.h"
#include "cookDataBase.h"
#include "cookSeedFile.h"

class CR4ExtractCharacters : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CR4ExtractCharacters, ICommandlet, 0 );

public:
	CR4ExtractCharacters( );

	virtual bool Execute( const CommandletOptions& options );
	virtual const Char* GetOneLiner( ) const { return TXT("Extracts a list of character templates from cook.db"); }
	virtual void PrintHelp( ) const;

private:
	struct FileInfo
	{
		CCookerDataBase::TCookerDataBaseID		m_id;
		StringAnsi								m_path;

		Bool									m_isInCharactersDir;
		TDynArray< FileInfo* >					m_characterUsers;

		TDynArray< FileInfo* >					m_hardDeps;

		FileInfo( CCookerDataBase::TCookerDataBaseID id, const StringAnsi& path )
			: m_id( id )
			, m_path( path )
			, m_isInCharactersDir( false )
		{}
	};

	TDynArray< FileInfo* >					m_files;
	THashMap< StringAnsi, FileInfo* >		m_fileMap;
};

BEGIN_CLASS_RTTI( CR4ExtractCharacters )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI( )

IMPLEMENT_ENGINE_CLASS( CR4ExtractCharacters );

CR4ExtractCharacters::CR4ExtractCharacters( )
{
	m_commandletName = CName( TXT( "r4characters" ) );
}

bool CR4ExtractCharacters::Execute( const CommandletOptions& options )
{
	// get DB path
	String dbPath;
	if ( !options.GetSingleOptionValue( TXT("db"), dbPath ) )
	{
		ERR_WCC( TXT("Expecting path to cook.db") );
		return false;
	}

	// get output file path
	String outPath;
	if ( !options.GetSingleOptionValue( TXT("out"), outPath ) )
	{
		ERR_WCC( TXT("Expecting path to output file") );
		return false;
	}

	// load cook db
	CCookerDataBase db;
	if ( !db.LoadFromFile( dbPath ) )
	{
		ERR_WCC( TXT("Failed to load cook.db from '%ls'"), dbPath.AsChar() );
		return false;
	}

	// extract data
	{
		CTimeCounter timer;

		// get file entries from DB
		TDynArray< CCookerResourceEntry > dataBaseFiles;
		db.GetFileEntries( dataBaseFiles );

		// export files from DB to local list
		for ( const auto& entry : dataBaseFiles )
		{
			const CCookerDataBase::TCookerDataBaseID dbID = db.GetFileEntry( entry.GetFilePath() );
			if ( dbID != 0 )
			{
				FileInfo* info = new FileInfo( dbID, entry.GetFilePath() );
				m_fileMap.Insert( entry.GetFilePath(), info );
				m_files.PushBack( info );
			}
		}

		// map dependencies
		for ( FileInfo* file : m_files )
		{
			TDynArray<CCookerResourceEntry> hardDeps, softDeps, inplaceDeps;
			db.GetFileDependencies( file->m_id, hardDeps, softDeps, inplaceDeps );

			// map hard deps
			for ( const CCookerResourceEntry& dep : hardDeps )
			{
				// find in map
				FileInfo* depFile = nullptr;
				if ( m_fileMap.Find( dep.GetFilePath(), depFile ) )
				{
					RED_FATAL_ASSERT( depFile != nullptr, "Failed dependency" );
					file->m_hardDeps.PushBack( depFile );
				}
			}
		}

		LOG_WCC( TXT("Data extracted in %1.2fs"), timer.GetTimePeriod() );
	}

	// mark entity template in the character directories
	Uint32 numInitialCharacterTemplates = 0;
	TDynArray< FileInfo* > potentialCharacters;
	for ( FileInfo* file : m_files )
	{
		if ( file->m_path.EndsWith( "w2ent") )
		{
			if ( file->m_path.BeginsWith( "characters\\npc_entities\\" ) )
			{
				numInitialCharacterTemplates += 1;
				file->m_isInCharactersDir = true;
				potentialCharacters.PushBack( file );
			}
		}
	}
	LOG_WCC( TXT("Found %d templates in character directory"), numInitialCharacterTemplates );

	// mark files that are referenced from outside characters dirs
	for ( FileInfo* file : m_files )
	{
		// must not be in characters directory
		if ( !file->m_path.BeginsWith( "characters" ) && !file->m_path.BeginsWith( "qa" ) )
		{
			for ( FileInfo* dep : file->m_hardDeps )
			{
				if ( dep->m_isInCharactersDir )
				{
					dep->m_characterUsers.PushBack( file );
				}
			}
		}
	}

	// print users of "true" characters
	Uint32 numTrueCharacterTemplates = 0;
	CCookerSeedFile seedFile;
	for ( FileInfo* file : potentialCharacters )
	{
		// template is not used outside the characters directory
		if ( file->m_characterUsers.Empty() )
			continue;

		// add to output file
		seedFile.AddEntry( file->m_path, "characters" );

		// stats
		numTrueCharacterTemplates += 1;
		LOG_WCC( TXT("Character '%hs', used %d times"), 
			file->m_path.AsChar(), file->m_characterUsers.Size() );

		for ( Uint32 i=0; i<file->m_characterUsers.Size(); ++i )
		{
			LOG_WCC( TXT("    [%d]: '%hs'"), 
				i, file->m_characterUsers[i]->m_path.AsChar() );
		}
	}
	LOG_WCC( TXT("Found %d true characters templates"), numTrueCharacterTemplates );

	// save the output file
	if ( !seedFile.SaveToFile( outPath ) )
	{
		ERR_WCC( TXT("Failed to save output file '%ls'"), outPath.AsChar() );
		return false;
	}

	// saved
	return true;
}

void CR4ExtractCharacters::PrintHelp( ) const
{
	LOG_WCC( TXT("Usage:") ); 
	LOG_WCC( TXT("  r4characters -db=<path to cook.db> -out=<output seed file>") );
}
