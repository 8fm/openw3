#include "build.h"
#include "r4JournalManager.h"
#include "journalEvents.h"
#include "r4GameResource.h"
#include "journalQuest.h"
#include "commonMapManager.h"
#include "../../common/core/depot.h"
#include "../../common/game/entityParams.h"
#include "../../common/game/questGraph.h"
#include "buffImmunity.h"
#include "monsterParam.h"
#include "r4LootDefinitions.h"
#include "r4Telemetry.h"
#if !defined( NO_SECOND_SCREEN )
#include "../../common/platformCommon/secondScreenManager.h"
#include "r4SecondScreenManager.h"
#endif

#include "../../common/core/dataError.h"
#include "../../common/core/gameSave.h"
#include "../../common/engine/gameSaveManager.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/utils.h"
#include "../../common/core/gatheredResource.h"

#define DEFAULT_TICK_INTERVAL 1.0f

IMPLEMENT_ENGINE_CLASS( SJournalCreatureParams )
IMPLEMENT_ENGINE_CLASS( CWitcherJournalManager );

// CNames used for serialisation
RED_DEFINE_STATIC_NAME( JObjectiveCounters );
RED_DEFINE_STATIC_NAME( JObjectiveCounter );
RED_DEFINE_STATIC_NAME( JTrackedQuest );
RED_DEFINE_STATIC_NAME( JHighlightedObjective );
RED_DEFINE_STATIC_NAME( JHuntingClues );
RED_DEFINE_STATIC_NAME( JHuntingClue );
RED_DEFINE_STATIC_NAME( JHuntingClueGuid );
RED_DEFINE_STATIC_NAME( JMonsterKnown );
RED_DEFINE_STATIC_NAME( JMonsterKnownGuid );
RED_DEFINE_STATIC_NAME( JEntryAdvancedInfo );
RED_DEFINE_STATIC_NAME( JEntryAdvancedInfoGuid );
RED_DEFINE_STATIC_NAME( JHuntingQuestGuid );
RED_DEFINE_STATIC_NAME( JStoryBook );
RED_DEFINE_STATIC_NAME( JSBLastPageRead );
RED_DEFINE_STATIC_NAME( CanTrackQuest )
RED_DEFINE_STATIC_NAME( ep1 )
RED_DEFINE_STATIC_NAME( ep2 )

IMPLEMENT_ENGINE_CLASS( SJournalQuestObjectiveData );

CGatheredResource resInitialJournalStats( TXT("gameplay\\journal\\start.w2je"), RGF_Startup );

CWitcherJournalManager::CWitcherJournalManager()
	: m_regularQuestCount( 0 )
	, m_monsterHuntQuestCount( 0 )
	, m_treasureHuntQuestCount( 0 )
	, m_ep1QuestCount( 0 )
	, m_ep2QuestCount( 0 )
	, m_trackedQuest( INVALID_ENTRY_INDEX )
	, m_highlightedQuest( INVALID_ENTRY_INDEX )
	, m_highlightedQuestObjective( INVALID_ENTRY_INDEX )
	, m_tickInterval( DEFAULT_TICK_INTERVAL )
	, m_debugInfo( 0 )
	, m_debugVideo( false )
	, m_cachedDisplayNameIndexForSave( 343480 ) // first quest in game...
{
	const CClass* classDesc = ClassID< CJournalQuest >();

	const CProperty* prop = NULL;

	prop = classDesc->FindProperty( CNAME( type ) );
	AddCacheMetadata( prop );

	prop = classDesc->FindProperty( CNAME( world ) );
	AddCacheMetadata( prop );

	AddExceptionClass( ClassID< CJournalQuestDescriptionGroup >(), ClassID< CJournalQuestGroup >() );
	AddExceptionClass( ClassID< CJournalQuestDescriptionGroup >(), ClassID< CJournalQuest >() );
	AddExceptionClass( ClassID< CJournalQuestDescriptionEntry >(), ClassID< CJournalQuestGroup >() );
	AddExceptionClass( ClassID< CJournalQuestDescriptionEntry >(), ClassID< CJournalQuest >() );
	AddExceptionClass( ClassID< CJournalQuestGroup >(),            ClassID< CJournalQuestGroup >() );
}

CWitcherJournalManager::~CWitcherJournalManager()
{

}

void CWitcherJournalManager::OnGenerateDebugFragments( CRenderFrame* frame )
{

#ifndef RED_FINAL_BUILD
	if ( m_debugInfo == 3 )
	{
		TDynArray< const CJournalQuestGroup* > groups;

		GetQuestGroups( groups );
		for ( Uint32 i = 0; i < groups.Size(); ++i )
		{
			String groupName = groups[ i ]->GetName();
			EJournalStatus groupStatus = GetEntryStatus( groups[ i ] );
	
			Color color = Color::WHITE;
			switch( groupStatus )
			{
			case JS_Active:
				color = Color::YELLOW;
				break;
			case JS_Success:
				color = Color::GREEN;
				break;
			case JS_Failed:
				color = Color::RED;
				break;
			}
			frame->AddDebugScreenText( 50, 500 + i * 19, groupName.AsChar(), 0, true, color, Color::BLACK, nullptr );
		}
	}
#endif

#ifndef NO_EDITOR

	if ( m_debugVideo )
	{
		String line;
		Int32 row = 0;
		const SLoadingScreenParam& activeParam = GGame->GetActiveLoadingScreenParam();
		const String& videoFilename = activeParam.m_videoToPlay;

		String realFilename = videoFilename;
		Int32 descCount = 0;

		size_t index;
		if ( realFilename.FindSubstring( TXT("|"), index ) )
		{
			realFilename.RemoveAt( index );
			descCount = ( realFilename[ static_cast< Int32 >( index ) ] - '0' );
			realFilename.RemoveAt( index );
		}
		line = String::Printf( TXT("Video to be played:  %s"), realFilename.AsChar() );
		frame->AddDebugScreenText( 30,  180 + row++ * 19, line.AsChar(), 0, true, Color::WHITE, Color::BLACK, nullptr );
		line = String::Printf( TXT("Number of subtitles: %d"), descCount );
		frame->AddDebugScreenText( 30,  180 + row++ * 19, line.AsChar(), 0, true, Color::WHITE, Color::BLACK, nullptr );
	}

	if ( m_debugInfo == 0 )
	{
		return;
	}
	else if ( m_debugInfo == 1 )
	{
		Uint32 row = 0;
		Color textColor = Color::GREEN;
		String line;

		if ( GGame && GGame->GetActiveWorld() )
		{
			frame->AddDebugScreenText( 30,  150 + (-2) * 19, GGame->GetActiveWorld()->GetDepotPath(), 0, true, textColor, Color::BLACK, nullptr );
		}
		else
		{
			frame->AddDebugScreenText( 30,  150 + (-2) * 19, TXT("?"), 0, true, textColor, Color::BLACK, nullptr );
		}

		// quests
		line = String::Printf( TXT("%d "), 0 );
		if ( !IsEntryValid( m_trackedQuest ) )
		{
			line += TXT("<NONE>");
			textColor = Color::GRAY;
		}
		else
		{
			SJournalEntryStatus& status = m_activeEntries[ m_trackedQuest ];
			const CJournalQuest* quest = Cast< CJournalQuest >( status.m_entry );
			if ( quest )
			{
				line += String::Printf( TXT("%d %s"), status.m_status, quest->GetTitle().AsChar() );
				switch ( status.m_status )
				{
				case JS_Active:
					textColor = Color::YELLOW;
					break;
				case JS_Failed:
					textColor = Color::RED;
					break;
				case JS_Success:
					textColor = Color::GREEN;
					break;
				}

			}
			else
			{
				line += TXT("Ouch! Something went wrong");
				textColor = Color::WHITE;
			}
		}
		frame->AddDebugScreenText( 30,  150 + (row++) * 19, line, 0, true, textColor, Color::BLACK, nullptr );

		// objectives
		const TEntryGroup& group = m_trackedQuestObjectives;
		for ( Uint32 o = 0; o < group.Size(); o++ )
		{
			Uint32 index = group[ o ];

			line = String::Printf( TXT("    %d "), o );
			if ( !IsEntryValid( index ) )
			{
				line += TXT("Ouch! Something went wrong");
				textColor = Color::WHITE;
			}
			else
			{
				SJournalEntryStatus& status = m_activeEntries[ index ];
				const CJournalQuestObjective* obj = Cast< CJournalQuestObjective >( status.m_entry );
				if ( obj )
				{
					line += String::Printf( TXT("%d %s"), status.m_status, obj->GetTitle().AsChar() );
					switch ( status.m_status )
					{
					case JS_Active:
						textColor = Color::YELLOW;
						break;
					case JS_Failed:
						textColor = Color::RED;
						break;
					case JS_Success:
						textColor = Color::GREEN;
						break;
					}
				}
				else
				{
					line += TXT("Ouch! Something went wrong");
					textColor = Color::WHITE;
				}
			}
			frame->AddDebugScreenText( 30,  150 + (row++) * 19, line, 0, true, textColor, Color::BLACK, nullptr );
		}
	}
	else if ( m_debugInfo == 2 )
	{
		Uint32 row = 0;
		Color textColor;
		String line;

		if ( m_activeEntries.Size() == 0 )
		{
			frame->AddDebugScreenText( 30,  150 + row * 19, TXT("THERE ARE NO ACTIVE ENTRIES"), 0, true, Color::RED, Color::BLACK, nullptr );
		}
		else
		{
			for ( Uint32 i = 0; i < m_activeEntries.Size(); ++i )
			{
				const SJournalEntryStatus& entry = m_activeEntries[ i ];

				textColor = Color::GRAY;
				if ( entry.m_status == JS_Active )
				{
					textColor = Color::YELLOW;
				}
				else if ( entry.m_status == JS_Failed )
				{
					textColor = Color::RED;
				}
				else if ( entry.m_status == JS_Success )
				{
					textColor = Color::GREEN;
				}

				line = String::Printf( TXT("%d"), i );
				frame->AddDebugScreenText( 30,  150 + row * 19, line, 0, true, textColor, Color::BLACK, nullptr );
				line = String::Printf( TXT("%s"), entry.m_entry->GetClass()->GetName().AsChar() );
				frame->AddDebugScreenText( 60,  150 + row * 19, line, 0, true, textColor, Color::BLACK, nullptr );
				line = String::Printf( TXT("%s"), entry.m_entry->GetName().AsChar() );
				frame->AddDebugScreenText( 260,  150 + row * 19, line, 0, true, textColor, Color::BLACK, nullptr );
				row++;
			}
			row++;

			line = String::Printf( TXT("T %d    "), m_trackedQuest );
			frame->AddDebugScreenText( 30,  150 + row * 19, line, 0, true, Color::WHITE, Color::BLACK, nullptr );
			line = TXT("");
			for ( Uint32 j = 0; j < m_trackedQuestObjectives.Size(); ++j )
			{
				line += String::Printf( TXT("%d  "), m_trackedQuestObjectives[ j ] );
			}
			row++;

			line = String::Printf( TXT("H %d    "), m_highlightedQuest );
			frame->AddDebugScreenText( 30,  150 + row * 19, line, 0, true, Color::WHITE, Color::BLACK, nullptr );
			line = String::Printf( TXT("%d"), m_highlightedQuestObjective );
			frame->AddDebugScreenText( 60,  150 + row * 19, line, 0, true, Color::WHITE, Color::BLACK, nullptr );
			row++;
		}
	}
#endif //NO_EDITOR
}

