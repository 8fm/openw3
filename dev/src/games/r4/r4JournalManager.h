#pragma once

#include "../../common/game/journalManager.h"

#include "journal.h"
#include "buffImmunity.h"

// ----------------------------------------------------------------------------------

struct SJournalQuestObjectiveData
{
	DECLARE_RTTI_STRUCT( SJournalQuestObjectiveData );

	EJournalStatus	m_status;
	THandle<CJournalQuestObjective> m_objectiveEntry;
};

BEGIN_CLASS_RTTI( SJournalQuestObjectiveData )
	PROPERTY( m_status )
	PROPERTY( m_objectiveEntry ) 
END_CLASS_RTTI()

struct SJournalCreatureParams
{
	DECLARE_RTTI_STRUCT( SJournalCreatureParams );

	TDynArray< CName >					m_abilities;
	TDynArray< CName >					m_autoEffects;
	CBuffImmunity						m_buffImmunity;
	Int32								m_monsterCategory;
	Bool								m_isTeleporting;
	TDynArray< CName >					m_droppedItems;
};

BEGIN_CLASS_RTTI( SJournalCreatureParams )
	PROPERTY( m_abilities )
	PROPERTY( m_autoEffects )
	PROPERTY( m_buffImmunity )
	PROPERTY( m_monsterCategory )
	PROPERTY( m_isTeleporting )
	PROPERTY( m_droppedItems )
END_CLASS_RTTI();

// ----------------------------------------------------------------------------------

class CWitcherJournalManager : public CJournalManager
{
	DECLARE_ENGINE_CLASS( CWitcherJournalManager, CJournalManager, 0 )

private:
	typedef THashMap< CGUID, Int32 >			TObjectiveCounters;			// Maps Quest Objective CGUIDs -> [Manual/Kill/Item] Count
	

	TObjectiveCounters						m_objectiveCounters;

	Uint32									m_trackedQuest;					//< Index into m_activeEntries
	TEntryGroup								m_trackedQuestObjectives;
	Uint32									m_highlightedQuest;
	Uint32									m_highlightedQuestObjective;
	Uint32									m_cachedDisplayNameIndexForSave;

	// Hunting Clues	
	THashMap< CName, TDynArray< TEntryGroup > > m_clueToCreatureMap;		// Enum Name ("Clue Category") -> Enum Value ("Clue Index") -> List of creature entries with that clue
	THashMap< CGUID, TEntryGroup >				m_foundHuntingQuestClues;	// Items contained in the entry group are found clues

	// Cached stats
	Int32									m_regularQuestCount;
	Int32									m_monsterHuntQuestCount;
	Int32									m_treasureHuntQuestCount;
	Int32									m_ep1QuestCount;
	Int32									m_ep2QuestCount;

	Float									m_tickInterval;

	THashSet< CGUID >						m_knownMonsterQuests;
	THashSet< CGUID >						m_entriesWithAdvancedInformation;

	// debug stuff
	Int32									m_debugInfo;
	Bool									m_debugVideo;

public:
	CWitcherJournalManager();
	virtual ~CWitcherJournalManager();
	virtual void OnGenerateDebugFragments( CRenderFrame* frame );
	virtual void Tick( Float timeDelta );

	// --- Interface methods ---
protected:
	virtual void OnGameStart( const CGameInfo& gameInfo );
	virtual void OnGameEnd( const CGameInfo& gameInfo );
	virtual Bool OnSaveGame( IGameSaver* saver );
	void OnLoadGame( IGameLoader* loader );
	virtual Uint32 GetDisplayNameIndexForSavedGame() override;
	virtual Bool CanActivateEntry( const CJournalBase* target, EJournalStatus status ) override;

public:
	virtual void OnInitGuiDuringGameLoad();
	virtual void OnInitGuiDuringWorldChange();

