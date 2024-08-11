#include "build.h"

#include "r4Telemetry.h"

#if !defined( NO_TELEMETRY )

#	if defined ( RED_CONFIGURATION_DEBUG )
#		pragma comment (lib, "RedTelemetryLib_d.lib")
#	elif defined ( RED_CONFIGURATION_NOPTS )
#		pragma comment (lib, "RedTelemetryLib_noopt.lib")
#	else
#		pragma comment (lib, "RedTelemetryLib.lib")
#	endif

#if defined( RED_PLATFORM_DURANGO )
#pragma comment (lib, "EtwPlus.lib")
#endif

#include "r4Player.h"
#include "r4JournalManager.h"
#include "../../common/redThreads/redThreadsThread.h"
#include "../../common/core/scriptStackFrame.h"
#include "../../common/core/configVar.h"
#include "../../common/core/configVarLegacyWrapper.h"
#include "../../common/core/gameSave.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/gameResource.h"
#include "../../common/engine/userProfile.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../../common/engine/redGuiMenu.h"
#include "../../common/engine/debugWindowsManager.h"
#include "../../common/engine/debugWindowTelemetryTags.h"

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

const Event sEventsTranslationArray[] =
{
#	define EVENT( x, cat, name ) { TXT( cat ), TXT( name ) },
#	include "r4Events.enum"
#	undef EVENT
};

const Char* sStatsTranslationArray[] =
{
#	define STAT( x, name ) { TXT( name ) },
#	include "r4CommonStats.enum"
#	undef STAT
};

IMPLEMENT_ENGINE_CLASS( CR4Telemetry );

RED_DEFINE_STATIC_NAME( TelemetrySessionId )
		
//////////////////////////////////////////////////////////////////////////
//! This class is a singleton, so its creation is thread-safe
CR4Telemetry::CR4Telemetry() : m_gameStarted( false )
{
	m_ddiStatCountMethod[TXT("f_edi")] = ECM_One;
	m_ddiStatCountMethod[TXT("ha_nlg")] = ECM_Max;
}

//////////////////////////////////////////////////////////////////////////
CR4Telemetry::~CR4Telemetry()
{}

void CR4Telemetry::Initialize()
{
	m_userEventHandle = GUserProfileManager->RegisterListener( &CR4Telemetry::OnUserEvent, this );

	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );

#ifndef RED_FINAL_BUILD	
	m_telemetryServices[ TXT("telemetry") ] = new CRedTelemetryService(TXT("telemetry"), Telemetry::BT_RED_TEL_API);
#endif //! RED_FINAL_BUILD
	m_telemetryServices[ CRedTelemetryServiceDDI::s_serviceName ] = new CRedTelemetryService(CRedTelemetryServiceDDI::s_serviceName, Telemetry::BT_NONE);
#if defined( RED_PLATFORM_DURANGO)
	m_telemetryServices[ TXT("xbox_data_platform") ] = new CRedTelemetryService( TXT("xbox_data_platform"), Telemetry::BT_XBOX_DATA_PLATFORM_API );
#endif
}

void CR4Telemetry::Shutdown()
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );

	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		delete iter->m_second;
	}
	m_telemetryServices.Clear();

	GUserProfileManager->UnregisterListener( m_userEventHandle );
}


