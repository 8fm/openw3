#include "build.h"
#include "questsSystem.h"
#include "questThread.h"
#include "questsAnalyzer.h"

#include "storySceneInput.h"
#include "questScenePlayer.h"
#include "questExternalScenePlayer.h"

#include "questGraphBlock.h"
#include "questInteractionCondition.h"
#include "questReactionCondition.h"
#include "questCutsceneCondition.h"
#include "questUserInputCondition.h"
#include "questStartBlock.h"
#include "storyScenePlayer.h"
#include "storySceneSystem.h"
#include "quest.h"
#include "questScopeBlock.h"
#include "commonGameResource.h"
#include "factsDB.h"

#include "../../common/core/depot.h"
#include "../core/gameSave.h"
#include "storySceneItems.h"
#include "questGraphInstance.h"
#include "questGraph.h"
#include "../core/resource.h"

IMPLEMENT_ENGINE_CLASS( CQuestsSystem )

CQuestsSystem::CQuestsSystem()
	: m_active( false )
	, m_stable( false )
	, m_latchThreadCountStable( false )
	, m_pauseCount( 0 )
	, m_deadPhaseHackfixCounter( 0 )
{
}

CQuestsSystem::~CQuestsSystem()
{
}

void CQuestsSystem::Initialize()
{
	ASSERT( !m_scenesManager && !m_interactionsDialogPlayer && !m_scriptedDialogPlayer );

	m_scenesManager = new CQuestScenesManager();
	m_interactionsDialogPlayer = new CQuestExternalScenePlayer();
	m_scriptedDialogPlayer = new CQuestExternalScenePlayer();

	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
}

void CQuestsSystem::Shutdown()
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );

	delete m_scenesManager;
	m_scenesManager = NULL;

	delete m_interactionsDialogPlayer;
	m_interactionsDialogPlayer = NULL;

	delete m_scriptedDialogPlayer;
	m_scriptedDialogPlayer = NULL;
}

void CQuestsSystem::Activate()
{
	if ( m_active )
	{
		Deactivate();
	}

	m_active = true;
}

void CQuestsSystem::Deactivate()
{
	StopAll();
	
	ASSERT( m_threads.Empty() );
	ASSERT( m_threadsToAdd.Empty() );
	ASSERT( m_threadsToRemove.Empty() );
	ASSERT( m_threadsToStabilize.Empty() );
	ASSERT( m_questsMap.Empty() );

	m_questsMap.Clear();
	m_contextDialogs.Clear();
	m_interactionListeners.Clear();
	m_reactionListeners.Clear();
	m_cutscenesListener.Clear();
	m_inputListener.Clear();

	RED_ASSERT( GGame && GGame->GetInputManager() );
	GGame->GetInputManager()->UnregisterListener( this );	

	m_interactionsDialogPlayer->Activate( m_active );

	m_scenesManager->Reset();
	m_active = false;
}

void CQuestsSystem::Pause( bool enable ) 
{ 
	if ( enable ) 
	{
		++m_pauseCount;
	}
	else
	{
		m_pauseCount = 0;
	}

	NotifySystemPaused();

}

Bool CQuestsSystem::IsQuestRunning( const THandle< CQuest >& quest )
{
	return GetQuestThread( quest ) != nullptr;
}

