/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "gameSession.h"
#include "gameResource.h"
#include "../core/loadingJobManager.h"
#include "../core/garbageCollector.h"
#include "../core/feedback.h"
#include "../core/loadingProfiler.h"
#include "../engine/worldStreaming.h"
#include "../core/gameSave.h"
#include "game.h"
#include "../engine/gameTime.h"
#include "renderFrame.h"
#include "baseEngine.h"
#include "renderer.h"
#include "gameSaveManager.h"
#include "../core/contentManager.h"
#include "../core/messagePump.h"
#include "soundSystem.h"

RED_DEFINE_STATIC_NAME( runtimeGUIDCounter )

IMPLEMENT_RTTI_ENUM( ESaveAttemptResult )
IMPLEMENT_RTTI_ENUM( ESessionRestoreResult )

//////////////////////////////////////////////////////////////////////////

STeleportInfo::STeleportInfo()
	: m_targetType( TargetType_Unused )
	, m_distanceFromTarget( 0 )
	, m_targetPosition( Vector::ZEROS )
	, m_targetRotation( EulerAngles::ZEROS )
	, m_offset( Vector::ZEROS )
{}

void STeleportInfo::SetTarget( TargetType targetType, const TagList& targetTag, Float distanceFromTarget, const Vector& offset, const TagList& actorsTags )
{ 
	m_targetType = targetType; 
	m_targetTag = targetTag; 
	m_distanceFromTarget = distanceFromTarget; 
	m_offset = offset; 
	m_actorsTags = actorsTags; 
}

void STeleportInfo::SetTarget( TargetType targetType, const CName& targetTag, const CName& actorTag ) 
{ 
	m_targetType = targetType; 
	m_targetTag.Clear();  
	m_targetTag.AddTag( targetTag ); 
	m_actorsTags.Clear(); 
	m_actorsTags.AddTag( actorTag );
}

void STeleportInfo::SetTarget( TargetType targetType, const Vector& position, const EulerAngles& rotation, const CName& actorTag ) 
{ 
	m_targetType = targetType; 
	m_targetPosition = position; 
	m_targetRotation = rotation; 
	m_actorsTags.Clear(); 
	m_actorsTags.AddTag( actorTag ); 
}

void STeleportInfo::SetTarget( TargetType targetType, const TagList& targetTag ) 
{ 
	m_targetType = targetType; 
	m_targetTag = targetTag; 
	m_actorsTags.Clear();
}

//////////////////////////////////////////////////////////////////////////

CGameSessionManager::CGameSessionManager()
	: m_nextLockIdx( 0 )
	, m_showBlackscreenAfterRestore( false )
	, m_saveLockFor5sec( GAMESAVELOCK_INVALID )
{
}

CGameSessionManager::~CGameSessionManager()
{
}

// Introduce save lock for 5 seconds - this is needed to meet TCR 047
void CGameSessionManager::CreateTimedLock( Float lockDuration )
{
	if ( m_saveLockCountdown <= 0.f )
	{
		CreateNoSaveLock( TXT("5 seconds after save attempt"), m_saveLockFor5sec, false, true );
	}

	m_saveLockCountdown = lockDuration;
}

void CGameSessionManager::UpdateLocks( Float timeDelta )
{
	if ( m_saveLockCountdown > 0.f )
	{
		m_saveLockCountdown -= timeDelta;
	}

	if ( m_saveLockCountdown <= 0.f && m_saveLockFor5sec != GAMESAVELOCK_INVALID )
	{
		ReleaseNoSaveLock( m_saveLockFor5sec );
		m_saveLockFor5sec = GAMESAVELOCK_INVALID;
	}
}

GameTime CGameSessionManager::CalculateTimePlayed() const
{
	Red::System::DateTime gameTimePlayed;
	gameTimePlayed.SetRaw( m_sessionHistoryTimePlayed + CalculateCurrentSessionTime() );

	return GameTime( gameTimePlayed.GetDay(), gameTimePlayed.GetHour(), gameTimePlayed.GetMinute(), gameTimePlayed.GetSecond() );
}

Bool CGameSessionManager::ContainsHistoryEventFromVersion( Uint16 version ) const
{
	for ( const auto& sessionEvent : m_sessionHistory )
	{
		if ( sessionEvent.m_saveVersion == version )
		{
			return true;
		}
	}

	return false;
}

Bool CGameSessionManager::IsNewGame() const
{
	return m_gameInfo.IsNewGame() && !SHIsLoadedGame();
}

Bool CGameSessionManager::IsStandaloneDLCStarting() const
{
	return m_gameInfo.IsStandaloneDLCStarting();
}

void CGameSessionManager::OnEditorLoadGame()
{
	#ifndef NO_EDITOR
		if ( GIsEditor )
		{
			// yes, it's a hack :(
			m_gameInfo.m_isChangingWorldsInGame = false;
		}
	#endif
}

Uint64 CGameSessionManager::CalculateCurrentSessionTime() const
{
	Red::System::DateTime currentTime;
	Red::Clock::GetInstance().GetLocalTime( currentTime );
	return currentTime.GetRaw() - m_lastSessionHistoryTime.GetRaw();
}