	// --- Journal Entry Callbacks --
protected:
	virtual void EntryStatusChanged( CJournalBase* questEntry, EJournalStatus oldStatus, EJournalStatus newStatus, Bool silent );

	// --- Journal Manager interface methods ---
protected:
	virtual void TypeSpecificActivation( CJournalBase* entry, Uint32 index );

	// --- Quests ---
public:
	void GetQuestGroups( TDynArray< const CJournalQuestGroup* >& questGroups /*[out]*/ ) const;
	void GetQuests( const CJournalQuestGroup* parent, TDynArray< const CJournalQuest* >& quests /*[out]*/ ) const;
	void GetQuestPhases( const CJournalQuest* parent, TDynArray< const CJournalQuestPhase* >& questPhases /*[out]*/ ) const;
	void GetQuestObjectives( const CGUID& phaseGUID, TDynArray< const CJournalQuestObjective* >& questObjectives /*[out]*/ ) const;
	void GetTrackedQuestObjectives( TDynArray< const CJournalQuestObjective* >& objectives /*[out]*/ ) const;
	void GetStoryQuests( TDynArray< const CJournalQuest* >& quests /*[out]*/ ) const;
	
	Int32 GetQuestObjectiveCount( const CGUID& guid ) const
	{
		Int32 count = 0;
		if( m_objectiveCounters.Find( guid, count ) )
		{
			return count;
		}
		return 0;
	}

	void SetQuestObjectiveCount( const CGUID& guid, Int32 newCount )
	{
		Int32* count = m_objectiveCounters.FindPtr( guid );

		ASSERT( count && "Objective has not been activated yet" );
		if( count )
		{
			*count = newCount;
		}
	}

	Bool IsTrackedQuest( const CJournalQuest* quest ) const;
	Bool IsTrackedObjective( const CJournalQuestObjective* quest ) const;
	Bool CanTrackQuest( const CJournalQuest* quest ) const;
	const CJournalQuest* GetTrackedQuest() const;
	void TrackQuest( const CJournalQuest* quest, Bool manualTracking );
	void TrackQuest( const CGUID& questGUID, Bool manualTracking );
	void UpdateTrackedObjectives();
	void UpdateQuestObjectiveCounter( const CGUID& objective, Int32 count, Bool silent );

	void SetQuestMonsterAvailable(bool value, const CGUID& questID);
	void SetHasAdvancedInformation(bool value, const CGUID& entryID);

	const CJournalQuest* GetHighlightedQuest() const;
	const CJournalQuestObjective* GetHighlightedObjective() const;
	Bool HighlightObjective( const CJournalQuestObjective* objective, Bool onLoad = false, Bool fromScripts = false );
	Bool HighlightObjective( const CGUID& objectiveGUID, Bool onLoad = false, Bool fromScripts = false );
	Bool HighlightPrevNextObjective( Bool next );

	// Hunting Quest
	void ActivateHuntingQuestClue( THandle< CJournalPath > pathToHuntingTag, THandle< CJournalPath > pathToCreatureClue );

	const TDynArray< Uint32 >& GetHuntingQuestClues( THandle< CJournalPath > pathToQuest );
	void GetCreaturesWithHuntingQuestClue( const CName& categoryName, Uint32 clueIndex, TDynArray< const CJournalCreature* >& creatures /*[out]*/ ) const;

	const CJournalStoryBookPage* GetStoryBookPage( Uint32 pageIndex ) const;
	Uint32 GetNumStoryBookPages() const
	{
		const TEntryGroup* group = m_activeCategoryGroups.FindPtr( ClassID< CJournalStoryBookPage >() );
		if( group )
		{
			return group->Size();
		}

		return 0;
	}

	void GetStoryBookChapters( TDynArray< const CJournalStoryBookChapter* >& chapters /*[out]*/ ) const;
	void GetStoryBookPages( const CJournalStoryBookChapter* parent, TDynArray< const CJournalStoryBookPage* >& pages /*[out]*/ ) const;
	String GetCurrentStorybookVideoName( Int32 area = 0 ) const;
	const CName& GetCurrentLoadingScreenName() const;