//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::InitializeDebugWindows()
{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		
	RedGui::CRedGuiMenu* miscMenu = GDebugWin::GetInstance().GetMenu( TXT("Misc") );

	if( miscMenu != NULL )
	{
		DebugWindows::CDebugWindoTelemetryTags* debugWindoTelemetryTags = new DebugWindows::CDebugWindoTelemetryTags();

		GDebugWin::GetInstance().RegisterWindow( debugWindoTelemetryTags, DebugWindows::DW_TelemetryTags );	

		miscMenu->AppendItem( TXT("Telemetry Tags"), debugWindoTelemetryTags );
	}	

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::Log( ER4TelemetryEvents gameEvent )
{
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		RED_ASSERT( iter->m_second->GetInterface() != NULL, TXT( "Telemetry not created yet" ) );
		if( iter->m_second->GetInterface() )
		{		
			GetPlayerPosition();
			GetWorldId();
	#if !defined( NO_TELEMETRY_DEBUG )
			GetDebugStatistics();
	#endif

			String categoryStr, eventNameStr;
			EventCategoryToString( gameEvent, categoryStr, eventNameStr );
			iter->m_second->GetInterface()->Log( eventNameStr, categoryStr );
		}
	}


}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::LogL( ER4TelemetryEvents gameEvent, const String& label )
{
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		RED_ASSERT( iter->m_second->GetInterface() != NULL, TXT( "Telemetry not created yet" ) );

		if( iter->m_second->GetInterface() )
		{
			GetPlayerPosition();
			GetWorldId();
	#if !defined( NO_TELEMETRY_DEBUG )
			GetDebugStatistics();
	#endif

			String categoryStr, eventNameStr;
			EventCategoryToString( gameEvent, categoryStr, eventNameStr );
			iter->m_second->GetInterface()->LogL( eventNameStr, categoryStr, label );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::LogV( ER4TelemetryEvents gameEvent, Int32 value )
{
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		RED_ASSERT( iter->m_second->GetInterface() != NULL, TXT( "Telemetry not created yet" ) );

		if( iter->m_second->GetInterface() )
		{
			GetPlayerPosition();
			GetWorldId();
#if !defined( NO_TELEMETRY_DEBUG )
			GetDebugStatistics();
#endif

			String categoryStr, eventNameStr;
			EventCategoryToString( gameEvent, categoryStr, eventNameStr );
			iter->m_second->GetInterface()->LogV( eventNameStr, categoryStr, value );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::LogV( ER4TelemetryEvents gameEvent, const String& value )
{
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		RED_ASSERT( iter->m_second->GetInterface() != NULL, TXT( "Telemetry not created yet" ) );

		if( iter->m_second->GetInterface() )
		{
			GetPlayerPosition();
			GetWorldId();
#if !defined( NO_TELEMETRY_DEBUG )
			GetDebugStatistics();
#endif

			String categoryStr, eventNameStr;
			EventCategoryToString( gameEvent, categoryStr, eventNameStr );
			iter->m_second->GetInterface()->LogV( eventNameStr, categoryStr, value );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::LogVL( ER4TelemetryEvents gameEvent, Int32 value, const String& label )
{
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		RED_ASSERT( iter->m_second->GetInterface() != NULL, TXT( "Telemetry not created yet" ) );

		if( iter->m_second->GetInterface() )
		{
			GetPlayerPosition();
			GetWorldId();
	#if !defined( NO_TELEMETRY_DEBUG )
			GetDebugStatistics();
	#endif

			String categoryStr, eventNameStr;
			EventCategoryToString( gameEvent, categoryStr, eventNameStr );
			iter->m_second->GetInterface()->LogVL( eventNameStr, categoryStr, value, label );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::LogVL( ER4TelemetryEvents gameEvent, const String& value, const String& label )
{
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		RED_ASSERT( iter->m_second->GetInterface() != NULL, TXT( "Telemetry not created yet" ) );

		if( iter->m_second->GetInterface() )
		{
			GetPlayerPosition();
			GetWorldId();
	#if !defined( NO_TELEMETRY_DEBUG )
			GetDebugStatistics();
	#endif

			String categoryStr, eventNameStr;
			EventCategoryToString( gameEvent, categoryStr, eventNameStr );
			iter->m_second->GetInterface()->LogVL( eventNameStr, categoryStr, value, label );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::LogException( const String& exception )
{
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		if( iter->m_second->GetInterface() )
		{
			iter->m_second->GetInterface()->LogEx( exception );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::SetStatValue( ER4CommonStats stat, Int32 value )
{
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		RED_ASSERT( iter->m_second->GetInterface() != NULL, TXT( "Telemetry not created yet" ) );

		if( iter->m_second->GetInterface() )
		{
			String statName;
			CommonStatToString( stat, statName );
			iter->m_second->GetInterface()->SetCommonStatValue( statName, value );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::SetStatValue( ER4CommonStats stat, Float value )
{
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		RED_ASSERT( iter->m_second->GetInterface() != NULL, TXT( "Telemetry not created yet" ) );

		if( iter->m_second->GetInterface() )
		{
			String statName;
			CommonStatToString( stat, statName );
			iter->m_second->GetInterface()->SetCommonStatValue( statName, value );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::SetStatValue( ER4CommonStats stat, const String& value )
{
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		RED_ASSERT( iter->m_second->GetInterface() != NULL, TXT( "Telemetry not created yet" ) );

		if( iter->m_second->GetInterface() )
		{
			String statName;
			CommonStatToString( stat, statName );
			iter->m_second->GetInterface()->SetCommonStatValue( statName, value );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
#if !defined( NO_TELEMETRY_DEBUG )

void CR4Telemetry::GetDebugStatistics()
{
	SetStatValue( CS_MEMORY, static_cast< Float >( Memory::GetTotalBytesAllocated() ) );
	SetStatValue( CS_FPS, GEngine->GetLastTickRate() );
}

#endif

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::GetPlayerPosition()
{
	CEntity* playerEntity = GGame->GetPlayerEntity();
	if( playerEntity )
	{
		CR4Player* player = SafeCast< CR4Player >( playerEntity );
		RED_ASSERT( player != nullptr, TXT( "Cannot cast player class" ) );

		if( player )
		{
			const Vector position = player->GetWorldPosition();

			SetStatValue( CS_POSITION_X, position.X );
			SetStatValue( CS_POSITION_Y, position.Y );
			SetStatValue( CS_POSITION_Z, position.Z );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::GetWorldId()
{
	if( GGame && GGame->GetActiveWorld() )
	{
		String worldName = GGame->GetActiveWorld()->GetFriendlyName();
		SetStatValue( CS_WORLD_ID, static_cast< Int32 >( ::GetHash( worldName ) ) );
	}
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::EventCategoryToString( ER4TelemetryEvents category, String& categoryStr, String& nameStr ) const
{
	const Event& eventDef = sEventsTranslationArray[ static_cast< Int32 >( category ) ];
	categoryStr = eventDef.category;
	nameStr = eventDef.name;
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::CommonStatToString( ER4CommonStats stat, String& statStr ) const
{
	statStr = sStatsTranslationArray[ static_cast< Int32 >( stat ) ];
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::OnGameStart( const CGameInfo& gameInfo )
{
	m_gameStarted = true;
	if( gameInfo.m_isChangingWorldsInGame == false )
	{
		m_loadedSesssionId.Clear();
		if( gameInfo.m_gameLoadStream != NULL )
		{		
			OnLoadGame( gameInfo.m_gameLoadStream );
		}
				
		CGameResource* gameResource = GGame->GetGameResource();
		if ( gameResource != NULL )
		{
			CDiskFile* file = gameResource->GetFile();
			if( file != NULL )
			{
				AddSessionTag( file->GetFileName() );
			}				
		}	
		
		Int32 difficulty = 0;
		
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("ScriptConf"), TXT("Difficulty"), difficulty );

		SetStatValue( CS_DIFFICULTY_LVL, difficulty );


		THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
		for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
		{
			iter->m_second->StartSession( m_playerId, m_loadedSesssionId );
		}

		CWitcherJournalManager* manager = GCommonGame->GetSystem< CWitcherJournalManager >();
		if( manager )
		{
			this->SetStatValue( CS_GAME_PROGRESS, (float)manager->GetQuestProgress() );
			this->Log( TE_SYS_GAME_PROGRESS );
		}
	}	
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::OnGameEnd( const CGameInfo& gameInfo )
{
	m_gameStarted = false;
	if( gameInfo.m_isChangingWorldsInGame == false )
	{
		CGameResource* gameResource = GGame->GetGameResource();
		if ( gameResource != NULL )
		{
			CDiskFile* file = gameResource->GetFile();
			if( file != NULL )
			{
				RemoveSessionTag( file->GetFileName() );
			}				
		}			
		THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
		for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
		{
			iter->m_second->StopSession();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
Bool CR4Telemetry::OnSaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )
	String sesssionIdToSave;
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		if( iter->m_second->GetInterface() )
		{
			sesssionIdToSave = iter->m_second->GetInterface()->GetSessionId( Telemetry::BT_RED_TEL_API );
			break;
		}
	}
	
	CGameSaverBlock block( saver, CNAME(TelemetrySessionId) );
	saver->WriteValue< String >( CNAME(TelemetrySessionId), sesssionIdToSave );

	Log( TE_SYS_GAME_SAVED );

	END_TIMER_BLOCK( time )
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CR4Telemetry::OnLoadGame( IGameLoader* loader )
{
	if( loader->GetSaveVersion() >= SAVE_VERSION_TELEMETRY_NEW_SAVE )
	{
		CGameSaverBlock block( loader, CNAME(TelemetrySessionId) );

		loader->ReadValue< String >( CNAME(TelemetrySessionId), m_loadedSesssionId);

		Log( TE_SYS_GAME_LOADED );
	}
}

Bool CR4Telemetry::RemoveSessionTag( const String& tag )
{
	Bool removedSuccess = false;
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		if( iter->m_second->GetInterface() )
		{
			removedSuccess |= iter->m_second->GetInterface()->RemoveSessionTag( tag );
		}
	}
	return removedSuccess;
}

void CR4Telemetry::AddSessionTag( const String& tag )
{
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		if( iter->m_second->GetInterface() )
		{
			iter->m_second->GetInterface()->AddSessionTag( tag );
		}
	}
}

void CR4Telemetry::RemoveAllSessionTags()
{
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		if( iter->m_second->GetInterface() )
		{
			iter->m_second->GetInterface()->RemoveAllSessionTags();
		}
	}
}

void CR4Telemetry::OnUserEvent( const EUserEvent& event )
{	
	if( event == EUserEvent::UE_SignedIn )
	{
		m_playerId = GUserProfileManager->GetPlatformUserIdAsString();
#if defined( RED_PLATFORM_DURANGO)
		if( m_gameStarted )
		{
			//! StartSession is called when player sing in
			//! This handle situation when player was not signed in during game start or switch account
			m_telemetryServices[ TXT("xbox_data_platform") ]->StartSession( m_playerId, m_loadedSesssionId ); //! for now user management is supported only by XDP
		}
#endif
	}
	else if( event == EUserEvent::UE_SignedOut )
	{
		m_playerId.Clear();
#if defined( RED_PLATFORM_DURANGO)
		if( m_gameStarted )
		{		
			m_telemetryServices[ TXT("xbox_data_platform") ]->StopSession(); //! for now user management is supported only by XDP
		}
#endif
	}
}

void CR4Telemetry::LoadConfigs()
{
#ifndef RED_FINAL_BUILD
	m_telemetryServices[ TXT("telemetry") ]->LoadConfig( TXT( "telemetry_service_config" ) );
#endif //! RED_FINAL_BUILD
	m_telemetryServices[ CRedTelemetryServiceDDI::s_serviceName ]->LoadConfig( TXT( "telemetry_service_config" ) );
	m_telemetryServices[ CRedTelemetryServiceDDI::s_serviceName ]->SetImmediatePost( true );
	CRedTelemetryServiceDDI* redTelemetryServiceDDI = static_cast<CRedTelemetryServiceDDI*>( m_telemetryServices[ CRedTelemetryServiceDDI::s_serviceName ]->GetInterface() );
	redTelemetryServiceDDI->SetDelegate( this );
#if defined( RED_PLATFORM_DURANGO)
	m_telemetryServices[ TXT("xbox_data_platform") ]->LoadConfig( TXT( "telemetry_service_config" ) );
	m_telemetryServices[ TXT("xbox_data_platform") ]->SetImmediatePost( true );
#endif
}

Bool CR4Telemetry::StartCollecting()
{
	Bool startedSuccessful = false;
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		if( iter->m_second->GetInterface() )
		{
			startedSuccessful |= iter->m_second->GetInterface()->StartCollecting();
		}
	}
	return startedSuccessful;
}

Bool CR4Telemetry::StopCollecting()
{
	Bool stopedSuccessful = false;
	THashMap< String, CRedTelemetryService* >::iterator end = m_telemetryServices.End();
	for( THashMap< String, CRedTelemetryService* >::iterator iter = m_telemetryServices.Begin(); iter != end; ++iter )
	{
		if( iter->m_second->GetInterface() )
		{
			stopedSuccessful |= iter->m_second->GetInterface()->StopCollecting();
		}
	}
	return stopedSuccessful;
}

//! IRedTelemetryServiceDDIDelegate
void CR4Telemetry::LogInt32( const String& name, Int32 value )
{
	if( GUserProfileManager )
	{		
		Int32 oldValue = 0;
		ECountMethod countMethod = ECM_Inc;
		ECountMethod *countMethodPtr  = m_ddiStatCountMethod.FindPtr( name );
		String finalName = name;
		if( countMethodPtr )
		{
			countMethod = *countMethodPtr;
		}
		switch ( countMethod )
		{
		case CR4Telemetry::ECM_One:		

			if( finalName == TXT("f_edi") ) //! f_edi stat need to have new name
				finalName = TXT("f_edi_real");

			GUserProfileManager->IncrementStat( finalName, 1 );
			RED_LOG( RED_LOG_CHANNEL( DDI_TelemetryStats ), TXT( "DDI LogV: %ls increment by %i" ), finalName.AsChar(), 1 );
			break;
		case CR4Telemetry::ECM_Max:
			finalName += TXT("_max");
			GUserProfileManager->GetStat( finalName, oldValue );
			if( oldValue < value )
			{
				GUserProfileManager->SetStat( finalName, value );
				RED_LOG( RED_LOG_CHANNEL( DDI_TelemetryStats ), TXT( "DDI LogV: %ls Set Max %i" ), finalName.AsChar(), value );
			}
			break;
		case CR4Telemetry::ECM_Min:
			finalName += TXT("_min");
			GUserProfileManager->GetStat( finalName, oldValue );
			if( oldValue > value )
			{
				GUserProfileManager->SetStat( finalName, value );
				RED_LOG( RED_LOG_CHANNEL( DDI_TelemetryStats ), TXT( "DDI LogV: %ls Set Min %i" ), finalName.AsChar(), value );
			}			
			break;
		default: //! ECM_Inc
			GUserProfileManager->IncrementStat( finalName, value );
			RED_LOG( RED_LOG_CHANNEL( DDI_TelemetryStats ), TXT( "DDI LogV: %ls increment by %i" ), finalName.AsChar(), value );
			break;
		}		
	}
}

void CR4Telemetry::LogFloat( const String& name, Float value )
{
	if( GUserProfileManager )
	{
		Float oldValue = 0;
		ECountMethod countMethod = ECM_Inc;
		ECountMethod *countMethodPtr  = m_ddiStatCountMethod.FindPtr( name );
		String finalName = name;
		if( countMethodPtr )
		{
			countMethod = *countMethodPtr;
		}
		switch ( countMethod )
		{
		case CR4Telemetry::ECM_One:
			GUserProfileManager->IncrementStat( finalName, 1 );
			RED_LOG( RED_LOG_CHANNEL( DDI_TelemetryStats ), TXT( "DDI LogV: %ls increment by %f" ), finalName.AsChar(), 1 );
			break;
		case CR4Telemetry::ECM_Max:
			finalName += TXT("_max");
			GUserProfileManager->GetStat( finalName, oldValue );
			if( oldValue < value )
			{
				GUserProfileManager->SetStat( finalName, value );
				RED_LOG( RED_LOG_CHANNEL( DDI_TelemetryStats ), TXT( "DDI LogV: %ls Set Max %f" ), finalName.AsChar(), value );
			}
			break;
		case CR4Telemetry::ECM_Min:
			finalName += TXT("_min");
			GUserProfileManager->GetStat( finalName, oldValue );
			if( oldValue > value )
			{
				GUserProfileManager->SetStat( finalName, value );
				RED_LOG( RED_LOG_CHANNEL( DDI_TelemetryStats ), TXT( "DDI LogV: %ls Set Min %f" ), finalName.AsChar(), value );
			}			
			break;
		default: //! ECM_Inc
			GUserProfileManager->IncrementStat( finalName, value );
			RED_LOG( RED_LOG_CHANNEL( DDI_TelemetryStats ), TXT( "DDI LogV: %ls increment by %f" ), finalName.AsChar(), value );
			break;
		}
	}
}

#endif