void CWitcherJournalManager::Tick( Float timeDelta )
{
	TBaseClass::Tick( timeDelta );

	m_tickInterval -= timeDelta;
	if ( m_tickInterval > 0 )
	{
		return;
	}
	m_tickInterval = DEFAULT_TICK_INTERVAL;

	// tracked quest
	Bool trackNewQuest = false;
	if ( !IsEntryValid( m_trackedQuest ) )
	{
		trackNewQuest = true;
	}
	else if ( m_activeEntries[ m_trackedQuest ].m_status != JS_Active )
	{
		trackNewQuest = true;
		// clear manual tracking flag
	}

	if ( trackNewQuest )
	{
		const CJournalQuest* journalQuest = FindQuestToTrack();
		if ( journalQuest )
		{
			TrackQuest( journalQuest->GetGUID(), false );
		}
	}


	// highlighted objective
	if ( IsEntryValid( m_highlightedQuest ) )
	{
		if ( !IsQuestTracked( m_highlightedQuest ) || m_activeEntries[ m_highlightedQuest ].m_status != JS_Active )
		{
			InvalidateEntry( m_highlightedQuest );
		}
	}
	if ( IsEntryValid( m_highlightedQuestObjective ) )
	{
		if ( !IsObjectiveTracked( m_highlightedQuestObjective ) || m_activeEntries[ m_highlightedQuestObjective ].m_status != JS_Active )
		{
			InvalidateEntry( m_highlightedQuestObjective );
		}
	}

	if ( !IsEntryValid( m_highlightedQuestObjective ) )
	{
		// there is no highlighted objective
		if ( !IsEntryValid( m_highlightedQuest ) )
		{
			// there is also no highlighted quest, find active with minimum order and highlight it
			const CJournalQuestObjective* minOrderObjective = nullptr;
			Uint32 minOrder = 0;

			for ( Uint32 j = 0; j < m_trackedQuestObjectives.Size(); ++j )
			{
				Uint32 index = m_trackedQuestObjectives[ j ];
				if ( index < m_activeEntries.Size() )
				{
					// make sure objective is active
					if ( m_activeEntries[ index ].m_status ==  JS_Active )
					{
						const CJournalQuestObjective* objective = Cast< CJournalQuestObjective >( m_activeEntries[ index ].m_entry );
						if ( objective )
						{
							if ( !minOrderObjective || minOrder > objective->GetOrder() )
							{
								minOrderObjective = objective;
								minOrder = objective->GetOrder();
							}
						}
					}
				}
			}
			if ( minOrderObjective )
			{
				HighlightObjective( minOrderObjective );
			}
		}
		// there is some highlighted quest, highlight first available objective from that quest
		else
		{
			if ( m_trackedQuest == m_highlightedQuest )
			{
				const CJournalQuestObjective* minOrderObjective = nullptr;
				Uint32 minOrder = 0;

				for ( Uint32 j = 0; j < m_trackedQuestObjectives.Size(); ++j )
				{
					Uint32 index = m_trackedQuestObjectives[ j ];
					if ( index < m_activeEntries.Size() )
					{
						// make sure objective is active
						if ( m_activeEntries[ index ].m_status ==  JS_Active )
						{
							const CJournalQuestObjective* objective = Cast< CJournalQuestObjective >( m_activeEntries[ index ].m_entry );
							if ( objective )
							{
								if ( !minOrderObjective || minOrder > objective->GetOrder() )
								{
									minOrderObjective = objective;
									minOrder = objective->GetOrder();
								}
							}
						}
					}
				}
				if ( minOrderObjective )
				{
					HighlightObjective( minOrderObjective );
				}
			}
		}
	}
}

//! Called in order to initialize the system
void CWitcherJournalManager::OnGameStart( const CGameInfo& gameInfo )
{
	CWitcherGameResource* gameResource = Cast< CWitcherGameResource >( GGame->GetGameResource() );

#ifdef JOURNAL_PATH_PRECACHE_RESOURCES
	// This needs to happen before anything else
	CJournalPath::LoadResources( gameResource->GetJournalPath().AsChar() );
#endif

	if ( gameInfo.IsNewGame() || gameInfo.IsSavedGame() )
	{
		m_objectiveCounters.ClearFast();
	}

	TBaseClass::OnGameStart( gameInfo );

	m_cachedDisplayNameIndexForSave = 343480; // first quest in game...
	CJournalInitialEntriesResource* journalResource = resInitialJournalStats.LoadAndGet< CJournalInitialEntriesResource >();

	if ( gameInfo.IsNewGame() || gameInfo.IsSavedGame() )
	{
		// Tracked quests
		InvalidateEntry( m_trackedQuest );
		m_trackedQuestObjectives.ClearFast();
		InvalidateEntry( m_highlightedQuest );
		InvalidateEntry( m_highlightedQuestObjective );

		// Hunting Clues
		const TDynArray< CEnum* >& clueCategories = gameResource->GetHuntingClueCategories();
		m_clueToCreatureMap.Reserve( clueCategories.Size() );

		for( Uint32 i = 0; i < clueCategories.Size(); ++i )
		{
			Uint32 numberOfElements = clueCategories[ i ]->GetOptions().Size();
			m_clueToCreatureMap.GetRef( clueCategories[ i ]->GetName() ).Resize( numberOfElements );
		}

		// Init total quests count from resource
		if ( journalResource )
		{
			m_regularQuestCount      = journalResource->GetRegularQuestCount();
			m_monsterHuntQuestCount  = journalResource->GetMonsterHuntQuestCount();
			m_treasureHuntQuestCount = journalResource->GetTreasureHuntQuestCount();
		}

		// expansion packs stats
		for ( Uint32 i = EJCT_EP1; i <= EJCT_EP2; ++i )
		{
			String filename = CWitcherJournalManager::GetInitialEntriesPathAndFilename( i );
			if ( GDepot->FileExist( filename ) )
			{
				CJournalInitialEntriesResource* epJournalResource = Cast< CJournalInitialEntriesResource >( GDepot->LoadResource( filename ) );
				if ( epJournalResource )
				{
					m_regularQuestCount      += epJournalResource->GetRegularQuestCount();
					m_monsterHuntQuestCount  += epJournalResource->GetMonsterHuntQuestCount();
					m_treasureHuntQuestCount += epJournalResource->GetTreasureHuntQuestCount();
				}
			}
		}

	}

	if ( gameInfo.IsSavedGame() )
	{
		// load from savegame if applicable
		OnLoadGame( gameInfo.m_gameLoadStream );
	}

	if ( gameInfo.IsNewGame() || gameInfo.IsSavedGame() )
	{
		// activate initial entries on game start and also on game load (for older saves before dlcs were available)
		if ( journalResource )
		{
			const TDynArray< THandle< CJournalPath > >& entries = journalResource->GetEntries();
			for ( Uint32 i = 0; i < entries.Size(); i++ )
			{
				if ( !entries[ i ] || !entries[ i ]->IsValid() )
				{
					continue;
				}
				if ( GetEntryStatus( entries[ i ] ) == JS_Active )
				{
					continue;
				}
				RED_LOG( RED_LOG_CHANNEL( JournalInitialization ), TXT("%02d - %s"), i, entries[ i ]->GetFriendlyName().AsChar() );
				ActivateEntry( entries[ i ], JS_Active, true );
			}
		}
	}
}