CQuestThread* CQuestsSystem::GetQuestThread( const THandle< CQuest >& quest )
{
	const String questName = quest->GetFileName();
	for ( QuestsMap::const_iterator it = m_questsMap.Begin(); it != m_questsMap.End(); ++it )
	{
		if ( it->m_second->GetFileName() == questName )
		{
			return it->m_first;
		}
	}
	return nullptr;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
Bool CQuestsSystem::IsDuplicate( const THandle< CQuest >& quest ) const
{
	TDynArray< CGUID > allGUIDs;
	quest->GetGraph()->GetAllGUIDs( allGUIDs );
		
	TDynArray< CGUID > allRunningGUIDs;
	for ( QuestsMap::const_iterator it = m_questsMap.Begin(); it != m_questsMap.End(); ++it )
	{
		it->m_second->GetGraph()->GetAllGUIDs( allRunningGUIDs );		
	}
	
	TDynArray< CGUID > duplicatedGuids;
	TDynArray< CGUID >::const_iterator endGUIDs = allGUIDs.End();
	for ( TDynArray< CGUID >::const_iterator guidIter = allGUIDs.Begin(); guidIter != endGUIDs; ++guidIter )
	{
		if( allRunningGUIDs.FindPtr( *guidIter ) ) 
		{
			duplicatedGuids.PushBack( *guidIter );
		}
	}

	if ( duplicatedGuids.Size() > 0 )
	{
		RED_LOG_WARNING( Quests, TXT( "Duplicated GUIDs found! May cause problems with DLC mounting.") );
		for ( const CGUID& guid : duplicatedGuids )
		{
			RED_LOG_WARNING( Quests, RED_GUID_STRING_FORMAT, guid.parts.A, guid.parts.B, guid.parts.C, guid.parts.D );
		}
		return true;
	}
	return false;
}
#endif //! NO_EDITOR_GRAPH_SUPPORT

CQuestThread* CQuestsSystem::Run( const THandle< CQuest >& quest, CQuestStartBlock* startBlock )
{
	// check if the quest is already running
	CQuestThread* questThread = GetQuestThread( quest );
	if ( questThread )
	{
		ASSERT( Find( m_runningQuests.Begin(), m_runningQuests.End(), quest ) != m_runningQuests.End() );
		WARN_GAME( TXT( "Quest %s is already running"), quest->GetFileName().AsChar() );
		return questThread;
	}

	// Pin this quest to this quest system, so the garbage collector can't throw it out 
	m_runningQuests.PushBackUnique( quest );

	// start the quest in a dedicated thread
	CQuestThread* thread = CreateObject< CQuestThread >( this );
	thread->Activate( NULL, *quest->GetGraph() );
	thread->ActivateBlock( *startBlock, CName::NONE );

	m_threadsToAdd.PushBack( thread );
	m_questsMap.Insert( thread, quest );
	
	return thread;
}

void CQuestsSystem::Stop( CQuestThread* thread )
{
	if ( thread == NULL ) 
	{
		return;
	}

	// check if the thread is registered as scheduled or active -only then it's elegible for the destruction
	Bool oneOfOurs = false;
	if ( Find( m_threadsToAdd.Begin(), m_threadsToAdd.End(), thread ) != m_threadsToAdd.End() )
	{
		oneOfOurs = true;
	}
	
	if ( oneOfOurs || ( Find( m_threads.Begin(), m_threads.End(), thread ) != m_threads.End() ) )
	{
		oneOfOurs = true;
	}

	if ( oneOfOurs )
	{
		thread->Kill();
		RemoveThread( thread );
	}
}

void CQuestsSystem::StopAll()
{
	ManageThreads();
	Uint32 count = m_threads.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		Stop( m_threads[ i ] );
	}
	ManageThreads();
}

void CQuestsSystem::RemoveThread( CQuestThread* thread )
{
	if ( m_threadsToAdd.Remove( thread ) == false )
	{
		m_threadsToRemove.PushBack( thread );
	}
}

void CQuestsSystem::GetAllRunningThreads( TDynArray< CQuestThread* >& threads ) const
{
	TDynArray< CQuestThread* > threadsQueue;
	for ( TDynArray< CQuestThread* >::const_iterator it = m_threads.Begin(); it != m_threads.End(); ++it )
	{
		threadsQueue.PushBack( *it );
	}

	while( !threadsQueue.Empty() )
	{
		CQuestThread* currThread = threadsQueue.PopBack();
		threads.PushBack( currThread );

		const TDynArray< CQuestThread* >& children = currThread->GetChildrenThreads();
		for ( TDynArray< CQuestThread* >::const_iterator it = children.Begin(); it != children.End(); ++it )
		{
			threadsQueue.PushBack( *it );
		}
	}
}

void CQuestsSystem::Tick( Float timeDelta )
{
	m_latchThreadCountStable = true;

	if ( !m_active || IsPaused() )
	{
		return;
	}

	PC_SCOPE_PIX( QuestsSystem );
	ManageThreads();

	Uint32 count = m_threads.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_threads[ i ]->Tick();
	}

	Bool allThreadsStable = true;
	for ( Int32 j = m_threadsToStabilize.SizeInt()-1; j >= 0; --j )
	{
		const Bool threadStable = m_threadsToStabilize[ j ]->IsStable();

		allThreadsStable &= threadStable;
		if ( threadStable )
		{
			m_threadsToStabilize.RemoveAt( j );
		}
	}

	m_stable = allThreadsStable && m_latchThreadCountStable;

	m_scenesManager->Tick();
}

void CQuestsSystem::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	ManageThreads();

	Uint32 count = m_threads.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		file << m_threads[ i ];
	}

	if( file.IsGarbageCollector() )
	{
		for( auto &  iter : m_contextDialogs )
		{
			if( iter.m_injectedScene.IsLoaded() )
			{
				auto handle = iter.m_injectedScene.Get();
				file << handle;
			}

			if( iter.m_targetScene.IsLoaded() )
			{
				auto handle = iter.m_targetScene.Get();
				file << handle;
			}
		}
	}
}