void CGameSessionManager::GenerateDebugFragments( CRenderFrame* frame )
{
#ifndef RED_FINAL_BUILD
	// Message
	Uint32 x = 5;
	Uint32 y = 160;

	// Process debug messages
	for ( auto& msg : m_debugMessages )
	{
		frame->AddDebugScreenFormatedText( x, y, msg.m_color, msg.m_message.AsChar() );
		y += 15;

		if ( false == msg.m_saveLocksDebug.Empty() )
		{
			frame->AddDebugScreenFormatedText( x, y, Color::RED, TXT("Save locks were present:") );
			y += 15;

			for ( const auto& lock : msg.m_saveLocksDebug )
			{
				frame->AddDebugScreenFormatedText( x + 15, y, Color::RED, TXT("%ls"), lock.GetDebugString().AsChar() );
				y += 15;
			}
		}
		else
		{
			y += 5;
		}
	}

	for ( Int32 i = m_debugMessages.SizeInt() - 1; i >= 0; --i )
	{
		auto& msg = m_debugMessages[ i ];
		msg.m_timeout -= GEngine->GetLastTimeDelta();
		if ( msg.m_timeout <= 0 )
		{
			m_debugMessages.RemoveAt( i );
		}
	}

	// Old save message
	if ( IsOldSave() )
	{
		frame->AddDebugScreenFormatedText( x, y, Color::RED, TXT("PLAYING FROM A VERY OLD SAVE. EXPECT TROUBLE.") );
		y += 20;
	}

	const Uint32 taintedFlags = GContentManager ? GContentManager->GetTaintedFlags() : 0;
	if ( taintedFlags != 0 )
	{
		// TBD: display other taint flags
		if ( (taintedFlags & eContentTaintedFlag_Mods) != 0 )
		{
			frame->AddDebugScreenFormatedText( x, y, Color::YELLOW, TXT("PLAYING FROM A SAVE GAME THAT USED MODS. EXPECT THE UNEXPECTED.") );
			y += 20;
		}
	}

	// Save lock list
	if ( m_saveLocks.Size() && frame->GetFrameInfo().IsShowFlagOn( SHOW_OnScreenMessages ) )
	{
		// No saves
		frame->AddDebugScreenFormatedText( x, y, Color::RED, TXT("SAVE LOCKS ACTIVE NOW:") );
		y += 20;

		// Add reasons
		Uint32 i = 0;
		for ( THashMap< CGameSaveLock, SLockData >::const_iterator it = m_saveLocks.Begin(); it != m_saveLocks.End(); ++it, ++i )
		{
			frame->AddDebugScreenFormatedText( x + 15, y, Color::RED, TXT("[%i]: %ls"), i, it->m_second.GetDebugString().AsChar() );
			y += 15;
		}
	}

	// World layer streaming fences
	{
		TDynArray< CWorldLayerStreamingFence::DebugInfo > infos;
		CWorldLayerStreamingFence::GatherDebugInformation( infos );

		if ( !infos.Empty() )
		{
			y += 15;

			// No saves
			frame->AddDebugScreenFormatedText( x, y, Color::YELLOW, TXT("LAYER STREAMING") );
			y += 20;

			// Add reasons (with still unloaded layers)
			for ( const auto& info : infos )
			{
				if ( info.m_isCompleted )
				{
					frame->AddDebugScreenFormatedText( x, y, Color::GREEN, TXT("%ls - loaded in %1.2fs"), info.m_name.AsChar(), info.m_timeCompleted );
				}
				else if ( info.m_isStarted )
				{
					frame->AddDebugScreenFormatedText( x, y, Color::YELLOW, TXT("%ls - loading for %1.2fs"), info.m_name.AsChar(), info.m_timeSinceIssue );
				}
				else
				{
					frame->AddDebugScreenFormatedText( x, y, Color::RED, TXT("%ls - pending for %1.2fs"), info.m_name.AsChar(), info.m_timeSinceStart );
				}
				y += 15;

				// not loaded layers
				for ( const String& path : info.m_layersNotLoaded )
				{
					frame->AddDebugScreenFormatedText( x+30, y, Color::GRAY, path.AsChar() );
					y += 15;
				}

				y += 5;
			}
		}
	}
#endif
}

void CGameSessionManager::CreateNoSaveLock( const String& reason, CGameSaveLock& saveLock, Bool unique, Bool allowCheckpoints /* = true (!) */ )
{
	if ( unique && m_saveLocks.FindByValue( SLockData( reason ) ) != m_saveLocks.End() )
	{
		WARN_ENGINE( TXT("Trying to add a unique already existing lock. Please DEBUG.") );
		return;
	}

	if ( saveLock != CGameSessionManager::GAMESAVELOCK_INVALID )
	{
		ReleaseNoSaveLock( saveLock );
	}

	// Create lock
	saveLock = m_nextLockIdx++;
	Uint8 lockedTypes = SLockData::DEFAULT_LOCKED_TYPES & ( allowCheckpoints ? ~Uint8( FLAG( SGT_CheckPoint ) ) : 0xff );
	m_saveLocks.Insert( saveLock, SLockData( lockedTypes, unique, reason ) );

	// Stats
	const Uint32 count = m_saveLocks.Size();
	RED_LOG( Save, TXT("NoSave lock '%ls'. Lock count: %i"), reason.AsChar(), count );
}

void CGameSessionManager::ReleaseNoSaveLock( CGameSaveLock lock )
{
	if ( lock == CGameSessionManager::GAMESAVELOCK_INVALID )
	{
		return;
	}

	// Remove lock
	THashMap< CGameSaveLock, SLockData >::iterator it = m_saveLocks.Find( lock );
	if ( it == m_saveLocks.End() )
	{
		WARN_ENGINE( TXT("Trying to remove non existing save lock. Please DEBUG.") );
		return;
	}

	// Stats
	const Uint32 count = m_saveLocks.Size() - 1;
	RED_LOG( Save, TXT("NoSave unlock '%ls'. Lock count: %i"), it->m_second.m_reason.AsChar(), count );
	m_saveLocks.Erase( lock );
}

void CGameSessionManager::ReleaseNoSaveLockByName( const String& lockName )
{
	if ( lockName == String::EMPTY )
	{
		RED_LOG( Save, TXT( "NoSave : trying to unloc save with no name" ) );
		return;
	}

	if( m_saveLocks.EraseByValue( SLockData( lockName ) ) )
	{
		const Uint32 count = m_saveLocks.Size() - 1;
		RED_LOG( Save, TXT("NoSave unlock '%ls'. Lock count: %i"), lockName.AsChar(), count );
	}
	else
	{
		const Uint32 count = m_saveLocks.Size() - 1;
		RED_LOG( Save, TXT("Not found NoSave '%ls'. Lock count: %i"), lockName.AsChar(), count );
	}
}

