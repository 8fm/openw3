/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#if !defined( NO_SECOND_SCREEN )

#include "../../common/platformCommon/secondScreenManager.h"
#include "../../common/game/journalManager.h"
#include "journalEvents.h"

class CR4SecondScreenManager: public IGameSystem, public IGameSaveSection, public IW3JournalEventListener
{
private:

	DECLARE_ENGINE_CLASS( CR4SecondScreenManager, IGameSystem, 0 )

public:
	CR4SecondScreenManager();
	~CR4SecondScreenManager();

	virtual void Initialize() override;
	virtual void Shutdown() override;

	void SendJournalStatus( CJournalBase* questEntry, EJournalStatus status, Bool silent );

	//! IGameSystem
private:

	void OnGameStart( const CGameInfo& gameInfo );
	void OnGameEnd( const CGameInfo& gameInfo );

	void OnWorldStart( const CGameInfo& gameInfo );
	void OnWorldEnd( const CGameInfo& gameInfo );

	void Tick( Float timeDelta ){}

	//! IGameSaveSection
private:

	bool OnSaveGame( IGameSaver* saver );
	void OnLoadGame( IGameLoader* loader );

	//! IW3JournalEventListener
public:
	virtual void OnJournalEvent( const SW3JournalQuestStatusEvent& event );
	virtual void OnJournalEvent( const SW3JournalObjectiveStatusEvent& event );
	virtual void OnJournalEvent( const SW3JournalTrackEvent& event );
	virtual void OnJournalEvent( const SW3JournalQuestTrackEvent& event );
	virtual void OnJournalEvent( const SW3JournalQuestObjectiveTrackEvent& event );
	virtual void OnJournalEvent( const SW3JournalQuestObjectiveCounterTrackEvent& event );
	virtual void OnJournalEvent( const SW3JournalHighlightEvent& event );

protected:
	ASSIGN_GAME_SYSTEM_ID( GS_SecondScreen )
};


BEGIN_CLASS_RTTI( CR4SecondScreenManager )
	PARENT_CLASS( IGameSystem )
END_CLASS_RTTI();

#endif //NO_SECOND_SCREEN