void CQuestsSystem::ManageThreads()
{
	Uint32 count = m_threadsToAdd.Size();
	if ( count > 0 )
	{
		m_latchThreadCountStable = false;
	}
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_threadsToStabilize.PushBack( m_threadsToAdd[ i ] );
		m_threads.PushBack( m_threadsToAdd[ i ] );
		NotifyQuestStarted( m_threadsToAdd[ i ] );

		// attach listeners to children
		for ( TDynArray< IQuestSystemListener* >::iterator it = m_listeners.Begin();
			it != m_listeners.End(); ++it )
		{
			m_threadsToAdd[ i ]->AttachListener( **it );
		}
	}
	m_threadsToAdd.Clear();

	count = m_threadsToRemove.Size();
	if ( count > 0 )
	{
		m_latchThreadCountStable = false;
	}
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_threadsToStabilize.Remove( m_threadsToRemove[ i ] );
		m_threads.Remove( m_threadsToRemove[ i ] );

		QuestsMap::iterator it = m_questsMap.Find( m_threadsToRemove[ i ] ); 
		if ( m_questsMap.End() != it )
		{
			CQuest* quest = it->m_second;
			m_questsMap.Erase( it );
			RemoveQuestIfNotUsed( quest );
		}
		

		// detach listeners from children
		for ( TDynArray< IQuestSystemListener* >::iterator it = m_listeners.Begin();
			it != m_listeners.End(); ++it )
		{
			m_threadsToRemove[ i ]->DetachListener( **it );
		}

		NotifyQuestStopped( m_threadsToRemove[ i ] );

		m_threadsToRemove[ i ]->Discard();
	}
	m_threadsToRemove.Clear();
}

void CQuestsSystem::RemoveQuestIfNotUsed( CQuest* quest )
{
	for ( QuestsMap::const_iterator it = m_questsMap.Begin(); it != m_questsMap.End(); ++it )
	{
		if ( it->m_second == quest )
		{
			return;
		}
	}

	m_runningQuests.RemoveFast( quest );
}

void CQuestsSystem::AttachListener( IQuestSystemListener& listener )
{
	m_listeners.PushBackUnique( &listener );

	Uint32 count = m_threads.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		QuestsMap::iterator it = m_questsMap.Find( m_threads[ i ] );
		listener.OnQuestStarted( m_threads[ i ], *it->m_second );

		// attach listener to children
		m_threads[ i ]->AttachListener( listener );
	}

	listener.OnSystemPaused( IsPaused() );
}

void CQuestsSystem::DetachListener( IQuestSystemListener& listener )
{
	m_listeners.Remove( &listener );

	// detach listener from children
	Uint32 count = m_threads.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_threads[ i ]->DetachListener( listener );
	}
}

void CQuestsSystem::RegisterContextDialog( TSoftHandle< CStoryScene > injectedScene, TSoftHandle< CStoryScene > targetScene, IStoryScenePlaybackListener* listener, const THashMap< String, SSceneInjectedChoiceLineInfo >& choices )
{
	if ( choices.Empty() )
	{
		return;
	}

	const SDialogInfo toAdd( injectedScene, targetScene, listener );
	if( Find( m_contextDialogs.Begin(), m_contextDialogs.End(), toAdd ) != m_contextDialogs.End() )
	{
		return;
	}

	m_contextDialogs.PushBack( toAdd );
	SDialogInfo& dialogInfo = m_contextDialogs.Back();
	dialogInfo.m_choices = choices;
}

void CQuestsSystem::UnregisterContextDialog( TSoftHandle< CStoryScene > injectedScene, TSoftHandle< CStoryScene > targetScene, IStoryScenePlaybackListener* listener )
{
	const SDialogInfo toRemove( injectedScene, targetScene, listener );

	Bool found( false );
	const Int32 size = m_contextDialogs.SizeInt();
	for ( Int32 i=size-1; i>=0; --i )
	{
		const SDialogInfo& si = m_contextDialogs[ i ];
		if ( si == toRemove )
		{
			RED_FATAL_ASSERT( !found, "CQuestsSystem::UnregisterContextDialog" );
			if ( listener )
			{
				RED_FATAL_ASSERT( si.m_listener == toRemove.m_listener, "CQuestsSystem::UnregisterContextDialog" );
			}

			m_contextDialogs.RemoveAt( i );
			found = true;
		}
	}

	RED_ASSERT( found );
}