struct SScopedLoadingScreen : Red::System::NonCopyable
{
	SScopedLoadingScreen()
	{
		GGame->ShowLoadingScreen();
	}

	~SScopedLoadingScreen()
	{
		GGame->HideLoadingScreen();
	}
};

ESessionRestoreResult CGameSessionManager::CreateExpansionSession( IGameLoader* gameLoader, Bool suppressVideoToPlay /*= false*/ )
{
	ESessionRestoreResult result = RestoreSession( gameLoader, suppressVideoToPlay, true );
	if ( result == RESTORE_Success )
	{
		OnSHGameStarted();
	}
	return result;
}

ESessionRestoreResult CGameSessionManager::RestoreSession( IGameLoader* gameLoader, Bool suppressVideoToPlay /*= false*/, Bool isStandaloneDLCStart /*= false*/ )
{
	SCOPED_ENABLE_PUMP_MESSAGES_DURANGO_CERTHACK();

#ifdef RED_LOGGING_ENABLED
	{
		const Bool launch0Available = GContentManager->IsContentAvailable( CNAME(launch0) );
		LOG_ENGINE(TXT("RestoreSession: launch0 available [%ls]"), (launch0Available ? TXT("y") : TXT("n") ) );
		if ( ! launch0Available )
		{
			ERR_ENGINE(TXT("=== RestoreSession: launch0 *NOT* available === WE STARTED THE GAME BEFORE WE SHOULD HAVE BEEN ALLOWED TO!"));
			return RESTORE_InternalError;
		}
	}
#endif

	m_missingContent.Clear();
	GContentManager->ResetActivatedContent();

	// In case we didn't clean up when ending some previous session
	GGame->ClearKosherTaintFlags( CGame::eKosherTaintFlag_Session );

	GLoadingProfiler.Start();

	if ( m_gameInfo.m_gameLoadStream )
	{
		delete m_gameInfo.m_gameLoadStream;
		m_gameInfo.m_gameLoadStream = nullptr;
	}

	// End current game
	if ( GGame->IsActive() )
	{
		GGame->EndGame();
		GLoadingProfiler.FinishStage( TXT("ClosingGame") );
	}

	// Do not call GC during this period
	GObjectGC->DisableGC();

	// Reset no save lock
	m_saveLocks.Clear();

	// Session
	{
		// Restore game resource
		{
			CGameSaverBlock block( gameLoader, GGame->GetGameResource()->GetClass()->GetName() );

			String path;
			gameLoader->ReadValue( CNAME( path ), path );
			if ( !GGame->SetupGameResourceFromFile( path ) )
			{
				RED_LOG( Session, TXT("ERROR: Unable to start game with %s"), path.AsChar() );
				return RESTORE_NoGameDefinition;
			}
		}

		// Game difficulty and game number
		{
			CGameSaverBlock block( gameLoader, CNAME( saveInfo ) );

			TDynArray< Uint8 > buffer;
			Uint32 difficulty;
			gameLoader->ReadValue( CNAME( magic_number ), buffer ); // ! difficulty - variable name changed for deception
			UnwrapIntegerFromSomeNoise< Uint32 >( difficulty, buffer, 3 );
			GGame->SetGameDifficulty( difficulty );

			String saveDescription;
			gameLoader->ReadValue( CNAME( description ), saveDescription );

			Uint64 runtimeGUIDCounter;
			gameLoader->ReadValue( CNAME( runtimeGUIDCounter ), runtimeGUIDCounter );
			Red::System::CRuntimeGUIDGenerator::SetRuntimeCounter( runtimeGUIDCounter );

			Uint32 numHistory = gameLoader->ReadValue< Uint32 >( CNAME( count ), 0 );
			m_sessionHistory.Resize( numHistory );
			m_sessionHistoryTimePlayed = 0;
			for ( Uint32 i = 0; i < numHistory; ++i )
			{
				auto& evt = m_sessionHistory[ i ];
				evt.Load( gameLoader );
				evt.Log();

				switch ( evt.m_type )
				{
				case SSessionHistoryEvent::EVENT_GameSaved:
					m_sessionHistoryTimePlayed += ( evt.m_time.GetRaw() - m_lastSessionHistoryTime.GetRaw() );
					m_lastSessionHistoryTime = evt.m_time;
					break;

				case SSessionHistoryEvent::EVENT_GameStarted:
				case SSessionHistoryEvent::EVENT_GameLoaded:
					m_lastSessionHistoryTime = evt.m_time;
					break;
				}
			}
		}

		OnSHGameLoaded( gameLoader->GetSaveVersion() );

		// World info
		String currentWorld;
		{
			CGameSaverBlock block( gameLoader, CNAME(worldInfo) );
			gameLoader->ReadValue( CNAME(world), currentWorld );
		}

		// Additional string info
		Bool showBlackscreenAfterRestore( false );
		{
			CGameSaverBlock block( gameLoader, CNAME(AdditionalSaveInfo) );

			Bool isKosher = true;
			if ( gameLoader->GetSaveVersion() >= SAVE_VERSION_KOSHER_CHECK_PART_DEUX )
			{
				gameLoader->ReadValue( CNAME(HonorSystem2), isKosher );
				LOG_ENGINE(TXT("Checking for HonorSystem2: %d"), isKosher );
			}
			else if ( gameLoader->GetSaveVersion() >= SAVE_VERSION_KOSHER_CHECK )
			{
				// "HonorSystem" is present only in saves between SAVE_VERSION_KOSHER_CHECK and SAVE_VERSION_KOSHER_CHECK_PART_DEUX
				Bool oldIsKosher = false;
				gameLoader->ReadValue( CNAME(HonorSystem), oldIsKosher );
			}
			LOG_ENGINE(TXT("Kosher save game? %d"), isKosher );

			// Only define for PC, just in case. No sense for console.
#ifdef RED_PLATFORM_WINPC
			if ( !isKosher )
			{
				GGame->SetKosherTaintFlags( CGame::eKosherTaintFlag_Session );
			}
#endif

			gameLoader->ReadValue( CNAME( SavedBlackscreen ), showBlackscreenAfterRestore );
			if ( gameLoader->GetSaveVersion() < SAVE_VERSION_INCLUDE_PLAYER_STATE )
			{
				showBlackscreenAfterRestore = false; // Do not use data from old saves
			}

			// Loading screen info
			String videoToPlay;
			CName loadingScreenName;
			if ( gameLoader->GetSaveVersion() >= SAVE_QUEST_LOADING_VIDEO )
			{
				gameLoader->ReadValue( CNAME(QuestLoadingVideo), videoToPlay );
				LOG_ENGINE(TXT("Loaded QuestLoadingVideo '%ls' from save"), videoToPlay.AsChar() );
			}

			if ( gameLoader->GetSaveVersion() >= SAVE_VERSION_LOADING_SCREEN_NAME )
			{
				gameLoader->ReadValue( CNAME(LoadingScreenName), loadingScreenName );
				LOG_ENGINE(TXT("Loaded LoadingScreenName '%ls' from save"), loadingScreenName.AsChar() );
			}

			if ( !loadingScreenName )
			{
				LOG_ENGINE(TXT("LoadingScreenName 'NONE' - changing to 'default'"));
				loadingScreenName = CNAME(default);
			}

			String emptyInitString; // We don't persist instance data like this, at least not this save version. Only if necessary.
			GGame->ReplaceDefaultLoadingScreenParam( SLoadingScreenParam( loadingScreenName, emptyInitString, videoToPlay ) );

			if ( gameLoader->GetSaveVersion() >= SAVE_VERSION_CONTENT_TAINTED_FLAGS )
			{
				Uint32 taintedFlags = 0;
				gameLoader->ReadValue( CNAME(ContentTaintedFlags), taintedFlags );
				GContentManager->AddTaintedFlags( taintedFlags );
			}
			else
			{
				// There is no escape
				GContentManager->AddTaintedFlags( eContentTaintedFlag_BeforeTaintedFlags );
			}

			if ( (GContentManager->GetTaintedFlags() & eContentTaintedFlag_AllContentActivated) != 0 )
			{
				// Activate since it might not be in the savegame required content depending on the platform userprofile manager.
				LOG_ENGINE(TXT("CContentManager - eContentTaintedFlag_AllContentActivated - reactivating all content"));
				GContentManager->ActivateAllContentForDebugQuests();
			}

			if ( gameLoader->GetSaveVersion() >= SAVE_VERSION_ACTIVATED_CONTENT_IN_SAVE )
			{
				const TDynArray< CName >& baseContent = GContentManager->GetBaseContent();
				for ( const CName contentName : baseContent )
				{
					Bool isActivated = false;
					gameLoader->ReadValue( contentName, isActivated );
					LOG_ENGINE(TXT("Content '%ls' activated from save=%d"), contentName.AsChar(), (isActivated ? 1 : 0) );
					if ( isActivated )
					{
						if ( !GContentManager->ActivateContent( contentName ) )
						{
							m_missingContent.PushBack( contentName );
							ERR_ENGINE(TXT("Unable to activate content '%ls' from savegame!"), contentName.AsChar() );				
						}
					}
				}
			}
		}

		if ( !m_missingContent.Empty() )
		{
			delete gameLoader;
			m_gameInfo.m_gameLoadStream = nullptr;
			return RESTORE_MissingContent;	
		}

		if ( false == GGame->LoadRequiredDLCs( gameLoader, m_missingContent ) )
		{
			delete gameLoader;
			m_gameInfo.m_gameLoadStream = nullptr;

			RED_LOG( Save, TXT("DLCs were required, loading aborted.") );
			return RESTORE_DLCRequired;
		}

		// time scale sets
		TSortedArray< STimeScaleSourceSet, STimeScaleSourceSet::CompareFunc >& timeScaleSets = GGame->GetTimeScaleSets();
		timeScaleSets.Clear();

		Uint32 timeScaleSetCount = 0;
		{
			CGameSaverBlock block( gameLoader, CNAME(TimeScale) );

			gameLoader->ReadValue( CNAME( count ), timeScaleSetCount );
			if ( timeScaleSetCount > 0 )
			{
				timeScaleSets.Reserve( timeScaleSetCount );
				for ( Uint32 i = 0; i < timeScaleSetCount; ++i )
				{
					CGameSaverBlock block( gameLoader, CNAME( TimeScaleSet ) );

					STimeScaleSourceSet set;
					gameLoader->ReadValue/*< STimeScaleSourceSet >*/( CNAME( TimeScaleSet ), set );
					timeScaleSets.Insert( set );
				}
			}
		}

		GLoadingProfiler.FinishStage( TXT("RestoreSession") );

		// Start game on loaded world
		CGameInfo newGameInfo;
		newGameInfo.m_hideCursor = true;
		newGameInfo.m_inEditorGame = GIsEditor;
		newGameInfo.m_keepExistingLayers = false;
		newGameInfo.m_worldFileToLoad = currentWorld;
		newGameInfo.m_gameLoadStream = gameLoader;
		newGameInfo.m_allowQuestsToRun = true;
		newGameInfo.m_isChangingWorldsInGame = !GGame->GetActiveWorld() ? false : ( currentWorld != GGame->GetActiveWorld()->GetDepotPath() );
		newGameInfo.m_setBlackscreen = showBlackscreenAfterRestore;
		newGameInfo.m_standaloneDLCJustStarting = isStandaloneDLCStart;
		m_gameInfo = newGameInfo;

		// Reset if anything left non-default on the stack
		GGame->ClearLoadingScreenParams();

		// if suppress video, push clone here, it'll get played, then popped. No worries about screwing stuff up!
		if ( suppressVideoToPlay )
		{
			SLoadingScreenParam tempParam = GGame->GetActiveLoadingScreenParam();
			tempParam.m_videoToPlay = String::EMPTY;
			GGame->PushLoadingScreenParam( tempParam );
		}

		SScopedLoadingScreen loadingScreen;
		GSoundSystem->OnGameStart( m_gameInfo );


		// Start the game
		Bool result = GGame->StartGame( m_gameInfo );
		// CJobIOThread::SetContinuedProcessing( true ); <-- moved inside

		// Start game
		if ( !result )
		{
			delete m_gameInfo.m_gameLoadStream;
			m_gameInfo.m_gameLoadStream = nullptr;

			RED_LOG( Session, TXT("ERROR: Unable to start game on loaded session.") );
			return RESTORE_InternalError;
		}

		{
			PC_SCOPE_PIX( SystemsInitializationComplete ); 

			Bool wasGameBeingLoaded = false;
			// the entire game state was restored
			if ( m_gameInfo.m_gameLoadStream )
			{
				wasGameBeingLoaded = true;
				delete m_gameInfo.m_gameLoadStream;
				m_gameInfo.m_gameLoadStream = nullptr;
			}

			// post loading GC
			{
				GObjectGC->EnableGC();
				GObjectGC->CollectNow();
				GLoadingProfiler.FinishStage( TXT("PostInitGC") );
			}
		}

		// Dump final states
		GLoadingProfiler.FinishLoading( TXT("RestoreSession") );

		LOG_ENGINE(TXT(">>> GContentManager->DumpProgressToLog() after RestoreSession()<<<"));
		GContentManager->DumpProgressToLog();

		// Session restored
		return RESTORE_Success;
	}
}

