#include "build.h"
#include "r4DLCActionPointCategoriesMounter.h"
#include "../../common/core/depot.h"
#include "../../common/game/actionPoint.h"

IMPLEMENT_ENGINE_CLASS( CR4ActionPointCategoriesDLCMounter );

CR4ActionPointCategoriesDLCMounter::CR4ActionPointCategoriesDLCMounter():
	m_actionPointCategoriesTableLoaded( false )
{
}

bool CR4ActionPointCategoriesDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4ActionPointCategoriesDLCMounter::OnGameStarting()
{	
	Activate();
}

void CR4ActionPointCategoriesDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4ActionPointCategoriesDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4ActionPointCategoriesDLCMounter::OnEditorStopped()
{
	Deactivate();
}

bool CR4ActionPointCategoriesDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{	
	// analyze

	if( !m_actionPointCategoriesTableFilePath.Empty() )
	{
		if( GDepot->FileExist( m_actionPointCategoriesTableFilePath ) )
		{
			StringAnsi filePathAnsi = UNICODE_TO_ANSI( m_actionPointCategoriesTableFilePath.AsChar() );
			outputList.AddFile( filePathAnsi );		
		}		
	}
	return false;
}

#endif // !NO_EDITOR

void CR4ActionPointCategoriesDLCMounter::Activate()
{
	if( !m_actionPointCategoriesTableFilePath.Empty() )
	{
		if( GDepot->FileExist( m_actionPointCategoriesTableFilePath ) )
		{
			m_actionPointCategoriesTableLoaded = SActionPointResourcesManager::GetInstance().Load2dArray( m_actionPointCategoriesTableFilePath );
		}
	}
}

void CR4ActionPointCategoriesDLCMounter::Deactivate()
{
	if( m_actionPointCategoriesTableLoaded )
	{
		SActionPointResourcesManager::GetInstance().Unload2dArray( m_actionPointCategoriesTableFilePath );
		m_actionPointCategoriesTableLoaded = false;
	}
}