void CQuestsSystem::GetContextDialogChoices( TDynArray< SSceneChoice >& choices, const CStoryScene* targetScene, TDynArray< IStoryScenePlaybackListener* >* listeners, const CStorySceneChoice* targetChoice )
{
	Uint32 count = m_contextDialogs.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( m_contextDialogs[ i ].m_targetScene.GetPath() != targetScene->GetDepotPath() )
		{
			continue;
		}

		// Yes, we load scene in sync mode... TODO
		//RED_FATAL_ASSERT( m_contextDialogs[ i ].m_injectedScene.IsLoaded(), "CQuestsSystem::GetContextDialogChoices - m_injectedScene.IsLoaded()" );
		const CStoryScene* scene = m_contextDialogs[ i ].m_injectedScene.Get();
		if ( scene == NULL )
		{
			continue;
		}
		// inputs
		TDynArray< CStorySceneInput* > sceneInputs;
		scene->CollectControlParts< CStorySceneInput >( sceneInputs );

		Uint32 count = sceneInputs.Size();
		Bool choiceInjected = false;
		for ( Uint32 j = 0; j < count; ++j )
		{
			const SSceneInjectedChoiceLineInfo* description = m_contextDialogs[ i ].m_choices.FindPtr( sceneInputs[ j ]->GetName() );
			if ( description )
			{
				Uint32 orderNumber = choices.Size() + ( ( description->m_emphasisLine == false ) ? 100 : 0 );

				SSceneChoice choiceData;
				choiceData.link = sceneInputs[ j ];
				choiceData.m_description = description->m_choiceLine.GetString();
				choiceData.m_order = orderNumber;
				choiceData.m_emphasised = description->m_emphasisLine;
				choiceData.m_choiceToReturnTo = description->m_returnToChoice ? targetChoice : nullptr;
				choiceData.m_dialogAction = description->GetChoiceActionIcon();
				choiceData.m_disabled = description->IsDisabled();
				choiceData.m_playGoChunk = CName( TXT("content0") );
				choiceData.m_injectedChoice = true;
				choices.PushBack( choiceData );
				choiceInjected = true;
			}
		}

		if ( choiceInjected == true && listeners )
		{
			listeners->PushBack( m_contextDialogs[ i ].m_listener );
		}
		
	}
}

namespace
{
	Bool AlreadyMapped( CName id, TDynArray< TPair< const CStorySceneActor*, const CStorySceneVoicetagMapping* > >& mappings )
	{
		for ( TPair< const CStorySceneActor*, const CStorySceneVoicetagMapping* >& mappedPair : mappings )
		{
			if( mappedPair.m_first->m_id == id )
			{
				return true;
			}
		}
		return false;
	}
}

void CQuestsSystem::GetContextDialogsForScene( THandle< CStoryScene > targetSceneHandle, TDynArray< THandle< CStoryScene > >& outScenes ) const 
{
	CStoryScene* targetScene = targetSceneHandle.Get();
	for ( Uint32 i = 0; i < m_contextDialogs.Size(); ++i )
	{
		if ( m_contextDialogs[ i ].m_targetScene.GetPath() == targetScene->GetDepotPath()  )
		{
			SCENE_ASSERT__FIXME_LATER( m_contextDialogs[ i ].m_injectedScene.IsLoaded() );

			CStoryScene* injectedScene = m_contextDialogs[ i ].m_injectedScene.Get();
			if ( injectedScene )
			{
				outScenes.PushBackUnique( injectedScene );
			}
		}
	}
}

Bool CQuestsSystem::ShouldWaitForContextDialogs( const CStoryScene* targetScene ) const
{
	const String targetScenePath = targetScene->GetDepotPath();

	for ( Uint32 i = 0; i < m_contextDialogs.Size(); ++i )
	{
		const TSoftHandle< CStoryScene >& contextScene = m_contextDialogs[ i ].m_targetScene;
		if ( contextScene.GetPath() == targetScenePath )
		{
			// Slow call
			if ( !contextScene.IsLoaded() && contextScene.IsLoading() )
			{
				return true;
			}
		}
	}

	return false;
}

