#include "build.h"
#include "gameSaver.h"
#include "../../common/game/questsSystem.h"
#include "../../common/engine/registryAccess.h"
#include "../../common/core/jobGenericJobs.h"
#include "../../common/core/loadingJobManager.h"
#include "../../common/core/depot.h"
#include "../engine/localizationManager.h"
#include "../core/gameSave.h"
#include "../engine/gameSaveManager.h"
#include "../engine/screenshotSystem.h"
#include "../engine/layerGroup.h"
#include "../engine/layerInfo.h"
#include "../core/gatheredResource.h"
#include "journalManager.h"

#include "../engine/renderCommands.h"
#include "defaultSaveScreenshot.inl"
#include "../core/contentManager.h"

namespace Config
{
	TConfigVar< Bool > cvShowSaveCompatWarning( "Save", "ShowCompatWarning", true, eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
}

const Uint32 CGameSaver::SCREENSHOT_BUFFER_SIZE = 96 * 1024; // arbitrary value (needs to fit the compressed file)

CGameSaver::CGameSaver()
	: m_lastSaveTime()
	, m_lastAutoSaveAttemptTime()
	, m_lastSaveType( SGT_None )
	, m_requestedSaveType( SGT_None )
	, m_requestedSaveSlot( -1 )
	, m_realWriteSize( 0 )
	, m_screenShotBuffer( GUserProfileManager->GetScreenshotBuffer() )
	, m_busy( false )
	, m_screenshotDone( false )
	, m_waitingForScreenshot( false )
	, m_newGamePlusEnabled( false )
{
}

CGameSaver::~CGameSaver()
{
	m_saver.Reset();
}

Bool CGameSaver::DoSaveGame( CWorld* world, CWorldStorage* activeWorldStorage )
{
	LOG_ENGINE(TXT(">>> GContentManager->DumpProgressToLog() on SaveGame()") );
	GContentManager->DumpProgressToLog();

	TIMER_BLOCK( all )
		SSavegameInfo& info = m_currentSaveInfo;

		ASSERT( activeWorldStorage );

		// init saver
		IGameSaver* saver = nullptr;
		TIMER_BLOCK( init )
			activeWorldStorage->CaptureStateStage1( world );
		END_TIMER_BLOCK( init )

		TIMER_BLOCK( createSaver )
			saver = SGameSaveManager::GetInstance().CreateSaver( info );
			if ( nullptr == saver )
			{
				SGameSessionManager::GetInstance().AddFailedSaveDebugMessage( m_requestedSaveType, TXT("Cannot create the writer."), m_debugSaveReason.AsChar() );
				CancelSaving( activeWorldStorage, info.GetFileName().AsChar() );
				return false;
			}
			RED_LOG( Session, TXT("Saving session dump to '%ls'"), info.GetFileName().AsChar() );
		END_TIMER_BLOCK( createSaver )

		TIMER_BLOCK( session )
			if ( false == SGameSessionManager::GetInstance().SaveSession( saver ) )
			{
				SGameSessionManager::GetInstance().AddFailedSaveDebugMessage( m_requestedSaveType, TXT("Session saving failed."), m_debugSaveReason.AsChar() );
				CancelSaving( activeWorldStorage, info.GetFileName().AsChar() );
				delete saver;
				return false;
			}
			m_lastSaveTime = EngineTime::GetNow();
			m_lastSaveType = m_requestedSaveType;
		END_TIMER_BLOCK( session )

		// On screen message
		#ifndef FINAL
			Red::DateTime currentTime;
			Red::Clock::GetInstance().GetLocalTime( currentTime );
			SGameSessionManager::GetInstance().ShowDebugMessage( String::Printf( TXT("Game SAVED, type: %ls, slot: %ld, time: %02ld:%02ld:%02ld (tick %lld), save reason: %ls"), 
				CEnum::ToString( GetRequestedSaveType() ).AsChar(),
				info.m_slotIndex,
				currentTime.GetHour(), currentTime.GetMinute(), currentTime.GetSecond(),
				GEngine->GetCurrentEngineTick(),
				m_debugSaveReason.AsChar() ),
				Color::GREEN, CGameSessionManager::MSG_TIME );
		#endif

		SGameSessionManager::GetInstance().CreateTimedLock();

		m_saver.Reset( saver );
		m_requestedSaveType = SGT_None; 
		m_requestedSaveSlot = -1;
	END_TIMER_BLOCK( all );

	#ifndef NO_SAVE_VERBOSITY
		extern class CSaveClassStatsCollector GSaveStatsCollector;
		GSaveStatsCollector.Dump();
		GSaveStatsCollector.Reset();
	#endif

	return true;
}

RED_INLINE void CGameSaver::CancelSaving( CWorldStorage* ws, const Char* file )
{
	m_requestedSaveType = SGT_None; 
	m_requestedSaveSlot = -1;
	m_debugSaveReason = String::EMPTY;
	m_waitingForScreenshot = false;

	if ( ws )
	{
		ws->CancelCaptureState();
		GUserProfileManager->CancelGameSaving();
	}

	if ( file )
	{
		RED_LOG( Save, TXT("Cannot save here (file: %ls)"), file );
	}

	m_busy.SetValue( false );
}

void CGameSaver::FinalizeSaveGame( Bool sync )
{
	GUserProfileManager->OnScreenshotDone( Uint32( m_realWriteSize ) );
	m_realWriteSize = 0;
	m_waitingForScreenshot = false;
	if( m_saver )
	{
		if ( sync )
		{
			m_saver->Finalize();
			GUserProfileManager->FinalizeGameSaving( m_saver.Get() );
			m_saver.Reset();
		}
		else
		{
			CFinalizeSaveGameTask* task = new ( CTask::Root ) CFinalizeSaveGameTask( std::move( m_saver ) );

			#ifdef RED_PLATFORM_ORBIS
				ETaskSchedulerGroup group = TSG_Service;
			#else
				ETaskSchedulerGroup group = TSG_Normal;
			#endif

			GTaskManager->Issue( *task, TSP_Critical, group );
		
			// Fire and forget (The ref count will ensure it's not deleted until after it's been run)
			task->Release();
		}
	}
}

ESessionRestoreResult CGameSaver::LoadGame( const SSavegameInfo& info )
{
	ELoaderCreationResult res;
	IGameLoader* gameLoader = SGameSaveManager::GetInstance().CreateLoader( info, res );
	if ( nullptr == gameLoader )
	{
		RED_LOG( Session, TXT("Unable to restore session data. Not loading.") );
		return ( res == LOADER_WrongVersion ) ? RESTORE_WrongGameVersion : RESTORE_DataCorrupted;
	}

	// Restore session
	return SGameSessionManager::GetInstance().RestoreSession( gameLoader, info.IsSuppressVideo() );
}

void CGameSaver::GrabScreenshot()
{
	m_realWriteSize = 0;
	m_screenshotDone.SetValue( false );

	SScreenshotParameters parameters;
	{
		parameters.m_buffer = ( Uint32* ) m_screenShotBuffer;
		parameters.m_bufferSize = CGameSaver::SCREENSHOT_BUFFER_SIZE;
		parameters.m_fileName = String::EMPTY; // don't write to file
		parameters.m_flags =  SCF_SaveToBuffer | ( GCommonGame->IsNewGamePlus() ? SCF_AddNewGamePlusWatermark : 0 );
		parameters.m_height = SAVE_SCREENSHOT_HEIGHT;
		parameters.m_width = SAVE_SCREENSHOT_WIDTH;
		parameters.m_saveFormat = SF_PNG;
		parameters.m_superSamplingSize = 1;
		parameters.m_bufferSizeWritten = &m_realWriteSize;
		parameters.m_completionFlag = &m_screenshotDone;
	}

	ASSERT( m_screenShotBuffer == GUserProfileManager->GetScreenshotBuffer() );

	( new CRenderCommand_TakeScreenshot( parameters ) )->Commit();
}

void CGameSaver::GrabScreenshotNow()
{
	GrabScreenshot();
	GRender->Flush();
}

/* static */ Bool CGameSaver::ShouldShowTheCompatibilityWarning()
{
	return Config::cvShowSaveCompatWarning.Get();
}

void CGameSaver::RequestSaveGame( ESaveGameType type, Int16 slot, const String& reason, const String& customSaveName /*= String::EMPTY*/ )
{
	// Do not start saving until we *REALLY* finished previous save op
	if( m_busy.CompareExchange( true, false ) == true )
	{
		RED_LOG( Save, TXT( "Save already in process, backing out" ) );
		return;
	}

	// Check for any active save locks
	if( SGameSessionManager::GetInstance().AreGameSavesLocked( type ) )
	{
		SGameSessionManager::GetInstance().AddFailedSaveDebugMessage( type, TXT("Saves were locked."), m_debugSaveReason.AsChar() );
		CancelSaving( nullptr, nullptr );
		return;
	}

	CWorld* world = GGame->GetActiveWorld();
	if ( nullptr == world )
	{
		SGameSessionManager::GetInstance().AddFailedSaveDebugMessage( type, TXT("No active world."), m_debugSaveReason.AsChar() );
		CancelSaving( nullptr, nullptr );
		return;
	}

	if ( LOAD_NotInitialized != GUserProfileManager->GetLoadGameProgress() )
	{
		SGameSessionManager::GetInstance().AddFailedSaveDebugMessage( m_requestedSaveType, TXT("Cannot save the game while loading."), m_debugSaveReason.AsChar() );
		CancelSaving( nullptr, nullptr );
		return;
	}

	m_requestedSaveType = type;
	m_requestedSaveSlot = slot;
	SSavegameInfo& info = m_currentSaveInfo;

	info.m_slotType = m_requestedSaveType;
	Red::System::Clock::GetInstance().GetLocalTime( info.m_timeStamp );
	info.m_displayNameIndex = GCommonGame->GetSystem< CJournalManager >()->GetDisplayNameIndexForSavedGame();
	info.m_slotIndex = m_requestedSaveSlot;

	if ( false == customSaveName.Empty() )
	{
		info.m_filename = customSaveName;
		info.m_customFilename = true;
	}

	if ( type == SGT_AutoSave || type == SGT_CheckPoint )
	{
		m_lastAutoSaveAttemptTime = EngineTime::GetNow(); 
	}

	if ( SAVE_Error == GUserProfileManager->InitGameSaving( info ) )
	{
		SGameSessionManager::GetInstance().AddFailedSaveDebugMessage( type, TXT("Profile API reurned an error."), m_debugSaveReason.AsChar() );
		CancelSaving( nullptr, info.GetFileName().AsChar() );
		GUserProfileManager->CancelGameSaving();
		return;
	}

	GrabScreenshot();

	#ifndef FINAL
		m_debugSaveReason = reason;
	#else
		RED_UNUSED( reason );
	#endif
}

const Uint8* CGameSaver::GetDefaultScreenshotData() const
{
	return GCommonGame->IsNewGamePlus() ? NGPLUS_SCREENSHOT_DATA : DEFAULT_SCREENSHOT_DATA;
}

Uint32 CGameSaver::GetDefaultScreenshotDataSize() const
{
	return GCommonGame->IsNewGamePlus() ? NGPLUS_SCREENSHOT_SIZE : DEFAULT_SCREENSHOT_SIZE;
}

void CGameSaver::Update()
{
	if ( IsSaveInProgress() )
	{
		ESaveGameResult res = GUserProfileManager->GetSaveGameProgress();
		if ( SAVE_Initializing == res )
		{
			RED_LOG( Save, TXT("CGameSaver::Update() - waiting... (res == %ls)"), CEnum::ToString( res ).AsChar() );
			return; // just wait
		}

		SSavegameInfo& info = m_currentSaveInfo;
		CWorld* world = GGame->GetActiveWorld();
		if ( nullptr == world )
		{
			RED_LOG( Save, TXT("CGameSaver::Update() - NO WORLD! (res == %ls)"), CEnum::ToString( res ).AsChar()  );
			SGameSessionManager::GetInstance().AddFailedSaveDebugMessage( m_requestedSaveType, TXT("No active world."), m_debugSaveReason.AsChar() );
			CancelSaving( nullptr, info.GetFileName().AsChar() );
			GUserProfileManager->CancelGameSaving();
			GCommonGame->CallEvent( CNAME( OnSaveCompleted ), info.m_slotType, false );
			return;
		}

		if( SAVE_Saving != res && SGameSessionManager::GetInstance().AreGameSavesLocked( m_requestedSaveType ) )
		{
			RED_LOG( Save, TXT("CGameSaver::Update() - LOCK! (res == %ls)"), CEnum::ToString( res ).AsChar()  );
			SGameSessionManager::GetInstance().AddFailedSaveDebugMessage( m_requestedSaveType, TXT("Saves were locked."), m_debugSaveReason.AsChar() );
			CancelSaving( nullptr, info.GetFileName().AsChar() );
			GUserProfileManager->CancelGameSaving();
			GCommonGame->CallEvent( CNAME( OnSaveCompleted ), info.m_slotType, false );
			return;
		}

		CWorldStorage* activeWorldStorage = GGame->GetUniverseStorage()->GetWorldStorage( world );
		ASSERT( activeWorldStorage );

		if ( SAVE_Error == res )
		{
			RED_LOG( Save, TXT("CGameSaver::Update() - profile manager returned error, aborting (res == %ls)"), CEnum::ToString( res ).AsChar()  );
			SGameSessionManager::GetInstance().AddFailedSaveDebugMessage( m_requestedSaveType, TXT("Profile API reurned an error."), m_debugSaveReason.AsChar() );
			CancelSaving( activeWorldStorage, info.GetFileName().AsChar() );
			GCommonGame->CallEvent( CNAME( OnSaveCompleted ), info.m_slotType, false );
			return;
		}

		if ( SAVE_ReadyToSave == res )
		{
			RED_LOG( Save, TXT("CGameSaver::Update() - do save! (res == %ls)"), CEnum::ToString( res ).AsChar()  );
			m_waitingForScreenshot = DoSaveGame( world, activeWorldStorage );
		}

		if ( m_waitingForScreenshot && IsScreenshotBufferReady() )
		{
			RED_LOG( Save, TXT("CGameSaver::Update() - finalize (res == %ls)"), CEnum::ToString( res ).AsChar()  );
			FinalizeSaveGame( false ); // async
		}
	}
}

void CGameSaver::SaveGameSync( ESaveGameType type, Int16 slot, const String& reason, const String& customSaveName /*= String::EMPTY */ )
{
	ASSERT( SIsMainThread() );
	TIMER_BLOCK( syncSaveAll )

	if ( IsSaveInProgress() )
	{
		return; // another save is in progress, back out or it will never complete
	}

	RequestSaveGame( type, slot, reason, customSaveName ); 

	ESaveGameResult res = GUserProfileManager->GetSaveGameProgress();
	if ( res == SAVE_NotInitialized )
	{
		CancelSaving( nullptr, nullptr );
		return;
	}

	// update the profile in sync, to finish the init
	while ( res == SAVE_Initializing )
	{
		Red::Threads::YieldCurrentThread();
		GUserProfileManager->Update();
		res = GUserProfileManager->GetSaveGameProgress();
	}
	
	SSavegameInfo& info = m_currentSaveInfo;
	CWorld* world = GGame->GetActiveWorld();
	if ( nullptr == world )
	{
		CancelSaving( nullptr, info.GetFileName().AsChar() );
		GUserProfileManager->CancelGameSaving();
		return;
	}

	CWorldStorage* activeWorldStorage = GGame->GetUniverseStorage()->GetWorldStorage( world );
	ASSERT( activeWorldStorage );

	if ( res == SAVE_Error )
	{
		CancelSaving( activeWorldStorage, info.GetFileName().AsChar() );
		return;
	}

	if ( res == SAVE_ReadyToSave )
	{
		m_waitingForScreenshot = DoSaveGame( world, activeWorldStorage );
		if ( m_waitingForScreenshot )
		{
			GRender->Flush();
			FinalizeSaveGame( true ); // sync
		}
	}

	END_TIMER_BLOCK( syncSaveAll )
}

void CGameSaver::OnWorldStart()
{
	m_lastSaveType = SGT_None;
	m_lastSaveTime = EngineTime::GetNow();
	m_lastAutoSaveAttemptTime = EngineTime::GetNow();
}

void CGameSaver::OnSaveCompleted( Bool success )
{
	ASSERT( SIsMainThread() );
	RED_FATAL_ASSERT( m_busy.GetValue(), "Called OnSaveCompleted() when not saving. DEBUG NOW." );

	if ( m_lastSaveType == SGT_Manual )
	{
		Config::cvShowSaveCompatWarning.Set( false );
		SConfig::GetInstance().Save();
	}

	m_busy.SetValue( false );
}


#ifndef NO_SAVE_VERBOSITY
	void CGameSaver::DebugDumpLevelStats() const
	{
		const Uint32 MAX_GROUPS = 4096;
		CLayerGroup* groups[ MAX_GROUPS ];
		CWorld* world = GGame->GetActiveWorld();
		if ( nullptr == world )
		{
			return;
		}

		Uint32 numGroups = world->GetWorldLayers()->GetLayerGroupsForSave( groups, MAX_GROUPS );

		THashMap< CName, Uint32 > classes;
		for ( Uint32 g = 0; g < numGroups; ++g )
		{
			CLayerGroup* lg = groups[ g ];
			for ( auto layer = lg->GetLayers().Begin(); layer != lg->GetLayers().End(); ++layer )
			{
				CLayer* l = ( *layer )->GetLayer();
				if ( nullptr == l || false == l->IsAttached() )
				{
					continue;
				}

				for ( auto entity = l->GetEntities().Begin(); entity != l->GetEntities().End(); ++entity )
				{
					CPeristentEntity* e = Cast< CPeristentEntity > ( *entity );
					if ( nullptr == e )
					{
						continue;
					}

					if ( e->ShouldSave() )
					{
						classes[ e->GetClass()->GetName() ] += 1;
					}

					for ( ComponentIterator< CComponent > component( e ); component; ++component )
					{
						CComponent* c = *component;
						if ( c->ShouldSave() )
						{
							classes[ c->GetClass()->GetName() ] += 1;
						}
					}
				}
			}
		}

		for ( auto klass = classes.Begin(); klass != classes.End(); ++klass )
		{
			RED_LOG( Save, TXT("Class %s: %ld"), klass->m_first.AsChar(), klass->m_second );
		}
	}

#endif

CFinalizeSaveGameTask::CFinalizeSaveGameTask( Red::TUniquePtr< IGameSaver >&& igs )
	: CTask( 0 )
	, m_saver( std::move( igs ) )
{
}

CFinalizeSaveGameTask::~CFinalizeSaveGameTask()  
{

}

void CFinalizeSaveGameTask::Run()
{
	PC_SCOPE_PIX( CFinalizeSaveGameTask );

	// Finish writing all the data. ( SAVE_END_FILE_MAGIC etc. )
	m_saver->Finalize();

	// Let UserProfileManager decide what to do with the data.
	// Whether to just flush the data or pass it somewhere else, in ex. pass to XBox API.
	// UserProfileManager also takes control over the scope of IGameSaver.
	GUserProfileManager->FinalizeGameSaving( m_saver.Get() );
}