void CWitcherJournalManager::OnGameEnd( const CGameInfo& gameInfo )
{
	TBaseClass::OnGameEnd( gameInfo );

	if ( !gameInfo.IsChangingWorld() )
	{
		m_foundHuntingQuestClues.Clear();
		m_clueToCreatureMap.Clear();
		m_objectiveCounters.Clear();
		m_knownMonsterQuests.Clear();
		m_entriesWithAdvancedInformation.Clear();

		InvalidateEntry( m_trackedQuest );
		m_trackedQuestObjectives.Clear();
		InvalidateEntry( m_highlightedQuest );
		InvalidateEntry( m_highlightedQuestObjective );
	}

#ifdef JOURNAL_PATH_PRECACHE_RESOURCES
	CJournalPath::ClearResources();
#endif
}

//! Called when we are creating saving the save game
Bool CWitcherJournalManager::OnSaveGame( IGameSaver* saver )
{
	Bool retVal;

	TIMER_BLOCK( time )
		 
	// it can be set here as CR4Game is saved later
	const String& videoFile = GetCurrentStorybookVideoName();
	GGame->ReplaceDefaultLoadingScreenVideo( videoFile );
	const CName& screenName = GetCurrentLoadingScreenName();
	GGame->ReplaceDefaultLoadingScreenName( screenName );

	retVal = TBaseClass::OnSaveGame( saver );

	CGameSaverBlock block( saver, GetStaticClass()->GetName() );

	// Counters
	{
		CGameSaverBlock entriesBlock( saver, CNAME( JObjectiveCounters ) );

		saver->WriteValue( CNAME( Size ), m_objectiveCounters.Size() );

		for( TObjectiveCounters::iterator iter = m_objectiveCounters.Begin(); iter != m_objectiveCounters.End(); ++iter )
		{
			CGameSaverBlock entriesBlock( saver, CNAME( JObjectiveCounter ) );

			saver->WriteValue( CNAME( guid ), iter->m_first );
			saver->WriteValue( CNAME( count ), iter->m_second );
		}
	}

	// Tracked Quest
	{
		CGameSaverBlock entriesBlock( saver, CNAME( JTrackedQuest ) );

		CGUID guid;
		if ( IsEntryValid( m_trackedQuest ) )
		{
			guid = m_activeEntries[ m_trackedQuest ].m_entry->GetGUID();
		}

		saver->WriteValue( CNAME( guid ), guid );
	}

	// Highlighted Quest Objective
	{
		CGameSaverBlock entriesBlock( saver, CNAME( JHighlightedObjective ) );

		CGUID guid;
		if ( IsEntryValid( m_highlightedQuestObjective ) )
		{
			guid = m_activeEntries[ m_highlightedQuestObjective ].m_entry->GetGUID();
		}

		saver->WriteValue( CNAME( guid ), guid );
	}

	// Found Hunting Clues
	{
		CGameSaverBlock entriesBlock( saver, CNAME( JHuntingClues ) );

		saver->WriteValue( CNAME( Size ), m_foundHuntingQuestClues.Size() );

		for( THashMap< CGUID, TEntryGroup >::iterator iter = m_foundHuntingQuestClues.Begin(); iter != m_foundHuntingQuestClues.End(); ++iter )
		{
			CGameSaverBlock entriesBlock( saver, CNAME( JHuntingClue ) );

			TEntryGroup& clues = iter->m_second;

			saver->WriteValue( CNAME( JHuntingQuestGuid ), iter->m_first );
			saver->WriteValue( CNAME( Size ), clues.Size() );

			for( Uint32 i = 0; i < clues.Size(); ++i )
			{
				CGameSaverBlock entriesBlock( saver, CNAME( JHuntingClueGuid ) );

				const CGUID& guid = m_activeEntries[ clues[ i ] ].m_entry->GetGUID();
				saver->WriteValue( CNAME( guid ), guid );
			}
		}
	}

	// Quest Monster Known
	{
		CGameSaverBlock entriesBlock( saver, CNAME( JMonsterKnown) );

		saver->WriteValue( CNAME( Size ), m_knownMonsterQuests.Size() );

		THashSet< CGUID >::iterator iter = m_knownMonsterQuests.Begin();
		THashSet< CGUID >::iterator endIter = m_knownMonsterQuests.End();
		for (; iter != endIter; ++iter)
		{
			CGameSaverBlock entriesBlock( saver, CNAME( JMonsterKnownGuid) );

			const CGUID& guid = *iter;
			saver->WriteValue( CNAME(guid), guid );
		}
	}

	// Journal Advanced Info
	{
		CGameSaverBlock entriesBlock( saver, CNAME( JEntryAdvancedInfo) );

		saver->WriteValue( CNAME( Size ), m_entriesWithAdvancedInformation.Size() );

		THashSet< CGUID >::iterator iter = m_entriesWithAdvancedInformation.Begin();
		THashSet< CGUID >::iterator endIter = m_entriesWithAdvancedInformation.End();
		for (; iter != endIter; ++iter)
		{
			CGameSaverBlock entriesBlock( saver, CNAME( JEntryAdvancedInfoGuid) );

			const CGUID& guid = *iter;
			saver->WriteValue( CNAME(guid), guid );
		}
	}

	END_TIMER_BLOCK( time )

	return retVal;
}

void CWitcherJournalManager::OnLoadGame( IGameLoader* loader )
{
	CGameSaverBlock block( loader, GetStaticClass()->GetName() );

	// Counters
	{
		CGameSaverBlock entriesBlock( loader, CNAME( JObjectiveCounters ) );

		Uint32 size = loader->ReadValue< Uint32 >( CNAME( Size ) );
		for( Uint32 i = 0; i < size; ++i )
		{
			CGameSaverBlock entriesBlock( loader, CNAME( JObjectiveCounter ) );

			CGUID guid;
			Int32 count;

			loader->ReadValue( CNAME( guid ), guid );
			loader->ReadValue( CNAME( count ), count );

			m_objectiveCounters[ guid ] = count;
		}
	}

	// Tracked Quest
	{
		CGameSaverBlock entriesBlock( loader, CNAME( JTrackedQuest ) );

		CGUID guid;
		loader->ReadValue( CNAME( guid ), guid );

		if ( !guid.IsZero() )
		{
			TrackQuest( guid, false );
		}
	}

	// Highlighted Quest Objective
	{
		CGameSaverBlock entriesBlock( loader, CNAME( JHighlightedObjective ) );

		CGUID guid;
		loader->ReadValue( CNAME( guid ), guid );

		if ( !guid.IsZero() )
		{
			HighlightObjective( guid, true );
		}
	}

	// Found Hunting Clues
	{
		CGameSaverBlock entriesBlock( loader, CNAME( JHuntingClues ) );

		Uint32 numQuests = loader->ReadValue< Uint32 >( CNAME( Size ) );
		// do not reserve anything, these clues are cleared in OnGameEnd and there can be clues already here activated by CJournalManager::OnLoadGame
		// besides are these hunting clues even required?
		//m_foundHuntingQuestClues.Reserve( numQuests );
		for( Uint32 i = 0; i < numQuests; ++i )
		{
			CGameSaverBlock entriesBlock( loader, CNAME( JHuntingClue ) );

			CGUID questGuid;
			Uint32 numClues;

			loader->ReadValue( CNAME( JHuntingQuestGuid ), questGuid );
			loader->ReadValue( CNAME( Size ), numClues );

			TEntryGroup clues;
			clues.Resize( numClues );

			for( Uint32 i = 0; i < numClues; ++i )
			{
				CGameSaverBlock entriesBlock( loader, CNAME( JHuntingClueGuid ) );

				CGUID clueGuid;
				loader->ReadValue( CNAME( guid ), clueGuid );

				Uint32* index = m_activeEntryMap.FindPtr( clueGuid );
				ASSERT( index != NULL, TXT( "Missing journal hunting clue entry during load: %s (Part of hunting quest: %s)" ), ToString( clueGuid ).AsChar(), ToString( questGuid ).AsChar() );

				clues[ i ] = *index;
			}

			m_foundHuntingQuestClues.Insert( questGuid, clues );
		}
	}

	// Quest Monster Known
	{
		CGameSaverBlock entriesBlock( loader, CNAME( JMonsterKnown) );

		Uint32 numKnownMonsters = loader->ReadValue< Uint32 >( CNAME( Size ) );
		m_knownMonsterQuests.Reserve(numKnownMonsters);

		for (Uint32 i = 0; i < numKnownMonsters; ++i)
		{
			CGameSaverBlock entriesBlock( loader, CNAME( JMonsterKnownGuid) );

			CGUID questGuid;
			loader->ReadValue( CNAME( guid ), questGuid );

			m_knownMonsterQuests.Insert(questGuid);
		}
	}

	// Journal Advanced Info
	{
		CGameSaverBlock entriesBlock( loader, CNAME( JEntryAdvancedInfo) );

		Uint32 numKnownQuests = loader->ReadValue< Uint32 >( CNAME( Size ) );
		m_entriesWithAdvancedInformation.Reserve(numKnownQuests);

		for (Uint32 i = 0; i < numKnownQuests; ++i)
		{
			CGameSaverBlock entriesBlock( loader, CNAME( JEntryAdvancedInfoGuid) );

			CGUID questGuid;
			loader->ReadValue( CNAME( guid ), questGuid );

			m_entriesWithAdvancedInformation.Insert(questGuid);
		}
	}
}

void CWitcherJournalManager::OnInitGuiDuringGameLoad()
{
	TBaseClass::OnInitGuiDuringGameLoad();

	SendTrackEvent();
	SendHighlightEvent();
}

