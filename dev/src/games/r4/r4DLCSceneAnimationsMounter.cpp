#include "build.h"
#include "r4DLCSceneAnimationsMounter.h"
#include "../../common/core/depot.h"
#include "../../common/game/storySceneSystem.h"
#include "../../common/game/storySceneAnimationList.h"

IMPLEMENT_ENGINE_CLASS( CR4SceneAnimationsDLCMounter );

#define SCENE_ANIMATIONS_BODY_TABLE_LOADED				1
#define SCENE_ANIMATIONS_MIMICS_TABLE_LOADED			2
#define SCENE_ANIMATIONS_MIMICS_EMO_STATES_TABLE_LOADED 4

CR4SceneAnimationsDLCMounter::CR4SceneAnimationsDLCMounter():
	m_sceneAnimationsTableLoaded( 0 )
{
}

bool CR4SceneAnimationsDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4SceneAnimationsDLCMounter::OnGameStarting()
{	
	Activate();
}

void CR4SceneAnimationsDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4SceneAnimationsDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4SceneAnimationsDLCMounter::OnEditorStopped()
{
	Deactivate();
}

bool CR4SceneAnimationsDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{
	// analyze
	Bool status = true;
	if( !m_sceneAnimationsBodyFilePath.Empty() )
	{
		if( GDepot->FileExist( m_sceneAnimationsBodyFilePath ) )
		{
			StringAnsi filePathAnsi = UNICODE_TO_ANSI( m_sceneAnimationsBodyFilePath.AsChar() );
			outputList.AddFile( filePathAnsi );
			status &= IGameplayDLCMounter::DoAnalyzeCSV( outputList, m_sceneAnimationsBodyFilePath );
		}
	}

	if( !m_sceneAnimationsMimicsFilePath.Empty() )
	{
		if( GDepot->FileExist( m_sceneAnimationsMimicsFilePath ) )
		{
			StringAnsi filePathAnsi = UNICODE_TO_ANSI( m_sceneAnimationsMimicsFilePath.AsChar() );
			outputList.AddFile( filePathAnsi );
			status &= IGameplayDLCMounter::DoAnalyzeCSV( outputList, m_sceneAnimationsMimicsFilePath );
		}
	}

	if( !m_sceneAnimationsMimicsEmoStatesFilePath.Empty() )
	{
		if( GDepot->FileExist( m_sceneAnimationsMimicsEmoStatesFilePath ) )
		{
			StringAnsi filePathAnsi = UNICODE_TO_ANSI( m_sceneAnimationsMimicsEmoStatesFilePath.AsChar() );
			outputList.AddFile( filePathAnsi );
			status &= IGameplayDLCMounter::DoAnalyzeCSV( outputList, m_sceneAnimationsMimicsEmoStatesFilePath );
		}
	}
	return status;
}
#endif // !NO_EDITOR

void CR4SceneAnimationsDLCMounter::Activate()
{
	if( !m_sceneAnimationsBodyFilePath.Empty() )
	{
		if( GDepot->FileExist( m_sceneAnimationsBodyFilePath ) )
		{
			if( SSceneAnimationsResourcesManager::GetInstance().LoadSceneAnimationsBody2dArray( m_sceneAnimationsBodyFilePath ) )
			{
				m_sceneAnimationsTableLoaded |= SCENE_ANIMATIONS_BODY_TABLE_LOADED;
			}
		}		
	}
	if( !m_sceneAnimationsMimicsFilePath.Empty() )
	{
		if( GDepot->FileExist( m_sceneAnimationsMimicsFilePath ) )
		{
			if( SSceneAnimationsResourcesManager::GetInstance().LoadSceneAnimationsMimics2dArray( m_sceneAnimationsMimicsFilePath ) )
			{
				m_sceneAnimationsTableLoaded |= SCENE_ANIMATIONS_MIMICS_TABLE_LOADED;
			}
		}		
	}
	if( !m_sceneAnimationsMimicsEmoStatesFilePath.Empty() )
	{
		if( GDepot->FileExist( m_sceneAnimationsMimicsEmoStatesFilePath ) )
		{
			if( SSceneAnimationsResourcesManager::GetInstance().LoadSceneAnimationsMimicsEmoStates2dArray( m_sceneAnimationsMimicsEmoStatesFilePath ) )
			{
				m_sceneAnimationsTableLoaded |= SCENE_ANIMATIONS_MIMICS_EMO_STATES_TABLE_LOADED;
			}
		}		
	}
	GCommonGame->GetSystem< CStorySceneSystem >()->ReloadAnimationsList();
}

void CR4SceneAnimationsDLCMounter::Deactivate()
{
	if( m_sceneAnimationsTableLoaded & SCENE_ANIMATIONS_BODY_TABLE_LOADED )
	{
		SSceneAnimationsResourcesManager::GetInstance().UnloadSceneAnimationsBody2dArray( m_sceneAnimationsBodyFilePath );			
		m_sceneAnimationsTableLoaded ^= SCENE_ANIMATIONS_BODY_TABLE_LOADED;	
	}
	if( m_sceneAnimationsTableLoaded & SCENE_ANIMATIONS_MIMICS_TABLE_LOADED )
	{
		SSceneAnimationsResourcesManager::GetInstance().UnloadSceneAnimationsMimics2dArray( m_sceneAnimationsMimicsFilePath );			
		m_sceneAnimationsTableLoaded ^= SCENE_ANIMATIONS_MIMICS_TABLE_LOADED;	
	}
	if( m_sceneAnimationsTableLoaded & SCENE_ANIMATIONS_MIMICS_EMO_STATES_TABLE_LOADED )
	{
		SSceneAnimationsResourcesManager::GetInstance().UnloadSceneAnimationsMimicsEmoStates2dArray( m_sceneAnimationsMimicsEmoStatesFilePath );			
		m_sceneAnimationsTableLoaded ^= SCENE_ANIMATIONS_MIMICS_EMO_STATES_TABLE_LOADED;	
	}
	GCommonGame->GetSystem< CStorySceneSystem >()->ReloadAnimationsList();
}
