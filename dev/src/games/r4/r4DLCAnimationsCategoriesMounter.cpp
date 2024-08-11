#include "build.h"
#include "r4DLCAnimationsCategoriesMounter.h"
#include "../../common/core/depot.h"
#include "../../common/game/jobTreeLeaf.h"

IMPLEMENT_ENGINE_CLASS( CR4AnimationsCategoriesDLCMounter );

CR4AnimationsCategoriesDLCMounter::CR4AnimationsCategoriesDLCMounter():
	m_animationsCategoriesTableLoaded( false )
{
}

bool CR4AnimationsCategoriesDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4AnimationsCategoriesDLCMounter::OnGameStarting()
{	
	Activate();
}

void CR4AnimationsCategoriesDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4AnimationsCategoriesDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4AnimationsCategoriesDLCMounter::OnEditorStopped()
{
	Deactivate();
}

bool CR4AnimationsCategoriesDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{	
	// analyze
	Bool status = true;
	if( !m_animationsCategoriesTableFilePath.Empty() )
	{
		if( GDepot->FileExist( m_animationsCategoriesTableFilePath ) )
		{	
			StringAnsi filePathAnsi = UNICODE_TO_ANSI( m_animationsCategoriesTableFilePath.AsChar() );
			outputList.AddFile( filePathAnsi );		
			status = IGameplayDLCMounter::DoAnalyzeCSV( outputList, m_animationsCategoriesTableFilePath );
		}		
	}
	return status;
}

#endif // !NO_EDITOR

void CR4AnimationsCategoriesDLCMounter::Activate()
{
	if( !m_animationsCategoriesTableFilePath.Empty() )
	{
		if( GDepot->FileExist( m_animationsCategoriesTableFilePath ) )
		{
			m_animationsCategoriesTableLoaded = SAnimationsCategoriesResourcesManager::GetInstance().Load2dArray( m_animationsCategoriesTableFilePath );
		}
	}
}

void CR4AnimationsCategoriesDLCMounter::Deactivate()
{
	if( m_animationsCategoriesTableLoaded )
	{
		SAnimationsCategoriesResourcesManager::GetInstance().Unload2dArray( m_animationsCategoriesTableFilePath );
		m_animationsCategoriesTableLoaded = false;
	}
}