void CWitcherJournalManager::OnInitGuiDuringWorldChange()
{
	TBaseClass::OnInitGuiDuringWorldChange();

	SendTrackEvent();
	SendHighlightEvent();
}

void CWitcherJournalManager::EntryStatusChanged( CJournalBase* questEntry, EJournalStatus oldStatus, EJournalStatus newStatus, Bool silent ) 
{
	if ( questEntry->IsA< CJournalStoryBookPageDescription >() )
	{
		// FIXME: Can probably do a cheaper update.
		const String& videoFile = GetCurrentStorybookVideoName();
		GGame->ReplaceDefaultLoadingScreenVideo( videoFile );
	}

	if( questEntry->IsA< CJournalQuestObjective >() )
	{
		if ( IsTrackedObjective( Cast< CJournalQuestObjective >( questEntry ) ) )
		{
			UpdateTrackedObjectives();
		}
	}

#if !defined( NO_TELEMETRY ) || !defined( NO_SECOND_SCREEN )
	if( questEntry != NULL )
	{
		if( questEntry->IsA< CJournalQuest >() )
		{
#if !defined( NO_TELEMETRY )
			CR4Telemetry *telemetrySystem = GCommonGame->GetSystem< CR4Telemetry >();

			if ( telemetrySystem )
			{
				if(  newStatus == JS_Active )
					telemetrySystem->LogL( TE_QUEST_ACTIVATED, questEntry->GetName() );
				else if( oldStatus != JS_Inactive && newStatus == JS_Success ) // not sent on restore state
					telemetrySystem->LogL( TE_QUEST_FINISHED, questEntry->GetName() );
				else if( oldStatus != JS_Inactive && newStatus == JS_Failed ) // not sent on restore state
					telemetrySystem->LogL( TE_QUEST_FAILED, questEntry->GetName() );

				telemetrySystem->SetStatValue( CS_GAME_PROGRESS, (float)GetQuestProgress() );
				telemetrySystem->Log( TE_SYS_GAME_PROGRESS );				
			}
#endif
		}
#if !defined( NO_SECOND_SCREEN )
		CR4SecondScreenManager *secondScreenManager = GCommonGame->GetSystem< CR4SecondScreenManager >();
		if( secondScreenManager )
		{
			secondScreenManager->SendJournalStatus( questEntry, newStatus, silent );
		}
#endif
	}	
#endif


	/////////////////////////////////////////
	//
	// TTP 110940 & 110941
	//
#ifndef RED_FINAL_BUILD
	if ( questEntry && questEntry->IsA< CJournalQuest >() )
	{
		if ( newStatus == JS_Success )
		{
			RED_LOG( RED_LOG_CHANNEL( SucceededQuest ), TXT("[%ls]"), questEntry->GetName().AsChar() );
		}
	}
#endif //RED_FINAL_BUILD
	//
	//
	//
	/////////////////////////////////////////
}

void CWitcherJournalManager::TypeSpecificActivation( CJournalBase* entry, Uint32 index )
{
	if( entry->IsA< CJournalQuestObjective >() )
	{
		// Add counter value for quest objectives
		m_objectiveCounters.Insert( entry->GetGUID(), 0 );
	}
	else if( entry->IsA< CJournalQuest >() )
	{
		CJournalQuest* quest = static_cast< CJournalQuest* >( entry );

		if( quest->IsMonsterHuntQuest() )
		{
			if( m_foundHuntingQuestClues.Insert( quest->GetGUID(), TEntryGroup() ) )
			{
				SW3JournalHuntingQuestAddedEvent event;
				event.m_quest = quest;
				event.m_entry = quest;
				SendEvent( event );
			}
		}
	}
	else if( entry->IsA< CJournalCreatureHuntingClue >() )
	{
		CJournalCreatureHuntingClue* clueEntry = static_cast< CJournalCreatureHuntingClue* >( entry );

		const CName& categoryName = clueEntry->GetClueCategory()->GetName();
		Uint32 clueIndex = clueEntry->GetClueIndex();

		CJournalCreatureHuntingClueGroup* group = entry->GetParentAs< CJournalCreatureHuntingClueGroup >();
		CJournalCreature* creature = group->GetParentAs< CJournalCreature >();

		Uint32 creatureIndex = m_activeEntryMap.GetRef( creature->GetGUID() );

		TDynArray< TEntryGroup >& category = m_clueToCreatureMap.GetRef( categoryName );

		if( clueIndex < category.Size() )
		{
			category[ clueIndex ].PushBackUnique( creatureIndex );
		}
		else
		{
#ifndef NO_DATA_ASSERTS
			ASSERT( entry->GetParent() != NULL, TXT( "Corrupt Journal Structure -> Entry %s has no CResource in the parent hierarchy" ), entry->GetName().AsChar() );
			DATA_HALT( DES_Major, CResourceObtainer::GetResource( entry->GetParentAs< CComponent > () ), TXT( "Invalid CJournal Creature Hunting Clue with name %s" ), entry->GetName().AsChar() );
#endif
		}
	}
}

void CWitcherJournalManager::GetQuestGroups( TDynArray< const CJournalQuestGroup* >& questGroups ) const
{
	GetAllActivatedOfType< CJournalQuestGroup >( questGroups );
}

void CWitcherJournalManager::GetQuests( const CJournalQuestGroup* parent, TDynArray< const CJournalQuest* >& quests ) const
{
	ASSERT( parent );
	GetAllActivatedChildren( parent->GetGUID(), quests );
}

void CWitcherJournalManager::GetQuestPhases( const CJournalQuest* parent, TDynArray< const CJournalQuestPhase* >& questPhases ) const
{
	ASSERT( parent );
	GetAllActivatedChildren( parent->GetGUID(), questPhases );
}

void CWitcherJournalManager::GetQuestObjectives( const CGUID& phaseGUID, TDynArray< const CJournalQuestObjective* >& questObjectives ) const
{
	GetAllActivatedChildren( phaseGUID, questObjectives );
}

void CWitcherJournalManager::GetTrackedQuestObjectives( TDynArray< const CJournalQuestObjective* >& objectives ) const
{	
	const TEntryGroup* trackedObjectives = GetTrackedEntryGroup();

	if( trackedObjectives )
	{
		FillArrayWithEntryGroup( objectives, trackedObjectives );
	}

	::Sort( objectives.Begin(), objectives.End(), []( const CJournalQuestObjective* a, const CJournalQuestObjective* b )
	{
		return a->GetOrder() < b->GetOrder();
	}
	);
}

void CWitcherJournalManager::GetStoryQuests( TDynArray< const CJournalQuest* >& quests ) const
{
	const CClass* classDesc = ClassID< CJournalQuest >();
	const CProperty* prop = classDesc->FindProperty( CNAME( type ) );

	eQuestType typeWanted = QuestType_Story;

	const TEntryGroup* group = GetEntryGroupFromCache( prop, &typeWanted );

	FillArrayWithEntryGroup( quests, group );
}

void CWitcherJournalManager::TrackQuest( const CJournalQuest* quest, Bool manualTracking )
{
	TrackQuest( quest->GetGUID(), manualTracking );
}

void CWitcherJournalManager::TrackQuest( const CGUID& questGUID, Bool manualTracking )
{
	Uint32 index = 0;
	if ( m_activeEntryMap.Find( questGUID, index ) )
	{
		ASSERT( m_activeEntries[ index ].m_entry->IsA< CJournalQuest >(), TXT( "Can only track quests, '%ls' is a '%ls'" ), m_activeEntries[ index ].m_entry->GetName().AsChar(), m_activeEntries[ index ].m_entry->GetClass()->GetName().AsString().AsChar() );

		if ( m_trackedQuest != index )
		{
			m_trackedQuest = index;
			UpdateTrackedObjectives();
			SendTrackEvent();
		}
	}
}

void CWitcherJournalManager::UpdateTrackedObjectives()
{
	if ( !IsEntryValid( m_trackedQuest ) )
	{
		// no quest is being tracked
		return;
	}

	m_trackedQuestObjectives.Clear();

	const SJournalEntryStatus& status = m_activeEntries[ m_trackedQuest ];

	const TEntryGroup* phases = m_activeEntryChildren.FindPtr( status.m_entry->GetGUID() );
	if ( phases )
	{
		for( Uint32 i = 0; i < phases->Size(); ++i )
		{
			Uint32 phaseIndex = (*phases)[ i ];
			const CJournalBase* phase = m_activeEntries[ phaseIndex ].m_entry;
			if ( !phase->IsA< CJournalQuestPhase >() ) // need to check type, it could be CJournalQuestDescriptionGroup
			{
				continue;
			}

			const TEntryGroup* objectives = m_activeEntryChildren.FindPtr( phase->GetGUID() );
			if( objectives )
			{
				for( Uint32 iObjective = 0; iObjective < objectives->Size(); ++iObjective )
				{
					Uint32 objectiveIndex = (*objectives)[ iObjective ];

					m_trackedQuestObjectives.PushBack( objectiveIndex );
				}
			}
		}
	}

	CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
	if ( commonMapManager )
	{
		commonMapManager->InvalidateQuestMapPinData();
	}
}

void CWitcherJournalManager::SendTrackEvent()
{
	if ( !IsEntryValid( m_trackedQuest ) )
	{
		return;
	}

	SW3JournalTrackEvent event;
	event.m_quest = Cast< CJournalQuest >( m_activeEntries[ m_trackedQuest ].m_entry );
	SendEvent( event );
}