void CGameSessionManager::RegisterGameSaveSection( IGameSaveSection* section )
{
	ASSERT( !m_sections.Exist( section ) );
	
	// DEBUG CHECK ONLY
	CObject *objSrc = dynamic_cast< CObject* >( section );
	if ( objSrc )
	{
		for ( Uint32 i = 0; i < m_sections.Size(); ++i )
		{
			CObject *objDest = dynamic_cast< CObject* >( m_sections[i] );

			if ( objDest && objDest->GetClass() == objSrc->GetClass() )
			{
				LOG_ENGINE( TXT("CGameSessionManager::RegisterGameSaveSection(): Trying to register already registered object: %s"), objSrc->GetFriendlyName().AsChar() );
				ASSERT( !TXT("CGameSessionManager::RegisterGameSaveSection(): Trying to register already registered object. See log.") );
			}
		}
	}
	// DEBUG CHECK ONLY

	m_sections.PushBackUnique( section );
}

void CGameSessionManager::UnregisterGameSaveSection( IGameSaveSection* section )
{
	ASSERT( m_sections.Exist( section ) );
	m_sections.Remove( section );
}

void CGameSessionManager::DefineGameSystemDependency( IGameSystem* child, IGameSystem* parent /*= NULL */ )
{
	TDependenciesMap::iterator depIt = m_dependenciesMap.Find( parent );
	if ( depIt == m_dependenciesMap.End() )
	{
		m_dependenciesMap.Insert( parent, TDynArray< IGameSystem* >() );
		depIt = m_dependenciesMap.Find( parent );
	}
	ASSERT( depIt != m_dependenciesMap.End() );

	TDynArray< IGameSystem* >& dependencies = depIt->m_second;
	dependencies.PushBackUnique( child );
}

