#include "build.h"
#include "../../common/core/depot.h"
#include "../../common/core/scriptingSystem.h"
#include "../../common/game/factsDB.h"
#include "../../common/game/quest.h"
#include "../../common/game/storySceneVoiceTagsManager.h"
#include "dlcQuestMounter.h"
#include "r4QuestSystem.h"
#include "questGraphWalker.h"

IMPLEMENT_ENGINE_CLASS( CR4QuestDLCMounter );

RED_DEFINE_STATIC_NAME( LoadQuestLevels );
RED_DEFINE_STATIC_NAME( UnloadQuestLevels );

CR4QuestDLCMounter::CR4QuestDLCMounter():
	m_sceneVoiceTagsTableLoaded( false )
{
}

bool CR4QuestDLCMounter::OnCheckContentUsage()
{
	if ( !m_taintFact.Empty() && m_quest.IsValid() )
	{
		auto* factsDB = GCommonGame->GetSystem< CFactsDB >();;
		return factsDB && factsDB->DoesExist( m_taintFact.AsChar() );
	}

	// we don't depend on this quest somehow...
	return false;
}

void CR4QuestDLCMounter::OnGameStarting()
{
	if( !m_questLevelsFilePath.Empty() )
	{
		if( GDepot->FileExist( m_questLevelsFilePath ) )
		{
			CallFunction( this, CNAME( LoadQuestLevels ), m_questLevelsFilePath );
		}
	}	

	LoadVoicetagTable();
}

void CR4QuestDLCMounter::OnGameStarted()
{
	if ( m_quest.IsValid() )
	{
		CQuestStartBlock* startBlock = m_quest->GetInput();
		if ( startBlock != nullptr )
		{
			auto* questSystem = GCommonGame->GetSystem< CR4QuestSystem >();
			if ( questSystem != nullptr )
			{
				//! quest can already running on load save 
				if( questSystem->IsQuestRunning( m_quest ) == false )
				{
#ifndef NO_EDITOR_GRAPH_SUPPORT
					//! in develop environment quest started from DLC can be already running form default game definition
					//! in this situation CR4QuestDLCMounter will not start second instance of DLC quest
					{
// 						PROFILER_Start();
// 						PROFILER_NextFrame();
						{
							
/*							PC_SCOPE( questSystem_IsDuplicate );*/
							if( questSystem->IsDuplicate( m_quest ) ) 
							{
								return;	
							}
						}	
// 						PROFILER_NextFrame();
// 						PROFILER_Stop();
					}
						
#endif //! NO_EDITOR_GRAPH_SUPPORT
					questSystem->Run( m_quest, startBlock );
				}				
			}
		}
	}
}

void CR4QuestDLCMounter::OnGameEnding()
{
	if( !m_questLevelsFilePath.Empty() )
	{
		if( GDepot->FileExist( m_questLevelsFilePath ) )
		{
			IScriptable* gpGame = GScriptingSystem->GetGlobal( CScriptingSystem::GP_GAME );
			if( gpGame != nullptr ) //! script game object can be unloaded on this stage 
			{
				CallFunction( this, CNAME( UnloadQuestLevels ), m_questLevelsFilePath );
			}
		}
	}	

	UnloadVoicetagTable();
}

#ifndef NO_EDITOR

void CR4QuestDLCMounter::OnEditorStarted()
{
	LoadVoicetagTable();
}

void CR4QuestDLCMounter::OnEditorStopped()
{
	UnloadVoicetagTable();
}

bool CR4QuestDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{
	// analyze

	if( !m_questLevelsFilePath.Empty() )
	{
		if( GDepot->FileExist( m_questLevelsFilePath ) )
		{
			StringAnsi filePathAnsi = UNICODE_TO_ANSI( m_questLevelsFilePath.AsChar() );
			outputList.AddFile( filePathAnsi );
		}		
	}	

	if( !m_sceneVoiceTagsTableFilePath.Empty() )
	{
		if( GDepot->FileExist( m_sceneVoiceTagsTableFilePath ) )
		{
			StringAnsi filePathAnsi = UNICODE_TO_ANSI( m_sceneVoiceTagsTableFilePath.AsChar() );
			outputList.AddFile( filePathAnsi );
		}		
	}

	if ( m_quest )
	{
		CQuestGraphWalker walker;
		CTimeCounter timer;

		LOG_GAME( TXT("Analyzing content distribution in quest '%ls'..."), m_quest->GetDepotPath().AsChar() );
		walker.WalkQuest( m_quest );

		// add additional files to the bundles
		walker.EmitToSeedFile( outputList );
		LOG_GAME( TXT("Quest content analyzed in %1.2fs"), timer.GetTimePeriod() );
	}
	return true;
}

#endif // !NO_EDITOR

void CR4QuestDLCMounter::LoadVoicetagTable()
{
	if( !m_sceneVoiceTagsTableFilePath.Empty() )
	{
		if( GDepot->FileExist( m_sceneVoiceTagsTableFilePath ) )
		{
			m_sceneVoiceTagsTableLoaded = SStorySceneVoiceTagsManager::GetInstance().AddVoiceTagsTable( m_sceneVoiceTagsTableFilePath );
			if( m_sceneVoiceTagsTableLoaded )
			{
				SStorySceneVoiceTagsManager::GetInstance().ReloadVoiceTags();
			}
		}
	}
}

void CR4QuestDLCMounter::UnloadVoicetagTable()
{
	if( m_sceneVoiceTagsTableLoaded )
	{
		if( SStorySceneVoiceTagsManager::GetInstance().RemVoiceTagsTable( m_sceneVoiceTagsTableFilePath ) )
		{
			SStorySceneVoiceTagsManager::GetInstance().ReloadVoiceTags();
		}
		m_sceneVoiceTagsTableLoaded = false;
	}
}
