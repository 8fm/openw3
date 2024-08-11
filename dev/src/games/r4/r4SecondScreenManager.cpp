
/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if !defined( NO_SECOND_SCREEN )
#include "r4SecondScreenManager.h"
#include "r4JournalManager.h"
#include "../../common/game/gameWorld.h"
#include "../../common/game/commonGame.h"

IMPLEMENT_ENGINE_CLASS( CR4SecondScreenManager );

CR4SecondScreenManager::CR4SecondScreenManager()
{
	CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();
	if( journalMgr != NULL )
	{
		journalMgr->RegisterEventListener( this );
	}
	
}

CR4SecondScreenManager::~CR4SecondScreenManager()
{
	CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();
	if( journalMgr != NULL )
	{
		journalMgr->UnregisterEventListener( this );
	}
}

void CR4SecondScreenManager::OnGameStart( const CGameInfo& gameInfo )
{
	if( gameInfo.m_isChangingWorldsInGame == false )
	{
		if( gameInfo.m_gameLoadStream != NULL )
		{		
			OnLoadGame( gameInfo.m_gameLoadStream );
		}
		else
		{
			SCSecondScreenManager::GetInstance().SendState( CSecondScreenManager::GS_GAME_SESSION );
			RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Send info on game start." ) );
		}
	}
}

void CR4SecondScreenManager::OnGameEnd( const CGameInfo& gameInfo )
{
	if( gameInfo.m_isChangingWorldsInGame == false )
	{
		SCSecondScreenManager::GetInstance().SendState( CSecondScreenManager::GS_NONE );
		RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Send info on game end." ) );
	}
}

void CR4SecondScreenManager::OnWorldStart( const CGameInfo& gameInfo )
{
	String worldName = gameInfo.m_worldFileToLoad;
	if( worldName.Empty()  ) //! gameInfo.m_worldFileToLoad is empty if game session is loaded from Editor 
	{
		worldName = GCommonGame->GetActiveWorld()->GetDepotPath();
	}
	SCSecondScreenManager::GetInstance().SendChangeArea( worldName );
	RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Send info on world start." ) );
}

void CR4SecondScreenManager::OnWorldEnd( const CGameInfo& gameInfo )
{
	SCSecondScreenManager::GetInstance().SendChangeArea( String::EMPTY );
	RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Send info on world end." ) );
}


bool CR4SecondScreenManager::OnSaveGame( IGameSaver* saver )
{
	RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Send info on game save." ) );
	return true;
}

void CR4SecondScreenManager::OnLoadGame( IGameLoader* loader )
{
	SCSecondScreenManager::GetInstance().SendState( CSecondScreenManager::GS_GAME_SESSION );
	RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "Send info on game load." ) );
}

void CR4SecondScreenManager::SendJournalStatus( CJournalBase* questEntry, EJournalStatus status, Bool silent ) 
{
#ifdef SECOND_SCREEN_PROFILE_CODE
	PC_SCOPE( R4SS_SendJournalStatus );
#endif
	if( questEntry->IsA< CJournalQuestGroup >() )
	{
		CJournalQuestGroup* journalQuestGroup = static_cast<CJournalQuestGroup*>(questEntry);
		SCSecondScreenManager::GetInstance().SendJournalQuestGroupStatus( 
			questEntry->GetGUID()
			, questEntry->GetUniqueScriptIdentifier()
			, questEntry->GetName()
			, journalQuestGroup->GetTitleIndex()
			, status );
	}
	else if( questEntry->IsA< CJournalQuest >() )
	{
		CJournalQuest* journalQuest = static_cast<CJournalQuest*>(questEntry);
		SCSecondScreenManager::GetInstance().SendJournalQuestStatus( 
			questEntry->GetGUID()
			, journalQuest->GetParentGUID()
			, questEntry->GetUniqueScriptIdentifier()
			, questEntry->GetName()
			, journalQuest->GetType()
			, journalQuest->GetWorld()
			, journalQuest->GetTitleIndex()
			, status
			, silent );
	}
	else if( questEntry->IsA< CJournalQuestPhase >() )
	{
		CJournalQuestPhase* journalQuestPhase = static_cast<CJournalQuestPhase*>(questEntry);
		SCSecondScreenManager::GetInstance().SendJournalQuestPhaseStatus( 
			questEntry->GetGUID()
			, journalQuestPhase->GetParentGUID()
			, questEntry->GetUniqueScriptIdentifier()
			, questEntry->GetName()
			, status );
	}
	else if( questEntry->IsA< CJournalQuestObjective >() )
	{
		CJournalQuestObjective* journalQuestObjective = static_cast<CJournalQuestObjective*>(questEntry);
		SCSecondScreenManager::GetInstance().SendJournalQuestObjectiveStatus( 
			questEntry->GetGUID()
			, journalQuestObjective->GetParentGUID()
			, questEntry->GetUniqueScriptIdentifier()
			, questEntry->GetName()
			, journalQuestObjective->GetTitleIndex()
			, journalQuestObjective->GetImage()
			, journalQuestObjective->GetWorld()
			, journalQuestObjective->GetCounterType()
			, journalQuestObjective->GetCount()
			, status );
		Uint32 childrenCount = journalQuestObjective->GetNumChildren();
		for ( Uint32 childIndex = 0; childIndex < childrenCount; ++childIndex )
		{
			SendJournalStatus( journalQuestObjective->GetChild( childIndex ), status, silent ); 			
		}
	}
	else if( questEntry->IsA< CJournalQuestItemTag >() )
	{
		CJournalQuestItemTag* journalQuestItemTag = static_cast<CJournalQuestItemTag*>(questEntry);
		SCSecondScreenManager::GetInstance().SendJournalQuestItemTagStatus( 
			questEntry->GetGUID()
			, journalQuestItemTag->GetParentGUID()
			, questEntry->GetUniqueScriptIdentifier()
			, questEntry->GetName()
			, journalQuestItemTag->GetItem()
			, status 
			);
	}
	else if( questEntry->IsA< CJournalQuestEnemyTag >() )
	{
		CJournalQuestEnemyTag* journalQuestEnemyTag = static_cast<CJournalQuestEnemyTag*>(questEntry);
		SCSecondScreenManager::GetInstance().SendJournalQuestEnemyTagStatus( 
			questEntry->GetGUID()
			, journalQuestEnemyTag->GetParentGUID()
			, questEntry->GetUniqueScriptIdentifier()
			, questEntry->GetName()
			, journalQuestEnemyTag->GetTag()
			, status 
			);
	}
	else if( questEntry->IsA< CJournalQuestMapPin >() )
	{
		CJournalQuestMapPin* journalQuestMapPin = static_cast<CJournalQuestMapPin*>(questEntry);
		SCSecondScreenManager::GetInstance().SendJournalQuestMapPinStatus( 
			questEntry->GetGUID()
			, journalQuestMapPin->GetParentGUID()
			, questEntry->GetUniqueScriptIdentifier()
			, questEntry->GetName()
			, journalQuestMapPin->GetMapPinID() 
			, status 
			);
	}
	else if( questEntry->IsA< CJournalQuestDescriptionGroup >() )
	{
		CJournalQuestDescriptionGroup* journalQuestDescriptionGroup = static_cast<CJournalQuestDescriptionGroup*>(questEntry);
		SCSecondScreenManager::GetInstance().SendJournalQuestDescriptionGroupStatus( 
			questEntry->GetGUID()
			, journalQuestDescriptionGroup->GetParentGUID()
			, questEntry->GetUniqueScriptIdentifier()
			, questEntry->GetName()
			, status 
			);
	}
	else if( questEntry->IsA< CJournalQuestDescriptionEntry >() )
	{
		CJournalQuestDescriptionEntry* journalQuestDescriptionEntry = static_cast<CJournalQuestDescriptionEntry*>(questEntry);
		SCSecondScreenManager::GetInstance().SendJournalQuestDescriptionEntryStatus( 
			questEntry->GetGUID()
			, journalQuestDescriptionEntry->GetParentGUID()
			, questEntry->GetUniqueScriptIdentifier()
			, questEntry->GetName()
			, journalQuestDescriptionEntry->GetDescriptionIndex() 
			, status 
			);
	}
}
void CR4SecondScreenManager::OnJournalEvent( const SW3JournalQuestStatusEvent& event )
{
	const CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();

	if ( event.m_quest == journalMgr->GetTrackedQuest() )
	{
		//! set actual quest
		SCSecondScreenManager::GetInstance().SendJournalTrackedQuestUpdated( event.m_quest->GetGUID() );
	}
}