void CQuestsSystem::GetContextDialogActorData( CStoryScene* targetScene, TDynArray< TPair< const CStorySceneActor*, const CStorySceneVoicetagMapping* > >& out )
{
	Int32 indOfDialog = -1;
	for ( Uint32 i = 0; i < m_contextDialogs.Size(); ++i )
	{
		if ( m_contextDialogs[ i ].m_targetScene.GetPath() == targetScene->GetDepotPath()  )
		{
			indOfDialog = (Int32)i;

			// Yes, we load scene in sync mode... TODO
			//RED_FATAL_ASSERT( m_contextDialogs[ i ].m_injectedScene.IsLoaded(), "CQuestsSystem::GetContextDialogActorData - m_contextDialogs[ i ].m_injectedScene.IsLoaded()" );
			SCENE_ASSERT( m_contextDialogs[ i ].m_injectedScene.IsLoaded() );

			CStoryScene* injectedScene = m_contextDialogs[ i ].m_injectedScene.Get();
			if ( injectedScene )
			{
				TDynArray< CStorySceneInput* > sceneInputs;
				injectedScene->CollectControlParts< CStorySceneInput >( sceneInputs );

				Uint32 count = sceneInputs.Size();
				Bool choiceInjected = false;
				for ( Uint32 j = 0; j < count; ++j )
				{
					const SSceneInjectedChoiceLineInfo* description = m_contextDialogs[ indOfDialog ].m_choices.FindPtr( sceneInputs[ j ]->GetName() );
					if ( description )
					{
						const TDynArray< CStorySceneActor* >& actorDefinitions = injectedScene->GetSceneActorsDefinitions();
						const TDynArray< CStorySceneVoicetagMapping >& actorMappings = sceneInputs[ j ]->GetVoicetagMappings();

						for ( TDynArray< CStorySceneVoicetagMapping >::const_iterator mappingIter = actorMappings.Begin(); mappingIter != actorMappings.End(); ++mappingIter )
						{
							if ( mappingIter->m_voicetag == CName::NONE || AlreadyMapped(  mappingIter->m_voicetag, out ) )
							{
								continue;
							}

							for ( TDynArray< CStorySceneActor* >::const_iterator definitionIter = actorDefinitions.Begin(); definitionIter != actorDefinitions.End(); ++definitionIter )
							{
								const CStorySceneActor* actorDef = (*definitionIter);
								if ( actorDef && actorDef->m_id == mappingIter->m_voicetag )
								{
									out.PushBack( TPair< const CStorySceneActor*, const CStorySceneVoicetagMapping* >( actorDef, &(*mappingIter) ) );
									break;
								}
							}
						}
					}
				}
			}
		}
	}
}

void CQuestsSystem::AttachInteractionListener( CQuestInteractionCondition& listener )
{
	m_interactionListeners.PushBackUnique( &listener );
}

void CQuestsSystem::DetachInteractionListener( CQuestInteractionCondition& listener )
{
	m_interactionListeners.Remove( &listener );
}

void CQuestsSystem::OnInteractionExecuted( const String& eventName, CEntity* owner )
{
	Uint32 count = m_interactionListeners.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_interactionListeners[ i ]->OnInteraction( eventName, owner );
	}
}

void CQuestsSystem::AttachReactionListener( CQuestReactionCondition& listener )
{
	m_reactionListeners.PushBackUnique( &listener );
}

void CQuestsSystem::DetachReactionListener( CQuestReactionCondition& listener )
{
	m_reactionListeners.Remove( &listener );
}

void CQuestsSystem::OnReactionExecuted( CNewNPC* npc, CInterestPointInstance* interestPoint )
{
	Uint32 count = m_reactionListeners.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_reactionListeners[ i ]->OnReaction( npc, interestPoint );
	}
}

Bool CQuestsSystem::AreThereAnyDialogsForActor( const CActor& actor )
{
	return m_interactionsDialogPlayer->AreThereAnyDialogsForActor( actor );
}

void CQuestsSystem::AttachCutsceneListener( CQuestCutsceneCondition& listener )
{
	m_cutscenesListener.PushBackUnique( &listener );
}

void CQuestsSystem::DetachCutsceneListener( CQuestCutsceneCondition& listener )
{
	m_cutscenesListener.Remove( &listener );
}

void CQuestsSystem::OnCutsceneEvent( const String& csName, const CName& csEvent )
{
	Uint32 count = m_cutscenesListener.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_cutscenesListener[ i ]->OnEvent( csName, csEvent );
	}
}

void CQuestsSystem::AttachGameInputListener( CQuestInputCondition& listener )
{
	m_inputListener.PushBackUnique( &listener );

	RED_ASSERT( GGame && GGame->GetInputManager() );
	GGame->GetInputManager()->RegisterListener( this, listener.GetGameInputName() );
}

