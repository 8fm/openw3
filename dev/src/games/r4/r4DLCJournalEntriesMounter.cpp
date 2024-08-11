#include "build.h"
#include "r4DLCJournalEntriesMounter.h"
#include "../../common/core/depot.h"
#include "../../common/game/dlcDefinition.h"

#include "r4JournalManager.h"

IMPLEMENT_ENGINE_CLASS( CR4JournalEntriesDLCMounter );

CR4JournalEntriesDLCMounter::CR4JournalEntriesDLCMounter()
{
}

bool CR4JournalEntriesDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4JournalEntriesDLCMounter::OnGameStarting()
{
	// nothing
}

void CR4JournalEntriesDLCMounter::OnGameStarted()
{	
	Activate();
}

void CR4JournalEntriesDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4JournalEntriesDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4JournalEntriesDLCMounter::OnEditorStopped()
{
	Deactivate();
}

bool CR4JournalEntriesDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{	
	const String& filename = m_journalEntriesFilePath.GetPath();
	if( !filename.Empty() )
	{
		if( GDepot->FileExist( filename ) )
		{
			const StringAnsi actualDepotPathAnsi( UNICODE_TO_ANSI( filename.AsChar() ) );
			outputList.AddFile( actualDepotPathAnsi );
		}
	}
	return true;
}

#endif // !NO_EDITOR

void CR4JournalEntriesDLCMounter::Activate()
{
	const CDLCDefinition* dlcDefinition = Cast< CDLCDefinition >( GetParent() );
	if ( dlcDefinition )
	{
		const String& filename = m_journalEntriesFilePath.GetPath();
		if( !filename.Empty() )
		{
			if( GDepot->FileExist( filename ) )
			{
				CWitcherJournalManager* journalManager = GCommonGame->GetSystem< CWitcherJournalManager >();
				if( journalManager )
				{
					journalManager->LoadInitialEntriesFile( dlcDefinition->GetID(), filename );
				}
			}
			else
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL(DLC), TXT( "Rewards XML file '%ls' not present." ), filename.AsChar() );
			}
		}
	}
}

void CR4JournalEntriesDLCMounter::Deactivate()
{
	const CDLCDefinition* dlcDefinition = Cast< CDLCDefinition >( GetParent() );
	if ( dlcDefinition )
	{
		CWitcherJournalManager* journalManager = GCommonGame->GetSystem< CWitcherJournalManager >();
		if( journalManager )
		{
			journalManager->UnloadInitialEntriesFile( dlcDefinition->GetID() );
		}
	}
}