void CWitcherJournalManager::SendHighlightEvent()
{
	const CJournalQuestObjective* highlightedObjective = nullptr;

	if ( IsEntryValid( m_highlightedQuestObjective ) )
	{
		highlightedObjective = Cast< CJournalQuestObjective >( m_activeEntries[ m_highlightedQuestObjective ].m_entry );
	}

	SW3JournalHighlightEvent event;
	event.m_objective = highlightedObjective;
	SendEvent( event );
}

void CWitcherJournalManager::UpdateQuestObjectiveCounter( const CGUID& objectiveGUID, Int32 count, Bool silent )
{
	SetQuestObjectiveCount( objectiveGUID, count );

	Uint32 index = 0;
	if ( m_activeEntryMap.Find( objectiveGUID, index ) )
	{
		ASSERT( m_activeEntries[ index ].m_entry->IsA< CJournalQuestObjective >() );

		CJournalQuestObjective* objective = Cast< CJournalQuestObjective >( m_activeEntries[ index ].m_entry );
		if ( objective )
		{
			CObject* ancestor = objective->GetParent();
			while ( ancestor && ! ancestor->IsA< CJournalQuest >() )
			{
				ancestor = ancestor->GetParent();
			}

			ASSERT( ancestor && ancestor->IsA< CJournalQuest >() );
			CJournalQuest* quest = Cast< CJournalQuest >( ancestor );
			if ( quest )
			{
				// Send notification event
				SW3JournalQuestObjectiveCounterTrackEvent event;

				event.m_quest			   = quest;
				event.m_questObjective     = objective;
				event.m_counter            = count;

				SendEvent( event );
			}
		}
	}
}

void CWitcherJournalManager::SetQuestMonsterAvailable(bool value, const CGUID& questID)
{
	if (value)
	{
		m_knownMonsterQuests.Insert(questID);
	}
	else if (m_knownMonsterQuests.Find(questID) != m_knownMonsterQuests.End())
	{
		m_knownMonsterQuests.Erase(m_knownMonsterQuests.Find(questID));
	}
}

void CWitcherJournalManager::SetHasAdvancedInformation(bool value, const CGUID& entryID)
{
	if (value)
	{
		m_entriesWithAdvancedInformation.Insert(entryID);
	}
	else if (m_entriesWithAdvancedInformation.Find(entryID) != m_entriesWithAdvancedInformation.End())
	{
		m_entriesWithAdvancedInformation.Erase(m_entriesWithAdvancedInformation.Find(entryID));
	}
}

Bool CWitcherJournalManager::IsTrackedQuest( const CJournalQuest* quest ) const
{
	return quest && ( quest == GetTrackedQuest() );
}

Bool CWitcherJournalManager::IsTrackedObjective( const CJournalQuestObjective* objective ) const
{
	if ( !objective )
	{
		return false;
	}
	return IsTrackedQuest( objective->GetParentQuest() );
}

Bool CWitcherJournalManager::CanTrackQuest( const CJournalQuest* quest ) const
{
	if ( quest == GetTrackedQuest() )
	{
		return false;
	}

	// this is wrong, quest levels should be defined in CJournalQuest
	Bool ret = false;
	if ( !CallFunctionRet( GGame, CNAME( CanTrackQuest ), THandle< CJournalQuest >( quest ), ret ) )
	{
		return false;
	}
	return ret;
}

const CJournalQuest* CWitcherJournalManager::GetTrackedQuest() const
{
	if ( IsEntryValid( m_trackedQuest ) )
	{
		if ( m_trackedQuest < m_activeEntries.Size() )
		{
		    if ( m_activeEntries[ m_trackedQuest ].m_entry )
		    {
		    	ASSERT( m_activeEntries[ m_trackedQuest ].m_entry->IsA< CJournalQuest >() );
		    	return static_cast< CJournalQuest* >( m_activeEntries[ m_trackedQuest ].m_entry );
		    }
		}
	}

	return NULL;
}

const CJournalQuest* CWitcherJournalManager::GetHighlightedQuest() const
{
	if ( IsEntryValid( m_highlightedQuest ) )
	{
		if ( m_highlightedQuest < m_activeEntries.Size() )
		{
			return Cast< CJournalQuest >( m_activeEntries[ m_highlightedQuest ].m_entry );
		}
	}
	return nullptr;
}

const CJournalQuestObjective* CWitcherJournalManager::GetHighlightedObjective() const
{
	if ( IsEntryValid( m_highlightedQuestObjective ) )
	{
		if ( m_highlightedQuestObjective < m_activeEntries.Size() )
		{
			return Cast< CJournalQuestObjective >( m_activeEntries[ m_highlightedQuestObjective ].m_entry );
		}
	}
	return nullptr;
}

Bool CWitcherJournalManager::HighlightObjective( const CJournalQuestObjective* objective, Bool onLoad /*= false*/, Bool fromScripts /*= false*/ )
{
	const CJournalQuest* quest = objective->GetParentQuest();

	Uint32 highlightedQuest = INVALID_ENTRY_INDEX;
	Uint32 highlightedQuestObjective = INVALID_ENTRY_INDEX;

	if ( !IsTrackedQuest( quest ) || !IsTrackedObjective( objective ) )
	{
		// you can't highlight objective that is not tracked
		return false;
	}

	for ( Uint32 i = 0; i < m_activeEntries.Size(); ++i )
	{
		if ( m_activeEntries[ i ].m_entry == quest )
		{
			highlightedQuest = i;
		}
		if ( m_activeEntries[ i ].m_entry == objective )
		{
			highlightedQuestObjective = i;
		}
	}
	if ( IsEntryValid( highlightedQuest ) && IsEntryValid( highlightedQuestObjective ) )
	{
		if ( m_highlightedQuest != highlightedQuest || m_highlightedQuestObjective != highlightedQuestObjective )
		{
			// highlighting is going to change
			m_highlightedQuest = highlightedQuest;
			m_highlightedQuestObjective = highlightedQuestObjective;
			SendHighlightEvent();
		}
	}

	{
		CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
		if (commonMapManager)
		{
			commonMapManager->SetHighlightableMapPinsFromObjective( GetHighlightedObjective() );

			if ( !onLoad )
			{
				if ( fromScripts )
				{
					// update all mappins immediately
					commonMapManager->ForceUpdateDynamicMapPins();
				}
				else
				{
					// just make sure to update in next tick
					commonMapManager->ScheduleUpdateDynamicMapPins();
				}
			}
		}
	}
	return true;
}

Bool CWitcherJournalManager::HighlightObjective( const CGUID& objectiveGUID, Bool onLoad /*= false*/, Bool fromScripts /*= false*/ )
{
	Uint32 index = 0;
	if ( m_activeEntryMap.Find( objectiveGUID, index ) )
	{
		const CJournalQuestObjective* objective = Cast< CJournalQuestObjective >( m_activeEntries[ index ].m_entry );
		if ( objective )
		{
			return HighlightObjective( objective, onLoad, fromScripts );
		}
	}
	return false;
}

Bool CWitcherJournalManager::HighlightPrevNextObjective( Bool next )
{
	if ( !IsEntryValid( m_highlightedQuestObjective ) )
	{
		// there is no highlighted objective, we can't highlight next or previous
		return false;
	}

	TDynArray< const CJournalQuestObjective* > allQuestObjectives;
	TDynArray< const CJournalQuestObjective* > activeQuestObjectives;

	allQuestObjectives.Reserve( 10 );
	activeQuestObjectives.Reserve( 10 );
	GetTrackedQuestObjectives( allQuestObjectives );

	for ( Uint32 i = 0; i < allQuestObjectives.Size(); ++i )
	{
		if ( GetEntryStatus( allQuestObjectives[ i ] ) == JS_Active )
		{
			activeQuestObjectives.PushBack( allQuestObjectives[ i ] );
		}
	}

	Uint32 activeQuestObjectivesSize = activeQuestObjectives.Size();
	if ( activeQuestObjectivesSize <= 1 )
	{
		return false;
	}

	const CJournalBase* highlightedObjective = nullptr;
	if ( IsEntryValid( m_highlightedQuestObjective ) )
	{
		highlightedObjective = m_activeEntries[ m_highlightedQuestObjective ].m_entry;
	}
	if ( !highlightedObjective )
	{
		return false;
	}

	for ( Uint32 i = 0; i < activeQuestObjectivesSize; ++i )
	{
		if ( highlightedObjective == activeQuestObjectives[ i ] )
		{
			Uint32 activeObjectiveIndex = 0;
			if ( next )
			{
				activeObjectiveIndex = ( i + 1 ) % activeQuestObjectivesSize;
			}
			else
			{
				activeObjectiveIndex = ( i - 1 + activeQuestObjectivesSize ) % activeQuestObjectivesSize;
			}
			HighlightObjective( Cast< CJournalQuestObjective >( activeQuestObjectives[ activeObjectiveIndex ] ) );
			return true;
		}
	}
	return false;
}

void CWitcherJournalManager::ActivateHuntingQuestClue( CJournalQuest* quest, CJournalCreatureHuntingClue* clue )
{
	ASSERT( quest != NULL );
	ASSERT( clue != NULL );

	if( !m_foundHuntingQuestClues.KeyExist( quest->GetGUID() ) )
	{
		ERR_R4( TXT( "Quest '%ls' was not activated (and hence visible in the quest log) prior to marking clue found" ), quest->GetName().AsChar() );
	}

	Uint32* index = m_activeEntryMap.FindPtr( clue->GetGUID() );

	if( index )
	{
		if( m_foundHuntingQuestClues.GetRef( quest->GetGUID() ).PushBackUnique( *index ) )
		{
			SW3JournalHuntingQuestClueFoundEvent event;
			event.m_entry = quest;
			event.m_quest = quest;
			event.m_creatureClue = clue;

			SendEvent( event );
		}
	}
	else
	{
		ERR_R4( TXT( "Clue '%ls' was not activated (and hence visible in the bestiary) prior to marking it found" ), clue->GetName().AsChar() );
	}
}