void CQuestsSystem::DetachGameInputListener( CQuestInputCondition& listener )
{
	m_inputListener.Remove( &listener );

	if( m_inputListener.Empty() )
	{
		RED_ASSERT( GGame && GGame->GetInputManager() );
		GGame->GetInputManager()->UnregisterListener( this );
	}
}

Bool CQuestsSystem::OnGameInputEvent( const SInputAction & action )
{
	Uint32 count = m_inputListener.Size();

	for ( Uint32 i = 0; i < count; ++i )
	{
		m_inputListener[ i ]->OnEvent( action.m_aName, action.m_value );
	}

	return true;
}

void CQuestsSystem::NotifySystemPaused()
{
	Bool paused = IsPaused();

	for ( TDynArray< IQuestSystemListener* >::iterator it = m_listeners.Begin();
		it != m_listeners.End(); ++it )
	{
		(*it)->OnSystemPaused( paused );
	}
}

void CQuestsSystem::NotifyQuestStarted( CQuestThread* thread )
{
	if ( m_listeners.Empty() )
	{
		return;
	}

	QuestsMap::iterator questIt = m_questsMap.Find( thread );
	for ( TDynArray< IQuestSystemListener* >::iterator it = m_listeners.Begin();
		it != m_listeners.End(); ++it )
	{
		(*it)->OnQuestStarted( thread, *questIt->m_second );
	}
}

void CQuestsSystem::NotifyQuestStopped( CQuestThread* thread )
{
	for ( TDynArray< IQuestSystemListener* >::iterator it = m_listeners.Begin();
		it != m_listeners.End(); ++it )
	{
		(*it)->OnQuestStopped( thread );
	}
}

void CQuestsSystem::OnGameStart( const CGameInfo& gameInfo )
{
	// Do not even start the quest system, if quests are not allowed in this game
	if ( false == gameInfo.m_allowQuestsToRun )
	{
		return;
	}

	if ( gameInfo.m_isChangingWorldsInGame == true )
	{
		Pause( false );

		for ( TDynArray< CQuestThread* >::iterator threadIter = m_threads.Begin();
			threadIter != m_threads.End(); ++threadIter )
		{
			(*threadIter)->OnNewWorldLoading( gameInfo.m_worldFileToLoad );
		}

		return;
	}

	// Initialize quest system
	Activate();

	// Load state 
	if ( gameInfo.m_gameLoadStream )
	{
		LoadGame( gameInfo.m_gameLoadStream );
	}
	else 
	{
		CCommonGameResource* gameResource = Cast< CCommonGameResource >( GGame->GetGameResource() );
		// ... lock loading screen till GGame->Fade( true ) is called from quest
		if ( gameResource == NULL )
		{
			ERR_GAME( TXT( "Game resource not found. Cannot run the main quest!!!" ) );
			return;
		}

		THandle< CQuest > mainQuest = gameResource->GetMainQuest().Get();
		if ( !mainQuest.IsValid() )
		{
			ERR_GAME( TXT( "Main quest is not defined. Cannot run the main quest!!!" ) );
			return;
		}
		
		Run( mainQuest, mainQuest->GetInput() );
	}
}

void CQuestsSystem::OnGameEnd( const CGameInfo& gameInfo )
{
}

void CQuestsSystem::OnWorldEnd( const CGameInfo& gameInfo )
{
	if ( gameInfo.m_isChangingWorldsInGame == false )
	{
		// Deactivate the quests system
		Deactivate();
	}
	else
	{
		Pause( true );
	}
}

Bool CQuestsSystem::OnSaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	CGameSaverBlock block( saver, CNAME(questSystem) );

	// Save hackfix counter
	saver->WriteValue( CNAME( count ), m_deadPhaseHackfixCounter );

	// save the external scene players state
	{
		CGameSaverBlock block( saver, CNAME(questExternalScenePlayers) );
		m_interactionsDialogPlayer->OnSaveGame( saver );
		m_scriptedDialogPlayer->OnSaveGame( saver );
	}

	// Update quests
	ManageThreads();

	// Count the number of threads to save
	Uint32 numQuests = 0;
	const Uint32 count = m_threads.Size();
	for ( Uint32 i=0; i<count; ++i )
	{
		QuestsMap::iterator it = m_questsMap.Find( m_threads[ i ] );
		if ( it != m_questsMap.End() )
		{
			++numQuests;
		}
	}

	// Save number of master threads
	saver->WriteValue( CNAME(numQuests), numQuests );

	// Save master threads
	for ( Uint32 i=0; i<count; ++i )
	{
		QuestsMap::iterator it = m_questsMap.Find( m_threads[ i ] );
		if ( it != m_questsMap.End() )
		{
			CQuest* quest = it->m_second;
			ASSERT( quest );

			// Save quest 
			{
				CGameSaverBlock block1( saver, CNAME(quest) );
				saver->WriteValue< String >( CNAME(fileName), quest->GetFile()->GetDepotPath() );

				// Save thread data
				m_threads[ i ]->SaveGame( saver );
			}
		}	
	}

	END_TIMER_BLOCK( time )

	// Saved
	return true;
}