void CGameSessionManager::RemoveGameSystemDependency( IGameSystem* parent )
{
	// Hashmap class can deal with missing/invalid keys so we don't have to
	m_dependenciesMap.Erase( parent );
}

Bool CGameSessionManager::SaveSession( IGameSaver* gameSaver, const String& saveDescription /*= String::EMPTY*/ )
{
	// saving game requires some memory, so why not call GC...
	//SGarbageCollector::GetInstance().Collect();

	// Cannot save session if game is not active
	if ( !GGame->IsActive() )
	{
		RED_LOG( Session, TXT("Cannot save session if game is not active'") );
		return false;
	}

	CWorld* world = GGame->GetActiveWorld();
	ASSERT( world );

	GGame->GetUniverseStorage()->GetWorldStorage( world )->CaptureStateStage2();
	
	// Start	
	TIMER_BLOCK( time )

	// Save game resource
	{
		CGameSaverBlock block( gameSaver, GGame->GetGameResource()->GetClass()->GetName() );
		gameSaver->WriteValue( CNAME( path ), GGame->GetGameResourcePath() );
	}

	OnSHGameSaved();

	// Save game difficulty and game number
	{
		CGameSaverBlock block( gameSaver, CNAME( saveInfo ) );

		TDynArray< Uint8 > buffer;
		WrapIntegerWithSomeNoise< Uint32 >( GGame->GetDifficulty(), buffer, 3 );
		gameSaver->WriteValue( CNAME( magic_number ), buffer ); // ! difficulty - variable name changed for deception
	
		gameSaver->WriteValue( CNAME( description ), saveDescription );

		const Uint64 runtimeGUIDCounter = Red::System::CRuntimeGUIDGenerator::GetRuntimeCounter();
		gameSaver->WriteValue( CNAME( runtimeGUIDCounter ), runtimeGUIDCounter );

		gameSaver->WriteValue< Uint32 >( CNAME( count ), m_sessionHistory.Size() );
		for ( Uint32 i = 0; i < m_sessionHistory.Size(); ++i )
		{
			m_sessionHistory[ i ].Save( gameSaver );
		}
	}

	// Dump save sections
	for ( Uint32 i=0; i<m_sections.Size(); i++ )
	{
		IGameSaveSection* section = m_sections[i];
		if ( !section->OnSaveGame( gameSaver ) )
		{
			// Not saved
			return false;
		}
	}

	// Done
	END_TIMER_BLOCK( time )

	// Saved
	return true;
}


