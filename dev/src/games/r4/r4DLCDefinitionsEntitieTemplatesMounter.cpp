#include "build.h"
#include "r4DLCDefinitionsEntitieTemplatesMounter.h"
#include "../../common/core/depot.h"

IMPLEMENT_ENGINE_CLASS( CR4DefinitionsEntitieTemplatesDLCMounter );

CR4DefinitionsEntitieTemplatesDLCMounter::CR4DefinitionsEntitieTemplatesDLCMounter()
	: m_directoryLoaded( false )
{
}

bool CR4DefinitionsEntitieTemplatesDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4DefinitionsEntitieTemplatesDLCMounter::OnGameStarting()
{	
	Activate();
}

void CR4DefinitionsEntitieTemplatesDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4DefinitionsEntitieTemplatesDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4DefinitionsEntitieTemplatesDLCMounter::OnEditorStopped()
{
	Deactivate();
}

bool CR4DefinitionsEntitieTemplatesDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{	
	if( !m_entitieTemplatesDirectoryPath.Empty() )
	{
		CDirectory * directory = GDepot->FindPath( m_entitieTemplatesDirectoryPath.AsChar() );
		if( directory != NULL )
		{
			CDefinitionsManager definitionManager;

			//! remove default loaded item
			CDirectory * defaultItemsDirectory = GDepot->FindPath( TXT("items\\") );
			if( defaultItemsDirectory != NULL )
			{
				definitionManager.RemoveTemplateNamesRecursive( defaultItemsDirectory );
			}

			definitionManager.LoadTemplateNamesRecursive( directory );

			// Get template files
			TDynArray< String > itemTemplates;
			definitionManager.GetTemplateFilesList( itemTemplates );	

			// Add templates to the bundle list
			for( Uint32 j = 0; j < itemTemplates.Size(); j++ )
			{
				const String& depotPath = itemTemplates[j];

				// find actual depot file
				CDiskFile* file = GDepot->FindFileUseLinks( depotPath, 0 );
				if ( file )
				{
					const StringAnsi actualDepotPath( UNICODE_TO_ANSI( file->GetDepotPath().AsChar() ) );
					outputList.AddFile( actualDepotPath );
				}
			}
			return true;
		}
	}
	return false;
}

#endif // !NO_EDITOR

void CR4DefinitionsEntitieTemplatesDLCMounter::Activate()
{
	if( !m_entitieTemplatesDirectoryPath.Empty() )
	{
		CDirectory * directory = GDepot->FindPath( m_entitieTemplatesDirectoryPath.AsChar() );
		if( directory != NULL )
		{
			GCommonGame->GetDefinitionsManager()->LoadTemplateNamesRecursive( directory );
			m_directoryLoaded = true;
		}
	}
}

void CR4DefinitionsEntitieTemplatesDLCMounter::Deactivate()
{
	if( m_directoryLoaded )
	{
		CDirectory* directory = GDepot->FindPath( m_entitieTemplatesDirectoryPath.AsChar() );
		if( directory != NULL )
		{
			GCommonGame->GetDefinitionsManager()->RemoveTemplateNamesRecursive( directory );
			m_directoryLoaded = false;
		}
	}
}