void CQuestsSystem::LoadGame( IGameLoader* loader )
{
	CGameSaverBlock block0( loader, CNAME(questSystem) );

	m_deadPhaseHackfixCounter = 0;
	if ( loader->GetSaveVersion() >= SAVE_VERSION_DEAD_PHASE_HACKFIX )
	{
		// Load hackfix counter
		loader->ReadValue< Uint32 >( CNAME( count ), m_deadPhaseHackfixCounter );
	}

	// load the external scene players state
	{
		CGameSaverBlock block( loader, CNAME(questExternalScenePlayers) );
		m_interactionsDialogPlayer->OnLoadGame( loader );
		m_scriptedDialogPlayer->OnLoadGame( loader );
	}
	
	// Load number of active quests
	Uint32 numQuests = 0;
	loader->ReadValue( CNAME(numQuests), numQuests );

	// Load quests
	for ( Uint32 i=0; i<numQuests; i++ )
	{
		CGameSaverBlock block1( loader, CNAME(quest) );

		// Load file name
		String questFileName;
		loader->ReadValue< String >( CNAME(fileName), questFileName );

		// Restore quest
		THandle< CQuest > quest = LoadResource< CQuest >( questFileName );
		if ( quest )
		{
			CQuestStartBlock* startBlock = quest->GetInput();
			CQuestThread* questThread = Run( quest, startBlock );

			// Restore quest thread
			questThread->LoadGame( loader );
		}
	}

	m_interactionsDialogPlayer->ClearBrokenDialogs();
	m_scriptedDialogPlayer->ClearBrokenDialogs();


	CFactsDB* const factsDB = GCommonGame ? GCommonGame->GetSystem< CFactsDB >() : nullptr;
	if ( factsDB )
	{
		factsDB->AddFact( TXT( "game_is_loaded" ), 1, GGame->GetEngineTime(), 3 );
	}
}

Bool CQuestsSystem::IsNPCInQuestScene( const CNewNPC* npc ) const
{
	return false;
}

void CQuestsSystem::ResetStability()
{
	PC_SCOPE( CQuestsSystem_ResetStability );

	TDynArray< CQuestThread* > runningThreads;
	GetAllRunningThreads( runningThreads );
	for ( CQuestThread* thread : runningThreads )
	{
		thread->ResetStabilizedThreads();
	}
	m_threadsToStabilize = m_threads;
}

#ifndef RED_FINAL_BUILD
class CQuestsDebugDumper
{
private:
	struct SStackItem
	{
		CQuestThread*	m_questThread;
		Uint32			m_depth;

		SStackItem()
			: m_questThread( nullptr )
			, m_depth( 0 )
		{}

		SStackItem( CQuestThread* questThread, Uint32 depth )
			: m_questThread( questThread )
			, m_depth( depth )
		{}
	};

public:
	void DumpAllThreads() const
	{

		CQuestsSystem* questsSystem = GCommonGame ? GCommonGame->GetSystem<CQuestsSystem>() : nullptr;
		if ( !questsSystem )
		{
			ERR_GAME(TXT("CQuestsDebugDumper::DumpUnstableThreads: No quests system available!"));
			return;
		}

		TDynArray< SStackItem > threadsStack;
		for ( CQuestThread* thread : questsSystem->GetRunningThreads() )
		{
			threadsStack.PushBack( SStackItem( thread, 0 ) );
		}
		
		const Char* padding = TXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
		while( !threadsStack.Empty() )
		{
			const SStackItem curItem = threadsStack.PopBack();
			CQuestThread* const questThread = curItem.m_questThread;
		
			const CQuestGraph* questGraph = nullptr;
			if ( questThread->m_graph && questThread->m_graph->GetParentGraph() )
			{
				questGraph = questThread->m_graph->GetParentGraph();
			}

			const CObject* res = questGraph;
			while ( res && !res->IsA<CResource>() )
			{
				res = res->GetParent();
			}

			const String graphName = res && res->IsA< CResource >() ? static_cast< const CResource* >( res )->GetDepotPath() : ( questGraph ? questGraph->GetFriendlyName() :TXT("<<Null>>") );

			LOG_GAME(TXT("%.*lsThread: %ls | %ls"),
				curItem.m_depth, padding,
				questThread->GetName().AsChar(), 
				graphName.AsChar());
						
			TDynArray< const CQuestGraphBlock* > blocks;
			questThread->GetActiveBlocks( blocks );
#ifndef NO_EDITOR_GRAPH_SUPPORT
			for ( const CQuestGraphBlock* block : blocks )
			{
				LOG_GAME(TXT("%.*ls        Block: %30.30ls (%ls) | x=%d | y=%d"), 
					curItem.m_depth, padding,
					block->GetBlockName().AsChar(),
					block->GetClass()->GetName().AsChar(),
					(Int32)block->GetPosition().X,
					(Int32)block->GetPosition().Y );
			}
#endif

			const TDynArray< CQuestThread* >& children = questThread->GetChildrenThreads();
			for ( CQuestThread* childThread : children )
			{
				threadsStack.PushBack( SStackItem( childThread, curItem.m_depth + 1 ) );
			}
		}
	}