	void LoadInitialEntriesFile( const CName& dlcId, const String& filename );
	void UnloadInitialEntriesFile( const CName& dlcId );
	Uint32 GetQuestProgress() const;

	void GetActiveQuestObjectives( TDynArray< const CJournalQuestObjective* >& activeObjectives ) const;

	static String GetInitialEntriesPathAndFilename(  Uint32 epIndex  );
#ifndef NO_EDITOR
	static String GetInitialEntriesFilename(  Uint32 epIndex );
#endif //NO_EDITOR

private:
	void SendTrackEvent();
	void SendHighlightEvent();
	void ActivateHuntingQuestClue( CJournalQuest* quest, CJournalCreatureHuntingClue* clue );

	const TEntryGroup* GetTrackedEntryGroup() const
	{
		return &m_trackedQuestObjectives;
	}

	void GetQuestRewards( const String& questPath, TDynArray< CName >& rewards );

protected:
	Bool IsQuestTracked( Uint32 activeEntriesIndex );
	Bool IsObjectiveTracked( Uint32 activeEntriesIndex );
	RED_INLINE Bool IsEntryValid( Uint32 index ) const	{  return index != INVALID_ENTRY_INDEX;  }
	RED_INLINE void InvalidateEntry( Uint32& index )		{  index = INVALID_ENTRY_INDEX;  }
	const CJournalQuest* FindQuestToTrack();

	// -- Script interface --
private:
	void funcSetTrackedQuest( CScriptStackFrame& stack, void* result );
	void funcGetTrackedQuest( CScriptStackFrame& stack, void* result );
	void funcGetHighlightedQuest( CScriptStackFrame& stack, void* result );
	void funcGetHighlightedObjective( CScriptStackFrame& stack, void* result );
	void funcSetHighlightedObjective( CScriptStackFrame& stack, void* result );
	void funcSetPrevNextHighlightedObjective( CScriptStackFrame& stack, void* result );
	void funcGetCreaturesWithHuntingQuestClue( CScriptStackFrame& stack, void* result );
	void funcGetNumberOfCluesFoundForQuest( CScriptStackFrame& stack, void* result );
	void funcGetAllCluesFoundForQuest( CScriptStackFrame& stack, void* result );
	void funcSetHuntingClueFoundForQuest( CScriptStackFrame& stack, void* result );
	void funcGetTrackedQuestObjectivesData( CScriptStackFrame& stack, void* result );

	void funcGetQuestHasMonsterKnown( CScriptStackFrame& stack, void* result );
	void funcSetQuestHasMonsterKnown( CScriptStackFrame& stack, void* result );

	void funcGetEntryHasAdvancedInfo( CScriptStackFrame& stack, void* result );
	void funcSetEntryHasAdvancedInfo( CScriptStackFrame& stack, void* result );

	void funcGetQuestObjectiveCount( CScriptStackFrame& stack, void* result );
	void funcSetQuestObjectiveCount( CScriptStackFrame& stack, void* result );

	void funcGetCreatureParams( CScriptStackFrame& stack, void* result );

	void funcToggleDebugInfo( CScriptStackFrame& stack, void* result );
	void funcShowLoadingScreenVideo( CScriptStackFrame& stack, void* result );

	void funcGetQuestRewards( CScriptStackFrame& stack, void* result );
	void funcGetRegularQuestCount( CScriptStackFrame& stack, void* result );
	void funcGetMonsterHuntQuestCount( CScriptStackFrame& stack, void* result );
	void funcGetTreasureHuntQuestCount( CScriptStackFrame& stack, void* result );
	void funcGetQuestProgress( CScriptStackFrame& stack, void* result );
	void funcGetJournalAreasWithQuests( CScriptStackFrame& stack, void* result );