extern Bool GHackDelaySettingsSave;

Bool CGameSessionManager::CreateSession( const String& gameWorldFile, Bool isChangingWorld /*=false*/, const STeleportInfo* teleportInfo /*=nullptr*/, const String& loadingVideo /*= String::EMPTY*/, IGameLoader* playerLoadStream /* = nullptr */ )
{
	SCOPED_ENABLE_PUMP_MESSAGES_DURANGO_CERTHACK();

#ifdef RED_LOGGING_ENABLED
	{
		GContentManager->RefreshActivatedContent();
		const Bool launch0Available = GContentManager->IsContentAvailable( CNAME(launch0) );
		LOG_ENGINE(TXT("CreateSession: launch0 available [%ls]"), (launch0Available ? TXT("y") : TXT("n") ) );
		if ( ! launch0Available )
		{
			ERR_ENGINE(TXT("=== RestoreSession: launch0 *NOT* available === WE STARTED THE GAME BEFORE WE SHOULD HAVE BEEN ALLOWED TO!"));
		}

		LOG_ENGINE(TXT("RestoreSession currently activated content:"));
		for ( const CName content : GContentManager->GetActivatedContent() )
		{
			LOG_ENGINE(TXT("\t%ls"), content.AsChar() );
		}
	}
#endif

	// In case we didn't clean up when ending some previous session
	if ( !isChangingWorld )
	{
		GGame->ClearKosherTaintFlags( CGame::eKosherTaintFlag_Session );
	}

	// Reset if anything left non-default on the stack
	GGame->ClearLoadingScreenParams();

	// Override any default video
	if ( !loadingVideo.Empty() )
	{
		SLoadingScreenParam tempParam = GGame->GetActiveLoadingScreenParam();
		const String videoToPlay = loadingVideo.EqualsNC(TXT("none")) ? String::EMPTY : loadingVideo;
		if ( videoToPlay.Empty() )
		{
			// Force it to no video instead of using the default one
			LOG_ENGINE(TXT("videoToPlay is set to EMPTY (loadingVideo=%ls"), loadingVideo.AsChar());
		}
		tempParam.m_videoToPlay = loadingVideo;
		GGame->PushLoadingScreenParam( tempParam );
	}

	// Show loading screen
	SScopedLoadingScreen loadingScreen;

	// Update universe storage from world we're changing from when the loading screen is up if it's delayed too much
	if ( isChangingWorld )
	{
		const THandle< CWorld > activeWorld = GGame->GetActiveWorld();
		RED_FATAL_ASSERT( activeWorld, "Changing from inactive world?" );

		CUniverseStorage* universeStorage = GGame->GetUniverseStorage();
		RED_FATAL_ASSERT( universeStorage, "No universe storage?" );

		universeStorage->GetWorldStorage( activeWorld )->CaptureStateStage1( activeWorld );
		universeStorage->GetWorldStorage( activeWorld )->CaptureStateStage2();
		universeStorage->GetWorldStorage( activeWorld )->CaptureStateStage3();

		universeStorage->CapturePlayerManagedAttachments( GGame->GetPlayerEntity() );
	}

	GObjectGC->DisableGC();

	GLoadingProfiler.Start();

	// Delete current game load stream
	if ( m_gameInfo.m_gameLoadStream )
	{
		delete m_gameInfo.m_gameLoadStream;
		m_gameInfo.m_gameLoadStream = nullptr;
	}

	// Change world
	RED_LOG( Session, TXT("Creating session on '%ls'"), gameWorldFile.AsChar() );
	
	// End current game
	if ( GGame->IsActive() )
	{
		m_gameInfo.m_isChangingWorldsInGame = isChangingWorld;
		GGame->EndGame();
	}

	// Reset cnames reporter
	if ( false == isChangingWorld && nullptr == playerLoadStream )
	{
		m_namesReporter.Reset();
	}

	// Start game on loaded world
	CGameInfo newGameInfo;
	newGameInfo.m_hideCursor = true;
	newGameInfo.m_inEditorGame = isChangingWorld ? m_gameInfo.m_inEditorGame : false;
	newGameInfo.m_keepExistingLayers = false;
	newGameInfo.m_worldFileToLoad = gameWorldFile;
	newGameInfo.m_playerLoadStream = playerLoadStream;
	newGameInfo.m_isChangingWorldsInGame = isChangingWorld;
	newGameInfo.m_allowQuestsToRun = true;
	if ( teleportInfo )
	{
		newGameInfo.m_teleport = *teleportInfo;
	}
	m_gameInfo = newGameInfo;

	// Reset no save lock
	m_saveLocks.Clear();

	GSoundSystem->OnGameStart( newGameInfo );

	{
		Red::System::ScopedFlag< Bool > settingsFlag( GHackDelaySettingsSave = true, false );
		if ( !GGame->StartGame( newGameInfo ) )
		{
			m_gameInfo.m_playerLoadStream = nullptr; // it will be deleted outside, don't keep the pointer
			RED_LOG( Session, TXT("ERROR: Unable to start game on created session.") );
			return false;
		}

		m_gameInfo.m_playerLoadStream = nullptr; // it will be deleted outside, don't keep the pointer
	}

	SConfig::GetInstance().Save();

	// Post loading GC (relic)
	{
		GObjectGC->EnableGC();
		GObjectGC->CollectNow();
		GLoadingProfiler.FinishStage( TXT("PostInitGC") );
	}

	if ( !isChangingWorld )
	{
		OnSHGameStarted();
	}

	GLoadingProfiler.FinishLoading( TXT("CreateSession") );

	LOG_ENGINE(TXT(">>> GContentManager->DumpProgressToLog() after CreateSession()<<<"));
	GContentManager->DumpProgressToLog();

	// Created
	return true;
}