	void DumpUnstableThreads() const
	{
		CQuestsSystem* questsSystem = GCommonGame ? GCommonGame->GetSystem<CQuestsSystem>() : nullptr;
		if ( !questsSystem )
		{
			ERR_GAME(TXT("CQuestsDebugDumper::DumpUnstableThreads: No quests system available!"));
			return;
		}

		TDynArray< SStackItem > threadsStack;
		for ( CQuestThread* thread : questsSystem->GetRunningThreads() )
		{
			if ( !thread->IsStable() )
			{
				threadsStack.PushBack( SStackItem( thread, 0 ) );
			}
		}

		LOG_GAME(TXT("Quest threads: BCS = Block count table, NLB = No loading blocker, CTCS = Child thread count stable, CTS = Child threads stable"));
		LOG_GAME(TXT("Quest blocks: LB = Loading blocker"));
		const Char* padding = TXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
		while( !threadsStack.Empty() )
		{
			const SStackItem curItem = threadsStack.PopBack();
			CQuestThread* const questThread = curItem.m_questThread;
			const Helper::SStableTickData& tickData = questThread->m_stableTickData;
		
			const CQuestGraph* questGraph = nullptr;
			if ( questThread->m_graph && questThread->m_graph->GetParentGraph() )
			{
				questGraph = questThread->m_graph->GetParentGraph();
			}

			const CObject* res = questGraph;
			while ( res && !res->IsA<CResource>() )
			{
				res = res->GetParent();
			}
			
			const String graphName = res && res->IsA< CResource >() ? static_cast< const CResource* >( res )->GetDepotPath() : questGraph->GetFriendlyName();
			const Bool onlyChildrenUnstable = tickData.m_latchBlockCountStable && tickData.m_latchChildThreadCountStable && !tickData.m_latchChildThreadsStable;

			LOG_GAME(TXT("%.*ls%ls: %ls | BCS: %d | CTCS: %d | CTS: %d | Graph in: %ls"),
				curItem.m_depth, padding,
				onlyChildrenUnstable ? TXT("(Thread)") : TXT("Thread"),
				questThread->GetName().AsChar(), 
				tickData.m_latchBlockCountStable, 
				tickData.m_latchChildThreadCountStable,
				tickData.m_latchChildThreadsStable,
				graphName.AsChar());


#ifndef NO_EDITOR_GRAPH_SUPPORT
			if ( !tickData.m_latchBlockCountStable )
			{
				TDynArray< const CQuestGraphBlock* > blocks;
				questThread->GetActiveBlocks( blocks );

				for ( const CQuestGraphBlock* block : blocks )
				{
					LOG_GAME(TXT("%.*ls        Block: %30.30ls | x=%d | y=%d"), 
						curItem.m_depth, padding,
						block->GetBlockName().AsChar(),
						(Int32)block->GetPosition().X,
						(Int32)block->GetPosition().Y );
				}
			}
#endif

			const TDynArray< CQuestThread* >& children = questThread->GetChildrenThreads();
			for ( CQuestThread* childThread : children )
			{
				if ( !childThread->IsStable() )
				{
					threadsStack.PushBack( SStackItem( childThread, curItem.m_depth + 1 ) );
				}
			}
		}
	}
};

void DebugDumpUnstableQuestThreads()
{
	CQuestsDebugDumper dumper;
	dumper.DumpUnstableThreads();
};

void DebugDumpAllQuestThreads()
{
	CQuestsDebugDumper dumper;
	dumper.DumpAllThreads();
}

#endif // !RED_FINAL_BUILD