	void funcForceSettingLoadingScreenVideoForWorld( CScriptStackFrame& stack, void* result );
	void funcForceSettingLoadingScreenContextNameForWorld( CScriptStackFrame& stack, void* result );

	void funcForceUntrackingQuestForEP1Savegame( CScriptStackFrame& stack, void* result );

	ASSIGN_GAME_SYSTEM_ID( GS_Journal );
};

BEGIN_CLASS_RTTI( CWitcherJournalManager )
	PARENT_CLASS( CJournalManager )
	NATIVE_FUNCTION( "SetTrackedQuest", funcSetTrackedQuest )
	NATIVE_FUNCTION( "GetTrackedQuest", funcGetTrackedQuest )

	NATIVE_FUNCTION( "GetHighlightedQuest", funcGetHighlightedQuest )
	NATIVE_FUNCTION( "GetHighlightedObjective", funcGetHighlightedObjective )
	NATIVE_FUNCTION( "SetHighlightedObjective", funcSetHighlightedObjective )
	NATIVE_FUNCTION( "SetPrevNextHighlightedObjective", funcSetPrevNextHighlightedObjective )

	NATIVE_FUNCTION( "GetQuestHasMonsterKnown", funcGetQuestHasMonsterKnown )
	NATIVE_FUNCTION( "SetQuestHasMonsterKnown", funcSetQuestHasMonsterKnown )

	NATIVE_FUNCTION( "GetEntryHasAdvancedInfo", funcGetEntryHasAdvancedInfo )
	NATIVE_FUNCTION( "SetEntryHasAdvancedInfo", funcSetEntryHasAdvancedInfo )

	NATIVE_FUNCTION( "GetQuestObjectiveCount", funcGetQuestObjectiveCount )
	NATIVE_FUNCTION( "SetQuestObjectiveCount", funcSetQuestObjectiveCount )

	NATIVE_FUNCTION( "GetCreaturesWithHuntingQuestClue", funcGetCreaturesWithHuntingQuestClue )
	NATIVE_FUNCTION( "GetNumberOfCluesFoundForQuest", funcGetNumberOfCluesFoundForQuest )
	NATIVE_FUNCTION( "GetAllCluesFoundForQuest", funcGetAllCluesFoundForQuest )
	NATIVE_FUNCTION( "SetHuntingClueFoundForQuest", funcSetHuntingClueFoundForQuest )

	NATIVE_FUNCTION( "GetTrackedQuestObjectivesData", funcGetTrackedQuestObjectivesData );

	NATIVE_FUNCTION( "GetCreatureParams", funcGetCreatureParams );

	NATIVE_FUNCTION( "ToggleDebugInfo", funcToggleDebugInfo );
	NATIVE_FUNCTION( "ShowLoadingScreenVideo", funcShowLoadingScreenVideo );

	NATIVE_FUNCTION( "GetQuestRewards", funcGetQuestRewards );
	NATIVE_FUNCTION( "GetRegularQuestCount", funcGetRegularQuestCount );
	NATIVE_FUNCTION( "GetMonsterHuntQuestCount", funcGetMonsterHuntQuestCount );
	NATIVE_FUNCTION( "GetTreasureHuntQuestCount", funcGetTreasureHuntQuestCount );
	NATIVE_FUNCTION( "GetQuestProgress", funcGetQuestProgress );

	NATIVE_FUNCTION( "GetJournalAreasWithQuests", funcGetJournalAreasWithQuests )

	NATIVE_FUNCTION( "ForceSettingLoadingScreenVideoForWorld", funcForceSettingLoadingScreenVideoForWorld )
	NATIVE_FUNCTION( "ForceSettingLoadingScreenContextNameForWorld", funcForceSettingLoadingScreenContextNameForWorld )

	NATIVE_FUNCTION( "ForceUntrackingQuestForEP1Savegame", funcForceUntrackingQuestForEP1Savegame )

END_CLASS_RTTI();