void CWitcherJournalManager::ActivateHuntingQuestClue( THandle< CJournalPath > pathToHuntingTag, THandle< CJournalPath > pathToCreatureClue )
{
	CJournalCreatureHuntingClue* clue	= pathToCreatureClue->GetTargetAs< CJournalCreatureHuntingClue >();
	CJournalQuest* quest				= pathToHuntingTag->GetTargetAs< CJournalQuest >();

	ActivateHuntingQuestClue( quest, clue );
}

void CWitcherJournalManager::GetCreaturesWithHuntingQuestClue( const CName& categoryName, Uint32 clueIndex, TDynArray< const CJournalCreature* >& creatures /*[out]*/ ) const
{
	const TDynArray< TEntryGroup >* categoryGroup = m_clueToCreatureMap.FindPtr( categoryName );

	if( categoryGroup && categoryGroup->Size() > clueIndex )
	{
		FillArrayWithEntryGroup( creatures, &( ( *categoryGroup )[ clueIndex ] ) );
	}
}

const CJournalStoryBookPage* CWitcherJournalManager::GetStoryBookPage( Uint32 pageIndex ) const
{
	const TEntryGroup* pages = m_activeCategoryGroups.FindPtr( ClassID< CJournalStoryBookPage >() );

	if( pages )
	{
		Uint32 entryIndex = (*pages)[ pageIndex ];

		const CJournalBase* entry = m_activeEntries[ entryIndex ].m_entry;
		ASSERT( entry->IsA< CJournalStoryBookPage >() );

		return static_cast< const CJournalStoryBookPage* >( entry );
	}

	return NULL;
}

void CWitcherJournalManager::GetStoryBookChapters( TDynArray< const CJournalStoryBookChapter* >& chapters /*[out]*/ ) const
{
	GetAllActivatedOfType< CJournalStoryBookChapter >( chapters );
}

void CWitcherJournalManager::GetStoryBookPages( const CJournalStoryBookChapter* parent, TDynArray< const CJournalStoryBookPage* >& pages /*[out]*/ ) const
{
	ASSERT( parent );
	GetAllActivatedChildren( parent->GetGUID(), pages );
}

String CWitcherJournalManager::GetCurrentStorybookVideoName( Int32 area /*= 0*/ ) const
{
	CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
	if ( !commonMapManager )
	{
		WARN_GAME(TXT("GetCurrentStorybookVideoName: no map manager!"));
		return String::EMPTY;
	}

	Int32 currWorld = 0;
	if ( area > 0 )
	{
		currWorld = area;
	}
	else
	{
		currWorld = commonMapManager->GetCurrentJournalArea();
	}

	const TEntryGroup* descriptionGroup = m_activeCategoryGroups.FindPtr( ClassID< CJournalStoryBookPageDescription >() );
	if ( descriptionGroup )
	{
		for ( Int32 i = descriptionGroup->Size() - 1; i >= 0; --i )
		{
			Uint32 index = ( *descriptionGroup )[ i ];

			const SJournalEntryStatus& status = m_activeEntries[ index ];
			const CJournalStoryBookPageDescription* description = Cast< CJournalStoryBookPageDescription >( status.m_entry );
			if ( description )
			{
				if ( description->IsFinal() )
				{
					return CLoadingScreen::GetIgnoredVideoName();
				}
				const CJournalStoryBookPage* page = description->GetParentAs< CJournalStoryBookPage >();
				if ( page )
				{
					if ( page->GetWorld() == 0 || page->GetWorld() == currWorld )
					{
						const String& videoFilename = description->GetVideoFilename();
						if ( ! videoFilename.Empty() )
						{
							String fullVideoFileName = String::Printf( TXT("cutscenes\\storybook\\%ls"), videoFilename.AsChar() );
#ifdef RED_LOGGING_ENABLED
							LOG_GAME(TXT("GetCurrentStorybookVideoName: videoName '%ls'"), fullVideoFileName.AsChar() );
#endif
							return fullVideoFileName;
						}

						// if filename is missing in journal, should we break or continue?
						break;
					}
#ifdef RED_LOGGING_ENABLED
					else
					{
						LOG_GAME( TXT("GetCurrentStorybookVideoName: CJournalStoryBookPage '%ls' not for this world. Skipped."), page->GetTitle().AsChar() );
					}
#endif
				}
			}
		}
	}

	return String::EMPTY;
}

const CName& CWitcherJournalManager::GetCurrentLoadingScreenName() const
{
	CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
	if ( !commonMapManager )
	{
		return CName::NONE;
	}

	return commonMapManager->GetCurrentLocalisationName();
}

void CWitcherJournalManager::LoadInitialEntriesFile( const CName& dlcId, const String& filename )
{
	const CJournalInitialEntriesResource* resource = Cast< CJournalInitialEntriesResource >( GDepot->LoadResource( filename ) );
	if ( resource )
	{
		if ( dlcId == CNAME( ep1 ) )
		{
			m_ep1QuestCount = resource->GetRegularQuestCount();
		}
		else if ( dlcId == CNAME( ep2 ) )
		{
			m_ep2QuestCount = resource->GetRegularQuestCount();
		}
	}
}

void CWitcherJournalManager::UnloadInitialEntriesFile( const CName& dlcId )
{
	if ( dlcId == CNAME( ep1 ) )
	{
		m_ep1QuestCount = 0;
	}
	else if ( dlcId == CNAME( ep2 ) )
	{
		m_ep2QuestCount = 0;
	}
}

Uint32 CWitcherJournalManager::GetQuestProgress() const
{
	Uint32 succeededQuestCount = 0;

	const TEntryGroup* questGroup = m_activeCategoryGroups.FindPtr( ClassID< CJournalQuest >() );
	if ( questGroup )
	{
		for( Uint32 i = 0; i < questGroup->Size(); ++i )
		{
			Uint32 index = ( *questGroup )[ i ];
			const CJournalQuest* quest = Cast< CJournalQuest >( m_activeEntries[ index ].m_entry );
			if ( quest )
			{
				if ( m_activeEntries[ index ].m_status == JS_Success )
				{
					succeededQuestCount++;
				}
			}
		}
	}

	// TODO some more fancy way 
	Int32 allQuestsCount =  m_regularQuestCount + m_monsterHuntQuestCount + m_treasureHuntQuestCount + m_ep1QuestCount + m_ep2QuestCount;;
	return allQuestsCount > 0 ? ( 100 * succeededQuestCount ) / allQuestsCount : 0;
}

void CWitcherJournalManager::GetActiveQuestObjectives( TDynArray< const CJournalQuestObjective* >& activeObjectives ) const
{
	GetAllActivatedOfType< CJournalQuestObjective >( activeObjectives );
}

String CWitcherJournalManager::GetInitialEntriesPathAndFilename( Uint32 epIndex )
{
	switch ( epIndex )
	{
	case 0:
		return resInitialJournalStats.GetPath().ToString();
	case 1:
		return TXT("dlc\\ep1\\data\\gameplay\\journal\\startep1.w2je");
	case 2:
		return TXT("dlc\\bob\\data\\gameplay\\journal\\startep2.w2je");
	}
	return String::EMPTY;
}

#ifndef NO_EDITOR
String CWitcherJournalManager::GetInitialEntriesFilename( Uint32 epIndex )
{
	String pathAndFilename = GetInitialEntriesPathAndFilename( epIndex );
	String filename;
	size_t index;
	if ( pathAndFilename.FindSubstring( TXT("\\"), index, true ) )
	{
		filename = pathAndFilename.RightString( pathAndFilename.GetLength() - index - 1 );
	}
	return filename;
}
#endif //NO_EDITOR

void CWitcherJournalManager::GetQuestRewards( const String& questPath, TDynArray< CName >& rewards )
{
	// TODO caching results
	CResource* resource = GDepot->LoadResource( questPath );
	if ( !resource )
	{
		return;
	}
	CQuestPhase* questPhase = Cast< CQuestPhase >( resource );
	if ( !questPhase )
	{
		return;
	}
	CQuestGraph* graph = questPhase->GetGraph();
	if ( !graph )
	{
		return;
	}

	rewards.ClearFast();
	return graph->GetRewards( rewards );
}

Bool CWitcherJournalManager::IsQuestTracked( Uint32 activeEntriesIndex )
{
	return m_trackedQuest == activeEntriesIndex;
}

Bool CWitcherJournalManager::IsObjectiveTracked( Uint32 activeEntriesIndex )
{
	for ( Uint32 j = 0; j < m_trackedQuestObjectives.Size(); ++j )
	{
		if ( m_trackedQuestObjectives[ j ] == activeEntriesIndex )
		{
			return true;
		}
	}
	return false;
}