void CR4SecondScreenManager::OnJournalEvent( const SW3JournalObjectiveStatusEvent& event )
{
	CObject* ancestor = event.m_objective->GetParent();
	while ( ancestor && ! ancestor->IsA< CJournalQuest >() )
	{
		ancestor = ancestor->GetParent();
	}

	ASSERT( ancestor && ancestor->IsA< CJournalQuest >() );
	CJournalQuest* quest = Cast< CJournalQuest >( ancestor );

	const CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();

	if ( quest == journalMgr->GetTrackedQuest() )
	{
		//! set tracked quest objective
		SCSecondScreenManager::GetInstance().SendJournalTrackedQuestObjectiveUpdated( event.m_objective->GetGUID() );
	}
}
void CR4SecondScreenManager::OnJournalEvent( const SW3JournalTrackEvent& event )
{
	const CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();

	if ( event.m_quest == journalMgr->GetTrackedQuest() )
	{
		//! set actual quest
		SCSecondScreenManager::GetInstance().SendJournalTrackedQuestUpdated( event.m_quest->GetGUID() );
	}	
}

void CR4SecondScreenManager::OnJournalEvent( const SW3JournalQuestTrackEvent& event )
{
	// supposedly not used
	RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "SW3JournalQuestTrackEvent" ) );
}



void CR4SecondScreenManager::OnJournalEvent( const SW3JournalQuestObjectiveTrackEvent& event )
{
	CObject* ancestor = event.m_objective->GetParent();
	while ( ancestor && ! ancestor->IsA< CJournalQuest >() )
	{
		ancestor = ancestor->GetParent();
	}

	ASSERT( ancestor && ancestor->IsA< CJournalQuest >() );
	CJournalQuest* quest = Cast< CJournalQuest >( ancestor );

	const CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();

	if ( quest == journalMgr->GetTrackedQuest() )
	{
		//! set tracked quest objective
		SCSecondScreenManager::GetInstance().SendJournalTrackedQuestObjectiveUpdated( event.m_objective->GetGUID() );
	}
}

void CR4SecondScreenManager::OnJournalEvent( const SW3JournalQuestObjectiveCounterTrackEvent& event )
{
	//! set tracked quest objective
	RED_LOG( RED_LOG_CHANNEL( SecondScreen ), TXT( "SW3JournalQuestObjectiveCounterTrackEvent" ) );
}

void CR4SecondScreenManager::OnJournalEvent( const SW3JournalHighlightEvent& event )
{
}

void CR4SecondScreenManager::Initialize()
{
	SCSecondScreenManager::GetInstance().Init();
	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
}

void CR4SecondScreenManager::Shutdown()
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );
	SCSecondScreenManager::GetInstance().Shutdown();
}

#endif //NO_SECOND_SCREEN

