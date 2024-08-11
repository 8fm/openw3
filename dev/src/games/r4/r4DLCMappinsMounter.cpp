#include "build.h"
#include "r4DLCMappinsMounter.h"
#include "../../common/core/depot.h"
#include "../../common/game/dlcDefinition.h"
#include "commonMapManager.h"

IMPLEMENT_ENGINE_CLASS( CR4MappinsDLCMounter );

CR4MappinsDLCMounter::CR4MappinsDLCMounter()
{
}

bool CR4MappinsDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4MappinsDLCMounter::OnGameStarting()
{
	// nothing
}

void CR4MappinsDLCMounter::OnGameStarted()
{	
	Activate();
}

void CR4MappinsDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4MappinsDLCMounter::OnEditorStarted()
{
}

void CR4MappinsDLCMounter::OnEditorStopped()
{
}

bool CR4MappinsDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{	
	const String& filename = m_mappinsFilePath.GetPath();
	if( !filename.Empty() )
	{
		if( GDepot->FileExist( filename ) )
		{
			const StringAnsi actualDepotPathAnsi( UNICODE_TO_ANSI( filename.AsChar() ) );
			outputList.AddFile( actualDepotPathAnsi );
		}
	}

	const String& filename2 = m_questMappinsFilePath.GetPath();
	if( !filename2.Empty() )
	{
		if( GDepot->FileExist( filename2 ) )
		{
			const StringAnsi actualDepotPathAnsi( UNICODE_TO_ANSI( filename2.AsChar() ) );
			outputList.AddFile( actualDepotPathAnsi );
		}
	}
	return true;
}

#endif // !NO_EDITOR

void CR4MappinsDLCMounter::Activate()
{
	const CDLCDefinition* dlcDefinition = Cast< CDLCDefinition >( GetParent() );
	if ( dlcDefinition )
	{
		const String& filename = m_mappinsFilePath.GetPath();
		if( !filename.Empty() )
		{
			if( GDepot->FileExist( filename ) )
			{
				CCommonMapManager* manager = GCommonGame->GetSystem< CCommonMapManager >();
				if ( manager )
				{
					manager->LoadEntityMapPinsForDLC( dlcDefinition->GetID(), m_worldFilePath.GetPath(), m_mappinsFilePath.GetPath() );
				}
			}
			else
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL(DLC), TXT( "Mappin resource file '%ls' not present." ), filename.AsChar() );
			}
		}
		const String& filename2 = m_questMappinsFilePath.GetPath();
		if( !filename2.Empty() )
		{
			if( GDepot->FileExist( filename2 ) )
			{
				CCommonMapManager* manager = GCommonGame->GetSystem< CCommonMapManager >();
				if ( manager )
				{
					manager->LoadQuestMapPinsForDLC( dlcDefinition->GetID(), m_worldFilePath.GetPath(), m_questMappinsFilePath.GetPath() );
				}
			}
			else
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL(DLC), TXT( "Mappin resource file '%ls' not present." ), filename.AsChar() );
			}
		}
	}

}

void CR4MappinsDLCMounter::Deactivate()
{
	// not required, it's reloaded when game is started or loaded from save
}