const CJournalQuest* CWitcherJournalManager::FindQuestToTrack()
{
	const CJournalQuest* currentAreaStoryQuest = nullptr;
	const CJournalQuest* currentAreaStoryEP1Quest = nullptr;
	const CJournalQuest* currentAreaStoryEP2Quest = nullptr;
	const CJournalQuest* currentAreaChapterQuest = nullptr;
	const CJournalQuest* differentAreaStoryQuest = nullptr;
	const CJournalQuest* differentAreaStoryEP1Quest = nullptr;
	const CJournalQuest* differentAreaStoryEP2Quest = nullptr;
	const CJournalQuest* differentAreaChapterQuest = nullptr;

	CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
	if ( !commonMapManager )
	{
		return nullptr;
	}
	Int32 currentArea = commonMapManager->GetCurrentJournalArea();

	const TEntryGroup* questGroup = m_activeCategoryGroups.FindPtr( ClassID< CJournalQuest >() );
	if ( !questGroup )
	{
		return nullptr;
	}

	for ( Uint32 i = 0; i < questGroup->Size(); ++i )
	{
		Uint32 index = ( *questGroup )[ i ];
		const SJournalEntryStatus& status = m_activeEntries[ index ];

		const CJournalQuest* questEntry = Cast< CJournalQuest >( status.m_entry );
		if ( !questEntry )
		{
			continue;
		}
		if ( status.m_status != JS_Active )
		{
			continue;
		}
		if ( !CanTrackQuest( questEntry ) )
		{
			continue;
		}

		if ( questEntry->GetWorld() == currentArea )
		{
			switch ( questEntry->GetType() )
			{
			case QuestType_Story:
				if ( questEntry->GetContentType() == EJCT_Vanilla )
				{
					if ( !currentAreaStoryQuest )
					{
						currentAreaStoryQuest = questEntry;
					}
				}
				else if ( questEntry->GetContentType() == EJCT_EP1 )
				{
					if ( !currentAreaStoryEP1Quest )
					{
						currentAreaStoryEP1Quest = questEntry;
					}
				}
				else if ( questEntry->GetContentType() == EJCT_EP2 )
				{
					if ( !currentAreaStoryEP2Quest )
					{
						currentAreaStoryEP2Quest = questEntry;
					}
				}
				break;
			case QuestType_Chapter:
				if ( !currentAreaChapterQuest )
				{
					currentAreaChapterQuest = questEntry;
				}
				break;
			}
		}
		else
		{
			switch ( questEntry->GetType() )
			{
			case QuestType_Story:
				if ( questEntry->GetContentType() == EJCT_Vanilla )
				{
					if ( !differentAreaStoryQuest )
					{
						differentAreaStoryQuest = questEntry;
					}
				}
				else if ( questEntry->GetContentType() == EJCT_EP1 )
				{
					if ( !differentAreaStoryEP1Quest )
					{
						differentAreaStoryEP1Quest = questEntry;
					}
				}
				else if ( questEntry->GetContentType() == EJCT_EP2 )
				{
					if ( !differentAreaStoryEP2Quest )
					{
						differentAreaStoryEP2Quest = questEntry;
					}
				}
				break;
			case QuestType_Chapter:
				if ( !differentAreaChapterQuest )
				{
					differentAreaChapterQuest = questEntry;
				}
				break;
			}
		}
	}

	if ( currentAreaChapterQuest )
	{
		// TODO find closest quest?
		return currentAreaChapterQuest;
	}
	if ( currentAreaStoryQuest )
	{
		return currentAreaStoryQuest;
	}
	if ( currentAreaStoryEP1Quest )
	{
		return currentAreaStoryEP1Quest;
	}
	if ( currentAreaStoryEP2Quest )
	{
		return currentAreaStoryEP2Quest;
	}
	if ( differentAreaChapterQuest )
	{
		return differentAreaChapterQuest;
	}
	if ( differentAreaStoryQuest )
	{
		return differentAreaStoryQuest;
	}
	if ( differentAreaStoryEP1Quest )
	{
		return differentAreaStoryEP1Quest;
	}
	if ( differentAreaStoryEP2Quest )
	{
		return differentAreaStoryEP2Quest;
	}
	return nullptr;
}

void CWitcherJournalManager::funcSetTrackedQuest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, entryHandle, NULL );
	FINISH_PARAMETERS;

	CJournalBase* entry = entryHandle.Get();

	if ( entry )
	{
		if( entry->IsA< CJournalQuestObjective >() )
		{
			CJournalQuestObjective* objective = static_cast< CJournalQuestObjective* >( entry );
			entry = objective->GetParentQuest();
		}

		if( entry->IsA< CJournalQuest >() )
		{
			// player can manually track quests only at User level
			TrackQuest( entry->GetGUID(), true );
		}
		else
		{
			ERR_R4( TXT( " %s ('%ls') is not a type of journal entry that can be tracked" ), entry->GetClass()->GetName().AsString().AsChar(), entry->GetName().AsChar() );
		}
	}
}

void CWitcherJournalManager::funcGetTrackedQuest( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetTrackedQuest() );
}

void CWitcherJournalManager::funcGetHighlightedQuest( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetHighlightedQuest() );
}

void CWitcherJournalManager::funcGetHighlightedObjective( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetHighlightedObjective() );
}

void CWitcherJournalManager::funcSetHighlightedObjective( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, entryHandle, NULL );
	FINISH_PARAMETERS;

	Bool ret = false;
	CJournalBase* entry = entryHandle.Get();
	if( entry )
	{
		const CJournalQuestObjective* objective = Cast< CJournalQuestObjective >( entry );
		if( objective )
		{
			ret = HighlightObjective( objective, false, true );
		}
	}

	RETURN_BOOL( ret );
}

void CWitcherJournalManager::funcSetPrevNextHighlightedObjective( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, next, true );
	FINISH_PARAMETERS;

	RETURN_BOOL( HighlightPrevNextObjective( next ) );
}

void CWitcherJournalManager::funcGetCreaturesWithHuntingQuestClue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, categoryName, CName::NONE );
	GET_PARAMETER( Uint32, clueIndex, 0 );
	GET_PARAMETER_REF( TDynArray< THandle< CJournalCreature > >, out, TDynArray< THandle< CJournalCreature > >() );
	FINISH_PARAMETERS;

	const TDynArray< TEntryGroup >* categoryGroup = m_clueToCreatureMap.FindPtr( categoryName );

	if( categoryGroup && categoryGroup->Size() > clueIndex )
	{
		FillHandleArrayWithEntryGroup( out, &( ( *categoryGroup )[ clueIndex ] ) );
	}
}

void CWitcherJournalManager::funcSetHuntingClueFoundForQuest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalQuest >, questHandle, NULL );
	GET_PARAMETER( THandle< CJournalCreatureHuntingClue >, clueHandle, NULL );
	FINISH_PARAMETERS;

	CJournalQuest* quest = questHandle.Get();
	CJournalCreatureHuntingClue* clue = clueHandle.Get();

	if( quest && clue )
	{
		ActivateHuntingQuestClue( quest, clue );
	}
}

void CWitcherJournalManager::funcGetNumberOfCluesFoundForQuest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalQuest >, questHandle, NULL );
	FINISH_PARAMETERS;

	Int32 numCluesFound = 0;

	CJournalQuest* quest = questHandle.Get();

	if( quest && quest->IsMonsterHuntQuest() )
	{
		TEntryGroup* foundClues = m_foundHuntingQuestClues.FindPtr( quest->GetGUID() );

		if( foundClues )
		{
			numCluesFound = foundClues->Size();
		}
	}

	RETURN_INT( numCluesFound );
}

void CWitcherJournalManager::funcGetAllCluesFoundForQuest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalQuest >, questHandle, NULL );
	GET_PARAMETER_REF( TDynArray< THandle< CJournalCreatureHuntingClue > >, out, TDynArray< THandle< CJournalCreatureHuntingClue > >() );
	FINISH_PARAMETERS;

	CJournalQuest* quest = questHandle.Get();

	if( quest && quest->IsMonsterHuntQuest() )
	{
		TEntryGroup* foundClues = m_foundHuntingQuestClues.FindPtr( quest->GetGUID() );
		
		if( foundClues )
		{
			FillHandleArrayWithEntryGroup( out, foundClues );
		}
	}
}

void CWitcherJournalManager::funcGetTrackedQuestObjectivesData( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< SJournalQuestObjectiveData >, outQuestObjectives, TDynArray< SJournalQuestObjectiveData >() );
	FINISH_PARAMETERS;

	outQuestObjectives.Clear();

	const CJournalQuest* trackedQuest = GetTrackedQuest();
	if ( !trackedQuest )
	{
		return;
	}

	TDynArray< const CJournalQuestObjective* > questObjectives;
	GetTrackedQuestObjectives( questObjectives );

	for ( TDynArray< const CJournalQuestObjective* >::const_iterator it = questObjectives.Begin(); it != questObjectives.End(); ++it )
	{
		const CJournalQuestObjective* questObjective = *it;
		outQuestObjectives.PushBack( SJournalQuestObjectiveData() );
		SJournalQuestObjectiveData& data = outQuestObjectives.Back();
		data.m_status = GetEntryStatus( questObjective );
		data.m_objectiveEntry = questObjective;
	}
}

void CWitcherJournalManager::funcGetQuestHasMonsterKnown( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, entryHandle, NULL );
	FINISH_PARAMETERS;

	CJournalBase* entry = entryHandle.Get();
	if ( entry )
	{
		RETURN_BOOL( m_knownMonsterQuests.Find( entry->GetGUID() ) != m_knownMonsterQuests.End() );
		return;
	}

	RETURN_BOOL( false );
}

void CWitcherJournalManager::funcSetQuestHasMonsterKnown( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, entryHandle, NULL );
	GET_PARAMETER( Bool, isKnown, true );
	FINISH_PARAMETERS;

	CJournalBase* entry = entryHandle.Get();
	if ( entry )
	{
		SetQuestMonsterAvailable( isKnown, entry->GetGUID() );
	}
}

