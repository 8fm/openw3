#include "build.h"
#include "r4DLCJournalMounter.h"
#include "../../common/core/depot.h"
#include "../../common/game/journalPath.h"

IMPLEMENT_ENGINE_CLASS( CR4JournalDLCMounter );

CR4JournalDLCMounter::CR4JournalDLCMounter()
{
}

bool CR4JournalDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4JournalDLCMounter::OnGameStarting()
{
	// nothing
}

void CR4JournalDLCMounter::OnGameStarted()
{	
	Activate();
}

void CR4JournalDLCMounter::OnGameEnding()
{
	Deactivate();
}

void CR4JournalDLCMounter::NormalizeJournalDirectoryPath()
{
	static const Char endOfDirPath = '\\';
	if( m_journalDirectoryPath.Empty() == false )
	{
		const Char last = *(m_journalDirectoryPath.Begin() + (m_journalDirectoryPath.GetLength() - 1));
		if( last != endOfDirPath )
		{
			m_journalDirectoryPath += endOfDirPath;
		}
	}
}

#ifndef NO_EDITOR

void CR4JournalDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4JournalDLCMounter::OnEditorStopped()
{
	Deactivate();
}

void CR4JournalDLCMounter::OnSerialize( class IFile& file )
{
	if( file.IsWriter() )
	{
		NormalizeJournalDirectoryPath();
	}
	TBaseClass::OnSerialize( file );
}

bool CR4JournalDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{	
	if( !m_journalDirectoryPath.Empty() )
	{
		CDirectory* dlcDirectory = GDepot->FindPath( m_journalDirectoryPath );

		if( dlcDirectory )
		{
			TDynArray< String > resourcesPaths;
			dlcDirectory->FindResourcesByExtension( ResourceExtension< CJournalResource >(), resourcesPaths );
			for ( String filePath : resourcesPaths )
			{
				outputList.AddFile( UNICODE_TO_ANSI( filePath.AsChar() ) );
			}
		}
	}
	return false;
}

#endif // !NO_EDITOR

void CR4JournalDLCMounter::Activate()
{
	if( !m_journalDirectoryPath.Empty() )
	{
		CDirectory * directory = GDepot->FindPath( m_journalDirectoryPath.AsChar() );
		if( directory != NULL )
		{
			#ifdef JOURNAL_PATH_PRECACHE_RESOURCES
			CJournalPath::UpdateResources( m_journalDirectoryPath.AsChar() );
			#endif
		}
		else
		{
			RED_LOG_WARNING( RED_LOG_CHANNEL(DLC), TXT( "Journal directory '%ls' not present." ), m_journalDirectoryPath.AsChar() );
		}
	}
}

void CR4JournalDLCMounter::Deactivate()
{
	// on end game all journals are cleaned
}