Bool CGameSessionManager::CreateEditorSession( Bool fastPlay, const Vector& cameraPosition, const EulerAngles& cameraRotation, Bool hideCursor, const String& loadingVideo /*= String::EMPTY*/ )
{
	RED_LOG( Session, TXT("Creating editor game session") );

	// In case we didn't clean up when ending some previous session
	GGame->ClearKosherTaintFlags( CGame::eKosherTaintFlag_Session );


	GLoadingProfiler.Start();

	if ( m_gameInfo.m_gameLoadStream )
	{
		delete m_gameInfo.m_gameLoadStream;
		m_gameInfo.m_gameLoadStream = nullptr;
	}

	m_missingContent.Clear();

	// Reset if anything left non-default on the stack
	GGame->ClearLoadingScreenParams();

	// Override any default video
	if ( !loadingVideo.Empty() )
	{
		SLoadingScreenParam tempParam = GGame->GetActiveLoadingScreenParam();
		const String videoToPlay = loadingVideo.EqualsNC(TXT("none")) ? String::EMPTY : loadingVideo;
		if ( videoToPlay.Empty() )
		{
			// Force it to no video instead of using the default one
			LOG_ENGINE(TXT("videoToPlay is set to EMPTY (loadingVideo=%ls"), loadingVideo.AsChar());
		}
		tempParam.m_videoToPlay = loadingVideo;
		GGame->PushLoadingScreenParam( tempParam );
	}

	// Show loading screen
	SScopedLoadingScreen loadingScreen;

	// No active world to start the game on :)
	if ( !GGame->GetActiveWorld() )
	{
		GFeedback->ShowWarn( TXT("Unable to start game on empty world") );
		return false;
	}

	// Game is already active
	if ( GGame->IsActive() )
	{
		GFeedback->ShowWarn( TXT("Game is already active, unable to start another one") );
		return false;
	}

	// Start game on loaded world
	CGameInfo newGameInfo;
	newGameInfo.m_inEditorGame = true;
	newGameInfo.m_keepExistingLayers = fastPlay;
	newGameInfo.m_cameraPosition = cameraPosition;
	newGameInfo.m_cameraRotation = cameraRotation;
	newGameInfo.m_hideCursor = hideCursor;
	newGameInfo.m_allowQuestsToRun = !fastPlay;
	m_gameInfo = newGameInfo;

	// Reset no save lock
	m_saveLocks.Clear();

	GSoundSystem->OnGameStart( newGameInfo );

	// Start game
	if ( !GGame->StartGame( newGameInfo ) )
	{
		RED_LOG( Session, TXT("ERROR: Unable to start game on created session.") );
		return false;
	}

	GLoadingProfiler.FinishLoading( TXT("CreateEditorSession") );

	OnSHGameStarted();

	// Done
	return true;
}

void CGameSessionManager::ShowDebugMessage( const String& mesg, const Color& color, Float timeout )
{
#ifndef FINAL
	SDebugMessage msg;
	msg.m_message = mesg;
	msg.m_timeout = timeout;
	msg.m_color = color;

	msg.m_saveLocksDebug.Reserve( m_saveLocks.Size() );
	for ( THashMap< CGameSaveLock, SLockData >::const_iterator it = m_saveLocks.Begin(); it != m_saveLocks.End(); ++it )
	{
		msg.m_saveLocksDebug.PushBack( it->m_second );		
	}

	m_debugMessages.PushBack( msg );
	RED_LOG( Save, TXT("Showed on screen message: %ls"), msg.m_message.AsChar() );

	PrintActiveSaveLocks();
#endif // ndef FINAL
}

void CGameSessionManager::EndSession()
{
	SCOPED_ENABLE_PUMP_MESSAGES_DURANGO_CERTHACK();

	// Set back to default so if loading a gamedef or savegame without a video, then we won't play the last set
	GGame->PushLoadingScreenParam( SLoadingScreenParam::BLACKSCREEN ); // Can't use post-process fade
	GGame->ShowLoadingScreen();
	m_gameInfo.m_isChangingWorldsInGame = false;
	GGame->EndGame();

	GSoundSystem->OnGameEnd();

	GGame->GetUniverseStorage()->Reset();
	GGame->HideLoadingScreen();
	GGame->SetBlackscreen( false, TXT("EndSession force fade off" ) );

	m_missingContent.Clear();
	GContentManager->ResetActivatedContent();

	GGame->ClearKosherTaintFlags( CGame::eKosherTaintFlag_Session );

	// Clear it last in case anything changed it during OnGameEnd()
	GGame->ClearLoadingScreenParams( SLoadingScreenParam::DEFAULT );
}

void CGameSessionManager::AddFailedSaveDebugMessage( ESaveGameType type, const Char* reason, const Char* initiatedBy )
{
#ifndef FINAL
	Red::DateTime currentTime;
	Red::Clock::GetInstance().GetLocalTime( currentTime );
	SDebugMessage msg;
	msg.m_message = CEnum::ToString< ESaveGameType > ( type ) + TXT(" failed: ") + reason + String::Printf( TXT(" time: %02ld:%02ld:%02ld (tick %lld) "), currentTime.GetHour(), currentTime.GetMinute(), currentTime.GetSecond(), GEngine->GetCurrentEngineTick() ) + initiatedBy;
	msg.m_timeout = MSG_TIME;
	msg.m_color = Color::RED;

	msg.m_saveLocksDebug.Reserve( m_saveLocks.Size() );
	for ( THashMap< CGameSaveLock, SLockData >::const_iterator it = m_saveLocks.Begin(); it != m_saveLocks.End(); ++it )
	{
		msg.m_saveLocksDebug.PushBack( it->m_second );		
	}

	m_debugMessages.PushBack( msg );
	RED_LOG( Save, TXT("Showed on screen message: %ls"), msg.m_message.AsChar() );

	PrintActiveSaveLocks();
#endif // ndef FINAL
}