void CWitcherJournalManager::funcGetEntryHasAdvancedInfo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, entryHandle, NULL );
	FINISH_PARAMETERS;

	CJournalBase* entry = entryHandle.Get();
	if ( entry )
	{
		RETURN_BOOL( m_entriesWithAdvancedInformation.Find( entry->GetGUID() ) != m_entriesWithAdvancedInformation.End() );
		return;
	}

	RETURN_BOOL( false );
}

void CWitcherJournalManager::funcSetEntryHasAdvancedInfo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalBase >, entryHandle, NULL );
	GET_PARAMETER( Bool, hasAdvancedInfo, true );
	FINISH_PARAMETERS;

	CJournalBase* entry = entryHandle.Get();
	if ( entry )
	{
		SetHasAdvancedInformation( hasAdvancedInfo, entry->GetGUID() );
	}
}

void CWitcherJournalManager::funcGetQuestObjectiveCount( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CGUID, questGUID, CGUID() );
	FINISH_PARAMETERS;
	RETURN_INT( GetQuestObjectiveCount( questGUID ) )
}

void CWitcherJournalManager::funcSetQuestObjectiveCount( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CGUID, questGUID, CGUID() );
	GET_PARAMETER( Int32, count , 0 );
	FINISH_PARAMETERS;
	SetQuestObjectiveCount( questGUID, count);
}

void CWitcherJournalManager::funcGetCreatureParams( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, entityTemplateFilename , 0 );
	GET_PARAMETER_REF( SJournalCreatureParams, data, SJournalCreatureParams() );
	FINISH_PARAMETERS;

	data.m_abilities.ClearFast();
	data.m_autoEffects.ClearFast();
	data.m_buffImmunity.Clear();
	data.m_monsterCategory = 0;
	data.m_isTeleporting = false;
	data.m_droppedItems.ClearFast();

	if ( entityTemplateFilename.Empty() )
	{
		RETURN_BOOL( false )
		return;
	}
	CEntityTemplate* entityTemplate = Cast< CEntityTemplate > ( GDepot->LoadResource( entityTemplateFilename ) );
	if ( !entityTemplate )
	{
		RETURN_BOOL( false )
		return;
	}

	TDynArray< CGameplayEntityParam* > params;
	entityTemplate->CollectGameplayParams( params , CCharacterStatsParam::GetStaticClass() );
	for( Uint32 i = 0; i < params.Size(); ++i  )
	{
		CCharacterStatsParam* param = Cast< CCharacterStatsParam >( params[ i ] );
		if ( param )
		{
			data.m_abilities.PushBackUnique( param->GetAbilities() );
		}
	}
	params.ClearFast();

	entityTemplate->CollectGameplayParams( params , CAutoEffectsParam::GetStaticClass() );
	for( Uint32 i = 0; i < params.Size(); ++i  )
	{
		CAutoEffectsParam* param = Cast< CAutoEffectsParam >( params[ i ] );
		if ( param )
		{
			data.m_autoEffects.PushBackUnique( param->GetAutoEffects() );
		}
	}
	params.ClearFast();

	entityTemplate->CollectGameplayParams( params , CBuffImmunityParam::GetStaticClass() );
	for( Uint32 i = 0; i < params.Size(); ++i  )
	{
		CBuffImmunityParam* param = Cast< CBuffImmunityParam >( params[ i ] );
		if ( param )
		{
			data.m_buffImmunity.m_immunityTo.PushBackUnique( param->GetImmunities() );
			Int32 flags = param->GetFlags();
			data.m_buffImmunity.m_potion		= data.m_buffImmunity.m_potion		|| ((flags & IF_Potion)    );	 
			data.m_buffImmunity.m_positive		= data.m_buffImmunity.m_positive	|| ((flags & IF_Positive)  ); 
			data.m_buffImmunity.m_negative		= data.m_buffImmunity.m_negative	|| ((flags & IF_Negative)  );
			data.m_buffImmunity.m_neutral		= data.m_buffImmunity.m_neutral		|| ((flags & IF_Neutral)   );
			data.m_buffImmunity.m_immobilize	= data.m_buffImmunity.m_immobilize	|| ((flags & IF_Immobilize)); 
			data.m_buffImmunity.m_confuse		= data.m_buffImmunity.m_confuse		|| ((flags & IF_Confuse)   ); 
			data.m_buffImmunity.m_damage		= data.m_buffImmunity.m_damage		|| ((flags & IF_Damage)    ); 
		}
	}
	params.ClearFast();

	entityTemplate->CollectGameplayParams( params , CMonsterParam::GetStaticClass() );
	for( Uint32 i = 0; i < params.Size(); ++i  )
	{
		CMonsterParam* param = Cast< CMonsterParam >( params[ i ] );
		if ( param )
		{
			data.m_monsterCategory = param->GetMonsterCategory();
			data.m_isTeleporting = param->IsMonsterTeleporting();
		}
	}
	params.ClearFast();

	const CLootDefinitions* lootDefs = GCommonGame->GetDefinitionsManager()->GetLootDefinitions();
	if ( lootDefs )
	{
		TDynArray< const CR4LootContainerParam* > containers;
		entityTemplate->CollectGameplayParams( params , CR4LootParam::GetStaticClass() );
		for( Uint32 i = 0; i < params.Size(); ++i  )
		{
			CR4LootParam* param = Cast< CR4LootParam >( params[ i ] );
			if ( param )
			{
				param->GetContainersParams( containers );
				for ( Uint32 i = 0; i < containers.Size(); i++ )
				{
					CName aa = containers[ i ]->GetName();
					const CR4LootContainerDefinition* lootContainerDef = Cast< CR4LootContainerDefinition >( lootDefs->GetDefinition( aa ) );
					if ( lootContainerDef )
					{
						const TDynArray< CR4LootItemDefinition >& itemDef = lootContainerDef->GetItems();
						for ( Uint32 j = 0; j < itemDef.Size(); j++ )
						{
							data.m_droppedItems.PushBackUnique( itemDef[ j ].GetName() );
						}
					}
				}
			}
		}
	}
	params.ClearFast();

	RETURN_BOOL( true )
}

void CWitcherJournalManager::funcToggleDebugInfo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, debugInfo, 0 );
	FINISH_PARAMETERS;

	m_debugInfo = debugInfo;
}

void CWitcherJournalManager::funcShowLoadingScreenVideo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, debugVideo, false );
	FINISH_PARAMETERS;

	m_debugVideo = debugVideo;
}

void CWitcherJournalManager::funcGetQuestRewards( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CJournalQuest >, questHandle, NULL );
	FINISH_PARAMETERS;

	if ( result )
	{
		TDynArray< CName >& retVal = *(TDynArray< CName >*) result;
		retVal.ClearFast();

		CJournalQuest* quest = questHandle.Get();
		if( quest )
		{
			String questPath = quest->GetQuestPhasePath();
			if ( !questPath.Empty() )
			{
				GetQuestRewards( questPath, retVal );
			}
		}
	}
}

void CWitcherJournalManager::funcGetRegularQuestCount( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_regularQuestCount );
}

void CWitcherJournalManager::funcGetMonsterHuntQuestCount( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_monsterHuntQuestCount );
}

void CWitcherJournalManager::funcGetTreasureHuntQuestCount( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_treasureHuntQuestCount );
}

void CWitcherJournalManager::funcGetQuestProgress( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( GetQuestProgress() );
}

void CWitcherJournalManager::funcGetJournalAreasWithQuests( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( result )
	{
		TDynArray< Int32 > & retVal = *(TDynArray< Int32 >*) result;
		retVal.ClearFast();

		TDynArray< const CJournalQuestObjective* > trackedObjectives;
		GetTrackedQuestObjectives( trackedObjectives );

		for ( Uint32 i = 0; i < trackedObjectives.Size(); ++i )
		{
			retVal.PushBackUnique( static_cast< Int32 >( trackedObjectives[ i ]->GetWorld() ) );
		}
	}
}

void CWitcherJournalManager::funcForceSettingLoadingScreenVideoForWorld( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, area, 0 );
	FINISH_PARAMETERS;

	const String& videoFile = GetCurrentStorybookVideoName( area );
	GGame->ReplaceDefaultLoadingScreenVideo( videoFile );
}

void CWitcherJournalManager::funcForceSettingLoadingScreenContextNameForWorld( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, contextName, CName::NONE );
	FINISH_PARAMETERS;

	GGame->ReplaceDefaultLoadingScreenName( contextName );
}

void CWitcherJournalManager::funcForceUntrackingQuestForEP1Savegame( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	m_trackedQuest = -1;
}

Uint32 CWitcherJournalManager::GetDisplayNameIndexForSavedGame() 
{
	const CJournalQuest* trackedQuest = GetTrackedQuest();
	if ( trackedQuest )
	{
		// title can contain forbidden characters
		Uint32 currentValue = trackedQuest->GetTitleIndex();
		if ( currentValue > 0 )
		{
			m_cachedDisplayNameIndexForSave = currentValue;
		}
	}
	
	return m_cachedDisplayNameIndexForSave;
}

Bool CWitcherJournalManager::CanActivateEntry( const CJournalBase* target, EJournalStatus status )
{
	if ( status == JS_Failed )
	{
		if ( target && target->IsA< CJournalQuest >() )
		{
			EJournalStatus status = GetEntryStatus( target );
			if ( status == JS_Inactive )
			{
				// TTP 95719 - don't fail quest that has never been activated or been deactivated
				return false;
			}
		}
	}
	return true;
}