void CGameSessionManager::PrintActiveSaveLocks() const
{
#ifndef FINAL
	if ( m_saveLocks.Empty() )
	{
		RED_LOG( Save, TXT("There were no save locks.") );
	}
	else
	{
		RED_LOG( Save, TXT("Active save locks:") );
		Uint32 i = 0;
		for ( THashMap< CGameSaveLock, SLockData >::const_iterator it = m_saveLocks.Begin(); it != m_saveLocks.End(); ++it, ++i )
		{
			RED_LOG( Save, TXT("%ld: %ls"), i, it->m_second.GetDebugString().AsChar() );
		}
	}
#endif // ndef FINAL
}

void CGameSessionManager::OnSHGameStarted()
{
	m_sessionHistory.ClearFast();
	const auto evt = SSessionHistoryEvent( SSessionHistoryEvent::EVENT_GameStarted, SAVE_VERSION );
	m_sessionHistory.PushBack( evt );
	m_sessionHistoryTimePlayed = 0;
	m_lastSessionHistoryTime = evt.m_time;
}

void CGameSessionManager::OnSHGameSaved()
{
	const auto evt = SSessionHistoryEvent( SSessionHistoryEvent::EVENT_GameSaved, SAVE_VERSION );
	m_sessionHistory.PushBack( evt );
	m_sessionHistoryTimePlayed += ( evt.m_time.GetRaw() - m_lastSessionHistoryTime.GetRaw() );
	m_lastSessionHistoryTime = evt.m_time;
}

void CGameSessionManager::OnSHGameLoaded( Uint32 version )
{
	const auto evt = SSessionHistoryEvent( SSessionHistoryEvent::EVENT_GameLoaded, version );
	m_sessionHistory.PushBack( evt );
	m_lastSessionHistoryTime = evt.m_time;
}

Bool CGameSessionManager::SHIsLoadedGame() const
{
	for ( const auto& evt : m_sessionHistory )
	{
		if ( evt.m_type == SSessionHistoryEvent::EVENT_GameLoaded )
			return true;
	}

	return false;
}

Bool CGameSessionManager::IsOldSave() const
{
	if ( m_sessionHistory.Empty() )
	{
		// Don't even have a session history! but I guess that means there's no save, so it can't be old? :/
		return false;
	}

	const SSessionHistoryEvent& evtZero = m_sessionHistory[ 0 ];
	if ( evtZero.m_type != SSessionHistoryEvent::EVENT_GameStarted )
	{
		// loaded save is older than this system. For now, return false, we don't know how old it is.
		return false;
	}

	Red::DateTime now;
	Red::Clock::GetInstance().GetLocalTime( now );

	struct Total 
	{
		static Int32 Days( const Red::DateTime d ) { return d.GetYear() * 365 /* yeah, i know */ + d.GetMonth() * 30 /* yeah, i know */ + d.GetDay(); }
	};

	Int32 daysDiff = ( Total::Days( now ) - Total::Days( evtZero.m_time ) );	// not really accurate, but we don't need accuracy here
	ASSERT( daysDiff >= 0 );

	return ( daysDiff > 30 );
}

Bool CGameSessionManager::AreGameSavesLocked( ESaveGameType type ) const
{
	for ( THashMap< CGameSaveLock, SLockData >::const_iterator it = m_saveLocks.Begin(); it != m_saveLocks.End(); ++it )
	{
		if ( it->m_second.m_lockedTypes & FLAG( type ) )
		{
			return true;
		}
	}

	return false;
}

const Float CGameSessionManager::MSG_TIME = 45.f;

CGameSessionManager::SSessionHistoryEvent::SSessionHistoryEvent(EType t, Uint32 v) : m_type( t )
	, m_saveVersion( Uint16( v ) )
{
	Red::Clock::GetInstance().GetLocalTime( m_time );
}

void CGameSessionManager::SSessionHistoryEvent::Save(IGameSaver* saver)
{
	saver->WriteValue< Uint64 > ( CNAME( time ), m_time.GetRaw() );
	saver->WriteValue< Uint8 > ( CNAME( type ), Uint8( m_type ) );
	saver->WriteValue< Uint16 > ( CNAME( v ), Uint16( m_saveVersion ) );
}

void CGameSessionManager::SSessionHistoryEvent::Load(IGameLoader* loader)
{
	Uint64 time = loader->ReadValue< Uint64 > ( CNAME( time ), 0 );
	ASSERT( time );
	if ( time )
	{
		m_time.SetRaw( time );
	}

	m_type = EType( loader->ReadValue< Uint8 > ( CNAME( type ), 0 ) );
	ASSERT( m_type );

	m_saveVersion = loader->ReadValue< Uint16 > ( CNAME( v ), 0 );
}

void CGameSessionManager::SSessionHistoryEvent::Log()
{
#ifdef RED_LOGGING_ENABLED
	static const Char* types[] = { TXT("NULL"), TXT("Game started"), TXT("Game saved"), TXT("Game loaded") };
	RED_LOG( Session, TXT("Session history: %s @ %02ld.%02ld.%04ld %02ld:%02ld (ver: %ld)"), types[ m_type ], m_time.GetDay() + 1, m_time.GetMonth() + 1, m_time.GetYear(), m_time.GetHour(), m_time.GetMinute(), m_saveVersion );
#endif
}

String CGameSessionManager::SLockData::GetDebugString() const
{
	return String::Printf( TXT("%ls, unique: %ls, locked types: %ls%ls%ls%ls"), m_reason.AsChar(),
		m_unique ? TXT("true") : TXT("false"),
		( m_lockedTypes & FLAG( SGT_AutoSave ) ) ? TXT(" auto") : TXT(""),	
		( m_lockedTypes & FLAG( SGT_QuickSave ) ) ? TXT(", quick") : TXT(""),
		( m_lockedTypes & FLAG( SGT_Manual ) ) ? TXT(", manual") : TXT(""),
		( m_lockedTypes & FLAG( SGT_CheckPoint ) ) ? TXT(", checkpoint") : TXT("") );
}
